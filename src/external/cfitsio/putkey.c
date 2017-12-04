/*  This file, putkey.c, contains routines that write keywords to          */
/*  a FITS header.                                                         */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
/* stddef.h is apparently needed to define size_t */
#include <stddef.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int ffcrim(fitsfile *fptr,      /* I - FITS file pointer           */
           int bitpix,          /* I - bits per pixel              */
           int naxis,           /* I - number of axes in the array */
           long *naxes,         /* I - size of each axis           */
           int *status)         /* IO - error status               */
/*
  create an IMAGE extension following the current HDU. If the
  current HDU is empty (contains no header keywords), then simply
  write the required image (or primary array) keywords to the current
  HDU. 
*/
{
    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* create new extension if current header is not empty */
    if ((fptr->Fptr)->headend != (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        ffcrhd(fptr, status);

    /* write the required header keywords */
    ffphpr(fptr, TRUE, bitpix, naxis, naxes, 0, 1, TRUE, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcrimll(fitsfile *fptr,    /* I - FITS file pointer           */
           int bitpix,          /* I - bits per pixel              */
           int naxis,           /* I - number of axes in the array */
           LONGLONG *naxes,     /* I - size of each axis           */
           int *status)         /* IO - error status               */
/*
  create an IMAGE extension following the current HDU. If the
  current HDU is empty (contains no header keywords), then simply
  write the required image (or primary array) keywords to the current
  HDU. 
*/
{
    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* create new extension if current header is not empty */
    if ((fptr->Fptr)->headend != (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        ffcrhd(fptr, status);

    /* write the required header keywords */
    ffphprll(fptr, TRUE, bitpix, naxis, naxes, 0, 1, TRUE, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcrtb(fitsfile *fptr,  /* I - FITS file pointer                        */
           int tbltype,     /* I - type of table to create                  */
           LONGLONG naxis2, /* I - number of rows in the table              */
           int tfields,     /* I - number of columns in the table           */
           char **ttype,    /* I - name of each column                      */
           char **tform,    /* I - value of TFORMn keyword for each column  */
           char **tunit,    /* I - value of TUNITn keyword for each column  */
           const char *extnm, /* I - value of EXTNAME keyword, if any         */
           int *status)     /* IO - error status                            */
/*
  Create a table extension in a FITS file. 
*/
{
    LONGLONG naxis1 = 0;
    long *tbcol = 0;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* create new extension if current header is not empty */
    if ((fptr->Fptr)->headend != (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        ffcrhd(fptr, status);

    if ((fptr->Fptr)->curhdu == 0)  /* have to create dummy primary array */
    {
       ffcrim(fptr, 16, 0, tbcol, status);
       ffcrhd(fptr, status);
    }
    
    if (tbltype == BINARY_TBL)
    {
      /* write the required header keywords. This will write PCOUNT = 0 */
      ffphbn(fptr, naxis2, tfields, ttype, tform, tunit, extnm, 0, status);
    }
    else if (tbltype == ASCII_TBL)
    {
      /* write the required header keywords */
      /* default values for naxis1 and tbcol will be calculated */
      ffphtb(fptr, naxis1, naxis2, tfields, ttype, tbcol, tform, tunit,
             extnm, status);
    }
    else
      *status = NOT_TABLE;

    return(*status);
}
/*-------------------------------------------------------------------------*/
int ffpktp(fitsfile *fptr,       /* I - FITS file pointer       */
           const char *filename, /* I - name of template file   */
           int *status)          /* IO - error status           */
/*
  read keywords from template file and append to the FITS file
*/
{
    FILE *diskfile;
    char card[FLEN_CARD], template[161];
    char keyname[FLEN_KEYWORD], newname[FLEN_KEYWORD];
    int keytype;
    size_t slen;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    diskfile = fopen(filename,"r"); 
    if (!diskfile)          /* couldn't open file */
    {
            ffpmsg("ffpktp could not open the following template file:");
            ffpmsg(filename);
            return(*status = FILE_NOT_OPENED); 
    }

    while (fgets(template, 160, diskfile) )  /* get next template line */
    {
      template[160] = '\0';      /* make sure string is terminated */
      slen = strlen(template);   /* get string length */
      template[slen - 1] = '\0';  /* over write the 'newline' char */

      if (ffgthd(template, card, &keytype, status) > 0) /* parse template */
         break;

      strncpy(keyname, card, 8);
      keyname[8] = '\0';

      if (keytype == -2)            /* rename the card */
      {
         strncpy(newname, &card[40], 8);
         newname[8] = '\0';

         ffmnam(fptr, keyname, newname, status); 
      }
      else if (keytype == -1)      /* delete the card */
      {
         ffdkey(fptr, keyname, status);
      }
      else if (keytype == 0)       /* update the card */
      {
         ffucrd(fptr, keyname, card, status);
      }
      else if (keytype == 1)      /* append the card */
      {
         ffprec(fptr, card, status);
      }
      else    /* END card; stop here */
      {
         break; 
      }
    }

    fclose(diskfile);   /* close the template file */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpky( fitsfile *fptr,     /* I - FITS file pointer        */
           int  datatype,      /* I - datatype of the value    */
           const char *keyname,/* I - name of keyword to write */
           void *value,        /* I - keyword value            */
           const char *comm,   /* I - keyword comment          */
           int  *status)       /* IO - error status            */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes a keyword value with the datatype specified by the 2nd argument.
*/
{
    char errmsg[81];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (datatype == TSTRING)
    {
        ffpkys(fptr, keyname, (char *) value, comm, status);
    }
    else if (datatype == TBYTE)
    {
        ffpkyj(fptr, keyname, (LONGLONG) *(unsigned char *) value, comm, status);
    }
    else if (datatype == TSBYTE)
    {
        ffpkyj(fptr, keyname, (LONGLONG) *(signed char *) value, comm, status);
    }
    else if (datatype == TUSHORT)
    {
        ffpkyj(fptr, keyname, (LONGLONG) *(unsigned short *) value, comm, status);
    }
    else if (datatype == TSHORT)
    {
        ffpkyj(fptr, keyname, (LONGLONG) *(short *) value, comm, status);
    }
    else if (datatype == TUINT)
    {
        ffpkyg(fptr, keyname, (double) *(unsigned int *) value, 0,
               comm, status);
    }
    else if (datatype == TINT)
    {
        ffpkyj(fptr, keyname, (LONGLONG) *(int *) value, comm, status);
    }
    else if (datatype == TLOGICAL)
    {
        ffpkyl(fptr, keyname, *(int *) value, comm, status);
    }
    else if (datatype == TULONG)
    {
        ffpkyg(fptr, keyname, (double) *(unsigned long *) value, 0,
               comm, status);
    }
    else if (datatype == TLONG)
    {
        ffpkyj(fptr, keyname, (LONGLONG) *(long *) value, comm, status);
    }
    else if (datatype == TLONGLONG)
    {
        ffpkyj(fptr, keyname, *(LONGLONG *) value, comm, status);
    }
    else if (datatype == TFLOAT)
    {
        ffpkye(fptr, keyname, *(float *) value, -7, comm, status);
    }
    else if (datatype == TDOUBLE)
    {
        ffpkyd(fptr, keyname, *(double *) value, -15, comm, status);
    }
    else if (datatype == TCOMPLEX)
    {
        ffpkyc(fptr, keyname, (float *) value, -7, comm, status);
    }
    else if (datatype == TDBLCOMPLEX)
    {
        ffpkym(fptr, keyname, (double *) value, -15, comm, status);
    }
    else
    {
        sprintf(errmsg, "Bad keyword datatype code: %d (ffpky)", datatype);
        ffpmsg(errmsg);
        *status = BAD_DATATYPE;
    }

    return(*status);
} 
/*-------------------------------------------------------------------------*/
int ffprec(fitsfile *fptr,     /* I - FITS file pointer        */
           const char *card,   /* I - string to be written     */
           int *status)        /* IO - error status            */
/*
  write a keyword record (80 bytes long) to the end of the header
*/
{
    char tcard[FLEN_CARD];
    size_t len, ii;
    long nblocks;
    int keylength;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if ( ((fptr->Fptr)->datastart - (fptr->Fptr)->headend) == 80) /* no room */
    {
        nblocks = 1;
        if (ffiblk(fptr, nblocks, 0, status) > 0) /* insert 2880-byte block */
            return(*status);  
    }

    strncpy(tcard,card,80);
    tcard[80] = '\0';

    len = strlen(tcard);

    /* silently replace any illegal characters with a space */
    for (ii=0; ii < len; ii++)   
        if (tcard[ii] < ' ' || tcard[ii] > 126) tcard[ii] = ' ';

    for (ii=len; ii < 80; ii++)    /* fill card with spaces if necessary */
        tcard[ii] = ' ';

    keylength = strcspn(tcard, "=");   /* support for free-format keywords */
    if (keylength == 80) keylength = 8;
    
    /* test for the common commentary keywords which by definition have 8-char names */
    if ( !fits_strncasecmp( "COMMENT ", tcard, 8) || !fits_strncasecmp( "HISTORY ", tcard, 8) ||
         !fits_strncasecmp( "        ", tcard, 8) || !fits_strncasecmp( "CONTINUE", tcard, 8) )
	 keylength = 8;

    for (ii=0; ii < keylength; ii++)       /* make sure keyword name is uppercase */
        tcard[ii] = toupper(tcard[ii]);

    fftkey(tcard, status);        /* test keyword name contains legal chars */

/*  no need to do this any more, since any illegal characters have been removed
    fftrec(tcard, status);  */        /* test rest of keyword for legal chars */

    ffmbyt(fptr, (fptr->Fptr)->headend, IGNORE_EOF, status); /* move to end */

    ffpbyt(fptr, 80, tcard, status);   /* write the 80 byte card */

    if (*status <= 0)
       (fptr->Fptr)->headend += 80;    /* update end-of-header position */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkyu( fitsfile *fptr,     /* I - FITS file pointer        */
            const char *keyname,/* I - name of keyword to write */
            const char *comm,   /* I - keyword comment          */
            int  *status)       /* IO - error status            */
/*
  Write (put) a null-valued keyword and comment into the FITS header.  
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    strcpy(valstring," ");  /* create a dummy value string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword */
    ffprec(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkys( fitsfile *fptr,     /* I - FITS file pointer        */
            const char *keyname,/* I - name of keyword to write */
            const char *value,  /* I - keyword value            */
            const char *comm,   /* I - keyword comment          */
            int  *status)       /* IO - error status            */
/*
  Write (put) the keyword, value and comment into the FITS header.
  The value string will be truncated at 68 characters which is the
  maximum length that will fit on a single FITS keyword.
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffs2c(value, valstring, status);   /* put quotes around the string */

    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword */
    ffprec(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkls( fitsfile *fptr,     /* I - FITS file pointer        */
            const char *keyname,/* I - name of keyword to write */
            const char *value,  /* I - keyword value            */
            const char *comm,   /* I - keyword comment          */
            int  *status)       /* IO - error status            */
/*
  Write (put) the keyword, value and comment into the FITS header.
  This routine is a modified version of ffpkys which supports the
  HEASARC long string convention and can write arbitrarily long string
  keyword values.  The value is continued over multiple keywords that
  have the name COMTINUE without an equal sign in column 9 of the card.
  This routine also supports simple string keywords which are less than
  69 characters in length.
*/
{
    char valstring[FLEN_CARD];
    char card[FLEN_CARD], tmpkeyname[FLEN_CARD];
    char tstring[FLEN_CARD], *cptr;
    int next, remain, vlen, nquote, nchar, namelen, contin, tstatus = -1;
    int commlen=0, nocomment = 0;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    remain = maxvalue(strlen(value), 1); /* no. of chars to write (at least 1) */  
    if (comm) { 
       commlen = strlen(comm);
       if (commlen > 47) commlen = 47;  /* only guarantee preserving the first 47 characters */
    }

    /* count the number of single quote characters are in the string */
    tstring[0] = '\0';
    strncat(tstring, value, 68); /* copy 1st part of string to temp buff */
    nquote = 0;
    cptr = strchr(tstring, '\'');   /* search for quote character */
    while (cptr)  /* search for quote character */
    {
        nquote++;            /*  increment no. of quote characters  */
        cptr++;              /*  increment pointer to next character */
        cptr = strchr(cptr, '\'');  /* search for another quote char */
    }

    strncpy(tmpkeyname, keyname, 80);
    tmpkeyname[80] = '\0';
    
    cptr = tmpkeyname;
    while(*cptr == ' ')   /* skip over leading spaces in name */
        cptr++;

    /* determine the number of characters that will fit on the line */
    /* Note: each quote character is expanded to 2 quotes */

    namelen = strlen(cptr);
    if (namelen <= 8 && (fftkey(cptr, &tstatus) <= 0) )
    {
        /* This a normal 8-character FITS keyword */
        nchar = 68 - nquote; /*  max of 68 chars fit in a FITS string value */
    }
    else
    {
	   nchar = 80 - nquote - namelen - 5;
    }

    contin = 0;
    next = 0;                  /* pointer to next character to write */

    while (remain > 0)
    {
        tstring[0] = '\0';
        strncat(tstring, &value[next], nchar); /* copy string to temp buff */
        ffs2c(tstring, valstring, status);  /* expand quotes, and put quotes around the string */

        if (remain > nchar)   /* if string is continued, put & as last char */
        {
            vlen = strlen(valstring);
            nchar -= 1;        /* outputting one less character now */

            if (valstring[vlen-2] != '\'')
                valstring[vlen-2] = '&';  /*  over write last char with &  */
            else
            { /* last char was a pair of single quotes, so over write both */
                valstring[vlen-3] = '&';
                valstring[vlen-1] = '\0';
            }
        }

        if (contin)           /* This is a CONTINUEd keyword */
        {
           if (nocomment) {
               ffmkky("CONTINUE", valstring, NULL, card, status); /* make keyword w/o comment */
           } else {
               ffmkky("CONTINUE", valstring, comm, card, status); /* make keyword */
	   }
           strncpy(&card[8], "   ",  2);  /* overwrite the '=' */
        }
        else
        {
           ffmkky(keyname, valstring, comm, card, status);  /* make keyword */
        }

        ffprec(fptr, card, status);  /* write the keyword */

        contin = 1;
        remain -= nchar;
        next  += nchar;
        nocomment = 0;

        if (remain > 0) 
        {
           /* count the number of single quote characters in next section */
           tstring[0] = '\0';
           strncat(tstring, &value[next], 68); /* copy next part of string */
           nquote = 0;
           cptr = strchr(tstring, '\'');   /* search for quote character */
           while (cptr)  /* search for quote character */
           {
               nquote++;            /*  increment no. of quote characters  */
               cptr++;              /*  increment pointer to next character */
               cptr = strchr(cptr, '\'');  /* search for another quote char */
           }
           nchar = 68 - nquote;  /* max number of chars to write this time */
        }

        /* make adjustment if necessary to allow reasonable room for a comment on last CONTINUE card 
	   only need to do this if 
	     a) there is a comment string, and
	     b) the remaining value string characters could all fit on the next CONTINUE card, and
	     c) there is not enough room on the next CONTINUE card for both the remaining value
	        characters, and at least 47 characters of the comment string.
	*/
	
        if (commlen > 0 && remain + nquote < 69 && remain + nquote + commlen > 65) 
	{
            if (nchar > 18) { /* only false if there are a rediculous number of quotes in the string */
	        nchar = remain - 15;  /* force continuation onto another card, so that */
		                      /* there is room for a comment up to 47 chara long */
                nocomment = 1;  /* don't write the comment string this time */
            }
	}
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffplsw( fitsfile *fptr,     /* I - FITS file pointer  */
            int  *status)       /* IO - error status       */
/*
  Write the LONGSTRN keyword and a series of related COMMENT keywords
  which document that this FITS header may contain long string keyword
  values which are continued over multiple keywords using the HEASARC
  long string keyword convention.  If the LONGSTRN keyword already exists
  then this routine simple returns without doing anything.
*/
{
    char valstring[FLEN_VALUE], comm[FLEN_COMMENT];
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = 0;
    if (ffgkys(fptr, "LONGSTRN", valstring, comm, &tstatus) == 0)
        return(*status);     /* keyword already exists, so just return */

    ffpkys(fptr, "LONGSTRN", "OGIP 1.0", 
       "The HEASARC Long String Convention may be used.", status);

    ffpcom(fptr,
    "  This FITS file may contain long string keyword values that are", status);

    ffpcom(fptr,
    "  continued over multiple keywords.  The HEASARC convention uses the &",
    status);

    ffpcom(fptr,
    "  character at the end of each substring which is then continued", status);

    ffpcom(fptr,
    "  on the next keyword which has the name CONTINUE.", status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkyl( fitsfile *fptr,     /* I - FITS file pointer        */
            const char *keyname,/* I - name of keyword to write */
            int  value,         /* I - keyword value            */
            const char *comm,   /* I - keyword comment          */
            int  *status)       /* IO - error status            */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Values equal to 0 will result in a False FITS keyword; any other
  non-zero value will result in a True FITS keyword.
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffl2c(value, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkyj( fitsfile *fptr,     /* I - FITS file pointer        */
            const char *keyname,/* I - name of keyword to write */
            LONGLONG value,     /* I - keyword value            */
            const char *comm,   /* I - keyword comment          */
            int  *status)       /* IO - error status            */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes an integer keyword value.
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffi2c(value, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkyf( fitsfile *fptr,      /* I - FITS file pointer                   */
            const char  *keyname,/* I - name of keyword to write            */
            float value,         /* I - keyword value                       */
            int   decim,         /* I - number of decimal places to display */
            const char  *comm,   /* I - keyword comment                     */
            int   *status)       /* IO - error status                       */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes a fixed float keyword value.
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffr2f(value, decim, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkye( fitsfile *fptr,      /* I - FITS file pointer                   */
            const char  *keyname,/* I - name of keyword to write            */
            float value,         /* I - keyword value                       */
            int   decim,         /* I - number of decimal places to display */
            const char  *comm,   /* I - keyword comment                     */
            int   *status)       /* IO - error status                       */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes an exponential float keyword value.
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffr2e(value, decim, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkyg( fitsfile *fptr,      /* I - FITS file pointer                   */
            const char  *keyname,/* I - name of keyword to write            */
            double value,        /* I - keyword value                       */
            int   decim,         /* I - number of decimal places to display */
            const char  *comm,   /* I - keyword comment                     */
            int   *status)       /* IO - error status                       */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes a fixed double keyword value.*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffd2f(value, decim, valstring, status);  /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkyd( fitsfile *fptr,      /* I - FITS file pointer                   */
            const char  *keyname,/* I - name of keyword to write            */
            double value,        /* I - keyword value                       */
            int   decim,         /* I - number of decimal places to display */
            const char  *comm,   /* I - keyword comment                     */
            int   *status)       /* IO - error status                       */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes an exponential double keyword value.*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffd2e(value, decim, valstring, status);  /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkyc( fitsfile *fptr,      /* I - FITS file pointer                   */
            const char  *keyname,/* I - name of keyword to write            */
            float *value,        /* I - keyword value (real, imaginary)     */
            int   decim,         /* I - number of decimal places to display */
            const char  *comm,   /* I - keyword comment                     */
            int   *status)       /* IO - error status                       */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes an complex float keyword value. Format = (realvalue, imagvalue)
*/
{
    char valstring[FLEN_VALUE], tmpstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    strcpy(valstring, "(" );
    ffr2e(value[0], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ", ");
    ffr2e(value[1], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ")");

    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkym( fitsfile *fptr,      /* I - FITS file pointer                   */
            const char  *keyname,/* I - name of keyword to write            */
            double *value,       /* I - keyword value (real, imaginary)     */
            int   decim,         /* I - number of decimal places to display */
            const char  *comm,   /* I - keyword comment                     */
            int   *status)       /* IO - error status                       */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes an complex double keyword value. Format = (realvalue, imagvalue)
*/
{
    char valstring[FLEN_VALUE], tmpstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    strcpy(valstring, "(" );
    ffd2e(value[0], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ", ");
    ffd2e(value[1], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ")");

    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkfc( fitsfile *fptr,      /* I - FITS file pointer                   */
            const char  *keyname,/* I - name of keyword to write            */
            float *value,        /* I - keyword value (real, imaginary)     */
            int   decim,         /* I - number of decimal places to display */
            const char  *comm,   /* I - keyword comment                     */
            int   *status)       /* IO - error status                       */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes an complex float keyword value. Format = (realvalue, imagvalue)
*/
{
    char valstring[FLEN_VALUE], tmpstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    strcpy(valstring, "(" );
    ffr2f(value[0], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ", ");
    ffr2f(value[1], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ")");

    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkfm( fitsfile *fptr,      /* I - FITS file pointer                   */
            const char  *keyname,/* I - name of keyword to write            */
            double *value,       /* I - keyword value (real, imaginary)     */
            int   decim,         /* I - number of decimal places to display */
            const char  *comm,   /* I - keyword comment                     */
            int   *status)       /* IO - error status                       */
/*
  Write (put) the keyword, value and comment into the FITS header.
  Writes an complex double keyword value. Format = (realvalue, imagvalue)
*/
{
    char valstring[FLEN_VALUE], tmpstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    strcpy(valstring, "(" );
    ffd2f(value[0], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ", ");
    ffd2f(value[1], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ")");

    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkyt( fitsfile *fptr,      /* I - FITS file pointer        */
            const char  *keyname,/* I - name of keyword to write */
            long  intval,        /* I - integer part of value    */
            double fraction,     /* I - fractional part of value */
            const char  *comm,   /* I - keyword comment          */
            int   *status)       /* IO - error status            */
/*
  Write (put) a 'triple' precision keyword where the integer and
  fractional parts of the value are passed in separate parameters to
  increase the total amount of numerical precision.
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];
    char fstring[20], *cptr;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (fraction > 1. || fraction < 0.)
    {
        ffpmsg("fraction must be between 0. and 1. (ffpkyt)");
        return(*status = BAD_F2C);
    }

    ffi2c(intval, valstring, status);  /* convert integer to string */
    ffd2f(fraction, 16, fstring, status);  /* convert to 16 decimal string */

    cptr = strchr(fstring, '.');    /* find the decimal point */
    strcat(valstring, cptr);    /* append the fraction to the integer */

    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffprec(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*-----------------------------------------------------------------*/
int ffpcom( fitsfile *fptr,      /* I - FITS file pointer   */
            const char  *comm,   /* I - comment string      */
            int   *status)       /* IO - error status       */
/*
  Write 1 or more COMMENT keywords.  If the comment string is too
  long to fit on a single keyword (72 chars) then it will automatically
  be continued on multiple CONTINUE keywords.
*/
{
    char card[FLEN_CARD];
    int len, ii;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    len = strlen(comm);
    ii = 0;

    for (; len > 0; len -= 72)
    {
        strcpy(card, "COMMENT ");
        strncat(card, &comm[ii], 72);
        ffprec(fptr, card, status);
        ii += 72;
    }

    return(*status);
}
/*-----------------------------------------------------------------*/
int ffphis( fitsfile *fptr,      /* I - FITS file pointer  */
            const char *history, /* I - history string     */
            int   *status)       /* IO - error status      */
/*
  Write 1 or more HISTORY keywords.  If the history string is too
  long to fit on a single keyword (72 chars) then it will automatically
  be continued on multiple HISTORY keywords.
*/
{
    char card[FLEN_CARD];
    int len, ii;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    len = strlen(history);
    ii = 0;

    for (; len > 0; len -= 72)
    {
        strcpy(card, "HISTORY ");
        strncat(card, &history[ii], 72);
        ffprec(fptr, card, status);
        ii += 72;
    }

    return(*status);
}
/*-----------------------------------------------------------------*/
int ffpdat( fitsfile *fptr,      /* I - FITS file pointer  */
            int   *status)       /* IO - error status      */
/*
  Write the DATE keyword into the FITS header.  If the keyword already
  exists then the date will simply be updated in the existing keyword.
*/
{
    int timeref;
    char date[30], tmzone[10], card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffgstm(date, &timeref, status);

    if (timeref)           /* GMT not available on this machine */
        strcpy(tmzone, " Local");    
    else
        strcpy(tmzone, " UT");    

    strcpy(card, "DATE    = '");
    strcat(card, date);
    strcat(card, "' / file creation date (YYYY-MM-DDThh:mm:ss");
    strcat(card, tmzone);
    strcat(card, ")");

    ffucrd(fptr, "DATE", card, status);

    return(*status);
}
/*-------------------------------------------------------------------*/
int ffverifydate(int year,          /* I - year (0 - 9999)           */
                 int month,         /* I - month (1 - 12)            */
                 int day,           /* I - day (1 - 31)              */
                 int   *status)     /* IO - error status             */
/*
  Verify that the date is valid
*/
{
    int ndays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    char errmsg[81];
    

    if (year < 0 || year > 9999)
    {
       sprintf(errmsg, 
       "input year value = %d is out of range 0 - 9999", year);
       ffpmsg(errmsg);
       return(*status = BAD_DATE);
    }
    else if (month < 1 || month > 12)
    {
       sprintf(errmsg, 
       "input month value = %d is out of range 1 - 12", month);
       ffpmsg(errmsg);
       return(*status = BAD_DATE);
    }
    
    if (ndays[month] == 31) {
        if (day < 1 || day > 31)
        {
           sprintf(errmsg, 
           "input day value = %d is out of range 1 - 31 for month %d", day, month);
           ffpmsg(errmsg);
           return(*status = BAD_DATE);
        }
    } else if (ndays[month] == 30) {
        if (day < 1 || day > 30)
        {
           sprintf(errmsg, 
           "input day value = %d is out of range 1 - 30 for month %d", day, month);
           ffpmsg(errmsg);
           return(*status = BAD_DATE);
        }
    } else {
        if (day < 1 || day > 28)
        {
            if (day == 29)
            {
	      /* year is a leap year if it is divisible by 4 but not by 100,
	         except years divisible by 400 are leap years
	      */
	        if ((year % 4 == 0 && year % 100 != 0 ) || year % 400 == 0)
		   return (*status);
		   
 	        sprintf(errmsg, 
           "input day value = %d is out of range 1 - 28 for February %d (not leap year)", day, year);
                ffpmsg(errmsg);
	    } else {
                sprintf(errmsg, 
                "input day value = %d is out of range 1 - 28 (or 29) for February", day);
                ffpmsg(errmsg);
	    }
	    
            return(*status = BAD_DATE);
        }
    }
    return(*status);
}
/*-----------------------------------------------------------------*/
int ffgstm( char *timestr,   /* O  - returned system date and time string  */
            int  *timeref,   /* O - GMT = 0, Local time = 1  */
            int   *status)   /* IO - error status      */
/*
  Returns the current date and time in format 'yyyy-mm-ddThh:mm:ss'.
*/
{
    time_t tp;
    struct tm *ptr;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    time(&tp);
    ptr = gmtime(&tp);         /* get GMT (= UTC) time */

    if (timeref)
    {
        if (ptr)
            *timeref = 0;   /* returning GMT */
        else
            *timeref = 1;   /* returning local time */
    }

    if (!ptr)                  /* GMT not available on this machine */
        ptr = localtime(&tp); 

    strftime(timestr, 25, "%Y-%m-%dT%H:%M:%S", ptr);

    return(*status);
}
/*-----------------------------------------------------------------*/
int ffdt2s(int year,          /* I - year (0 - 9999)           */
           int month,         /* I - month (1 - 12)            */
           int day,           /* I - day (1 - 31)              */
           char *datestr,     /* O - date string: "YYYY-MM-DD" */
           int   *status)     /* IO - error status             */
/*
  Construct a date character string
*/
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    *datestr = '\0';
    
    if (ffverifydate(year, month, day, status) > 0)
    {
        ffpmsg("invalid date (ffdt2s)");
        return(*status);
    }

    if (year >= 1900 && year <= 1998)  /* use old 'dd/mm/yy' format */
        sprintf(datestr, "%.2d/%.2d/%.2d", day, month, year - 1900);

    else  /* use the new 'YYYY-MM-DD' format */
        sprintf(datestr, "%.4d-%.2d-%.2d", year, month, day);

    return(*status);
}
/*-----------------------------------------------------------------*/
int ffs2dt(char *datestr,   /* I - date string: "YYYY-MM-DD" or "dd/mm/yy" */
           int *year,       /* O - year (0 - 9999)                         */
           int *month,      /* O - month (1 - 12)                          */
           int *day,        /* O - day (1 - 31)                            */
           int   *status)   /* IO - error status                           */
/*
  Parse a date character string into year, month, and day values
*/
{
    int slen, lyear, lmonth, lday;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (year)
        *year = 0;
    if (month)
        *month = 0;
    if (day)
        *day   = 0;

    if (!datestr)
    {
        ffpmsg("error: null input date string (ffs2dt)");
        return(*status = BAD_DATE);   /* Null datestr pointer ??? */
    }

    slen = strlen(datestr);

    if (slen == 8 && datestr[2] == '/' && datestr[5] == '/')
    {
        if (isdigit((int) datestr[0]) && isdigit((int) datestr[1])
         && isdigit((int) datestr[3]) && isdigit((int) datestr[4])
         && isdigit((int) datestr[6]) && isdigit((int) datestr[7]) )
        {
            /* this is an old format string: "dd/mm/yy" */
            lyear  = atoi(&datestr[6]) + 1900;
            lmonth = atoi(&datestr[3]);
	    lday   = atoi(datestr);
	    
            if (year)
                *year = lyear;
            if (month)
                *month = lmonth;
            if (day)
                *day   = lday;
        }
        else
        {
            ffpmsg("input date string has illegal format (ffs2dt):");
            ffpmsg(datestr);
            return(*status = BAD_DATE);
        }
    }
    else if (slen >= 10 && datestr[4] == '-' && datestr[7] == '-')
        {
        if (isdigit((int) datestr[0]) && isdigit((int) datestr[1])
         && isdigit((int) datestr[2]) && isdigit((int) datestr[3])
         && isdigit((int) datestr[5]) && isdigit((int) datestr[6])
         && isdigit((int) datestr[8]) && isdigit((int) datestr[9]) )
        {
            if (slen > 10 && datestr[10] != 'T')
            {
                ffpmsg("input date string has illegal format (ffs2dt):");
                ffpmsg(datestr);
                return(*status = BAD_DATE);
            }

            /* this is a new format string: "yyyy-mm-dd" */
            lyear  = atoi(datestr);
            lmonth = atoi(&datestr[5]);
            lday   = atoi(&datestr[8]);

            if (year)
               *year  = lyear;
            if (month)
               *month = lmonth;
            if (day)
               *day   = lday;
        }
        else
        {
                ffpmsg("input date string has illegal format (ffs2dt):");
                ffpmsg(datestr);
                return(*status = BAD_DATE);
        }
    }
    else
    {
                ffpmsg("input date string has illegal format (ffs2dt):");
                ffpmsg(datestr);
                return(*status = BAD_DATE);
    }


    if (ffverifydate(lyear, lmonth, lday, status) > 0)
    {
        ffpmsg("invalid date (ffs2dt)");
    }

    return(*status);
}
/*-----------------------------------------------------------------*/
int fftm2s(int year,          /* I - year (0 - 9999)           */
           int month,         /* I - month (1 - 12)            */
           int day,           /* I - day (1 - 31)              */
           int hour,          /* I - hour (0 - 23)             */
           int minute,        /* I - minute (0 - 59)           */
           double second,     /* I - second (0. - 60.9999999)  */
           int decimals,      /* I - number of decimal points to write      */
           char *datestr,     /* O - date string: "YYYY-MM-DDThh:mm:ss.ddd" */
                              /*   or "hh:mm:ss.ddd" if year, month day = 0 */
           int   *status)     /* IO - error status             */
/*
  Construct a date and time character string
*/
{
    int width;
    char errmsg[81];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    *datestr='\0';

    if (year != 0 || month != 0 || day !=0)
    { 
        if (ffverifydate(year, month, day, status) > 0)
	{
            ffpmsg("invalid date (fftm2s)");
            return(*status);
        }
    }

    if (hour < 0 || hour > 23)
    {
       sprintf(errmsg, 
       "input hour value is out of range 0 - 23: %d (fftm2s)", hour);
       ffpmsg(errmsg);
       return(*status = BAD_DATE);
    }
    else if (minute < 0 || minute > 59)
    {
       sprintf(errmsg, 
       "input minute value is out of range 0 - 59: %d (fftm2s)", minute);
       ffpmsg(errmsg);
       return(*status = BAD_DATE);
    }
    else if (second < 0. || second >= 61)
    {
       sprintf(errmsg, 
       "input second value is out of range 0 - 60.999: %f (fftm2s)", second);
       ffpmsg(errmsg);
       return(*status = BAD_DATE);
    }
    else if (decimals > 25)
    {
       sprintf(errmsg, 
       "input decimals value is out of range 0 - 25: %d (fftm2s)", decimals);
       ffpmsg(errmsg);
       return(*status = BAD_DATE);
    }

    if (decimals == 0)
       width = 2;
    else
       width = decimals + 3;

    if (decimals < 0)
    {
        /* a negative decimals value means return only the date, not time */
        sprintf(datestr, "%.4d-%.2d-%.2d", year, month, day);
    }
    else if (year == 0 && month == 0 && day == 0)
    {
        /* return only the time, not the date */
        sprintf(datestr, "%.2d:%.2d:%0*.*f",
            hour, minute, width, decimals, second);
    }
    else
    {
        /* return both the time and date */
        sprintf(datestr, "%.4d-%.2d-%.2dT%.2d:%.2d:%0*.*f",
            year, month, day, hour, minute, width, decimals, second);
    }
    return(*status);
}
/*-----------------------------------------------------------------*/
int ffs2tm(char *datestr,     /* I - date string: "YYYY-MM-DD"    */
                              /*     or "YYYY-MM-DDThh:mm:ss.ddd" */
                              /*     or "dd/mm/yy"                */
           int *year,         /* O - year (0 - 9999)              */
           int *month,        /* O - month (1 - 12)               */
           int *day,          /* O - day (1 - 31)                 */
           int *hour,          /* I - hour (0 - 23)                */
           int *minute,        /* I - minute (0 - 59)              */
           double *second,     /* I - second (0. - 60.9999999)     */
           int   *status)     /* IO - error status                */
/*
  Parse a date character string into date and time values
*/
{
    int slen;
    char errmsg[81];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (year)
       *year   = 0;
    if (month)
       *month  = 0;
    if (day)
       *day    = 0;
    if (hour)
       *hour   = 0;
    if (minute)
       *minute = 0;
    if (second)
       *second = 0.;

    if (!datestr)
    {
        ffpmsg("error: null input date string (ffs2tm)");
        return(*status = BAD_DATE);   /* Null datestr pointer ??? */
    }

    if (datestr[2] == '/' || datestr[4] == '-')
    {
        /*  Parse the year, month, and date */
        if (ffs2dt(datestr, year, month, day, status) > 0)
            return(*status);

        slen = strlen(datestr);
        if (slen == 8 || slen == 10)
            return(*status);               /* OK, no time fields */
        else if (slen < 19) 
        {
            ffpmsg("input date string has illegal format:");
            ffpmsg(datestr);
            return(*status = BAD_DATE);
        }

        else if (datestr[10] == 'T' && datestr[13] == ':' && datestr[16] == ':')
        {
          if (isdigit((int) datestr[11]) && isdigit((int) datestr[12])
           && isdigit((int) datestr[14]) && isdigit((int) datestr[15])
           && isdigit((int) datestr[17]) && isdigit((int) datestr[18]) )
            {
                if (slen > 19 && datestr[19] != '.')
                {
                  ffpmsg("input date string has illegal format:");
                  ffpmsg(datestr);
                  return(*status = BAD_DATE);
                }

                /* this is a new format string: "yyyy-mm-ddThh:mm:ss.dddd" */
                if (hour)
                    *hour   = atoi(&datestr[11]);

                if (minute)
                    *minute = atoi(&datestr[14]);

                if (second)
                    *second = atof(&datestr[17]);
            }
            else
            {
                  ffpmsg("input date string has illegal format:");
                  ffpmsg(datestr);
                  return(*status = BAD_DATE);
            }

        }
    }
    else   /* no date fields */
    {
        if (datestr[2] == ':' && datestr[5] == ':')   /* time string */
        {
            if (isdigit((int) datestr[0]) && isdigit((int) datestr[1])
             && isdigit((int) datestr[3]) && isdigit((int) datestr[4])
             && isdigit((int) datestr[6]) && isdigit((int) datestr[7]) )
            {
                 /* this is a time string: "hh:mm:ss.dddd" */
                 if (hour)
                    *hour   = atoi(&datestr[0]);

                 if (minute)
                    *minute = atoi(&datestr[3]);

                if (second)
                    *second = atof(&datestr[6]);
            }
            else
            {
                  ffpmsg("input date string has illegal format:");
                  ffpmsg(datestr);
                  return(*status = BAD_DATE);
            }

        }
        else
        {
                  ffpmsg("input date string has illegal format:");
                  ffpmsg(datestr);
                  return(*status = BAD_DATE);
        }

    }

    if (hour)
       if (*hour < 0 || *hour > 23)
       {
          sprintf(errmsg, 
          "hour value is out of range 0 - 23: %d (ffs2tm)", *hour);
          ffpmsg(errmsg);
          return(*status = BAD_DATE);
       }

    if (minute)
       if (*minute < 0 || *minute > 59)
       {
          sprintf(errmsg, 
          "minute value is out of range 0 - 59: %d (ffs2tm)", *minute);
          ffpmsg(errmsg);
          return(*status = BAD_DATE);
       }

    if (second)
       if (*second < 0 || *second >= 61.)
       {
          sprintf(errmsg, 
          "second value is out of range 0 - 60.9999: %f (ffs2tm)", *second);
          ffpmsg(errmsg);
          return(*status = BAD_DATE);
       }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgsdt( int *day, int *month, int *year, int *status )
{  
/*
      This routine is included for backward compatibility
            with the Fortran FITSIO library.

   ffgsdt : Get current System DaTe (GMT if available)

      Return integer values of the day, month, and year

         Function parameters:
            day      Day of the month
            month    Numerical month (1=Jan, etc.)
            year     Year (1999, 2000, etc.)
            status   output error status

*/
   time_t now;
   struct tm *date;

   now = time( NULL );
   date = gmtime(&now);         /* get GMT (= UTC) time */

   if (!date)                  /* GMT not available on this machine */
   {
       date = localtime(&now); 
   }

   *day = date->tm_mday;
   *month = date->tm_mon + 1;
   *year = date->tm_year + 1900;  /* tm_year is defined as years since 1900 */
   return( *status );
}
/*--------------------------------------------------------------------------*/
int ffpkns( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyroot,      /* I - root name of keywords to write       */
            int  nstart,        /* I - starting index number                */
            int  nkey,          /* I - number of keywords to write          */
            char *value[],      /* I - array of pointers to keyword values  */
            char *comm[],       /* I - array of pointers to keyword comment */
            int  *status)       /* IO - error status                        */
/*
  Write (put) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NKEY -1) inclusive.  Writes string keywords.
  The value strings will be truncated at 68 characters, and the HEASARC
  long string keyword convention is not supported by this routine.
*/
{
    char keyname[FLEN_KEYWORD], tcomment[FLEN_COMMENT];
    int ii, jj, repeat, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check if first comment string is to be repeated for all the keywords */
    /* by looking to see if the last non-blank character is a '&' char      */

    repeat = 0;

    if (comm)
    {
      len = strlen(comm[0]);

      while (len > 0  && comm[0][len - 1] == ' ')
        len--;                               /* ignore trailing blanks */

      if (comm[0][len - 1] == '&')
      {
        len = minvalue(len, FLEN_COMMENT);
        tcomment[0] = '\0';
        strncat(tcomment, comm[0], len-1); /* don't copy the final '&' char */
        repeat = 1;
      }
    }
    else
    {
      repeat = 1;
      tcomment[0] = '\0';
    }

    for (ii=0, jj=nstart; ii < nkey; ii++, jj++)
    {
        ffkeyn(keyroot, jj, keyname, status);
        if (repeat)
            ffpkys(fptr, keyname, value[ii], tcomment, status);
        else
            ffpkys(fptr, keyname, value[ii], comm[ii], status);

        if (*status > 0)
            return(*status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpknl( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyroot,      /* I - root name of keywords to write       */
            int  nstart,        /* I - starting index number                */
            int  nkey,          /* I - number of keywords to write          */
            int  *value,        /* I - array of keyword values              */
            char *comm[],       /* I - array of pointers to keyword comment */
            int  *status)       /* IO - error status                        */
/*
  Write (put) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NKEY -1) inclusive.  Writes logical keywords
  Values equal to zero will be written as a False FITS keyword value; any
  other non-zero value will result in a True FITS keyword.
*/
{
    char keyname[FLEN_KEYWORD], tcomment[FLEN_COMMENT];
    int ii, jj, repeat, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check if first comment string is to be repeated for all the keywords */
    /* by looking to see if the last non-blank character is a '&' char      */

    repeat = 0;
    if (comm)
    {
      len = strlen(comm[0]);

      while (len > 0  && comm[0][len - 1] == ' ')
        len--;                               /* ignore trailing blanks */

      if (comm[0][len - 1] == '&')
      {
        len = minvalue(len, FLEN_COMMENT);
        tcomment[0] = '\0';
        strncat(tcomment, comm[0], len-1); /* don't copy the final '&' char */
        repeat = 1;
      }
    }
    else
    {
      repeat = 1;
      tcomment[0] = '\0';
    }


    for (ii=0, jj=nstart; ii < nkey; ii++, jj++)
    {
        ffkeyn(keyroot, jj, keyname, status);

        if (repeat)
            ffpkyl(fptr, keyname, value[ii], tcomment, status);
        else
            ffpkyl(fptr, keyname, value[ii], comm[ii], status);

        if (*status > 0)
            return(*status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpknj( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyroot,      /* I - root name of keywords to write       */
            int  nstart,        /* I - starting index number                */
            int  nkey,          /* I - number of keywords to write          */
            long *value,        /* I - array of keyword values              */
            char *comm[],       /* I - array of pointers to keyword comment */
            int  *status)       /* IO - error status                        */
/*
  Write (put) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NKEY -1) inclusive.  Write integer keywords
*/
{
    char keyname[FLEN_KEYWORD], tcomment[FLEN_COMMENT];
    int ii, jj, repeat, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check if first comment string is to be repeated for all the keywords */
    /* by looking to see if the last non-blank character is a '&' char      */

    repeat = 0;

    if (comm)
    {
      len = strlen(comm[0]);

      while (len > 0  && comm[0][len - 1] == ' ')
        len--;                               /* ignore trailing blanks */

      if (comm[0][len - 1] == '&')
      {
        len = minvalue(len, FLEN_COMMENT);
        tcomment[0] = '\0';
        strncat(tcomment, comm[0], len-1); /* don't copy the final '&' char */
        repeat = 1;
      }
    }
    else
    {
      repeat = 1;
      tcomment[0] = '\0';
    }

    for (ii=0, jj=nstart; ii < nkey; ii++, jj++)
    {
        ffkeyn(keyroot, jj, keyname, status);
        if (repeat)
            ffpkyj(fptr, keyname, value[ii], tcomment, status);
        else
            ffpkyj(fptr, keyname, value[ii], comm[ii], status);

        if (*status > 0)
            return(*status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpknjj( fitsfile *fptr,    /* I - FITS file pointer                    */
            const char *keyroot,      /* I - root name of keywords to write       */
            int  nstart,        /* I - starting index number                */
            int  nkey,          /* I - number of keywords to write          */
            LONGLONG *value,    /* I - array of keyword values              */
            char *comm[],       /* I - array of pointers to keyword comment */
            int  *status)       /* IO - error status                        */
/*
  Write (put) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NKEY -1) inclusive.  Write integer keywords
*/
{
    char keyname[FLEN_KEYWORD], tcomment[FLEN_COMMENT];
    int ii, jj, repeat, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check if first comment string is to be repeated for all the keywords */
    /* by looking to see if the last non-blank character is a '&' char      */

    repeat = 0;

    if (comm)
    {
      len = strlen(comm[0]);

      while (len > 0  && comm[0][len - 1] == ' ')
        len--;                               /* ignore trailing blanks */

      if (comm[0][len - 1] == '&')
      {
        len = minvalue(len, FLEN_COMMENT);
        tcomment[0] = '\0';
        strncat(tcomment, comm[0], len-1); /* don't copy the final '&' char */
        repeat = 1;
      }
    }
    else
    {
      repeat = 1;
      tcomment[0] = '\0';
    }

    for (ii=0, jj=nstart; ii < nkey; ii++, jj++)
    {
        ffkeyn(keyroot, jj, keyname, status);
        if (repeat)
            ffpkyj(fptr, keyname, value[ii], tcomment, status);
        else
            ffpkyj(fptr, keyname, value[ii], comm[ii], status);

        if (*status > 0)
            return(*status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpknf( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyroot,      /* I - root name of keywords to write       */
            int  nstart,        /* I - starting index number                */
            int  nkey,          /* I - number of keywords to write          */
            float *value,       /* I - array of keyword values              */
            int decim,          /* I - number of decimals to display        */
            char *comm[],       /* I - array of pointers to keyword comment */
            int  *status)       /* IO - error status                        */
/*
  Write (put) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NKEY -1) inclusive.  Writes fixed float values.
*/
{
    char keyname[FLEN_KEYWORD], tcomment[FLEN_COMMENT];
    int ii, jj, repeat, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check if first comment string is to be repeated for all the keywords */
    /* by looking to see if the last non-blank character is a '&' char      */

    repeat = 0;

    if (comm)
    {
      len = strlen(comm[0]);

      while (len > 0  && comm[0][len - 1] == ' ')
        len--;                               /* ignore trailing blanks */

      if (comm[0][len - 1] == '&')
      {
        len = minvalue(len, FLEN_COMMENT);
        tcomment[0] = '\0';
        strncat(tcomment, comm[0], len-1); /* don't copy the final '&' char */
        repeat = 1;
      }
    }
    else
    {
      repeat = 1;
      tcomment[0] = '\0';
    }

    for (ii=0, jj=nstart; ii < nkey; ii++, jj++)
    {
        ffkeyn(keyroot, jj, keyname, status);
        if (repeat)
            ffpkyf(fptr, keyname, value[ii], decim, tcomment, status);
        else
            ffpkyf(fptr, keyname, value[ii], decim, comm[ii], status);

        if (*status > 0)
            return(*status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkne( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyroot,      /* I - root name of keywords to write       */
            int  nstart,        /* I - starting index number                */
            int  nkey,          /* I - number of keywords to write          */
            float *value,       /* I - array of keyword values              */
            int decim,          /* I - number of decimals to display        */
            char *comm[],       /* I - array of pointers to keyword comment */
            int  *status)       /* IO - error status                        */
/*
  Write (put) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NKEY -1) inclusive.  Writes exponential float values.
*/
{
    char keyname[FLEN_KEYWORD], tcomment[FLEN_COMMENT];
    int ii, jj, repeat, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check if first comment string is to be repeated for all the keywords */
    /* by looking to see if the last non-blank character is a '&' char      */

    repeat = 0;

    if (comm)
    {
      len = strlen(comm[0]);

      while (len > 0  && comm[0][len - 1] == ' ')
        len--;                               /* ignore trailing blanks */

      if (comm[0][len - 1] == '&')
      {
        len = minvalue(len, FLEN_COMMENT);
        tcomment[0] = '\0';
        strncat(tcomment, comm[0], len-1); /* don't copy the final '&' char */
        repeat = 1;
      }
    }
    else
    {
      repeat = 1;
      tcomment[0] = '\0';
    }

    for (ii=0, jj=nstart; ii < nkey; ii++, jj++)
    {
        ffkeyn(keyroot, jj, keyname, status);
        if (repeat)
            ffpkye(fptr, keyname, value[ii], decim, tcomment, status);
        else
            ffpkye(fptr, keyname, value[ii], decim, comm[ii], status);

        if (*status > 0)
            return(*status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpkng( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyroot,      /* I - root name of keywords to write       */
            int  nstart,        /* I - starting index number                */
            int  nkey,          /* I - number of keywords to write          */
            double *value,      /* I - array of keyword values              */
            int decim,          /* I - number of decimals to display        */
            char *comm[],       /* I - array of pointers to keyword comment */
            int  *status)       /* IO - error status                        */
/*
  Write (put) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NKEY -1) inclusive.  Writes fixed double values.
*/
{
    char keyname[FLEN_KEYWORD], tcomment[FLEN_COMMENT];
    int ii, jj, repeat, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check if first comment string is to be repeated for all the keywords */
    /* by looking to see if the last non-blank character is a '&' char      */

    repeat = 0;

    if (comm)
    {
      len = strlen(comm[0]);

      while (len > 0  && comm[0][len - 1] == ' ')
        len--;                               /* ignore trailing blanks */

      if (comm[0][len - 1] == '&')
      {
        len = minvalue(len, FLEN_COMMENT);
        tcomment[0] = '\0';
        strncat(tcomment, comm[0], len-1); /* don't copy the final '&' char */
        repeat = 1;
      }
    }
    else
    {
      repeat = 1;
      tcomment[0] = '\0';
    }

    for (ii=0, jj=nstart; ii < nkey; ii++, jj++)
    {
        ffkeyn(keyroot, jj, keyname, status);
        if (repeat)
            ffpkyg(fptr, keyname, value[ii], decim, tcomment, status);
        else
            ffpkyg(fptr, keyname, value[ii], decim, comm[ii], status);

        if (*status > 0)
            return(*status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpknd( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyroot,      /* I - root name of keywords to write       */
            int  nstart,        /* I - starting index number                */
            int  nkey,          /* I - number of keywords to write          */
            double *value,      /* I - array of keyword values              */
            int decim,          /* I - number of decimals to display        */
            char *comm[],       /* I - array of pointers to keyword comment */
            int  *status)       /* IO - error status                        */
/*
  Write (put) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NKEY -1) inclusive.  Writes exponential double values.
*/
{
    char keyname[FLEN_KEYWORD], tcomment[FLEN_COMMENT];
    int ii, jj, repeat, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check if first comment string is to be repeated for all the keywords */
    /* by looking to see if the last non-blank character is a '&' char      */

    repeat = 0;

    if (comm)
    {
      len = strlen(comm[0]);

      while (len > 0  && comm[0][len - 1] == ' ')
        len--;                               /* ignore trailing blanks */

      if (comm[0][len - 1] == '&')
      {
        len = minvalue(len, FLEN_COMMENT);
        tcomment[0] = '\0';
        strncat(tcomment, comm[0], len-1); /* don't copy the final '&' char */
        repeat = 1;
      }
    }
    else
    {
      repeat = 1;
      tcomment[0] = '\0';
    }

    for (ii=0, jj=nstart; ii < nkey; ii++, jj++)
    {
        ffkeyn(keyroot, jj, keyname, status);
        if (repeat)
            ffpkyd(fptr, keyname, value[ii], decim, tcomment, status);
        else
            ffpkyd(fptr, keyname, value[ii], decim, comm[ii], status);

        if (*status > 0)
            return(*status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffptdm( fitsfile *fptr, /* I - FITS file pointer                        */
            int colnum,     /* I - column number                            */
            int naxis,      /* I - number of axes in the data array         */
            long naxes[],   /* I - length of each data axis                 */
            int *status)    /* IO - error status                            */
/*
  write the TDIMnnn keyword describing the dimensionality of a column
*/
{
    char keyname[FLEN_KEYWORD], tdimstr[FLEN_VALUE], comm[FLEN_COMMENT];
    char value[80], message[81];
    int ii;
    long totalpix = 1, repeat;
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    if (colnum < 1 || colnum > 999)
    {
        ffpmsg("column number is out of range 1 - 999 (ffptdm)");
        return(*status = BAD_COL_NUM);
    }

    if (naxis < 1)
    {
        ffpmsg("naxis is less than 1 (ffptdm)");
        return(*status = BAD_DIMEN);
    }

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if ( (fptr->Fptr)->hdutype != BINARY_TBL)
    {
       ffpmsg(
    "Error: The TDIMn keyword is only allowed in BINTABLE extensions (ffptdm)");
       return(*status = NOT_BTABLE);
    }

    strcpy(tdimstr, "(");            /* start constructing the TDIM value */   

    for (ii = 0; ii < naxis; ii++)
    {
        if (ii > 0)
            strcat(tdimstr, ",");   /* append the comma separator */

        if (naxes[ii] < 0)
        {
            ffpmsg("one or more TDIM values are less than 0 (ffptdm)");
            return(*status = BAD_TDIM);
        }

        sprintf(value, "%ld", naxes[ii]);
        strcat(tdimstr, value);     /* append the axis size */

        totalpix *= naxes[ii];
    }

    colptr = (fptr->Fptr)->tableptr;  /* point to first column structure */
    colptr += (colnum - 1);      /* point to the specified column number */

    if ((long) colptr->trepeat != totalpix)
    {
      /* There is an apparent inconsistency between TDIMn and TFORMn. */
      /* The colptr->trepeat value may be out of date, so re-read     */
      /* the TFORMn keyword to be sure.                               */

      ffkeyn("TFORM", colnum, keyname, status);   /* construct TFORMn name  */
      ffgkys(fptr, keyname, value, NULL, status); /* read TFORMn keyword    */
      ffbnfm(value, NULL, &repeat, NULL, status); /* parse the repeat count */

      if (*status > 0 || repeat != totalpix)
      {
        sprintf(message,
        "column vector length, %ld, does not equal TDIMn array size, %ld",
        (long) colptr->trepeat, totalpix);
        ffpmsg(message);
        return(*status = BAD_TDIM);
      }
    }

    strcat(tdimstr, ")" );            /* append the closing parenthesis */

    strcpy(comm, "size of the multidimensional array");
    ffkeyn("TDIM", colnum, keyname, status);      /* construct TDIMn name */
    ffpkys(fptr, keyname, tdimstr, comm, status);  /* write the keyword */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffptdmll( fitsfile *fptr, /* I - FITS file pointer                      */
            int colnum,     /* I - column number                            */
            int naxis,      /* I - number of axes in the data array         */
            LONGLONG naxes[], /* I - length of each data axis               */
            int *status)    /* IO - error status                            */
/*
  write the TDIMnnn keyword describing the dimensionality of a column
*/
{
    char keyname[FLEN_KEYWORD], tdimstr[FLEN_VALUE], comm[FLEN_COMMENT];
    char value[80], message[81];
    int ii;
    LONGLONG totalpix = 1, repeat;
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    if (colnum < 1 || colnum > 999)
    {
        ffpmsg("column number is out of range 1 - 999 (ffptdm)");
        return(*status = BAD_COL_NUM);
    }

    if (naxis < 1)
    {
        ffpmsg("naxis is less than 1 (ffptdm)");
        return(*status = BAD_DIMEN);
    }

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if ( (fptr->Fptr)->hdutype != BINARY_TBL)
    {
       ffpmsg(
    "Error: The TDIMn keyword is only allowed in BINTABLE extensions (ffptdm)");
       return(*status = NOT_BTABLE);
    }

    strcpy(tdimstr, "(");            /* start constructing the TDIM value */   

    for (ii = 0; ii < naxis; ii++)
    {
        if (ii > 0)
            strcat(tdimstr, ",");   /* append the comma separator */

        if (naxes[ii] < 0)
        {
            ffpmsg("one or more TDIM values are less than 0 (ffptdm)");
            return(*status = BAD_TDIM);
        }

        /* cast to double because the 64-bit int conversion character in */
        /* sprintf is platform dependent ( %lld, %ld, %I64d )            */

        sprintf(value, "%.0f", (double) naxes[ii]);

        strcat(tdimstr, value);     /* append the axis size */

        totalpix *= naxes[ii];
    }

    colptr = (fptr->Fptr)->tableptr;  /* point to first column structure */
    colptr += (colnum - 1);      /* point to the specified column number */

    if ( colptr->trepeat != totalpix)
    {
      /* There is an apparent inconsistency between TDIMn and TFORMn. */
      /* The colptr->trepeat value may be out of date, so re-read     */
      /* the TFORMn keyword to be sure.                               */

      ffkeyn("TFORM", colnum, keyname, status);   /* construct TFORMn name  */
      ffgkys(fptr, keyname, value, NULL, status); /* read TFORMn keyword    */
      ffbnfmll(value, NULL, &repeat, NULL, status); /* parse the repeat count */

      if (*status > 0 || repeat != totalpix)
      {
        sprintf(message,
        "column vector length, %.0f, does not equal TDIMn array size, %.0f",
        (double) (colptr->trepeat), (double) totalpix);
        ffpmsg(message);
        return(*status = BAD_TDIM);
      }
    }

    strcat(tdimstr, ")" );            /* append the closing parenthesis */

    strcpy(comm, "size of the multidimensional array");
    ffkeyn("TDIM", colnum, keyname, status);      /* construct TDIMn name */
    ffpkys(fptr, keyname, tdimstr, comm, status);  /* write the keyword */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffphps( fitsfile *fptr, /* I - FITS file pointer                        */
            int bitpix,     /* I - number of bits per data value pixel      */
            int naxis,      /* I - number of axes in the data array         */
            long naxes[],   /* I - length of each data axis                 */
            int *status)    /* IO - error status                            */
/*
  write STANDARD set of required primary header keywords
*/
{
    int simple = 1;     /* does file conform to FITS standard? 1/0  */
    long pcount = 0;    /* number of group parameters (usually 0)   */
    long gcount = 1;    /* number of random groups (usually 1 or 0) */
    int extend = 1;     /* may FITS file have extensions?           */

    ffphpr(fptr, simple, bitpix, naxis, naxes, pcount, gcount, extend, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffphpsll( fitsfile *fptr, /* I - FITS file pointer                        */
            int bitpix,     /* I - number of bits per data value pixel      */
            int naxis,      /* I - number of axes in the data array         */
            LONGLONG naxes[],   /* I - length of each data axis                 */
            int *status)    /* IO - error status                            */
/*
  write STANDARD set of required primary header keywords
*/
{
    int simple = 1;     /* does file conform to FITS standard? 1/0  */
    LONGLONG pcount = 0;    /* number of group parameters (usually 0)   */
    LONGLONG gcount = 1;    /* number of random groups (usually 1 or 0) */
    int extend = 1;     /* may FITS file have extensions?           */

    ffphprll(fptr, simple, bitpix, naxis, naxes, pcount, gcount, extend, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffphpr( fitsfile *fptr, /* I - FITS file pointer                        */
            int simple,     /* I - does file conform to FITS standard? 1/0  */
            int bitpix,     /* I - number of bits per data value pixel      */
            int naxis,      /* I - number of axes in the data array         */
            long naxes[],   /* I - length of each data axis                 */
            LONGLONG pcount, /* I - number of group parameters (usually 0)   */
            LONGLONG gcount, /* I - number of random groups (usually 1 or 0) */
            int extend,     /* I - may FITS file have extensions?           */
            int *status)    /* IO - error status                            */
/*
  write required primary header keywords
*/
{
    int ii;
    LONGLONG naxesll[20];
   
    for (ii = 0; (ii < naxis) && (ii < 20); ii++)
       naxesll[ii] = naxes[ii];

    ffphprll(fptr, simple, bitpix, naxis, naxesll, pcount, gcount,
             extend, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffphprll( fitsfile *fptr, /* I - FITS file pointer                        */
            int simple,     /* I - does file conform to FITS standard? 1/0  */
            int bitpix,     /* I - number of bits per data value pixel      */
            int naxis,      /* I - number of axes in the data array         */
            LONGLONG naxes[], /* I - length of each data axis                 */
            LONGLONG pcount,  /* I - number of group parameters (usually 0)   */
            LONGLONG gcount,  /* I - number of random groups (usually 1 or 0) */
            int extend,     /* I - may FITS file have extensions?           */
            int *status)    /* IO - error status                            */
/*
  write required primary header keywords
*/
{
    int ii;
    long longbitpix, tnaxes[20];
    char name[FLEN_KEYWORD], comm[FLEN_COMMENT], message[FLEN_ERRMSG];

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if ((fptr->Fptr)->headend != (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        return(*status = HEADER_NOT_EMPTY);

    if (naxis != 0)   /* never try to compress a null image */
    {
      if ( (fptr->Fptr)->request_compress_type )
      {
      
       for (ii = 0; ii < naxis; ii++)
           tnaxes[ii] = (long) naxes[ii];
	   
        /* write header for a compressed image */
        imcomp_init_table(fptr, bitpix, naxis, tnaxes, 1, status);
        return(*status);
      }
    }  

    if ((fptr->Fptr)->curhdu == 0)
    {                /* write primary array header */
        if (simple)
            strcpy(comm, "file does conform to FITS standard");
        else
            strcpy(comm, "file does not conform to FITS standard");

        ffpkyl(fptr, "SIMPLE", simple, comm, status);
    }
    else
    {               /* write IMAGE extension header */
        strcpy(comm, "IMAGE extension");
        ffpkys(fptr, "XTENSION", "IMAGE", comm, status);
    }

    longbitpix = bitpix;

    /* test for the 3 special cases that represent unsigned integers */
    if (longbitpix == USHORT_IMG)
        longbitpix = SHORT_IMG;
    else if (longbitpix == ULONG_IMG)
        longbitpix = LONG_IMG;
    else if (longbitpix == SBYTE_IMG)
        longbitpix = BYTE_IMG;

    if (longbitpix != BYTE_IMG && longbitpix != SHORT_IMG && 
        longbitpix != LONG_IMG && longbitpix != LONGLONG_IMG &&
        longbitpix != FLOAT_IMG && longbitpix != DOUBLE_IMG)
    {
        sprintf(message,
        "Illegal value for BITPIX keyword: %d", bitpix);
        ffpmsg(message);
        return(*status = BAD_BITPIX);
    }

    strcpy(comm, "number of bits per data pixel");
    if (ffpkyj(fptr, "BITPIX", longbitpix, comm, status) > 0)
        return(*status);

    if (naxis < 0 || naxis > 999)
    {
        sprintf(message,
        "Illegal value for NAXIS keyword: %d", naxis);
        ffpmsg(message);
        return(*status = BAD_NAXIS);
    }

    strcpy(comm, "number of data axes");
    ffpkyj(fptr, "NAXIS", naxis, comm, status);

    strcpy(comm, "length of data axis ");
    for (ii = 0; ii < naxis; ii++)
    {
        if (naxes[ii] < 0)
        {
            sprintf(message,
            "Illegal negative value for NAXIS%d keyword: %.0f", ii + 1, (double) (naxes[ii]));
            ffpmsg(message);
            return(*status = BAD_NAXES);
        }

        sprintf(&comm[20], "%d", ii + 1);
        ffkeyn("NAXIS", ii + 1, name, status);
        ffpkyj(fptr, name, naxes[ii], comm, status);
    }

    if ((fptr->Fptr)->curhdu == 0)  /* the primary array */
    {
        if (extend)
        {
            /* only write EXTEND keyword if value = true */
            strcpy(comm, "FITS dataset may contain extensions");
            ffpkyl(fptr, "EXTEND", extend, comm, status);
        }

        if (pcount < 0)
        {
            ffpmsg("pcount value is less than 0");
            return(*status = BAD_PCOUNT);
        }

        else if (gcount < 1)
        {
            ffpmsg("gcount value is less than 1");
            return(*status = BAD_GCOUNT);
        }

        else if (pcount > 0 || gcount > 1)
        {
            /* only write these keyword if non-standard values */
            strcpy(comm, "random group records are present");
            ffpkyl(fptr, "GROUPS", 1, comm, status);

            strcpy(comm, "number of random group parameters");
            ffpkyj(fptr, "PCOUNT", pcount, comm, status);
  
            strcpy(comm, "number of random groups");
            ffpkyj(fptr, "GCOUNT", gcount, comm, status);
        }

      /* write standard block of self-documentating comments */
      ffprec(fptr,
      "COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy",
      status);
      ffprec(fptr,
      "COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H",
      status);
    }

    else  /* an IMAGE extension */

    {   /* image extension; cannot have random groups */
        if (pcount != 0)
        {
            ffpmsg("image extensions must have pcount = 0");
            *status = BAD_PCOUNT;
        }

        else if (gcount != 1)
        {
            ffpmsg("image extensions must have gcount = 1");
            *status = BAD_GCOUNT;
        }

        else
        {
            strcpy(comm, "required keyword; must = 0");
            ffpkyj(fptr, "PCOUNT", 0, comm, status);
  
            strcpy(comm, "required keyword; must = 1");
            ffpkyj(fptr, "GCOUNT", 1, comm, status);
        }
    }

    /* Write the BSCALE and BZERO keywords, if an unsigned integer image */
    if (bitpix == USHORT_IMG)
    {
        strcpy(comm, "offset data range to that of unsigned short");
        ffpkyg(fptr, "BZERO", 32768., 0, comm, status);
        strcpy(comm, "default scaling factor");
        ffpkyg(fptr, "BSCALE", 1.0, 0, comm, status);
    }
    else if (bitpix == ULONG_IMG)
    {
        strcpy(comm, "offset data range to that of unsigned long");
        ffpkyg(fptr, "BZERO", 2147483648., 0, comm, status);
        strcpy(comm, "default scaling factor");
        ffpkyg(fptr, "BSCALE", 1.0, 0, comm, status);
    }
    else if (bitpix == SBYTE_IMG)
    {
        strcpy(comm, "offset data range to that of signed byte");
        ffpkyg(fptr, "BZERO", -128., 0, comm, status);
        strcpy(comm, "default scaling factor");
        ffpkyg(fptr, "BSCALE", 1.0, 0, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffphtb(fitsfile *fptr,  /* I - FITS file pointer                        */
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
  Put required Header keywords into the ASCII TaBle:
*/
{
    int ii, ncols, gotmem = 0;
    long rowlen; /* must be 'long' because it is passed to ffgabc */
    char tfmt[30], name[FLEN_KEYWORD], comm[FLEN_COMMENT], extnm[FLEN_VALUE];

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (*status > 0)
        return(*status);
    else if ((fptr->Fptr)->headend != (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        return(*status = HEADER_NOT_EMPTY);
    else if (naxis1 < 0)
        return(*status = NEG_WIDTH);
    else if (naxis2 < 0)
        return(*status = NEG_ROWS);
    else if (tfields < 0 || tfields > 999)
        return(*status = BAD_TFIELDS);
    
    extnm[0] = '\0';
    if (extnmx)
        strncat(extnm, extnmx, FLEN_VALUE-1);

    rowlen = (long) naxis1;

    if (!tbcol || !tbcol[0] || (!naxis1 && tfields)) /* spacing not defined? */
    {
      /* allocate mem for tbcol; malloc can have problems allocating small */
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
    ffpkys(fptr, "XTENSION", "TABLE", "ASCII table extension", status);
    ffpkyj(fptr, "BITPIX", 8, "8-bit ASCII characters", status);
    ffpkyj(fptr, "NAXIS", 2, "2-dimensional ASCII table", status);
    ffpkyj(fptr, "NAXIS1", rowlen, "width of table in characters", status);
    ffpkyj(fptr, "NAXIS2", naxis2, "number of rows in table", status);
    ffpkyj(fptr, "PCOUNT", 0, "no group parameters (required keyword)", status);
    ffpkyj(fptr, "GCOUNT", 1, "one data group (required keyword)", status);
    ffpkyj(fptr, "TFIELDS", tfields, "number of fields in each row", status);

    for (ii = 0; ii < tfields; ii++) /* loop over every column */
    {
        if ( *(ttype[ii]) )  /* optional TTYPEn keyword */
        {
          sprintf(comm, "label for field %3d", ii + 1);
          ffkeyn("TTYPE", ii + 1, name, status);
          ffpkys(fptr, name, ttype[ii], comm, status);
        }

        if (tbcol[ii] < 1 || tbcol[ii] > rowlen)
           *status = BAD_TBCOL;

        sprintf(comm, "beginning column of field %3d", ii + 1);
        ffkeyn("TBCOL", ii + 1, name, status);
        ffpkyj(fptr, name, tbcol[ii], comm, status);

        strcpy(tfmt, tform[ii]);  /* required TFORMn keyword */
        ffupch(tfmt);
        ffkeyn("TFORM", ii + 1, name, status);
        ffpkys(fptr, name, tfmt, "Fortran-77 format of field", status);

        if (tunit)
        {
         if (tunit[ii] && *(tunit[ii]) )  /* optional TUNITn keyword */
         {
          ffkeyn("TUNIT", ii + 1, name, status);
          ffpkys(fptr, name, tunit[ii], "physical unit of field", status) ;
         }
        }

        if (*status > 0)
            break;       /* abort loop on error */
    }

    if (extnm[0])       /* optional EXTNAME keyword */
        ffpkys(fptr, "EXTNAME", extnm,
               "name of this ASCII table extension", status);

    if (*status > 0)
        ffpmsg("Failed to write ASCII table header keywords (ffphtb)");

    if (gotmem)
        free(tbcol); 

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffphbn(fitsfile *fptr,  /* I - FITS file pointer                        */
           LONGLONG naxis2,     /* I - number of rows in the table              */
           int tfields,     /* I - number of columns in the table           */
           char **ttype,    /* I - name of each column                      */
           char **tform,    /* I - value of TFORMn keyword for each column  */
           char **tunit,    /* I - value of TUNITn keyword for each column  */
           const char *extnmx,   /* I - value of EXTNAME keyword, if any         */
           LONGLONG pcount,     /* I - size of the variable length heap area    */
           int *status)     /* IO - error status                            */
/*
  Put required Header keywords into the Binary Table:
*/
{
    int ii, datatype, iread = 0;
    long repeat, width;
    LONGLONG naxis1;

    char tfmt[30], name[FLEN_KEYWORD], comm[FLEN_COMMENT], extnm[FLEN_VALUE];
    char *cptr;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if ((fptr->Fptr)->headend != (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        return(*status = HEADER_NOT_EMPTY);
    else if (naxis2 < 0)
        return(*status = NEG_ROWS);
    else if (pcount < 0)
        return(*status = BAD_PCOUNT);
    else if (tfields < 0 || tfields > 999)
        return(*status = BAD_TFIELDS);

    extnm[0] = '\0';
    if (extnmx)
        strncat(extnm, extnmx, FLEN_VALUE-1);

    ffpkys(fptr, "XTENSION", "BINTABLE", "binary table extension", status);
    ffpkyj(fptr, "BITPIX", 8, "8-bit bytes", status);
    ffpkyj(fptr, "NAXIS", 2, "2-dimensional binary table", status);

    naxis1 = 0;
    for (ii = 0; ii < tfields; ii++)  /* sum the width of each field */
    {
        ffbnfm(tform[ii], &datatype, &repeat, &width, status);

        if (datatype == TSTRING)
            naxis1 += repeat;   /* one byte per char */
        else if (datatype == TBIT)
            naxis1 += (repeat + 7) / 8;
        else if (datatype > 0)
            naxis1 += repeat * (datatype / 10);
        else if (tform[ii][0] == 'P' || tform[ii][1] == 'P'||
                 tform[ii][0] == 'p' || tform[ii][1] == 'p')
           /* this is a 'P' variable length descriptor (neg. datatype) */
            naxis1 += 8;
        else
           /* this is a 'Q' variable length descriptor (neg. datatype) */
            naxis1 += 16;

        if (*status > 0)
            break;       /* abort loop on error */
    }

    ffpkyj(fptr, "NAXIS1", naxis1, "width of table in bytes", status);
    ffpkyj(fptr, "NAXIS2", naxis2, "number of rows in table", status);

    /*
      the initial value of PCOUNT (= size of the variable length array heap)
      should always be zero.  If any variable length data is written, then
      the value of PCOUNT will be updated when the HDU is closed
    */
    ffpkyj(fptr, "PCOUNT", 0, "size of special data area", status);
    ffpkyj(fptr, "GCOUNT", 1, "one data group (required keyword)", status);
    ffpkyj(fptr, "TFIELDS", tfields, "number of fields in each row", status);

    for (ii = 0; ii < tfields; ii++) /* loop over every column */
    {
        if ( *(ttype[ii]) )  /* optional TTYPEn keyword */
        {
          sprintf(comm, "label for field %3d", ii + 1);
          ffkeyn("TTYPE", ii + 1, name, status);
          ffpkys(fptr, name, ttype[ii], comm, status);
        }

        strcpy(tfmt, tform[ii]);  /* required TFORMn keyword */
        ffupch(tfmt);

        ffkeyn("TFORM", ii + 1, name, status);
        strcpy(comm, "data format of field");

        ffbnfm(tfmt, &datatype, &repeat, &width, status);

        if (datatype == TSTRING)
        {
            strcat(comm, ": ASCII Character");

            /* Do sanity check to see if an ASCII table format was used,  */
            /* e.g., 'A8' instead of '8A', or a bad unit width eg '8A9'.  */
            /* Don't want to return an error status, so write error into  */
            /* the keyword comment.  */

            cptr = strchr(tfmt,'A');
            cptr++;

            if (cptr)
               iread = sscanf(cptr,"%ld", &width);

            if (iread == 1 && (width > repeat)) 
            {
              if (repeat == 1)
                strcpy(comm, "ERROR??  USING ASCII TABLE SYNTAX BY MISTAKE??");
              else
                strcpy(comm, "rAw FORMAT ERROR! UNIT WIDTH w > COLUMN WIDTH r");
            }
        }
        else if (datatype == TBIT)
           strcat(comm, ": BIT");
        else if (datatype == TBYTE)
           strcat(comm, ": BYTE");
        else if (datatype == TLOGICAL)
           strcat(comm, ": 1-byte LOGICAL");
        else if (datatype == TSHORT)
           strcat(comm, ": 2-byte INTEGER");
        else if (datatype == TUSHORT)
           strcat(comm, ": 2-byte INTEGER");
        else if (datatype == TLONG)
           strcat(comm, ": 4-byte INTEGER");
        else if (datatype == TLONGLONG)
           strcat(comm, ": 8-byte INTEGER");
        else if (datatype == TULONG)
           strcat(comm, ": 4-byte INTEGER");
        else if (datatype == TFLOAT)
           strcat(comm, ": 4-byte REAL");
        else if (datatype == TDOUBLE)
           strcat(comm, ": 8-byte DOUBLE");
        else if (datatype == TCOMPLEX)
           strcat(comm, ": COMPLEX");
        else if (datatype == TDBLCOMPLEX)
           strcat(comm, ": DOUBLE COMPLEX");
        else if (datatype < 0)
           strcat(comm, ": variable length array");

        if (abs(datatype) == TSBYTE) /* signed bytes */
        {
           /* Replace the 'S' with an 'B' in the TFORMn code */
           cptr = tfmt;
           while (*cptr != 'S') 
              cptr++;

           *cptr = 'B';
           ffpkys(fptr, name, tfmt, comm, status);

           /* write the TZEROn and TSCALn keywords */
           ffkeyn("TZERO", ii + 1, name, status);
           strcpy(comm, "offset for signed bytes");

           ffpkyg(fptr, name, -128., 0, comm, status);

           ffkeyn("TSCAL", ii + 1, name, status);
           strcpy(comm, "data are not scaled");
           ffpkyg(fptr, name, 1., 0, comm, status);
        }
        else if (abs(datatype) == TUSHORT) 
        {
           /* Replace the 'U' with an 'I' in the TFORMn code */
           cptr = tfmt;
           while (*cptr != 'U') 
              cptr++;

           *cptr = 'I';
           ffpkys(fptr, name, tfmt, comm, status);

           /* write the TZEROn and TSCALn keywords */
           ffkeyn("TZERO", ii + 1, name, status);
           strcpy(comm, "offset for unsigned integers");

           ffpkyg(fptr, name, 32768., 0, comm, status);

           ffkeyn("TSCAL", ii + 1, name, status);
           strcpy(comm, "data are not scaled");
           ffpkyg(fptr, name, 1., 0, comm, status);
        }
        else if (abs(datatype) == TULONG) 
        {
           /* Replace the 'V' with an 'J' in the TFORMn code */
           cptr = tfmt;
           while (*cptr != 'V') 
              cptr++;

           *cptr = 'J';
           ffpkys(fptr, name, tfmt, comm, status);

           /* write the TZEROn and TSCALn keywords */
           ffkeyn("TZERO", ii + 1, name, status);
           strcpy(comm, "offset for unsigned integers");

           ffpkyg(fptr, name, 2147483648., 0, comm, status);

           ffkeyn("TSCAL", ii + 1, name, status);
           strcpy(comm, "data are not scaled");
           ffpkyg(fptr, name, 1., 0, comm, status);
        }
        else
        {
           ffpkys(fptr, name, tfmt, comm, status);
        }

        if (tunit)
        {
         if (tunit[ii] && *(tunit[ii]) ) /* optional TUNITn keyword */
         {
          ffkeyn("TUNIT", ii + 1, name, status);
          ffpkys(fptr, name, tunit[ii],
             "physical unit of field", status);
         }
        }

        if (*status > 0)
            break;       /* abort loop on error */
    }

    if (extnm[0])       /* optional EXTNAME keyword */
        ffpkys(fptr, "EXTNAME", extnm,
               "name of this binary table extension", status);

    if (*status > 0)
        ffpmsg("Failed to write binary table header keywords (ffphbn)");

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffphext(fitsfile *fptr,  /* I - FITS file pointer                       */
           const char *xtensionx,   /* I - value for the XTENSION keyword          */
           int bitpix,       /* I - value for the BIXPIX keyword            */
           int naxis,        /* I - value for the NAXIS keyword             */
           long naxes[],     /* I - value for the NAXISn keywords           */
           LONGLONG pcount,  /* I - value for the PCOUNT keyword            */
           LONGLONG gcount,  /* I - value for the GCOUNT keyword            */
           int *status)      /* IO - error status                           */
/*
  Put required Header keywords into a conforming extension:
*/
{
    char message[FLEN_ERRMSG],comm[81], name[20], xtension[FLEN_VALUE];
    int ii;
 
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (*status > 0)
        return(*status);
    else if ((fptr->Fptr)->headend != (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        return(*status = HEADER_NOT_EMPTY);

    if (naxis < 0 || naxis > 999)
    {
        sprintf(message,
        "Illegal value for NAXIS keyword: %d", naxis);
        ffpmsg(message);
        return(*status = BAD_NAXIS);
    }

    xtension[0] = '\0';
    strncat(xtension, xtensionx, FLEN_VALUE-1);

    ffpkys(fptr, "XTENSION", xtension, "extension type", status);
    ffpkyj(fptr, "BITPIX",   bitpix,   "number of bits per data pixel", status);
    ffpkyj(fptr, "NAXIS",    naxis,    "number of data axes", status);

    strcpy(comm, "length of data axis ");
    for (ii = 0; ii < naxis; ii++)
    {
        if (naxes[ii] < 0)
        {
            sprintf(message,
            "Illegal negative value for NAXIS%d keyword: %.0f", ii + 1, (double) (naxes[ii]));
            ffpmsg(message);
            return(*status = BAD_NAXES);
        }

        sprintf(&comm[20], "%d", ii + 1);
        ffkeyn("NAXIS", ii + 1, name, status);
        ffpkyj(fptr, name, naxes[ii], comm, status);
    }


    ffpkyj(fptr, "PCOUNT", pcount, " ", status);
    ffpkyj(fptr, "GCOUNT", gcount, " ", status);

    if (*status > 0)
        ffpmsg("Failed to write extension header keywords (ffphext)");

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffi2c(LONGLONG ival,  /* I - value to be converted to a string */
          char *cval,     /* O - character string representation of the value */
          int *status)    /* IO - error status */
/*
  convert  value to a null-terminated formatted string.
*/
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    cval[0] = '\0';

#if defined(_MSC_VER)
    /* Microsoft Visual C++ 6.0 uses '%I64d' syntax  for 8-byte integers */
    if (sprintf(cval, "%I64d", ival) < 0)

#elif (USE_LL_SUFFIX == 1)
    if (sprintf(cval, "%lld", ival) < 0)
#else
    if (sprintf(cval, "%ld", ival) < 0)
#endif
    {
        ffpmsg("Error in ffi2c converting integer to string");
        *status = BAD_I2C;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffl2c(int lval,    /* I - value to be converted to a string */
          char *cval,  /* O - character string representation of the value */
          int *status) /* IO - error status ) */
/*
  convert logical value to a null-terminated formatted string.  If the
  input value == 0, then the output character is the letter F, else
  the output character is the letter T.  The output string is null terminated.
*/
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (lval)
        strcpy(cval,"T");
    else
        strcpy(cval,"F");

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffs2c(const char *instr, /* I - null terminated input string  */
          char *outstr,      /* O - null terminated quoted output string */
          int *status)       /* IO - error status */
/*
  convert an input string to a quoted string. Leading spaces 
  are significant.  FITS string keyword values must be at least 
  8 chars long so pad out string with spaces if necessary.
      Example:   km/s ==> 'km/s    '
  Single quote characters in the input string will be replace by
  two single quote characters. e.g., o'brian ==> 'o''brian'
*/
{
    size_t len, ii, jj;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (!instr)            /* a null input pointer?? */
    {
       strcpy(outstr, "''");   /* a null FITS string */
       return(*status);
    }

    outstr[0] = '\'';      /* start output string with a quote */

    len = strlen(instr);
    if (len > 68)
        len = 68;    /* limit input string to 68 chars */

    for (ii=0, jj=1; ii < len && jj < 69; ii++, jj++)
    {
        outstr[jj] = instr[ii];  /* copy each char from input to output */
        if (instr[ii] == '\'')
        {
            jj++;
            outstr[jj]='\'';   /* duplicate any apostrophies in the input */
        }
    }

    for (; jj < 9; jj++)       /* pad string so it is at least 8 chars long */
        outstr[jj] = ' ';

    if (jj == 70)   /* only occurs if the last char of string was a quote */
        outstr[69] = '\0';
    else
    {
        outstr[jj] = '\'';         /* append closing quote character */
        outstr[jj+1] = '\0';          /* terminate the string */
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffr2f(float fval,   /* I - value to be converted to a string */
          int  decim,   /* I - number of decimal places to display */
          char *cval,   /* O - character string representation of the value */
          int  *status) /* IO - error status */
/*
  convert float value to a null-terminated F format string
*/
{
    char *cptr;
        
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    cval[0] = '\0';

    if (decim < 0)
    {
        ffpmsg("Error in ffr2f:  no. of decimal places < 0");
        return(*status = BAD_DECIM);
    }

    if (sprintf(cval, "%.*f", decim, fval) < 0)
    {
        ffpmsg("Error in ffr2f converting float to string");
        *status = BAD_F2C;
    }

    /* replace comma with a period (e.g. in French locale) */
    if ( (cptr = strchr(cval, ','))) *cptr = '.';

    /* test if output string is 'NaN', 'INDEF', or 'INF' */
    if (strchr(cval, 'N'))
    {
        ffpmsg("Error in ffr2f: float value is a NaN or INDEF");
        *status = BAD_F2C;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffr2e(float fval,  /* I - value to be converted to a string */
         int decim,    /* I - number of decimal places to display */
         char *cval,   /* O - character string representation of the value */
         int *status)  /* IO - error status */
/*
  convert float value to a null-terminated exponential format string
*/
{
    char *cptr;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    cval[0] = '\0';

    if (decim < 0)
    {   /* use G format if decim is negative */
        if ( sprintf(cval, "%.*G", -decim, fval) < 0)
        {
            ffpmsg("Error in ffr2e converting float to string");
            *status = BAD_F2C;
        }
        else
        {
            /* test if E format was used, and there is no displayed decimal */
            if ( !strchr(cval, '.') && strchr(cval,'E') )
            {
                /* reformat value with a decimal point and single zero */
                if ( sprintf(cval, "%.1E", fval) < 0)
                {
                    ffpmsg("Error in ffr2e converting float to string");
                    *status = BAD_F2C;
                }

                return(*status);  
            }
        }
    }
    else
    {
        if ( sprintf(cval, "%.*E", decim, fval) < 0)
        {
            ffpmsg("Error in ffr2e converting float to string");
            *status = BAD_F2C;
        }
    }

    if (*status <= 0)
    {
        /* replace comma with a period (e.g. in French locale) */
        if ( (cptr = strchr(cval, ','))) *cptr = '.';

        /* test if output string is 'NaN', 'INDEF', or 'INF' */
        if (strchr(cval, 'N'))
        {
            ffpmsg("Error in ffr2e: float value is a NaN or INDEF");
            *status = BAD_F2C;
        }
        else if ( !strchr(cval, '.') && !strchr(cval,'E') )
        {
            /* add decimal point if necessary to distinquish from integer */
            strcat(cval, ".");
        }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffd2f(double dval,  /* I - value to be converted to a string */
          int decim,    /* I - number of decimal places to display */
          char *cval,   /* O - character string representation of the value */
          int *status)  /* IO - error status */
/*
  convert double value to a null-terminated F format string
*/
{
    char *cptr;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    cval[0] = '\0';

    if (decim < 0)
    {
        ffpmsg("Error in ffd2f:  no. of decimal places < 0");
        return(*status = BAD_DECIM);
    }

    if (sprintf(cval, "%.*f", decim, dval) < 0)
    {
        ffpmsg("Error in ffd2f converting double to string");
        *status = BAD_F2C;
    }

    /* replace comma with a period (e.g. in French locale) */
    if ( (cptr = strchr(cval, ','))) *cptr = '.';

    /* test if output string is 'NaN', 'INDEF', or 'INF' */
    if (strchr(cval, 'N'))
    {
        ffpmsg("Error in ffd2f: double value is a NaN or INDEF");
        *status = BAD_F2C;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffd2e(double dval,  /* I - value to be converted to a string */
          int decim,    /* I - number of decimal places to display */
          char *cval,   /* O - character string representation of the value */
          int *status)  /* IO - error status */
/*
  convert double value to a null-terminated exponential format string.
*/
{
    char *cptr;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    cval[0] = '\0';

    if (decim < 0)
    {   /* use G format if decim is negative */
        if ( sprintf(cval, "%.*G", -decim, dval) < 0)
        {
            ffpmsg("Error in ffd2e converting float to string");
            *status = BAD_F2C;
        }
        else
        {
            /* test if E format was used, and there is no displayed decimal */
            if ( !strchr(cval, '.') && strchr(cval,'E') )
            {
                /* reformat value with a decimal point and single zero */
                if ( sprintf(cval, "%.1E", dval) < 0)
                {
                    ffpmsg("Error in ffd2e converting float to string");
                    *status = BAD_F2C;
                }

                return(*status);  
            }
        }
    }
    else
    {
        if ( sprintf(cval, "%.*E", decim, dval) < 0)
        {
            ffpmsg("Error in ffd2e converting float to string");
            *status = BAD_F2C;
        }
    }

    if (*status <= 0)
    {
        /* replace comma with a period (e.g. in French locale) */
        if ( (cptr = strchr(cval, ','))) *cptr = '.';

        /* test if output string is 'NaN', 'INDEF', or 'INF' */
        if (strchr(cval, 'N'))
        {
            ffpmsg("Error in ffd2e: double value is a NaN or INDEF");
            *status = BAD_F2C;
        }
        else if ( !strchr(cval, '.') && !strchr(cval,'E') )
        {
            /* add decimal point if necessary to distinquish from integer */
            strcat(cval, ".");
        }
    }

    return(*status);
}

