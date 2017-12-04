/*              S H A R E D   M E M O R Y   D R I V E R
                =======================================

                  by Jerzy.Borkowski@obs.unige.ch

09-Mar-98 : initial version 1.0 released
23-Mar-98 : shared_malloc now accepts new handle as an argument
23-Mar-98 : shmem://0, shmem://1, etc changed to shmem://h0, etc due to bug
            in url parser.
10-Apr-98 : code cleanup
13-May-99 : delayed initialization added, global table deleted on exit when
            no shmem segments remain, and last process terminates
*/

#ifdef HAVE_SHMEM_SERVICES
#include "fitsio2.h"                         /* drvrsmem.h is included by it */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(unix) || defined(__unix__)  || defined(__unix) || defined(HAVE_UNISTD_H)
#include <unistd.h> 
#endif


static int shared_kbase = 0;                    /* base for shared memory handles */
static int shared_maxseg = 0;                   /* max number of shared memory blocks */
static int shared_range = 0;                    /* max number of tried entries */
static int shared_fd = SHARED_INVALID;          /* handle of global access lock file */
static int shared_gt_h = SHARED_INVALID;        /* handle of global table segment */
static SHARED_LTAB *shared_lt = NULL;           /* local table pointer */
static SHARED_GTAB *shared_gt = NULL;           /* global table pointer */
static int shared_create_mode = 0666;           /* permission flags for created objects */
static int shared_debug = 1;                    /* simple debugging tool, set to 0 to disable messages */
static int shared_init_called = 0;              /* flag whether shared_init() has been called, used for delayed init */

                /* static support routines prototypes */

static  int shared_clear_entry(int idx);        /* unconditionally clear entry */
static  int shared_destroy_entry(int idx);      /* unconditionally destroy sema & shseg and clear entry */
static  int shared_mux(int idx, int mode);      /* obtain exclusive access to specified segment */
static  int shared_demux(int idx, int mode);    /* free exclusive access to specified segment */

static  int shared_process_count(int sem);      /* valid only for time of invocation */
static  int shared_delta_process(int sem, int delta); /* change number of processes hanging on segment */
static  int shared_attach_process(int sem);
static  int shared_detach_process(int sem);
static  int shared_get_free_entry(int newhandle);       /* get free entry in shared_key, or -1, entry is set rw locked */
static  int shared_get_hash(long size, int idx);/* return hash value for malloc */
static  long shared_adjust_size(long size);     /* size must be >= 0 !!! */
static  int shared_check_locked_index(int idx); /* verify that given idx is valid */ 
static  int shared_map(int idx);                /* map all tables for given idx, check for validity */
static  int shared_validate(int idx, int mode); /* use intrnally inside crit.sect !!! */

                /* support routines - initialization */


static  int shared_clear_entry(int idx)         /* unconditionally clear entry */
 { if ((idx < 0) || (idx >= shared_maxseg)) return(SHARED_BADARG);
   shared_gt[idx].key = SHARED_INVALID;         /* clear entries in global table */
   shared_gt[idx].handle = SHARED_INVALID;
   shared_gt[idx].sem = SHARED_INVALID;
   shared_gt[idx].semkey = SHARED_INVALID;
   shared_gt[idx].nprocdebug = 0;
   shared_gt[idx].size = 0;
   shared_gt[idx].attr = 0;

   return(SHARED_OK);
 }

static  int shared_destroy_entry(int idx)       /* unconditionally destroy sema & shseg and clear entry */
 { int r, r2;
   union semun filler;

   if ((idx < 0) || (idx >= shared_maxseg)) return(SHARED_BADARG);
   r2 = r = SHARED_OK;
   filler.val = 0;                              /* this is to make cc happy (warning otherwise) */
   if (SHARED_INVALID != shared_gt[idx].sem)  r = semctl(shared_gt[idx].sem, 0, IPC_RMID, filler); /* destroy semaphore */
   if (SHARED_INVALID != shared_gt[idx].handle) r2 = shmctl(shared_gt[idx].handle, IPC_RMID, 0); /* destroy shared memory segment */
   if (SHARED_OK == r) r = r2;                  /* accumulate error code in r, free r2 */
   r2 = shared_clear_entry(idx);
   return((SHARED_OK == r) ? r2 : r);
 }

