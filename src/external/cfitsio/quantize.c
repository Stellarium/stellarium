/*
  The following code is based on algorithms written by Richard White at STScI and made
  available for use in CFITSIO in July 1999 and updated in January 2008. 
*/

# include <stdio.h>
# include <stdlib.h>
# include <math.h>
# include <limits.h>
# include <float.h>

#include "fitsio2.h"

/* nearest integer function */
# define NINT(x)  ((x >= 0.) ? (int) (x + 0.5) : (int) (x - 0.5))

#define NULL_VALUE -2147483647 /* value used to represent undefined pixels */
#define ZERO_VALUE -2147483646 /* value used to represent zero-valued pixels */
#define N_RESERVED_VALUES 10   /* number of reserved values, starting with */
                               /* and including NULL_VALUE.  These values */
                               /* may not be used to represent the quantized */
                               /* and scaled floating point pixel values */
			       /* If lossy Hcompression is used, and the */
			       /* array contains null values, then it is also */
			       /* possible for the compressed values to slightly */
			       /* exceed the range of the actual (lossless) values */
			       /* so we must reserve a little more space */
			       
/* more than this many standard deviations from the mean is an outlier */
# define SIGMA_CLIP     5.
# define NITER          3	/* number of sigma-clipping iterations */

static int FnMeanSigma_short(short *array, long npix, int nullcheck, 
  short nullvalue, long *ngoodpix, double *mean, double *sigma, int *status);       
static int FnMeanSigma_int(int *array, long npix, int nullcheck,
  int nullvalue, long *ngoodpix, double *mean, double *sigma, int *status);       
static int FnMeanSigma_float(float *array, long npix, int nullcheck,
  float nullvalue, long *ngoodpix, double *mean, double *sigma, int *status);       
static int FnMeanSigma_double(double *array, long npix, int nullcheck,
  double nullvalue, long *ngoodpix, double *mean, double *sigma, int *status);       

static int FnNoise5_short(short *array, long nx, long ny, int nullcheck, 
   short nullvalue, long *ngood, short *minval, short *maxval, 
   double *n2, double *n3, double *n5, int *status);   
static int FnNoise5_int(int *array, long nx, long ny, int nullcheck, 
   int nullvalue, long *ngood, int *minval, int *maxval, 
   double *n2, double *n3, double *n5, int *status);   
static int FnNoise5_float(float *array, long nx, long ny, int nullcheck, 
   float nullvalue, long *ngood, float *minval, float *maxval, 
   double *n2, double *n3, double *n5, int *status);   
static int FnNoise5_double(double *array, long nx, long ny, int nullcheck, 
   double nullvalue, long *ngood, double *minval, double *maxval, 
   double *n2, double *n3, double *n5, int *status);   

static int FnNoise3_short(short *array, long nx, long ny, int nullcheck, 
   short nullvalue, long *ngood, short *minval, short *maxval, double *noise, int *status);       
static int FnNoise3_int(int *array, long nx, long ny, int nullcheck, 
   int nullvalue, long *ngood, int *minval, int *maxval, double *noise, int *status);          
static int FnNoise3_float(float *array, long nx, long ny, int nullcheck, 
   float nullvalue, long *ngood, float *minval, float *maxval, double *noise, int *status);        
static int FnNoise3_double(double *array, long nx, long ny, int nullcheck, 
   double nullvalue, long *ngood, double *minval, double *maxval, double *noise, int *status);        

static int FnNoise1_short(short *array, long nx, long ny, 
   int nullcheck, short nullvalue, double *noise, int *status);       
static int FnNoise1_int(int *array, long nx, long ny, 
   int nullcheck, int nullvalue, double *noise, int *status);       
static int FnNoise1_float(float *array, long nx, long ny, 
   int nullcheck, float nullvalue, double *noise, int *status);       
static int FnNoise1_double(double *array, long nx, long ny, 
   int nullcheck, double nullvalue, double *noise, int *status);       

static int FnCompare_short (const void *, const void *);
static int FnCompare_int (const void *, const void *);
static int FnCompare_float (const void *, const void *);
static int FnCompare_double (const void *, const void *);
static float quick_select_float(float arr[], int n);
static short quick_select_short(short arr[], int n);
static int quick_select_int(int arr[], int n);
static LONGLONG quick_select_longlong(LONGLONG arr[], int n);
static double quick_select_double(double arr[], int n);

/*---------------------------------------------------------------------------*/
int fits_quantize_float (long row, float fdata[], long nxpix, long nypix, int nullcheck, 
	float in_null_value, float qlevel, int dither_method, int idata[], double *bscale,
	double *bzero, int *iminval, int *imaxval) {

/* arguments:
long row            i: if positive, used to calculate random dithering seed value
                       (this is only used when dithering the quantized values)
float fdata[]       i: array of image pixels to be compressed
long nxpix          i: number of pixels in each row of fdata
long nypix          i: number of rows in fdata
nullcheck           i: check for nullvalues in fdata?
float in_null_value i: value used to represent undefined pixels in fdata
float qlevel        i: quantization level
int dither_method   i; which dithering method to use
int idata[]         o: values of fdata after applying bzero and bscale
double bscale       o: scale factor
double bzero        o: zero offset
int iminval         o: minimum quantized value that is returned
int imaxval         o: maximum quantized value that is returned

The function value will be one if the input fdata were copied to idata;
in this case the parameters bscale and bzero can be used to convert back to
nearly the original floating point values:  fdata ~= idata * bscale + bzero.
If the function value is zero, the data were not copied to idata.
*/

	int status, iseed = 0;
	long i, nx, ngood = 0;
	double stdev, noise2, noise3, noise5;	/* MAD 2nd, 3rd, and 5th order noise values */
	float minval = 0., maxval = 0.;  /* min & max of fdata */
	double delta;		/* bscale, 1 in idata = delta in fdata */
	double zeropt;	        /* bzero */
	double temp;
        int nextrand = 0;
	extern float *fits_rand_value; /* this is defined in imcompress.c */
	LONGLONG iqfactor;

	nx = nxpix * nypix;
	if (nx <= 1) {
	    *bscale = 1.;
	    *bzero  = 0.;
	    return (0);
	}

        if (qlevel >= 0.) {

	    /* estimate background noise using MAD pixel differences */
	    FnNoise5_float(fdata, nxpix, nypix, nullcheck, in_null_value, &ngood,
	        &minval, &maxval, &noise2, &noise3, &noise5, &status);      

	    if (nullcheck && ngood == 0) {   /* special case of an image filled with Nulls */
	        /* set parameters to dummy values, which are not used */
		minval = 0.;
		maxval = 1.;
		stdev = 1;
	    } else {

	        /* use the minimum of noise2, noise3, and noise5 as the best noise value */
	        stdev = noise3;
	        if (noise2 != 0. && noise2 < stdev) stdev = noise2;
	        if (noise5 != 0. && noise5 < stdev) stdev = noise5;
            }

	    if (qlevel == 0.)
	        delta = stdev / 4.;  /* default quantization */
	    else
	        delta = stdev / qlevel;

	    if (delta == 0.) 
	        return (0);			/* don't quantize */

	} else {
	    /* negative value represents the absolute quantization level */
	    delta = -qlevel;

	    /* only nned to calculate the min and max values */
	    FnNoise3_float(fdata, nxpix, nypix, nullcheck, in_null_value, &ngood,
	        &minval, &maxval, 0, &status);      
 	}

        /* check that the range of quantized levels is not > range of int */
	if ((maxval - minval) / delta > 2. * 2147483647. - N_RESERVED_VALUES )
	    return (0);			/* don't quantize */

        if (row > 0) { /* we need to dither the quantized values */
            if (!fits_rand_value) 
	        if (fits_init_randoms()) return(MEMORY_ALLOCATION);

	    /* initialize the index to the next random number in the list */
            iseed = (int) ((row - 1) % N_RANDOM);
	    nextrand = (int) (fits_rand_value[iseed] * 500.);
	}

        if (ngood == nx) {   /* don't have to check for nulls */
            /* return all positive values, if possible since some */
            /* compression algorithms either only work for positive integers, */
            /* or are more efficient.  */

            if (dither_method == SUBTRACTIVE_DITHER_2)
	    {
                /* shift the range to be close to the value used to represent zeros */
                zeropt = minval - delta * (NULL_VALUE + N_RESERVED_VALUES);
            }
	    else if ((maxval - minval) / delta < 2147483647. - N_RESERVED_VALUES )
            {
                zeropt = minval;
		/* fudge the zero point so it is an integer multiple of delta */
		/* This helps to ensure the same scaling will be performed if the */
		/* file undergoes multiple fpack/funpack cycles */
		iqfactor = (LONGLONG) (zeropt/delta  + 0.5);
		zeropt = iqfactor * delta;               
            }
            else
            {
                /* center the quantized levels around zero */
                zeropt = (minval + maxval) / 2.;
            }

            if (row > 0) {  /* dither the values when quantizing */
              for (i = 0;  i < nx;  i++) {
	    
		if (dither_method == SUBTRACTIVE_DITHER_2 && fdata[i] == 0.0) {
		   idata[i] = ZERO_VALUE;
		} else {
		   idata[i] =  NINT((((double) fdata[i] - zeropt) / delta) + fits_rand_value[nextrand] - 0.5);
		}

                nextrand++;
		if (nextrand == N_RANDOM) {
		    iseed++;
		    if (iseed == N_RANDOM) iseed = 0;
	            nextrand = (int) (fits_rand_value[iseed] * 500);
                }
              }
            } else {  /* do not dither the values */

       	        for (i = 0;  i < nx;  i++) {
	            idata[i] = NINT ((fdata[i] - zeropt) / delta);
                }
            } 
        }
        else {
            /* data contains null values; shift the range to be */
            /* close to the value used to represent null values */
            zeropt = minval - delta * (NULL_VALUE + N_RESERVED_VALUES);

            if (row > 0) {  /* dither the values */
	      for (i = 0;  i < nx;  i++) {
                if (fdata[i] != in_null_value) {
		    if (dither_method == SUBTRACTIVE_DITHER_2 && fdata[i] == 0.0) {
		       idata[i] = ZERO_VALUE;
		    } else {
		       idata[i] =  NINT((((double) fdata[i] - zeropt) / delta) + fits_rand_value[nextrand] - 0.5);
		    }
                } else {
                    idata[i] = NULL_VALUE;
                }

                /* increment the random number index, regardless */
                nextrand++;
		if (nextrand == N_RANDOM) {
		    iseed++;
		    if (iseed == N_RANDOM) iseed = 0;
	            nextrand = (int) (fits_rand_value[iseed] * 500);
                }
              }
            } else {  /* do not dither the values */
	       for (i = 0;  i < nx;  i++) {
 
                 if (fdata[i] != in_null_value) {
		    idata[i] =  NINT((fdata[i] - zeropt) / delta);
                 } else { 
                    idata[i] = NULL_VALUE;
                 }
               }
            }
	}

        /* calc min and max values */
        temp = (minval - zeropt) / delta;
        *iminval =  NINT (temp);
        temp = (maxval - zeropt) / delta;
        *imaxval =  NINT (temp);

	*bscale = delta;
	*bzero = zeropt;
	return (1);			/* yes, data have been quantized */
}
/*---------------------------------------------------------------------------*/
int fits_quantize_double (long row, double fdata[], long nxpix, long nypix, int nullcheck, 
	double in_null_value, float qlevel, int dither_method, int idata[], double *bscale,
	double *bzero, int *iminval, int *imaxval) {

/* arguments:
long row            i: tile number = row number in the binary table
                       (this is only used when dithering the quantized values)
double fdata[]      i: array of image pixels to be compressed
long nxpix          i: number of pixels in each row of fdata
long nypix          i: number of rows in fdata
nullcheck           i: check for nullvalues in fdata?
double in_null_value i: value used to represent undefined pixels in fdata
float qlevel        i: quantization level
int dither_method   i; which dithering method to use
int idata[]         o: values of fdata after applying bzero and bscale
double bscale       o: scale factor
double bzero        o: zero offset
int iminval         o: minimum quantized value that is returned
int imaxval         o: maximum quantized value that is returned

The function value will be one if the input fdata were copied to idata;
in this case the parameters bscale and bzero can be used to convert back to
nearly the original floating point values:  fdata ~= idata * bscale + bzero.
If the function value is zero, the data were not copied to idata.
*/

	int status, iseed = 0;
	long i, nx, ngood = 0;
	double stdev, noise2 = 0., noise3 = 0., noise5 = 0.;	/* MAD 2nd, 3rd, and 5th order noise values */
	double minval = 0., maxval = 0.;  /* min & max of fdata */
	double delta;		/* bscale, 1 in idata = delta in fdata */
	double zeropt;	        /* bzero */
	double temp;
        int nextrand = 0;
	extern float *fits_rand_value;
	LONGLONG iqfactor;

	nx = nxpix * nypix;
	if (nx <= 1) {
	    *bscale = 1.;
	    *bzero  = 0.;
	    return (0);
	}

        if (qlevel >= 0.) {

	    /* estimate background noise using MAD pixel differences */
	    FnNoise5_double(fdata, nxpix, nypix, nullcheck, in_null_value, &ngood,
	        &minval, &maxval, &noise2, &noise3, &noise5, &status);      

	    if (nullcheck && ngood == 0) {   /* special case of an image filled with Nulls */
	        /* set parameters to dummy values, which are not used */
		minval = 0.;
		maxval = 1.;
		stdev = 1;
	    } else {

	        /* use the minimum of noise2, noise3, and noise5 as the best noise value */
	        stdev = noise3;
	        if (noise2 != 0. && noise2 < stdev) stdev = noise2;
	        if (noise5 != 0. && noise5 < stdev) stdev = noise5;
            }

	    if (qlevel == 0.)
	        delta = stdev / 4.;  /* default quantization */
	    else
	        delta = stdev / qlevel;

	    if (delta == 0.) 
	        return (0);			/* don't quantize */

	} else {
	    /* negative value represents the absolute quantization level */
	    delta = -qlevel;

	    /* only nned to calculate the min and max values */
	    FnNoise3_double(fdata, nxpix, nypix, nullcheck, in_null_value, &ngood,
	        &minval, &maxval, 0, &status);      
 	}

        /* check that the range of quantized levels is not > range of int */
	if ((maxval - minval) / delta > 2. * 2147483647. - N_RESERVED_VALUES )
	    return (0);			/* don't quantize */

        if (row > 0) { /* we need to dither the quantized values */
            if (!fits_rand_value) 
	       if (fits_init_randoms()) return(MEMORY_ALLOCATION);

	    /* initialize the index to the next random number in the list */
            iseed = (int) ((row - 1) % N_RANDOM);
	    nextrand = (int) (fits_rand_value[iseed] * 500);
	}

        if (ngood == nx) {   /* don't have to check for nulls */
            /* return all positive values, if possible since some */
            /* compression algorithms either only work for positive integers, */
            /* or are more efficient.  */

            if (dither_method == SUBTRACTIVE_DITHER_2)
	    {
                /* shift the range to be close to the value used to represent zeros */
                zeropt = minval - delta * (NULL_VALUE + N_RESERVED_VALUES);
            }
	    else if ((maxval - minval) / delta < 2147483647. - N_RESERVED_VALUES )
            {
                zeropt = minval;
		/* fudge the zero point so it is an integer multiple of delta */
		/* This helps to ensure the same scaling will be performed if the */
		/* file undergoes multiple fpack/funpack cycles */
		iqfactor = (LONGLONG) (zeropt/delta  + 0.5);
		zeropt = iqfactor * delta;               
            }
            else
            {
                /* center the quantized levels around zero */
                zeropt = (minval + maxval) / 2.;
            }

            if (row > 0) {  /* dither the values when quantizing */
       	      for (i = 0;  i < nx;  i++) {

		if (dither_method == SUBTRACTIVE_DITHER_2 && fdata[i] == 0.0) {
		   idata[i] = ZERO_VALUE;
		} else {
		   idata[i] =  NINT((((double) fdata[i] - zeropt) / delta) + fits_rand_value[nextrand] - 0.5);
		}

                nextrand++;
		if (nextrand == N_RANDOM) {
                    iseed++;
		    if (iseed == N_RANDOM) iseed = 0;
	            nextrand = (int) (fits_rand_value[iseed] * 500);
                }
              }
            } else {  /* do not dither the values */

       	        for (i = 0;  i < nx;  i++) {
	            idata[i] = NINT ((fdata[i] - zeropt) / delta);
                }
            } 
        }
        else {
            /* data contains null values; shift the range to be */
            /* close to the value used to represent null values */
            zeropt = minval - delta * (NULL_VALUE + N_RESERVED_VALUES);

            if (row > 0) {  /* dither the values */
	      for (i = 0;  i < nx;  i++) {
                if (fdata[i] != in_null_value) {
		    if (dither_method == SUBTRACTIVE_DITHER_2 && fdata[i] == 0.0) {
		       idata[i] = ZERO_VALUE;
		    } else {
		       idata[i] =  NINT((((double) fdata[i] - zeropt) / delta) + fits_rand_value[nextrand] - 0.5);
		    }
                } else {
                    idata[i] = NULL_VALUE;
                }

                /* increment the random number index, regardless */
                nextrand++;
		if (nextrand == N_RANDOM) {
		    iseed++;
		    if (iseed == N_RANDOM) iseed = 0;
	            nextrand = (int) (fits_rand_value[iseed] * 500);
                }
              }
            } else {  /* do not dither the values */
	       for (i = 0;  i < nx;  i++) {
                 if (fdata[i] != in_null_value)
		    idata[i] =  NINT((fdata[i] - zeropt) / delta);
                 else 
                    idata[i] = NULL_VALUE;
               }
            }
	}

        /* calc min and max values */
        temp = (minval - zeropt) / delta;
        *iminval =  NINT (temp);
        temp = (maxval - zeropt) / delta;
        *imaxval =  NINT (temp);

	*bscale = delta;
	*bzero = zeropt;

	return (1);			/* yes, data have been quantized */
}
/*--------------------------------------------------------------------------*/
int fits_img_stats_short(short *array, /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
	long ny,            /* number of rows in the image */
	                    /* (if this is a 3D image, then ny should be the */
			    /* product of the no. of rows times the no. of planes) */
	int nullcheck,      /* check for null values, if true */
	short nullvalue,    /* value of null pixels, if nullcheck is true */

   /* returned parameters (if the pointer is not null)  */
	long *ngoodpix,     /* number of non-null pixels in the image */
	short *minvalue,    /* returned minimum non-null value in the array */
	short *maxvalue,    /* returned maximum non-null value in the array */
	double *mean,       /* returned mean value of all non-null pixels */
	double *sigma,      /* returned R.M.S. value of all non-null pixels */
	double *noise1,     /* 1st order estimate of noise in image background level */
	double *noise2,     /* 2nd order estimate of noise in image background level */
	double *noise3,     /* 3rd order estimate of noise in image background level */
	double *noise5,     /* 5th order estimate of noise in image background level */
	int *status)        /* error status */

