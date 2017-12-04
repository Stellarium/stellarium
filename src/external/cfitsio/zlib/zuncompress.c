/* gzcompress.h -- definitions for the .Z decompression routine used in CFITSIO */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define get_char() get_byte()

/* gzip.h -- common declarations for all gzip modules  */

#define OF(args)  args
typedef void *voidp;

#define memzero(s, n)     memset ((voidp)(s), 0, (n))

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

/* private version of MIN function */
#define MINZIP(a,b) ((a) <= (b) ? (a) : (b))

/* Return codes from gzip */
#define OK      0
#define ERROR   1
#define COMPRESSED  1
#define DEFLATED    8
#define INBUFSIZ  0x8000    /* input buffer size */
#define INBUF_EXTRA  64     /* required by unlzw() */
#define OUTBUFSIZ  16384    /* output buffer size */
#define OUTBUF_EXTRA 2048   /* required by unlzw() */
#define DIST_BUFSIZE 0x8000 /* buffer for distances, see trees.c */
#define WSIZE 0x8000        /* window size--must be a power of two, and */
#define DECLARE(type, array, size)  type array[size]
#define tab_suffix window
#define tab_prefix prev    /* hash link (see deflate.c) */
#define head (prev+WSIZE)  /* hash head (see deflate.c) */
#define	LZW_MAGIC      "\037\235" /* Magic header for lzw files, 1F 9D */
#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf(0))

/* Diagnostic functions */
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)

/* lzw.h -- define the lzw functions. */

#ifndef BITS
#  define BITS 16
#endif
#define INIT_BITS 9              /* Initial number of bits per code */
#define BIT_MASK    0x1f /* Mask for 'number of compression bits' */
#define BLOCK_MODE  0x80
#define LZW_RESERVED 0x60 /* reserved bits */
#define	CLEAR  256       /* flush the dictionary */
#define FIRST  (CLEAR+1) /* first free entry */

/* prototypes */

#define local static
void ffpmsg(const char *err_message);

local int  fill_inbuf    OF((int eof_ok));
local void write_buf     OF((voidp buf, unsigned cnt));
local void error         OF((char *m));
local int unlzw  OF((FILE *in, FILE *out));

typedef int file_t;     /* Do not use stdio */

int (*work) OF((FILE *infile, FILE *outfile)) = unlzw; /* function to call */

local void error         OF((char *m));

		/* global buffers */

static DECLARE(uch, inbuf,  INBUFSIZ +INBUF_EXTRA);
static DECLARE(uch, outbuf, OUTBUFSIZ+OUTBUF_EXTRA);
static DECLARE(ush, d_buf,  DIST_BUFSIZE);
static DECLARE(uch, window, 2L*WSIZE);

#ifndef MAXSEG_64K
    static DECLARE(ush, tab_prefix, 1L<<BITS);
#else
    static DECLARE(ush, tab_prefix0, 1L<<(BITS-1));
    static DECLARE(ush, tab_prefix1, 1L<<(BITS-1));
#endif

		/* local variables */

/* 11/25/98: added 'static' to local variable definitions, to avoid */
/* conflict with external source files */

static int maxbits = BITS;   /* max bits per code for LZW */
static int method = DEFLATED;/* compression method */
static int exit_code = OK;   /* program exit code */
static int last_member;      /* set for .zip and .Z files */
static long bytes_in;             /* number of input bytes */
static long bytes_out;            /* number of output bytes */
static char ifname[128];          /* input file name */
static FILE *ifd;               /* input file descriptor */
static FILE *ofd;               /* output file descriptor */
static void **memptr;          /* memory location for uncompressed file */
static size_t *memsize;        /* size (bytes) of memory allocated for file */
void *(*realloc_fn)(void *p, size_t newsize);  /* reallocation function */
static unsigned insize;     /* valid bytes in inbuf */
static unsigned inptr;      /* index of next byte to be processed in inbuf */

/* prototype for the following functions */
int zuncompress2mem(char *filename, 
             FILE *diskfile, 
             char **buffptr, 
             size_t *buffsize, 
             void *(*mem_realloc)(void *p, size_t newsize),
             size_t *filesize,
             int *status);

/*--------------------------------------------------------------------------*/
int zuncompress2mem(char *filename,  /* name of input file                 */
             FILE *indiskfile,     /* I - file pointer                        */
             char **buffptr,   /* IO - memory pointer                     */
             size_t *buffsize,   /* IO - size of buffer, in bytes           */
             void *(*mem_realloc)(void *p, size_t newsize), /* function     */
             size_t *filesize,   /* O - size of file, in bytes              */
             int *status)        /* IO - error status                       */

