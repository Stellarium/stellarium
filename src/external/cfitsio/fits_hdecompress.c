/*  #########################################################################
These routines to apply the H-compress decompression algorithm to a 2-D Fits
image were written by R. White at the STScI and were obtained from the STScI at
http://www.stsci.edu/software/hcompress.html

This source file is a concatination of the following sources files in the
original distribution 
  hinv.c 
  hsmooth.c 
  undigitize.c 
  decode.c 
  dodecode.c 
  qtree_decode.c 
  qread.c 
  bit_input.c


The following modifications have been made to the original code:

  - commented out redundant "include" statements
  - added the nextchar global variable 
  - changed all the 'extern' declarations to 'static', since all the routines are in
    the same source file
  - changed the first parameter in decode (and in lower level routines from a file stream
    to a char array
  - modified the myread routine, and lower level byte reading routines,  to copy 
    the input bytes to a char array, instead of reading them from a file stream
  - changed the function declarations to the more modern ANSI C style
  - changed calls to printf and perror to call the CFITSIO ffpmsg routine
  - replace "exit" statements with "return" statements

 ############################################################################  */
 
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "fitsio2.h"

/* WDP added test to see if min and max are already defined */
#ifndef min
#define min(a,b)        (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b)        (((a)>(b))?(a):(b))
#endif

static long nextchar;

static int decode(unsigned char *infile, int *a, int *nx, int *ny, int *scale);
static int decode64(unsigned char *infile, LONGLONG *a, int *nx, int *ny, int *scale);
static int hinv(int a[], int nx, int ny, int smooth ,int scale);
static int hinv64(LONGLONG a[], int nx, int ny, int smooth ,int scale);
static void undigitize(int a[], int nx, int ny, int scale);
static void undigitize64(LONGLONG a[], int nx, int ny, int scale);
static void unshuffle(int a[], int n, int n2, int tmp[]);
static void unshuffle64(LONGLONG a[], int n, int n2, LONGLONG tmp[]);
static void hsmooth(int a[], int nxtop, int nytop, int ny, int scale);
static void hsmooth64(LONGLONG a[], int nxtop, int nytop, int ny, int scale);
static void qread(unsigned char *infile,char *a, int n);
static int  readint(unsigned char *infile);
static LONGLONG readlonglong(unsigned char *infile);
static int dodecode(unsigned char *infile, int a[], int nx, int ny, unsigned char nbitplanes[3]);
static int dodecode64(unsigned char *infile, LONGLONG a[], int nx, int ny, unsigned char nbitplanes[3]);
static int qtree_decode(unsigned char *infile, int a[], int n, int nqx, int nqy, int nbitplanes);
static int qtree_decode64(unsigned char *infile, LONGLONG a[], int n, int nqx, int nqy, int nbitplanes);
static void start_inputing_bits(void);
static int input_bit(unsigned char *infile);
static int input_nbits(unsigned char *infile, int n);
/*  make input_nybble a separate routine, for added effiency */
/* #define input_nybble(infile)	input_nbits(infile,4) */
static int input_nybble(unsigned char *infile);
static int input_nnybble(unsigned char *infile, int n, unsigned char *array);

static void qtree_expand(unsigned char *infile, unsigned char a[], int nx, int ny, unsigned char b[]);
static void qtree_bitins(unsigned char a[], int nx, int ny, int b[], int n, int bit);
static void qtree_bitins64(unsigned char a[], int nx, int ny, LONGLONG b[], int n, int bit);
static void qtree_copy(unsigned char a[], int nx, int ny, unsigned char b[], int n);
static void read_bdirect(unsigned char *infile, int a[], int n, int nqx, int nqy, unsigned char scratch[], int bit);
static void read_bdirect64(unsigned char *infile, LONGLONG a[], int n, int nqx, int nqy, unsigned char scratch[], int bit);
static int  input_huffman(unsigned char *infile);

/* ---------------------------------------------------------------------- */
int fits_hdecompress(unsigned char *input, int smooth, int *a, int *ny, int *nx, 
                     int *scale, int *status)
{
  /* 
     decompress the input byte stream using the H-compress algorithm
  
   input  - input array of compressed bytes
   a - pre-allocated array to hold the output uncompressed image
   nx - returned X axis size
   ny - returned Y axis size

 NOTE: the nx and ny dimensions as defined within this code are reversed from
 the usual FITS notation.  ny is the fastest varying dimension, which is
 usually considered the X axis in the FITS image display

  */
int stat;

  if (*status > 0) return(*status);

	/* decode the input array */

        FFLOCK;  /* decode uses the nextchar global variable */
	stat = decode(input, a, nx, ny, scale);
        FFUNLOCK;

        *status = stat;
	if (stat) return(*status);
	
	/*
	 * Un-Digitize
	 */
	undigitize(a, *nx, *ny, *scale);

	/*
	 * Inverse H-transform
	 */
	stat = hinv(a, *nx, *ny, smooth, *scale);
        *status = stat;
	
  return(*status);
}
/* ---------------------------------------------------------------------- */
int fits_hdecompress64(unsigned char *input, int smooth, LONGLONG *a, int *ny, int *nx, 
                     int *scale, int *status)
{
  /* 
     decompress the input byte stream using the H-compress algorithm
  
   input  - input array of compressed bytes
   a - pre-allocated array to hold the output uncompressed image
   nx - returned X axis size
   ny - returned Y axis size

 NOTE: the nx and ny dimensions as defined within this code are reversed from
 the usual FITS notation.  ny is the fastest varying dimension, which is
 usually considered the X axis in the FITS image display

  */
  int stat, *iarray, ii, nval;

  if (*status > 0) return(*status);

	/* decode the input array */

        FFLOCK;  /* decode uses the nextchar global variable */
	stat = decode64(input, a, nx, ny, scale);
        FFUNLOCK;

        *status = stat;
	if (stat) return(*status);
	
	/*
	 * Un-Digitize
	 */
	undigitize64(a, *nx, *ny, *scale);

	/*
	 * Inverse H-transform
	 */
	stat = hinv64(a, *nx, *ny, smooth, *scale);

        *status = stat;
	
         /* pack the I*8 values back into an I*4 array */
        iarray = (int *) a;
	nval = (*nx) * (*ny);

	for (ii = 0; ii < nval; ii++)
	   iarray[ii] = (int) a[ii];	

  return(*status);
}

/*  ############################################################################  */
/*  ############################################################################  */

/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* hinv.c   Inverse H-transform of NX x NY integer image
 *
 * Programmer: R. White		Date: 23 July 1993
 */

