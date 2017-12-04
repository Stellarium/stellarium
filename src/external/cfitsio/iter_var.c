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
    char filename[]  = "vari.fits";     /* name of rate FITS file */

    status = 0; 

    fits_open_file(&fptr, filename, READWRITE, &status); /* open file */

    /* move to the desired binary table extension */
    if (fits_movnam_hdu(fptr, BINARY_TBL, "COMPRESSED_IMAGE", 0, &status) )
        fits_report_error(stderr, status);    /* print out error messages */

    n_cols  = 1;   /* number of columns */

    /* define input column structure members for the iterator function */
    fits_iter_set_by_name(&cols[0], fptr, "COMPRESSED_DATA", 0,  InputCol);

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
    long repeat;

    /* declare variables static to preserve their values between calls */
    static unsigned char *counts;

    /*--------------------------------------------------------*/
    /*  Initialization procedures: execute on the first call  */
    /*--------------------------------------------------------*/
    if (firstrow == 1)
    {

printf("Datatype of column = %d\n",fits_iter_get_datatype(&cols[0]));

       /* assign the input pointers to the appropriate arrays and null ptrs*/
       counts       = (long *)  fits_iter_get_array(&cols[0]);

    }

    /*--------------------------------------------*/
    /*  Main loop: process all the rows of data */
    /*--------------------------------------------*/

    /*  NOTE: 1st element of array is the null pixel value!  */
    /*  Loop from 1 to nrows, not 0 to nrows - 1.  */


    for (ii = 1; ii <= nrows; ii++)
    {
       repeat = fits_iter_get_repeat(&cols[0]);
       printf ("repeat = %d, %d\n",repeat, counts[1]);
       
    }


    return(0);  /* return successful status */
}
