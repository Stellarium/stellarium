/*  This file, putcol.c, contains routines that write data elements to     */
/*  a FITS image or table. These are the generic routines.                 */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int ffppx(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int  datatype,   /* I - datatype of the value                   */
            long  *firstpix, /* I - coord of  first pixel to write(1 based) */
            LONGLONG  nelem,     /* I - number of values to write               */
            void  *array,    /* I - array of values that are written        */
            int  *status)    /* IO - error status                           */
/*
  Write an array of pixels to the primary array.  The datatype of the
  input array is defined by the 2nd argument. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written). 
  
  This routine is simillar to ffppr, except it supports writing to 
  large images with more than 2**31 pixels.
*/
{
    int naxis, ii;
    long group = 1;
    LONGLONG firstelem, dimsize = 1, naxes[9];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);
    ffgiszll(fptr, 9, naxes, status);

    firstelem = 0;
    for (ii=0; ii < naxis; ii++)
    {
        firstelem += ((firstpix[ii] - 1) * dimsize);
        dimsize *= naxes[ii];
    }
    firstelem++;

    if (datatype == TBYTE)
    {
      ffpprb(fptr, group, firstelem, nelem, (unsigned char *) array, status);
    }
    else if (datatype == TSBYTE)
    {
      ffpprsb(fptr, group, firstelem, nelem, (signed char *) array, status);
    }
    else if (datatype == TUSHORT)
    {
      ffpprui(fptr, group, firstelem, nelem, (unsigned short *) array,
              status);
    }
    else if (datatype == TSHORT)
    {
      ffppri(fptr, group, firstelem, nelem, (short *) array, status);
    }
    else if (datatype == TUINT)
    {
      ffppruk(fptr, group, firstelem, nelem, (unsigned int *) array, status);
    }
    else if (datatype == TINT)
    {
      ffpprk(fptr, group, firstelem, nelem, (int *) array, status);
    }
    else if (datatype == TULONG)
    {
      ffppruj(fptr, group, firstelem, nelem, (unsigned long *) array, status);
    }
    else if (datatype == TLONG)
    {
      ffpprj(fptr, group, firstelem, nelem, (long *) array, status);
    }
    else if (datatype == TLONGLONG)
    {
      ffpprjj(fptr, group, firstelem, nelem, (LONGLONG *) array, status);
    }
    else if (datatype == TFLOAT)
    {
      ffppre(fptr, group, firstelem, nelem, (float *) array, status);
    }
    else if (datatype == TDOUBLE)
    {
      ffpprd(fptr, group, firstelem, nelem, (double *) array, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffppxll(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int  datatype,   /* I - datatype of the value                   */
            LONGLONG  *firstpix, /* I - coord of  first pixel to write(1 based) */
            LONGLONG  nelem,     /* I - number of values to write               */
            void  *array,    /* I - array of values that are written        */
            int  *status)    /* IO - error status                           */
/*
  Write an array of pixels to the primary array.  The datatype of the
  input array is defined by the 2nd argument. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written). 
  
  This routine is simillar to ffppr, except it supports writing to 
  large images with more than 2**31 pixels.
*/
{
    int naxis, ii;
    long group = 1;
    LONGLONG firstelem, dimsize = 1, naxes[9];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);
    ffgiszll(fptr, 9, naxes, status);

    firstelem = 0;
    for (ii=0; ii < naxis; ii++)
    {
        firstelem += ((firstpix[ii] - 1) * dimsize);
        dimsize *= naxes[ii];
    }
    firstelem++;

    if (datatype == TBYTE)
    {
      ffpprb(fptr, group, firstelem, nelem, (unsigned char *) array, status);
    }
    else if (datatype == TSBYTE)
    {
      ffpprsb(fptr, group, firstelem, nelem, (signed char *) array, status);
    }
    else if (datatype == TUSHORT)
    {
      ffpprui(fptr, group, firstelem, nelem, (unsigned short *) array,
              status);
    }
    else if (datatype == TSHORT)
    {
      ffppri(fptr, group, firstelem, nelem, (short *) array, status);
    }
    else if (datatype == TUINT)
    {
      ffppruk(fptr, group, firstelem, nelem, (unsigned int *) array, status);
    }
    else if (datatype == TINT)
    {
      ffpprk(fptr, group, firstelem, nelem, (int *) array, status);
    }
    else if (datatype == TULONG)
    {
      ffppruj(fptr, group, firstelem, nelem, (unsigned long *) array, status);
    }
    else if (datatype == TLONG)
    {
      ffpprj(fptr, group, firstelem, nelem, (long *) array, status);
    }
    else if (datatype == TLONGLONG)
    {
      ffpprjj(fptr, group, firstelem, nelem, (LONGLONG *) array, status);
    }
    else if (datatype == TFLOAT)
    {
      ffppre(fptr, group, firstelem, nelem, (float *) array, status);
    }
    else if (datatype == TDOUBLE)
    {
      ffpprd(fptr, group, firstelem, nelem, (double *) array, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffppxn(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int  datatype,   /* I - datatype of the value                   */
            long  *firstpix, /* I - first vector element to write(1 = 1st)  */
            LONGLONG  nelem,     /* I - number of values to write               */
            void  *array,    /* I - array of values that are written        */
            void  *nulval,   /* I - pointer to the null value               */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to the primary array.  The datatype of the
  input array is defined by the 2nd argument. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written).

  This routine supports writing to large images with
  more than 2**31 pixels.
*/
{
    int naxis, ii;
    long group = 1;
    LONGLONG firstelem, dimsize = 1, naxes[9];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (nulval == NULL)  /* null value not defined? */
    {
        ffppx(fptr, datatype, firstpix, nelem, array, status);
        return(*status);
    }

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);
    ffgiszll(fptr, 9, naxes, status);

    firstelem = 0;
    for (ii=0; ii < naxis; ii++)
    {
        firstelem += ((firstpix[ii] - 1) * dimsize);
        dimsize *= naxes[ii];
    }
    firstelem++;

    if (datatype == TBYTE)
    {
      ffppnb(fptr, group, firstelem, nelem, (unsigned char *) array, 
             *(unsigned char *) nulval, status);
    }
    else if (datatype == TSBYTE)
    {
      ffppnsb(fptr, group, firstelem, nelem, (signed char *) array, 
             *(signed char *) nulval, status);
    }
    else if (datatype == TUSHORT)
    {
      ffppnui(fptr, group, firstelem, nelem, (unsigned short *) array,
              *(unsigned short *) nulval,status);
    }
    else if (datatype == TSHORT)
    {
      ffppni(fptr, group, firstelem, nelem, (short *) array,
             *(short *) nulval, status);
    }
    else if (datatype == TUINT)
    {
      ffppnuk(fptr, group, firstelem, nelem, (unsigned int *) array,
             *(unsigned int *) nulval, status);
    }
    else if (datatype == TINT)
    {
      ffppnk(fptr, group, firstelem, nelem, (int *) array,
             *(int *) nulval, status);
    }
    else if (datatype == TULONG)
    {
      ffppnuj(fptr, group, firstelem, nelem, (unsigned long *) array,
              *(unsigned long *) nulval,status);
    }
    else if (datatype == TLONG)
    {
      ffppnj(fptr, group, firstelem, nelem, (long *) array,
             *(long *) nulval, status);
    }
    else if (datatype == TLONGLONG)
    {
      ffppnjj(fptr, group, firstelem, nelem, (LONGLONG *) array,
             *(LONGLONG *) nulval, status);
    }
    else if (datatype == TFLOAT)
    {
      ffppne(fptr, group, firstelem, nelem, (float *) array,
             *(float *) nulval, status);
    }
    else if (datatype == TDOUBLE)
    {
      ffppnd(fptr, group, firstelem, nelem, (double *) array,
             *(double *) nulval, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffppxnll(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int  datatype,   /* I - datatype of the value                   */
            LONGLONG  *firstpix, /* I - first vector element to write(1 = 1st)  */
            LONGLONG  nelem,     /* I - number of values to write               */
            void  *array,    /* I - array of values that are written        */
            void  *nulval,   /* I - pointer to the null value               */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to the primary array.  The datatype of the
  input array is defined by the 2nd argument. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written).

  This routine supports writing to large images with
  more than 2**31 pixels.
*/
{
    int naxis, ii;
    long  group = 1;
    LONGLONG firstelem, dimsize = 1, naxes[9];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (nulval == NULL)  /* null value not defined? */
    {
        ffppxll(fptr, datatype, firstpix, nelem, array, status);
        return(*status);
    }

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);
    ffgiszll(fptr, 9, naxes, status);

    firstelem = 0;
    for (ii=0; ii < naxis; ii++)
    {
        firstelem += ((firstpix[ii] - 1) * dimsize);
        dimsize *= naxes[ii];
    }
    firstelem++;

    if (datatype == TBYTE)
    {
      ffppnb(fptr, group, firstelem, nelem, (unsigned char *) array, 
             *(unsigned char *) nulval, status);
    }
    else if (datatype == TSBYTE)
    {
      ffppnsb(fptr, group, firstelem, nelem, (signed char *) array, 
             *(signed char *) nulval, status);
    }
    else if (datatype == TUSHORT)
    {
      ffppnui(fptr, group, firstelem, nelem, (unsigned short *) array,
              *(unsigned short *) nulval,status);
    }
    else if (datatype == TSHORT)
    {
      ffppni(fptr, group, firstelem, nelem, (short *) array,
             *(short *) nulval, status);
    }
    else if (datatype == TUINT)
    {
      ffppnuk(fptr, group, firstelem, nelem, (unsigned int *) array,
             *(unsigned int *) nulval, status);
    }
    else if (datatype == TINT)
    {
      ffppnk(fptr, group, firstelem, nelem, (int *) array,
             *(int *) nulval, status);
    }
    else if (datatype == TULONG)
    {
      ffppnuj(fptr, group, firstelem, nelem, (unsigned long *) array,
              *(unsigned long *) nulval,status);
    }
    else if (datatype == TLONG)
    {
      ffppnj(fptr, group, firstelem, nelem, (long *) array,
             *(long *) nulval, status);
    }
    else if (datatype == TLONGLONG)
    {
      ffppnjj(fptr, group, firstelem, nelem, (LONGLONG *) array,
             *(LONGLONG *) nulval, status);
    }
    else if (datatype == TFLOAT)
    {
      ffppne(fptr, group, firstelem, nelem, (float *) array,
             *(float *) nulval, status);
    }
    else if (datatype == TDOUBLE)
    {
      ffppnd(fptr, group, firstelem, nelem, (double *) array,
             *(double *) nulval, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffppr(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int  datatype,   /* I - datatype of the value                   */
            LONGLONG  firstelem, /* I - first vector element to write(1 = 1st)  */
            LONGLONG  nelem,     /* I - number of values to write               */
            void  *array,    /* I - array of values that are written        */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to the primary array.  The datatype of the
  input array is defined by the 2nd argument. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written).

*/
{
    long group = 1;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (datatype == TBYTE)
    {
      ffpprb(fptr, group, firstelem, nelem, (unsigned char *) array, status);
    }
    else if (datatype == TSBYTE)
    {
      ffpprsb(fptr, group, firstelem, nelem, (signed char *) array, status);
    }
    else if (datatype == TUSHORT)
    {
      ffpprui(fptr, group, firstelem, nelem, (unsigned short *) array,
              status);
    }
    else if (datatype == TSHORT)
    {
      ffppri(fptr, group, firstelem, nelem, (short *) array, status);
    }
    else if (datatype == TUINT)
    {
      ffppruk(fptr, group, firstelem, nelem, (unsigned int *) array, status);
    }
    else if (datatype == TINT)
    {
      ffpprk(fptr, group, firstelem, nelem, (int *) array, status);
    }
    else if (datatype == TULONG)
    {
      ffppruj(fptr, group, firstelem, nelem, (unsigned long *) array, status);
    }
    else if (datatype == TLONG)
    {
      ffpprj(fptr, group, firstelem, nelem, (long *) array, status);
    }
    else if (datatype == TLONGLONG)
    {
      ffpprjj(fptr, group, firstelem, nelem, (LONGLONG *) array, status);
    }
    else if (datatype == TFLOAT)
    {
      ffppre(fptr, group, firstelem, nelem, (float *) array, status);
    }
    else if (datatype == TDOUBLE)
    {
      ffpprd(fptr, group, firstelem, nelem, (double *) array, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffppn(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int  datatype,   /* I - datatype of the value                   */
            LONGLONG  firstelem, /* I - first vector element to write(1 = 1st)  */
            LONGLONG  nelem,     /* I - number of values to write               */
            void  *array,    /* I - array of values that are written        */
            void  *nulval,   /* I - pointer to the null value               */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to the primary array.  The datatype of the
  input array is defined by the 2nd argument. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written).

*/
{
    long group = 1;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (nulval == NULL)  /* null value not defined? */
    {
        ffppr(fptr, datatype, firstelem, nelem, array, status);
        return(*status);
    }

    if (datatype == TBYTE)
    {
      ffppnb(fptr, group, firstelem, nelem, (unsigned char *) array, 
             *(unsigned char *) nulval, status);
    }
    else if (datatype == TSBYTE)
    {
      ffppnsb(fptr, group, firstelem, nelem, (signed char *) array, 
             *(signed char *) nulval, status);
    }
    else if (datatype == TUSHORT)
    {
      ffppnui(fptr, group, firstelem, nelem, (unsigned short *) array,
              *(unsigned short *) nulval,status);
    }
    else if (datatype == TSHORT)
    {
      ffppni(fptr, group, firstelem, nelem, (short *) array,
             *(short *) nulval, status);
    }
    else if (datatype == TUINT)
    {
      ffppnuk(fptr, group, firstelem, nelem, (unsigned int *) array,
             *(unsigned int *) nulval, status);
    }
    else if (datatype == TINT)
    {
      ffppnk(fptr, group, firstelem, nelem, (int *) array,
             *(int *) nulval, status);
    }
    else if (datatype == TULONG)
    {
      ffppnuj(fptr, group, firstelem, nelem, (unsigned long *) array,
              *(unsigned long *) nulval,status);
    }
    else if (datatype == TLONG)
    {
      ffppnj(fptr, group, firstelem, nelem, (long *) array,
             *(long *) nulval, status);
    }
    else if (datatype == TLONGLONG)
    {
      ffppnjj(fptr, group, firstelem, nelem, (LONGLONG *) array,
             *(LONGLONG *) nulval, status);
    }
    else if (datatype == TFLOAT)
    {
      ffppne(fptr, group, firstelem, nelem, (float *) array,
             *(float *) nulval, status);
    }
    else if (datatype == TDOUBLE)
    {
      ffppnd(fptr, group, firstelem, nelem, (double *) array,
             *(double *) nulval, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpss(  fitsfile *fptr,   /* I - FITS file pointer                       */
            int  datatype,    /* I - datatype of the value                   */
            long *blc,        /* I - 'bottom left corner' of the subsection  */
            long *trc ,       /* I - 'top right corner' of the subsection    */
            void *array,      /* I - array of values that are written        */
            int  *status)     /* IO - error status                           */
/*
  Write a section of values to the primary array. The datatype of the
  input array is defined by the 2nd argument.  Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written).

  This routine supports writing to large images with
  more than 2**31 pixels.
*/
{
    int naxis;
    long naxes[9];

    if (*status > 0)   /* inherit input status value if > 0 */
        return(*status);

    /* get the size of the image */
    ffgidm(fptr, &naxis, status);
    ffgisz(fptr, 9, naxes, status);

    if (datatype == TBYTE)
    {
        ffpssb(fptr, 1, naxis, naxes, blc, trc,
               (unsigned char *) array, status);
    }
    else if (datatype == TSBYTE)
    {
        ffpsssb(fptr, 1, naxis, naxes, blc, trc,
               (signed char *) array, status);
    }
    else if (datatype == TUSHORT)
    {
        ffpssui(fptr, 1, naxis, naxes, blc, trc,
               (unsigned short *) array, status);
    }
    else if (datatype == TSHORT)
    {
        ffpssi(fptr, 1, naxis, naxes, blc, trc,
               (short *) array, status);
    }
    else if (datatype == TUINT)
    {
        ffpssuk(fptr, 1, naxis, naxes, blc, trc,
               (unsigned int *) array, status);
    }
    else if (datatype == TINT)
    {
        ffpssk(fptr, 1, naxis, naxes, blc, trc,
               (int *) array, status);
    }
    else if (datatype == TULONG)
    {
        ffpssuj(fptr, 1, naxis, naxes, blc, trc,
               (unsigned long *) array, status);
    }
    else if (datatype == TLONG)
    {
        ffpssj(fptr, 1, naxis, naxes, blc, trc,
               (long *) array, status);
    }
    else if (datatype == TLONGLONG)
    {
        ffpssjj(fptr, 1, naxis, naxes, blc, trc,
               (LONGLONG *) array, status);
    }    else if (datatype == TFLOAT)
    {
        ffpsse(fptr, 1, naxis, naxes, blc, trc,
               (float *) array, status);
    }
    else if (datatype == TDOUBLE)
    {
        ffpssd(fptr, 1, naxis, naxes, blc, trc,
               (double *) array, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpcl(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int  datatype,   /* I - datatype of the value                   */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            LONGLONG  firstrow,  /* I - first row to write (1 = 1st row)        */
            LONGLONG  firstelem, /* I - first vector element to write (1 = 1st) */
            LONGLONG  nelem,     /* I - number of elements to write             */
            void  *array,    /* I - array of values that are written        */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to a table column.  The datatype of the
  input array is defined by the 2nd argument. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS column is not the same as the array being written).

*/
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (datatype == TBIT)
    {
      ffpclx(fptr, colnum, firstrow, (long) firstelem, (long) nelem, (char *) array, 
             status);
    }
    else if (datatype == TBYTE)
    {
      ffpclb(fptr, colnum, firstrow, firstelem, nelem, (unsigned char *) array,
             status);
    }
    else if (datatype == TSBYTE)
    {
      ffpclsb(fptr, colnum, firstrow, firstelem, nelem, (signed char *) array,
             status);
    }
    else if (datatype == TUSHORT)
    {
      ffpclui(fptr, colnum, firstrow, firstelem, nelem, 
             (unsigned short *) array, status);
    }
    else if (datatype == TSHORT)
    {
      ffpcli(fptr, colnum, firstrow, firstelem, nelem, (short *) array,
             status);
    }
    else if (datatype == TUINT)
    {
      ffpcluk(fptr, colnum, firstrow, firstelem, nelem, (unsigned int *) array,
               status);
    }
    else if (datatype == TINT)
    {
      ffpclk(fptr, colnum, firstrow, firstelem, nelem, (int *) array,
               status);
    }
    else if (datatype == TULONG)
    {
      ffpcluj(fptr, colnum, firstrow, firstelem, nelem, (unsigned long *) array,
              status);
    }
    else if (datatype == TLONG)
    {
      ffpclj(fptr, colnum, firstrow, firstelem, nelem, (long *) array,
             status);
    }
    else if (datatype == TLONGLONG)
    {
      ffpcljj(fptr, colnum, firstrow, firstelem, nelem, (LONGLONG *) array,
             status);
    }
    else if (datatype == TFLOAT)
    {
      ffpcle(fptr, colnum, firstrow, firstelem, nelem, (float *) array,
             status);
    }
    else if (datatype == TDOUBLE)
    {
      ffpcld(fptr, colnum, firstrow, firstelem, nelem, (double *) array,
             status);
    }
    else if (datatype == TCOMPLEX)
    {
      ffpcle(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2,
             (float *) array, status);
    }
    else if (datatype == TDBLCOMPLEX)
    {
      ffpcld(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2,
             (double *) array, status);
    }
    else if (datatype == TLOGICAL)
    {
      ffpcll(fptr, colnum, firstrow, firstelem, nelem, (char *) array,
             status);
    }
    else if (datatype == TSTRING)
    {
      ffpcls(fptr, colnum, firstrow, firstelem, nelem, (char **) array,
             status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpcn(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int  datatype,   /* I - datatype of the value                   */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            LONGLONG  firstrow,  /* I - first row to write (1 = 1st row)        */
            LONGLONG  firstelem, /* I - first vector element to write (1 = 1st) */
            LONGLONG  nelem,     /* I - number of elements to write             */
            void  *array,    /* I - array of values that are written        */
            void  *nulval,   /* I - pointer to the null value               */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to a table column.  The datatype of the
  input array is defined by the 2nd argument. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS column is not the same as the array being written).

*/
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (nulval == NULL)  /* null value not defined? */
    {
        ffpcl(fptr, datatype, colnum, firstrow, firstelem, nelem, array,
              status);
        return(*status);
    }

    if (datatype == TBYTE)
    {
      ffpcnb(fptr, colnum, firstrow, firstelem, nelem, (unsigned char *) array,
            *(unsigned char *) nulval, status);
    }
    else if (datatype == TSBYTE)
    {
      ffpcnsb(fptr, colnum, firstrow, firstelem, nelem, (signed char *) array,
            *(signed char *) nulval, status);
    }
    else if (datatype == TUSHORT)
    {
     ffpcnui(fptr, colnum, firstrow, firstelem, nelem, (unsigned short *) array,
             *(unsigned short *) nulval, status);
    }
    else if (datatype == TSHORT)
    {
      ffpcni(fptr, colnum, firstrow, firstelem, nelem, (short *) array,
             *(unsigned short *) nulval, status);
    }
    else if (datatype == TUINT)
    {
      ffpcnuk(fptr, colnum, firstrow, firstelem, nelem, (unsigned int *) array,
             *(unsigned int *) nulval, status);
    }
    else if (datatype == TINT)
    {
      ffpcnk(fptr, colnum, firstrow, firstelem, nelem, (int *) array,
             *(int *) nulval, status);
    }
    else if (datatype == TULONG)
    {
      ffpcnuj(fptr, colnum, firstrow, firstelem, nelem, (unsigned long *) array,
              *(unsigned long *) nulval, status);
    }
    else if (datatype == TLONG)
    {
      ffpcnj(fptr, colnum, firstrow, firstelem, nelem, (long *) array,
             *(long *) nulval, status);
    }
    else if (datatype == TLONGLONG)
    {
      ffpcnjj(fptr, colnum, firstrow, firstelem, nelem, (LONGLONG *) array,
             *(LONGLONG *) nulval, status);
    }
    else if (datatype == TFLOAT)
    {
      ffpcne(fptr, colnum, firstrow, firstelem, nelem, (float *) array,
             *(float *) nulval, status);
    }
    else if (datatype == TDOUBLE)
    {
      ffpcnd(fptr, colnum, firstrow, firstelem, nelem, (double *) array,
             *(double *) nulval, status);
    }
    else if (datatype == TCOMPLEX)
    {
      ffpcne(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2,
             (float *) array, *(float *) nulval, status);
    }
    else if (datatype == TDBLCOMPLEX)
    {
      ffpcnd(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2,
             (double *) array, *(double *) nulval, status);
    }
    else if (datatype == TLOGICAL)
    {
      ffpcnl(fptr, colnum, firstrow, firstelem, nelem, (char *) array,
             *(char *) nulval, status);
    }
    else if (datatype == TSTRING)
    {
      ffpcns(fptr, colnum, firstrow, firstelem, nelem, (char **) array,
             (char *) nulval, status);
    }
    else
      *status = BAD_DATATYPE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_iter_set_by_name(iteratorCol *col, /* I - iterator col structure */
           fitsfile *fptr,  /* I - FITS file pointer                      */
           char *colname,   /* I - column name                            */
           int datatype,    /* I - column datatype                        */
           int iotype)      /* I - InputCol, InputOutputCol, or OutputCol */
/*
  set all the parameters for an iterator column, by column name
*/
{
    col->fptr = fptr;
    strcpy(col->colname, colname);
    col->colnum = 0;  /* set column number undefined since name is given */
    col->datatype = datatype;
    col->iotype = iotype;
    return(0);
}
/*--------------------------------------------------------------------------*/
int fits_iter_set_by_num(iteratorCol *col, /* I - iterator column structure */
           fitsfile *fptr,  /* I - FITS file pointer                      */
           int colnum,      /* I - column number                          */
           int datatype,    /* I - column datatype                        */
           int iotype)      /* I - InputCol, InputOutputCol, or OutputCol */
/*
  set all the parameters for an iterator column, by column number
*/
{
    col->fptr = fptr;
    col->colnum = colnum; 
    col->datatype = datatype;
    col->iotype = iotype;
    return(0);
}
/*--------------------------------------------------------------------------*/
int fits_iter_set_file(iteratorCol *col, /* I - iterator column structure   */
           fitsfile *fptr)   /* I - FITS file pointer                      */
/*
  set iterator column parameter
*/
{
    col->fptr = fptr;
    return(0);
}
/*--------------------------------------------------------------------------*/
int fits_iter_set_colname(iteratorCol *col, /* I - iterator col structure  */
           char *colname)    /* I - column name                            */
/*
  set iterator column parameter
*/
{
    strcpy(col->colname, colname);
    col->colnum = 0;  /* set column number undefined since name is given */
    return(0);
}
/*--------------------------------------------------------------------------*/
int fits_iter_set_colnum(iteratorCol *col, /* I - iterator column structure */
           int colnum)       /* I - column number                          */
/*
  set iterator column parameter
*/
{
    col->colnum = colnum; 
    return(0);
}
/*--------------------------------------------------------------------------*/
int fits_iter_set_datatype(iteratorCol *col, /* I - iterator col structure */
           int datatype)    /* I - column datatype                        */
/*
  set iterator column parameter
*/
{
    col->datatype = datatype;
    return(0);
}
/*--------------------------------------------------------------------------*/
int fits_iter_set_iotype(iteratorCol *col, /* I - iterator column structure */
           int iotype)       /* I - InputCol, InputOutputCol, or OutputCol */
/*
  set iterator column parameter
*/
{
    col->iotype = iotype;
    return(0);
}
/*--------------------------------------------------------------------------*/
fitsfile * fits_iter_get_file(iteratorCol *col) /* I -iterator col structure */
/*
  get iterator column parameter
*/
{
     return(col->fptr);
}
/*--------------------------------------------------------------------------*/
char * fits_iter_get_colname(iteratorCol *col) /* I -iterator col structure */
/*
  get iterator column parameter
*/
{
    return(col->colname);
}
/*--------------------------------------------------------------------------*/
int fits_iter_get_colnum(iteratorCol *col) /* I - iterator column structure */
/*
  get iterator column parameter
*/
{
    return(col->colnum);
}
/*--------------------------------------------------------------------------*/
int fits_iter_get_datatype(iteratorCol *col) /* I - iterator col structure */
/*
  get iterator column parameter
*/
{
    return(col->datatype);
}
/*--------------------------------------------------------------------------*/
int fits_iter_get_iotype(iteratorCol *col) /* I - iterator column structure */
/*
  get iterator column parameter
*/
{
     return(col->iotype);
}
/*--------------------------------------------------------------------------*/
void * fits_iter_get_array(iteratorCol *col) /* I - iterator col structure */
/*
  get iterator column parameter
*/
{
     return(col->array);
}
/*--------------------------------------------------------------------------*/
long fits_iter_get_tlmin(iteratorCol *col) /* I - iterator column structure */
/*
  get iterator column parameter
*/
{
     return(col->tlmin);
}
/*--------------------------------------------------------------------------*/
long fits_iter_get_tlmax(iteratorCol *col) /* I - iterator column structure */
/*
  get iterator column parameter
*/
{
     return(col->tlmax);
}
/*--------------------------------------------------------------------------*/
long fits_iter_get_repeat(iteratorCol *col) /* I - iterator col structure */
/*
  get iterator column parameter
*/
{
     return(col->repeat);
}
/*--------------------------------------------------------------------------*/
char * fits_iter_get_tunit(iteratorCol *col) /* I - iterator col structure */
/*
  get iterator column parameter
*/
{
    return(col->tunit);
}
/*--------------------------------------------------------------------------*/
char * fits_iter_get_tdisp(iteratorCol *col) /* I -iterator col structure   */
/*
  get iterator column parameter
*/
{
    return(col->tdisp);
}
/*--------------------------------------------------------------------------*/
int ffiter(int n_cols,
           iteratorCol *cols,
           long offset,
           long n_per_loop,
           int (*work_fn)(long total_n,
                          long offset,
                          long first_n,
                          long n_values,
                          int n_cols,
                          iteratorCol *cols,
                          void *userPointer),
           void *userPointer,
           int *status)
/*
   The iterator function.  This function will pass the specified
   columns from a FITS table or pixels from a FITS image to the 
   user-supplied function.  Depending on the size of the table
   or image, only a subset of the rows or pixels may be passed to the
   function on each call, in which case the function will be called
   multiple times until all the rows or pixels have been processed.
*/
{
    typedef struct  /* structure to store the column null value */
    {  
        int      nullsize;    /* length of the null value, in bytes */
        union {   /*  default null value for the column */
            char   *stringnull;
            unsigned char   charnull;
            signed char scharnull;
            int    intnull;
            short  shortnull;
            long   longnull;
            unsigned int   uintnull;
            unsigned short ushortnull;
            unsigned long  ulongnull;
            float  floatnull;
            double doublenull;
	    LONGLONG longlongnull;
        } null;
    } colNulls;

    void *dataptr, *defaultnull;
    colNulls *col;
    int ii, jj, tstatus, naxis, bitpix;
    int typecode, hdutype, jtype, type, anynul, nfiles, nbytes;
    long totaln, nleft, frow, felement, n_optimum, i_optimum, ntodo;
    long rept, rowrept, width, tnull, naxes[9] = {1,1,1,1,1,1,1,1,1}, groups;
    double zeros = 0.;
    char message[FLEN_ERRMSG], keyname[FLEN_KEYWORD], nullstr[FLEN_VALUE];
    char **stringptr, *nullptr, *cptr;

    if (*status > 0)
        return(*status);

    if (n_cols  < 0 || n_cols > 999 )
    {
        ffpmsg("Illegal number of columms (ffiter)");
        return(*status = BAD_COL_NUM);  /* negative number of columns */
    }

    /*------------------------------------------------------------*/
    /* Make sure column numbers and datatypes are in legal range  */
    /* and column numbers and datatypes are legal.                */ 
    /* Also fill in other parameters in the column structure.     */
    /*------------------------------------------------------------*/

    ffghdt(cols[0].fptr, &hdutype, status);  /* type of first HDU */

    for (jj = 0; jj < n_cols; jj++)
    {
        /* check that output datatype code value is legal */
        type = cols[jj].datatype;  

        /* Allow variable length arrays for InputCol and InputOutputCol columns,
	   but not for OutputCol columns.  Variable length arrays have a
	   negative type code value. */

        if ((cols[jj].iotype != OutputCol) && (type<0)) {
            type*=-1;
        }

        if (type != 0      && type != TBYTE  &&
            type != TSBYTE && type != TLOGICAL && type != TSTRING &&
            type != TSHORT && type != TINT     && type != TLONG && 
            type != TFLOAT && type != TDOUBLE  && type != TCOMPLEX &&
            type != TULONG && type != TUSHORT  && type != TDBLCOMPLEX &&
	    type != TLONGLONG )
        {
	    if (type < 0) {
	      sprintf(message,
              "Variable length array not allowed for output column number %d (ffiter)",
                    jj + 1);
	    } else {
            sprintf(message,
                   "Illegal datatype for column number %d: %d  (ffiter)",
                    jj + 1, cols[jj].datatype);
	    }
	    
            ffpmsg(message);
            return(*status = BAD_DATATYPE);
        }

        /* initialize TLMINn, TLMAXn, column name, and display format */
        cols[jj].tlmin = 0;
        cols[jj].tlmax = 0;
        cols[jj].tunit[0] = '\0';
        cols[jj].tdisp[0] = '\0';

        ffghdt(cols[jj].fptr, &jtype, status);  /* get HDU type */

        if (hdutype == IMAGE_HDU) /* operating on FITS images */
        {
            if (jtype != IMAGE_HDU)
            {
                sprintf(message,
                "File %d not positioned to an image extension (ffiter)",
                    jj + 1);
                return(*status = NOT_IMAGE);
            }

            /* since this is an image, set a dummy column number = 0 */
            cols[jj].colnum = 0;
            strcpy(cols[jj].colname, "IMAGE");  /* dummy name for images */

            tstatus = 0;
            ffgkys(cols[jj].fptr, "BUNIT", cols[jj].tunit, 0, &tstatus);
        }
        else  /* operating on FITS tables */
        {
            if (jtype == IMAGE_HDU)
            {
                sprintf(message,
                "File %d not positioned to a table extension (ffiter)",
                    jj + 1);
                return(*status = NOT_TABLE);
            }

            if (cols[jj].colnum < 1)
            {
                /* find the column number for the named column */
                if (ffgcno(cols[jj].fptr, CASEINSEN, cols[jj].colname,
                           &cols[jj].colnum, status) )
                {
                    sprintf(message,
                      "Column '%s' not found for column number %d  (ffiter)",
                       cols[jj].colname, jj + 1);
                    ffpmsg(message);
                    return(*status);
                }
            }

            /* check that the column number is valid */
            if (cols[jj].colnum < 1 || 
                cols[jj].colnum > ((cols[jj].fptr)->Fptr)->tfield)
            {
                sprintf(message,
                  "Column %d has illegal table position number: %d  (ffiter)",
                    jj + 1, cols[jj].colnum);
                ffpmsg(message);
                return(*status = BAD_COL_NUM);
            }

            /* look for column description keywords and update structure */
            tstatus = 0;
            ffkeyn("TLMIN", cols[jj].colnum, keyname, &tstatus);
            ffgkyj(cols[jj].fptr, keyname, &cols[jj].tlmin, 0, &tstatus);

            tstatus = 0;
            ffkeyn("TLMAX", cols[jj].colnum, keyname, &tstatus);
            ffgkyj(cols[jj].fptr, keyname, &cols[jj].tlmax, 0, &tstatus);

            tstatus = 0;
            ffkeyn("TTYPE", cols[jj].colnum, keyname, &tstatus);
            ffgkys(cols[jj].fptr, keyname, cols[jj].colname, 0, &tstatus);
            if (tstatus)
                cols[jj].colname[0] = '\0';

            tstatus = 0;
            ffkeyn("TUNIT", cols[jj].colnum, keyname, &tstatus);
            ffgkys(cols[jj].fptr, keyname, cols[jj].tunit, 0, &tstatus);

            tstatus = 0;
            ffkeyn("TDISP", cols[jj].colnum, keyname, &tstatus);
            ffgkys(cols[jj].fptr, keyname, cols[jj].tdisp, 0, &tstatus);
        }
    }  /* end of loop over all columns */

    /*-----------------------------------------------------------------*/
    /* use the first file to set the total number of values to process */
    /*-----------------------------------------------------------------*/

    offset = maxvalue(offset, 0L);  /* make sure offset is legal */

    if (hdutype == IMAGE_HDU)   /* get total number of pixels in the image */
    {
      fits_get_img_dim(cols[0].fptr, &naxis, status);
      fits_get_img_size(cols[0].fptr, 9, naxes, status);

      tstatus = 0;
      ffgkyj(cols[0].fptr, "GROUPS", &groups, NULL, &tstatus);
      if (!tstatus && groups && (naxis > 1) && (naxes[0] == 0) )
      {
         /* this is a random groups file, with NAXIS1 = 0 */
         /* Use GCOUNT, the number of groups, as the first multiplier  */
         /* to calculate the total number of pixels in all the groups. */
         ffgkyj(cols[0].fptr, "GCOUNT", &totaln, NULL, status);

      }  else {
         totaln = naxes[0];
      }

      for (ii = 1; ii < naxis; ii++)
          totaln *= naxes[ii];

      frow = 1;
      felement = 1 + offset;
    }
    else   /* get total number or rows in the table */
    {
      ffgkyj(cols[0].fptr, "NAXIS2", &totaln, 0, status);
      frow = 1 + offset;
      felement = 1;
    }

    /*  adjust total by the input starting offset value */
    totaln -= offset;
    totaln = maxvalue(totaln, 0L);   /* don't allow negative number */

    /*------------------------------------------------------------------*/
    /* Determine number of values to pass to work function on each loop */
    /*------------------------------------------------------------------*/

    if (n_per_loop == 0)
    {
        /* Determine optimum number of values for each iteration.    */
        /* Look at all the fitsfile pointers to determine the number */
        /* of unique files.                                          */

        nfiles = 1;
        ffgrsz(cols[0].fptr, &n_optimum, status);

        for (jj = 1; jj < n_cols; jj++)
        {
            for (ii = 0; ii < jj; ii++)
            {
                if (cols[ii].fptr == cols[jj].fptr)
                   break;
            }

            if (ii == jj)  /* this is a new file */
            {
                nfiles++;
                ffgrsz(cols[jj].fptr, &i_optimum, status);
                n_optimum = minvalue(n_optimum, i_optimum);
            }
        }

        /* divid n_optimum by the number of files that will be processed */
        n_optimum = n_optimum / nfiles;
        n_optimum = maxvalue(n_optimum, 1);
    }
    else if (n_per_loop < 0)  /* must pass all the values at one time */
    {
        n_optimum = totaln;
    }
    else /* calling routine specified how many values to pass at a time */
    {
        n_optimum = minvalue(n_per_loop, totaln);
    }

    /*--------------------------------------*/
    /* allocate work arrays for each column */
    /* and determine the null pixel value   */
    /*--------------------------------------*/

    col = calloc(n_cols, sizeof(colNulls) ); /* memory for the null values */
    if (!col)
    {
        ffpmsg("ffiter failed to allocate memory for null values");
        *status = MEMORY_ALLOCATION;  /* memory allocation failed */
        return(*status);
    }

    for (jj = 0; jj < n_cols; jj++)
    {
        /* get image or column datatype and vector length */
        if (hdutype == IMAGE_HDU)   /* get total number of pixels in the image */
        {
           fits_get_img_type(cols[jj].fptr, &bitpix, status);
           switch(bitpix) {
             case BYTE_IMG:
                 typecode = TBYTE;
                 break;
             case SHORT_IMG:
                 typecode = TSHORT;
                 break;
             case LONG_IMG:
                 typecode = TLONG;
                 break;
             case FLOAT_IMG:
                 typecode = TFLOAT;
                 break;
             case DOUBLE_IMG:
                 typecode = TDOUBLE;
                 break;
             case LONGLONG_IMG:
                 typecode = TLONGLONG;
                 break;
            }
        }
        else
        {
            if (ffgtcl(cols[jj].fptr, cols[jj].colnum, &typecode, &rept,
                  &width, status) > 0)
                goto cleanup;
		
	    if (typecode < 0) {  /* if any variable length arrays, then the */ 
	        n_optimum = 1;   /* must process the table 1 row at a time */
		
              /* Allow variable length arrays for InputCol and InputOutputCol columns,
	       but not for OutputCol columns.  Variable length arrays have a
	       negative type code value. */

              if (cols[jj].iotype == OutputCol) {
 	        sprintf(message,
                "Variable length array not allowed for output column number %d (ffiter)",
                    jj + 1);
                ffpmsg(message);
                return(*status = BAD_DATATYPE);
              }
	   }
        }

        /* special case where sizeof(long) = 8: use TINT instead of TLONG */
        if (abs(typecode) == TLONG && sizeof(long) == 8 && sizeof(int) == 4) {
		if(typecode<0) {
			typecode = -TINT;
		} else {
			typecode = TINT;
		}
        }

        /* Special case: interprete 'X' column as 'B' */
        if (abs(typecode) == TBIT)
        {
            typecode  = typecode / TBIT * TBYTE;
            rept = (rept + 7) / 8;
        }

        if (cols[jj].datatype == 0)    /* output datatype not specified? */
        {
            /* special case if sizeof(long) = 8: use TINT instead of TLONG */
            if (abs(typecode) == TLONG && sizeof(long) == 8 && sizeof(int) == 4)
                cols[jj].datatype = TINT;
            else
                cols[jj].datatype = abs(typecode);
        }

        /* calc total number of elements to do on each iteration */
        if (hdutype == IMAGE_HDU || cols[jj].datatype == TSTRING)
        {
            ntodo = n_optimum; 
            cols[jj].repeat = 1;

            /* get the BLANK keyword value, if it exists */
            if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
                || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
            {
                tstatus = 0;
                ffgkyj(cols[jj].fptr, "BLANK", &tnull, 0, &tstatus);
                if (tstatus)
                {
                    tnull = 0L;  /* no null values */
                }
            }
        }
        else
        {
	    if (typecode < 0) 
	    {
              /* get max size of the variable length vector; dont't trust the value
	         given by the TFORM keyword  */
	      rept = 1;
	      for (ii = 0; ii < totaln; ii++) {
		ffgdes(cols[jj].fptr, cols[jj].colnum, frow + ii, &rowrept, NULL, status);
		
		rept = maxvalue(rept, rowrept);
	      }
            }
	    
            ntodo = n_optimum * rept;   /* vector columns */
            cols[jj].repeat = rept;

            /* get the TNULL keyword value, if it exists */
            if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
                || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
            {
                tstatus = 0;
                if (hdutype == ASCII_TBL) /* TNULLn value is a string */
                {
                    ffkeyn("TNULL", cols[jj].colnum, keyname, &tstatus);
                    ffgkys(cols[jj].fptr, keyname, nullstr, 0, &tstatus);
                    if (tstatus)
                    {
                        tnull = 0L; /* keyword doesn't exist; no null values */
                    }
                    else
                    {
                        cptr = nullstr;
                        while (*cptr == ' ')  /* skip over leading blanks */
                           cptr++;

                        if (*cptr == '\0')  /* TNULLn is all blanks? */
                            tnull = LONG_MIN;
                        else
                        {                                                
                            /* attempt to read TNULLn string as an integer */
                            ffc2ii(nullstr, &tnull, &tstatus);

                            if (tstatus)
                                tnull = LONG_MIN;  /* choose smallest value */
                        }                          /* to represent nulls */
                    }
                }
                else  /* Binary table; TNULLn value is an integer */
                {
                    ffkeyn("TNULL", cols[jj].colnum, keyname, &tstatus);
                    ffgkyj(cols[jj].fptr, keyname, &tnull, 0, &tstatus);
                    if (tstatus)
                    {
                        tnull = 0L; /* keyword doesn't exist; no null values */
                    }
                    else if (tnull == 0)
                    {
                        /* worst possible case: a value of 0 is used to   */
                        /* represent nulls in the FITS file.  We have to  */
                        /* use a non-zero null value here (zero is used to */
                        /* mean there are no null values in the array) so we */
                        /* will use the smallest possible integer instead. */

                        tnull = LONG_MIN;  /* choose smallest possible value */
                    }
                }
            }
        }

        /* Note that the data array starts with 2nd element;  */
        /* 1st element of the array gives the null data value */

        switch (cols[jj].datatype)
        {
         case TBYTE:
          cols[jj].array = calloc(ntodo + 1, sizeof(char));
          col[jj].nullsize  = sizeof(char);  /* number of bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
              || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              tnull = minvalue(tnull, 255);
              tnull = maxvalue(tnull, 0);
              col[jj].null.charnull = (unsigned char) tnull;
          }
          else
          {
              col[jj].null.charnull = (unsigned char) 255; /* use 255 as null */
          }
          break;

         case TSBYTE:
          cols[jj].array = calloc(ntodo + 1, sizeof(char));
          col[jj].nullsize  = sizeof(char);  /* number of bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
              || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              tnull = minvalue(tnull, 127);
              tnull = maxvalue(tnull, -128);
              col[jj].null.scharnull = (signed char) tnull;
          }
          else
          {
              col[jj].null.scharnull = (signed char) -128; /* use -128  null */
          }
          break;

         case TSHORT:
          cols[jj].array = calloc(ntodo + 1, sizeof(short));
          col[jj].nullsize  = sizeof(short);  /* number of bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
              || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              tnull = minvalue(tnull, SHRT_MAX);
              tnull = maxvalue(tnull, SHRT_MIN);
              col[jj].null.shortnull = (short) tnull;
          }
          else
          {
              col[jj].null.shortnull = SHRT_MIN;  /* use minimum as null */
          }
          break;

         case TUSHORT:
          cols[jj].array = calloc(ntodo + 1, sizeof(unsigned short));
          col[jj].nullsize  = sizeof(unsigned short);  /* bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
               || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              tnull = minvalue(tnull, (long) USHRT_MAX);
              tnull = maxvalue(tnull, 0);  /* don't allow negative value */
              col[jj].null.ushortnull = (unsigned short) tnull;
          }
          else
          {
              col[jj].null.ushortnull = USHRT_MAX;   /* use maximum null */
          }
          break;

         case TINT:
          cols[jj].array = calloc(sizeof(int), ntodo + 1);
          col[jj].nullsize  = sizeof(int);  /* number of bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
               || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              tnull = minvalue(tnull, INT_MAX);
              tnull = maxvalue(tnull, INT_MIN);
              col[jj].null.intnull = (int) tnull;
          }
          else
          {
              col[jj].null.intnull = INT_MIN;  /* use minimum as null */
          }
          break;

         case TUINT:
          cols[jj].array = calloc(ntodo + 1, sizeof(unsigned int));
          col[jj].nullsize  = sizeof(unsigned int);  /* bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
               || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              tnull = minvalue(tnull, INT32_MAX);
              tnull = maxvalue(tnull, 0);
              col[jj].null.uintnull = (unsigned int) tnull;
          }
          else
          {
              col[jj].null.uintnull = UINT_MAX;  /* use maximum as null */
          }
          break;

         case TLONG:
          cols[jj].array = calloc(ntodo + 1, sizeof(long));
          col[jj].nullsize  = sizeof(long);  /* number of bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
               || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              col[jj].null.longnull = tnull;
          }
          else
          {
              col[jj].null.longnull = LONG_MIN;   /* use minimum as null */
          }
          break;

         case TULONG:
          cols[jj].array = calloc(ntodo + 1, sizeof(unsigned long));
          col[jj].nullsize  = sizeof(unsigned long);  /* bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
               || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              if (tnull < 0)  /* can't use a negative null value */
                  col[jj].null.ulongnull = LONG_MAX;
              else
                  col[jj].null.ulongnull = (unsigned long) tnull;
          }
          else
          {
              col[jj].null.ulongnull = LONG_MAX;   /* use maximum as null */
          }
          break;

         case TFLOAT:
          cols[jj].array = calloc(ntodo + 1, sizeof(float));
          col[jj].nullsize  = sizeof(float);  /* number of bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
               || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              col[jj].null.floatnull = (float) tnull;
          }
          else
          {
              col[jj].null.floatnull = FLOATNULLVALUE;  /* special value */
          }
          break;

         case TCOMPLEX:
          cols[jj].array = calloc((ntodo * 2) + 1, sizeof(float));
          col[jj].nullsize  = sizeof(float);  /* number of bytes per value */
          col[jj].null.floatnull = FLOATNULLVALUE;  /* special value */
          break;

         case TDOUBLE:
          cols[jj].array = calloc(ntodo + 1, sizeof(double));
          col[jj].nullsize  = sizeof(double);  /* number of bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG
               || abs(typecode) == TINT || abs(typecode) == TLONGLONG)
          {
              col[jj].null.doublenull = (double) tnull;
          }
          else
          {
              col[jj].null.doublenull = DOUBLENULLVALUE;  /* special value */
          }
          break;

         case TDBLCOMPLEX:
          cols[jj].array = calloc((ntodo * 2) + 1, sizeof(double));
          col[jj].nullsize  = sizeof(double);  /* number of bytes per value */
          col[jj].null.doublenull = DOUBLENULLVALUE;  /* special value */
          break;

         case TSTRING:
          /* allocate array of pointers to all the strings  */
	  if( hdutype==ASCII_TBL ) rept = width;
          stringptr = calloc((ntodo + 1) , sizeof(stringptr));
          cols[jj].array = stringptr;
          col[jj].nullsize  = rept + 1;  /* number of bytes per value */

          if (stringptr)
          {
            /* allocate string to store the null string value */
            col[jj].null.stringnull = calloc(rept + 1, sizeof(char) );
            col[jj].null.stringnull[1] = 1; /* to make sure string != 0 */

            /* allocate big block for the array of table column strings */
            stringptr[0] = calloc((ntodo + 1) * (rept + 1), sizeof(char) );

            if (stringptr[0])
            {
              for (ii = 1; ii <= ntodo; ii++)
              {   /* pointer to each string */
                stringptr[ii] = stringptr[ii - 1] + (rept + 1);
              }

              /* get the TNULL keyword value, if it exists */
              tstatus = 0;
              ffkeyn("TNULL", cols[jj].colnum, keyname, &tstatus);
              ffgkys(cols[jj].fptr, keyname, nullstr, 0, &tstatus);
              if (!tstatus)
                  strncat(col[jj].null.stringnull, nullstr, rept);
            }
            else
            {
              ffpmsg("ffiter failed to allocate memory arrays");
              *status = MEMORY_ALLOCATION;  /* memory allocation failed */
              goto cleanup;
            }
          }
          break;

         case TLOGICAL:

          cols[jj].array = calloc(ntodo + 1, sizeof(char));
          col[jj].nullsize  = sizeof(char);  /* number of bytes per value */

          /* use value = 2 to flag null values in logical columns */
          col[jj].null.charnull = 2;
          break;

         case TLONGLONG:
          cols[jj].array = calloc(ntodo + 1, sizeof(LONGLONG));
          col[jj].nullsize  = sizeof(LONGLONG);  /* number of bytes per value */

          if (abs(typecode) == TBYTE || abs(typecode) == TSHORT || abs(typecode) == TLONG ||
	      abs(typecode) == TLONGLONG || abs(typecode) == TINT)
          {
              col[jj].null.longlongnull = tnull;
          }
          else
          {
              col[jj].null.longlongnull = LONGLONG_MIN;   /* use minimum as null */
          }
          break;

         default:
          sprintf(message,
                  "Column %d datatype currently not supported: %d:  (ffiter)",
                   jj + 1, cols[jj].datatype);
          ffpmsg(message);
          *status = BAD_DATATYPE;
          goto cleanup;

        }   /* end of switch block */

        /* check that all the arrays were allocated successfully */
        if (!cols[jj].array)
        {
            ffpmsg("ffiter failed to allocate memory arrays");
            *status = MEMORY_ALLOCATION;  /* memory allocation failed */
            goto cleanup;
        }
    }

    /*--------------------------------------------------*/
    /* main loop while there are values left to process */
    /*--------------------------------------------------*/

    nleft = totaln;

    while (nleft)
    {
      ntodo = minvalue(nleft, n_optimum); /* no. of values for this loop */

      /*  read input columns from FITS file(s)  */
      for (jj = 0; jj < n_cols; jj++)
      {
        if (cols[jj].iotype != OutputCol)
        {
          if (cols[jj].datatype == TSTRING)
          {
            stringptr = cols[jj].array;
            dataptr = stringptr + 1;
            defaultnull = col[jj].null.stringnull; /* ptr to the null value */
          }
          else
          {
            dataptr = (char *) cols[jj].array + col[jj].nullsize;
            defaultnull = &col[jj].null.charnull; /* ptr to the null value */
          }

          if (hdutype == IMAGE_HDU)   
          {
              if (ffgpv(cols[jj].fptr, cols[jj].datatype,
                    felement, cols[jj].repeat * ntodo, defaultnull,
                    dataptr,  &anynul, status) > 0)
              {
                 break;
              }
          }
          else
          {
	      if (ffgtcl(cols[jj].fptr, cols[jj].colnum, &typecode, &rept,&width, status) > 0)
	          goto cleanup;
		  
	      if (typecode<0)
	      {
	        /* get size of the variable length vector */
		ffgdes(cols[jj].fptr, cols[jj].colnum, frow,&cols[jj].repeat, NULL,status);
	      }
		
              if (ffgcv(cols[jj].fptr, cols[jj].datatype, cols[jj].colnum,
                    frow, felement, cols[jj].repeat * ntodo, defaultnull,
                    dataptr,  &anynul, status) > 0)
              {
                 break;
              }
          }

          /* copy the appropriate null value into first array element */

          if (anynul)   /* are there any nulls in the data? */
          {   
            if (cols[jj].datatype == TSTRING)
            {
              stringptr = cols[jj].array;
              memcpy(*stringptr, col[jj].null.stringnull, col[jj].nullsize);
            }
            else
            {
              memcpy(cols[jj].array, defaultnull, col[jj].nullsize);
            }
          }
          else /* no null values so copy zero into first element */
          {
            if (cols[jj].datatype == TSTRING)
            {
              stringptr = cols[jj].array;
              memset(*stringptr, 0, col[jj].nullsize);  
            }
            else
            {
              memset(cols[jj].array, 0, col[jj].nullsize);  
            }
          }
        }
      }

      if (*status > 0) 
         break;   /* looks like an error occurred; quit immediately */

      /* call work function */

      if (hdutype == IMAGE_HDU) 
          *status = work_fn(totaln, offset, felement, ntodo, n_cols, cols,
                    userPointer);
      else
          *status = work_fn(totaln, offset, frow, ntodo, n_cols, cols,
                    userPointer);

      if (*status > 0 || *status < -1 ) 
         break;   /* looks like an error occurred; quit immediately */

      /*  write output columns  before quiting if status = -1 */
      tstatus = 0;
      for (jj = 0; jj < n_cols; jj++)
      {
        if (cols[jj].iotype != InputCol)
        {
          if (cols[jj].datatype == TSTRING)
          {
            stringptr = cols[jj].array;
            dataptr = stringptr + 1;
            nullptr = *stringptr;
            nbytes = 2;
          }
          else
          {
            dataptr = (char *) cols[jj].array + col[jj].nullsize;
            nullptr = (char *) cols[jj].array;
            nbytes = col[jj].nullsize;
          }

          if (memcmp(nullptr, &zeros, nbytes) ) 
          {
            /* null value flag not zero; must check for and write nulls */
            if (hdutype == IMAGE_HDU)   
            {
                if (ffppn(cols[jj].fptr, cols[jj].datatype, 
                      felement, cols[jj].repeat * ntodo, dataptr,
                      nullptr, &tstatus) > 0)
                break;
            }
            else
            {
	    	if (ffgtcl(cols[jj].fptr, cols[jj].colnum, &typecode, &rept,&width, status) > 0)
		    goto cleanup;
		    
		if (typecode<0)  /* variable length array colum */
		{
		   ffgdes(cols[jj].fptr, cols[jj].colnum, frow,&cols[jj].repeat, NULL,status);
		}

                if (ffpcn(cols[jj].fptr, cols[jj].datatype, cols[jj].colnum, frow,
                      felement, cols[jj].repeat * ntodo, dataptr,
                      nullptr, &tstatus) > 0)
                break;
            }
          }
          else
          { 
            /* no null values; just write the array */
            if (hdutype == IMAGE_HDU)   
            {
                if (ffppr(cols[jj].fptr, cols[jj].datatype,
                      felement, cols[jj].repeat * ntodo, dataptr,
                      &tstatus) > 0)
                break;
            }
            else
            {
	    	if (ffgtcl(cols[jj].fptr, cols[jj].colnum, &typecode, &rept,&width, status) > 0)
		    goto cleanup;
		    
		if (typecode<0)  /* variable length array column */
		{
		   ffgdes(cols[jj].fptr, cols[jj].colnum, frow,&cols[jj].repeat, NULL,status);
		}

                 if (ffpcl(cols[jj].fptr, cols[jj].datatype, cols[jj].colnum, frow,
                      felement, cols[jj].repeat * ntodo, dataptr,
                      &tstatus) > 0)
                break;
            }
          }
        }
      }

      if (*status == 0)
         *status = tstatus;   /* propagate any error status from the writes */

      if (*status) 
         break;   /* exit on any error */

      nleft -= ntodo;

      if (hdutype == IMAGE_HDU)
          felement += ntodo;
      else
          frow  += ntodo;
    }

cleanup:

    /*----------------------------------*/
    /* free work arrays for the columns */
    /*----------------------------------*/

    for (jj = 0; jj < n_cols; jj++)
    {
        if (cols[jj].datatype == TSTRING)
        {
            if (cols[jj].array)
            {
                stringptr = cols[jj].array;
                free(*stringptr);     /* free the block of strings */
                free(col[jj].null.stringnull); /* free the null string */
            }
        }
        if (cols[jj].array)
            free(cols[jj].array); /* memory for the array of values from the col */
    }
    free(col);   /* the structure containing the null values */
    return(*status);
}