/*  ############################################################################  */
static int 
hinv(int a[], int nx, int ny, int smooth ,int scale)
/*
int smooth;    0 for no smoothing, else smooth during inversion 
int scale;     used if smoothing is specified 
*/
{
int nmax, log2n, i, j, k;
int nxtop,nytop,nxf,nyf,c;
int oddx,oddy;
int shift, bit0, bit1, bit2, mask0, mask1, mask2,
	prnd0, prnd1, prnd2, nrnd0, nrnd1, nrnd2, lowbit0, lowbit1;
int h0, hx, hy, hc;
int s10, s00;
int *tmp;

	/*
	 * log2n is log2 of max(nx,ny) rounded up to next power of 2
	 */
	nmax = (nx>ny) ? nx : ny;
	log2n = (int) (log((float) nmax)/log(2.0)+0.5);
	if ( nmax > (1<<log2n) ) {
		log2n += 1;
	}
	/*
	 * get temporary storage for shuffling elements
	 */  
	tmp = (int *) malloc(((nmax+1)/2)*sizeof(int));
	if (tmp == (int *) NULL) {
		ffpmsg("hinv: insufficient memory");
		return(DATA_DECOMPRESSION_ERR);
	}
	/*
	 * set up masks, rounding parameters
	 */
	shift  = 1;
	bit0   = 1 << (log2n - 1);
	bit1   = bit0 << 1;
	bit2   = bit0 << 2;
	mask0  = -bit0;
	mask1  = mask0 << 1;
	mask2  = mask0 << 2;
	prnd0  = bit0 >> 1;
	prnd1  = bit1 >> 1;
	prnd2  = bit2 >> 1;
	nrnd0  = prnd0 - 1;
	nrnd1  = prnd1 - 1;
	nrnd2  = prnd2 - 1;
	/*
	 * round h0 to multiple of bit2
	 */
	a[0] = (a[0] + ((a[0] >= 0) ? prnd2 : nrnd2)) & mask2;
	/*
	 * do log2n expansions
	 *
	 * We're indexing a as a 2-D array with dimensions (nx,ny).
	 */
	nxtop = 1;
	nytop = 1;
	nxf = nx;
	nyf = ny;
	c = 1<<log2n;
	for (k = log2n-1; k>=0; k--) {
		/*
		 * this somewhat cryptic code generates the sequence
		 * ntop[k-1] = (ntop[k]+1)/2, where ntop[log2n] = n
		 */
		c = c>>1;
		nxtop = nxtop<<1;
		nytop = nytop<<1;
		if (nxf <= c) { nxtop -= 1; } else { nxf -= c; }
		if (nyf <= c) { nytop -= 1; } else { nyf -= c; }
		/*
		 * double shift and fix nrnd0 (because prnd0=0) on last pass
		 */
		if (k == 0) {
			nrnd0 = 0;
			shift = 2;
		}
		/*
		 * unshuffle in each dimension to interleave coefficients
		 */
		for (i = 0; i<nxtop; i++) {
			unshuffle(&a[ny*i],nytop,1,tmp);
		}
		for (j = 0; j<nytop; j++) {
			unshuffle(&a[j],nxtop,ny,tmp);
		}
		/*
		 * smooth by interpolating coefficients if SMOOTH != 0
		 */
		if (smooth) hsmooth(a,nxtop,nytop,ny,scale);
		oddx = nxtop % 2;
		oddy = nytop % 2;
		for (i = 0; i<nxtop-oddx; i += 2) {
			s00 = ny*i;				/* s00 is index of a[i,j]	*/
			s10 = s00+ny;			/* s10 is index of a[i+1,j]	*/
			for (j = 0; j<nytop-oddy; j += 2) {
				h0 = a[s00  ];
				hx = a[s10  ];
				hy = a[s00+1];
				hc = a[s10+1];
				/*
				 * round hx and hy to multiple of bit1, hc to multiple of bit0
				 * h0 is already a multiple of bit2
				 */
				hx = (hx + ((hx >= 0) ? prnd1 : nrnd1)) & mask1;
				hy = (hy + ((hy >= 0) ? prnd1 : nrnd1)) & mask1;
				hc = (hc + ((hc >= 0) ? prnd0 : nrnd0)) & mask0;
				/*
				 * propagate bit0 of hc to hx,hy
				 */
				lowbit0 = hc & bit0;
				hx = (hx >= 0) ? (hx - lowbit0) : (hx + lowbit0);
				hy = (hy >= 0) ? (hy - lowbit0) : (hy + lowbit0);
				/*
				 * Propagate bits 0 and 1 of hc,hx,hy to h0.
				 * This could be simplified if we assume h0>0, but then
				 * the inversion would not be lossless for images with
				 * negative pixels.
				 */
				lowbit1 = (hc ^ hx ^ hy) & bit1;
				h0 = (h0 >= 0)
					? (h0 + lowbit0 - lowbit1)
					: (h0 + ((lowbit0 == 0) ? lowbit1 : (lowbit0-lowbit1)));
				/*
				 * Divide sums by 2 (4 last time)
				 */
				a[s10+1] = (h0 + hx + hy + hc) >> shift;
				a[s10  ] = (h0 + hx - hy - hc) >> shift;
				a[s00+1] = (h0 - hx + hy - hc) >> shift;
				a[s00  ] = (h0 - hx - hy + hc) >> shift;
				s00 += 2;
				s10 += 2;
			}
			if (oddy) {
				/*
				 * do last element in row if row length is odd
				 * s00+1, s10+1 are off edge
				 */
				h0 = a[s00  ];
				hx = a[s10  ];
				hx = ((hx >= 0) ? (hx+prnd1) : (hx+nrnd1)) & mask1;
				lowbit1 = hx & bit1;
				h0 = (h0 >= 0) ? (h0 - lowbit1) : (h0 + lowbit1);
				a[s10  ] = (h0 + hx) >> shift;
				a[s00  ] = (h0 - hx) >> shift;
			}
		}
		if (oddx) {
			/*
			 * do last row if column length is odd
			 * s10, s10+1 are off edge
			 */
			s00 = ny*i;
			for (j = 0; j<nytop-oddy; j += 2) {
				h0 = a[s00  ];
				hy = a[s00+1];
				hy = ((hy >= 0) ? (hy+prnd1) : (hy+nrnd1)) & mask1;
				lowbit1 = hy & bit1;
				h0 = (h0 >= 0) ? (h0 - lowbit1) : (h0 + lowbit1);
				a[s00+1] = (h0 + hy) >> shift;
				a[s00  ] = (h0 - hy) >> shift;
				s00 += 2;
			}
			if (oddy) {
				/*
				 * do corner element if both row and column lengths are odd
				 * s00+1, s10, s10+1 are off edge
				 */
				h0 = a[s00  ];
				a[s00  ] = h0 >> shift;
			}
		}
		/*
		 * divide all the masks and rounding values by 2
		 */
		bit2 = bit1;
		bit1 = bit0;
		bit0 = bit0 >> 1;
		mask1 = mask0;
		mask0 = mask0 >> 1;
		prnd1 = prnd0;
		prnd0 = prnd0 >> 1;
		nrnd1 = nrnd0;
		nrnd0 = prnd0 - 1;
	}
	free(tmp);
	return(0);
}
/*  ############################################################################  */
static int 
hinv64(LONGLONG a[], int nx, int ny, int smooth ,int scale)
/*
int smooth;    0 for no smoothing, else smooth during inversion 
int scale;     used if smoothing is specified 
*/
{
int nmax, log2n, i, j, k;
int nxtop,nytop,nxf,nyf,c;
int oddx,oddy;
int shift;
LONGLONG mask0, mask1, mask2, prnd0, prnd1, prnd2, bit0, bit1, bit2;
LONGLONG  nrnd0, nrnd1, nrnd2, lowbit0, lowbit1;
LONGLONG h0, hx, hy, hc;
int s10, s00;
LONGLONG *tmp;

	/*
	 * log2n is log2 of max(nx,ny) rounded up to next power of 2
	 */
	nmax = (nx>ny) ? nx : ny;
	log2n = (int) (log((float) nmax)/log(2.0)+0.5);
	if ( nmax > (1<<log2n) ) {
		log2n += 1;
	}
	/*
	 * get temporary storage for shuffling elements
	 */  
	tmp = (LONGLONG *) malloc(((nmax+1)/2)*sizeof(LONGLONG));
	if (tmp == (LONGLONG *) NULL) {
		ffpmsg("hinv64: insufficient memory");
		return(DATA_DECOMPRESSION_ERR);
	}
	/*
	 * set up masks, rounding parameters
	 */
	shift  = 1;
	bit0   = ((LONGLONG) 1) << (log2n - 1);
	bit1   = bit0 << 1;
	bit2   = bit0 << 2;
	mask0  = -bit0;
	mask1  = mask0 << 1;
	mask2  = mask0 << 2;
	prnd0  = bit0 >> 1;
	prnd1  = bit1 >> 1;
	prnd2  = bit2 >> 1;
	nrnd0  = prnd0 - 1;
	nrnd1  = prnd1 - 1;
	nrnd2  = prnd2 - 1;
	/*
	 * round h0 to multiple of bit2
	 */
	a[0] = (a[0] + ((a[0] >= 0) ? prnd2 : nrnd2)) & mask2;
	/*
	 * do log2n expansions
	 *
	 * We're indexing a as a 2-D array with dimensions (nx,ny).
	 */
	nxtop = 1;
	nytop = 1;
	nxf = nx;
	nyf = ny;
	c = 1<<log2n;
	for (k = log2n-1; k>=0; k--) {
		/*
		 * this somewhat cryptic code generates the sequence
		 * ntop[k-1] = (ntop[k]+1)/2, where ntop[log2n] = n
		 */
		c = c>>1;
		nxtop = nxtop<<1;
		nytop = nytop<<1;
		if (nxf <= c) { nxtop -= 1; } else { nxf -= c; }
		if (nyf <= c) { nytop -= 1; } else { nyf -= c; }
		/*
		 * double shift and fix nrnd0 (because prnd0=0) on last pass
		 */
		if (k == 0) {
			nrnd0 = 0;
			shift = 2;
		}
		/*
		 * unshuffle in each dimension to interleave coefficients
		 */
		for (i = 0; i<nxtop; i++) {
			unshuffle64(&a[ny*i],nytop,1,tmp);
		}
		for (j = 0; j<nytop; j++) {
			unshuffle64(&a[j],nxtop,ny,tmp);
		}
		/*
		 * smooth by interpolating coefficients if SMOOTH != 0
		 */
		if (smooth) hsmooth64(a,nxtop,nytop,ny,scale);
		oddx = nxtop % 2;
		oddy = nytop % 2;
		for (i = 0; i<nxtop-oddx; i += 2) {
			s00 = ny*i;				/* s00 is index of a[i,j]	*/
			s10 = s00+ny;			/* s10 is index of a[i+1,j]	*/
			for (j = 0; j<nytop-oddy; j += 2) {
				h0 = a[s00  ];
				hx = a[s10  ];
				hy = a[s00+1];
				hc = a[s10+1];
				/*
				 * round hx and hy to multiple of bit1, hc to multiple of bit0
				 * h0 is already a multiple of bit2
				 */
				hx = (hx + ((hx >= 0) ? prnd1 : nrnd1)) & mask1;
				hy = (hy + ((hy >= 0) ? prnd1 : nrnd1)) & mask1;
				hc = (hc + ((hc >= 0) ? prnd0 : nrnd0)) & mask0;
				/*
				 * propagate bit0 of hc to hx,hy
				 */
				lowbit0 = hc & bit0;
				hx = (hx >= 0) ? (hx - lowbit0) : (hx + lowbit0);
				hy = (hy >= 0) ? (hy - lowbit0) : (hy + lowbit0);
				/*
				 * Propagate bits 0 and 1 of hc,hx,hy to h0.
				 * This could be simplified if we assume h0>0, but then
				 * the inversion would not be lossless for images with
				 * negative pixels.
				 */
				lowbit1 = (hc ^ hx ^ hy) & bit1;
				h0 = (h0 >= 0)
					? (h0 + lowbit0 - lowbit1)
					: (h0 + ((lowbit0 == 0) ? lowbit1 : (lowbit0-lowbit1)));
				/*
				 * Divide sums by 2 (4 last time)
				 */
				a[s10+1] = (h0 + hx + hy + hc) >> shift;
				a[s10  ] = (h0 + hx - hy - hc) >> shift;
				a[s00+1] = (h0 - hx + hy - hc) >> shift;
				a[s00  ] = (h0 - hx - hy + hc) >> shift;
				s00 += 2;
				s10 += 2;
			}
			if (oddy) {
				/*
				 * do last element in row if row length is odd
				 * s00+1, s10+1 are off edge
				 */
				h0 = a[s00  ];
				hx = a[s10  ];
				hx = ((hx >= 0) ? (hx+prnd1) : (hx+nrnd1)) & mask1;
				lowbit1 = hx & bit1;
				h0 = (h0 >= 0) ? (h0 - lowbit1) : (h0 + lowbit1);
				a[s10  ] = (h0 + hx) >> shift;
				a[s00  ] = (h0 - hx) >> shift;
			}
		}
		if (oddx) {
			/*
			 * do last row if column length is odd
			 * s10, s10+1 are off edge
			 */
			s00 = ny*i;
			for (j = 0; j<nytop-oddy; j += 2) {
				h0 = a[s00  ];
				hy = a[s00+1];
				hy = ((hy >= 0) ? (hy+prnd1) : (hy+nrnd1)) & mask1;
				lowbit1 = hy & bit1;
				h0 = (h0 >= 0) ? (h0 - lowbit1) : (h0 + lowbit1);
				a[s00+1] = (h0 + hy) >> shift;
				a[s00  ] = (h0 - hy) >> shift;
				s00 += 2;
			}
			if (oddy) {
				/*
				 * do corner element if both row and column lengths are odd
				 * s00+1, s10, s10+1 are off edge
				 */
				h0 = a[s00  ];
				a[s00  ] = h0 >> shift;
			}
		}
		/*
		 * divide all the masks and rounding values by 2
		 */
		bit2 = bit1;
		bit1 = bit0;
		bit0 = bit0 >> 1;
		mask1 = mask0;
		mask0 = mask0 >> 1;
		prnd1 = prnd0;
		prnd0 = prnd0 >> 1;
		nrnd1 = nrnd0;
		nrnd0 = prnd0 - 1;
	}
	free(tmp);
	return(0);
}