/*
  Uncompress the file into memory.  Fill whatever amount of memory has
  already been allocated, then realloc more memory, using the supplied
  input function, if necessary.
*/
{
    char magic[2]; /* magic header */

    if (*status > 0)
        return(*status);

    /*  save input parameters into global variables */
    ifname[0] = '\0';
    strncat(ifname, filename, 127);
    ifd = indiskfile;
    memptr = (void **) buffptr;
    memsize = buffsize;
    realloc_fn = mem_realloc;

    /* clear input and output buffers */

    insize = inptr = 0;
    bytes_in = bytes_out = 0L;

    magic[0] = (char)get_byte();
    magic[1] = (char)get_byte();

    if (memcmp(magic, LZW_MAGIC, 2) != 0) {
      error("ERROR: input .Z file is in unrecognized compression format.\n");
      return(-1);
    }

    work = unlzw;
    method = COMPRESSED;
    last_member = 1;

    /* do the uncompression */
    if ((*work)(ifd, ofd) != OK) {
        method = -1; /* force cleanup */
        *status = 414;    /* report some sort of decompression error */
    }

    if (filesize)  *filesize = bytes_out;

    return(*status);
}
/*=========================================================================*/
/*=========================================================================*/
/* this marks the begining of the original file 'unlzw.c'                  */
/*=========================================================================*/
/*=========================================================================*/

/* unlzw.c -- decompress files in LZW format.
 * The code in this file is directly derived from the public domain 'compress'
 * written by Spencer Thomas, Joe Orost, James Woods, Jim McKie, Steve Davies,
 * Ken Turkowski, Dave Mack and Peter Jannesen.
 */

typedef	unsigned char	char_type;
typedef          long   code_int;
typedef unsigned long 	count_int;
typedef unsigned short	count_short;
typedef unsigned long 	cmp_code_int;

#define MAXCODE(n)	(1L << (n))
    
#ifndef	REGISTERS
#	define	REGISTERS	2
#endif
#define	REG1	
#define	REG2	
#define	REG3	
#define	REG4	
#define	REG5	
#define	REG6	
#define	REG7	
#define	REG8	
#define	REG9	
#define	REG10
#define	REG11	
#define	REG12	
#define	REG13
#define	REG14
#define	REG15
#define	REG16
#if REGISTERS >= 1
#	undef	REG1
#	define	REG1	register
#endif
#if REGISTERS >= 2
#	undef	REG2
#	define	REG2	register
#endif
#if REGISTERS >= 3
#	undef	REG3
#	define	REG3	register
#endif
#if REGISTERS >= 4
#	undef	REG4
#	define	REG4	register
#endif
#if REGISTERS >= 5
#	undef	REG5
#	define	REG5	register
#endif
#if REGISTERS >= 6
#	undef	REG6
#	define	REG6	register
#endif
#if REGISTERS >= 7
#	undef	REG7
#	define	REG7	register
#endif
#if REGISTERS >= 8
#	undef	REG8
#	define	REG8	register
#endif
#if REGISTERS >= 9
#	undef	REG9
#	define	REG9	register
#endif
#if REGISTERS >= 10
#	undef	REG10
#	define	REG10	register
#endif
#if REGISTERS >= 11
#	undef	REG11
#	define	REG11	register
#endif
#if REGISTERS >= 12
#	undef	REG12
#	define	REG12	register
#endif
#if REGISTERS >= 13
#	undef	REG13
#	define	REG13	register
#endif
#if REGISTERS >= 14
#	undef	REG14
#	define	REG14	register
#endif
#if REGISTERS >= 15
#	undef	REG15
#	define	REG15	register
#endif
#if REGISTERS >= 16
#	undef	REG16
#	define	REG16	register
#endif
    
#ifndef	BYTEORDER
#	define	BYTEORDER	0000
#endif
	
#ifndef	NOALLIGN
#	define	NOALLIGN	0
#endif


union	bytes {
    long  word;
    struct {
#if BYTEORDER == 4321
	char_type	b1;
	char_type	b2;
	char_type	b3;
	char_type	b4;
#else
#if BYTEORDER == 1234
	char_type	b4;
	char_type	b3;
	char_type	b2;
	char_type	b1;
#else
#	undef	BYTEORDER
	int  dummy;
#endif
#endif
    } bytes;
};

#if BYTEORDER == 4321 && NOALLIGN == 1
#  define input(b,o,c,n,m){ \
     (c) = (*(long *)(&(b)[(o)>>3])>>((o)&0x7))&(m); \
     (o) += (n); \
   }
#else
#  define input(b,o,c,n,m){ \
     REG1 char_type *p = &(b)[(o)>>3]; \
     (c) = ((((long)(p[0]))|((long)(p[1])<<8)| \
     ((long)(p[2])<<16))>>((o)&0x7))&(m); \
     (o) += (n); \
   }