void    shared_cleanup(void)                    /* this must (should) be called during exit/abort */
 { int          i, j, r, oktodelete, filelocked, segmentspresent;
   flock_t      flk;
   struct shmid_ds  ds;

   if (shared_debug) printf("shared_cleanup:");
   if (NULL != shared_lt)
     { if (shared_debug) printf(" deleting segments:");
       for (i=0; i<shared_maxseg; i++)
        { if (0 == shared_lt[i].tcnt) continue; /* we're not using this segment, skip this ... */
          if (-1 != shared_lt[i].lkcnt) continue;  /* seg not R/W locked by us, skip this ... */

          r = shared_destroy_entry(i);          /* destroy unconditionally sema & segment */
          if (shared_debug) 
            { if (SHARED_OK == r) printf(" [%d]", i);
              else printf(" [error on %d !!!!]", i);

            }
        }
       free((void *)shared_lt);                 /* free local table */
       shared_lt = NULL;
     }
   if (NULL != shared_gt)                       /* detach global index table */
     { oktodelete = 0;
       filelocked = 0;
       if (shared_debug) printf(" detaching globalsharedtable");
       if (SHARED_INVALID != shared_fd)

       flk.l_type = F_WRLCK;                    /* lock whole lock file */
       flk.l_whence = 0;
       flk.l_start = 0;
       flk.l_len = shared_maxseg;
       if (-1 != fcntl(shared_fd, F_SETLK, &flk))
         { filelocked = 1;                      /* success, scan global table, to see if there are any segs */
           segmentspresent = 0;                 /* assume, there are no segs in the system */
           for (j=0; j<shared_maxseg; j++)
            { if (SHARED_INVALID != shared_gt[j].key)
                { segmentspresent = 1;          /* yes, there is at least one */
                  break;
                }
            }
           if (0 == segmentspresent)            /* if there are no segs ... */
             if (0 == shmctl(shared_gt_h, IPC_STAT, &ds)) /* get number of processes attached to table */
               { if (ds.shm_nattch <= 1) oktodelete = 1; /* if only one (we), then it is safe (but see text 4 lines later) to unlink */
               }
         }
       shmdt((char *)shared_gt);                /* detach global table */
       if (oktodelete)                          /* delete global table from system, if no shm seg present */
         { shmctl(shared_gt_h, IPC_RMID, 0);    /* there is a race condition here - time window between shmdt and shmctl */
           shared_gt_h = SHARED_INVALID;
         }
       shared_gt = NULL;
       if (filelocked)                          /* if we locked, we need to unlock */
         { flk.l_type = F_UNLCK;
           flk.l_whence = 0;
           flk.l_start = 0;
           flk.l_len = shared_maxseg;
           fcntl(shared_fd, F_SETLK, &flk);
         }
     }
   shared_gt_h = SHARED_INVALID;

   if (SHARED_INVALID != shared_fd)             /* close lock file */
     { if (shared_debug) printf(" closing lockfile");
       close(shared_fd);
       shared_fd = SHARED_INVALID;
     }

   
   shared_kbase = 0;
   shared_maxseg = 0;
   shared_range = 0;
   shared_init_called = 0;

   if (shared_debug) printf(" <<done>>\n");
   return;
 }


int     shared_init(int debug_msgs)             /* initialize shared memory stuff, you have to call this routine once */
 { int i;
   char buf[1000], *p;
   mode_t oldumask;

   shared_init_called = 1;                      /* tell everybody no need to call us for the 2nd time */
   shared_debug = debug_msgs;                   /* set required debug mode */
   
   if (shared_debug) printf("shared_init:");

   shared_kbase = 0;                            /* adapt to current env. settings */
   if (NULL != (p = getenv(SHARED_ENV_KEYBASE))) shared_kbase = atoi(p);
   if (0 == shared_kbase) shared_kbase = SHARED_KEYBASE;
   if (shared_debug) printf(" keybase=%d", shared_kbase);

   shared_maxseg = 0;
   if (NULL != (p = getenv(SHARED_ENV_MAXSEG))) shared_maxseg = atoi(p);
   if (0 == shared_maxseg) shared_maxseg = SHARED_MAXSEG;
   if (shared_debug) printf(" maxseg=%d", shared_maxseg);
   
   shared_range = 3 * shared_maxseg;

   if (SHARED_INVALID == shared_fd)             /* create rw locking file (this file is never deleted) */
     { if (shared_debug) printf(" lockfileinit=");
       sprintf(buf, "%s.%d.%d", SHARED_FDNAME, shared_kbase, shared_maxseg);
       oldumask = umask(0);

       shared_fd = open(buf, O_TRUNC | O_EXCL | O_CREAT | O_RDWR, shared_create_mode);
       umask(oldumask);
       if (SHARED_INVALID == shared_fd)         /* or just open rw locking file, in case it already exists */
         { shared_fd = open(buf, O_TRUNC | O_RDWR, shared_create_mode);
           if (SHARED_INVALID == shared_fd) return(SHARED_NOFILE);
           if (shared_debug) printf("slave");

         }
       else
         { if (shared_debug) printf("master");
         }
     }

   if (SHARED_INVALID == shared_gt_h)           /* global table not attached, try to create it in shared memory */
     { if (shared_debug) printf(" globalsharedtableinit=");
       shared_gt_h = shmget(shared_kbase, shared_maxseg * sizeof(SHARED_GTAB), IPC_CREAT | IPC_EXCL | shared_create_mode); /* try open as a master */
       if (SHARED_INVALID == shared_gt_h)       /* if failed, try to open as a slave */
         { shared_gt_h = shmget(shared_kbase, shared_maxseg * sizeof(SHARED_GTAB), shared_create_mode);
           if (SHARED_INVALID == shared_gt_h) return(SHARED_IPCERR); /* means deleted ID residing in system, shared mem unusable ... */
           shared_gt = (SHARED_GTAB *)shmat(shared_gt_h, 0, 0); /* attach segment */
           if (((SHARED_GTAB *)SHARED_INVALID) == shared_gt) return(SHARED_IPCERR);
           if (shared_debug) printf("slave");
         }
       else
         { shared_gt = (SHARED_GTAB *)shmat(shared_gt_h, 0, 0); /* attach segment */
           if (((SHARED_GTAB *)SHARED_INVALID) == shared_gt) return(SHARED_IPCERR);
           for (i=0; i<shared_maxseg; i++) shared_clear_entry(i);       /* since we are master, init data */
           if (shared_debug) printf("master");
         }
     }

   if (NULL == shared_lt)                       /* initialize local table */
     { if (shared_debug) printf(" localtableinit=");
       if (NULL == (shared_lt = (SHARED_LTAB *)malloc(shared_maxseg * sizeof(SHARED_LTAB)))) return(SHARED_NOMEM);
       for (i=0; i<shared_maxseg; i++)
        { shared_lt[i].p = NULL;                /* not mapped */
          shared_lt[i].tcnt = 0;                /* unused (or zero threads using this seg) */
          shared_lt[i].lkcnt = 0;               /* segment is unlocked */
          shared_lt[i].seekpos = 0L;            /* r/w pointer at the beginning of file */
        }
       if (shared_debug) printf("ok");
     }

   atexit(shared_cleanup);                      /* we want shared_cleanup to be called at exit or abort */

   if (shared_debug) printf(" <<done>>\n");
   return(SHARED_OK);
 }


