#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "zlib.h"  

#define GZBUFSIZE 115200    /* 40 FITS blocks */
#define BUFFINCR   28800    /* 10 FITS blocks */

/* prototype for the following functions */
int uncompress2mem(char *filename, 
             FILE *diskfile, 
             char **buffptr, 
             size_t *buffsize, 
             void *(*mem_realloc)(void *p, size_t newsize),
             size_t *filesize,
             int *status);

int uncompress2mem_from_mem(                                                
             char *inmemptr,     
             size_t inmemsize, 
             char **buffptr,  
             size_t *buffsize,  
             void *(*mem_realloc)(void *p, size_t newsize), 
             size_t *filesize,  
             int *status);

int uncompress2file(char *filename, 
             FILE *indiskfile, 
             FILE *outdiskfile, 
             int *status);


int compress2mem_from_mem(                                                
             char *inmemptr,     
             size_t inmemsize, 
             char **buffptr,  
             size_t *buffsize,  
             void *(*mem_realloc)(void *p, size_t newsize), 
             size_t *filesize,  
             int *status);

int compress2file_from_mem(                                                
             char *inmemptr,     
             size_t inmemsize, 
             FILE *outdiskfile, 
             size_t *filesize,   /* O - size of file, in bytes              */
             int *status);


/*--------------------------------------------------------------------------*/
int uncompress2mem(char *filename,  /* name of input file                 */
             FILE *diskfile,     /* I - file pointer                        */
             char **buffptr,   /* IO - memory pointer                     */
             size_t *buffsize,   /* IO - size of buffer, in bytes           */
             void *(*mem_realloc)(void *p, size_t newsize), /* function     */
             size_t *filesize,   /* O - size of file, in bytes              */
             int *status)        /* IO - error status                       */

/*
  Uncompress the disk file into memory.  Fill whatever amount of memory has
  already been allocated, then realloc more memory, using the supplied
  input function, if necessary.
*/
{
    int err, len;
    char *filebuff;
    z_stream d_stream;   /* decompression stream */
    /* Input args buffptr and buffsize may refer to a block of memory
        larger than the 2^32 4 byte limit.  If so, must be broken
        up into "pages" when assigned to d_stream.  
        (d_stream.avail_out is a uInt type, which might be smaller
        than buffsize's size_t type.)
    */
    const uLong nPages = (uLong)(*buffsize)/(uLong)UINT_MAX;
    uLong iPage=0;
    uInt outbuffsize = (nPages > 0) ? UINT_MAX : (uInt)(*buffsize);
    

    if (*status > 0) 
        return(*status); 

    /* Allocate memory to hold compressed bytes read from the file. */
    filebuff = (char*)malloc(GZBUFSIZE);
    if (!filebuff) return(*status = 113); /* memory error */

    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;
    d_stream.next_out = (unsigned char*) *buffptr;
    d_stream.avail_out = outbuffsize;

    /* Initialize the decompression.  The argument (15+16) tells the
       decompressor that we are to use the gzip algorithm */

    err = inflateInit2(&d_stream, (15+16));
    if (err != Z_OK) return(*status = 414);

    /* loop through the file, reading a buffer and uncompressing it */
    for (;;)
    {
        len = fread(filebuff, 1, GZBUFSIZE, diskfile);
	if (ferror(diskfile)) {
              inflateEnd(&d_stream);
              free(filebuff);
              return(*status = 414);
	}

        if (len == 0) break;  /* no more data */

        d_stream.next_in = (unsigned char*)filebuff;
        d_stream.avail_in = len;

        for (;;) {
            /* uncompress as much of the input as will fit in the output */
            err = inflate(&d_stream, Z_NO_FLUSH);

            if (err == Z_STREAM_END ) { /* We reached the end of the input */
	        break; 
            } else if (err == Z_OK ) { 

                if (!d_stream.avail_in) break; /* need more input */
		
                /* need more space in output buffer */
                /* First check if more memory is available above the
                    4Gb limit in the originally input buffptr array */
                if (iPage < nPages)
                {
                   ++iPage;
                   d_stream.next_out = (unsigned char*)(*buffptr + iPage*(uLong)UINT_MAX);
                   if (iPage < nPages)
                      d_stream.avail_out = UINT_MAX;
                   else
                      d_stream.avail_out = (uInt)((uLong)(*buffsize) % (uLong)UINT_MAX);
                }
                else if (mem_realloc) {   
                    *buffptr = mem_realloc(*buffptr,*buffsize + BUFFINCR);
                    if (*buffptr == NULL){
                        inflateEnd(&d_stream);
                        free(filebuff);
                        return(*status = 414);  /* memory allocation failed */
                    }

                    d_stream.avail_out = BUFFINCR;
                    d_stream.next_out = (unsigned char*) (*buffptr + *buffsize);
                    *buffsize = *buffsize + BUFFINCR;
                } else  { /* error: no realloc function available */
                    inflateEnd(&d_stream);
                    free(filebuff);
                    return(*status = 414);
                }
            } else {  /* some other error */
                inflateEnd(&d_stream);
                free(filebuff);
                return(*status = 414);
            }
        }
	
	if (feof(diskfile))  break;
/*     
        These settings for next_out and avail_out appear to be redundant,
        as the inflate() function should already be re-setting these.
        For case where *buffsize < 4Gb this did not matter, but for
        > 4Gb it would produce the wrong value in the avail_out assignment.
        (C. Gordon Jul 2016)
        d_stream.next_out = (unsigned char*) (*buffptr + d_stream.total_out);
        d_stream.avail_out = *buffsize - d_stream.total_out;
*/    }

    /* Set the output file size to be the total output data */
    *filesize = d_stream.total_out;
    
    free(filebuff); /* free temporary output data buffer */
    
    err = inflateEnd(&d_stream); /* End the decompression */
    if (err != Z_OK) return(*status = 414);
  
    return(*status);
}
/*--------------------------------------------------------------------------*/
int uncompress2mem_from_mem(                                                
             char *inmemptr,     /* I - memory pointer to compressed bytes */
             size_t inmemsize,   /* I - size of input compressed file      */
             char **buffptr,   /* IO - memory pointer                      */
             size_t *buffsize,   /* IO - size of buffer, in bytes           */
             void *(*mem_realloc)(void *p, size_t newsize), /* function     */
             size_t *filesize,   /* O - size of file, in bytes              */
             int *status)        /* IO - error status                       */