/*  ############################################################################  */
static void
unshuffle(int a[], int n, int n2, int tmp[])
/*
int a[];	 array to shuffle					
int n;		 number of elements to shuffle	
int n2;		 second dimension					
int tmp[];	 scratch storage					
*/
{
int i;
int nhalf;
int *p1, *p2, *pt;
 
	/*
	 * copy 2nd half of array to tmp
	 */
	nhalf = (n+1)>>1;
	pt = tmp;
	p1 = &a[n2*nhalf];				/* pointer to a[i]			*/
	for (i=nhalf; i<n; i++) {
		*pt = *p1;
		p1 += n2;
		pt += 1;
	}
	/*
	 * distribute 1st half of array to even elements
	 */
	p2 = &a[ n2*(nhalf-1) ];		/* pointer to a[i]			*/
	p1 = &a[(n2*(nhalf-1))<<1];		/* pointer to a[2*i]		*/
	for (i=nhalf-1; i >= 0; i--) {
		*p1 = *p2;
		p2 -= n2;
		p1 -= (n2+n2);
	}
	/*
	 * now distribute 2nd half of array (in tmp) to odd elements
	 */
	pt = tmp;
	p1 = &a[n2];					/* pointer to a[i]			*/
	for (i=1; i<n; i += 2) {
		*p1 = *pt;
		p1 += (n2+n2);
		pt += 1;
	}
}
/*  ############################################################################  */
static void
unshuffle64(LONGLONG a[], int n, int n2, LONGLONG tmp[])
/*
LONGLONG a[];	 array to shuffle					
int n;		 number of elements to shuffle	
int n2;		 second dimension					
LONGLONG tmp[];	 scratch storage					
*/
{
int i;
int nhalf;
LONGLONG *p1, *p2, *pt;
 
	/*
	 * copy 2nd half of array to tmp
	 */
	nhalf = (n+1)>>1;
	pt = tmp;
	p1 = &a[n2*nhalf];				/* pointer to a[i]			*/
	for (i=nhalf; i<n; i++) {
		*pt = *p1;
		p1 += n2;
		pt += 1;
	}
	/*
	 * distribute 1st half of array to even elements
	 */
	p2 = &a[ n2*(nhalf-1) ];		/* pointer to a[i]			*/
	p1 = &a[(n2*(nhalf-1))<<1];		/* pointer to a[2*i]		*/
	for (i=nhalf-1; i >= 0; i--) {
		*p1 = *p2;
		p2 -= n2;
		p1 -= (n2+n2);
	}
	/*
	 * now distribute 2nd half of array (in tmp) to odd elements
	 */
	pt = tmp;
	p1 = &a[n2];					/* pointer to a[i]			*/
	for (i=1; i<n; i += 2) {
		*p1 = *pt;
		p1 += (n2+n2);
		pt += 1;
	}
}

/*  ############################################################################  */
/*  ############################################################################  */

/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* hsmooth.c	Smooth H-transform image by adjusting coefficients toward
 *				interpolated values
 *
 * Programmer: R. White		Date: 13 April 1992
 */

