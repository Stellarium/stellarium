/*  #########################################################################
These routines to apply the H-compress compression algorithm to a 2-D Fits
image were written by R. White at the STScI and were obtained from the STScI at
http://www.stsci.edu/software/hcompress.html

This source file is a concatination of the following sources files in the
original distribution 
 htrans.c 
 digitize.c 
 encode.c 
 qwrite.c 
 doencode.c 
 bit_output.c 
 qtree_encode.c

The following modifications have been made to the original code:

  - commented out redundant "include" statements
  - added the noutchar global variable 
  - changed all the 'extern' declarations to 'static', since all the routines are in
    the same source file
  - changed the first parameter in encode (and in lower level routines from a file stream
    to a char array
  - modifid the encode routine to return the size of the compressed array of bytes
  - changed calls to printf and perror to call the CFITSIO ffpmsg routine
  - modified the mywrite routine, and lower level byte writing routines,  to copy 
    the output bytes to a char array, instead of writing them to a file stream
  - replace "exit" statements with "return" statements
  - changed the function declarations to the more modern ANSI C style

 ############################################################################  */
 
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "fitsio2.h"

static long noutchar;
static long noutmax;

static int htrans(int a[],int nx,int ny);
static void digitize(int a[], int nx, int ny, int scale);
static int encode(char *outfile, long *nlen, int a[], int nx, int ny, int scale);
static void shuffle(int a[], int n, int n2, int tmp[]);

static int htrans64(LONGLONG a[],int nx,int ny);
static void digitize64(LONGLONG a[], int nx, int ny, int scale);
static int encode64(char *outfile, long *nlen, LONGLONG a[], int nx, int ny, int scale);
static void shuffle64(LONGLONG a[], int n, int n2, LONGLONG tmp[]);

static  void writeint(char *outfile, int a);
static  void writelonglong(char *outfile, LONGLONG a);
static  int doencode(char *outfile, int a[], int nx, int ny, unsigned char nbitplanes[3]);
static  int doencode64(char *outfile, LONGLONG a[], int nx, int ny, unsigned char nbitplanes[3]);
static int  qwrite(char *file, char buffer[], int n);

static int qtree_encode(char *outfile, int a[], int n, int nqx, int nqy, int nbitplanes);
static int qtree_encode64(char *outfile, LONGLONG a[], int n, int nqx, int nqy, int nbitplanes);
static void start_outputing_bits(void);
static void done_outputing_bits(char *outfile);
static void output_nbits(char *outfile, int bits, int n);

static void qtree_onebit(int a[], int n, int nx, int ny, unsigned char b[], int bit);
static void qtree_onebit64(LONGLONG a[], int n, int nx, int ny, unsigned char b[], int bit);
static void qtree_reduce(unsigned char a[], int n, int nx, int ny, unsigned char b[]);
static int  bufcopy(unsigned char a[], int n, unsigned char buffer[], int *b, int bmax);
static void write_bdirect(char *outfile, int a[], int n,int nqx, int nqy, unsigned char scratch[], int bit);
static void write_bdirect64(char *outfile, LONGLONG a[], int n,int nqx, int nqy, unsigned char scratch[], int bit);

/* #define output_nybble(outfile,c)	output_nbits(outfile,c,4) */
static void output_nybble(char *outfile, int bits);
static void output_nnybble(char *outfile, int n, unsigned char array[]);

#define output_huffman(outfile,c)	output_nbits(outfile,code[c],ncode[c])

/* ---------------------------------------------------------------------- */
int fits_hcompress(int *a, int ny, int nx, int scale, char *output, 
                  long *nbytes, int *status)
{
  /* 
     compress the input image using the H-compress algorithm
  
   a  - input image array
   nx - size of X axis of image
   ny - size of Y axis of image
   scale - quantization scale factor. Larger values results in more (lossy) compression
           scale = 0 does lossless compression
   output - pre-allocated array to hold the output compressed stream of bytes
   nbyts  - input value = size of the output buffer;
            returned value = size of the compressed byte stream, in bytes

 NOTE: the nx and ny dimensions as defined within this code are reversed from
 the usual FITS notation.  ny is the fastest varying dimension, which is
 usually considered the X axis in the FITS image display

  */

  int stat;
  
  if (*status > 0) return(*status);

  /* H-transform */
  stat = htrans(a, nx, ny);
  if (stat) {
     *status = stat;
     return(*status);
  }

  /* digitize */
  digitize(a, nx, ny, scale);

  /* encode and write to output array */

  FFLOCK;
  noutmax = *nbytes;  /* input value is the allocated size of the array */
  *nbytes = 0;  /* reset */

  stat = encode(output, nbytes, a, nx, ny, scale);
  FFUNLOCK;
  
  *status = stat;
  return(*status);
}
/* ---------------------------------------------------------------------- */
int fits_hcompress64(LONGLONG *a, int ny, int nx, int scale, char *output, 
                  long *nbytes, int *status)
{
  /* 
     compress the input image using the H-compress algorithm
  
   a  - input image array
   nx - size of X axis of image
   ny - size of Y axis of image
   scale - quantization scale factor. Larger values results in more (lossy) compression
           scale = 0 does lossless compression
   output - pre-allocated array to hold the output compressed stream of bytes
   nbyts  - size of the compressed byte stream, in bytes

 NOTE: the nx and ny dimensions as defined within this code are reversed from
 the usual FITS notation.  ny is the fastest varying dimension, which is
 usually considered the X axis in the FITS image display

  */

  int stat;
  
  if (*status > 0) return(*status);

  /* H-transform */
  stat = htrans64(a, nx, ny);
  if (stat) {
     *status = stat;
     return(*status);
  }

  /* digitize */
  digitize64(a, nx, ny, scale);

  /* encode and write to output array */

  FFLOCK;
  noutmax = *nbytes;  /* input value is the allocated size of the array */
  *nbytes = 0;  /* reset */

  stat = encode64(output, nbytes, a, nx, ny, scale);
  FFUNLOCK;

  *status = stat;
  return(*status);
}

 
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* htrans.c   H-transform of NX x NY integer image
 *
 * Programmer: R. White		Date: 11 May 1992
 */