/*
  Uncompress the file in memory into memory.  Fill whatever amount of memory has
  already been allocated, then realloc more memory, using the supplied
  input function, if necessary.
*/
{
    int err; 
    z_stream d_stream;   /* decompression stream */

    if (*status > 0) 
        return(*status); 

    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;

    /* Initialize the decompression.  The argument (15+16) tells the
       decompressor that we are to use the gzip algorithm */
    err = inflateInit2(&d_stream, (15+16));
    if (err != Z_OK) return(*status = 414);

    d_stream.next_in = (unsigned char*)inmemptr;
    d_stream.avail_in = inmemsize;

    d_stream.next_out = (unsigned char*) *buffptr;
    d_stream.avail_out = *buffsize;

    for (;;) {
        /* uncompress as much of the input as will fit in the output */
        err = inflate(&d_stream, Z_NO_FLUSH);

        if (err == Z_STREAM_END) { /* We reached the end of the input */
	    break; 
        } else if (err == Z_OK ) { /* need more space in output buffer */

            if (mem_realloc) {   
                *buffptr = mem_realloc(*buffptr,*buffsize + BUFFINCR);
                if (*buffptr == NULL){
                    inflateEnd(&d_stream);
                    return(*status = 414);  /* memory allocation failed */
                }

                d_stream.avail_out = BUFFINCR;
                d_stream.next_out = (unsigned char*) (*buffptr + *buffsize);
                *buffsize = *buffsize + BUFFINCR;

            } else  { /* error: no realloc function available */
                inflateEnd(&d_stream);
                return(*status = 414);
            }
        } else {  /* some other error */
            inflateEnd(&d_stream);
            return(*status = 414);
        }
    }

    /* Set the output file size to be the total output data */
    if (filesize) *filesize = d_stream.total_out;

    /* End the decompression */
    err = inflateEnd(&d_stream);

    if (err != Z_OK) return(*status = 414);
    
    return(*status);
}
/*--------------------------------------------------------------------------*/
int uncompress2file(char *filename,  /* name of input file                  */
             FILE *indiskfile,     /* I - input file pointer                */
             FILE *outdiskfile,    /* I - output file pointer               */
             int *status)        /* IO - error status                       */
