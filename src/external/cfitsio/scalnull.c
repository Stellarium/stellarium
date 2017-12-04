/*  This file, scalnull.c, contains the FITSIO routines used to define     */
/*  the starting heap address, the value scaling and the null values.      */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include "fitsio2.h"
/*--------------------------------------------------------------------------*/
int ffpthp(fitsfile *fptr,      /* I - FITS file pointer */
           long theap,          /* I - starting addrss for the heap */
           int *status)         /* IO - error status     */
/*
  Define the starting address for the heap for a binary table.
  The default address is NAXIS1 * NAXIS2.  It is in units of
  bytes relative to the beginning of the regular binary table data.
  This routine also writes the appropriate THEAP keyword to the
  FITS header.
*/
{
    if (*status > 0 || theap < 1)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    (fptr->Fptr)->heapstart = theap;

    ffukyj(fptr, "THEAP", theap, "byte offset to heap area", status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpscl(fitsfile *fptr,      /* I - FITS file pointer               */
           double scale,        /* I - scaling factor: value of BSCALE */
           double zero,         /* I - zero point: value of BZERO      */
           int *status)         /* IO - error status                   */
/*
  Define the linear scaling factor for the primary array or image extension
  pixel values. This routine overrides the scaling values given by the
  BSCALE and BZERO keywords if present.  Note that this routine does not
  write or modify the BSCALE and BZERO keywords, but instead only modifies
  the values temporarily in the internal buffer.  Thus, a subsequent call to
  the ffrdef routine will reset the scaling back to the BSCALE and BZERO
  keyword values (or 1. and 0. respectively if the keywords are not present).
*/
{
    tcolumn *colptr;
    int hdutype;

    if (*status > 0)
        return(*status);

    if (scale == 0)
        return(*status = ZERO_SCALE);  /* zero scale value is illegal */

    if (ffghdt(fptr, &hdutype, status) > 0)  /* get HDU type */
        return(*status);

    if (hdutype != IMAGE_HDU)
        return(*status = NOT_IMAGE);         /* not proper HDU type */

    if (fits_is_compressed_image(fptr, status)) /* compressed images */
    {
        (fptr->Fptr)->cn_bscale = scale;
        (fptr->Fptr)->cn_bzero  = zero;
        return(*status);
    }

    /* set pointer to the first 'column' (contains group parameters if any) */
    colptr = (fptr->Fptr)->tableptr; 

    colptr++;   /* increment to the 2nd 'column' pointer  (the image itself) */

    colptr->tscale = scale;
    colptr->tzero = zero;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpnul(fitsfile *fptr,      /* I - FITS file pointer                */
           LONGLONG nulvalue,   /* I - null pixel value: value of BLANK */
           int *status)         /* IO - error status                    */
/*
  Define the value used to represent undefined pixels in the primary array or
  image extension. This only applies to integer image pixel (i.e. BITPIX > 0).
  This routine overrides the null pixel value given by the BLANK keyword
  if present.  Note that this routine does not write or modify the BLANK
  keyword, but instead only modifies the value temporarily in the internal
  buffer. Thus, a subsequent call to the ffrdef routine will reset the null
  value back to the BLANK  keyword value (or not defined if the keyword is not
  present).
*/
{
    tcolumn *colptr;
    int hdutype;

    if (*status > 0)
        return(*status);

    if (ffghdt(fptr, &hdutype, status) > 0)  /* get HDU type */
        return(*status);

    if (hdutype != IMAGE_HDU)
        return(*status = NOT_IMAGE);         /* not proper HDU type */

    if (fits_is_compressed_image(fptr, status)) /* ignore compressed images */
        return(*status);

    /* set pointer to the first 'column' (contains group parameters if any) */
    colptr = (fptr->Fptr)->tableptr; 

    colptr++;   /* increment to the 2nd 'column' pointer  (the image itself) */

    colptr->tnull = nulvalue;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fftscl(fitsfile *fptr,      /* I - FITS file pointer */
           int colnum,          /* I - column number to apply scaling to */
           double scale,        /* I - scaling factor: value of TSCALn   */
           double zero,         /* I - zero point: value of TZEROn       */
           int *status)         /* IO - error status     */
/*
  Define the linear scaling factor for the TABLE or BINTABLE extension
  column values. This routine overrides the scaling values given by the
  TSCALn and TZEROn keywords if present.  Note that this routine does not
  write or modify the TSCALn and TZEROn keywords, but instead only modifies
  the values temporarily in the internal buffer.  Thus, a subsequent call to
  the ffrdef routine will reset the scaling back to the TSCALn and TZEROn
  keyword values (or 1. and 0. respectively if the keywords are not present).
*/
{
    tcolumn *colptr;
    int hdutype;

    if (*status > 0)
        return(*status);

    if (scale == 0)
        return(*status = ZERO_SCALE);  /* zero scale value is illegal */

    if (ffghdt(fptr, &hdutype, status) > 0)  /* get HDU type */
        return(*status);

    if (hdutype == IMAGE_HDU)
        return(*status = NOT_TABLE);         /* not proper HDU type */

    colptr = (fptr->Fptr)->tableptr;   /* set pointer to the first column */
    colptr += (colnum - 1);     /* increment to the correct column */

    colptr->tscale = scale;
    colptr->tzero = zero;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fftnul(fitsfile *fptr,      /* I - FITS file pointer                  */
           int colnum,          /* I - column number to apply nulvalue to */
           LONGLONG nulvalue,   /* I - null pixel value: value of TNULLn  */
           int *status)         /* IO - error status                      */
/*
  Define the value used to represent undefined pixels in the BINTABLE column.
  This only applies to integer datatype columns (TFORM = B, I, or J).
  This routine overrides the null pixel value given by the TNULLn keyword
  if present.  Note that this routine does not write or modify the TNULLn
  keyword, but instead only modifies the value temporarily in the internal
  buffer. Thus, a subsequent call to the ffrdef routine will reset the null
  value back to the TNULLn  keyword value (or not defined if the keyword is not
  present).
*/
{
    tcolumn *colptr;
    int hdutype;

    if (*status > 0)
        return(*status);

    if (ffghdt(fptr, &hdutype, status) > 0)  /* get HDU type */
        return(*status);

    if (hdutype != BINARY_TBL)
        return(*status = NOT_BTABLE);        /* not proper HDU type */
 
    colptr = (fptr->Fptr)->tableptr;   /* set pointer to the first column */
    colptr += (colnum - 1);    /* increment to the correct column */

    colptr->tnull = nulvalue;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffsnul(fitsfile *fptr,      /* I - FITS file pointer                  */
           int colnum,          /* I - column number to apply nulvalue to */
           char *nulstring,     /* I - null pixel value: value of TNULLn  */
           int *status)         /* IO - error status                      */
/*
  Define the string used to represent undefined pixels in the ASCII TABLE
  column. This routine overrides the null  value given by the TNULLn keyword
  if present.  Note that this routine does not write or modify the TNULLn
  keyword, but instead only modifies the value temporarily in the internal
  buffer. Thus, a subsequent call to the ffrdef routine will reset the null
  value back to the TNULLn keyword value (or not defined if the keyword is not
  present).
*/
{
    tcolumn *colptr;
    int hdutype;

    if (*status > 0)
        return(*status);

    if (ffghdt(fptr, &hdutype, status) > 0)  /* get HDU type */
        return(*status);

    if (hdutype != ASCII_TBL)
        return(*status = NOT_ATABLE);        /* not proper HDU type */
 
    colptr = (fptr->Fptr)->tableptr;   /* set pointer to the first column */
    colptr += (colnum - 1);    /* increment to the correct column */

    colptr->strnull[0] = '\0';
    strncat(colptr->strnull, nulstring, 19);  /* limit string to 19 chars */

    return(*status);
}