/* ######################################################################### */
static int htrans(int a[],int nx,int ny)
{
int nmax, log2n, h0, hx, hy, hc, nxtop, nytop, i, j, k;
int oddx, oddy;
int shift, mask, mask2, prnd, prnd2, nrnd2;
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
	if(tmp == (int *) NULL) {
	        ffpmsg("htrans: insufficient memory");
		return(DATA_COMPRESSION_ERR);
	}
	/*
	 * set up rounding and shifting masks
	 */
	shift = 0;
	mask  = -2;
	mask2 = mask << 1;
	prnd  = 1;
	prnd2 = prnd << 1;
	nrnd2 = prnd2 - 1;
	/*
	 * do log2n reductions
	 *
	 * We're indexing a as a 2-D array with dimensions (nx,ny).
	 */
	nxtop = nx;
	nytop = ny;

	for (k = 0; k<log2n; k++) {
		oddx = nxtop % 2;
		oddy = nytop % 2;
		for (i = 0; i<nxtop-oddx; i += 2) {
			s00 = i*ny;				/* s00 is index of a[i,j]	*/
			s10 = s00+ny;			/* s10 is index of a[i+1,j]	*/
			for (j = 0; j<nytop-oddy; j += 2) {
				/*
				 * Divide h0,hx,hy,hc by 2 (1 the first time through).
				 */
				h0 = (a[s10+1] + a[s10] + a[s00+1] + a[s00]) >> shift;
				hx = (a[s10+1] + a[s10] - a[s00+1] - a[s00]) >> shift;
				hy = (a[s10+1] - a[s10] + a[s00+1] - a[s00]) >> shift;
				hc = (a[s10+1] - a[s10] - a[s00+1] + a[s00]) >> shift;

				/*
				 * Throw away the 2 bottom bits of h0, bottom bit of hx,hy.
				 * To get rounding to be same for positive and negative
				 * numbers, nrnd2 = prnd2 - 1.
				 */
				a[s10+1] = hc;
				a[s10  ] = ( (hx>=0) ? (hx+prnd)  :  hx        ) & mask ;
				a[s00+1] = ( (hy>=0) ? (hy+prnd)  :  hy        ) & mask ;
				a[s00  ] = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
				s00 += 2;
				s10 += 2;
			}
			if (oddy) {
				/*
				 * do last element in row if row length is odd
				 * s00+1, s10+1 are off edge
				 */
				h0 = (a[s10] + a[s00]) << (1-shift);
				hx = (a[s10] - a[s00]) << (1-shift);
				a[s10  ] = ( (hx>=0) ? (hx+prnd)  :  hx        ) & mask ;
				a[s00  ] = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
				s00 += 1;
				s10 += 1;
			}
		}
		if (oddx) {
			/*
			 * do last row if column length is odd
			 * s10, s10+1 are off edge
			 */
			s00 = i*ny;
			for (j = 0; j<nytop-oddy; j += 2) {
				h0 = (a[s00+1] + a[s00]) << (1-shift);
				hy = (a[s00+1] - a[s00]) << (1-shift);
				a[s00+1] = ( (hy>=0) ? (hy+prnd)  :  hy        ) & mask ;
				a[s00  ] = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
				s00 += 2;
			}
			if (oddy) {
				/*
				 * do corner element if both row and column lengths are odd
				 * s00+1, s10, s10+1 are off edge
				 */
				h0 = a[s00] << (2-shift);
				a[s00  ] = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
			}
		}
		/*
		 * now shuffle in each dimension to group coefficients by order
		 */
		for (i = 0; i<nxtop; i++) {
			shuffle(&a[ny*i],nytop,1,tmp);
		}
		for (j = 0; j<nytop; j++) {
			shuffle(&a[j],nxtop,ny,tmp);
		}
		/*
		 * image size reduced by 2 (round up if odd)
		 */
		nxtop = (nxtop+1)>>1;
		nytop = (nytop+1)>>1;
		/*
		 * divisor doubles after first reduction
		 */
		shift = 1;
		/*
		 * masks, rounding values double after each iteration
		 */
		mask  = mask2;
		prnd  = prnd2;
		mask2 = mask2 << 1;
		prnd2 = prnd2 << 1;
		nrnd2 = prnd2 - 1;
	}
	free(tmp);
	return(0);
}
/* ######################################################################### */

static int htrans64(LONGLONG a[],int nx,int ny)
{
int nmax, log2n, nxtop, nytop, i, j, k;
int oddx, oddy;
int shift;
int s10, s00;
LONGLONG h0, hx, hy, hc, prnd, prnd2, nrnd2, mask, mask2;
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
	if(tmp == (LONGLONG *) NULL) {
	        ffpmsg("htrans64: insufficient memory");
		return(DATA_COMPRESSION_ERR);
	}
	/*
	 * set up rounding and shifting masks
	 */
	shift = 0;
	mask  = (LONGLONG) -2;
	mask2 = mask << 1;
	prnd  = (LONGLONG) 1;
	prnd2 = prnd << 1;
	nrnd2 = prnd2 - 1;
	/*
	 * do log2n reductions
	 *
	 * We're indexing a as a 2-D array with dimensions (nx,ny).
	 */
	nxtop = nx;
	nytop = ny;

	for (k = 0; k<log2n; k++) {
		oddx = nxtop % 2;
		oddy = nytop % 2;
		for (i = 0; i<nxtop-oddx; i += 2) {
			s00 = i*ny;				/* s00 is index of a[i,j]	*/
			s10 = s00+ny;			/* s10 is index of a[i+1,j]	*/
			for (j = 0; j<nytop-oddy; j += 2) {
				/*
				 * Divide h0,hx,hy,hc by 2 (1 the first time through).
				 */
				h0 = (a[s10+1] + a[s10] + a[s00+1] + a[s00]) >> shift;
				hx = (a[s10+1] + a[s10] - a[s00+1] - a[s00]) >> shift;
				hy = (a[s10+1] - a[s10] + a[s00+1] - a[s00]) >> shift;
				hc = (a[s10+1] - a[s10] - a[s00+1] + a[s00]) >> shift;

				/*
				 * Throw away the 2 bottom bits of h0, bottom bit of hx,hy.
				 * To get rounding to be same for positive and negative
				 * numbers, nrnd2 = prnd2 - 1.
				 */
				a[s10+1] = hc;
				a[s10  ] = ( (hx>=0) ? (hx+prnd)  :  hx        ) & mask ;
				a[s00+1] = ( (hy>=0) ? (hy+prnd)  :  hy        ) & mask ;
				a[s00  ] = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
				s00 += 2;
				s10 += 2;
			}
			if (oddy) {
				/*
				 * do last element in row if row length is odd
				 * s00+1, s10+1 are off edge
				 */
				h0 = (a[s10] + a[s00]) << (1-shift);
				hx = (a[s10] - a[s00]) << (1-shift);
				a[s10  ] = ( (hx>=0) ? (hx+prnd)  :  hx        ) & mask ;
				a[s00  ] = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
				s00 += 1;
				s10 += 1;
			}
		}
		if (oddx) {
			/*
			 * do last row if column length is odd
			 * s10, s10+1 are off edge
			 */
			s00 = i*ny;
			for (j = 0; j<nytop-oddy; j += 2) {
				h0 = (a[s00+1] + a[s00]) << (1-shift);
				hy = (a[s00+1] - a[s00]) << (1-shift);
				a[s00+1] = ( (hy>=0) ? (hy+prnd)  :  hy        ) & mask ;
				a[s00  ] = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
				s00 += 2;
			}
			if (oddy) {
				/*
				 * do corner element if both row and column lengths are odd
				 * s00+1, s10, s10+1 are off edge
				 */
				h0 = a[s00] << (2-shift);
				a[s00  ] = ( (h0>=0) ? (h0+prnd2) : (h0+nrnd2) ) & mask2;
			}
		}
		/*
		 * now shuffle in each dimension to group coefficients by order
		 */
		for (i = 0; i<nxtop; i++) {
			shuffle64(&a[ny*i],nytop,1,tmp);
		}
		for (j = 0; j<nytop; j++) {
			shuffle64(&a[j],nxtop,ny,tmp);
		}
		/*
		 * image size reduced by 2 (round up if odd)
		 */
		nxtop = (nxtop+1)>>1;
		nytop = (nytop+1)>>1;
		/*
		 * divisor doubles after first reduction
		 */
		shift = 1;
		/*
		 * masks, rounding values double after each iteration
		 */
		mask  = mask2;
		prnd  = prnd2;
		mask2 = mask2 << 1;
		prnd2 = prnd2 << 1;
		nrnd2 = prnd2 - 1;
	}
	free(tmp);
	return(0);
}