int     shared_recover(int id)                  /* try to recover dormant segments after applic crash */
 { int i, r, r2;

   if (NULL == shared_gt) return(SHARED_NOTINIT);       /* not initialized */
   if (NULL == shared_lt) return(SHARED_NOTINIT);       /* not initialized */
   r = SHARED_OK;
   for (i=0; i<shared_maxseg; i++)
    { if (-1 != id) if (i != id) continue;
      if (shared_lt[i].tcnt) continue;          /* somebody (we) is using it */
      if (SHARED_INVALID == shared_gt[i].key) continue; /* unused slot */
      if (shared_mux(i, SHARED_NOWAIT | SHARED_RDWRITE)) continue; /* acquire exclusive access to segment, but do not wait */
      r2 = shared_process_count(shared_gt[i].sem);
      if ((shared_gt[i].nprocdebug > r2) || (0 == r2))
        { if (shared_debug) printf("Bogus handle=%d nproc=%d sema=%d:", i, shared_gt[i].nprocdebug, r2);
          r = shared_destroy_entry(i);
          if (shared_debug)
            { printf("%s", r ? "error couldn't clear handle" : "handle cleared");
            }
        }
      shared_demux(i, SHARED_RDWRITE);
    }
   return(r);                                           /* table full */
 }

                /* API routines - mutexes and locking */

static  int shared_mux(int idx, int mode)       /* obtain exclusive access to specified segment */
 { flock_t flk;

   int r;

   if (0 == shared_init_called)                 /* delayed initialization */
     { if (SHARED_OK != (r = shared_init(0))) return(r);

     }
   if (SHARED_INVALID == shared_fd) return(SHARED_NOTINIT);
   if ((idx < 0) || (idx >= shared_maxseg)) return(SHARED_BADARG);
   flk.l_type = ((mode & SHARED_RDWRITE) ? F_WRLCK : F_RDLCK);
   flk.l_whence = 0;
   flk.l_start = idx;
   flk.l_len = 1;
   if (shared_debug) printf(" [mux (%d): ", idx);
   if (-1 == fcntl(shared_fd, ((mode & SHARED_NOWAIT) ? F_SETLK : F_SETLKW), &flk))
     { switch (errno)
        { case EAGAIN: ;

          case EACCES: if (shared_debug) printf("again]");
                       return(SHARED_AGAIN);
          default:     if (shared_debug) printf("err]");
                       return(SHARED_IPCERR);
        }
     }
   if (shared_debug) printf("ok]");
   return(SHARED_OK);
 }



static  int shared_demux(int idx, int mode)     /* free exclusive access to specified segment */
 { flock_t flk;

   if (SHARED_INVALID == shared_fd) return(SHARED_NOTINIT);
   if ((idx < 0) || (idx >= shared_maxseg)) return(SHARED_BADARG);
   flk.l_type = F_UNLCK;
   flk.l_whence = 0;
   flk.l_start = idx;
   flk.l_len = 1;
   if (shared_debug) printf(" [demux (%d): ", idx);
   if (-1 == fcntl(shared_fd, F_SETLKW, &flk))
     { switch (errno)
        { case EAGAIN: ;
          case EACCES: if (shared_debug) printf("again]");
                       return(SHARED_AGAIN);
          default:     if (shared_debug) printf("err]");
                       return(SHARED_IPCERR);
        }

     }
   if (shared_debug) printf("mode=%d ok]", mode);
   return(SHARED_OK);
 }



static int shared_process_count(int sem)                /* valid only for time of invocation */
 { union semun su;

   su.val = 0;                                          /* to force compiler not to give warning messages */
   return(semctl(sem, 0, GETVAL, su));                  /* su is unused here */
 }


static int shared_delta_process(int sem, int delta)     /* change number of processes hanging on segment */
 { struct sembuf sb;
 
   if (SHARED_INVALID == sem) return(SHARED_BADARG);    /* semaphore not attached */
   sb.sem_num = 0;
   sb.sem_op = delta;
   sb.sem_flg = SEM_UNDO;
   return((-1 == semop(sem, &sb, 1)) ? SHARED_IPCERR : SHARED_OK);
 }


static int shared_attach_process(int sem)
 { if (shared_debug) printf(" [attach process]");
   return(shared_delta_process(sem, 1));
 }