/*
    Compute statistics of the input short integer image.
*/
{
	long ngood;
	short minval = 0, maxval = 0;
	double xmean = 0., xsigma = 0., xnoise = 0., xnoise2 = 0., xnoise3 = 0., xnoise5 = 0.;

	/* need to calculate mean and/or sigma and/or limits? */
	if (mean || sigma ) {
		FnMeanSigma_short(array, nx * ny, nullcheck, nullvalue, 
			&ngood, &xmean, &xsigma, status);

	    if (ngoodpix) *ngoodpix = ngood;
	    if (mean)     *mean = xmean;
	    if (sigma)    *sigma = xsigma;
	}

	if (noise1) {
		FnNoise1_short(array, nx, ny, nullcheck, nullvalue, 
		  &xnoise, status);

		*noise1  = xnoise;
	}

	if (minvalue || maxvalue || noise3) {
		FnNoise5_short(array, nx, ny, nullcheck, nullvalue, 
			&ngood, &minval, &maxval, &xnoise2, &xnoise3, &xnoise5, status);

		if (ngoodpix) *ngoodpix = ngood;
		if (minvalue) *minvalue= minval;
		if (maxvalue) *maxvalue = maxval;
		if (noise2) *noise2  = xnoise2;
		if (noise3) *noise3  = xnoise3;
		if (noise5) *noise5  = xnoise5;
	}
	return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_img_stats_int(int *array, /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
	long ny,            /* number of rows in the image */
	                    /* (if this is a 3D image, then ny should be the */
			    /* product of the no. of rows times the no. of planes) */
	int nullcheck,      /* check for null values, if true */
	int nullvalue,    /* value of null pixels, if nullcheck is true */

   /* returned parameters (if the pointer is not null)  */
	long *ngoodpix,     /* number of non-null pixels in the image */
	int *minvalue,    /* returned minimum non-null value in the array */
	int *maxvalue,    /* returned maximum non-null value in the array */
	double *mean,       /* returned mean value of all non-null pixels */
	double *sigma,      /* returned R.M.S. value of all non-null pixels */
	double *noise1,     /* 1st order estimate of noise in image background level */
	double *noise2,     /* 2nd order estimate of noise in image background level */
	double *noise3,     /* 3rd order estimate of noise in image background level */
	double *noise5,     /* 5th order estimate of noise in image background level */
	int *status)        /* error status */

/*
    Compute statistics of the input integer image.
*/
{
	long ngood;
	int minval = 0, maxval = 0;
	double xmean = 0., xsigma = 0., xnoise = 0., xnoise2 = 0., xnoise3 = 0., xnoise5 = 0.;

	/* need to calculate mean and/or sigma and/or limits? */
	if (mean || sigma ) {
		FnMeanSigma_int(array, nx * ny, nullcheck, nullvalue, 
			&ngood, &xmean, &xsigma, status);

	    if (ngoodpix) *ngoodpix = ngood;
	    if (mean)     *mean = xmean;
	    if (sigma)    *sigma = xsigma;
	}

	if (noise1) {
		FnNoise1_int(array, nx, ny, nullcheck, nullvalue, 
		  &xnoise, status);

		*noise1  = xnoise;
	}

	if (minvalue || maxvalue || noise3) {
		FnNoise5_int(array, nx, ny, nullcheck, nullvalue, 
			&ngood, &minval, &maxval, &xnoise2, &xnoise3, &xnoise5, status);

		if (ngoodpix) *ngoodpix = ngood;
		if (minvalue) *minvalue= minval;
		if (maxvalue) *maxvalue = maxval;
		if (noise2) *noise2  = xnoise2;
		if (noise3) *noise3  = xnoise3;
		if (noise5) *noise5  = xnoise5;
	}
	return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_img_stats_float(float *array, /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
	long ny,            /* number of rows in the image */
	                    /* (if this is a 3D image, then ny should be the */
			    /* product of the no. of rows times the no. of planes) */
	int nullcheck,      /* check for null values, if true */
	float nullvalue,    /* value of null pixels, if nullcheck is true */

   /* returned parameters (if the pointer is not null)  */
	long *ngoodpix,     /* number of non-null pixels in the image */
	float *minvalue,    /* returned minimum non-null value in the array */
	float *maxvalue,    /* returned maximum non-null value in the array */
	double *mean,       /* returned mean value of all non-null pixels */
	double *sigma,      /* returned R.M.S. value of all non-null pixels */
	double *noise1,     /* 1st order estimate of noise in image background level */
	double *noise2,     /* 2nd order estimate of noise in image background level */
	double *noise3,     /* 3rd order estimate of noise in image background level */
	double *noise5,     /* 5th order estimate of noise in image background level */
	int *status)        /* error status */

/*
    Compute statistics of the input float image.
*/
{
	long ngood;
	float minval, maxval;
	double xmean = 0., xsigma = 0., xnoise = 0., xnoise2 = 0., xnoise3 = 0., xnoise5 = 0.;

	/* need to calculate mean and/or sigma and/or limits? */
	if (mean || sigma ) {
		FnMeanSigma_float(array, nx * ny, nullcheck, nullvalue, 
			&ngood, &xmean, &xsigma, status);

	    if (ngoodpix) *ngoodpix = ngood;
	    if (mean)     *mean = xmean;
	    if (sigma)    *sigma = xsigma;
	}

	if (noise1) {
		FnNoise1_float(array, nx, ny, nullcheck, nullvalue, 
		  &xnoise, status);

		*noise1  = xnoise;
	}

	if (minvalue || maxvalue || noise3) {
		FnNoise5_float(array, nx, ny, nullcheck, nullvalue, 
			&ngood, &minval, &maxval, &xnoise2, &xnoise3, &xnoise5, status);

		if (ngoodpix) *ngoodpix = ngood;
		if (minvalue) *minvalue= minval;
		if (maxvalue) *maxvalue = maxval;
		if (noise2) *noise2  = xnoise2;
		if (noise3) *noise3  = xnoise3;
		if (noise5) *noise5  = xnoise5;
	}
	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnMeanSigma_short
       (short *array,       /*  2 dimensional array of image pixels */
        long npix,          /* number of pixels in the image */
	int nullcheck,      /* check for null values, if true */
	short nullvalue,    /* value of null pixels, if nullcheck is true */

   /* returned parameters */
   
	long *ngoodpix,     /* number of non-null pixels in the image */
	double *mean,       /* returned mean value of all non-null pixels */
	double *sigma,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */

/*
Compute mean and RMS sigma of the non-null pixels in the input array.
*/
{
	long ii, ngood = 0;
	short *value;
	double sum = 0., sum2 = 0., xtemp;

	value = array;
	    
	if (nullcheck) {
	        for (ii = 0; ii < npix; ii++, value++) {
		    if (*value != nullvalue) {
		        ngood++;
		        xtemp = (double) *value;
		        sum += xtemp;
		        sum2 += (xtemp * xtemp);
		    }
		}
	} else {
	        ngood = npix;
	        for (ii = 0; ii < npix; ii++, value++) {
		        xtemp = (double) *value;
		        sum += xtemp;
		        sum2 += (xtemp * xtemp);
		}
	}

	if (ngood > 1) {
		if (ngoodpix) *ngoodpix = ngood;
		xtemp = sum / ngood;
		if (mean)     *mean = xtemp;
		if (sigma)    *sigma = sqrt((sum2 / ngood) - (xtemp * xtemp));
	} else if (ngood == 1){
		if (ngoodpix) *ngoodpix = 1;
		if (mean)     *mean = sum;
		if (sigma)    *sigma = 0.0;
	} else {
		if (ngoodpix) *ngoodpix = 0;
	        if (mean)     *mean = 0.;
		if (sigma)    *sigma = 0.;
	}	    
	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnMeanSigma_int
       (int *array,       /*  2 dimensional array of image pixels */
        long npix,          /* number of pixels in the image */
	int nullcheck,      /* check for null values, if true */
	int nullvalue,    /* value of null pixels, if nullcheck is true */

   /* returned parameters */
   
	long *ngoodpix,     /* number of non-null pixels in the image */
	double *mean,       /* returned mean value of all non-null pixels */
	double *sigma,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */

/*
Compute mean and RMS sigma of the non-null pixels in the input array.
*/
{
	long ii, ngood = 0;
	int *value;
	double sum = 0., sum2 = 0., xtemp;

	value = array;
	    
	if (nullcheck) {
	        for (ii = 0; ii < npix; ii++, value++) {
		    if (*value != nullvalue) {
		        ngood++;
		        xtemp = (double) *value;
		        sum += xtemp;
		        sum2 += (xtemp * xtemp);
		    }
		}
	} else {
	        ngood = npix;
	        for (ii = 0; ii < npix; ii++, value++) {
		        xtemp = (double) *value;
		        sum += xtemp;
		        sum2 += (xtemp * xtemp);
		}
	}

	if (ngood > 1) {
		if (ngoodpix) *ngoodpix = ngood;
		xtemp = sum / ngood;
		if (mean)     *mean = xtemp;
		if (sigma)    *sigma = sqrt((sum2 / ngood) - (xtemp * xtemp));
	} else if (ngood == 1){
		if (ngoodpix) *ngoodpix = 1;
		if (mean)     *mean = sum;
		if (sigma)    *sigma = 0.0;
	} else {
		if (ngoodpix) *ngoodpix = 0;
	        if (mean)     *mean = 0.;
		if (sigma)    *sigma = 0.;
	}	    
	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnMeanSigma_float
       (float *array,       /*  2 dimensional array of image pixels */
        long npix,          /* number of pixels in the image */
	int nullcheck,      /* check for null values, if true */
	float nullvalue,    /* value of null pixels, if nullcheck is true */

   /* returned parameters */
   
	long *ngoodpix,     /* number of non-null pixels in the image */
	double *mean,       /* returned mean value of all non-null pixels */
	double *sigma,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */

/*
Compute mean and RMS sigma of the non-null pixels in the input array.
*/
{
	long ii, ngood = 0;
	float *value;
	double sum = 0., sum2 = 0., xtemp;

	value = array;
	    
	if (nullcheck) {
	        for (ii = 0; ii < npix; ii++, value++) {
		    if (*value != nullvalue) {
		        ngood++;
		        xtemp = (double) *value;
		        sum += xtemp;
		        sum2 += (xtemp * xtemp);
		    }
		}
	} else {
	        ngood = npix;
	        for (ii = 0; ii < npix; ii++, value++) {
		        xtemp = (double) *value;
		        sum += xtemp;
		        sum2 += (xtemp * xtemp);
		}
	}

	if (ngood > 1) {
		if (ngoodpix) *ngoodpix = ngood;
		xtemp = sum / ngood;
		if (mean)     *mean = xtemp;
		if (sigma)    *sigma = sqrt((sum2 / ngood) - (xtemp * xtemp));
	} else if (ngood == 1){
		if (ngoodpix) *ngoodpix = 1;
		if (mean)     *mean = sum;
		if (sigma)    *sigma = 0.0;
	} else {
		if (ngoodpix) *ngoodpix = 0;
	        if (mean)     *mean = 0.;
		if (sigma)    *sigma = 0.;
	}	    
	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnMeanSigma_double
       (double *array,       /*  2 dimensional array of image pixels */
        long npix,          /* number of pixels in the image */
	int nullcheck,      /* check for null values, if true */
	double nullvalue,    /* value of null pixels, if nullcheck is true */

   /* returned parameters */
   
	long *ngoodpix,     /* number of non-null pixels in the image */
	double *mean,       /* returned mean value of all non-null pixels */
	double *sigma,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */

/*
Compute mean and RMS sigma of the non-null pixels in the input array.
*/
{
	long ii, ngood = 0;
	double *value;
	double sum = 0., sum2 = 0., xtemp;

	value = array;
	    
	if (nullcheck) {
	        for (ii = 0; ii < npix; ii++, value++) {
		    if (*value != nullvalue) {
		        ngood++;
		        xtemp = *value;
		        sum += xtemp;
		        sum2 += (xtemp * xtemp);
		    }
		}
	} else {
	        ngood = npix;
	        for (ii = 0; ii < npix; ii++, value++) {
		        xtemp = *value;
		        sum += xtemp;
		        sum2 += (xtemp * xtemp);
		}
	}

	if (ngood > 1) {
		if (ngoodpix) *ngoodpix = ngood;
		xtemp = sum / ngood;
		if (mean)     *mean = xtemp;
		if (sigma)    *sigma = sqrt((sum2 / ngood) - (xtemp * xtemp));
	} else if (ngood == 1){
		if (ngoodpix) *ngoodpix = 1;
		if (mean)     *mean = sum;
		if (sigma)    *sigma = 0.0;
	} else {
		if (ngoodpix) *ngoodpix = 0;
	        if (mean)     *mean = 0.;
		if (sigma)    *sigma = 0.;
	}	    
	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise5_short
       (short *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	short nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	long *ngood,        /* number of good, non-null pixels? */
	short *minval,    /* minimum non-null value */
	short *maxval,    /* maximum non-null value */
	double *noise2,      /* returned 2nd order MAD of all non-null pixels */
	double *noise3,      /* returned 3rd order MAD of all non-null pixels */
	double *noise5,      /* returned 5th order MAD of all non-null pixels */
	int *status)        /* error status */

/*
Estimate the median and background noise in the input image using 2nd, 3rd and 5th
order Median Absolute Differences.

The noise in the background of the image is calculated using the MAD algorithms 
developed for deriving the signal to noise ratio in spectra
(see issue #42 of the ST-ECF newsletter, http://www.stecf.org/documents/newsletter/)

3rd order:  noise = 1.482602 / sqrt(6) * median (abs(2*flux(i) - flux(i-2) - flux(i+2)))

The returned estimates are the median of the values that are computed for each 
row of the image.
*/
{
	long ii, jj, nrows = 0, nrows2 = 0, nvals, nvals2, ngoodpix = 0;
	int *differences2, *differences3, *differences5;
	short *rowpix, v1, v2, v3, v4, v5, v6, v7, v8, v9;
	short xminval = SHRT_MAX, xmaxval = SHRT_MIN;
	int do_range = 0;
	double *diffs2, *diffs3, *diffs5; 
	double xnoise2 = 0, xnoise3 = 0, xnoise5 = 0;
	
	if (nx < 9) {
		/* treat entire array as an image with a single row */
		nx = nx * ny;
		ny = 1;
	}

	/* rows must have at least 9 pixels */
	if (nx < 9) {

		for (ii = 0; ii < nx; ii++) {
		    if (nullcheck && array[ii] == nullvalue)
		        continue;
		    else {
			if (array[ii] < xminval) xminval = array[ii];
			if (array[ii] > xmaxval) xmaxval = array[ii];
			ngoodpix++;
		    }
		}
		if (minval) *minval = xminval;
		if (maxval) *maxval = xmaxval;
		if (ngood) *ngood = ngoodpix;
		if (noise2) *noise2 = 0.;
		if (noise3) *noise3 = 0.;
		if (noise5) *noise5 = 0.;
		return(*status);
	}

	/* do we need to compute the min and max value? */
	if (minval || maxval) do_range = 1;
	
        /* allocate arrays used to compute the median and noise estimates */
	differences2 = calloc(nx, sizeof(int));
	if (!differences2) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}
	differences3 = calloc(nx, sizeof(int));
	if (!differences3) {
		free(differences2);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}
	differences5 = calloc(nx, sizeof(int));
	if (!differences5) {
		free(differences2);
		free(differences3);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs2 = calloc(ny, sizeof(double));
	if (!diffs2) {
		free(differences2);
		free(differences3);
		free(differences5);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs3 = calloc(ny, sizeof(double));
	if (!diffs3) {
		free(differences2);
		free(differences3);
		free(differences5);
		free(diffs2);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs5 = calloc(ny, sizeof(double));
	if (!diffs5) {
		free(differences2);
		free(differences3);
		free(differences5);
		free(diffs2);
		free(diffs3);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v1 < xminval) xminval = v1;
			if (v1 > xmaxval) xmaxval = v1;
		}

		/***** find the 2nd valid pixel in row (which we will skip over) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v2 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;
		
		if (do_range) {
			if (v2 < xminval) xminval = v2;
			if (v2 > xmaxval) xmaxval = v2;
		}

		/***** find the 3rd valid pixel in row */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v3 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v3 < xminval) xminval = v3;
			if (v3 > xmaxval) xmaxval = v3;
		}
				
		/* find the 4nd valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v4 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v4 < xminval) xminval = v4;
			if (v4 > xmaxval) xmaxval = v4;
		}
			
		/* find the 5th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v5 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v5 < xminval) xminval = v5;
			if (v5 > xmaxval) xmaxval = v5;
		}
				
		/* find the 6th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v6 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v6 < xminval) xminval = v6;
			if (v6 > xmaxval) xmaxval = v6;
		}
				
		/* find the 7th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v7 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v7 < xminval) xminval = v7;
			if (v7 > xmaxval) xmaxval = v7;
		}
				
		/* find the 8th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v8 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v8 < xminval) xminval = v8;
			if (v8 > xmaxval) xmaxval = v8;
		}
		/* now populate the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		nvals2 = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		    v9 = rowpix[ii];  /* store the good pixel value */

		    if (do_range) {
			if (v9 < xminval) xminval = v9;
			if (v9 > xmaxval) xmaxval = v9;
		    }

		    /* construct array of absolute differences */

		    if (!(v5 == v6 && v6 == v7) ) {
		        differences2[nvals2] =  abs((int) v5 - (int) v7);
			nvals2++;
		    }

		    if (!(v3 == v4 && v4 == v5 && v5 == v6 && v6 == v7) ) {
		        differences3[nvals] =  abs((2 * (int) v5) - (int) v3 - (int) v7);
		        differences5[nvals] =  abs((6 * (int) v5) - (4 * (int) v3) - (4 * (int) v7) + (int) v1 + (int) v9);
		        nvals++;  
		    } else {
		        /* ignore constant background regions */
			ngoodpix++;
		    }

		    /* shift over 1 pixel */
		    v1 = v2;
		    v2 = v3;
		    v3 = v4;
		    v4 = v5;
		    v5 = v6;
		    v6 = v7;
		    v7 = v8;
		    v8 = v9;
	        }  /* end of loop over pixels in the row */

		/* compute the median diffs */
		/* Note that there are 8 more pixel values than there are diffs values. */
		ngoodpix += nvals;

		if (nvals == 0) {
		    continue;  /* cannot compute medians on this row */
		} else if (nvals == 1) {
		    if (nvals2 == 1) {
		        diffs2[nrows2] = differences2[0];
			nrows2++;
		    }
		        
		    diffs3[nrows] = differences3[0];
		    diffs5[nrows] = differences5[0];
		} else {
                    /* quick_select returns the median MUCH faster than using qsort */
		    if (nvals2 > 1) {
                        diffs2[nrows2] = quick_select_int(differences2, nvals);
			nrows2++;
		    }

                    diffs3[nrows] = quick_select_int(differences3, nvals);
                    diffs5[nrows] = quick_select_int(differences5, nvals);
		}

		nrows++;
	}  /* end of loop over rows */

	    /* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise3 = 0;
	       xnoise5 = 0;
	} else if (nrows == 1) {
	       xnoise3 = diffs3[0];
	       xnoise5 = diffs5[0];
	} else {	    
	       qsort(diffs3, nrows, sizeof(double), FnCompare_double);
	       qsort(diffs5, nrows, sizeof(double), FnCompare_double);
	       xnoise3 =  (diffs3[(nrows - 1)/2] + diffs3[nrows/2]) / 2.;
	       xnoise5 =  (diffs5[(nrows - 1)/2] + diffs5[nrows/2]) / 2.;
	}

	if (nrows2 == 0) { 
	       xnoise2 = 0;
	} else if (nrows2 == 1) {
	       xnoise2 = diffs2[0];
	} else {	    
	       qsort(diffs2, nrows2, sizeof(double), FnCompare_double);
	       xnoise2 =  (diffs2[(nrows2 - 1)/2] + diffs2[nrows2/2]) / 2.;
	}

	if (ngood)  *ngood  = ngoodpix;
	if (minval) *minval = xminval;
	if (maxval) *maxval = xmaxval;
	if (noise2)  *noise2  = 1.0483579 * xnoise2;
	if (noise3)  *noise3  = 0.6052697 * xnoise3;
	if (noise5)  *noise5  = 0.1772048 * xnoise5;

	free(diffs5);
	free(diffs3);
	free(diffs2);
	free(differences5);
	free(differences3);
	free(differences2);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise5_int
       (int *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	int nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	long *ngood,        /* number of good, non-null pixels? */
	int *minval,    /* minimum non-null value */
	int *maxval,    /* maximum non-null value */
	double *noise2,      /* returned 2nd order MAD of all non-null pixels */
	double *noise3,      /* returned 3rd order MAD of all non-null pixels */
	double *noise5,      /* returned 5th order MAD of all non-null pixels */
	int *status)        /* error status */

/*
Estimate the median and background noise in the input image using 2nd, 3rd and 5th
order Median Absolute Differences.

The noise in the background of the image is calculated using the MAD algorithms 
developed for deriving the signal to noise ratio in spectra
(see issue #42 of the ST-ECF newsletter, http://www.stecf.org/documents/newsletter/)

3rd order:  noise = 1.482602 / sqrt(6) * median (abs(2*flux(i) - flux(i-2) - flux(i+2)))

The returned estimates are the median of the values that are computed for each 
row of the image.
*/
{
	long ii, jj, nrows = 0, nrows2 = 0, nvals, nvals2, ngoodpix = 0;
	LONGLONG *differences2, *differences3, *differences5, tdiff;
	int *rowpix, v1, v2, v3, v4, v5, v6, v7, v8, v9;
	int xminval = INT_MAX, xmaxval = INT_MIN;
	int do_range = 0;
	double *diffs2, *diffs3, *diffs5; 
	double xnoise2 = 0, xnoise3 = 0, xnoise5 = 0;
	
	if (nx < 9) {
		/* treat entire array as an image with a single row */
		nx = nx * ny;
		ny = 1;
	}

	/* rows must have at least 9 pixels */
	if (nx < 9) {

		for (ii = 0; ii < nx; ii++) {
		    if (nullcheck && array[ii] == nullvalue)
		        continue;
		    else {
			if (array[ii] < xminval) xminval = array[ii];
			if (array[ii] > xmaxval) xmaxval = array[ii];
			ngoodpix++;
		    }
		}
		if (minval) *minval = xminval;
		if (maxval) *maxval = xmaxval;
		if (ngood) *ngood = ngoodpix;
		if (noise2) *noise2 = 0.;
		if (noise3) *noise3 = 0.;
		if (noise5) *noise5 = 0.;
		return(*status);
	}

	/* do we need to compute the min and max value? */
	if (minval || maxval) do_range = 1;
	
        /* allocate arrays used to compute the median and noise estimates */
	differences2 = calloc(nx, sizeof(LONGLONG));
	if (!differences2) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}
	differences3 = calloc(nx, sizeof(LONGLONG));
	if (!differences3) {
		free(differences2);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}
	differences5 = calloc(nx, sizeof(LONGLONG));
	if (!differences5) {
		free(differences2);
		free(differences3);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs2 = calloc(ny, sizeof(double));
	if (!diffs2) {
		free(differences2);
		free(differences3);
		free(differences5);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs3 = calloc(ny, sizeof(double));
	if (!diffs3) {
		free(differences2);
		free(differences3);
		free(differences5);
		free(diffs2);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs5 = calloc(ny, sizeof(double));
	if (!diffs5) {
		free(differences2);
		free(differences3);
		free(differences5);
		free(diffs2);
		free(diffs3);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v1 < xminval) xminval = v1;
			if (v1 > xmaxval) xmaxval = v1;
		}

		/***** find the 2nd valid pixel in row (which we will skip over) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v2 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;
		
		if (do_range) {
			if (v2 < xminval) xminval = v2;
			if (v2 > xmaxval) xmaxval = v2;
		}

		/***** find the 3rd valid pixel in row */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v3 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v3 < xminval) xminval = v3;
			if (v3 > xmaxval) xmaxval = v3;
		}
				
		/* find the 4nd valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v4 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v4 < xminval) xminval = v4;
			if (v4 > xmaxval) xmaxval = v4;
		}
			
		/* find the 5th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v5 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v5 < xminval) xminval = v5;
			if (v5 > xmaxval) xmaxval = v5;
		}
				
		/* find the 6th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v6 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v6 < xminval) xminval = v6;
			if (v6 > xmaxval) xmaxval = v6;
		}
				
		/* find the 7th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v7 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v7 < xminval) xminval = v7;
			if (v7 > xmaxval) xmaxval = v7;
		}
				
		/* find the 8th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v8 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v8 < xminval) xminval = v8;
			if (v8 > xmaxval) xmaxval = v8;
		}
		/* now populate the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		nvals2 = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		    v9 = rowpix[ii];  /* store the good pixel value */

		    if (do_range) {
			if (v9 < xminval) xminval = v9;
			if (v9 > xmaxval) xmaxval = v9;
		    }

		    /* construct array of absolute differences */

		    if (!(v5 == v6 && v6 == v7) ) {
		        tdiff =  (LONGLONG) v5 - (LONGLONG) v7;
			if (tdiff < 0)
		            differences2[nvals2] =  -1 * tdiff;
			else
		            differences2[nvals2] =  tdiff;

			nvals2++;
		    }

		    if (!(v3 == v4 && v4 == v5 && v5 == v6 && v6 == v7) ) {
		        tdiff =  (2 * (LONGLONG) v5) - (LONGLONG) v3 - (LONGLONG) v7;
			if (tdiff < 0)
		            differences3[nvals] =  -1 * tdiff;
			else
		            differences3[nvals] =  tdiff;

		        tdiff =  (6 * (LONGLONG) v5) - (4 * (LONGLONG) v3) - (4 * (LONGLONG) v7) + (LONGLONG) v1 + (LONGLONG) v9;
			if (tdiff < 0)
		            differences5[nvals] =  -1 * tdiff;
			else
		            differences5[nvals] =  tdiff;

		        nvals++;  
		    } else {
		        /* ignore constant background regions */
			ngoodpix++;
		    }

		    /* shift over 1 pixel */
		    v1 = v2;
		    v2 = v3;
		    v3 = v4;
		    v4 = v5;
		    v5 = v6;
		    v6 = v7;
		    v7 = v8;
		    v8 = v9;
	        }  /* end of loop over pixels in the row */

		/* compute the median diffs */
		/* Note that there are 8 more pixel values than there are diffs values. */
		ngoodpix += nvals;

		if (nvals == 0) {
		    continue;  /* cannot compute medians on this row */
		} else if (nvals == 1) {
		    if (nvals2 == 1) {
		        diffs2[nrows2] = (double) differences2[0];
			nrows2++;
		    }
		        
		    diffs3[nrows] = (double) differences3[0];
		    diffs5[nrows] = (double) differences5[0];
		} else {
                    /* quick_select returns the median MUCH faster than using qsort */
		    if (nvals2 > 1) {
                        diffs2[nrows2] = (double) quick_select_longlong(differences2, nvals);
			nrows2++;
		    }

                    diffs3[nrows] = (double) quick_select_longlong(differences3, nvals);
                    diffs5[nrows] = (double) quick_select_longlong(differences5, nvals);
		}

		nrows++;
	}  /* end of loop over rows */

	    /* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise3 = 0;
	       xnoise5 = 0;
	} else if (nrows == 1) {
	       xnoise3 = diffs3[0];
	       xnoise5 = diffs5[0];
	} else {	    
	       qsort(diffs3, nrows, sizeof(double), FnCompare_double);
	       qsort(diffs5, nrows, sizeof(double), FnCompare_double);
	       xnoise3 =  (diffs3[(nrows - 1)/2] + diffs3[nrows/2]) / 2.;
	       xnoise5 =  (diffs5[(nrows - 1)/2] + diffs5[nrows/2]) / 2.;
	}

	if (nrows2 == 0) { 
	       xnoise2 = 0;
	} else if (nrows2 == 1) {
	       xnoise2 = diffs2[0];
	} else {	    
	       qsort(diffs2, nrows2, sizeof(double), FnCompare_double);
	       xnoise2 =  (diffs2[(nrows2 - 1)/2] + diffs2[nrows2/2]) / 2.;
	}

	if (ngood)  *ngood  = ngoodpix;
	if (minval) *minval = xminval;
	if (maxval) *maxval = xmaxval;
	if (noise2)  *noise2  = 1.0483579 * xnoise2;
	if (noise3)  *noise3  = 0.6052697 * xnoise3;
	if (noise5)  *noise5  = 0.1772048 * xnoise5;

	free(diffs5);
	free(diffs3);
	free(diffs2);
	free(differences5);
	free(differences3);
	free(differences2);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise5_float
       (float *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	float nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	long *ngood,        /* number of good, non-null pixels? */
	float *minval,    /* minimum non-null value */
	float *maxval,    /* maximum non-null value */
	double *noise2,      /* returned 2nd order MAD of all non-null pixels */
	double *noise3,      /* returned 3rd order MAD of all non-null pixels */
	double *noise5,      /* returned 5th order MAD of all non-null pixels */
	int *status)        /* error status */

/*
Estimate the median and background noise in the input image using 2nd, 3rd and 5th
order Median Absolute Differences.

The noise in the background of the image is calculated using the MAD algorithms 
developed for deriving the signal to noise ratio in spectra
(see issue #42 of the ST-ECF newsletter, http://www.stecf.org/documents/newsletter/)

3rd order:  noise = 1.482602 / sqrt(6) * median (abs(2*flux(i) - flux(i-2) - flux(i+2)))

The returned estimates are the median of the values that are computed for each 
row of the image.
*/
{
	long ii, jj, nrows = 0, nrows2 = 0, nvals, nvals2, ngoodpix = 0;
	float *differences2, *differences3, *differences5;
	float *rowpix, v1, v2, v3, v4, v5, v6, v7, v8, v9;
	float xminval = FLT_MAX, xmaxval = -FLT_MAX;
	int do_range = 0;
	double *diffs2, *diffs3, *diffs5; 
	double xnoise2 = 0, xnoise3 = 0, xnoise5 = 0;
	
	if (nx < 9) {
		/* treat entire array as an image with a single row */
		nx = nx * ny;
		ny = 1;
	}

	/* rows must have at least 9 pixels */
	if (nx < 9) {

		for (ii = 0; ii < nx; ii++) {
		    if (nullcheck && array[ii] == nullvalue)
		        continue;
		    else {
			if (array[ii] < xminval) xminval = array[ii];
			if (array[ii] > xmaxval) xmaxval = array[ii];
			ngoodpix++;
		    }
		}
		if (minval) *minval = xminval;
		if (maxval) *maxval = xmaxval;
		if (ngood) *ngood = ngoodpix;
		if (noise2) *noise2 = 0.;
		if (noise3) *noise3 = 0.;
		if (noise5) *noise5 = 0.;
		return(*status);
	}

	/* do we need to compute the min and max value? */
	if (minval || maxval) do_range = 1;
	
        /* allocate arrays used to compute the median and noise estimates */
	differences2 = calloc(nx, sizeof(float));
	if (!differences2) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}
	differences3 = calloc(nx, sizeof(float));
	if (!differences3) {
		free(differences2);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}
	differences5 = calloc(nx, sizeof(float));
	if (!differences5) {
		free(differences2);
		free(differences3);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs2 = calloc(ny, sizeof(double));
	if (!diffs2) {
		free(differences2);
		free(differences3);
		free(differences5);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs3 = calloc(ny, sizeof(double));
	if (!diffs3) {
		free(differences2);
		free(differences3);
		free(differences5);
		free(diffs2);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs5 = calloc(ny, sizeof(double));
	if (!diffs5) {
		free(differences2);
		free(differences3);
		free(differences5);
		free(diffs2);
		free(diffs3);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v1 < xminval) xminval = v1;
			if (v1 > xmaxval) xmaxval = v1;
		}

		/***** find the 2nd valid pixel in row (which we will skip over) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v2 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;
		
		if (do_range) {
			if (v2 < xminval) xminval = v2;
			if (v2 > xmaxval) xmaxval = v2;
		}

		/***** find the 3rd valid pixel in row */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v3 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v3 < xminval) xminval = v3;
			if (v3 > xmaxval) xmaxval = v3;
		}
				
		/* find the 4nd valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v4 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v4 < xminval) xminval = v4;
			if (v4 > xmaxval) xmaxval = v4;
		}
			
		/* find the 5th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v5 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v5 < xminval) xminval = v5;
			if (v5 > xmaxval) xmaxval = v5;
		}
				
		/* find the 6th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v6 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v6 < xminval) xminval = v6;
			if (v6 > xmaxval) xmaxval = v6;
		}
				
		/* find the 7th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v7 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v7 < xminval) xminval = v7;
			if (v7 > xmaxval) xmaxval = v7;
		}
				
		/* find the 8th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v8 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v8 < xminval) xminval = v8;
			if (v8 > xmaxval) xmaxval = v8;
		}
		/* now populate the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		nvals2 = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		    v9 = rowpix[ii];  /* store the good pixel value */

		    if (do_range) {
			if (v9 < xminval) xminval = v9;
			if (v9 > xmaxval) xmaxval = v9;
		    }

		    /* construct array of absolute differences */

		    if (!(v5 == v6 && v6 == v7) ) {
		        differences2[nvals2] = (float) fabs(v5 - v7);
			nvals2++;
		    }

		    if (!(v3 == v4 && v4 == v5 && v5 == v6 && v6 == v7) ) {
		        differences3[nvals] = (float) fabs((2 * v5) - v3 - v7);
		        differences5[nvals] = (float) fabs((6 * v5) - (4 * v3) - (4 * v7) + v1 + v9);
		        nvals++;  
		    } else {
		        /* ignore constant background regions */
			ngoodpix++;
		    }

		    /* shift over 1 pixel */
		    v1 = v2;
		    v2 = v3;
		    v3 = v4;
		    v4 = v5;
		    v5 = v6;
		    v6 = v7;
		    v7 = v8;
		    v8 = v9;
	        }  /* end of loop over pixels in the row */

		/* compute the median diffs */
		/* Note that there are 8 more pixel values than there are diffs values. */
		ngoodpix += nvals;

		if (nvals == 0) {
		    continue;  /* cannot compute medians on this row */
		} else if (nvals == 1) {
		    if (nvals2 == 1) {
		        diffs2[nrows2] = differences2[0];
			nrows2++;
		    }
		        
		    diffs3[nrows] = differences3[0];
		    diffs5[nrows] = differences5[0];
		} else {
                    /* quick_select returns the median MUCH faster than using qsort */
		    if (nvals2 > 1) {
                        diffs2[nrows2] = quick_select_float(differences2, nvals);
			nrows2++;
		    }

                    diffs3[nrows] = quick_select_float(differences3, nvals);
                    diffs5[nrows] = quick_select_float(differences5, nvals);
		}

		nrows++;
	}  /* end of loop over rows */

	    /* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise3 = 0;
	       xnoise5 = 0;
	} else if (nrows == 1) {
	       xnoise3 = diffs3[0];
	       xnoise5 = diffs5[0];
	} else {	    
	       qsort(diffs3, nrows, sizeof(double), FnCompare_double);
	       qsort(diffs5, nrows, sizeof(double), FnCompare_double);
	       xnoise3 =  (diffs3[(nrows - 1)/2] + diffs3[nrows/2]) / 2.;
	       xnoise5 =  (diffs5[(nrows - 1)/2] + diffs5[nrows/2]) / 2.;
	}

	if (nrows2 == 0) { 
	       xnoise2 = 0;
	} else if (nrows2 == 1) {
	       xnoise2 = diffs2[0];
	} else {	    
	       qsort(diffs2, nrows2, sizeof(double), FnCompare_double);
	       xnoise2 =  (diffs2[(nrows2 - 1)/2] + diffs2[nrows2/2]) / 2.;
	}

	if (ngood)  *ngood  = ngoodpix;
	if (minval) *minval = xminval;
	if (maxval) *maxval = xmaxval;
	if (noise2)  *noise2  = 1.0483579 * xnoise2;
	if (noise3)  *noise3  = 0.6052697 * xnoise3;
	if (noise5)  *noise5  = 0.1772048 * xnoise5;

	free(diffs5);
	free(diffs3);
	free(diffs2);
	free(differences5);
	free(differences3);
	free(differences2);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise5_double
       (double *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	double nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	long *ngood,        /* number of good, non-null pixels? */
	double *minval,    /* minimum non-null value */
	double *maxval,    /* maximum non-null value */
	double *noise2,      /* returned 2nd order MAD of all non-null pixels */
	double *noise3,      /* returned 3rd order MAD of all non-null pixels */
	double *noise5,      /* returned 5th order MAD of all non-null pixels */
	int *status)        /* error status */

/*
Estimate the median and background noise in the input image using 2nd, 3rd and 5th
order Median Absolute Differences.

The noise in the background of the image is calculated using the MAD algorithms 
developed for deriving the signal to noise ratio in spectra
(see issue #42 of the ST-ECF newsletter, http://www.stecf.org/documents/newsletter/)

3rd order:  noise = 1.482602 / sqrt(6) * median (abs(2*flux(i) - flux(i-2) - flux(i+2)))

The returned estimates are the median of the values that are computed for each 
row of the image.
*/
{
	long ii, jj, nrows = 0, nrows2 = 0, nvals, nvals2, ngoodpix = 0;
	double *differences2, *differences3, *differences5;
	double *rowpix, v1, v2, v3, v4, v5, v6, v7, v8, v9;
	double xminval = DBL_MAX, xmaxval = -DBL_MAX;
	int do_range = 0;
	double *diffs2, *diffs3, *diffs5; 
	double xnoise2 = 0, xnoise3 = 0, xnoise5 = 0;
	
	if (nx < 9) {
		/* treat entire array as an image with a single row */
		nx = nx * ny;
		ny = 1;
	}

	/* rows must have at least 9 pixels */
	if (nx < 9) {

		for (ii = 0; ii < nx; ii++) {
		    if (nullcheck && array[ii] == nullvalue)
		        continue;
		    else {
			if (array[ii] < xminval) xminval = array[ii];
			if (array[ii] > xmaxval) xmaxval = array[ii];
			ngoodpix++;
		    }
		}
		if (minval) *minval = xminval;
		if (maxval) *maxval = xmaxval;
		if (ngood) *ngood = ngoodpix;
		if (noise2) *noise2 = 0.;
		if (noise3) *noise3 = 0.;
		if (noise5) *noise5 = 0.;
		return(*status);
	}

	/* do we need to compute the min and max value? */
	if (minval || maxval) do_range = 1;
	
        /* allocate arrays used to compute the median and noise estimates */
	differences2 = calloc(nx, sizeof(double));
	if (!differences2) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}
	differences3 = calloc(nx, sizeof(double));
	if (!differences3) {
		free(differences2);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}
	differences5 = calloc(nx, sizeof(double));
	if (!differences5) {
		free(differences2);
		free(differences3);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs2 = calloc(ny, sizeof(double));
	if (!diffs2) {
		free(differences2);
		free(differences3);
		free(differences5);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs3 = calloc(ny, sizeof(double));
	if (!diffs3) {
		free(differences2);
		free(differences3);
		free(differences5);
		free(diffs2);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs5 = calloc(ny, sizeof(double));
	if (!diffs5) {
		free(differences2);
		free(differences3);
		free(differences5);
		free(diffs2);
		free(diffs3);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v1 < xminval) xminval = v1;
			if (v1 > xmaxval) xmaxval = v1;
		}

		/***** find the 2nd valid pixel in row (which we will skip over) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v2 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;
		
		if (do_range) {
			if (v2 < xminval) xminval = v2;
			if (v2 > xmaxval) xmaxval = v2;
		}

		/***** find the 3rd valid pixel in row */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v3 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v3 < xminval) xminval = v3;
			if (v3 > xmaxval) xmaxval = v3;
		}
				
		/* find the 4nd valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v4 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v4 < xminval) xminval = v4;
			if (v4 > xmaxval) xmaxval = v4;
		}
			
		/* find the 5th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v5 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v5 < xminval) xminval = v5;
			if (v5 > xmaxval) xmaxval = v5;
		}
				
		/* find the 6th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v6 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v6 < xminval) xminval = v6;
			if (v6 > xmaxval) xmaxval = v6;
		}
				
		/* find the 7th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v7 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v7 < xminval) xminval = v7;
			if (v7 > xmaxval) xmaxval = v7;
		}
				
		/* find the 8th valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v8 = rowpix[ii];  /* store the good pixel value */
		ngoodpix++;

		if (do_range) {
			if (v8 < xminval) xminval = v8;
			if (v8 > xmaxval) xmaxval = v8;
		}
		/* now populate the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		nvals2 = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		    v9 = rowpix[ii];  /* store the good pixel value */

		    if (do_range) {
			if (v9 < xminval) xminval = v9;
			if (v9 > xmaxval) xmaxval = v9;
		    }

		    /* construct array of absolute differences */

		    if (!(v5 == v6 && v6 == v7) ) {
		        differences2[nvals2] =  fabs(v5 - v7);
			nvals2++;
		    }

		    if (!(v3 == v4 && v4 == v5 && v5 == v6 && v6 == v7) ) {
		        differences3[nvals] =  fabs((2 * v5) - v3 - v7);
		        differences5[nvals] =  fabs((6 * v5) - (4 * v3) - (4 * v7) + v1 + v9);
		        nvals++;  
		    } else {
		        /* ignore constant background regions */
			ngoodpix++;
		    }

		    /* shift over 1 pixel */
		    v1 = v2;
		    v2 = v3;
		    v3 = v4;
		    v4 = v5;
		    v5 = v6;
		    v6 = v7;
		    v7 = v8;
		    v8 = v9;
	        }  /* end of loop over pixels in the row */

		/* compute the median diffs */
		/* Note that there are 8 more pixel values than there are diffs values. */
		ngoodpix += nvals;

		if (nvals == 0) {
		    continue;  /* cannot compute medians on this row */
		} else if (nvals == 1) {
		    if (nvals2 == 1) {
		        diffs2[nrows2] = differences2[0];
			nrows2++;
		    }
		        
		    diffs3[nrows] = differences3[0];
		    diffs5[nrows] = differences5[0];
		} else {
                    /* quick_select returns the median MUCH faster than using qsort */
		    if (nvals2 > 1) {
                        diffs2[nrows2] = quick_select_double(differences2, nvals);
			nrows2++;
		    }

                    diffs3[nrows] = quick_select_double(differences3, nvals);
                    diffs5[nrows] = quick_select_double(differences5, nvals);
		}

		nrows++;
	}  /* end of loop over rows */

	    /* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise3 = 0;
	       xnoise5 = 0;
	} else if (nrows == 1) {
	       xnoise3 = diffs3[0];
	       xnoise5 = diffs5[0];
	} else {	    
	       qsort(diffs3, nrows, sizeof(double), FnCompare_double);
	       qsort(diffs5, nrows, sizeof(double), FnCompare_double);
	       xnoise3 =  (diffs3[(nrows - 1)/2] + diffs3[nrows/2]) / 2.;
	       xnoise5 =  (diffs5[(nrows - 1)/2] + diffs5[nrows/2]) / 2.;
	}

	if (nrows2 == 0) { 
	       xnoise2 = 0;
	} else if (nrows2 == 1) {
	       xnoise2 = diffs2[0];
	} else {	    
	       qsort(diffs2, nrows2, sizeof(double), FnCompare_double);
	       xnoise2 =  (diffs2[(nrows2 - 1)/2] + diffs2[nrows2/2]) / 2.;
	}

	if (ngood)  *ngood  = ngoodpix;
	if (minval) *minval = xminval;
	if (maxval) *maxval = xmaxval;
	if (noise2)  *noise2  = 1.0483579 * xnoise2;
	if (noise3)  *noise3  = 0.6052697 * xnoise3;
	if (noise5)  *noise5  = 0.1772048 * xnoise5;

	free(diffs5);
	free(diffs3);
	free(diffs2);
	free(differences5);
	free(differences3);
	free(differences2);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise3_short
       (short *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	short nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	long *ngood,        /* number of good, non-null pixels? */
	short *minval,    /* minimum non-null value */
	short *maxval,    /* maximum non-null value */
	double *noise,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */

