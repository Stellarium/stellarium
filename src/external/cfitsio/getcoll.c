/*  This file, getcoll.c, contains routines that read data elements from   */
/*  a FITS image or table, with logical datatype.                          */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <stdlib.h>
#include <string.h>
#include "fitsio2.h"
/*--------------------------------------------------------------------------*/
int ffgcvl( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            char  nulval,     /* I - value for null pixels                   */
            char *array,      /* O - array of values                         */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of logical values from a column in the current FITS HDU.
  Any undefined pixels will be set equal to the value of 'nulval' unless
  nulval = 0 in which case no checks for undefined pixels will be made.
*/
{
    char cdummy;

    ffgcll( fptr, colnum, firstrow, firstelem, nelem, 1, nulval, array,
            &cdummy, anynul, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcl(  fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            char *array,      /* O - array of values                         */
            int  *status)     /* IO - error status                           */
/*
  !!!! THIS ROUTINE IS DEPRECATED AND SHOULD NOT BE USED !!!!!!
                  !!!! USE ffgcvl INSTEAD  !!!!!!
  Read an array of logical values from a column in the current FITS HDU.
  No checking for null values will be performed.
*/
{
    char nulval = 0;
    int anynul;

    ffgcvl( fptr, colnum, firstrow, firstelem, nelem, nulval, array,
            &anynul, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcfl( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            char *array,      /* O - array of values                         */
            char *nularray,   /* O - array of flags = 1 if nultyp = 2        */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of logical values from a column in the current FITS HDU.
*/
{
    char nulval = 0;

    ffgcll( fptr, colnum, firstrow, firstelem, nelem, 2, nulval, array,
            nularray, anynul, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcll( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG  firstelem, /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of values to read                */
            int   nultyp,     /* I - null value handling code:               */
                              /*     1: set undefined pixels = nulval        */
                              /*     2: set nularray=1 for undefined pixels  */
            char nulval,      /* I - value for null pixels if nultyp = 1     */
            char *array,      /* O - array of values                         */
            char *nularray,   /* O - array of flags = 1 if nultyp = 2        */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of logical values from a column in the current FITS HDU.
*/
{
    double dtemp;
    int tcode, maxelem, hdutype, ii, nulcheck;
    long twidth, incre;
    long ntodo;
    LONGLONG repeat, startpos, elemnum, readptr, tnull, rowlen, rownum, remain, next;
    double scale, zero;
    char tform[20];
    char message[FLEN_ERRMSG];
    char snull[20];   /*  the FITS null value  */
    unsigned char buffer[DBUFFSIZE], *buffptr;

    if (*status > 0 || nelem == 0)  /* inherit input status value if > 0 */
        return(*status);

    if (anynul)
       *anynul = 0;

    if (nultyp == 2)      
       memset(nularray, 0, (size_t) nelem);   /* initialize nullarray */

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/
    if (ffgcprll( fptr, colnum, firstrow, firstelem, nelem, 0, &scale, &zero,
        tform, &twidth, &tcode, &maxelem, &startpos,  &elemnum, &incre,
        &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0)
        return(*status);

    if (tcode != TLOGICAL)   
        return(*status = NOT_LOGICAL_COL);
 
    /*------------------------------------------------------------------*/
    /*  Decide whether to check for null values in the input FITS file: */
    /*------------------------------------------------------------------*/
    nulcheck = nultyp; /* by default, check for null values in the FITS file */

    if (nultyp == 1 && nulval == 0)
       nulcheck = 0;    /* calling routine does not want to check for nulls */

    /*---------------------------------------------------------------------*/
    /*  Now read the logical values from the FITS column.                  */
    /*---------------------------------------------------------------------*/

    remain = nelem;           /* remaining number of values to read */
    next = 0;                 /* next element in array to be read   */
    rownum = 0;               /* row number, relative to firstrow   */
    ntodo = (long) remain;           /* max number of elements to read at one time */

    while (ntodo)
    {
      /*
         limit the number of pixels to read at one time to the number that
         remain in the current vector.    
      */
      ntodo = (long) minvalue(ntodo, maxelem);      
      ntodo = (long) minvalue(ntodo, (repeat - elemnum));

      readptr = startpos + (rowlen * rownum) + (elemnum * incre);

      ffgi1b(fptr, readptr, ntodo, incre, buffer, status);

      /* convert from T or F to 1 or 0 */
      buffptr = buffer;
      for (ii = 0; ii < ntodo; ii++, next++, buffptr++)
      {
        if (*buffptr == 'T')
          array[next] = 1;
        else if (*buffptr =='F') 
          array[next] = 0;
        else if (*buffptr == 0)
        {
          array[next] = nulval;  /* set null values to input nulval */
          if (anynul)
              *anynul = 1;

          if (nulcheck == 2)
          {
            nularray[next] = 1;  /* set null flags */
          }
        }
        else  /* some other illegal character; return the char value */
        {
          if (*buffptr == 1) {
            /* this is an unfortunate case where the illegal value is the same
               as what we set True values to, so set the value to the character '1'
               instead, which has ASCII value 49.  */
            array[next] = 49;
          } else {
            array[next] = (char) *buffptr;
          }
        }
      }

      if (*status > 0)  /* test for error during previous read operation */
      {
	dtemp = (double) next;
        sprintf(message,
          "Error reading elements %.0f thruough %.0f of logical array (ffgcl).",
           dtemp+1., dtemp + ntodo);
        ffpmsg(message);
        return(*status);
      }

      /*--------------------------------------------*/
      /*  increment the counters for the next loop  */
      /*--------------------------------------------*/
      remain -= ntodo;
      if (remain)
      {
        elemnum += ntodo;

        if (elemnum == repeat)  /* completed a row; start on later row */
          {
            elemnum = 0;
            rownum++;
          }
      }
      ntodo = (long) remain;  /* this is the maximum number to do in next loop */

    }  /*  End of main while Loop  */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcx(  fitsfile *fptr,  /* I - FITS file pointer                       */
            int   colnum,    /* I - number of column to write (1 = 1st col) */
            LONGLONG  frow,      /* I - first row to write (1 = 1st row)        */
            LONGLONG  fbit,      /* I - first bit to write (1 = 1st)            */
            LONGLONG  nbit,      /* I - number of bits to write                 */
            char *larray,    /* O - array of logicals corresponding to bits */
            int  *status)    /* IO - error status                           */
/*
  read an array of logical values from a specified bit or byte
  column of the binary table.    larray is set = TRUE, if the corresponding
  bit = 1, otherwise it is set to FALSE.
  The binary table column being read from must have datatype 'B' or 'X'. 
*/
{
    LONGLONG bstart;
    long offset, ndone, ii, repeat, bitloc, fbyte;
    LONGLONG  rstart, estart;
    int tcode, descrp;
    unsigned char cbuff;
    static unsigned char onbit[8] = {128,  64,  32,  16,   8,   4,   2,   1};
    tcolumn *colptr;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /*  check input parameters */
    if (nbit < 1)
        return(*status);
    else if (frow < 1)
        return(*status = BAD_ROW_NUM);
    else if (fbit < 1)
        return(*status = BAD_ELEM_NUM);

    /* position to the correct HDU */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    fbyte = (long) ((fbit + 7) / 8);
    bitloc = (long) (fbit - 1 - ((fbit - 1) / 8 * 8));
    ndone = 0;
    rstart = frow - 1;
    estart = fbyte - 1;

    colptr  = (fptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */

    tcode = colptr->tdatatype;

    if (abs(tcode) > TBYTE)
        return(*status = NOT_LOGICAL_COL); /* not correct datatype column */

    if (tcode > 0)
    {
        descrp = FALSE;  /* not a variable length descriptor column */
        /* N.B: REPEAT is the number of bytes, not number of bits */
        repeat = (long) colptr->trepeat;

        if (tcode == TBIT)
            repeat = (repeat + 7) / 8;  /* convert from bits to bytes */

        if (fbyte > repeat)
            return(*status = BAD_ELEM_NUM);

        /* calc the i/o pointer location to start of sequence of pixels */
        bstart = (fptr->Fptr)->datastart + ((fptr->Fptr)->rowlength * rstart) +
               colptr->tbcol + estart;
    }
    else
    {
        descrp = TRUE;  /* a variable length descriptor column */
        /* only bit arrays (tform = 'X') are supported for variable */
        /* length arrays.  REPEAT is the number of BITS in the array. */

        ffgdes(fptr, colnum, frow, &repeat, &offset, status);

        if (tcode == -TBIT)
            repeat = (repeat + 7) / 8;

        if ((fbit + nbit + 6) / 8 > repeat)
            return(*status = BAD_ELEM_NUM);

        /* calc the i/o pointer location to start of sequence of pixels */
        bstart = (fptr->Fptr)->datastart + offset + (fptr->Fptr)->heapstart + estart;
    }

    /* move the i/o pointer to the start of the pixel sequence */
    if (ffmbyt(fptr, bstart, REPORT_EOF, status) > 0)
        return(*status);

    /* read the next byte */
    while (1)
    {
      if (ffgbyt(fptr, 1, &cbuff, status) > 0)
        return(*status);

      for (ii = bitloc; (ii < 8) && (ndone < nbit); ii++, ndone++)
      {
        if(cbuff & onbit[ii])       /* test if bit is set */
          larray[ndone] = TRUE;
        else
          larray[ndone] = FALSE;
      }

      if (ndone == nbit)   /* finished all the bits */
        return(*status);

      /* not done, so get the next byte */
      if (!descrp)
      {
        estart++;
        if (estart == repeat) 
        {
          /* move the i/o pointer to the next row of pixels */
          estart = 0;
          rstart = rstart + 1;
          bstart = (fptr->Fptr)->datastart + ((fptr->Fptr)->rowlength * rstart) +
               colptr->tbcol;

          ffmbyt(fptr, bstart, REPORT_EOF, status);
        }
      }
      bitloc = 0;
    }
}
/*--------------------------------------------------------------------------*/
int ffgcxui(fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG  nrows,      /* I - no. of rows to read                     */
            long  input_first_bit, /* I - first bit to read (1 = 1st)        */
            int   input_nbits,     /* I - number of bits to read (<= 32)     */
            unsigned short *array, /* O - array of integer values            */
            int  *status)     /* IO - error status                           */
/*
  Read a consecutive string of bits from an 'X' or 'B' column and
  interprete them as an unsigned integer.  The number of bits must be
  less than or equal to 16 or the total number of bits in the column, 
  which ever is less.
*/
{
    int ii, firstbit, nbits, bytenum, startbit, numbits, endbit;
    int firstbyte, lastbyte, nbytes, rshift, lshift;
    unsigned short colbyte[5];
    tcolumn *colptr;
    char message[81];

    if (*status > 0 || nrows == 0)
        return(*status);

    /*  check input parameters */
    if (firstrow < 1)
    {
          sprintf(message, "Starting row number is less than 1: %ld (ffgcxui)",
                (long) firstrow);
          ffpmsg(message);
          return(*status = BAD_ROW_NUM);
    }
    else if (input_first_bit < 1)
    {
          sprintf(message, "Starting bit number is less than 1: %ld (ffgcxui)",
                input_first_bit);
          ffpmsg(message);
          return(*status = BAD_ELEM_NUM);
    }
    else if (input_nbits > 16)
    {
          sprintf(message, "Number of bits to read is > 16: %d (ffgcxui)",
                input_nbits);
          ffpmsg(message);
          return(*status = BAD_ELEM_NUM);
    }

    /* position to the correct HDU */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype != BINARY_TBL)
    {
        ffpmsg("This is not a binary table extension (ffgcxui)");
        return(*status = NOT_BTABLE);
    }

    if (colnum > (fptr->Fptr)->tfield)
    {
      sprintf(message, "Specified column number is out of range: %d (ffgcxui)",
                colnum);
        ffpmsg(message);
        sprintf(message, "  There are %d columns in this table.",
                (fptr->Fptr)->tfield );
        ffpmsg(message);

        return(*status = BAD_COL_NUM);
    }       

    colptr  = (fptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */

    if (abs(colptr->tdatatype) > TBYTE)
    {
        ffpmsg("Can only read bits from X or B type columns. (ffgcxui)");
        return(*status = NOT_LOGICAL_COL); /* not correct datatype column */
    }

    firstbyte = (input_first_bit - 1              ) / 8 + 1;
    lastbyte  = (input_first_bit + input_nbits - 2) / 8 + 1;
    nbytes = lastbyte - firstbyte + 1;

    if (colptr->tdatatype == TBIT && 
        input_first_bit + input_nbits - 1 > (long) colptr->trepeat)
    {
        ffpmsg("Too many bits. Tried to read past width of column (ffgcxui)");
        return(*status = BAD_ELEM_NUM);
    }
    else if (colptr->tdatatype == TBYTE && lastbyte > (long) colptr->trepeat)
    {
        ffpmsg("Too many bits. Tried to read past width of column (ffgcxui)");
        return(*status = BAD_ELEM_NUM);
    }

    for (ii = 0; ii < nrows; ii++)
    {
        /* read the relevant bytes from the row */
        if (ffgcvui(fptr, colnum, firstrow+ii, firstbyte, nbytes, 0, 
               colbyte, NULL, status) > 0)
        {
             ffpmsg("Error reading bytes from column (ffgcxui)");
             return(*status);
        }

        firstbit = (input_first_bit - 1) % 8; /* modulus operator */
        nbits = input_nbits;

        array[ii] = 0;

        /* select and shift the bits from each byte into the output word */
        while(nbits)
        {
            bytenum = firstbit / 8;

            startbit = firstbit % 8;  
            numbits = minvalue(nbits, 8 - startbit);
            endbit = startbit + numbits - 1;

            rshift = 7 - endbit;
            lshift = nbits - numbits;

            array[ii] = ((colbyte[bytenum] >> rshift) << lshift) | array[ii];

            nbits -= numbits;
            firstbit += numbits;
        }
    }

    return(*status);
}

/*--------------------------------------------------------------------------*/
int ffgcxuk(fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG  nrows,      /* I - no. of rows to read                     */
            long  input_first_bit, /* I - first bit to read (1 = 1st)        */
            int   input_nbits,     /* I - number of bits to read (<= 32)     */
            unsigned int *array,   /* O - array of integer values            */
            int  *status)     /* IO - error status                           */
/*
  Read a consecutive string of bits from an 'X' or 'B' column and
  interprete them as an unsigned integer.  The number of bits must be
  less than or equal to 32 or the total number of bits in the column, 
  which ever is less.
*/
{
    int ii, firstbit, nbits, bytenum, startbit, numbits, endbit;
    int firstbyte, lastbyte, nbytes, rshift, lshift;
    unsigned int colbyte[5];
    tcolumn *colptr;
    char message[81];

    if (*status > 0 || nrows == 0)
        return(*status);

    /*  check input parameters */
    if (firstrow < 1)
    {
          sprintf(message, "Starting row number is less than 1: %ld (ffgcxuk)",
                (long) firstrow);
          ffpmsg(message);
          return(*status = BAD_ROW_NUM);
    }
    else if (input_first_bit < 1)
    {
          sprintf(message, "Starting bit number is less than 1: %ld (ffgcxuk)",
                input_first_bit);
          ffpmsg(message);
          return(*status = BAD_ELEM_NUM);
    }
    else if (input_nbits > 32)
    {
          sprintf(message, "Number of bits to read is > 32: %d (ffgcxuk)",
                input_nbits);
          ffpmsg(message);
          return(*status = BAD_ELEM_NUM);
    }

    /* position to the correct HDU */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype != BINARY_TBL)
    {
        ffpmsg("This is not a binary table extension (ffgcxuk)");
        return(*status = NOT_BTABLE);
    }

    if (colnum > (fptr->Fptr)->tfield)
    {
      sprintf(message, "Specified column number is out of range: %d (ffgcxuk)",
                colnum);
        ffpmsg(message);
        sprintf(message, "  There are %d columns in this table.",
                (fptr->Fptr)->tfield );
        ffpmsg(message);

        return(*status = BAD_COL_NUM);
    }       

    colptr  = (fptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */

    if (abs(colptr->tdatatype) > TBYTE)
    {
        ffpmsg("Can only read bits from X or B type columns. (ffgcxuk)");
        return(*status = NOT_LOGICAL_COL); /* not correct datatype column */
    }

    firstbyte = (input_first_bit - 1              ) / 8 + 1;
    lastbyte  = (input_first_bit + input_nbits - 2) / 8 + 1;
    nbytes = lastbyte - firstbyte + 1;

    if (colptr->tdatatype == TBIT && 
        input_first_bit + input_nbits - 1 > (long) colptr->trepeat)
    {
        ffpmsg("Too many bits. Tried to read past width of column (ffgcxuk)");
        return(*status = BAD_ELEM_NUM);
    }
    else if (colptr->tdatatype == TBYTE && lastbyte > (long) colptr->trepeat)
    {
        ffpmsg("Too many bits. Tried to read past width of column (ffgcxuk)");
        return(*status = BAD_ELEM_NUM);
    }

    for (ii = 0; ii < nrows; ii++)
    {
        /* read the relevant bytes from the row */
        if (ffgcvuk(fptr, colnum, firstrow+ii, firstbyte, nbytes, 0, 
               colbyte, NULL, status) > 0)
        {
             ffpmsg("Error reading bytes from column (ffgcxuk)");
             return(*status);
        }

        firstbit = (input_first_bit - 1) % 8; /* modulus operator */
        nbits = input_nbits;

        array[ii] = 0;

        /* select and shift the bits from each byte into the output word */
        while(nbits)
        {
            bytenum = firstbit / 8;

            startbit = firstbit % 8;  
            numbits = minvalue(nbits, 8 - startbit);
            endbit = startbit + numbits - 1;

            rshift = 7 - endbit;
            lshift = nbits - numbits;

            array[ii] = ((colbyte[bytenum] >> rshift) << lshift) | array[ii];

            nbits -= numbits;
            firstbit += numbits;
        }
    }

    return(*status);
}
