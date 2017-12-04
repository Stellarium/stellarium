#include <stdio.h>
#include "fitsio.h"

int main(int argc, char *argv[])
{
    fitsfile *infptr, *outfptr;   /* FITS file pointers defined in fitsio.h */
    int status = 0;       /* status must always be initialized = 0  */

    if (argc != 3)
    {
 printf("Usage:  fitscopy inputfile outputfile\n");
 printf("\n");
 printf("Copy an input file to an output file, optionally filtering\n");
 printf("the file in the process.  This seemingly simple program can\n");
 printf("apply powerful filters which transform the input file as\n");
 printf("it is being copied.  Filters may be used to extract a\n");
 printf("subimage from a larger image, select rows from a table,\n");
 printf("filter a table with a GTI time extension or a SAO region file,\n");
 printf("create or delete columns in a table, create an image by\n");
 printf("binning (histogramming) 2 table columns, and convert IRAF\n");
 printf("format *.imh or raw binary data files into FITS images.\n");
 printf("See the CFITSIO User's Guide for a complete description of\n");
 printf("the Extended File Name filtering syntax.\n");
 printf("\n");
 printf("Examples:\n");
 printf("\n");
 printf("fitscopy in.fit out.fit                   (simple file copy)\n");
 printf("fitscopy - -                              (stdin to stdout)\n");
 printf("fitscopy in.fit[11:50,21:60] out.fit      (copy a subimage)\n");
 printf("fitscopy iniraf.imh out.fit               (IRAF image to FITS)\n");
 printf("fitscopy in.dat[i512,512] out.fit         (raw array to FITS)\n");
 printf("fitscopy in.fit[events][pi>35] out.fit    (copy rows with pi>35)\n");
 printf("fitscopy in.fit[events][bin X,Y] out.fit  (bin an image) \n");
 printf("fitscopy in.fit[events][col x=.9*y] out.fit        (new x column)\n");
 printf("fitscopy in.fit[events][gtifilter()] out.fit       (time filter)\n");
 printf("fitscopy in.fit[2][regfilter(\"pow.reg\")] out.fit (spatial filter)\n");
 printf("\n");
 printf("Note that it may be necessary to enclose the input file name\n");
 printf("in single quote characters on the Unix command line.\n");
      return(0);
    }
    /* Open the input file */
    if ( !fits_open_file(&infptr, argv[1], READONLY, &status) )
    {
      /* Create the output file */
      if ( !fits_create_file(&outfptr, argv[2], &status) )
      {
 
        /* copy the previous, current, and following HDUs */
        fits_copy_file(infptr, outfptr, 1, 1, 1, &status);

        fits_close_file(outfptr,  &status);
      }
      fits_close_file(infptr, &status);
    }

    /* if error occured, print out error message */
    if (status) fits_report_error(stderr, status);
    return(status);
}