#endif

#ifndef MAXSEG_64K
   /* DECLARE(ush, tab_prefix, (1<<BITS)); -- prefix code */
#  define tab_prefixof(i) tab_prefix[i]
#  define clear_tab_prefixof()	memzero(tab_prefix, 256);
#else
   /* DECLARE(ush, tab_prefix0, (1<<(BITS-1)); -- prefix for even codes */
   /* DECLARE(ush, tab_prefix1, (1<<(BITS-1)); -- prefix for odd  codes */
   ush *tab_prefix[2];
#  define tab_prefixof(i) tab_prefix[(i)&1][(i)>>1]
#  define clear_tab_prefixof()	\
      memzero(tab_prefix0, 128), \
      memzero(tab_prefix1, 128);
#endif
#define de_stack        ((char_type *)(&d_buf[DIST_BUFSIZE-1]))
#define tab_suffixof(i) tab_suffix[i]

int block_mode = BLOCK_MODE; /* block compress mode -C compatible with 2.0 */

/* ============================================================================
 * Decompress in to out.  This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.
 * IN assertions: the buffer inbuf contains already the beginning of
 *   the compressed data, from offsets iptr to insize-1 included.
 *   The magic header has already been checked and skipped.
 *   bytes_in and bytes_out have been initialized.
 */
local int unlzw(FILE *in, FILE *out) 
    /* input and output file descriptors */
{
    REG2   char_type  *stackp;
    REG3   code_int   code;
    REG4   int        finchar;
    REG5   code_int   oldcode;
    REG6   code_int   incode;
    REG7   long       inbits;
    REG8   long       posbits;
    REG9   int        outpos;
/*  REG10  int        insize; (global) */
    REG11  unsigned   bitmask;
    REG12  code_int   free_ent;
    REG13  code_int   maxcode;
    REG14  code_int   maxmaxcode;
    REG15  int        n_bits;
    REG16  int        rsize;
    
    ofd = out;

#ifdef MAXSEG_64K
    tab_prefix[0] = tab_prefix0;
    tab_prefix[1] = tab_prefix1;
#endif
    maxbits = get_byte();
    block_mode = maxbits & BLOCK_MODE;
    if ((maxbits & LZW_RESERVED) != 0) {
	error( "warning, unknown flags in unlzw decompression");
    }
    maxbits &= BIT_MASK;
    maxmaxcode = MAXCODE(maxbits);
    
    if (maxbits > BITS) {
	error("compressed with too many bits; cannot handle file");
	exit_code = ERROR;
	return ERROR;
    }
    rsize = insize;
    maxcode = MAXCODE(n_bits = INIT_BITS)-1;
    bitmask = (1<<n_bits)-1;
    oldcode = -1;
    finchar = 0;
    outpos = 0;
    posbits = inptr<<3;

    free_ent = ((block_mode) ? FIRST : 256);
    
    clear_tab_prefixof(); /* Initialize the first 256 entries in the table. */
    
    for (code = 255 ; code >= 0 ; --code) {
	tab_suffixof(code) = (char_type)code;
    }
    do {
	REG1 int i;
	int  e;
	int  o;
	
    resetbuf:
	e = insize-(o = (posbits>>3));
	
	for (i = 0 ; i < e ; ++i) {
	    inbuf[i] = inbuf[i+o];
	}
	insize = e;
	posbits = 0;
	
	if (insize < INBUF_EXTRA) {
/*  modified to use fread instead of read - WDP 10/22/97  */
/*	    if ((rsize = read(in, (char*)inbuf+insize, INBUFSIZ)) == EOF) { */

	    if ((rsize = fread((char*)inbuf+insize, 1, INBUFSIZ, in)) == EOF) {
		error("unexpected end of file");
	        exit_code = ERROR;
                return ERROR;
	    }
	    insize += rsize;
	    bytes_in += (ulg)rsize;
	}
	inbits = ((rsize != 0) ? ((long)insize - insize%n_bits)<<3 : 
		  ((long)insize<<3)-(n_bits-1));
	
	while (inbits > posbits) {
	    if (free_ent > maxcode) {
		posbits = ((posbits-1) +
			   ((n_bits<<3)-(posbits-1+(n_bits<<3))%(n_bits<<3)));
		++n_bits;
		if (n_bits == maxbits) {
		    maxcode = maxmaxcode;
		} else {
		    maxcode = MAXCODE(n_bits)-1;
		}
		bitmask = (1<<n_bits)-1;
		goto resetbuf;
	    }
	    input(inbuf,posbits,code,n_bits,bitmask);
	    Tracev((stderr, "%d ", code));

	    if (oldcode == -1) {
		if (code >= 256) {
                    error("corrupt input.");
	            exit_code = ERROR;
                    return ERROR;
                }

		outbuf[outpos++] = (char_type)(finchar = (int)(oldcode=code));
		continue;
	    }
	    if (code == CLEAR && block_mode) {
		clear_tab_prefixof();
		free_ent = FIRST - 1;
		posbits = ((posbits-1) +
			   ((n_bits<<3)-(posbits-1+(n_bits<<3))%(n_bits<<3)));
		maxcode = MAXCODE(n_bits = INIT_BITS)-1;
		bitmask = (1<<n_bits)-1;
		goto resetbuf;
	    }
	    incode = code;
	    stackp = de_stack;
	    
	    if (code >= free_ent) { /* Special case for KwKwK string. */
		if (code > free_ent) {
		    if (outpos > 0) {
			write_buf((char*)outbuf, outpos);
			bytes_out += (ulg)outpos;
		    }
		    error("corrupt input.");
	            exit_code = ERROR;
                    return ERROR;

		}
		*--stackp = (char_type)finchar;
		code = oldcode;
	    }

	    while ((cmp_code_int)code >= (cmp_code_int)256) {
		/* Generate output characters in reverse order */
		*--stackp = tab_suffixof(code);
		code = tab_prefixof(code);
	    }
	    *--stackp =	(char_type)(finchar = tab_suffixof(code));
	    
	    /* And put them out in forward order */
	    {
	/*	REG1 int	i;   already defined above (WDP) */
	    
		if (outpos+(i = (de_stack-stackp)) >= OUTBUFSIZ) {
		    do {
			if (i > OUTBUFSIZ-outpos) i = OUTBUFSIZ-outpos;

			if (i > 0) {
			    memcpy(outbuf+outpos, stackp, i);
			    outpos += i;
			}
			if (outpos >= OUTBUFSIZ) {
			    write_buf((char*)outbuf, outpos);
			    bytes_out += (ulg)outpos;
			    outpos = 0;
			}
			stackp+= i;
		    } while ((i = (de_stack-stackp)) > 0);
		} else {
		    memcpy(outbuf+outpos, stackp, i);
		    outpos += i;
		}
	    }

	    if ((code = free_ent) < maxmaxcode) { /* Generate the new entry. */

		tab_prefixof(code) = (unsigned short)oldcode;
		tab_suffixof(code) = (char_type)finchar;
		free_ent = code+1;
	    } 
	    oldcode = incode;	/* Remember previous code.	*/
	}
    } while (rsize != 0);
    
    if (outpos > 0) {
	write_buf((char*)outbuf, outpos);
	bytes_out += (ulg)outpos;
    }
    return OK;
}
/* ========================================================================*/
/* this marks the start of the code from 'util.c'  */

