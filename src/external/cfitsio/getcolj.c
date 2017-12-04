/*  This file, getcolj.c, contains routines that read data elements from   */
/*  a FITS image or table, with long data type.                            */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int ffgpvj( fitsfile *fptr,   /* I - FITS file pointer                       */
            long  group,      /* I - group to read (1 = 1st group)           */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            long  nulval,     /* I - value for undefined pixels              */
            long  *array,     /* O - array of values that are returned       */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Undefined elements will be set equal to NULVAL, unless NULVAL=0
  in which case no checking for undefined values will be performed.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    long row;
    char cdummy;
    int nullcheck = 1;
    long nullvalue;

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */
         nullvalue = nulval;  /* set local variable */

        fits_read_compressed_pixels(fptr, TLONG, firstelem, nelem,
            nullcheck, &nullvalue, array, NULL, anynul, status);
        return(*status);
    }

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    row=maxvalue(1,group);

    ffgclj(fptr, 2, row, firstelem, nelem, 1, 1, nulval,
               array, &cdummy, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgpfj( fitsfile *fptr,   /* I - FITS file pointer                       */
            long  group,      /* I - group to read (1 = 1st group)           */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            long  *array,     /* O - array of values that are returned       */
            char *nularray,   /* O - array of null pixel flags               */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Any undefined pixels in the returned array will be set = 0 and the 
  corresponding nularray value will be set = 1.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    long row;
    int nullcheck = 2;

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        fits_read_compressed_pixels(fptr, TLONG, firstelem, nelem,
            nullcheck, NULL, array, nularray, anynul, status);
        return(*status);
    }

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    row=maxvalue(1,group);

    ffgclj(fptr, 2, row, firstelem, nelem, 1, 2, 0L,
               array, nularray, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffg2dj(fitsfile *fptr,  /* I - FITS file pointer                       */
           long  group,     /* I - group to read (1 = 1st group)           */
           long  nulval,    /* set undefined pixels equal to this          */
           LONGLONG  ncols,     /* I - number of pixels in each row of array   */
           LONGLONG  naxis1,    /* I - FITS image NAXIS1 value                 */
           LONGLONG  naxis2,    /* I - FITS image NAXIS2 value                 */
           long  *array,    /* O - array to be filled and returned         */
           int  *anynul,    /* O - set to 1 if any values are null; else 0 */
           int  *status)    /* IO - error status                           */
/*
  Read an entire 2-D array of values to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of the
  FITS array is not the same as the array being read).  Any null
  values in the array will be set equal to the value of nulval, unless
  nulval = 0 in which case no null checking will be performed.
*/
{
    /* call the 3D reading routine, with the 3rd dimension = 1 */

    ffg3dj(fptr, group, nulval, ncols, naxis2, naxis1, naxis2, 1, array, 
           anynul, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffg3dj(fitsfile *fptr,  /* I - FITS file pointer                       */
           long  group,     /* I - group to read (1 = 1st group)           */
           long  nulval,    /* set undefined pixels equal to this          */
           LONGLONG  ncols,     /* I - number of pixels in each row of array   */
           LONGLONG  nrows,     /* I - number of rows in each plane of array   */
           LONGLONG  naxis1,    /* I - FITS image NAXIS1 value                 */
           LONGLONG  naxis2,    /* I - FITS image NAXIS2 value                 */
           LONGLONG  naxis3,    /* I - FITS image NAXIS3 value                 */
           long  *array,    /* O - array to be filled and returned         */
           int  *anynul,    /* O - set to 1 if any values are null; else 0 */
           int  *status)    /* IO - error status                           */
/*
  Read an entire 3-D array of values to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of the
  FITS array is not the same as the array being read).  Any null
  values in the array will be set equal to the value of nulval, unless
  nulval = 0 in which case no null checking will be performed.
*/
{
    long tablerow, ii, jj;
    char cdummy;
    int nullcheck = 1;
    long inc[] = {1,1,1};
    LONGLONG fpixel[] = {1,1,1}, nfits, narray;
    LONGLONG lpixel[3], nullvalue;

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        lpixel[0] = ncols;
        lpixel[1] = nrows;
        lpixel[2] = naxis3;
        nullvalue = nulval;  /* set local variable */

        fits_read_compressed_img(fptr, TLONG, fpixel, lpixel, inc,
            nullcheck, &nullvalue, array, NULL, anynul, status);
        return(*status);
    }

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */
    tablerow=maxvalue(1,group);

    if (ncols == naxis1 && nrows == naxis2)  /* arrays have same size? */
    {
       /* all the image pixels are contiguous, so read all at once */
       ffgclj(fptr, 2, tablerow, 1, naxis1 * naxis2 * naxis3, 1, 1, nulval,
               array, &cdummy, anynul, status);
       return(*status);
    }

    if (ncols < naxis1 || nrows < naxis2)
       return(*status = BAD_DIMEN);

    nfits = 1;   /* next pixel in FITS image to read */
    narray = 0;  /* next pixel in output array to be filled */

    /* loop over naxis3 planes in the data cube */
    for (jj = 0; jj < naxis3; jj++)
    {
      /* loop over the naxis2 rows in the FITS image, */
      /* reading naxis1 pixels to each row            */

      for (ii = 0; ii < naxis2; ii++)
      {
       if (ffgclj(fptr, 2, tablerow, nfits, naxis1, 1, 1, nulval,
          &array[narray], &cdummy, anynul, status) > 0)
          return(*status);

       nfits += naxis1;
       narray += ncols;
      }
      narray += (nrows - naxis2) * ncols;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgsvj(fitsfile *fptr, /* I - FITS file pointer                         */
           int  colnum,    /* I - number of the column to read (1 = 1st)    */
           int naxis,      /* I - number of dimensions in the FITS array    */
           long  *naxes,   /* I - size of each dimension                    */
           long  *blc,     /* I - 'bottom left corner' of the subsection    */
           long  *trc,     /* I - 'top right corner' of the subsection      */
           long  *inc,     /* I - increment to be applied in each dimension */
           long nulval,    /* I - value to set undefined pixels             */
           long *array,    /* O - array to be filled and returned           */
           int  *anynul,   /* O - set to 1 if any values are null; else 0   */
           int  *status)   /* IO - error status                             */
/*
  Read a subsection of data values from an image or a table column.
  This routine is set up to handle a maximum of nine dimensions.
*/
{
    long ii,i0, i1,i2,i3,i4,i5,i6,i7,i8,row,rstr,rstp,rinc;
    long str[9],stp[9],incr[9],dir[9];
    long nelem, nultyp, ninc, numcol;
    LONGLONG felem, dsize[10], blcll[9], trcll[9];
    int hdutype, anyf;
    char ldummy, msg[FLEN_ERRMSG];
    int nullcheck = 1;
    long nullvalue;

    if (naxis < 1 || naxis > 9)
    {
        sprintf(msg, "NAXIS = %d in call to ffgsvj is out of range", naxis);
        ffpmsg(msg);
        return(*status = BAD_DIMEN);
    }

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        for (ii=0; ii < naxis; ii++) {
	    blcll[ii] = blc[ii];
	    trcll[ii] = trc[ii];
	}

        nullvalue = nulval;  /* set local variable */

        fits_read_compressed_img(fptr, TLONG, blcll, trcll, inc,
            nullcheck, &nullvalue, array, NULL, anynul, status);
        return(*status);
    }

/*
    if this is a primary array, then the input COLNUM parameter should
    be interpreted as the row number, and we will alway read the image
    data from column 2 (any group parameters are in column 1).
*/
    if (ffghdt(fptr, &hdutype, status) > 0)
        return(*status);

    if (hdutype == IMAGE_HDU)
    {
        /* this is a primary array, or image extension */
        if (colnum == 0)
        {
            rstr = 1;
            rstp = 1;
        }
        else
        {
            rstr = colnum;
            rstp = colnum;
        }
        rinc = 1;
        numcol = 2;
    }
    else
    {
        /* this is a table, so the row info is in the (naxis+1) elements */
        rstr = blc[naxis];
        rstp = trc[naxis];
        rinc = inc[naxis];
        numcol = colnum;
    }

    nultyp = 1;
    if (anynul)
        *anynul = FALSE;

    i0 = 0;
    for (ii = 0; ii < 9; ii++)
    {
        str[ii] = 1;
        stp[ii] = 1;
        incr[ii] = 1;
        dsize[ii] = 1;
        dir[ii] = 1;
    }

    for (ii = 0; ii < naxis; ii++)
    {
      if (trc[ii] < blc[ii])
      {
        if (hdutype == IMAGE_HDU)
        {
           dir[ii] = -1;
        }
        else
        {
          sprintf(msg, "ffgsvj: illegal range specified for axis %ld", ii + 1);
          ffpmsg(msg);
          return(*status = BAD_PIX_NUM);
        }
      }

      str[ii] = blc[ii];
      stp[ii] = trc[ii];
      incr[ii] = inc[ii];
      dsize[ii + 1] = dsize[ii] * naxes[ii];
      dsize[ii] = dsize[ii] * dir[ii];
    }
    dsize[naxis] = dsize[naxis] * dir[naxis];

    if (naxis == 1 && naxes[0] == 1)
    {
      /* This is not a vector column, so read all the rows at once */
      nelem = (rstp - rstr) / rinc + 1;
      ninc = rinc;
      rstp = rstr;
    }
    else
    {
      /* have to read each row individually, in all dimensions */
      nelem = (stp[0]*dir[0] - str[0]*dir[0]) / inc[0] + 1;
      ninc = incr[0] * dir[0];
    }

    for (row = rstr; row <= rstp; row += rinc)
    {
     for (i8 = str[8]*dir[8]; i8 <= stp[8]*dir[8]; i8 += incr[8])
     {
      for (i7 = str[7]*dir[7]; i7 <= stp[7]*dir[7]; i7 += incr[7])
      {
       for (i6 = str[6]*dir[6]; i6 <= stp[6]*dir[6]; i6 += incr[6])
       {
        for (i5 = str[5]*dir[5]; i5 <= stp[5]*dir[5]; i5 += incr[5])
        {
         for (i4 = str[4]*dir[4]; i4 <= stp[4]*dir[4]; i4 += incr[4])
         {
          for (i3 = str[3]*dir[3]; i3 <= stp[3]*dir[3]; i3 += incr[3])
          {
           for (i2 = str[2]*dir[2]; i2 <= stp[2]*dir[2]; i2 += incr[2])
           {
            for (i1 = str[1]*dir[1]; i1 <= stp[1]*dir[1]; i1 += incr[1])
            {

              felem=str[0] + (i1 - dir[1]) * dsize[1] + (i2 - dir[2]) * dsize[2] + 
                             (i3 - dir[3]) * dsize[3] + (i4 - dir[4]) * dsize[4] +
                             (i5 - dir[5]) * dsize[5] + (i6 - dir[6]) * dsize[6] +
                             (i7 - dir[7]) * dsize[7] + (i8 - dir[8]) * dsize[8];

              if ( ffgclj(fptr, numcol, row, felem, nelem, ninc, nultyp,
                   nulval, &array[i0], &ldummy, &anyf, status) > 0)
                   return(*status);

              if (anyf && anynul)
                  *anynul = TRUE;

              i0 += nelem;
            }
           }
          }
         }
        }
       }
      }
     }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgsfj(fitsfile *fptr, /* I - FITS file pointer                         */
           int  colnum,    /* I - number of the column to read (1 = 1st)    */
           int naxis,      /* I - number of dimensions in the FITS array    */
           long  *naxes,   /* I - size of each dimension                    */
           long  *blc,     /* I - 'bottom left corner' of the subsection    */
           long  *trc,     /* I - 'top right corner' of the subsection      */
           long  *inc,     /* I - increment to be applied in each dimension */
           long *array,    /* O - array to be filled and returned           */
           char *flagval,  /* O - set to 1 if corresponding value is null   */
           int  *anynul,   /* O - set to 1 if any values are null; else 0   */
           int  *status)   /* IO - error status                             */
/*
  Read a subsection of data values from an image or a table column.
  This routine is set up to handle a maximum of nine dimensions.
*/
{
    long ii,i0, i1,i2,i3,i4,i5,i6,i7,i8,row,rstr,rstp,rinc;
    long str[9],stp[9],incr[9],dsize[10];
    LONGLONG blcll[9], trcll[9];
    long felem, nelem, nultyp, ninc, numcol;
    long nulval = 0;
    int hdutype, anyf;
    char msg[FLEN_ERRMSG];
    int nullcheck = 2;

    if (naxis < 1 || naxis > 9)
    {
        sprintf(msg, "NAXIS = %d in call to ffgsvj is out of range", naxis);
        ffpmsg(msg);
        return(*status = BAD_DIMEN);
    }

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        for (ii=0; ii < naxis; ii++) {
	    blcll[ii] = blc[ii];
	    trcll[ii] = trc[ii];
	}

        fits_read_compressed_img(fptr, TLONG, blcll, trcll, inc,
            nullcheck, NULL, array, flagval, anynul, status);
        return(*status);
    }

/*
    if this is a primary array, then the input COLNUM parameter should
    be interpreted as the row number, and we will alway read the image
    data from column 2 (any group parameters are in column 1).
*/
    if (ffghdt(fptr, &hdutype, status) > 0)
        return(*status);

    if (hdutype == IMAGE_HDU)
    {
        /* this is a primary array, or image extension */
        if (colnum == 0)
        {
            rstr = 1;
            rstp = 1;
        }
        else
        {
            rstr = colnum;
            rstp = colnum;
        }
        rinc = 1;
        numcol = 2;
    }
    else
    {
        /* this is a table, so the row info is in the (naxis+1) elements */
        rstr = blc[naxis];
        rstp = trc[naxis];
        rinc = inc[naxis];
        numcol = colnum;
    }

    nultyp = 2;
    if (anynul)
        *anynul = FALSE;

    i0 = 0;
    for (ii = 0; ii < 9; ii++)
    {
        str[ii] = 1;
        stp[ii] = 1;
        incr[ii] = 1;
        dsize[ii] = 1;
    }

    for (ii = 0; ii < naxis; ii++)
    {
      if (trc[ii] < blc[ii])
      {
        sprintf(msg, "ffgsvj: illegal range specified for axis %ld", ii + 1);
        ffpmsg(msg);
        return(*status = BAD_PIX_NUM);
      }

      str[ii] = blc[ii];
      stp[ii] = trc[ii];
      incr[ii] = inc[ii];
      dsize[ii + 1] = dsize[ii] * naxes[ii];
    }

    if (naxis == 1 && naxes[0] == 1)
    {
      /* This is not a vector column, so read all the rows at once */
      nelem = (rstp - rstr) / rinc + 1;
      ninc = rinc;
      rstp = rstr;
    }
    else
    {
      /* have to read each row individually, in all dimensions */
      nelem = (stp[0] - str[0]) / inc[0] + 1;
      ninc = incr[0];
    }

    for (row = rstr; row <= rstp; row += rinc)
    {
     for (i8 = str[8]; i8 <= stp[8]; i8 += incr[8])
     {
      for (i7 = str[7]; i7 <= stp[7]; i7 += incr[7])
      {
       for (i6 = str[6]; i6 <= stp[6]; i6 += incr[6])
       {
        for (i5 = str[5]; i5 <= stp[5]; i5 += incr[5])
        {
         for (i4 = str[4]; i4 <= stp[4]; i4 += incr[4])
         {
          for (i3 = str[3]; i3 <= stp[3]; i3 += incr[3])
          {
           for (i2 = str[2]; i2 <= stp[2]; i2 += incr[2])
           {
            for (i1 = str[1]; i1 <= stp[1]; i1 += incr[1])
            {
              felem=str[0] + (i1 - 1) * dsize[1] + (i2 - 1) * dsize[2] + 
                             (i3 - 1) * dsize[3] + (i4 - 1) * dsize[4] +
                             (i5 - 1) * dsize[5] + (i6 - 1) * dsize[6] +
                             (i7 - 1) * dsize[7] + (i8 - 1) * dsize[8];

              if ( ffgclj(fptr, numcol, row, felem, nelem, ninc, nultyp,
                   nulval, &array[i0], &flagval[i0], &anyf, status) > 0)
                   return(*status);

              if (anyf && anynul)
                  *anynul = TRUE;

              i0 += nelem;
            }
           }
          }
         }
        }
       }
      }
     }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffggpj( fitsfile *fptr,   /* I - FITS file pointer                       */
            long  group,      /* I - group to read (1 = 1st group)           */
            long  firstelem,  /* I - first vector element to read (1 = 1st)  */
            long  nelem,      /* I - number of values to read                */
            long  *array,     /* O - array of values that are returned       */
            int  *status)     /* IO - error status                           */
/*
  Read an array of group parameters from the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
*/
{
    long row;
    int idummy;
    char cdummy;
    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    row=maxvalue(1,group);

    ffgclj(fptr, 1, row, firstelem, nelem, 1, 1, 0L,
               array, &cdummy, &idummy, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcvj(fitsfile *fptr,   /* I - FITS file pointer                       */
           int  colnum,      /* I - number of column to read (1 = 1st col)  */
           LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
           LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
           LONGLONG  nelem,      /* I - number of values to read                */
           long  nulval,     /* I - value for null pixels                   */
           long *array,      /* O - array of values that are read           */
           int  *anynul,     /* O - set to 1 if any values are null; else 0 */
           int  *status)     /* IO - error status                           */
/*
  Read an array of values from a column in the current FITS HDU. Automatic
  datatype conversion will be performed if the datatype of the column does not
  match the datatype of the array parameter. The output values will be scaled 
  by the FITS TSCALn and TZEROn values if these values have been defined.
  Any undefined pixels will be set equal to the value of 'nulval' unless
  nulval = 0 in which case no checks for undefined pixels will be made.
*/
{
    char cdummy;

    ffgclj(fptr, colnum, firstrow, firstelem, nelem, 1, 1, nulval,
           array, &cdummy, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcfj(fitsfile *fptr,   /* I - FITS file pointer                       */
           int  colnum,      /* I - number of column to read (1 = 1st col)  */
           LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
           LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
           LONGLONG  nelem,      /* I - number of values to read                */
           long  *array,     /* O - array of values that are read           */
           char *nularray,   /* O - array of flags: 1 if null pixel; else 0 */
           int  *anynul,     /* O - set to 1 if any values are null; else 0 */
           int  *status)     /* IO - error status                           */
/*
  Read an array of values from a column in the current FITS HDU. Automatic
  datatype conversion will be performed if the datatype of the column does not
  match the datatype of the array parameter. The output values will be scaled 
  by the FITS TSCALn and TZEROn values if these values have been defined.
  Nularray will be set = 1 if the corresponding array pixel is undefined, 
  otherwise nularray will = 0.
*/
{
    long dummy = 0;

    ffgclj(fptr, colnum, firstrow, firstelem, nelem, 1, 2, dummy,
           array, nularray, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgclj( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            long  elemincre,  /* I - pixel increment; e.g., 2 = every other  */
            int   nultyp,     /* I - null value handling code:               */
                              /*     1: set undefined pixels = nulval        */
                              /*     2: set nularray=1 for undefined pixels  */
            long  nulval,     /* I - value for null pixels if nultyp = 1     */
            long  *array,     /* O - array of values that are read           */
            char *nularray,   /* O - array of flags = 1 if nultyp = 2        */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from a column in the current FITS HDU.
  The column number may refer to a real column in an ASCII or binary table, 
  or it may refer be a virtual column in a 1 or more grouped FITS primary
  array or image extension.  FITSIO treats a primary array as a binary table
  with 2 vector columns: the first column contains the group parameters (often
  with length = 0) and the second column contains the array of image pixels.
  Each row of the table represents a group in the case of multigroup FITS
  images.

  The output array of values will be converted from the datatype of the column 
  and will be scaled by the FITS TSCALn and TZEROn values if necessary.
*/
{
    double scale, zero, power = 1., dtemp;
    int tcode, maxelem2, hdutype, xcode, decimals;
    long twidth, incre;
    long ii, xwidth, ntodo;
    int convert, nulcheck, readcheck = 0;
    LONGLONG repeat, startpos, elemnum, readptr, tnull;
    LONGLONG rowlen, rownum, remain, next, rowincre, maxelem;
    char tform[20];
    char message[81];
    char snull[20];   /*  the FITS null value if reading from ASCII table  */

    double cbuff[DBUFFSIZE / sizeof(double)]; /* align cbuff on word boundary */
    void *buffer;

    if (*status > 0 || nelem == 0)  /* inherit input status value if > 0 */
        return(*status);

    buffer = cbuff;

    if (anynul)
        *anynul = 0;

    if (nultyp == 2)
        memset(nularray, 0, (size_t) nelem);   /* initialize nullarray */

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/
    if (elemincre < 0)
        readcheck = -1;  /* don't do range checking in this case */

    if (ffgcprll(fptr, colnum, firstrow, firstelem, nelem, readcheck, &scale, &zero,
         tform, &twidth, &tcode, &maxelem2, &startpos, &elemnum, &incre,
         &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0 )
         return(*status);
    maxelem = maxelem2;

    incre *= elemincre;   /* multiply incre to just get every nth pixel */

    if (tcode == TSTRING)    /* setup for ASCII tables */
    {
      /* get the number of implied decimal places if no explicit decmal point */
      ffasfm(tform, &xcode, &xwidth, &decimals, status); 
      for(ii = 0; ii < decimals; ii++)
        power *= 10.;
    }
    /*------------------------------------------------------------------*/
    /*  Decide whether to check for null values in the input FITS file: */
    /*------------------------------------------------------------------*/
    nulcheck = nultyp; /* by default check for null values in the FITS file */

    if (nultyp == 1 && nulval == 0)
       nulcheck = 0;    /* calling routine does not want to check for nulls */

    else if (tcode%10 == 1 &&        /* if reading an integer column, and  */ 
            tnull == NULL_UNDEFINED) /* if a null value is not defined,    */
            nulcheck = 0;            /* then do not check for null values. */

    else if (tcode == TSHORT && (tnull > SHRT_MAX || tnull < SHRT_MIN) )
            nulcheck = 0;            /* Impossible null value */

    else if (tcode == TBYTE && (tnull > 255 || tnull < 0) )
            nulcheck = 0;            /* Impossible null value */

    else if (tcode == TSTRING && snull[0] == ASCII_NULL_UNDEFINED)
         nulcheck = 0;

    /*----------------------------------------------------------------------*/
    /*  If FITS column and output data array have same datatype, then we do */
    /*  not need to use a temporary buffer to store intermediate datatype.  */
    /*----------------------------------------------------------------------*/
    convert = 1;
    if ((tcode == TLONG) && (LONGSIZE == 32))  /* Special Case:                        */
    {                             /* no type convertion required, so read */
                                  /* data directly into output buffer.    */

        if (nelem < (LONGLONG)INT32_MAX/4) {
            maxelem = nelem;
        } else {
            maxelem = INT32_MAX/4;   
        }

        if (nulcheck == 0 && scale == 1. && zero == 0. )
            convert = 0;  /* no need to scale data or find nulls */
    }

    /*---------------------------------------------------------------------*/
    /*  Now read the pixels from the FITS column. If the column does not   */
    /*  have the same datatype as the output array, then we have to read   */
    /*  the raw values into a temporary buffer (of limited size).  In      */
    /*  the case of a vector colum read only 1 vector of values at a time  */
    /*  then skip to the next row if more values need to be read.          */
    /*  After reading the raw values, then call the fffXXYY routine to (1) */
    /*  test for undefined values, (2) convert the datatype if necessary,  */
    /*  and (3) scale the values by the FITS TSCALn and TZEROn linear      */
    /*  scaling parameters.                                                */
    /*---------------------------------------------------------------------*/
    remain = nelem;           /* remaining number of values to read */
    next = 0;                 /* next element in array to be read   */
    rownum = 0;               /* row number, relative to firstrow   */

    while (remain)
    {
        /* limit the number of pixels to read at one time to the number that
           will fit in the buffer or to the number of pixels that remain in
           the current vector, which ever is smaller.
        */
        ntodo = (long) minvalue(remain, maxelem);
        if (elemincre >= 0)
        {
          ntodo = (long) minvalue(ntodo, ((repeat - elemnum - 1)/elemincre +1));
        }
        else
        {
          ntodo = (long) minvalue(ntodo, (elemnum/(-elemincre) +1));
        }

        readptr = startpos + ((LONGLONG)rownum * rowlen) + (elemnum * (incre / elemincre));

        switch (tcode) 
        {
            case (TLONG):
	      if (LONGSIZE == 32) {
                ffgi4b(fptr, readptr, ntodo, incre, (INT32BIT *) &array[next],
                       status);
                if (convert)
                    fffi4i4((INT32BIT *) &array[next], ntodo, scale, zero, 
                           nulcheck, (INT32BIT) tnull, nulval, &nularray[next], 
                            anynul, &array[next], status);
	      } else { /* case where sizeof(long) = 8 */
                ffgi4b(fptr, readptr, ntodo, incre, (INT32BIT *) buffer,
                       status);
                if (convert)
                    fffi4i4((INT32BIT *) buffer, ntodo, scale, zero, 
                           nulcheck, (INT32BIT) tnull, nulval, &nularray[next], 
                            anynul, &array[next], status);
	      }

                break;
            case (TLONGLONG):
                ffgi8b(fptr, readptr, ntodo, incre, (long *) buffer, status);
                fffi8i4((LONGLONG *) buffer, ntodo, scale, zero, 
                           nulcheck, tnull, nulval, &nularray[next], 
                            anynul, &array[next], status);
                break;
            case (TBYTE):
                ffgi1b(fptr, readptr, ntodo, incre, (unsigned char *) buffer,
                       status);
                fffi1i4((unsigned char *) buffer, ntodo, scale, zero, nulcheck, 
                     (unsigned char) tnull, nulval, &nularray[next], anynul, 
                     &array[next], status);
                break;
            case (TSHORT):
                ffgi2b(fptr, readptr, ntodo, incre, (short  *) buffer, status);
                fffi2i4((short  *) buffer, ntodo, scale, zero, nulcheck, 
                      (short) tnull, nulval, &nularray[next], anynul, 
                      &array[next], status);
                break;
            case (TFLOAT):
                ffgr4b(fptr, readptr, ntodo, incre, (float  *) buffer, status);
                fffr4i4((float  *) buffer, ntodo, scale, zero, nulcheck, 
                       nulval, &nularray[next], anynul, 
                       &array[next], status);
                break;
            case (TDOUBLE):
                ffgr8b(fptr, readptr, ntodo, incre, (double *) buffer, status);
                fffr8i4((double *) buffer, ntodo, scale, zero, nulcheck, 
                          nulval, &nularray[next], anynul, 
                          &array[next], status);
                break;
            case (TSTRING):
                ffmbyt(fptr, readptr, REPORT_EOF, status);
       
                if (incre == twidth)    /* contiguous bytes */
                     ffgbyt(fptr, ntodo * twidth, buffer, status);
                else
                     ffgbytoff(fptr, twidth, ntodo, incre - twidth, buffer,
                               status);

                fffstri4((char *) buffer, ntodo, scale, zero, twidth, power,
                     nulcheck, snull, nulval, &nularray[next], anynul,
                     &array[next], status);
                break;

            default:  /*  error trap for invalid column format */
                sprintf(message, 
                   "Cannot read numbers from column %d which has format %s",
                    colnum, tform);
                ffpmsg(message);
                if (hdutype == ASCII_TBL)
                    return(*status = BAD_ATABLE_FORMAT);
                else
                    return(*status = BAD_BTABLE_FORMAT);

        } /* End of switch block */

        /*-------------------------*/
        /*  Check for fatal error  */
        /*-------------------------*/
        if (*status > 0)  /* test for error during previous read operation */
        {
	  dtemp = (double) next;
          if (hdutype > 0)
            sprintf(message,
            "Error reading elements %.0f thru %.0f from column %d (ffgclj).",
              dtemp+1., dtemp+ntodo, colnum);
          else
            sprintf(message,
            "Error reading elements %.0f thru %.0f from image (ffgclj).",
              dtemp+1., dtemp+ntodo);

          ffpmsg(message);
          return(*status);
        }

        /*--------------------------------------------*/
        /*  increment the counters for the next loop  */
        /*--------------------------------------------*/
        remain -= ntodo;
        if (remain)
        {
            next += ntodo;
            elemnum = elemnum + (ntodo * elemincre);

            if (elemnum >= repeat)  /* completed a row; start on later row */
            {
                rowincre = elemnum / repeat;
                rownum += rowincre;
                elemnum = elemnum - (rowincre * repeat);
            }
            else if (elemnum < 0)  /* completed a row; start on a previous row */
            {
                rowincre = (-elemnum - 1) / repeat + 1;
                rownum -= rowincre;
                elemnum = (rowincre * repeat) + elemnum;
            }
        }
    }  /*  End of main while Loop  */


    /*--------------------------------*/
    /*  check for numerical overflow  */
    /*--------------------------------*/
    if (*status == OVERFLOW_ERR)
    {
        ffpmsg(
        "Numerical overflow during type conversion while reading FITS data.");
        *status = NUM_OVERFLOW;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffi1i4(unsigned char *input, /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            unsigned char tnull,  /* I - value of FITS TNULLn keyword if any */
            long nullval,         /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            long *output,         /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to tnull.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
                output[ii] = (long) input[ii];  /* copy input to output */
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (dvalue > DLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                    output[ii] = (long) input[ii];
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (dvalue > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffi2i4(short *input,         /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            short tnull,          /* I - value of FITS TNULLn keyword if any */
            long nullval,         /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            long *output,         /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to tnull.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
                output[ii] = (long) input[ii];   /* copy input to output */
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (dvalue > DLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                    output[ii] = (long) input[ii];
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (dvalue > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffi4i4(INT32BIT *input,      /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            INT32BIT tnull,       /* I - value of FITS TNULLn keyword if any */
            long nullval,         /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            long *output,         /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to tnull.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++) { 
                 output[ii] = (long) input[ii];   /* copy input to output */
	    }
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (dvalue > DLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                    output[ii] = input[ii];
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (dvalue > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffi8i4(LONGLONG *input,      /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            LONGLONG tnull,       /* I - value of FITS TNULLn keyword if any */
            long nullval,         /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            long *output,         /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to tnull.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] < LONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (input[ii] > LONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) input[ii];
            }
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (dvalue > DLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    if (input[ii] < LONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (input[ii] > LONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) input[ii];
                }
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (dvalue > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffr4i4(float *input,         /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            long nullval,         /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            long *output,         /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to NaN.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;
    short *sptr, iret;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] < DLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (input[ii] > DLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) input[ii];
            }
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (dvalue > DLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        sptr = (short *) input;

#if BYTESWAPPED && MACHINE != VAXVMS && MACHINE != ALPHAVMS
        sptr++;       /* point to MSBs */
#endif

        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++, sptr += 2)
            {
              if (0 != (iret = fnan(*sptr) ) )  /* test for NaN or underflow */
              {
                  if (iret == 1)  /* is it a NaN? */
                  {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                  }
                  else            /* it's an underflow */
                     output[ii] = 0;
              }
              else
                {
                    if (input[ii] < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (input[ii] > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) input[ii];
                }
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++, sptr += 2)
            {
              if (0 != (iret = fnan(*sptr) ) )  /* test for NaN or underflow */
              {
                  if (iret == 1)  /* is it a NaN? */
                  {  
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                  }
                  else            /* it's an underflow */
                  {
                    if (zero < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (zero > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) zero;
                  }
              }
              else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (dvalue > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffr8i4(double *input,        /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            long nullval,         /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            long *output,         /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to NaN.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;
    short *sptr, iret;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] < DLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (input[ii] > DLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) input[ii];
            }
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MIN;
                }
                else if (dvalue > DLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONG_MAX;
                }
                else
                    output[ii] = (long) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        sptr = (short *) input;

#if BYTESWAPPED && MACHINE != VAXVMS && MACHINE != ALPHAVMS
        sptr += 3;       /* point to MSBs */
#endif
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++, sptr += 4)
            {
              if (0 != (iret = dnan(*sptr)) )  /* test for NaN or underflow */
              {
                  if (iret == 1)  /* is it a NaN? */
                  {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                  }
                  else            /* it's an underflow */
                     output[ii] = 0;
              }
              else
                {
                    if (input[ii] < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (input[ii] > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) input[ii];
                }
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++, sptr += 4)
            {
              if (0 != (iret = dnan(*sptr)) )  /* test for NaN or underflow */
              {
                  if (iret == 1)  /* is it a NaN? */
                  {  
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                  }
                  else            /* it's an underflow */
                  {
                    if (zero < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (zero > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) zero;
                  }
              }
              else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MIN;
                    }
                    else if (dvalue > DLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONG_MAX;
                    }
                    else
                        output[ii] = (long) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffstri4(char *input,         /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            long twidth,          /* I - width of each substring of chars    */
            double implipower,    /* I - power of 10 of implied decimal      */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            char  *snull,         /* I - value of FITS null string, if any   */
            long nullval,         /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            long *output,         /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file. Check
  for null values and do scaling if required. The nullcheck code value
  determines how any null values in the input array are treated. A null
  value is an input pixel that is equal to snull.  If nullcheck= 0, then
  no special checking for nulls is performed.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    int nullen;
    long ii;
    double dvalue;
    char *cstring, message[81];
    char *cptr, *tpos;
    char tempstore, chrzero = '0';
    double val, power;
    int exponent, sign, esign, decpt;

    nullen = strlen(snull);
    cptr = input;  /* pointer to start of input string */
    for (ii = 0; ii < ntodo; ii++)
    {
      cstring = cptr;
      /* temporarily insert a null terminator at end of the string */
      tpos = cptr + twidth;
      tempstore = *tpos;
      *tpos = 0;

      /* check if null value is defined, and if the    */
      /* column string is identical to the null string */
      if (snull[0] != ASCII_NULL_UNDEFINED && 
         !strncmp(snull, cptr, nullen) )
      {
        if (nullcheck)  
        {
          *anynull = 1;    
          if (nullcheck == 1)
            output[ii] = nullval;
          else
            nullarray[ii] = 1;
        }
        cptr += twidth;
      }
      else
      {
        /* value is not the null value, so decode it */
        /* remove any embedded blank characters from the string */

        decpt = 0;
        sign = 1;
        val  = 0.;
        power = 1.;
        exponent = 0;
        esign = 1;

        while (*cptr == ' ')               /* skip leading blanks */
           cptr++;

        if (*cptr == '-' || *cptr == '+')  /* check for leading sign */
        {
          if (*cptr == '-')
             sign = -1;

          cptr++;

          while (*cptr == ' ')         /* skip blanks between sign and value */
            cptr++;
        }

        while (*cptr >= '0' && *cptr <= '9')
        {
          val = val * 10. + *cptr - chrzero;  /* accumulate the value */
          cptr++;

          while (*cptr == ' ')         /* skip embedded blanks in the value */
            cptr++;
        }

        if (*cptr == '.' || *cptr == ',')    /* check for decimal point */
        {
          decpt = 1;       /* set flag to show there was a decimal point */
          cptr++;
          while (*cptr == ' ')         /* skip any blanks */
            cptr++;

          while (*cptr >= '0' && *cptr <= '9')
          {
            val = val * 10. + *cptr - chrzero;  /* accumulate the value */
            power = power * 10.;
            cptr++;

            while (*cptr == ' ')         /* skip embedded blanks in the value */
              cptr++;
          }
        }

        if (*cptr == 'E' || *cptr == 'D')  /* check for exponent */
        {
          cptr++;
          while (*cptr == ' ')         /* skip blanks */
              cptr++;
  
          if (*cptr == '-' || *cptr == '+')  /* check for exponent sign */
          {
            if (*cptr == '-')
               esign = -1;

            cptr++;

            while (*cptr == ' ')        /* skip blanks between sign and exp */
              cptr++;
          }

          while (*cptr >= '0' && *cptr <= '9')
          {
            exponent = exponent * 10 + *cptr - chrzero;  /* accumulate exp */
            cptr++;

            while (*cptr == ' ')         /* skip embedded blanks */
              cptr++;
          }
        }

        if (*cptr  != 0)  /* should end up at the null terminator */
        {
          sprintf(message, "Cannot read number from ASCII table");
          ffpmsg(message);
          sprintf(message, "Column field = %s.", cstring);
          ffpmsg(message);
          /* restore the char that was overwritten by the null */
          *tpos = tempstore;
          return(*status = BAD_C2D);
        }

        if (!decpt)  /* if no explicit decimal, use implied */
           power = implipower;

        dvalue = (sign * val / power) * pow(10., (double) (esign * exponent));

        dvalue = dvalue * scale + zero;   /* apply the scaling */

        if (dvalue < DLONG_MIN)
        {
            *status = OVERFLOW_ERR;
            output[ii] = LONG_MIN;
        }
        else if (dvalue > DLONG_MAX)
        {
            *status = OVERFLOW_ERR;
            output[ii] = LONG_MAX;
        }
        else
            output[ii] = (long) dvalue;
      }
      /* restore the char that was overwritten by the null */
      *tpos = tempstore;
    }
    return(*status);
}

/* ======================================================================== */
/*      the following routines support the 'long long' data type            */
/* ======================================================================== */

/*--------------------------------------------------------------------------*/
int ffgpvjj(fitsfile *fptr,   /* I - FITS file pointer                       */
            long  group,      /* I - group to read (1 = 1st group)           */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            LONGLONG  nulval, /* I - value for undefined pixels              */
            LONGLONG  *array, /* O - array of values that are returned       */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Undefined elements will be set equal to NULVAL, unless NULVAL=0
  in which case no checking for undefined values will be performed.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    long row;
    char cdummy;
    int nullcheck = 1;
    LONGLONG nullvalue;

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */
         nullvalue = nulval;  /* set local variable */

        fits_read_compressed_pixels(fptr, TLONGLONG, firstelem, nelem,
            nullcheck, &nullvalue, array, NULL, anynul, status);
        return(*status);
    }

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    row=maxvalue(1,group);

    ffgcljj(fptr, 2, row, firstelem, nelem, 1, 1, nulval,
               array, &cdummy, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgpfjj(fitsfile *fptr,   /* I - FITS file pointer                       */
            long  group,      /* I - group to read (1 = 1st group)           */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            LONGLONG  *array, /* O - array of values that are returned       */
            char *nularray,   /* O - array of null pixel flags               */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
  Any undefined pixels in the returned array will be set = 0 and the 
  corresponding nularray value will be set = 1.
  ANYNUL is returned with a value of .true. if any pixels are undefined.
*/
{
    long row;
    int nullcheck = 2;
    LONGLONG dummy = 0;

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        fits_read_compressed_pixels(fptr, TLONGLONG, firstelem, nelem,
            nullcheck, NULL, array, nularray, anynul, status);
        return(*status);
    }

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    row=maxvalue(1,group);

    ffgcljj(fptr, 2, row, firstelem, nelem, 1, 2, dummy,
               array, nularray, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffg2djj(fitsfile *fptr, /* I - FITS file pointer                       */
           long  group,     /* I - group to read (1 = 1st group)           */
           LONGLONG nulval ,/* set undefined pixels equal to this          */
           LONGLONG  ncols,     /* I - number of pixels in each row of array   */
           LONGLONG  naxis1,    /* I - FITS image NAXIS1 value                 */
           LONGLONG  naxis2,    /* I - FITS image NAXIS2 value                 */
           LONGLONG  *array,/* O - array to be filled and returned         */
           int  *anynul,    /* O - set to 1 if any values are null; else 0 */
           int  *status)    /* IO - error status                           */
/*
  Read an entire 2-D array of values to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of the
  FITS array is not the same as the array being read).  Any null
  values in the array will be set equal to the value of nulval, unless
  nulval = 0 in which case no null checking will be performed.
*/
{
    /* call the 3D reading routine, with the 3rd dimension = 1 */

    ffg3djj(fptr, group, nulval, ncols, naxis2, naxis1, naxis2, 1, array, 
           anynul, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffg3djj(fitsfile *fptr, /* I - FITS file pointer                       */
           long  group,     /* I - group to read (1 = 1st group)           */
           LONGLONG nulval, /* set undefined pixels equal to this          */
           LONGLONG  ncols,     /* I - number of pixels in each row of array   */
           LONGLONG  nrows,     /* I - number of rows in each plane of array   */
           LONGLONG  naxis1,    /* I - FITS image NAXIS1 value                 */
           LONGLONG  naxis2,    /* I - FITS image NAXIS2 value                 */
           LONGLONG  naxis3,    /* I - FITS image NAXIS3 value                 */
           LONGLONG  *array,/* O - array to be filled and returned         */
           int  *anynul,    /* O - set to 1 if any values are null; else 0 */
           int  *status)    /* IO - error status                           */
/*
  Read an entire 3-D array of values to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of the
  FITS array is not the same as the array being read).  Any null
  values in the array will be set equal to the value of nulval, unless
  nulval = 0 in which case no null checking will be performed.
*/
{
    long tablerow, ii, jj;
    char cdummy;
    int nullcheck = 1;
    long inc[] = {1,1,1};
    LONGLONG fpixel[] = {1,1,1}, nfits, narray;
    LONGLONG lpixel[3];
    LONGLONG nullvalue;

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        lpixel[0] = ncols;
        lpixel[1] = nrows;
        lpixel[2] = naxis3;
        nullvalue = nulval;  /* set local variable */

        fits_read_compressed_img(fptr, TLONGLONG, fpixel, lpixel, inc,
            nullcheck, &nullvalue, array, NULL, anynul, status);
        return(*status);
    }

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */
    tablerow=maxvalue(1,group);

    if (ncols == naxis1 && nrows == naxis2)  /* arrays have same size? */
    {
       /* all the image pixels are contiguous, so read all at once */
       ffgcljj(fptr, 2, tablerow, 1, naxis1 * naxis2 * naxis3, 1, 1, nulval,
               array, &cdummy, anynul, status);
       return(*status);
    }

    if (ncols < naxis1 || nrows < naxis2)
       return(*status = BAD_DIMEN);

    nfits = 1;   /* next pixel in FITS image to read */
    narray = 0;  /* next pixel in output array to be filled */

    /* loop over naxis3 planes in the data cube */
    for (jj = 0; jj < naxis3; jj++)
    {
      /* loop over the naxis2 rows in the FITS image, */
      /* reading naxis1 pixels to each row            */

      for (ii = 0; ii < naxis2; ii++)
      {
       if (ffgcljj(fptr, 2, tablerow, nfits, naxis1, 1, 1, nulval,
          &array[narray], &cdummy, anynul, status) > 0)
          return(*status);

       nfits += naxis1;
       narray += ncols;
      }
      narray += (nrows - naxis2) * ncols;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgsvjj(fitsfile *fptr, /* I - FITS file pointer                         */
           int  colnum,    /* I - number of the column to read (1 = 1st)    */
           int naxis,      /* I - number of dimensions in the FITS array    */
           long  *naxes,   /* I - size of each dimension                    */
           long  *blc,     /* I - 'bottom left corner' of the subsection    */
           long  *trc,     /* I - 'top right corner' of the subsection      */
           long  *inc,     /* I - increment to be applied in each dimension */
           LONGLONG nulval,/* I - value to set undefined pixels             */
           LONGLONG *array,/* O - array to be filled and returned           */
           int  *anynul,   /* O - set to 1 if any values are null; else 0   */
           int  *status)   /* IO - error status                             */
/*
  Read a subsection of data values from an image or a table column.
  This routine is set up to handle a maximum of nine dimensions.
*/
{
    long ii,i0, i1,i2,i3,i4,i5,i6,i7,i8,row,rstr,rstp,rinc;
    long str[9],stp[9],incr[9],dir[9];
    long nelem, nultyp, ninc, numcol;
    LONGLONG felem, dsize[10], blcll[9], trcll[9];
    int hdutype, anyf;
    char ldummy, msg[FLEN_ERRMSG];
    int nullcheck = 1;
    LONGLONG nullvalue;

    if (naxis < 1 || naxis > 9)
    {
        sprintf(msg, "NAXIS = %d in call to ffgsvj is out of range", naxis);
        ffpmsg(msg);
        return(*status = BAD_DIMEN);
    }

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        for (ii=0; ii < naxis; ii++) {
	    blcll[ii] = blc[ii];
	    trcll[ii] = trc[ii];
	}

        nullvalue = nulval;  /* set local variable */

        fits_read_compressed_img(fptr, TLONGLONG, blcll, trcll, inc,
            nullcheck, &nullvalue, array, NULL, anynul, status);
        return(*status);
    }

/*
    if this is a primary array, then the input COLNUM parameter should
    be interpreted as the row number, and we will alway read the image
    data from column 2 (any group parameters are in column 1).
*/
    if (ffghdt(fptr, &hdutype, status) > 0)
        return(*status);

    if (hdutype == IMAGE_HDU)
    {
        /* this is a primary array, or image extension */
        if (colnum == 0)
        {
            rstr = 1;
            rstp = 1;
        }
        else
        {
            rstr = colnum;
            rstp = colnum;
        }
        rinc = 1;
        numcol = 2;
    }
    else
    {
        /* this is a table, so the row info is in the (naxis+1) elements */
        rstr = blc[naxis];
        rstp = trc[naxis];
        rinc = inc[naxis];
        numcol = colnum;
    }

    nultyp = 1;
    if (anynul)
        *anynul = FALSE;

    i0 = 0;
    for (ii = 0; ii < 9; ii++)
    {
        str[ii] = 1;
        stp[ii] = 1;
        incr[ii] = 1;
        dsize[ii] = 1;
        dir[ii] = 1;
    }

    for (ii = 0; ii < naxis; ii++)
    {
      if (trc[ii] < blc[ii])
      {
        if (hdutype == IMAGE_HDU)
        {
           dir[ii] = -1;
        }
        else
        {
          sprintf(msg, "ffgsvj: illegal range specified for axis %ld", ii + 1);
          ffpmsg(msg);
          return(*status = BAD_PIX_NUM);
        }
      }

      str[ii] = blc[ii];
      stp[ii] = trc[ii];
      incr[ii] = inc[ii];
      dsize[ii + 1] = dsize[ii] * naxes[ii];
      dsize[ii] = dsize[ii] * dir[ii];
    }
    dsize[naxis] = dsize[naxis] * dir[naxis];

    if (naxis == 1 && naxes[0] == 1)
    {
      /* This is not a vector column, so read all the rows at once */
      nelem = (rstp - rstr) / rinc + 1;
      ninc = rinc;
      rstp = rstr;
    }
    else
    {
      /* have to read each row individually, in all dimensions */
      nelem = (stp[0]*dir[0] - str[0]*dir[0]) / inc[0] + 1;
      ninc = incr[0] * dir[0];
    }

    for (row = rstr; row <= rstp; row += rinc)
    {
     for (i8 = str[8]*dir[8]; i8 <= stp[8]*dir[8]; i8 += incr[8])
     {
      for (i7 = str[7]*dir[7]; i7 <= stp[7]*dir[7]; i7 += incr[7])
      {
       for (i6 = str[6]*dir[6]; i6 <= stp[6]*dir[6]; i6 += incr[6])
       {
        for (i5 = str[5]*dir[5]; i5 <= stp[5]*dir[5]; i5 += incr[5])
        {
         for (i4 = str[4]*dir[4]; i4 <= stp[4]*dir[4]; i4 += incr[4])
         {
          for (i3 = str[3]*dir[3]; i3 <= stp[3]*dir[3]; i3 += incr[3])
          {
           for (i2 = str[2]*dir[2]; i2 <= stp[2]*dir[2]; i2 += incr[2])
           {
            for (i1 = str[1]*dir[1]; i1 <= stp[1]*dir[1]; i1 += incr[1])
            {

              felem=str[0] + (i1 - dir[1]) * dsize[1] + (i2 - dir[2]) * dsize[2] + 
                             (i3 - dir[3]) * dsize[3] + (i4 - dir[4]) * dsize[4] +
                             (i5 - dir[5]) * dsize[5] + (i6 - dir[6]) * dsize[6] +
                             (i7 - dir[7]) * dsize[7] + (i8 - dir[8]) * dsize[8];

              if ( ffgcljj(fptr, numcol, row, felem, nelem, ninc, nultyp,
                   nulval, &array[i0], &ldummy, &anyf, status) > 0)
                   return(*status);

              if (anyf && anynul)
                  *anynul = TRUE;

              i0 += nelem;
            }
           }
          }
         }
        }
       }
      }
     }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgsfjj(fitsfile *fptr, /* I - FITS file pointer                         */
           int  colnum,    /* I - number of the column to read (1 = 1st)    */
           int naxis,      /* I - number of dimensions in the FITS array    */
           long  *naxes,   /* I - size of each dimension                    */
           long  *blc,     /* I - 'bottom left corner' of the subsection    */
           long  *trc,     /* I - 'top right corner' of the subsection      */
           long  *inc,     /* I - increment to be applied in each dimension */
           LONGLONG *array,/* O - array to be filled and returned           */
           char *flagval,  /* O - set to 1 if corresponding value is null   */
           int  *anynul,   /* O - set to 1 if any values are null; else 0   */
           int  *status)   /* IO - error status                             */
/*
  Read a subsection of data values from an image or a table column.
  This routine is set up to handle a maximum of nine dimensions.
*/
{
    long ii,i0, i1,i2,i3,i4,i5,i6,i7,i8,row,rstr,rstp,rinc;
    long str[9],stp[9],incr[9],dsize[10];
    LONGLONG blcll[9], trcll[9];
    long felem, nelem, nultyp, ninc, numcol;
    LONGLONG nulval = 0;
    int hdutype, anyf;
    char msg[FLEN_ERRMSG];
    int nullcheck = 2;

    if (naxis < 1 || naxis > 9)
    {
        sprintf(msg, "NAXIS = %d in call to ffgsvj is out of range", naxis);
        ffpmsg(msg);
        return(*status = BAD_DIMEN);
    }

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        for (ii=0; ii < naxis; ii++) {
	    blcll[ii] = blc[ii];
	    trcll[ii] = trc[ii];
	}

         fits_read_compressed_img(fptr, TLONGLONG, blcll, trcll, inc,
            nullcheck, NULL, array, flagval, anynul, status);
        return(*status);
    }

/*
    if this is a primary array, then the input COLNUM parameter should
    be interpreted as the row number, and we will alway read the image
    data from column 2 (any group parameters are in column 1).
*/
    if (ffghdt(fptr, &hdutype, status) > 0)
        return(*status);

    if (hdutype == IMAGE_HDU)
    {
        /* this is a primary array, or image extension */
        if (colnum == 0)
        {
            rstr = 1;
            rstp = 1;
        }
        else
        {
            rstr = colnum;
            rstp = colnum;
        }
        rinc = 1;
        numcol = 2;
    }
    else
    {
        /* this is a table, so the row info is in the (naxis+1) elements */
        rstr = blc[naxis];
        rstp = trc[naxis];
        rinc = inc[naxis];
        numcol = colnum;
    }

    nultyp = 2;
    if (anynul)
        *anynul = FALSE;

    i0 = 0;
    for (ii = 0; ii < 9; ii++)
    {
        str[ii] = 1;
        stp[ii] = 1;
        incr[ii] = 1;
        dsize[ii] = 1;
    }

    for (ii = 0; ii < naxis; ii++)
    {
      if (trc[ii] < blc[ii])
      {
        sprintf(msg, "ffgsvj: illegal range specified for axis %ld", ii + 1);
        ffpmsg(msg);
        return(*status = BAD_PIX_NUM);
      }

      str[ii] = blc[ii];
      stp[ii] = trc[ii];
      incr[ii] = inc[ii];
      dsize[ii + 1] = dsize[ii] * naxes[ii];
    }

    if (naxis == 1 && naxes[0] == 1)
    {
      /* This is not a vector column, so read all the rows at once */
      nelem = (rstp - rstr) / rinc + 1;
      ninc = rinc;
      rstp = rstr;
    }
    else
    {
      /* have to read each row individually, in all dimensions */
      nelem = (stp[0] - str[0]) / inc[0] + 1;
      ninc = incr[0];
    }

    for (row = rstr; row <= rstp; row += rinc)
    {
     for (i8 = str[8]; i8 <= stp[8]; i8 += incr[8])
     {
      for (i7 = str[7]; i7 <= stp[7]; i7 += incr[7])
      {
       for (i6 = str[6]; i6 <= stp[6]; i6 += incr[6])
       {
        for (i5 = str[5]; i5 <= stp[5]; i5 += incr[5])
        {
         for (i4 = str[4]; i4 <= stp[4]; i4 += incr[4])
         {
          for (i3 = str[3]; i3 <= stp[3]; i3 += incr[3])
          {
           for (i2 = str[2]; i2 <= stp[2]; i2 += incr[2])
           {
            for (i1 = str[1]; i1 <= stp[1]; i1 += incr[1])
            {
              felem=str[0] + (i1 - 1) * dsize[1] + (i2 - 1) * dsize[2] + 
                             (i3 - 1) * dsize[3] + (i4 - 1) * dsize[4] +
                             (i5 - 1) * dsize[5] + (i6 - 1) * dsize[6] +
                             (i7 - 1) * dsize[7] + (i8 - 1) * dsize[8];

              if ( ffgcljj(fptr, numcol, row, felem, nelem, ninc, nultyp,
                   nulval, &array[i0], &flagval[i0], &anyf, status) > 0)
                   return(*status);

              if (anyf && anynul)
                  *anynul = TRUE;

              i0 += nelem;
            }
           }
          }
         }
        }
       }
      }
     }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffggpjj(fitsfile *fptr,   /* I - FITS file pointer                       */
            long  group,      /* I - group to read (1 = 1st group)           */
            long  firstelem,  /* I - first vector element to read (1 = 1st)  */
            long  nelem,      /* I - number of values to read                */
            LONGLONG  *array, /* O - array of values that are returned       */
            int  *status)     /* IO - error status                           */
/*
  Read an array of group parameters from the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being read).
*/
{
    long row;
    int idummy;
    char cdummy;
    LONGLONG dummy = 0;

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    row=maxvalue(1,group);

    ffgcljj(fptr, 1, row, firstelem, nelem, 1, 1, dummy,
               array, &cdummy, &idummy, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcvjj(fitsfile *fptr,  /* I - FITS file pointer                       */
           int  colnum,      /* I - number of column to read (1 = 1st col)  */
           LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
           LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
           LONGLONG  nelem,      /* I - number of values to read                */
           LONGLONG  nulval, /* I - value for null pixels                   */
           LONGLONG *array,  /* O - array of values that are read           */
           int  *anynul,     /* O - set to 1 if any values are null; else 0 */
           int  *status)     /* IO - error status                           */
/*
  Read an array of values from a column in the current FITS HDU. Automatic
  datatype conversion will be performed if the datatype of the column does not
  match the datatype of the array parameter. The output values will be scaled 
  by the FITS TSCALn and TZEROn values if these values have been defined.
  Any undefined pixels will be set equal to the value of 'nulval' unless
  nulval = 0 in which case no checks for undefined pixels will be made.
*/
{
    char cdummy;

    ffgcljj(fptr, colnum, firstrow, firstelem, nelem, 1, 1, nulval,
           array, &cdummy, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcfjj(fitsfile *fptr,  /* I - FITS file pointer                       */
           int  colnum,      /* I - number of column to read (1 = 1st col)  */
           LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
           LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
           LONGLONG  nelem,      /* I - number of values to read                */
           LONGLONG  *array, /* O - array of values that are read           */
           char *nularray,   /* O - array of flags: 1 if null pixel; else 0 */
           int  *anynul,     /* O - set to 1 if any values are null; else 0 */
           int  *status)     /* IO - error status                           */
/*
  Read an array of values from a column in the current FITS HDU. Automatic
  datatype conversion will be performed if the datatype of the column does not
  match the datatype of the array parameter. The output values will be scaled 
  by the FITS TSCALn and TZEROn values if these values have been defined.
  Nularray will be set = 1 if the corresponding array pixel is undefined, 
  otherwise nularray will = 0.
*/
{
    LONGLONG dummy = 0;

    ffgcljj(fptr, colnum, firstrow, firstelem, nelem, 1, 2, dummy,
           array, nularray, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcljj( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            long  elemincre,  /* I - pixel increment; e.g., 2 = every other  */
            int   nultyp,     /* I - null value handling code:               */
                              /*     1: set undefined pixels = nulval        */
                              /*     2: set nularray=1 for undefined pixels  */
            LONGLONG  nulval, /* I - value for null pixels if nultyp = 1     */
            LONGLONG  *array, /* O - array of values that are read           */
            char *nularray,   /* O - array of flags = 1 if nultyp = 2        */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of values from a column in the current FITS HDU.
  The column number may refer to a real column in an ASCII or binary table, 
  or it may refer be a virtual column in a 1 or more grouped FITS primary
  array or image extension.  FITSIO treats a primary array as a binary table
  with 2 vector columns: the first column contains the group parameters (often
  with length = 0) and the second column contains the array of image pixels.
  Each row of the table represents a group in the case of multigroup FITS
  images.

  The output array of values will be converted from the datatype of the column 
  and will be scaled by the FITS TSCALn and TZEROn values if necessary.
*/
{
    double scale, zero, power = 1., dtemp;
    int tcode, maxelem2, hdutype, xcode, decimals;
    long twidth, incre;
    long ii, xwidth, ntodo;
    int convert, nulcheck, readcheck = 0;
    LONGLONG repeat, startpos, elemnum, readptr, tnull;
    LONGLONG rowlen, rownum, remain, next, rowincre, maxelem;
    char tform[20];
    char message[81];
    char snull[20];   /*  the FITS null value if reading from ASCII table  */

    double cbuff[DBUFFSIZE / sizeof(double)]; /* align cbuff on word boundary */
    void *buffer;

    if (*status > 0 || nelem == 0)  /* inherit input status value if > 0 */
        return(*status);

    buffer = cbuff;

    if (anynul)
        *anynul = 0;

    if (nultyp == 2)
        memset(nularray, 0, (size_t) nelem);   /* initialize nullarray */

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/
    if (elemincre < 0)
        readcheck = -1;  /* don't do range checking in this case */

    if (ffgcprll(fptr, colnum, firstrow, firstelem, nelem, readcheck, &scale, &zero,
         tform, &twidth, &tcode, &maxelem2, &startpos, &elemnum, &incre,
         &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0 )
         return(*status);
    maxelem = maxelem2;

    incre *= elemincre;   /* multiply incre to just get every nth pixel */

    if (tcode == TSTRING)    /* setup for ASCII tables */
    {
      /* get the number of implied decimal places if no explicit decmal point */
      ffasfm(tform, &xcode, &xwidth, &decimals, status); 
      for(ii = 0; ii < decimals; ii++)
        power *= 10.;
    }
    /*------------------------------------------------------------------*/
    /*  Decide whether to check for null values in the input FITS file: */
    /*------------------------------------------------------------------*/
    nulcheck = nultyp; /* by default check for null values in the FITS file */

    if (nultyp == 1 && nulval == 0)
       nulcheck = 0;    /* calling routine does not want to check for nulls */

    else if (tcode%10 == 1 &&        /* if reading an integer column, and  */ 
            tnull == NULL_UNDEFINED) /* if a null value is not defined,    */
            nulcheck = 0;            /* then do not check for null values. */

    else if (tcode == TSHORT && (tnull > SHRT_MAX || tnull < SHRT_MIN) )
            nulcheck = 0;            /* Impossible null value */

    else if (tcode == TBYTE && (tnull > 255 || tnull < 0) )
            nulcheck = 0;            /* Impossible null value */

    else if (tcode == TSTRING && snull[0] == ASCII_NULL_UNDEFINED)
         nulcheck = 0;

    /*----------------------------------------------------------------------*/
    /*  If FITS column and output data array have same datatype, then we do */
    /*  not need to use a temporary buffer to store intermediate datatype.  */
    /*----------------------------------------------------------------------*/
    convert = 1;
    if (tcode == TLONGLONG)  /* Special Case:                        */
    {                             /* no type convertion required, so read */
                                  /* data directly into output buffer.    */

        if (nelem < (LONGLONG)INT32_MAX/8) {
            maxelem = nelem;
        } else {
            maxelem = INT32_MAX/8;
        }

        if (nulcheck == 0 && scale == 1. && zero == 0.)
            convert = 0;  /* no need to scale data or find nulls */
    }

    /*---------------------------------------------------------------------*/
    /*  Now read the pixels from the FITS column. If the column does not   */
    /*  have the same datatype as the output array, then we have to read   */
    /*  the raw values into a temporary buffer (of limited size).  In      */
    /*  the case of a vector colum read only 1 vector of values at a time  */
    /*  then skip to the next row if more values need to be read.          */
    /*  After reading the raw values, then call the fffXXYY routine to (1) */
    /*  test for undefined values, (2) convert the datatype if necessary,  */
    /*  and (3) scale the values by the FITS TSCALn and TZEROn linear      */
    /*  scaling parameters.                                                */
    /*---------------------------------------------------------------------*/
    remain = nelem;           /* remaining number of values to read */
    next = 0;                 /* next element in array to be read   */
    rownum = 0;               /* row number, relative to firstrow   */

    while (remain)
    {
        /* limit the number of pixels to read at one time to the number that
           will fit in the buffer or to the number of pixels that remain in
           the current vector, which ever is smaller.
        */
        ntodo = (long) minvalue(remain, maxelem);
        if (elemincre >= 0)
        {
          ntodo = (long) minvalue(ntodo, ((repeat - elemnum - 1)/elemincre +1));
        }
        else
        {
          ntodo = (long) minvalue(ntodo, (elemnum/(-elemincre) +1));
        }

        readptr = startpos + ((LONGLONG)rownum * rowlen) + (elemnum * (incre / elemincre));

        switch (tcode) 
        {
            case (TLONGLONG):
                ffgi8b(fptr, readptr, ntodo, incre, (long *) &array[next],
                       status);
                if (convert)
                    fffi8i8((LONGLONG *) &array[next], ntodo, scale, zero, 
                           nulcheck, tnull, nulval, &nularray[next], 
                           anynul, &array[next], status);
                break;
            case (TLONG):
                ffgi4b(fptr, readptr, ntodo, incre, (INT32BIT *) buffer,
                       status);
                fffi4i8((INT32BIT *) buffer, ntodo, scale, zero, 
                        nulcheck, (INT32BIT) tnull, nulval, &nularray[next], 
                        anynul, &array[next], status);
                break;
            case (TBYTE):
                ffgi1b(fptr, readptr, ntodo, incre, (unsigned char *) buffer,
                       status);
                fffi1i8((unsigned char *) buffer, ntodo, scale, zero, nulcheck, 
                     (unsigned char) tnull, nulval, &nularray[next], anynul, 
                     &array[next], status);
                break;
            case (TSHORT):
                ffgi2b(fptr, readptr, ntodo, incre, (short  *) buffer, status);
                fffi2i8((short  *) buffer, ntodo, scale, zero, nulcheck, 
                      (short) tnull, nulval, &nularray[next], anynul, 
                      &array[next], status);
                break;
            case (TFLOAT):
                ffgr4b(fptr, readptr, ntodo, incre, (float  *) buffer, status);
                fffr4i8((float  *) buffer, ntodo, scale, zero, nulcheck, 
                       nulval, &nularray[next], anynul, 
                       &array[next], status);
                break;
            case (TDOUBLE):
                ffgr8b(fptr, readptr, ntodo, incre, (double *) buffer, status);
                fffr8i8((double *) buffer, ntodo, scale, zero, nulcheck, 
                          nulval, &nularray[next], anynul, 
                          &array[next], status);
                break;
            case (TSTRING):
                ffmbyt(fptr, readptr, REPORT_EOF, status);
       
                if (incre == twidth)    /* contiguous bytes */
                     ffgbyt(fptr, ntodo * twidth, buffer, status);
                else
                     ffgbytoff(fptr, twidth, ntodo, incre - twidth, buffer,
                               status);

                fffstri8((char *) buffer, ntodo, scale, zero, twidth, power,
                     nulcheck, snull, nulval, &nularray[next], anynul,
                     &array[next], status);
                break;

            default:  /*  error trap for invalid column format */
                sprintf(message, 
                   "Cannot read numbers from column %d which has format %s",
                    colnum, tform);
                ffpmsg(message);
                if (hdutype == ASCII_TBL)
                    return(*status = BAD_ATABLE_FORMAT);
                else
                    return(*status = BAD_BTABLE_FORMAT);

        } /* End of switch block */

        /*-------------------------*/
        /*  Check for fatal error  */
        /*-------------------------*/
        if (*status > 0)  /* test for error during previous read operation */
        {
	  dtemp = (double) next;
          if (hdutype > 0)
            sprintf(message,
            "Error reading elements %.0f thru %.0f from column %d (ffgclj).",
              dtemp+1., dtemp+ntodo, colnum);
          else
            sprintf(message,
            "Error reading elements %.0f thru %.0f from image (ffgclj).",
              dtemp+1., dtemp+ntodo);

          ffpmsg(message);
          return(*status);
        }

        /*--------------------------------------------*/
        /*  increment the counters for the next loop  */
        /*--------------------------------------------*/
        remain -= ntodo;
        if (remain)
        {
            next += ntodo;
            elemnum = elemnum + (ntodo * elemincre);

            if (elemnum >= repeat)  /* completed a row; start on later row */
            {
                rowincre = elemnum / repeat;
                rownum += rowincre;
                elemnum = elemnum - (rowincre * repeat);
            }
            else if (elemnum < 0)  /* completed a row; start on a previous row */
            {
                rowincre = (-elemnum - 1) / repeat + 1;
                rownum -= rowincre;
                elemnum = (rowincre * repeat) + elemnum;
            }
        }
    }  /*  End of main while Loop  */


    /*--------------------------------*/
    /*  check for numerical overflow  */
    /*--------------------------------*/
    if (*status == OVERFLOW_ERR)
    {
        ffpmsg(
        "Numerical overflow during type conversion while reading FITS data.");
        *status = NUM_OVERFLOW;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffi1i8(unsigned char *input, /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            unsigned char tnull,  /* I - value of FITS TNULLn keyword if any */
            LONGLONG nullval,     /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            LONGLONG *output,     /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to tnull.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
                output[ii] = (LONGLONG) input[ii];  /* copy input to output */
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONGLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MIN;
                }
                else if (dvalue > DLONGLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MAX;
                }
                else
                    output[ii] = (LONGLONG) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                    output[ii] = (LONGLONG) input[ii];
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (dvalue > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffi2i8(short *input,         /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            short tnull,          /* I - value of FITS TNULLn keyword if any */
            LONGLONG nullval,     /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            LONGLONG *output,     /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to tnull.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
                output[ii] = (LONGLONG) input[ii];   /* copy input to output */
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONGLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MIN;
                }
                else if (dvalue > DLONGLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MAX;
                }
                else
                    output[ii] = (LONGLONG) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                    output[ii] = (LONGLONG) input[ii];
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (dvalue > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffi4i8(INT32BIT *input,      /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            INT32BIT tnull,       /* I - value of FITS TNULLn keyword if any */
            LONGLONG nullval,     /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            LONGLONG *output,     /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to tnull.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
                output[ii] = (LONGLONG) input[ii];   /* copy input to output */
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONGLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MIN;
                }
                else if (dvalue > DLONGLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MAX;
                }
                else
                    output[ii] = (LONGLONG) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                    output[ii] = (LONGLONG) input[ii];

            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (dvalue > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffi8i8(LONGLONG *input,      /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            LONGLONG tnull,       /* I - value of FITS TNULLn keyword if any */
            LONGLONG nullval,     /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            LONGLONG *output,     /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to tnull.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
                output[ii] =  input[ii];   /* copy input to output */
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONGLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MIN;
                }
                else if (dvalue > DLONGLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MAX;
                }
                else
                    output[ii] = (LONGLONG) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                    output[ii] = input[ii];

            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] == tnull)
                {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                }
                else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (dvalue > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffr4i8(float *input,         /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            LONGLONG nullval,     /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            LONGLONG *output,     /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to NaN.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;
    short *sptr, iret;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] < DLONGLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MIN;
                }
                else if (input[ii] > DLONGLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MAX;
                }
                else
                    output[ii] = (LONGLONG) input[ii];
            }
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONGLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MIN;
                }
                else if (dvalue > DLONGLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MAX;
                }
                else
                    output[ii] = (LONGLONG) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        sptr = (short *) input;

#if BYTESWAPPED && MACHINE != VAXVMS && MACHINE != ALPHAVMS
        sptr++;       /* point to MSBs */
#endif

        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++, sptr += 2)
            {
              if (0 != (iret = fnan(*sptr) ) )  /* test for NaN or underflow */
              {
                  if (iret == 1)  /* is it a NaN? */
                  {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                  }
                  else            /* it's an underflow */
                     output[ii] = 0;
              }
              else
                {
                    if (input[ii] < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (input[ii] > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) input[ii];
                }
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++, sptr += 2)
            {
              if (0 != (iret = fnan(*sptr) ) )  /* test for NaN or underflow */
              {
                  if (iret == 1)  /* is it a NaN? */
                  {  
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                  }
                  else            /* it's an underflow */
                  {
                    if (zero < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (zero > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) zero;
                  }
              }
              else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (dvalue > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffr8i8(double *input,        /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            LONGLONG nullval,     /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            LONGLONG *output,     /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file.
  Check for null values and do datatype conversion and scaling if required.
  The nullcheck code value determines how any null values in the input array
  are treated.  A null value is an input pixel that is equal to NaN.  If 
  nullcheck = 0, then no checking for nulls is performed and any null values
  will be transformed just like any other pixel.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    long ii;
    double dvalue;
    short *sptr, iret;

    if (nullcheck == 0)     /* no null checking required */
    {
        if (scale == 1. && zero == 0.)      /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++)
            {
                if (input[ii] < DLONGLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MIN;
                }
                else if (input[ii] > DLONGLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MAX;
                }
                else
                    output[ii] = (LONGLONG) input[ii];
            }
        }
        else             /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++)
            {
                dvalue = input[ii] * scale + zero;

                if (dvalue < DLONGLONG_MIN)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MIN;
                }
                else if (dvalue > DLONGLONG_MAX)
                {
                    *status = OVERFLOW_ERR;
                    output[ii] = LONGLONG_MAX;
                }
                else
                    output[ii] = (LONGLONG) dvalue;
            }
        }
    }
    else        /* must check for null values */
    {
        sptr = (short *) input;

#if BYTESWAPPED && MACHINE != VAXVMS && MACHINE != ALPHAVMS
        sptr += 3;       /* point to MSBs */
#endif
        if (scale == 1. && zero == 0.)  /* no scaling */
        {       
            for (ii = 0; ii < ntodo; ii++, sptr += 4)
            {
              if (0 != (iret = dnan(*sptr)) )  /* test for NaN or underflow */
              {
                  if (iret == 1)  /* is it a NaN? */
                  {
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                  }
                  else            /* it's an underflow */
                     output[ii] = 0;
              }
              else
                {
                    if (input[ii] < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (input[ii] > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) input[ii];
                }
            }
        }
        else                  /* must scale the data */
        {
            for (ii = 0; ii < ntodo; ii++, sptr += 4)
            {
              if (0 != (iret = dnan(*sptr)) )  /* test for NaN or underflow */
              {
                  if (iret == 1)  /* is it a NaN? */
                  {  
                    *anynull = 1;
                    if (nullcheck == 1)
                        output[ii] = nullval;
                    else
                        nullarray[ii] = 1;
                  }
                  else            /* it's an underflow */
                  {
                    if (zero < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (zero > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) zero;
                  }
              }
              else
                {
                    dvalue = input[ii] * scale + zero;

                    if (dvalue < DLONGLONG_MIN)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MIN;
                    }
                    else if (dvalue > DLONGLONG_MAX)
                    {
                        *status = OVERFLOW_ERR;
                        output[ii] = LONGLONG_MAX;
                    }
                    else
                        output[ii] = (LONGLONG) dvalue;
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffstri8(char *input,         /* I - array of values to be converted     */
            long ntodo,           /* I - number of elements in the array     */
            double scale,         /* I - FITS TSCALn or BSCALE value         */
            double zero,          /* I - FITS TZEROn or BZERO  value         */
            long twidth,          /* I - width of each substring of chars    */
            double implipower,    /* I - power of 10 of implied decimal      */
            int nullcheck,        /* I - null checking code; 0 = don't check */
                                  /*     1:set null pixels = nullval         */
                                  /*     2: if null pixel, set nullarray = 1 */
            char  *snull,         /* I - value of FITS null string, if any   */
            LONGLONG nullval,     /* I - set null pixels, if nullcheck = 1   */
            char *nullarray,      /* I - bad pixel array, if nullcheck = 2   */
            int  *anynull,        /* O - set to 1 if any pixels are null     */
            LONGLONG *output,     /* O - array of converted pixels           */
            int *status)          /* IO - error status                       */
/*
  Copy input to output following reading of the input from a FITS file. Check
  for null values and do scaling if required. The nullcheck code value
  determines how any null values in the input array are treated. A null
  value is an input pixel that is equal to snull.  If nullcheck= 0, then
  no special checking for nulls is performed.  If nullcheck = 1, then the
  output pixel will be set = nullval if the corresponding input pixel is null.
  If nullcheck = 2, then if the pixel is null then the corresponding value of
  nullarray will be set to 1; the value of nullarray for non-null pixels 
  will = 0.  The anynull parameter will be set = 1 if any of the returned
  pixels are null, otherwise anynull will be returned with a value = 0;
*/
{
    int nullen;
    long ii;
    double dvalue;
    char *cstring, message[81];
    char *cptr, *tpos;
    char tempstore, chrzero = '0';
    double val, power;
    int exponent, sign, esign, decpt;

    nullen = strlen(snull);
    cptr = input;  /* pointer to start of input string */
    for (ii = 0; ii < ntodo; ii++)
    {
      cstring = cptr;
      /* temporarily insert a null terminator at end of the string */
      tpos = cptr + twidth;
      tempstore = *tpos;
      *tpos = 0;

      /* check if null value is defined, and if the    */
      /* column string is identical to the null string */
      if (snull[0] != ASCII_NULL_UNDEFINED && 
         !strncmp(snull, cptr, nullen) )
      {
        if (nullcheck)  
        {
          *anynull = 1;    
          if (nullcheck == 1)
            output[ii] = nullval;
          else
            nullarray[ii] = 1;
        }
        cptr += twidth;
      }
      else
      {
        /* value is not the null value, so decode it */
        /* remove any embedded blank characters from the string */

        decpt = 0;
        sign = 1;
        val  = 0.;
        power = 1.;
        exponent = 0;
        esign = 1;

        while (*cptr == ' ')               /* skip leading blanks */
           cptr++;

        if (*cptr == '-' || *cptr == '+')  /* check for leading sign */
        {
          if (*cptr == '-')
             sign = -1;

          cptr++;

          while (*cptr == ' ')         /* skip blanks between sign and value */
            cptr++;
        }

        while (*cptr >= '0' && *cptr <= '9')
        {
          val = val * 10. + *cptr - chrzero;  /* accumulate the value */
          cptr++;

          while (*cptr == ' ')         /* skip embedded blanks in the value */
            cptr++;
        }

        if (*cptr == '.' || *cptr == ',')    /* check for decimal point */
        {
          decpt = 1;       /* set flag to show there was a decimal point */
          cptr++;
          while (*cptr == ' ')         /* skip any blanks */
            cptr++;

          while (*cptr >= '0' && *cptr <= '9')
          {
            val = val * 10. + *cptr - chrzero;  /* accumulate the value */
            power = power * 10.;
            cptr++;

            while (*cptr == ' ')         /* skip embedded blanks in the value */
              cptr++;
          }
        }

        if (*cptr == 'E' || *cptr == 'D')  /* check for exponent */
        {
          cptr++;
          while (*cptr == ' ')         /* skip blanks */
              cptr++;
  
          if (*cptr == '-' || *cptr == '+')  /* check for exponent sign */
          {
            if (*cptr == '-')
               esign = -1;

            cptr++;

            while (*cptr == ' ')        /* skip blanks between sign and exp */
              cptr++;
          }

          while (*cptr >= '0' && *cptr <= '9')
          {
            exponent = exponent * 10 + *cptr - chrzero;  /* accumulate exp */
            cptr++;

            while (*cptr == ' ')         /* skip embedded blanks */
              cptr++;
          }
        }

        if (*cptr  != 0)  /* should end up at the null terminator */
        {
          sprintf(message, "Cannot read number from ASCII table");
          ffpmsg(message);
          sprintf(message, "Column field = %s.", cstring);
          ffpmsg(message);
          /* restore the char that was overwritten by the null */
          *tpos = tempstore;
          return(*status = BAD_C2D);
        }

        if (!decpt)  /* if no explicit decimal, use implied */
           power = implipower;

        dvalue = (sign * val / power) * pow(10., (double) (esign * exponent));

        dvalue = dvalue * scale + zero;   /* apply the scaling */

        if (dvalue < DLONGLONG_MIN)
        {
            *status = OVERFLOW_ERR;
            output[ii] = LONGLONG_MIN;
        }
        else if (dvalue > DLONGLONG_MAX)
        {
            *status = OVERFLOW_ERR;
            output[ii] = LONGLONG_MAX;
        }
        else
            output[ii] = (LONGLONG) dvalue;
      }
      /* restore the char that was overwritten by the null */
      *tpos = tempstore;
    }
    return(*status);
}