static int shared_detach_process(int sem)
 { if (shared_debug) printf(" [detach process]");
   return(shared_delta_process(sem, -1));
 }

                /* API routines - hashing and searching */


static int shared_get_free_entry(int newhandle)         /* get newhandle, or -1, entry is set rw locked */
 {
   if (NULL == shared_gt) return(-1);                   /* not initialized */
   if (NULL == shared_lt) return(-1);                   /* not initialized */
   if (newhandle < 0) return(-1);
   if (newhandle >= shared_maxseg) return(-1);
   if (shared_lt[newhandle].tcnt) return(-1);                   /* somebody (we) is using it */
   if (shared_mux(newhandle, SHARED_NOWAIT | SHARED_RDWRITE)) return(-1); /* used by others */
   if (SHARED_INVALID == shared_gt[newhandle].key) return(newhandle); /* we have found free slot, lock it and return index */
   shared_demux(newhandle, SHARED_RDWRITE);
   if (shared_debug) printf("[free_entry - ERROR - entry unusable]");
   return(-1);                                          /* table full */
 }


static int shared_get_hash(long size, int idx)  /* return hash value for malloc */
 { static int counter = 0;
   int hash;

   hash = (counter + size * idx) % shared_range;
   counter = (counter + 1) % shared_range;
   return(hash);
 }


static  long shared_adjust_size(long size)              /* size must be >= 0 !!! */
 { return(((size + sizeof(BLKHEAD) + SHARED_GRANUL - 1) / SHARED_GRANUL) * SHARED_GRANUL); }


                /* API routines - core : malloc/realloc/free/attach/detach/lock/unlock */

int     shared_malloc(long size, int mode, int newhandle)               /* return idx or SHARED_INVALID */
 { int h, i, r, idx, key;
   union semun filler;
   BLKHEAD *bp;
   
   if (0 == shared_init_called)                 /* delayed initialization */
     { if (SHARED_OK != (r = shared_init(0))) return(r);
     }
   if (shared_debug) printf("malloc (size = %ld, mode = %d):", size, mode);
   if (size < 0) return(SHARED_INVALID);
   if (-1 == (idx = shared_get_free_entry(newhandle)))  return(SHARED_INVALID);
   if (shared_debug) printf(" idx=%d", idx);
   for (i = 0; ; i++)
    { if (i >= shared_range)                            /* table full, signal error & exit */
        { shared_demux(idx, SHARED_RDWRITE);
          return(SHARED_INVALID);
        }
      key = shared_kbase + ((i + shared_get_hash(size, idx)) % shared_range);
      if (shared_debug) printf(" key=%d", key);
      h = shmget(key, shared_adjust_size(size), IPC_CREAT | IPC_EXCL | shared_create_mode);
      if (shared_debug) printf(" handle=%d", h);
      if (SHARED_INVALID == h) continue;                /* segment already accupied */
      bp = (BLKHEAD *)shmat(h, 0, 0);                   /* try attach */
      if (shared_debug) printf(" p=%p", bp);
      if (((BLKHEAD *)SHARED_INVALID) == bp)            /* cannot attach, delete segment, try with another key */
        { shmctl(h, IPC_RMID, 0);
          continue;
        }                                               /* now create semaphor counting number of processes attached */
      if (SHARED_INVALID == (shared_gt[idx].sem = semget(key, 1, IPC_CREAT | IPC_EXCL | shared_create_mode)))
        { shmdt((void *)bp);                            /* cannot create segment, delete everything */
          shmctl(h, IPC_RMID, 0);
          continue;                                     /* try with another key */
        }
      if (shared_debug) printf(" sem=%d", shared_gt[idx].sem);
      if (shared_attach_process(shared_gt[idx].sem))    /* try attach process */
        { semctl(shared_gt[idx].sem, 0, IPC_RMID, filler);      /* destroy semaphore */
          shmdt((char *)bp);                            /* detach shared mem segment */
          shmctl(h, IPC_RMID, 0);                       /* destroy shared mem segment */
          continue;                                     /* try with another key */
        }
      bp->s.tflag = BLOCK_SHARED;                       /* fill in data in segment's header (this is really not necessary) */
      bp->s.ID[0] = SHARED_ID_0;
      bp->s.ID[1] = SHARED_ID_1;
      bp->s.handle = idx;                               /* used in yorick */
      if (mode & SHARED_RESIZE)
        { if (shmdt((char *)bp)) r = SHARED_IPCERR;     /* if segment is resizable, then detach segment */
          shared_lt[idx].p = NULL;
        }
      else  { shared_lt[idx].p = bp; }
      shared_lt[idx].tcnt = 1;                          /* one thread using segment */
      shared_lt[idx].lkcnt = 0;                         /* no locks at the moment */
      shared_lt[idx].seekpos = 0L;                      /* r/w pointer positioned at beg of block */
      shared_gt[idx].handle = h;                        /* fill in data in global table */
      shared_gt[idx].size = size;
      shared_gt[idx].attr = mode;
      shared_gt[idx].semkey = key;
      shared_gt[idx].key = key;
      shared_gt[idx].nprocdebug = 0;

      break;
    }
   shared_demux(idx, SHARED_RDWRITE);                   /* hope this will not fail */
   return(idx);
 }


