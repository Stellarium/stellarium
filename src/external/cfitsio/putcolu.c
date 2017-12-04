/*  This file, putcolu.c, contains routines that write data elements to    */
/*  a FITS image or table.  Writes null values.                            */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int ffppru( fitsfile *fptr,  /* I - FITS file pointer                       */
            long  group,      /* I - group to write(1 = 1st group)          */
            LONGLONG  firstelem,  /* I - first vector element to write(1 = 1st) */
            LONGLONG  nelem,      /* I - number of values to write              */
            int  *status)     /* IO - error status                          */
/*
  Write null values to the primary array.

*/
{
    long row;

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        ffpmsg("writing to compressed image is not supported");

        return(*status = DATA_COMPRESSION_ERR);
    }

    row=maxvalue(1,group);

    ffpclu(fptr, 2, row, firstelem, nelem, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpprn( fitsfile *fptr,  /* I - FITS file pointer                       */
            LONGLONG  firstelem,  /* I - first vector element to write(1 = 1st) */
            LONGLONG  nelem,      /* I - number of values to write              */
            int  *status)     /* IO - error status                          */
/*
  Write null values to the primary array. (Doesn't support groups).

*/
{
    long row = 1;

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        ffpmsg("writing to compressed image is not supported");

        return(*status = DATA_COMPRESSION_ERR);
    }

    ffpclu(fptr, 2, row, firstelem, nelem, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpclu( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            LONGLONG  firstrow,  /* I - first row to write (1 = 1st row)        */
            LONGLONG  firstelem, /* I - first vector element to write (1 = 1st) */
            LONGLONG  nelempar,     /* I - number of values to write               */
            int  *status)    /* IO - error status                           */
/*
  Set elements of a table column to the appropriate null value for the column
  The column number may refer to a real column in an ASCII or binary table, 
  or it may refer to a virtual column in a 1 or more grouped FITS primary
  array.  FITSIO treats a primary array as a binary table
  with 2 vector columns: the first column contains the group parameters (often
  with length = 0) and the second column contains the array of image pixels.
  Each row of the table represents a group in the case of multigroup FITS
  images.
  
  This routine support COMPLEX and DOUBLE COMPLEX binary table columns, and
  sets both the real and imaginary components of the element to a NaN.
*/
{
    int tcode, maxelem, hdutype, writemode = 2, leng;
    short i2null;
    INT32BIT i4null;
    long twidth, incre;
    LONGLONG ii;
    LONGLONG largeelem, nelem, tnull, i8null;
    LONGLONG repeat, startpos, elemnum, wrtptr, rowlen, rownum, remain, next, ntodo;
    double scale, zero;
    unsigned char i1null, lognul = 0;
    char tform[20], *cstring = 0;
    char message[FLEN_ERRMSG];
    char snull[20];   /*  the FITS null value  */
    long   jbuff[2] = { -1, -1};  /* all bits set is equivalent to a NaN */
    size_t buffsize;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    nelem = nelempar;
    
    largeelem = firstelem;

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/

    /* note that writemode = 2 by default (not 1), so that the returned */
    /* repeat and incre values will be the actual values for this column. */

    /* If writing nulls to a variable length column then dummy data values  */
    /* must have already been written to the heap. */
    /* We just have to overwrite the previous values with null values. */
    /* Set writemode = 0 in this case, to test that values have been written */

    fits_get_coltype(fptr, colnum, &tcode, NULL, NULL, status);
    if (tcode < 0)
         writemode = 0;  /* this is a variable length column */

    if (abs(tcode) >= TCOMPLEX)
    { /* treat complex columns as pairs of numbers */
      largeelem = (largeelem - 1) * 2 + 1;
      nelem *= 2;
    }

    if (ffgcprll( fptr, colnum, firstrow, largeelem, nelem, writemode, &scale,
       &zero, tform, &twidth, &tcode, &maxelem, &startpos,  &elemnum, &incre,
        &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0)
        return(*status);

    if (tcode == TSTRING)
    {
      if (snull[0] == ASCII_NULL_UNDEFINED)
      {
        ffpmsg(
        "Null value string for ASCII table column is not defined (FTPCLU).");
        return(*status = NO_NULL);
      }

      /* allocate buffer to hold the null string.  Must write the entire */
      /* width of the column (twidth bytes) to avoid possible problems */
      /* with uninitialized FITS blocks, in case the field spans blocks */

      buffsize = maxvalue(20, twidth);
      cstring = (char *) malloc(buffsize);
      if (!cstring)
         return(*status = MEMORY_ALLOCATION);

      memset(cstring, ' ', buffsize);  /* initialize  with blanks */

      leng = strlen(snull);
      if (hdutype == BINARY_TBL)
         leng++;        /* copy the terminator too in binary tables */

      strncpy(cstring, snull, leng);  /* copy null string to temp buffer */
    }
    else if ( tcode == TBYTE  ||
              tcode == TSHORT ||
              tcode == TLONG  ||
              tcode == TLONGLONG) 
    {
      if (tnull == NULL_UNDEFINED)
      {
        ffpmsg(
        "Null value for integer table column is not defined (FTPCLU).");
        return(*status = NO_NULL);
      }

      if (tcode == TBYTE)
         i1null = (unsigned char) tnull;
      else if (tcode == TSHORT)
      {
         i2null = (short) tnull;
#if BYTESWAPPED
         ffswap2(&i2null, 1); /* reverse order of bytes */
#endif
      }
      else if (tcode == TLONG)
      {
         i4null = (INT32BIT) tnull;
#if BYTESWAPPED
         ffswap4(&i4null, 1); /* reverse order of bytes */
#endif
      }
      else
      {
         i8null = tnull;
#if BYTESWAPPED
         ffswap8((double *)(&i8null), 1);  /* reverse order of bytes */
#endif
      }
    }

    /*---------------------------------------------------------------------*/
    /*  Now write the pixels to the FITS column.                           */
    /*---------------------------------------------------------------------*/
    remain = nelem;           /* remaining number of values to write  */
    next = 0;                 /* next element in array to be written  */
    rownum = 0;               /* row number, relative to firstrow     */
    ntodo = remain;           /* number of elements to write at one time */

    while (ntodo)
    {
        /* limit the number of pixels to process at one time to the number that
           will fit in the buffer space or to the number of pixels that remain
           in the current vector, which ever is smaller.
        */
        ntodo = minvalue(ntodo, (repeat - elemnum));
        wrtptr = startpos + ((LONGLONG)rownum * rowlen) + (elemnum * incre);

        ffmbyt(fptr, wrtptr, IGNORE_EOF, status); /* move to write position */

        switch (tcode) 
        {
            case (TBYTE):
 
                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 1,  &i1null, status);
                break;

            case (TSHORT):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 2, &i2null, status);
                break;

            case (TLONG):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 4, &i4null, status);
                break;

            case (TLONGLONG):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 8, &i8null, status);
                break;

            case (TFLOAT):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 4, jbuff, status);
                break;

            case (TDOUBLE):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 8, jbuff, status);
                break;

            case (TLOGICAL):
 
                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 1, &lognul, status);
                break;

            case (TSTRING):  /* an ASCII table column */
                /* repeat always = 1, so ntodo is also guaranteed to = 1 */
                ffpbyt(fptr, twidth, cstring, status);
                break;

            default:  /*  error trap  */
                sprintf(message, 
                   "Cannot write null value to column %d which has format %s",
                     colnum,tform);
                ffpmsg(message);
                return(*status);

        } /* End of switch block */

        /*-------------------------*/
        /*  Check for fatal error  */
        /*-------------------------*/
        if (*status > 0)  /* test for error during previous write operation */
        {
           sprintf(message,
             "Error writing %.0f thru %.0f of null values (ffpclu).",
              (double) (next+1), (double) (next+ntodo));
           ffpmsg(message);

           if (cstring)
              free(cstring);

           return(*status);
        }

        /*--------------------------------------------*/
        /*  increment the counters for the next loop  */
        /*--------------------------------------------*/
        remain -= ntodo;
        if (remain)
        {
            next += ntodo;
            elemnum += ntodo;
            if (elemnum == repeat)  /* completed a row; start on next row */
            {
                elemnum = 0;
                rownum++;
            }
        }
        ntodo = remain;  /* this is the maximum number to do in next loop */

    }  /*  End of main while Loop  */

    if (cstring)
       free(cstring);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpcluc( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            LONGLONG  firstrow,  /* I - first row to write (1 = 1st row)        */
            LONGLONG  firstelem, /* I - first vector element to write (1 = 1st) */
            LONGLONG  nelem,     /* I - number of values to write               */
            int  *status)    /* IO - error status                           */
