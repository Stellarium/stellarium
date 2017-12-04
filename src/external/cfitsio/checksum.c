/*  This file, checksum.c, contains the checksum-related routines in the   */
/*  FITSIO library.                                                        */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include "fitsio2.h"
/*------------------------------------------------------------------------*/
int ffcsum(fitsfile *fptr,      /* I - FITS file pointer                  */
           long nrec,           /* I - number of 2880-byte blocks to sum  */
           unsigned long *sum,  /* IO - accumulated checksum              */
           int *status)         /* IO - error status                      */
/*
    Calculate a 32-bit 1's complement checksum of the FITS 2880-byte blocks.
    This routine is based on the C algorithm developed by Rob
    Seaman at NOAO that was presented at the 1994 ADASS conference,  
    published in the Astronomical Society of the Pacific Conference Series.
    This uses a 32-bit 1's complement checksum in which the overflow bits
    are permuted back into the sum and therefore all bit positions are
    sampled evenly. 
*/
{
    long ii, jj;
    unsigned short sbuf[1440];
    unsigned long hi, lo, hicarry, locarry;

    if (*status > 0)
        return(*status);
  /*
    Sum the specified number of FITS 2880-byte records.  This assumes that
    the FITSIO file pointer points to the start of the records to be summed.
    Read each FITS block as 1440 short values (do byte swapping if needed).
  */
    for (jj = 0; jj < nrec; jj++)
    {
      ffgbyt(fptr, 2880, sbuf, status);

#if BYTESWAPPED

      ffswap2( (short *)sbuf, 1440); /* reverse order of bytes in each value */

#endif

      hi = (*sum >> 16);
      lo = *sum & 0xFFFF;

      for (ii = 0; ii < 1440; ii += 2)
      {
        hi += sbuf[ii];
        lo += sbuf[ii+1];
      }

      hicarry = hi >> 16;    /* fold carry bits in */
      locarry = lo >> 16;

      while (hicarry | locarry)
      {
        hi = (hi & 0xFFFF) + locarry;
        lo = (lo & 0xFFFF) + hicarry;
        hicarry = hi >> 16;
        locarry = lo >> 16;
      }

      *sum = (hi << 16) + lo;
    }
    return(*status);
}
/*-------------------------------------------------------------------------*/
void ffesum(unsigned long sum,  /* I - accumulated checksum                */
           int complm,          /* I - = 1 to encode complement of the sum */
           char *ascii)         /* O - 16-char ASCII encoded checksum      */
/*
    encode the 32 bit checksum by converting every 
    2 bits of each byte into an ASCII character (32 bit word encoded 
    as 16 character string).   Only ASCII letters and digits are used
    to encode the values (no ASCII punctuation characters).

    If complm=TRUE, then the complement of the sum will be encoded.

    This routine is based on the C algorithm developed by Rob
    Seaman at NOAO that was presented at the 1994 ADASS conference,
    published in the Astronomical Society of the Pacific Conference Series.
*/
{
    unsigned int exclude[13] = { 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
                                       0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60 };
    unsigned long mask[4] = { 0xff000000, 0xff0000, 0xff00, 0xff  };

    int offset = 0x30;     /* ASCII 0 (zero) */

    unsigned long value;
    int byte, quotient, remainder, ch[4], check, ii, jj, kk;
    char asc[32];

    if (complm)
        value = 0xFFFFFFFF - sum;   /* complement each bit of the value */
    else
        value = sum;

    for (ii = 0; ii < 4; ii++)
    {
        byte = (value & mask[ii]) >> (24 - (8 * ii));
        quotient = byte / 4 + offset;
        remainder = byte % 4;
        for (jj = 0; jj < 4; jj++)
            ch[jj] = quotient;

        ch[0] += remainder;

        for (check = 1; check;)   /* avoid ASCII  punctuation */
            for (check = 0, kk = 0; kk < 13; kk++)
                for (jj = 0; jj < 4; jj += 2)
                    if ((unsigned char) ch[jj] == exclude[kk] ||
                        (unsigned char) ch[jj+1] == exclude[kk])
                    {
                        ch[jj]++;
                        ch[jj+1]--;
                        check++;
                    }

        for (jj = 0; jj < 4; jj++)        /* assign the bytes */
            asc[4*jj+ii] = ch[jj];
    }

    for (ii = 0; ii < 16; ii++)       /* shift the bytes 1 to the right */
        ascii[ii] = asc[(ii+15)%16];

    ascii[16] = '\0';
}
/*-------------------------------------------------------------------------*/
unsigned long ffdsum(char *ascii,  /* I - 16-char ASCII encoded checksum   */
                     int complm,   /* I - =1 to decode complement of the   */
                     unsigned long *sum)  /* O - 32-bit checksum           */