/* ######################################################################### */
static void
shuffle(int a[], int n, int n2, int tmp[])
{

/* 
int a[];	 array to shuffle					
int n;		 number of elements to shuffle	
int n2;		 second dimension					
int tmp[];	 scratch storage					
*/

int i;
int *p1, *p2, *pt;

	/*
	 * copy odd elements to tmp
	 */
	pt = tmp;
	p1 = &a[n2];
	for (i=1; i < n; i += 2) {
		*pt = *p1;
		pt += 1;
		p1 += (n2+n2);
	}
	/*
	 * compress even elements into first half of A
	 */
	p1 = &a[n2];
	p2 = &a[n2+n2];
	for (i=2; i<n; i += 2) {
		*p1 = *p2;
		p1 += n2;
		p2 += (n2+n2);
	}
	/*
	 * put odd elements into 2nd half
	 */
	pt = tmp;
	for (i = 1; i<n; i += 2) {
		*p1 = *pt;
		p1 += n2;
		pt += 1;
	}
}
/* ######################################################################### */
static void
shuffle64(LONGLONG a[], int n, int n2, LONGLONG tmp[])
{

/* 
LONGLONG a[];	 array to shuffle					
int n;		 number of elements to shuffle	
int n2;		 second dimension					
LONGLONG tmp[];	 scratch storage					
*/

int i;
LONGLONG *p1, *p2, *pt;

	/*
	 * copy odd elements to tmp
	 */
	pt = tmp;
	p1 = &a[n2];
	for (i=1; i < n; i += 2) {
		*pt = *p1;
		pt += 1;
		p1 += (n2+n2);
	}
	/*
	 * compress even elements into first half of A
	 */
	p1 = &a[n2];
	p2 = &a[n2+n2];
	for (i=2; i<n; i += 2) {
		*p1 = *p2;
		p1 += n2;
		p2 += (n2+n2);
	}
	/*
	 * put odd elements into 2nd half
	 */
	pt = tmp;
	for (i = 1; i<n; i += 2) {
		*p1 = *pt;
		p1 += n2;
		pt += 1;
	}
}
/* ######################################################################### */
/* ######################################################################### */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* digitize.c	digitize H-transform
 *
 * Programmer: R. White		Date: 11 March 1991
 */

/* ######################################################################### */
static void  
digitize(int a[], int nx, int ny, int scale)
{
int d, *p;

	/*
	 * round to multiple of scale
	 */
	if (scale <= 1) return;
	d=(scale+1)/2-1;
	for (p=a; p <= &a[nx*ny-1]; p++) *p = ((*p>0) ? (*p+d) : (*p-d))/scale;
}

/* ######################################################################### */
static void  
digitize64(LONGLONG a[], int nx, int ny, int scale)
{
LONGLONG d, *p, scale64;

	/*
	 * round to multiple of scale
	 */
	if (scale <= 1) return;
	d=(scale+1)/2-1;
	scale64 = scale;  /* use a 64-bit int for efficiency in the big loop */

	for (p=a; p <= &a[nx*ny-1]; p++) *p = ((*p>0) ? (*p+d) : (*p-d))/scale64;
}
/* ######################################################################### */
/* ######################################################################### */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* encode.c		encode H-transform and write to outfile
 *
 * Programmer: R. White		Date: 2 February 1994
 */

static char code_magic[2] = { (char)0xDD, (char)0x99 };