/*  ############################################################################  */
static void 
hsmooth(int a[], int nxtop, int nytop, int ny, int scale)
/*
int a[];			 array of H-transform coefficients		
int nxtop,nytop;	 size of coefficient block to use			
int ny;				 actual 1st dimension of array			
int scale;			 truncation scale factor that was used	
*/
{
int i, j;
int ny2, s10, s00, diff, dmax, dmin, s, smax;
int hm, h0, hp, hmm, hpm, hmp, hpp, hx2, hy2;
int m1,m2;

	/*
	 * Maximum change in coefficients is determined by scale factor.
	 * Since we rounded during division (see digitize.c), the biggest
	 * permitted change is scale/2.
	 */
	smax = (scale >> 1);
	if (smax <= 0) return;
	ny2 = ny << 1;
	/*
	 * We're indexing a as a 2-D array with dimensions (nxtop,ny) of which
	 * only (nxtop,nytop) are used.  The coefficients on the edge of the
	 * array are not adjusted (which is why the loops below start at 2
	 * instead of 0 and end at nxtop-2 instead of nxtop.)
	 */
	/*
	 * Adjust x difference hx
	 */
	for (i = 2; i<nxtop-2; i += 2) {
		s00 = ny*i;				/* s00 is index of a[i,j]	*/
		s10 = s00+ny;			/* s10 is index of a[i+1,j]	*/
		for (j = 0; j<nytop; j += 2) {
			/*
			 * hp is h0 (mean value) in next x zone, hm is h0 in previous x zone
			 */
			hm = a[s00-ny2];
			h0 = a[s00];
			hp = a[s00+ny2];
			/*
			 * diff = 8 * hx slope that would match h0 in neighboring zones
			 */
			diff = hp-hm;
			/*
			 * monotonicity constraints on diff
			 */
			dmax = max( min( (hp-h0), (h0-hm) ), 0 ) << 2;
			dmin = min( max( (hp-h0), (h0-hm) ), 0 ) << 2;
			/*
			 * if monotonicity would set slope = 0 then don't change hx.
			 * note dmax>=0, dmin<=0.
			 */
			if (dmin < dmax) {
				diff = max( min(diff, dmax), dmin);
				/*
				 * Compute change in slope limited to range +/- smax.
				 * Careful with rounding negative numbers when using
				 * shift for divide by 8.
				 */
				s = diff-(a[s10]<<3);
				s = (s>=0) ? (s>>3) : ((s+7)>>3) ;
				s = max( min(s, smax), -smax);
				a[s10] = a[s10]+s;
			}
			s00 += 2;
			s10 += 2;
		}
	}
	/*
	 * Adjust y difference hy
	 */
	for (i = 0; i<nxtop; i += 2) {
		s00 = ny*i+2;
		s10 = s00+ny;
		for (j = 2; j<nytop-2; j += 2) {
			hm = a[s00-2];
			h0 = a[s00];
			hp = a[s00+2];
			diff = hp-hm;
			dmax = max( min( (hp-h0), (h0-hm) ), 0 ) << 2;
			dmin = min( max( (hp-h0), (h0-hm) ), 0 ) << 2;
			if (dmin < dmax) {
				diff = max( min(diff, dmax), dmin);
				s = diff-(a[s00+1]<<3);
				s = (s>=0) ? (s>>3) : ((s+7)>>3) ;
				s = max( min(s, smax), -smax);
				a[s00+1] = a[s00+1]+s;
			}
			s00 += 2;
			s10 += 2;
		}
	}
	/*
	 * Adjust curvature difference hc
	 */
	for (i = 2; i<nxtop-2; i += 2) {
		s00 = ny*i+2;
		s10 = s00+ny;
		for (j = 2; j<nytop-2; j += 2) {
			/*
			 * ------------------    y
			 * | hmp |    | hpp |    |
			 * ------------------    |
			 * |     | h0 |     |    |
			 * ------------------    -------x
			 * | hmm |    | hpm |
			 * ------------------
			 */
			hmm = a[s00-ny2-2];
			hpm = a[s00+ny2-2];
			hmp = a[s00-ny2+2];
			hpp = a[s00+ny2+2];
			h0  = a[s00];
			/*
			 * diff = 64 * hc value that would match h0 in neighboring zones
			 */
			diff = hpp + hmm - hmp - hpm;
			/*
			 * 2 times x,y slopes in this zone
			 */
			hx2 = a[s10  ]<<1;
			hy2 = a[s00+1]<<1;
			/*
			 * monotonicity constraints on diff
			 */
			m1 = min(max(hpp-h0,0)-hx2-hy2, max(h0-hpm,0)+hx2-hy2);
			m2 = min(max(h0-hmp,0)-hx2+hy2, max(hmm-h0,0)+hx2+hy2);
			dmax = min(m1,m2) << 4;
			m1 = max(min(hpp-h0,0)-hx2-hy2, min(h0-hpm,0)+hx2-hy2);
			m2 = max(min(h0-hmp,0)-hx2+hy2, min(hmm-h0,0)+hx2+hy2);
			dmin = max(m1,m2) << 4;
			/*
			 * if monotonicity would set slope = 0 then don't change hc.
			 * note dmax>=0, dmin<=0.
			 */
			if (dmin < dmax) {
				diff = max( min(diff, dmax), dmin);
				/*
				 * Compute change in slope limited to range +/- smax.
				 * Careful with rounding negative numbers when using
				 * shift for divide by 64.
				 */
				s = diff-(a[s10+1]<<6);
				s = (s>=0) ? (s>>6) : ((s+63)>>6) ;
				s = max( min(s, smax), -smax);
				a[s10+1] = a[s10+1]+s;
			}
			s00 += 2;
			s10 += 2;
		}
	}
}
/*  ############################################################################  */
static void 
hsmooth64(LONGLONG a[], int nxtop, int nytop, int ny, int scale)
/*
LONGLONG a[];			 array of H-transform coefficients		
int nxtop,nytop;	 size of coefficient block to use			
int ny;				 actual 1st dimension of array			
int scale;			 truncation scale factor that was used	
*/
{
int i, j;
int ny2, s10, s00;
LONGLONG hm, h0, hp, hmm, hpm, hmp, hpp, hx2, hy2, diff, dmax, dmin, s, smax, m1, m2;

	/*
	 * Maximum change in coefficients is determined by scale factor.
	 * Since we rounded during division (see digitize.c), the biggest
	 * permitted change is scale/2.
	 */
	smax = (scale >> 1);
	if (smax <= 0) return;
	ny2 = ny << 1;
	/*
	 * We're indexing a as a 2-D array with dimensions (nxtop,ny) of which
	 * only (nxtop,nytop) are used.  The coefficients on the edge of the
	 * array are not adjusted (which is why the loops below start at 2
	 * instead of 0 and end at nxtop-2 instead of nxtop.)
	 */
	/*
	 * Adjust x difference hx
	 */
	for (i = 2; i<nxtop-2; i += 2) {
		s00 = ny*i;				/* s00 is index of a[i,j]	*/
		s10 = s00+ny;			/* s10 is index of a[i+1,j]	*/
		for (j = 0; j<nytop; j += 2) {
			/*
			 * hp is h0 (mean value) in next x zone, hm is h0 in previous x zone
			 */
			hm = a[s00-ny2];
			h0 = a[s00];
			hp = a[s00+ny2];
			/*
			 * diff = 8 * hx slope that would match h0 in neighboring zones
			 */
			diff = hp-hm;
			/*
			 * monotonicity constraints on diff
			 */
			dmax = max( min( (hp-h0), (h0-hm) ), 0 ) << 2;
			dmin = min( max( (hp-h0), (h0-hm) ), 0 ) << 2;
			/*
			 * if monotonicity would set slope = 0 then don't change hx.
			 * note dmax>=0, dmin<=0.
			 */
			if (dmin < dmax) {
				diff = max( min(diff, dmax), dmin);
				/*
				 * Compute change in slope limited to range +/- smax.
				 * Careful with rounding negative numbers when using
				 * shift for divide by 8.
				 */
				s = diff-(a[s10]<<3);
				s = (s>=0) ? (s>>3) : ((s+7)>>3) ;
				s = max( min(s, smax), -smax);
				a[s10] = a[s10]+s;
			}
			s00 += 2;
			s10 += 2;
		}
	}
	/*
	 * Adjust y difference hy
	 */
	for (i = 0; i<nxtop; i += 2) {
		s00 = ny*i+2;
		s10 = s00+ny;
		for (j = 2; j<nytop-2; j += 2) {
			hm = a[s00-2];
			h0 = a[s00];
			hp = a[s00+2];
			diff = hp-hm;
			dmax = max( min( (hp-h0), (h0-hm) ), 0 ) << 2;
			dmin = min( max( (hp-h0), (h0-hm) ), 0 ) << 2;
			if (dmin < dmax) {
				diff = max( min(diff, dmax), dmin);
				s = diff-(a[s00+1]<<3);
				s = (s>=0) ? (s>>3) : ((s+7)>>3) ;
				s = max( min(s, smax), -smax);
				a[s00+1] = a[s00+1]+s;
			}
			s00 += 2;
			s10 += 2;
		}
	}
	/*
	 * Adjust curvature difference hc
	 */
	for (i = 2; i<nxtop-2; i += 2) {
		s00 = ny*i+2;
		s10 = s00+ny;
		for (j = 2; j<nytop-2; j += 2) {
			/*
			 * ------------------    y
			 * | hmp |    | hpp |    |
			 * ------------------    |
			 * |     | h0 |     |    |
			 * ------------------    -------x
			 * | hmm |    | hpm |
			 * ------------------
			 */
			hmm = a[s00-ny2-2];
			hpm = a[s00+ny2-2];
			hmp = a[s00-ny2+2];
			hpp = a[s00+ny2+2];
			h0  = a[s00];
			/*
			 * diff = 64 * hc value that would match h0 in neighboring zones
			 */
			diff = hpp + hmm - hmp - hpm;
			/*
			 * 2 times x,y slopes in this zone
			 */
			hx2 = a[s10  ]<<1;
			hy2 = a[s00+1]<<1;
			/*
			 * monotonicity constraints on diff
			 */
			m1 = min(max(hpp-h0,0)-hx2-hy2, max(h0-hpm,0)+hx2-hy2);
			m2 = min(max(h0-hmp,0)-hx2+hy2, max(hmm-h0,0)+hx2+hy2);
			dmax = min(m1,m2) << 4;
			m1 = max(min(hpp-h0,0)-hx2-hy2, min(h0-hpm,0)+hx2-hy2);
			m2 = max(min(h0-hmp,0)-hx2+hy2, min(hmm-h0,0)+hx2+hy2);
			dmin = max(m1,m2) << 4;
			/*
			 * if monotonicity would set slope = 0 then don't change hc.
			 * note dmax>=0, dmin<=0.
			 */
			if (dmin < dmax) {
				diff = max( min(diff, dmax), dmin);
				/*
				 * Compute change in slope limited to range +/- smax.
				 * Careful with rounding negative numbers when using
				 * shift for divide by 64.
				 */
				s = diff-(a[s10+1]<<6);
				s = (s>=0) ? (s>>6) : ((s+63)>>6) ;
				s = max( min(s, smax), -smax);
				a[s10+1] = a[s10+1]+s;
			}
			s00 += 2;
			s10 += 2;
		}
	}
}


/*  ############################################################################  */
/*  ############################################################################  */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* undigitize.c		undigitize H-transform
 *
 * Programmer: R. White		Date: 9 May 1991
 */

/*  ############################################################################  */
static void
undigitize(int a[], int nx, int ny, int scale)
{
int *p;

	/*
	 * multiply by scale
	 */
	if (scale <= 1) return;
	for (p=a; p <= &a[nx*ny-1]; p++) *p = (*p)*scale;
}
/*  ############################################################################  */
static void
undigitize64(LONGLONG a[], int nx, int ny, int scale)
{
LONGLONG *p, scale64;

	/*
	 * multiply by scale
	 */
	if (scale <= 1) return;
	scale64 = (LONGLONG) scale;   /* use a 64-bit int for efficiency in the big loop */
	
	for (p=a; p <= &a[nx*ny-1]; p++) *p = (*p)*scale64;
}

/*  ############################################################################  */
/*  ############################################################################  */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* decode.c		read codes from infile and construct array
 *
 * Programmer: R. White		Date: 2 February 1994
 */


static char code_magic[2] = { (char)0xDD, (char)0x99 };

/*  ############################################################################  */
static int decode(unsigned char *infile, int *a, int *nx, int *ny, int *scale)
/*
char *infile;				 input file							
int  *a;				 address of output array [nx][ny]		
int  *nx,*ny;				 size of output array					
int  *scale;				 scale factor for digitization		
*/
{
LONGLONG sumall;
int stat;
unsigned char nbitplanes[3];
char tmagic[2];

	/* initialize the byte read position to the beginning of the array */;
	nextchar = 0;
	
	/*
	 * File starts either with special 2-byte magic code or with
	 * FITS keyword "SIMPLE  ="
	 */
	qread(infile, tmagic, sizeof(tmagic));
	/*
	 * check for correct magic code value
	 */
	if (memcmp(tmagic,code_magic,sizeof(code_magic)) != 0) {
		ffpmsg("bad file format");
		return(DATA_DECOMPRESSION_ERR);
	}
	*nx =readint(infile);				/* x size of image			*/
	*ny =readint(infile);				/* y size of image			*/
	*scale=readint(infile);				/* scale factor for digitization	*/
	
	/* sum of all pixels	*/
	sumall=readlonglong(infile);
	/* # bits in quadrants	*/

	qread(infile, (char *) nbitplanes, sizeof(nbitplanes));

	stat = dodecode(infile, a, *nx, *ny, nbitplanes);
	/*
	 * put sum of all pixels back into pixel 0
	 */
	a[0] = (int) sumall;
	return(stat);
}
/*  ############################################################################  */
static int decode64(unsigned char *infile, LONGLONG *a, int *nx, int *ny, int *scale)
/*
char *infile;				 input file							
LONGLONG  *a;				 address of output array [nx][ny]		
int  *nx,*ny;				 size of output array					
int  *scale;				 scale factor for digitization		
*/
{
int stat;
LONGLONG sumall;
unsigned char nbitplanes[3];
char tmagic[2];

	/* initialize the byte read position to the beginning of the array */;
	nextchar = 0;
	
	/*
	 * File starts either with special 2-byte magic code or with
	 * FITS keyword "SIMPLE  ="
	 */
	qread(infile, tmagic, sizeof(tmagic));
	/*
	 * check for correct magic code value
	 */
	if (memcmp(tmagic,code_magic,sizeof(code_magic)) != 0) {
		ffpmsg("bad file format");
		return(DATA_DECOMPRESSION_ERR);
	}
	*nx =readint(infile);				/* x size of image			*/
	*ny =readint(infile);				/* y size of image			*/
	*scale=readint(infile);				/* scale factor for digitization	*/
	
	/* sum of all pixels	*/
	sumall=readlonglong(infile);
	/* # bits in quadrants	*/

	qread(infile, (char *) nbitplanes, sizeof(nbitplanes));

	stat = dodecode64(infile, a, *nx, *ny, nbitplanes);
	/*
	 * put sum of all pixels back into pixel 0
	 */
	a[0] = sumall;

	return(stat);
}


