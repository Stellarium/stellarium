/*  This file, buffers.c, contains the core set of FITSIO routines         */
/*  that use or manage the internal set of IO buffers.                     */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int ffmbyt(fitsfile *fptr,    /* I - FITS file pointer                */
           LONGLONG bytepos,     /* I - byte position in file to move to */
           int err_mode,      /* I - 1=ignore error, 0 = return error */
           int *status)       /* IO - error status                    */
{
/*
  Move to the input byte location in the file.  When writing to a file, a move
  may sometimes be made to a position beyond the current EOF.  The err_mode
  parameter determines whether such conditions should be returned as an error
  or simply ignored.
*/
    long record;

    if (*status > 0)
       return(*status);

    if (bytepos < 0)
        return(*status = NEG_FILE_POS);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    record = (long) (bytepos / IOBUFLEN);  /* zero-indexed record number */

    /* if this is not the current record, then load it */
    if ( ((fptr->Fptr)->curbuf < 0) || 
         (record != (fptr->Fptr)->bufrecnum[(fptr->Fptr)->curbuf])) 
        ffldrc(fptr, record, err_mode, status);

    if (*status <= 0)
        (fptr->Fptr)->bytepos = bytepos;  /* save new file position */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpbyt(fitsfile *fptr,   /* I - FITS file pointer                    */
           LONGLONG nbytes,      /* I - number of bytes to write             */
           void *buffer,     /* I - buffer containing the bytes to write */
           int *status)      /* IO - error status                        */
/*
  put (write) the buffer of bytes to the output FITS file, starting at
  the current file position.  Write large blocks of data directly to disk;
  write smaller segments to intermediate IO buffers to improve efficiency.
*/
{
    int ii, nbuff;
    LONGLONG filepos;
    long recstart, recend;
    long ntodo, bufpos, nspace, nwrite;
    char *cptr;

    if (*status > 0)
       return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (nbytes > LONG_MAX) {
        ffpmsg("Number of bytes to write is greater than LONG_MAX (ffpbyt).");
        *status = WRITE_ERROR;
	return(*status);
    }
    
    ntodo =  (long) nbytes;
    cptr = (char *)buffer;

    if ((fptr->Fptr)->curbuf < 0)  /* no current data buffer for this file */
    {                              /* so reload the last one that was used */
      ffldrc(fptr, (long) (((fptr->Fptr)->bytepos) / IOBUFLEN), REPORT_EOF, status);
    }

    if (nbytes >= MINDIRECT)
    {
      /* write large blocks of data directly to disk instead of via buffers */
      /* first, fill up the current IO buffer before flushing it to disk */

      nbuff = (fptr->Fptr)->curbuf;      /* current IO buffer number */
      filepos = (fptr->Fptr)->bytepos;   /* save the write starting position */
      recstart = (fptr->Fptr)->bufrecnum[nbuff];                 /* starting record */
      recend = (long) ((filepos + nbytes - 1) / IOBUFLEN);  /* ending record   */

      /* bufpos is the starting position within the IO buffer */
      bufpos = (long) (filepos - ((LONGLONG)recstart * IOBUFLEN));
      nspace = IOBUFLEN - bufpos;   /* amount of space left in the buffer */

      if (nspace)
      { /* fill up the IO buffer */
        memcpy((fptr->Fptr)->iobuffer + (nbuff * IOBUFLEN) + bufpos, cptr, nspace);
        ntodo -= nspace;           /* decrement remaining number of bytes */
        cptr += nspace;            /* increment user buffer pointer */
        filepos += nspace;         /* increment file position pointer */
        (fptr->Fptr)->dirty[nbuff] = TRUE;       /* mark record as having been modified */
      }

      for (ii = 0; ii < NIOBUF; ii++) /* flush any affected buffers to disk */
      {
        if ((fptr->Fptr)->bufrecnum[ii] >= recstart
            && (fptr->Fptr)->bufrecnum[ii] <= recend )
        {
          if ((fptr->Fptr)->dirty[ii])        /* flush modified buffer to disk */
             ffbfwt(fptr->Fptr, ii, status);

          (fptr->Fptr)->bufrecnum[ii] = -1;  /* disassociate buffer from the file */
        }
      }

      /* move to the correct write position */
      if ((fptr->Fptr)->io_pos != filepos)
         ffseek(fptr->Fptr, filepos);

      nwrite = ((ntodo - 1) / IOBUFLEN) * IOBUFLEN; /* don't write last buff */

      ffwrite(fptr->Fptr, nwrite, cptr, status); /* write the data */
      ntodo -= nwrite;                /* decrement remaining number of bytes */
      cptr += nwrite;                  /* increment user buffer pointer */
      (fptr->Fptr)->io_pos = filepos + nwrite; /* update the file position */

      if ((fptr->Fptr)->io_pos >= (fptr->Fptr)->filesize) /* at the EOF? */
      {
        (fptr->Fptr)->filesize = (fptr->Fptr)->io_pos; /* increment file size */

        /* initialize the current buffer with the correct fill value */
        if ((fptr->Fptr)->hdutype == ASCII_TBL)
          memset((fptr->Fptr)->iobuffer + (nbuff * IOBUFLEN), 32, IOBUFLEN);  /* blank fill */
        else
          memset((fptr->Fptr)->iobuffer + (nbuff * IOBUFLEN),  0, IOBUFLEN);  /* zero fill */
      }
      else
      {
        /* read next record */
        ffread(fptr->Fptr, IOBUFLEN, (fptr->Fptr)->iobuffer + (nbuff * IOBUFLEN), status);
        (fptr->Fptr)->io_pos += IOBUFLEN; 
      }

      /* copy remaining bytes from user buffer into current IO buffer */
      memcpy((fptr->Fptr)->iobuffer + (nbuff * IOBUFLEN), cptr, ntodo);
      (fptr->Fptr)->dirty[nbuff] = TRUE;       /* mark record as having been modified */
      (fptr->Fptr)->bufrecnum[nbuff] = recend; /* record number */

      (fptr->Fptr)->logfilesize = maxvalue((fptr->Fptr)->logfilesize, 
                                       (LONGLONG)(recend + 1) * IOBUFLEN);
      (fptr->Fptr)->bytepos = filepos + nwrite + ntodo;
    }
    else
    {
      /* bufpos is the starting position in IO buffer */
      bufpos = (long) ((fptr->Fptr)->bytepos - ((LONGLONG)(fptr->Fptr)->bufrecnum[(fptr->Fptr)->curbuf] *
               IOBUFLEN));
      nspace = IOBUFLEN - bufpos;   /* amount of space left in the buffer */

      while (ntodo)
      {
        nwrite = minvalue(ntodo, nspace);

        /* copy bytes from user's buffer to the IO buffer */
        memcpy((fptr->Fptr)->iobuffer + ((fptr->Fptr)->curbuf * IOBUFLEN) + bufpos, cptr, nwrite);
        ntodo -= nwrite;            /* decrement remaining number of bytes */
        cptr += nwrite;
        (fptr->Fptr)->bytepos += nwrite;  /* increment file position pointer */
        (fptr->Fptr)->dirty[(fptr->Fptr)->curbuf] = TRUE; /* mark record as modified */

        if (ntodo)                  /* load next record into a buffer */
        {
          ffldrc(fptr, (long) ((fptr->Fptr)->bytepos / IOBUFLEN), IGNORE_EOF, status);
          bufpos = 0;
          nspace = IOBUFLEN;
        }
      }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpbytoff(fitsfile *fptr, /* I - FITS file pointer                   */
           long gsize,        /* I - size of each group of bytes         */
           long ngroups,      /* I - number of groups to write           */
           long offset,       /* I - size of gap between groups          */
           void *buffer,      /* I - buffer to be written                */
           int *status)       /* IO - error status                       */
/*
  put (write) the buffer of bytes to the output FITS file, with an offset
  between each group of bytes.  This function combines ffmbyt and ffpbyt
  for increased efficiency.
*/
{
    int bcurrent;
    long ii, bufpos, nspace, nwrite, record;
    char *cptr, *ioptr;

    if (*status > 0)
       return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if ((fptr->Fptr)->curbuf < 0)  /* no current data buffer for this file */
    {                              /* so reload the last one that was used */
      ffldrc(fptr, (long) (((fptr->Fptr)->bytepos) / IOBUFLEN), REPORT_EOF, status);
    }

    cptr = (char *)buffer;
    bcurrent = (fptr->Fptr)->curbuf;     /* number of the current IO buffer */
    record = (fptr->Fptr)->bufrecnum[bcurrent];  /* zero-indexed record number */
    bufpos = (long) ((fptr->Fptr)->bytepos - ((LONGLONG)record * IOBUFLEN)); /* start pos */
    nspace = IOBUFLEN - bufpos;  /* amount of space left in buffer */
    ioptr = (fptr->Fptr)->iobuffer + (bcurrent * IOBUFLEN) + bufpos;  

    for (ii = 1; ii < ngroups; ii++)  /* write all but the last group */
    {
      /* copy bytes from user's buffer to the IO buffer */
      nwrite = minvalue(gsize, nspace);
      memcpy(ioptr, cptr, nwrite);
      cptr += nwrite;          /* increment buffer pointer */

      if (nwrite < gsize)        /* entire group did not fit */
      {
        (fptr->Fptr)->dirty[bcurrent] = TRUE;  /* mark record as having been modified */
        record++;
        ffldrc(fptr, record, IGNORE_EOF, status);  /* load next record */
        bcurrent = (fptr->Fptr)->curbuf;
        ioptr   = (fptr->Fptr)->iobuffer + (bcurrent * IOBUFLEN);

        nwrite  = gsize - nwrite;
        memcpy(ioptr, cptr, nwrite);
        cptr   += nwrite;            /* increment buffer pointer */
        ioptr  += (offset + nwrite); /* increment IO buffer pointer */
        nspace = IOBUFLEN - offset - nwrite;  /* amount of space left */
      }
      else
      {
        ioptr  += (offset + nwrite);  /* increment IO bufer pointer */
        nspace -= (offset + nwrite);
      }

      if (nspace <= 0) /* beyond current record? */
      {
        (fptr->Fptr)->dirty[bcurrent] = TRUE;
        record += ((IOBUFLEN - nspace) / IOBUFLEN); /* new record number */
        ffldrc(fptr, record, IGNORE_EOF, status);
        bcurrent = (fptr->Fptr)->curbuf;

        bufpos = (-nspace) % IOBUFLEN; /* starting buffer pos */
        nspace = IOBUFLEN - bufpos;
        ioptr = (fptr->Fptr)->iobuffer + (bcurrent * IOBUFLEN) + bufpos;  
      }
    }
      
    /* now write the last group */
    nwrite = minvalue(gsize, nspace);
    memcpy(ioptr, cptr, nwrite);
    cptr += nwrite;          /* increment buffer pointer */

    if (nwrite < gsize)        /* entire group did not fit */
    {
      (fptr->Fptr)->dirty[bcurrent] = TRUE;  /* mark record as having been modified */
      record++;
      ffldrc(fptr, record, IGNORE_EOF, status);  /* load next record */
      bcurrent = (fptr->Fptr)->curbuf;
      ioptr   = (fptr->Fptr)->iobuffer + (bcurrent * IOBUFLEN);

      nwrite  = gsize - nwrite;
      memcpy(ioptr, cptr, nwrite);
    }

    (fptr->Fptr)->dirty[bcurrent] = TRUE;    /* mark record as having been modified */
    (fptr->Fptr)->bytepos = (fptr->Fptr)->bytepos + (ngroups * gsize)
                                  + (ngroups - 1) * offset;
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgbyt(fitsfile *fptr,    /* I - FITS file pointer             */
           LONGLONG nbytes,       /* I - number of bytes to read       */
           void *buffer,      /* O - buffer to read into           */
           int *status)       /* IO - error status                 */
/*
  get (read) the requested number of bytes from the file, starting at
  the current file position.  Read large blocks of data directly from disk;
  read smaller segments via intermediate IO buffers to improve efficiency.
*/
{
    int ii;
    LONGLONG filepos;
    long recstart, recend, ntodo, bufpos, nspace, nread;
    char *cptr;

    if (*status > 0)
       return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    cptr = (char *)buffer;

    if (nbytes >= MINDIRECT)
    {
      /* read large blocks of data directly from disk instead of via buffers */
      filepos = (fptr->Fptr)->bytepos; /* save the read starting position */

/*  note that in this case, ffmbyt has not been called, and so        */
/*  bufrecnum[(fptr->Fptr)->curbuf] does not point to the intended */
/*  output buffer */

      recstart = (long) (filepos / IOBUFLEN);               /* starting record */
      recend = (long) ((filepos + nbytes - 1) / IOBUFLEN);  /* ending record   */

      for (ii = 0; ii < NIOBUF; ii++) /* flush any affected buffers to disk */
      {
        if ((fptr->Fptr)->dirty[ii] && 
            (fptr->Fptr)->bufrecnum[ii] >= recstart && (fptr->Fptr)->bufrecnum[ii] <= recend)
            {
              ffbfwt(fptr->Fptr, ii, status);    /* flush modified buffer to disk */
            }
      }

       /* move to the correct read position */
      if ((fptr->Fptr)->io_pos != filepos)
         ffseek(fptr->Fptr, filepos);

      ffread(fptr->Fptr, (long) nbytes, cptr, status); /* read the data */
      (fptr->Fptr)->io_pos = filepos + nbytes; /* update the file position */
    }
    else
    {
      /* read small chucks of data using the IO buffers for efficiency */

      if ((fptr->Fptr)->curbuf < 0)  /* no current data buffer for this file */
      {                              /* so reload the last one that was used */
        ffldrc(fptr, (long) (((fptr->Fptr)->bytepos) / IOBUFLEN), REPORT_EOF, status);
      }

      /* bufpos is the starting position in IO buffer */
      bufpos = (long) ((fptr->Fptr)->bytepos - ((LONGLONG)(fptr->Fptr)->bufrecnum[(fptr->Fptr)->curbuf] *
                IOBUFLEN));
      nspace = IOBUFLEN - bufpos;   /* amount of space left in the buffer */

      ntodo =  (long) nbytes;
      while (ntodo)
      {
        nread  = minvalue(ntodo, nspace);

        /* copy bytes from IO buffer to user's buffer */
        memcpy(cptr, (fptr->Fptr)->iobuffer + ((fptr->Fptr)->curbuf * IOBUFLEN) + bufpos, nread);
        ntodo -= nread;            /* decrement remaining number of bytes */
        cptr  += nread;
        (fptr->Fptr)->bytepos += nread;    /* increment file position pointer */

        if (ntodo)                  /* load next record into a buffer */
        {
          ffldrc(fptr, (long) ((fptr->Fptr)->bytepos / IOBUFLEN), REPORT_EOF, status);
          bufpos = 0;
          nspace = IOBUFLEN;
        }
      }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgbytoff(fitsfile *fptr, /* I - FITS file pointer                   */
           long gsize,        /* I - size of each group of bytes         */
           long ngroups,      /* I - number of groups to read            */
           long offset,       /* I - size of gap between groups (may be < 0) */
           void *buffer,      /* I - buffer to be filled                 */
           int *status)       /* IO - error status                       */
/*
  get (read) the requested number of bytes from the file, starting at
  the current file position.  This function combines ffmbyt and ffgbyt
  for increased efficiency.
*/
{
    int bcurrent;
    long ii, bufpos, nspace, nread, record;
    char *cptr, *ioptr;

    if (*status > 0)
       return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if ((fptr->Fptr)->curbuf < 0)  /* no current data buffer for this file */
    {                              /* so reload the last one that was used */
      ffldrc(fptr, (long) (((fptr->Fptr)->bytepos) / IOBUFLEN), REPORT_EOF, status);
    }

    cptr = (char *)buffer;
    bcurrent = (fptr->Fptr)->curbuf;     /* number of the current IO buffer */
    record = (fptr->Fptr)->bufrecnum[bcurrent];  /* zero-indexed record number */
    bufpos = (long) ((fptr->Fptr)->bytepos - ((LONGLONG)record * IOBUFLEN)); /* start pos */
    nspace = IOBUFLEN - bufpos;  /* amount of space left in buffer */
    ioptr = (fptr->Fptr)->iobuffer + (bcurrent * IOBUFLEN) + bufpos;  

    for (ii = 1; ii < ngroups; ii++)  /* read all but the last group */
    {
      /* copy bytes from IO buffer to the user's buffer */
      nread = minvalue(gsize, nspace);
      memcpy(cptr, ioptr, nread);
      cptr += nread;          /* increment buffer pointer */

      if (nread < gsize)        /* entire group did not fit */
      {
        record++;
        ffldrc(fptr, record, REPORT_EOF, status);  /* load next record */
        bcurrent = (fptr->Fptr)->curbuf;
        ioptr   = (fptr->Fptr)->iobuffer + (bcurrent * IOBUFLEN);

        nread  = gsize - nread;
        memcpy(cptr, ioptr, nread);
        cptr   += nread;            /* increment buffer pointer */
        ioptr  += (offset + nread); /* increment IO buffer pointer */
        nspace = IOBUFLEN - offset - nread;  /* amount of space left */
      }
      else
      {
        ioptr  += (offset + nread);  /* increment IO bufer pointer */
        nspace -= (offset + nread);
      }

      if (nspace <= 0 || nspace > IOBUFLEN) /* beyond current record? */
      {
        if (nspace <= 0)
        {
          record += ((IOBUFLEN - nspace) / IOBUFLEN); /* new record number */
          bufpos = (-nspace) % IOBUFLEN; /* starting buffer pos */
        }
        else
        {
          record -= ((nspace - 1 ) / IOBUFLEN); /* new record number */
          bufpos = IOBUFLEN - (nspace % IOBUFLEN); /* starting buffer pos */
        }

        ffldrc(fptr, record, REPORT_EOF, status);
        bcurrent = (fptr->Fptr)->curbuf;

        nspace = IOBUFLEN - bufpos;
        ioptr = (fptr->Fptr)->iobuffer + (bcurrent * IOBUFLEN) + bufpos;
      }
    }

    /* now read the last group */
    nread = minvalue(gsize, nspace);
    memcpy(cptr, ioptr, nread);
    cptr += nread;          /* increment buffer pointer */

    if (nread < gsize)        /* entire group did not fit */
    {
      record++;
      ffldrc(fptr, record, REPORT_EOF, status);  /* load next record */
      bcurrent = (fptr->Fptr)->curbuf;
      ioptr   = (fptr->Fptr)->iobuffer + (bcurrent * IOBUFLEN);

      nread  = gsize - nread;
      memcpy(cptr, ioptr, nread);
    }

    (fptr->Fptr)->bytepos = (fptr->Fptr)->bytepos + (ngroups * gsize)
                                  + (ngroups - 1) * offset;
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffldrc(fitsfile *fptr,        /* I - FITS file pointer             */
           long record,           /* I - record number to be loaded    */
           int err_mode,          /* I - 1=ignore EOF, 0 = return EOF error */
           int *status)           /* IO - error status                 */
{
/*
  low-level routine to load a specified record from a file into
  a physical buffer, if it is not already loaded.  Reset all
  pointers to make this the new current record for that file.
  Update ages of all the physical buffers.
*/
    int ibuff, nbuff;
    LONGLONG rstart;

    /* check if record is already loaded in one of the buffers */
    /* search from youngest to oldest buffer for efficiency */

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    for (ibuff = NIOBUF - 1; ibuff >= 0; ibuff--)
    {
      nbuff = (fptr->Fptr)->ageindex[ibuff];
      if (record == (fptr->Fptr)->bufrecnum[nbuff]) {
         goto updatebuf;  /* use 'goto' for efficiency */
      }
    }

    /* record is not already loaded */
    rstart = (LONGLONG)record * IOBUFLEN;

    if ( !err_mode && (rstart >= (fptr->Fptr)->logfilesize) )  /* EOF? */
         return(*status = END_OF_FILE);

    if (ffwhbf(fptr, &nbuff) < 0)  /* which buffer should we reuse? */
       return(*status = TOO_MANY_FILES); 

    if ((fptr->Fptr)->dirty[nbuff])
       ffbfwt(fptr->Fptr, nbuff, status); /* write dirty buffer to disk */

    if (rstart >= (fptr->Fptr)->filesize)  /* EOF? */
    {
      /* initialize an empty buffer with the correct fill value */
      if ((fptr->Fptr)->hdutype == ASCII_TBL)
         memset((fptr->Fptr)->iobuffer + (nbuff * IOBUFLEN), 32, IOBUFLEN); /* blank fill */
      else
         memset((fptr->Fptr)->iobuffer + (nbuff * IOBUFLEN),  0, IOBUFLEN);  /* zero fill */

      (fptr->Fptr)->logfilesize = maxvalue((fptr->Fptr)->logfilesize, 
              rstart + IOBUFLEN);

      (fptr->Fptr)->dirty[nbuff] = TRUE;  /* mark record as having been modified */
    }
    else  /* not EOF, so read record from disk */
    {
      if ((fptr->Fptr)->io_pos != rstart)
           ffseek(fptr->Fptr, rstart);

      ffread(fptr->Fptr, IOBUFLEN, (fptr->Fptr)->iobuffer + (nbuff * IOBUFLEN), status);
      (fptr->Fptr)->io_pos = rstart + IOBUFLEN;  /* set new IO position */
    }

    (fptr->Fptr)->bufrecnum[nbuff] = record;   /* record number contained in buffer */

updatebuf:

    (fptr->Fptr)->curbuf = nbuff; /* this is the current buffer for this file */

    if (ibuff < 0)
    { 
      /* find the current position of the buffer in the age index */
      for (ibuff = 0; ibuff < NIOBUF; ibuff++)
         if ((fptr->Fptr)->ageindex[ibuff] == nbuff)
            break;  
    }

    /* increment the age of all the buffers that were younger than it */
    for (ibuff++; ibuff < NIOBUF; ibuff++)
      (fptr->Fptr)->ageindex[ibuff - 1] = (fptr->Fptr)->ageindex[ibuff];

    (fptr->Fptr)->ageindex[NIOBUF - 1] = nbuff; /* this is now the youngest buffer */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffwhbf(fitsfile *fptr,        /* I - FITS file pointer             */
           int *nbuff)            /* O - which buffer to use           */
{
/*
  decide which buffer to (re)use to hold a new file record
*/
        return(*nbuff = (fptr->Fptr)->ageindex[0]);  /* return oldest buffer */
}
/*--------------------------------------------------------------------------*/
int ffflus(fitsfile *fptr,   /* I - FITS file pointer                       */
           int *status)      /* IO - error status                           */
/*
  Flush all the data in the current FITS file to disk. This ensures that if
  the program subsequently dies, the disk FITS file will be closed correctly.
*/
{
    int hdunum, hdutype;

    if (*status > 0)
        return(*status);

    ffghdn(fptr, &hdunum);     /* get the current HDU number */

    if (ffchdu(fptr,status) > 0)   /* close out the current HDU */
        ffpmsg("ffflus could not close the current HDU.");

    ffflsh(fptr, FALSE, status);  /* flush any modified IO buffers to disk */

    if (ffgext(fptr, hdunum - 1, &hdutype, status) > 0) /* reopen HDU */
        ffpmsg("ffflus could not reopen the current HDU.");

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffflsh(fitsfile *fptr,        /* I - FITS file pointer           */
           int clearbuf,          /* I - also clear buffer contents? */
           int *status)           /* IO - error status               */
{
/*
  flush all dirty IO buffers associated with the file to disk
*/
    int ii;

/*
   no need to move to a different HDU

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
*/
    for (ii = 0; ii < NIOBUF; ii++)
    {
	/* flush modified buffer to disk */
        if ((fptr->Fptr)->bufrecnum[ii] >= 0 &&(fptr->Fptr)->dirty[ii])
           ffbfwt(fptr->Fptr, ii, status);

        if (clearbuf)
          (fptr->Fptr)->bufrecnum[ii] = -1;  /* set contents of buffer as undefined */
    }

    if (*status != READONLY_FILE)
      ffflushx(fptr->Fptr);  /* flush system buffers to disk */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffbfeof(fitsfile *fptr,        /* I - FITS file pointer           */
           int *status)           /* IO - error status               */
{
/*
  clear any buffers beyond the end of file
*/
    int ii;

    for (ii = 0; ii < NIOBUF; ii++)
    {
        if ( (LONGLONG) (fptr->Fptr)->bufrecnum[ii] * IOBUFLEN >= fptr->Fptr->filesize)
        {
            (fptr->Fptr)->bufrecnum[ii] = -1;  /* set contents of buffer as undefined */
        }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffbfwt(FITSfile *Fptr,        /* I - FITS file pointer           */
           int nbuff,             /* I - which buffer to write          */
           int *status)           /* IO - error status                  */
{
/*
  write contents of buffer to file;  If the position of the buffer
  is beyond the current EOF, then the file may need to be extended
  with fill values, and/or with the contents of some of the other
  i/o buffers.
*/
    int  ii,ibuff;
    long jj, irec, minrec, nloop;
    LONGLONG filepos;

    static char zeros[IOBUFLEN];  /*  initialized to zero by default */

    if (!(Fptr->writemode) )
    {
        ffpmsg("Error: trying to write to READONLY file.");
        if (Fptr->driver == 8) {  /* gzip compressed file */
	  ffpmsg("Cannot write to a GZIP or COMPRESS compressed file.");
	}
        Fptr->dirty[nbuff] = FALSE;  /* reset buffer status to prevent later probs */
        *status = READONLY_FILE;
        return(*status);
    }

    filepos = (LONGLONG)Fptr->bufrecnum[nbuff] * IOBUFLEN;

    if (filepos <= Fptr->filesize)
    {
      /* record is located within current file, so just write it */

      /* move to the correct write position */
      if (Fptr->io_pos != filepos)
         ffseek(Fptr, filepos);

      ffwrite(Fptr, IOBUFLEN, Fptr->iobuffer + (nbuff * IOBUFLEN), status);
      Fptr->io_pos = filepos + IOBUFLEN;

      if (filepos == Fptr->filesize)   /* appended new record? */
         Fptr->filesize += IOBUFLEN;   /* increment the file size */

      Fptr->dirty[nbuff] = FALSE;
    }

    else  /* if record is beyond the EOF, append any other records */ 
          /* and/or insert fill values if necessary */
    {
      /* move to EOF */
      if (Fptr->io_pos != Fptr->filesize)
         ffseek(Fptr, Fptr->filesize);

      ibuff = NIOBUF;  /* initialize to impossible value */
      while(ibuff != nbuff) /* repeat until requested buffer is written */
      {
        minrec = (long) (Fptr->filesize / IOBUFLEN);

        /* write lowest record beyond the EOF first */

        irec = Fptr->bufrecnum[nbuff]; /* initially point to the requested buffer */
        ibuff = nbuff;

        for (ii = 0; ii < NIOBUF; ii++)
        {
          if (Fptr->bufrecnum[ii] >= minrec &&
            Fptr->bufrecnum[ii] < irec)
          {
            irec = Fptr->bufrecnum[ii];  /* found a lower record */
            ibuff = ii;
          }
        }

        filepos = (LONGLONG)irec * IOBUFLEN;  /* byte offset of record in file */

        /* append 1 or more fill records if necessary */
        if (filepos > Fptr->filesize)
        {                    
          nloop = (long) ((filepos - (Fptr->filesize)) / IOBUFLEN); 
          for (jj = 0; jj < nloop && !(*status); jj++)
            ffwrite(Fptr, IOBUFLEN, zeros, status);

/*
ffseek(Fptr, filepos);
*/
          Fptr->filesize = filepos;   /* increment the file size */
        } 

        /* write the buffer itself */
        ffwrite(Fptr, IOBUFLEN, Fptr->iobuffer + (ibuff * IOBUFLEN), status);
        Fptr->dirty[ibuff] = FALSE;

        Fptr->filesize += IOBUFLEN;     /* increment the file size */
      } /* loop back if more buffers need to be written */

      Fptr->io_pos = Fptr->filesize;  /* currently positioned at EOF */
    }

    return(*status);       
}
/*--------------------------------------------------------------------------*/
int ffgrsz( fitsfile *fptr, /* I - FITS file pionter                        */
            long *ndata,    /* O - optimal amount of data to access         */
            int  *status)   /* IO - error status                            */
/*
  Returns an optimal value for the number of rows in a binary table
  or the number of pixels in an image that should be read or written
  at one time for maximum efficiency. Accessing more data than this
  may cause excessive flushing and rereading of buffers to/from disk.
*/
{
    int typecode, bytesperpixel;

    /* There are NIOBUF internal buffers available each IOBUFLEN bytes long. */

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
      if ( ffrdef(fptr, status) > 0)   /* rescan header to get hdu struct */
           return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU ) /* calc pixels per buffer size */
    {
      /* image pixels are in column 2 of the 'table' */
      ffgtcl(fptr, 2, &typecode, NULL, NULL, status);
      bytesperpixel = typecode / 10;
      *ndata = ((NIOBUF - 1) * IOBUFLEN) / bytesperpixel;
    }
    else   /* calc number of rows that fit in buffers */
    {
      *ndata = (long) (((NIOBUF - 1) * IOBUFLEN) / maxvalue(1,
               (fptr->Fptr)->rowlength));
      *ndata = maxvalue(1, *ndata); 
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtbb(fitsfile *fptr,        /* I - FITS file pointer                 */
           LONGLONG firstrow,         /* I - starting row (1 = first row)      */
           LONGLONG firstchar,        /* I - starting byte in row (1=first)    */
           LONGLONG nchars,           /* I - number of bytes to read           */
           unsigned char *values, /* I - array of bytes to read            */
           int *status)           /* IO - error status                     */
/*
  read a consecutive string of bytes from an ascii or binary table.
  This will span multiple rows of the table if nchars + firstchar is
  greater than the length of a row.
*/
{
    LONGLONG bytepos, endrow;

    if (*status > 0 || nchars <= 0)
        return(*status);

    else if (firstrow < 1)
        return(*status=BAD_ROW_NUM);

    else if (firstchar < 1)
        return(*status=BAD_ELEM_NUM);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* check that we do not exceed number of rows in the table */
    endrow = ((firstchar + nchars - 2) / (fptr->Fptr)->rowlength) + firstrow;
    if (endrow > (fptr->Fptr)->numrows)
    {
        ffpmsg("attempt to read past end of table (ffgtbb)");
        return(*status=BAD_ROW_NUM);
    }

    /* move the i/o pointer to the start of the sequence of characters */
    bytepos = (fptr->Fptr)->datastart +
              ((fptr->Fptr)->rowlength * (firstrow - 1)) +
              firstchar - 1;

    ffmbyt(fptr, bytepos, REPORT_EOF, status);
    ffgbyt(fptr, nchars, values, status);  /* read the bytes */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgi1b(fitsfile *fptr, /* I - FITS file pointer                         */
           LONGLONG byteloc,  /* I - position within file to start reading     */
           long nvals,     /* I - number of pixels to read                  */
           long incre,     /* I - byte increment between pixels             */
           unsigned char *values, /* O - returned array of values           */
           int *status)    /* IO - error status                             */
/*
  get (read) the array of values from the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
    LONGLONG postemp;

    if (incre == 1)      /* read all the values at once (contiguous bytes) */
    {
        if (nvals < MINDIRECT)  /* read normally via IO buffers */
        {
           ffmbyt(fptr, byteloc, REPORT_EOF, status);
           ffgbyt(fptr, nvals, values, status);
        }
        else            /* read directly from disk, bypassing IO buffers */
        {
           postemp = (fptr->Fptr)->bytepos;   /* store current file position */
           (fptr->Fptr)->bytepos = byteloc;   /* set to the desired position */
           ffgbyt(fptr, nvals, values, status);
           (fptr->Fptr)->bytepos = postemp;   /* reset to original position */
        }
    }
    else         /* have to read each value individually (not contiguous ) */
    {
        ffmbyt(fptr, byteloc, REPORT_EOF, status);
        ffgbytoff(fptr, 1, nvals, incre - 1, values, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgi2b(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG byteloc,   /* I - position within file to start reading    */
           long nvals,      /* I - number of pixels to read                 */
           long incre,      /* I - byte increment between pixels            */
           short *values,   /* O - returned array of values                 */
           int *status)     /* IO - error status                            */
/*
  get (read) the array of values from the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
    LONGLONG postemp;

    if (incre == 2)      /* read all the values at once (contiguous bytes) */
    {
        if (nvals * 2 < MINDIRECT)  /* read normally via IO buffers */
        {
           ffmbyt(fptr, byteloc, REPORT_EOF, status);
           ffgbyt(fptr, nvals * 2, values, status);
        }
        else            /* read directly from disk, bypassing IO buffers */
        {
           postemp = (fptr->Fptr)->bytepos;   /* store current file position */
           (fptr->Fptr)->bytepos = byteloc;   /* set to the desired position */
           ffgbyt(fptr, nvals * 2, values, status);
           (fptr->Fptr)->bytepos = postemp;   /* reset to original position */
        }
    }
    else         /* have to read each value individually (not contiguous ) */
    {
        ffmbyt(fptr, byteloc, REPORT_EOF, status);
        ffgbytoff(fptr, 2, nvals, incre - 2, values, status);
    }

#if BYTESWAPPED
    ffswap2(values, nvals);    /* reverse order of bytes in each value */
#endif

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgi4b(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG byteloc,   /* I - position within file to start reading    */
           long nvals,      /* I - number of pixels to read                 */
           long incre,      /* I - byte increment between pixels            */
           INT32BIT *values, /* O - returned array of values                */
           int *status)     /* IO - error status                            */
/*
  get (read) the array of values from the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
    LONGLONG postemp;

    if (incre == 4)      /* read all the values at once (contiguous bytes) */
    {
        if (nvals * 4 < MINDIRECT)  /* read normally via IO buffers */
        {
           ffmbyt(fptr, byteloc, REPORT_EOF, status);
           ffgbyt(fptr, nvals * 4, values, status);
        }
        else            /* read directly from disk, bypassing IO buffers */
        {
           postemp = (fptr->Fptr)->bytepos;   /* store current file position */
           (fptr->Fptr)->bytepos = byteloc;   /* set to the desired position */
           ffgbyt(fptr, nvals * 4, values, status);
           (fptr->Fptr)->bytepos = postemp;   /* reset to original position */
        }
    }
    else         /* have to read each value individually (not contiguous ) */
    {
        ffmbyt(fptr, byteloc, REPORT_EOF, status);
        ffgbytoff(fptr, 4, nvals, incre - 4, values, status);
    }

#if BYTESWAPPED
    ffswap4(values, nvals);    /* reverse order of bytes in each value */
#endif

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgi8b(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG byteloc,   /* I - position within file to start reading    */
           long nvals,      /* I - number of pixels to read                 */
           long incre,      /* I - byte increment between pixels            */
           long *values,  /* O - returned array of values                 */
           int *status)     /* IO - error status                            */
/*
  get (read) the array of values from the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.

  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  This routine reads 'nvals' 8-byte integers into 'values'.
  This works both on platforms that have sizeof(long) = 64, and 32,
  as long as 'values' has been allocated to large enough to hold
  8 * nvals bytes of data.
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
{
    LONGLONG  postemp;

    if (incre == 8)      /* read all the values at once (contiguous bytes) */
    {
        if (nvals * 8 < MINDIRECT)  /* read normally via IO buffers */
        {
           ffmbyt(fptr, byteloc, REPORT_EOF, status);
           ffgbyt(fptr, nvals * 8, values, status);
        }
        else            /* read directly from disk, bypassing IO buffers */
        {
           postemp = (fptr->Fptr)->bytepos;   /* store current file position */
           (fptr->Fptr)->bytepos = byteloc;   /* set to the desired position */
           ffgbyt(fptr, nvals * 8, values, status);
           (fptr->Fptr)->bytepos = postemp;   /* reset to original position */
        }
    }
    else         /* have to read each value individually (not contiguous ) */
    {
        ffmbyt(fptr, byteloc, REPORT_EOF, status);
        ffgbytoff(fptr, 8, nvals, incre - 8, values, status);
    }

#if BYTESWAPPED
    ffswap8((double *) values, nvals); /* reverse bytes in each value */
#endif

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgr4b(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG byteloc,   /* I - position within file to start reading    */
           long nvals,      /* I - number of pixels to read                 */
           long incre,      /* I - byte increment between pixels            */
           float *values,   /* O - returned array of values                 */
           int *status)     /* IO - error status                            */
/*
  get (read) the array of values from the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
    LONGLONG postemp;

#if MACHINE == VAXVMS
    long ii;

#elif (MACHINE == ALPHAVMS) && (FLOATTYPE == GFLOAT)
    short *sptr;
    long ii;

#endif


    if (incre == 4)      /* read all the values at once (contiguous bytes) */
    {
        if (nvals * 4 < MINDIRECT)  /* read normally via IO buffers */
        {
           ffmbyt(fptr, byteloc, REPORT_EOF, status);
           ffgbyt(fptr, nvals * 4, values, status);
        }
        else            /* read directly from disk, bypassing IO buffers */
        {
           postemp = (fptr->Fptr)->bytepos;   /* store current file position */
           (fptr->Fptr)->bytepos = byteloc;   /* set to the desired position */
           ffgbyt(fptr, nvals * 4, values, status);
           (fptr->Fptr)->bytepos = postemp;   /* reset to original position */
        }
    }
    else         /* have to read each value individually (not contiguous ) */
    {
        ffmbyt(fptr, byteloc, REPORT_EOF, status);
        ffgbytoff(fptr, 4, nvals, incre - 4, values, status);
    }


#if MACHINE == VAXVMS

    ii = nvals;                      /* call VAX macro routine to convert */
    ieevur(values, values, &ii);     /* from  IEEE float -> F float       */

#elif (MACHINE == ALPHAVMS) && (FLOATTYPE == GFLOAT)

    ffswap2( (short *) values, nvals * 2);  /* swap pairs of bytes */

    /* convert from IEEE float format to VMS GFLOAT float format */
    sptr = (short *) values;
    for (ii = 0; ii < nvals; ii++, sptr += 2)
    {
        if (!fnan(*sptr) )  /* test for NaN or underflow */
            values[ii] *= 4.0;
    }

#elif BYTESWAPPED
    ffswap4((INT32BIT *)values, nvals);  /* reverse order of bytes in values */
#endif

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgr8b(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG byteloc,   /* I - position within file to start reading    */
           long nvals,      /* I - number of pixels to read                 */
           long incre,      /* I - byte increment between pixels            */
           double *values,  /* O - returned array of values                 */
           int *status)     /* IO - error status                            */
/*
  get (read) the array of values from the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
    LONGLONG  postemp;

#if MACHINE == VAXVMS
    long ii;

#elif (MACHINE == ALPHAVMS) && (FLOATTYPE == GFLOAT)
    short *sptr;
    long ii;

#endif

    if (incre == 8)      /* read all the values at once (contiguous bytes) */
    {
        if (nvals * 8 < MINDIRECT)  /* read normally via IO buffers */
        {
           ffmbyt(fptr, byteloc, REPORT_EOF, status);
           ffgbyt(fptr, nvals * 8, values, status);
        }
        else            /* read directly from disk, bypassing IO buffers */
        {
           postemp = (fptr->Fptr)->bytepos;   /* store current file position */
           (fptr->Fptr)->bytepos = byteloc;   /* set to the desired position */
           ffgbyt(fptr, nvals * 8, values, status);
           (fptr->Fptr)->bytepos = postemp;   /* reset to original position */
        }
    }
    else         /* have to read each value individually (not contiguous ) */
    {
        ffmbyt(fptr, byteloc, REPORT_EOF, status);
        ffgbytoff(fptr, 8, nvals, incre - 8, values, status);
    }

#if MACHINE == VAXVMS
    ii = nvals;                      /* call VAX macro routine to convert */
    ieevud(values, values, &ii);     /* from  IEEE float -> D float       */

#elif (MACHINE == ALPHAVMS) && (FLOATTYPE == GFLOAT)
    ffswap2( (short *) values, nvals * 4);  /* swap pairs of bytes */

    /* convert from IEEE float format to VMS GFLOAT float format */
    sptr = (short *) values;
    for (ii = 0; ii < nvals; ii++, sptr += 4)
    {
        if (!dnan(*sptr) )  /* test for NaN or underflow */
            values[ii] *= 4.0;
    }

#elif BYTESWAPPED
    ffswap8(values, nvals);   /* reverse order of bytes in each value */
#endif

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffptbb(fitsfile *fptr,        /* I - FITS file pointer                 */
           LONGLONG firstrow,         /* I - starting row (1 = first row)      */
           LONGLONG firstchar,        /* I - starting byte in row (1=first)    */
           LONGLONG nchars,           /* I - number of bytes to write          */
           unsigned char *values, /* I - array of bytes to write           */
           int *status)           /* IO - error status                     */
/*
  write a consecutive string of bytes to an ascii or binary table.
  This will span multiple rows of the table if nchars + firstchar is
  greater than the length of a row.
*/
{
    LONGLONG bytepos, endrow, nrows;
    char message[81];

    if (*status > 0 || nchars <= 0)
        return(*status);

    else if (firstrow < 1)
        return(*status=BAD_ROW_NUM);

    else if (firstchar < 1)
        return(*status=BAD_ELEM_NUM);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart < 0) /* rescan header if data undefined */
        ffrdef(fptr, status);

    endrow = ((firstchar + nchars - 2) / (fptr->Fptr)->rowlength) + firstrow;

    /* check if we are writing beyond the current end of table */
    if (endrow > (fptr->Fptr)->numrows)
    {
        /* if there are more HDUs following the current one, or */
        /* if there is a data heap, then we must insert space */
        /* for the new rows.  */
        if ( !((fptr->Fptr)->lasthdu) || (fptr->Fptr)->heapsize > 0)
        {
            nrows = endrow - ((fptr->Fptr)->numrows);

            /* ffirow also updates the heap address and numrows */
            if (ffirow(fptr, (fptr->Fptr)->numrows, nrows, status) > 0)
            {
                 sprintf(message,
                 "ffptbb failed to add space for %.0f new rows in table.",
                         (double) nrows);
                 ffpmsg(message);
                 return(*status);
            }
        }
        else
        {
            /* manally update heap starting address */
            (fptr->Fptr)->heapstart += 
            ((LONGLONG)(endrow - (fptr->Fptr)->numrows) * 
                    (fptr->Fptr)->rowlength );

            (fptr->Fptr)->numrows = endrow; /* update number of rows */
        }
    }

    /* move the i/o pointer to the start of the sequence of characters */
    bytepos = (fptr->Fptr)->datastart +
              ((fptr->Fptr)->rowlength * (firstrow - 1)) +
              firstchar - 1;

    ffmbyt(fptr, bytepos, IGNORE_EOF, status);
    ffpbyt(fptr, nchars, values, status);  /* write the bytes */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpi1b(fitsfile *fptr, /* I - FITS file pointer                         */
           long nvals,     /* I - number of pixels in the values array      */
           long incre,     /* I - byte increment between pixels             */
           unsigned char *values, /* I - array of values to write           */
           int *status)    /* IO - error status                             */
/*
  put (write) the array of values to the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
    if (incre == 1)      /* write all the values at once (contiguous bytes) */

        ffpbyt(fptr, nvals, values, status);

    else         /* have to write each value individually (not contiguous ) */

        ffpbytoff(fptr, 1, nvals, incre - 1, values, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpi2b(fitsfile *fptr, /* I - FITS file pointer                         */
           long nvals,     /* I - number of pixels in the values array      */
           long incre,     /* I - byte increment between pixels             */
           short *values,  /* I - array of values to write                  */
           int *status)    /* IO - error status                             */
/*
  put (write) the array of values to the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
#if BYTESWAPPED
    ffswap2(values, nvals);  /* reverse order of bytes in each value */
#endif

    if (incre == 2)      /* write all the values at once (contiguous bytes) */

        ffpbyt(fptr, nvals * 2, values, status);

    else         /* have to write each value individually (not contiguous ) */

        ffpbytoff(fptr, 2, nvals, incre - 2, values, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpi4b(fitsfile *fptr, /* I - FITS file pointer                         */
           long nvals,     /* I - number of pixels in the values array      */
           long incre,     /* I - byte increment between pixels             */
           INT32BIT *values, /* I - array of values to write                */
           int *status)    /* IO - error status                             */
/*
  put (write) the array of values to the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
#if BYTESWAPPED
    ffswap4(values, nvals);    /* reverse order of bytes in each value */
#endif

    if (incre == 4)      /* write all the values at once (contiguous bytes) */

        ffpbyt(fptr, nvals * 4, values, status);

    else         /* have to write each value individually (not contiguous ) */

        ffpbytoff(fptr, 4, nvals, incre - 4, values, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpi8b(fitsfile *fptr, /* I - FITS file pointer                         */
           long nvals,     /* I - number of pixels in the values array      */
           long incre,     /* I - byte increment between pixels             */
           long *values,   /* I - array of values to write                */
           int *status)    /* IO - error status                             */
/*
  put (write) the array of values to the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.

  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  This routine writes 'nvals' 8-byte integers from 'values'.
  This works both on platforms that have sizeof(long) = 64, and 32,
  as long as 'values' has been allocated to large enough to hold
  8 * nvals bytes of data.
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
{
#if BYTESWAPPED
    ffswap8((double *) values, nvals);    /* reverse bytes in each value */
#endif

    if (incre == 8)      /* write all the values at once (contiguous bytes) */

        ffpbyt(fptr, nvals * 8, values, status);

    else         /* have to write each value individually (not contiguous ) */

        ffpbytoff(fptr, 8, nvals, incre - 8, values, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpr4b(fitsfile *fptr, /* I - FITS file pointer                         */
           long nvals,     /* I - number of pixels in the values array      */
           long incre,     /* I - byte increment between pixels             */
           float *values,  /* I - array of values to write                  */
           int *status)    /* IO - error status                             */
/*
  put (write) the array of values to the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
#if MACHINE == VAXVMS
    long ii;

    ii = nvals;                      /* call VAX macro routine to convert */
    ieevpr(values, values, &ii);     /* from F float -> IEEE float        */

#elif (MACHINE == ALPHAVMS) && (FLOATTYPE == GFLOAT)
    long ii;

    /* convert from VMS FFLOAT float format to IEEE float format */
    for (ii = 0; ii < nvals; ii++)
        values[ii] *= 0.25;

    ffswap2( (short *) values, nvals * 2);  /* swap pairs of bytes */

#elif BYTESWAPPED
    ffswap4((INT32BIT *) values, nvals); /* reverse order of bytes in values */
#endif

    if (incre == 4)      /* write all the values at once (contiguous bytes) */

        ffpbyt(fptr, nvals * 4, values, status);

    else         /* have to write each value individually (not contiguous ) */

        ffpbytoff(fptr, 4, nvals, incre - 4, values, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpr8b(fitsfile *fptr, /* I - FITS file pointer                         */
           long nvals,     /* I - number of pixels in the values array      */
           long incre,     /* I - byte increment between pixels             */
           double *values, /* I - array of values to write                  */
           int *status)    /* IO - error status                             */
/*
  put (write) the array of values to the FITS file, doing machine dependent
  format conversion (e.g. byte-swapping) if necessary.
*/
{
#if MACHINE == VAXVMS
    long ii;

    ii = nvals;                      /* call VAX macro routine to convert */
    ieevpd(values, values, &ii);     /* from D float -> IEEE float        */

#elif (MACHINE == ALPHAVMS) && (FLOATTYPE == GFLOAT)
    long ii;

    /* convert from VMS GFLOAT float format to IEEE float format */
    for (ii = 0; ii < nvals; ii++)
        values[ii] *= 0.25;

    ffswap2( (short *) values, nvals * 4);  /* swap pairs of bytes */

#elif BYTESWAPPED
    ffswap8(values, nvals); /* reverse order of bytes in each value */
#endif

    if (incre == 8)      /* write all the values at once (contiguous bytes) */

        ffpbyt(fptr, nvals * 8, values, status);

    else         /* have to write each value individually (not contiguous ) */

        ffpbytoff(fptr, 8, nvals, incre - 8, values, status);

    return(*status);
}