/* ######################################################################### */
static int encode(char *outfile, long *nlength, int a[], int nx, int ny, int scale)
{

/* FILE *outfile;  - change outfile to a char array */  
/*
  long * nlength    returned length (in bytes) of the encoded array)
  int a[];								 input H-transform array (nx,ny)
  int nx,ny;								 size of H-transform array	
  int scale;								 scale factor for digitization
*/
int nel, nx2, ny2, i, j, k, q, vmax[3], nsign, bits_to_go;
unsigned char nbitplanes[3];
unsigned char *signbits;
int stat;

        noutchar = 0;  /* initialize the number of compressed bytes that have been written */
	nel = nx*ny;
	/*
	 * write magic value
	 */
	qwrite(outfile, code_magic, sizeof(code_magic));
	writeint(outfile, nx);			/* size of image */
	writeint(outfile, ny);
	writeint(outfile, scale);		/* scale factor for digitization */
	/*
	 * write first value of A (sum of all pixels -- the only value
	 * which does not compress well)
	 */
	writelonglong(outfile, (LONGLONG) a[0]);

	a[0] = 0;
	/*
	 * allocate array for sign bits and save values, 8 per byte
              (initialize to all zeros)
	 */
	signbits = (unsigned char *) calloc(1, (nel+7)/8);
	if (signbits == (unsigned char *) NULL) {
		ffpmsg("encode: insufficient memory");
		return(DATA_COMPRESSION_ERR);
	}
	nsign = 0;
	bits_to_go = 8;
/*	signbits[0] = 0; */
	for (i=0; i<nel; i++) {
		if (a[i] > 0) {
			/*
			 * positive element, put zero at end of buffer
			 */
			signbits[nsign] <<= 1;
			bits_to_go -= 1;
		} else if (a[i] < 0) {
			/*
			 * negative element, shift in a one
			 */
			signbits[nsign] <<= 1;
			signbits[nsign] |= 1;
			bits_to_go -= 1;
			/*
			 * replace a by absolute value
			 */
			a[i] = -a[i];
		}
		if (bits_to_go == 0) {
			/*
			 * filled up this byte, go to the next one
			 */
			bits_to_go = 8;
			nsign += 1;
/*			signbits[nsign] = 0; */
		}
	}
	if (bits_to_go != 8) {
		/*
		 * some bits in last element
		 * move bits in last byte to bottom and increment nsign
		 */
		signbits[nsign] <<= bits_to_go;
		nsign += 1;
	}
	/*
	 * calculate number of bit planes for 3 quadrants
	 *
	 * quadrant 0=bottom left, 1=bottom right or top left, 2=top right, 
	 */
	for (q=0; q<3; q++) {
		vmax[q] = 0;
	}
	/*
	 * get maximum absolute value in each quadrant
	 */
	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;
	j=0;	/* column counter	*/
	k=0;	/* row counter		*/
	for (i=0; i<nel; i++) {
		q = (j>=ny2) + (k>=nx2);
		if (vmax[q] < a[i]) vmax[q] = a[i];
		if (++j >= ny) {
			j = 0;
			k += 1;
		}
	}
	/*
	 * now calculate number of bits for each quadrant
	 */

        /* this is a more efficient way to do this, */
 
 
        for (q = 0; q < 3; q++) {
            for (nbitplanes[q] = 0; vmax[q]>0; vmax[q] = vmax[q]>>1, nbitplanes[q]++) ; 
        }


/*
	for (q = 0; q < 3; q++) {
		nbitplanes[q] = (int) (log((float) (vmax[q]+1))/log(2.0)+0.5);
		if ( (vmax[q]+1) > (1<<nbitplanes[q]) ) {
			nbitplanes[q] += 1;
		}
	}
*/

	/*
	 * write nbitplanes
	 */
	if (0 == qwrite(outfile, (char *) nbitplanes, sizeof(nbitplanes))) {
	        *nlength = noutchar;
		ffpmsg("encode: output buffer too small");
		return(DATA_COMPRESSION_ERR);
        }
	 
	/*
	 * write coded array
	 */
	stat = doencode(outfile, a, nx, ny, nbitplanes);
	/*
	 * write sign bits
	 */

	if (nsign > 0) {

	   if ( 0 == qwrite(outfile, (char *) signbits, nsign)) {
	        free(signbits);
	        *nlength = noutchar;
		ffpmsg("encode: output buffer too small");
		return(DATA_COMPRESSION_ERR);
          }
	} 
	
	free(signbits);
	*nlength = noutchar;

        if (noutchar >= noutmax) {
		ffpmsg("encode: output buffer too small");
		return(DATA_COMPRESSION_ERR);
        }  
	
	return(stat); 
}
/* ######################################################################### */
static int encode64(char *outfile, long *nlength, LONGLONG a[], int nx, int ny, int scale)
{

/* FILE *outfile;  - change outfile to a char array */  
/*
  long * nlength    returned length (in bytes) of the encoded array)
  LONGLONG a[];								 input H-transform array (nx,ny)
  int nx,ny;								 size of H-transform array	
  int scale;								 scale factor for digitization
*/
int nel, nx2, ny2, i, j, k, q, nsign, bits_to_go;
LONGLONG vmax[3];
unsigned char nbitplanes[3];
unsigned char *signbits;
int stat;

        noutchar = 0;  /* initialize the number of compressed bytes that have been written */
	nel = nx*ny;
	/*
	 * write magic value
	 */
	qwrite(outfile, code_magic, sizeof(code_magic));
	writeint(outfile, nx);				/* size of image	*/
	writeint(outfile, ny);
	writeint(outfile, scale);			/* scale factor for digitization */
	/*
	 * write first value of A (sum of all pixels -- the only value
	 * which does not compress well)
	 */
	writelonglong(outfile, a[0]);

	a[0] = 0;
	/*
	 * allocate array for sign bits and save values, 8 per byte
	 */
	signbits = (unsigned char *) calloc(1, (nel+7)/8);
	if (signbits == (unsigned char *) NULL) {
		ffpmsg("encode64: insufficient memory");
		return(DATA_COMPRESSION_ERR);
	}
	nsign = 0;
	bits_to_go = 8;
/*	signbits[0] = 0; */
	for (i=0; i<nel; i++) {
		if (a[i] > 0) {
			/*
			 * positive element, put zero at end of buffer
			 */
			signbits[nsign] <<= 1;
			bits_to_go -= 1;
		} else if (a[i] < 0) {
			/*
			 * negative element, shift in a one
			 */
			signbits[nsign] <<= 1;
			signbits[nsign] |= 1;
			bits_to_go -= 1;
			/*
			 * replace a by absolute value
			 */
			a[i] = -a[i];
		}
		if (bits_to_go == 0) {
			/*
			 * filled up this byte, go to the next one
			 */
			bits_to_go = 8;
			nsign += 1;
/*			signbits[nsign] = 0; */
		}
	}
	if (bits_to_go != 8) {
		/*
		 * some bits in last element
		 * move bits in last byte to bottom and increment nsign
		 */
		signbits[nsign] <<= bits_to_go;
		nsign += 1;
	}
	/*
	 * calculate number of bit planes for 3 quadrants
	 *
	 * quadrant 0=bottom left, 1=bottom right or top left, 2=top right, 
	 */
	for (q=0; q<3; q++) {
		vmax[q] = 0;
	}
	/*
	 * get maximum absolute value in each quadrant
	 */
	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;
	j=0;	/* column counter	*/
	k=0;	/* row counter		*/
	for (i=0; i<nel; i++) {
		q = (j>=ny2) + (k>=nx2);
		if (vmax[q] < a[i]) vmax[q] = a[i];
		if (++j >= ny) {
			j = 0;
			k += 1;
		}
	}
	/*
	 * now calculate number of bits for each quadrant
	 */
	 
        /* this is a more efficient way to do this, */
 
 
        for (q = 0; q < 3; q++) {
            for (nbitplanes[q] = 0; vmax[q]>0; vmax[q] = vmax[q]>>1, nbitplanes[q]++) ; 
        }


/*
	for (q = 0; q < 3; q++) {
		nbitplanes[q] = log((float) (vmax[q]+1))/log(2.0)+0.5;
		if ( (vmax[q]+1) > (((LONGLONG) 1)<<nbitplanes[q]) ) {
			nbitplanes[q] += 1;
		}
	}
*/

	/*
	 * write nbitplanes
	 */

	if (0 == qwrite(outfile, (char *) nbitplanes, sizeof(nbitplanes))) {
	        *nlength = noutchar;
		ffpmsg("encode: output buffer too small");
		return(DATA_COMPRESSION_ERR);
        }
	 
	/*
	 * write coded array
	 */
	stat = doencode64(outfile, a, nx, ny, nbitplanes);
	/*
	 * write sign bits
	 */

	if (nsign > 0) {

	   if ( 0 == qwrite(outfile, (char *) signbits, nsign)) {
	        free(signbits);
	        *nlength = noutchar;
		ffpmsg("encode: output buffer too small");
		return(DATA_COMPRESSION_ERR);
          }
	} 

	free(signbits);
	*nlength = noutchar;

        if (noutchar >= noutmax) {
		ffpmsg("encode64: output buffer too small");
		return(DATA_COMPRESSION_ERR);
        }
		
	return(stat); 
}
/* ######################################################################### */
/* ######################################################################### */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* qwrite.c	Write binary data
 *
 * Programmer: R. White		Date: 11 March 1991
 */

