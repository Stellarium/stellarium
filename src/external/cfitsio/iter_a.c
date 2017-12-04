#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fitsio.h"

/*
  This program illustrates how to use the CFITSIO iterator function.
  It reads and modifies the input 'iter_a.fit' file by computing a
  value for the 'rate' column as a function of the values in the other
  'counts' and 'time' columns.
*/
main()
{
    extern flux_rate(); /* external work function is passed to the iterator */
    fitsfile *fptr;
    iteratorCol cols[3];  /* structure used by the iterator function */
    int n_cols;
    long rows_per_loop, offset;

    int status, nkeys, keypos, hdutype, ii, jj;
    char filename[]  = "iter_a.fit";     /* name of rate FITS file */

    status = 0; 

    fits_open_file(&fptr, filename, READWRITE, &status); /* open file */

    /* move to the desired binary table extension */
    if (fits_movnam_hdu(fptr, BINARY_TBL, "RATE", 0, &status) )
        fits_report_error(stderr, status);    /* print out error messages */

    n_cols  = 3;   /* number of columns */

    /* define input column structure members for the iterator function */
    fits_iter_set_by_name(&cols[0], fptr, "COUNTS", TLONG,  InputCol);
    fits_iter_set_by_name(&cols[1], fptr, "TIME",   TFLOAT, InputCol);
    fits_iter_set_by_name(&cols[2], fptr, "RATE",   TFLOAT, OutputCol);

    rows_per_loop = 0;  /* use default optimum number of rows */
    offset = 0;         /* process all the rows */

    /* apply the rate function to each row of the table */
    printf("Calling iterator function...%d\n", status);

    fits_iterate_data(n_cols, cols, offset, rows_per_loop,
                      flux_rate, 0L, &status);

    fits_close_file(fptr, &status);      /* all done */

    if (status)
        fits_report_error(stderr, status);  /* print out error messages */

    return(status);
}
/*--------------------------------------------------------------------------*/
int flux_rate(long totalrows, long offset, long firstrow, long nrows,
             int ncols, iteratorCol *cols, void *user_strct ) 

/*
   Sample iterator function that calculates the output flux 'rate' column
   by dividing the input 'counts' by the 'time' column.
   It also applies a constant deadtime correction factor if the 'deadtime'
   keyword exists.  Finally, this creates or updates the 'LIVETIME'
   keyword with the sum of all the individual integration times.
*/
{
    int ii, status = 0;

    /* declare variables static to preserve their values between calls */
    static long *counts;
    static float *interval;
    static float *rate;
    static float deadtime, livetime; /* must preserve values between calls */

    /*--------------------------------------------------------*/
    /*  Initialization procedures: execute on the first call  */
    /*--------------------------------------------------------*/
    if (firstrow == 1)
    {
       if (ncols != 3)
           return(-1);  /* number of columns incorrect */

       if (fits_iter_get_datatype(&cols[0]) != TLONG  ||
           fits_iter_get_datatype(&cols[1]) != TFLOAT ||
           fits_iter_get_datatype(&cols[2]) != TFLOAT )
           return(-2);  /* bad data type */

       /* assign the input pointers to the appropriate arrays and null ptrs*/
       counts       = (long *)  fits_iter_get_array(&cols[0]);
       interval     = (float *) fits_iter_get_array(&cols[1]);
       rate         = (float *) fits_iter_get_array(&cols[2]);

       livetime = 0;  /* initialize the total integration time */

       /* try to get the deadtime keyword value */
       fits_read_key(cols[0].fptr, TFLOAT, "DEADTIME", &deadtime, '\0',
                     &status);
       if (status)
       {
           deadtime = 1.0;  /* default deadtime if keyword doesn't exist */
       }
       else if (deadtime < 0. || deadtime > 1.0)
       {
           return(-1);    /* bad deadtime value */
       }

       printf("deadtime = %f\n", deadtime);
    }

    /*--------------------------------------------*/
    /*  Main loop: process all the rows of data */
    /*--------------------------------------------*/

    /*  NOTE: 1st element of array is the null pixel value!  */
    /*  Loop from 1 to nrows, not 0 to nrows - 1.  */

    /* this version tests for null values */
    rate[0] = DOUBLENULLVALUE;   /* define the value that represents null */

    for (ii = 1; ii <= nrows; ii++)
    {
       if (counts[ii] == counts[0])   /*  undefined counts value? */
       {
           rate[ii] = DOUBLENULLVALUE;
       }
       else if (interval[ii] > 0.)
       {
           rate[ii] = counts[ii] / interval[ii] / deadtime;
           livetime += interval[ii];  /* accumulate total integration time */
       }
       else
           return(-2);  /* bad integration time */
    }

    /*-------------------------------------------------------*/
    /*  Clean up procedures:  after processing all the rows  */
    /*-------------------------------------------------------*/

    if (firstrow + nrows - 1 == totalrows)
    {
        /*  update the LIVETIME keyword value */

        fits_update_key(cols[0].fptr, TFLOAT, "LIVETIME", &livetime, 
                 "total integration time", &status);
        printf("livetime = %f\n", livetime);
   }
    return(0);  /* return successful status */
}