/*
  Uncompress the file into another file. 
*/
{
    int err, len;
    unsigned long bytes_out = 0;
    char *infilebuff, *outfilebuff;
    z_stream d_stream;   /* decompression stream */

    if (*status > 0) 
        return(*status); 

    /* Allocate buffers to hold compressed and uncompressed */
    infilebuff = (char*)malloc(GZBUFSIZE);
    if (!infilebuff) return(*status = 113); /* memory error */

    outfilebuff = (char*)malloc(GZBUFSIZE);
    if (!outfilebuff) return(*status = 113); /* memory error */

    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;

    d_stream.next_out = (unsigned char*) outfilebuff;
    d_stream.avail_out = GZBUFSIZE;

    /* Initialize the decompression.  The argument (15+16) tells the
       decompressor that we are to use the gzip algorithm */

    err = inflateInit2(&d_stream, (15+16));
    if (err != Z_OK) return(*status = 414);

    /* loop through the file, reading a buffer and uncompressing it */
    for (;;)
    {
        len = fread(infilebuff, 1, GZBUFSIZE, indiskfile);
	if (ferror(indiskfile)) {
              inflateEnd(&d_stream);
              free(infilebuff);
              free(outfilebuff);
              return(*status = 414);
	}

        if (len == 0) break;  /* no more data */

        d_stream.next_in = (unsigned char*)infilebuff;
        d_stream.avail_in = len;

        for (;;) {
            /* uncompress as much of the input as will fit in the output */
            err = inflate(&d_stream, Z_NO_FLUSH);

            if (err == Z_STREAM_END ) { /* We reached the end of the input */
	        break; 
            } else if (err == Z_OK ) { 

                if (!d_stream.avail_in) break; /* need more input */
		
                /* flush out the full output buffer */
                if ((int)fwrite(outfilebuff, 1, GZBUFSIZE, outdiskfile) != GZBUFSIZE) {
                    inflateEnd(&d_stream);
                    free(infilebuff);
                    free(outfilebuff);
                    return(*status = 414);
                }
                bytes_out += GZBUFSIZE;
                d_stream.next_out = (unsigned char*) outfilebuff;
                d_stream.avail_out = GZBUFSIZE;

            } else {  /* some other error */
                inflateEnd(&d_stream);
                free(infilebuff);
                free(outfilebuff);
                return(*status = 414);
            }
        }
	
	if (feof(indiskfile))  break;
    }

    /* write out any remaining bytes in the buffer */
    if (d_stream.total_out > bytes_out) {
        if ((int)fwrite(outfilebuff, 1, (d_stream.total_out - bytes_out), outdiskfile) 
	    != (d_stream.total_out - bytes_out)) {
            inflateEnd(&d_stream);
            free(infilebuff);
            free(outfilebuff);
            return(*status = 414);
        }
    }

    free(infilebuff); /* free temporary output data buffer */
    free(outfilebuff); /* free temporary output data buffer */

    err = inflateEnd(&d_stream); /* End the decompression */
    if (err != Z_OK) return(*status = 414);
  
    return(*status);
}
/*--------------------------------------------------------------------------*/
int compress2mem_from_mem(                                                
             char *inmemptr,     /* I - memory pointer to uncompressed bytes */
             size_t inmemsize,   /* I - size of input uncompressed file      */
             char **buffptr,   /* IO - memory pointer for compressed file    */
             size_t *buffsize,   /* IO - size of buffer, in bytes           */
             void *(*mem_realloc)(void *p, size_t newsize), /* function     */
             size_t *filesize,   /* O - size of file, in bytes              */
             int *status)        /* IO - error status                       */

