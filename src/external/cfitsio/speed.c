#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

/*
  Every program which uses the CFITSIO interface must include the
  the fitsio.h header file.  This contains the prototypes for all
  the routines and defines the error status values and other symbolic
  constants used in the interface.  
*/
#include "fitsio.h"

#define minvalue(A,B) ((A) < (B) ? (A) : (B))

/* size of the image */
#define XSIZE 3000
#define YSIZE 3000

/* size of data buffer */
#define SHTSIZE 20000
static long sarray[ SHTSIZE ] = {SHTSIZE * 0};

/* no. of rows in binary table */
#define BROWS 2500000

/* no. of rows in ASCII table */
#define AROWS 400000

/*  CLOCKS_PER_SEC should be defined by most compilers */
#if defined(CLOCKS_PER_SEC)
#define CLOCKTICKS CLOCKS_PER_SEC
#else
/* on SUN OS machine, CLOCKS_PER_SEC is not defined, so set its value */
#define CLOCKTICKS 1000000
#define difftime(A,B) ((double) A - (double) B)
#endif

/* define variables for measuring elapsed time */
clock_t scpu, ecpu;
time_t start, finish;
long startsec;   /* start of elapsed time interval */
int startmilli;  /* start of elapsed time interval */

int writeimage(fitsfile *fptr, int *status);
int writebintable(fitsfile *fptr, int *status);
int writeasctable(fitsfile *fptr, int *status);
int readimage(fitsfile *fptr, int *status);
int readatable(fitsfile *fptr, int *status);
int readbtable(fitsfile *fptr, int *status);
void printerror( int status);
int marktime(int *status);
int gettime(double *elapse, float *elapscpu, int *status);
int main(void);

