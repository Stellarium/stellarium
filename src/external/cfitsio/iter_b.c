#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fitsio.h"

/*
  This program illustrates how to use the CFITSIO iterator function.
  It simply prints out the values in a character string and a logical
  type column in a table, and toggles the value in the logical column
  so that T -> F and F -> T.
*/
main()
{
    extern str_iter(); /* external work function is passed to the iterator */
    fitsfile *fptr;
    iteratorCol cols[2];
    int n_cols;
    long rows_per_loop, offset;
    int status = 0;
    char filename[]  = "iter_b.fit";     /* name of rate FITS file */

    /* open the file and move to the correct extension */
    fits_open_file(&fptr, filename, READWRITE, &status);
    fits_movnam_hdu(fptr, BINARY_TBL, "iter_test", 0, &status);

    /* define input column structure members for the iterator function */
    n_cols  = 2;   /* number of columns */

    /* define input column structure members for the iterator function */
    fits_iter_set_by_name(&cols[0], fptr, "Avalue", TSTRING,  InputOutputCol);
    fits_iter_set_by_name(&cols[1], fptr, "Lvalue", TLOGICAL, InputOutputCol);

    rows_per_loop = 0;  /* use default optimum number of rows */
    offset = 0;         /* process all the rows */

    /* apply the  function to each row of the table */
    printf("Calling iterator function...%d\n", status);

    fits_iterate_data(n_cols, cols, offset, rows_per_loop,
                      str_iter, 0L, &status);

    fits_close_file(fptr, &status);      /* all done */

    if (status)
       fits_report_error(stderr, status); /* print out error messages */

    return(status);
}
/*--------------------------------------------------------------------------*/
int str_iter(long totalrows, long offset, long firstrow, long nrows,
             int ncols, iteratorCol *cols, void *user_strct )

/*
   Sample iterator function.
*/
{
    int ii;

    /* declare variables static to preserve their values between calls */
    static char **stringvals;
    static char *logicalvals;

    /*--------------------------------------------------------*/
    /*  Initialization procedures: execute on the first call  */
    /*--------------------------------------------------------*/
    if (firstrow == 1)
    {
       if (ncols != 2)
           return(-1);  /* number of columns incorrect */

       if (fits_iter_get_datatype(&cols[0]) != TSTRING ||
           fits_iter_get_datatype(&cols[1]) != TLOGICAL )
           return(-2);  /* bad data type */

       /* assign the input pointers to the appropriate arrays */
       stringvals       = (char **) fits_iter_get_array(&cols[0]);
       logicalvals      = (char *)  fits_iter_get_array(&cols[1]);

       printf("Total rows, No. rows = %d %d\n",totalrows, nrows);
    }

    /*------------------------------------------*/
    /*  Main loop: process all the rows of data */
    /*------------------------------------------*/

    /*  NOTE: 1st element of array is the null pixel value!  */
    /*  Loop from 1 to nrows, not 0 to nrows - 1.  */
   
    for (ii = 1; ii <= nrows; ii++)
    {
      printf("%s %d\n", stringvals[ii], logicalvals[ii]);
      if (logicalvals[ii])
      {
         logicalvals[ii] = FALSE;
         strcpy(stringvals[ii], "changed to false");
      }
      else
      {
         logicalvals[ii] = TRUE;
         strcpy(stringvals[ii], "changed to true");
      }
    }

    /*-------------------------------------------------------*/
    /*  Clean up procedures:  after processing all the rows  */
    /*-------------------------------------------------------*/

    if (firstrow + nrows - 1 == totalrows)
    {
      /* no action required in this case */
    }
 
    return(0);
}