/*
    decode the 16-char ASCII encoded checksum into an unsigned 32-bit long.
    If complm=TRUE, then the complement of the sum will be decoded.

    This routine is based on the C algorithm developed by Rob
    Seaman at NOAO that was presented at the 1994 ADASS conference,
    published in the Astronomical Society of the Pacific Conference Series.
*/
{
    char cbuf[16];
    unsigned long hi = 0, lo = 0, hicarry, locarry;
    int ii;

    /* remove the permuted FITS byte alignment and the ASCII 0 offset */
    for (ii = 0; ii < 16; ii++)
    {
        cbuf[ii] = ascii[(ii+1)%16];
        cbuf[ii] -= 0x30;
    }

    for (ii = 0; ii < 16; ii += 4)
    {
        hi += (cbuf[ii]   << 8) + cbuf[ii+1];
        lo += (cbuf[ii+2] << 8) + cbuf[ii+3];
    }

    hicarry = hi >> 16;
    locarry = lo >> 16;
    while (hicarry || locarry)
    {
        hi = (hi & 0xFFFF) + locarry;
        lo = (lo & 0xFFFF) + hicarry;
        hicarry = hi >> 16;
        locarry = lo >> 16;
    }

    *sum = (hi << 16) + lo;
    if (complm)
        *sum = 0xFFFFFFFF - *sum;   /* complement each bit of the value */

    return(*sum);
}
/*------------------------------------------------------------------------*/
int ffpcks(fitsfile *fptr,      /* I - FITS file pointer                  */
           int *status)         /* IO - error status                      */