/*
  Set elements of a table column to the appropriate null value for the column
  The column number may refer to a real column in an ASCII or binary table, 
  or it may refer to a virtual column in a 1 or more grouped FITS primary
  array.  FITSIO treats a primary array as a binary table
  with 2 vector columns: the first column contains the group parameters (often
  with length = 0) and the second column contains the array of image pixels.
  Each row of the table represents a group in the case of multigroup FITS
  images.
  
  This routine does not do anything special in the case of COMPLEX table columns
  (unlike the similar ffpclu routine).  This routine is mainly for use by
  ffpcne which already compensates for the effective doubling of the number of 
  elements in a complex column.
*/
{
    int tcode, maxelem, hdutype, writemode = 2, leng;
    short i2null;
    INT32BIT i4null;
    long twidth, incre;
    LONGLONG ii;
    LONGLONG tnull, i8null;
    LONGLONG repeat, startpos, elemnum, wrtptr, rowlen, rownum, remain, next, ntodo;
    double scale, zero;
    unsigned char i1null, lognul = 0;
    char tform[20], *cstring = 0;
    char message[FLEN_ERRMSG];
    char snull[20];   /*  the FITS null value  */
    long   jbuff[2] = { -1, -1};  /* all bits set is equivalent to a NaN */
    size_t buffsize;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/

    /* note that writemode = 2 by default (not 1), so that the returned */
    /* repeat and incre values will be the actual values for this column. */

    /* If writing nulls to a variable length column then dummy data values  */
    /* must have already been written to the heap. */
    /* We just have to overwrite the previous values with null values. */
    /* Set writemode = 0 in this case, to test that values have been written */

    fits_get_coltype(fptr, colnum, &tcode, NULL, NULL, status);
    if (tcode < 0)
         writemode = 0;  /* this is a variable length column */
    
    if (ffgcprll( fptr, colnum, firstrow, firstelem, nelem, writemode, &scale,
       &zero, tform, &twidth, &tcode, &maxelem, &startpos,  &elemnum, &incre,
        &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0)
        return(*status);

    if (tcode == TSTRING)
    {
      if (snull[0] == ASCII_NULL_UNDEFINED)
      {
        ffpmsg(
        "Null value string for ASCII table column is not defined (FTPCLU).");
        return(*status = NO_NULL);
      }

      /* allocate buffer to hold the null string.  Must write the entire */
      /* width of the column (twidth bytes) to avoid possible problems */
      /* with uninitialized FITS blocks, in case the field spans blocks */

      buffsize = maxvalue(20, twidth);
      cstring = (char *) malloc(buffsize);
      if (!cstring)
         return(*status = MEMORY_ALLOCATION);

      memset(cstring, ' ', buffsize);  /* initialize  with blanks */

      leng = strlen(snull);
      if (hdutype == BINARY_TBL)
         leng++;        /* copy the terminator too in binary tables */

      strncpy(cstring, snull, leng);  /* copy null string to temp buffer */

    }
    else if ( tcode == TBYTE  ||
              tcode == TSHORT ||
              tcode == TLONG  ||
              tcode == TLONGLONG) 
    {
      if (tnull == NULL_UNDEFINED)
      {
        ffpmsg(
        "Null value for integer table column is not defined (FTPCLU).");
        return(*status = NO_NULL);
      }

      if (tcode == TBYTE)
         i1null = (unsigned char) tnull;
      else if (tcode == TSHORT)
      {
         i2null = (short) tnull;
#if BYTESWAPPED
         ffswap2(&i2null, 1); /* reverse order of bytes */
#endif
      }
      else if (tcode == TLONG)
      {
         i4null = (INT32BIT) tnull;
#if BYTESWAPPED
         ffswap4(&i4null, 1); /* reverse order of bytes */
#endif
      }
      else
      {
         i8null = tnull;
#if BYTESWAPPED
         ffswap4( (INT32BIT*) &i8null, 2); /* reverse order of bytes */
#endif
      }
    }

    /*---------------------------------------------------------------------*/
    /*  Now write the pixels to the FITS column.                           */
    /*---------------------------------------------------------------------*/
    remain = nelem;           /* remaining number of values to write  */
    next = 0;                 /* next element in array to be written  */
    rownum = 0;               /* row number, relative to firstrow     */
    ntodo = remain;           /* number of elements to write at one time */

    while (ntodo)
    {
        /* limit the number of pixels to process at one time to the number that
           will fit in the buffer space or to the number of pixels that remain
           in the current vector, which ever is smaller.
        */
        ntodo = minvalue(ntodo, (repeat - elemnum));
        wrtptr = startpos + ((LONGLONG)rownum * rowlen) + (elemnum * incre);

        ffmbyt(fptr, wrtptr, IGNORE_EOF, status); /* move to write position */

        switch (tcode) 
        {
            case (TBYTE):
 
                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 1,  &i1null, status);
                break;

            case (TSHORT):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 2, &i2null, status);
                break;

            case (TLONG):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 4, &i4null, status);
                break;

            case (TLONGLONG):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 8, &i8null, status);
                break;

            case (TFLOAT):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 4, jbuff, status);
                break;

            case (TDOUBLE):

                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 8, jbuff, status);
                break;

            case (TLOGICAL):
 
                for (ii = 0; ii < ntodo; ii++)
                  ffpbyt(fptr, 1, &lognul, status);
                break;

            case (TSTRING):  /* an ASCII table column */
                /* repeat always = 1, so ntodo is also guaranteed to = 1 */
                ffpbyt(fptr, twidth, cstring, status);
                break;

            default:  /*  error trap  */
                sprintf(message, 
                   "Cannot write null value to column %d which has format %s",
                     colnum,tform);
                ffpmsg(message);
                return(*status);

        } /* End of switch block */

        /*-------------------------*/
        /*  Check for fatal error  */
        /*-------------------------*/
        if (*status > 0)  /* test for error during previous write operation */
        {
           sprintf(message,
             "Error writing %.0f thru %.0f of null values (ffpclu).",
              (double) (next+1), (double) (next+ntodo));
           ffpmsg(message);

           if (cstring)
              free(cstring);

           return(*status);
        }

        /*--------------------------------------------*/
        /*  increment the counters for the next loop  */
        /*--------------------------------------------*/
        remain -= ntodo;
        if (remain)
        {
            next += ntodo;
            elemnum += ntodo;
            if (elemnum == repeat)  /* completed a row; start on next row */
            {
                elemnum = 0;
                rownum++;
            }
        }
        ntodo = remain;  /* this is the maximum number to do in next loop */

    }  /*  End of main while Loop  */

    if (cstring)
       free(cstring);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffprwu(fitsfile *fptr,
           LONGLONG firstrow,
           LONGLONG nrows, 
           int *status)