int     shared_attach(int idx)
 { int r, r2;

   if (SHARED_OK != (r = shared_mux(idx, SHARED_RDWRITE | SHARED_WAIT))) return(r);
   if (SHARED_OK != (r = shared_map(idx)))
     { shared_demux(idx, SHARED_RDWRITE);
       return(r);
     }
   if (shared_attach_process(shared_gt[idx].sem))       /* try attach process */
     { shmdt((char *)(shared_lt[idx].p));               /* cannot attach process, detach everything */
       shared_lt[idx].p = NULL;
       shared_demux(idx, SHARED_RDWRITE);
       return(SHARED_BADARG);
     }
   shared_lt[idx].tcnt++;                               /* one more thread is using segment */
   if (shared_gt[idx].attr & SHARED_RESIZE)             /* if resizeable, detach and return special pointer */
     { if (shmdt((char *)(shared_lt[idx].p))) r = SHARED_IPCERR;  /* if segment is resizable, then detach segment */
       shared_lt[idx].p = NULL;
     }
   shared_lt[idx].seekpos = 0L;                         /* r/w pointer positioned at beg of block */
   r2 = shared_demux(idx, SHARED_RDWRITE);
   return(r ? r : r2);
 }



static int      shared_check_locked_index(int idx)      /* verify that given idx is valid */ 
 { int r;

   if (0 == shared_init_called)                         /* delayed initialization */
     { if (SHARED_OK != (r = shared_init(0))) return(r);

     }
   if ((idx < 0) || (idx >= shared_maxseg)) return(SHARED_BADARG);
   if (NULL == shared_lt[idx].p) return(SHARED_BADARG); /* NULL pointer, not attached ?? */
   if (0 == shared_lt[idx].lkcnt) return(SHARED_BADARG); /* not locked ?? */
   if ((SHARED_ID_0 != (shared_lt[idx].p)->s.ID[0]) || (SHARED_ID_1 != (shared_lt[idx].p)->s.ID[1]) || 
       (BLOCK_SHARED != (shared_lt[idx].p)->s.tflag))   /* invalid data in segment */
     return(SHARED_BADARG);
   return(SHARED_OK);
 }



static int      shared_map(int idx)                     /* map all tables for given idx, check for validity */
 { int h;                                               /* have to obtain excl. access before calling shared_map */
   BLKHEAD *bp;

   if ((idx < 0) || (idx >= shared_maxseg)) return(SHARED_BADARG);
   if (SHARED_INVALID == shared_gt[idx].key)  return(SHARED_BADARG);
   if (SHARED_INVALID == (h = shmget(shared_gt[idx].key, 1, shared_create_mode)))  return(SHARED_BADARG);
   if (((BLKHEAD *)SHARED_INVALID) == (bp = (BLKHEAD *)shmat(h, 0, 0)))  return(SHARED_BADARG);
   if ((SHARED_ID_0 != bp->s.ID[0]) || (SHARED_ID_1 != bp->s.ID[1]) || (BLOCK_SHARED != bp->s.tflag) || (h != shared_gt[idx].handle))
     { shmdt((char *)bp);                               /* invalid segment, detach everything */
       return(SHARED_BADARG);

     }
   if (shared_gt[idx].sem != semget(shared_gt[idx].semkey, 1, shared_create_mode)) /* check if sema is still there */
     { shmdt((char *)bp);                               /* cannot attach semaphore, detach everything */
       return(SHARED_BADARG);
     }
   shared_lt[idx].p = bp;                               /* store pointer to shmem data */
   return(SHARED_OK);
 }


static  int     shared_validate(int idx, int mode)      /* use intrnally inside crit.sect !!! */
 { int r;

   if (SHARED_OK != (r = shared_mux(idx, mode)))  return(r);            /* idx checked by shared_mux */
   if (NULL == shared_lt[idx].p)
     if (SHARED_OK != (r = shared_map(idx)))
       { shared_demux(idx, mode); 
         return(r);
       }
   if ((SHARED_ID_0 != (shared_lt[idx].p)->s.ID[0]) || (SHARED_ID_1 != (shared_lt[idx].p)->s.ID[1]) || (BLOCK_SHARED != (shared_lt[idx].p)->s.tflag))
     { shared_demux(idx, mode);
       return(r);
     }
   return(SHARED_OK);
 }


SHARED_P shared_realloc(int idx, long newsize)  /* realloc shared memory segment */
 { int h, key, i, r;
   BLKHEAD *bp;
   long transfersize;

   r = SHARED_OK;
   if (newsize < 0) return(NULL);
   if (shared_check_locked_index(idx)) return(NULL);
   if (0 == (shared_gt[idx].attr & SHARED_RESIZE)) return(NULL);
   if (-1 != shared_lt[idx].lkcnt) return(NULL); /* check for RW lock */
   if (shared_adjust_size(shared_gt[idx].size) == shared_adjust_size(newsize))
     { shared_gt[idx].size = newsize;

       return((SHARED_P)((shared_lt[idx].p) + 1));
     }
   for (i = 0; ; i++)
    { if (i >= shared_range)  return(NULL);     /* table full, signal error & exit */
      key = shared_kbase + ((i + shared_get_hash(newsize, idx)) % shared_range);
      h = shmget(key, shared_adjust_size(newsize), IPC_CREAT | IPC_EXCL | shared_create_mode);
      if (SHARED_INVALID == h) continue;        /* segment already accupied */
      bp = (BLKHEAD *)shmat(h, 0, 0);           /* try attach */
      if (((BLKHEAD *)SHARED_INVALID) == bp)    /* cannot attach, delete segment, try with another key */
        { shmctl(h, IPC_RMID, 0);
          continue;
        }
      *bp = *(shared_lt[idx].p);                /* copy header, then data */
      transfersize = ((newsize < shared_gt[idx].size) ? newsize : shared_gt[idx].size);
      if (transfersize > 0)
        memcpy((void *)(bp + 1), (void *)((shared_lt[idx].p) + 1), transfersize);
      if (shmdt((char *)(shared_lt[idx].p))) r = SHARED_IPCERR; /* try to detach old segment */
      if (shmctl(shared_gt[idx].handle, IPC_RMID, 0)) if (SHARED_OK == r) r = SHARED_IPCERR;  /* destroy old shared memory segment */
      shared_gt[idx].size = newsize;            /* signal new size */
      shared_gt[idx].handle = h;                /* signal new handle */
      shared_gt[idx].key = key;                 /* signal new key */
      shared_lt[idx].p = bp;
      break;
    }
   return((SHARED_P)(bp + 1));
 }


