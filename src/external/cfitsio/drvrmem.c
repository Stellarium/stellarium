/*  This file, drvrmem.c, contains driver routines for memory files.        */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include <stddef.h>  /* apparently needed to define size_t */
#include "fitsio2.h"

#if HAVE_BZIP2
#include "bzlib.h"
#endif

/* prototype for .Z file uncompression function in zuncompress.c */
int zuncompress2mem(char *filename, 
             FILE *diskfile, 
             char **buffptr, 
             size_t *buffsize, 
             void *(*mem_realloc)(void *p, size_t newsize),
             size_t *filesize,
             int *status);

#if HAVE_BZIP2
/* prototype for .bz2 uncompression function (in this file) */
void bzip2uncompress2mem(char *filename, FILE *diskfile, int hdl,
                         size_t* filesize, int* status);
#endif


#define RECBUFLEN 1000

static char stdin_outfile[FLEN_FILENAME];

typedef struct    /* structure containing mem file structure */ 
{
    char **memaddrptr;   /* Pointer to memory address pointer; */
                         /* This may or may not point to memaddr. */
    char *memaddr;       /* Pointer to starting memory address; may */
                         /* not always be used, so use *memaddrptr instead */
    size_t *memsizeptr;  /* Pointer to the size of the memory allocation. */
                         /* This may or may not point to memsize. */
    size_t memsize;      /* Size of the memory allocation; this may not */
                         /* always be used, so use *memsizeptr instead. */
    size_t deltasize;    /* Suggested increment for reallocating memory */
    void *(*mem_realloc)(void *p, size_t newsize);  /* realloc function */
    LONGLONG currentpos;   /* current file position, relative to start */
    LONGLONG fitsfilesize; /* size of the FITS file (always <= *memsizeptr) */
    FILE *fileptr;      /* pointer to compressed output disk file */
} memdriver;

static memdriver memTable[NMAXFILES];  /* allocate mem file handle tables */

