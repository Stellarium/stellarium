/*		S H A R E D   M E M O R Y   D R I V E R
		=======================================

		  by Jerzy.Borkowski@obs.unige.ch

09-Mar-98 : initial version 1.0 released
23-Mar-98 : shared_malloc now accepts new handle as an argument
*/


#include <sys/ipc.h>		/* this is necessary for Solaris/Linux */
#include <sys/shm.h>
#include <sys/sem.h>

#ifdef _AIX
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

		/* configuration parameters */

#define	SHARED_MAXSEG	(16)		/* maximum number of shared memory blocks */

#define	SHARED_KEYBASE	(14011963)	/* base for shared memory keys, may be overriden by getenv */
#define	SHARED_FDNAME	("/tmp/.shmem-lockfile") /* template for lock file name */

#define	SHARED_ENV_KEYBASE ("SHMEM_LIB_KEYBASE") /* name of environment variable */
#define	SHARED_ENV_MAXSEG ("SHMEM_LIB_MAXSEG")	/* name of environment variable */

		/* useful constants */

#define	SHARED_RDONLY	(0)		/* flag for shared_(un)lock, lock for read */
#define	SHARED_RDWRITE	(1)		/* flag for shared_(un)lock, lock for write */
#define	SHARED_WAIT	(0)		/* flag for shared_lock, block if cannot lock immediate */
#define	SHARED_NOWAIT	(2)		/* flag for shared_lock, fail if cannot lock immediate */
#define	SHARED_NOLOCK	(0x100)		/* flag for shared_validate function */

#define	SHARED_RESIZE	(4)		/* flag for shared_malloc, object is resizeable */
#define	SHARED_PERSIST	(8)		/* flag for shared_malloc, object is not deleted after last proc detaches */

#define	SHARED_INVALID	(-1)		/* invalid handle for semaphore/shared memory */

#define	SHARED_EMPTY	(0)		/* entries for shared_used table */
#define	SHARED_USED	(1)

#define	SHARED_GRANUL	(16384)		/* granularity of shared_malloc allocation = phys page size, system dependent */



		/* checkpoints in shared memory segments - might be omitted */

#define	SHARED_ID_0	('J')		/* first byte of identifier in BLKHEAD */
#define	SHARED_ID_1	('B')		/* second byte of identifier in BLKHEAD */

#define	BLOCK_REG	(0)		/* value for tflag member of BLKHEAD */
#define	BLOCK_SHARED	(1)		/* value for tflag member of BLKHEAD */

		/* generic error codes */

#define	SHARED_OK	(0)

#define	SHARED_ERR_MIN_IDX	SHARED_BADARG
#define	SHARED_ERR_MAX_IDX	SHARED_NORESIZE


#define	DAL_SHM_FREE	(0)
#define	DAL_SHM_USED	(1)

#define	DAL_SHM_ID0	('D')
#define	DAL_SHM_ID1	('S')
#define	DAL_SHM_ID2	('M')

#define	DAL_SHM_SEGHEAD_ID	(0x19630114)



		/* data types */

/* BLKHEAD object is placed at the beginning of every memory segment (both
  shared and regular) to allow automatic recognition of segments type */

typedef union
      { struct BLKHEADstruct
	      {	char	ID[2];		/* ID = 'JB', just as a checkpoint */
		char	tflag;		/* is it shared memory or regular one ? */
		int	handle;		/* this is not necessary, used only for non-resizeable objects via ptr */
	      } s;
	double	d;			/* for proper alignment on every machine */
      } BLKHEAD;

typedef void *SHARED_P;			/* generic type of shared memory pointer */

typedef	struct SHARED_GTABstruct	/* data type used in global table */
      {	int	sem;			/* access semaphore (1 field): process count */
	int	semkey;			/* key value used to generate semaphore handle */
	int	key;			/* key value used to generate shared memory handle (realloc changes it) */
	int	handle;			/* handle of shared memory segment */
	int	size;			/* size of shared memory segment */
	int	nprocdebug;		/* attached proc counter, helps remove zombie segments */
	char	attr;			/* attributes of shared memory object */
      } SHARED_GTAB;

typedef	struct SHARED_LTABstruct	/* data type used in local table */
      {	BLKHEAD	*p;			/* pointer to segment (may be null) */
	int	tcnt;			/* number of threads in this process attached to segment */
	int	lkcnt;			/* >=0 <- number of read locks, -1 - write lock */
	long	seekpos;		/* current pointer position, read/write/seek operations change it */
      } SHARED_LTAB;


	/* system dependent definitions */

#ifndef HAVE_FLOCK_T
typedef struct flock flock_t;
#define HAVE_FLOCK_T
#endif

#ifndef HAVE_UNION_SEMUN
union semun
      {	int val;
	struct semid_ds *buf;
	unsigned short *array;
      };
#define HAVE_UNION_SEMUN
#endif


typedef struct DAL_SHM_SEGHEAD_STRUCT	DAL_SHM_SEGHEAD;

struct DAL_SHM_SEGHEAD_STRUCT
      {	int	ID;			/* ID for debugging */
	int	h;			/* handle of sh. mem */
	int	size;			/* size of data area */
	int	nodeidx;		/* offset of root object (node struct typically) */
      };

		/* API routines */

#ifdef __cplusplus
extern "C" {
#endif

void	shared_cleanup(void);			/* must be called at exit/abort */
int	shared_init(int debug_msgs);		/* must be called before any other shared memory routine */
int	shared_recover(int id);			/* try to recover dormant segment(s) after applic crash */
int	shared_malloc(long size, int mode, int newhandle);	/* allocate n-bytes of shared memory */
int	shared_attach(int idx);			/* attach to segment given index to table */
int	shared_free(int idx);			/* release shared memory */
SHARED_P shared_lock(int idx, int mode);	/* lock segment for reading */
SHARED_P shared_realloc(int idx, long newsize);	/* reallocate n-bytes of shared memory (ON LOCKED SEGMENT ONLY) */
int	shared_size(int idx);			/* get size of attached shared memory segment (ON LOCKED SEGMENT ONLY) */
int	shared_attr(int idx);			/* get attributes of attached shared memory segment (ON LOCKED SEGMENT ONLY) */
int	shared_set_attr(int idx, int newattr);	/* set attributes of attached shared memory segment (ON LOCKED SEGMENT ONLY) */
int	shared_unlock(int idx);			/* unlock segment (ON LOCKED SEGMENT ONLY) */
int	shared_set_debug(int debug_msgs);	/* set/reset debug mode */
int	shared_set_createmode(int mode);	/* set/reset debug mode */
int	shared_list(int id);			/* list segment(s) */
int	shared_uncond_delete(int id);		/* uncondintionally delete (NOWAIT operation) segment(s) */
int	shared_getaddr(int id, char **address);	/* get starting address of FITS file in segment */

int	smem_init(void);
int	smem_shutdown(void);
int	smem_setoptions(int options);
int	smem_getoptions(int *options);
int	smem_getversion(int *version);
int	smem_open(char *filename, int rwmode, int *driverhandle);
int	smem_create(char *filename, int *driverhandle);
int	smem_close(int driverhandle);
int	smem_remove(char *filename);
int	smem_size(int driverhandle, LONGLONG *size);
int	smem_flush(int driverhandle);
int	smem_seek(int driverhandle, LONGLONG offset);
int	smem_read(int driverhandle, void *buffer, long nbytes);
int	smem_write(int driverhandle, void *buffer, long nbytes);

#ifdef __cplusplus
}
#endif
