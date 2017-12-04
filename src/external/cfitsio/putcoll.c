/*  This file, putcoll.c, contains routines that write data elements to    */
/*  a FITS image or table, with logical datatype.                          */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int ffpcll( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            LONGLONG  firstrow,  /* I - first row to write (1 = 1st row)        */
            LONGLONG  firstelem, /* I - first vector element to write (1 = 1st) */
            LONGLONG  nelem,     /* I - number of values to write               */
            char *array,     /* I - array of values to write                */
            int  *status)    /* IO - error status                           */
/*
  Write an array of logical values to a column in the current FITS HDU.
*/
{
    int tcode, maxelem, hdutype;
    long twidth, incre;
    LONGLONG repeat, startpos, elemnum, wrtptr, rowlen, rownum, remain, next, tnull;
    double scale, zero;
    char tform[20], ctrue = 'T', cfalse = 'F';
    char message[FLEN_ERRMSG];
    char snull[20];   /*  the FITS null value  */

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/
    if (ffgcprll( fptr, colnum, firstrow, firstelem, nelem, 1, &scale, &zero,
        tform, &twidth, &tcode, &maxelem, &startpos,  &elemnum, &incre,
        &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0)
        return(*status);

    if (tcode != TLOGICAL)   
        return(*status = NOT_LOGICAL_COL);

    /*---------------------------------------------------------------------*/
    /*  Now write the logical values one at a time to the FITS column.     */
    /*---------------------------------------------------------------------*/
    remain = nelem;           /* remaining number of values to write  */
    next = 0;                 /* next element in array to be written  */
    rownum = 0;               /* row number, relative to firstrow     */

    while (remain)
    {
      wrtptr = startpos + (rowlen * rownum) + (elemnum * incre);

      ffmbyt(fptr, wrtptr, IGNORE_EOF, status);  /* move to write position */

      if (array[next])
         ffpbyt(fptr, 1, &ctrue, status);
      else
         ffpbyt(fptr, 1, &cfalse, status);

      if (*status > 0)  /* test for error during previous write operation */
      {
        sprintf(message,
           "Error writing element %.0f of input array of logicals (ffpcll).",
            (double) (next+1));
        ffpmsg(message);
        return(*status);
      }

      /*--------------------------------------------*/
      /*  increment the counters for the next loop  */
      /*--------------------------------------------*/
      remain--;
      if (remain)
      {
        next++;
        elemnum++;
        if (elemnum == repeat)  /* completed a row; start on next row */
        {
           elemnum = 0;
           rownum++;
        }
      }

    }  /*  End of main while Loop  */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpcnl( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            LONGLONG  firstrow,  /* I - first row to write (1 = 1st row)        */
            LONGLONG  firstelem, /* I - first vector element to write (1 = 1st) */
            LONGLONG  nelem,     /* I - number of values to write               */
            char  *array,    /* I - array of values to write                */
            char  nulvalue,  /* I - array flagging undefined pixels if true */
            int  *status)    /* IO - error status                           */
/*
  Write an array of elements to the specified column of a table.  Any input
  pixels flagged as null will be replaced by the appropriate
  null value in the output FITS file. 
*/
{
    tcolumn *colptr;
    LONGLONG  ngood = 0, nbad = 0, ii;
    LONGLONG repeat, first, fstelm, fstrow;
    int tcode;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
    {
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);
    }

    colptr  = (fptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */

    tcode  = colptr->tdatatype;

    if (tcode > 0)
       repeat = colptr->trepeat;  /* repeat count for this column */
    else
       repeat = firstelem -1 + nelem;  /* variable length arrays */

    /* first write the whole input vector, then go back and fill in the nulls */
    if (ffpcll(fptr, colnum, firstrow, firstelem, nelem, array, status) > 0)
          return(*status);

    /* absolute element number in the column */
    first = (firstrow - 1) * repeat + firstelem;

    for (ii = 0; ii < nelem; ii++)
    {
      if (array[ii] != nulvalue)  /* is this a good pixel? */
      {
         if (nbad)  /* write previous string of bad pixels */
         {
            fstelm = ii - nbad + first;  /* absolute element number */
            fstrow = (fstelm - 1) / repeat + 1;  /* starting row number */
            fstelm = fstelm - (fstrow - 1) * repeat;  /* relative number */

            if (ffpclu(fptr, colnum, fstrow, fstelm, nbad, status) > 0)
                return(*status);

            nbad=0;
         }

         ngood = ngood +1;  /* the consecutive number of good pixels */
      }
      else
      {
         if (ngood)  /* write previous string of good pixels */
         {
            fstelm = ii - ngood + first;  /* absolute element number */
            fstrow = (fstelm - 1) / repeat + 1;  /* starting row number */
            fstelm = fstelm - (fstrow - 1) * repeat;  /* relative number */

/*  good values have already been written
            if (ffpcll(fptr, colnum, fstrow, fstelm, ngood, &array[ii-ngood],
                status) > 0)
                return(*status);
*/
            ngood=0;
         }

         nbad = nbad +1;  /* the consecutive number of bad pixels */
      }
    }

    /* finished loop;  now just write the last set of pixels */

    if (ngood)  /* write last string of good pixels */
    {
      fstelm = ii - ngood + first;  /* absolute element number */
      fstrow = (fstelm - 1) / repeat + 1;  /* starting row number */
      fstelm = fstelm - (fstrow - 1) * repeat;  /* relative number */

/*  these have already been written
      ffpcll(fptr, colnum, fstrow, fstelm, ngood, &array[ii-ngood], status);
*/
    }
    else if (nbad) /* write last string of bad pixels */
    {
      fstelm = ii - nbad + first;  /* absolute element number */
      fstrow = (fstelm - 1) / repeat + 1;  /* starting row number */
      fstelm = fstelm - (fstrow - 1) * repeat;  /* relative number */

      ffpclu(fptr, colnum, fstrow, fstelm, nbad, status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpclx( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            LONGLONG  frow,      /* I - first row to write (1 = 1st row)        */
            long  fbit,      /* I - first bit to write (1 = 1st)            */
            long  nbit,      /* I - number of bits to write                 */
            char *larray,    /* I - array of logicals corresponding to bits */
            int  *status)    /* IO - error status                           */
/*
  write an array of logical values to a specified bit or byte
  column of the binary table.   If larray is TRUE, then the corresponding
  bit is set to 1, otherwise it is set to 0.
  The binary table column being written to must have datatype 'B' or 'X'. 
*/
{
    LONGLONG offset, bstart, repeat, rowlen, elemnum, rstart, estart, tnull;
    long fbyte, lbyte, nbyte, bitloc, ndone;
    long ii, twidth, incre;
    int tcode, descrp, maxelem, hdutype;
    double dummyd;
    char tform[12], snull[12];
    unsigned char cbuff;
    static unsigned char onbit[8] = {128,  64,  32,  16,   8,   4,   2,   1};
    static unsigned char offbit[8] = {127, 191, 223, 239, 247, 251, 253, 254};
    LONGLONG heapoffset, lrepeat;
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

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    fbyte = (fbit + 7) / 8;
    lbyte = (fbit + nbit + 6) / 8;
    nbyte = lbyte - fbyte +1;

    /* Save the current heapsize; ffgcprll will increment the value if */
    /* we are writing to a variable length column. */
    offset = (fptr->Fptr)->heapsize;

    /* call ffgcprll in case we are writing beyond the current end of   */
    /* the table; it will allocate more space and shift any following */
    /* HDU's.  Otherwise, we have little use for most of the returned */
    /* parameters, therefore just use dummy parameters.               */

    if (ffgcprll( fptr, colnum, frow, fbyte, nbyte, 1, &dummyd, &dummyd,
        tform, &twidth, &tcode, &maxelem, &bstart, &elemnum, &incre,
        &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0)
        return(*status);

    bitloc = fbit - 1 - ((fbit - 1) / 8 * 8);
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
        repeat = colptr->trepeat;

        if (tcode == TBIT)
            repeat = (repeat + 7) / 8; /* convert from bits to bytes */

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

        repeat = fbit + nbit -1;

        /* write the number of elements and the starting offset.    */
        /* Note: ffgcprll previous wrote the descripter, but with the */
        /* wrong repeat value  (gave bytes instead of bits).        */
        /* Make sure to not change the current heap offset value!  */

        if (tcode == -TBIT) {
            ffgdesll(fptr, colnum, frow, &lrepeat, &heapoffset, status);
            ffpdes(  fptr, colnum, frow, (long) repeat, heapoffset, status);
	}

        /* Calc the i/o pointer location to start of sequence of pixels.   */
        /* ffgcprll has already calculated a value for bstart that         */
        /* points to the first element of the vector; we just have to      */
        /* increment it to point to the first element we want to write to. */
        /* Note: ffgcprll also already updated the size of the heap, so we */
        /* don't have to do that again here.                               */

        bstart += estart;
    }

    /* move the i/o pointer to the start of the pixel sequence */
    ffmbyt(fptr, bstart, IGNORE_EOF, status);

    /* read the next byte (we may only be modifying some of the bits) */
    while (1)
    {
      if (ffgbyt(fptr, 1, &cbuff, status) == END_OF_FILE)
      {
        /* hit end of file trying to read the byte, so just set byte = 0 */
        *status = 0;
        cbuff = 0;
      }

      /* move back, to be able to overwrite the byte */
      ffmbyt(fptr, bstart, IGNORE_EOF, status);
 
      for (ii = bitloc; (ii < 8) && (ndone < nbit); ii++, ndone++)
      {
        if(larray[ndone])
          cbuff = cbuff | onbit[ii];
        else
          cbuff = cbuff & offbit[ii];
      }

      ffpbyt(fptr, 1, &cbuff, status); /* write the modified byte */
      if (ndone == nbit)  /* finished all the bits */
        return(*status);

      /* not done, so get the next byte */
      bstart++;
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

          ffmbyt(fptr, bstart, IGNORE_EOF, status);
        }
      }
      bitloc = 0;
    }
}