/*--------------------------------------------------------------------------*/
int mem_init(void)
{
    int ii;

    for (ii = 0; ii < NMAXFILES; ii++) /* initialize all empty slots in table */
    {
       memTable[ii].memaddrptr = 0;
       memTable[ii].memaddr = 0;
    }
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_setoptions(int options)
{
  /* do something with the options argument, to stop compiler warning */
  options = 0;
  return(options);
}
/*--------------------------------------------------------------------------*/
int mem_getoptions(int *options)
{
  *options = 0;
  return(0);
}
/*--------------------------------------------------------------------------*/
int mem_getversion(int *version)
{
    *version = 10;
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_shutdown(void)
{
  return(0);
}
/*--------------------------------------------------------------------------*/
int mem_create(char *filename, int *handle)
/*
  Create a new empty memory file for subsequent writes.
  The file name is ignored in this case.
*/
{
    int status;

    /* initially allocate 1 FITS block = 2880 bytes */
    status = mem_createmem(2880L, handle);

    if (status)
    {
        ffpmsg("failed to create empty memory file (mem_create)");
        return(status);
    }

    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_create_comp(char *filename, int *handle)
/*
  Create a new empty memory file for subsequent writes.
  Also create an empty compressed .gz file.  The memory file
  will be compressed and written to the disk file when the file is closed.
*/
{
    FILE *diskfile;
    char mode[4];
    int  status;

    /* first, create disk file for the compressed output */


    if ( !strcmp(filename, "-.gz") || !strcmp(filename, "stdout.gz") ||
         !strcmp(filename, "STDOUT.gz") )
    {
       /* special case: create uncompressed FITS file in memory, then
          compress it an write it out to 'stdout' when it is closed.  */

       diskfile = stdout;
    }
    else
    {
        /* normal case: create disk file for the compressed output */

        strcpy(mode, "w+b");    /* create file with read-write */

        diskfile = fopen(filename, "r"); /* does file already exist? */

        if (diskfile)
        {
            fclose(diskfile);         /* close file and exit with error */
            return(FILE_NOT_CREATED); 
        }

#if MACHINE == ALPHAVMS || MACHINE == VAXVMS
        /* specify VMS record structure: fixed format, 2880 byte records */
        /* but force stream mode access to enable random I/O access      */
        diskfile = fopen(filename, mode, "rfm=fix", "mrs=2880", "ctx=stm"); 
#else
        diskfile = fopen(filename, mode); 
#endif

        if (!(diskfile))           /* couldn't create file */
        {
            return(FILE_NOT_CREATED); 
        }
    }

    /* now create temporary memory file */

    /* initially allocate 1 FITS block = 2880 bytes */
    status = mem_createmem(2880L, handle);

    if (status)
    {
        ffpmsg("failed to create empty memory file (mem_create_comp)");
        return(status);
    }

    memTable[*handle].fileptr = diskfile;

    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_openmem(void **buffptr,   /* I - address of memory pointer          */
                size_t *buffsize, /* I - size of buffer, in bytes           */
                size_t deltasize, /* I - increment for future realloc's     */
                void *(*memrealloc)(void *p, size_t newsize),  /* function  */
                int *handle)
/* 
  lowest level routine to open a pre-existing memory file.
*/
{
    int ii;

    *handle = -1;
    for (ii = 0; ii < NMAXFILES; ii++)  /* find empty slot in handle table */
    {
        if (memTable[ii].memaddrptr == 0)
        {
            *handle = ii;
            break;
        }
    }
    if (*handle == -1)
       return(TOO_MANY_FILES);    /* too many files opened */

    memTable[ii].memaddrptr = (char **) buffptr; /* pointer to start addres */
    memTable[ii].memsizeptr = buffsize;     /* allocated size of memory */
    memTable[ii].deltasize = deltasize;     /* suggested realloc increment */
    memTable[ii].fitsfilesize = *buffsize;  /* size of FITS file (upper limit) */
    memTable[ii].currentpos = 0;            /* at beginning of the file */
    memTable[ii].mem_realloc = memrealloc;  /* memory realloc function */
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_createmem(size_t msize, int *handle)
/* 
  lowest level routine to allocate a memory file.
*/
{
    int ii;

    *handle = -1;
    for (ii = 0; ii < NMAXFILES; ii++)  /* find empty slot in handle table */
    {
        if (memTable[ii].memaddrptr == 0)
        {
            *handle = ii;
            break;
        }
    }
    if (*handle == -1)
       return(TOO_MANY_FILES);    /* too many files opened */

    /* use the internally allocated memaddr and memsize variables */
    memTable[ii].memaddrptr = &memTable[ii].memaddr;
    memTable[ii].memsizeptr = &memTable[ii].memsize;

    /* allocate initial block of memory for the file */
    if (msize > 0)
    {
        memTable[ii].memaddr = (char *) malloc(msize); 
        if ( !(memTable[ii].memaddr) )
        {
            ffpmsg("malloc of initial memory failed (mem_createmem)");
            return(FILE_NOT_OPENED);
        }
    }

    /* set initial state of the file */
    memTable[ii].memsize = msize;
    memTable[ii].deltasize = 2880;
    memTable[ii].fitsfilesize = 0;
    memTable[ii].currentpos = 0;
    memTable[ii].mem_realloc = realloc;
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_truncate(int handle, LONGLONG filesize)
/*
  truncate the file to a new size
*/
{
    char *ptr;

    /* call the memory reallocation function, if defined */
    if ( memTable[handle].mem_realloc )
    {    /* explicit LONGLONG->size_t cast */
        ptr = (memTable[handle].mem_realloc)(
                                *(memTable[handle].memaddrptr),
                                 (size_t) filesize);
        if (!ptr)
        {
            ffpmsg("Failed to reallocate memory (mem_truncate)");
            return(MEMORY_ALLOCATION);
        }

        /* if allocated more memory, initialize it to zero */
        if ( filesize > *(memTable[handle].memsizeptr) )
        {
             memset(ptr + *(memTable[handle].memsizeptr),
                    0,
                ((size_t) filesize) - *(memTable[handle].memsizeptr) );
        }

        *(memTable[handle].memaddrptr) = ptr;
        *(memTable[handle].memsizeptr) = (size_t) (filesize);
    }

    memTable[handle].currentpos = filesize;
    memTable[handle].fitsfilesize = filesize;
    return(0);
}
/*--------------------------------------------------------------------------*/
int stdin_checkfile(char *urltype, char *infile, char *outfile)
/*
   do any special case checking when opening a file on the stdin stream
*/
{
    if (strlen(outfile))
    {
        stdin_outfile[0] = '\0';
        strncat(stdin_outfile,outfile,FLEN_FILENAME-1); /* an output file is specified */
	strcpy(urltype,"stdinfile://");
    }
    else
        *stdin_outfile = '\0';  /* no output file was specified */

    return(0);
}
/*--------------------------------------------------------------------------*/
int stdin_open(char *filename, int rwmode, int *handle)
/*
  open a FITS file from the stdin file stream by copying it into memory
  The file name is ignored in this case.
*/
{
    int status;
    char cbuff;

    if (*stdin_outfile)
    {
      /* copy the stdin stream to the specified disk file then open the file */

      /* Create the output file */
      status =  file_create(stdin_outfile,handle);

      if (status)
      {
        ffpmsg("Unable to create output file to copy stdin (stdin_open):");
        ffpmsg(stdin_outfile);
        return(status);
      }
 
      /* copy the whole stdin stream to the file */
      status = stdin2file(*handle);
      file_close(*handle);

      if (status)
      {
        ffpmsg("failed to copy stdin to file (stdin_open)");
        ffpmsg(stdin_outfile);
        return(status);
      }

      /* reopen file with proper rwmode attribute */
      status = file_open(stdin_outfile, rwmode, handle);
    }
    else
    {
   
      /* get the first character, then put it back */
      cbuff = fgetc(stdin);
      ungetc(cbuff, stdin);
    
      /* compressed files begin with 037 or 'P' */
      if (cbuff == 31 || cbuff == 75)
      {
         /* looks like the input stream is compressed */
         status = mem_compress_stdin_open(filename, rwmode, handle);
	 
      }
      else
      {
        /* copy the stdin stream into memory then open file in memory */

        if (rwmode != READONLY)
        {
          ffpmsg("cannot open stdin with WRITE access");
          return(READONLY_FILE);
        }

        status = mem_createmem(2880L, handle);

        if (status)
        {
          ffpmsg("failed to create empty memory file (stdin_open)");
          return(status);
        }
 
        /* copy the whole stdin stream into memory */
        status = stdin2mem(*handle);

        if (status)
        {
          ffpmsg("failed to copy stdin into memory (stdin_open)");
          free(memTable[*handle].memaddr);
        }
      }
    }

    return(status);
}
/*--------------------------------------------------------------------------*/
int stdin2mem(int hd)  /* handle number */
/*
  Copy the stdin stream into memory.  Fill whatever amount of memory
  has already been allocated, then realloc more memory if necessary.
*/
{
    size_t nread, memsize, delta;
    LONGLONG filesize;
    char *memptr;
    char simple[] = "SIMPLE";
    int c, ii, jj;

    memptr = *memTable[hd].memaddrptr;
    memsize = *memTable[hd].memsizeptr;
    delta = memTable[hd].deltasize;

    filesize = 0;
    ii = 0;

    for(jj = 0; (c = fgetc(stdin)) != EOF && jj < 2000; jj++)
    {
       /* Skip over any garbage at the beginning of the stdin stream by */
       /* reading 1 char at a time, looking for 'S', 'I', 'M', 'P', 'L', 'E' */
       /* Give up if not found in the first 2000 characters */

       if (c == simple[ii])
       {
           ii++;
           if (ii == 6)   /* found the complete string? */
           {
              memcpy(memptr, simple, 6);  /* copy "SIMPLE" to buffer */
              filesize = 6;
              break;
           }
       }
       else
          ii = 0;  /* reset search to beginning of the string */
    }

   if (filesize == 0)
   {
       ffpmsg("Couldn't find the string 'SIMPLE' in the stdin stream.");
       ffpmsg("This does not look like a FITS file.");
       return(FILE_NOT_OPENED);
   }

    /* fill up the remainder of the initial memory allocation */
    nread = fread(memptr + 6, 1, memsize - 6, stdin);
    nread += 6;  /* add in the 6 characters in 'SIMPLE' */

    if (nread < memsize)    /* reached the end? */
    {
       memTable[hd].fitsfilesize = nread;
       return(0);
    }

    filesize = nread;

    while (1)
    {
        /* allocate memory for another FITS block */
        memptr = realloc(memptr, memsize + delta);

        if (!memptr)
        {
            ffpmsg("realloc failed while copying stdin (stdin2mem)");
            return(MEMORY_ALLOCATION);
        }
        memsize += delta;

        /* read another FITS block */
        nread = fread(memptr + filesize, 1, delta, stdin);

        filesize += nread;

        if (nread < delta)    /* reached the end? */
           break;
    }

     memTable[hd].fitsfilesize = filesize;
    *memTable[hd].memaddrptr = memptr;
    *memTable[hd].memsizeptr = memsize;

    return(0);
}
/*--------------------------------------------------------------------------*/
int stdin2file(int handle)  /* handle number */
/*
  Copy the stdin stream to a file.  .
*/
{
    size_t nread;
    char simple[] = "SIMPLE";
    int c, ii, jj, status;
    char recbuf[RECBUFLEN];

    ii = 0;
    for(jj = 0; (c = fgetc(stdin)) != EOF && jj < 2000; jj++)
    {
       /* Skip over any garbage at the beginning of the stdin stream by */
       /* reading 1 char at a time, looking for 'S', 'I', 'M', 'P', 'L', 'E' */
       /* Give up if not found in the first 2000 characters */

       if (c == simple[ii])
       {
           ii++;
           if (ii == 6)   /* found the complete string? */
           {
              memcpy(recbuf, simple, 6);  /* copy "SIMPLE" to buffer */
              break;
           }
       }
       else
          ii = 0;  /* reset search to beginning of the string */
    }

   if (ii != 6)
   {
       ffpmsg("Couldn't find the string 'SIMPLE' in the stdin stream");
       return(FILE_NOT_OPENED);
   }

    /* fill up the remainder of the buffer */
    nread = fread(recbuf + 6, 1, RECBUFLEN - 6, stdin);
    nread += 6;  /* add in the 6 characters in 'SIMPLE' */

    status = file_write(handle, recbuf, nread);
    if (status)
       return(status);

    /* copy the rest of stdin stream */
    while(0 != (nread = fread(recbuf,1,RECBUFLEN, stdin)))
    {
        status = file_write(handle, recbuf, nread);
        if (status)
           return(status);
    }

    return(status);
}
/*--------------------------------------------------------------------------*/
int stdout_close(int handle)
/*
  copy the memory file to stdout, then free the memory
*/
{
    int status = 0;

    /* copy from memory to standard out.  explicit LONGLONG->size_t cast */
    if(fwrite(memTable[handle].memaddr, 1,
              ((size_t) memTable[handle].fitsfilesize), stdout) !=
              (size_t) memTable[handle].fitsfilesize )
    {
                ffpmsg("failed to copy memory file to stdout (stdout_close)");
                status = WRITE_ERROR;
    }

    free( memTable[handle].memaddr );   /* free the memory */
    memTable[handle].memaddrptr = 0;
    memTable[handle].memaddr = 0;
    return(status);
}
/*--------------------------------------------------------------------------*/
int mem_compress_openrw(char *filename, int rwmode, int *hdl)
/*
  This routine opens the compressed diskfile and creates an empty memory
  buffer with an appropriate size, then calls mem_uncompress2mem. It allows
  the memory 'file' to be opened with READWRITE access.
*/
{
   return(mem_compress_open(filename, READONLY, hdl));  
}
/*--------------------------------------------------------------------------*/
int mem_compress_open(char *filename, int rwmode, int *hdl)
/*
  This routine opens the compressed diskfile and creates an empty memory
  buffer with an appropriate size, then calls mem_uncompress2mem.
*/
{
    FILE *diskfile;
    int status, estimated = 1;
    unsigned char buffer[4];
    size_t finalsize, filesize;
    LONGLONG llsize = 0;
    unsigned int modulosize;
    char *ptr;

    if (rwmode != READONLY)
    {
        ffpmsg(
  "cannot open compressed file with WRITE access (mem_compress_open)");
        ffpmsg(filename);
        return(READONLY_FILE);
    }

    /* open the compressed disk file */
    status = file_openfile(filename, READONLY, &diskfile);
    if (status)
    {
        ffpmsg("failed to open compressed disk file (compress_open)");
        ffpmsg(filename);
        return(status);
    }

    if (fread(buffer, 1, 2, diskfile) != 2)  /* read 2 bytes */
    {
        fclose(diskfile);
        return(READ_ERROR);
    }

    if (memcmp(buffer, "\037\213", 2) == 0)  /* GZIP */
    {
        /* the uncompressed file size is give at the end */
        /* of the file in the ISIZE field  (modulo 2^32) */

        fseek(diskfile, 0, 2);            /* move to end of file */
        filesize = ftell(diskfile);       /* position = size of file */
        fseek(diskfile, -4L, 1);          /* move back 4 bytes */
        fread(buffer, 1, 4L, diskfile);   /* read 4 bytes */

        /* have to worry about integer byte order */
	modulosize  = buffer[0];
	modulosize |= buffer[1] << 8;
	modulosize |= buffer[2] << 16;
	modulosize |= buffer[3] << 24;

/*
  the field ISIZE in the gzipped file header only stores 4 bytes and contains
  the uncompressed file size modulo 2^32.  If the uncompressed file size
  is less than the compressed file size (filesize), then one probably needs to
  add 2^32 = 4294967296 to the uncompressed file size, assuming that the gzip
  produces a compressed file that is smaller than the original file.

  But one must allow for the case of very small files, where the
  gzipped file may actually be larger then the original uncompressed file.
  Therefore, only perform the modulo 2^32 correction test if the compressed 
  file is greater than 10,000 bytes in size.  (Note: this threhold would
  fail only if the original file was greater than 2^32 bytes in size AND gzip 
  was able to compress it by more than a factor of 400,000 (!) which seems
  highly unlikely.)
  
  Also, obviously, this 2^32 modulo correction cannot be performed if the
  finalsize variable is only 32-bits long.  Typically, the 'size_t' integer
  type must be 8 bytes or larger in size to support data files that are 
  greater than 2 GB (2^31 bytes) in size.  
*/
        finalsize = modulosize;

        if (sizeof(size_t) > 4 && filesize > 10000) {
	    llsize = (LONGLONG) finalsize;  
	    /* use LONGLONG variable to suppress compiler warning */
            while (llsize <  (LONGLONG) filesize) llsize += 4294967296;

            finalsize = (size_t) llsize;
        }

        estimated = 0;  /* file size is known, not estimated */
    }
    else if (memcmp(buffer, "\120\113", 2) == 0)   /* PKZIP */
    {
        /* the uncompressed file size is give at byte 22 the file */

        fseek(diskfile, 22L, 0);            /* move to byte 22 */
        fread(buffer, 1, 4L, diskfile);   /* read 4 bytes */

        /* have to worry about integer byte order */
	modulosize  = buffer[0];
	modulosize |= buffer[1] << 8;
	modulosize |= buffer[2] << 16;
	modulosize |= buffer[3] << 24;
        finalsize = modulosize;

        estimated = 0;  /* file size is known, not estimated */
    }
    else if (memcmp(buffer, "\037\036", 2) == 0)  /* PACK */
        finalsize = 0;  /* for most methods we can't determine final size */
    else if (memcmp(buffer, "\037\235", 2) == 0)  /* LZW */
        finalsize = 0;  /* for most methods we can't determine final size */
    else if (memcmp(buffer, "\037\240", 2) == 0)  /* LZH */
        finalsize = 0;  /* for most methods we can't determine final size */
#if HAVE_BZIP2
    else if (memcmp(buffer, "BZ", 2) == 0)        /* BZip2 */
        finalsize = 0;  /* for most methods we can't determine final size */
#endif
    else
    {
        /* not a compressed file; this should never happen */
        fclose(diskfile);
        return(1);
    }

    if (finalsize == 0)  /* estimate uncompressed file size */
    {
            fseek(diskfile, 0, 2);   /* move to end of the compressed file */
            finalsize = ftell(diskfile);  /* position = size of file */
            finalsize = finalsize * 3;   /* assume factor of 3 compression */
    }

    fseek(diskfile, 0, 0);   /* move back to beginning of file */

    /* create a memory file big enough (hopefully) for the uncompressed file */
    status = mem_createmem(finalsize, hdl);

    if (status && estimated)
    {
        /* memory allocation failed, so try a smaller estimated size */
        finalsize = finalsize / 3;
        status = mem_createmem(finalsize, hdl);
    }

    if (status)
    {
        fclose(diskfile);
        ffpmsg("failed to create empty memory file (compress_open)");
        return(status);
    }

    /* uncompress file into memory */
    status = mem_uncompress2mem(filename, diskfile, *hdl);

    fclose(diskfile);

    if (status)
    {
        mem_close_free(*hdl);   /* free up the memory */
        ffpmsg("failed to uncompress file into memory (compress_open)");
        return(status);
    }

    /* if we allocated too much memory initially, then free it */
    if (*(memTable[*hdl].memsizeptr) > 
       (( (size_t) memTable[*hdl].fitsfilesize) + 256L) ) 
    {
        ptr = realloc(*(memTable[*hdl].memaddrptr), 
                     ((size_t) memTable[*hdl].fitsfilesize) );
        if (!ptr)
        {
            ffpmsg("Failed to reduce size of allocated memory (compress_open)");
            return(MEMORY_ALLOCATION);
        }

        *(memTable[*hdl].memaddrptr) = ptr;
        *(memTable[*hdl].memsizeptr) = (size_t) (memTable[*hdl].fitsfilesize);
    }

    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_compress_stdin_open(char *filename, int rwmode, int *hdl)
/*
  This routine reads the compressed input stream and creates an empty memory
  buffer, then calls mem_uncompress2mem.
*/
{
    int status;
    char *ptr;

    if (rwmode != READONLY)
    {
        ffpmsg(
  "cannot open compressed input stream with WRITE access (mem_compress_stdin_open)");
        return(READONLY_FILE);
    }
 
    /* create a memory file for the uncompressed file */
    status = mem_createmem(28800, hdl);

    if (status)
    {
        ffpmsg("failed to create empty memory file (compress_stdin_open)");
        return(status);
    }

    /* uncompress file into memory */
    status = mem_uncompress2mem(filename, stdin, *hdl);

    if (status)
    {
        mem_close_free(*hdl);   /* free up the memory */
        ffpmsg("failed to uncompress stdin into memory (compress_stdin_open)");
        return(status);
    }

    /* if we allocated too much memory initially, then free it */
    if (*(memTable[*hdl].memsizeptr) > 
       (( (size_t) memTable[*hdl].fitsfilesize) + 256L) ) 
    {
        ptr = realloc(*(memTable[*hdl].memaddrptr), 
                      ((size_t) memTable[*hdl].fitsfilesize) );
        if (!ptr)
        {
            ffpmsg("Failed to reduce size of allocated memory (compress_stdin_open)");
            return(MEMORY_ALLOCATION);
        }

        *(memTable[*hdl].memaddrptr) = ptr;
        *(memTable[*hdl].memsizeptr) = (size_t) (memTable[*hdl].fitsfilesize);
    }

    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_iraf_open(char *filename, int rwmode, int *hdl)
/*
  This routine creates an empty memory buffer, then calls iraf2mem to
  open the IRAF disk file and convert it to a FITS file in memeory.
*/
{
    int status;
    size_t filesize = 0;

    /* create a memory file with size = 0 for the FITS converted IRAF file */
    status = mem_createmem(filesize, hdl);
    if (status)
    {
        ffpmsg("failed to create empty memory file (mem_iraf_open)");
        return(status);
    }

    /* convert the iraf file into a FITS file in memory */
    status = iraf2mem(filename, memTable[*hdl].memaddrptr,
                      memTable[*hdl].memsizeptr, &filesize, &status);

    if (status)
    {
        mem_close_free(*hdl);   /* free up the memory */
        ffpmsg("failed to convert IRAF file into memory (mem_iraf_open)");
        return(status);
    }

    memTable[*hdl].currentpos = 0;           /* save starting position */
    memTable[*hdl].fitsfilesize=filesize;   /* and initial file size  */

    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_rawfile_open(char *filename, int rwmode, int *hdl)
/*
  This routine creates an empty memory buffer, writes a minimal
  image header, then copies the image data from the raw file into
  memory.  It will byteswap the pixel values if the raw array
  is in little endian byte order.
*/
{
    FILE *diskfile;
    fitsfile *fptr;
    short *sptr;
    int status, endian, datatype, bytePerPix, naxis;
    long dim[5] = {1,1,1,1,1}, ii, nvals, offset = 0;
    size_t filesize = 0, datasize;
    char rootfile[FLEN_FILENAME], *cptr = 0, *cptr2 = 0;
    void *ptr;

    if (rwmode != READONLY)
    {
        ffpmsg(
  "cannot open raw binary file with WRITE access (mem_rawfile_open)");
        ffpmsg(filename);
        return(READONLY_FILE);
    }

    cptr = strchr(filename, '[');   /* search for opening bracket [ */

    if (!cptr)
    {
        ffpmsg("binary file name missing '[' character (mem_rawfile_open)");
        ffpmsg(filename);
        return(URL_PARSE_ERROR);
    }

    *rootfile = '\0';
    strncat(rootfile, filename, cptr - filename);  /* store the rootname */

    cptr++;

    while (*cptr == ' ')
       cptr++;    /* skip leading blanks */

    /* Get the Data Type of the Image */

    if (*cptr == 'b' || *cptr == 'B')
    {
      datatype = BYTE_IMG;
      bytePerPix = 1;
    }
    else if (*cptr == 'i' || *cptr == 'I')
    {
      datatype = SHORT_IMG;
      bytePerPix = 2;
    }
    else if (*cptr == 'u' || *cptr == 'U')
    {
      datatype = USHORT_IMG;
      bytePerPix = 2;

    }
    else if (*cptr == 'j' || *cptr == 'J')
    {
      datatype = LONG_IMG;
      bytePerPix = 4;
    }  
    else if (*cptr == 'r' || *cptr == 'R' || *cptr == 'f' || *cptr == 'F')
    {
      datatype = FLOAT_IMG;
      bytePerPix = 4;
    }    
    else if (*cptr == 'd' || *cptr == 'D')
    {
      datatype = DOUBLE_IMG;
      bytePerPix = 8;
    }
    else
    {
        ffpmsg("error in raw binary file datatype (mem_rawfile_open)");
        ffpmsg(filename);
        return(URL_PARSE_ERROR);
    }

    cptr++;

    /* get Endian: Big or Little; default is same as the local machine */
    
    if (*cptr == 'b' || *cptr == 'B')
    {
        endian = 0;
        cptr++;
    }
    else if (*cptr == 'l' || *cptr == 'L')
    {
        endian = 1;
        cptr++;
    }
    else
        endian = BYTESWAPPED; /* byteswapped machines are little endian */

    /* read each dimension (up to 5) */

    naxis = 1;
    dim[0] = strtol(cptr, &cptr2, 10);
    
    if (cptr2 && *cptr2 == ',')
    {
      naxis = 2;
      dim[1] = strtol(cptr2+1, &cptr, 10);

      if (cptr && *cptr == ',')
      {
        naxis = 3;
        dim[2] = strtol(cptr+1, &cptr2, 10);

        if (cptr2 && *cptr2 == ',')
        {
          naxis = 4;
          dim[3] = strtol(cptr2+1, &cptr, 10);

          if (cptr && *cptr == ',')
            naxis = 5;
            dim[4] = strtol(cptr+1, &cptr2, 10);
        }
      }
    }

    cptr = maxvalue(cptr, cptr2);

    if (*cptr == ':')   /* read starting offset value */
        offset = strtol(cptr+1, 0, 10);

    nvals = dim[0] * dim[1] * dim[2] * dim[3] * dim[4];
    datasize = nvals * bytePerPix;
    filesize = nvals * bytePerPix + 2880;
    filesize = ((filesize - 1) / 2880 + 1) * 2880; 

    /* open the raw binary disk file */
    status = file_openfile(rootfile, READONLY, &diskfile);
    if (status)
    {
        ffpmsg("failed to open raw  binary file (mem_rawfile_open)");
        ffpmsg(rootfile);
        return(status);
    }

    /* create a memory file with corrct size for the FITS converted raw file */
    status = mem_createmem(filesize, hdl);
    if (status)
    {
        ffpmsg("failed to create memory file (mem_rawfile_open)");
        fclose(diskfile);
        return(status);
    }

    /* open this piece of memory as a new FITS file */
    ffimem(&fptr, (void **) memTable[*hdl].memaddrptr, &filesize, 0, 0, &status);

    /* write the required header keywords */
    ffcrim(fptr, datatype, naxis, dim, &status);

    /* close the FITS file, but keep the memory allocated */
    ffclos(fptr, &status);

    if (status > 0)
    {
        ffpmsg("failed to write basic image header (mem_rawfile_open)");
        fclose(diskfile);
        mem_close_free(*hdl);   /* free up the memory */
        return(status);
    }

    if (offset > 0)
       fseek(diskfile, offset, 0);   /* offset to start of the data */

    /* read the raw data into memory */
    ptr = *memTable[*hdl].memaddrptr + 2880;

    if (fread((char *) ptr, 1, datasize, diskfile) != datasize)
      status = READ_ERROR;

    fclose(diskfile);  /* close the raw binary disk file */

    if (status)
    {
        mem_close_free(*hdl);   /* free up the memory */
        ffpmsg("failed to copy raw file data into memory (mem_rawfile_open)");
        return(status);
    }

    if (datatype == USHORT_IMG)  /* have to subtract 32768 from each unsigned */
    {                            /* value to conform to FITS convention. More */
                                 /* efficient way to do this is to just flip  */
                                 /* the most significant bit.                 */

      sptr = (short *) ptr;

      if (endian == BYTESWAPPED)  /* working with native format */
      {
        for (ii = 0; ii < nvals; ii++, sptr++)
        {
          *sptr =  ( *sptr ) ^ 0x8000;
        }
      }
      else  /* pixels are byteswapped WRT the native format */
      {
        for (ii = 0; ii < nvals; ii++, sptr++)
        {
          *sptr =  ( *sptr ) ^ 0x80;
        }
      }
    }

    if (endian)  /* swap the bytes if array is in little endian byte order */
    {
      if (datatype == SHORT_IMG || datatype == USHORT_IMG)
      {
        ffswap2( (short *) ptr, nvals);
      }
      else if (datatype == LONG_IMG || datatype == FLOAT_IMG)
      {
        ffswap4( (INT32BIT *) ptr, nvals);
      }

      else if (datatype == DOUBLE_IMG)
      {
        ffswap8( (double *) ptr, nvals);
      }
    }

    memTable[*hdl].currentpos = 0;           /* save starting position */
    memTable[*hdl].fitsfilesize=filesize;    /* and initial file size  */

    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_uncompress2mem(char *filename, FILE *diskfile, int hdl)
{
/*
  lower level routine to uncompress a file into memory.  The file
  has already been opened and the memory buffer has been allocated.
*/

  size_t finalsize;
  int status;
  /* uncompress file into memory */
  status = 0;

    if (strstr(filename, ".Z")) {
         zuncompress2mem(filename, diskfile,
		 memTable[hdl].memaddrptr,   /* pointer to memory address */
		 memTable[hdl].memsizeptr,   /* pointer to size of memory */
		 realloc,                     /* reallocation function */
		 &finalsize, &status);        /* returned file size nd status*/
#if HAVE_BZIP2
    } else if (strstr(filename, ".bz2")) {
        bzip2uncompress2mem(filename, diskfile, hdl, &finalsize, &status);
#endif
    } else {
         uncompress2mem(filename, diskfile,
		 memTable[hdl].memaddrptr,   /* pointer to memory address */
		 memTable[hdl].memsizeptr,   /* pointer to size of memory */
		 realloc,                     /* reallocation function */
		 &finalsize, &status);        /* returned file size nd status*/
    } 

  memTable[hdl].currentpos = 0;           /* save starting position */
  memTable[hdl].fitsfilesize=finalsize;   /* and initial file size  */
  return status;
}
/*--------------------------------------------------------------------------*/
int mem_size(int handle, LONGLONG *filesize)
/*
  return the size of the file; only called when the file is first opened
*/
{
    *filesize = memTable[handle].fitsfilesize;
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_close_free(int handle)
/*
  close the file and free the memory.
*/
{
    free( *(memTable[handle].memaddrptr) );

    memTable[handle].memaddrptr = 0;
    memTable[handle].memaddr = 0;
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_close_keep(int handle)
/*
  close the memory file but do not free the memory.
*/
{
    memTable[handle].memaddrptr = 0;
    memTable[handle].memaddr = 0;
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_close_comp(int handle)
/*
  compress the memory file, writing it out to the fileptr (which might
  be stdout)
*/
{
    int status = 0;
    size_t compsize;

    /* compress file in  memory to a .gz disk file */

    if(compress2file_from_mem(memTable[handle].memaddr,
              (size_t) (memTable[handle].fitsfilesize), 
              memTable[handle].fileptr,
              &compsize, &status ) )
    {
            ffpmsg("failed to copy memory file to file (mem_close_comp)");
            status = WRITE_ERROR;
    }

    free( memTable[handle].memaddr );   /* free the memory */
    memTable[handle].memaddrptr = 0;
    memTable[handle].memaddr = 0;

    /* close the compressed disk file (except if it is 'stdout' */
    if (memTable[handle].fileptr != stdout)
        fclose(memTable[handle].fileptr);

    return(status);
}
/*--------------------------------------------------------------------------*/
int mem_seek(int handle, LONGLONG offset)
/*
  seek to position relative to start of the file.
*/
{
    if (offset >  memTable[handle].fitsfilesize )
        return(END_OF_FILE);

    memTable[handle].currentpos = offset;
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_read(int hdl, void *buffer, long nbytes)
/*
  read bytes from the current position in the file
*/
{
    if (memTable[hdl].currentpos + nbytes > memTable[hdl].fitsfilesize)
        return(END_OF_FILE);

    memcpy(buffer,
           *(memTable[hdl].memaddrptr) + memTable[hdl].currentpos,
           nbytes);

    memTable[hdl].currentpos += nbytes;
    return(0);
}
/*--------------------------------------------------------------------------*/
int mem_write(int hdl, void *buffer, long nbytes)
/*
  write bytes at the current position in the file
*/
{
    size_t newsize;
    char *ptr;

    if ((size_t) (memTable[hdl].currentpos + nbytes) > 
         *(memTable[hdl].memsizeptr) )
    {
               
        if (!(memTable[hdl].mem_realloc))
        {
            ffpmsg("realloc function not defined (mem_write)");
            return(WRITE_ERROR);
        }

        /*
          Attempt to reallocate additional memory:
          the memory buffer size is incremented by the larger of:
             1 FITS block (2880 bytes) or
             the defined 'deltasize' parameter
         */

        newsize = maxvalue( (size_t)
            (((memTable[hdl].currentpos + nbytes - 1) / 2880) + 1) * 2880,
            *(memTable[hdl].memsizeptr) + memTable[hdl].deltasize);

        /* call the realloc function */
        ptr = (memTable[hdl].mem_realloc)(
                                    *(memTable[hdl].memaddrptr),
                                     newsize);
        if (!ptr)
        {
            ffpmsg("Failed to reallocate memory (mem_write)");
            return(MEMORY_ALLOCATION);
        }

        *(memTable[hdl].memaddrptr) = ptr;
        *(memTable[hdl].memsizeptr) = newsize;
    }

    /* now copy the bytes from the buffer into memory */
    memcpy( *(memTable[hdl].memaddrptr) + memTable[hdl].currentpos,
             buffer,
             nbytes);

    memTable[hdl].currentpos += nbytes;
    memTable[hdl].fitsfilesize =
               maxvalue(memTable[hdl].fitsfilesize,
                        memTable[hdl].currentpos);
    return(0);
}


#if HAVE_BZIP2
void bzip2uncompress2mem(char *filename, FILE *diskfile, int hdl,
                        size_t* filesize, int* status) {
    BZFILE* b;
    int  bzerror;
    char buf[8192];
    size_t total_read = 0;
    char* errormsg = NULL;

    *filesize = 0;
    *status = 0;
    b = BZ2_bzReadOpen(&bzerror, diskfile, 0, 0, NULL, 0);
    if (bzerror != BZ_OK) {
        BZ2_bzReadClose(&bzerror, b);
        if (bzerror == BZ_MEM_ERROR)
            ffpmsg("failed to open a bzip2 file: out of memory\n");
        else if (bzerror == BZ_CONFIG_ERROR)
            ffpmsg("failed to open a bzip2 file: miscompiled bzip2 library\n");
        else if (bzerror == BZ_IO_ERROR)
            ffpmsg("failed to open a bzip2 file: I/O error");
        else
            ffpmsg("failed to open a bzip2 file");
        *status = READ_ERROR;
        return;
    }
    bzerror = BZ_OK;
    while (bzerror == BZ_OK) {
        int nread;
        nread = BZ2_bzRead(&bzerror, b, buf, sizeof(buf));
        if (bzerror == BZ_OK || bzerror == BZ_STREAM_END) {
            *status = mem_write(hdl, buf, nread);
            if (*status) {
                BZ2_bzReadClose(&bzerror, b);
                if (*status == MEMORY_ALLOCATION)
                    ffpmsg("Failed to reallocate memory while uncompressing bzip2 file");
                return;
            }
            total_read += nread;
        } else {
            if (bzerror == BZ_IO_ERROR)
                errormsg = "failed to read bzip2 file: I/O error";
            else if (bzerror == BZ_UNEXPECTED_EOF)
                errormsg = "failed to read bzip2 file: unexpected end-of-file";
            else if (bzerror == BZ_DATA_ERROR)
                errormsg = "failed to read bzip2 file: data integrity error";
            else if (bzerror == BZ_MEM_ERROR)
                errormsg = "failed to read bzip2 file: insufficient memory";
        }
    }
    BZ2_bzReadClose(&bzerror, b);
    if (bzerror != BZ_OK) {
        if (errormsg)
            ffpmsg(errormsg);
        else
            ffpmsg("failure closing bzip2 file after reading\n");
        *status = READ_ERROR;
        return;
    }
    *filesize = total_read;
}
#endif