/* ######################################################################### */
static void
writeint(char *outfile, int a)
{
int i;
unsigned char b[4];

	/* Write integer A one byte at a time to outfile.
	 *
	 * This is portable from Vax to Sun since it eliminates the
	 * need for byte-swapping.
	 */
	for (i=3; i>=0; i--) {
		b[i] = a & 0x000000ff;
		a >>= 8;
	}
	for (i=0; i<4; i++) qwrite(outfile, (char *) &b[i],1);
}

/* ######################################################################### */
static void
writelonglong(char *outfile, LONGLONG a)
{
int i;
unsigned char b[8];

	/* Write integer A one byte at a time to outfile.
	 *
	 * This is portable from Vax to Sun since it eliminates the
	 * need for byte-swapping.
	 */
	for (i=7; i>=0; i--) {
		b[i] = (unsigned char) (a & 0x000000ff);
		a >>= 8;
	}
	for (i=0; i<8; i++) qwrite(outfile, (char *) &b[i],1);
}
/* ######################################################################### */
static int
qwrite(char *file, char buffer[], int n){
    /*
     * write n bytes from buffer into file
     * returns number of bytes read (=n) if successful, <=0 if not
     */

     if (noutchar + n > noutmax) return(0);  /* buffer overflow */
     
     memcpy(&file[noutchar], buffer, n);
     noutchar += n;

     return(n);
}
/* ######################################################################### */
/* ######################################################################### */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* doencode.c	Encode 2-D array and write stream of characters on outfile
 *
 * This version assumes that A is positive.
 *
 * Programmer: R. White		Date: 7 May 1991
 */

/* ######################################################################### */
static int
doencode(char *outfile, int a[], int nx, int ny, unsigned char nbitplanes[3])
{
/* char *outfile;						 output data stream
int a[];							 Array of values to encode			
int nx,ny;							 Array dimensions [nx][ny]			
unsigned char nbitplanes[3];		 Number of bit planes in quadrants	
*/

int nx2, ny2, stat;

	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;
	/*
	 * Initialize bit output
	 */
	start_outputing_bits();
	/*
	 * write out the bit planes for each quadrant
	 */
	stat = qtree_encode(outfile, &a[0],          ny, nx2,  ny2,  nbitplanes[0]);

        if (!stat)
		stat = qtree_encode(outfile, &a[ny2],        ny, nx2,  ny/2, nbitplanes[1]);

        if (!stat)
		stat = qtree_encode(outfile, &a[ny*nx2],     ny, nx/2, ny2,  nbitplanes[1]);

        if (!stat)
		stat = qtree_encode(outfile, &a[ny*nx2+ny2], ny, nx/2, ny/2, nbitplanes[2]);
	/*
	 * Add zero as an EOF symbol
	 */
	output_nybble(outfile, 0);
	done_outputing_bits(outfile);
	
	return(stat);
}
/* ######################################################################### */
static int
doencode64(char *outfile, LONGLONG a[], int nx, int ny, unsigned char nbitplanes[3])
{
/* char *outfile;						 output data stream
LONGLONG a[];							 Array of values to encode			
int nx,ny;							 Array dimensions [nx][ny]			
unsigned char nbitplanes[3];		 Number of bit planes in quadrants	
*/

int nx2, ny2, stat;

	nx2 = (nx+1)/2;
	ny2 = (ny+1)/2;
	/*
	 * Initialize bit output
	 */
	start_outputing_bits();
	/*
	 * write out the bit planes for each quadrant
	 */
	stat = qtree_encode64(outfile, &a[0],          ny, nx2,  ny2,  nbitplanes[0]);

        if (!stat)
		stat = qtree_encode64(outfile, &a[ny2],        ny, nx2,  ny/2, nbitplanes[1]);

        if (!stat)
		stat = qtree_encode64(outfile, &a[ny*nx2],     ny, nx/2, ny2,  nbitplanes[1]);

        if (!stat)
		stat = qtree_encode64(outfile, &a[ny*nx2+ny2], ny, nx/2, ny/2, nbitplanes[2]);
	/*
	 * Add zero as an EOF symbol
	 */
	output_nybble(outfile, 0);
	done_outputing_bits(outfile);
	
	return(stat);
}
/* ######################################################################### */
/* ######################################################################### */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* BIT OUTPUT ROUTINES */


static LONGLONG bitcount;

/* THE BIT BUFFER */

static int buffer2;			/* Bits buffered for output	*/
static int bits_to_go2;			/* Number of bits free in buffer */


/* ######################################################################### */
/* INITIALIZE FOR BIT OUTPUT */

static void
start_outputing_bits(void)
{
	buffer2 = 0;			/* Buffer is empty to start	*/
	bits_to_go2 = 8;		/* with				*/
	bitcount = 0;
}

/* ######################################################################### */
/* OUTPUT N BITS (N must be <= 8) */

static void
output_nbits(char *outfile, int bits, int n)
{
    /* AND mask for the right-most n bits */
    static int mask[9] = {0, 1, 3, 7, 15, 31, 63, 127, 255};
	/*
	 * insert bits at end of buffer
	 */
	buffer2 <<= n;
/*	buffer2 |= ( bits & ((1<<n)-1) ); */
	buffer2 |= ( bits & (*(mask+n)) );
	bits_to_go2 -= n;
	if (bits_to_go2 <= 0) {
		/*
		 * buffer2 full, put out top 8 bits
		 */

	        outfile[noutchar] = ((buffer2>>(-bits_to_go2)) & 0xff);

		if (noutchar < noutmax) noutchar++;
		
		bits_to_go2 += 8;
	}
	bitcount += n;
}
/* ######################################################################### */
/*  OUTPUT a 4 bit nybble */
static void
output_nybble(char *outfile, int bits)
{
	/*
	 * insert 4 bits at end of buffer
	 */
	buffer2 = (buffer2<<4) | ( bits & 15 );
	bits_to_go2 -= 4;
	if (bits_to_go2 <= 0) {
		/*
		 * buffer2 full, put out top 8 bits
		 */

	        outfile[noutchar] = ((buffer2>>(-bits_to_go2)) & 0xff);

		if (noutchar < noutmax) noutchar++;
		
		bits_to_go2 += 8;
	}
	bitcount += 4;
}
/*  ############################################################################  */
/* OUTPUT array of 4 BITS  */

