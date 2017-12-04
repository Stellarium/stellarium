
/*  This file, getcol.c, contains routines that read data elements from    */
/*  a FITS image or table.  There are generic datatype routines.           */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <stdlib.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int ffgpxv( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            long *firstpix,   /* I - coord of first pixel to read (1s based) */
            LONGLONG nelem,   /* I - number of values to read                */
            void *nulval,     /* I - value for undefined pixels              */
            void *array,      /* O - array of values that are returned       */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Undefined elements will be set equal to NULVAL, unless NULVAL=0
  in which case no checking for undefined values will be performed.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    LONGLONG tfirstpix[99];
    int naxis, ii;

    if (*status > 0 || nelem == 0)   /* inherit input status value if > 0 */
        return(*status);

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);
    
    for (ii=0; ii < naxis; ii++)
       tfirstpix[ii] = firstpix[ii];

    ffgpxvll(fptr, datatype, tfirstpix, nelem, nulval, array, anynul, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgpxvll( fitsfile *fptr, /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            LONGLONG *firstpix, /* I - coord of first pixel to read (1s based) */
            LONGLONG nelem,   /* I - number of values to read                */
            void *nulval,     /* I - value for undefined pixels              */
            void *array,      /* O - array of values that are returned       */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Undefined elements will be set equal to NULVAL, unless NULVAL=0
  in which case no checking for undefined values will be performed.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    int naxis, ii;
    char cdummy;
    int nullcheck = 1;
    LONGLONG naxes[9], trc[9]= {1,1,1,1,1,1,1,1,1};
    long inc[9]= {1,1,1,1,1,1,1,1,1};
    LONGLONG dimsize = 1, firstelem;

    if (*status > 0 || nelem == 0)   /* inherit input status value if > 0 */
        return(*status);

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);

    ffgiszll(fptr, 9, naxes, status);

    if (naxis == 0 || naxes[0] == 0) {
       *status = BAD_DIMEN;
       return(*status);
    }

    /* calculate the position of the first element in the array */
    firstelem = 0;
    for (ii=0; ii < naxis; ii++)
    {
        firstelem += ((firstpix[ii] - 1) * dimsize);
        dimsize *= naxes[ii];
        trc[ii] = firstpix[ii];
    }
    firstelem++;

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        /* test for special case of reading an integral number of */
        /* rows in a 2D or 3D image (which includes reading the whole image */

	if (naxis > 1 && naxis < 4 && firstpix[0] == 1 &&
            (nelem / naxes[0]) * naxes[0] == nelem) {

                /* calculate coordinate of last pixel */
		trc[0] = naxes[0];  /* reading whole rows */
		trc[1] = firstpix[1] + (nelem / naxes[0] - 1);
                while (trc[1] > naxes[1])  {
		    trc[1] = trc[1] - naxes[1];
		    trc[2] = trc[2] + 1;  /* increment to next plane of cube */
                }

                fits_read_compressed_img(fptr, datatype, firstpix, trc, inc,
                   1, nulval, array, NULL, anynul, status);

        } else {

                fits_read_compressed_pixels(fptr, datatype, firstelem,
                   nelem, nullcheck, nulval, array, NULL, anynul, status);
        }

        return(*status);
    }

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    if (datatype == TBYTE)
    {
      if (nulval == 0)
        ffgclb(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (unsigned char *) array, &cdummy, anynul, status);
      else
        ffgclb(fptr, 2, 1, firstelem, nelem, 1, 1, *(unsigned char *) nulval,
               (unsigned char *) array, &cdummy, anynul, status);
    }
    else if (datatype == TSBYTE)
    {
      if (nulval == 0)
        ffgclsb(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (signed char *) array, &cdummy, anynul, status);
      else
        ffgclsb(fptr, 2, 1, firstelem, nelem, 1, 1, *(signed char *) nulval,
               (signed char *) array, &cdummy, anynul, status);
    }
    else if (datatype == TUSHORT)
    {
      if (nulval == 0)
        ffgclui(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (unsigned short *) array, &cdummy, anynul, status);
      else
        ffgclui(fptr, 2, 1, firstelem, nelem, 1, 1, *(unsigned short *) nulval,
               (unsigned short *) array, &cdummy, anynul, status);
    }
    else if (datatype == TSHORT)
    {
      if (nulval == 0)
        ffgcli(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (short *) array, &cdummy, anynul, status);
      else
        ffgcli(fptr, 2, 1, firstelem, nelem, 1, 1, *(short *) nulval,
               (short *) array, &cdummy, anynul, status);
    }
    else if (datatype == TUINT)
    {
      if (nulval == 0)
        ffgcluk(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (unsigned int *) array, &cdummy, anynul, status);
      else
        ffgcluk(fptr, 2, 1, firstelem, nelem, 1, 1, *(unsigned int *) nulval,
               (unsigned int *) array, &cdummy, anynul, status);
    }
    else if (datatype == TINT)
    {
      if (nulval == 0)
        ffgclk(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (int *) array, &cdummy, anynul, status);
      else
        ffgclk(fptr, 2, 1, firstelem, nelem, 1, 1, *(int *) nulval,
               (int *) array, &cdummy, anynul, status);
    }
    else if (datatype == TULONG)
    {
      if (nulval == 0)
        ffgcluj(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (unsigned long *) array, &cdummy, anynul, status);
      else
        ffgcluj(fptr, 2, 1, firstelem, nelem, 1, 1, *(unsigned long *) nulval,
               (unsigned long *) array, &cdummy, anynul, status);
    }
    else if (datatype == TLONG)
    {
      if (nulval == 0)
        ffgclj(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (long *) array, &cdummy, anynul, status);
      else
        ffgclj(fptr, 2, 1, firstelem, nelem, 1, 1, *(long *) nulval,
               (long *) array, &cdummy, anynul, status);
    }
    else if (datatype == TLONGLONG)
    {
      if (nulval == 0)
        ffgcljj(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (LONGLONG *) array, &cdummy, anynul, status);
      else
        ffgcljj(fptr, 2, 1, firstelem, nelem, 1, 1, *(LONGLONG *) nulval,
               (LONGLONG *) array, &cdummy, anynul, status);
    }
    else if (datatype == TFLOAT)
    {
      if (nulval == 0)
        ffgcle(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (float *) array, &cdummy, anynul, status);
      else
        ffgcle(fptr, 2, 1, firstelem, nelem, 1, 1, *(float *) nulval,
               (float *) array, &cdummy, anynul, status);
    }
    else if (datatype == TDOUBLE)
    {
      if (nulval == 0)
        ffgcld(fptr, 2, 1, firstelem, nelem, 1, 1, 0,
               (double *) array, &cdummy, anynul, status);
      else
        ffgcld(fptr, 2, 1, firstelem, nelem, 1, 1, *(double *) nulval,
               (double *) array, &cdummy, anynul, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgpxf( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            long *firstpix,   /* I - coord of first pixel to read (1s based) */
            LONGLONG nelem,       /* I - number of values to read            */
            void *array,      /* O - array of values that are returned       */
            char *nullarray,  /* O - returned array of null value flags      */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  The nullarray values will = 1 if the corresponding array value is null.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    LONGLONG tfirstpix[99];
    int naxis, ii;

    if (*status > 0 || nelem == 0)   /* inherit input status value if > 0 */
        return(*status);

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);

    for (ii=0; ii < naxis; ii++)
       tfirstpix[ii] = firstpix[ii];

    ffgpxfll(fptr, datatype, tfirstpix, nelem, array, nullarray, anynul, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgpxfll( fitsfile *fptr, /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            LONGLONG *firstpix, /* I - coord of first pixel to read (1s based) */
            LONGLONG nelem,       /* I - number of values to read              */
            void *array,      /* O - array of values that are returned       */
            char *nullarray,  /* O - returned array of null value flags      */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  The nullarray values will = 1 if the corresponding array value is null.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    int naxis, ii;
    int nullcheck = 2;
    LONGLONG naxes[9];
    LONGLONG dimsize = 1, firstelem;

    if (*status > 0 || nelem == 0)   /* inherit input status value if > 0 */
        return(*status);

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);
    ffgiszll(fptr, 9, naxes, status);

    /* calculate the position of the first element in the array */
    firstelem = 0;
    for (ii=0; ii < naxis; ii++)
    {
        firstelem += ((firstpix[ii] - 1) * dimsize);
        dimsize *= naxes[ii];
    }
    firstelem++;

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        fits_read_compressed_pixels(fptr, datatype, firstelem, nelem,
            nullcheck, NULL, array, nullarray, anynul, status);
        return(*status);
    }

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    if (datatype == TBYTE)
    {
        ffgclb(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (unsigned char *) array, nullarray, anynul, status);
    }
    else if (datatype == TSBYTE)
    {
        ffgclsb(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (signed char *) array, nullarray, anynul, status);
    }
    else if (datatype == TUSHORT)
    {
        ffgclui(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (unsigned short *) array, nullarray, anynul, status);
    }
    else if (datatype == TSHORT)
    {
        ffgcli(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (short *) array, nullarray, anynul, status);
    }
    else if (datatype == TUINT)
    {
        ffgcluk(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (unsigned int *) array, nullarray, anynul, status);
    }
    else if (datatype == TINT)
    {
        ffgclk(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (int *) array, nullarray, anynul, status);
    }
    else if (datatype == TULONG)
    {
        ffgcluj(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (unsigned long *) array, nullarray, anynul, status);
    }
    else if (datatype == TLONG)
    {
        ffgclj(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (long *) array, nullarray, anynul, status);
    }
    else if (datatype == TLONGLONG)
    {
        ffgcljj(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (LONGLONG *) array, nullarray, anynul, status);
    }
    else if (datatype == TFLOAT)
    {
        ffgcle(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (float *) array, nullarray, anynul, status);
    }
    else if (datatype == TDOUBLE)
    {
        ffgcld(fptr, 2, 1, firstelem, nelem, 1, 2, 0,
               (double *) array, nullarray, anynul, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgsv(  fitsfile *fptr,   /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            long *blc,        /* I - 'bottom left corner' of the subsection  */
            long *trc ,       /* I - 'top right corner' of the subsection    */
            long *inc,        /* I - increment to be applied in each dim.    */
            void *nulval,     /* I - value for undefined pixels              */
            void *array,      /* O - array of values that are returned       */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an section of values from the primary array. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Undefined elements will be set equal to NULVAL, unless NULVAL=0
  in which case no checking for undefined values will be performed.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    int naxis, ii;
    long naxes[9];
    LONGLONG nelem = 1;

    if (*status > 0)   /* inherit input status value if > 0 */
        return(*status);

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);
    ffgisz(fptr, 9, naxes, status);

    /* test for the important special case where we are reading the whole image */
    /* this is only useful for images that are not tile-compressed */
    if (!fits_is_compressed_image(fptr, status)) {
        for (ii = 0; ii < naxis; ii++) {
            if (inc[ii] != 1 || blc[ii] !=1 || trc[ii] != naxes[ii])
                break;

            nelem = nelem * naxes[ii];
        }

        if (ii == naxis) {
            /* read the whole image more efficiently */
            ffgpxv(fptr, datatype, blc, nelem, nulval, array, anynul, status);
            return(*status);
        }
    }

    if (datatype == TBYTE)
    {
      if (nulval == 0)
        ffgsvb(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (unsigned char *) array, anynul, status);
      else
        ffgsvb(fptr, 1, naxis, naxes, blc, trc, inc, *(unsigned char *) nulval,
               (unsigned char *) array, anynul, status);
    }
    else if (datatype == TSBYTE)
    {
      if (nulval == 0)
        ffgsvsb(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (signed char *) array, anynul, status);
      else
        ffgsvsb(fptr, 1, naxis, naxes, blc, trc, inc, *(signed char *) nulval,
               (signed char *) array, anynul, status);
    }
    else if (datatype == TUSHORT)
    {
      if (nulval == 0)
        ffgsvui(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (unsigned short *) array, anynul, status);
      else
        ffgsvui(fptr, 1, naxis, naxes,blc, trc, inc, *(unsigned short *) nulval,
               (unsigned short *) array, anynul, status);
    }
    else if (datatype == TSHORT)
    {
      if (nulval == 0)
        ffgsvi(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (short *) array, anynul, status);
      else
        ffgsvi(fptr, 1, naxis, naxes, blc, trc, inc, *(short *) nulval,
               (short *) array, anynul, status);
    }
    else if (datatype == TUINT)
    {
      if (nulval == 0)
        ffgsvuk(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (unsigned int *) array, anynul, status);
      else
        ffgsvuk(fptr, 1, naxis, naxes, blc, trc, inc, *(unsigned int *) nulval,
               (unsigned int *) array, anynul, status);
    }
    else if (datatype == TINT)
    {
      if (nulval == 0)
        ffgsvk(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (int *) array, anynul, status);
      else
        ffgsvk(fptr, 1, naxis, naxes, blc, trc, inc, *(int *) nulval,
               (int *) array, anynul, status);
    }
    else if (datatype == TULONG)
    {
      if (nulval == 0)
        ffgsvuj(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (unsigned long *) array, anynul, status);
      else
        ffgsvuj(fptr, 1, naxis, naxes, blc, trc, inc, *(unsigned long *) nulval,
               (unsigned long *) array, anynul, status);
    }
    else if (datatype == TLONG)
    {
      if (nulval == 0)
        ffgsvj(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (long *) array, anynul, status);
      else
        ffgsvj(fptr, 1, naxis, naxes, blc, trc, inc, *(long *) nulval,
               (long *) array, anynul, status);
    }
    else if (datatype == TLONGLONG)
    {
      if (nulval == 0)
        ffgsvjj(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (LONGLONG *) array, anynul, status);
      else
        ffgsvjj(fptr, 1, naxis, naxes, blc, trc, inc, *(LONGLONG *) nulval,
               (LONGLONG *) array, anynul, status);
    }
    else if (datatype == TFLOAT)
    {
      if (nulval == 0)
        ffgsve(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (float *) array, anynul, status);
      else
        ffgsve(fptr, 1, naxis, naxes, blc, trc, inc, *(float *) nulval,
               (float *) array, anynul, status);
    }
    else if (datatype == TDOUBLE)
    {
      if (nulval == 0)
        ffgsvd(fptr, 1, naxis, naxes, blc, trc, inc, 0,
               (double *) array, anynul, status);
      else
        ffgsvd(fptr, 1, naxis, naxes, blc, trc, inc, *(double *) nulval,
               (double *) array, anynul, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgpv(  fitsfile *fptr,   /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            LONGLONG firstelem,   /* I - first vector element to read (1 = 1st)  */
            LONGLONG nelem,       /* I - number of values to read                */
            void *nulval,     /* I - value for undefined pixels              */
            void *array,      /* O - array of values that are returned       */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Undefined elements will be set equal to NULVAL, unless NULVAL=0
  in which case no checking for undefined values will be performed.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{

    if (*status > 0 || nelem == 0)   /* inherit input status value if > 0 */
        return(*status);

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    if (datatype == TBYTE)
    {
      if (nulval == 0)
        ffgpvb(fptr, 1, firstelem, nelem, 0,
               (unsigned char *) array, anynul, status);
      else
        ffgpvb(fptr, 1, firstelem, nelem, *(unsigned char *) nulval,
               (unsigned char *) array, anynul, status);
    }
    else if (datatype == TSBYTE)
    {
      if (nulval == 0)
        ffgpvsb(fptr, 1, firstelem, nelem, 0,
               (signed char *) array, anynul, status);
      else
        ffgpvsb(fptr, 1, firstelem, nelem, *(signed char *) nulval,
               (signed char *) array, anynul, status);
    }
    else if (datatype == TUSHORT)
    {
      if (nulval == 0)
        ffgpvui(fptr, 1, firstelem, nelem, 0,
               (unsigned short *) array, anynul, status);
      else
        ffgpvui(fptr, 1, firstelem, nelem, *(unsigned short *) nulval,
               (unsigned short *) array, anynul, status);
    }
    else if (datatype == TSHORT)
    {
      if (nulval == 0)
        ffgpvi(fptr, 1, firstelem, nelem, 0,
               (short *) array, anynul, status);
      else
        ffgpvi(fptr, 1, firstelem, nelem, *(short *) nulval,
               (short *) array, anynul, status);
    }
    else if (datatype == TUINT)
    {
      if (nulval == 0)
        ffgpvuk(fptr, 1, firstelem, nelem, 0,
               (unsigned int *) array, anynul, status);
      else
        ffgpvuk(fptr, 1, firstelem, nelem, *(unsigned int *) nulval,
               (unsigned int *) array, anynul, status);
    }
    else if (datatype == TINT)
    {
      if (nulval == 0)
        ffgpvk(fptr, 1, firstelem, nelem, 0,
               (int *) array, anynul, status);
      else
        ffgpvk(fptr, 1, firstelem, nelem, *(int *) nulval,
               (int *) array, anynul, status);
    }
    else if (datatype == TULONG)
    {
      if (nulval == 0)
        ffgpvuj(fptr, 1, firstelem, nelem, 0,
               (unsigned long *) array, anynul, status);
      else
        ffgpvuj(fptr, 1, firstelem, nelem, *(unsigned long *) nulval,
               (unsigned long *) array, anynul, status);
    }
    else if (datatype == TLONG)
    {
      if (nulval == 0)
        ffgpvj(fptr, 1, firstelem, nelem, 0,
               (long *) array, anynul, status);
      else
        ffgpvj(fptr, 1, firstelem, nelem, *(long *) nulval,
               (long *) array, anynul, status);
    }
    else if (datatype == TLONGLONG)
    {
      if (nulval == 0)
        ffgpvjj(fptr, 1, firstelem, nelem, 0,
               (LONGLONG *) array, anynul, status);
      else
        ffgpvjj(fptr, 1, firstelem, nelem, *(LONGLONG *) nulval,
               (LONGLONG *) array, anynul, status);
    }
    else if (datatype == TFLOAT)
    {
      if (nulval == 0)
        ffgpve(fptr, 1, firstelem, nelem, 0,
               (float *) array, anynul, status);
      else
        ffgpve(fptr, 1, firstelem, nelem, *(float *) nulval,
               (float *) array, anynul, status);
    }
    else if (datatype == TDOUBLE)
    {
      if (nulval == 0)
        ffgpvd(fptr, 1, firstelem, nelem, 0,
               (double *) array, anynul, status);
      else
      {
        ffgpvd(fptr, 1, firstelem, nelem, *(double *) nulval,
               (double *) array, anynul, status);
      }
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgpf(  fitsfile *fptr,   /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            LONGLONG firstelem,   /* I - first vector element to read (1 = 1st)  */
            LONGLONG nelem,       /* I - number of values to read                */
            void *array,      /* O - array of values that are returned       */
            char *nullarray,  /* O - array of null value flags               */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  The nullarray values will = 1 if the corresponding array value is null.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{

    if (*status > 0 || nelem == 0)   /* inherit input status value if > 0 */
        return(*status);

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    if (datatype == TBYTE)
    {
        ffgpfb(fptr, 1, firstelem, nelem, 
               (unsigned char *) array, nullarray, anynul, status);
    }
    else if (datatype == TSBYTE)
    {
        ffgpfsb(fptr, 1, firstelem, nelem, 
               (signed char *) array, nullarray, anynul, status);
    }
    else if (datatype == TUSHORT)
    {
        ffgpfui(fptr, 1, firstelem, nelem, 
               (unsigned short *) array, nullarray, anynul, status);
    }
    else if (datatype == TSHORT)
    {
        ffgpfi(fptr, 1, firstelem, nelem, 
               (short *) array, nullarray, anynul, status);
    }
    else if (datatype == TUINT)
    {
        ffgpfuk(fptr, 1, firstelem, nelem, 
               (unsigned int *) array, nullarray, anynul, status);
    }
    else if (datatype == TINT)
    {
        ffgpfk(fptr, 1, firstelem, nelem, 
               (int *) array, nullarray, anynul, status);
    }
    else if (datatype == TULONG)
    {
        ffgpfuj(fptr, 1, firstelem, nelem, 
               (unsigned long *) array, nullarray, anynul, status);
    }
    else if (datatype == TLONG)
    {
        ffgpfj(fptr, 1, firstelem, nelem,
               (long *) array, nullarray, anynul, status);
    }
    else if (datatype == TLONGLONG)
    {
        ffgpfjj(fptr, 1, firstelem, nelem,
               (LONGLONG *) array, nullarray, anynul, status);
    }
    else if (datatype == TFLOAT)
    {
        ffgpfe(fptr, 1, firstelem, nelem, 
               (float *) array, nullarray, anynul, status);
    }
    else if (datatype == TDOUBLE)
    {
        ffgpfd(fptr, 1, firstelem, nelem,
               (double *) array, nullarray, anynul, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcv(  fitsfile *fptr,   /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            int  colnum,      /* I - number of column to write (1 = 1st col) */
            LONGLONG  firstrow,   /* I - first row to write (1 = 1st row)        */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG nelem,       /* I - number of values to read                */
            void *nulval,     /* I - value for undefined pixels              */
            void *array,      /* O - array of values that are returned       */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from a table column. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Undefined elements will be set equal to NULVAL, unless NULVAL=0
  in which case no checking for undefined values will be performed.
  ANYNUL is returned with a value of true if any pixels are undefined.
*/
{
    char cdummy[2];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (datatype == TBIT)
    {
      ffgcx(fptr, colnum, firstrow, firstelem, nelem, (char *) array, status);
    }
    else if (datatype == TBYTE)
    {
      if (nulval == 0)
        ffgclb(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
              (unsigned char *) array, cdummy, anynul, status);
      else
       ffgclb(fptr, colnum, firstrow, firstelem, nelem, 1, 1, *(unsigned char *)
              nulval, (unsigned char *) array, cdummy, anynul, status);
    }
    else if (datatype == TSBYTE)
    {
      if (nulval == 0)
        ffgclsb(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
              (signed char *) array, cdummy, anynul, status);
      else
       ffgclsb(fptr, colnum, firstrow, firstelem, nelem, 1, 1, *(signed char *)
              nulval, (signed char *) array, cdummy, anynul, status);
    }
    else if (datatype == TUSHORT)
    {
      if (nulval == 0)
        ffgclui(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
               (unsigned short *) array, cdummy, anynul, status);
      else
        ffgclui(fptr, colnum, firstrow, firstelem, nelem, 1, 1,
               *(unsigned short *) nulval,
               (unsigned short *) array, cdummy, anynul, status);
    }
    else if (datatype == TSHORT)
    {
      if (nulval == 0)
        ffgcli(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
              (short *) array, cdummy, anynul, status);
      else
        ffgcli(fptr, colnum, firstrow, firstelem, nelem, 1, 1, *(short *)
              nulval, (short *) array, cdummy, anynul, status);
    }
    else if (datatype == TUINT)
    {
      if (nulval == 0)
        ffgcluk(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
              (unsigned int *) array, cdummy, anynul, status);
      else
        ffgcluk(fptr, colnum, firstrow, firstelem, nelem, 1, 1,
         *(unsigned int *) nulval, (unsigned int *) array, cdummy, anynul,
         status);
    }
    else if (datatype == TINT)
    {
      if (nulval == 0)
        ffgclk(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
              (int *) array, cdummy, anynul, status);
      else
        ffgclk(fptr, colnum, firstrow, firstelem, nelem, 1, 1, *(int *)
            nulval, (int *) array, cdummy, anynul, status);
    }
    else if (datatype == TULONG)
    {
      if (nulval == 0)
        ffgcluj(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
               (unsigned long *) array, cdummy, anynul, status);
      else
        ffgcluj(fptr, colnum, firstrow, firstelem, nelem, 1, 1,
               *(unsigned long *) nulval, 
               (unsigned long *) array, cdummy, anynul, status);
    }
    else if (datatype == TLONG)
    {
      if (nulval == 0)
        ffgclj(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
              (long *) array, cdummy, anynul, status);
      else
        ffgclj(fptr, colnum, firstrow, firstelem, nelem, 1, 1, *(long *)
              nulval, (long *) array, cdummy, anynul, status);
    }
    else if (datatype == TLONGLONG)
    {
      if (nulval == 0)
        ffgcljj(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0,
              (LONGLONG *) array, cdummy, anynul, status);
      else
        ffgcljj(fptr, colnum, firstrow, firstelem, nelem, 1, 1, *(LONGLONG *)
              nulval, (LONGLONG *) array, cdummy, anynul, status);
    }
    else if (datatype == TFLOAT)
    {
      if (nulval == 0)
        ffgcle(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0.,
              (float *) array, cdummy, anynul, status);
      else
      ffgcle(fptr, colnum, firstrow, firstelem, nelem, 1, 1, *(float *)
               nulval,(float *) array, cdummy, anynul, status);
    }
    else if (datatype == TDOUBLE)
    {
      if (nulval == 0)
        ffgcld(fptr, colnum, firstrow, firstelem, nelem, 1, 1, 0.,
              (double *) array, cdummy, anynul, status);
      else
        ffgcld(fptr, colnum, firstrow, firstelem, nelem, 1, 1, *(double *)
              nulval, (double *) array, cdummy, anynul, status);
    }
    else if (datatype == TCOMPLEX)
    {
      if (nulval == 0)
        ffgcle(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2,
           1, 1, 0., (float *) array, cdummy, anynul, status);
      else
        ffgcle(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2,
           1, 1, *(float *) nulval, (float *) array, cdummy, anynul, status);
    }
    else if (datatype == TDBLCOMPLEX)
    {
      if (nulval == 0)
        ffgcld(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2, 
         1, 1, 0., (double *) array, cdummy, anynul, status);
      else
        ffgcld(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2, 
         1, 1, *(double *) nulval, (double *) array, cdummy, anynul, status);
    }

    else if (datatype == TLOGICAL)
    {
      if (nulval == 0)
        ffgcll(fptr, colnum, firstrow, firstelem, nelem, 1, 0,
          (char *) array, cdummy, anynul, status);
      else
        ffgcll(fptr, colnum, firstrow, firstelem, nelem, 1, *(char *) nulval,
          (char *) array, cdummy, anynul, status);
    }
    else if (datatype == TSTRING)
    {
      if (nulval == 0)
      {
        cdummy[0] = '\0';
        ffgcls(fptr, colnum, firstrow, firstelem, nelem, 1, 
             cdummy, (char **) array, cdummy, anynul, status);
      }
      else
        ffgcls(fptr, colnum, firstrow, firstelem, nelem, 1, (char *)
             nulval, (char **) array, cdummy, anynul, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcf(  fitsfile *fptr,   /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            int  colnum,      /* I - number of column to write (1 = 1st col) */
            LONGLONG  firstrow,   /* I - first row to write (1 = 1st row)        */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG nelem,       /* I - number of values to read                */
            void *array,      /* O - array of values that are returned       */
            char *nullarray,  /* O - array of null value flags               */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from a table column. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  ANYNUL is returned with a value of true if any pixels are undefined.
*/
{
    double nulval = 0.;
    char cnulval[2];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (datatype == TBIT)
    {
      ffgcx(fptr, colnum, firstrow, firstelem, nelem, (char *) array, status);
    }
    else if (datatype == TBYTE)
    {
       ffgclb(fptr, colnum, firstrow, firstelem, nelem, 1, 2, (unsigned char )
              nulval, (unsigned char *) array, nullarray, anynul, status);
    }
    else if (datatype == TSBYTE)
    {
       ffgclsb(fptr, colnum, firstrow, firstelem, nelem, 1, 2, (signed char )
              nulval, (signed char *) array, nullarray, anynul, status);
    }
    else if (datatype == TUSHORT)
    {
        ffgclui(fptr, colnum, firstrow, firstelem, nelem, 1, 2,
               (unsigned short ) nulval,
               (unsigned short *) array, nullarray, anynul, status);
    }
    else if (datatype == TSHORT)
    {
        ffgcli(fptr, colnum, firstrow, firstelem, nelem, 1, 2, (short )
              nulval, (short *) array, nullarray, anynul, status);
    }
    else if (datatype == TUINT)
    {
        ffgcluk(fptr, colnum, firstrow, firstelem, nelem, 1, 2,
         (unsigned int ) nulval, (unsigned int *) array, nullarray, anynul,
         status);
    }
    else if (datatype == TINT)
    {
        ffgclk(fptr, colnum, firstrow, firstelem, nelem, 1, 2, (int )
            nulval, (int *) array, nullarray, anynul, status);
    }
    else if (datatype == TULONG)
    {
        ffgcluj(fptr, colnum, firstrow, firstelem, nelem, 1, 2,
               (unsigned long ) nulval, 
               (unsigned long *) array, nullarray, anynul, status);
    }
    else if (datatype == TLONG)
    {
        ffgclj(fptr, colnum, firstrow, firstelem, nelem, 1, 2, (long )
              nulval, (long *) array, nullarray, anynul, status);
    }
    else if (datatype == TLONGLONG)
    {
        ffgcljj(fptr, colnum, firstrow, firstelem, nelem, 1, 2, (LONGLONG )
              nulval, (LONGLONG *) array, nullarray, anynul, status);
    }
    else if (datatype == TFLOAT)
    {
      ffgcle(fptr, colnum, firstrow, firstelem, nelem, 1, 2, (float )
               nulval,(float *) array, nullarray, anynul, status);
    }
    else if (datatype == TDOUBLE)
    {
        ffgcld(fptr, colnum, firstrow, firstelem, nelem, 1, 2, 
              nulval, (double *) array, nullarray, anynul, status);
    }
    else if (datatype == TCOMPLEX)
    {
        ffgcfc(fptr, colnum, firstrow, firstelem, nelem,
           (float *) array, nullarray, anynul, status);
    }
    else if (datatype == TDBLCOMPLEX)
    {
        ffgcfm(fptr, colnum, firstrow, firstelem, nelem, 
           (double *) array, nullarray, anynul, status);
    }

    else if (datatype == TLOGICAL)
    {
        ffgcll(fptr, colnum, firstrow, firstelem, nelem, 2, (char ) nulval,
          (char *) array, nullarray, anynul, status);
    }
    else if (datatype == TSTRING)
    {
        ffgcls(fptr, colnum, firstrow, firstelem, nelem, 2, 
             cnulval, (char **) array, nullarray, anynul, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}