int     shared_free(int idx)                    /* detach segment, if last process & !PERSIST, destroy segment */
 { int cnt, r, r2;

   if (SHARED_OK != (r = shared_validate(idx, SHARED_RDWRITE | SHARED_WAIT))) return(r);
   if (SHARED_OK != (r = shared_detach_process(shared_gt[idx].sem)))    /* update number of processes using segment */
     { shared_demux(idx, SHARED_RDWRITE);
       return(r);
     }
   shared_lt[idx].tcnt--;                       /* update number of threads using segment */
   if (shared_lt[idx].tcnt > 0)  return(shared_demux(idx, SHARED_RDWRITE));  /* if more threads are using segment we are done */
   if (shmdt((char *)(shared_lt[idx].p)))       /* if, we are the last thread, try to detach segment */
     { shared_demux(idx, SHARED_RDWRITE);
       return(SHARED_IPCERR);
     }
   shared_lt[idx].p = NULL;                     /* clear entry in local table */
   shared_lt[idx].seekpos = 0L;                 /* r/w pointer positioned at beg of block */
   if (-1 == (cnt = shared_process_count(shared_gt[idx].sem))) /* get number of processes hanging on segment */
     { shared_demux(idx, SHARED_RDWRITE);
       return(SHARED_IPCERR);
     }
   if ((0 == cnt) && (0 == (shared_gt[idx].attr & SHARED_PERSIST)))  r = shared_destroy_entry(idx); /* no procs on seg, destroy it */
   r2 = shared_demux(idx, SHARED_RDWRITE);
   return(r ? r : r2);
 }


SHARED_P shared_lock(int idx, int mode)         /* lock given segment for exclusive access */
 { int r;

   if (shared_mux(idx, mode))  return(NULL);    /* idx checked by shared_mux */
   if (0 != shared_lt[idx].lkcnt)               /* are we already locked ?? */
     if (SHARED_OK != (r = shared_map(idx)))
       { shared_demux(idx, mode); 
         return(NULL);
       }
   if (NULL == shared_lt[idx].p)                /* stupid pointer ?? */
     if (SHARED_OK != (r = shared_map(idx)))
       { shared_demux(idx, mode); 
         return(NULL);
       }
   if ((SHARED_ID_0 != (shared_lt[idx].p)->s.ID[0]) || (SHARED_ID_1 != (shared_lt[idx].p)->s.ID[1]) || (BLOCK_SHARED != (shared_lt[idx].p)->s.tflag))
     { shared_demux(idx, mode);
       return(NULL);
     }
   if (mode & SHARED_RDWRITE)
     { shared_lt[idx].lkcnt = -1;

       shared_gt[idx].nprocdebug++;
     }

   else shared_lt[idx].lkcnt++;
   shared_lt[idx].seekpos = 0L;                 /* r/w pointer positioned at beg of block */
   return((SHARED_P)((shared_lt[idx].p) + 1));
 }


int     shared_unlock(int idx)                  /* unlock given segment, assumes seg is locked !! */
 { int r, r2, mode;

   if (SHARED_OK != (r = shared_check_locked_index(idx))) return(r);
   if (shared_lt[idx].lkcnt > 0)
     { shared_lt[idx].lkcnt--;                  /* unlock read lock */
       mode = SHARED_RDONLY;
     }
   else
     { shared_lt[idx].lkcnt = 0;                /* unlock write lock */
       shared_gt[idx].nprocdebug--;
       mode = SHARED_RDWRITE;
     }
   if (0 == shared_lt[idx].lkcnt) if (shared_gt[idx].attr & SHARED_RESIZE)
     { if (shmdt((char *)(shared_lt[idx].p))) r = SHARED_IPCERR; /* segment is resizable, then detach segment */
       shared_lt[idx].p = NULL;                 /* signal detachment in local table */
     }
   r2 = shared_demux(idx, mode);                /* unlock segment, rest is only parameter checking */
   return(r ? r : r2);
 }

                /* API routines - support and info routines */


int     shared_attr(int idx)                    /* get the attributes of the shared memory segment */
 { int r;

   if (shared_check_locked_index(idx)) return(SHARED_INVALID);
   r = shared_gt[idx].attr;
   return(r);
 }