/*  ############################################################################  */
/*  ############################################################################  */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* dodecode.c	Decode stream of characters on infile and return array
 *
 * This version encodes the different quadrants separately
 *
 * Programmer: R. White		Date: 9 May 1991
 */

/*  ############################################################################  */
static int
dodecode(unsigned char *infile, int a[], int nx, int ny, unsigned char nbitplanes[3])

/* int a[];					 			
   int nx,ny;					 Array dimensions are [nx][ny]		
   unsigned char nbitplanes[3];		 Number of bit planes in quadrants
*/
{
int i, nel, nx2, ny2, stat;

	nel = nx*ny;
	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;

	/*
	 * initialize a to zero
	 */
	for (i=0; i<nel; i++) a[i] = 0;
	/*
	 * Initialize bit input
	 */
	start_inputing_bits();
	/*
	 * read bit planes for each quadrant
	 */
	stat = qtree_decode(infile, &a[0],          ny, nx2,  ny2,  nbitplanes[0]);
        if (stat) return(stat);
	
	stat = qtree_decode(infile, &a[ny2],        ny, nx2,  ny/2, nbitplanes[1]);
        if (stat) return(stat);
	
	stat = qtree_decode(infile, &a[ny*nx2],     ny, nx/2, ny2,  nbitplanes[1]);
        if (stat) return(stat);
	
	stat = qtree_decode(infile, &a[ny*nx2+ny2], ny, nx/2, ny/2, nbitplanes[2]);
        if (stat) return(stat);
	
	/*
	 * make sure there is an EOF symbol (nybble=0) at end
	 */
	if (input_nybble(infile) != 0) {
		ffpmsg("dodecode: bad bit plane values");
		return(DATA_DECOMPRESSION_ERR);
	}
	/*
	 * now get the sign bits
	 * Re-initialize bit input
	 */
	start_inputing_bits();
	for (i=0; i<nel; i++) {
		if (a[i]) {
			/* tried putting the input_bit code in-line here, instead of */
			/* calling the function, but it made no difference in the speed */
			if (input_bit(infile)) a[i] = -a[i];
		}
	}
	return(0);
}
/*  ############################################################################  */
static int
dodecode64(unsigned char *infile, LONGLONG a[], int nx, int ny, unsigned char nbitplanes[3])

/* LONGLONG a[];					 			
   int nx,ny;					 Array dimensions are [nx][ny]		
   unsigned char nbitplanes[3];		 Number of bit planes in quadrants
*/
{
int i, nel, nx2, ny2, stat;

	nel = nx*ny;
	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;

	/*
	 * initialize a to zero
	 */
	for (i=0; i<nel; i++) a[i] = 0;
	/*
	 * Initialize bit input
	 */
	start_inputing_bits();
	/*
	 * read bit planes for each quadrant
	 */
	stat = qtree_decode64(infile, &a[0],          ny, nx2,  ny2,  nbitplanes[0]);
        if (stat) return(stat);
	
	stat = qtree_decode64(infile, &a[ny2],        ny, nx2,  ny/2, nbitplanes[1]);
        if (stat) return(stat);
	
	stat = qtree_decode64(infile, &a[ny*nx2],     ny, nx/2, ny2,  nbitplanes[1]);
        if (stat) return(stat);
	
	stat = qtree_decode64(infile, &a[ny*nx2+ny2], ny, nx/2, ny/2, nbitplanes[2]);
        if (stat) return(stat);
	
	/*
	 * make sure there is an EOF symbol (nybble=0) at end
	 */
	if (input_nybble(infile) != 0) {
		ffpmsg("dodecode64: bad bit plane values");
		return(DATA_DECOMPRESSION_ERR);
	}
	/*
	 * now get the sign bits
	 * Re-initialize bit input
	 */
	start_inputing_bits();
	for (i=0; i<nel; i++) {
		if (a[i]) {
			if (input_bit(infile) != 0) a[i] = -a[i];
		}
	}
	return(0);
}

/*  ############################################################################  */
/*  ############################################################################  */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* qtree_decode.c	Read stream of codes from infile and construct bit planes
 *					in quadrant of 2-D array using binary quadtree coding
 *
 * Programmer: R. White		Date: 7 May 1991
 */

/*  ############################################################################  */
static int
qtree_decode(unsigned char *infile, int a[], int n, int nqx, int nqy, int nbitplanes)

/*
char *infile;
int a[];				 a is 2-D array with dimensions (n,n)	
int n;					 length of full row in a				
int nqx;				 partial length of row to decode		
int nqy;				 partial length of column (<=n)		
int nbitplanes;				 number of bitplanes to decode		
*/
{
int log2n, k, bit, b, nqmax;
int nx,ny,nfx,nfy,c;
int nqx2, nqy2;
unsigned char *scratch;

	/*
	 * log2n is log2 of max(nqx,nqy) rounded up to next power of 2
	 */
	nqmax = (nqx>nqy) ? nqx : nqy;
	log2n = (int) (log((float) nqmax)/log(2.0)+0.5);
	if (nqmax > (1<<log2n)) {
		log2n += 1;
	}
	/*
	 * allocate scratch array for working space
	 */
	nqx2=(nqx+1)/2;
	nqy2=(nqy+1)/2;
	scratch = (unsigned char *) malloc(nqx2*nqy2);
	if (scratch == (unsigned char *) NULL) {
		ffpmsg("qtree_decode: insufficient memory");
		return(DATA_DECOMPRESSION_ERR);
	}
	/*
	 * now decode each bit plane, starting at the top
	 * A is assumed to be initialized to zero
	 */
	for (bit = nbitplanes-1; bit >= 0; bit--) {
		/*
		 * Was bitplane was quadtree-coded or written directly?
		 */
		b = input_nybble(infile);

		if(b == 0) {
			/*
			 * bit map was written directly
			 */
			read_bdirect(infile,a,n,nqx,nqy,scratch,bit);
		} else if (b != 0xf) {
			ffpmsg("qtree_decode: bad format code");
			return(DATA_DECOMPRESSION_ERR);
		} else {
			/*
			 * bitmap was quadtree-coded, do log2n expansions
			 *
			 * read first code
			 */
			scratch[0] = input_huffman(infile);
			/*
			 * now do log2n expansions, reading codes from file as necessary
			 */
			nx = 1;
			ny = 1;
			nfx = nqx;
			nfy = nqy;
			c = 1<<log2n;
			for (k = 1; k<log2n; k++) {
				/*
				 * this somewhat cryptic code generates the sequence
				 * n[k-1] = (n[k]+1)/2 where n[log2n]=nqx or nqy
				 */
				c = c>>1;
				nx = nx<<1;
				ny = ny<<1;
				if (nfx <= c) { nx -= 1; } else { nfx -= c; }
				if (nfy <= c) { ny -= 1; } else { nfy -= c; }
				qtree_expand(infile,scratch,nx,ny,scratch);
			}
			/*
			 * now copy last set of 4-bit codes to bitplane bit of array a
			 */
			qtree_bitins(scratch,nqx,nqy,a,n,bit);
		}
	}
	free(scratch);
	return(0);
}
/*  ############################################################################  */
static int
qtree_decode64(unsigned char *infile, LONGLONG a[], int n, int nqx, int nqy, int nbitplanes)