local int fill_inbuf(int eof_ok)
         /* set if EOF acceptable as a result */
{
    int len;

      /* Read as much as possible from file */
      insize = 0;
      do {
        len = fread((char*)inbuf+insize, 1, INBUFSIZ-insize, ifd);
        if (len == 0 || len == EOF) break;
	insize += len;
      } while (insize < INBUFSIZ);

    if (insize == 0) {
	if (eof_ok) return EOF;
	error("unexpected end of file");
        exit_code = ERROR;
        return ERROR;
    }

    bytes_in += (ulg)insize;
    inptr = 1;
    return inbuf[0];
}
/* =========================================================================== */
local void write_buf(voidp buf, unsigned cnt)
/*              copy buffer into memory; allocate more memory if required*/
{
    if (!realloc_fn)
    {
      /* append buffer to file */
      /* added 'unsigned' to get rid of compiler warning (WDP 1/1/99) */
      if ((unsigned long) fwrite(buf, 1, cnt, ofd) != cnt)
      {
          error
          ("failed to write buffer to uncompressed output file (write_buf)");
          exit_code = ERROR;
          return;
      }
    }
    else
    {
      /* get more memory if current buffer is too small */
      if (bytes_out + cnt > *memsize)
      {
        *memptr = realloc_fn(*memptr, bytes_out + cnt);
        *memsize = bytes_out + cnt;  /* new memory buffer size */

        if (!(*memptr))
        {
            error("malloc failed while uncompressing (write_buf)");
            exit_code = ERROR;
            return;
        }  
      }
      /* copy  into memory buffer */
      memcpy((char *) *memptr + bytes_out, (char *) buf, cnt);
    }
}
/* ======================================================================== */
local void error(char *m)
/*                Error handler */
{
    ffpmsg(ifname);
    ffpmsg(m);
}