/*
Estimate the median and background noise in the input image using 3rd order differences.

The noise in the background of the image is calculated using the 3rd order algorithm 
developed for deriving the signal to noise ratio in spectra
(see issue #42 of the ST-ECF newsletter, http://www.stecf.org/documents/newsletter/)

  noise = 1.482602 / sqrt(6) * median (abs(2*flux(i) - flux(i-2) - flux(i+2)))

The returned estimates are the median of the values that are computed for each 
row of the image.
*/
{
	long ii, jj, nrows = 0, nvals, ngoodpix = 0;
	short *differences, *rowpix, v1, v2, v3, v4, v5;
	short xminval = SHRT_MAX, xmaxval = SHRT_MIN, do_range = 0;
	double *diffs, xnoise = 0, sigma;

	if (nx < 5) {
		/* treat entire array as an image with a single row */
		nx = nx * ny;
		ny = 1;
	}

	/* rows must have at least 5 pixels */
	if (nx < 5) {

		for (ii = 0; ii < nx; ii++) {
		    if (nullcheck && array[ii] == nullvalue)
		        continue;
		    else {
			if (array[ii] < xminval) xminval = array[ii];
			if (array[ii] > xmaxval) xmaxval = array[ii];
			ngoodpix++;
		    }
		}
		if (minval) *minval = xminval;
		if (maxval) *maxval = xmaxval;
		if (ngood) *ngood = ngoodpix;
		if (noise) *noise = 0.;
		return(*status);
	}

	/* do we need to compute the min and max value? */
	if (minval || maxval) do_range = 1;
	
        /* allocate arrays used to compute the median and noise estimates */
	differences = calloc(nx, sizeof(short));
	if (!differences) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs = calloc(ny, sizeof(double));
	if (!diffs) {
		free(differences);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v1 < xminval) xminval = v1;
			if (v1 > xmaxval) xmaxval = v1;
		}

		/***** find the 2nd valid pixel in row (which we will skip over) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v2 = rowpix[ii];  /* store the good pixel value */
		
		if (do_range) {
			if (v2 < xminval) xminval = v2;
			if (v2 > xmaxval) xmaxval = v2;
		}

		/***** find the 3rd valid pixel in row */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v3 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v3 < xminval) xminval = v3;
			if (v3 > xmaxval) xmaxval = v3;
		}
				
		/* find the 4nd valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v4 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v4 < xminval) xminval = v4;
			if (v4 > xmaxval) xmaxval = v4;
		}
		
		/* now populate the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		    v5 = rowpix[ii];  /* store the good pixel value */

		    if (do_range) {
			if (v5 < xminval) xminval = v5;
			if (v5 > xmaxval) xmaxval = v5;
		    }

		    /* construct array of 3rd order absolute differences */
		    if (!(v1 == v2 && v2 == v3 && v3 == v4 && v4 == v5)) {
		        differences[nvals] = abs((2 * v3) - v1 - v5);
		        nvals++;  
		    } else {
		        /* ignore constant background regions */
			ngoodpix++;
		    }


		    /* shift over 1 pixel */
		    v1 = v2;
		    v2 = v3;
		    v3 = v4;
		    v4 = v5;
	        }  /* end of loop over pixels in the row */

		/* compute the 3rd order diffs */
		/* Note that there are 4 more pixel values than there are diffs values. */
		ngoodpix += (nvals + 4);

		if (nvals == 0) {
		    continue;  /* cannot compute medians on this row */
		} else if (nvals == 1) {
		    diffs[nrows] = differences[0];
		} else {
                    /* quick_select returns the median MUCH faster than using qsort */
                    diffs[nrows] = quick_select_short(differences, nvals);
		}

		nrows++;
	}  /* end of loop over rows */

	    /* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise = 0;
	} else if (nrows == 1) {
	       xnoise = diffs[0];
	} else {	    


	       qsort(diffs, nrows, sizeof(double), FnCompare_double);
	       xnoise =  (diffs[(nrows - 1)/2] + diffs[nrows/2]) / 2.;

              FnMeanSigma_double(diffs, nrows, 0, 0.0, 0, &xnoise, &sigma, status); 

	      /* do a 4.5 sigma rejection of outliers */
	      jj = 0;
	      sigma = 4.5 * sigma;
	      for (ii = 0; ii < nrows; ii++) {
		if ( fabs(diffs[ii] - xnoise) <= sigma)	 {
		   if (jj != ii)
		       diffs[jj] = diffs[ii];
		   jj++;
	        } 
	      }
	      if (ii != jj)
                FnMeanSigma_double(diffs, jj, 0, 0.0, 0, &xnoise, &sigma, status); 
	}

	if (ngood)  *ngood  = ngoodpix;
	if (minval) *minval = xminval;
	if (maxval) *maxval = xmaxval;
	if (noise)  *noise  = 0.6052697 * xnoise;

	free(diffs);
	free(differences);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise3_int
       (int *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	int nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	long *ngood,        /* number of good, non-null pixels? */
	int *minval,    /* minimum non-null value */
	int *maxval,    /* maximum non-null value */
	double *noise,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */

/*
Estimate the background noise in the input image using 3rd order differences.

The noise in the background of the image is calculated using the 3rd order algorithm 
developed for deriving the signal to noise ratio in spectra
(see issue #42 of the ST-ECF newsletter, http://www.stecf.org/documents/newsletter/)

  noise = 1.482602 / sqrt(6) * median (abs(2*flux(i) - flux(i-2) - flux(i+2)))

The returned estimates are the median of the values that are computed for each 
row of the image.
*/
{
	long ii, jj, nrows = 0, nvals, ngoodpix = 0;
	int *differences, *rowpix, v1, v2, v3, v4, v5;
	int xminval = INT_MAX, xmaxval = INT_MIN, do_range = 0;
	double *diffs, xnoise = 0, sigma;
	
	if (nx < 5) {
		/* treat entire array as an image with a single row */
		nx = nx * ny;
		ny = 1;
	}

	/* rows must have at least 5 pixels */
	if (nx < 5) {

		for (ii = 0; ii < nx; ii++) {
		    if (nullcheck && array[ii] == nullvalue)
		        continue;
		    else {
			if (array[ii] < xminval) xminval = array[ii];
			if (array[ii] > xmaxval) xmaxval = array[ii];
			ngoodpix++;
		    }
		}
		if (minval) *minval = xminval;
		if (maxval) *maxval = xmaxval;
		if (ngood) *ngood = ngoodpix;
		if (noise) *noise = 0.;
		return(*status);
	}

	/* do we need to compute the min and max value? */
	if (minval || maxval) do_range = 1;
	
        /* allocate arrays used to compute the median and noise estimates */
	differences = calloc(nx, sizeof(int));
	if (!differences) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs = calloc(ny, sizeof(double));
	if (!diffs) {
		free(differences);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v1 < xminval) xminval = v1;
			if (v1 > xmaxval) xmaxval = v1;
		}

		/***** find the 2nd valid pixel in row (which we will skip over) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v2 = rowpix[ii];  /* store the good pixel value */
		
		if (do_range) {
			if (v2 < xminval) xminval = v2;
			if (v2 > xmaxval) xmaxval = v2;
		}

		/***** find the 3rd valid pixel in row */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v3 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v3 < xminval) xminval = v3;
			if (v3 > xmaxval) xmaxval = v3;
		}
				
		/* find the 4nd valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v4 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v4 < xminval) xminval = v4;
			if (v4 > xmaxval) xmaxval = v4;
		}
		
		/* now populate the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		    v5 = rowpix[ii];  /* store the good pixel value */

		    if (do_range) {
			if (v5 < xminval) xminval = v5;
			if (v5 > xmaxval) xmaxval = v5;
		    }

		    /* construct array of 3rd order absolute differences */
		    if (!(v1 == v2 && v2 == v3 && v3 == v4 && v4 == v5)) {
		        differences[nvals] = abs((2 * v3) - v1 - v5);
		        nvals++;  
		    } else {
		        /* ignore constant background regions */
			ngoodpix++;
		    }

		    /* shift over 1 pixel */
		    v1 = v2;
		    v2 = v3;
		    v3 = v4;
		    v4 = v5;
	        }  /* end of loop over pixels in the row */

		/* compute the 3rd order diffs */
		/* Note that there are 4 more pixel values than there are diffs values. */
		ngoodpix += (nvals + 4);

		if (nvals == 0) {
		    continue;  /* cannot compute medians on this row */
		} else if (nvals == 1) {
		    diffs[nrows] = differences[0];
		} else {
                    /* quick_select returns the median MUCH faster than using qsort */
                    diffs[nrows] = quick_select_int(differences, nvals);
		}

		nrows++;
	}  /* end of loop over rows */

	    /* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise = 0;
	} else if (nrows == 1) {
	       xnoise = diffs[0];
	} else {	    

	       qsort(diffs, nrows, sizeof(double), FnCompare_double);
	       xnoise =  (diffs[(nrows - 1)/2] + diffs[nrows/2]) / 2.;

              FnMeanSigma_double(diffs, nrows, 0, 0.0, 0, &xnoise, &sigma, status); 

	      /* do a 4.5 sigma rejection of outliers */
	      jj = 0;
	      sigma = 4.5 * sigma;
	      for (ii = 0; ii < nrows; ii++) {
		if ( fabs(diffs[ii] - xnoise) <= sigma)	 {
		   if (jj != ii)
		       diffs[jj] = diffs[ii];
		   jj++;
	        }
	      }
	      if (ii != jj)
                FnMeanSigma_double(diffs, jj, 0, 0.0, 0, &xnoise, &sigma, status); 
	}

	if (ngood)  *ngood  = ngoodpix;
	if (minval) *minval = xminval;
	if (maxval) *maxval = xmaxval;
	if (noise)  *noise  = 0.6052697 * xnoise;

	free(diffs);
	free(differences);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise3_float
       (float *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	float nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	long *ngood,        /* number of good, non-null pixels? */
	float *minval,    /* minimum non-null value */
	float *maxval,    /* maximum non-null value */
	double *noise,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */

/*
Estimate the median and background noise in the input image using 3rd order differences.

The noise in the background of the image is calculated using the 3rd order algorithm 
developed for deriving the signal to noise ratio in spectra
(see issue #42 of the ST-ECF newsletter, http://www.stecf.org/documents/newsletter/)

  noise = 1.482602 / sqrt(6) * median (abs(2*flux(i) - flux(i-2) - flux(i+2)))

The returned estimates are the median of the values that are computed for each 
row of the image.
*/
{
	long ii, jj, nrows = 0, nvals, ngoodpix = 0;
	float *differences, *rowpix, v1, v2, v3, v4, v5;
	float xminval = FLT_MAX, xmaxval = -FLT_MAX;
	int do_range = 0;
	double *diffs, xnoise = 0;

	if (nx < 5) {
		/* treat entire array as an image with a single row */
		nx = nx * ny;
		ny = 1;
	}

	/* rows must have at least 5 pixels to calc noise, so just calc min, max, ngood */
	if (nx < 5) {

		for (ii = 0; ii < nx; ii++) {
		    if (nullcheck && array[ii] == nullvalue)
		        continue;
		    else {
			if (array[ii] < xminval) xminval = array[ii];
			if (array[ii] > xmaxval) xmaxval = array[ii];
			ngoodpix++;
		    }
		}
		if (minval) *minval = xminval;
		if (maxval) *maxval = xmaxval;
		if (ngood) *ngood = ngoodpix;
		if (noise) *noise = 0.;
		return(*status);
	}

	/* do we need to compute the min and max value? */
	if (minval || maxval) do_range = 1;
	
        /* allocate arrays used to compute the median and noise estimates */
	if (noise) {
	    differences = calloc(nx, sizeof(float));
	    if (!differences) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	    }

	    diffs = calloc(ny, sizeof(double));
	    if (!diffs) {
		free(differences);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	    }
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v1 < xminval) xminval = v1;
			if (v1 > xmaxval) xmaxval = v1;
		}

		/***** find the 2nd valid pixel in row (which we will skip over) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v2 = rowpix[ii];  /* store the good pixel value */
		
		if (do_range) {
			if (v2 < xminval) xminval = v2;
			if (v2 > xmaxval) xmaxval = v2;
		}

		/***** find the 3rd valid pixel in row */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v3 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v3 < xminval) xminval = v3;
			if (v3 > xmaxval) xmaxval = v3;
		}
				
		/* find the 4nd valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v4 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v4 < xminval) xminval = v4;
			if (v4 > xmaxval) xmaxval = v4;
		}
		
		/* now populate the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) {
			  ii++;
		        }
			
		    if (ii == nx) break;  /* hit end of row */
		    v5 = rowpix[ii];  /* store the good pixel value */

		    if (do_range) {
			if (v5 < xminval) xminval = v5;
			if (v5 > xmaxval) xmaxval = v5;
		    }

		    /* construct array of 3rd order absolute differences */
		    if (noise) {
		        if (!(v1 == v2 && v2 == v3 && v3 == v4 && v4 == v5)) {

		            differences[nvals] = (float) fabs((2. * v3) - v1 - v5);
		            nvals++;  
		       } else {
		            /* ignore constant background regions */
			    ngoodpix++;
		       }
		    } else {
		       /* just increment the number of non-null pixels */
		       ngoodpix++;
		    }

		    /* shift over 1 pixel */
		    v1 = v2;
		    v2 = v3;
		    v3 = v4;
		    v4 = v5;
	        }  /* end of loop over pixels in the row */

		/* compute the 3rd order diffs */
		/* Note that there are 4 more pixel values than there are diffs values. */
		ngoodpix += (nvals + 4);

		if (noise) {
		    if (nvals == 0) {
		        continue;  /* cannot compute medians on this row */
		    } else if (nvals == 1) {
		        diffs[nrows] = differences[0];
		    } else {
                        /* quick_select returns the median MUCH faster than using qsort */
                        diffs[nrows] = quick_select_float(differences, nvals);
		    }
		}
		nrows++;
	}  /* end of loop over rows */

	    /* compute median of the values for each row */
	if (noise) {
	    if (nrows == 0) { 
	       xnoise = 0;
	    } else if (nrows == 1) {
	       xnoise = diffs[0];
	    } else {	    
	       qsort(diffs, nrows, sizeof(double), FnCompare_double);
	       xnoise =  (diffs[(nrows - 1)/2] + diffs[nrows/2]) / 2.;
	    }
	}

	if (ngood)  *ngood  = ngoodpix;
	if (minval) *minval = xminval;
	if (maxval) *maxval = xmaxval;
	if (noise) {
		*noise  = 0.6052697 * xnoise;
		free(diffs);
		free(differences);
	}

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise3_double
       (double *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	double nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	long *ngood,        /* number of good, non-null pixels? */
	double *minval,    /* minimum non-null value */
	double *maxval,    /* maximum non-null value */
	double *noise,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */

/*
Estimate the median and background noise in the input image using 3rd order differences.

The noise in the background of the image is calculated using the 3rd order algorithm 
developed for deriving the signal to noise ratio in spectra
(see issue #42 of the ST-ECF newsletter, http://www.stecf.org/documents/newsletter/)

  noise = 1.482602 / sqrt(6) * median (abs(2*flux(i) - flux(i-2) - flux(i+2)))

The returned estimates are the median of the values that are computed for each 
row of the image.
*/
{
	long ii, jj, nrows = 0, nvals, ngoodpix = 0;
	double *differences, *rowpix, v1, v2, v3, v4, v5;
	double xminval = DBL_MAX, xmaxval = -DBL_MAX;
	int do_range = 0;
	double *diffs, xnoise = 0;
	
	if (nx < 5) {
		/* treat entire array as an image with a single row */
		nx = nx * ny;
		ny = 1;
	}

	/* rows must have at least 5 pixels */
	if (nx < 5) {

		for (ii = 0; ii < nx; ii++) {
		    if (nullcheck && array[ii] == nullvalue)
		        continue;
		    else {
			if (array[ii] < xminval) xminval = array[ii];
			if (array[ii] > xmaxval) xmaxval = array[ii];
			ngoodpix++;
		    }
		}
		if (minval) *minval = xminval;
		if (maxval) *maxval = xmaxval;
		if (ngood) *ngood = ngoodpix;
		if (noise) *noise = 0.;
		return(*status);
	}

	/* do we need to compute the min and max value? */
	if (minval || maxval) do_range = 1;
	
        /* allocate arrays used to compute the median and noise estimates */
	if (noise) {
	    differences = calloc(nx, sizeof(double));
	    if (!differences) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	    }

	    diffs = calloc(ny, sizeof(double));
	    if (!diffs) {
		free(differences);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	    }
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v1 < xminval) xminval = v1;
			if (v1 > xmaxval) xmaxval = v1;
		}

		/***** find the 2nd valid pixel in row (which we will skip over) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v2 = rowpix[ii];  /* store the good pixel value */
		
		if (do_range) {
			if (v2 < xminval) xminval = v2;
			if (v2 > xmaxval) xmaxval = v2;
		}

		/***** find the 3rd valid pixel in row */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v3 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v3 < xminval) xminval = v3;
			if (v3 > xmaxval) xmaxval = v3;
		}
				
		/* find the 4nd valid pixel in row (to be skipped) */
		ii++;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v4 = rowpix[ii];  /* store the good pixel value */

		if (do_range) {
			if (v4 < xminval) xminval = v4;
			if (v4 > xmaxval) xmaxval = v4;
		}
		
		/* now populate the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		    v5 = rowpix[ii];  /* store the good pixel value */

		    if (do_range) {
			if (v5 < xminval) xminval = v5;
			if (v5 > xmaxval) xmaxval = v5;
		    }

		    /* construct array of 3rd order absolute differences */
		    if (noise) {
		        if (!(v1 == v2 && v2 == v3 && v3 == v4 && v4 == v5)) {

		            differences[nvals] = fabs((2. * v3) - v1 - v5);
		            nvals++;  
		        } else {
		            /* ignore constant background regions */
			    ngoodpix++;
		        }
		    } else {
		       /* just increment the number of non-null pixels */
		       ngoodpix++;
		    }

		    /* shift over 1 pixel */
		    v1 = v2;
		    v2 = v3;
		    v3 = v4;
		    v4 = v5;
	        }  /* end of loop over pixels in the row */

		/* compute the 3rd order diffs */
		/* Note that there are 4 more pixel values than there are diffs values. */
		ngoodpix += (nvals + 4);

		if (noise) {
		    if (nvals == 0) {
		        continue;  /* cannot compute medians on this row */
		    } else if (nvals == 1) {
		        diffs[nrows] = differences[0];
		    } else {
                        /* quick_select returns the median MUCH faster than using qsort */
                        diffs[nrows] = quick_select_double(differences, nvals);
		    }
		}
		nrows++;
	}  /* end of loop over rows */

	    /* compute median of the values for each row */
	if (noise) {
	    if (nrows == 0) { 
	       xnoise = 0;
	    } else if (nrows == 1) {
	       xnoise = diffs[0];
	    } else {	    
	       qsort(diffs, nrows, sizeof(double), FnCompare_double);
	       xnoise =  (diffs[(nrows - 1)/2] + diffs[nrows/2]) / 2.;
	    }
	}

	if (ngood)  *ngood  = ngoodpix;
	if (minval) *minval = xminval;
	if (maxval) *maxval = xmaxval;
	if (noise) {
		*noise  = 0.6052697 * xnoise;
		free(diffs);
		free(differences);
	}

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise1_short
       (short *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	short nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	double *noise,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */
/*
Estimate the background noise in the input image using sigma of 1st order differences.

  noise = 1.0 / sqrt(2) * rms of (flux[i] - flux[i-1])

The returned estimate is the median of the values that are computed for each 
row of the image.
*/
{
	int iter;
	long ii, jj, kk, nrows = 0, nvals;
	short *differences, *rowpix, v1;
	double  *diffs, xnoise, mean, stdev;

	/* rows must have at least 3 pixels to estimate noise */
	if (nx < 3) {
		*noise = 0;
		return(*status);
	}
	
        /* allocate arrays used to compute the median and noise estimates */
	differences = calloc(nx, sizeof(short));
	if (!differences) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs = calloc(ny, sizeof(double));
	if (!diffs) {
		free(differences);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */

		/* now continue populating the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		
		    /* construct array of 1st order differences */
		    differences[nvals] = v1 - rowpix[ii];

		    nvals++;  
		    /* shift over 1 pixel */
		    v1 = rowpix[ii];
	        }  /* end of loop over pixels in the row */

		if (nvals < 2)
		   continue;
		else {

		    FnMeanSigma_short(differences, nvals, 0, 0, 0, &mean, &stdev, status);

		    if (stdev > 0.) {
		        for (iter = 0;  iter < NITER;  iter++) {
		            kk = 0;
		            for (ii = 0;  ii < nvals;  ii++) {
		                if (fabs (differences[ii] - mean) < SIGMA_CLIP * stdev) {
			            if (kk < ii)
			                differences[kk] = differences[ii];
			            kk++;
		                }
		            }
		            if (kk == nvals) break;

		            nvals = kk;
		            FnMeanSigma_short(differences, nvals, 0, 0, 0, &mean, &stdev, status);
	              }
		   }

		   diffs[nrows] = stdev;
		   nrows++;
		}
	}  /* end of loop over rows */

	/* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise = 0;
	} else if (nrows == 1) {
	       xnoise = diffs[0];
	} else {
	       qsort(diffs, nrows, sizeof(double), FnCompare_double);
	       xnoise =  (diffs[(nrows - 1)/2] + diffs[nrows/2]) / 2.;
	}

	*noise = .70710678 * xnoise;

	free(diffs);
	free(differences);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise1_int
       (int *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	int nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	double *noise,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */
/*
Estimate the background noise in the input image using sigma of 1st order differences.

  noise = 1.0 / sqrt(2) * rms of (flux[i] - flux[i-1])

The returned estimate is the median of the values that are computed for each 
row of the image.
*/
{
	int iter;
	long ii, jj, kk, nrows = 0, nvals;
	int *differences, *rowpix, v1;
	double  *diffs, xnoise, mean, stdev;

	/* rows must have at least 3 pixels to estimate noise */
	if (nx < 3) {
		*noise = 0;
		return(*status);
	}
	
        /* allocate arrays used to compute the median and noise estimates */
	differences = calloc(nx, sizeof(int));
	if (!differences) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs = calloc(ny, sizeof(double));
	if (!diffs) {
		free(differences);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */

		/* now continue populating the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		
		    /* construct array of 1st order differences */
		    differences[nvals] = v1 - rowpix[ii];

		    nvals++;  
		    /* shift over 1 pixel */
		    v1 = rowpix[ii];
	        }  /* end of loop over pixels in the row */

		if (nvals < 2)
		   continue;
		else {

		    FnMeanSigma_int(differences, nvals, 0, 0, 0, &mean, &stdev, status);

		    if (stdev > 0.) {
		        for (iter = 0;  iter < NITER;  iter++) {
		            kk = 0;
		            for (ii = 0;  ii < nvals;  ii++) {
		                if (fabs (differences[ii] - mean) < SIGMA_CLIP * stdev) {
			            if (kk < ii)
			                differences[kk] = differences[ii];
			            kk++;
		                }
		            }
		            if (kk == nvals) break;

		            nvals = kk;
		            FnMeanSigma_int(differences, nvals, 0, 0, 0, &mean, &stdev, status);
	              }
		   }

		   diffs[nrows] = stdev;
		   nrows++;
		}
	}  /* end of loop over rows */

	/* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise = 0;
	} else if (nrows == 1) {
	       xnoise = diffs[0];
	} else {
	       qsort(diffs, nrows, sizeof(double), FnCompare_double);
	       xnoise =  (diffs[(nrows - 1)/2] + diffs[nrows/2]) / 2.;
	}

	*noise = .70710678 * xnoise;

	free(diffs);
	free(differences);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise1_float
       (float *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	float nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	double *noise,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */
/*
Estimate the background noise in the input image using sigma of 1st order differences.

  noise = 1.0 / sqrt(2) * rms of (flux[i] - flux[i-1])

The returned estimate is the median of the values that are computed for each 
row of the image.
*/
{
	int iter;
	long ii, jj, kk, nrows = 0, nvals;
	float *differences, *rowpix, v1;
	double  *diffs, xnoise, mean, stdev;

	/* rows must have at least 3 pixels to estimate noise */
	if (nx < 3) {
		*noise = 0;
		return(*status);
	}
	
        /* allocate arrays used to compute the median and noise estimates */
	differences = calloc(nx, sizeof(float));
	if (!differences) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs = calloc(ny, sizeof(double));
	if (!diffs) {
		free(differences);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */

		/* now continue populating the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		
		    /* construct array of 1st order differences */
		    differences[nvals] = v1 - rowpix[ii];

		    nvals++;  
		    /* shift over 1 pixel */
		    v1 = rowpix[ii];
	        }  /* end of loop over pixels in the row */

		if (nvals < 2)
		   continue;
		else {

		    FnMeanSigma_float(differences, nvals, 0, 0, 0, &mean, &stdev, status);

		    if (stdev > 0.) {
		        for (iter = 0;  iter < NITER;  iter++) {
		            kk = 0;
		            for (ii = 0;  ii < nvals;  ii++) {
		                if (fabs (differences[ii] - mean) < SIGMA_CLIP * stdev) {
			            if (kk < ii)
			                differences[kk] = differences[ii];
			            kk++;
		                }
		            }
		            if (kk == nvals) break;

		            nvals = kk;
		            FnMeanSigma_float(differences, nvals, 0, 0, 0, &mean, &stdev, status);
	              }
		   }

		   diffs[nrows] = stdev;
		   nrows++;
		}
	}  /* end of loop over rows */

	/* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise = 0;
	} else if (nrows == 1) {
	       xnoise = diffs[0];
	} else {
	       qsort(diffs, nrows, sizeof(double), FnCompare_double);
	       xnoise =  (diffs[(nrows - 1)/2] + diffs[nrows/2]) / 2.;
	}

	*noise = .70710678 * xnoise;

	free(diffs);
	free(differences);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnNoise1_double
       (double *array,       /*  2 dimensional array of image pixels */
        long nx,            /* number of pixels in each row of the image */
        long ny,            /* number of rows in the image */
	int nullcheck,      /* check for null values, if true */
	double nullvalue,    /* value of null pixels, if nullcheck is true */
   /* returned parameters */   
	double *noise,      /* returned R.M.S. value of all non-null pixels */
	int *status)        /* error status */