static void output_nnybble(char *outfile, int n, unsigned char array[])
{
	/* pack the 4 lower bits in each element of the array into the outfile array */

int ii, jj, kk = 0, shift;

	if (n == 1) {
		output_nybble(outfile, (int) array[0]);
		return;
	}
/* forcing byte alignment doesn;t help, and even makes it go slightly slower
if (bits_to_go2 != 8)
   output_nbits(outfile, kk, bits_to_go2);
*/
	if (bits_to_go2 <= 4)
	{
		/* just room for 1 nybble; write it out separately */
		output_nybble(outfile, array[0]);
		kk++;  /* index to next array element */

		if (n == 2)  /* only 1 more nybble to write out */
		{
			output_nybble(outfile, (int) array[1]);
			return;
		}
	}


        /* bits_to_go2 is now in the range 5 - 8 */
	shift = 8 - bits_to_go2;  

	/* now write out pairs of nybbles; this does not affect value of bits_to_go2 */
	jj = (n - kk) / 2;
	
	if (bits_to_go2 == 8) {
	    /* special case if nybbles are aligned on byte boundary */
	    /* this actually seems to make very little differnece in speed */
	    buffer2 = 0;
	    for (ii = 0; ii < jj; ii++)
	    {
		outfile[noutchar] = ((array[kk] & 15)<<4) | (array[kk+1] & 15);
		kk += 2;
		noutchar++;
	    }
	} else {
	    for (ii = 0; ii < jj; ii++)
	    {
		buffer2 = (buffer2<<8) | ((array[kk] & 15)<<4) | (array[kk+1] & 15);
		kk += 2;

		/*
		 buffer2 full, put out top 8 bits
		 */

	        outfile[noutchar] = ((buffer2>>shift) & 0xff);
		noutchar++;
	    }
	}

	bitcount += (8 * (ii - 1));

	/* write out last odd nybble, if present */
	if (kk != n) output_nybble(outfile, (int) array[n - 1]); 

	return; 
}


/* ######################################################################### */
/* FLUSH OUT THE LAST BITS */

static void
done_outputing_bits(char *outfile)
{
	if(bits_to_go2 < 8) {
/*		putc(buffer2<<bits_to_go2,outfile); */

	        outfile[noutchar] = (buffer2<<bits_to_go2);
		if (noutchar < noutmax) noutchar++;

		/* count the garbage bits too */
		bitcount += bits_to_go2;
	}
}
/* ######################################################################### */
/* ######################################################################### */
/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* qtree_encode.c	Encode values in quadrant of 2-D array using binary
 *					quadtree coding for each bit plane.  Assumes array is
 *					positive.
 *
 * Programmer: R. White		Date: 15 May 1991
 */

/*
 * Huffman code values and number of bits in each code
 */
static int code[16] =
	{
	0x3e, 0x00, 0x01, 0x08, 0x02, 0x09, 0x1a, 0x1b,
	0x03, 0x1c, 0x0a, 0x1d, 0x0b, 0x1e, 0x3f, 0x0c
	};
static int ncode[16] =
	{
	6,    3,    3,    4,    3,    4,    5,    5,
	3,    5,    4,    5,    4,    5,    6,    4
	};

/*
 * variables for bit output to buffer when Huffman coding
 */
static int bitbuffer, bits_to_go3;

/*
 * macros to write out 4-bit nybble, Huffman code for this value
 */


/* ######################################################################### */
static int
qtree_encode(char *outfile, int a[], int n, int nqx, int nqy, int nbitplanes)
{

/*
int a[];
int n;								 physical dimension of row in a		
int nqx;							 length of row			
int nqy;							 length of column (<=n)				
int nbitplanes;						 number of bit planes to output	
*/
	
int log2n, i, k, bit, b, bmax, nqmax, nqx2, nqy2, nx, ny;
unsigned char *scratch, *buffer;

	/*
	 * log2n is log2 of max(nqx,nqy) rounded up to next power of 2
	 */
	nqmax = (nqx>nqy) ? nqx : nqy;
	log2n = (int) (log((float) nqmax)/log(2.0)+0.5);
	if (nqmax > (1<<log2n)) {
		log2n += 1;
	}
	/*
	 * initialize buffer point, max buffer size
	 */
	nqx2 = (nqx+1)/2;
	nqy2 = (nqy+1)/2;
	bmax = (nqx2*nqy2+1)/2;
	/*
	 * We're indexing A as a 2-D array with dimensions (nqx,nqy).
	 * Scratch is 2-D with dimensions (nqx/2,nqy/2) rounded up.
	 * Buffer is used to store string of codes for output.
	 */
	scratch = (unsigned char *) malloc(2*bmax);
	buffer = (unsigned char *) malloc(bmax);
	if ((scratch == (unsigned char *) NULL) ||
		(buffer  == (unsigned char *) NULL)) {		
		ffpmsg("qtree_encode: insufficient memory");
		return(DATA_COMPRESSION_ERR);
	}
	/*
	 * now encode each bit plane, starting with the top
	 */
	for (bit=nbitplanes-1; bit >= 0; bit--) {
		/*
		 * initial bit buffer
		 */
		b = 0;
		bitbuffer = 0;
		bits_to_go3 = 0;
		/*
		 * on first pass copy A to scratch array
		 */
		qtree_onebit(a,n,nqx,nqy,scratch,bit);
		nx = (nqx+1)>>1;
		ny = (nqy+1)>>1;
		/*
		 * copy non-zero values to output buffer, which will be written
		 * in reverse order
		 */
		if (bufcopy(scratch,nx*ny,buffer,&b,bmax)) {
			/*
			 * quadtree is expanding data,
			 * change warning code and just fill buffer with bit-map
			 */
			write_bdirect(outfile,a,n,nqx,nqy,scratch,bit);
			goto bitplane_done;
		}
		/*
		 * do log2n reductions
		 */
		for (k = 1; k<log2n; k++) {
			qtree_reduce(scratch,ny,nx,ny,scratch);
			nx = (nx+1)>>1;
			ny = (ny+1)>>1;
			if (bufcopy(scratch,nx*ny,buffer,&b,bmax)) {
				write_bdirect(outfile,a,n,nqx,nqy,scratch,bit);
				goto bitplane_done;
			}
		}
		/*
		 * OK, we've got the code in buffer
		 * Write quadtree warning code, then write buffer in reverse order
		 */
		output_nybble(outfile,0xF);
		if (b==0) {
			if (bits_to_go3>0) {
				/*
				 * put out the last few bits
				 */
				output_nbits(outfile, bitbuffer & ((1<<bits_to_go3)-1),
					bits_to_go3);
			} else {
				/*
				 * have to write a zero nybble if there are no 1's in array
				 */
				output_huffman(outfile,0);
			}
		} else {
			if (bits_to_go3>0) {
				/*
				 * put out the last few bits
				 */
				output_nbits(outfile, bitbuffer & ((1<<bits_to_go3)-1),
					bits_to_go3);
			}
			for (i=b-1; i>=0; i--) {
				output_nbits(outfile,buffer[i],8);
			}
		}
		bitplane_done: ;
	}
	free(buffer);
	free(scratch);
	return(0);
}
/* ######################################################################### */
static int
qtree_encode64(char *outfile, LONGLONG a[], int n, int nqx, int nqy, int nbitplanes)
{

/*
LONGLONG a[];
int n;								 physical dimension of row in a		
int nqx;							 length of row			
int nqy;							 length of column (<=n)				
int nbitplanes;						 number of bit planes to output	
*/
	
int log2n, i, k, bit, b, nqmax, nqx2, nqy2, nx, ny;
int bmax;  /* this potentially needs to be made a 64-bit int to support large arrays */
unsigned char *scratch, *buffer;

	/*
	 * log2n is log2 of max(nqx,nqy) rounded up to next power of 2
	 */
	nqmax = (nqx>nqy) ? nqx : nqy;
	log2n = (int) (log((float) nqmax)/log(2.0)+0.5);
	if (nqmax > (1<<log2n)) {
		log2n += 1;
	}
	/*
	 * initialize buffer point, max buffer size
	 */
	nqx2 = (nqx+1)/2;
	nqy2 = (nqy+1)/2;
	bmax = (( nqx2)* ( nqy2)+1)/2;
	/*
	 * We're indexing A as a 2-D array with dimensions (nqx,nqy).
	 * Scratch is 2-D with dimensions (nqx/2,nqy/2) rounded up.
	 * Buffer is used to store string of codes for output.
	 */
	scratch = (unsigned char *) malloc(2*bmax);
	buffer = (unsigned char *) malloc(bmax);
	if ((scratch == (unsigned char *) NULL) ||
		(buffer  == (unsigned char *) NULL)) {
		ffpmsg("qtree_encode64: insufficient memory");
		return(DATA_COMPRESSION_ERR);
	}
	/*
	 * now encode each bit plane, starting with the top
	 */
	for (bit=nbitplanes-1; bit >= 0; bit--) {
		/*
		 * initial bit buffer
		 */
		b = 0;
		bitbuffer = 0;
		bits_to_go3 = 0;
		/*
		 * on first pass copy A to scratch array
		 */
		qtree_onebit64(a,n,nqx,nqy,scratch,bit);
		nx = (nqx+1)>>1;
		ny = (nqy+1)>>1;
		/*
		 * copy non-zero values to output buffer, which will be written
		 * in reverse order
		 */
		if (bufcopy(scratch,nx*ny,buffer,&b,bmax)) {
			/*
			 * quadtree is expanding data,
			 * change warning code and just fill buffer with bit-map
			 */
			write_bdirect64(outfile,a,n,nqx,nqy,scratch,bit);
			goto bitplane_done;
		}
		/*
		 * do log2n reductions
		 */
		for (k = 1; k<log2n; k++) {
			qtree_reduce(scratch,ny,nx,ny,scratch);
			nx = (nx+1)>>1;
			ny = (ny+1)>>1;
			if (bufcopy(scratch,nx*ny,buffer,&b,bmax)) {
				write_bdirect64(outfile,a,n,nqx,nqy,scratch,bit);
				goto bitplane_done;
			}
		}
		/*
		 * OK, we've got the code in buffer
		 * Write quadtree warning code, then write buffer in reverse order
		 */
		output_nybble(outfile,0xF);
		if (b==0) {
			if (bits_to_go3>0) {
				/*
				 * put out the last few bits
				 */
				output_nbits(outfile, bitbuffer & ((1<<bits_to_go3)-1),
					bits_to_go3);
			} else {
				/*
				 * have to write a zero nybble if there are no 1's in array
				 */
				output_huffman(outfile,0);
			}
		} else {
			if (bits_to_go3>0) {
				/*
				 * put out the last few bits
				 */
				output_nbits(outfile, bitbuffer & ((1<<bits_to_go3)-1),
					bits_to_go3);
			}
			for (i=b-1; i>=0; i--) {
				output_nbits(outfile,buffer[i],8);
			}
		}
		bitplane_done: ;
	}
	free(buffer);
	free(scratch);
	return(0);
}

