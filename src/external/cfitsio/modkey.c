/*  This file, modkey.c, contains routines that modify, insert, or update  */
/*  keywords in a FITS header.                                             */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
/* stddef.h is apparently needed to define size_t */
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include "fitsio2.h"
/*--------------------------------------------------------------------------*/
int ffuky( fitsfile *fptr,     /* I - FITS file pointer        */
           int  datatype,      /* I - datatype of the value    */
           const char *keyname,/* I - name of keyword to write */
           void *value,        /* I - keyword value            */
           const char *comm,   /* I - keyword comment          */
           int  *status)       /* IO - error status            */
/*
  Update the keyword, value and comment in the FITS header.
  The datatype is specified by the 2nd argument.
*/
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (datatype == TSTRING)
    {
        ffukys(fptr, keyname, (char *) value, comm, status);
    }
    else if (datatype == TBYTE)
    {
        ffukyj(fptr, keyname, (LONGLONG) *(unsigned char *) value, comm, status);
    }
    else if (datatype == TSBYTE)
    {
        ffukyj(fptr, keyname, (LONGLONG) *(signed char *) value, comm, status);
    }
    else if (datatype == TUSHORT)
    {
        ffukyj(fptr, keyname, (LONGLONG) *(unsigned short *) value, comm, status);
    }
    else if (datatype == TSHORT)
    {
        ffukyj(fptr, keyname, (LONGLONG) *(short *) value, comm, status);
    }
    else if (datatype == TINT)
    {
        ffukyj(fptr, keyname, (LONGLONG) *(int *) value, comm, status);
    }
    else if (datatype == TUINT)
    {
        ffukyg(fptr, keyname, (double) *(unsigned int *) value, 0,
               comm, status);
    }
    else if (datatype == TLOGICAL)
    {
        ffukyl(fptr, keyname, *(int *) value, comm, status);
    }
    else if (datatype == TULONG)
    {
        ffukyg(fptr, keyname, (double) *(unsigned long *) value, 0,
               comm, status);
    }
    else if (datatype == TLONG)
    {
        ffukyj(fptr, keyname, (LONGLONG) *(long *) value, comm, status);
    }
    else if (datatype == TLONGLONG)
    {
        ffukyj(fptr, keyname, *(LONGLONG *) value, comm, status);
    }
    else if (datatype == TFLOAT)
    {
        ffukye(fptr, keyname, *(float *) value, -7, comm, status);
    }
    else if (datatype == TDOUBLE)
    {
        ffukyd(fptr, keyname, *(double *) value, -15, comm, status);
    }
    else if (datatype == TCOMPLEX)
    {
        ffukyc(fptr, keyname, (float *) value, -7, comm, status);
    }
    else if (datatype == TDBLCOMPLEX)
    {
        ffukym(fptr, keyname, (double *) value, -15, comm, status);
    }
    else
        *status = BAD_DATATYPE;

    return(*status);
} 
/*--------------------------------------------------------------------------*/
int ffukyu(fitsfile *fptr,      /* I - FITS file pointer  */
           const char *keyname, /* I - keyword name       */
           const char *comm,    /* I - keyword comment    */
           int *status)         /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkyu(fptr, keyname, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkyu(fptr, keyname, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukys(fitsfile *fptr,       /* I - FITS file pointer  */
           const char *keyname,  /* I - keyword name       */
           const char *value,    /* I - keyword value      */
           const char *comm,     /* I - keyword comment    */
           int *status)          /* IO - error status      */ 
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkys(fptr, keyname, value, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkys(fptr, keyname, value, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukls(fitsfile *fptr,      /* I - FITS file pointer  */
           const char *keyname, /* I - keyword name       */
           const char *value,   /* I - keyword value      */
           const char *comm,    /* I - keyword comment    */
           int *status)         /* IO - error status      */ 
{
    /* update a long string keyword */

    int tstatus;
    char junk[FLEN_ERRMSG];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkls(fptr, keyname, value, comm, status) == KEY_NO_EXIST)
    {
        /* since the ffmkls call failed, it wrote a bogus error message */
        fits_read_errmsg(junk);  /* clear the error message */
	
        *status = tstatus;
        ffpkls(fptr, keyname, value, comm, status);
    }
    return(*status);
}/*--------------------------------------------------------------------------*/
int ffukyl(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           int value,          /* I - keyword value      */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkyl(fptr, keyname, value, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkyl(fptr, keyname, value, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukyj(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           LONGLONG value,     /* I - keyword value      */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkyj(fptr, keyname, value, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkyj(fptr, keyname, value, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukyf(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           float value,        /* I - keyword value      */
           int decim,          /* I - no of decimals     */         
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkyf(fptr, keyname, value, decim, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkyf(fptr, keyname, value, decim, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukye(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           float value,        /* I - keyword value      */
           int decim,          /* I - no of decimals     */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkye(fptr, keyname, value, decim, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkye(fptr, keyname, value, decim, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukyg(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           double value,       /* I - keyword value      */
           int decim,          /* I - no of decimals     */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkyg(fptr, keyname, value, decim, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkyg(fptr, keyname, value, decim, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukyd(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           double value,       /* I - keyword value      */
           int decim,          /* I - no of decimals     */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkyd(fptr, keyname, value, decim, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkyd(fptr, keyname, value, decim, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukfc(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           float *value,       /* I - keyword value      */
           int decim,          /* I - no of decimals     */         
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkfc(fptr, keyname, value, decim, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkfc(fptr, keyname, value, decim, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukyc(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           float *value,       /* I - keyword value      */
           int decim,          /* I - no of decimals     */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkyc(fptr, keyname, value, decim, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkyc(fptr, keyname, value, decim, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukfm(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           double *value,      /* I - keyword value      */
           int decim,          /* I - no of decimals     */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkfm(fptr, keyname, value, decim, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkfm(fptr, keyname, value, decim, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffukym(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           double *value,      /* I - keyword value      */
           int decim,          /* I - no of decimals     */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmkym(fptr, keyname, value, decim, comm, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffpkym(fptr, keyname, value, decim, comm, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffucrd(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           const char *card,   /* I - card string value  */
           int *status)        /* IO - error status      */
{
    int tstatus;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    tstatus = *status;

    if (ffmcrd(fptr, keyname, card, status) == KEY_NO_EXIST)
    {
        *status = tstatus;
        ffprec(fptr, card, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmrec(fitsfile *fptr,    /* I - FITS file pointer               */
           int nkey,          /* I - number of the keyword to modify */
           const char *card,  /* I - card string value               */
           int *status)       /* IO - error status                   */
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffmaky(fptr, nkey+1, status);
    ffmkey(fptr, card, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmcrd(fitsfile *fptr,      /* I - FITS file pointer  */
           const char *keyname, /* I - keyword name       */
           const char *card,    /* I - card string value  */
           int *status)         /* IO - error status      */
{
    char tcard[FLEN_CARD], valstring[FLEN_CARD], comm[FLEN_CARD], value[FLEN_CARD];
    char nextcomm[FLEN_COMMENT];
    int keypos, len;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgcrd(fptr, keyname, tcard, status) > 0)
        return(*status);

    ffmkey(fptr, card, status);

    /* calc position of keyword in header */
    keypos = (int) ((((fptr->Fptr)->nextkey) - ((fptr->Fptr)->headstart[(fptr->Fptr)->curhdu])) / 80) + 1;

    ffpsvc(tcard, valstring, comm, status);

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check for string value which may be continued over multiple keywords */
    ffpmrk(); /* put mark on message stack; erase any messages after this */
    ffc2s(valstring, value, status);   /* remove quotes and trailing spaces */

    if (*status == VALUE_UNDEFINED) {
       ffcmrk();  /* clear any spurious error messages, back to the mark */
       *status = 0;
    } else {
 
      len = strlen(value);

      while (len && value[len - 1] == '&')  /* ampersand used as continuation char */
      {
        ffgcnt(fptr, value, nextcomm, status);
        if (*value)
        {
            ffdrec(fptr, keypos, status);  /* delete the keyword */
            len = strlen(value);
        }
        else   /* a null valstring indicates no continuation */
            len = 0;
      }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmnam(fitsfile *fptr,     /* I - FITS file pointer     */
           const char *oldname,/* I - existing keyword name */
           const char *newname,/* I - new name for keyword  */
           int *status)        /* IO - error status         */
{
    char comm[FLEN_COMMENT];
    char value[FLEN_VALUE];
    char card[FLEN_CARD];
 
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, oldname, value, comm, status) > 0)
        return(*status);

    ffmkky(newname, value, comm, card, status);  /* construct the card */
    ffmkey(fptr, card, status);  /* rewrite with new name */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmcom(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    char oldcomm[FLEN_COMMENT];
    char value[FLEN_VALUE];
    char card[FLEN_CARD];
 
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, value, oldcomm, status) > 0)
        return(*status);

    ffmkky(keyname, value, comm, card, status);  /* construct the card */
    ffmkey(fptr, card, status);  /* rewrite with new comment */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpunt(fitsfile *fptr,     /* I - FITS file pointer   */
           const char *keyname,/* I - keyword name        */
           const char *unit,   /* I - keyword unit string */
           int *status)        /* IO - error status       */
/*
    Write (put) the units string into the comment field of the existing keyword.
    This routine uses a  FITS convention  in which the units are enclosed in 
    square brackets following the '/' comment field delimiter, e.g.:

    KEYWORD =                   12 / [kpc] comment string goes here
*/
{
    char oldcomm[FLEN_COMMENT];
    char newcomm[FLEN_COMMENT];
    char value[FLEN_VALUE];
    char card[FLEN_CARD];
    char *loc;
    size_t len;
 
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, value, oldcomm, status) > 0)
        return(*status);

    /* copy the units string to the new comment string if not null */
    if (*unit)
    {
        strcpy(newcomm, "[");
        strncat(newcomm, unit, 45);  /* max allowed length is about 45 chars */
        strcat(newcomm, "] ");
        len = strlen(newcomm);  
        len = FLEN_COMMENT - len - 1;  /* amount of space left in the field */
    }
    else
    {
        newcomm[0] = '\0';
        len = FLEN_COMMENT - 1;
    }

    if (oldcomm[0] == '[')  /* check for existing units field */
    {
        loc = strchr(oldcomm, ']');  /* look for the closing bracket */
        if (loc)
        {
            loc++;
            while (*loc == ' ')   /* skip any blank spaces */
               loc++;

            strncat(newcomm, loc, len);  /* concat remainder of comment */
        }
        else
        {
            strncat(newcomm, oldcomm, len);  /* append old comment onto new */
        }
    }
    else
    {
        strncat(newcomm, oldcomm, len);
    }

    ffmkky(keyname, value, newcomm, card, status);  /* construct the card */
    ffmkey(fptr, card, status);  /* rewrite with new units string */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkyu(fitsfile *fptr,     /* I - FITS file pointer  */
           const char *keyname,/* I - keyword name       */
           const char *comm,   /* I - keyword comment    */
           int *status)        /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    strcpy(valstring," ");  /* create a dummy value string */

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkys(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           const char *value,       /* I - keyword value      */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
  /* NOTE: This routine does not support long continued strings */
  /*  It will correctly overwrite an existing long continued string, */
  /*  but it will not write a new long string.  */

    char oldval[FLEN_VALUE], valstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD], nextcomm[FLEN_COMMENT];
    int len, keypos;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, oldval, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    ffs2c(value, valstring, status);   /* convert value to a string */

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status); /* overwrite the previous keyword */

    keypos = (int) (((((fptr->Fptr)->nextkey) - ((fptr->Fptr)->headstart[(fptr->Fptr)->curhdu])) / 80) + 1);

    if (*status > 0)           
        return(*status);

    /* check if old string value was continued over multiple keywords */
    ffpmrk(); /* put mark on message stack; erase any messages after this */
    ffc2s(oldval, valstring, status); /* remove quotes and trailing spaces */

    if (*status == VALUE_UNDEFINED) {
       ffcmrk();  /* clear any spurious error messages, back to the mark */
       *status = 0;
    } else {
        
      len = strlen(valstring);

      while (len && valstring[len - 1] == '&')  /* ampersand is continuation char */
      {
        ffgcnt(fptr, valstring, nextcomm, status);
        if (*valstring)
        {
            ffdrec(fptr, keypos, status);  /* delete the continuation */
            len = strlen(valstring);
        }
        else   /* a null valstring indicates no continuation */
            len = 0;
      }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkls( fitsfile *fptr,           /* I - FITS file pointer        */
            const char *keyname,      /* I - name of keyword to write */
            const char *value,        /* I - keyword value            */
            const char *incomm,       /* I - keyword comment          */
            int  *status)             /* IO - error status            */
/*
  Modify the value and optionally the comment of a long string keyword.
  This routine supports the
  HEASARC long string convention and can modify arbitrarily long string
  keyword values.  The value is continued over multiple keywords that
  have the name COMTINUE without an equal sign in column 9 of the card.
  This routine also supports simple string keywords which are less than
  69 characters in length.

  This routine is not very efficient, so it should be used sparingly.
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD], tmpkeyname[FLEN_CARD];
    char comm[FLEN_COMMENT];
    char tstring[FLEN_VALUE], *cptr;
    char *longval;
    int next, remain, vlen, nquote, nchar, namelen, contin, tstatus = -1;
    int nkeys, keypos;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (!incomm || incomm[0] == '&')  /* preserve the old comment string */
    {
        ffghps(fptr, &nkeys, &keypos, status); /* save current position */

        if (ffgkls(fptr, keyname, &longval, comm, status) > 0)
            return(*status);            /* keyword doesn't exist */

        free(longval);  /* don't need the old value */

        /* move back to previous position to ensure that we delete */
        /* the right keyword in case there are more than one keyword */
        /* with this same name. */
        ffgrec(fptr, keypos - 1, card, status); 
    } else {
        /* copy the input comment string */
        strncpy(comm, incomm, FLEN_COMMENT-1);
        comm[FLEN_COMMENT-1] = '\0';
    }

    /* delete the old keyword */
    if (ffdkey(fptr, keyname, status) > 0)
        return(*status);            /* keyword doesn't exist */

    ffghps(fptr, &nkeys, &keypos, status); /* save current position */

    /* now construct the new keyword, and insert into header */
    remain = strlen(value);    /* number of characters to write out */
    next = 0;                  /* pointer to next character to write */
    
    /* count the number of single quote characters in the string */
    nquote = 0;
    cptr = strchr(value, '\'');   /* search for quote character */

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
    while (remain > 0)
    {
        strncpy(tstring, &value[next], nchar); /* copy string to temp buff */
        tstring[nchar] = '\0';
        ffs2c(tstring, valstring, status);  /* put quotes around the string */

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
           ffmkky("CONTINUE", valstring, comm, card, status); /* make keyword */
           strncpy(&card[8], "   ",  2);  /* overwrite the '=' */
        }
        else
        {
           ffmkky(keyname, valstring, comm, card, status);  /* make keyword */
        }

        ffirec(fptr, keypos, card, status);  /* insert the keyword */
       
        keypos++;        /* next insert position */
        contin = 1;
        remain -= nchar;
        next  += nchar;
        nchar = 68 - nquote;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkyl(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           int value,               /* I - keyword value      */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    ffl2c(value, valstring, status);   /* convert value to a string */

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkyj(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           LONGLONG value,          /* I - keyword value      */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    ffi2c(value, valstring, status);   /* convert value to a string */

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkyf(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           float value,             /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    ffr2f(value, decim, valstring, status);   /* convert value to a string */

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkye(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           float value,             /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    ffr2e(value, decim, valstring, status);   /* convert value to a string */

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkyg(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           double value,            /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    ffd2f(value, decim, valstring, status);   /* convert value to a string */

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkyd(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           double value,            /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    ffd2e(value, decim, valstring, status);   /* convert value to a string */

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkfc(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           float *value,            /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE], tmpstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    strcpy(valstring, "(" );
    ffr2f(value[0], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ", ");
    ffr2f(value[1], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ")");

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkyc(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           float *value,            /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE], tmpstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    strcpy(valstring, "(" );
    ffr2e(value[0], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ", ");
    ffr2e(value[1], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ")");

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkfm(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           double *value,           /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE], tmpstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    strcpy(valstring, "(" );
    ffd2f(value[0], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ", ");
    ffd2f(value[1], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ")");

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkym(fitsfile *fptr,    /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           double *value,     /* I - keyword value      */
           int decim,         /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */
           int *status)       /* IO - error status      */
{
    char valstring[FLEN_VALUE], tmpstring[FLEN_VALUE];
    char oldcomm[FLEN_COMMENT];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, oldcomm, status) > 0)
        return(*status);                               /* get old comment */

    strcpy(valstring, "(" );
    ffd2e(value[0], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ", ");
    ffd2e(value[1], decim, tmpstring, status); /* convert to string */
    strcat(valstring, tmpstring);
    strcat(valstring, ")");

    if (!comm || comm[0] == '&')  /* preserve the current comment string */
        ffmkky(keyname, valstring, oldcomm, card, status);
    else
        ffmkky(keyname, valstring, comm, card, status);

    ffmkey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikyu(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
/*
  Insert a null-valued keyword and comment into the FITS header.  
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    strcpy(valstring," ");  /* create a dummy value string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffikey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikys(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           const char *value,       /* I - keyword value      */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffs2c(value, valstring, status);   /* put quotes around the string */

    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffikey(fptr, card, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikls( fitsfile *fptr,           /* I - FITS file pointer        */
            const char *keyname,      /* I - name of keyword to write */
            const char *value,        /* I - keyword value            */
            const char *comm,         /* I - keyword comment          */
            int  *status)             /* IO - error status            */
/*
  Insert a long string keyword.  This routine supports the
  HEASARC long string convention and can insert arbitrarily long string
  keyword values.  The value is continued over multiple keywords that
  have the name COMTINUE without an equal sign in column 9 of the card.
  This routine also supports simple string keywords which are less than
  69 characters in length.
*/
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD], tmpkeyname[FLEN_CARD];
    char tstring[FLEN_VALUE], *cptr;
    int next, remain, vlen, nquote, nchar, namelen, contin, tstatus = -1;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /*  construct the new keyword, and insert into header */
    remain = strlen(value);    /* number of characters to write out */
    next = 0;                  /* pointer to next character to write */
    
    /* count the number of single quote characters in the string */
    nquote = 0;
    cptr = strchr(value, '\'');   /* search for quote character */

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
    while (remain > 0)
    {
        strncpy(tstring, &value[next], nchar); /* copy string to temp buff */
        tstring[nchar] = '\0';
        ffs2c(tstring, valstring, status);  /* put quotes around the string */

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
           ffmkky("CONTINUE", valstring, comm, card, status); /* make keyword */
           strncpy(&card[8], "   ",  2);  /* overwrite the '=' */
        }
        else
        {
           ffmkky(keyname, valstring, comm, card, status);  /* make keyword */
        }

        ffikey(fptr, card, status);  /* insert the keyword */
       
        contin = 1;
        remain -= nchar;
        next  += nchar;
        nchar = 68 - nquote;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikyl(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           int value,               /* I - keyword value      */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffl2c(value, valstring, status);   /* convert logical to 'T' or 'F' */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikyj(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           LONGLONG value,          /* I - keyword value      */
           const char *comm,        /* I - keyword comment    */
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffi2c(value, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikyf(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           float value,             /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */ 
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffr2f(value, decim, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status); 
}
/*--------------------------------------------------------------------------*/
int ffikye(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           float value,             /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */ 
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffr2e(value, decim, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikyg(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           double value,            /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */ 
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffd2f(value, decim, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikyd(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           double value,            /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */ 
           int *status)             /* IO - error status      */
{
    char valstring[FLEN_VALUE];
    char card[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffd2e(value, decim, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, card, status);  /* construct the keyword*/
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikfc(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           float *value,            /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */ 
           int *status)             /* IO - error status      */
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
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikyc(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           float *value,            /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */ 
           int *status)             /* IO - error status      */
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
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikfm(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           double *value,           /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */ 
           int *status)             /* IO - error status      */
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
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikym(fitsfile *fptr,          /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           double *value,           /* I - keyword value      */
           int decim,               /* I - no of decimals     */
           const char *comm,        /* I - keyword comment    */ 
           int *status)             /* IO - error status      */
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
    ffikey(fptr, card, status);  /* write the keyword*/

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffirec(fitsfile *fptr,    /* I - FITS file pointer              */
           int nkey,          /* I - position to insert new keyword */
           const char *card,  /* I - card string value              */
           int *status)       /* IO - error status                  */
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    ffmaky(fptr, nkey, status);  /* move to insert position */
    ffikey(fptr, card, status);  /* insert the keyword card */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffikey(fitsfile *fptr,    /* I - FITS file pointer  */
           const char *card,  /* I - card string value  */
           int *status)       /* IO - error status      */
/*
  insert a keyword at the position of (fptr->Fptr)->nextkey
*/
{
    int ii, len, nshift, keylength;
    long nblocks;
    LONGLONG bytepos;
    char *inbuff, *outbuff, *tmpbuff, buff1[FLEN_CARD], buff2[FLEN_CARD];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if ( ((fptr->Fptr)->datastart - (fptr->Fptr)->headend) == 80) /* only room for END card */
    {
        nblocks = 1;
        if (ffiblk(fptr, nblocks, 0, status) > 0) /* add new 2880-byte block*/
            return(*status);  
    }

    /* no. keywords to shift */
    nshift= (int) (( (fptr->Fptr)->headend - (fptr->Fptr)->nextkey ) / 80); 

    strncpy(buff2, card, 80);     /* copy card to output buffer */
    buff2[80] = '\0';

    len = strlen(buff2);

    /* silently replace any illegal characters with a space */
    for (ii=0; ii < len; ii++)   
        if (buff2[ii] < ' ' || buff2[ii] > 126) buff2[ii] = ' ';

    for (ii=len; ii < 80; ii++)   /* fill buffer with spaces if necessary */
        buff2[ii] = ' ';

    keylength = strcspn(buff2, "=");
    if (keylength == 80) keylength = 8;
    
    for (ii=0; ii < keylength; ii++)       /* make sure keyword name is uppercase */
        buff2[ii] = toupper(buff2[ii]);

    fftkey(buff2, status);        /* test keyword name contains legal chars */

/*  no need to do this any more, since any illegal characters have been removed
    fftrec(buff2, status);  */      /* test rest of keyword for legal chars   */

    inbuff = buff1;
    outbuff = buff2;

    bytepos = (fptr->Fptr)->nextkey;           /* pointer to next keyword in header */
    ffmbyt(fptr, bytepos, REPORT_EOF, status);

    for (ii = 0; ii < nshift; ii++) /* shift each keyword down one position */
    {
        ffgbyt(fptr, 80, inbuff, status);   /* read the current keyword */

        ffmbyt(fptr, bytepos, REPORT_EOF, status); /* move back */
        ffpbyt(fptr, 80, outbuff, status);  /* overwrite with other buffer */

        tmpbuff = inbuff;   /* swap input and output buffers */
        inbuff = outbuff;
        outbuff = tmpbuff;

        bytepos += 80;
    }

    ffpbyt(fptr, 80, outbuff, status);  /* write the final keyword */

    (fptr->Fptr)->headend += 80; /* increment the position of the END keyword */
    (fptr->Fptr)->nextkey += 80; /* increment the pointer to next keyword */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdkey(fitsfile *fptr,    /* I - FITS file pointer  */
           const char *keyname,     /* I - keyword name       */
           int *status)       /* IO - error status      */
/*
  delete a specified header keyword
*/
{
    int keypos, len;
    char valstring[FLEN_VALUE], comm[FLEN_COMMENT], value[FLEN_VALUE];
    char message[FLEN_ERRMSG], nextcomm[FLEN_COMMENT];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgkey(fptr, keyname, valstring, comm, status) > 0) /* read keyword */
    {
        sprintf(message, "Could not find the %s keyword to delete (ffdkey)",
                keyname);
        ffpmsg(message);
        return(*status);
    }

    /* calc position of keyword in header */
    keypos = (int) ((((fptr->Fptr)->nextkey) - ((fptr->Fptr)->headstart[(fptr->Fptr)->curhdu])) / 80);

    ffdrec(fptr, keypos, status);  /* delete the keyword */

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check for string value which may be continued over multiple keywords */
    ffpmrk(); /* put mark on message stack; erase any messages after this */
    ffc2s(valstring, value, status);   /* remove quotes and trailing spaces */

    if (*status == VALUE_UNDEFINED) {
       ffcmrk();  /* clear any spurious error messages, back to the mark */
       *status = 0;
    } else {
 
      len = strlen(value);

      while (len && value[len - 1] == '&')  /* ampersand used as continuation char */
      {
        ffgcnt(fptr, value, nextcomm, status);
        if (*value)
        {
            ffdrec(fptr, keypos, status);  /* delete the keyword */
            len = strlen(value);
        }
        else   /* a null valstring indicates no continuation */
            len = 0;
      }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdstr(fitsfile *fptr,    /* I - FITS file pointer  */
           const char *string,     /* I - keyword name       */
           int *status)       /* IO - error status      */
/*
  delete a specified header keyword containing the input string
*/
{
    int keypos, len;
    char valstring[FLEN_VALUE], comm[FLEN_COMMENT], value[FLEN_VALUE];
    char card[FLEN_CARD], message[FLEN_ERRMSG], nextcomm[FLEN_COMMENT];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (ffgstr(fptr, string, card, status) > 0) /* read keyword */
    {
        sprintf(message, "Could not find the %s keyword to delete (ffdkey)",
                string);
        ffpmsg(message);
        return(*status);
    }

    /* calc position of keyword in header */
    keypos = (int) ((((fptr->Fptr)->nextkey) - ((fptr->Fptr)->headstart[(fptr->Fptr)->curhdu])) / 80);

    ffdrec(fptr, keypos, status);  /* delete the keyword */

    /* check for string value which may be continued over multiple keywords */
    ffpsvc(card, valstring, comm, status);

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* check for string value which may be continued over multiple keywords */
    ffpmrk(); /* put mark on message stack; erase any messages after this */
    ffc2s(valstring, value, status);   /* remove quotes and trailing spaces */

    if (*status == VALUE_UNDEFINED) {
       ffcmrk();  /* clear any spurious error messages, back to the mark */
       *status = 0;
    } else {
 
      len = strlen(value);

      while (len && value[len - 1] == '&')  /* ampersand used as continuation char */
      {
        ffgcnt(fptr, value, nextcomm, status);
        if (*value)
        {
            ffdrec(fptr, keypos, status);  /* delete the keyword */
            len = strlen(value);
        }
        else   /* a null valstring indicates no continuation */
            len = 0;
      }
    }

    return(*status);
}/*--------------------------------------------------------------------------*/
int ffdrec(fitsfile *fptr,   /* I - FITS file pointer  */
           int keypos,       /* I - position in header of keyword to delete */
           int *status)      /* IO - error status      */
/*
  Delete a header keyword at position keypos. The 1st keyword is at keypos=1.
*/
{
    int ii, nshift;
    LONGLONG bytepos;
    char *inbuff, *outbuff, *tmpbuff, buff1[81], buff2[81];
    char message[FLEN_ERRMSG];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (keypos < 1 ||
        keypos > (fptr->Fptr)->headend - (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] / 80 )
        return(*status = KEY_OUT_BOUNDS);

    (fptr->Fptr)->nextkey = (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] + (keypos - 1) * 80;

    nshift=(int) (( (fptr->Fptr)->headend - (fptr->Fptr)->nextkey ) / 80); /* no. keywords to shift */

    if (nshift <= 0)
    {
        sprintf(message, "Cannot delete keyword number %d.  It does not exist.",
                keypos);
        ffpmsg(message);
        return(*status = KEY_OUT_BOUNDS);
    }

    bytepos = (fptr->Fptr)->headend - 80;  /* last keyword in header */  

    /* construct a blank keyword */
    strcpy(buff2, "                                        ");
    strcat(buff2, "                                        ");
    inbuff  = buff1;
    outbuff = buff2;
    for (ii = 0; ii < nshift; ii++) /* shift each keyword up one position */
    {

        ffmbyt(fptr, bytepos, REPORT_EOF, status);
        ffgbyt(fptr, 80, inbuff, status);   /* read the current keyword */

        ffmbyt(fptr, bytepos, REPORT_EOF, status);
        ffpbyt(fptr, 80, outbuff, status);  /* overwrite with next keyword */

        tmpbuff = inbuff;   /* swap input and output buffers */
        inbuff = outbuff;
        outbuff = tmpbuff;

        bytepos -= 80;
    }

    (fptr->Fptr)->headend -= 80; /* decrement the position of the END keyword */
    return(*status);
}