/*
Estimate the background noise in the input image using sigma of 1st order differences.

  noise = 1.0 / sqrt(2) * rms of (flux[i] - flux[i-1])

The returned estimate is the median of the values that are computed for each 
row of the image.
*/
{
	int iter;
	long ii, jj, kk, nrows = 0, nvals;
	double *differences, *rowpix, v1;
	double  *diffs, xnoise, mean, stdev;

	/* rows must have at least 3 pixels to estimate noise */
	if (nx < 3) {
		*noise = 0;
		return(*status);
	}
	
        /* allocate arrays used to compute the median and noise estimates */
	differences = calloc(nx, sizeof(double));
	if (!differences) {
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	diffs = calloc(ny, sizeof(double));
	if (!diffs) {
		free(differences);
        	*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* loop over each row of the image */
	for (jj=0; jj < ny; jj++) {

                rowpix = array + (jj * nx); /* point to first pixel in the row */

		/***** find the first valid pixel in row */
		ii = 0;
		if (nullcheck)
		    while (ii < nx && rowpix[ii] == nullvalue) ii++;

		if (ii == nx) continue;  /* hit end of row */
		v1 = rowpix[ii];  /* store the good pixel value */

		/* now continue populating the differences arrays */
		/* for the remaining pixels in the row */
		nvals = 0;
		for (ii++; ii < nx; ii++) {

		    /* find the next valid pixel in row */
                    if (nullcheck)
		        while (ii < nx && rowpix[ii] == nullvalue) ii++;
		     
		    if (ii == nx) break;  /* hit end of row */
		
		    /* construct array of 1st order differences */
		    differences[nvals] = v1 - rowpix[ii];

		    nvals++;  
		    /* shift over 1 pixel */
		    v1 = rowpix[ii];
	        }  /* end of loop over pixels in the row */

		if (nvals < 2)
		   continue;
		else {

		    FnMeanSigma_double(differences, nvals, 0, 0, 0, &mean, &stdev, status);

		    if (stdev > 0.) {
		        for (iter = 0;  iter < NITER;  iter++) {
		            kk = 0;
		            for (ii = 0;  ii < nvals;  ii++) {
		                if (fabs (differences[ii] - mean) < SIGMA_CLIP * stdev) {
			            if (kk < ii)
			                differences[kk] = differences[ii];
			            kk++;
		                }
		            }
		            if (kk == nvals) break;

		            nvals = kk;
		            FnMeanSigma_double(differences, nvals, 0, 0, 0, &mean, &stdev, status);
	              }
		   }

		   diffs[nrows] = stdev;
		   nrows++;
		}
	}  /* end of loop over rows */

	/* compute median of the values for each row */
	if (nrows == 0) { 
	       xnoise = 0;
	} else if (nrows == 1) {
	       xnoise = diffs[0];
	} else {
	       qsort(diffs, nrows, sizeof(double), FnCompare_double);
	       xnoise =  (diffs[(nrows - 1)/2] + diffs[nrows/2]) / 2.;
	}

	*noise = .70710678 * xnoise;

	free(diffs);
	free(differences);

	return(*status);
}
/*--------------------------------------------------------------------------*/
static int FnCompare_short(const void *v1, const void *v2)
{
   const short *i1 = v1;
   const short *i2 = v2;
   
   if (*i1 < *i2)
     return(-1);
   else if (*i1 > *i2)
     return(1);
   else
     return(0);
}
/*--------------------------------------------------------------------------*/
static int FnCompare_int(const void *v1, const void *v2)
{
   const int *i1 = v1;
   const int *i2 = v2;
   
   if (*i1 < *i2)
     return(-1);
   else if (*i1 > *i2)
     return(1);
   else
     return(0);
}
/*--------------------------------------------------------------------------*/
static int FnCompare_float(const void *v1, const void *v2)
{
   const float *i1 = v1;
   const float *i2 = v2;
   
   if (*i1 < *i2)
     return(-1);
   else if (*i1 > *i2)
     return(1);
   else
     return(0);
}
/*--------------------------------------------------------------------------*/
static int FnCompare_double(const void *v1, const void *v2)
{
   const double *i1 = v1;
   const double *i2 = v2;
   
   if (*i1 < *i2)
     return(-1);
   else if (*i1 > *i2)
     return(1);
   else
     return(0);
}
/*--------------------------------------------------------------------------*/

/*
 *  These Quickselect routines are based on the algorithm described in
 *  "Numerical recipes in C", Second Edition,
 *  Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
 *  This code by Nicolas Devillard - 1998. Public domain.
 */

/*--------------------------------------------------------------------------*/

#define ELEM_SWAP(a,b) { register float t=(a);(a)=(b);(b)=t; }

static float quick_select_float(float arr[], int n) 
{
    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median] ;

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    }
}

#undef ELEM_SWAP

/*--------------------------------------------------------------------------*/

#define ELEM_SWAP(a,b) { register short t=(a);(a)=(b);(b)=t; }

static short quick_select_short(short arr[], int n) 
{
    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median] ;

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    }
}

#undef ELEM_SWAP

/*--------------------------------------------------------------------------*/

#define ELEM_SWAP(a,b) { register int t=(a);(a)=(b);(b)=t; }

static int quick_select_int(int arr[], int n) 
{
    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median] ;

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    }
}

#undef ELEM_SWAP

/*--------------------------------------------------------------------------*/

#define ELEM_SWAP(a,b) { register LONGLONG  t=(a);(a)=(b);(b)=t; }

static LONGLONG quick_select_longlong(LONGLONG arr[], int n) 
{
    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median] ;

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    }
}

#undef ELEM_SWAP

/*--------------------------------------------------------------------------*/

#define ELEM_SWAP(a,b) { register double t=(a);(a)=(b);(b)=t; }

static double quick_select_double(double arr[], int n) 
{
    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median] ;

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    }
}

#undef ELEM_SWAP