/* ######################################################################### */
/*
 * copy non-zero codes from array to buffer
 */
static int
bufcopy(unsigned char a[], int n, unsigned char buffer[], int *b, int bmax)
{
int i;

	for (i = 0; i < n; i++) {
		if (a[i] != 0) {
			/*
			 * add Huffman code for a[i] to buffer
			 */
			bitbuffer |= code[a[i]] << bits_to_go3;
			bits_to_go3 += ncode[a[i]];
			if (bits_to_go3 >= 8) {
				buffer[*b] = bitbuffer & 0xFF;
				*b += 1;
				/*
				 * return warning code if we fill buffer
				 */
				if (*b >= bmax) return(1);
				bitbuffer >>= 8;
				bits_to_go3 -= 8;
			}
		}
	}
	return(0);
}

/* ######################################################################### */
/*
 * Do first quadtree reduction step on bit BIT of array A.
 * Results put into B.
 * 
 */
static void
qtree_onebit(int a[], int n, int nx, int ny, unsigned char b[], int bit)
{
int i, j, k;
int b0, b1, b2, b3;
int s10, s00;

	/*
	 * use selected bit to get amount to shift
	 */
	b0 = 1<<bit;
	b1 = b0<<1;
	b2 = b0<<2;
	b3 = b0<<3;
	k = 0;							/* k is index of b[i/2,j/2]	*/
	for (i = 0; i<nx-1; i += 2) {
		s00 = n*i;					/* s00 is index of a[i,j]	*/
/* tried using s00+n directly in the statements, but this had no effect on performance */
		s10 = s00+n;				/* s10 is index of a[i+1,j]	*/
		for (j = 0; j<ny-1; j += 2) {

/*
 this was not any faster..
 
         b[k] = (a[s00]  & b0) ? 
	            (a[s00+1] & b0) ?
	                (a[s10] & b0)   ?
		            (a[s10+1] & b0) ? 15 : 14
                         :  (a[s10+1] & b0) ? 13 : 12
		      : (a[s10] & b0)   ?
		            (a[s10+1] & b0) ? 11 : 10
                         :  (a[s10+1] & b0) ?  9 :  8
	          : (a[s00+1] & b0) ?
	                (a[s10] & b0)   ?
		            (a[s10+1] & b0) ? 7 : 6
                         :  (a[s10+1] & b0) ? 5 : 4

		      : (a[s10] & b0)   ?
		            (a[s10+1] & b0) ? 3 : 2
                         :  (a[s10+1] & b0) ? 1 : 0;
*/

/*
this alternative way of calculating b[k] was slowwer than the original code
		    if ( a[s00]     & b0)
			if ( a[s00+1]     & b0)
			    if ( a[s10]     & b0)
				if ( a[s10+1]     & b0)
					b[k] = 15;
				else
					b[k] = 14;
			    else
				if ( a[s10+1]     & b0)
					b[k] = 13;
				else
					b[k] = 12;
			else
			    if ( a[s10]     & b0)
				if ( a[s10+1]     & b0)
					b[k] = 11;
				else
					b[k] = 10;
			    else
				if ( a[s10+1]     & b0)
					b[k] = 9;
				else
					b[k] = 8;
		    else
			if ( a[s00+1]     & b0)
			    if ( a[s10]     & b0)
				if ( a[s10+1]     & b0)
					b[k] = 7;
				else
					b[k] = 6;
			    else
				if ( a[s10+1]     & b0)
					b[k] = 5;
				else
					b[k] = 4;
			else
			    if ( a[s10]     & b0)
				if ( a[s10+1]     & b0)
					b[k] = 3;
				else
					b[k] = 2;
			    else
				if ( a[s10+1]     & b0)
					b[k] = 1;
				else
					b[k] = 0;
*/
			


			b[k] = ( ( a[s10+1]     & b0)
				   | ((a[s10  ]<<1) & b1)
				   | ((a[s00+1]<<2) & b2)
				   | ((a[s00  ]<<3) & b3) ) >> bit;

			k += 1;
			s00 += 2;
			s10 += 2;
		}
		if (j < ny) {
			/*
			 * row size is odd, do last element in row
			 * s00+1,s10+1 are off edge
			 */
			b[k] = ( ((a[s10  ]<<1) & b1)
				   | ((a[s00  ]<<3) & b3) ) >> bit;
			k += 1;
		}
	}
	if (i < nx) {
		/*
		 * column size is odd, do last row
		 * s10,s10+1 are off edge
		 */
		s00 = n*i;
		for (j = 0; j<ny-1; j += 2) {
			b[k] = ( ((a[s00+1]<<2) & b2)
				   | ((a[s00  ]<<3) & b3) ) >> bit;
			k += 1;
			s00 += 2;
		}
		if (j < ny) {
			/*
			 * both row and column size are odd, do corner element
			 * s00+1, s10, s10+1 are off edge
			 */
			b[k] = ( ((a[s00  ]<<3) & b3) ) >> bit;
			k += 1;
		}
	}
}
/* ######################################################################### */
/*
 * Do first quadtree reduction step on bit BIT of array A.
 * Results put into B.
 * 
 */