/*
  Compress the file into memory.  Fill whatever amount of memory has
  already been allocated, then realloc more memory, using the supplied
  input function, if necessary.
*/
{
    int err;
    z_stream c_stream;  /* compression stream */

    if (*status > 0)
        return(*status);

    c_stream.zalloc = (alloc_func)0;
    c_stream.zfree = (free_func)0;
    c_stream.opaque = (voidpf)0;

    /* Initialize the compression.  The argument (15+16) tells the 
       compressor that we are to use the gzip algorythm.
       Also use Z_BEST_SPEED for maximum speed with very minor loss
       in compression factor. */
    err = deflateInit2(&c_stream, Z_BEST_SPEED, Z_DEFLATED,
                       (15+16), 8, Z_DEFAULT_STRATEGY);

    if (err != Z_OK) return(*status = 413);

    c_stream.next_in = (unsigned char*)inmemptr;
    c_stream.avail_in = inmemsize;

    c_stream.next_out = (unsigned char*) *buffptr;
    c_stream.avail_out = *buffsize;

    for (;;) {
        /* compress as much of the input as will fit in the output */
        err = deflate(&c_stream, Z_FINISH);

        if (err == Z_STREAM_END) {  /* We reached the end of the input */
	   break;
        } else if (err == Z_OK ) { /* need more space in output buffer */

            if (mem_realloc) {   
                *buffptr = mem_realloc(*buffptr,*buffsize + BUFFINCR);
                if (*buffptr == NULL){
                    deflateEnd(&c_stream);
                    return(*status = 413);  /* memory allocation failed */
                }

                c_stream.avail_out = BUFFINCR;
                c_stream.next_out = (unsigned char*) (*buffptr + *buffsize);
                *buffsize = *buffsize + BUFFINCR;

            } else  { /* error: no realloc function available */
                deflateEnd(&c_stream);
                return(*status = 413);
            }
        } else {  /* some other error */
            deflateEnd(&c_stream);
            return(*status = 413);
        }
    }

    /* Set the output file size to be the total output data */
    if (filesize) *filesize = c_stream.total_out;

    /* End the compression */
    err = deflateEnd(&c_stream);

    if (err != Z_OK) return(*status = 413);
     
    return(*status);
}
/*--------------------------------------------------------------------------*/
int compress2file_from_mem(                                                
             char *inmemptr,     /* I - memory pointer to uncompressed bytes */
             size_t inmemsize,   /* I - size of input uncompressed file      */
             FILE *outdiskfile, 
             size_t *filesize,   /* O - size of file, in bytes              */
             int *status)

/*
  Compress the memory file into disk file. 
*/
{
    int err;
    unsigned long bytes_out = 0;
    char  *outfilebuff;
    z_stream c_stream;  /* compression stream */

    if (*status > 0)
        return(*status);

    /* Allocate buffer to hold compressed bytes */
    outfilebuff = (char*)malloc(GZBUFSIZE);
    if (!outfilebuff) return(*status = 113); /* memory error */

    c_stream.zalloc = (alloc_func)0;
    c_stream.zfree = (free_func)0;
    c_stream.opaque = (voidpf)0;

    /* Initialize the compression.  The argument (15+16) tells the 
       compressor that we are to use the gzip algorythm.
       Also use Z_BEST_SPEED for maximum speed with very minor loss
       in compression factor. */
    err = deflateInit2(&c_stream, Z_BEST_SPEED, Z_DEFLATED,
                       (15+16), 8, Z_DEFAULT_STRATEGY);

    if (err != Z_OK) return(*status = 413);

    c_stream.next_in = (unsigned char*)inmemptr;
    c_stream.avail_in = inmemsize;

    c_stream.next_out = (unsigned char*) outfilebuff;
    c_stream.avail_out = GZBUFSIZE;

    for (;;) {
        /* compress as much of the input as will fit in the output */
        err = deflate(&c_stream, Z_FINISH);

        if (err == Z_STREAM_END) {  /* We reached the end of the input */
	   break;
        } else if (err == Z_OK ) { /* need more space in output buffer */

            /* flush out the full output buffer */
            if ((int)fwrite(outfilebuff, 1, GZBUFSIZE, outdiskfile) != GZBUFSIZE) {
                deflateEnd(&c_stream);
                free(outfilebuff);
                return(*status = 413);
            }
            bytes_out += GZBUFSIZE;
            c_stream.next_out = (unsigned char*) outfilebuff;
            c_stream.avail_out = GZBUFSIZE;


        } else {  /* some other error */
            deflateEnd(&c_stream);
            free(outfilebuff);
            return(*status = 413);
        }
    }

    /* write out any remaining bytes in the buffer */
    if (c_stream.total_out > bytes_out) {
        if ((int)fwrite(outfilebuff, 1, (c_stream.total_out - bytes_out), outdiskfile) 
	    != (c_stream.total_out - bytes_out)) {
            deflateEnd(&c_stream);
            free(outfilebuff);
            return(*status = 413);
        }
    }

    free(outfilebuff); /* free temporary output data buffer */

    /* Set the output file size to be the total output data */
    if (filesize) *filesize = c_stream.total_out;

    /* End the compression */
    err = deflateEnd(&c_stream);

    if (err != Z_OK) return(*status = 413);
     
    return(*status);
}