/*
char *infile;
LONGLONG a[];				 a is 2-D array with dimensions (n,n)	
int n;					 length of full row in a				
int nqx;				 partial length of row to decode		
int nqy;				 partial length of column (<=n)		
int nbitplanes;				 number of bitplanes to decode		
*/
{
int log2n, k, bit, b, nqmax;
int nx,ny,nfx,nfy,c;
int nqx2, nqy2;
unsigned char *scratch;

	/*
	 * log2n is log2 of max(nqx,nqy) rounded up to next power of 2
	 */
	nqmax = (nqx>nqy) ? nqx : nqy;
	log2n = (int) (log((float) nqmax)/log(2.0)+0.5);
	if (nqmax > (1<<log2n)) {
		log2n += 1;
	}
	/*
	 * allocate scratch array for working space
	 */
	nqx2=(nqx+1)/2;
	nqy2=(nqy+1)/2;
	scratch = (unsigned char *) malloc(nqx2*nqy2);
	if (scratch == (unsigned char *) NULL) {
		ffpmsg("qtree_decode64: insufficient memory");
		return(DATA_DECOMPRESSION_ERR);
	}
	/*
	 * now decode each bit plane, starting at the top
	 * A is assumed to be initialized to zero
	 */
	for (bit = nbitplanes-1; bit >= 0; bit--) {
		/*
		 * Was bitplane was quadtree-coded or written directly?
		 */
		b = input_nybble(infile);

		if(b == 0) {
			/*
			 * bit map was written directly
			 */
			read_bdirect64(infile,a,n,nqx,nqy,scratch,bit);
		} else if (b != 0xf) {
			ffpmsg("qtree_decode64: bad format code");
			return(DATA_DECOMPRESSION_ERR);
		} else {
			/*
			 * bitmap was quadtree-coded, do log2n expansions
			 *
			 * read first code
			 */
			scratch[0] = input_huffman(infile);
			/*
			 * now do log2n expansions, reading codes from file as necessary
			 */
			nx = 1;
			ny = 1;
			nfx = nqx;
			nfy = nqy;
			c = 1<<log2n;
			for (k = 1; k<log2n; k++) {
				/*
				 * this somewhat cryptic code generates the sequence
				 * n[k-1] = (n[k]+1)/2 where n[log2n]=nqx or nqy
				 */
				c = c>>1;
				nx = nx<<1;
				ny = ny<<1;
				if (nfx <= c) { nx -= 1; } else { nfx -= c; }
				if (nfy <= c) { ny -= 1; } else { nfy -= c; }
				qtree_expand(infile,scratch,nx,ny,scratch);
			}
			/*
			 * now copy last set of 4-bit codes to bitplane bit of array a
			 */
			qtree_bitins64(scratch,nqx,nqy,a,n,bit);
		}
	}
	free(scratch);
	return(0);
}


/*  ############################################################################  */
/*
 * do one quadtree expansion step on array a[(nqx+1)/2,(nqy+1)/2]
 * results put into b[nqx,nqy] (which may be the same as a)
 */
static void
qtree_expand(unsigned char *infile, unsigned char a[], int nx, int ny, unsigned char b[])
{
int i;

	/*
	 * first copy a to b, expanding each 4-bit value
	 */
	qtree_copy(a,nx,ny,b,ny);
	/*
	 * now read new 4-bit values into b for each non-zero element
	 */
	for (i = nx*ny-1; i >= 0; i--) {
		if (b[i]) b[i] = input_huffman(infile);
	}
}

/*  ############################################################################  */
/*
 * copy 4-bit values from a[(nx+1)/2,(ny+1)/2] to b[nx,ny], expanding
 * each value to 2x2 pixels
 * a,b may be same array
 */
static void
qtree_copy(unsigned char a[], int nx, int ny, unsigned char b[], int n)
/*   int n;		declared y dimension of b */
{
int i, j, k, nx2, ny2;
int s00, s10;

	/*
	 * first copy 4-bit values to b
	 * start at end in case a,b are same array
	 */
	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;
	k = ny2*(nx2-1)+ny2-1;			/* k   is index of a[i,j]		*/
	for (i = nx2-1; i >= 0; i--) {
		s00 = 2*(n*i+ny2-1);		/* s00 is index of b[2*i,2*j]		*/
		for (j = ny2-1; j >= 0; j--) {
			b[s00] = a[k];
			k -= 1;
			s00 -= 2;
		}
	}
	/*
	 * now expand each 2x2 block
	 */
	for (i = 0; i<nx-1; i += 2) {

  /* Note:
     Unlike the case in qtree_bitins, this code runs faster on a 32-bit linux
     machine using the s10 intermediate variable, rather that using s00+n. 
     Go figure!
  */
		s00 = n*i;				/* s00 is index of b[i,j]	*/
		s10 = s00+n;				/* s10 is index of b[i+1,j]	*/

		for (j = 0; j<ny-1; j += 2) {

		    switch (b[s00]) {
		    case(0):
			b[s10+1] = 0;
			b[s10  ] = 0;
			b[s00+1] = 0;
			b[s00  ] = 0;

			break;
		    case(1):
			b[s10+1] = 1;
			b[s10  ] = 0;
			b[s00+1] = 0;
			b[s00  ] = 0;

			break;
		    case(2):
			b[s10+1] = 0;
			b[s10  ] = 1;
			b[s00+1] = 0;
			b[s00  ] = 0;

			break;
		    case(3):
			b[s10+1] = 1;
			b[s10  ] = 1;
			b[s00+1] = 0;
			b[s00  ] = 0;

			break;
		    case(4):
			b[s10+1] = 0;
			b[s10  ] = 0;
			b[s00+1] = 1;
			b[s00  ] = 0;

			break;
		    case(5):
			b[s10+1] = 1;
			b[s10  ] = 0;
			b[s00+1] = 1;
			b[s00  ] = 0;

			break;
		    case(6):
			b[s10+1] = 0;
			b[s10  ] = 1;
			b[s00+1] = 1;
			b[s00  ] = 0;

			break;
		    case(7):
			b[s10+1] = 1;
			b[s10  ] = 1;
			b[s00+1] = 1;
			b[s00  ] = 0;

			break;
		    case(8):
			b[s10+1] = 0;
			b[s10  ] = 0;
			b[s00+1] = 0;
			b[s00  ] = 1;

			break;
		    case(9):
			b[s10+1] = 1;
			b[s10  ] = 0;
			b[s00+1] = 0;
			b[s00  ] = 1;
			break;
		    case(10):
			b[s10+1] = 0;
			b[s10  ] = 1;
			b[s00+1] = 0;
			b[s00  ] = 1;

			break;
		    case(11):
			b[s10+1] = 1;
			b[s10  ] = 1;
			b[s00+1] = 0;
			b[s00  ] = 1;

			break;
		    case(12):
			b[s10+1] = 0;
			b[s10  ] = 0;
			b[s00+1] = 1;
			b[s00  ] = 1;

			break;
		    case(13):
			b[s10+1] = 1;
			b[s10  ] = 0;
			b[s00+1] = 1;
			b[s00  ] = 1;

			break;
		    case(14):
			b[s10+1] = 0;
			b[s10  ] = 1;
			b[s00+1] = 1;
			b[s00  ] = 1;

			break;
		    case(15):
			b[s10+1] = 1;
			b[s10  ] = 1;
			b[s00+1] = 1;
			b[s00  ] = 1;

			break;
		    }
/*
			b[s10+1] =  b[s00]     & 1;
			b[s10  ] = (b[s00]>>1) & 1;
			b[s00+1] = (b[s00]>>2) & 1;
			b[s00  ] = (b[s00]>>3) & 1;
*/

			s00 += 2;
			s10 += 2;
		}

		if (j < ny) {
			/*
			 * row size is odd, do last element in row
			 * s00+1, s10+1 are off edge
			 */
                        /* not worth converting this to use 16 case statements */
			b[s10  ] = (b[s00]>>1) & 1;
			b[s00  ] = (b[s00]>>3) & 1;
		}
	}
	if (i < nx) {
		/*
		 * column size is odd, do last row
		 * s10, s10+1 are off edge
		 */
		s00 = n*i;
		for (j = 0; j<ny-1; j += 2) {
                        /* not worth converting this to use 16 case statements */
			b[s00+1] = (b[s00]>>2) & 1;
			b[s00  ] = (b[s00]>>3) & 1;
			s00 += 2;
		}
		if (j < ny) {
			/*
			 * both row and column size are odd, do corner element
			 * s00+1, s10, s10+1 are off edge
			 */
                        /* not worth converting this to use 16 case statements */
			b[s00  ] = (b[s00]>>3) & 1;
		}
	}
}

/*  ############################################################################  */
/*
 * Copy 4-bit values from a[(nx+1)/2,(ny+1)/2] to b[nx,ny], expanding
 * each value to 2x2 pixels and inserting into bitplane BIT of B.
 * A,B may NOT be same array (it wouldn't make sense to be inserting
 * bits into the same array anyway.)
 */