/* 
 * fits_write_nullrows / ffprwu - write TNULLs to all columns in one or more rows
 *
 * fitsfile *fptr - pointer to FITS HDU opened for read/write
 * long int firstrow - first table row to set to null. (firstrow >= 1)
 * long int nrows - total number or rows to set to null. (nrows >= 1)
 * int *status - upon return, *status contains CFITSIO status code
 *
 * RETURNS: CFITSIO status code
 *
 * written by Craig Markwardt, GSFC 
 */
{
  LONGLONG ntotrows;
  int ncols, i;
  int typecode = 0;
  LONGLONG repeat = 0, width = 0;
  int nullstatus;

  if (*status > 0) return *status;

  if ((firstrow <= 0) || (nrows <= 0)) return (*status = BAD_ROW_NUM);

  fits_get_num_rowsll(fptr, &ntotrows, status);

  if (firstrow + nrows - 1 > ntotrows) return (*status = BAD_ROW_NUM);
  
  fits_get_num_cols(fptr, &ncols, status);
  if (*status) return *status;


  /* Loop through each column and write nulls */
  for (i=1; i <= ncols; i++) {
    repeat = 0;  typecode = 0;  width = 0;
    fits_get_coltypell(fptr, i, &typecode, &repeat, &width, status);
    if (*status) break;

    /* NOTE: data of TSTRING type must not write the total repeat
       count, since the repeat count is the *character* count, not the
       nstring count.  Divide by string width to get number of
       strings. */
    
    if (typecode == TSTRING) repeat /= width;

    /* Write NULLs */
    nullstatus = 0;
    fits_write_col_null(fptr, i, firstrow, 1, repeat*nrows, &nullstatus);

    /* ignore error if no null value is defined for the column */
    if (nullstatus && nullstatus != NO_NULL) return (*status = nullstatus);
    
  }
    
  return *status;
}

