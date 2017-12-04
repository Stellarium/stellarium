/*  This file, getkey.c, contains routines that read keywords from         */
/*  a FITS header.                                                         */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
/* stddef.h is apparently needed to define size_t */
#include <stddef.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int ffghsp(fitsfile *fptr,  /* I - FITS file pointer                     */
           int *nexist,     /* O - number of existing keywords in header */
           int *nmore,      /* O - how many more keywords will fit       */
           int *status)     /* IO - error status                         */
/*
  returns the number of existing keywords (not counting the END keyword)
  and the number of more keyword that will fit in the current header 
  without having to insert more FITS blocks.
*/
{
    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (nexist)
        *nexist = (int) (( ((fptr->Fptr)->headend) - 
                ((fptr->Fptr)->headstart[(fptr->Fptr)->curhdu]) ) / 80);

    if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
    {
      if (nmore)
        *nmore = -1;   /* data not written yet, so room for any keywords */
    }
    else
    {
      /* calculate space available between the data and the END card */
      if (nmore)
        *nmore = (int) (((fptr->Fptr)->datastart - (fptr->Fptr)->headend) / 80 - 1);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghps(fitsfile *fptr, /* I - FITS file pointer                     */
          int *nexist,     /* O - number of existing keywords in header */
          int *position,   /* O - position of next keyword to be read   */
          int *status)     /* IO - error status                         */
/*
  return the number of existing keywords and the position of the next
  keyword that will be read.
*/
{
    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (nexist)
      *nexist = (int) (( ((fptr->Fptr)->headend) - ((fptr->Fptr)->headstart[(fptr->Fptr)->curhdu]) ) / 80);

    if (position)
      *position = (int) (( ((fptr->Fptr)->nextkey) - ((fptr->Fptr)->headstart[(fptr->Fptr)->curhdu]) ) / 80 + 1);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffnchk(fitsfile *fptr,  /* I - FITS file pointer                     */
           int *status)     /* IO - error status                         */
/*
  function returns the position of the first null character (ASCII 0), if
  any, in the current header.  Null characters are illegal, but the other
  CFITSIO routines that read the header will not detect this error, because
  the null gets interpreted as a normal end of string character.
*/
{
    long ii, nblock;
    LONGLONG bytepos;
    int length, nullpos;
    char block[2881];
    
    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
    {
        return(0);  /* Don't check a file that is just being created.  */
                    /* It cannot contain nulls since CFITSIO wrote it. */
    }
    else
    {
        /* calculate number of blocks in the header */
        nblock = (long) (( (fptr->Fptr)->datastart - 
                   (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] ) / 2880);
    }

    bytepos = (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu];
    ffmbyt(fptr, bytepos, REPORT_EOF, status);  /* move to read pos. */

    block[2880] = '\0';
    for (ii = 0; ii < nblock; ii++)
    {
        if (ffgbyt(fptr, 2880, block, status) > 0)
            return(0);   /* read error of some sort */

        length = strlen(block);
        if (length != 2880)
        {
            nullpos = (ii * 2880) + length + 1;
            return(nullpos);
        }
    }

    return(0);
}
/*--------------------------------------------------------------------------*/
int ffmaky(fitsfile *fptr,    /* I - FITS file pointer                    */
          int nrec,           /* I - one-based keyword number to move to  */
          int *status)        /* IO - error status                        */
{
/*
  move pointer to the specified absolute keyword position.  E.g. this keyword 
  will then be read by the next call to ffgnky.
*/
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    (fptr->Fptr)->nextkey = (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] + ( (nrec - 1) * 80);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmrky(fitsfile *fptr,    /* I - FITS file pointer                   */
          int nmove,          /* I - relative number of keywords to move */
          int *status)        /* IO - error status                       */
{
/*
  move pointer to the specified keyword position relative to the current
  position.  E.g. this keyword  will then be read by the next call to ffgnky.
*/

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    (fptr->Fptr)->nextkey += (nmove * 80);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgnky(fitsfile *fptr,  /* I - FITS file pointer     */
           char *card,      /* O - card string           */
           int *status)     /* IO - error status         */
/*
  read the next keyword from the header - used internally by cfitsio
*/
{
    int jj, nrec;
    LONGLONG bytepos, endhead;
    char message[FLEN_ERRMSG];

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    card[0] = '\0';  /* make sure card is terminated, even affer read error */

/*
  Check that nextkey points to a legal keyword position.  Note that headend
  is the current end of the header, i.e., the position where a new keyword
  would be appended, however, if there are more than 1 FITS block worth of
  blank keywords at the end of the header (36 keywords per 2880 byte block)
  then the actual physical END card must be located at a starting position
  which is just 2880 bytes prior to the start of the data unit.
*/

    bytepos = (fptr->Fptr)->nextkey;
    endhead = maxvalue( ((fptr->Fptr)->headend), ((fptr->Fptr)->datastart - 2880) );

    /* nextkey must be < endhead and > than  headstart */
    if (bytepos > endhead ||  
        bytepos < (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] ) 
    {
        nrec= (int) ((bytepos - (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu]) / 80 + 1);
        sprintf(message, "Cannot get keyword number %d.  It does not exist.",
                nrec);
        ffpmsg(message);
        return(*status = KEY_OUT_BOUNDS);
    }
      
    ffmbyt(fptr, bytepos, REPORT_EOF, status);  /* move to read pos. */

    card[80] = '\0';  /* make sure card is terminate, even if ffgbyt fails */

    if (ffgbyt(fptr, 80, card, status) <= 0) 
    {
        (fptr->Fptr)->nextkey += 80;   /* increment pointer to next keyword */

        /* strip off trailing blanks with terminated string */
        jj = 79;
        while (jj >= 0 && card[jj] == ' ')
               jj--;

        card[jj + 1] = '\0';
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgnxk( fitsfile *fptr,     /* I - FITS file pointer              */
            char **inclist,     /* I - list of included keyword names */
            int ninc,           /* I - number of names in inclist     */
            char **exclist,     /* I - list of excluded keyword names */
            int nexc,           /* I - number of names in exclist     */
            char *card,         /* O - first matching keyword         */
            int  *status)       /* IO - error status                  */
/*
    Return the next keyword that matches one of the names in inclist
    but does not match any of the names in exclist.  The search
    goes from the current position to the end of the header, only.
    Wild card characters may be used in the name lists ('*', '?' and '#').
*/
{
    int casesn, match, exact, namelen;
    long ii, jj;
    char keybuf[FLEN_CARD], keyname[FLEN_KEYWORD];

    card[0] = '\0';
    if (*status > 0)
        return(*status);

    casesn = FALSE;

    /* get next card, and return with an error if hit end of header */
    while( ffgcrd(fptr, "*", keybuf, status) <= 0)
    {
        ffgknm(keybuf, keyname, &namelen, status); /* get the keyword name */
        
        /* does keyword match any names in the include list? */
        for (ii = 0; ii < ninc; ii++)
        {
            ffcmps(inclist[ii], keyname, casesn, &match, &exact);
            if (match)
            {
                /* does keyword match any names in the exclusion list? */
                jj = -1;
                while ( ++jj < nexc )
                {
                    ffcmps(exclist[jj], keyname, casesn, &match, &exact);
                    if (match)
                        break;
                }

                if (jj >= nexc)
                {
                    /* not in exclusion list, so return this keyword */
                    strcat(card, keybuf);
                    return(*status);
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgky( fitsfile *fptr,     /* I - FITS file pointer        */
           int  datatype,      /* I - datatype of the value    */
           const char *keyname,      /* I - name of keyword to read  */
           void *value,        /* O - keyword value            */
           char *comm,         /* O - keyword comment          */
           int  *status)       /* IO - error status            */
/*
  Read (get) the keyword value and comment from the FITS header.
  Reads a keyword value with the datatype specified by the 2nd argument.
*/
{
    long longval;
    double doubleval;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (datatype == TSTRING)
    {
        ffgkys(fptr, keyname, (char *) value, comm, status);
    }
    else if (datatype == TBYTE)
    {
        if (ffgkyj(fptr, keyname, &longval, comm, status) <= 0)
        {
            if (longval > UCHAR_MAX || longval < 0)
                *status = NUM_OVERFLOW;
            else
                *(unsigned char *) value = (unsigned char) longval;
        }
    }
    else if (datatype == TSBYTE)
    {
        if (ffgkyj(fptr, keyname, &longval, comm, status) <= 0)
        {
            if (longval > 127 || longval < -128)
                *status = NUM_OVERFLOW;
            else
                *(signed char *) value = (signed char) longval;
        }
    }
    else if (datatype == TUSHORT)
    {
        if (ffgkyj(fptr, keyname, &longval, comm, status) <= 0)
        {
            if (longval > (long) USHRT_MAX || longval < 0)
                *status = NUM_OVERFLOW;
            else
                *(unsigned short *) value = (unsigned short) longval;
        }
    }
    else if (datatype == TSHORT)
    {
        if (ffgkyj(fptr, keyname, &longval, comm, status) <= 0)
        {
            if (longval > SHRT_MAX || longval < SHRT_MIN)
                *status = NUM_OVERFLOW;
            else
                *(short *) value = (short) longval;
        }
    }
    else if (datatype == TUINT)
    {
        if (ffgkyj(fptr, keyname, &longval, comm, status) <= 0)
        {
            if (longval > (long) UINT_MAX || longval < 0)
                *status = NUM_OVERFLOW;
            else
                *(unsigned int *) value = longval;
        }
    }
    else if (datatype == TINT)
    {
        if (ffgkyj(fptr, keyname, &longval, comm, status) <= 0)
        {
            if (longval > INT_MAX || longval < INT_MIN)
                *status = NUM_OVERFLOW;
            else
                *(int *) value = longval;
        }
    }
    else if (datatype == TLOGICAL)
    {
        ffgkyl(fptr, keyname, (int *) value, comm, status);
    }
    else if (datatype == TULONG)
    {
        if (ffgkyd(fptr, keyname, &doubleval, comm, status) <= 0)
        {
            if (doubleval > (double) ULONG_MAX || doubleval < 0)
                *status = NUM_OVERFLOW;
            else
                 *(unsigned long *) value = (unsigned long) doubleval;
        }
    }
    else if (datatype == TLONG)
    {
        ffgkyj(fptr, keyname, (long *) value, comm, status);
    }
    else if (datatype == TLONGLONG)
    {
        ffgkyjj(fptr, keyname, (LONGLONG *) value, comm, status);
    }
    else if (datatype == TFLOAT)
    {
        ffgkye(fptr, keyname, (float *) value, comm, status);
    }
    else if (datatype == TDOUBLE)
    {
        ffgkyd(fptr, keyname, (double *) value, comm, status);
    }
    else if (datatype == TCOMPLEX)
    {
        ffgkyc(fptr, keyname, (float *) value, comm, status);
    }
    else if (datatype == TDBLCOMPLEX)
    {
        ffgkym(fptr, keyname, (double *) value, comm, status);
    }
    else
        *status = BAD_DATATYPE;

    return(*status);
} 
/*--------------------------------------------------------------------------*/
int ffgkey( fitsfile *fptr,     /* I - FITS file pointer        */
            const char *keyname,      /* I - name of keyword to read  */
            char *keyval,       /* O - keyword value            */
            char *comm,         /* O - keyword comment          */
            int  *status)       /* IO - error status            */
/*
  Read (get) the named keyword, returning the keyword value and comment.
  The value is just the literal string of characters in the value field
  of the keyword.  In the case of a string valued keyword, the returned
  value includes the leading and closing quote characters.  The value may be
  up to 70 characters long, and the comment may be up to 72 characters long.
  If the keyword has no value (no equal sign in column 9) then a null value
  is returned.
*/
{
    char card[FLEN_CARD];

    keyval[0] = '\0';
    if (comm)
       comm[0] = '\0';

    if (*status > 0)
        return(*status);

    if (ffgcrd(fptr, keyname, card, status) > 0)    /* get the 80-byte card */
        return(*status);

    ffpsvc(card, keyval, comm, status);      /* parse the value and comment */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgrec( fitsfile *fptr,     /* I - FITS file pointer          */
            int nrec,           /* I - number of keyword to read  */
            char *card,         /* O - keyword card               */
            int  *status)       /* IO - error status              */
/*
  Read (get) the nrec-th keyword, returning the entire keyword card up to
  80 characters long.  The first keyword in the header has nrec = 1, not 0.
  The returned card value is null terminated with any trailing blank 
  characters removed.  If nrec = 0, then this routine simply moves the
  current header pointer to the top of the header.
*/
{
    if (*status > 0)
        return(*status);

    if (nrec == 0)
    {
        ffmaky(fptr, 1, status);  /* simply move to beginning of header */
        if (card)
            card[0] = '\0';           /* and return null card */
    }
    else if (nrec > 0)
    {
        ffmaky(fptr, nrec, status);
        ffgnky(fptr, card, status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcrd( fitsfile *fptr,     /* I - FITS file pointer        */
            const char *name,         /* I - name of keyword to read  */
            char *card,         /* O - keyword card             */
            int  *status)       /* IO - error status            */
/*
  Read (get) the named keyword, returning the entire keyword card up to
  80 characters long.  
  The returned card value is null terminated with any trailing blank 
  characters removed.

  If the input name contains wild cards ('?' matches any single char
  and '*' matches any sequence of chars, # matches any string of decimal
  digits) then the search ends once the end of header is reached and does 
  not automatically resume from the top of the header.
*/
{
    int nkeys, nextkey, ntodo, namelen, namelen_limit, namelenminus1, cardlen;
    int ii = 0, jj, kk, wild, match, exact, hier = 0;
    char keyname[FLEN_KEYWORD], cardname[FLEN_KEYWORD];
    char *ptr1, *ptr2, *gotstar;

    if (*status > 0)
        return(*status);

    *keyname = '\0';
    
    while (name[ii] == ' ')  /* skip leading blanks in name */
        ii++;

    strncat(keyname, &name[ii], FLEN_KEYWORD - 1);

    namelen = strlen(keyname);

    while (namelen > 0 && keyname[namelen - 1] == ' ')
         namelen--;            /* ignore trailing blanks in name */

    keyname[namelen] = '\0';  /* terminate the name */

    for (ii=0; ii < namelen; ii++)       
        keyname[ii] = toupper(keyname[ii]);    /*  make upper case  */

    if (FSTRNCMP("HIERARCH", keyname, 8) == 0)
    {
        if (namelen == 8)
        {
            /* special case: just looking for any HIERARCH keyword */
            hier = 1;
        }
        else
        {
            /* ignore the leading HIERARCH and look for the 'real' name */
            /* starting with first non-blank character following HIERARCH */
            ptr1 = keyname;
            ptr2 = &keyname[8];

            while(*ptr2 == ' ')
                ptr2++;

            namelen = 0;
            while(*ptr2)
            {
                *ptr1 = *ptr2;
                 ptr1++;
                 ptr2++;
                 namelen++;
            }
            *ptr1 = '\0';
        }
    }

    /* does input name contain wild card chars?  ('?',  '*', or '#') */
    /* wild cards are currently not supported with HIERARCH keywords */

    namelen_limit = namelen;
    gotstar = 0;
    if (namelen < 9 && 
       (strchr(keyname,'?') || (gotstar = strchr(keyname,'*')) || 
        strchr(keyname,'#')) )
    {
        wild = 1;

        /* if we found a '*' wild card in the name, there might be */
        /* more than one.  Support up to 2 '*' in the template. */
        /* Thus we need to compare keywords whose names have at least */
        /* namelen - 2 characters.                                   */
        if (gotstar)
           namelen_limit -= 2;           
    }
    else
        wild = 0;

    ffghps(fptr, &nkeys, &nextkey, status); /* get no. keywords and position */

    namelenminus1 = maxvalue(namelen - 1, 1);
    ntodo = nkeys - nextkey + 1;  /* first, read from next keyword to end */
    for (jj=0; jj < 2; jj++)
    {
      for (kk = 0; kk < ntodo; kk++)
      {
        ffgnky(fptr, card, status);     /* get next keyword */

        if (hier)
        {
           if (FSTRNCMP("HIERARCH", card, 8) == 0)
                return(*status);  /* found a HIERARCH keyword */
        }
        else
        {
          ffgknm(card, cardname, &cardlen, status); /* get the keyword name */

          if (cardlen >= namelen_limit)  /* can't match if card < name */
          { 
            /* if there are no wild cards, lengths must be the same */
            if (!( !wild && cardlen != namelen) )
            {
              for (ii=0; ii < cardlen; ii++)
              {    
                /* make sure keyword is in uppercase */
                if (cardname[ii] > 96)
                {
                  /* This assumes the ASCII character set in which */
                  /* upper case characters start at ASCII(97)  */
                  /* Timing tests showed that this is 20% faster */
                  /* than calling the isupper function.          */

                  cardname[ii] = toupper(cardname[ii]);  /* make upper case */
                }
              }

              if (wild)
              {
                ffcmps(keyname, cardname, 1, &match, &exact);
                if (match)
                    return(*status); /* found a matching keyword */
              }
              else if (keyname[namelenminus1] == cardname[namelenminus1])
              {
                /* test the last character of the keyword name first, on */
                /* the theory that it is less likely to match then the first */
                /* character since many keywords begin with 'T', for example */

                if (FSTRNCMP(keyname, cardname, namelenminus1) == 0)
                {
                  return(*status);   /* found the matching keyword */
                }
              }
	      else if (namelen == 0 && cardlen == 0)
	      {
	         /* matched a blank keyword */
		 return(*status);
	      }
            }
          }
        }
      }

      if (wild || jj == 1)
            break;  /* stop at end of header if template contains wildcards */

      ffmaky(fptr, 1, status);  /* reset pointer to beginning of header */
      ntodo = nextkey - 1;      /* number of keyword to read */ 
    }

    return(*status = KEY_NO_EXIST);  /* couldn't find the keyword */
}
/*--------------------------------------------------------------------------*/
int ffgstr( fitsfile *fptr,     /* I - FITS file pointer        */
            const char *string, /* I - string to match  */
            char *card,         /* O - keyword card             */
            int  *status)       /* IO - error status            */
/*
  Read (get) the next keyword record that contains the input character string,
  returning the entire keyword card up to 80 characters long.
  The returned card value is null terminated with any trailing blank 
  characters removed.
*/
{
    int nkeys, nextkey, ntodo, stringlen;
    int jj, kk;

    if (*status > 0)
        return(*status);

    stringlen = strlen(string);
    if (stringlen > 80) {
        return(*status = KEY_NO_EXIST);  /* matching string is too long to exist */
    }

    ffghps(fptr, &nkeys, &nextkey, status); /* get no. keywords and position */
    ntodo = nkeys - nextkey + 1;  /* first, read from next keyword to end */

    for (jj=0; jj < 2; jj++)
    {
      for (kk = 0; kk < ntodo; kk++)
      {
        ffgnky(fptr, card, status);     /* get next keyword */
        if (strstr(card, string) != 0) {
            return(*status);   /* found the matching string */
        }
      }

      ffmaky(fptr, 1, status);  /* reset pointer to beginning of header */
      ntodo = nextkey - 1;      /* number of keyword to read */ 
    }

    return(*status = KEY_NO_EXIST);  /* couldn't find the keyword */
}
/*--------------------------------------------------------------------------*/
int ffgknm( char *card,         /* I - keyword card                   */
            char *name,         /* O - name of the keyword            */
            int *length,        /* O - length of the keyword name     */
            int  *status)       /* IO - error status                  */

/*
  Return the name of the keyword, and the name length.  This supports the
  ESO HIERARCH convention where keyword names may be > 8 characters long.
*/
{
    char *ptr1, *ptr2;
    int ii, namelength;

    namelength = FLEN_KEYWORD - 1;
    *name = '\0';
    *length = 0;

    /* support for ESO HIERARCH keywords; find the '=' */
    if (FSTRNCMP(card, "HIERARCH ", 9) == 0)
    {
        ptr2 = strchr(card, '=');

        if (!ptr2)   /* no value indicator ??? */
        {
            /* this probably indicates an error, so just return FITS name */
            strcat(name, "HIERARCH");
            *length = 8;
            return(*status);
        }

        /* find the start and end of the HIERARCH name */
        ptr1 = &card[9];
        while (*ptr1 == ' ')   /* skip spaces */
            ptr1++;

        strncat(name, ptr1, ptr2 - ptr1);
        ii = ptr2 - ptr1;

        while (ii > 0 && name[ii - 1] == ' ')  /* remove trailing spaces */
            ii--;

        name[ii] = '\0';
        *length = ii;
    }
    else
    {
        for (ii = 0; ii < namelength; ii++)
        {
           /* look for string terminator, or a blank */
           if (*(card+ii) != ' ' && *(card+ii) != '=' && *(card+ii) !='\0')
           {
               *(name+ii) = *(card+ii);
           }
           else
           {
               name[ii] = '\0';
               *length = ii;
               return(*status);
           }
        }

        /* if we got here, keyword is namelength characters long */
        name[namelength] = '\0';
        *length = namelength;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgunt( fitsfile *fptr,     /* I - FITS file pointer         */
            const char *keyname,      /* I - name of keyword to read   */
            char *unit,         /* O - keyword units             */
            int  *status)       /* IO - error status             */
/*
    Read (get) the units string from the comment field of the existing
    keyword. This routine uses a local FITS convention (not defined in the
    official FITS standard) in which the units are enclosed in 
    square brackets following the '/' comment field delimiter, e.g.:

    KEYWORD =                   12 / [kpc] comment string goes here
*/
{
    char valstring[FLEN_VALUE];
    char comm[FLEN_COMMENT];
    char *loc;

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */

    if (comm[0] == '[')
    {
        loc = strchr(comm, ']');   /*  find the closing bracket */
        if (loc)
            *loc = '\0';           /*  terminate the string */

        strcpy(unit, &comm[1]);    /*  copy the string */
     }
     else
        unit[0] = '\0';
 
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkys( fitsfile *fptr,     /* I - FITS file pointer         */
            const char *keyname,      /* I - name of keyword to read   */
            char *value,        /* O - keyword value             */
            char *comm,         /* O - keyword comment           */
            int  *status)       /* IO - error status             */
/*
  Get KeYword with a String value:
  Read (get) a simple string valued keyword.  The returned value may be up to 
  68 chars long ( + 1 null terminator char).  The routine does not support the
  HEASARC convention for continuing long string values over multiple keywords.
  The ffgkls routine may be used to read long continued strings. The returned
  comment string may be up to 69 characters long (including null terminator).
*/
{
    char valstring[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */
    value[0] = '\0';
    ffc2s(valstring, value, status);   /* remove quotes from string */
 
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgksl( fitsfile *fptr,     /* I - FITS file pointer             */
           const char *keyname, /* I - name of keyword to read       */
           int *length,         /* O - length of the string value    */
           int  *status)        /* IO - error status                 */
/*
  Get the length of the keyword value string.
  This routine explicitly supports the CONTINUE convention for long string values.
*/
{
    char valstring[FLEN_VALUE], value[FLEN_VALUE];
    int position, contin, len;
    
    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, NULL, status);  /* read the keyword */

    if (*status > 0)
        return(*status);

    ffghps(fptr, NULL,  &position, status); /* save the current header position */
    
    if (!valstring[0])  { /* null value string? */
        *length = 0;
    } else {
      ffc2s(valstring, value, status);  /* in case string contains "/" char  */
      *length = strlen(value);

      /* If last character is a & then value may be continued on next keyword */
      contin = 1;
      while (contin)  
      {
        len = strlen(value);

        if (len && *(value+len-1) == '&')  /*  is last char an anpersand?  */
        {
            ffgcnt(fptr, value, NULL, status);
            if (*value)    /* a null valstring indicates no continuation */
            {
               *length += strlen(value) - 1;
            }
            else
	    {
                contin = 0;
            }
        }
        else
	{
            contin = 0;
	}
      }
    }

    ffmaky(fptr, position - 1, status); /* reset header pointer to the keyword */
                                        /* since in many cases the program will read */
					/* the string value after getting the length */
    
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkls( fitsfile *fptr,     /* I - FITS file pointer             */
           const char *keyname, /* I - name of keyword to read       */
           char **value,        /* O - pointer to keyword value      */
           char *comm,          /* O - keyword comment (may be NULL) */
           int  *status)        /* IO - error status                 */
/*
  This is the original routine for reading long string keywords that use
  the CONTINUE keyword convention.  In 2016 a new routine called
  ffgsky / fits_read_string_key was added, which may provide a more 
  convenient user interface  for most applications.

  Get Keyword with possible Long String value:
  Read (get) the named keyword, returning the value and comment.
  The returned value string may be arbitrarily long (by using the HEASARC
  convention for continuing long string values over multiple keywords) so
  this routine allocates the required memory for the returned string value.
  It is up to the calling routine to free the memory once it is finished
  with the value string.  The returned comment string may be up to 69
  characters long.
*/
{
    char valstring[FLEN_VALUE], nextcomm[FLEN_COMMENT];
    int contin, commspace = 0;
    size_t len;

    if (*status > 0)
        return(*status);

    *value = NULL;  /* initialize a null pointer in case of error */

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */

    if (*status > 0)
        return(*status);

    if (comm)
    {
        /* remaining space in comment string */
        commspace = FLEN_COMMENT - strlen(comm) - 2;
    }
    
    if (!valstring[0])   /* null value string? */
    {
      *value = (char *) malloc(1);  /* allocate and return a null string */
      **value = '\0';
    }
    else
    {
      /* allocate space,  plus 1 for null */
      *value = (char *) malloc(strlen(valstring) + 1);

      ffc2s(valstring, *value, status);   /* convert string to value */
      len = strlen(*value);

      /* If last character is a & then value may be continued on next keyword */
      contin = 1;
      while (contin)  
      {
        if (len && *(*value+len-1) == '&')  /*  is last char an anpersand?  */
        {
            ffgcnt(fptr, valstring, nextcomm, status);
            if (*valstring)    /* a null valstring indicates no continuation */
            {
               *(*value+len-1) = '\0';         /* erase the trailing & char */
               len += strlen(valstring) - 1;
               *value = (char *) realloc(*value, len + 1); /* increase size */
               strcat(*value, valstring);     /* append the continued chars */
            }
            else
	    {
                contin = 0;
            }

            /* concantenate comment strings (if any) */
	    if ((commspace > 0) && (*nextcomm != 0)) 
	    {
                strncat(comm, " ", 1);
		strncat(comm, nextcomm, commspace);
                commspace = FLEN_COMMENT - strlen(comm) - 2;
            }
        }
        else
	{
            contin = 0;
	}
      }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgsky( fitsfile *fptr,     /* I - FITS file pointer             */
           const char *keyname, /* I - name of keyword to read       */
           int firstchar,       /* I - first character of string to return */
           int maxchar,         /* I - maximum length of string to return */
	                        /*    (string will be null terminated)  */      
           char *value,         /* O - pointer to keyword value      */
           int *valuelen,       /* O - total length of the keyword value string */
                                /*     The returned 'value' string may only */
				/*     contain a piece of the total string, depending */
				/*     on the value of firstchar and maxchar */
           char *comm,          /* O - keyword comment (may be NULL) */
           int  *status)        /* IO - error status                 */
/*
  Read and return the value of the specified string-valued keyword.
  
  This new routine was added in 2016 to provide a more convenient user
  interface than the older ffgkls routine.

  Read a string keyword, returning up to 'naxchars' characters of the value
  starting with the 'firstchar' character.
  The input 'value' string must be allocated at least 1 char bigger to
  allow for the terminating null character.
  
  This routine may be used to read continued string keywords that use 
  the CONTINUE keyword convention, as well as normal string keywords
  that are contained within a single header record.
  
  This routine differs from the ffkls routine in that it does not
  internally allocate memory for the returned value string, and consequently
  the calling routine does not need to call fffree to free the memory.
*/
{
    char valstring[FLEN_VALUE], nextcomm[FLEN_COMMENT];
    char *tempstring;
    int contin, commspace = 0;
    size_t len;

    if (*status > 0)
        return(*status);

    tempstring = NULL;  /* initialize in case of error */
    *value = '\0';
    if (valuelen) *valuelen = 0;
    
    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */

    if (*status > 0)
        return(*status);

    if (comm)
    {
        /* remaining space in comment string */
        commspace = FLEN_COMMENT - strlen(comm) - 2;
    }
    
    if (!valstring[0])   /* null value string? */
    {
      tempstring = (char *) malloc(1);  /* allocate and return a null string */
      *tempstring = '\0';
    }
    else
    {
      /* allocate space,  plus 1 for null */
      tempstring = (char *) malloc(strlen(valstring) + 1);

      ffc2s(valstring, tempstring, status);   /* convert string to value */
      len = strlen(tempstring);

      /* If last character is a & then value may be continued on next keyword */
      contin = 1;
      while (contin && *status <= 0)  
      {
        if (len && *(tempstring+len-1) == '&')  /*  is last char an anpersand?  */
        {
            ffgcnt(fptr, valstring, nextcomm, status);
            if (*valstring)    /* a null valstring indicates no continuation */
            {
               *(tempstring+len-1) = '\0';         /* erase the trailing & char */
               len += strlen(valstring) - 1;
               tempstring = (char *) realloc(tempstring, len + 1); /* increase size */
               strcat(tempstring, valstring);     /* append the continued chars */
            }
            else
	    {
                contin = 0;
            }

            /* concantenate comment strings (if any) */
	    if ((commspace > 0) && (*nextcomm != 0)) 
	    {
                strncat(comm, " ", 1);
		strncat(comm, nextcomm, commspace);
                commspace = FLEN_COMMENT - strlen(comm) - 2;
            }
        }
        else
	{
            contin = 0;
	}
      }
    }
    
    if (tempstring) 
    {
        len = strlen(tempstring);
	if (firstchar <= len)
            strncat(value, tempstring + (firstchar - 1), maxchar);
        free(tempstring);
	if (valuelen) *valuelen = len;  /* total length of the keyword value */
    }
    
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fffree( void *value,       /* I - pointer to keyword value  */
            int  *status)      /* IO - error status             */
/*
  Free the memory that was previously allocated by CFITSIO, 
  such as by ffgkls or fits_hdr2str
*/
{
    if (*status > 0)
        return(*status);

    if (value)
        free(value);

    return(*status);
}
 /*--------------------------------------------------------------------------*/
int ffgcnt( fitsfile *fptr,     /* I - FITS file pointer         */
            char *value,        /* O - continued string value    */
            char *comm,         /* O - continued comment string  */
            int  *status)       /* IO - error status             */
/*
  Attempt to read the next keyword, returning the string value
  if it is a continuation of the previous string keyword value.
  This uses the HEASARC convention for continuing long string values
  over multiple keywords.  Each continued string is terminated with a
  backslash character, and the continuation follows on the next keyword
  which must have the name CONTINUE without an equal sign in column 9
  of the card.  If the next card is not a continuation, then the returned
  value string will be null.
*/
{
    int tstatus;
    char card[FLEN_CARD], strval[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    tstatus = 0;
    value[0] = '\0';

    if (ffgnky(fptr, card, &tstatus) > 0)  /*  read next keyword  */
        return(*status);                   /*  hit end of header  */

    if (strncmp(card, "CONTINUE  ", 10) == 0)  /* a continuation card? */
    {
        strncpy(card, "D2345678=  ", 10); /* overwrite a dummy keyword name */
        ffpsvc(card, strval, comm, &tstatus);  /*  get the string value & comment */
        ffc2s(strval, value, &tstatus);    /* remove the surrounding quotes */

        if (tstatus)       /*  return null if error status was returned  */
           value[0] = '\0';
    }
    else
        ffmrky(fptr, -1, status);  /* reset the keyword pointer */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkyl( fitsfile *fptr,     /* I - FITS file pointer         */
            const char *keyname,      /* I - name of keyword to read   */
            int  *value,        /* O - keyword value             */
            char *comm,         /* O - keyword comment           */
            int  *status)       /* IO - error status             */
/*
  Read (get) the named keyword, returning the value and comment.
  The returned value = 1 if the keyword is true, else = 0 if false.
  The comment may be up to 69 characters long.
*/
{
    char valstring[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */
    ffc2l(valstring, value, status);   /* convert string to value */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkyj( fitsfile *fptr,     /* I - FITS file pointer         */
            const char *keyname,      /* I - name of keyword to read   */
            long *value,        /* O - keyword value             */
            char *comm,         /* O - keyword comment           */
            int  *status)       /* IO - error status             */
/*
  Read (get) the named keyword, returning the value and comment.
  The value will be implicitly converted to a (long) integer if it not
  already of this datatype.  The comment may be up to 69 characters long.
*/
{
    char valstring[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */
    ffc2i(valstring, value, status);   /* convert string to value */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkyjj( fitsfile *fptr,     /* I - FITS file pointer         */
            const char *keyname,      /* I - name of keyword to read   */
            LONGLONG *value,    /* O - keyword value             */
            char *comm,         /* O - keyword comment           */
            int  *status)       /* IO - error status             */
/*
  Read (get) the named keyword, returning the value and comment.
  The value will be implicitly converted to a (long) integer if it not
  already of this datatype.  The comment may be up to 69 characters long.
*/
{
    char valstring[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */
    ffc2j(valstring, value, status);   /* convert string to value */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkye( fitsfile *fptr,     /* I - FITS file pointer         */
            const char  *keyname,     /* I - name of keyword to read   */
            float *value,       /* O - keyword value             */
            char  *comm,        /* O - keyword comment           */
            int   *status)      /* IO - error status             */
/*
  Read (get) the named keyword, returning the value and comment.
  The value will be implicitly converted to a float if it not
  already of this datatype.  The comment may be up to 69 characters long.
*/
{
    char valstring[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */
    ffc2r(valstring, value, status);   /* convert string to value */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkyd( fitsfile *fptr,      /* I - FITS file pointer         */
            const char   *keyname,     /* I - name of keyword to read   */
            double *value,       /* O - keyword value             */
            char   *comm,        /* O - keyword comment           */
            int    *status)      /* IO - error status             */
/*
  Read (get) the named keyword, returning the value and comment.
  The value will be implicitly converted to a double if it not
  already of this datatype.  The comment may be up to 69 characters long.
*/
{
    char valstring[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */
    ffc2d(valstring, value, status);   /* convert string to value */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkyc( fitsfile *fptr,     /* I - FITS file pointer         */
            const char  *keyname,     /* I - name of keyword to read   */
            float *value,       /* O - keyword value (real,imag) */
            char  *comm,        /* O - keyword comment           */
            int   *status)      /* IO - error status             */
/*
  Read (get) the named keyword, returning the value and comment.
  The keyword must have a complex value. No implicit data conversion
  will be performed.
*/
{
    char valstring[FLEN_VALUE], message[81];
    int len;

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */

    if (valstring[0] != '(' )   /* test that this is a complex keyword */
    {
      sprintf(message, "keyword %s does not have a complex value (ffgkyc):",
              keyname);
      ffpmsg(message);
      ffpmsg(valstring);
      return(*status = BAD_C2F);
    }

    valstring[0] = ' ';            /* delete the opening parenthesis */
    len = strcspn(valstring, ")" );  
    valstring[len] = '\0';         /* delete the closing parenthesis */

    len = strcspn(valstring, ",");
    valstring[len] = '\0';

    ffc2r(valstring, &value[0], status);       /* convert the real part */
    ffc2r(&valstring[len + 1], &value[1], status); /* convert imag. part */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkym( fitsfile *fptr,     /* I - FITS file pointer         */
            const char  *keyname,     /* I - name of keyword to read   */
            double *value,      /* O - keyword value (real,imag) */
            char  *comm,        /* O - keyword comment           */
            int   *status)      /* IO - error status             */
/*
  Read (get) the named keyword, returning the value and comment.
  The keyword must have a complex value. No implicit data conversion
  will be performed.
*/
{
    char valstring[FLEN_VALUE], message[81];
    int len;

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */

    if (valstring[0] != '(' )   /* test that this is a complex keyword */
    {
      sprintf(message, "keyword %s does not have a complex value (ffgkym):",
              keyname);
      ffpmsg(message);
      ffpmsg(valstring);
      return(*status = BAD_C2D);
    }

    valstring[0] = ' ';            /* delete the opening parenthesis */
    len = strcspn(valstring, ")" );  
    valstring[len] = '\0';         /* delete the closing parenthesis */

    len = strcspn(valstring, ",");
    valstring[len] = '\0';

    ffc2d(valstring, &value[0], status);        /* convert the real part */
    ffc2d(&valstring[len + 1], &value[1], status);  /* convert the imag. part */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkyt( fitsfile *fptr,      /* I - FITS file pointer                 */
            const char   *keyname,     /* I - name of keyword to read           */
            long   *ivalue,      /* O - integer part of keyword value     */
            double *fraction,    /* O - fractional part of keyword value  */
            char   *comm,        /* O - keyword comment                   */
            int    *status)      /* IO - error status                     */
/*
  Read (get) the named keyword, returning the value and comment.
  The integer and fractional parts of the value are returned in separate
  variables, to allow more numerical precision to be passed.  This
  effectively passes a 'triple' precision value, with a 4-byte integer
  and an 8-byte fraction.  The comment may be up to 69 characters long.
*/
{
    char valstring[FLEN_VALUE];
    char *loc;

    if (*status > 0)
        return(*status);

    ffgkey(fptr, keyname, valstring, comm, status);  /* read the keyword */

    /*  read the entire value string as a double, to get the integer part */
    ffc2d(valstring, fraction, status);

    *ivalue = (long) *fraction;

    *fraction = *fraction - *ivalue;

    /* see if we need to read the fractional part again with more precision */
    /* look for decimal point, without an exponential E or D character */

    loc = strchr(valstring, '.');
    if (loc)
    {
        if (!strchr(valstring, 'E') && !strchr(valstring, 'D'))
            ffc2d(loc, fraction, status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkyn( fitsfile *fptr,      /* I - FITS file pointer             */
            int    nkey,         /* I - number of the keyword to read */
            char   *keyname,     /* O - name of the keyword           */
            char   *value,       /* O - keyword value                 */
            char   *comm,        /* O - keyword comment               */
            int    *status)      /* IO - error status                 */
/*
  Read (get) the nkey-th keyword returning the keyword name, value and comment.
  The value is just the literal string of characters in the value field
  of the keyword.  In the case of a string valued keyword, the returned
  value includes the leading and closing quote characters.  The value may be
  up to 70 characters long, and the comment may be up to 72 characters long.
  If the keyword has no value (no equal sign in column 9) then a null value
  is returned.  If comm = NULL, then do not return the comment string.
*/
{
    char card[FLEN_CARD], sbuff[FLEN_CARD];
    int namelen;

    keyname[0] = '\0';
    value[0] = '\0';
    if (comm)
        comm[0] = '\0';

    if (*status > 0)
        return(*status);

    if (ffgrec(fptr, nkey, card, status) > 0 )  /* get the 80-byte card */
        return(*status);

    ffgknm(card, keyname, &namelen, status); /* get the keyword name */

    if (ffpsvc(card, value, comm, status) > 0)   /* parse value and comment */
        return(*status);

    if (fftrec(keyname, status) > 0)  /* test keyword name; catches no END */
    {
     sprintf(sbuff,"Name of keyword no. %d contains illegal character(s): %s",
              nkey, keyname);
     ffpmsg(sbuff);

     if (nkey % 36 == 0)  /* test if at beginning of 36-card FITS record */
            ffpmsg("  (This may indicate a missing END keyword).");
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkns( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyname,      /* I - root name of keywords to read        */
            int  nstart,        /* I - starting index number                */
            int  nmax,          /* I - maximum number of keywords to return */
            char *value[],      /* O - array of pointers to keyword values  */
            int  *nfound,       /* O - number of values that were returned  */
            int  *status)       /* IO - error status                        */
/*
  Read (get) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NMAX -1) inclusive.  
  This routine does NOT support the HEASARC long string convention.
*/
{
    int nend, lenroot, ii, nkeys, mkeys, tstatus, undefinedval;
    long ival;
    char keyroot[FLEN_KEYWORD], keyindex[8], card[FLEN_CARD];
    char svalue[FLEN_VALUE], comm[FLEN_COMMENT], *equalssign;

    if (*status > 0)
        return(*status);

    *nfound = 0;
    nend = nstart + nmax - 1;

    keyroot[0] = '\0';
    strncat(keyroot, keyname, FLEN_KEYWORD - 1);
     
    lenroot = strlen(keyroot);
    
    if (lenroot == 0)     /*  root must be at least 1 char long  */
        return(*status);

    for (ii=0; ii < lenroot; ii++)           /*  make sure upper case  */
        keyroot[ii] = toupper(keyroot[ii]);

    ffghps(fptr, &nkeys, &mkeys, status);  /*  get the number of keywords  */

    undefinedval = FALSE;
    for (ii=3; ii <= nkeys; ii++)  
    {
       if (ffgrec(fptr, ii, card, status) > 0)     /*  get next keyword  */
           return(*status);

       if (strncmp(keyroot, card, lenroot) == 0)  /* see if keyword matches */
       {
          keyindex[0] = '\0';
          equalssign = strchr(card, '=');
	  if (equalssign == 0) continue;  /* keyword has no value */

          strncat(keyindex, &card[lenroot], equalssign - card  - lenroot);  /*  copy suffix  */
          tstatus = 0;
          if (ffc2ii(keyindex, &ival, &tstatus) <= 0)     /*  test suffix  */
          {
             if (ival <= nend && ival >= nstart)
             {
                ffpsvc(card, svalue, comm, status);  /*  parse the value */
                ffc2s(svalue, value[ival-nstart], status); /* convert */
                if (ival - nstart + 1 > *nfound)
                      *nfound = ival - nstart + 1;  /*  max found */ 

                if (*status == VALUE_UNDEFINED)
                {
                   undefinedval = TRUE;
                   *status = 0;  /* reset status to read remaining values */
                }
             }
          }
       }
    }
    if (undefinedval && (*status <= 0) )
        *status = VALUE_UNDEFINED;  /* report at least 1 value undefined */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgknl( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyname,      /* I - root name of keywords to read        */
            int  nstart,        /* I - starting index number                */
            int  nmax,          /* I - maximum number of keywords to return */
            int  *value,        /* O - array of keyword values              */
            int  *nfound,       /* O - number of values that were returned  */
            int  *status)       /* IO - error status                        */
/*
  Read (get) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NMAX -1) inclusive.  
  The returned value = 1 if the keyword is true, else = 0 if false.
*/
{
    int nend, lenroot, ii, nkeys, mkeys, tstatus, undefinedval;
    long ival;
    char keyroot[FLEN_KEYWORD], keyindex[8], card[FLEN_CARD];
    char svalue[FLEN_VALUE], comm[FLEN_COMMENT], *equalssign;

    if (*status > 0)
        return(*status);

    *nfound = 0;
    nend = nstart + nmax - 1;

    keyroot[0] = '\0';
    strncat(keyroot, keyname, FLEN_KEYWORD - 1);

    lenroot = strlen(keyroot);
    
    if (lenroot == 0)     /*  root must be at least 1 char long  */
        return(*status);
 
    for (ii=0; ii < lenroot; ii++)           /*  make sure upper case  */
        keyroot[ii] = toupper(keyroot[ii]);

    ffghps(fptr, &nkeys, &mkeys, status);  /*  get the number of keywords  */

    ffmaky(fptr, 3, status);  /* move to 3rd keyword (skip 1st 2 keywords) */

    undefinedval = FALSE;
    for (ii=3; ii <= nkeys; ii++)  
    {
       if (ffgnky(fptr, card, status) > 0)     /*  get next keyword  */
           return(*status);

       if (strncmp(keyroot, card, lenroot) == 0)  /* see if keyword matches */
       {
          keyindex[0] = '\0';
          equalssign = strchr(card, '=');
	  if (equalssign == 0) continue;  /* keyword has no value */

          strncat(keyindex, &card[lenroot], equalssign - card  - lenroot);  /*  copy suffix  */

          tstatus = 0;
          if (ffc2ii(keyindex, &ival, &tstatus) <= 0)    /*  test suffix  */
          {
             if (ival <= nend && ival >= nstart)
             {
                ffpsvc(card, svalue, comm, status);   /*  parse the value */
                ffc2l(svalue, &value[ival-nstart], status); /* convert*/
                if (ival - nstart + 1 > *nfound)
                      *nfound = ival - nstart + 1;  /*  max found */ 

                if (*status == VALUE_UNDEFINED)
                {
                    undefinedval = TRUE;
                   *status = 0;  /* reset status to read remaining values */
                }
             }
          }
       }
    }
    if (undefinedval && (*status <= 0) )
        *status = VALUE_UNDEFINED;  /* report at least 1 value undefined */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgknj( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyname,      /* I - root name of keywords to read        */
            int  nstart,        /* I - starting index number                */
            int  nmax,          /* I - maximum number of keywords to return */
            long *value,        /* O - array of keyword values              */
            int  *nfound,       /* O - number of values that were returned  */
            int  *status)       /* IO - error status                        */
/*
  Read (get) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NMAX -1) inclusive.  
*/
{
    int nend, lenroot, ii, nkeys, mkeys, tstatus, undefinedval;
    long ival;
    char keyroot[FLEN_KEYWORD], keyindex[8], card[FLEN_CARD];
    char svalue[FLEN_VALUE], comm[FLEN_COMMENT], *equalssign;

    if (*status > 0)
        return(*status);

    *nfound = 0;
    nend = nstart + nmax - 1;

    keyroot[0] = '\0';
    strncat(keyroot, keyname, FLEN_KEYWORD - 1);

    lenroot = strlen(keyroot);
    
    if (lenroot == 0)     /*  root must be at least 1 char long  */
        return(*status);
 
    for (ii=0; ii < lenroot; ii++)           /*  make sure upper case  */
        keyroot[ii] = toupper(keyroot[ii]);

    ffghps(fptr, &nkeys, &mkeys, status);  /*  get the number of keywords  */

    ffmaky(fptr, 3, status);  /* move to 3rd keyword (skip 1st 2 keywords) */

    undefinedval = FALSE;
    for (ii=3; ii <= nkeys; ii++)  
    {
       if (ffgnky(fptr, card, status) > 0)     /*  get next keyword  */
           return(*status);

       if (strncmp(keyroot, card, lenroot) == 0)  /* see if keyword matches */
       {
          keyindex[0] = '\0';
          equalssign = strchr(card, '=');
	  if (equalssign == 0) continue;  /* keyword has no value */

          strncat(keyindex, &card[lenroot], equalssign - card  - lenroot);  /*  copy suffix  */

          tstatus = 0;
          if (ffc2ii(keyindex, &ival, &tstatus) <= 0)     /*  test suffix  */
          {
             if (ival <= nend && ival >= nstart)
             {
                ffpsvc(card, svalue, comm, status);   /*  parse the value */
                ffc2i(svalue, &value[ival-nstart], status);  /* convert */
                if (ival - nstart + 1 > *nfound)
                      *nfound = ival - nstart + 1;  /*  max found */ 

                if (*status == VALUE_UNDEFINED)
                {
                    undefinedval = TRUE;
                   *status = 0;  /* reset status to read remaining values */
                }
             }
          }
       }
    }
    if (undefinedval && (*status <= 0) )
        *status = VALUE_UNDEFINED;  /* report at least 1 value undefined */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgknjj( fitsfile *fptr,    /* I - FITS file pointer                    */
            const char *keyname,      /* I - root name of keywords to read        */
            int  nstart,        /* I - starting index number                */
            int  nmax,          /* I - maximum number of keywords to return */
            LONGLONG *value,    /* O - array of keyword values              */
            int  *nfound,       /* O - number of values that were returned  */
            int  *status)       /* IO - error status                        */
/*
  Read (get) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NMAX -1) inclusive.  
*/
{
    int nend, lenroot, ii, nkeys, mkeys, tstatus, undefinedval;
    long ival;
    char keyroot[FLEN_KEYWORD], keyindex[8], card[FLEN_CARD];
    char svalue[FLEN_VALUE], comm[FLEN_COMMENT], *equalssign;

    if (*status > 0)
        return(*status);

    *nfound = 0;
    nend = nstart + nmax - 1;

    keyroot[0] = '\0';
    strncat(keyroot, keyname, FLEN_KEYWORD - 1);

    lenroot = strlen(keyroot);
    
    if (lenroot == 0)     /*  root must be at least 1 char long  */
        return(*status);

    for (ii=0; ii < lenroot; ii++)           /*  make sure upper case  */
        keyroot[ii] = toupper(keyroot[ii]);

    ffghps(fptr, &nkeys, &mkeys, status);  /*  get the number of keywords  */

    ffmaky(fptr, 3, status);  /* move to 3rd keyword (skip 1st 2 keywords) */

    undefinedval = FALSE;
    for (ii=3; ii <= nkeys; ii++)  
    {
       if (ffgnky(fptr, card, status) > 0)     /*  get next keyword  */
           return(*status);

       if (strncmp(keyroot, card, lenroot) == 0)  /* see if keyword matches */
       {
          keyindex[0] = '\0';
          equalssign = strchr(card, '=');
	  if (equalssign == 0) continue;  /* keyword has no value */

          strncat(keyindex, &card[lenroot], equalssign - card  - lenroot);  /*  copy suffix  */

          tstatus = 0;
          if (ffc2ii(keyindex, &ival, &tstatus) <= 0)     /*  test suffix  */
          {
             if (ival <= nend && ival >= nstart)
             {
                ffpsvc(card, svalue, comm, status);   /*  parse the value */
                ffc2j(svalue, &value[ival-nstart], status);  /* convert */
                if (ival - nstart + 1 > *nfound)
                      *nfound = ival - nstart + 1;  /*  max found */ 

                if (*status == VALUE_UNDEFINED)
                {
                    undefinedval = TRUE;
                   *status = 0;  /* reset status to read remaining values */
                }
             }
          }
       }
    }
    if (undefinedval && (*status <= 0) )
        *status = VALUE_UNDEFINED;  /* report at least 1 value undefined */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkne( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyname,      /* I - root name of keywords to read        */
            int  nstart,        /* I - starting index number                */
            int  nmax,          /* I - maximum number of keywords to return */
            float *value,       /* O - array of keyword values              */
            int  *nfound,       /* O - number of values that were returned  */
            int  *status)       /* IO - error status                        */
/*
  Read (get) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NMAX -1) inclusive.  
*/
{
    int nend, lenroot, ii, nkeys, mkeys, tstatus, undefinedval;
    long ival;
    char keyroot[FLEN_KEYWORD], keyindex[8], card[FLEN_CARD];
    char svalue[FLEN_VALUE], comm[FLEN_COMMENT], *equalssign;

    if (*status > 0)
        return(*status);

    *nfound = 0;
    nend = nstart + nmax - 1;

    keyroot[0] = '\0';
    strncat(keyroot, keyname, FLEN_KEYWORD - 1);

    lenroot = strlen(keyroot);
    
    if (lenroot == 0)     /*  root must be at least 1 char long  */
        return(*status);

    for (ii=0; ii < lenroot; ii++)           /*  make sure upper case  */
        keyroot[ii] = toupper(keyroot[ii]);

    ffghps(fptr, &nkeys, &mkeys, status);  /*  get the number of keywords  */

    ffmaky(fptr, 3, status);  /* move to 3rd keyword (skip 1st 2 keywords) */

    undefinedval = FALSE;
    for (ii=3; ii <= nkeys; ii++)  
    {
       if (ffgnky(fptr, card, status) > 0)     /*  get next keyword  */
           return(*status);

       if (strncmp(keyroot, card, lenroot) == 0)  /* see if keyword matches */
       {
          keyindex[0] = '\0';
          equalssign = strchr(card, '=');
	  if (equalssign == 0) continue;  /* keyword has no value */

          strncat(keyindex, &card[lenroot], equalssign - card  - lenroot);  /*  copy suffix  */

          tstatus = 0;
          if (ffc2ii(keyindex, &ival, &tstatus) <= 0)     /*  test suffix  */
          {
             if (ival <= nend && ival >= nstart)
             {
                ffpsvc(card, svalue, comm, status);   /*  parse the value */
                ffc2r(svalue, &value[ival-nstart], status); /* convert */
                if (ival - nstart + 1 > *nfound)
                      *nfound = ival - nstart + 1;  /*  max found */ 

                if (*status == VALUE_UNDEFINED)
                {
                    undefinedval = TRUE;
                   *status = 0;  /* reset status to read remaining values */
                }
             }
          }
       }
    }
    if (undefinedval && (*status <= 0) )
        *status = VALUE_UNDEFINED;  /* report at least 1 value undefined */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgknd( fitsfile *fptr,     /* I - FITS file pointer                    */
            const char *keyname,      /* I - root name of keywords to read        */
            int  nstart,        /* I - starting index number                */
            int  nmax,          /* I - maximum number of keywords to return */
            double *value,      /* O - array of keyword values              */
            int  *nfound,       /* O - number of values that were returned  */
            int  *status)       /* IO - error status                        */
/*
  Read (get) an indexed array of keywords with index numbers between
  NSTART and (NSTART + NMAX -1) inclusive.  
*/
{
    int nend, lenroot, ii, nkeys, mkeys, tstatus, undefinedval;
    long ival;
    char keyroot[FLEN_KEYWORD], keyindex[8], card[FLEN_CARD];
    char svalue[FLEN_VALUE], comm[FLEN_COMMENT], *equalssign;

    if (*status > 0)
        return(*status);

    *nfound = 0;
    nend = nstart + nmax - 1;

    keyroot[0] = '\0';
    strncat(keyroot, keyname, FLEN_KEYWORD - 1);

    lenroot = strlen(keyroot);

    if (lenroot == 0)     /*  root must be at least 1 char long  */
        return(*status);

    for (ii=0; ii < lenroot; ii++)           /*  make sure upper case  */
        keyroot[ii] = toupper(keyroot[ii]);

    ffghps(fptr, &nkeys, &mkeys, status);  /*  get the number of keywords  */

    ffmaky(fptr, 3, status);  /* move to 3rd keyword (skip 1st 2 keywords) */

    undefinedval = FALSE;
    for (ii=3; ii <= nkeys; ii++)  
    {
       if (ffgnky(fptr, card, status) > 0)     /*  get next keyword  */
           return(*status);
       if (strncmp(keyroot, card, lenroot) == 0)   /* see if keyword matches */
       {
          keyindex[0] = '\0';
          equalssign = strchr(card, '=');
	  if (equalssign == 0) continue;  /* keyword has no value */

          strncat(keyindex, &card[lenroot], equalssign - card  - lenroot);  /*  copy suffix  */
          tstatus = 0;
          if (ffc2ii(keyindex, &ival, &tstatus) <= 0)      /*  test suffix */
          {
             if (ival <= nend && ival >= nstart) /* is index within range? */
             {
                ffpsvc(card, svalue, comm, status);   /*  parse the value */
                ffc2d(svalue, &value[ival-nstart], status); /* convert */
                if (ival - nstart + 1 > *nfound)
                      *nfound = ival - nstart + 1;  /*  max found */ 

                if (*status == VALUE_UNDEFINED)
                {
                    undefinedval = TRUE;
                   *status = 0;  /* reset status to read remaining values */
                }
             }
          }
       }
    }
    if (undefinedval && (*status <= 0) )
        *status = VALUE_UNDEFINED;  /* report at least 1 value undefined */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtdm(fitsfile *fptr,  /* I - FITS file pointer                        */
           int colnum,      /* I - number of the column to read             */
           int maxdim,      /* I - maximum no. of dimensions to read;       */
           int *naxis,      /* O - number of axes in the data array         */
           long naxes[],    /* O - length of each data axis                 */
           int *status)     /* IO - error status                            */
/*
  read and parse the TDIMnnn keyword to get the dimensionality of a column
*/
{
    int tstatus = 0;
    char keyname[FLEN_KEYWORD], tdimstr[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    ffkeyn("TDIM", colnum, keyname, status);      /* construct keyword name */

    ffgkys(fptr, keyname, tdimstr, NULL, &tstatus); /* try reading keyword */

    ffdtdm(fptr, tdimstr, colnum, maxdim,naxis, naxes, status); /* decode it */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtdmll(fitsfile *fptr,  /* I - FITS file pointer                      */
           int colnum,      /* I - number of the column to read             */
           int maxdim,      /* I - maximum no. of dimensions to read;       */
           int *naxis,      /* O - number of axes in the data array         */
           LONGLONG naxes[], /* O - length of each data axis                 */
           int *status)     /* IO - error status                            */
/*
  read and parse the TDIMnnn keyword to get the dimensionality of a column
*/
{
    int tstatus = 0;
    char keyname[FLEN_KEYWORD], tdimstr[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    ffkeyn("TDIM", colnum, keyname, status);      /* construct keyword name */

    ffgkys(fptr, keyname, tdimstr, NULL, &tstatus); /* try reading keyword */

    ffdtdmll(fptr, tdimstr, colnum, maxdim,naxis, naxes, status); /* decode it */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdtdm(fitsfile *fptr,  /* I - FITS file pointer                        */
           char *tdimstr,   /* I - TDIMn keyword value string. e.g. (10,10) */
           int colnum,      /* I - number of the column             */
           int maxdim,      /* I - maximum no. of dimensions to read;       */
           int *naxis,      /* O - number of axes in the data array         */
           long naxes[],    /* O - length of each data axis                 */
           int *status)     /* IO - error status                            */
/*
  decode the TDIMnnn keyword to get the dimensionality of a column.
  Check that the value is legal and consistent with the TFORM value.
  If colnum = 0, then the validity checking is disabled.
*/
{
    long dimsize, totalpix = 1;
    char *loc, *lastloc, message[81];
    tcolumn *colptr = 0;

    if (*status > 0)
        return(*status);

    if (colnum != 0) {
        if (fptr->HDUposition != (fptr->Fptr)->curhdu)
            ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

        if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
            return(*status = BAD_COL_NUM);

        colptr = (fptr->Fptr)->tableptr;   /* set pointer to the first column */
        colptr += (colnum - 1);    /* increment to the correct column */

        if (!tdimstr[0])   /* TDIMn keyword doesn't exist? */
        {
            *naxis = 1;                   /* default = 1 dimensional */
            if (maxdim > 0)
                naxes[0] = (long) colptr->trepeat; /* default length = repeat */

            return(*status);
        }
    }

    *naxis = 0;

    loc = strchr(tdimstr, '(' );  /* find the opening quote */
    if (!loc)
    {
            sprintf(message, "Illegal dimensions format: %s", tdimstr);
            return(*status = BAD_TDIM);
    }

    while (loc)
    {
            loc++;
            dimsize = strtol(loc, &loc, 10);  /* read size of next dimension */
            if (*naxis < maxdim)
                naxes[*naxis] = dimsize;

            if (dimsize < 0)
            {
                ffpmsg("one or more dimension are less than 0 (ffdtdm)");
                ffpmsg(tdimstr);
                return(*status = BAD_TDIM);
            }

            totalpix *= dimsize;
            (*naxis)++;
            lastloc = loc;
            loc = strchr(loc, ',');  /* look for comma before next dimension */
    }

    loc = strchr(lastloc, ')' );  /* check for the closing quote */
    if (!loc)
    {
            sprintf(message, "Illegal dimensions format: %s", tdimstr);
            return(*status = BAD_TDIM);
    }

    if (colnum != 0) {
        if ((colptr->tdatatype > 0) && ((long) colptr->trepeat != totalpix))
        {
          sprintf(message,
          "column vector length, %ld, does not equal TDIMn array size, %ld",
          (long) colptr->trepeat, totalpix);
          ffpmsg(message);
          ffpmsg(tdimstr);
          return(*status = BAD_TDIM);
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdtdmll(fitsfile *fptr,  /* I - FITS file pointer                        */
           char *tdimstr,   /* I - TDIMn keyword value string. e.g. (10,10) */
           int colnum,      /* I - number of the column             */
           int maxdim,      /* I - maximum no. of dimensions to read;       */
           int *naxis,      /* O - number of axes in the data array         */
           LONGLONG naxes[],    /* O - length of each data axis                 */
           int *status)     /* IO - error status                            */
/*
  decode the TDIMnnn keyword to get the dimensionality of a column.
  Check that the value is legal and consistent with the TFORM value.
*/
{
    LONGLONG dimsize;
    LONGLONG totalpix = 1;
    char *loc, *lastloc, message[81];
    tcolumn *colptr;
    double doublesize;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
        return(*status = BAD_COL_NUM);

    colptr = (fptr->Fptr)->tableptr;   /* set pointer to the first column */
    colptr += (colnum - 1);    /* increment to the correct column */

    if (!tdimstr[0])   /* TDIMn keyword doesn't exist? */
    {
        *naxis = 1;                   /* default = 1 dimensional */
        if (maxdim > 0)
            naxes[0] = colptr->trepeat; /* default length = repeat */
    }
    else
    {
        *naxis = 0;

        loc = strchr(tdimstr, '(' );  /* find the opening quote */
        if (!loc)
        {
            sprintf(message, "Illegal TDIM keyword value: %s", tdimstr);
            return(*status = BAD_TDIM);
        }

        while (loc)
        {
            loc++;

    /* Read value as a double because the string to 64-bit int function is  */
    /* platform dependent (strtoll, strtol, _atoI64).  This still gives     */
    /* about 48 bits of precision, which is plenty for this purpose.        */

            doublesize = strtod(loc, &loc);
            dimsize = (LONGLONG) (doublesize + 0.1);

            if (*naxis < maxdim)
                naxes[*naxis] = dimsize;

            if (dimsize < 0)
            {
                ffpmsg("one or more TDIM values are less than 0 (ffdtdm)");
                ffpmsg(tdimstr);
                return(*status = BAD_TDIM);
            }

            totalpix *= dimsize;
            (*naxis)++;
            lastloc = loc;
            loc = strchr(loc, ',');  /* look for comma before next dimension */
        }

        loc = strchr(lastloc, ')' );  /* check for the closing quote */
        if (!loc)
        {
            sprintf(message, "Illegal TDIM keyword value: %s", tdimstr);
            return(*status = BAD_TDIM);
        }

        if ((colptr->tdatatype > 0) && (colptr->trepeat != totalpix))
        {
          sprintf(message,
          "column vector length, %.0f, does not equal TDIMn array size, %.0f",
          (double) (colptr->trepeat), (double) totalpix);
          ffpmsg(message);
          ffpmsg(tdimstr);
          return(*status = BAD_TDIM);
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghpr(fitsfile *fptr,  /* I - FITS file pointer                        */
           int maxdim,      /* I - maximum no. of dimensions to read;       */
           int *simple,     /* O - does file conform to FITS standard? 1/0  */
           int *bitpix,     /* O - number of bits per data value pixel      */
           int *naxis,      /* O - number of axes in the data array         */
           long naxes[],    /* O - length of each data axis                 */
           long *pcount,    /* O - number of group parameters (usually 0)   */
           long *gcount,    /* O - number of random groups (usually 1 or 0) */
           int *extend,     /* O - may FITS file haave extensions?          */
           int *status)     /* IO - error status                            */
/*
  Get keywords from the Header of the PRimary array:
  Check that the keywords conform to the FITS standard and return the
  parameters which determine the size and structure of the primary array
  or IMAGE extension.
*/
{
    int idummy, ii;
    LONGLONG lldummy;
    double ddummy;
    LONGLONG tnaxes[99];

    ffgphd(fptr, maxdim, simple, bitpix, naxis, tnaxes, pcount, gcount, extend,
          &ddummy, &ddummy, &lldummy, &idummy, status);
	  
    if (naxis && naxes) {
         for (ii = 0; (ii < *naxis) && (ii < maxdim); ii++)
	     naxes[ii] = (long) tnaxes[ii];
    } else if (naxes) {
         for (ii = 0; ii < maxdim; ii++)
	     naxes[ii] = (long) tnaxes[ii];
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghprll(fitsfile *fptr,  /* I - FITS file pointer                        */
           int maxdim,      /* I - maximum no. of dimensions to read;       */
           int *simple,     /* O - does file conform to FITS standard? 1/0  */
           int *bitpix,     /* O - number of bits per data value pixel      */
           int *naxis,      /* O - number of axes in the data array         */
           LONGLONG naxes[],    /* O - length of each data axis                 */
           long *pcount,    /* O - number of group parameters (usually 0)   */
           long *gcount,    /* O - number of random groups (usually 1 or 0) */
           int *extend,     /* O - may FITS file haave extensions?          */
           int *status)     /* IO - error status                            */
/*
  Get keywords from the Header of the PRimary array:
  Check that the keywords conform to the FITS standard and return the
  parameters which determine the size and structure of the primary array
  or IMAGE extension.
*/
{
    int idummy;
    LONGLONG lldummy;
    double ddummy;

    ffgphd(fptr, maxdim, simple, bitpix, naxis, naxes, pcount, gcount, extend,
          &ddummy, &ddummy, &lldummy, &idummy, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghtb(fitsfile *fptr,  /* I - FITS file pointer                        */
           int maxfield,    /* I - maximum no. of columns to read;          */
           long *naxis1,    /* O - length of table row in bytes             */
           long *naxis2,    /* O - number of rows in the table              */
           int *tfields,    /* O - number of columns in the table           */
           char **ttype,    /* O - name of each column                      */
           long *tbcol,     /* O - byte offset in row to each column        */
           char **tform,    /* O - value of TFORMn keyword for each column  */
           char **tunit,    /* O - value of TUNITn keyword for each column  */
           char *extnm,   /* O - value of EXTNAME keyword, if any         */
           int *status)     /* IO - error status                            */
/*
  Get keywords from the Header of the ASCII TaBle:
  Check that the keywords conform to the FITS standard and return the
  parameters which describe the table.
*/
{
    int ii, maxf, nfound, tstatus;
    long fields;
    char name[FLEN_KEYWORD], value[FLEN_VALUE], comm[FLEN_COMMENT];
    char xtension[FLEN_VALUE], message[81];
    LONGLONG llnaxis1, llnaxis2, pcount;

    if (*status > 0)
        return(*status);

    /* read the first keyword of the extension */
    ffgkyn(fptr, 1, name, value, comm, status);

    if (!strcmp(name, "XTENSION"))
    {
            if (ffc2s(value, xtension, status) > 0)  /* get the value string */
            {
                ffpmsg("Bad value string for XTENSION keyword:");
                ffpmsg(value);
                return(*status);
            }

            /* allow the quoted string value to begin in any column and */
            /* allow any number of trailing blanks before the closing quote */
            if ( (value[0] != '\'')   ||  /* first char must be a quote */
                 ( strcmp(xtension, "TABLE") ) )
            {
                sprintf(message,
                "This is not a TABLE extension: %s", value);
                ffpmsg(message);
                return(*status = NOT_ATABLE);
            }
    }

    else  /* error: 1st keyword of extension != XTENSION */
    {
        sprintf(message,
        "First keyword of the extension is not XTENSION: %s", name);
        ffpmsg(message);
        return(*status = NO_XTENSION);
    }

    if (ffgttb(fptr, &llnaxis1, &llnaxis2, &pcount, &fields, status) > 0)
        return(*status);

    if (naxis1)
       *naxis1 = (long) llnaxis1;

    if (naxis2)
       *naxis2 = (long) llnaxis2;

    if (pcount != 0)
    {
       sprintf(message, "PCOUNT = %.0f is illegal in ASCII table; must = 0",
               (double) pcount);
       ffpmsg(message);
       return(*status = BAD_PCOUNT);
    }

    if (tfields)
       *tfields = fields;

    if (maxfield < 0)
        maxf = fields;
    else
        maxf = minvalue(maxfield, fields);

    if (maxf > 0)
    {
        for (ii = 0; ii < maxf; ii++)
        {   /* initialize optional keyword values */
            if (ttype)
                *ttype[ii] = '\0';   

            if (tunit)
                *tunit[ii] = '\0';
        }

   
        if (ttype)
            ffgkns(fptr, "TTYPE", 1, maxf, ttype, &nfound, status);

        if (tunit)
            ffgkns(fptr, "TUNIT", 1, maxf, tunit, &nfound, status);

        if (*status > 0)
            return(*status);

        if (tbcol)
        {
            ffgknj(fptr, "TBCOL", 1, maxf, tbcol, &nfound, status);

            if (*status > 0 || nfound != maxf)
            {
                ffpmsg(
        "Required TBCOL keyword(s) not found in ASCII table header (ffghtb).");
                return(*status = NO_TBCOL);
            }
        }

        if (tform)
        {
            ffgkns(fptr, "TFORM", 1, maxf, tform, &nfound, status);

            if (*status > 0 || nfound != maxf)
            {
                ffpmsg(
        "Required TFORM keyword(s) not found in ASCII table header (ffghtb).");
                return(*status = NO_TFORM);
            }
        }
    }

    if (extnm)
    {
        extnm[0] = '\0';

        tstatus = *status;
        ffgkys(fptr, "EXTNAME", extnm, comm, status);

        if (*status == KEY_NO_EXIST)
            *status = tstatus;  /* keyword not required, so ignore error */
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghtbll(fitsfile *fptr, /* I - FITS file pointer                        */
           int maxfield,    /* I - maximum no. of columns to read;          */
           LONGLONG *naxis1, /* O - length of table row in bytes             */
           LONGLONG *naxis2, /* O - number of rows in the table              */
           int *tfields,    /* O - number of columns in the table           */
           char **ttype,    /* O - name of each column                      */
           LONGLONG *tbcol, /* O - byte offset in row to each column        */
           char **tform,    /* O - value of TFORMn keyword for each column  */
           char **tunit,    /* O - value of TUNITn keyword for each column  */
           char *extnm,     /* O - value of EXTNAME keyword, if any         */
           int *status)     /* IO - error status                            */
/*
  Get keywords from the Header of the ASCII TaBle:
  Check that the keywords conform to the FITS standard and return the
  parameters which describe the table.
*/
{
    int ii, maxf, nfound, tstatus;
    long fields;
    char name[FLEN_KEYWORD], value[FLEN_VALUE], comm[FLEN_COMMENT];
    char xtension[FLEN_VALUE], message[81];
    LONGLONG llnaxis1, llnaxis2, pcount;

    if (*status > 0)
        return(*status);

    /* read the first keyword of the extension */
    ffgkyn(fptr, 1, name, value, comm, status);

    if (!strcmp(name, "XTENSION"))
    {
            if (ffc2s(value, xtension, status) > 0)  /* get the value string */
            {
                ffpmsg("Bad value string for XTENSION keyword:");
                ffpmsg(value);
                return(*status);
            }

            /* allow the quoted string value to begin in any column and */
            /* allow any number of trailing blanks before the closing quote */
            if ( (value[0] != '\'')   ||  /* first char must be a quote */
                 ( strcmp(xtension, "TABLE") ) )
            {
                sprintf(message,
                "This is not a TABLE extension: %s", value);
                ffpmsg(message);
                return(*status = NOT_ATABLE);
            }
    }

    else  /* error: 1st keyword of extension != XTENSION */
    {
        sprintf(message,
        "First keyword of the extension is not XTENSION: %s", name);
        ffpmsg(message);
        return(*status = NO_XTENSION);
    }

    if (ffgttb(fptr, &llnaxis1, &llnaxis2, &pcount, &fields, status) > 0)
        return(*status);

    if (naxis1)
       *naxis1 = llnaxis1;

    if (naxis2)
       *naxis2 = llnaxis2;

    if (pcount != 0)
    {
       sprintf(message, "PCOUNT = %.0f is illegal in ASCII table; must = 0",
             (double) pcount);
       ffpmsg(message);
       return(*status = BAD_PCOUNT);
    }

    if (tfields)
       *tfields = fields;

    if (maxfield < 0)
        maxf = fields;
    else
        maxf = minvalue(maxfield, fields);

    if (maxf > 0)
    {
        for (ii = 0; ii < maxf; ii++)
        {   /* initialize optional keyword values */
            if (ttype)
                *ttype[ii] = '\0';   

            if (tunit)
                *tunit[ii] = '\0';
        }

   
        if (ttype)
            ffgkns(fptr, "TTYPE", 1, maxf, ttype, &nfound, status);

        if (tunit)
            ffgkns(fptr, "TUNIT", 1, maxf, tunit, &nfound, status);

        if (*status > 0)
            return(*status);

        if (tbcol)
        {
            ffgknjj(fptr, "TBCOL", 1, maxf, tbcol, &nfound, status);

            if (*status > 0 || nfound != maxf)
            {
                ffpmsg(
        "Required TBCOL keyword(s) not found in ASCII table header (ffghtbll).");
                return(*status = NO_TBCOL);
            }
        }

        if (tform)
        {
            ffgkns(fptr, "TFORM", 1, maxf, tform, &nfound, status);

            if (*status > 0 || nfound != maxf)
            {
                ffpmsg(
        "Required TFORM keyword(s) not found in ASCII table header (ffghtbll).");
                return(*status = NO_TFORM);
            }
        }
    }

    if (extnm)
    {
        extnm[0] = '\0';

        tstatus = *status;
        ffgkys(fptr, "EXTNAME", extnm, comm, status);

        if (*status == KEY_NO_EXIST)
            *status = tstatus;  /* keyword not required, so ignore error */
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghbn(fitsfile *fptr,  /* I - FITS file pointer                        */
           int maxfield,    /* I - maximum no. of columns to read;          */
           long *naxis2,    /* O - number of rows in the table              */
           int *tfields,    /* O - number of columns in the table           */
           char **ttype,    /* O - name of each column                      */
           char **tform,    /* O - TFORMn value for each column             */
           char **tunit,    /* O - TUNITn value for each column             */
           char *extnm,     /* O - value of EXTNAME keyword, if any         */
           long *pcount,    /* O - value of PCOUNT keyword                  */
           int *status)     /* IO - error status                            */
/*
  Get keywords from the Header of the BiNary table:
  Check that the keywords conform to the FITS standard and return the
  parameters which describe the table.
*/
{
    int ii, maxf, nfound, tstatus;
    long  fields;
    char name[FLEN_KEYWORD], value[FLEN_VALUE], comm[FLEN_COMMENT];
    char xtension[FLEN_VALUE], message[81];
    LONGLONG naxis1ll, naxis2ll, pcountll;

    if (*status > 0)
        return(*status);

    /* read the first keyword of the extension */
    ffgkyn(fptr, 1, name, value, comm, status);

    if (!strcmp(name, "XTENSION"))
    {
            if (ffc2s(value, xtension, status) > 0)  /* get the value string */
            {
                ffpmsg("Bad value string for XTENSION keyword:");
                ffpmsg(value);
                return(*status);
            }

            /* allow the quoted string value to begin in any column and */
            /* allow any number of trailing blanks before the closing quote */
            if ( (value[0] != '\'')   ||  /* first char must be a quote */
                 ( strcmp(xtension, "BINTABLE") &&
                   strcmp(xtension, "A3DTABLE") &&
                   strcmp(xtension, "3DTABLE")
                 ) )
            {
                sprintf(message,
                "This is not a BINTABLE extension: %s", value);
                ffpmsg(message);
                return(*status = NOT_BTABLE);
            }
    }

    else  /* error: 1st keyword of extension != XTENSION */
    {
        sprintf(message,
        "First keyword of the extension is not XTENSION: %s", name);
        ffpmsg(message);
        return(*status = NO_XTENSION);
    }

    if (ffgttb(fptr, &naxis1ll, &naxis2ll, &pcountll, &fields, status) > 0)
        return(*status);

    if (naxis2)
       *naxis2 = (long) naxis2ll;

    if (pcount)
       *pcount = (long) pcountll;

    if (tfields)
        *tfields = fields;

    if (maxfield < 0)
        maxf = fields;
    else
        maxf = minvalue(maxfield, fields);

    if (maxf > 0)
    {
        for (ii = 0; ii < maxf; ii++)
        {   /* initialize optional keyword values */
            if (ttype)
                *ttype[ii] = '\0';   

            if (tunit)
                *tunit[ii] = '\0';
        }

        if (ttype)
            ffgkns(fptr, "TTYPE", 1, maxf, ttype, &nfound, status);

        if (tunit)
            ffgkns(fptr, "TUNIT", 1, maxf, tunit, &nfound, status);

        if (*status > 0)
            return(*status);

        if (tform)
        {
            ffgkns(fptr, "TFORM", 1, maxf, tform, &nfound, status);

            if (*status > 0 || nfound != maxf)
            {
                ffpmsg(
        "Required TFORM keyword(s) not found in binary table header (ffghbn).");
                return(*status = NO_TFORM);
            }
        }
    }

    if (extnm)
    {
        extnm[0] = '\0';

        tstatus = *status;
        ffgkys(fptr, "EXTNAME", extnm, comm, status);

        if (*status == KEY_NO_EXIST)
          *status = tstatus;  /* keyword not required, so ignore error */
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghbnll(fitsfile *fptr,  /* I - FITS file pointer                        */
           int maxfield,    /* I - maximum no. of columns to read;          */
           LONGLONG *naxis2,    /* O - number of rows in the table              */
           int *tfields,    /* O - number of columns in the table           */
           char **ttype,    /* O - name of each column                      */
           char **tform,    /* O - TFORMn value for each column             */
           char **tunit,    /* O - TUNITn value for each column             */
           char *extnm,     /* O - value of EXTNAME keyword, if any         */
           LONGLONG *pcount,    /* O - value of PCOUNT keyword                  */
           int *status)     /* IO - error status                            */
/*
  Get keywords from the Header of the BiNary table:
  Check that the keywords conform to the FITS standard and return the
  parameters which describe the table.
*/
{
    int ii, maxf, nfound, tstatus;
    long  fields;
    char name[FLEN_KEYWORD], value[FLEN_VALUE], comm[FLEN_COMMENT];
    char xtension[FLEN_VALUE], message[81];
    LONGLONG naxis1ll, naxis2ll, pcountll;

    if (*status > 0)
        return(*status);

    /* read the first keyword of the extension */
    ffgkyn(fptr, 1, name, value, comm, status);

    if (!strcmp(name, "XTENSION"))
    {
            if (ffc2s(value, xtension, status) > 0)  /* get the value string */
            {
                ffpmsg("Bad value string for XTENSION keyword:");
                ffpmsg(value);
                return(*status);
            }

            /* allow the quoted string value to begin in any column and */
            /* allow any number of trailing blanks before the closing quote */
            if ( (value[0] != '\'')   ||  /* first char must be a quote */
                 ( strcmp(xtension, "BINTABLE") &&
                   strcmp(xtension, "A3DTABLE") &&
                   strcmp(xtension, "3DTABLE")
                 ) )
            {
                sprintf(message,
                "This is not a BINTABLE extension: %s", value);
                ffpmsg(message);
                return(*status = NOT_BTABLE);
            }
    }

    else  /* error: 1st keyword of extension != XTENSION */
    {
        sprintf(message,
        "First keyword of the extension is not XTENSION: %s", name);
        ffpmsg(message);
        return(*status = NO_XTENSION);
    }

    if (ffgttb(fptr, &naxis1ll, &naxis2ll, &pcountll, &fields, status) > 0)
        return(*status);

    if (naxis2)
       *naxis2 = naxis2ll;

    if (pcount)
       *pcount = pcountll;

    if (tfields)
        *tfields = fields;

    if (maxfield < 0)
        maxf = fields;
    else
        maxf = minvalue(maxfield, fields);

    if (maxf > 0)
    {
        for (ii = 0; ii < maxf; ii++)
        {   /* initialize optional keyword values */
            if (ttype)
                *ttype[ii] = '\0';   

            if (tunit)
                *tunit[ii] = '\0';
        }

        if (ttype)
            ffgkns(fptr, "TTYPE", 1, maxf, ttype, &nfound, status);

        if (tunit)
            ffgkns(fptr, "TUNIT", 1, maxf, tunit, &nfound, status);

        if (*status > 0)
            return(*status);

        if (tform)
        {
            ffgkns(fptr, "TFORM", 1, maxf, tform, &nfound, status);

            if (*status > 0 || nfound != maxf)
            {
                ffpmsg(
        "Required TFORM keyword(s) not found in binary table header (ffghbn).");
                return(*status = NO_TFORM);
            }
        }
    }

    if (extnm)
    {
        extnm[0] = '\0';

        tstatus = *status;
        ffgkys(fptr, "EXTNAME", extnm, comm, status);

        if (*status == KEY_NO_EXIST)
          *status = tstatus;  /* keyword not required, so ignore error */
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgphd(fitsfile *fptr,  /* I - FITS file pointer                        */
           int maxdim,      /* I - maximum no. of dimensions to read;       */
           int *simple,     /* O - does file conform to FITS standard? 1/0  */
           int *bitpix,     /* O - number of bits per data value pixel      */
           int *naxis,      /* O - number of axes in the data array         */
           LONGLONG naxes[],    /* O - length of each data axis                 */
           long *pcount,    /* O - number of group parameters (usually 0)   */
           long *gcount,    /* O - number of random groups (usually 1 or 0) */
           int *extend,     /* O - may FITS file haave extensions?          */
           double *bscale,  /* O - array pixel linear scaling factor        */
           double *bzero,   /* O - array pixel linear scaling zero point    */
           LONGLONG *blank, /* O - value used to represent undefined pixels */
           int *nspace,     /* O - number of blank keywords prior to END    */
           int *status)     /* IO - error status                            */
{
/*
  Get the Primary HeaDer parameters.  Check that the keywords conform to
  the FITS standard and return the parameters which determine the size and
  structure of the primary array or IMAGE extension.
*/
    int unknown, found_end, tstatus, ii, nextkey, namelen;
    long longbitpix, longnaxis;
    LONGLONG axislen;
    char message[FLEN_ERRMSG], keyword[FLEN_KEYWORD];
    char card[FLEN_CARD];
    char name[FLEN_KEYWORD], value[FLEN_VALUE], comm[FLEN_COMMENT];
    char xtension[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (simple)
       *simple = 1;

    unknown = 0;

    /*--------------------------------------------------------------------*/
    /*  Get 1st keyword of HDU and test whether it is SIMPLE or XTENSION  */
    /*--------------------------------------------------------------------*/
    ffgkyn(fptr, 1, name, value, comm, status);

    if ((fptr->Fptr)->curhdu == 0) /* Is this the beginning of the FITS file? */
    {
        if (!strcmp(name, "SIMPLE"))
        {
            if (value[0] == 'F')
            {
                if (simple)
                    *simple=0;          /* not a simple FITS file */
            }
            else if (value[0] != 'T')
                return(*status = BAD_SIMPLE);
        }

        else
        {
            sprintf(message,
                   "First keyword of the file is not SIMPLE: %s", name);
            ffpmsg(message);
            return(*status = NO_SIMPLE);
        }
    }

    else    /* not beginning of the file, so presumably an IMAGE extension */
    {       /* or it could be a compressed image in a binary table */

        if (!strcmp(name, "XTENSION"))
        {
            if (ffc2s(value, xtension, status) > 0)  /* get the value string */
            {
                ffpmsg("Bad value string for XTENSION keyword:");
                ffpmsg(value);
                return(*status);
            }

            /* allow the quoted string value to begin in any column and */
            /* allow any number of trailing blanks before the closing quote */
            if ( (value[0] != '\'')   ||  /* first char must be a quote */
                  ( strcmp(xtension, "IMAGE")  &&
                    strcmp(xtension, "IUEIMAGE") ) )
            {
                unknown = 1;  /* unknown type of extension; press on anyway */
                sprintf(message,
                   "This is not an IMAGE extension: %s", value);
                ffpmsg(message);
            }
        }

        else  /* error: 1st keyword of extension != XTENSION */
        {
            sprintf(message,
            "First keyword of the extension is not XTENSION: %s", name);
            ffpmsg(message);
            return(*status = NO_XTENSION);
        }
    }

    if (unknown && (fptr->Fptr)->compressimg)
    {
        /* this is a compressed image, so read ZBITPIX, ZNAXIS keywords */
        unknown = 0;  /* reset flag */
        ffxmsg(3, message); /* clear previous spurious error message */

        if (bitpix)
        {
            ffgidt(fptr, bitpix, status); /* get bitpix value */

            if (*status > 0)
            {
                ffpmsg("Error reading BITPIX value of compressed image");
                return(*status);
            }
        }

        if (naxis)
        {
            ffgidm(fptr, naxis, status); /* get NAXIS value */

            if (*status > 0)
            {
                ffpmsg("Error reading NAXIS value of compressed image");
                return(*status);
            }
        }

        if (naxes)
        {
            ffgiszll(fptr, maxdim, naxes, status);  /* get NAXISn value */

            if (*status > 0)
            {
                ffpmsg("Error reading NAXISn values of compressed image");
                return(*status);
            }
        }

        nextkey = 9; /* skip required table keywords in the following search */
    }
    else
    {

        /*----------------------------------------------------------------*/
        /*  Get 2nd keyword;  test whether it is BITPIX with legal value  */
        /*----------------------------------------------------------------*/
        ffgkyn(fptr, 2, name, value, comm, status);  /* BITPIX = 2nd keyword */

        if (strcmp(name, "BITPIX"))
        {
            sprintf(message,
            "Second keyword of the extension is not BITPIX: %s", name);
            ffpmsg(message);
            return(*status = NO_BITPIX);
        }

        if (ffc2ii(value,  &longbitpix, status) > 0)
        {
            sprintf(message,
            "Value of BITPIX keyword is not an integer: %s", value);
            ffpmsg(message);
            return(*status = BAD_BITPIX);
        }
        else if (longbitpix != BYTE_IMG && longbitpix != SHORT_IMG &&
             longbitpix != LONG_IMG && longbitpix != LONGLONG_IMG &&
             longbitpix != FLOAT_IMG && longbitpix != DOUBLE_IMG)
        {
            sprintf(message,
            "Illegal value for BITPIX keyword: %s", value);
            ffpmsg(message);
            return(*status = BAD_BITPIX);
        }
        if (bitpix)
            *bitpix = longbitpix;  /* do explicit type conversion */

        /*---------------------------------------------------------------*/
        /*  Get 3rd keyword;  test whether it is NAXIS with legal value  */
        /*---------------------------------------------------------------*/
        ffgtkn(fptr, 3, "NAXIS",  &longnaxis, status);

        if (*status == BAD_ORDER)
            return(*status = NO_NAXIS);
        else if (*status == NOT_POS_INT || longnaxis > 999)
        {
            sprintf(message,"NAXIS = %ld is illegal", longnaxis);
            ffpmsg(message);
            return(*status = BAD_NAXIS);
        }
        else
            if (naxis)
                 *naxis = longnaxis;  /* do explicit type conversion */

        /*---------------------------------------------------------*/
        /*  Get the next NAXISn keywords and test for legal values */
        /*---------------------------------------------------------*/
        for (ii=0, nextkey=4; ii < longnaxis; ii++, nextkey++)
        {
            ffkeyn("NAXIS", ii+1, keyword, status);
            ffgtknjj(fptr, 4+ii, keyword, &axislen, status);

            if (*status == BAD_ORDER)
                return(*status = NO_NAXES);
            else if (*status == NOT_POS_INT)
                return(*status = BAD_NAXES);
            else if (ii < maxdim)
                if (naxes)
                    naxes[ii] = axislen;
        }
    }

    /*---------------------------------------------------------*/
    /*  now look for other keywords of interest:               */
    /*  BSCALE, BZERO, BLANK, PCOUNT, GCOUNT, EXTEND, and END  */
    /*---------------------------------------------------------*/

    /*  initialize default values in case keyword is not present */
    if (bscale)
        *bscale = 1.0;
    if (bzero)
        *bzero  = 0.0;
    if (pcount)
        *pcount = 0;
    if (gcount)
        *gcount = 1;
    if (extend)
        *extend = 0;
    if (blank)
      *blank = NULL_UNDEFINED; /* no default null value for BITPIX=8,16,32 */

    *nspace = 0;
    found_end = 0;
    tstatus = *status;

    for (; !found_end; nextkey++)  
    {
      /* get next keyword */
      /* don't use ffgkyn here because it trys to parse the card to read */
      /* the value string, thus failing to read the file just because of */
      /* minor syntax errors in optional keywords.                       */

      if (ffgrec(fptr, nextkey, card, status) > 0 )  /* get the 80-byte card */
      {
        if (*status == KEY_OUT_BOUNDS)
        {
          found_end = 1;  /* simply hit the end of the header */
          *status = tstatus;  /* reset error status */
        }
        else          
        {
          ffpmsg("Failed to find the END keyword in header (ffgphd).");
        }
      }
      else /* got the next keyword without error */
      {
        ffgknm(card, name, &namelen, status); /* get the keyword name */

        if (fftrec(name, status) > 0)  /* test keyword name; catches no END */
        {
          sprintf(message,
              "Name of keyword no. %d contains illegal character(s): %s",
              nextkey, name);
          ffpmsg(message);

          if (nextkey % 36 == 0) /* test if at beginning of 36-card record */
            ffpmsg("  (This may indicate a missing END keyword).");
        }

        if (!strcmp(name, "BSCALE") && bscale)
        {
            *nspace = 0;  /* reset count of blank keywords */
            ffpsvc(card, value, comm, status); /* parse value and comment */

            if (ffc2dd(value, bscale, status) > 0) /* convert to double */
            {
                /* reset error status and continue, but still issue warning */
                *status = tstatus;
                *bscale = 1.0;

                sprintf(message,
                "Error reading BSCALE keyword value as a double: %s", value);
                ffpmsg(message);
            }
        }

        else if (!strcmp(name, "BZERO") && bzero)
        {
            *nspace = 0;  /* reset count of blank keywords */
            ffpsvc(card, value, comm, status); /* parse value and comment */

            if (ffc2dd(value, bzero, status) > 0) /* convert to double */
            {
                /* reset error status and continue, but still issue warning */
                *status = tstatus;
                *bzero = 0.0;

                sprintf(message,
                "Error reading BZERO keyword value as a double: %s", value);
                ffpmsg(message);
            }
        }

        else if (!strcmp(name, "BLANK") && blank)
        {
            *nspace = 0;  /* reset count of blank keywords */
            ffpsvc(card, value, comm, status); /* parse value and comment */

            if (ffc2jj(value, blank, status) > 0) /* convert to LONGLONG */
            {
                /* reset error status and continue, but still issue warning */
                *status = tstatus;
                *blank = NULL_UNDEFINED;

                sprintf(message,
                "Error reading BLANK keyword value as an integer: %s", value);
                ffpmsg(message);
            }
        }

        else if (!strcmp(name, "PCOUNT") && pcount)
        {
            *nspace = 0;  /* reset count of blank keywords */
            ffpsvc(card, value, comm, status); /* parse value and comment */

            if (ffc2ii(value, pcount, status) > 0) /* convert to long */
            {
                sprintf(message,
                "Error reading PCOUNT keyword value as an integer: %s", value);
                ffpmsg(message);
            }
        }

        else if (!strcmp(name, "GCOUNT") && gcount)
        {
            *nspace = 0;  /* reset count of blank keywords */
            ffpsvc(card, value, comm, status); /* parse value and comment */

            if (ffc2ii(value, gcount, status) > 0) /* convert to long */
            {
                sprintf(message,
                "Error reading GCOUNT keyword value as an integer: %s", value);
                ffpmsg(message);
            }
        }

        else if (!strcmp(name, "EXTEND") && extend)
        {
            *nspace = 0;  /* reset count of blank keywords */
            ffpsvc(card, value, comm, status); /* parse value and comment */

            if (ffc2ll(value, extend, status) > 0) /* convert to logical */
            {
                /* reset error status and continue, but still issue warning */
                *status = tstatus;
                *extend = 0;

                sprintf(message,
                "Error reading EXTEND keyword value as a logical: %s", value);
                ffpmsg(message);
            }
        }

        else if (!strcmp(name, "END"))
            found_end = 1;

        else if (!card[0] )
            *nspace = *nspace + 1;  /* this is a blank card in the header */

        else
            *nspace = 0;  /* reset count of blank keywords immediately
                            before the END keyword to zero   */
      }

      if (*status > 0)  /* exit on error after writing error message */
      {
        if ((fptr->Fptr)->curhdu == 0)
            ffpmsg(
            "Failed to read the required primary array header keywords.");
        else
            ffpmsg(
            "Failed to read the required image extension header keywords.");

        return(*status);
      }
    }

    if (unknown)
       *status = NOT_IMAGE;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgttb(fitsfile *fptr,      /* I - FITS file pointer*/
           LONGLONG *rowlen,        /* O - length of a table row, in bytes */
           LONGLONG *nrows,         /* O - number of rows in the table */
           LONGLONG *pcount,    /* O - value of PCOUNT keyword */
           long *tfields,       /* O - number of fields in the table */
           int *status)         /* IO - error status    */
{
/*
  Get and Test TaBle;
  Test that this is a legal ASCII or binary table and get some keyword values.
  We assume that the calling routine has already tested the 1st keyword
  of the extension to ensure that this is really a table extension.
*/
    if (*status > 0)
        return(*status);

    if (fftkyn(fptr, 2, "BITPIX", "8", status) == BAD_ORDER) /* 2nd keyword */
        return(*status = NO_BITPIX);  /* keyword not BITPIX */
    else if (*status == NOT_POS_INT)
        return(*status = BAD_BITPIX); /* value != 8 */

    if (fftkyn(fptr, 3, "NAXIS", "2", status) == BAD_ORDER) /* 3rd keyword */
        return(*status = NO_NAXIS);  /* keyword not NAXIS */
    else if (*status == NOT_POS_INT)
        return(*status = BAD_NAXIS); /* value != 2 */

    if (ffgtknjj(fptr, 4, "NAXIS1", rowlen, status) == BAD_ORDER) /* 4th key */
        return(*status = NO_NAXES);  /* keyword not NAXIS1 */
    else if (*status == NOT_POS_INT)
        return(*status == BAD_NAXES); /* bad NAXIS1 value */

    if (ffgtknjj(fptr, 5, "NAXIS2", nrows, status) == BAD_ORDER) /* 5th key */
        return(*status = NO_NAXES);  /* keyword not NAXIS2 */
    else if (*status == NOT_POS_INT)
        return(*status == BAD_NAXES); /* bad NAXIS2 value */

    if (ffgtknjj(fptr, 6, "PCOUNT", pcount, status) == BAD_ORDER) /* 6th key */
        return(*status = NO_PCOUNT);  /* keyword not PCOUNT */
    else if (*status == NOT_POS_INT)
        return(*status = BAD_PCOUNT); /* bad PCOUNT value */

    if (fftkyn(fptr, 7, "GCOUNT", "1", status) == BAD_ORDER) /* 7th keyword */
        return(*status = NO_GCOUNT);  /* keyword not GCOUNT */
    else if (*status == NOT_POS_INT)
        return(*status = BAD_GCOUNT); /* value != 1 */

    if (ffgtkn(fptr, 8, "TFIELDS", tfields, status) == BAD_ORDER) /* 8th key*/
        return(*status = NO_TFIELDS);  /* keyword not TFIELDS */
    else if (*status == NOT_POS_INT || *tfields > 999)
        return(*status == BAD_TFIELDS); /* bad TFIELDS value */


    if (*status > 0)
       ffpmsg(
       "Error reading required keywords in the table header (FTGTTB).");

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtkn(fitsfile *fptr,  /* I - FITS file pointer              */
           int numkey,      /* I - number of the keyword to read  */
           char *name,      /* I - expected name of the keyword   */
           long *value,     /* O - integer value of the keyword   */
           int *status)     /* IO - error status                  */
{
/*
  test that keyword number NUMKEY has the expected name and get the
  integer value of the keyword.  Return an error if the keyword
  name does not match the input name, or if the value of the
  keyword is not a positive integer.
*/
    char keyname[FLEN_KEYWORD], valuestring[FLEN_VALUE];
    char comm[FLEN_COMMENT], message[FLEN_ERRMSG];
   
    if (*status > 0)
        return(*status);
    
    keyname[0] = '\0';
    valuestring[0] = '\0';

    if (ffgkyn(fptr, numkey, keyname, valuestring, comm, status) <= 0)
    {
        if (strcmp(keyname, name) )
            *status = BAD_ORDER;  /* incorrect keyword name */

        else
        {
            ffc2ii(valuestring, value, status);  /* convert to integer */

            if (*status > 0 || *value < 0 )
               *status = NOT_POS_INT;
        }

        if (*status > 0)
        {
            sprintf(message,
              "ffgtkn found unexpected keyword or value for keyword no. %d.",
              numkey);
            ffpmsg(message);

            sprintf(message,
              " Expected positive integer keyword %s, but instead", name);
            ffpmsg(message);

            sprintf(message,
              " found keyword %s with value %s", keyname, valuestring);
            ffpmsg(message);
        }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtknjj(fitsfile *fptr,  /* I - FITS file pointer              */
           int numkey,      /* I - number of the keyword to read  */
           char *name,      /* I - expected name of the keyword   */
           LONGLONG *value, /* O - integer value of the keyword   */
           int *status)     /* IO - error status                  */
{
/*
  test that keyword number NUMKEY has the expected name and get the
  integer value of the keyword.  Return an error if the keyword
  name does not match the input name, or if the value of the
  keyword is not a positive integer.
*/
    char keyname[FLEN_KEYWORD], valuestring[FLEN_VALUE];
    char comm[FLEN_COMMENT], message[FLEN_ERRMSG];
   
    if (*status > 0)
        return(*status);
    
    keyname[0] = '\0';
    valuestring[0] = '\0';

    if (ffgkyn(fptr, numkey, keyname, valuestring, comm, status) <= 0)
    {
        if (strcmp(keyname, name) )
            *status = BAD_ORDER;  /* incorrect keyword name */

        else
        {
            ffc2jj(valuestring, value, status);  /* convert to integer */

            if (*status > 0 || *value < 0 )
               *status = NOT_POS_INT;
        }

        if (*status > 0)
        {
            sprintf(message,
              "ffgtknjj found unexpected keyword or value for keyword no. %d.",
              numkey);
            ffpmsg(message);

            sprintf(message,
              " Expected positive integer keyword %s, but instead", name);
            ffpmsg(message);

            sprintf(message,
              " found keyword %s with value %s", keyname, valuestring);
            ffpmsg(message);
        }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fftkyn(fitsfile *fptr,  /* I - FITS file pointer              */
           int numkey,      /* I - number of the keyword to read  */
           char *name,      /* I - expected name of the keyword   */
           char *value,     /* I - expected value of the keyword  */
           int *status)     /* IO - error status                  */
{
/*
  test that keyword number NUMKEY has the expected name and the
  expected value string.
*/
    char keyname[FLEN_KEYWORD], valuestring[FLEN_VALUE];
    char comm[FLEN_COMMENT], message[FLEN_ERRMSG];
   
    if (*status > 0)
        return(*status);
    
    keyname[0] = '\0';
    valuestring[0] = '\0';

    if (ffgkyn(fptr, numkey, keyname, valuestring, comm, status) <= 0)
    {
        if (strcmp(keyname, name) )
            *status = BAD_ORDER;  /* incorrect keyword name */

        if (strcmp(value, valuestring) )
            *status = NOT_POS_INT;  /* incorrect keyword value */
    }

    if (*status > 0)
    {
        sprintf(message,
          "fftkyn found unexpected keyword or value for keyword no. %d.",
          numkey);
        ffpmsg(message);

        sprintf(message,
          " Expected keyword %s with value %s, but", name, value);
        ffpmsg(message);

        sprintf(message,
          " found keyword %s with value %s", keyname, valuestring);
        ffpmsg(message);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffh2st(fitsfile *fptr,   /* I - FITS file pointer           */
           char **header,    /* O - returned header string      */
           int  *status)     /* IO - error status               */

/*
  read header keywords into a long string of chars.  This routine allocates
  memory for the string, so the calling routine must eventually free the
  memory when it is not needed any more.
*/
{
    int nkeys;
    long nrec;
    LONGLONG headstart;

    if (*status > 0)
        return(*status);

    /* get number of keywords in the header (doesn't include END) */
    if (ffghsp(fptr, &nkeys, NULL, status) > 0)
        return(*status);

    nrec = (nkeys / 36 + 1);

    /* allocate memory for all the keywords (multiple of 2880 bytes) */
    *header = (char *) calloc ( nrec * 2880 + 1, 1);
    if (!(*header))
    {
         *status = MEMORY_ALLOCATION;
         ffpmsg("failed to allocate memory to hold all the header keywords");
         return(*status);
    }

    ffghadll(fptr, &headstart, NULL, NULL, status); /* get header address */
    ffmbyt(fptr, headstart, REPORT_EOF, status);   /* move to header */
    ffgbyt(fptr, nrec * 2880, *header, status);     /* copy header */
    *(*header + (nrec * 2880)) = '\0';

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffhdr2str( fitsfile *fptr,  /* I - FITS file pointer                    */
            int exclude_comm,   /* I - if TRUE, exclude commentary keywords */
            char **exclist,     /* I - list of excluded keyword names       */
            int nexc,           /* I - number of names in exclist           */
            char **header,      /* O - returned header string               */
            int *nkeys,         /* O - returned number of 80-char keywords  */
            int  *status)       /* IO - error status                        */
/*
  read header keywords into a long string of chars.  This routine allocates
  memory for the string, so the calling routine must eventually free the
  memory when it is not needed any more.  If exclude_comm is TRUE, then all 
  the COMMENT, HISTORY, and <blank> keywords will be excluded from the output
  string of keywords.  Any other list of keywords to be excluded may be
  specified with the exclist parameter.
*/
{
    int casesn, match, exact, totkeys;
    long ii, jj;
    char keybuf[162], keyname[FLEN_KEYWORD], *headptr;

    *nkeys = 0;

    if (*status > 0)
        return(*status);

    /* get number of keywords in the header (doesn't include END) */
    if (ffghsp(fptr, &totkeys, NULL, status) > 0)
        return(*status);

    /* allocate memory for all the keywords */
    /* (will reallocate it later to minimize the memory size) */
    
    *header = (char *) calloc ( (totkeys + 1) * 80 + 1, 1);
    if (!(*header))
    {
         *status = MEMORY_ALLOCATION;
         ffpmsg("failed to allocate memory to hold all the header keywords");
         return(*status);
    }

    headptr = *header;
    casesn = FALSE;

    /* read every keyword */
    for (ii = 1; ii <= totkeys; ii++) 
    {
        ffgrec(fptr, ii, keybuf, status);
        /* pad record with blanks so that it is at least 80 chars long */
        strcat(keybuf,
    "                                                                                ");

        keyname[0] = '\0';
        strncat(keyname, keybuf, 8); /* copy the keyword name */
        
        if (exclude_comm)
        {
            if (!FSTRCMP("COMMENT ", keyname) ||
                !FSTRCMP("HISTORY ", keyname) ||
                !FSTRCMP("        ", keyname) )
              continue;  /* skip this commentary keyword */
        }

        /* does keyword match any names in the exclusion list? */
        for (jj = 0; jj < nexc; jj++ )
        {
            ffcmps(exclist[jj], keyname, casesn, &match, &exact);
                 if (match)
                     break;
        }

        if (jj == nexc)
        {
            /* not in exclusion list, add this keyword to the string */
            strcpy(headptr, keybuf);
            headptr += 80;
            (*nkeys)++;
        }
    }

    /* add the END keyword */
    strcpy(headptr,
    "END                                                                             ");
    headptr += 80;
    (*nkeys)++;

    *headptr = '\0';   /* terminate the header string */
    /* minimize the allocated memory */
    *header = (char *) realloc(*header, (*nkeys *80) + 1);  

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcnvthdr2str( fitsfile *fptr,  /* I - FITS file pointer                    */
            int exclude_comm,   /* I - if TRUE, exclude commentary keywords */
            char **exclist,     /* I - list of excluded keyword names       */
            int nexc,           /* I - number of names in exclist           */
            char **header,      /* O - returned header string               */
            int *nkeys,         /* O - returned number of 80-char keywords  */
            int  *status)       /* IO - error status                        */
/*
  Same as ffhdr2str, except that if the input HDU is a tile compressed image
  (stored in a binary table) then it will first convert that header back
  to that of a normal uncompressed FITS image before concatenating the header
  keyword records.
*/
{
    fitsfile *tempfptr;
    
    if (*status > 0)
        return(*status);

    if (fits_is_compressed_image(fptr, status) )
    {
        /* this is a tile compressed image, so need to make an uncompressed */
	/* copy of the image header in memory before concatenating the keywords */
        if (fits_create_file(&tempfptr, "mem://", status) > 0) {
	    return(*status);
	}

	if (fits_img_decompress_header(fptr, tempfptr, status) > 0) {
	    fits_delete_file(tempfptr, status);
	    return(*status);
	}

	ffhdr2str(tempfptr, exclude_comm, exclist, nexc, header, nkeys, status);
	fits_close_file(tempfptr, status);

    } else {
        ffhdr2str(fptr, exclude_comm, exclist, nexc, header, nkeys, status);
    }

    return(*status);
}