/*
   Create or update the checksum keywords in the CHDU.  These keywords
   provide a checksum verification of the FITS HDU based on the ASCII
   coded 1's complement checksum algorithm developed by Rob Seaman at NOAO.
*/
{
    char datestr[20], checksum[FLEN_VALUE], datasum[FLEN_VALUE];
    char  comm[FLEN_COMMENT], chkcomm[FLEN_COMMENT], datacomm[FLEN_COMMENT];
    int tstatus;
    long nrec;
    LONGLONG headstart, datastart, dataend;
    unsigned long dsum, olddsum, sum;
    double tdouble;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* generate current date string and construct the keyword comments */
    ffgstm(datestr, NULL, status);
    strcpy(chkcomm, "HDU checksum updated ");
    strcat(chkcomm, datestr);
    strcpy(datacomm, "data unit checksum updated ");
    strcat(datacomm, datestr);

    /* write the CHECKSUM keyword if it does not exist */
    tstatus = *status;
    if (ffgkys(fptr, "CHECKSUM", checksum, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        strcpy(checksum, "0000000000000000");
        ffpkys(fptr, "CHECKSUM", checksum, chkcomm, status);
    }

    /* write the DATASUM keyword if it does not exist */
    tstatus = *status;
    if (ffgkys(fptr, "DATASUM", datasum, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        olddsum = 0;
        ffpkys(fptr, "DATASUM", "         0", datacomm, status);

        /* set the CHECKSUM keyword as undefined, if it isn't already */
        if (strcmp(checksum, "0000000000000000") )
        {
            strcpy(checksum, "0000000000000000");
            ffmkys(fptr, "CHECKSUM", checksum, chkcomm, status);
        }
    }
    else
    {
        /* decode the datasum into an unsigned long variable */

        /* olddsum = strtoul(datasum, 0, 10); doesn't work on SUN OS */

        tdouble = atof(datasum);
        olddsum = (unsigned long) tdouble;
    }

    /* close header: rewrite END keyword and following blank fill */
    /* and re-read the required keywords to determine the structure */
    if (ffrdef(fptr, status) > 0)
        return(*status);

    if ((fptr->Fptr)->heapsize > 0)
         ffuptf(fptr, status);  /* update the variable length TFORM values */

    /* write the correct data fill values, if they are not already correct */
    if (ffpdfl(fptr, status) > 0)
        return(*status);

    /* calc size of data unit, in FITS 2880-byte blocks */
    if (ffghadll(fptr, &headstart, &datastart, &dataend, status) > 0)
        return(*status);

    nrec = (long) ((dataend - datastart) / 2880);
    dsum = 0;

    if (nrec > 0)
    {
        /* accumulate the 32-bit 1's complement checksum */
        ffmbyt(fptr, datastart, REPORT_EOF, status);
        if (ffcsum(fptr, nrec, &dsum, status) > 0)
            return(*status);
    }

    if (dsum != olddsum)
    {
        /* update the DATASUM keyword with the correct value */ 
        sprintf(datasum, "%lu", dsum);
        ffmkys(fptr, "DATASUM", datasum, datacomm, status);

        /* set the CHECKSUM keyword as undefined, if it isn't already */
        if (strcmp(checksum, "0000000000000000") )
        {
            strcpy(checksum, "0000000000000000");
            ffmkys(fptr, "CHECKSUM", checksum, chkcomm, status);
        }
    }        

    if (strcmp(checksum, "0000000000000000") )
    {
        /* check if CHECKSUM is still OK; move to the start of the header */
        ffmbyt(fptr, headstart, REPORT_EOF, status);

        /* accumulate the header checksum into the previous data checksum */
        nrec = (long) ((datastart - headstart) / 2880);
        sum = dsum;
        if (ffcsum(fptr, nrec, &sum, status) > 0)
            return(*status);

        if (sum == 0 || sum == 0xFFFFFFFF)
           return(*status);            /* CHECKSUM is correct */

        /* Zero the CHECKSUM and recompute the new value */
        ffmkys(fptr, "CHECKSUM", "0000000000000000", chkcomm, status);
    }

    /* move to the start of the header */
    ffmbyt(fptr, headstart, REPORT_EOF, status);

    /* accumulate the header checksum into the previous data checksum */
    nrec = (long) ((datastart - headstart) / 2880);
    sum = dsum;
    if (ffcsum(fptr, nrec, &sum, status) > 0)
           return(*status);

    /* encode the COMPLEMENT of the checksum into a 16-character string */
    ffesum(sum, TRUE, checksum);

    /* update the CHECKSUM keyword value with the new string */
    ffmkys(fptr, "CHECKSUM", checksum, "&", status);

    return(*status);
}
/*------------------------------------------------------------------------*/
int ffupck(fitsfile *fptr,      /* I - FITS file pointer                  */
           int *status)         /* IO - error status                      */
/*
   Update the CHECKSUM keyword value.  This assumes that the DATASUM
   keyword exists and has the correct value.
*/
{
    char datestr[20], chkcomm[FLEN_COMMENT], comm[FLEN_COMMENT];
    char checksum[FLEN_VALUE], datasum[FLEN_VALUE];
    int tstatus;
    long nrec;
    LONGLONG headstart, datastart, dataend;
    unsigned long sum, dsum;
    double tdouble;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* generate current date string and construct the keyword comments */
    ffgstm(datestr, NULL, status);
    strcpy(chkcomm, "HDU checksum updated ");
    strcat(chkcomm, datestr);

    /* get the DATASUM keyword and convert it to a unsigned long */
    if (ffgkys(fptr, "DATASUM", datasum, comm, status) == KEY_NO_EXIST)
    {
        ffpmsg("DATASUM keyword not found (ffupck");
        return(*status);
    }

    tdouble = atof(datasum); /* read as a double as a workaround */
    dsum = (unsigned long) tdouble;

    /* get size of the HDU */
    if (ffghadll(fptr, &headstart, &datastart, &dataend, status) > 0)
        return(*status);

    /* get the checksum keyword, if it exists */
    tstatus = *status;
    if (ffgkys(fptr, "CHECKSUM", checksum, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        strcpy(checksum, "0000000000000000");
        ffpkys(fptr, "CHECKSUM", checksum, chkcomm, status);
    }
    else
    {
        /* check if CHECKSUM is still OK */
        /* rewrite END keyword and following blank fill */
        if (ffwend(fptr, status) > 0)
            return(*status);

        /* move to the start of the header */
        ffmbyt(fptr, headstart, REPORT_EOF, status);

        /* accumulate the header checksum into the previous data checksum */
        nrec = (long) ((datastart - headstart) / 2880);
        sum = dsum;
        if (ffcsum(fptr, nrec, &sum, status) > 0)
           return(*status);

        if (sum == 0 || sum == 0xFFFFFFFF)
           return(*status);    /* CHECKSUM is already correct */

        /* Zero the CHECKSUM and recompute the new value */
        ffmkys(fptr, "CHECKSUM", "0000000000000000", chkcomm, status);
    }

    /* move to the start of the header */
    ffmbyt(fptr, headstart, REPORT_EOF, status);

    /* accumulate the header checksum into the previous data checksum */
    nrec = (long) ((datastart - headstart) / 2880);
    sum = dsum;
    if (ffcsum(fptr, nrec, &sum, status) > 0)
           return(*status);

    /* encode the COMPLEMENT of the checksum into a 16-character string */
    ffesum(sum, TRUE, checksum);

    /* update the CHECKSUM keyword value with the new string */
    ffmkys(fptr, "CHECKSUM", checksum, "&", status);

    return(*status);
}
/*------------------------------------------------------------------------*/
int ffvcks(fitsfile *fptr,      /* I - FITS file pointer                  */
           int *datastatus,     /* O - data checksum status               */
           int *hdustatus,      /* O - hdu checksum status                */
                                /*     1  verification is correct         */
                                /*     0  checksum keyword is not present */
                                /*    -1 verification not correct         */
           int *status)         /* IO - error status                      */
/*
    Verify the HDU by comparing the value of the computed checksums against
    the values of the DATASUM and CHECKSUM keywords if they are present.
*/
{
    int tstatus;
    double tdouble;
    unsigned long datasum, hdusum, olddatasum;
    char chksum[FLEN_VALUE], comm[FLEN_COMMENT];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    *datastatus = -1;
    *hdustatus  = -1;

    tstatus = *status;
    if (ffgkys(fptr, "CHECKSUM", chksum, comm, status) == KEY_NO_EXIST)
    {
        *hdustatus = 0;             /* CHECKSUM keyword does not exist */
        *status = tstatus;
    }
    if (chksum[0] == '\0')
        *hdustatus = 0;    /* all blank checksum means it is undefined */

    if (ffgkys(fptr, "DATASUM", chksum, comm, status) == KEY_NO_EXIST)
    {
        *datastatus = 0;            /* DATASUM keyword does not exist */
        *status = tstatus;
    }
    if (chksum[0] == '\0')
        *datastatus = 0;    /* all blank checksum means it is undefined */

    if ( *status > 0 || (!(*hdustatus) && !(*datastatus)) )
        return(*status);            /* return if neither keywords exist */

    /* convert string to unsigned long */

    /* olddatasum = strtoul(chksum, 0, 10);  doesn't work w/ gcc on SUN OS */
    /* sscanf(chksum, "%u", &olddatasum);   doesn't work w/ cc on VAX/VMS */

    tdouble = atof(chksum); /* read as a double as a workaround */
    olddatasum = (unsigned long) tdouble;

    /*  calculate the data checksum and the HDU checksum */
    if (ffgcks(fptr, &datasum, &hdusum, status) > 0)
        return(*status);

    if (*datastatus)
        if (datasum == olddatasum)
            *datastatus = 1;

    if (*hdustatus)
        if (hdusum == 0 || hdusum == 0xFFFFFFFF)
            *hdustatus = 1;

    return(*status);
}
/*------------------------------------------------------------------------*/
int ffgcks(fitsfile *fptr,           /* I - FITS file pointer             */
           unsigned long *datasum,   /* O - data checksum                 */
           unsigned long *hdusum,    /* O - hdu checksum                  */
           int *status)              /* IO - error status                 */

    /* calculate the checksums of the data unit and the total HDU */
{
    long nrec;
    LONGLONG headstart, datastart, dataend;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* get size of the HDU */
    if (ffghadll(fptr, &headstart, &datastart, &dataend, status) > 0)
        return(*status);

    nrec = (long) ((dataend - datastart) / 2880);

    *datasum = 0;

    if (nrec > 0)
    {
        /* accumulate the 32-bit 1's complement checksum */
        ffmbyt(fptr, datastart, REPORT_EOF, status);
        if (ffcsum(fptr, nrec, datasum, status) > 0)
            return(*status);
    }

    /* move to the start of the header and calc. size of header */
    ffmbyt(fptr, headstart, REPORT_EOF, status);
    nrec = (long) ((datastart - headstart) / 2880);

    /* accumulate the header checksum into the previous data checksum */
    *hdusum = *datasum;
    ffcsum(fptr, nrec, hdusum, status);

    return(*status);
}