int main()
{
/*************************************************************************
    This program tests the speed of writing/reading FITS files with cfitsio
**************************************************************************/

    FILE *diskfile;
    fitsfile *fptr;        /* pointer to the FITS file, defined in fitsio.h */
    int status, ii;
    long rawloop;
    char filename[] = "speedcc.fit";           /* name for new FITS file */
    char buffer[2880] = {2880 * 0};
    time_t tbegin, tend;
    float rate, size, elapcpu, cpufrac;
    double elapse;

    tbegin = time(0);

    remove(filename);               /* Delete old file if it already exists */

    diskfile =  fopen(filename,"w+b");
    rawloop = XSIZE * YSIZE / 720;

    printf("                                                ");
    printf(" SIZE / ELAPSE(%%CPU) = RATE\n");
    printf("RAW fwrite (2880 bytes/loop)...                 ");
    marktime(&status);

    for (ii = 0; ii < rawloop; ii++)
      if (fwrite(buffer, 1, 2880, diskfile) != 2880)
        printf("write error \n");

    gettime(&elapse, &elapcpu, &status);

    cpufrac = elapcpu / elapse * 100.;
    size = 2880. * rawloop / 1000000.;
    rate = size / elapse;
    printf(" %4.1fMB/%6.3fs(%3.0f) = %5.2fMB/s\n", size, elapse, cpufrac,rate);

    /* read back the binary records */
    fseek(diskfile, 0, 0);

    printf("RAW fread  (2880 bytes/loop)...                 ");
    marktime(&status);

    for (ii = 0; ii < rawloop; ii++)
      if (fread(buffer, 1, 2880, diskfile) != 2880)
        printf("read error \n");

    gettime(&elapse, &elapcpu, &status);

    cpufrac = elapcpu / elapse * 100.;
    size = 2880. * rawloop / 1000000.;
    rate = size / elapse;
    printf(" %4.1fMB/%6.3fs(%3.0f) = %5.2fMB/s\n", size, elapse, cpufrac,rate);

    fclose(diskfile);
    remove(filename);

    status = 0;     
    fptr = 0;

    if (fits_create_file(&fptr, filename, &status)) /* create new FITS file */
       printerror( status);          
   
    if (writeimage(fptr, &status))
       printerror( status);     

    if (writebintable(fptr, &status))
       printerror( status);     

    if (writeasctable(fptr, &status))
       printerror( status);     

    if (readimage(fptr, &status))
       printerror( status);     

    if (readbtable(fptr, &status))
       printerror( status);     

    if (readatable(fptr, &status))
       printerror( status);     

    if (fits_close_file(fptr, &status))     
         printerror( status );

    tend = time(0);
    elapse = difftime(tend, tbegin) + 0.5;
    printf("Total elapsed time = %.3fs, status = %d\n",elapse, status);
    return(0);
}
/*--------------------------------------------------------------------------*/
int writeimage(fitsfile *fptr, int *status)

    /**************************************************/
    /* write the primary array containing a 2-D image */
    /**************************************************/
{
    long  nremain, ii;
    float rate, size, elapcpu, cpufrac;
    double elapse;

    /* initialize FITS image parameters */
    int bitpix   =  32;   /* 32-bit  signed integer pixel values       */
    long naxis    =   2;  /* 2-dimensional image                            */    
    long naxes[2] = {XSIZE, YSIZE }; /* image size */

    /* write the required keywords for the primary array image */
    if ( fits_create_img(fptr, bitpix, naxis, naxes, status) )
         printerror( *status );          

    printf("\nWrite %dx%d I*4 image, %d pixels/loop:   ",XSIZE,YSIZE,SHTSIZE);
    marktime(status);

    nremain = XSIZE * YSIZE;
    for (ii = 1; ii <= nremain; ii += SHTSIZE)
    {
      ffpprj(fptr, 0, ii, SHTSIZE, sarray, status);
    }

    ffflus(fptr, status);  /* flush all buffers to disk */

    gettime(&elapse, &elapcpu, status);

    cpufrac = elapcpu / elapse * 100.;
    size = XSIZE * 4. * YSIZE / 1000000.;
    rate = size / elapse;
    printf(" %4.1fMB/%6.3fs(%3.0f) = %5.2fMB/s\n", size, elapse, cpufrac,rate);

    return( *status );
}
/*--------------------------------------------------------------------------*/
int writebintable (fitsfile *fptr, int *status)

    /*********************************************************/
    /* Create a binary table extension containing 3 columns  */
    /*********************************************************/
{
    int tfields = 2;
    long nremain, ntodo, firstrow = 1, firstelem = 1, nrows;
    float rate, size, elapcpu, cpufrac;
    double elapse;

    char extname[] = "Speed_Test";           /* extension name */

    /* define the name, datatype, and physical units for the columns */
    char *ttype[] = { "first", "second" };
    char *tform[] = {"1J",       "1J"   };
    char *tunit[] = { " ",       " "    };

    /* append a new empty binary table onto the FITS file */

    if ( fits_create_tbl( fptr, BINARY_TBL, BROWS, tfields, ttype, tform,
                tunit, extname, status) )
         printerror( *status );

    /* get table row size and optimum number of rows to write per loop */
    fits_get_rowsize(fptr, &nrows, status);
    nrows = minvalue(nrows, SHTSIZE);
    nremain = BROWS;

    printf("Write %7drow x %dcol bintable %4ld rows/loop:", BROWS, tfields,
       nrows);
    marktime(status);

    while(nremain)
    {
      ntodo = minvalue(nrows, nremain);
      ffpclj(fptr, 1, firstrow, firstelem, ntodo, sarray, status);
      ffpclj(fptr, 2, firstrow, firstelem, ntodo, sarray, status);
      firstrow += ntodo;
      nremain -= ntodo;
    }

    ffflus(fptr, status);  /* flush all buffers to disk */

    gettime(&elapse, &elapcpu, status);

    cpufrac = elapcpu / elapse * 100.;
    size = BROWS * 8. / 1000000.;
    rate = size / elapse;
    printf(" %4.1fMB/%6.3fs(%3.0f) = %5.2fMB/s\n", size, elapse, cpufrac,rate);

    return( *status );
}
/*--------------------------------------------------------------------------*/
int writeasctable (fitsfile *fptr, int *status)

    /*********************************************************/
    /* Create an ASCII table extension containing 2 columns  */
    /*********************************************************/
{
    int tfields = 2;
    long nremain, ntodo, firstrow = 1, firstelem = 1;
    long nrows;
    float rate, size, elapcpu, cpufrac;
    double elapse;

    char extname[] = "Speed_Test";           /* extension name */

    /* define the name, datatype, and physical units for the columns */
    char *ttype[] = { "first", "second" };
    char *tform[] = {"I6",       "I6"   };
    char *tunit[] = { " ",      " "     };

    /* append a new empty ASCII table onto the FITS file */
    if ( fits_create_tbl( fptr, ASCII_TBL, AROWS, tfields, ttype, tform,
                tunit, extname, status) )
         printerror( *status );

    /* get table row size and optimum number of rows to write per loop */
    fits_get_rowsize(fptr, &nrows, status);
    nrows = minvalue(nrows, SHTSIZE);
    nremain = AROWS;

    printf("Write %7drow x %dcol asctable %4ld rows/loop:", AROWS, tfields,
           nrows);
    marktime(status);

    while(nremain)
    {
      ntodo = minvalue(nrows, nremain);
      ffpclj(fptr, 1, firstrow, firstelem, ntodo, sarray, status);
      ffpclj(fptr, 2, firstrow, firstelem, ntodo, sarray, status);
      firstrow += ntodo;
      nremain -= ntodo;
    }

    ffflus(fptr, status);  /* flush all buffers to disk */

    gettime(&elapse, &elapcpu, status);

    cpufrac = elapcpu / elapse * 100.;
    size = AROWS * 13. / 1000000.;
    rate = size / elapse;
    printf(" %4.1fMB/%6.3fs(%3.0f) = %5.2fMB/s\n", size, elapse, cpufrac,rate);

    return( *status );
}
/*--------------------------------------------------------------------------*/
int readimage( fitsfile *fptr, int *status )

    /*********************/
    /* Read a FITS image */
    /*********************/
{
    int anynull, hdutype;
    long nremain, ii;
    long longnull = 0;
    float rate, size, elapcpu, cpufrac;
    double elapse;

    /* move to the primary array */
    if ( fits_movabs_hdu(fptr, 1, &hdutype, status) ) 
         printerror( *status );

    printf("\nRead back image                                 ");
    marktime(status);

    nremain = XSIZE * YSIZE;
    for (ii=1; ii <= nremain; ii += SHTSIZE)
    {
      ffgpvj(fptr, 0, ii, SHTSIZE, longnull, sarray, &anynull, status);
    }

    gettime(&elapse, &elapcpu, status);

    cpufrac = elapcpu / elapse * 100.;
    size = XSIZE * 4. * YSIZE / 1000000.;
    rate = size / elapse;
    printf(" %4.1fMB/%6.3fs(%3.0f) = %5.2fMB/s\n", size, elapse, cpufrac,rate);

    return( *status );
}
/*--------------------------------------------------------------------------*/
int readbtable( fitsfile *fptr, int *status )

    /************************************************************/
    /* read and print data values from the binary table */
    /************************************************************/
{
    int hdutype, anynull;
    long nremain, ntodo, firstrow = 1, firstelem = 1;
    long nrows;
    long lnull = 0;
    float rate, size, elapcpu, cpufrac;
    double elapse;

    /* move to the table */
    if ( fits_movrel_hdu(fptr, 1, &hdutype, status) ) 
           printerror( *status );

    /* get table row size and optimum number of rows to read per loop */
    fits_get_rowsize(fptr, &nrows, status);
    nrows = minvalue(nrows, SHTSIZE);
    
    /*  read the columns */  
    nremain = BROWS;

    printf("Read back BINTABLE                              ");
    marktime(status);

    while(nremain)
    {
      ntodo = minvalue(nrows, nremain);
      ffgcvj(fptr, 1, firstrow, firstelem, ntodo,
                     lnull, sarray, &anynull, status);
      ffgcvj(fptr, 2, firstrow, firstelem, ntodo,
                     lnull, sarray, &anynull, status);
      firstrow += ntodo; 
      nremain  -= ntodo;
    }

    gettime(&elapse, &elapcpu, status);

    cpufrac = elapcpu / elapse * 100.;
    size = BROWS * 8. / 1000000.;
    rate = size / elapse;
    printf(" %4.1fMB/%6.3fs(%3.0f) = %5.2fMB/s\n", size, elapse, cpufrac,rate);

    return( *status );
}
/*--------------------------------------------------------------------------*/
int readatable( fitsfile *fptr, int *status )

    /************************************************************/
    /* read and print data values from an ASCII or binary table */
    /************************************************************/
{
    int hdutype, anynull;
    long nremain, ntodo, firstrow = 1, firstelem = 1;
    long nrows;
    long lnull = 0;
    float rate, size, elapcpu, cpufrac;
    double elapse;

    /* move to the table */
    if ( fits_movrel_hdu(fptr, 1, &hdutype, status) ) 
           printerror( *status );

    /* get table row size and optimum number of rows to read per loop */
    fits_get_rowsize(fptr, &nrows, status);
    nrows = minvalue(nrows, SHTSIZE);
 
    /*  read the columns */  
    nremain = AROWS;

    printf("Read back ASCII Table                           ");
    marktime(status);

    while(nremain)
    {
      ntodo = minvalue(nrows, nremain);
      ffgcvj(fptr, 1, firstrow, firstelem, ntodo,
                     lnull, sarray, &anynull, status);
      ffgcvj(fptr, 2, firstrow, firstelem, ntodo,
                     lnull, sarray, &anynull, status);
      firstrow += ntodo;
      nremain  -= ntodo;
    }

    gettime(&elapse, &elapcpu, status);

    cpufrac = elapcpu / elapse * 100.;
    size = AROWS * 13. / 1000000.;
    rate = size / elapse;
    printf(" %4.1fMB/%6.3fs(%3.0f) = %5.2fMB/s\n", size, elapse, cpufrac,rate);

    return( *status );
}
/*--------------------------------------------------------------------------*/
void printerror( int status)
{
    /*****************************************************/
    /* Print out cfitsio error messages and exit program */
    /*****************************************************/

    char status_str[FLEN_STATUS], errmsg[FLEN_ERRMSG];
  
    if (status)
      fprintf(stderr, "\n*** Error occurred during program execution ***\n");

    fits_get_errstatus(status, status_str);   /* get the error description */
    fprintf(stderr, "\nstatus = %d: %s\n", status, status_str);

    /* get first message; null if stack is empty */
    if ( fits_read_errmsg(errmsg) ) 
    {
         fprintf(stderr, "\nError message stack:\n");
         fprintf(stderr, " %s\n", errmsg);

         while ( fits_read_errmsg(errmsg) )  /* get remaining messages */
             fprintf(stderr, " %s\n", errmsg);
    }

    exit( status );       /* terminate the program, returning error status */
}
/*--------------------------------------------------------------------------*/
int marktime( int *status)
{
    double telapse;
    time_t temp;
    struct  timeval tv;

    temp = time(0);

    /* Since elapsed time is only measured to the nearest second */
    /* keep getting the time until the seconds tick just changes. */
    /* This provides more consistent timing measurements since the */
    /* intervals all start on an integer seconds. */

    telapse = 0.;

        scpu = clock();
        start = time(0);
/*
    while (telapse == 0.)
    {
        scpu = clock();
        start = time(0);
        telapse = difftime( start, temp );
    }
*/
        gettimeofday (&tv, NULL);

	startsec = tv.tv_sec;
        startmilli = tv.tv_usec/1000;

    return( *status );
}
/*--------------------------------------------------------------------------*/
int gettime(double *elapse, float *elapscpu, int *status)
{
        struct  timeval tv;
	int stopmilli;
	long stopsec;


        gettimeofday (&tv, NULL);
    ecpu = clock();
    finish = time(0);

        stopmilli = tv.tv_usec/1000;
	stopsec = tv.tv_sec;
	

	*elapse = (stopsec - startsec) + (stopmilli - startmilli)/1000.;

/*    *elapse = difftime(finish, start) + 0.5; */
    *elapscpu = (ecpu - scpu) * 1.0 / CLOCKTICKS;

    return( *status );
}
