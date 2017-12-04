#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fitsio.h"

/*
    This example program illustrates how to use the CFITSIO iterator function.

    This program creates a 2D histogram of the X and Y columns of an event
    list.  The 'main' routine just creates the empty new image, then executes
    the 'writehisto' work function by calling the CFITSIO iterator function.

    'writehisto' opens the FITS event list that contains the X and Y columns.
    It then calls a second work function, calchisto, (by recursively calling
    the CFITSIO iterator function) which actually computes the 2D histogram.
*/

/*   Globally defined parameters */

long xsize = 480; /* size of the histogram image */
long ysize = 480;
long xbinsize = 32;
long ybinsize = 32;

main()
{
    extern writehisto();  /* external work function passed to the iterator */
    extern long xsize, ysize;  /* size of image */

    fitsfile *fptr;
    iteratorCol cols[1];
    int n_cols, status = 0;
    long n_per_loop, offset, naxes[2];
    char filename[]  = "histoimg.fit";     /* name of FITS image */

    remove(filename);   /* delete previous version of the file if it exists */
    fits_create_file(&fptr, filename, &status);  /* create new output image */

    naxes[0] = xsize;
    naxes[1] = ysize;
    fits_create_img(fptr, LONG_IMG, 2, naxes, &status); /* create primary HDU */

    n_cols  = 1;   /* number of columns */

    /* define input column structure members for the iterator function */
    fits_iter_set_by_name(&cols[0], fptr, " ", TLONG, OutputCol);

    n_per_loop = -1;  /* force whole array to be passed at one time */
    offset = 0;       /* don't skip over any pixels */

    /* execute the function to create and write the 2D histogram */
    printf("Calling writehisto iterator work function... %d\n", status);

    fits_iterate_data(n_cols, cols, offset, n_per_loop,
                      writehisto, 0L, &status);

    fits_close_file(fptr, &status);      /* all done; close the file */

    if (status)
        fits_report_error(stderr, status);  /* print out error messages */
    else
        printf("Program completed successfully.\n");

    return(status);
}
/*--------------------------------------------------------------------------*/
int writehisto(long totaln, long offset, long firstn, long nvalues,
             int narrays, iteratorCol *histo, void *userPointer)
/*
   Iterator work function that writes out the 2D histogram.
   The histogram values are calculated by another work function, calchisto.

   This routine is executed only once since nvalues was forced to = totaln.
*/
{
    extern calchisto();  /* external function called by the iterator */
    long *histogram;
    fitsfile *tblptr;
    iteratorCol cols[2];
    int n_cols, status = 0;
    long rows_per_loop, rowoffset;
    char filename[]  = "iter_c.fit";     /* name of FITS table */

    /* do sanity checking of input values */
    if (totaln != nvalues)
        return(-1);  /* whole image must be passed at one time */

    if (narrays != 1)
        return(-2);  /* number of images is incorrect */

    if (fits_iter_get_datatype(&histo[0]) != TLONG)
        return(-3);  /* input array has wrong data type */

    /* assign the FITS array pointer to the global histogram pointer */
    histogram = (long *) fits_iter_get_array(&histo[0]);

    /* open the file and move to the table containing the X and Y columns */
    fits_open_file(&tblptr, filename, READONLY, &status);
    fits_movnam_hdu(tblptr, BINARY_TBL, "EVENTS", 0, &status);
    if (status)
       return(status);
   
    n_cols = 2; /* number of columns */

    /* define input column structure members for the iterator function */
    fits_iter_set_by_name(&cols[0], tblptr, "X", TLONG,  InputCol);
    fits_iter_set_by_name(&cols[1], tblptr, "Y", TLONG, InputCol);

    rows_per_loop = 0;  /* take default number of rows per interation */
    rowoffset = 0;     

    /* calculate the histogram */
    printf("Calling calchisto iterator work function... %d\n", status);

    fits_iterate_data(n_cols, cols, rowoffset, rows_per_loop,
                      calchisto, histogram, &status);

    fits_close_file(tblptr, &status);      /* all done */
    return(status);
}
/*--------------------------------------------------------------------------*/
int calchisto(long totalrows, long offset, long firstrow, long nrows,
             int ncols, iteratorCol *cols, void *userPointer)

/*
   Interator work function that calculates values for the 2D histogram.
*/
{
    extern long xsize, ysize, xbinsize, ybinsize;
    long ii, ihisto, xbin, ybin;
    static long *xcol, *ycol, *histogram;  /* static to preserve values */

    /*--------------------------------------------------------*/
    /*  Initialization procedures: execute on the first call  */
    /*--------------------------------------------------------*/
    if (firstrow == 1)
    {
        /* do sanity checking of input values */
       if (ncols != 2)
         return(-3);  /* number of arrays is incorrect */

       if (fits_iter_get_datatype(&cols[0]) != TLONG ||
           fits_iter_get_datatype(&cols[1]) != TLONG)
         return(-4);  /* wrong datatypes */

       /* assign the input array points to the X and Y arrays */
       xcol = (long *) fits_iter_get_array(&cols[0]);
       ycol = (long *) fits_iter_get_array(&cols[1]);
       histogram = (long *) userPointer;

       /* initialize the histogram image pixels = 0 */
       for (ii = 0; ii <= xsize * ysize; ii++)
           histogram[ii] = 0L;
    }

    /*------------------------------------------------------------------*/
    /*  Main loop: increment the 2D histogram at position of each event */
    /*------------------------------------------------------------------*/

    for (ii = 1; ii <= nrows; ii++) 
    {
        xbin = xcol[ii] / xbinsize;
        ybin = ycol[ii] / ybinsize;

        ihisto = ( ybin * xsize ) + xbin + 1;
        histogram[ihisto]++;
    }

    return(0);
}