static void
qtree_bitins(unsigned char a[], int nx, int ny, int b[], int n, int bit)
/*
   int n;		declared y dimension of b
*/
{
int i, j, k;
int s00;
int plane_val;

	plane_val = 1 << bit;
	
	/*
	 * expand each 2x2 block
	 */
	k = 0;						/* k   is index of a[i/2,j/2]	*/
	for (i = 0; i<nx-1; i += 2) {
		s00 = n*i;				/* s00 is index of b[i,j]	*/

  /* Note:
     this code appears to run very slightly faster on a 32-bit linux
     machine using s00+n rather than the s10 intermediate variable
  */
  /*		s10 = s00+n;	*/			/* s10 is index of b[i+1,j]	*/
		for (j = 0; j<ny-1; j += 2) {

		    switch (a[k]) {
		    case(0):
			break;
		    case(1):
			b[s00+n+1] |= plane_val;
			break;
		    case(2):
			b[s00+n  ] |= plane_val;
			break;
		    case(3):
			b[s00+n+1] |= plane_val;
			b[s00+n  ] |= plane_val;
			break;
		    case(4):
			b[s00+1] |= plane_val;
			break;
		    case(5):
			b[s00+n+1] |= plane_val;
			b[s00+1] |= plane_val;
			break;
		    case(6):
			b[s00+n  ] |= plane_val;
			b[s00+1] |= plane_val;
			break;
		    case(7):
			b[s00+n+1] |= plane_val;
			b[s00+n  ] |= plane_val;
			b[s00+1] |= plane_val;
			break;
		    case(8):
			b[s00  ] |= plane_val;
			break;
		    case(9):
			b[s00+n+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(10):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(11):
			b[s00+n+1] |= plane_val;
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(12):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(13):
			b[s00+n+1] |= plane_val;
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(14):
			b[s00+n  ] |= plane_val;
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(15):
			b[s00+n+1] |= plane_val;
			b[s00+n  ] |= plane_val;
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    }

/*
			b[s10+1] |= ( a[k]     & 1) << bit;
			b[s10  ] |= ((a[k]>>1) & 1) << bit;
			b[s00+1] |= ((a[k]>>2) & 1) << bit;
			b[s00  ] |= ((a[k]>>3) & 1) << bit;
*/
			s00 += 2;
/*			s10 += 2; */
			k += 1;
		}
		if (j < ny) {
			/*
			 * row size is odd, do last element in row
			 * s00+1, s10+1 are off edge
			 */

		    switch (a[k]) {
		    case(0):
			break;
		    case(1):
			break;
		    case(2):
			b[s00+n  ] |= plane_val;
			break;
		    case(3):
			b[s00+n  ] |= plane_val;
			break;
		    case(4):
			break;
		    case(5):
			break;
		    case(6):
			b[s00+n  ] |= plane_val;
			break;
		    case(7):
			b[s00+n  ] |= plane_val;
			break;
		    case(8):
			b[s00  ] |= plane_val;
			break;
		    case(9):
			b[s00  ] |= plane_val;
			break;
		    case(10):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(11):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(12):
			b[s00  ] |= plane_val;
			break;
		    case(13):
			b[s00  ] |= plane_val;
			break;
		    case(14):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(15):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    }

/*
			b[s10  ] |= ((a[k]>>1) & 1) << bit;
			b[s00  ] |= ((a[k]>>3) & 1) << bit;
*/
			k += 1;
		}
	}
	if (i < nx) {
		/*
		 * column size is odd, do last row
		 * s10, s10+1 are off edge
		 */
		s00 = n*i;
		for (j = 0; j<ny-1; j += 2) {

		    switch (a[k]) {
		    case(0):
			break;
		    case(1):
			break;
		    case(2):
			break;
		    case(3):
			break;
		    case(4):
			b[s00+1] |= plane_val;
			break;
		    case(5):
			b[s00+1] |= plane_val;
			break;
		    case(6):
			b[s00+1] |= plane_val;
			break;
		    case(7):
			b[s00+1] |= plane_val;
			break;
		    case(8):
			b[s00  ] |= plane_val;
			break;
		    case(9):
			b[s00  ] |= plane_val;
			break;
		    case(10):
			b[s00  ] |= plane_val;
			break;
		    case(11):
			b[s00  ] |= plane_val;
			break;
		    case(12):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(13):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(14):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(15):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    }

/*
			b[s00+1] |= ((a[k]>>2) & 1) << bit;
			b[s00  ] |= ((a[k]>>3) & 1) << bit;
*/

			s00 += 2;
			k += 1;
		}
		if (j < ny) {
			/*
			 * both row and column size are odd, do corner element
			 * s00+1, s10, s10+1 are off edge
			 */

		    switch (a[k]) {
		    case(0):
			break;
		    case(1):
			break;
		    case(2):
			break;
		    case(3):
			break;
		    case(4):
			break;
		    case(5):
			break;
		    case(6):
			break;
		    case(7):
			break;
		    case(8):
			b[s00  ] |= plane_val;
			break;
		    case(9):
			b[s00  ] |= plane_val;
			break;
		    case(10):
			b[s00  ] |= plane_val;
			break;
		    case(11):
			b[s00  ] |= plane_val;
			break;
		    case(12):
			b[s00  ] |= plane_val;
			break;
		    case(13):
			b[s00  ] |= plane_val;
			break;
		    case(14):
			b[s00  ] |= plane_val;
			break;
		    case(15):
			b[s00  ] |= plane_val;
			break;
		    }

/*
			b[s00  ] |= ((a[k]>>3) & 1) << bit;
*/
			k += 1;
		}
	}
}
/*  ############################################################################  */
/*
 * Copy 4-bit values from a[(nx+1)/2,(ny+1)/2] to b[nx,ny], expanding
 * each value to 2x2 pixels and inserting into bitplane BIT of B.
 * A,B may NOT be same array (it wouldn't make sense to be inserting
 * bits into the same array anyway.)
 */
static void
qtree_bitins64(unsigned char a[], int nx, int ny, LONGLONG b[], int n, int bit)
/*
   int n;		declared y dimension of b
*/
{
int i, j, k;
int s00;
LONGLONG plane_val;

	plane_val = ((LONGLONG) 1) << bit;

	/*
	 * expand each 2x2 block
	 */
	k = 0;							/* k   is index of a[i/2,j/2]	*/
	for (i = 0; i<nx-1; i += 2) {
		s00 = n*i;					/* s00 is index of b[i,j]		*/

  /* Note:
     this code appears to run very slightly faster on a 32-bit linux
     machine using s00+n rather than the s10 intermediate variable
  */
  /*		s10 = s00+n;	*/			/* s10 is index of b[i+1,j]	*/
		for (j = 0; j<ny-1; j += 2) {

		    switch (a[k]) {
		    case(0):
			break;
		    case(1):
			b[s00+n+1] |= plane_val;
			break;
		    case(2):
			b[s00+n  ] |= plane_val;
			break;
		    case(3):
			b[s00+n+1] |= plane_val;
			b[s00+n  ] |= plane_val;
			break;
		    case(4):
			b[s00+1] |= plane_val;
			break;
		    case(5):
			b[s00+n+1] |= plane_val;
			b[s00+1] |= plane_val;
			break;
		    case(6):
			b[s00+n  ] |= plane_val;
			b[s00+1] |= plane_val;
			break;
		    case(7):
			b[s00+n+1] |= plane_val;
			b[s00+n  ] |= plane_val;
			b[s00+1] |= plane_val;
			break;
		    case(8):
			b[s00  ] |= plane_val;
			break;
		    case(9):
			b[s00+n+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(10):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(11):
			b[s00+n+1] |= plane_val;
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(12):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(13):
			b[s00+n+1] |= plane_val;
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(14):
			b[s00+n  ] |= plane_val;
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(15):
			b[s00+n+1] |= plane_val;
			b[s00+n  ] |= plane_val;
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    }

/*
			b[s10+1] |= ((LONGLONG) ( a[k]     & 1)) << bit;
			b[s10  ] |= ((((LONGLONG)a[k])>>1) & 1) << bit;
			b[s00+1] |= ((((LONGLONG)a[k])>>2) & 1) << bit;
			b[s00  ] |= ((((LONGLONG)a[k])>>3) & 1) << bit;
*/
			s00 += 2;
/*			s10 += 2;  */
			k += 1;
		}
		if (j < ny) {
			/*
			 * row size is odd, do last element in row
			 * s00+1, s10+1 are off edge
			 */

		    switch (a[k]) {
		    case(0):
			break;
		    case(1):
			break;
		    case(2):
			b[s00+n  ] |= plane_val;
			break;
		    case(3):
			b[s00+n  ] |= plane_val;
			break;
		    case(4):
			break;
		    case(5):
			break;
		    case(6):
			b[s00+n  ] |= plane_val;
			break;
		    case(7):
			b[s00+n  ] |= plane_val;
			break;
		    case(8):
			b[s00  ] |= plane_val;
			break;
		    case(9):
			b[s00  ] |= plane_val;
			break;
		    case(10):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(11):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(12):
			b[s00  ] |= plane_val;
			break;
		    case(13):
			b[s00  ] |= plane_val;
			break;
		    case(14):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(15):
			b[s00+n  ] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    }
/*
			b[s10  ] |= ((((LONGLONG)a[k])>>1) & 1) << bit;
			b[s00  ] |= ((((LONGLONG)a[k])>>3) & 1) << bit;
*/
			k += 1;
		}
	}
	if (i < nx) {
		/*
		 * column size is odd, do last row
		 * s10, s10+1 are off edge
		 */
		s00 = n*i;
		for (j = 0; j<ny-1; j += 2) {

		    switch (a[k]) {
		    case(0):
			break;
		    case(1):
			break;
		    case(2):
			break;
		    case(3):
			break;
		    case(4):
			b[s00+1] |= plane_val;
			break;
		    case(5):
			b[s00+1] |= plane_val;
			break;
		    case(6):
			b[s00+1] |= plane_val;
			break;
		    case(7):
			b[s00+1] |= plane_val;
			break;
		    case(8):
			b[s00  ] |= plane_val;
			break;
		    case(9):
			b[s00  ] |= plane_val;
			break;
		    case(10):
			b[s00  ] |= plane_val;
			break;
		    case(11):
			b[s00  ] |= plane_val;
			break;
		    case(12):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(13):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(14):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    case(15):
			b[s00+1] |= plane_val;
			b[s00  ] |= plane_val;
			break;
		    }

/*
			b[s00+1] |= ((((LONGLONG)a[k])>>2) & 1) << bit;
			b[s00  ] |= ((((LONGLONG)a[k])>>3) & 1) << bit;
*/
			s00 += 2;
			k += 1;
		}
		if (j < ny) {
			/*
			 * both row and column size are odd, do corner element
			 * s00+1, s10, s10+1 are off edge
			 */

		    switch (a[k]) {
		    case(0):
			break;
		    case(1):
			break;
		    case(2):
			break;
		    case(3):
			break;
		    case(4):
			break;
		    case(5):
			break;
		    case(6):
			break;
		    case(7):
			break;
		    case(8):
			b[s00  ] |= plane_val;
			break;
		    case(9):
			b[s00  ] |= plane_val;
			break;
		    case(10):
			b[s00  ] |= plane_val;
			break;
		    case(11):
			b[s00  ] |= plane_val;
			break;
		    case(12):
			b[s00  ] |= plane_val;
			break;
		    case(13):
			b[s00  ] |= plane_val;
			break;
		    case(14):
			b[s00  ] |= plane_val;
			break;
		    case(15):
			b[s00  ] |= plane_val;
			break;
		    }
/*
			b[s00  ] |= ((((LONGLONG)a[k])>>3) & 1) << bit;
*/
			k += 1;
		}
	}
}

/*  ############################################################################  */
static void
read_bdirect(unsigned char *infile, int a[], int n, int nqx, int nqy, unsigned char scratch[], int bit)
{
	/*
	 * read bit image packed 4 pixels/nybble
	 */
/*
int i;
	for (i = 0; i < ((nqx+1)/2) * ((nqy+1)/2); i++) {
		scratch[i] = input_nybble(infile);
	}
*/
        input_nnybble(infile, ((nqx+1)/2) * ((nqy+1)/2), scratch);
	
	/*
	 * insert in bitplane BIT of image A
	 */
	qtree_bitins(scratch,nqx,nqy,a,n,bit);
}
/*  ############################################################################  */
static void
read_bdirect64(unsigned char *infile, LONGLONG a[], int n, int nqx, int nqy, unsigned char scratch[], int bit)
{
	/*
	 * read bit image packed 4 pixels/nybble
	 */
/*
int i;
	for (i = 0; i < ((nqx+1)/2) * ((nqy+1)/2); i++) {
		scratch[i] = input_nybble(infile);
	}
*/
        input_nnybble(infile, ((nqx+1)/2) * ((nqy+1)/2), scratch);

	/*
	 * insert in bitplane BIT of image A
	 */
	qtree_bitins64(scratch,nqx,nqy,a,n,bit);
}

/*  ############################################################################  */
/*
 * Huffman decoding for fixed codes
 *
 * Coded values range from 0-15
 *
 * Huffman code values (hex):
 *
 *	3e, 00, 01, 08, 02, 09, 1a, 1b,
 *	03, 1c, 0a, 1d, 0b, 1e, 3f, 0c
 *
 * and number of bits in each code:
 *
 *	6,  3,  3,  4,  3,  4,  5,  5,
 *	3,  5,  4,  5,  4,  5,  6,  4
 */
static int input_huffman(unsigned char *infile)
{
int c;

	/*
	 * get first 3 bits to start
	 */
	c = input_nbits(infile,3);
	if (c < 4) {
		/*
		 * this is all we need
		 * return 1,2,4,8 for c=0,1,2,3
		 */
		return(1<<c);
	}
	/*
	 * get the next bit
	 */
	c = input_bit(infile) | (c<<1);
	if (c < 13) {
		/*
		 * OK, 4 bits is enough
		 */
		switch (c) {
			case  8 : return(3);
			case  9 : return(5);
			case 10 : return(10);
			case 11 : return(12);
			case 12 : return(15);
		}
	}
	/*
	 * get yet another bit
	 */
	c = input_bit(infile) | (c<<1);
	if (c < 31) {
		/*
		 * OK, 5 bits is enough
		 */
		switch (c) {
			case 26 : return(6);
			case 27 : return(7);
			case 28 : return(9);
			case 29 : return(11);
			case 30 : return(13);
		}
	}
	/*
	 * need the 6th bit
	 */
	c = input_bit(infile) | (c<<1);
	if (c == 62) {
		return(0);
	} else {
		return(14);
	}
}

/*  ############################################################################  */
/*  ############################################################################  */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* qread.c	Read binary data
 *
 * Programmer: R. White		Date: 11 March 1991
 */

static int readint(unsigned char *infile)
{
int a,i;
unsigned char b[4];

	/* Read integer A one byte at a time from infile.
	 *
	 * This is portable from Vax to Sun since it eliminates the
	 * need for byte-swapping.
	 *
         *  This routine is only called to read the first 3 values
	 *  in the compressed file, so it doesn't have to be 
	 *  super-efficient
	 */
	for (i=0; i<4; i++) qread(infile,(char *) &b[i],1);
	a = b[0];
	for (i=1; i<4; i++) a = (a<<8) + b[i];
	return(a);
}

/*  ############################################################################  */
static LONGLONG readlonglong(unsigned char *infile)
{
int i;
LONGLONG a;
unsigned char b[8];

	/* Read integer A one byte at a time from infile.
	 *
	 * This is portable from Vax to Sun since it eliminates the
	 * need for byte-swapping.
	 *
         *  This routine is only called to read the first 3 values
	 *  in the compressed file, so it doesn't have to be 
	 *  super-efficient
	 */
	for (i=0; i<8; i++) qread(infile,(char *) &b[i],1);
	a = b[0];
	for (i=1; i<8; i++) a = (a<<8) + b[i];
	return(a);
}

/*  ############################################################################  */
static void qread(unsigned char *file, char buffer[], int n)
{
    /*
     * read n bytes from file into buffer
     *
     */

    memcpy(buffer, &file[nextchar], n);
    nextchar += n;
}

/*  ############################################################################  */
/*  ############################################################################  */
/* Copyright (c) 1993 Association of Universities for Research
 * in Astronomy. All rights reserved. Produced under National
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */

/* BIT INPUT ROUTINES */

/* THE BIT BUFFER */

static int buffer2;			/* Bits waiting to be input	*/
static int bits_to_go;			/* Number of bits still in buffer */

/* INITIALIZE BIT INPUT */

/*  ############################################################################  */
static void start_inputing_bits(void)
{
	/*
	 * Buffer starts out with no bits in it
	 */
	bits_to_go = 0;
}

/*  ############################################################################  */
/* INPUT A BIT */

static int input_bit(unsigned char *infile)
{
	if (bits_to_go == 0) {			/* Read the next byte if no	*/

		buffer2 = infile[nextchar];
		nextchar++;
		
		bits_to_go = 8;
	}
	/*
	 * Return the next bit
	 */
	bits_to_go -= 1;
	return((buffer2>>bits_to_go) & 1);
}

/*  ############################################################################  */
/* INPUT N BITS (N must be <= 8) */

static int input_nbits(unsigned char *infile, int n)
{
    /* AND mask for retreiving the right-most n bits */
    static int mask[9] = {0, 1, 3, 7, 15, 31, 63, 127, 255};

	if (bits_to_go < n) {
		/*
		 * need another byte's worth of bits
		 */

		buffer2 = (buffer2<<8) | (int) infile[nextchar];
		nextchar++;
		bits_to_go += 8;
	}
	/*
	 * now pick off the first n bits
	 */
	bits_to_go -= n;

        /* there was a slight gain in speed by replacing the following line */
/*	return( (buffer2>>bits_to_go) & ((1<<n)-1) ); */
	return( (buffer2>>bits_to_go) & (*(mask+n)) ); 
}
/*  ############################################################################  */
/* INPUT 4 BITS  */

static int input_nybble(unsigned char *infile)
{
	if (bits_to_go < 4) {
		/*
		 * need another byte's worth of bits
		 */

		buffer2 = (buffer2<<8) | (int) infile[nextchar];
		nextchar++;
		bits_to_go += 8;
	}
	/*
	 * now pick off the first 4 bits
	 */
	bits_to_go -= 4;

	return( (buffer2>>bits_to_go) & 15 ); 
}
/*  ############################################################################  */
/* INPUT array of 4 BITS  */

static int input_nnybble(unsigned char *infile, int n, unsigned char array[])
{
	/* copy n 4-bit nybbles from infile to the lower 4 bits of array */

int ii, kk, shift1, shift2;

/*  forcing byte alignment doesn;t help, and even makes it go slightly slower
if (bits_to_go != 8) input_nbits(infile, bits_to_go);
*/
	if (n == 1) {
		array[0] = input_nybble(infile);
		return(0);
	}
	
	if (bits_to_go == 8) {
		/*
		   already have 2 full nybbles in buffer2, so 
		   backspace the infile array to reuse last char
		*/
		nextchar--;
		bits_to_go = 0;
	}
	
	/* bits_to_go now has a value in the range 0 - 7.  After adding  */
	/* another byte, bits_to_go effectively will be in range 8 - 15 */	

	shift1 = bits_to_go + 4;   /* shift1 will be in range 4 - 11 */
	shift2 = bits_to_go;	   /* shift2 will be in range 0 -  7 */
	kk = 0;

	/* special case */
	if (bits_to_go == 0) 
	{
	    for (ii = 0; ii < n/2; ii++) {
		/*
		 * refill the buffer with next byte
		 */
		buffer2 = (buffer2<<8) | (int) infile[nextchar];
		nextchar++;
		array[kk]     = (int) ((buffer2>>4) & 15);
		array[kk + 1] = (int) ((buffer2) & 15);    /* no shift required */
		kk += 2;
	    }
	}
	else
	{
	    for (ii = 0; ii < n/2; ii++) {
		/*
		 * refill the buffer with next byte
		 */
		buffer2 = (buffer2<<8) | (int) infile[nextchar];
		nextchar++;
		array[kk]     = (int) ((buffer2>>shift1) & 15);
		array[kk + 1] = (int) ((buffer2>>shift2) & 15);
		kk += 2;
	    }
	}


	if (ii * 2 != n) {  /* have to read last odd byte */
		array[n-1] = input_nybble(infile);
	}

	return( (buffer2>>bits_to_go) & 15 ); 
}