int     shared_set_attr(int idx, int newattr)   /* get the attributes of the shared memory segment */
 { int r;

   if (shared_check_locked_index(idx)) return(SHARED_INVALID);
   if (-1 != shared_lt[idx].lkcnt) return(SHARED_INVALID); /* ADDED - check for RW lock */
   r = shared_gt[idx].attr;
   shared_gt[idx].attr = newattr;
   return(r);

 }


int     shared_set_debug(int mode)              /* set/reset debug mode */
 { int r = shared_debug;

   shared_debug = mode;
   return(r);
 }


int     shared_set_createmode(int mode)          /* set/reset debug mode */
 { int r = shared_create_mode;

   shared_create_mode = mode;
   return(r);
 }




int     shared_list(int id)
 { int i, r;

   if (NULL == shared_gt) return(SHARED_NOTINIT);       /* not initialized */
   if (NULL == shared_lt) return(SHARED_NOTINIT);       /* not initialized */
   if (shared_debug) printf("shared_list:");
   r = SHARED_OK;
   printf(" Idx    Key   Nproc   Size   Flags\n");
   printf("==============================================\n");
   for (i=0; i<shared_maxseg; i++)
    { if (-1 != id) if (i != id) continue;
      if (SHARED_INVALID == shared_gt[i].key) continue; /* unused slot */
      switch (shared_mux(i, SHARED_NOWAIT | SHARED_RDONLY)) /* acquire exclusive access to segment, but do not wait */

       { case SHARED_AGAIN:
                printf("!%3d %08lx %4d  %8d", i, (unsigned long int)shared_gt[i].key,
                                shared_gt[i].nprocdebug, shared_gt[i].size);
                if (SHARED_RESIZE & shared_gt[i].attr) printf(" RESIZABLE");
                if (SHARED_PERSIST & shared_gt[i].attr) printf(" PERSIST");
                printf("\n");
                break;
         case SHARED_OK:
                printf(" %3d %08lx %4d  %8d", i, (unsigned long int)shared_gt[i].key,

                                shared_gt[i].nprocdebug, shared_gt[i].size);
                if (SHARED_RESIZE & shared_gt[i].attr) printf(" RESIZABLE");
                if (SHARED_PERSIST & shared_gt[i].attr) printf(" PERSIST");
                printf("\n");
                shared_demux(i, SHARED_RDONLY);
                break;
         default:
                continue;
       }
    }
   if (shared_debug) printf(" done\n");
   return(r);                                           /* table full */
 }

int     shared_getaddr(int id, char **address)
 { int i;
   char segname[10];

   if (NULL == shared_gt) return(SHARED_NOTINIT);       /* not initialized */
   if (NULL == shared_lt) return(SHARED_NOTINIT);       /* not initialized */
 
   strcpy(segname,"h");
   sprintf(segname+1,"%d", id);
 
   if (smem_open(segname,0,&i)) return(SHARED_BADARG);
 
   *address = ((char *)(((DAL_SHM_SEGHEAD *)(shared_lt[i].p + 1)) + 1));
 /*  smem_close(i); */
   return(SHARED_OK);
 }


int     shared_uncond_delete(int id)
 { int i, r;

   if (NULL == shared_gt) return(SHARED_NOTINIT);       /* not initialized */
   if (NULL == shared_lt) return(SHARED_NOTINIT);       /* not initialized */
   if (shared_debug) printf("shared_uncond_delete:");
   r = SHARED_OK;
   for (i=0; i<shared_maxseg; i++)
    { if (-1 != id) if (i != id) continue;
      if (shared_attach(i))
        { if (-1 != id) printf("no such handle\n");
          continue;
        }
      printf("handle %d:", i);
      if (NULL == shared_lock(i, SHARED_RDWRITE | SHARED_NOWAIT)) 
        { printf(" cannot lock in RW mode, not deleted\n");
          continue;
        }
      if (shared_set_attr(i, SHARED_RESIZE) >= SHARED_ERRBASE)
        { printf(" cannot clear PERSIST attribute");
        }
      if (shared_free(i))
        { printf(" delete failed\n");
        }
      else
        { printf(" deleted\n");
        }
    }
   if (shared_debug) printf(" done\n");
   return(r);                                           /* table full */
 }


/************************* CFITSIO DRIVER FUNCTIONS ***************************/

int     smem_init(void)
 { return(0);
 }

int     smem_shutdown(void)

 { if (shared_init_called) shared_cleanup();
   return(0);
 }

int     smem_setoptions(int option)
 { option = 0;
   return(0);
 }


int     smem_getoptions(int *options)
 { if (NULL == options) return(SHARED_NULPTR);
   *options = 0;
   return(0);
 }

int     smem_getversion(int *version)
 { if (NULL == version) return(SHARED_NULPTR);
   *version = 10;
   return(0);
 }


int     smem_open(char *filename, int rwmode, int *driverhandle)
 { int h, nitems, r;
   DAL_SHM_SEGHEAD *sp;


   if (NULL == filename) return(SHARED_NULPTR);
   if (NULL == driverhandle) return(SHARED_NULPTR);
   nitems = sscanf(filename, "h%d", &h);
   if (1 != nitems) return(SHARED_BADARG);

   if (SHARED_OK != (r = shared_attach(h))) return(r);

   if (NULL == (sp = (DAL_SHM_SEGHEAD *)shared_lock(h,
                ((READWRITE == rwmode) ? SHARED_RDWRITE : SHARED_RDONLY))))
     {  shared_free(h);
        return(SHARED_BADARG);
     }

   if ((h != sp->h) || (DAL_SHM_SEGHEAD_ID != sp->ID))
     { shared_unlock(h);
       shared_free(h);

       return(SHARED_BADARG);
     }

   *driverhandle = h;
   return(0);
 }


