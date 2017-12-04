/*  This file, edithdu.c, contains the FITSIO routines related to       */
/*  copying, inserting, or deleting HDUs in a FITS file                 */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include "fitsio2.h"
/*--------------------------------------------------------------------------*/
int ffcopy(fitsfile *infptr,    /* I - FITS file pointer to input file  */
           fitsfile *outfptr,   /* I - FITS file pointer to output file */
           int morekeys,        /* I - reserve space in output header   */
           int *status)         /* IO - error status     */
/*
  copy the CHDU from infptr to the CHDU of outfptr.
  This will also allocate space in the output header for MOREKY keywords
*/
{
    int nspace;
    
    if (*status > 0)
        return(*status);

    if (infptr == outfptr)
        return(*status = SAME_FILE);

    if (ffcphd(infptr, outfptr, status) > 0)  /* copy the header keywords */
       return(*status);

    if (morekeys > 0) {
      ffhdef(outfptr, morekeys, status); /* reserve space for more keywords */

    } else {
        if (ffghsp(infptr, NULL, &nspace, status) > 0) /* get existing space */
            return(*status);

        if (nspace > 0) {
            ffhdef(outfptr, nspace, status); /* preserve same amount of space */
            if (nspace >= 35) {  

	        /* There is at least 1 full empty FITS block in the header. */
	        /* Physically write the END keyword at the beginning of the */
	        /* last block to preserve this extra space now rather than */
		/* later.  This is needed by the stream: driver which cannot */
		/* seek back to the header to write the END keyword later. */

	        ffwend(outfptr, status);
            }
        }
    }

    ffcpdt(infptr, outfptr, status);  /* now copy the data unit */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcpfl(fitsfile *infptr,    /* I - FITS file pointer to input file  */
           fitsfile *outfptr,   /* I - FITS file pointer to output file */
           int previous,        /* I - copy any previous HDUs?   */
           int current,         /* I - copy the current HDU?     */
           int following,       /* I - copy any following HDUs?   */
           int *status)         /* IO - error status     */
/*
  copy all or part of the input file to the output file.
*/
{
    int hdunum, ii;

    if (*status > 0)
        return(*status);

    if (infptr == outfptr)
        return(*status = SAME_FILE);

    ffghdn(infptr, &hdunum);

    if (previous) {   /* copy any previous HDUs */
        for (ii=1; ii < hdunum; ii++) {
            ffmahd(infptr, ii, NULL, status);
            ffcopy(infptr, outfptr, 0, status);
        }
    }

    if (current && (*status <= 0) ) {  /* copy current HDU */
        ffmahd(infptr, hdunum, NULL, status);
        ffcopy(infptr, outfptr, 0, status);
    }

    if (following && (*status <= 0) ) { /* copy any remaining HDUs */
        ii = hdunum + 1;
        while (1)
        { 
            if (ffmahd(infptr, ii, NULL, status) ) {
                 /* reset expected end of file status */
                 if (*status == END_OF_FILE)
                    *status = 0;
                 break;
            }

            if (ffcopy(infptr, outfptr, 0, status))
                 break;  /* quit on unexpected error */

            ii++;
        }
    }

    ffmahd(infptr, hdunum, NULL, status);  /* restore initial position */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcphd(fitsfile *infptr,    /* I - FITS file pointer to input file  */
           fitsfile *outfptr,   /* I - FITS file pointer to output file */
           int *status)         /* IO - error status     */
/*
  copy the header keywords from infptr to outfptr.
*/
{
    int nkeys, ii, inPrim = 0, outPrim = 0;
    long naxis, naxes[1];
    char *card, comm[FLEN_COMMENT];
    char *tmpbuff;

    if (*status > 0)
        return(*status);

    if (infptr == outfptr)
        return(*status = SAME_FILE);

    /* set the input pointer to the correct HDU */
    if (infptr->HDUposition != (infptr->Fptr)->curhdu)
        ffmahd(infptr, (infptr->HDUposition) + 1, NULL, status);

    if (ffghsp(infptr, &nkeys, NULL, status) > 0) /* get no. of keywords */
        return(*status);

    /* create a memory buffer to hold the header records */
    tmpbuff = (char*) malloc(nkeys*FLEN_CARD*sizeof(char));
    if (!tmpbuff)
        return(*status = MEMORY_ALLOCATION);

    /* read all of the header records in the input HDU */
    for (ii = 0; ii < nkeys; ii++)
      ffgrec(infptr, ii+1, tmpbuff + (ii * FLEN_CARD), status);

    if (infptr->HDUposition == 0)  /* set flag if this is the Primary HDU */
       inPrim = 1;

    /* if input is an image hdu, get the number of axes */
    naxis = -1;   /* negative if HDU is a table */
    if ((infptr->Fptr)->hdutype == IMAGE_HDU)
        ffgkyj(infptr, "NAXIS", &naxis, NULL, status);

    /* set the output pointer to the correct HDU */
    if (outfptr->HDUposition != (outfptr->Fptr)->curhdu)
        ffmahd(outfptr, (outfptr->HDUposition) + 1, NULL, status);

    /* check if output header is empty; if not create new empty HDU */
    if ((outfptr->Fptr)->headend !=
        (outfptr->Fptr)->headstart[(outfptr->Fptr)->curhdu] )
           ffcrhd(outfptr, status);   

    if (outfptr->HDUposition == 0)
    {
        if (naxis < 0)
        {
            /* the input HDU is a table, so we have to create */
            /* a dummy Primary array before copying it to the output */
            ffcrim(outfptr, 8, 0, naxes, status);
            ffcrhd(outfptr, status); /* create new empty HDU */
        }
        else
        {
            /* set flag that this is the Primary HDU */
            outPrim = 1;
        }
    }

    if (*status > 0)  /* check for errors before proceeding */
    {
        free(tmpbuff);
        return(*status);
    }
    if ( inPrim == 1 && outPrim == 0 )
    {
        /* copying from primary array to image extension */
        strcpy(comm, "IMAGE extension");
        ffpkys(outfptr, "XTENSION", "IMAGE", comm, status);

        /* copy BITPIX through NAXISn keywords */
        for (ii = 1; ii < 3 + naxis; ii++)
        {
            card = tmpbuff + (ii * FLEN_CARD);
            ffprec(outfptr, card, status);
        }

        strcpy(comm, "number of random group parameters");
        ffpkyj(outfptr, "PCOUNT", 0, comm, status);
  
        strcpy(comm, "number of random groups");
        ffpkyj(outfptr, "GCOUNT", 1, comm, status);


        /* copy remaining keywords, excluding EXTEND, and reference COMMENT keywords */
        for (ii = 3 + naxis ; ii < nkeys; ii++)
        {
            card = tmpbuff+(ii * FLEN_CARD);
            if (FSTRNCMP(card, "EXTEND  ", 8) &&
                FSTRNCMP(card, "COMMENT   FITS (Flexible Image Transport System) format is", 58) && 
                FSTRNCMP(card, "COMMENT   and Astrophysics', volume 376, page 3", 47) )
            {
                 ffprec(outfptr, card, status);
            }
        }
    }
    else if ( inPrim == 0 && outPrim == 1 )
    {
        /* copying between image extension and primary array */
        strcpy(comm, "file does conform to FITS standard");
        ffpkyl(outfptr, "SIMPLE", TRUE, comm, status);

        /* copy BITPIX through NAXISn keywords */
        for (ii = 1; ii < 3 + naxis; ii++)
        {
            card = tmpbuff + (ii * FLEN_CARD);
            ffprec(outfptr, card, status);
        }

        /* add the EXTEND keyword */
        strcpy(comm, "FITS dataset may contain extensions");
        ffpkyl(outfptr, "EXTEND", TRUE, comm, status);

      /* write standard block of self-documentating comments */
      ffprec(outfptr,
      "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy",
      status);
      ffprec(outfptr,
      "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H",
      status);

        /* copy remaining keywords, excluding pcount, gcount */
        for (ii = 3 + naxis; ii < nkeys; ii++)
        {
            card = tmpbuff+(ii * FLEN_CARD);
            if (FSTRNCMP(card, "PCOUNT  ", 8) && FSTRNCMP(card, "GCOUNT  ", 8))
            {
                 ffprec(outfptr, card, status);
            }
        }
    }
    else
    {
        /* input and output HDUs are same type; simply copy all keywords */
        for (ii = 0; ii < nkeys; ii++)
        {
            card = tmpbuff+(ii * FLEN_CARD);
            ffprec(outfptr, card, status);
        }
    }

    free(tmpbuff);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcpdt(fitsfile *infptr,    /* I - FITS file pointer to input file  */
           fitsfile *outfptr,   /* I - FITS file pointer to output file */
           int *status)         /* IO - error status     */
{
/*
  copy the data unit from the CHDU of infptr to the CHDU of outfptr. 
  This will overwrite any data already in the outfptr CHDU.
*/
    long nb, ii;
    LONGLONG indatastart, indataend, outdatastart;
    char buffer[2880];

    if (*status > 0)
        return(*status);

    if (infptr == outfptr)
        return(*status = SAME_FILE);

    ffghadll(infptr,  NULL, &indatastart, &indataend, status);
    ffghadll(outfptr, NULL, &outdatastart, NULL, status);

    /* Calculate the number of blocks to be copied  */
    nb = (long) ((indataend - indatastart) / 2880);

    if (nb > 0)
    {
      if (infptr->Fptr == outfptr->Fptr)
      {
        /* copying between 2 HDUs in the SAME file */
        for (ii = 0; ii < nb; ii++)
        {
            ffmbyt(infptr,  indatastart,  REPORT_EOF, status);
            ffgbyt(infptr,  2880L, buffer, status); /* read input block */

            ffmbyt(outfptr, outdatastart, IGNORE_EOF, status);
            ffpbyt(outfptr, 2880L, buffer, status); /* write output block */

            indatastart  += 2880; /* move address */
            outdatastart += 2880; /* move address */
        }
      }
      else
      {
        /* copying between HDUs in separate files */
        /* move to the initial copy position in each of the files */
        ffmbyt(infptr,  indatastart,  REPORT_EOF, status);
        ffmbyt(outfptr, outdatastart, IGNORE_EOF, status);

        for (ii = 0; ii < nb; ii++)
        {
            ffgbyt(infptr,  2880L, buffer, status); /* read input block */
            ffpbyt(outfptr, 2880L, buffer, status); /* write output block */
        }
      }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffwrhdu(fitsfile *infptr,    /* I - FITS file pointer to input file  */
            FILE *outstream,     /* I - stream to write HDU to */
            int *status)         /* IO - error status     */
{
/*
  write the data unit from the CHDU of infptr to the output file stream
*/
    long nb, ii;
    LONGLONG hdustart, hduend;
    char buffer[2880];

    if (*status > 0)
        return(*status);

    ffghadll(infptr, &hdustart,  NULL, &hduend, status);

    nb = (long) ((hduend - hdustart) / 2880);  /* number of blocks to copy */

    if (nb > 0)
    {

        /* move to the start of the HDU */
        ffmbyt(infptr,  hdustart,  REPORT_EOF, status);

        for (ii = 0; ii < nb; ii++)
        {
            ffgbyt(infptr,  2880L, buffer, status); /* read input block */
            fwrite(buffer, 1, 2880, outstream ); /* write to output stream */
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffiimg(fitsfile *fptr,      /* I - FITS file pointer           */
           int bitpix,          /* I - bits per pixel              */
           int naxis,           /* I - number of axes in the array */
           long *naxes,         /* I - size of each axis           */
           int *status)         /* IO - error status               */
/*
  insert an IMAGE extension following the current HDU 
*/
{
    LONGLONG tnaxes[99];
    int ii;
    
    if (*status > 0)
        return(*status);

    if (naxis > 99) {
        ffpmsg("NAXIS value is too large (>99)  (ffiimg)");
	return(*status = 212);
    }

    for (ii = 0; (ii < naxis); ii++)
       tnaxes[ii] = naxes[ii];
       
    ffiimgll(fptr, bitpix, naxis, tnaxes, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffiimgll(fitsfile *fptr,    /* I - FITS file pointer           */
           int bitpix,          /* I - bits per pixel              */
           int naxis,           /* I - number of axes in the array */
           LONGLONG *naxes,     /* I - size of each axis           */
           int *status)         /* IO - error status               */
/*
  insert an IMAGE extension following the current HDU 
*/
{
    int bytlen, nexthdu, maxhdu, ii, onaxis;
    long nblocks;
    LONGLONG npixels, newstart, datasize;
    char errmsg[FLEN_ERRMSG], card[FLEN_CARD], naxiskey[FLEN_KEYWORD];

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    maxhdu = (fptr->Fptr)->maxhdu;

    if (*status != PREPEND_PRIMARY)
    {
      /* if the current header is completely empty ...  */
      if (( (fptr->Fptr)->headend == (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu])
        /* or, if we are at the end of the file, ... */
      ||  ( (((fptr->Fptr)->curhdu) == maxhdu ) &&
       ((fptr->Fptr)->headstart[maxhdu + 1] >= (fptr->Fptr)->logfilesize ) ) )
      {
        /* then simply append new image extension */
        ffcrimll(fptr, bitpix, naxis, naxes, status);
        return(*status);
      }
    }

    if (bitpix == 8)
        bytlen = 1;
    else if (bitpix == 16)
        bytlen = 2;
    else if (bitpix == 32 || bitpix == -32)
        bytlen = 4;
    else if (bitpix == 64 || bitpix == -64)
        bytlen = 8;
    else
    {
        sprintf(errmsg,
        "Illegal value for BITPIX keyword: %d", bitpix);
        ffpmsg(errmsg);
        return(*status = BAD_BITPIX);  /* illegal bitpix value */
    }
    if (naxis < 0 || naxis > 999)
    {
        sprintf(errmsg,
        "Illegal value for NAXIS keyword: %d", naxis);
        ffpmsg(errmsg);
        return(*status = BAD_NAXIS);
    }

    for (ii = 0; ii < naxis; ii++)
    {
        if (naxes[ii] < 0)
        {
            sprintf(errmsg,
            "Illegal value for NAXIS%d keyword: %ld", ii + 1,  (long) naxes[ii]);
            ffpmsg(errmsg);
            return(*status = BAD_NAXES);
        }
    }

    /* calculate number of pixels in the image */
    if (naxis == 0)
        npixels = 0;
    else 
        npixels = naxes[0];

    for (ii = 1; ii < naxis; ii++)
        npixels = npixels * naxes[ii];

    datasize = npixels * bytlen;          /* size of image in bytes */
    nblocks = (long) (((datasize + 2879) / 2880) + 1);  /* +1 for the header */

    if ((fptr->Fptr)->writemode == READWRITE) /* must have write access */
    {   /* close the CHDU */
        ffrdef(fptr, status);  /* scan header to redefine structure */
        ffpdfl(fptr, status);  /* insure correct data file values */
    }
    else
        return(*status = READONLY_FILE);

    if (*status == PREPEND_PRIMARY)
    {
        /* inserting a new primary array; the current primary */
        /* array must be transformed into an image extension. */

        *status = 0;   
        ffmahd(fptr, 1, NULL, status);  /* move to the primary array */

        ffgidm(fptr, &onaxis, status);
        if (onaxis > 0)
            ffkeyn("NAXIS",onaxis, naxiskey, status);
        else
            strcpy(naxiskey, "NAXIS");

        ffgcrd(fptr, naxiskey, card, status);  /* read last NAXIS keyword */
        
        ffikyj(fptr, "PCOUNT", 0, "required keyword", status); /* add PCOUNT and */
        ffikyj(fptr, "GCOUNT", 1, "required keyword", status); /* GCOUNT keywords */

        if (*status > 0)
            return(*status);

        if (ffdkey(fptr, "EXTEND", status) ) /* delete the EXTEND keyword */
            *status = 0;

        /* redefine internal structure for this HDU */
        ffrdef(fptr, status);


        /* insert space for the primary array */
        if (ffiblk(fptr, nblocks, -1, status) > 0)  /* insert the blocks */
            return(*status);

        nexthdu = 0;  /* number of the new hdu */
        newstart = 0; /* starting addr of HDU */
    }
    else
    {
        nexthdu = ((fptr->Fptr)->curhdu) + 1; /* number of the next (new) hdu */
        newstart = (fptr->Fptr)->headstart[nexthdu]; /* save starting addr of HDU */

        (fptr->Fptr)->hdutype = IMAGE_HDU;  /* so that correct fill value is used */
        /* ffiblk also increments headstart for all following HDUs */
        if (ffiblk(fptr, nblocks, 1, status) > 0)  /* insert the blocks */
            return(*status);
    }

    ((fptr->Fptr)->maxhdu)++;      /* increment known number of HDUs in the file */
    for (ii = (fptr->Fptr)->maxhdu; ii > (fptr->Fptr)->curhdu; ii--)
        (fptr->Fptr)->headstart[ii + 1] = (fptr->Fptr)->headstart[ii]; /* incre start addr */

    if (nexthdu == 0)
       (fptr->Fptr)->headstart[1] = nblocks * 2880; /* start of the old Primary array */

    (fptr->Fptr)->headstart[nexthdu] = newstart; /* set starting addr of HDU */

    /* set default parameters for this new empty HDU */
    (fptr->Fptr)->curhdu = nexthdu;   /* we are now located at the next HDU */
    fptr->HDUposition = nexthdu;      /* we are now located at the next HDU */
    (fptr->Fptr)->nextkey = (fptr->Fptr)->headstart[nexthdu];  
    (fptr->Fptr)->headend = (fptr->Fptr)->headstart[nexthdu];
    (fptr->Fptr)->datastart = ((fptr->Fptr)->headstart[nexthdu]) + 2880;
    (fptr->Fptr)->hdutype = IMAGE_HDU;  /* might need to be reset... */

    /* write the required header keywords */
    ffphprll(fptr, TRUE, bitpix, naxis, naxes, 0, 1, TRUE, status);

    /* redefine internal structure for this HDU */
    ffrdef(fptr, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffitab(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG naxis1,     /* I - width of row in the table                */
           LONGLONG naxis2,     /* I - number of rows in the table              */
           int tfields,     /* I - number of columns in the table           */
           char **ttype,    /* I - name of each column                      */
           long *tbcol,     /* I - byte offset in row to each column        */
           char **tform,    /* I - value of TFORMn keyword for each column  */
           char **tunit,    /* I - value of TUNITn keyword for each column  */
           const char *extnmx,   /* I - value of EXTNAME keyword, if any         */
           int *status)     /* IO - error status                            */
/*
  insert an ASCII table extension following the current HDU 
*/
{
    int nexthdu, maxhdu, ii, nunit, nhead, ncols, gotmem = 0;
    long nblocks, rowlen;
    LONGLONG datasize, newstart;
    char errmsg[81], extnm[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    extnm[0] = '\0';
    if (extnmx)
      strncat(extnm, extnmx, FLEN_VALUE-1);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    maxhdu = (fptr->Fptr)->maxhdu;
    /* if the current header is completely empty ...  */
    if (( (fptr->Fptr)->headend == (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        /* or, if we are at the end of the file, ... */
    ||  ( (((fptr->Fptr)->curhdu) == maxhdu ) &&
       ((fptr->Fptr)->headstart[maxhdu + 1] >= (fptr->Fptr)->logfilesize ) ) )
    {
        /* then simply append new image extension */
        ffcrtb(fptr, ASCII_TBL, naxis2, tfields, ttype, tform, tunit,
               extnm, status);
        return(*status);
    }

    if (naxis1 < 0)
        return(*status = NEG_WIDTH);
    else if (naxis2 < 0)
        return(*status = NEG_ROWS);
    else if (tfields < 0 || tfields > 999)
    {
        sprintf(errmsg,
        "Illegal value for TFIELDS keyword: %d", tfields);
        ffpmsg(errmsg);
        return(*status = BAD_TFIELDS);
    }

    /* count number of optional TUNIT keywords to be written */
    nunit = 0;
    for (ii = 0; ii < tfields; ii++)
    {
        if (tunit && *tunit && *tunit[ii])
            nunit++;
    }

    if (*extnm)
         nunit++;     /* add one for the EXTNAME keyword */

    rowlen = (long) naxis1;

    if (!tbcol || !tbcol[0] || (!naxis1 && tfields)) /* spacing not defined? */
    {
      /* allocate mem for tbcol; malloc may have problems allocating small */
      /* arrays, so allocate at least 20 bytes */

      ncols = maxvalue(5, tfields);
      tbcol = (long *) calloc(ncols, sizeof(long));

      if (tbcol)
      {
        gotmem = 1;

        /* calculate width of a row and starting position of each column. */
        /* Each column will be separated by 1 blank space */
        ffgabc(tfields, tform, 1, &rowlen, tbcol, status);
      }
    }

    nhead = (9 + (3 * tfields) + nunit + 35) / 36;  /* no. of header blocks */
    datasize = (LONGLONG)rowlen * naxis2;          /* size of table in bytes */
    nblocks = (long) (((datasize + 2879) / 2880) + nhead);  /* size of HDU */

    if ((fptr->Fptr)->writemode == READWRITE) /* must have write access */
    {   /* close the CHDU */
        ffrdef(fptr, status);  /* scan header to redefine structure */
        ffpdfl(fptr, status);  /* insure correct data file values */
    }
    else
        return(*status = READONLY_FILE);

    nexthdu = ((fptr->Fptr)->curhdu) + 1; /* number of the next (new) hdu */
    newstart = (fptr->Fptr)->headstart[nexthdu]; /* save starting addr of HDU */

    (fptr->Fptr)->hdutype = ASCII_TBL;  /* so that correct fill value is used */
    /* ffiblk also increments headstart for all following HDUs */
    if (ffiblk(fptr, nblocks, 1, status) > 0)  /* insert the blocks */
    {
        if (gotmem)
            free(tbcol); 
        return(*status);
    }

    ((fptr->Fptr)->maxhdu)++;      /* increment known number of HDUs in the file */
    for (ii = (fptr->Fptr)->maxhdu; ii > (fptr->Fptr)->curhdu; ii--)
        (fptr->Fptr)->headstart[ii + 1] = (fptr->Fptr)->headstart[ii]; /* incre start addr */

    (fptr->Fptr)->headstart[nexthdu] = newstart; /* set starting addr of HDU */

    /* set default parameters for this new empty HDU */
    (fptr->Fptr)->curhdu = nexthdu;   /* we are now located at the next HDU */
    fptr->HDUposition = nexthdu;      /* we are now located at the next HDU */
    (fptr->Fptr)->nextkey = (fptr->Fptr)->headstart[nexthdu];  
    (fptr->Fptr)->headend = (fptr->Fptr)->headstart[nexthdu];
    (fptr->Fptr)->datastart = ((fptr->Fptr)->headstart[nexthdu]) + (nhead * 2880);
    (fptr->Fptr)->hdutype = ASCII_TBL;  /* might need to be reset... */

    /* write the required header keywords */

    ffphtb(fptr, rowlen, naxis2, tfields, ttype, tbcol, tform, tunit,
           extnm, status);

    if (gotmem)
        free(tbcol); 

    /* redefine internal structure for this HDU */

    ffrdef(fptr, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffibin(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG naxis2,     /* I - number of rows in the table              */
           int tfields,     /* I - number of columns in the table           */
           char **ttype,    /* I - name of each column                      */
           char **tform,    /* I - value of TFORMn keyword for each column  */
           char **tunit,    /* I - value of TUNITn keyword for each column  */
           const char *extnmx,     /* I - value of EXTNAME keyword, if any         */
           LONGLONG pcount, /* I - size of special data area (heap)         */
           int *status)     /* IO - error status                            */
/*
  insert a Binary table extension following the current HDU 
*/
{
    int nexthdu, maxhdu, ii, nunit, nhead, datacode;
    LONGLONG naxis1;
    long nblocks, repeat, width;
    LONGLONG datasize, newstart;
    char errmsg[81], extnm[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    extnm[0] = '\0';
    if (extnmx)
      strncat(extnm, extnmx, FLEN_VALUE-1);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    maxhdu = (fptr->Fptr)->maxhdu;
    /* if the current header is completely empty ...  */
    if (( (fptr->Fptr)->headend == (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        /* or, if we are at the end of the file, ... */
    ||  ( (((fptr->Fptr)->curhdu) == maxhdu ) &&
       ((fptr->Fptr)->headstart[maxhdu + 1] >= (fptr->Fptr)->logfilesize ) ) )
    {
        /* then simply append new image extension */
        ffcrtb(fptr, BINARY_TBL, naxis2, tfields, ttype, tform, tunit,
               extnm, status);
        return(*status);
    }

    if (naxis2 < 0)
        return(*status = NEG_ROWS);
    else if (tfields < 0 || tfields > 999)
    {
        sprintf(errmsg,
        "Illegal value for TFIELDS keyword: %d", tfields);
        ffpmsg(errmsg);
        return(*status = BAD_TFIELDS);
    }

    /* count number of optional TUNIT keywords to be written */
    nunit = 0;
    for (ii = 0; ii < tfields; ii++)
    {
        if (tunit && *tunit && *tunit[ii])
            nunit++;
    }

    if (*extnm)
         nunit++;     /* add one for the EXTNAME keyword */

    nhead = (9 + (2 * tfields) + nunit + 35) / 36;  /* no. of header blocks */

    /* calculate total width of the table */
    naxis1 = 0;
    for (ii = 0; ii < tfields; ii++)
    {
        ffbnfm(tform[ii], &datacode, &repeat, &width, status);

        if (datacode == TBIT)
            naxis1 = naxis1 + ((repeat + 7) / 8);
        else if (datacode == TSTRING)
            naxis1 += repeat;
        else
            naxis1 = naxis1 + (repeat * width);
    }

    datasize = ((LONGLONG)naxis1 * naxis2) + pcount;         /* size of table in bytes */
    nblocks = (long) ((datasize + 2879) / 2880) + nhead;  /* size of HDU */

    if ((fptr->Fptr)->writemode == READWRITE) /* must have write access */
    {   /* close the CHDU */
        ffrdef(fptr, status);  /* scan header to redefine structure */
        ffpdfl(fptr, status);  /* insure correct data file values */
    }
    else
        return(*status = READONLY_FILE);

    nexthdu = ((fptr->Fptr)->curhdu) + 1; /* number of the next (new) hdu */
    newstart = (fptr->Fptr)->headstart[nexthdu]; /* save starting addr of HDU */

    (fptr->Fptr)->hdutype = BINARY_TBL;  /* so that correct fill value is used */

    /* ffiblk also increments headstart for all following HDUs */
    if (ffiblk(fptr, nblocks, 1, status) > 0)  /* insert the blocks */
        return(*status);

    ((fptr->Fptr)->maxhdu)++;      /* increment known number of HDUs in the file */
    for (ii = (fptr->Fptr)->maxhdu; ii > (fptr->Fptr)->curhdu; ii--)
        (fptr->Fptr)->headstart[ii + 1] = (fptr->Fptr)->headstart[ii]; /* incre start addr */

    (fptr->Fptr)->headstart[nexthdu] = newstart; /* set starting addr of HDU */

    /* set default parameters for this new empty HDU */
    (fptr->Fptr)->curhdu = nexthdu;   /* we are now located at the next HDU */
    fptr->HDUposition = nexthdu;      /* we are now located at the next HDU */
    (fptr->Fptr)->nextkey = (fptr->Fptr)->headstart[nexthdu];  
    (fptr->Fptr)->headend = (fptr->Fptr)->headstart[nexthdu];
    (fptr->Fptr)->datastart = ((fptr->Fptr)->headstart[nexthdu]) + (nhead * 2880);
    (fptr->Fptr)->hdutype = BINARY_TBL;  /* might need to be reset... */

    /* write the required header keywords. This will write PCOUNT = 0 */
    /* so that the variable length data will be written at the right place */
    ffphbn(fptr, naxis2, tfields, ttype, tform, tunit, extnm, pcount,
           status);

    /* redefine internal structure for this HDU (with PCOUNT = 0) */
    ffrdef(fptr, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdhdu(fitsfile *fptr,      /* I - FITS file pointer                   */
           int *hdutype,        /* O - type of the new CHDU after deletion */
           int *status)         /* IO - error status                       */
/*
  Delete the CHDU.  If the CHDU is the primary array, then replace the HDU
  with an empty primary array with no data.   Return the
  type of the new CHDU after the old CHDU is deleted.
*/
{
    int tmptype = 0;
    long nblocks, ii, naxes[1];

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if ((fptr->Fptr)->curhdu == 0) /* replace primary array with null image */
    {
        /* ignore any existing keywords */
        (fptr->Fptr)->headend = 0;
        (fptr->Fptr)->nextkey = 0;

        /* write default primary array header */
        ffphpr(fptr,1,8,0,naxes,0,1,1,status);

        /* calc number of blocks to delete (leave just 1 block) */
        nblocks = (long) (( (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu + 1] - 
                2880 ) / 2880);

        /* ffdblk also updates the starting address of all following HDUs */
        if (nblocks > 0)
        {
            if (ffdblk(fptr, nblocks, status) > 0) /* delete the HDU */
                return(*status);
        }

        /* this might not be necessary, but is doesn't hurt */
        (fptr->Fptr)->datastart = DATA_UNDEFINED;

        ffrdef(fptr, status);  /* reinitialize the primary array */
    }
    else
    {

        /* calc number of blocks to delete */
        nblocks = (long) (( (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu + 1] - 
                (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] ) / 2880);

        /* ffdblk also updates the starting address of all following HDUs */
        if (ffdblk(fptr, nblocks, status) > 0) /* delete the HDU */
            return(*status);

        /* delete the CHDU from the list of HDUs */
        for (ii = (fptr->Fptr)->curhdu + 1; ii <= (fptr->Fptr)->maxhdu; ii++)
            (fptr->Fptr)->headstart[ii] = (fptr->Fptr)->headstart[ii + 1];

        (fptr->Fptr)->headstart[(fptr->Fptr)->maxhdu + 1] = 0;
        ((fptr->Fptr)->maxhdu)--; /* decrement the known number of HDUs */

        if (ffrhdu(fptr, &tmptype, status) > 0)  /* initialize next HDU */
        {
            /* failed (end of file?), so move back one HDU */
            *status = 0;
            ffcmsg();       /* clear extraneous error messages */
            ffgext(fptr, ((fptr->Fptr)->curhdu) - 1, &tmptype, status);
        }
    }

    if (hdutype)
       *hdutype = tmptype;

    return(*status);
}

