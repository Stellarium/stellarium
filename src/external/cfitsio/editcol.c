/*  This file, editcol.c, contains the set of FITSIO routines that    */
/*  insert or delete rows or columns in a table or resize an image    */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "fitsio2.h"
/*--------------------------------------------------------------------------*/
int ffrsim(fitsfile *fptr,      /* I - FITS file pointer           */
           int bitpix,          /* I - bits per pixel              */
           int naxis,           /* I - number of axes in the array */
           long *naxes,         /* I - size of each axis           */
           int *status)         /* IO - error status               */
/*
   resize an existing primary array or IMAGE extension.
*/
{
    LONGLONG tnaxes[99];
    int ii;
    
    if (*status > 0)
        return(*status);

    for (ii = 0; (ii < naxis) && (ii < 99); ii++)
        tnaxes[ii] = naxes[ii];

    ffrsimll(fptr, bitpix, naxis, tnaxes, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffrsimll(fitsfile *fptr,    /* I - FITS file pointer           */
           int bitpix,          /* I - bits per pixel              */
           int naxis,           /* I - number of axes in the array */
           LONGLONG *naxes,     /* I - size of each axis           */
           int *status)         /* IO - error status               */
/*
   resize an existing primary array or IMAGE extension.
*/
{
    int ii, simple, obitpix, onaxis, extend, nmodify;
    long  nblocks, longval;
    long pcount, gcount, longbitpix;
    LONGLONG onaxes[99], newsize, oldsize;
    char comment[FLEN_COMMENT], keyname[FLEN_KEYWORD], message[FLEN_ERRMSG];

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
         /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    /* get current image size parameters */
    if (ffghprll(fptr, 99, &simple, &obitpix, &onaxis, onaxes, &pcount,
               &gcount, &extend, status) > 0)
        return(*status);

    longbitpix = bitpix;

    /* test for the 2 special cases that represent unsigned integers */
    if (longbitpix == USHORT_IMG)
        longbitpix = SHORT_IMG;
    else if (longbitpix == ULONG_IMG)
        longbitpix = LONG_IMG;

    /* test that the new values are legal */

    if (longbitpix != BYTE_IMG && longbitpix != SHORT_IMG && 
        longbitpix != LONG_IMG && longbitpix != LONGLONG_IMG &&
        longbitpix != FLOAT_IMG && longbitpix != DOUBLE_IMG)
    {
        sprintf(message,
        "Illegal value for BITPIX keyword: %d", bitpix);
        ffpmsg(message);
        return(*status = BAD_BITPIX);
    }

    if (naxis < 0 || naxis > 999)
    {
        sprintf(message,
        "Illegal value for NAXIS keyword: %d", naxis);
        ffpmsg(message);
        return(*status = BAD_NAXIS);
    }

    if (naxis == 0)
        newsize = 0;
    else
        newsize = 1;

    for (ii = 0; ii < naxis; ii++)
    {
        if (naxes[ii] < 0)
        {
            sprintf(message,
            "Illegal value for NAXIS%d keyword: %.0f", ii + 1,  (double) (naxes[ii]));
            ffpmsg(message);
            return(*status = BAD_NAXES);
        }

        newsize *= naxes[ii];  /* compute new image size, in pixels */
    }

    /* compute size of old image, in bytes */

    if (onaxis == 0)
        oldsize = 0;
    else
    {
        oldsize = 1;
        for (ii = 0; ii < onaxis; ii++)
            oldsize *= onaxes[ii];  
        oldsize = (oldsize + pcount) * gcount * (abs(obitpix) / 8);
    }

    oldsize = (oldsize + 2879) / 2880; /* old size, in blocks */

    newsize = (newsize + pcount) * gcount * (abs(longbitpix) / 8);
    newsize = (newsize + 2879) / 2880; /* new size, in blocks */

    if (newsize > oldsize)   /* have to insert new blocks for image */
    {
        nblocks = (long) (newsize - oldsize);
        if (ffiblk(fptr, nblocks, 1, status) > 0)  
            return(*status);
    }
    else if (oldsize > newsize)  /* have to delete blocks from image */
    {
        nblocks = (long) (oldsize - newsize);
        if (ffdblk(fptr, nblocks, status) > 0)  
            return(*status);
    }

    /* now update the header keywords */

    strcpy(comment,"&");  /* special value to leave comments unchanged */

    if (longbitpix != obitpix)
    {                         /* update BITPIX value */
        ffmkyj(fptr, "BITPIX", longbitpix, comment, status);
    }

    if (naxis != onaxis)
    {                        /* update NAXIS value */
        longval = naxis;
        ffmkyj(fptr, "NAXIS", longval, comment, status);
    }

    /* modify the existing NAXISn keywords */
    nmodify = minvalue(naxis, onaxis); 
    for (ii = 0; ii < nmodify; ii++)
    {
        ffkeyn("NAXIS", ii+1, keyname, status);
        ffmkyj(fptr, keyname, naxes[ii], comment, status);
    }

    if (naxis > onaxis)  /* insert additional NAXISn keywords */
    {
        strcpy(comment,"length of data axis");  
        for (ii = onaxis; ii < naxis; ii++)
        {
            ffkeyn("NAXIS", ii+1, keyname, status);
            ffikyj(fptr, keyname, naxes[ii], comment, status);
        }
    }
    else if (onaxis > naxis) /* delete old NAXISn keywords */
    {
        for (ii = naxis; ii < onaxis; ii++)
        {
            ffkeyn("NAXIS", ii+1, keyname, status);
            ffdkey(fptr, keyname, status);
        }
    }

    /* Update the BSCALE and BZERO keywords, if an unsigned integer image */
    if (bitpix == USHORT_IMG)
    {
        strcpy(comment, "offset data range to that of unsigned short");
        ffukyg(fptr, "BZERO", 32768., 0, comment, status);
        strcpy(comment, "default scaling factor");
        ffukyg(fptr, "BSCALE", 1.0, 0, comment, status);
    }
    else if (bitpix == ULONG_IMG)
    {
        strcpy(comment, "offset data range to that of unsigned long");
        ffukyg(fptr, "BZERO", 2147483648., 0, comment, status);
        strcpy(comment, "default scaling factor");
        ffukyg(fptr, "BSCALE", 1.0, 0, comment, status);
    }

    /* re-read the header, to make sure structures are updated */
    ffrdef(fptr, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffirow(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG firstrow,   /* I - insert space AFTER this row              */
                            /*     0 = insert space at beginning of table   */
           LONGLONG nrows,      /* I - number of rows to insert                 */
           int *status)     /* IO - error status                            */
/*
 insert NROWS blank rows immediated after row firstrow (1 = first row).
 Set firstrow = 0 to insert space at the beginning of the table.
*/
{
    int tstatus;
    LONGLONG naxis1, naxis2;
    LONGLONG datasize, firstbyte, nshift, nbytes;
    LONGLONG freespace;
    long nblock;

    if (*status > 0)
        return(*status);

        /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
         /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        ffpmsg("Can only add rows to TABLE or BINTABLE extension (ffirow)");
        return(*status = NOT_TABLE);
    }

    if (nrows < 0 )
        return(*status = NEG_BYTES);
    else if (nrows == 0)
        return(*status);   /* no op, so just return */

    /* get the current size of the table */
    /* use internal structure since NAXIS2 keyword may not be up to date */
    naxis1 = (fptr->Fptr)->rowlength;
    naxis2 = (fptr->Fptr)->numrows;

    if (firstrow > naxis2)
    {
        ffpmsg(
   "Insert position greater than the number of rows in the table (ffirow)");
        return(*status = BAD_ROW_NUM);
    }
    else if (firstrow < 0)
    {
        ffpmsg("Insert position is less than 0 (ffirow)");
        return(*status = BAD_ROW_NUM);
    }

    /* current data size */
    datasize = (fptr->Fptr)->heapstart + (fptr->Fptr)->heapsize;
    freespace = ( ( (datasize + 2879) / 2880) * 2880) - datasize;
    nshift = naxis1 * nrows;          /* no. of bytes to add to table */

    if ( (freespace - nshift) < 0)   /* not enough existing space? */
    {
        nblock = (long) ((nshift - freespace + 2879) / 2880);   /* number of blocks */
        ffiblk(fptr, nblock, 1, status);               /* insert the blocks */
    }

    firstbyte = naxis1 * firstrow;    /* relative insert position */
    nbytes = datasize - firstbyte;           /* no. of bytes to shift down */
    firstbyte += ((fptr->Fptr)->datastart);  /* absolute insert position */

    ffshft(fptr, firstbyte, nbytes, nshift, status); /* shift rows and heap */

    /* update the heap starting address */
    (fptr->Fptr)->heapstart += nshift;

    /* update the THEAP keyword if it exists */
    tstatus = 0;
    ffmkyj(fptr, "THEAP", (fptr->Fptr)->heapstart, "&", &tstatus);

    /* update the NAXIS2 keyword */
    ffmkyj(fptr, "NAXIS2", naxis2 + nrows, "&", status);
    ((fptr->Fptr)->numrows) += nrows;
    ((fptr->Fptr)->origrows) += nrows;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdrow(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG firstrow,   /* I - first row to delete (1 = first)          */
           LONGLONG nrows,      /* I - number of rows to delete                 */
           int *status)     /* IO - error status                            */
/*
 delete NROWS rows from table starting with firstrow (1 = first row of table).
*/
{
    int tstatus;
    LONGLONG naxis1, naxis2;
    LONGLONG datasize, firstbyte, nbytes, nshift;
    LONGLONG freespace;
    long nblock;
    char comm[FLEN_COMMENT];

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
        /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        ffpmsg("Can only delete rows in TABLE or BINTABLE extension (ffdrow)");
        return(*status = NOT_TABLE);
    }

    if (nrows < 0 )
        return(*status = NEG_BYTES);
    else if (nrows == 0)
        return(*status);   /* no op, so just return */

    ffgkyjj(fptr, "NAXIS1", &naxis1, comm, status); /* get the current   */

   /* ffgkyj(fptr, "NAXIS2", &naxis2, comm, status);*/ /* size of the table */

    /* the NAXIS2 keyword may not be up to date, so use the structure value */
    naxis2 = (fptr->Fptr)->numrows;

    if (firstrow > naxis2)
    {
        ffpmsg(
   "Delete position greater than the number of rows in the table (ffdrow)");
        return(*status = BAD_ROW_NUM);
    }
    else if (firstrow < 1)
    {
        ffpmsg("Delete position is less than 1 (ffdrow)");
        return(*status = BAD_ROW_NUM);
    }
    else if (firstrow + nrows - 1 > naxis2)
    {
        ffpmsg("No. of rows to delete exceeds size of table (ffdrow)");
        return(*status = BAD_ROW_NUM);
    }

    nshift = naxis1 * nrows;   /* no. of bytes to delete from table */
    /* cur size of data */
    datasize = (fptr->Fptr)->heapstart + (fptr->Fptr)->heapsize;

    firstbyte = naxis1 * (firstrow + nrows - 1); /* relative del pos */
    nbytes = datasize - firstbyte;    /* no. of bytes to shift up */
    firstbyte += ((fptr->Fptr)->datastart);   /* absolute delete position */

    ffshft(fptr, firstbyte, nbytes,  nshift * (-1), status); /* shift data */

    freespace = ( ( (datasize + 2879) / 2880) * 2880) - datasize;
    nblock = (long) ((nshift + freespace) / 2880);   /* number of blocks */

    /* delete integral number blocks */
    if (nblock > 0) 
        ffdblk(fptr, nblock, status);

    /* update the heap starting address */
    (fptr->Fptr)->heapstart -= nshift;

    /* update the THEAP keyword if it exists */
    tstatus = 0;
    ffmkyj(fptr, "THEAP", (long)(fptr->Fptr)->heapstart, "&", &tstatus);

    /* update the NAXIS2 keyword */
    ffmkyj(fptr, "NAXIS2", naxis2 - nrows, "&", status);
    ((fptr->Fptr)->numrows) -= nrows;
    ((fptr->Fptr)->origrows) -= nrows;

    /* Update the heap data, if any.  This will remove any orphaned data */
    /* that was only pointed to by the rows that have been deleted */
    ffcmph(fptr, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdrrg(fitsfile *fptr,  /* I - FITS file pointer to table               */
           char *ranges,    /* I - ranges of rows to delete (1 = first)     */
           int *status)     /* IO - error status                            */
/*
 delete the ranges of rows from the table (1 = first row of table).

The 'ranges' parameter typically looks like:
    '10-20, 30 - 40, 55' or '50-'
and gives a list of rows or row ranges separated by commas.
*/
{
    char *cptr;
    int nranges, nranges2, ii;
    long *minrow, *maxrow, nrows, *rowarray, jj, kk;
    LONGLONG naxis2;
    
    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
        /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        ffpmsg("Can only delete rows in TABLE or BINTABLE extension (ffdrrg)");
        return(*status = NOT_TABLE);
    }

    /* the NAXIS2 keyword may not be up to date, so use the structure value */
    naxis2 = (fptr->Fptr)->numrows;

    /* find how many ranges were specified ( = no. of commas in string + 1) */
    cptr = ranges;
    for (nranges = 1; (cptr = strchr(cptr, ',')); nranges++)
        cptr++;
 
    minrow = calloc(nranges, sizeof(long));
    maxrow = calloc(nranges, sizeof(long));

    if (!minrow || !maxrow) {
        *status = MEMORY_ALLOCATION;
        ffpmsg("failed to allocate memory for row ranges (ffdrrg)");
        if (maxrow) free(maxrow);
        if (minrow) free(minrow);
        return(*status);
    }

    /* parse range list into array of range min and max values */
    ffrwrg(ranges, naxis2, nranges, &nranges2, minrow, maxrow, status);
    if (*status > 0 || nranges2 == 0) {
        free(maxrow);
        free(minrow);
        return(*status);
    }

    /* determine total number or rows to delete */
    nrows = 0;
    for (ii = 0; ii < nranges2; ii++) {
       nrows = nrows + maxrow[ii] - minrow[ii] + 1;
    }

    rowarray = calloc(nrows, sizeof(long));
    if (!rowarray) {
        *status = MEMORY_ALLOCATION;
        ffpmsg("failed to allocate memory for row array (ffdrrg)");
        return(*status);
    }

    for (kk = 0, ii = 0; ii < nranges2; ii++) {
       for (jj = minrow[ii]; jj <= maxrow[ii]; jj++) {
           rowarray[kk] = jj;
           kk++;
       }
    }

    /* delete the rows */
    ffdrws(fptr, rowarray, nrows, status);
    
    free(rowarray);
    free(maxrow);
    free(minrow);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdrws(fitsfile *fptr,  /* I - FITS file pointer                        */
           long *rownum,    /* I - list of rows to delete (1 = first)       */
           long nrows,      /* I - number of rows to delete                 */
           int *status)     /* IO - error status                            */
/*
 delete the list of rows from the table (1 = first row of table).
*/
{
    LONGLONG naxis1, naxis2, insertpos, nextrowpos;
    long ii, nextrow;
    char comm[FLEN_COMMENT];
    unsigned char *buffer;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* rescan header if data structure is undefined */
    if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        ffpmsg("Can only delete rows in TABLE or BINTABLE extension (ffdrws)");
        return(*status = NOT_TABLE);
    }

    if (nrows < 0 )
        return(*status = NEG_BYTES);
    else if (nrows == 0)
        return(*status);   /* no op, so just return */

    ffgkyjj(fptr, "NAXIS1", &naxis1, comm, status); /* row width   */
    ffgkyjj(fptr, "NAXIS2", &naxis2, comm, status); /* number of rows */

    /* check that input row list is in ascending order */
    for (ii = 1; ii < nrows; ii++)
    {
        if (rownum[ii - 1] >= rownum[ii])
        {
            ffpmsg("row numbers are not in increasing order (ffdrws)");
            return(*status = BAD_ROW_NUM);
        }
    }

    if (rownum[0] < 1)
    {
        ffpmsg("first row to delete is less than 1 (ffdrws)");
        return(*status = BAD_ROW_NUM);
    }
    else if (rownum[nrows - 1] > naxis2)
    {
        ffpmsg("last row to delete exceeds size of table (ffdrws)");
        return(*status = BAD_ROW_NUM);
    }

    buffer = (unsigned char *) malloc( (size_t) naxis1);  /* buffer for one row */

    if (!buffer)
    {
        ffpmsg("malloc failed (ffdrws)");
        return(*status = MEMORY_ALLOCATION);
    }

    /* byte location to start of first row to delete, and the next row */
    insertpos = (fptr->Fptr)->datastart + ((rownum[0] - 1) * naxis1);
    nextrowpos = insertpos + naxis1;
    nextrow = rownum[0] + 1;

    /* work through the list of rows to delete */
    for (ii = 1; ii < nrows; nextrow++, nextrowpos += naxis1)
    {
        if (nextrow < rownum[ii])  
        {   /* keep this row, so copy it to the new position */

            ffmbyt(fptr, nextrowpos, REPORT_EOF, status);
            ffgbyt(fptr, naxis1, buffer, status);  /* read the bytes */

            ffmbyt(fptr, insertpos, IGNORE_EOF, status);
            ffpbyt(fptr, naxis1, buffer, status);  /* write the bytes */

            if (*status > 0)
            {
                ffpmsg("error while copying good rows in table (ffdrws)");
                free(buffer);
                return(*status);
            }
            insertpos += naxis1;
        }
        else
        {   /* skip over this row since it is in the list */
            ii++;
        }
    }

    /* finished with all the rows to delete; copy remaining rows */
    while(nextrow <= naxis2)
    {
        ffmbyt(fptr, nextrowpos, REPORT_EOF, status);
        ffgbyt(fptr, naxis1, buffer, status);  /* read the bytes */

        ffmbyt(fptr, insertpos, IGNORE_EOF, status);
        ffpbyt(fptr, naxis1, buffer, status);  /* write the bytes */

        if (*status > 0)
        {
            ffpmsg("failed to copy remaining rows in table (ffdrws)");
            free(buffer);
            return(*status);
        }
        insertpos  += naxis1;
        nextrowpos += naxis1;
        nextrow++; 
    }
    free(buffer);
    
    /* now delete the empty rows at the end of the table */
    ffdrow(fptr, naxis2 - nrows + 1, nrows, status);

    /* Update the heap data, if any.  This will remove any orphaned data */
    /* that was only pointed to by the rows that have been deleted */
    ffcmph(fptr, status);
    
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdrwsll(fitsfile *fptr, /* I - FITS file pointer                        */
           LONGLONG *rownum, /* I - list of rows to delete (1 = first)       */
           LONGLONG nrows,  /* I - number of rows to delete                 */
           int *status)     /* IO - error status                            */
/*
 delete the list of rows from the table (1 = first row of table).
*/
{
    LONGLONG insertpos, nextrowpos;
    LONGLONG naxis1, naxis2, ii, nextrow;
    char comm[FLEN_COMMENT];
    unsigned char *buffer;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* rescan header if data structure is undefined */
    if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        ffpmsg("Can only delete rows in TABLE or BINTABLE extension (ffdrws)");
        return(*status = NOT_TABLE);
    }

    if (nrows < 0 )
        return(*status = NEG_BYTES);
    else if (nrows == 0)
        return(*status);   /* no op, so just return */

    ffgkyjj(fptr, "NAXIS1", &naxis1, comm, status); /* row width   */
    ffgkyjj(fptr, "NAXIS2", &naxis2, comm, status); /* number of rows */

    /* check that input row list is in ascending order */
    for (ii = 1; ii < nrows; ii++)
    {
        if (rownum[ii - 1] >= rownum[ii])
        {
            ffpmsg("row numbers are not in increasing order (ffdrws)");
            return(*status = BAD_ROW_NUM);
        }
    }

    if (rownum[0] < 1)
    {
        ffpmsg("first row to delete is less than 1 (ffdrws)");
        return(*status = BAD_ROW_NUM);
    }
    else if (rownum[nrows - 1] > naxis2)
    {
        ffpmsg("last row to delete exceeds size of table (ffdrws)");
        return(*status = BAD_ROW_NUM);
    }

    buffer = (unsigned char *) malloc( (size_t) naxis1);  /* buffer for one row */

    if (!buffer)
    {
        ffpmsg("malloc failed (ffdrwsll)");
        return(*status = MEMORY_ALLOCATION);
    }

    /* byte location to start of first row to delete, and the next row */
    insertpos = (fptr->Fptr)->datastart + ((rownum[0] - 1) * naxis1);
    nextrowpos = insertpos + naxis1;
    nextrow = rownum[0] + 1;

    /* work through the list of rows to delete */
    for (ii = 1; ii < nrows; nextrow++, nextrowpos += naxis1)
    {
        if (nextrow < rownum[ii])  
        {   /* keep this row, so copy it to the new position */

            ffmbyt(fptr, nextrowpos, REPORT_EOF, status);
            ffgbyt(fptr, naxis1, buffer, status);  /* read the bytes */

            ffmbyt(fptr, insertpos, IGNORE_EOF, status);
            ffpbyt(fptr, naxis1, buffer, status);  /* write the bytes */

            if (*status > 0)
            {
                ffpmsg("error while copying good rows in table (ffdrws)");
                free(buffer);
                return(*status);
            }
            insertpos += naxis1;
        }
        else
        {   /* skip over this row since it is in the list */
            ii++;
        }
    }

    /* finished with all the rows to delete; copy remaining rows */
    while(nextrow <= naxis2)
    {
        ffmbyt(fptr, nextrowpos, REPORT_EOF, status);
        ffgbyt(fptr, naxis1, buffer, status);  /* read the bytes */

        ffmbyt(fptr, insertpos, IGNORE_EOF, status);
        ffpbyt(fptr, naxis1, buffer, status);  /* write the bytes */

        if (*status > 0)
        {
            ffpmsg("failed to copy remaining rows in table (ffdrws)");
            free(buffer);
            return(*status);
        }
        insertpos  += naxis1;
        nextrowpos += naxis1;
        nextrow++; 
    }
    free(buffer);
    
    /* now delete the empty rows at the end of the table */
    ffdrow(fptr, naxis2 - nrows + 1, nrows, status);

    /* Update the heap data, if any.  This will remove any orphaned data */
    /* that was only pointed to by the rows that have been deleted */
    ffcmph(fptr, status);
    
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffrwrg(
      char *rowlist,      /* I - list of rows and row ranges */
      LONGLONG maxrows,       /* I - number of rows in the table */
      int maxranges,     /* I - max number of ranges to be returned */
      int *numranges,    /* O - number ranges returned */
      long *minrow,       /* O - first row in each range */
      long *maxrow,       /* O - last row in each range */
      int *status)        /* IO - status value */
{
/*
   parse the input list of row ranges, returning the number of ranges,
   and the min and max row value in each range. 

   The only characters allowed in the input rowlist are 
       decimal digits, minus sign, and comma (and non-significant spaces) 

   Example:  

     list = "10-20, 30-35,50"

   would return numranges = 3, minrow[] = {10, 30, 50}, maxrow[] = {20, 35, 50}

   error is returned if min value of range is > max value of range or if the
   ranges are not monotonically increasing.
*/
    char *next;
    long minval, maxval;

    if (*status > 0)
        return(*status);

    if (maxrows <= 0 ) {
        *status = RANGE_PARSE_ERROR;
        ffpmsg("Input maximum range value is <= 0 (fits_parse_ranges)");
        return(*status);
    }

    next = rowlist;
    *numranges = 0;

    while (*next == ' ')next++;   /* skip spaces */
   
    while (*next != '\0') {

      /* find min value of next range; *next must be '-' or a digit */
      if (*next == '-') {
          minval = 1;    /* implied minrow value = 1 */
      } else if ( isdigit((int) *next) ) {
          minval = strtol(next, &next, 10);
      } else {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Syntax error in this row range list:");
          ffpmsg(rowlist);
          return(*status);
      }

      while (*next == ' ')next++;   /* skip spaces */

      /* find max value of next range; *next must be '-', or ',' */
      if (*next == '-') {
          next++;
          while (*next == ' ')next++;   /* skip spaces */

          if ( isdigit((int) *next) ) {
              maxval = strtol(next, &next, 10);
          } else if (*next == ',' || *next == '\0') {
              maxval = (long) maxrows;  /* implied max value */
          } else {
              *status = RANGE_PARSE_ERROR;
              ffpmsg("Syntax error in this row range list:");
              ffpmsg(rowlist);
              return(*status);
          }
      } else if (*next == ',' || *next == '\0') {
          maxval = minval;  /* only a single integer in this range */
      } else {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Syntax error in this row range list:");
          ffpmsg(rowlist);
          return(*status);
      }

      if (*numranges + 1 > maxranges) {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Overflowed maximum number of ranges (fits_parse_ranges)");
          return(*status);
      }

      if (minval < 1 ) {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Syntax error in this row range list: row number < 1");
          ffpmsg(rowlist);
          return(*status);
      }

      if (maxval < minval) {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Syntax error in this row range list: min > max");
          ffpmsg(rowlist);
          return(*status);
      }

      if (*numranges > 0) {
          if (minval <= maxrow[(*numranges) - 1]) {
             *status = RANGE_PARSE_ERROR;
             ffpmsg("Syntax error in this row range list.  Range minimum is");
             ffpmsg("  less than or equal to previous range maximum");
             ffpmsg(rowlist);
             return(*status);
         }
      }

      if (minval <= maxrows) {   /* ignore range if greater than maxrows */
          if (maxval > maxrows)
              maxval = (long) maxrows;

           minrow[*numranges] = minval;
           maxrow[*numranges] = maxval;

           (*numranges)++;
      }

      while (*next == ' ')next++;   /* skip spaces */
      if (*next == ',') {
           next++;
           while (*next == ' ')next++;   /* skip more spaces */
      }
    }

    if (*numranges == 0) {  /* a null string was entered */
         minrow[0] = 1;
         maxrow[0] = (long) maxrows;
         *numranges = 1;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffrwrgll(
      char *rowlist,      /* I - list of rows and row ranges */
      LONGLONG maxrows,       /* I - number of rows in the list */
      int maxranges,     /* I - max number of ranges to be returned */
      int *numranges,    /* O - number ranges returned */
      LONGLONG *minrow,       /* O - first row in each range */
      LONGLONG *maxrow,       /* O - last row in each range */
      int *status)        /* IO - status value */
{
/*
   parse the input list of row ranges, returning the number of ranges,
   and the min and max row value in each range. 

   The only characters allowed in the input rowlist are 
       decimal digits, minus sign, and comma (and non-significant spaces) 

   Example:  

     list = "10-20, 30-35,50"

   would return numranges = 3, minrow[] = {10, 30, 50}, maxrow[] = {20, 35, 50}

   error is returned if min value of range is > max value of range or if the
   ranges are not monotonically increasing.
*/
    char *next;
    LONGLONG minval, maxval;
    double dvalue;

    if (*status > 0)
        return(*status);

    if (maxrows <= 0 ) {
        *status = RANGE_PARSE_ERROR;
        ffpmsg("Input maximum range value is <= 0 (fits_parse_ranges)");
        return(*status);
    }

    next = rowlist;
    *numranges = 0;

    while (*next == ' ')next++;   /* skip spaces */
   
    while (*next != '\0') {

      /* find min value of next range; *next must be '-' or a digit */
      if (*next == '-') {
          minval = 1;    /* implied minrow value = 1 */
      } else if ( isdigit((int) *next) ) {

        /* read as a double, because the string to LONGLONG function */
        /* is platform dependent (strtoll, strtol, _atoI64)          */

          dvalue = strtod(next, &next);
          minval = (LONGLONG) (dvalue + 0.1);

      } else {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Syntax error in this row range list:");
          ffpmsg(rowlist);
          return(*status);
      }

      while (*next == ' ')next++;   /* skip spaces */

      /* find max value of next range; *next must be '-', or ',' */
      if (*next == '-') {
          next++;
          while (*next == ' ')next++;   /* skip spaces */

          if ( isdigit((int) *next) ) {

            /* read as a double, because the string to LONGLONG function */
            /* is platform dependent (strtoll, strtol, _atoI64)          */

              dvalue = strtod(next, &next);
              maxval = (LONGLONG) (dvalue + 0.1);

          } else if (*next == ',' || *next == '\0') {
              maxval = maxrows;  /* implied max value */
          } else {
              *status = RANGE_PARSE_ERROR;
              ffpmsg("Syntax error in this row range list:");
              ffpmsg(rowlist);
              return(*status);
          }
      } else if (*next == ',' || *next == '\0') {
          maxval = minval;  /* only a single integer in this range */
      } else {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Syntax error in this row range list:");
          ffpmsg(rowlist);
          return(*status);
      }

      if (*numranges + 1 > maxranges) {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Overflowed maximum number of ranges (fits_parse_ranges)");
          return(*status);
      }

      if (minval < 1 ) {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Syntax error in this row range list: row number < 1");
          ffpmsg(rowlist);
          return(*status);
      }

      if (maxval < minval) {
          *status = RANGE_PARSE_ERROR;
          ffpmsg("Syntax error in this row range list: min > max");
          ffpmsg(rowlist);
          return(*status);
      }

      if (*numranges > 0) {
          if (minval <= maxrow[(*numranges) - 1]) {
             *status = RANGE_PARSE_ERROR;
             ffpmsg("Syntax error in this row range list.  Range minimum is");
             ffpmsg("  less than or equal to previous range maximum");
             ffpmsg(rowlist);
             return(*status);
         }
      }

      if (minval <= maxrows) {   /* ignore range if greater than maxrows */
          if (maxval > maxrows)
              maxval = maxrows;

           minrow[*numranges] = minval;
           maxrow[*numranges] = maxval;

           (*numranges)++;
      }

      while (*next == ' ')next++;   /* skip spaces */
      if (*next == ',') {
           next++;
           while (*next == ' ')next++;   /* skip more spaces */
      }
    }

    if (*numranges == 0) {  /* a null string was entered */
         minrow[0] = 1;
         maxrow[0] = maxrows;
         *numranges = 1;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fficol(fitsfile *fptr,  /* I - FITS file pointer                        */
           int numcol,      /* I - position for new col. (1 = 1st)          */
           char *ttype,     /* I - name of column (TTYPE keyword)           */
           char *tform,     /* I - format of column (TFORM keyword)         */
           int *status)     /* IO - error status                            */
/*
 Insert a new column into an existing table at position numcol.  If
 numcol is greater than the number of existing columns in the table
 then the new column will be appended as the last column in the table.
*/
{
    char *name, *format;

    name = ttype;
    format = tform;

    fficls(fptr, numcol, 1, &name, &format, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fficls(fitsfile *fptr,  /* I - FITS file pointer                        */
           int fstcol,      /* I - position for first new col. (1 = 1st)    */
           int ncols,       /* I - number of columns to insert              */
           char **ttype,    /* I - array of column names(TTYPE keywords)    */
           char **tform,    /* I - array of formats of column (TFORM)       */
           int *status)     /* IO - error status                            */
/*
 Insert 1 or more new columns into an existing table at position numcol.  If
 fstcol is greater than the number of existing columns in the table
 then the new column will be appended as the last column in the table.
*/
{
    int colnum, datacode, decims, tfields, tstatus, ii;
    LONGLONG datasize, firstbyte, nbytes, nadd, naxis1, naxis2, freespace;
    LONGLONG tbcol, firstcol, delbyte;
    long nblock, width, repeat;
    char tfm[FLEN_VALUE], keyname[FLEN_KEYWORD], comm[FLEN_COMMENT], *cptr;
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
        /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
       ffpmsg("Can only add columns to TABLE or BINTABLE extension (fficol)");
       return(*status = NOT_TABLE);
    }

    /*  is the column number valid?  */
    tfields = (fptr->Fptr)->tfield;
    if (fstcol < 1 )
        return(*status = BAD_COL_NUM);
    else if (fstcol > tfields)
        colnum = tfields + 1;   /* append as last column */
    else
        colnum = fstcol;

    /* parse the tform value and calc number of bytes to add to each row */
    delbyte = 0;
    for (ii = 0; ii < ncols; ii++)
    {
        strcpy(tfm, tform[ii]);
        ffupch(tfm);         /* make sure format is in upper case */

        if ((fptr->Fptr)->hdutype == ASCII_TBL)
        {
            ffasfm(tfm, &datacode, &width, &decims, status);
            delbyte += width + 1;  /*  add one space between the columns */
        }
        else
        {
            ffbnfm(tfm, &datacode, &repeat, &width, status);

            if (datacode < 0)  {       /* variable length array column */
	        if (strchr(tfm, 'Q'))
		  delbyte += 16;
		else
                  delbyte += 8;
            } else if (datacode == 1)          /* bit column; round up  */
                delbyte += (repeat + 7) / 8; /* to multiple of 8 bits */
            else if (datacode == 16)  /* ASCII string column */
                delbyte += repeat;
            else                      /* numerical data type */
                delbyte += (datacode / 10) * repeat;
        }
    }

    if (*status > 0) 
        return(*status);

    /* get the current size of the table */
    /* use internal structure since NAXIS2 keyword may not be up to date */
    naxis1 = (fptr->Fptr)->rowlength;
    naxis2 = (fptr->Fptr)->numrows;

    /* current size of data */
    datasize = (fptr->Fptr)->heapstart + (fptr->Fptr)->heapsize;
    freespace = ( ( (datasize + 2879) / 2880) * 2880) - datasize;
    nadd = delbyte * naxis2;   /* no. of bytes to add to table */

    if ( (freespace - nadd) < 0)   /* not enough existing space? */
    {
        nblock = (long) ((nadd - freespace + 2879) / 2880);     /* number of blocks  */
        if (ffiblk(fptr, nblock, 1, status) > 0)       /* insert the blocks */
            return(*status);
    }

    /* shift heap down (if it exists) */
    if ((fptr->Fptr)->heapsize > 0)
    {
        nbytes = (fptr->Fptr)->heapsize;    /* no. of bytes to shift down */

        /* absolute heap pos */
        firstbyte = (fptr->Fptr)->datastart + (fptr->Fptr)->heapstart;

        if (ffshft(fptr, firstbyte, nbytes, nadd, status) > 0) /* move heap */
            return(*status);
    }

    /* update the heap starting address */
    (fptr->Fptr)->heapstart += nadd;

    /* update the THEAP keyword if it exists */
    tstatus = 0;
    ffmkyj(fptr, "THEAP", (fptr->Fptr)->heapstart, "&", &tstatus);

    /* calculate byte position in the row where to insert the new column */
    if (colnum > tfields)
        firstcol = naxis1;
    else
    {
        colptr = (fptr->Fptr)->tableptr;
        colptr += (colnum - 1);
        firstcol = colptr->tbcol;
    }

    /* insert delbyte bytes in every row, at byte position firstcol */
    ffcins(fptr, naxis1, naxis2, delbyte, firstcol, status);

    if ((fptr->Fptr)->hdutype == ASCII_TBL)
    {
        /* adjust the TBCOL values of the existing columns */
        for(ii = 0; ii < tfields; ii++)
        {
            ffkeyn("TBCOL", ii + 1, keyname, status);
            ffgkyjj(fptr, keyname, &tbcol, comm, status);
            if (tbcol > firstcol)
            {
                tbcol += delbyte;
                ffmkyj(fptr, keyname, tbcol, "&", status);
            }
        }
    }

    /* update the mandatory keywords */
    ffmkyj(fptr, "TFIELDS", tfields + ncols, "&", status);
    ffmkyj(fptr, "NAXIS1", naxis1 + delbyte, "&", status);

    /* increment the index value on any existing column keywords */
    if(colnum <= tfields)
        ffkshf(fptr, colnum, tfields, ncols, status);

    /* add the required keywords for the new columns */
    for (ii = 0; ii < ncols; ii++, colnum++)
    {
        strcpy(comm, "label for field");
        ffkeyn("TTYPE", colnum, keyname, status);
        ffpkys(fptr, keyname, ttype[ii], comm, status);

        strcpy(comm, "format of field");
        strcpy(tfm, tform[ii]);
        ffupch(tfm);         /* make sure format is in upper case */
        ffkeyn("TFORM", colnum, keyname, status);

        if (abs(datacode) == TSBYTE) 
        {
           /* Replace the 'S' with an 'B' in the TFORMn code */
           cptr = tfm;
           while (*cptr != 'S') 
              cptr++;

           *cptr = 'B';
           ffpkys(fptr, keyname, tfm, comm, status);

           /* write the TZEROn and TSCALn keywords */
           ffkeyn("TZERO", colnum, keyname, status);
           strcpy(comm, "offset for signed bytes");

           ffpkyg(fptr, keyname, -128., 0, comm, status);

           ffkeyn("TSCAL", colnum, keyname, status);
           strcpy(comm, "data are not scaled");
           ffpkyg(fptr, keyname, 1., 0, comm, status);
        }
        else if (abs(datacode) == TUSHORT) 
        {
           /* Replace the 'U' with an 'I' in the TFORMn code */
           cptr = tfm;
           while (*cptr != 'U') 
              cptr++;

           *cptr = 'I';
           ffpkys(fptr, keyname, tfm, comm, status);

           /* write the TZEROn and TSCALn keywords */
           ffkeyn("TZERO", colnum, keyname, status);
           strcpy(comm, "offset for unsigned integers");

           ffpkyg(fptr, keyname, 32768., 0, comm, status);

           ffkeyn("TSCAL", colnum, keyname, status);
           strcpy(comm, "data are not scaled");
           ffpkyg(fptr, keyname, 1., 0, comm, status);
        }
        else if (abs(datacode) == TULONG) 
        {
           /* Replace the 'V' with an 'J' in the TFORMn code */
           cptr = tfm;
           while (*cptr != 'V') 
              cptr++;

           *cptr = 'J';
           ffpkys(fptr, keyname, tfm, comm, status);

           /* write the TZEROn and TSCALn keywords */
           ffkeyn("TZERO", colnum, keyname, status);
           strcpy(comm, "offset for unsigned integers");

           ffpkyg(fptr, keyname, 2147483648., 0, comm, status);

           ffkeyn("TSCAL", colnum, keyname, status);
           strcpy(comm, "data are not scaled");
           ffpkyg(fptr, keyname, 1., 0, comm, status);
        }
        else
        {
           ffpkys(fptr, keyname, tfm, comm, status);
        }

        if ((fptr->Fptr)->hdutype == ASCII_TBL)   /* write the TBCOL keyword */
        {
            if (colnum == tfields + 1)
                tbcol = firstcol + 2;  /* allow space between preceding col */
            else
                tbcol = firstcol + 1;

            strcpy(comm, "beginning column of field");
            ffkeyn("TBCOL", colnum, keyname, status);
            ffpkyj(fptr, keyname, tbcol, comm, status);

            /* increment the column starting position for the next column */
            ffasfm(tfm, &datacode, &width, &decims, status);
            firstcol += width + 1;  /*  add one space between the columns */
        }
    }
    ffrdef(fptr, status); /* initialize the new table structure */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmvec(fitsfile *fptr,  /* I - FITS file pointer                        */
           int colnum,      /* I - position of col to be modified           */
           LONGLONG newveclen,  /* I - new vector length of column (TFORM)       */
           int *status)     /* IO - error status                            */
/*
  Modify the vector length of a column in a binary table, larger or smaller.
  E.g., change a column from TFORMn = '1E' to '20E'.
*/
{
    int datacode, tfields, tstatus;
    LONGLONG datasize, size, firstbyte, nbytes, nadd, ndelete;
    LONGLONG naxis1, naxis2, firstcol, freespace;
    LONGLONG width, delbyte, repeat;
    long nblock;
    char tfm[FLEN_VALUE], keyname[FLEN_KEYWORD], tcode[2];
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
        /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype != BINARY_TBL)
    {
       ffpmsg(
  "Can only change vector length of a column in BINTABLE extension (ffmvec)");
       return(*status = NOT_TABLE);
    }

    /*  is the column number valid?  */
    tfields = (fptr->Fptr)->tfield;
    if (colnum < 1 || colnum > tfields)
        return(*status = BAD_COL_NUM);

    /* look up the current vector length and element width */

    colptr = (fptr->Fptr)->tableptr;
    colptr += (colnum - 1);

    datacode = colptr->tdatatype; /* datatype of the column */
    repeat =  colptr->trepeat;  /* field repeat count  */
    width =  colptr->twidth;   /*  width of a single element in chars */

    if (datacode < 0)
    {
        ffpmsg(
        "Can't modify vector length of variable length column (ffmvec)");
        return(*status = BAD_TFORM);
    }

    if (repeat == newveclen)
        return(*status);  /* column already has the desired vector length */

    if (datacode == TSTRING)
        width = 1;      /* width was equal to width of unit string */

    naxis1 =  (fptr->Fptr)->rowlength;   /* current width of the table */
    naxis2 = (fptr->Fptr)->numrows;

    delbyte = (newveclen - repeat) * width;    /* no. of bytes to insert */
    if (datacode == TBIT)  /* BIT column is a special case */
       delbyte = ((newveclen + 7) / 8) - ((repeat + 7) / 8);

    if (delbyte > 0)  /* insert space for more elements */
    {
      /* current size of data */
      datasize = (fptr->Fptr)->heapstart + (fptr->Fptr)->heapsize;
      freespace = ( ( (datasize + 2879) / 2880) * 2880) - datasize;

      nadd = (LONGLONG)delbyte * naxis2;   /* no. of bytes to add to table */

      if ( (freespace - nadd) < 0)   /* not enough existing space? */
      {
        nblock = (long) ((nadd - freespace + 2879) / 2880);    /* number of blocks  */
        if (ffiblk(fptr, nblock, 1, status) > 0)      /* insert the blocks */
          return(*status);
      }

      /* shift heap down (if it exists) */
      if ((fptr->Fptr)->heapsize > 0)
      {
        nbytes = (fptr->Fptr)->heapsize;    /* no. of bytes to shift down */

        /* absolute heap pos */
        firstbyte = (fptr->Fptr)->datastart + (fptr->Fptr)->heapstart;

        if (ffshft(fptr, firstbyte, nbytes, nadd, status) > 0) /* move heap */
            return(*status);
      }

      /* update the heap starting address */
      (fptr->Fptr)->heapstart += nadd;

      /* update the THEAP keyword if it exists */
      tstatus = 0;
      ffmkyj(fptr, "THEAP", (fptr->Fptr)->heapstart, "&", &tstatus);

      firstcol = colptr->tbcol + (repeat * width);  /* insert position */

      /* insert delbyte bytes in every row, at byte position firstcol */
      ffcins(fptr, naxis1, naxis2, delbyte, firstcol, status);
    }
    else if (delbyte < 0)
    {
      /* current size of table */
      size = (fptr->Fptr)->heapstart + (fptr->Fptr)->heapsize;
      freespace = ((size + 2879) / 2880) * 2880 - size - ((LONGLONG)delbyte * naxis2);
      nblock = (long) (freespace / 2880);   /* number of empty blocks to delete */
      firstcol = colptr->tbcol + (newveclen * width);  /* delete position */

      /* delete elements from the vector */
      ffcdel(fptr, naxis1, naxis2, -delbyte, firstcol, status);
 
      /* abs heap pos */
      firstbyte = (fptr->Fptr)->datastart + (fptr->Fptr)->heapstart;
      ndelete = (LONGLONG)delbyte * naxis2; /* size of shift (negative) */

      /* shift heap up (if it exists) */
      if ((fptr->Fptr)->heapsize > 0)
      {
        nbytes = (fptr->Fptr)->heapsize;    /* no. of bytes to shift up */
        if (ffshft(fptr, firstbyte, nbytes, ndelete, status) > 0)
          return(*status);
      }

      /* delete the empty  blocks at the end of the HDU */
      if (nblock > 0)
        ffdblk(fptr, nblock, status);

      /* update the heap starting address */
      (fptr->Fptr)->heapstart += ndelete;  /* ndelete is negative */

      /* update the THEAP keyword if it exists */
      tstatus = 0;
      ffmkyj(fptr, "THEAP", (fptr->Fptr)->heapstart, "&", &tstatus);
    }

    /* construct the new TFORM keyword for the column */
    if (datacode == TBIT)
      strcpy(tcode,"X");
    else if (datacode == TBYTE)
      strcpy(tcode,"B");
    else if (datacode == TLOGICAL)
      strcpy(tcode,"L");
    else if (datacode == TSTRING)
      strcpy(tcode,"A");
    else if (datacode == TSHORT)
      strcpy(tcode,"I");
    else if (datacode == TLONG)
      strcpy(tcode,"J");
    else if (datacode == TLONGLONG)
      strcpy(tcode,"K");
    else if (datacode == TFLOAT)
      strcpy(tcode,"E");
    else if (datacode == TDOUBLE)
      strcpy(tcode,"D");
    else if (datacode == TCOMPLEX)
      strcpy(tcode,"C");
    else if (datacode == TDBLCOMPLEX)
      strcpy(tcode,"M");

    /* write as a double value because the LONGLONG conversion */
    /* character in sprintf is platform dependent ( %lld, %ld, %I64d ) */

    sprintf(tfm,"%.0f%s",(double) newveclen, tcode); 

    ffkeyn("TFORM", colnum, keyname, status);  /* Keyword name */
    ffmkys(fptr, keyname, tfm, "&", status);   /* modify TFORM keyword */

    ffmkyj(fptr, "NAXIS1", naxis1 + delbyte, "&", status); /* modify NAXIS1 */

    ffrdef(fptr, status); /* reinitialize the new table structure */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcpcl(fitsfile *infptr,    /* I - FITS file pointer to input file  */
           fitsfile *outfptr,   /* I - FITS file pointer to output file */
           int incol,           /* I - number of input column   */
           int outcol,          /* I - number for output column  */
           int create_col,      /* I - create new col if TRUE, else overwrite */
           int *status)         /* IO - error status     */
/*
  copy a column from infptr and insert it in the outfptr table.
*/
{
    int tstatus, colnum, typecode, otypecode, anynull;
    int inHduType, outHduType;
    long tfields, repeat, orepeat, width, owidth, nrows, outrows;
    long inloop, outloop, maxloop, ndone, ntodo, npixels;
    long firstrow, firstelem, ii;
    char keyname[FLEN_KEYWORD], ttype[FLEN_VALUE], tform[FLEN_VALUE];
    char ttype_comm[FLEN_COMMENT],tform_comm[FLEN_COMMENT];
    char *lvalues = 0, nullflag, **strarray = 0;
    char nulstr[] = {'\5', '\0'};  /* unique null string value */
    double dnull = 0.l, *dvalues = 0;
    float fnull = 0., *fvalues = 0;

    if (*status > 0)
        return(*status);

    if (infptr->HDUposition != (infptr->Fptr)->curhdu)
    {
        ffmahd(infptr, (infptr->HDUposition) + 1, NULL, status);
    }
    else if ((infptr->Fptr)->datastart == DATA_UNDEFINED)
        ffrdef(infptr, status);                /* rescan header */
    inHduType = (infptr->Fptr)->hdutype;
    
    if (outfptr->HDUposition != (outfptr->Fptr)->curhdu)
    {
        ffmahd(outfptr, (outfptr->HDUposition) + 1, NULL, status);
    }
    else if ((outfptr->Fptr)->datastart == DATA_UNDEFINED)
        ffrdef(outfptr, status);               /* rescan header */
    outHduType = (outfptr->Fptr)->hdutype;
    
    if (*status > 0)
        return(*status);

    if (inHduType == IMAGE_HDU || outHduType == IMAGE_HDU)
    {
       ffpmsg
       ("Can not copy columns to or from IMAGE HDUs (ffcpcl)");
       return(*status = NOT_TABLE);
    }

    if ( inHduType == BINARY_TBL &&  outHduType == ASCII_TBL)
    {
       ffpmsg
       ("Copying from Binary table to ASCII table is not supported (ffcpcl)");
       return(*status = NOT_BTABLE);
    }

    /* get the datatype and vector repeat length of the column */
    ffgtcl(infptr, incol, &typecode, &repeat, &width, status);

    if (typecode < 0)
    {
        ffpmsg("Variable-length columns are not supported (ffcpcl)");
        return(*status = BAD_TFORM);
    }

    if (create_col)    /* insert new column in output table? */
    {
        tstatus = 0;
        ffkeyn("TTYPE", incol, keyname, &tstatus);
        ffgkys(infptr, keyname, ttype, ttype_comm, &tstatus);
        ffkeyn("TFORM", incol, keyname, &tstatus);
    
        if (ffgkys(infptr, keyname, tform, tform_comm, &tstatus) )
        {
          ffpmsg
          ("Could not find TTYPE and TFORM keywords in input table (ffcpcl)");
          return(*status = NO_TFORM);
        }

        if (inHduType == ASCII_TBL && outHduType == BINARY_TBL)
        {
            /* convert from ASCII table to BINARY table format string */
            if (typecode == TSTRING)
                ffnkey(width, "A", tform, status);

            else if (typecode == TLONG)
                strcpy(tform, "1J");

            else if (typecode == TSHORT)
                strcpy(tform, "1I");

            else if (typecode == TFLOAT)
                strcpy(tform,"1E");

            else if (typecode == TDOUBLE)
                strcpy(tform,"1D");
        }

        if (ffgkyj(outfptr, "TFIELDS", &tfields, 0, &tstatus))
        {
           ffpmsg
           ("Could not read TFIELDS keyword in output table (ffcpcl)");
           return(*status = NO_TFIELDS);
        }

        colnum = minvalue((int) tfields + 1, outcol); /* output col. number */

        /* create the empty column */
        if (fficol(outfptr, colnum, ttype, tform, status) > 0)
        {
           ffpmsg
           ("Could not append new column to output file (ffcpcl)");
           return(*status);
        }

        if ((infptr->Fptr == outfptr->Fptr)
           && (infptr->HDUposition == outfptr->HDUposition)
           && (colnum <= incol))  {
	       incol++;  /* the input column has been shifted over */
        }

        /* copy the comment strings from the input file for TTYPE and TFORM */
        tstatus = 0;
        ffkeyn("TTYPE", colnum, keyname, &tstatus);
        ffmcom(outfptr, keyname, ttype_comm, &tstatus);
        ffkeyn("TFORM", colnum, keyname, &tstatus);
        ffmcom(outfptr, keyname, tform_comm, &tstatus);

        /* copy other column-related keywords if they exist */

        ffcpky(infptr, outfptr, incol, colnum, "TUNIT", status);
        ffcpky(infptr, outfptr, incol, colnum, "TSCAL", status);
        ffcpky(infptr, outfptr, incol, colnum, "TZERO", status);
        ffcpky(infptr, outfptr, incol, colnum, "TDISP", status);
        ffcpky(infptr, outfptr, incol, colnum, "TLMIN", status);
        ffcpky(infptr, outfptr, incol, colnum, "TLMAX", status);
        ffcpky(infptr, outfptr, incol, colnum, "TDIM", status);

        /*  WCS keywords */
        ffcpky(infptr, outfptr, incol, colnum, "TCTYP", status);
        ffcpky(infptr, outfptr, incol, colnum, "TCUNI", status);
        ffcpky(infptr, outfptr, incol, colnum, "TCRVL", status);
        ffcpky(infptr, outfptr, incol, colnum, "TCRPX", status);
        ffcpky(infptr, outfptr, incol, colnum, "TCDLT", status);
        ffcpky(infptr, outfptr, incol, colnum, "TCROT", status);

        if (inHduType == ASCII_TBL && outHduType == BINARY_TBL)
        {
            /* binary tables only have TNULLn keyword for integer columns */
            if (typecode == TLONG || typecode == TSHORT)
            {
                /* check if null string is defined; replace with integer */
                ffkeyn("TNULL", incol, keyname, &tstatus);
                if (ffgkys(infptr, keyname, ttype, 0, &tstatus) <= 0)
                {
                   ffkeyn("TNULL", colnum, keyname, &tstatus);
                   if (typecode == TLONG)
                      ffpkyj(outfptr, keyname, -9999999L, "Null value", status);
                   else
                      ffpkyj(outfptr, keyname, -32768L, "Null value", status);
                }
            }
        }
        else
        {
            ffcpky(infptr, outfptr, incol, colnum, "TNULL", status);
        }

        /* rescan header to recognize the new keywords */
        if (ffrdef(outfptr, status) )
            return(*status);
    }
    else
    {
        colnum = outcol;
        /* get the datatype and vector repeat length of the output column */
        ffgtcl(outfptr, outcol, &otypecode, &orepeat, &owidth, status);

        if (orepeat != repeat) {
            ffpmsg("Input and output vector columns must have same length (ffcpcl)");
            return(*status = BAD_TFORM);
        }
    }

    ffgkyj(infptr,  "NAXIS2", &nrows,   0, status);  /* no. of input rows */
    ffgkyj(outfptr, "NAXIS2", &outrows, 0, status);  /* no. of output rows */
    nrows = minvalue(nrows, outrows);

    if (typecode == TBIT)
        repeat = (repeat + 7) / 8;  /* convert from bits to bytes */
    else if (typecode == TSTRING && inHduType == BINARY_TBL)
        repeat = repeat / width;  /* convert from chars to unit strings */

    /* get optimum number of rows to copy at one time */
    ffgrsz(infptr,  &inloop,  status);
    ffgrsz(outfptr, &outloop, status);

    /* adjust optimum number, since 2 tables are open at once */
    maxloop = minvalue(inloop, outloop); /* smallest of the 2 tables */
    maxloop = maxvalue(1, maxloop / 2);  /* at least 1 row */
    maxloop = minvalue(maxloop, nrows);  /* max = nrows to be copied */
    maxloop *= repeat;                   /* mult by no of elements in a row */

    /* allocate memory for arrays */
    if (typecode == TLOGICAL)
    {
       lvalues   = (char *) calloc(maxloop, sizeof(char) );
       if (!lvalues)
       {
         ffpmsg
         ("malloc failed to get memory for logicals (ffcpcl)");
         return(*status = ARRAY_TOO_BIG);
       }
    }
    else if (typecode == TSTRING)
    {
       /* allocate array of pointers */
       strarray = (char **) calloc(maxloop, sizeof(strarray));

       /* allocate space for each string */
       for (ii = 0; ii < maxloop; ii++)
          strarray[ii] = (char *) calloc(width+1, sizeof(char));
    }
    else if (typecode == TCOMPLEX)
    {
       fvalues = (float *) calloc(maxloop * 2, sizeof(float) );
       if (!fvalues)
       {
         ffpmsg
         ("malloc failed to get memory for complex (ffcpcl)");
         return(*status = ARRAY_TOO_BIG);
       }
       fnull = 0.;
    }
    else if (typecode == TDBLCOMPLEX)
    {
       dvalues = (double *) calloc(maxloop * 2, sizeof(double) );
       if (!dvalues)
       {
         ffpmsg
         ("malloc failed to get memory for dbl complex (ffcpcl)");
         return(*status = ARRAY_TOO_BIG);
       }
       dnull = 0.;
    }
    else    /* numerical datatype; read them all as doubles */
    {
       dvalues = (double *) calloc(maxloop, sizeof(double) );
       if (!dvalues)
       {
         ffpmsg
         ("malloc failed to get memory for doubles (ffcpcl)");
         return(*status = ARRAY_TOO_BIG);
       }
         dnull = -9.99991999E31;  /* use an unlikely value for nulls */
    }

    npixels = nrows * repeat;          /* total no. of pixels to copy */
    ntodo = minvalue(npixels, maxloop);   /* no. to copy per iteration */
    ndone = 0;             /* total no. of pixels that have been copied */

    while (ntodo)      /* iterate through the table */
    {
        firstrow = ndone / repeat + 1;
        firstelem = ndone - ((firstrow - 1) * repeat) + 1;

        /* read from input table */
        if (typecode == TLOGICAL)
            ffgcl(infptr, incol, firstrow, firstelem, ntodo, 
                       lvalues, status);
        else if (typecode == TSTRING)
            ffgcvs(infptr, incol, firstrow, firstelem, ntodo,
                       nulstr, strarray, &anynull, status);

        else if (typecode == TCOMPLEX)  
            ffgcvc(infptr, incol, firstrow, firstelem, ntodo, fnull, 
                   fvalues, &anynull, status);

        else if (typecode == TDBLCOMPLEX)
            ffgcvm(infptr, incol, firstrow, firstelem, ntodo, dnull, 
                   dvalues, &anynull, status);

        else       /* all numerical types */
            ffgcvd(infptr, incol, firstrow, firstelem, ntodo, dnull, 
                   dvalues, &anynull, status);

        if (*status > 0)
        {
            ffpmsg("Error reading input copy of column (ffcpcl)");
            break;
        }

        /* write to output table */
        if (typecode == TLOGICAL)
        {
            nullflag = 2;

            ffpcnl(outfptr, colnum, firstrow, firstelem, ntodo, 
                       lvalues, nullflag, status);

        }

        else if (typecode == TSTRING)
        {
            if (anynull)
                ffpcns(outfptr, colnum, firstrow, firstelem, ntodo,
                       strarray, nulstr, status);
            else
                ffpcls(outfptr, colnum, firstrow, firstelem, ntodo,
                       strarray, status);
        }

        else if (typecode == TCOMPLEX)  
        {                      /* doesn't support writing nulls */
            ffpclc(outfptr, colnum, firstrow, firstelem, ntodo, 
                       fvalues, status);
        }

        else if (typecode == TDBLCOMPLEX)  
        {                      /* doesn't support writing nulls */
            ffpclm(outfptr, colnum, firstrow, firstelem, ntodo, 
                       dvalues, status);
        }

        else  /* all other numerical types */
        {
            if (anynull)
                ffpcnd(outfptr, colnum, firstrow, firstelem, ntodo, 
                       dvalues, dnull, status);
            else
                ffpcld(outfptr, colnum, firstrow, firstelem, ntodo, 
                       dvalues, status);
        }

        if (*status > 0)
        {
            ffpmsg("Error writing output copy of column (ffcpcl)");
            break;
        }

        npixels -= ntodo;
        ndone += ntodo;
        ntodo = minvalue(npixels, maxloop);
    }

    /* free the previously allocated memory */
    if (typecode == TLOGICAL)
    {
        free(lvalues);
    }
    else if (typecode == TSTRING)
    {
         for (ii = 0; ii < maxloop; ii++)
             free(strarray[ii]);

         free(strarray);
    }
    else
    {
        free(dvalues);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcprw(fitsfile *infptr,    /* I - FITS file pointer to input file  */
           fitsfile *outfptr,   /* I - FITS file pointer to output file */
           LONGLONG firstrow,   /* I - number of first row to copy (1 based)  */
           LONGLONG nrows,      /* I - number of rows to copy  */
           int *status)         /* IO - error status     */
/*
  copy consecutive set of rows from infptr and append it in the outfptr table.
*/
{
    LONGLONG innaxis1, innaxis2, outnaxis1, outnaxis2, ii, jj;
    unsigned char *buffer;

    if (*status > 0)
        return(*status);

    if (infptr->HDUposition != (infptr->Fptr)->curhdu)
    {
        ffmahd(infptr, (infptr->HDUposition) + 1, NULL, status);
    }
    else if ((infptr->Fptr)->datastart == DATA_UNDEFINED)
        ffrdef(infptr, status);                /* rescan header */

    if (outfptr->HDUposition != (outfptr->Fptr)->curhdu)
    {
        ffmahd(outfptr, (outfptr->HDUposition) + 1, NULL, status);
    }
    else if ((outfptr->Fptr)->datastart == DATA_UNDEFINED)
        ffrdef(outfptr, status);               /* rescan header */

    if (*status > 0)
        return(*status);

    if ((infptr->Fptr)->hdutype == IMAGE_HDU || (outfptr->Fptr)->hdutype == IMAGE_HDU)
    {
       ffpmsg
       ("Can not copy rows to or from IMAGE HDUs (ffcprw)");
       return(*status = NOT_TABLE);
    }

    if ( ((infptr->Fptr)->hdutype == BINARY_TBL &&  (outfptr->Fptr)->hdutype == ASCII_TBL) ||
         ((infptr->Fptr)->hdutype == ASCII_TBL &&  (outfptr->Fptr)->hdutype == BINARY_TBL) )
    {
       ffpmsg
       ("Copying rows between Binary and ASCII tables is not supported (ffcprw)");
       return(*status = NOT_BTABLE);
    }

    ffgkyjj(infptr,  "NAXIS1", &innaxis1,  0, status);  /* width of input rows */
    ffgkyjj(infptr,  "NAXIS2", &innaxis2,  0, status);  /* no. of input rows */
    ffgkyjj(outfptr, "NAXIS1", &outnaxis1, 0, status);  /* width of output rows */
    ffgkyjj(outfptr, "NAXIS2", &outnaxis2, 0, status);  /* no. of output rows */

    if (*status > 0)
        return(*status);

    if (outnaxis1 > innaxis1) {
       ffpmsg
       ("Input and output tables do not have same width (ffcprw)");
       return(*status = BAD_ROW_WIDTH);
    }    

    if (firstrow + nrows - 1 > innaxis2) {
       ffpmsg
       ("Not enough rows in input table to copy (ffcprw)");
       return(*status = BAD_ROW_NUM);
    }

    /* allocate buffer to hold 1 row of data */
    buffer = malloc( (size_t) innaxis1);
    if (!buffer) {
       ffpmsg
       ("Unable to allocate memory (ffcprw)");
       return(*status = MEMORY_ALLOCATION);
    }
 
    /* copy the rows, 1 at a time */
    jj = outnaxis2 + 1;
    for (ii = firstrow; ii < firstrow + nrows; ii++) {
        fits_read_tblbytes (infptr,  ii, 1, innaxis1, buffer, status);
        fits_write_tblbytes(outfptr, jj, 1, innaxis1, buffer, status);
        jj++;
    }

    outnaxis2 += nrows;
    fits_update_key(outfptr, TLONGLONG, "NAXIS2", &outnaxis2, 0, status);

    free(buffer);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcpky(fitsfile *infptr,    /* I - FITS file pointer to input file  */
           fitsfile *outfptr,   /* I - FITS file pointer to output file */
           int incol,           /* I - input index number   */
           int outcol,          /* I - output index number  */
           char *rootname,      /* I - root name of the keyword to be copied */
           int *status)         /* IO - error status     */
/*
  copy an indexed keyword from infptr to outfptr.
*/
{
    int tstatus = 0;
    char keyname[FLEN_KEYWORD];
    char value[FLEN_VALUE], comment[FLEN_COMMENT], card[FLEN_CARD];

    ffkeyn(rootname, incol, keyname, &tstatus);
    if (ffgkey(infptr, keyname, value, comment, &tstatus) <= 0)
    {
        ffkeyn(rootname, outcol, keyname, &tstatus);
        ffmkky(keyname, value, comment, card, status);
        ffprec(outfptr, card, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdcol(fitsfile *fptr,  /* I - FITS file pointer                        */
           int colnum,      /* I - column to delete (1 = 1st)               */
           int *status)     /* IO - error status                            */
/*
  Delete a column from a table.
*/
{
    int ii, tstatus;
    LONGLONG firstbyte, size, ndelete, nbytes, naxis1, naxis2, firstcol, delbyte, freespace;
    LONGLONG tbcol;
    long nblock, nspace;
    char keyname[FLEN_KEYWORD], comm[FLEN_COMMENT];
    tcolumn *colptr, *nextcol;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
    /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
       ffpmsg
       ("Can only delete column from TABLE or BINTABLE extension (ffdcol)");
       return(*status = NOT_TABLE);
    }

    if (colnum < 1 || colnum > (fptr->Fptr)->tfield )
        return(*status = BAD_COL_NUM);

    colptr = (fptr->Fptr)->tableptr;
    colptr += (colnum - 1);
    firstcol = colptr->tbcol;  /* starting byte position of the column */

    /* use column width to determine how many bytes to delete in each row */
    if ((fptr->Fptr)->hdutype == ASCII_TBL)
    {
      delbyte = colptr->twidth;  /* width of ASCII column */

      if (colnum < (fptr->Fptr)->tfield) /* check for space between next column */
      {
        nextcol = colptr + 1;
        nspace = (long) ((nextcol->tbcol) - (colptr->tbcol) - delbyte);
        if (nspace > 0)
            delbyte++;
      }
      else if (colnum > 1)   /* check for space between last 2 columns */
      {
        nextcol = colptr - 1;
        nspace = (long) ((colptr->tbcol) - (nextcol->tbcol) - (nextcol->twidth));
        if (nspace > 0)
        {
           delbyte++;
           firstcol--;  /* delete the leading space */
        }
      }
    }
    else   /* a binary table */
    {
      if (colnum < (fptr->Fptr)->tfield)
      {
         nextcol = colptr + 1;
         delbyte = (nextcol->tbcol) - (colptr->tbcol);
      }
      else
      {
         delbyte = ((fptr->Fptr)->rowlength) - (colptr->tbcol);
      }
    }

    naxis1 = (fptr->Fptr)->rowlength;   /* current width of the table */
    naxis2 = (fptr->Fptr)->numrows;

    /* current size of table */
    size = (fptr->Fptr)->heapstart + (fptr->Fptr)->heapsize;
    freespace = ((LONGLONG)delbyte * naxis2) + ((size + 2879) / 2880) * 2880 - size;
    nblock = (long) (freespace / 2880);   /* number of empty blocks to delete */

    ffcdel(fptr, naxis1, naxis2, delbyte, firstcol, status); /* delete col */

    /* absolute heap position */
    firstbyte = (fptr->Fptr)->datastart + (fptr->Fptr)->heapstart;
    ndelete = (LONGLONG)delbyte * naxis2; /* size of shift */

    /* shift heap up (if it exists) */
    if ((fptr->Fptr)->heapsize > 0)
    {
      nbytes = (fptr->Fptr)->heapsize;    /* no. of bytes to shift up */

      if (ffshft(fptr, firstbyte, nbytes, -ndelete, status) > 0) /* mv heap */
          return(*status);
    }

    /* delete the empty  blocks at the end of the HDU */
    if (nblock > 0)
        ffdblk(fptr, nblock, status);

    /* update the heap starting address */
    (fptr->Fptr)->heapstart -= ndelete;

    /* update the THEAP keyword if it exists */
    tstatus = 0;
    ffmkyj(fptr, "THEAP", (long)(fptr->Fptr)->heapstart, "&", &tstatus);

    if ((fptr->Fptr)->hdutype == ASCII_TBL)
    {
      /* adjust the TBCOL values of the remaining columns */
      for (ii = 1; ii <= (fptr->Fptr)->tfield; ii++)
      {
        ffkeyn("TBCOL", ii, keyname, status);
        ffgkyjj(fptr, keyname, &tbcol, comm, status);
        if (tbcol > firstcol)
        {
          tbcol = tbcol - delbyte;
          ffmkyj(fptr, keyname, tbcol, "&", status);
        }
      }
    }

    /* update the mandatory keywords */
    ffmkyj(fptr, "TFIELDS", ((fptr->Fptr)->tfield) - 1, "&", status);        
    ffmkyj(fptr,  "NAXIS1",   naxis1 - delbyte, "&", status);
    /*
      delete the index keywords starting with 'T' associated with the 
      deleted column and subtract 1 from index of all higher keywords
    */
    ffkshf(fptr, colnum, (fptr->Fptr)->tfield, -1, status);

    ffrdef(fptr, status);  /* initialize the new table structure */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcins(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG naxis1,     /* I - width of the table, in bytes             */
           LONGLONG naxis2,     /* I - number of rows in the table              */
           LONGLONG ninsert,    /* I - number of bytes to insert in each row    */
           LONGLONG bytepos,    /* I - rel. position in row to insert bytes     */
           int *status)     /* IO - error status                            */
/*
 Insert 'ninsert' bytes into each row of the table at position 'bytepos'.
*/
{
    unsigned char buffer[10000], cfill;
    LONGLONG newlen, fbyte, nbytes, irow, nseg, ii;

    if (*status > 0)
        return(*status);

    if (naxis2 == 0)
        return(*status);  /* just return if there are 0 rows in the table */

    /* select appropriate fill value */
    if ((fptr->Fptr)->hdutype == ASCII_TBL)
        cfill = 32;                     /* ASCII tables use blank fill */
    else
        cfill = 0;    /* primary array and binary tables use zero fill */

    newlen = naxis1 + ninsert;

    if (newlen <= 10000)
    {
       /*******************************************************************
       CASE #1: optimal case where whole new row fits in the work buffer
       *******************************************************************/

        for (ii = 0; ii < ninsert; ii++)
            buffer[ii] = cfill;      /* initialize buffer with fill value */

        /* first move the trailing bytes (if any) in the last row */
        fbyte = bytepos + 1;
        nbytes = naxis1 - bytepos;
        ffgtbb(fptr, naxis2, fbyte, nbytes, &buffer[ninsert], status);
        (fptr->Fptr)->rowlength = newlen; /*  new row length */

        /* write the row (with leading fill bytes) in the new place */
        nbytes += ninsert;
        ffptbb(fptr, naxis2, fbyte, nbytes, buffer, status);
        (fptr->Fptr)->rowlength = naxis1;  /* reset to orig. value */

        /*  now move the rest of the rows */
        for (irow = naxis2 - 1; irow > 0; irow--)
        {
            /* read the row to be shifted (work backwards thru the table) */
            ffgtbb(fptr, irow, fbyte, naxis1, &buffer[ninsert], status);
            (fptr->Fptr)->rowlength = newlen; /* new row length */

            /* write the row (with the leading fill bytes) in the new place */
            ffptbb(fptr, irow, fbyte, newlen, buffer, status);
            (fptr->Fptr)->rowlength = naxis1; /* reset to orig value */
        }
    }
    else
    {
        /*****************************************************************
        CASE #2:  whole row doesn't fit in work buffer; move row in pieces
        ******************************************************************
        first copy the data, then go back and write fill into the new column
        start by copying the trailing bytes (if any) in the last row.     */

        nbytes = naxis1 - bytepos;
        nseg = (nbytes + 9999) / 10000;
        fbyte = (nseg - 1) * 10000 + bytepos + 1;
        nbytes = naxis1 - fbyte + 1;

        for (ii = 0; ii < nseg; ii++)
        {
            ffgtbb(fptr, naxis2, fbyte, nbytes, buffer, status);
            (fptr->Fptr)->rowlength =   newlen;  /* new row length */

            ffptbb(fptr, naxis2, fbyte + ninsert, nbytes, buffer, status);
            (fptr->Fptr)->rowlength =   naxis1; /* reset to orig value */

            fbyte -= 10000;
            nbytes = 10000;
        }

        /* now move the rest of the rows */
        nseg = (naxis1 + 9999) / 10000;
        for (irow = naxis2 - 1; irow > 0; irow--)
        {
          fbyte = (nseg - 1) * 10000 + bytepos + 1;
          nbytes = naxis1 - (nseg - 1) * 10000;
          for (ii = 0; ii < nseg; ii++)
          { 
            /* read the row to be shifted (work backwards thru the table) */
            ffgtbb(fptr, irow, fbyte, nbytes, buffer, status);
            (fptr->Fptr)->rowlength =   newlen;  /* new row length */

            /* write the row in the new place */
            ffptbb(fptr, irow, fbyte + ninsert, nbytes, buffer, status);
            (fptr->Fptr)->rowlength =   naxis1; /* reset to orig value */

            fbyte -= 10000;
            nbytes = 10000;
          }
        }

        /* now write the fill values into the new column */
        nbytes = minvalue(ninsert, 10000);
        memset(buffer, cfill, (size_t) nbytes); /* initialize with fill value */

        nseg = (ninsert + 9999) / 10000;
        (fptr->Fptr)->rowlength =  newlen;  /* new row length */

        for (irow = 1; irow <= naxis2; irow++)
        {
          fbyte = bytepos + 1;
          nbytes = ninsert - ((nseg - 1) * 10000);
          for (ii = 0; ii < nseg; ii++)
          {
            ffptbb(fptr, irow, fbyte, nbytes, buffer, status);
            fbyte += nbytes;
            nbytes = 10000;
          }
        }
        (fptr->Fptr)->rowlength = naxis1;  /* reset to orig value */
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcdel(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG naxis1,     /* I - width of the table, in bytes             */
           LONGLONG naxis2,     /* I - number of rows in the table              */
           LONGLONG ndelete,    /* I - number of bytes to delete in each row    */
           LONGLONG bytepos,    /* I - rel. position in row to delete bytes     */
           int *status)     /* IO - error status                            */
/*
 delete 'ndelete' bytes from each row of the table at position 'bytepos'.  */
{
    unsigned char buffer[10000];
    LONGLONG i1, i2, ii, irow, nseg;
    LONGLONG newlen, remain, nbytes;

    if (*status > 0)
        return(*status);

    if (naxis2 == 0)
        return(*status);  /* just return if there are 0 rows in the table */

    newlen = naxis1 - ndelete;

    if (newlen <= 10000)
    {
      /*******************************************************************
      CASE #1: optimal case where whole new row fits in the work buffer
      *******************************************************************/
      i1 = bytepos + 1;
      i2 = i1 + ndelete;
      for (irow = 1; irow < naxis2; irow++)
      {
        ffgtbb(fptr, irow, i2, newlen, buffer, status); /* read row */
        (fptr->Fptr)->rowlength = newlen;  /* new row length */

        ffptbb(fptr, irow, i1, newlen, buffer, status); /* write row */
        (fptr->Fptr)->rowlength = naxis1;  /* reset to orig value */
      }

      /* now do the last row */
      remain = naxis1 - (bytepos + ndelete);

      if (remain > 0)
      {
        ffgtbb(fptr, naxis2, i2, remain, buffer, status); /* read row */
        (fptr->Fptr)->rowlength = newlen;  /* new row length */

        ffptbb(fptr, naxis2, i1, remain, buffer, status); /* write row */
        (fptr->Fptr)->rowlength = naxis1;  /* reset to orig value */
      }
    }
    else
    {
        /*****************************************************************
        CASE #2:  whole row doesn't fit in work buffer; move row in pieces
        ******************************************************************/

        nseg = (newlen + 9999) / 10000;
        for (irow = 1; irow < naxis2; irow++)
        {
          i1 = bytepos + 1;
          i2 = i1 + ndelete;

          nbytes = newlen - (nseg - 1) * 10000;
          for (ii = 0; ii < nseg; ii++)
          { 
            ffgtbb(fptr, irow, i2, nbytes, buffer, status); /* read bytes */
            (fptr->Fptr)->rowlength = newlen;  /* new row length */

            ffptbb(fptr, irow, i1, nbytes, buffer, status); /* rewrite bytes */
            (fptr->Fptr)->rowlength = naxis1; /* reset to orig value */

            i1 += nbytes;
            i2 += nbytes;
            nbytes = 10000;
          }
        }

        /* now do the last row */
        remain = naxis1 - (bytepos + ndelete);

        if (remain > 0)
        {
          nseg = (remain + 9999) / 10000;
          i1 = bytepos + 1;
          i2 = i1 + ndelete;
          nbytes = remain - (nseg - 1) * 10000;
          for (ii = 0; ii < nseg; ii++)
          { 
            ffgtbb(fptr, naxis2, i2, nbytes, buffer, status);
            (fptr->Fptr)->rowlength = newlen;  /* new row length */

            ffptbb(fptr, naxis2, i1, nbytes, buffer, status); /* write row */
            (fptr->Fptr)->rowlength = naxis1;  /* reset to orig value */

            i1 += nbytes;
            i2 += nbytes;
            nbytes = 10000;
          }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffkshf(fitsfile *fptr,  /* I - FITS file pointer                        */
           int colmin,      /* I - starting col. to be incremented; 1 = 1st */
           int colmax,      /* I - last column to be incremented            */
           int incre,       /* I - shift index number by this amount        */
           int *status)     /* IO - error status                            */
/*
  shift the index value on any existing column keywords
  This routine will modify the name of any keyword that begins with 'T'
  and has an index number in the range COLMIN - COLMAX, inclusive.

  if incre is positive, then the index values will be incremented.
  if incre is negative, then the kewords with index = COLMIN
  will be deleted and the index of higher numbered keywords will
  be decremented.
*/
{
    int nkeys, nmore, nrec, tstatus, i1;
    long ivalue;
    char rec[FLEN_CARD], q[FLEN_KEYWORD], newkey[FLEN_KEYWORD];

    ffghsp(fptr, &nkeys, &nmore, status);  /* get number of keywords */

    /* go thru header starting with the 9th keyword looking for 'TxxxxNNN' */

    for (nrec = 9; nrec <= nkeys; nrec++)
    {     
        ffgrec(fptr, nrec, rec, status);

        if (rec[0] == 'T')
        {
            i1 = 0;
            strncpy(q, &rec[1], 4);
            if (!strncmp(q, "BCOL", 4) || !strncmp(q, "FORM", 4) ||
                !strncmp(q, "TYPE", 4) || !strncmp(q, "SCAL", 4) ||
                !strncmp(q, "UNIT", 4) || !strncmp(q, "NULL", 4) ||
                !strncmp(q, "ZERO", 4) || !strncmp(q, "DISP", 4) ||
                !strncmp(q, "LMIN", 4) || !strncmp(q, "LMAX", 4) ||
                !strncmp(q, "DMIN", 4) || !strncmp(q, "DMAX", 4) ||
                !strncmp(q, "CTYP", 4) || !strncmp(q, "CRPX", 4) ||
                !strncmp(q, "CRVL", 4) || !strncmp(q, "CDLT", 4) ||
                !strncmp(q, "CROT", 4) || !strncmp(q, "CUNI", 4) )
              i1 = 5;
            else if (!strncmp(rec, "TDIM", 4) )
              i1 = 4;

            if (i1)
            {
              /* try reading the index number suffix */
              q[0] = '\0';
              strncat(q, &rec[i1], 8 - i1);

              tstatus = 0;
              ffc2ii(q, &ivalue, &tstatus);

              if (tstatus == 0 && ivalue >= colmin && ivalue <= colmax)
              {
                if (incre <= 0 && ivalue == colmin)       
                {
                  ffdrec(fptr, nrec, status); /* delete keyword */
                  nkeys = nkeys - 1;
                  nrec = nrec - 1;
                }
                else
                {
                  ivalue = ivalue + incre;
                  q[0] = '\0';
                  strncat(q, rec, i1);
     
                  ffkeyn(q, ivalue, newkey, status);
                  strncpy(rec, "        ", 8);    /* erase old keyword name */
                  i1 = strlen(newkey);
                  strncpy(rec, newkey, i1);   /* overwrite new keyword name */
                  ffmrec(fptr, nrec, rec, status);  /* modify the record */
                }
              }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffshft(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG firstbyte, /* I - position of first byte in block to shift */
           LONGLONG nbytes,    /* I - size of block of bytes to shift          */
           LONGLONG nshift,    /* I - size of shift in bytes (+ or -)          */
           int *status)     /* IO - error status                            */
/*
    Shift block of bytes by nshift bytes (positive or negative).
    A positive nshift value moves the block down further in the file, while a
    negative value shifts the block towards the beginning of the file.
*/
{
#define shftbuffsize 100000
    long ntomov;
    LONGLONG ptr, ntodo;
    char buffer[shftbuffsize];

    if (*status > 0)
        return(*status);

    ntodo = nbytes;   /* total number of bytes to shift */

    if (nshift > 0)
            /* start at the end of the block and work backwards */
            ptr = firstbyte + nbytes;
    else
            /* start at the beginning of the block working forwards */
            ptr = firstbyte;

    while (ntodo)
    {
        /* number of bytes to move at one time */
        ntomov = (long) (minvalue(ntodo, shftbuffsize));

        if (nshift > 0)     /* if moving block down ... */
            ptr -= ntomov;

        /* move to position and read the bytes to be moved */

        ffmbyt(fptr, ptr, REPORT_EOF, status);
        ffgbyt(fptr, ntomov, buffer, status);

        /* move by shift amount and write the bytes */
        ffmbyt(fptr, ptr + nshift, IGNORE_EOF, status);
        if (ffpbyt(fptr, ntomov, buffer, status) > 0)
        {
           ffpmsg("Error while shifting block (ffshft)");
           return(*status);
        }

        ntodo -= ntomov;
        if (nshift < 0)     /* if moving block up ... */
            ptr += ntomov;
    }

    /* now overwrite the old data with fill */
    if ((fptr->Fptr)->hdutype == ASCII_TBL)
       memset(buffer, 32, shftbuffsize); /* fill ASCII tables with spaces */
    else
       memset(buffer,  0, shftbuffsize); /* fill other HDUs with zeros */


    if (nshift < 0)
    {
        ntodo = -nshift;
        /* point to the end of the shifted block */
        ptr = firstbyte + nbytes + nshift;
    }
    else
    {
        ntodo = nshift;
        /* point to original beginning of the block */
        ptr = firstbyte;
    }

    ffmbyt(fptr, ptr, REPORT_EOF, status);

    while (ntodo)
    {
        ntomov = (long) (minvalue(ntodo, shftbuffsize));
        ffpbyt(fptr, ntomov, buffer, status);
        ntodo -= ntomov;
    }
    return(*status);
}
