#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
  Every program which uses the CFITSIO interface must include the
  the fitsio.h header file.  This contains the prototypes for all
  the routines and defines the error status values and other symbolic
  constants used in the interface.  
*/
#include "fitsio.h"

int main( void );
void writeimage( void );
void writeascii( void );
void writebintable( void );
void copyhdu( void );
void selectrows( void );
void readheader( void );
void readimage( void );
void readtable( void );
void printerror( int status);

int main()
{
/*************************************************************************
   This is a simple main program that calls the following routines:

    writeimage    - write a FITS primary array image
    writeascii    - write a FITS ASCII table extension
    writebintable - write a FITS binary table extension
    copyhdu       - copy a header/data unit from one FITS file to another
    selectrows    - copy selected row from one HDU to another
    readheader    - read and print the header keywords in every extension
    readimage     - read a FITS image and compute the min and max value
    readtable     - read columns of data from ASCII and binary tables

**************************************************************************/

    writeimage();
    writeascii();
    writebintable();
    copyhdu();
    selectrows();
    readheader();
    readimage();
    readtable();

    printf("\nAll the cfitsio cookbook routines ran successfully.\n");
    return(0);
}
/*--------------------------------------------------------------------------*/
void writeimage( void )

    /******************************************************/
    /* Create a FITS primary array containing a 2-D image */
    /******************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status, ii, jj;
    long  fpixel, nelements, exposure;
    unsigned short *array[200];

    /* initialize FITS image parameters */
    char filename[] = "atestfil.fit";             /* name for new FITS file */
    int bitpix   =  USHORT_IMG; /* 16-bit unsigned short pixel values       */
    long naxis    =   2;  /* 2-dimensional image                            */    
    long naxes[2] = { 300, 200 };   /* image is 300 pixels wide by 200 rows */

    /* allocate memory for the whole image */ 
    array[0] = (unsigned short *)malloc( naxes[0] * naxes[1]
                                        * sizeof( unsigned short ) );

    /* initialize pointers to the start of each row of the image */
    for( ii=1; ii<naxes[1]; ii++ )
      array[ii] = array[ii-1] + naxes[0];

    remove(filename);               /* Delete old file if it already exists */

    status = 0;         /* initialize status before calling fitsio routines */

    if (fits_create_file(&fptr, filename, &status)) /* create new FITS file */
         printerror( status );           /* call printerror if error occurs */

    /* write the required keywords for the primary array image.     */
    /* Since bitpix = USHORT_IMG, this will cause cfitsio to create */
    /* a FITS image with BITPIX = 16 (signed short integers) with   */
    /* BSCALE = 1.0 and BZERO = 32768.  This is the convention that */
    /* FITS uses to store unsigned integers.  Note that the BSCALE  */
    /* and BZERO keywords will be automatically written by cfitsio  */
    /* in this case.                                                */

    if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) )
         printerror( status );          

    /* initialize the values in the image with a linear ramp function */
    for (jj = 0; jj < naxes[1]; jj++)
    {   for (ii = 0; ii < naxes[0]; ii++)
        {
            array[jj][ii] = ii + jj;
        }
    }

    fpixel = 1;                               /* first pixel to write      */
    nelements = naxes[0] * naxes[1];          /* number of pixels to write */

    /* write the array of unsigned integers to the FITS file */
    if ( fits_write_img(fptr, TUSHORT, fpixel, nelements, array[0], &status) )
        printerror( status );
      
    free( array[0] );  /* free previously allocated memory */

    /* write another optional keyword to the header */
    /* Note that the ADDRESS of the value is passed in the routine */
    exposure = 1500;
    if ( fits_update_key(fptr, TLONG, "EXPOSURE", &exposure,
         "Total Exposure Time", &status) )
         printerror( status );           

    if ( fits_close_file(fptr, &status) )                /* close the file */
         printerror( status );           

    return;
}
/*--------------------------------------------------------------------------*/
void writeascii ( void )

    /*******************************************************************/
    /* Create an ASCII table extension containing 3 columns and 6 rows */
    /*******************************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status;
    long firstrow, firstelem;

    int tfields = 3;       /* table will have 3 columns */
    long nrows  = 6;       /* table will have 6 rows    */

    char filename[] = "atestfil.fit";           /* name for new FITS file */
    char extname[] = "PLANETS_ASCII";             /* extension name */

    /* define the name, datatype, and physical units for the 3 columns */
    char *ttype[] = { "Planet", "Diameter", "Density" };
    char *tform[] = { "a8",     "I6",       "F4.2"    };
    char *tunit[] = { "\0",      "km",       "g/cm"    };

    /* define the name diameter, and density of each planet */
    char *planet[] = {"Mercury", "Venus", "Earth", "Mars","Jupiter","Saturn"};
    long  diameter[] = {4880,    12112,    12742,   6800,  143000,   121000};
    float density[]  = { 5.1f,     5.3f,      5.52f,   3.94f,    1.33f,    0.69f};

    status=0;

    /* open with write access the FITS file containing a primary array */
    if ( fits_open_file(&fptr, filename, READWRITE, &status) ) 
         printerror( status );

    /* append a new empty ASCII table onto the FITS file */
    if ( fits_create_tbl( fptr, ASCII_TBL, nrows, tfields, ttype, tform,
                tunit, extname, &status) )
         printerror( status );

    firstrow  = 1;  /* first row in table to write   */
    firstelem = 1;  /* first element in row  (ignored in ASCII tables) */

    /* write names to the first column (character strings) */
    /* write diameters to the second column (longs) */
    /* write density to the third column (floats) */

    fits_write_col(fptr, TSTRING, 1, firstrow, firstelem, nrows, planet,
                   &status);
    fits_write_col(fptr, TLONG, 2, firstrow, firstelem, nrows, diameter,
                   &status);
    fits_write_col(fptr, TFLOAT, 3, firstrow, firstelem, nrows, density,
                   &status);

    if ( fits_close_file(fptr, &status) )       /* close the FITS file */
         printerror( status );
    return;
}
/*--------------------------------------------------------------------------*/
void writebintable ( void )

    /*******************************************************************/
    /* Create a binary table extension containing 3 columns and 6 rows */
    /*******************************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status, hdutype;
    long firstrow, firstelem;

    int tfields   = 3;       /* table will have 3 columns */
    long nrows    = 6;       /* table will have 6 rows    */

    char filename[] = "atestfil.fit";           /* name for new FITS file */
    char extname[] = "PLANETS_Binary";           /* extension name */

    /* define the name, datatype, and physical units for the 3 columns */
    char *ttype[] = { "Planet", "Diameter", "Density" };
    char *tform[] = { "8a",     "1J",       "1E"    };
    char *tunit[] = { "\0",      "km",       "g/cm"    };

    /* define the name diameter, and density of each planet */
    char *planet[] = {"Mercury", "Venus", "Earth", "Mars","Jupiter","Saturn"};
    long  diameter[] = {4880,     12112,   12742,   6800,  143000,   121000};
    float density[]  = { 5.1f,      5.3f,     5.52f,   3.94f,   1.33f,     0.69f};

    status=0;

    /* open the FITS file containing a primary array and an ASCII table */
    if ( fits_open_file(&fptr, filename, READWRITE, &status) ) 
         printerror( status );

    if ( fits_movabs_hdu(fptr, 2, &hdutype, &status) ) /* move to 2nd HDU */
         printerror( status );

    /* append a new empty binary table onto the FITS file */
    if ( fits_create_tbl( fptr, BINARY_TBL, nrows, tfields, ttype, tform,
                tunit, extname, &status) )
         printerror( status );

    firstrow  = 1;  /* first row in table to write   */
    firstelem = 1;  /* first element in row  (ignored in ASCII tables) */

    /* write names to the first column (character strings) */
    /* write diameters to the second column (longs) */
    /* write density to the third column (floats) */

    fits_write_col(fptr, TSTRING, 1, firstrow, firstelem, nrows, planet,
                   &status);
    fits_write_col(fptr, TLONG, 2, firstrow, firstelem, nrows, diameter,
                   &status);
    fits_write_col(fptr, TFLOAT, 3, firstrow, firstelem, nrows, density,
                   &status);

    if ( fits_close_file(fptr, &status) )       /* close the FITS file */
         printerror( status );
    return;
}
/*--------------------------------------------------------------------------*/
void copyhdu( void)
{
    /********************************************************************/
    /* copy the 1st and 3rd HDUs from the input file to a new FITS file */
    /********************************************************************/
    fitsfile *infptr;      /* pointer to the FITS file, defined in fitsio.h */
    fitsfile *outfptr;                 /* pointer to the new FITS file      */

    char infilename[]  = "atestfil.fit";  /* name for existing FITS file   */
    char outfilename[] = "btestfil.fit";  /* name for new FITS file        */

    int status, morekeys, hdutype;

    status = 0;

    remove(outfilename);            /* Delete old file if it already exists */

    /* open the existing FITS file */
    if ( fits_open_file(&infptr, infilename, READONLY, &status) ) 
         printerror( status );

    if (fits_create_file(&outfptr, outfilename, &status)) /*create FITS file*/
         printerror( status );           /* call printerror if error occurs */

    /* copy the primary array from the input file to the output file */
    morekeys = 0;     /* don't reserve space for additional keywords */
    if ( fits_copy_hdu(infptr, outfptr, morekeys, &status) )
         printerror( status );

    /* move to the 3rd HDU in the input file */
    if ( fits_movabs_hdu(infptr, 3, &hdutype, &status) )
         printerror( status );

    /* copy 3rd HDU from the input file to the output file (to 2nd HDU) */
    if ( fits_copy_hdu(infptr, outfptr, morekeys, &status) )
         printerror( status );

    if (fits_close_file(outfptr, &status) ||
        fits_close_file(infptr, &status)) /* close files */
         printerror( status );

    return;
}
/*--------------------------------------------------------------------------*/
void selectrows( void )

    /*********************************************************************/
    /* select rows from an input table and copy them to the output table */
    /*********************************************************************/
{
    fitsfile *infptr, *outfptr;  /* pointer to input and output FITS files */
    unsigned char *buffer;
    char card[FLEN_CARD];
    int status, hdutype, nkeys, keypos, nfound, colnum, anynulls, ii;
    long naxes[2], frow, felem, noutrows, irow;
    float nullval, density[6];

    char infilename[]  = "atestfil.fit";  /* name for existing FITS file   */
    char outfilename[] = "btestfil.fit";  /* name for new FITS file        */

    status = 0;

    /* open the existing FITS files */
    if ( fits_open_file(&infptr,  infilename,  READONLY,  &status) ||
         fits_open_file(&outfptr, outfilename, READWRITE, &status) ) 
         printerror( status );

    /* move to the 3rd HDU in the input file (a binary table in this case) */
    if ( fits_movabs_hdu(infptr, 3, &hdutype, &status) )
         printerror( status );

    if (hdutype != BINARY_TBL)  {
        printf("Error: expected to find a binary table in this HDU\n");
        return;
    }
    /* move to the last (2rd) HDU in the output file */
    if ( fits_movabs_hdu(outfptr, 2, &hdutype, &status) )
         printerror( status );

    /* create new extension in the output file */
    if ( fits_create_hdu(outfptr, &status) )
         printerror( status );

    /* get number of keywords */
    if ( fits_get_hdrpos(infptr, &nkeys, &keypos, &status) ) 
         printerror( status );

    /* copy all the keywords from the input to the output extension */
    for (ii = 1; ii <= nkeys; ii++)  {
        fits_read_record (infptr, ii, card, &status); 
        fits_write_record(outfptr,    card, &status); 
    }

    /* read the NAXIS1 and NAXIS2 keyword to get table size */
    if (fits_read_keys_lng(infptr, "NAXIS", 1, 2, naxes, &nfound, &status) )
         printerror( status );

    /* find which column contains the DENSITY values */
    if ( fits_get_colnum(infptr, CASEINSEN, "density", &colnum, &status) )
         printerror( status );

    /* read the DENSITY column values */
    frow = 1;
    felem = 1;
    nullval = -99.;
    if (fits_read_col(infptr, TFLOAT, colnum, frow, felem, naxes[1], 
        &nullval, density, &anynulls, &status) )
        printerror( status );

    /* allocate buffer large enough for 1 row of the table */
    buffer = (unsigned char *) malloc(naxes[0]);

    /* If the density is less than 3.0, copy the row to the output table */
    for (noutrows = 0, irow = 1; irow <= naxes[1]; irow++)  {
      if (density[irow - 1] < 3.0)  {
        noutrows++;
        fits_read_tblbytes( infptr, irow,      1, naxes[0], buffer, &status); 
        fits_write_tblbytes(outfptr, noutrows, 1, naxes[0], buffer, &status); 
    } }

    /* update the NAXIS2 keyword with the correct number of rows */
    if ( fits_update_key(outfptr, TLONG, "NAXIS2", &noutrows, 0, &status) )
         printerror( status );

    if (fits_close_file(outfptr, &status) || fits_close_file(infptr, &status))
        printerror( status );

    return;
}
/*--------------------------------------------------------------------------*/
void readheader ( void )

    /**********************************************************************/
    /* Print out all the header keywords in all extensions of a FITS file */
    /**********************************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */

    int status, nkeys, keypos, hdutype, ii, jj;
    char filename[]  = "atestfil.fit";     /* name of existing FITS file   */
    char card[FLEN_CARD];   /* standard string lengths defined in fitsioc.h */

    status = 0;

    if ( fits_open_file(&fptr, filename, READONLY, &status) ) 
         printerror( status );

    /* attempt to move to next HDU, until we get an EOF error */
    for (ii = 1; !(fits_movabs_hdu(fptr, ii, &hdutype, &status) ); ii++) 
    {
        /* get no. of keywords */
        if (fits_get_hdrpos(fptr, &nkeys, &keypos, &status) )
            printerror( status );

        printf("Header listing for HDU #%d:\n", ii);
        for (jj = 1; jj <= nkeys; jj++)  {
            if ( fits_read_record(fptr, jj, card, &status) )
                 printerror( status );

            printf("%s\n", card); /* print the keyword card */
        }
        printf("END\n\n");  /* terminate listing with END */
    }

    if (status == END_OF_FILE)   /* status values are defined in fitsioc.h */
        status = 0;              /* got the expected EOF error; reset = 0  */
    else
       printerror( status );     /* got an unexpected error                */

    if ( fits_close_file(fptr, &status) )
         printerror( status );

    return;
}
/*--------------------------------------------------------------------------*/
void readimage( void )

    /************************************************************************/
    /* Read a FITS image and determine the minimum and maximum pixel values */
    /************************************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status,  nfound, anynull;
    long naxes[2], fpixel, nbuffer, npixels, ii;

#define buffsize 1000
    float datamin, datamax, nullval, buffer[buffsize];
    char filename[]  = "atestfil.fit";     /* name of existing FITS file   */

    status = 0;

    if ( fits_open_file(&fptr, filename, READONLY, &status) )
         printerror( status );

    /* read the NAXIS1 and NAXIS2 keyword to get image size */
    if ( fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status) )
         printerror( status );

    npixels  = naxes[0] * naxes[1];         /* number of pixels in the image */
    fpixel   = 1;
    nullval  = 0;                /* don't check for null values in the image */
    datamin  = 1.0E30f;
    datamax  = -1.0E30f;

    while (npixels > 0)
    {
      nbuffer = npixels;
      if (npixels > buffsize)
        nbuffer = buffsize;     /* read as many pixels as will fit in buffer */

      /* Note that even though the FITS images contains unsigned integer */
      /* pixel values (or more accurately, signed integer pixels with    */
      /* a bias of 32768),  this routine is reading the values into a    */
      /* float array.   Cfitsio automatically performs the datatype      */
      /* conversion in cases like this.                                  */

      if ( fits_read_img(fptr, TFLOAT, fpixel, nbuffer, &nullval,
                  buffer, &anynull, &status) )
           printerror( status );

      for (ii = 0; ii < nbuffer; ii++)  {
        if ( buffer[ii] < datamin )
            datamin = buffer[ii];

        if ( buffer[ii] > datamax )
            datamax = buffer[ii];
      }
      npixels -= nbuffer;    /* increment remaining number of pixels */
      fpixel  += nbuffer;    /* next pixel to be read in image */
    }

    printf("\nMin and max image pixels =  %.0f, %.0f\n", datamin, datamax);

    if ( fits_close_file(fptr, &status) )
         printerror( status );

    return;
}
/*--------------------------------------------------------------------------*/
void readtable( void )

    /************************************************************/
    /* read and print data values from an ASCII or binary table */
    /************************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status, hdunum, hdutype,  nfound, anynull, ii;
    long frow, felem, nelem, longnull, dia[6];
    float floatnull, den[6];
    char strnull[10], *name[6], *ttype[3]; 

    char filename[]  = "atestfil.fit";     /* name of existing FITS file   */

    status = 0;

    if ( fits_open_file(&fptr, filename, READONLY, &status) )
         printerror( status );

    for (ii = 0; ii < 3; ii++)      /* allocate space for the column labels */
        ttype[ii] = (char *) malloc(FLEN_VALUE);  /* max label length = 69 */

    for (ii = 0; ii < 6; ii++)    /* allocate space for string column value */
        name[ii] = (char *) malloc(10);   

    for (hdunum = 2; hdunum <= 3; hdunum++) /*read ASCII, then binary table */
    {
      /* move to the HDU */
      if ( fits_movabs_hdu(fptr, hdunum, &hdutype, &status) ) 
           printerror( status );

      if (hdutype == ASCII_TBL)
          printf("\nReading ASCII table in HDU %d:\n",  hdunum);
      else if (hdutype == BINARY_TBL)
          printf("\nReading binary table in HDU %d:\n", hdunum);
      else
      {
          printf("Error: this HDU is not an ASCII or binary table\n");
          printerror( status );
      }

      /* read the column names from the TTYPEn keywords */
      fits_read_keys_str(fptr, "TTYPE", 1, 3, ttype, &nfound, &status);

      printf(" Row  %10s %10s %10s\n", ttype[0], ttype[1], ttype[2]);

      frow      = 1;
      felem     = 1;
      nelem     = 6;
      strcpy(strnull, " ");
      longnull  = 0;
      floatnull = 0.;

      /*  read the columns */  
      fits_read_col(fptr, TSTRING, 1, frow, felem, nelem, strnull,  name,
                    &anynull, &status);
      fits_read_col(fptr, TLONG, 2, frow, felem, nelem, &longnull,  dia,
                    &anynull, &status);
      fits_read_col(fptr, TFLOAT, 3, frow, felem, nelem, &floatnull, den,
                    &anynull, &status);

      for (ii = 0; ii < 6; ii++)
        printf("%5d %10s %10ld %10.2f\n", ii + 1, name[ii], dia[ii], den[ii]);
    }

    for (ii = 0; ii < 3; ii++)      /* free the memory for the column labels */
        free( ttype[ii] );

    for (ii = 0; ii < 6; ii++)      /* free the memory for the string column */
        free( name[ii] );

    if ( fits_close_file(fptr, &status) ) 
         printerror( status );

    return;
}
/*--------------------------------------------------------------------------*/
void printerror( int status)
{
    /*****************************************************/
    /* Print out cfitsio error messages and exit program */
    /*****************************************************/


    if (status)
    {
       fits_report_error(stderr, status); /* print error report */

       exit( status );    /* terminate the program, returning error status */
    }
    return;
}