int     smem_create(char *filename, int *driverhandle)
 { DAL_SHM_SEGHEAD *sp;
   int h, sz, nitems;

   if (NULL == filename) return(SHARED_NULPTR);         /* currently ignored */
   if (NULL == driverhandle) return(SHARED_NULPTR);
   nitems = sscanf(filename, "h%d", &h);
   if (1 != nitems) return(SHARED_BADARG);

   if (SHARED_INVALID == (h = shared_malloc(sz = 2880 + sizeof(DAL_SHM_SEGHEAD), 
                        SHARED_RESIZE | SHARED_PERSIST, h)))
     return(SHARED_NOMEM);

   if (NULL == (sp = (DAL_SHM_SEGHEAD *)shared_lock(h, SHARED_RDWRITE)))
     { shared_free(h);
       return(SHARED_BADARG);
     }

   sp->ID = DAL_SHM_SEGHEAD_ID;
   sp->h = h;
   sp->size = sz;
   sp->nodeidx = -1;

   *driverhandle = h;
   
   return(0);
 }


int     smem_close(int driverhandle)
 { int r;

   if (SHARED_OK != (r = shared_unlock(driverhandle))) return(r);
   return(shared_free(driverhandle));
 }

int     smem_remove(char *filename)
 { int nitems, h, r;

   if (NULL == filename) return(SHARED_NULPTR);
   nitems = sscanf(filename, "h%d", &h);
   if (1 != nitems) return(SHARED_BADARG);

   if (0 == shared_check_locked_index(h))       /* are we locked ? */

     { if (-1 != shared_lt[h].lkcnt)            /* are we locked RO ? */
         { if (SHARED_OK != (r = shared_unlock(h))) return(r);  /* yes, so relock in RW */
           if (NULL == shared_lock(h, SHARED_RDWRITE)) return(SHARED_BADARG);
         }

     }
   else                                         /* not locked */
     { if (SHARED_OK != (r = smem_open(filename, READWRITE, &h)))
         return(r);                             /* so open in RW mode */
     }

   shared_set_attr(h, SHARED_RESIZE);           /* delete PERSIST attribute */
   return(smem_close(h));                       /* detach segment (this will delete it) */
 }

int     smem_size(int driverhandle, LONGLONG *size)
 {
   if (NULL == size) return(SHARED_NULPTR);
   if (shared_check_locked_index(driverhandle)) return(SHARED_INVALID);
   *size = (LONGLONG) (shared_gt[driverhandle].size - sizeof(DAL_SHM_SEGHEAD));
   return(0);
 }

int     smem_flush(int driverhandle)
 {
   if (shared_check_locked_index(driverhandle)) return(SHARED_INVALID);
   return(0);
 }

int     smem_seek(int driverhandle, LONGLONG offset)
 {
   if (offset < 0) return(SHARED_BADARG);
   if (shared_check_locked_index(driverhandle)) return(SHARED_INVALID);
   shared_lt[driverhandle].seekpos = offset;
   return(0);
 }

int     smem_read(int driverhandle, void *buffer, long nbytes)
 {
   if (NULL == buffer) return(SHARED_NULPTR);
   if (shared_check_locked_index(driverhandle)) return(SHARED_INVALID);
   if (nbytes < 0) return(SHARED_BADARG);
   if ((shared_lt[driverhandle].seekpos + nbytes) > shared_gt[driverhandle].size)
     return(SHARED_BADARG);             /* read beyond EOF */

   memcpy(buffer,
          ((char *)(((DAL_SHM_SEGHEAD *)(shared_lt[driverhandle].p + 1)) + 1)) +
                shared_lt[driverhandle].seekpos,
          nbytes);

   shared_lt[driverhandle].seekpos += nbytes;
   return(0);
 }

int     smem_write(int driverhandle, void *buffer, long nbytes)
 {
   if (NULL == buffer) return(SHARED_NULPTR);
   if (shared_check_locked_index(driverhandle)) return(SHARED_INVALID);
   if (-1 != shared_lt[driverhandle].lkcnt) return(SHARED_INVALID); /* are we locked RW ? */

   if (nbytes < 0) return(SHARED_BADARG);
   if ((unsigned long)(shared_lt[driverhandle].seekpos + nbytes) > (unsigned long)(shared_gt[driverhandle].size - sizeof(DAL_SHM_SEGHEAD)))
     {                  /* need to realloc shmem */
       if (NULL == shared_realloc(driverhandle, shared_lt[driverhandle].seekpos + nbytes + sizeof(DAL_SHM_SEGHEAD)))
         return(SHARED_NOMEM);
     }

   memcpy(((char *)(((DAL_SHM_SEGHEAD *)(shared_lt[driverhandle].p + 1)) + 1)) +
                shared_lt[driverhandle].seekpos,
          buffer,
          nbytes);

   shared_lt[driverhandle].seekpos += nbytes;
   return(0);
 }
#endif
