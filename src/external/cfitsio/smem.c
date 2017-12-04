#include <stdio.h>
#include <memory.h>
#include <string.h>
#ifdef __APPLE__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include "fitsio.h"     /* needed to define LONGLONG */
#include "drvrsmem.h"   /* uses LONGLONG */

int	main(int argc, char **argv)
{ int cmdok, listmode, longlistmode, recovermode, deletemode, id;
int status;
char *address;

listmode = longlistmode = recovermode = deletemode = 0;
id = -1;
cmdok = 1;

switch (argc)
 { case 1:	listmode = 1;
		break;
   case 2:
		if (0 == strcmp("-l", argv[1])) longlistmode = 1;
		else if (0 == strcmp("-r", argv[1])) recovermode = 1;
		else if (0 == strcmp("-d", argv[1])) deletemode = 1;
		else cmdok = 0;
		break;
   case 3:
		if (0 == strcmp("-r", argv[1])) recovermode = 1;
		else if (0 == strcmp("-d", argv[1])) deletemode = 1;
		else
		 { cmdok = 0;		/* signal invalid cmd line syntax */
		   break;
		 }
		if (1 != sscanf(argv[2], "%d", &id)) cmdok = 0;
		break;
   default:
		cmdok = 0;
		break;
 }

if (0 == cmdok)
  { printf("usage :\n\n");
    printf("smem            - list all shared memory segments\n");
    printf("\t!\tcouldn't obtain RDONLY lock - info unreliable\n");
    printf("\tIdx\thandle of shared memory segment (visible by application)\n");
    printf("\tKey\tcurrent system key of shared memory segment. Key\n");
    printf("\t\tchanges whenever shmem segment is reallocated. Use\n");
    printf("\t\tipcs (or ipcs -a) to view all shmem segments\n");
    printf("\tNproc\tnumber of processes attached to segment\n");
    printf("\tSize\tsize of shmem segment in bytes\n");
    printf("\tFlags\tRESIZABLE - realloc allowed, PERSIST - segment is not\n");
    printf("\t\tdeleted after shared_free called by last process attached\n");
    printf("\t\tto it.\n");
    printf("smem -d         - delete all shared memory segments (may block)\n");
    printf("smem -d id      - delete specific shared memory segment (may block)\n");
    printf("smem -r         - unconditionally reset all shared memory segments\n\t\t(does not block, recovers zombie handles left by kill -9)\n");
    printf("smem -r id      - unconditionally reset specific shared memory segment\n");
  }

if (shared_init(0))
  { printf("couldn't initialize shared memory, aborting ...\n");
    return(10);
  }

if (listmode) shared_list(id);
else if (recovermode) shared_recover(id);
else if (deletemode) shared_uncond_delete(id);

for (id = 0; id <16; id++) {
  status = shared_getaddr(id, &address);
  if (!status)printf("id, status, address %d %d %ld %.30s\n", id, status, address, address);
}
return(0);
}