static void
qtree_onebit64(LONGLONG a[], int n, int nx, int ny, unsigned char b[], int bit)
{
int i, j, k;
LONGLONG b0, b1, b2, b3;
int s10, s00;

	/*
	 * use selected bit to get amount to shift
	 */
	b0 = ((LONGLONG) 1)<<bit;
	b1 = b0<<1;
	b2 = b0<<2;
	b3 = b0<<3;
	k = 0;							/* k is index of b[i/2,j/2]	*/
	for (i = 0; i<nx-1; i += 2) {
		s00 = n*i;					/* s00 is index of a[i,j]	*/
		s10 = s00+n;				/* s10 is index of a[i+1,j]	*/
		for (j = 0; j<ny-1; j += 2) {
			b[k] = (unsigned char) (( ( a[s10+1]     & b0)
				   | ((a[s10  ]<<1) & b1)
				   | ((a[s00+1]<<2) & b2)
				   | ((a[s00  ]<<3) & b3) ) >> bit);
			k += 1;
			s00 += 2;
			s10 += 2;
		}
		if (j < ny) {
			/*
			 * row size is odd, do last element in row
			 * s00+1,s10+1 are off edge
			 */
			b[k] = (unsigned char) (( ((a[s10  ]<<1) & b1)
				   | ((a[s00  ]<<3) & b3) ) >> bit);
			k += 1;
		}
	}
	if (i < nx) {
		/*
		 * column size is odd, do last row
		 * s10,s10+1 are off edge
		 */
		s00 = n*i;
		for (j = 0; j<ny-1; j += 2) {
			b[k] = (unsigned char) (( ((a[s00+1]<<2) & b2)
				   | ((a[s00  ]<<3) & b3) ) >> bit);
			k += 1;
			s00 += 2;
		}
		if (j < ny) {
			/*
			 * both row and column size are odd, do corner element
			 * s00+1, s10, s10+1 are off edge
			 */
			b[k] = (unsigned char) (( ((a[s00  ]<<3) & b3) ) >> bit);
			k += 1;
		}
	}
}

/* ######################################################################### */
/*
 * do one quadtree reduction step on array a
 * results put into b (which may be the same as a)
 */
static void
qtree_reduce(unsigned char a[], int n, int nx, int ny, unsigned char b[])
{
int i, j, k;
int s10, s00;

	k = 0;							/* k is index of b[i/2,j/2]	*/
	for (i = 0; i<nx-1; i += 2) {
		s00 = n*i;					/* s00 is index of a[i,j]	*/
		s10 = s00+n;				/* s10 is index of a[i+1,j]	*/
		for (j = 0; j<ny-1; j += 2) {
			b[k] =	(a[s10+1] != 0)
				| ( (a[s10  ] != 0) << 1)
				| ( (a[s00+1] != 0) << 2)
				| ( (a[s00  ] != 0) << 3);
			k += 1;
			s00 += 2;
			s10 += 2;
		}
		if (j < ny) {
			/*
			 * row size is odd, do last element in row
			 * s00+1,s10+1 are off edge
			 */
			b[k] =  ( (a[s10  ] != 0) << 1)
				  | ( (a[s00  ] != 0) << 3);
			k += 1;
		}
	}
	if (i < nx) {
		/*
		 * column size is odd, do last row
		 * s10,s10+1 are off edge
		 */
		s00 = n*i;
		for (j = 0; j<ny-1; j += 2) {
			b[k] =  ( (a[s00+1] != 0) << 2)
				  | ( (a[s00  ] != 0) << 3);
			k += 1;
			s00 += 2;
		}
		if (j < ny) {
			/*
			 * both row and column size are odd, do corner element
			 * s00+1, s10, s10+1 are off edge
			 */
			b[k] = ( (a[s00  ] != 0) << 3);
			k += 1;
		}
	}
}

/* ######################################################################### */
static void
write_bdirect(char *outfile, int a[], int n,int nqx, int nqy, unsigned char scratch[], int bit)
{

	/*
	 * Write the direct bitmap warning code
	 */
	output_nybble(outfile,0x0);
	/*
	 * Copy A to scratch array (again!), packing 4 bits/nybble
	 */
	qtree_onebit(a,n,nqx,nqy,scratch,bit);
	/*
	 * write to outfile
	 */
/*
int i;
	for (i = 0; i < ((nqx+1)/2) * ((nqy+1)/2); i++) {
		output_nybble(outfile,scratch[i]);
	}
*/
	output_nnybble(outfile, ((nqx+1)/2) * ((nqy+1)/2), scratch);

}
/* ######################################################################### */
static void
write_bdirect64(char *outfile, LONGLONG a[], int n,int nqx, int nqy, unsigned char scratch[], int bit)
{

	/*
	 * Write the direct bitmap warning code
	 */
	output_nybble(outfile,0x0);
	/*
	 * Copy A to scratch array (again!), packing 4 bits/nybble
	 */
	qtree_onebit64(a,n,nqx,nqy,scratch,bit);
	/*
	 * write to outfile
	 */
/*
int i;
	for (i = 0; i < ((nqx+1)/2) * ((nqy+1)/2); i++) {
		output_nybble(outfile,scratch[i]);
	}
*/
	output_nnybble(outfile, ((nqx+1)/2) * ((nqy+1)/2), scratch);
}
