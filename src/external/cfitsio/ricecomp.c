/*
  The following code was written by Richard White at STScI and made
  available for use in CFITSIO in July 1999.  These routines were
  originally contained in 2 source files: rcomp.c and rdecomp.c,
  and the 'include' file now called ricecomp.h was originally called buffer.h.
*/

/*----------------------------------------------------------*/
/*                                                          */
/*    START OF SOURCE FILE ORIGINALLY CALLED rcomp.c        */
/*                                                          */
/*----------------------------------------------------------*/
/* @(#) rcomp.c 1.5 99/03/01 12:40:27 */
/* rcomp.c	Compress image line using
 *		(1) Difference of adjacent pixels
 *		(2) Rice algorithm coding
 *
 * Returns number of bytes written to code buffer or
 * -1 on failure
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * nonzero_count is lookup table giving number of bits in 8-bit values not including
 * leading zeros used in fits_rdecomp, fits_rdecomp_short and fits_rdecomp_byte
 */
static const int nonzero_count[256] = {
0, 
1, 
2, 2, 
3, 3, 3, 3, 
4, 4, 4, 4, 4, 4, 4, 4, 
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

typedef unsigned char Buffer_t;

typedef struct {
	int bitbuffer;		/* bit buffer			*/
	int bits_to_go;		/* bits to go in buffer		*/
	Buffer_t *start;	/* start of buffer		*/
	Buffer_t *current;	/* current position in buffer	*/
	Buffer_t *end;		/* end of buffer		*/
} Buffer;

#define putcbuf(c,mf) 	((*(mf->current)++ = c), 0)

#include "fitsio2.h"

static void start_outputing_bits(Buffer *buffer);
static int done_outputing_bits(Buffer *buffer);
static int output_nbits(Buffer *buffer, int bits, int n);

/*  only used for diagnoistics
static int case1, case2, case3;
int fits_get_case(int *c1, int*c2, int*c3) {

  *c1 = case1;
  *c2 = case2;
  *c3 = case3;
  return(0);
}
*/

/* this routine used to be called 'rcomp'  (WDP)  */
/*---------------------------------------------------------------------------*/

int fits_rcomp(int a[],		/* input array			*/
	  int nx,		/* number of input pixels	*/
	  unsigned char *c,	/* output buffer		*/
	  int clen,		/* max length of output		*/
	  int nblock)		/* coding block size		*/
{
Buffer bufmem, *buffer = &bufmem;
/* int bsize;  */
int i, j, thisblock;
int lastpix, nextpix, pdiff;
int v, fs, fsmask, top, fsmax, fsbits, bbits;
int lbitbuffer, lbits_to_go;
unsigned int psum;
double pixelsum, dpsum;
unsigned int *diff;

    /*
     * Original size of each pixel (bsize, bytes) and coding block
     * size (nblock, pixels)
     * Could make bsize a parameter to allow more efficient
     * compression of short & byte images.
     */
/*    bsize = 4;   */

/*    nblock = 32; now an input parameter*/
    /*
     * From bsize derive:
     * FSBITS = # bits required to store FS
     * FSMAX = maximum value for FS
     * BBITS = bits/pixel for direct coding
     */

/*
    switch (bsize) {
    case 1:
	fsbits = 3;
	fsmax = 6;
	break;
    case 2:
	fsbits = 4;
	fsmax = 14;
	break;
    case 4:
	fsbits = 5;
	fsmax = 25;
	break;
    default:
        ffpmsg("rdecomp: bsize must be 1, 2, or 4 bytes");
	return(-1);
    }
*/

    /* move out of switch block, to tweak performance */
    fsbits = 5;
    fsmax = 25;

    bbits = 1<<fsbits;

    /*
     * Set up buffer pointers
     */
    buffer->start = c;
    buffer->current = c;
    buffer->end = c+clen;
    buffer->bits_to_go = 8;
    /*
     * array for differences mapped to non-negative values
     */
    diff = (unsigned int *) malloc(nblock*sizeof(unsigned int));
    if (diff == (unsigned int *) NULL) {
        ffpmsg("fits_rcomp: insufficient memory");
	return(-1);
    }
    /*
     * Code in blocks of nblock pixels
     */
    start_outputing_bits(buffer);

    /* write out first int value to the first 4 bytes of the buffer */
    if (output_nbits(buffer, a[0], 32) == EOF) {
        ffpmsg("rice_encode: end of buffer");
        free(diff);
        return(-1);
    }

    lastpix = a[0];  /* the first difference will always be zero */

    thisblock = nblock;
    for (i=0; i<nx; i += nblock) {
	/* last block may be shorter */
	if (nx-i < nblock) thisblock = nx-i;
	/*
	 * Compute differences of adjacent pixels and map them to unsigned values.
	 * Note that this may overflow the integer variables -- that's
	 * OK, because we can recover when decompressing.  If we were
	 * compressing shorts or bytes, would want to do this arithmetic
	 * with short/byte working variables (though diff will still be
	 * passed as an int.)
	 *
	 * compute sum of mapped pixel values at same time
	 * use double precision for sum to allow 32-bit integer inputs
	 */
	pixelsum = 0.0;
	for (j=0; j<thisblock; j++) {
	    nextpix = a[i+j];
	    pdiff = nextpix - lastpix;
	    diff[j] = (unsigned int) ((pdiff<0) ? ~(pdiff<<1) : (pdiff<<1));
	    pixelsum += diff[j];
	    lastpix = nextpix;
	}

	/*
	 * compute number of bits to split from sum
	 */
	dpsum = (pixelsum - (thisblock/2) - 1)/thisblock;
	if (dpsum < 0) dpsum = 0.0;
	psum = ((unsigned int) dpsum ) >> 1;
	for (fs = 0; psum>0; fs++) psum >>= 1;

	/*
	 * write the codes
	 * fsbits ID bits used to indicate split level
	 */
	if (fs >= fsmax) {
	    /* Special high entropy case when FS >= fsmax
	     * Just write pixel difference values directly, no Rice coding at all.
	     */
	    if (output_nbits(buffer, fsmax+1, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	    for (j=0; j<thisblock; j++) {
		if (output_nbits(buffer, diff[j], bbits) == EOF) {
                    ffpmsg("rice_encode: end of buffer");
                    free(diff);
		    return(-1);
		}
	    }
	} else if (fs == 0 && pixelsum == 0) {
	    /*
	     * special low entropy case when FS = 0 and pixelsum=0 (all
	     * pixels in block are zero.)
	     * Output a 0 and return
	     */
	    if (output_nbits(buffer, 0, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	} else {
	    /* normal case: not either very high or very low entropy */
	    if (output_nbits(buffer, fs+1, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	    fsmask = (1<<fs) - 1;
	    /*
	     * local copies of bit buffer to improve optimization
	     */
	    lbitbuffer = buffer->bitbuffer;
	    lbits_to_go = buffer->bits_to_go;
	    for (j=0; j<thisblock; j++) {
		v = diff[j];
		top = v >> fs;
		/*
		 * top is coded by top zeros + 1
		 */
		if (lbits_to_go >= top+1) {
		    lbitbuffer <<= top+1;
		    lbitbuffer |= 1;
		    lbits_to_go -= top+1;
		} else {
		    lbitbuffer <<= lbits_to_go;
		    putcbuf(lbitbuffer & 0xff,buffer);

		    for (top -= lbits_to_go; top>=8; top -= 8) {
			putcbuf(0, buffer);
		    }
		    lbitbuffer = 1;
		    lbits_to_go = 7-top;
		}
		/*
		 * bottom FS bits are written without coding
		 * code is output_nbits, moved into this routine to reduce overheads
		 * This code potentially breaks if FS>24, so I am limiting
		 * FS to 24 by choice of FSMAX above.
		 */
		if (fs > 0) {
		    lbitbuffer <<= fs;
		    lbitbuffer |= v & fsmask;
		    lbits_to_go -= fs;
		    while (lbits_to_go <= 0) {
			putcbuf((lbitbuffer>>(-lbits_to_go)) & 0xff,buffer);
			lbits_to_go += 8;
		    }
		}
	    }

	    /* check if overflowed output buffer */
	    if (buffer->current > buffer->end) {
                 ffpmsg("rice_encode: end of buffer");
                 free(diff);
		 return(-1);
	    }
	    buffer->bitbuffer = lbitbuffer;
	    buffer->bits_to_go = lbits_to_go;
	}
    }
    done_outputing_bits(buffer);
    free(diff);
    /*
     * return number of bytes used
     */
    return(buffer->current - buffer->start);
}
/*---------------------------------------------------------------------------*/

int fits_rcomp_short(
	  short a[],		/* input array			*/
	  int nx,		/* number of input pixels	*/
	  unsigned char *c,	/* output buffer		*/
	  int clen,		/* max length of output		*/
	  int nblock)		/* coding block size		*/
{
Buffer bufmem, *buffer = &bufmem;
/* int bsize;  */
int i, j, thisblock;

/* 
NOTE: in principle, the following 2 variable could be declared as 'short'
but in fact the code runs faster (on 32-bit Linux at least) as 'int'
*/
int lastpix, nextpix;
/* int pdiff; */
short pdiff; 
int v, fs, fsmask, top, fsmax, fsbits, bbits;
int lbitbuffer, lbits_to_go;
/* unsigned int psum; */
unsigned short psum;
double pixelsum, dpsum;
unsigned int *diff;

    /*
     * Original size of each pixel (bsize, bytes) and coding block
     * size (nblock, pixels)
     * Could make bsize a parameter to allow more efficient
     * compression of short & byte images.
     */
/*    bsize = 2; */

/*    nblock = 32; now an input parameter */
    /*
     * From bsize derive:
     * FSBITS = # bits required to store FS
     * FSMAX = maximum value for FS
     * BBITS = bits/pixel for direct coding
     */

/*
    switch (bsize) {
    case 1:
	fsbits = 3;
	fsmax = 6;
	break;
    case 2:
	fsbits = 4;
	fsmax = 14;
	break;
    case 4:
	fsbits = 5;
	fsmax = 25;
	break;
    default:
        ffpmsg("rdecomp: bsize must be 1, 2, or 4 bytes");
	return(-1);
    }
*/

    /* move these out of switch block to further tweak performance */
    fsbits = 4;
    fsmax = 14;
    
    bbits = 1<<fsbits;

    /*
     * Set up buffer pointers
     */
    buffer->start = c;
    buffer->current = c;
    buffer->end = c+clen;
    buffer->bits_to_go = 8;
    /*
     * array for differences mapped to non-negative values
     */
    diff = (unsigned int *) malloc(nblock*sizeof(unsigned int));
    if (diff == (unsigned int *) NULL) {
        ffpmsg("fits_rcomp: insufficient memory");
	return(-1);
    }
    /*
     * Code in blocks of nblock pixels
     */
    start_outputing_bits(buffer);

    /* write out first short value to the first 2 bytes of the buffer */
    if (output_nbits(buffer, a[0], 16) == EOF) {
        ffpmsg("rice_encode: end of buffer");
        free(diff);
        return(-1);
    }

    lastpix = a[0];  /* the first difference will always be zero */

    thisblock = nblock;
    for (i=0; i<nx; i += nblock) {
	/* last block may be shorter */
	if (nx-i < nblock) thisblock = nx-i;
	/*
	 * Compute differences of adjacent pixels and map them to unsigned values.
	 * Note that this may overflow the integer variables -- that's
	 * OK, because we can recover when decompressing.  If we were
	 * compressing shorts or bytes, would want to do this arithmetic
	 * with short/byte working variables (though diff will still be
	 * passed as an int.)
	 *
	 * compute sum of mapped pixel values at same time
	 * use double precision for sum to allow 32-bit integer inputs
	 */
	pixelsum = 0.0;
	for (j=0; j<thisblock; j++) {
	    nextpix = a[i+j];
	    pdiff = nextpix - lastpix;
	    diff[j] = (unsigned int) ((pdiff<0) ? ~(pdiff<<1) : (pdiff<<1));
	    pixelsum += diff[j];
	    lastpix = nextpix;
	}
	/*
	 * compute number of bits to split from sum
	 */
	dpsum = (pixelsum - (thisblock/2) - 1)/thisblock;
	if (dpsum < 0) dpsum = 0.0;
/*	psum = ((unsigned int) dpsum ) >> 1; */
	psum = ((unsigned short) dpsum ) >> 1;
	for (fs = 0; psum>0; fs++) psum >>= 1;

	/*
	 * write the codes
	 * fsbits ID bits used to indicate split level
	 */
	if (fs >= fsmax) {
/* case3++; */
	    /* Special high entropy case when FS >= fsmax
	     * Just write pixel difference values directly, no Rice coding at all.
	     */
	    if (output_nbits(buffer, fsmax+1, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	    for (j=0; j<thisblock; j++) {
		if (output_nbits(buffer, diff[j], bbits) == EOF) {
                    ffpmsg("rice_encode: end of buffer");
                    free(diff);
		    return(-1);
		}
	    }
	} else if (fs == 0 && pixelsum == 0) {
/* case1++; */
	    /*
	     * special low entropy case when FS = 0 and pixelsum=0 (all
	     * pixels in block are zero.)
	     * Output a 0 and return
	     */
	    if (output_nbits(buffer, 0, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	} else {
/* case2++; */
	    /* normal case: not either very high or very low entropy */
	    if (output_nbits(buffer, fs+1, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	    fsmask = (1<<fs) - 1;
	    /*
	     * local copies of bit buffer to improve optimization
	     */
	    lbitbuffer = buffer->bitbuffer;
	    lbits_to_go = buffer->bits_to_go;
	    for (j=0; j<thisblock; j++) {
		v = diff[j];
		top = v >> fs;
		/*
		 * top is coded by top zeros + 1
		 */
		if (lbits_to_go >= top+1) {
		    lbitbuffer <<= top+1;
		    lbitbuffer |= 1;
		    lbits_to_go -= top+1;
		} else {
		    lbitbuffer <<= lbits_to_go;
		    putcbuf(lbitbuffer & 0xff,buffer);
		    for (top -= lbits_to_go; top>=8; top -= 8) {
			putcbuf(0, buffer);
		    }
		    lbitbuffer = 1;
		    lbits_to_go = 7-top;
		}
		/*
		 * bottom FS bits are written without coding
		 * code is output_nbits, moved into this routine to reduce overheads
		 * This code potentially breaks if FS>24, so I am limiting
		 * FS to 24 by choice of FSMAX above.
		 */
		if (fs > 0) {
		    lbitbuffer <<= fs;
		    lbitbuffer |= v & fsmask;
		    lbits_to_go -= fs;
		    while (lbits_to_go <= 0) {
			putcbuf((lbitbuffer>>(-lbits_to_go)) & 0xff,buffer);
			lbits_to_go += 8;
		    }
		}
	    }
	    /* check if overflowed output buffer */
	    if (buffer->current > buffer->end) {
                 ffpmsg("rice_encode: end of buffer");
                 free(diff);
		 return(-1);
	    }
	    buffer->bitbuffer = lbitbuffer;
	    buffer->bits_to_go = lbits_to_go;
	}
    }
    done_outputing_bits(buffer);
    free(diff);
    /*
     * return number of bytes used
     */
    return(buffer->current - buffer->start);
}
/*---------------------------------------------------------------------------*/

int fits_rcomp_byte(
	  signed char a[],		/* input array			*/
	  int nx,		/* number of input pixels	*/
	  unsigned char *c,	/* output buffer		*/
	  int clen,		/* max length of output		*/
	  int nblock)		/* coding block size		*/
{
Buffer bufmem, *buffer = &bufmem;
/* int bsize; */
int i, j, thisblock;

/* 
NOTE: in principle, the following 2 variable could be declared as 'short'
but in fact the code runs faster (on 32-bit Linux at least) as 'int'
*/
int lastpix, nextpix;
/* int pdiff; */
signed char pdiff; 
int v, fs, fsmask, top, fsmax, fsbits, bbits;
int lbitbuffer, lbits_to_go;
/* unsigned int psum; */
unsigned char psum;
double pixelsum, dpsum;
unsigned int *diff;

    /*
     * Original size of each pixel (bsize, bytes) and coding block
     * size (nblock, pixels)
     * Could make bsize a parameter to allow more efficient
     * compression of short & byte images.
     */
/*    bsize = 1;  */

/*    nblock = 32; now an input parameter */
    /*
     * From bsize derive:
     * FSBITS = # bits required to store FS
     * FSMAX = maximum value for FS
     * BBITS = bits/pixel for direct coding
     */

/*
    switch (bsize) {
    case 1:
	fsbits = 3;
	fsmax = 6;
	break;
    case 2:
	fsbits = 4;
	fsmax = 14;
	break;
    case 4:
	fsbits = 5;
	fsmax = 25;
	break;
    default:
        ffpmsg("rdecomp: bsize must be 1, 2, or 4 bytes");
	return(-1);
    }
*/

    /* move these out of switch block to further tweak performance */
    fsbits = 3;
    fsmax = 6;
    bbits = 1<<fsbits;

    /*
     * Set up buffer pointers
     */
    buffer->start = c;
    buffer->current = c;
    buffer->end = c+clen;
    buffer->bits_to_go = 8;
    /*
     * array for differences mapped to non-negative values
     */
    diff = (unsigned int *) malloc(nblock*sizeof(unsigned int));
    if (diff == (unsigned int *) NULL) {
        ffpmsg("fits_rcomp: insufficient memory");
	return(-1);
    }
    /*
     * Code in blocks of nblock pixels
     */
    start_outputing_bits(buffer);

    /* write out first byte value to the first  byte of the buffer */
    if (output_nbits(buffer, a[0], 8) == EOF) {
        ffpmsg("rice_encode: end of buffer");
        free(diff);
        return(-1);
    }

    lastpix = a[0];  /* the first difference will always be zero */

    thisblock = nblock;
    for (i=0; i<nx; i += nblock) {
	/* last block may be shorter */
	if (nx-i < nblock) thisblock = nx-i;
	/*
	 * Compute differences of adjacent pixels and map them to unsigned values.
	 * Note that this may overflow the integer variables -- that's
	 * OK, because we can recover when decompressing.  If we were
	 * compressing shorts or bytes, would want to do this arithmetic
	 * with short/byte working variables (though diff will still be
	 * passed as an int.)
	 *
	 * compute sum of mapped pixel values at same time
	 * use double precision for sum to allow 32-bit integer inputs
	 */
	pixelsum = 0.0;
	for (j=0; j<thisblock; j++) {
	    nextpix = a[i+j];
	    pdiff = nextpix - lastpix;
	    diff[j] = (unsigned int) ((pdiff<0) ? ~(pdiff<<1) : (pdiff<<1));
	    pixelsum += diff[j];
	    lastpix = nextpix;
	}
	/*
	 * compute number of bits to split from sum
	 */
	dpsum = (pixelsum - (thisblock/2) - 1)/thisblock;
	if (dpsum < 0) dpsum = 0.0;
/*	psum = ((unsigned int) dpsum ) >> 1; */
	psum = ((unsigned char) dpsum ) >> 1;
	for (fs = 0; psum>0; fs++) psum >>= 1;

	/*
	 * write the codes
	 * fsbits ID bits used to indicate split level
	 */
	if (fs >= fsmax) {
	    /* Special high entropy case when FS >= fsmax
	     * Just write pixel difference values directly, no Rice coding at all.
	     */
	    if (output_nbits(buffer, fsmax+1, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	    for (j=0; j<thisblock; j++) {
		if (output_nbits(buffer, diff[j], bbits) == EOF) {
                    ffpmsg("rice_encode: end of buffer");
                    free(diff);
		    return(-1);
		}
	    }
	} else if (fs == 0 && pixelsum == 0) {
	    /*
	     * special low entropy case when FS = 0 and pixelsum=0 (all
	     * pixels in block are zero.)
	     * Output a 0 and return
	     */
	    if (output_nbits(buffer, 0, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	} else {
	    /* normal case: not either very high or very low entropy */
	    if (output_nbits(buffer, fs+1, fsbits) == EOF) {
                ffpmsg("rice_encode: end of buffer");
                free(diff);
		return(-1);
	    }
	    fsmask = (1<<fs) - 1;
	    /*
	     * local copies of bit buffer to improve optimization
	     */
	    lbitbuffer = buffer->bitbuffer;
	    lbits_to_go = buffer->bits_to_go;
	    for (j=0; j<thisblock; j++) {
		v = diff[j];
		top = v >> fs;
		/*
		 * top is coded by top zeros + 1
		 */
		if (lbits_to_go >= top+1) {
		    lbitbuffer <<= top+1;
		    lbitbuffer |= 1;
		    lbits_to_go -= top+1;
		} else {
		    lbitbuffer <<= lbits_to_go;
		    putcbuf(lbitbuffer & 0xff,buffer);
		    for (top -= lbits_to_go; top>=8; top -= 8) {
			putcbuf(0, buffer);
		    }
		    lbitbuffer = 1;
		    lbits_to_go = 7-top;
		}
		/*
		 * bottom FS bits are written without coding
		 * code is output_nbits, moved into this routine to reduce overheads
		 * This code potentially breaks if FS>24, so I am limiting
		 * FS to 24 by choice of FSMAX above.
		 */
		if (fs > 0) {
		    lbitbuffer <<= fs;
		    lbitbuffer |= v & fsmask;
		    lbits_to_go -= fs;
		    while (lbits_to_go <= 0) {
			putcbuf((lbitbuffer>>(-lbits_to_go)) & 0xff,buffer);
			lbits_to_go += 8;
		    }
		}
	    }
	    /* check if overflowed output buffer */
	    if (buffer->current > buffer->end) {
                 ffpmsg("rice_encode: end of buffer");
                 free(diff);
		 return(-1);
	    }
	    buffer->bitbuffer = lbitbuffer;
	    buffer->bits_to_go = lbits_to_go;
	}
    }
    done_outputing_bits(buffer);
    free(diff);
    /*
     * return number of bytes used
     */
    return(buffer->current - buffer->start);
}
/*---------------------------------------------------------------------------*/
/* bit_output.c
 *
 * Bit output routines
 * Procedures return zero on success, EOF on end-of-buffer
 *
 * Programmer: R. White     Date: 20 July 1998
 */

/* Initialize for bit output */

static void start_outputing_bits(Buffer *buffer)
{
    /*
     * Buffer is empty to start with
     */
    buffer->bitbuffer = 0;
    buffer->bits_to_go = 8;
}

/*---------------------------------------------------------------------------*/
/* Output N bits (N must be <= 32) */

static int output_nbits(Buffer *buffer, int bits, int n)
{
/* local copies */
int lbitbuffer;
int lbits_to_go;
    /* AND mask for the right-most n bits */
    static unsigned int mask[33] = 
         {0,
	  0x1,       0x3,       0x7,       0xf,       0x1f,       0x3f,       0x7f,       0xff,
	  0x1ff,     0x3ff,     0x7ff,     0xfff,     0x1fff,     0x3fff,     0x7fff,     0xffff,
	  0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,   0x1fffff,   0x3fffff,   0x7fffff,   0xffffff,
	  0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};

    /*
     * insert bits at end of bitbuffer
     */
    lbitbuffer = buffer->bitbuffer;
    lbits_to_go = buffer->bits_to_go;
    if (lbits_to_go+n > 32) {
	/*
	 * special case for large n: put out the top lbits_to_go bits first
	 * note that 0 < lbits_to_go <= 8
	 */
	lbitbuffer <<= lbits_to_go;
/*	lbitbuffer |= (bits>>(n-lbits_to_go)) & ((1<<lbits_to_go)-1); */
	lbitbuffer |= (bits>>(n-lbits_to_go)) & *(mask+lbits_to_go);
	putcbuf(lbitbuffer & 0xff,buffer);
	n -= lbits_to_go;
	lbits_to_go = 8;
    }
    lbitbuffer <<= n;
/*    lbitbuffer |= ( bits & ((1<<n)-1) ); */
    lbitbuffer |= ( bits & *(mask+n) );
    lbits_to_go -= n;
    while (lbits_to_go <= 0) {
	/*
	 * bitbuffer full, put out top 8 bits
	 */
	putcbuf((lbitbuffer>>(-lbits_to_go)) & 0xff,buffer);
	lbits_to_go += 8;
    }
    buffer->bitbuffer = lbitbuffer;
    buffer->bits_to_go = lbits_to_go;
    return(0);
}
/*---------------------------------------------------------------------------*/
/* Flush out the last bits */

static int done_outputing_bits(Buffer *buffer)
{
    if(buffer->bits_to_go < 8) {
	putcbuf(buffer->bitbuffer<<buffer->bits_to_go,buffer);
	
/*	if (putcbuf(buffer->bitbuffer<<buffer->bits_to_go,buffer) == EOF)
	    return(EOF);
*/
    }
    return(0);
}
/*---------------------------------------------------------------------------*/
/*----------------------------------------------------------*/
/*                                                          */
/*    START OF SOURCE FILE ORIGINALLY CALLED rdecomp.c      */
/*                                                          */
/*----------------------------------------------------------*/

/* @(#) rdecomp.c 1.4 99/03/01 12:38:41 */
/* rdecomp.c	Decompress image line using
 *		(1) Difference of adjacent pixels
 *		(2) Rice algorithm coding
 *
 * Returns 0 on success or 1 on failure
 */

/*    moved these 'includes' to the beginning of the file (WDP)
#include <stdio.h>
#include <stdlib.h>
*/

/*---------------------------------------------------------------------------*/
/* this routine used to be called 'rdecomp'  (WDP)  */

int fits_rdecomp (unsigned char *c,		/* input buffer			*/
	     int clen,			/* length of input		*/
	     unsigned int array[],	/* output array			*/
	     int nx,			/* number of output pixels	*/
	     int nblock)		/* coding block size		*/
{
/* int bsize;  */
int i, k, imax;
int nbits, nzero, fs;
unsigned char *cend, bytevalue;
unsigned int b, diff, lastpix;
int fsmax, fsbits, bbits;
extern const int nonzero_count[];

   /*
     * Original size of each pixel (bsize, bytes) and coding block
     * size (nblock, pixels)
     * Could make bsize a parameter to allow more efficient
     * compression of short & byte images.
     */
/*    bsize = 4; */

/*    nblock = 32; now an input parameter */
    /*
     * From bsize derive:
     * FSBITS = # bits required to store FS
     * FSMAX = maximum value for FS
     * BBITS = bits/pixel for direct coding
     */

/*
    switch (bsize) {
    case 1:
	fsbits = 3;
	fsmax = 6;
	break;
    case 2:
	fsbits = 4;
	fsmax = 14;
	break;
    case 4:
	fsbits = 5;
	fsmax = 25;
	break;
    default:
        ffpmsg("rdecomp: bsize must be 1, 2, or 4 bytes");
	return 1;
    }
*/

    /* move out of switch block, to tweak performance */
    fsbits = 5;
    fsmax = 25;

    bbits = 1<<fsbits;

    /*
     * Decode in blocks of nblock pixels
     */

    /* first 4 bytes of input buffer contain the value of the first */
    /* 4 byte integer value, without any encoding */
    
    lastpix = 0;
    bytevalue = c[0];
    lastpix = lastpix | (bytevalue<<24);
    bytevalue = c[1];
    lastpix = lastpix | (bytevalue<<16);
    bytevalue = c[2];
    lastpix = lastpix | (bytevalue<<8);
    bytevalue = c[3];
    lastpix = lastpix | bytevalue;

    c += 4;  
    cend = c + clen - 4;

    b = *c++;		    /* bit buffer			*/
    nbits = 8;		    /* number of bits remaining in b	*/
    for (i = 0; i<nx; ) {
	/* get the FS value from first fsbits */
	nbits -= fsbits;
	while (nbits < 0) {
	    b = (b<<8) | (*c++);
	    nbits += 8;
	}
	fs = (b >> nbits) - 1;

	b &= (1<<nbits)-1;
	/* loop over the next block */
	imax = i + nblock;
	if (imax > nx) imax = nx;
	if (fs<0) {
	    /* low-entropy case, all zero differences */
	    for ( ; i<imax; i++) array[i] = lastpix;
	} else if (fs==fsmax) {
	    /* high-entropy case, directly coded pixel values */
	    for ( ; i<imax; i++) {
		k = bbits - nbits;
		diff = b<<k;
		for (k -= 8; k >= 0; k -= 8) {
		    b = *c++;
		    diff |= b<<k;
		}
		if (nbits>0) {
		    b = *c++;
		    diff |= b>>(-k);
		    b &= (1<<nbits)-1;
		} else {
		    b = 0;
		}
		/*
		 * undo mapping and differencing
		 * Note that some of these operations will overflow the
		 * unsigned int arithmetic -- that's OK, it all works
		 * out to give the right answers in the output file.
		 */
		if ((diff & 1) == 0) {
		    diff = diff>>1;
		} else {
		    diff = ~(diff>>1);
		}
		array[i] = diff+lastpix;
		lastpix = array[i];
	    }
	} else {
	    /* normal case, Rice coding */
	    for ( ; i<imax; i++) {
		/* count number of leading zeros */
		while (b == 0) {
		    nbits += 8;
		    b = *c++;
		}
		nzero = nbits - nonzero_count[b];
		nbits -= nzero+1;
		/* flip the leading one-bit */
		b ^= 1<<nbits;
		/* get the FS trailing bits */
		nbits -= fs;
		while (nbits < 0) {
		    b = (b<<8) | (*c++);
		    nbits += 8;
		}
		diff = (nzero<<fs) | (b>>nbits);
		b &= (1<<nbits)-1;

		/* undo mapping and differencing */
		if ((diff & 1) == 0) {
		    diff = diff>>1;
		} else {
		    diff = ~(diff>>1);
		}
		array[i] = diff+lastpix;
		lastpix = array[i];
	    }
	}
	if (c > cend) {
            ffpmsg("decompression error: hit end of compressed byte stream");
	    return 1;
	}
    }
    if (c < cend) {
        ffpmsg("decompression warning: unused bytes at end of compressed buffer");
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
/* this routine used to be called 'rdecomp'  (WDP)  */

int fits_rdecomp_short (unsigned char *c,		/* input buffer			*/
	     int clen,			/* length of input		*/
	     unsigned short array[],  	/* output array			*/
	     int nx,			/* number of output pixels	*/
	     int nblock)		/* coding block size		*/
{
int i, imax;
/* int bsize; */
int k;
int nbits, nzero, fs;
unsigned char *cend, bytevalue;
unsigned int b, diff, lastpix;
int fsmax, fsbits, bbits;
extern const int nonzero_count[];

   /*
     * Original size of each pixel (bsize, bytes) and coding block
     * size (nblock, pixels)
     * Could make bsize a parameter to allow more efficient
     * compression of short & byte images.
     */

/*    bsize = 2; */
    
/*    nblock = 32; now an input parameter */
    /*
     * From bsize derive:
     * FSBITS = # bits required to store FS
     * FSMAX = maximum value for FS
     * BBITS = bits/pixel for direct coding
     */

/*
    switch (bsize) {
    case 1:
	fsbits = 3;
	fsmax = 6;
	break;
    case 2:
	fsbits = 4;
	fsmax = 14;
	break;
    case 4:
	fsbits = 5;
	fsmax = 25;
	break;
    default:
        ffpmsg("rdecomp: bsize must be 1, 2, or 4 bytes");
	return 1;
    }
*/

    /* move out of switch block, to tweak performance */
    fsbits = 4;
    fsmax = 14;

    bbits = 1<<fsbits;

    /*
     * Decode in blocks of nblock pixels
     */

    /* first 2 bytes of input buffer contain the value of the first */
    /* 2 byte integer value, without any encoding */
    
    lastpix = 0;
    bytevalue = c[0];
    lastpix = lastpix | (bytevalue<<8);
    bytevalue = c[1];
    lastpix = lastpix | bytevalue;

    c += 2;  
    cend = c + clen - 2;

    b = *c++;		    /* bit buffer			*/
    nbits = 8;		    /* number of bits remaining in b	*/
    for (i = 0; i<nx; ) {
	/* get the FS value from first fsbits */
	nbits -= fsbits;
	while (nbits < 0) {
	    b = (b<<8) | (*c++);
	    nbits += 8;
	}
	fs = (b >> nbits) - 1;

	b &= (1<<nbits)-1;
	/* loop over the next block */
	imax = i + nblock;
	if (imax > nx) imax = nx;
	if (fs<0) {
	    /* low-entropy case, all zero differences */
	    for ( ; i<imax; i++) array[i] = lastpix;
	} else if (fs==fsmax) {
	    /* high-entropy case, directly coded pixel values */
	    for ( ; i<imax; i++) {
		k = bbits - nbits;
		diff = b<<k;
		for (k -= 8; k >= 0; k -= 8) {
		    b = *c++;
		    diff |= b<<k;
		}
		if (nbits>0) {
		    b = *c++;
		    diff |= b>>(-k);
		    b &= (1<<nbits)-1;
		} else {
		    b = 0;
		}
   
		/*
		 * undo mapping and differencing
		 * Note that some of these operations will overflow the
		 * unsigned int arithmetic -- that's OK, it all works
		 * out to give the right answers in the output file.
		 */
		if ((diff & 1) == 0) {
		    diff = diff>>1;
		} else {
		    diff = ~(diff>>1);
		}
		array[i] = diff+lastpix;
		lastpix = array[i];
	    }
	} else {
	    /* normal case, Rice coding */
	    for ( ; i<imax; i++) {
		/* count number of leading zeros */
		while (b == 0) {
		    nbits += 8;
		    b = *c++;
		}
		nzero = nbits - nonzero_count[b];
		nbits -= nzero+1;
		/* flip the leading one-bit */
		b ^= 1<<nbits;
		/* get the FS trailing bits */
		nbits -= fs;
		while (nbits < 0) {
		    b = (b<<8) | (*c++);
		    nbits += 8;
		}
		diff = (nzero<<fs) | (b>>nbits);
		b &= (1<<nbits)-1;

		/* undo mapping and differencing */
		if ((diff & 1) == 0) {
		    diff = diff>>1;
		} else {
		    diff = ~(diff>>1);
		}
		array[i] = diff+lastpix;
		lastpix = array[i];
	    }
	}
	if (c > cend) {
            ffpmsg("decompression error: hit end of compressed byte stream");
	    return 1;
	}
    }
    if (c < cend) {
        ffpmsg("decompression warning: unused bytes at end of compressed buffer");
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
/* this routine used to be called 'rdecomp'  (WDP)  */

int fits_rdecomp_byte (unsigned char *c,		/* input buffer			*/
	     int clen,			/* length of input		*/
	     unsigned char array[],  	/* output array			*/
	     int nx,			/* number of output pixels	*/
	     int nblock)		/* coding block size		*/
{
int i, imax;
/* int bsize; */
int k;
int nbits, nzero, fs;
unsigned char *cend;
unsigned int b, diff, lastpix;
int fsmax, fsbits, bbits;
extern const int nonzero_count[];

   /*
     * Original size of each pixel (bsize, bytes) and coding block
     * size (nblock, pixels)
     * Could make bsize a parameter to allow more efficient
     * compression of short & byte images.
     */

/*    bsize = 1; */
    
/*    nblock = 32; now an input parameter */
    /*
     * From bsize derive:
     * FSBITS = # bits required to store FS
     * FSMAX = maximum value for FS
     * BBITS = bits/pixel for direct coding
     */

/*
    switch (bsize) {
    case 1:
	fsbits = 3;
	fsmax = 6;
	break;
    case 2:
	fsbits = 4;
	fsmax = 14;
	break;
    case 4:
	fsbits = 5;
	fsmax = 25;
	break;
    default:
        ffpmsg("rdecomp: bsize must be 1, 2, or 4 bytes");
	return 1;
    }
*/

    /* move out of switch block, to tweak performance */
    fsbits = 3;
    fsmax = 6;

    bbits = 1<<fsbits;

    /*
     * Decode in blocks of nblock pixels
     */

    /* first byte of input buffer contain the value of the first */
    /* byte integer value, without any encoding */
    
    lastpix = c[0];
    c += 1;  
    cend = c + clen - 1;

    b = *c++;		    /* bit buffer			*/
    nbits = 8;		    /* number of bits remaining in b	*/
    for (i = 0; i<nx; ) {
	/* get the FS value from first fsbits */
	nbits -= fsbits;
	while (nbits < 0) {
	    b = (b<<8) | (*c++);
	    nbits += 8;
	}
	fs = (b >> nbits) - 1;

	b &= (1<<nbits)-1;
	/* loop over the next block */
	imax = i + nblock;
	if (imax > nx) imax = nx;
	if (fs<0) {
	    /* low-entropy case, all zero differences */
	    for ( ; i<imax; i++) array[i] = lastpix;
	} else if (fs==fsmax) {
	    /* high-entropy case, directly coded pixel values */
	    for ( ; i<imax; i++) {
		k = bbits - nbits;
		diff = b<<k;
		for (k -= 8; k >= 0; k -= 8) {
		    b = *c++;
		    diff |= b<<k;
		}
		if (nbits>0) {
		    b = *c++;
		    diff |= b>>(-k);
		    b &= (1<<nbits)-1;
		} else {
		    b = 0;
		}
   
		/*
		 * undo mapping and differencing
		 * Note that some of these operations will overflow the
		 * unsigned int arithmetic -- that's OK, it all works
		 * out to give the right answers in the output file.
		 */
		if ((diff & 1) == 0) {
		    diff = diff>>1;
		} else {
		    diff = ~(diff>>1);
		}
		array[i] = diff+lastpix;
		lastpix = array[i];
	    }
	} else {
	    /* normal case, Rice coding */
	    for ( ; i<imax; i++) {
		/* count number of leading zeros */
		while (b == 0) {
		    nbits += 8;
		    b = *c++;
		}
		nzero = nbits - nonzero_count[b];
		nbits -= nzero+1;
		/* flip the leading one-bit */
		b ^= 1<<nbits;
		/* get the FS trailing bits */
		nbits -= fs;
		while (nbits < 0) {
		    b = (b<<8) | (*c++);
		    nbits += 8;
		}
		diff = (nzero<<fs) | (b>>nbits);
		b &= (1<<nbits)-1;

		/* undo mapping and differencing */
		if ((diff & 1) == 0) {
		    diff = diff>>1;
		} else {
		    diff = ~(diff>>1);
		}
		array[i] = diff+lastpix;
		lastpix = array[i];
	    }
	}
	if (c > cend) {
            ffpmsg("decompression error: hit end of compressed byte stream");
	    return 1;
	}
    }
    if (c < cend) {
        ffpmsg("decompression warning: unused bytes at end of compressed buffer");
    }
    return 0;
}
