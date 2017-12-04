/*  This file, fitscore.c, contains the core set of FITSIO routines.       */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */
/*

Copyright (Unpublished--all rights reserved under the copyright laws of
the United States), U.S. Government as represented by the Administrator
of the National Aeronautics and Space Administration.  No copyright is
claimed in the United States under Title 17, U.S. Code.

Permission to freely use, copy, modify, and distribute this software
and its documentation without fee is hereby granted, provided that this
copyright notice and disclaimer of warranty appears in all copies.

DISCLAIMER:

THE SOFTWARE IS PROVIDED 'AS IS' WITHOUT ANY WARRANTY OF ANY KIND,
EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED TO,
ANY WARRANTY THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS, ANY
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE, AND FREEDOM FROM INFRINGEMENT, AND ANY WARRANTY THAT THE
DOCUMENTATION WILL CONFORM TO THE SOFTWARE, OR ANY WARRANTY THAT THE
SOFTWARE WILL BE ERROR FREE.  IN NO EVENT SHALL NASA BE LIABLE FOR ANY
DAMAGES, INCLUDING, BUT NOT LIMITED TO, DIRECT, INDIRECT, SPECIAL OR
CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN ANY WAY
CONNECTED WITH THIS SOFTWARE, WHETHER OR NOT BASED UPON WARRANTY,
CONTRACT, TORT , OR OTHERWISE, WHETHER OR NOT INJURY WAS SUSTAINED BY
PERSONS OR PROPERTY OR OTHERWISE, AND WHETHER OR NOT LOSS WAS SUSTAINED
FROM, OR AROSE OUT OF THE RESULTS OF, OR USE OF, THE SOFTWARE OR
SERVICES PROVIDED HEREUNDER."

*/


#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
/* stddef.h is apparently needed to define size_t with some compilers ?? */
#include <stddef.h>
#include <locale.h>
#include "fitsio2.h"

#define errmsgsiz 25
#define ESMARKER 27  /* Escape character is used as error stack marker */

#define DelAll     1 /* delete all messages on the error stack */
#define DelMark    2 /* delete newest messages back to and including marker */
#define DelNewest  3 /* delete the newest message from the stack */
#define GetMesg    4 /* pop and return oldest message, ignoring marks */
#define PutMesg    5 /* add a new message to the stack */
#define PutMark    6 /* add a marker to the stack */

#ifdef _REENTRANT
/*
    Fitsio_Lock and Fitsio_Pthread_Status are declared in fitsio2.h. 
*/
pthread_mutex_t Fitsio_Lock;
int Fitsio_Pthread_Status = 0;

#endif

int STREAM_DRIVER = 0;
struct lconv *lcxxx;

/*--------------------------------------------------------------------------*/
float ffvers(float *version)  /* IO - version number */
/*
  return the current version number of the FITSIO software
*/
{
      *version = (float) 3.42;

/*       Mar 2017

   Previous releases:
      *version = 3.41       Nov 2016
      *version = 3.40       Oct 2016
      *version = 3.39       Apr 2016
      *version = 3.38       Feb 2016
      *version = 3.37     3 Jun 2014
      *version = 3.36     6 Dec 2013
      *version = 3.35    23 May 2013
      *version = 3.34    20 Mar 2013
      *version = 3.33    14 Feb 2013
      *version = 3.32       Oct 2012
      *version = 3.31    18 Jul 2012
      *version = 3.30    11 Apr 2012
      *version = 3.29    22 Sep 2011
      *version = 3.28    12 May 2011
      *version = 3.27     3 Mar 2011
      *version = 3.26    30 Dec 2010
      *version = 3.25    9 June 2010
      *version = 3.24    26 Jan 2010
      *version = 3.23     7 Jan 2010
      *version = 3.22    28 Oct 2009
      *version = 3.21    24 Sep 2009
      *version = 3.20    31 Aug 2009
      *version = 3.18    12 May 2009 (beta version)
      *version = 3.14    18 Mar 2009 
      *version = 3.13     5 Jan 2009 
      *version = 3.12     8 Oct 2008 
      *version = 3.11    19 Sep 2008 
      *version = 3.10    20 Aug 2008 
      *version = 3.09     3 Jun 2008 
      *version = 3.08    15 Apr 2007  (internal release)
      *version = 3.07     5 Nov 2007  (internal release)
      *version = 3.06    27 Aug 2007  
      *version = 3.05    12 Jul 2007  (internal release)
      *version = 3.03    11 Dec 2006
      *version = 3.02    18 Sep 2006
      *version = 3.01       May 2006 included in FTOOLS 6.1 release
      *version = 3.006   20 Feb 2006 
      *version = 3.005   20 Dec 2005 (beta, in heasoft swift release
      *version = 3.004   16 Sep 2005 (beta, in heasoft swift release
      *version = 3.003   28 Jul 2005 (beta, in heasoft swift release
      *version = 3.002   15 Apr 2005 (beta)
      *version = 3.001   15 Mar 2005 (beta) released with heasoft 6.0
      *version = 3.000   1 Mar 2005 (internal release only)
      *version = 2.51     2 Dec 2004
      *version = 2.50    28 Jul 2004
      *version = 2.49    11 Feb 2004
      *version = 2.48    28 Jan 2004
      *version = 2.470   18 Aug 2003
      *version = 2.460   20 May 2003
      *version = 2.450   30 Apr 2003  (internal release only)
      *version = 2.440    8 Jan 2003
      *version = 2.430;   4 Nov 2002
      *version = 2.420;  19 Jul 2002
      *version = 2.410;  22 Apr 2002 used in ftools v5.2
      *version = 2.401;  28 Jan 2002
      *version = 2.400;  18 Jan 2002
      *version = 2.301;   7 Dec 2001
      *version = 2.300;  23 Oct 2001
      *version = 2.204;  26 Jul 2001
      *version = 2.203;  19 Jul 2001 used in ftools v5.1
      *version = 2.202;  22 May 2001
      *version = 2.201;  15 Mar 2001
      *version = 2.200;  26 Jan 2001
      *version = 2.100;  26 Sep 2000
      *version = 2.037;   6 Jul 2000
      *version = 2.036;   1 Feb 2000
      *version = 2.035;   7 Dec 1999 (internal release only)
      *version = 2.034;  23 Nov 1999
      *version = 2.033;  17 Sep 1999
      *version = 2.032;  25 May 1999
      *version = 2.031;  31 Mar 1999
      *version = 2.030;  24 Feb 1999
      *version = 2.029;  11 Feb 1999
      *version = 2.028;  26 Jan 1999
      *version = 2.027;  12 Jan 1999
      *version = 2.026;  23 Dec 1998
      *version = 2.025;   1 Dec 1998
      *version = 2.024;   9 Nov 1998
      *version = 2.023;   1 Nov 1998 first full release of V2.0
      *version = 1.42;   30 Apr 1998
      *version = 1.40;    6 Feb 1998
      *version = 1.33;   16 Dec 1997 (internal release only)
      *version = 1.32;   21 Nov 1997 (internal release only)
      *version = 1.31;    4 Nov 1997 (internal release only)
      *version = 1.30;   11 Sep 1997
      *version = 1.27;    3 Sep 1997 (internal release only)
      *version = 1.25;    2 Jul 1997
      *version = 1.24;    2 May 1997
      *version = 1.23;   24 Apr 1997
      *version = 1.22;   18 Apr 1997
      *version = 1.21;   26 Mar 1997
      *version = 1.2;    29 Jan 1997
      *version = 1.11;   04 Dec 1996
      *version = 1.101;  13 Nov 1996
      *version = 1.1;     6 Nov 1996
      *version = 1.04;   17 Sep 1996
      *version = 1.03;   20 Aug 1996
      *version = 1.02;   15 Aug 1996
      *version = 1.01;   12 Aug 1996
*/

    return(*version);
}
/*--------------------------------------------------------------------------*/
int ffflnm(fitsfile *fptr,    /* I - FITS file pointer  */
           char *filename,    /* O - name of the file   */
           int *status)       /* IO - error status      */
/*
  return the name of the FITS file
*/
{
    strcpy(filename,(fptr->Fptr)->filename);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffflmd(fitsfile *fptr,    /* I - FITS file pointer  */
           int *filemode,     /* O - open mode of the file  */
           int *status)       /* IO - error status      */
/*
  return the access mode of the FITS file
*/
{
    *filemode = (fptr->Fptr)->writemode;
    return(*status);
}
/*--------------------------------------------------------------------------*/
void ffgerr(int status,     /* I - error status value */
            char *errtext)  /* O - error message (max 30 char long + null) */
/*
  Return a short descriptive error message that corresponds to the input
  error status value.  The message may be up to 30 characters long, plus
  the terminating null character.
*/
{
  errtext[0] = '\0';

  if (status >= 0 && status < 300)
  {
    switch (status) {

    case 0:
       strcpy(errtext, "OK - no error");
       break;
    case 1:
       strcpy(errtext, "non-CFITSIO program error");
       break;
    case 101:
       strcpy(errtext, "same input and output files");
       break;
    case 103:
       strcpy(errtext, "attempt to open too many files");
       break;
    case 104:
       strcpy(errtext, "could not open the named file");
       break;
    case 105:
       strcpy(errtext, "couldn't create the named file");
       break;
    case 106:
       strcpy(errtext, "error writing to FITS file");
       break;
    case 107:
       strcpy(errtext, "tried to move past end of file");
       break;
    case 108:
       strcpy(errtext, "error reading from FITS file");
       break;
    case 110:
       strcpy(errtext, "could not close the file");
       break;
    case 111:
       strcpy(errtext, "array dimensions too big");
       break;
    case 112:
       strcpy(errtext, "cannot write to readonly file");
       break;
    case 113:
       strcpy(errtext, "could not allocate memory");
       break;
    case 114:
       strcpy(errtext, "invalid fitsfile pointer");
       break;
    case 115:
       strcpy(errtext, "NULL input pointer");
       break;
    case 116:
       strcpy(errtext, "error seeking file position");
       break;
    case 121:
       strcpy(errtext, "invalid URL prefix");
       break;
    case 122:
       strcpy(errtext, "too many I/O drivers");
       break;
    case 123:
       strcpy(errtext, "I/O driver init failed");
       break;
    case 124:
       strcpy(errtext, "no I/O driver for this URLtype");
       break;
    case 125:
       strcpy(errtext, "parse error in input file URL");
       break;
    case 126:
       strcpy(errtext, "parse error in range list");
       break;
    case 151:
       strcpy(errtext, "bad argument (shared mem drvr)");
       break;
    case 152:
       strcpy(errtext, "null ptr arg (shared mem drvr)");
       break;
    case 153:
       strcpy(errtext, "no free shared memory handles");
       break;
    case 154:
       strcpy(errtext, "share mem drvr not initialized");
       break;
    case 155:
       strcpy(errtext, "IPC system error (shared mem)");
       break;
    case 156:
       strcpy(errtext, "no memory (shared mem drvr)");
       break;
    case 157:
       strcpy(errtext, "share mem resource deadlock");
       break;
    case 158:
       strcpy(errtext, "lock file open/create failed");
       break;
    case 159:
       strcpy(errtext, "can't resize share mem block");
       break;
    case 201:
       strcpy(errtext, "header already has keywords");
       break;
    case 202:
       strcpy(errtext, "keyword not found in header");
       break;
    case 203:
       strcpy(errtext, "keyword number out of bounds");
       break;
    case 204:
       strcpy(errtext, "keyword value is undefined");
       break;
    case 205:
       strcpy(errtext, "string missing closing quote");
       break;
    case 206:
       strcpy(errtext, "error in indexed keyword name");
       break;
    case 207:
       strcpy(errtext, "illegal character in keyword");
       break;
    case 208:
       strcpy(errtext, "required keywords out of order");
       break;
    case 209:
       strcpy(errtext, "keyword value not positive int");
       break;
    case 210:
       strcpy(errtext, "END keyword not found");
       break;
    case 211:
       strcpy(errtext, "illegal BITPIX keyword value");
       break;
    case 212:
       strcpy(errtext, "illegal NAXIS keyword value");
       break;
    case 213:
       strcpy(errtext, "illegal NAXISn keyword value");
       break;
    case 214:
       strcpy(errtext, "illegal PCOUNT keyword value");
       break;
    case 215:
       strcpy(errtext, "illegal GCOUNT keyword value");
       break;
    case 216:
       strcpy(errtext, "illegal TFIELDS keyword value");
       break;
    case 217:
       strcpy(errtext, "negative table row size");
       break;
    case 218:
       strcpy(errtext, "negative number of rows");
       break;
    case 219:
       strcpy(errtext, "named column not found");
       break;
    case 220:
       strcpy(errtext, "illegal SIMPLE keyword value");
       break;
    case 221:
       strcpy(errtext, "first keyword not SIMPLE");
       break;
    case 222:
       strcpy(errtext, "second keyword not BITPIX");
       break;
    case 223:
       strcpy(errtext, "third keyword not NAXIS");
       break;
    case 224:
       strcpy(errtext, "missing NAXISn keywords");
       break;
    case 225:
       strcpy(errtext, "first keyword not XTENSION");
       break;
    case 226:
       strcpy(errtext, "CHDU not an ASCII table");
       break;
    case 227:
       strcpy(errtext, "CHDU not a binary table");
       break;
    case 228:
       strcpy(errtext, "PCOUNT keyword not found");
       break;
    case 229:
       strcpy(errtext, "GCOUNT keyword not found");
       break;
    case 230:
       strcpy(errtext, "TFIELDS keyword not found");
       break;
    case 231:
       strcpy(errtext, "missing TBCOLn keyword");
       break;
    case 232:
       strcpy(errtext, "missing TFORMn keyword");
       break;
    case 233:
       strcpy(errtext, "CHDU not an IMAGE extension");
       break;
    case 234:
       strcpy(errtext, "illegal TBCOLn keyword value");
       break;
    case 235:
       strcpy(errtext, "CHDU not a table extension");
       break;
    case 236:
       strcpy(errtext, "column exceeds width of table");
       break;
    case 237:
       strcpy(errtext, "more than 1 matching col. name");
       break;
    case 241:
       strcpy(errtext, "row width not = field widths");
       break;
    case 251:
       strcpy(errtext, "unknown FITS extension type");
       break;
    case 252:
       strcpy(errtext, "1st key not SIMPLE or XTENSION");
       break;
    case 253:
       strcpy(errtext, "END keyword is not blank");
       break;
    case 254:
       strcpy(errtext, "Header fill area not blank");
       break;
    case 255:
       strcpy(errtext, "Data fill area invalid");
       break;
    case 261:
       strcpy(errtext, "illegal TFORM format code");
       break;
    case 262:
       strcpy(errtext, "unknown TFORM datatype code");
       break;
    case 263:
       strcpy(errtext, "illegal TDIMn keyword value");
       break;
    case 264:
       strcpy(errtext, "invalid BINTABLE heap pointer");
       break;
    default:
       strcpy(errtext, "unknown error status");
       break;
    }
  }
  else if (status < 600)
  {
    switch(status) {

    case 301:
       strcpy(errtext, "illegal HDU number");
       break;
    case 302:
       strcpy(errtext, "column number < 1 or > tfields");
       break;
    case 304:
       strcpy(errtext, "negative byte address");
       break;
    case 306:
       strcpy(errtext, "negative number of elements");
       break;
    case 307:
       strcpy(errtext, "bad first row number");
       break;
    case 308:
       strcpy(errtext, "bad first element number");
       break;
    case 309:
       strcpy(errtext, "not an ASCII (A) column");
       break;
    case 310:
       strcpy(errtext, "not a logical (L) column");
       break;
    case 311:
       strcpy(errtext, "bad ASCII table datatype");
       break;
    case 312:
       strcpy(errtext, "bad binary table datatype");
       break;
    case 314:
       strcpy(errtext, "null value not defined");
       break;
    case 317:
       strcpy(errtext, "not a variable length column");
       break;
    case 320:
       strcpy(errtext, "illegal number of dimensions");
       break;
    case 321:
       strcpy(errtext, "1st pixel no. > last pixel no.");
       break;
    case 322:
       strcpy(errtext, "BSCALE or TSCALn = 0.");
       break;
    case 323:
       strcpy(errtext, "illegal axis length < 1");
       break;
    case 340:
       strcpy(errtext, "not group table");
       break;
    case 341:
       strcpy(errtext, "HDU already member of group");
       break;
    case 342:
       strcpy(errtext, "group member not found");
       break;
    case 343:
       strcpy(errtext, "group not found");
       break;
    case 344:
       strcpy(errtext, "bad group id");
       break;
    case 345:
       strcpy(errtext, "too many HDUs tracked");
       break;
    case 346:
       strcpy(errtext, "HDU alread tracked");
       break;
    case 347:
       strcpy(errtext, "bad Grouping option");
       break;
    case 348:
       strcpy(errtext, "identical pointers (groups)");
       break;
    case 360:
       strcpy(errtext, "malloc failed in parser");
       break;
    case 361:
       strcpy(errtext, "file read error in parser");
       break;
    case 362:
       strcpy(errtext, "null pointer arg (parser)");
       break;
    case 363:
       strcpy(errtext, "empty line (parser)");
       break;
    case 364:
       strcpy(errtext, "cannot unread > 1 line");
       break;
    case 365:
       strcpy(errtext, "parser too deeply nested");
       break;
    case 366:
       strcpy(errtext, "file open failed (parser)");
       break;
    case 367:
       strcpy(errtext, "hit EOF (parser)");
       break;
    case 368:
       strcpy(errtext, "bad argument (parser)");
       break;
    case 369:
       strcpy(errtext, "unexpected token (parser)");
       break;
    case 401:
       strcpy(errtext, "bad int to string conversion");
       break;
    case 402:
       strcpy(errtext, "bad float to string conversion");
       break;
    case 403:
       strcpy(errtext, "keyword value not integer");
       break;
    case 404:
       strcpy(errtext, "keyword value not logical");
       break;
    case 405:
       strcpy(errtext, "keyword value not floating pt");
       break;
    case 406:
       strcpy(errtext, "keyword value not double");
       break;
    case 407:
       strcpy(errtext, "bad string to int conversion");
       break;
    case 408:
       strcpy(errtext, "bad string to float conversion");
       break;
    case 409:
       strcpy(errtext, "bad string to double convert");
       break;
    case 410:
       strcpy(errtext, "illegal datatype code value");
       break;
    case 411:
       strcpy(errtext, "illegal no. of decimals");
       break;
    case 412:
       strcpy(errtext, "datatype conversion overflow");
       break;
    case 413:
       strcpy(errtext, "error compressing image");
       break;
    case 414:
       strcpy(errtext, "error uncompressing image");
       break;
    case 420:
       strcpy(errtext, "bad date or time conversion");
       break;
    case 431:
       strcpy(errtext, "syntax error in expression");
       break;
    case 432:
       strcpy(errtext, "expression result wrong type");
       break;
    case 433:
       strcpy(errtext, "vector result too large");
       break;
    case 434:
       strcpy(errtext, "missing output column");
       break;
    case 435:
       strcpy(errtext, "bad data in parsed column");
       break;
    case 436:
       strcpy(errtext, "output extension of wrong type");
       break;
    case 501:
       strcpy(errtext, "WCS angle too large");
       break;
    case 502:
       strcpy(errtext, "bad WCS coordinate");
       break;
    case 503:
       strcpy(errtext, "error in WCS calculation");
       break;
    case 504:
       strcpy(errtext, "bad WCS projection type");
       break;
    case 505:
       strcpy(errtext, "WCS keywords not found");
       break;
    default:
       strcpy(errtext, "unknown error status");
       break;
    }
  }
  else
  {
     strcpy(errtext, "unknown error status");
  }
  return;
}
/*--------------------------------------------------------------------------*/
void ffpmsg(const char *err_message)
/*
  put message on to error stack
*/
{
    ffxmsg(PutMesg, (char *)err_message);
    return;
}
/*--------------------------------------------------------------------------*/
void ffpmrk(void)
/*
  write a marker to the stack.  It is then possible to pop only those
  messages following the marker off of the stack, leaving the previous
  messages unaffected.

  The marker is ignored by the ffgmsg routine.
*/
{
    char *dummy = 0;

    ffxmsg(PutMark, dummy);
    return;
}
/*--------------------------------------------------------------------------*/
int ffgmsg(char *err_message)
/*
  get oldest message from error stack, ignoring markers
*/
{
    ffxmsg(GetMesg, err_message);
    return(*err_message);
}
/*--------------------------------------------------------------------------*/
void ffcmsg(void)
/*
  erase all messages in the error stack
*/
{
    char *dummy = 0;

    ffxmsg(DelAll, dummy);
    return;
}
/*--------------------------------------------------------------------------*/
void ffcmrk(void)
/*
  erase newest messages in the error stack, stopping if a marker is found.
  The marker is also erased in this case.
*/
{
    char *dummy = 0;

    ffxmsg(DelMark, dummy);
    return;
}
/*--------------------------------------------------------------------------*/
void ffxmsg( int action,
            char *errmsg)
/*
  general routine to get, put, or clear the error message stack.
  Use a static array rather than allocating memory as needed for
  the error messages because it is likely to be more efficient
  and simpler to implement.

  Action Code:
DelAll     1  delete all messages on the error stack 
DelMark    2  delete messages back to and including the 1st marker 
DelNewest  3  delete the newest message from the stack 
GetMesg    4  pop and return oldest message, ignoring marks 
PutMesg    5  add a new message to the stack 
PutMark    6  add a marker to the stack 

*/
{
    int ii;
    char markflag;
    static char *txtbuff[errmsgsiz], *tmpbuff, *msgptr;
    static char errbuff[errmsgsiz][81];  /* initialize all = \0 */
    static int nummsg = 0;

    FFLOCK;
    
    if (action == DelAll)  /* clear the whole message stack */
    {
      for (ii = 0; ii < nummsg; ii ++)
        *txtbuff[ii] = '\0';

      nummsg = 0;
    }
    else if (action == DelMark)  /* clear up to and including first marker */
    {
      while (nummsg > 0) {
        nummsg--;  
        markflag = *txtbuff[nummsg]; /* store possible marker character */
        *txtbuff[nummsg] = '\0';  /* clear the buffer for this msg */

        if (markflag == ESMARKER)
           break;   /* found a marker, so quit */
      }
    }
    else if (action == DelNewest)  /* remove newest message from stack */ 
    {
      if (nummsg > 0)
      {
        nummsg--;  
        *txtbuff[nummsg] = '\0';  /* clear the buffer for this msg */
      }
    }
    else if (action == GetMesg)  /* pop and return oldest message from stack */ 
    {                            /* ignoring markers */
      while (nummsg > 0)
      {
         strcpy(errmsg, txtbuff[0]);   /* copy oldest message to output */

         *txtbuff[0] = '\0';  /* clear the buffer for this msg */
           
         nummsg--;  
         for (ii = 0; ii < nummsg; ii++)
             txtbuff[ii] = txtbuff[ii + 1]; /* shift remaining pointers */

         if (errmsg[0] != ESMARKER) {   /* quit if this is not a marker */
            FFUNLOCK;
            return;
         }
       }
       errmsg[0] = '\0';  /*  no messages in the stack */
    }
    else if (action == PutMesg)  /* add new message to stack */
    {
     msgptr = errmsg;
     while (strlen(msgptr))
     {
      if (nummsg == errmsgsiz)
      {
        tmpbuff = txtbuff[0];  /* buffers full; reuse oldest buffer */
        *txtbuff[0] = '\0';  /* clear the buffer for this msg */

        nummsg--;
        for (ii = 0; ii < nummsg; ii++)
             txtbuff[ii] = txtbuff[ii + 1];   /* shift remaining pointers */

        txtbuff[nummsg] = tmpbuff;  /* set pointer for the new message */
      }
      else
      {
        for (ii = 0; ii < errmsgsiz; ii++)
        {
          if (*errbuff[ii] == '\0') /* find first empty buffer */
          {
            txtbuff[nummsg] = errbuff[ii];
            break;
          }
        }
      }

      strncat(txtbuff[nummsg], msgptr, 80);
      nummsg++;

      msgptr += minvalue(80, strlen(msgptr));
     }
    }
    else if (action == PutMark)  /* put a marker on the stack */
    {
      if (nummsg == errmsgsiz)
      {
        tmpbuff = txtbuff[0];  /* buffers full; reuse oldest buffer */
        *txtbuff[0] = '\0';  /* clear the buffer for this msg */

        nummsg--;
        for (ii = 0; ii < nummsg; ii++)
             txtbuff[ii] = txtbuff[ii + 1];   /* shift remaining pointers */

        txtbuff[nummsg] = tmpbuff;  /* set pointer for the new message */
      }
      else
      {
        for (ii = 0; ii < errmsgsiz; ii++)
        {
          if (*errbuff[ii] == '\0') /* find first empty buffer */
          {
            txtbuff[nummsg] = errbuff[ii];
            break;
          }
        }
      }

      *txtbuff[nummsg] = ESMARKER;      /* write the marker */
      *(txtbuff[nummsg] + 1) = '\0';
      nummsg++;

    }

    FFUNLOCK;
    return;
}
/*--------------------------------------------------------------------------*/
int ffpxsz(int datatype)
/*
   return the number of bytes per pixel associated with the datatype
*/
{
    if (datatype == TBYTE)
       return(sizeof(char));
    else if (datatype == TUSHORT)
       return(sizeof(short));
    else if (datatype == TSHORT)
       return(sizeof(short));
    else if (datatype == TULONG)
       return(sizeof(long));
    else if (datatype == TLONG)
       return(sizeof(long));
    else if (datatype == TINT)
       return(sizeof(int));
    else if (datatype == TUINT)
       return(sizeof(int));
    else if (datatype == TFLOAT)
       return(sizeof(float));
    else if (datatype == TDOUBLE)
       return(sizeof(double));
    else if (datatype == TLOGICAL)
       return(sizeof(char));
    else
       return(0);
}
/*--------------------------------------------------------------------------*/
int fftkey(const char *keyword,    /* I -  keyword name */
           int *status)      /* IO - error status */
/*
  Test that the keyword name conforms to the FITS standard.  Must contain
  only capital letters, digits, minus or underscore chars.  Trailing spaces
  are allowed.  If the input status value is less than zero, then the test
  is modified so that upper or lower case letters are allowed, and no 
  error messages are printed if the keyword is not legal.
*/
{
    size_t maxchr, ii;
    int spaces=0;
    char msg[81], testchar;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    maxchr=strlen(keyword);
    if (maxchr > 8)
        maxchr = 8;

    for (ii = 0; ii < maxchr; ii++)
    {
        if (*status == 0)
            testchar = keyword[ii];
        else
            testchar = toupper(keyword[ii]);

        if ( (testchar >= 'A' && testchar <= 'Z') ||
             (testchar >= '0' && testchar <= '9') ||
              testchar == '-' || testchar == '_'   )
              {
                if (spaces)
                {
                  if (*status == 0)
                  {
                     /* don't print error message if status < 0  */
                    sprintf(msg,
                       "Keyword name contains embedded space(s): %.8s",
                        keyword);
                     ffpmsg(msg);
                  }
                  return(*status = BAD_KEYCHAR);        
                }
              }
        else if (keyword[ii] == ' ')
            spaces = 1;

        else     
        {
          if (*status == 0)
          {
            /* don't print error message if status < 0  */
            sprintf(msg, "Character %d in this keyword is illegal: %.8s",
                    (int) (ii+1), keyword);
            ffpmsg(msg);

            /* explicitly flag the 2 most common cases */
            if (keyword[ii] == 0) 
                ffpmsg(" (This a NULL (0) character).");                
            else if (keyword[ii] == 9)
                ffpmsg(" (This an ASCII TAB (9) character).");   
          }             

          return(*status = BAD_KEYCHAR);        
        }                
    }
    return(*status);        
}
/*--------------------------------------------------------------------------*/
int fftrec(char *card,       /* I -  keyword card to test */
           int *status)      /* IO - error status */
/*
  Test that the keyword card conforms to the FITS standard.  Must contain
  only printable ASCII characters;
*/
{
    size_t ii, maxchr;
    char msg[81];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    maxchr = strlen(card);

    for (ii = 8; ii < maxchr; ii++)
    {
        if (card[ii] < 32 || card[ii] > 126)
        {
            sprintf(msg, 
           "Character %d in this keyword is illegal. Hex Value = %X",
              (int) (ii+1), (int) card[ii] );

            if (card[ii] == 0)
	        strcat(msg, " (NULL char.)");
            else if (card[ii] == 9)
	        strcat(msg, " (TAB char.)");
            else if (card[ii] == 10)
	        strcat(msg, " (Line Feed char.)");
            else if (card[ii] == 11)
	        strcat(msg, " (Vertical Tab)");
            else if (card[ii] == 12)
	        strcat(msg, " (Form Feed char.)");
            else if (card[ii] == 13)
	        strcat(msg, " (Carriage Return)");
            else if (card[ii] == 27)
	        strcat(msg, " (Escape char.)");
            else if (card[ii] == 127)
	        strcat(msg, " (Delete char.)");

            ffpmsg(msg);

            strncpy(msg, card, 80);
            msg[80] = '\0';
            ffpmsg(msg);
            return(*status = BAD_KEYCHAR);        
        }
    }
    return(*status);        
}
/*--------------------------------------------------------------------------*/
void ffupch(char *string)
/*
  convert string to upper case, in place.
*/
{
    size_t len, ii;

    len = strlen(string);
    for (ii = 0; ii < len; ii++)
        string[ii] = toupper(string[ii]);
    return;
}
/*--------------------------------------------------------------------------*/
int ffmkky(const char *keyname,   /* I - keyword name    */
            char *value,     /* I - keyword value   */
            const char *comm,      /* I - keyword comment */
            char *card,      /* O - constructed keyword card */
            int  *status)    /* IO - status value   */
/*
  Make a complete FITS 80-byte keyword card from the input name, value and
  comment strings. Output card is null terminated without any trailing blanks.
*/
{
    size_t namelen, len, ii;
    char tmpname[FLEN_KEYWORD], tmpname2[FLEN_KEYWORD],*cptr;
    char *saveptr;
    int tstatus = -1, nblank = 0, ntoken = 0, maxlen = 0, specialchar = 0;

    if (*status > 0)
        return(*status);

    *tmpname = '\0';
    *tmpname2 = '\0';
    *card = '\0';

    /* skip leading blanks in the name */
    while(*(keyname + nblank) == ' ')
        nblank++;

    strncat(tmpname, keyname + nblank, FLEN_KEYWORD - 1);

    len = strlen(value);        
    namelen = strlen(tmpname);

    /* delete non-significant trailing blanks in the name */
    if (namelen) {
        cptr = tmpname + namelen - 1;

        while(*cptr == ' ') {
            *cptr = '\0';
            cptr--;
        }

        namelen = cptr - tmpname + 1;
    }
    
    /* check that the name does not contain an '=' (equals sign) */
    if (strchr(tmpname, '=') ) {
        ffpmsg("Illegal keyword name; contains an equals sign (=)");
        ffpmsg(tmpname);
        return(*status = BAD_KEYCHAR);
    }

    if (namelen <= 8 && fftkey(tmpname, &tstatus) <= 0 ) { 
    
        /* a normal 8-char (or less) FITS keyword. */
        strcat(card, tmpname);   /* copy keyword name to buffer */
   
        for (ii = namelen; ii < 8; ii++)
            card[ii] = ' ';      /* pad keyword name with spaces */

        card[8]  = '=';          /* append '= ' in columns 9-10 */
        card[9]  = ' ';
        card[10] = '\0';        /* terminate the partial string */
        namelen = 10;
    } else if ((FSTRNCMP(tmpname, "HIERARCH ", 9) == 0) || 
               (FSTRNCMP(tmpname, "hierarch ", 9) == 0) ) {

        /* this is an explicit ESO HIERARCH keyword */

        strcat(card, tmpname);  /* copy keyword name to buffer */

        if (namelen + 3 + len > 80) {
            /* save 1 char by not putting a space before the equals sign */
            strcat(card, "= ");
            namelen += 2;
        } else {
            strcat(card, " = ");
            namelen += 3;
        }
    } else {

	/* scan the keyword name to determine the number and max length of the tokens */
	/* and test if any of the tokens contain nonstandard characters */
	
      	strncat(tmpname2, tmpname, FLEN_KEYWORD - 1);
        cptr = ffstrtok(tmpname2, " ",&saveptr);
	while (cptr) {
	    if (strlen(cptr) > maxlen) maxlen = strlen(cptr); /* find longest token */

	    /* name contains special characters? */
            tstatus = -1;  /* suppress any error message */
	    if (fftkey(cptr, &tstatus) > 0) specialchar = 1; 
	    
	    cptr = ffstrtok(NULL, " ",&saveptr);
	    ntoken++;
	}

        tstatus = -1;  /* suppress any error message */

/*      if (ntoken > 1) { */
        if (ntoken > 0) {  /*  temporarily change so that this case should always be true  */
	    /* for now at least, treat all cases as an implicit ESO HIERARCH keyword. */
	    /* This could  change if FITS is ever expanded to directly support longer keywords. */
	    
            strcat(card, "HIERARCH ");
            strcat(card, tmpname);
	    namelen += 9;

            if (namelen + 3 + len > 80) {
                /* save 1 char by not putting a space before the equals sign */
                strcat(card, "= ");
                namelen += 2;
            } else {
                strcat(card, " = ");
                namelen += 3;
            }

	} else if ((fftkey(tmpname, &tstatus) <= 0)) {
          /* should never get here (at least for now) */
            /* allow keyword names longer than 8 characters */

            strncat(card, tmpname, FLEN_KEYWORD - 1);
            strcat(card, "= ");
            namelen += 2;
        } else {
          /* should never get here (at least for now) */
            ffpmsg("Illegal keyword name:");
            ffpmsg(tmpname);
            return(*status = BAD_KEYCHAR);
        }
    }

    if (len > 0)  /* now process the value string */
    {
        if (value[0] == '\'')  /* is this a quoted string value? */
        {
            if (namelen > 77)
            {
                ffpmsg(
               "The following keyword + value is too long to fit on a card:");
                ffpmsg(keyname);
                ffpmsg(value);
                return(*status = BAD_KEYCHAR);
            }

            strncat(card, value, 80 - namelen); /* append the value string */
            len = minvalue(80, namelen + len);

            /* restore the closing quote if it got truncated */
            if (len == 80)
            {
                   card[79] = '\'';
            }

            if (comm)
            {
              if (comm[0] != 0)
              {
                if (len < 30)
                {
                  for (ii = len; ii < 30; ii++)
                    card[ii] = ' '; /* fill with spaces to col 30 */

                  card[30] = '\0';
                  len = 30;
                }
              }
            }
        }
        else
        {
            if (namelen + len > 80)
            {
                ffpmsg(
               "The following keyword + value is too long to fit on a card:");
                ffpmsg(keyname);
                ffpmsg(value);
                return(*status = BAD_KEYCHAR);
            }
            else if (namelen + len < 30)
            {
                /* add spaces so field ends at least in col 30 */
                strncat(card, "                    ", 30 - (namelen + len));
            }

            strncat(card, value, 80 - namelen); /* append the value string */
            len = minvalue(80, namelen + len);
            len = maxvalue(30, len);
        }

        if (comm)
        {
          if ((len < 77) && ( strlen(comm) > 0) )  /* room for a comment? */
          {
            strcat(card, " / ");   /* append comment separator */
            strncat(card, comm, 77 - len); /* append comment (what fits) */
          } 
        }
    }
    else
    {
      if (namelen == 10)  /* This case applies to normal keywords only */
      {
        card[8] = ' '; /* keywords with no value have no '=' */ 
        if (comm)
        {
          strncat(card, comm, 80 - namelen); /* append comment (what fits) */
        }
      }
    }

    /* issue a warning if this keyword does not strictly conform to the standard
	       HIERARCH convention, which requires,
	         1) at least 2 tokens in the name,
		 2) no tokens longer than 8 characters, and
		 3) no special characters in any of the tokens */

            if (ntoken == 1 || specialchar == 1) {
	       ffpmsg("Warning: the following keyword does not conform to the HIERARCH convention");
	     /*  ffpmsg(" (e.g., name is not hierarchical or contains non-standard characters)."); */
	       ffpmsg(card);
	    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmkey(fitsfile *fptr,    /* I - FITS file pointer  */
           const char *card,  /* I - card string value  */
           int *status)       /* IO - error status      */
/*
  replace the previously read card (i.e. starting 80 bytes before the
  (fptr->Fptr)->nextkey position) with the contents of the input card.
*/
{
    char tcard[81];
    size_t len, ii;
    int keylength = 8;

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    strncpy(tcard,card,80);
    tcard[80] = '\0';

    len = strlen(tcard);

    /* silently replace any illegal characters with a space */
    for (ii=0; ii < len; ii++)  
        if (tcard[ii] < ' ' || tcard[ii] > 126) tcard[ii] = ' ';

    for (ii=len; ii < 80; ii++)    /* fill card with spaces if necessary */
        tcard[ii] = ' ';

    keylength = strcspn(tcard, "=");
    if (keylength == 80) keylength = 8;

    for (ii=0; ii < keylength; ii++)       /* make sure keyword name is uppercase */
        tcard[ii] = toupper(tcard[ii]);

    fftkey(tcard, status);        /* test keyword name contains legal chars */

/*  no need to do this any more, since any illegal characters have been removed
    fftrec(tcard, status);   */     /* test rest of keyword for legal chars   */

    /* move position of keyword to be over written */
    ffmbyt(fptr, ((fptr->Fptr)->nextkey) - 80, REPORT_EOF, status); 
    ffpbyt(fptr, 80, tcard, status);   /* write the 80 byte card */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffkeyn(const char *keyroot,   /* I - root string for keyword name */
           int value,       /* I - index number to be appended to root name */
           char *keyname,   /* O - output root + index keyword name */
           int *status)     /* IO - error status  */
/*
  Construct a keyword name string by appending the index number to the root.
  e.g., if root = "TTYPE" and value = 12 then keyname = "TTYPE12".
  Note: this allows keyword names longer than 8 characters.
*/
{
    char suffix[16];
    size_t rootlen;

    keyname[0] = '\0';            /* initialize output name to null */
    rootlen = strlen(keyroot);

    if (rootlen == 0 || value < 0 )
       return(*status = 206);

    sprintf(suffix, "%d", value); /* construct keyword suffix */

    strcpy(keyname, keyroot);   /* copy root string to name string */
    while (rootlen > 0 && keyname[rootlen - 1] == ' ') {
        rootlen--;                 /* remove trailing spaces in root name */
        keyname[rootlen] = '\0';
    }

    strcat(keyname, suffix);    /* append suffix to the root */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffnkey(int value,       /* I - index number to be appended to root name */
           const char *keyroot,   /* I - root string for keyword name */
           char *keyname,   /* O - output root + index keyword name */
           int *status)     /* IO - error status  */
/*
  Construct a keyword name string by appending the root string to the index
  number. e.g., if root = "TTYPE" and value = 12 then keyname = "12TTYPE".
*/
{
    size_t rootlen;

    keyname[0] = '\0';            /* initialize output name to null */
    rootlen = strlen(keyroot);

    if (rootlen == 0 || rootlen > 7 || value < 0 )
       return(*status = 206);

    sprintf(keyname, "%d", value); /* construct keyword prefix */

    if (rootlen +  strlen(keyname) > 8)
       return(*status = 206);

    strcat(keyname, keyroot);  /* append root to the prefix */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpsvc(char *card,    /* I - FITS header card (nominally 80 bytes long) */
           char *value,   /* O - value string parsed from the card */
           char *comm,    /* O - comment string parsed from the card */
           int *status)   /* IO - error status   */
/*
  ParSe the Value and Comment strings from the input header card string.
  If the card contains a quoted string value, the returned value string
  includes the enclosing quote characters.  If comm = NULL, don't return
  the comment string.
*/
{
    int jj;
    size_t ii, cardlen, nblank, valpos;

    if (*status > 0)
        return(*status);

    value[0] = '\0';
    if (comm)
        comm[0] = '\0';

    cardlen = strlen(card);

    /* support for ESO HIERARCH keywords; find the '=' */
    if (FSTRNCMP(card, "HIERARCH ", 9) == 0)
    {
      valpos = strcspn(card, "=");

      if (valpos == cardlen)   /* no value indicator ??? */
      {
        if (comm != NULL)
        {
          if (cardlen > 8)
          {
            strcpy(comm, &card[8]);

            jj=cardlen - 8;
            for (jj--; jj >= 0; jj--)  /* replace trailing blanks with nulls */
            {
               if (comm[jj] == ' ')
                  comm[jj] = '\0';
               else
                  break;
            }
          }
        }
        return(*status);  /* no value indicator */
      }
      valpos++;  /* point to the position after the '=' */
    }
    else if (cardlen < 9  ||
        FSTRNCMP(card, "COMMENT ", 8) == 0 ||  /* keywords with no value */
        FSTRNCMP(card, "HISTORY ", 8) == 0 ||
        FSTRNCMP(card, "END     ", 8) == 0 ||
        FSTRNCMP(card, "CONTINUE", 8) == 0 ||
        FSTRNCMP(card, "        ", 8) == 0 )
    {
        /*  no value, so the comment extends from cols 9 - 80  */
        if (comm != NULL)
        {
          if (cardlen > 8)
          {
             strcpy(comm, &card[8]);

             jj=cardlen - 8;
             for (jj--; jj >= 0; jj--)  /* replace trailing blanks with nulls */
             {
               if (comm[jj] == ' ')
                  comm[jj] = '\0';
               else
                  break;
             }
          }
        }
        return(*status);
    }
    else if (FSTRNCMP(&card[8], "= ", 2) == 0  )
    {
        /* normal keyword with '= ' in cols 9-10 */
        valpos = 10;  /* starting position of the value field */
    }
    else
    {
      valpos = strcspn(card, "=");

      if (valpos == cardlen)   /* no value indicator ??? */
      {
        if (comm != NULL)
        {
          if (cardlen > 8)
          {
            strcpy(comm, &card[8]);

            jj=cardlen - 8;
            for (jj--; jj >= 0; jj--)  /* replace trailing blanks with nulls */
            {
               if (comm[jj] == ' ')
                  comm[jj] = '\0';
               else
                  break;
            }
          }
        }
        return(*status);  /* no value indicator */
      }
      valpos++;  /* point to the position after the '=' */
    }

    nblank = strspn(&card[valpos], " "); /* find number of leading blanks */

    if (nblank + valpos == cardlen)
    {
      /* the absence of a value string is legal, and simply indicates
         that the keyword value is undefined.  Don't write an error
         message in this case.
      */
        return(*status);
    }

    ii = valpos + nblank;

    if (card[ii] == '/' )  /* slash indicates start of the comment */
    {
         ii++;
    }
    else if (card[ii] == '\'' )  /* is this a quoted string value? */
    {
        value[0] = card[ii];
        for (jj=1, ii++; ii < cardlen; ii++, jj++)
        {
            if (card[ii] == '\'')  /*  is this the closing quote?  */
            {
                if (card[ii+1] == '\'')  /* 2 successive quotes? */ 
                {
                   value[jj] = card[ii];
                   ii++;  
                   jj++;
                }
                else
                {
                    value[jj] = card[ii];
                    break;   /* found the closing quote, so exit this loop  */
                }
            }
            value[jj] = card[ii];  /* copy the next character to the output */
        }

        if (ii == cardlen)
        {
            jj = minvalue(jj, 69);  /* don't exceed 70 char string length */
            value[jj] = '\'';  /*  close the bad value string  */
            value[jj+1] = '\0';  /*  terminate the bad value string  */
            ffpmsg("This keyword string value has no closing quote:");
            ffpmsg(card);
	    /*  May 2008 - modified to not fail on this minor error  */
/*            return(*status = NO_QUOTE);  */
        }
        else
        {
            value[jj+1] = '\0';  /*  terminate the good value string  */
            ii++;   /*  point to the character following the value  */
        }
    }
    else if (card[ii] == '(' )  /* is this a complex value? */
    {
        nblank = strcspn(&card[ii], ")" ); /* find closing ) */
        if (nblank == strlen( &card[ii] ) )
        {
            ffpmsg("This complex keyword value has no closing ')':");
            ffpmsg(card);
            return(*status = NO_QUOTE);
        }

        nblank++;
        strncpy(value, &card[ii], nblank);
        value[nblank] = '\0';
        ii = ii + nblank;        
    }
    else   /*  an integer, floating point, or logical FITS value string  */
    {
        nblank = strcspn(&card[ii], " /");  /* find the end of the token */
        strncpy(value, &card[ii], nblank);
        value[nblank] = '\0';
        ii = ii + nblank;
    }

    /*  now find the comment string, if any  */
    if (comm)
    {
      nblank = strspn(&card[ii], " ");  /*  find next non-space character  */
      ii = ii + nblank;

      if (ii < 80)
      {
        if (card[ii] == '/')   /*  ignore the slash separator  */
        {
            ii++;
            if (card[ii] == ' ')  /*  also ignore the following space  */
                ii++;
        }
        strcat(comm, &card[ii]);  /*  copy the remaining characters  */

        jj=strlen(comm);
        for (jj--; jj >= 0; jj--)  /* replace trailing blanks with nulls */
        {
            if (comm[jj] == ' ')
                comm[jj] = '\0';
            else
                break;
        }
      }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgthd(char *tmplt, /* I - input header template string */
           char *card,  /* O - returned FITS header record */
           int *hdtype, /* O - how to interpreter the returned card string */ 
            /*
              -2 = modify the name of a keyword; the old keyword name
                   is returned starting at address chars[0]; the new name
                   is returned starting at address char[40] (to be consistent
                   with the Fortran version).  Both names are null terminated. 
              -1 = card contains the name of a keyword that is to be deleted
               0 = append this keyword if it doesn't already exist, or 
                   modify the value if the keyword already exists.
               1 = append this comment keyword ('HISTORY', 
                   'COMMENT', or blank keyword name) 
               2  =  this is the END keyword; do not write it to the header
            */
           int *status)   /* IO - error status   */
/*
  'Get Template HeaDer'
  parse a template header line and create a formated
  character string which is suitable for appending to a FITS header 
*/
{
    char keyname[FLEN_KEYWORD], value[140], comment[140];
    char *tok, *suffix, *loc, tvalue[140];
    int len, vlen, more, tstatus;
    double dval;

    if (*status > 0)
        return(*status);

    card[0]   = '\0';
    *hdtype   = 0;

    if (!FSTRNCMP(tmplt, "        ", 8) )
    {
        /* if first 8 chars of template are blank, then this is a comment */
        strncat(card, tmplt, 80);
        *hdtype = 1;
        return(*status);
    }

    tok = tmplt;   /* point to start of template string */
 
    keyname[0] = '\0';
    value[0]   = '\0';
    comment[0] = '\0';

    len = strspn(tok, " ");  /* no. of spaces before keyword */
    tok += len;

    /* test for pecular case where token is a string of dashes */
    if (strncmp(tok, "--------------------", 20) == 0)
            return(*status = BAD_KEYCHAR);

    if (tok[0] == '-')  /* is there a leading minus sign? */
    {
        /* first token is name of keyword to be deleted or renamed */
        *hdtype = -1;
        tok++;
        len = strspn(tok, " ");  /* no. of spaces before keyword */
        tok += len;
        if (len < 8)  /* not a blank name? */
        {
          len = strcspn(tok, " =");  /* length of name */
          if (len >= FLEN_KEYWORD)
            return(*status = BAD_KEYCHAR);

          strncat(card, tok, len);

          /*
            The HIERARCH convention supports non-standard characters
            in the keyword name, so don't always convert to upper case or
            abort if there are illegal characters in the name or if the
            name is greater than 8 characters long.
          */

          if (len < 9)  /* this is possibly a normal FITS keyword name */
          {
            ffupch(card);
            tstatus = 0;
            if (fftkey(card, &tstatus) > 0)
            {
               /* name contained non-standard characters, so reset */
               card[0] = '\0';
               strncat(card, tok, len);
            }
          }

          tok += len;
        }

        /* second token, if present, is the new name for the keyword */

        len = strspn(tok, " ");  /* no. of spaces before next token */
        tok += len;

        if (tok[0] == '\0' || tok[0] == '=')
            return(*status);  /* no second token */

        *hdtype = -2;
        len = strcspn(tok, " ");  /* length of new name */
        if (len > 40)  /* name has to fit on columns 41-80 of card */
          return(*status = BAD_KEYCHAR);

        /* copy the new name to card + 40;  This is awkward, */
        /* but is consistent with the way the Fortran FITSIO works */
	strcat(card,"                                        ");
        strncpy(&card[40], tok, len+1); /* copy len+1 to get terminator */

        /*
            The HIERARCH convention supports non-standard characters
            in the keyword name, so don't always convert to upper case or
            abort if there are illegal characters in the name or if the
            name is greater than 8 characters long.
        */

        if (len < 9)  /* this is possibly a normal FITS keyword name */
        {
            ffupch(&card[40]);
            tstatus = 0;
            if (fftkey(&card[40], &tstatus) > 0)
            {
               /* name contained non-standard characters, so reset */
               strncpy(&card[40], tok, len);
            }
        }
    }
    else  /* no negative sign at beginning of template */
    {
      /* get the keyword name token */

      len = strcspn(tok, " =");  /* length of keyword name */
      if (len >= FLEN_KEYWORD)
        return(*status = BAD_KEYCHAR);

      strncat(keyname, tok, len);

      /*
        The HIERARCH convention supports non-standard characters
        in the keyword name, so don't always convert to upper case or
        abort if there are illegal characters in the name or if the
        name is greater than 8 characters long.
      */

      if (len < 9)  /* this is possibly a normal FITS keyword name */
      {
        ffupch(keyname);
        tstatus = 0;
        if (fftkey(keyname, &tstatus) > 0)
        {
           /* name contained non-standard characters, so reset */
           keyname[0] = '\0';
           strncat(keyname, tok, len);
        }
      }

      if (!FSTRCMP(keyname, "END") )
      {
         strcpy(card, "END");
         *hdtype = 2;
         return(*status);
      }

      tok += len; /* move token pointer to end of the keyword */

      if (!FSTRCMP(keyname, "COMMENT") || !FSTRCMP(keyname, "HISTORY")
         || !FSTRCMP(keyname, "HIERARCH") )
      {
        *hdtype = 1;   /* simply append COMMENT and HISTORY keywords */
        strcpy(card, keyname);
        strncat(card, tok, 73);
        return(*status);
      }

      /* look for the value token */
      len = strspn(tok, " =");  /* spaces or = between name and value */
      tok += len;

      if (*tok == '\'') /* is value enclosed in quotes? */
      {
          more = TRUE;
          while (more)
          {
            tok++;  /* temporarily move past the quote char */
            len = strcspn(tok, "'");  /* length of quoted string */
            tok--;
            strncat(value, tok, len + 2); 
 
            tok += len + 1;
            if (tok[0] != '\'')   /* check there is a closing quote */
              return(*status = NO_QUOTE);

            tok++;
            if (tok[0] != '\'')  /* 2 quote chars = literal quote */
              more = FALSE;
          }
      }
      else if (*tok == '/' || *tok == '\0')  /* There is no value */
      {
          strcat(value, " ");
      }
      else   /* not a quoted string value */
      {
          len = strcspn(tok, " /"); /* length of value string */

          strncat(value, tok, len);
          if (!( (tok[0] == 'T' || tok[0] == 'F') &&
                 (tok[1] == ' ' || tok[1] == '/' || tok[1] == '\0') )) 
          {
             /* not a logical value */

            dval = strtod(value, &suffix); /* try to read value as number */

            if (*suffix != '\0' && *suffix != ' ' && *suffix != '/')
            { 
                /* value not recognized as a number; might be because it */
                /* contains a 'd' or 'D' exponent character  */ 
                strcpy(tvalue, value);
                if ((loc = strchr(tvalue, 'D')))
                {          
                    *loc = 'E'; /*  replace D's with E's. */ 
                    dval = strtod(tvalue, &suffix); /* read value again */
                }
                else if ((loc = strchr(tvalue, 'd')))
                {
                    *loc = 'E'; /*  replace d's with E's. */ 
                    dval = strtod(tvalue, &suffix); /* read value again */
                }
                else if ((loc = strchr(tvalue, '.')))
                {
                    *loc = ','; /*  replace period with a comma */ 
                    dval = strtod(tvalue, &suffix); /* read value again */
                }
            }
   
            if (*suffix != '\0' && *suffix != ' ' && *suffix != '/')
            { 
              /* value is not a number; must enclose it in quotes */
              strcpy(value, "'");
              strncat(value, tok, len);
              strcat(value, "'");

              /* the following useless statement stops the compiler warning */
              /* that dval is not used anywhere */
              if (dval == 0.)
                 len += (int) dval; 
            }
            else  
            {
                /* value is a number; convert any 'e' to 'E', or 'd' to 'D' */
                loc = strchr(value, 'e');
                if (loc)
                {          
                    *loc = 'E';  
                }
                else
                {
                    loc = strchr(value, 'd');
                    if (loc)
                    {          
                        *loc = 'D';  
                    }
                }
            }
          }
          tok += len;
      }

      len = strspn(tok, " /"); /* no. of spaces between value and comment */
      tok += len;

      vlen = strlen(value);
      if (vlen > 0 && vlen < 10 && value[0] == '\'')
      {
          /* pad quoted string with blanks so it is at least 8 chars long */
          value[vlen-1] = '\0';
          strncat(value, "        ", 10 - vlen);
          strcat(&value[9], "'");
      }

      /* get the comment string */
      strncat(comment, tok, 70);

      /* construct the complete FITS header card */
      ffmkky(keyname, value, comment, card, status);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_translate_keyword(
      char *inrec,        /* I - input string */
      char *outrec,       /* O - output converted string, or */
                          /*     a null string if input does not  */
                          /*     match any of the patterns */
      char *patterns[][2],/* I - pointer to input / output string */
                          /*     templates */
      int npat,           /* I - number of templates passed */
      int n_value,        /* I - base 'n' template value of interest */
      int n_offset,       /* I - offset to be applied to the 'n' */
                          /*     value in the output string */
      int n_range,        /* I - controls range of 'n' template */
                          /*     values of interest (-1,0, or +1) */
      int *pat_num,       /* O - matched pattern number (0 based) or -1 */
      int *i,             /* O - value of i, if any, else 0 */
      int *j,             /* O - value of j, if any, else 0 */
      int *m,             /* O - value of m, if any, else 0 */
      int *n,             /* O - value of n, if any, else 0 */

      int *status)        /* IO - error status */

/* 

Translate a keyword name to a new name, based on a set of patterns.
The user passes an array of patterns to be matched.  Input pattern
number i is pattern[i][0], and output pattern number i is
pattern[i][1].  Keywords are matched against the input patterns.  If a
match is found then the keyword is re-written according to the output
pattern.

Order is important.  The first match is accepted.  The fastest match
will be made when templates with the same first character are grouped
together.

Several characters have special meanings:

     i,j - single digits, preserved in output template
     n - column number of one or more digits, preserved in output template
     m - generic number of one or more digits, preserved in output template
     a - coordinate designator, preserved in output template
     # - number of one or more digits
     ? - any character
     * - only allowed in first character position, to match all
         keywords; only useful as last pattern in the list

i, j, n, and m are returned by the routine.

For example, the input pattern "iCTYPn" will match "1CTYP5" (if n_value
is 5); the output pattern "CTYPEi" will be re-written as "CTYPE1".
Notice that "i" is preserved.

The following output patterns are special

Special output pattern characters:

    "-" - do not copy a keyword that matches the corresponding input pattern

    "+" - copy the input unchanged

The inrec string could be just the 8-char keyword name, or the entire 
80-char header record.  Characters 9 = 80 in the input string simply get
appended to the translated keyword name.

If n_range = 0, then only keywords with 'n' equal to n_value will be 
considered as a pattern match.  If n_range = +1, then all values of 
'n' greater than or equal to n_value will be a match, and if -1, 
then values of 'n' less than or equal to n_value will match.

  This routine was written by Craig Markwardt, GSFC
*/

{
    int i1 = 0, j1 = 0, n1 = 0, m1 = 0;
    int fac;
    char a = ' ';
    char oldp;
    char c, s;
    int ip, ic, pat, pass = 0, firstfail;
    char *spat;

    if (*status > 0)
        return(*status);
    if ((inrec == 0) || (outrec == 0)) 
      return (*status = NULL_INPUT_PTR);

    *outrec = '\0';
/*
    if (*inrec == '\0') return 0;
*/

    if (*inrec == '\0')    /* expand to full 8 char blank keyword name */
       strcpy(inrec, "        ");
       
    oldp = '\0';
    firstfail = 0;

    /* ===== Pattern match stage */
    for (pat=0; pat < npat; pat++) {
      spat = patterns[pat][0];
      
      i1 = 0; j1 = 0; m1 = -1; n1 = -1; a = ' ';  /* Initialize the place-holders */
      pass = 0;
      
      /* Pass the wildcard pattern */
      if (spat[0] == '*') { 
	pass = 1;
	break;
      }
      
      /* Optimization: if we have seen this initial pattern character before,
	 then it must have failed, and we can skip the pattern */
      if (firstfail && spat[0] == oldp) continue;
      oldp = spat[0];

      /* 
	 ip = index of pattern character being matched
	 ic = index of keyname character being matched
	 firstfail = 1 if we fail on the first characteor (0=not)
      */
      
      for (ip=0, ic=0, firstfail=1;
	   (spat[ip]) && (ic < 8);
	   ip++, ic++, firstfail=0) {
	c = inrec[ic];
	s = spat[ip];

	if (s == 'i') {
	  /* Special pattern: 'i' placeholder */
	  if (isdigit(c)) { i1 = c - '0'; pass = 1;}
	} else if (s == 'j') {
	  /* Special pattern: 'j' placeholder */
	  if (isdigit(c)) { j1 = c - '0'; pass = 1;}
	} else if ((s == 'n')||(s == 'm')||(s == '#')) {
	  /* Special patterns: multi-digit number */
	  int val = 0;
	  pass = 0;
	  if (isdigit(c)) {
	    pass = 1;  /* NOTE, could fail below */
	    
	    /* Parse decimal number */
	    while (ic<8 && isdigit(c)) { 
	      val = val*10 + (c - '0');
	      ic++; c = inrec[ic];
	    }
	    ic--; c = inrec[ic];
	    
	    if (s == 'n') { 
	      
	      /* Is it a column number? */
	      if ( val >= 1 && val <= 999 &&                    /* Row range check */
		   (((n_range == 0) && (val == n_value)) ||     /* Strict equality */
		    ((n_range == -1) && (val <= n_value)) ||    /* n <= n_value */
		    ((n_range == +1) && (val >= n_value))) ) {  /* n >= n_value */
		n1 = val;
	      } else {
		pass = 0;
	      }
	    } else if (s == 'm') {
	      
	      /* Generic number */
	      m1 = val; 
	    }
	  }
	} else if (s == 'a') {
	  /* Special pattern: coordinate designator */
	  if (isupper(c) || c == ' ') { a = c; pass = 1;} 
	} else if (s == '?') {
	  /* Match any individual character */
	  pass = 1;
	} else if (c == s) {
	  /* Match a specific character */
	  pass = 1;
	} else {
	  /* FAIL */
	  pass = 0;
	}
	if (!pass) break;
      }
      
      /* Must pass to the end of the keyword.  No partial matches allowed */
      if (pass && (ic >= 8 || inrec[ic] == ' ')) break;
    }

    /* Transfer the pattern-matched numbers to the output parameters */
    if (i) { *i = i1; }
    if (j) { *j = j1; }
    if (n) { *n = n1; }
    if (m) { *m = m1; }
    if (pat_num) { *pat_num = pat; }

    /* ===== Keyword rewriting and output stage */
    spat = patterns[pat][1];

    /* Return case: no match, or explicit deletion pattern */
    if (pass == 0 || spat[0] == '\0' || spat[0] == '-') return 0;

    /* A match: we start by copying the input record to the output */
    strcpy(outrec, inrec);

    /* Return case: return the input record unchanged */
    if (spat[0] == '+') return 0;


    /* Final case: a new output pattern */
    for (ip=0, ic=0; spat[ip]; ip++, ic++) {
      s = spat[ip];
      if (s == 'i') {
	outrec[ic] = (i1+'0');
      } else if (s == 'j') {
	outrec[ic] = (j1+'0');
      } else if (s == 'n') {
	if (n1 == -1) { n1 = n_value; }
	if (n1 > 0) {
	  n1 += n_offset;
	  for (fac = 1; (n1/fac) > 0; fac *= 10);
	  fac /= 10;
	  while(fac > 0) {
	    outrec[ic] = ((n1/fac) % 10) + '0';
	    fac /= 10;
	    ic ++;
	  }
	  ic--;
	}
      } else if (s == 'm' && m1 >= 0) {
	for (fac = 1; (m1/fac) > 0; fac *= 10);
	fac /= 10;
	while(fac > 0) {
	  outrec[ic] = ((m1/fac) % 10) + '0';
	  fac /= 10;
	  ic ++;
	}
	ic --;
      } else if (s == 'a') {
	outrec[ic] = a;
      } else {
	outrec[ic] = s;
      }
    }

    /* Pad the keyword name with spaces */
    for ( ; ic<8; ic++) { outrec[ic] = ' '; }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_translate_keywords(
	   fitsfile *infptr,   /* I - pointer to input HDU */
	   fitsfile *outfptr,  /* I - pointer to output HDU */
	   int firstkey,       /* I - first HDU record number to start with */
	   char *patterns[][2],/* I - pointer to input / output keyword templates */
	   int npat,           /* I - number of templates passed */
	   int n_value,        /* I - base 'n' template value of interest */
	   int n_offset,       /* I - offset to be applied to the 'n' */
 	                       /*     value in the output string */
	   int n_range,        /* I - controls range of 'n' template */
	                       /*     values of interest (-1,0, or +1) */
           int *status)        /* IO - error status */
/*
     Copy relevant keywords from the table header into the newly
     created primary array header.  Convert names of keywords where
     appropriate.  See fits_translate_keyword() for the definitions.

     Translation begins at header record number 'firstkey', and
     continues to the end of the header.

  This routine was written by Craig Markwardt, GSFC
*/
{
    int nrec, nkeys, nmore;
    char rec[FLEN_CARD];
    int i = 0, j = 0, n = 0, m = 0;
    int pat_num = 0, maxchr, ii;
    char outrec[FLEN_CARD];

    if (*status > 0)
        return(*status);

    ffghsp(infptr, &nkeys, &nmore, status);  /* get number of keywords */

    for (nrec = firstkey; nrec <= nkeys; nrec++) {
      outrec[0] = '\0';

      ffgrec(infptr, nrec, rec, status);

      /* silently overlook any illegal ASCII characters in the value or */
      /* comment fields of the record. It is usually not appropriate to */
      /* abort the process because of this minor transgression of the FITS rules. */
      /* Set the offending character to a blank */

      maxchr = strlen(rec);
      for (ii = 8; ii < maxchr; ii++)
      {
        if (rec[ii] < 32 || rec[ii] > 126)
          rec[ii] = ' ';
      }
      
      fits_translate_keyword(rec, outrec, patterns, npat, 
			     n_value, n_offset, n_range, 
			     &pat_num, &i, &j, &m, &n, status);
      
      if (outrec[0]) {
	ffprec(outfptr, outrec, status); /* copy the keyword */
	rec[8] = 0; outrec[8] = 0;
      } else {
	rec[8] = 0; outrec[8] = 0;
      }
    }	

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_copy_pixlist2image(
	   fitsfile *infptr,   /* I - pointer to input HDU */
	   fitsfile *outfptr,  /* I - pointer to output HDU */
	   int firstkey,       /* I - first HDU record number to start with */
           int naxis,          /* I - number of axes in the image */
           int *colnum,       /* I - numbers of the columns to be binned  */
           int *status)        /* IO - error status */
/*
     Copy relevant keywords from the pixel list table header into a newly
     created primary array header.  Convert names of keywords where
     appropriate.  See fits_translate_pixkeyword() for the definitions.

     Translation begins at header record number 'firstkey', and
     continues to the end of the header.
*/
{
    int nrec, nkeys, nmore;
    char rec[FLEN_CARD], outrec[FLEN_CARD];
    int pat_num = 0, npat;
    int iret, jret, nret, mret, lret;
    char *patterns[][2] = {

			   {"TCTYPn",  "CTYPEn"    },
			   {"TCTYna",  "CTYPEna"   },
			   {"TCUNIn",  "CUNITn"    },
			   {"TCUNna",  "CUNITna"   },
			   {"TCRVLn",  "CRVALn"    },
			   {"TCRVna",  "CRVALna"   },
			   {"TCDLTn",  "CDELTn"    },
			   {"TCDEna",  "CDELTna"   },
			   {"TCRPXn",  "CRPIXn"    },
			   {"TCRPna",  "CRPIXna"   },
			   {"TCROTn",  "CROTAn"    },
			   {"TPn_ma",  "PCn_ma"    },
			   {"TPCn_m",  "PCn_ma"    },
			   {"TCn_ma",  "CDn_ma"    },
			   {"TCDn_m",  "CDn_ma"    },
			   {"TVn_la",  "PVn_la"    },
			   {"TPVn_l",  "PVn_la"    },
			   {"TSn_la",  "PSn_la"    },
			   {"TPSn_l",  "PSn_la"    },
			   {"TWCSna",  "WCSNAMEa"  },
			   {"TCNAna",  "CNAMEna"   },
			   {"TCRDna",  "CRDERna"   },
			   {"TCSYna",  "CSYERna"   },
			   {"LONPna",  "LONPOLEa"  },
			   {"LATPna",  "LATPOLEa"  },
			   {"EQUIna",  "EQUINOXa"  },
			   {"MJDOBn",  "MJD-OBS"   },
			   {"MJDAn",   "MJD-AVG"   },
			   {"DAVGn",   "DATE-AVG"  },
			   {"RADEna",  "RADESYSa"  },
			   {"RFRQna",  "RESTFRQa"  },
			   {"RWAVna",  "RESTWAVa"  },
			   {"SPECna",  "SPECSYSa"  },
			   {"SOBSna",  "SSYSOBSa"  },
			   {"SSRCna",  "SSYSSRCa"  },

                           /* preserve common keywords */
			   {"LONPOLEa",   "+"       },
			   {"LATPOLEa",   "+"       },
			   {"EQUINOXa",   "+"       },
			   {"EPOCH",      "+"       },
			   {"MJD-????",   "+"       },
			   {"DATE????",   "+"       },
			   {"TIME????",   "+"       },
			   {"RADESYSa",   "+"       },
			   {"RADECSYS",   "+"       },
			   {"TELESCOP",   "+"       },
			   {"INSTRUME",   "+"       },
			   {"OBSERVER",   "+"       },
			   {"OBJECT",     "+"       },

                           /* Delete general table column keywords */
			   {"XTENSION", "-"       },
			   {"BITPIX",   "-"       },
			   {"NAXIS",    "-"       },
			   {"NAXISi",   "-"       },
			   {"PCOUNT",   "-"       },
			   {"GCOUNT",   "-"       },
			   {"TFIELDS",  "-"       },

			   {"TDIM#",   "-"       },
			   {"THEAP",   "-"       },
			   {"EXTNAME", "-"       }, 
			   {"EXTVER",  "-"       },
			   {"EXTLEVEL","-"       },
			   {"CHECKSUM","-"       },
			   {"DATASUM", "-"       },
			   {"NAXLEN",  "-"       },
			   {"AXLEN#",  "-"       },
			   {"CPREF",  "-"       },
			   
                           /* Delete table keywords related to other columns */
			   {"T????#a", "-"       }, 
 			   {"TC??#a",  "-"       },
 			   {"T??#_#",  "-"       },
 			   {"TWCS#a",  "-"       },

			   {"LONP#a",  "-"       },
			   {"LATP#a",  "-"       },
			   {"EQUI#a",  "-"       },
			   {"MJDOB#",  "-"       },
			   {"MJDA#",   "-"       },
			   {"RADE#a",  "-"       },
			   {"DAVG#",   "-"       },

			   {"iCTYP#",  "-"       },
			   {"iCTY#a",  "-"       },
			   {"iCUNI#",  "-"       },
			   {"iCUN#a",  "-"       },
			   {"iCRVL#",  "-"       },
			   {"iCDLT#",  "-"       },
			   {"iCRPX#",  "-"       },
			   {"iCTY#a",  "-"       },
			   {"iCUN#a",  "-"       },
			   {"iCRV#a",  "-"       },
			   {"iCDE#a",  "-"       },
			   {"iCRP#a",  "-"       },
			   {"ijPC#a",  "-"       },
			   {"ijCD#a",  "-"       },
			   {"iV#_#a",  "-"       },
			   {"iS#_#a",  "-"       },
			   {"iCRD#a",  "-"       },
			   {"iCSY#a",  "-"       },
			   {"iCROT#",  "-"       },
			   {"WCAX#a",  "-"       },
			   {"WCSN#a",  "-"       },
			   {"iCNA#a",  "-"       },

			   {"*",       "+"       }}; /* copy all other keywords */

    if (*status > 0)
        return(*status);

    npat = sizeof(patterns)/sizeof(patterns[0][0])/2;

    ffghsp(infptr, &nkeys, &nmore, status);  /* get number of keywords */

    for (nrec = firstkey; nrec <= nkeys; nrec++) {
      outrec[0] = '\0';

      ffgrec(infptr, nrec, rec, status);

      fits_translate_pixkeyword(rec, outrec, patterns, npat, 
			     naxis, colnum, 
			     &pat_num, &iret, &jret, &nret, &mret, &lret, status);

      if (outrec[0]) {
	ffprec(outfptr, outrec, status); /* copy the keyword */
      } 

      rec[8] = 0; outrec[8] = 0;
    }	

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_translate_pixkeyword(
      char *inrec,        /* I - input string */
      char *outrec,       /* O - output converted string, or */
                          /*     a null string if input does not  */
                          /*     match any of the patterns */
      char *patterns[][2],/* I - pointer to input / output string */
                          /*     templates */
      int npat,           /* I - number of templates passed */
      int naxis,          /* I - number of columns to be binned */
      int *colnum,       /* I - numbers of the columns to be binned */
      int *pat_num,       /* O - matched pattern number (0 based) or -1 */
      int *i,
      int *j,
      int *n,
      int *m,
      int *l,
      int *status)        /* IO - error status */
      
/* 

Translate a keyword name to a new name, based on a set of patterns.
The user passes an array of patterns to be matched.  Input pattern
number i is pattern[i][0], and output pattern number i is
pattern[i][1].  Keywords are matched against the input patterns.  If a
match is found then the keyword is re-written according to the output
pattern.

Order is important.  The first match is accepted.  The fastest match
will be made when templates with the same first character are grouped
together.

Several characters have special meanings:

     i,j - single digits, preserved in output template
     n, m - column number of one or more digits, preserved in output template
     k - generic number of one or more digits, preserved in output template
     a - coordinate designator, preserved in output template
     # - number of one or more digits
     ? - any character
     * - only allowed in first character position, to match all
         keywords; only useful as last pattern in the list

i, j, n, and m are returned by the routine.

For example, the input pattern "iCTYPn" will match "1CTYP5" (if n_value
is 5); the output pattern "CTYPEi" will be re-written as "CTYPE1".
Notice that "i" is preserved.

The following output patterns are special

Special output pattern characters:

    "-" - do not copy a keyword that matches the corresponding input pattern

    "+" - copy the input unchanged

The inrec string could be just the 8-char keyword name, or the entire 
80-char header record.  Characters 9 = 80 in the input string simply get
appended to the translated keyword name.

If n_range = 0, then only keywords with 'n' equal to n_value will be 
considered as a pattern match.  If n_range = +1, then all values of 
'n' greater than or equal to n_value will be a match, and if -1, 
then values of 'n' less than or equal to n_value will match.

*/

{
    int i1 = 0, j1 = 0, val;
    int fac, nval = 0, mval = 0, lval = 0;
    char a = ' ';
    char oldp;
    char c, s;
    int ip, ic, pat, pass = 0, firstfail;
    char *spat;

    if (*status > 0)
        return(*status);

    if ((inrec == 0) || (outrec == 0)) 
      return (*status = NULL_INPUT_PTR);

    *outrec = '\0';
    if (*inrec == '\0') return 0;

    oldp = '\0';
    firstfail = 0;

    /* ===== Pattern match stage */
    for (pat=0; pat < npat; pat++) {

      spat = patterns[pat][0];
      
      i1 = 0; j1 = 0;   a = ' ';  /* Initialize the place-holders */
      pass = 0;
      
      /* Pass the wildcard pattern */
      if (spat[0] == '*') { 
	pass = 1;
	break;
      }
      
      /* Optimization: if we have seen this initial pattern character before,
	 then it must have failed, and we can skip the pattern */
      if (firstfail && spat[0] == oldp) continue;
      oldp = spat[0];

      /* 
	 ip = index of pattern character being matched
	 ic = index of keyname character being matched
	 firstfail = 1 if we fail on the first characteor (0=not)
      */
      
      for (ip=0, ic=0, firstfail=1;
	   (spat[ip]) && (ic < 8);
	   ip++, ic++, firstfail=0) {
	c = inrec[ic];
	s = spat[ip];

	if (s == 'i') {
	  /* Special pattern: 'i' placeholder */
	  if (isdigit(c)) { i1 = c - '0'; pass = 1;}
	} else if (s == 'j') {
	  /* Special pattern: 'j' placeholder */
	  if (isdigit(c)) { j1 = c - '0'; pass = 1;}
	} else if ((s == 'n')||(s == 'm')||(s == 'l')||(s == '#')) {
	  /* Special patterns: multi-digit number */
          val = 0;
	  pass = 0;
	  if (isdigit(c)) {
	    pass = 1;  /* NOTE, could fail below */
	    
	    /* Parse decimal number */
	    while (ic<8 && isdigit(c)) { 
	      val = val*10 + (c - '0');
	      ic++; c = inrec[ic];
	    }
	    ic--; c = inrec[ic];

	    if (s == 'n' || s == 'm') { 
	      
	      /* Is it a column number? */
	      if ( val >= 1 && val <= 999) {
	         
		 if (val == colnum[0])
		     val = 1; 
		 else if (val == colnum[1]) 
		     val = 2; 
		 else if (val == colnum[2]) 
		     val = 3; 
		 else if (val == colnum[3]) 
		     val = 4; 
		 else {
		     pass = 0;
		     val = 0; 
		 }

	         if (s == 'n')
		    nval = val;
		 else
		    mval = val;
 
              } else {
		  pass = 0;
              }
	    } else if (s == 'l') {
	      /* Generic number */
	      lval = val; 
	    }
	  }
	} else if (s == 'a') {
	  /* Special pattern: coordinate designator */
	  if (isupper(c) || c == ' ') { a = c; pass = 1;} 
	} else if (s == '?') {
	  /* Match any individual character */
	  pass = 1;
	} else if (c == s) {
	  /* Match a specific character */
	  pass = 1;
	} else {
	  /* FAIL */
	  pass = 0;
	}
	
	if (!pass) break;
      }
      

      /* Must pass to the end of the keyword.  No partial matches allowed */
      if (pass && (ic >= 8 || inrec[ic] == ' ')) break;
    }


    /* Transfer the pattern-matched numbers to the output parameters */
    if (i) { *i = i1; }
    if (j) { *j = j1; }
    if (n) { *n = nval; }
    if (m) { *m = mval; }
    if (l) { *l = lval; }
    if (pat_num) { *pat_num = pat; }

    /* ===== Keyword rewriting and output stage */
    spat = patterns[pat][1];

    /* Return case: no match, or explicit deletion pattern */
    if (pass == 0 || spat[0] == '\0' || spat[0] == '-') return 0;

    /* A match: we start by copying the input record to the output */
    strcpy(outrec, inrec);

    /* Return case: return the input record unchanged */
    if (spat[0] == '+') return 0;

    /* Final case: a new output pattern */
    for (ip=0, ic=0; spat[ip]; ip++, ic++) {
      s = spat[ip];
      if (s == 'i') {
	outrec[ic] = (i1+'0');
      } else if (s == 'j') {
	outrec[ic] = (j1+'0');
      } else if (s == 'n' && nval > 0) {
	  for (fac = 1; (nval/fac) > 0; fac *= 10);
	  fac /= 10;
	  while(fac > 0) {
	    outrec[ic] = ((nval/fac) % 10) + '0';
	    fac /= 10;
	    ic ++;
	  }
	  ic--;
      } else if (s == 'm' && mval > 0) {
	  for (fac = 1; (mval/fac) > 0; fac *= 10);
	  fac /= 10;
	  while(fac > 0) {
	    outrec[ic] = ((mval/fac) % 10) + '0';
	    fac /= 10;
	    ic ++;
	  }
	  ic--;
      } else if (s == 'l' && lval >= 0) {
	for (fac = 1; (lval/fac) > 0; fac *= 10);
	fac /= 10;
	while(fac > 0) {
	  outrec[ic] = ((lval/fac) % 10) + '0';
	  fac /= 10;
	  ic ++;
	}
	ic --;
      } else if (s == 'a') {
	outrec[ic] = a;
      } else {
	outrec[ic] = s;
      }
    }

    /* Pad the keyword name with spaces */
    for ( ; ic<8; ic++) { outrec[ic] = ' '; }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffasfm(char *tform,    /* I - format code from the TFORMn keyword */
           int *dtcode,    /* O - numerical datatype code */
           long *twidth,   /* O - width of the field, in chars */
           int *decimals,  /* O - number of decimal places (F, E, D format) */
           int *status)    /* IO - error status      */
{
/*
  parse the ASCII table TFORM column format to determine the data
  type, the field width, and number of decimal places (if relevant)
*/
    int ii, datacode;
    long longval, width;
    float fwidth;
    char *form, temp[FLEN_VALUE], message[FLEN_ERRMSG];

    if (*status > 0)
        return(*status);

    if (dtcode)
        *dtcode = 0;

    if (twidth)
        *twidth = 0;

    if (decimals)
        *decimals = 0;

    ii = 0;
    while (tform[ii] != 0 && tform[ii] == ' ') /* find first non-blank char */
         ii++;

    strcpy(temp, &tform[ii]); /* copy format string */
    ffupch(temp);     /* make sure it is in upper case */
    form = temp;      /* point to start of format string */


    if (form[0] == 0)
    {
        ffpmsg("Error: ASCII table TFORM code is blank");
        return(*status = BAD_TFORM);
    }

    /*-----------------------------------------------*/
    /*       determine default datatype code         */
    /*-----------------------------------------------*/
    if (form[0] == 'A')
        datacode = TSTRING;
    else if (form[0] == 'I')
        datacode = TLONG;
    else if (form[0] == 'F')
        datacode = TFLOAT;
    else if (form[0] == 'E')
        datacode = TFLOAT;
    else if (form[0] == 'D')
        datacode = TDOUBLE;
    else
    {
        sprintf(message,
                "Illegal ASCII table TFORMn datatype: \'%s\'", tform);
        ffpmsg(message);
        return(*status = BAD_TFORM_DTYPE);
    }

    if (dtcode)
       *dtcode = datacode;

    form++;  /* point to the start of field width */

    if (datacode == TSTRING || datacode == TLONG)
    { 
        /*-----------------------------------------------*/
        /*              A or I data formats:             */
        /*-----------------------------------------------*/

        if (ffc2ii(form, &width, status) <= 0)  /* read the width field */
        {
            if (width <= 0)
            {
                width = 0;
                *status = BAD_TFORM;
            }
            else
            {                
                /* set to shorter precision if I4 or less */
                if (width <= 4 && datacode == TLONG)
                    datacode = TSHORT;
            }
        }
    }
    else
    {  
        /*-----------------------------------------------*/
        /*              F, E or D data formats:          */
        /*-----------------------------------------------*/

        if (ffc2rr(form, &fwidth, status) <= 0) /* read ww.dd width field */
        {
           if (fwidth <= 0.)
            *status = BAD_TFORM;
          else
          {
            width = (long) fwidth;  /* convert from float to long */

            if (width > 7 && *temp == 'F')
                datacode = TDOUBLE;  /* type double if >7 digits */

            if (width < 10)
                form = form + 1; /* skip 1 digit  */
            else
                form = form + 2; /* skip 2 digits */

            if (form[0] == '.') /* should be a decimal point here */
            {
                form++;  /*  point to start of decimals field */

                if (ffc2ii(form, &longval, status) <= 0) /* read decimals */
                {
                    if (decimals)
                        *decimals = longval;  /* long to short convertion */

                    if (longval >= width)  /* width < no. of decimals */
                        *status = BAD_TFORM; 

                    if (longval > 6 && *temp == 'E')
                        datacode = TDOUBLE;  /* type double if >6 digits */
                }
            }

          }
        }
    }
    if (*status > 0)
    {
        *status = BAD_TFORM;
        sprintf(message,"Illegal ASCII table TFORMn code: \'%s\'", tform);
        ffpmsg(message);
    }

    if (dtcode)
       *dtcode = datacode;

    if (twidth)
       *twidth = width;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffbnfm(char *tform,     /* I - format code from the TFORMn keyword */
           int *dtcode,   /* O - numerical datatype code */
           long *trepeat,    /* O - repeat count of the field  */
           long *twidth,     /* O - width of the field, in chars */
           int *status)     /* IO - error status      */
{
/*
  parse the binary table TFORM column format to determine the data
  type, repeat count, and the field width (if it is an ASCII (A) field)
*/
    size_t ii, nchar;
    int datacode, variable, iread;
    long width, repeat;
    char *form, temp[FLEN_VALUE], message[FLEN_ERRMSG];

    if (*status > 0)
        return(*status);

    if (dtcode)
        *dtcode = 0;

    if (trepeat)
        *trepeat = 0;

    if (twidth)
        *twidth = 0;

    nchar = strlen(tform);

    for (ii = 0; ii < nchar; ii++)
    {
        if (tform[ii] != ' ')     /* find first non-space char */
            break;
    }

    if (ii == nchar)
    {
        ffpmsg("Error: binary table TFORM code is blank (ffbnfm).");
        return(*status = BAD_TFORM);
    }

    strcpy(temp, &tform[ii]); /* copy format string */
    ffupch(temp);     /* make sure it is in upper case */
    form = temp;      /* point to start of format string */

    /*-----------------------------------------------*/
    /*       get the repeat count                    */
    /*-----------------------------------------------*/

    ii = 0;
    while(isdigit((int) form[ii]))
        ii++;   /* look for leading digits in the field */

    if (ii == 0)
        repeat = 1;  /* no explicit repeat count */
    else
        sscanf(form,"%ld", &repeat);  /* read repeat count */

    /*-----------------------------------------------*/
    /*             determine datatype code           */
    /*-----------------------------------------------*/

    form = form + ii;  /* skip over the repeat field */

    if (form[0] == 'P' || form[0] == 'Q')
    {
        variable = 1;  /* this is a variable length column */
/*        repeat = 1;  */ /* disregard any other repeat value */
        form++;        /* move to the next data type code char */
    }
    else
        variable = 0;

    if (form[0] == 'U')  /* internal code to signify unsigned integer */
    { 
        datacode = TUSHORT;
        width = 2;
    }
    else if (form[0] == 'I')
    {
        datacode = TSHORT;
        width = 2;
    }
    else if (form[0] == 'V') /* internal code to signify unsigned integer */
    {
        datacode = TULONG;
        width = 4;
    }
    else if (form[0] == 'J')
    {
        datacode = TLONG;
        width = 4;
    }
    else if (form[0] == 'K')
    {
        datacode = TLONGLONG;
        width = 8;
    }
    else if (form[0] == 'E')
    {
        datacode = TFLOAT;
        width = 4;
    }
    else if (form[0] == 'D')
    {
        datacode = TDOUBLE;
        width = 8;
    }
    else if (form[0] == 'A')
    {
        datacode = TSTRING;

        /*
          the following code is used to support the non-standard
          datatype of the form rAw where r = total width of the field
          and w = width of fixed-length substrings within the field.
        */
        iread = 0;
        if (form[1] != 0)
        {
            if (form[1] == '(' )  /* skip parenthesis around */
                form++;          /* variable length column width */

            iread = sscanf(&form[1],"%ld", &width);
        }

        if (iread != 1 || (!variable && (width > repeat)) )
            width = repeat;
  
    }
    else if (form[0] == 'L')
    {
        datacode = TLOGICAL;
        width = 1;
    }
    else if (form[0] == 'X')
    {
        datacode = TBIT;
        width = 1;
    }
    else if (form[0] == 'B')
    {
        datacode = TBYTE;
        width = 1;
    }
    else if (form[0] == 'S') /* internal code to signify signed byte */
    {
        datacode = TSBYTE;
        width = 1;
    }
    else if (form[0] == 'C')
    {
        datacode = TCOMPLEX;
        width = 8;
    }
    else if (form[0] == 'M')
    {
        datacode = TDBLCOMPLEX;
        width = 16;
    }
    else
    {
        sprintf(message,
        "Illegal binary table TFORMn datatype: \'%s\' ", tform);
        ffpmsg(message);
        return(*status = BAD_TFORM_DTYPE);
    }

    if (variable)
        datacode = datacode * (-1); /* flag variable cols w/ neg type code */

    if (dtcode)
       *dtcode = datacode;

    if (trepeat)
       *trepeat = repeat;

    if (twidth)
       *twidth = width;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffbnfmll(char *tform,     /* I - format code from the TFORMn keyword */
           int *dtcode,   /* O - numerical datatype code */
           LONGLONG *trepeat,    /* O - repeat count of the field  */
           long *twidth,     /* O - width of the field, in chars */
           int *status)     /* IO - error status      */
{
/*
  parse the binary table TFORM column format to determine the data
  type, repeat count, and the field width (if it is an ASCII (A) field)
*/
    size_t ii, nchar;
    int datacode, variable, iread;
    long width;
    LONGLONG repeat;
    char *form, temp[FLEN_VALUE], message[FLEN_ERRMSG];
    double drepeat;

    if (*status > 0)
        return(*status);

    if (dtcode)
        *dtcode = 0;

    if (trepeat)
        *trepeat = 0;

    if (twidth)
        *twidth = 0;

    nchar = strlen(tform);

    for (ii = 0; ii < nchar; ii++)
    {
        if (tform[ii] != ' ')     /* find first non-space char */
            break;
    }

    if (ii == nchar)
    {
        ffpmsg("Error: binary table TFORM code is blank (ffbnfmll).");
        return(*status = BAD_TFORM);
    }

    strcpy(temp, &tform[ii]); /* copy format string */
    ffupch(temp);     /* make sure it is in upper case */
    form = temp;      /* point to start of format string */

    /*-----------------------------------------------*/
    /*       get the repeat count                    */
    /*-----------------------------------------------*/

    ii = 0;
    while(isdigit((int) form[ii]))
        ii++;   /* look for leading digits in the field */

    if (ii == 0)
        repeat = 1;  /* no explicit repeat count */
    else {
       /* read repeat count */

        /* print as double, because the string-to-64-bit int conversion */
        /* character is platform dependent (%lld, %ld, %I64d)           */

        sscanf(form,"%lf", &drepeat);
        repeat = (LONGLONG) (drepeat + 0.1);
    }
    /*-----------------------------------------------*/
    /*             determine datatype code           */
    /*-----------------------------------------------*/

    form = form + ii;  /* skip over the repeat field */

    if (form[0] == 'P' || form[0] == 'Q')
    {
        variable = 1;  /* this is a variable length column */
/*        repeat = 1;  */  /* disregard any other repeat value */
        form++;        /* move to the next data type code char */
    }
    else
        variable = 0;

    if (form[0] == 'U')  /* internal code to signify unsigned integer */
    { 
        datacode = TUSHORT;
        width = 2;
    }
    else if (form[0] == 'I')
    {
        datacode = TSHORT;
        width = 2;
    }
    else if (form[0] == 'V') /* internal code to signify unsigned integer */
    {
        datacode = TULONG;
        width = 4;
    }
    else if (form[0] == 'J')
    {
        datacode = TLONG;
        width = 4;
    }
    else if (form[0] == 'K')
    {
        datacode = TLONGLONG;
        width = 8;
    }
    else if (form[0] == 'E')
    {
        datacode = TFLOAT;
        width = 4;
    }
    else if (form[0] == 'D')
    {
        datacode = TDOUBLE;
        width = 8;
    }
    else if (form[0] == 'A')
    {
        datacode = TSTRING;

        /*
          the following code is used to support the non-standard
          datatype of the form rAw where r = total width of the field
          and w = width of fixed-length substrings within the field.
        */
        iread = 0;
        if (form[1] != 0)
        {
            if (form[1] == '(' )  /* skip parenthesis around */
                form++;          /* variable length column width */

            iread = sscanf(&form[1],"%ld", &width);
        }

        if (iread != 1 || (!variable && (width > repeat)) )
            width = (long) repeat;
  
    }
    else if (form[0] == 'L')
    {
        datacode = TLOGICAL;
        width = 1;
    }
    else if (form[0] == 'X')
    {
        datacode = TBIT;
        width = 1;
    }
    else if (form[0] == 'B')
    {
        datacode = TBYTE;
        width = 1;
    }
    else if (form[0] == 'S') /* internal code to signify signed byte */
    {
        datacode = TSBYTE;
        width = 1;
    }
    else if (form[0] == 'C')
    {
        datacode = TCOMPLEX;
        width = 8;
    }
    else if (form[0] == 'M')
    {
        datacode = TDBLCOMPLEX;
        width = 16;
    }
    else
    {
        sprintf(message,
        "Illegal binary table TFORMn datatype: \'%s\' ", tform);
        ffpmsg(message);
        return(*status = BAD_TFORM_DTYPE);
    }

    if (variable)
        datacode = datacode * (-1); /* flag variable cols w/ neg type code */

    if (dtcode)
       *dtcode = datacode;

    if (trepeat)
       *trepeat = repeat;

    if (twidth)
       *twidth = width;

    return(*status);
}

/*--------------------------------------------------------------------------*/
void ffcfmt(char *tform,    /* value of an ASCII table TFORMn keyword */
            char *cform)    /* equivalent format code in C language syntax */
/*
  convert the FITS format string for an ASCII Table extension column into the
  equivalent C format string that can be used in a printf statement, after
  the values have been read as a double.
*/
{
    int ii;

    cform[0] = '\0';
    ii = 0;
    while (tform[ii] != 0 && tform[ii] == ' ') /* find first non-blank char */
         ii++;

    if (tform[ii] == 0)
        return;    /* input format string was blank */

    cform[0] = '%';  /* start the format string */

    strcpy(&cform[1], &tform[ii + 1]); /* append the width and decimal code */


    if (tform[ii] == 'A')
        strcat(cform, "s");
    else if (tform[ii] == 'I')
        strcat(cform, ".0f");  /*  0 precision to suppress decimal point */
    if (tform[ii] == 'F')
        strcat(cform, "f");
    if (tform[ii] == 'E')
        strcat(cform, "E");
    if (tform[ii] == 'D')
        strcat(cform, "E");

    return;
}
/*--------------------------------------------------------------------------*/
void ffcdsp(char *tform,    /* value of an ASCII table TFORMn keyword */
            char *cform)    /* equivalent format code in C language syntax */
/*
  convert the FITS TDISPn display format into the equivalent C format
  suitable for use in a printf statement.
*/
{
    int ii;

    cform[0] = '\0';
    ii = 0;
    while (tform[ii] != 0 && tform[ii] == ' ') /* find first non-blank char */
         ii++;

    if (tform[ii] == 0)
    {
        cform[0] = '\0';
        return;    /* input format string was blank */
    }

    if (strchr(tform+ii, '%'))  /* is there a % character in the string?? */
    {
        cform[0] = '\0';
        return;    /* illegal TFORM string (possibly even harmful) */
    }

    cform[0] = '%';  /* start the format string */

    strcpy(&cform[1], &tform[ii + 1]); /* append the width and decimal code */

    if      (tform[ii] == 'A' || tform[ii] == 'a')
        strcat(cform, "s");
    else if (tform[ii] == 'I' || tform[ii] == 'i')
        strcat(cform, "d");
    else if (tform[ii] == 'O' || tform[ii] == 'o')
        strcat(cform, "o");
    else if (tform[ii] == 'Z' || tform[ii] == 'z')
        strcat(cform, "X");
    else if (tform[ii] == 'F' || tform[ii] == 'f')
        strcat(cform, "f");
    else if (tform[ii] == 'E' || tform[ii] == 'e')
        strcat(cform, "E");
    else if (tform[ii] == 'D' || tform[ii] == 'd')
        strcat(cform, "E");
    else if (tform[ii] == 'G' || tform[ii] == 'g')
        strcat(cform, "G");
    else
        cform[0] = '\0';  /* unrecognized tform code */

    return;
}
/*--------------------------------------------------------------------------*/
int ffgcno( fitsfile *fptr,  /* I - FITS file pionter                       */
            int  casesen,    /* I - case sensitive string comparison? 0=no  */
            char *templt,    /* I - input name of column (w/wildcards)      */
            int  *colnum,    /* O - number of the named column; 1=first col */
            int  *status)    /* IO - error status                           */
/*
  Determine the column number corresponding to an input column name.
  The first column of the table = column 1;  
  This supports the * and ? wild cards in the input template.
*/
{
    char colname[FLEN_VALUE];  /*  temporary string to hold column name  */

    ffgcnn(fptr, casesen, templt, colname, colnum, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcnn( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  casesen,    /* I - case sensitive string comparison? 0=no  */
            char *templt,    /* I - input name of column (w/wildcards)      */
            char *colname,   /* O - full column name up to 68 + 1 chars long*/
            int  *colnum,    /* O - number of the named column; 1=first col */
            int  *status)    /* IO - error status                           */
/*
  Return the full column name and column number of the next column whose
  TTYPEn keyword value matches the input template string.
  The template may contain the * and ? wildcards.  Status = 237 is
  returned if the match is not unique.  If so, one may call this routine
  again with input status=237  to get the next match.  A status value of
  219 is returned when there are no more matching columns.
*/
{
    char errmsg[FLEN_ERRMSG];
    int tstatus, ii, founde, foundw, match, exact, unique;
    long ivalue;
    tcolumn *colptr;

    if (*status <= 0)
    {
        (fptr->Fptr)->startcol = 0;   /* start search with first column */
        tstatus = 0;
    }
    else if (*status == COL_NOT_UNIQUE) /* start search from previous spot */
    {
        tstatus = COL_NOT_UNIQUE;
        *status = 0;
    }
    else
        return(*status);  /* bad input status value */

    colname[0] = 0;    /* initialize null return */
    *colnum = 0;

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)   /* rescan header to get col struct */
            return(*status);

    colptr = (fptr->Fptr)->tableptr;   /* pointer to first column */
    colptr += ((fptr->Fptr)->startcol);      /* offset to starting column */

    founde = FALSE;   /* initialize 'found exact match' flag */
    foundw = FALSE;   /* initialize 'found wildcard match' flag */
    unique = FALSE;

    for (ii = (fptr->Fptr)->startcol; ii < (fptr->Fptr)->tfield; ii++, colptr++)
    {
        ffcmps(templt, colptr->ttype, casesen, &match, &exact);
        if (match)
        {
            if (founde && exact)
            {
                /* warning: this is the second exact match we've found     */
                /*reset pointer to first match so next search starts there */
               (fptr->Fptr)->startcol = *colnum;
               return(*status = COL_NOT_UNIQUE);
            }
            else if (founde)   /* a wildcard match */
            {
                /* already found exact match so ignore this non-exact match */
            }
            else if (exact)
            {
                /* this is the first exact match we have found, so save it. */
                strcpy(colname, colptr->ttype);
                *colnum = ii + 1;
                founde = TRUE;
            }
            else if (foundw)
            {
                /* we have already found a wild card match, so not unique */
                /* continue searching for other matches                   */
                unique = FALSE;
            }
            else
            {
               /* this is the first wild card match we've found. save it */
               strcpy(colname, colptr->ttype);
               *colnum = ii + 1;
               (fptr->Fptr)->startcol = *colnum;
               foundw = TRUE;
               unique = TRUE;
            }
        }
    }

    /* OK, we've checked all the names now see if we got any matches */
    if (founde)
    {
        if (tstatus == COL_NOT_UNIQUE)  /* we did find 1 exact match but */
            *status = COL_NOT_UNIQUE;   /* there was a previous match too */
    }
    else if (foundw)
    {
        /* found one or more wildcard matches; report error if not unique */
       if (!unique || tstatus == COL_NOT_UNIQUE)
           *status = COL_NOT_UNIQUE;
    }
    else
    {
        /* didn't find a match; check if template is a positive integer */
        ffc2ii(templt, &ivalue, &tstatus);
        if (tstatus ==  0 && ivalue <= (fptr->Fptr)->tfield && ivalue > 0)
        {
            *colnum = ivalue;

            colptr = (fptr->Fptr)->tableptr;   /* pointer to first column */
            colptr += (ivalue - 1);    /* offset to correct column */
            strcpy(colname, colptr->ttype);
        }
        else
        {
            *status = COL_NOT_FOUND;
            if (tstatus != COL_NOT_UNIQUE)
            {
              sprintf(errmsg, "ffgcnn could not find column: %.45s", templt);
              ffpmsg(errmsg);
            }
        }
    }
    
    (fptr->Fptr)->startcol = *colnum;  /* save pointer for next time */
    return(*status);
}
/*--------------------------------------------------------------------------*/
void ffcmps(char *templt,   /* I - input template (may have wildcards)      */
            char *colname,  /* I - full column name up to 68 + 1 chars long */
            int  casesen,   /* I - case sensitive string comparison? 1=yes  */
            int  *match,    /* O - do template and colname match? 1=yes     */
            int  *exact)    /* O - do strings exactly match, or wildcards   */
/*
  compare the template to the string and test if they match.
  The strings are limited to 68 characters or less (the max. length
  of a FITS string keyword value.  This routine reports whether
  the two strings match and whether the match is exact or
  involves wildcards.

  This algorithm is very similar to the way unix filename wildcards
  work except that this first treats a wild card as a literal character
  when looking for a match.  If there is no literal match, then
  it interpretes it as a wild card.  So the template 'AB*DE'
  is considered to be an exact rather than a wild card match to
  the string 'AB*DE'.  The '#' wild card in the template string will 
  match any consecutive string of decimal digits in the colname.
  
*/
{
    int ii, found, t1, s1, wildsearch = 0, tsave = 0, ssave = 0;
    char temp[FLEN_VALUE], col[FLEN_VALUE];

    *match = FALSE;
    *exact = TRUE;

    strncpy(temp, templt, FLEN_VALUE); /* copy strings to work area */
    strncpy(col, colname, FLEN_VALUE);
    temp[FLEN_VALUE - 1] = '\0';  /* make sure strings are terminated */
    col[FLEN_VALUE - 1]  = '\0';

    /* truncate trailing non-significant blanks */
    for (ii = strlen(temp) - 1; ii >= 0 && temp[ii] == ' '; ii--)
        temp[ii] = '\0';

    for (ii = strlen(col) - 1; ii >= 0 && col[ii] == ' '; ii--)
        col[ii] = '\0';
       
    if (!casesen)
    {             /* convert both strings to uppercase before comparison */
        ffupch(temp);
        ffupch(col);
    }

    if (!FSTRCMP(temp, col) )
    {
        *match = TRUE;     /* strings exactly match */
        return;
    }

    *exact = FALSE;    /* strings don't exactly match */

    t1 = 0;   /* start comparison with 1st char of each string */
    s1 = 0;

    while(1)  /* compare corresponding chars in each string */
    {
      if (temp[t1] == '\0' && col[s1] == '\0')
      { 
         /* completely scanned both strings so they match */
         *match = TRUE;
         return;
      }
      else if (temp[t1] == '\0')
      { 
        if (wildsearch)
        {
            /* 
               the previous wildcard search may have been going down
               a blind alley.  Backtrack, and resume the wildcard
               search with the next character in the string.
            */
            t1 = tsave;
            s1 = ssave + 1;
        }
        else
        {
            /* reached end of template string so they don't match */
            return;
        }
      }
      else if (col[s1] == '\0')
      { 
         /* reached end of other string; they match if the next */
         /* character in the template string is a '*' wild card */

        if (temp[t1] == '*' && temp[t1 + 1] == '\0')
        {
           *match = TRUE;
        }

        return;
      }

      if (temp[t1] == col[s1] || (temp[t1] == '?') )
      {
        s1++;  /* corresponding chars in the 2 strings match */
        t1++;  /* increment both pointers and loop back again */
      }
      else if (temp[t1] == '#' && isdigit((int) col[s1]) )
      {
        s1++;  /* corresponding chars in the 2 strings match */
        t1++;  /* increment both pointers */

        /* find the end of the string of digits */
        while (isdigit((int) col[s1]) ) 
            s1++;        
      }
      else if (temp[t1] == '*')
      {

        /* save current string locations, in case we need to restart */
        wildsearch = 1;
        tsave = t1;
        ssave = s1;

        /* get next char from template and look for it in the col name */
        t1++;
        if (temp[t1] == '\0' || temp[t1] == ' ')
        {
          /* reached end of template so strings match */
          *match = TRUE;
          return;
        }

        found = FALSE;
        while (col[s1] && !found)
        {
          if (temp[t1] == col[s1])
          {
            t1++;  /* found matching characters; incre both pointers */
            s1++;  /* and loop back to compare next chars */
            found = TRUE;
          }
          else
            s1++;  /* increment the column name pointer and try again */
        }

        if (!found)
        {
          return;  /* hit end of column name and failed to find a match */
        }
      }
      else
      {
        if (wildsearch)
        {
            /* 
               the previous wildcard search may have been going down
               a blind alley.  Backtrack, and resume the wildcard
               search with the next character in the string.
            */
            t1 = tsave;
            s1 = ssave + 1;
        }
        else
        {
          return;   /* strings don't match */
        }
      }
    }
}
/*--------------------------------------------------------------------------*/
int ffgtcl( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - column number                           */
            int *typecode,   /* O - datatype code (21 = short, etc)         */
            long *repeat,    /* O - repeat count of field                   */
            long *width,     /* O - if ASCII, width of field or unit string */
            int  *status)    /* IO - error status                           */
/*
  Get Type of table column. 
  Returns the datatype code of the column, as well as the vector
  repeat count and (if it is an ASCII character column) the
  width of the field or a unit string within the field.  This supports the
  TFORMn = 'rAw' syntax for specifying arrays of substrings, so
  if TFORMn = '60A12' then repeat = 60 and width = 12.
*/
{
    LONGLONG trepeat, twidth;
    
    ffgtclll(fptr, colnum, typecode, &trepeat, &twidth, status);

    if (*status > 0)
        return(*status);
	
    if (repeat)
        *repeat= (long) trepeat;
      
    if (width)
        *width = (long) twidth;
    
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtclll( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,       /* I - column number                           */
            int *typecode,   /* O - datatype code (21 = short, etc)         */
            LONGLONG *repeat, /* O - repeat count of field                   */
            LONGLONG *width, /* O - if ASCII, width of field or unit string */
            int  *status)    /* IO - error status                           */
/*
  Get Type of table column. 
  Returns the datatype code of the column, as well as the vector
  repeat count and (if it is an ASCII character column) the
  width of the field or a unit string within the field.  This supports the
  TFORMn = 'rAw' syntax for specifying arrays of substrings, so
  if TFORMn = '60A12' then repeat = 60 and width = 12.
*/
{
    tcolumn *colptr;
    int hdutype, decims;
    long tmpwidth;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
        return(*status = BAD_COL_NUM);

    colptr = (fptr->Fptr)->tableptr;   /* pointer to first column */
    colptr += (colnum - 1);    /* offset to correct column */

    if (ffghdt(fptr, &hdutype, status) > 0)
        return(*status);

    if (hdutype == ASCII_TBL)
    {
       ffasfm(colptr->tform, typecode, &tmpwidth, &decims, status);
       *width = tmpwidth;
       
      if (repeat)
           *repeat = 1;
    }
    else
    {
      if (typecode)
          *typecode = colptr->tdatatype;

      if (width)
          *width = colptr->twidth;

      if (repeat)
          *repeat = colptr->trepeat;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffeqty( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - column number                           */
            int *typecode,   /* O - datatype code (21 = short, etc)         */
            long *repeat,    /* O - repeat count of field                   */
            long *width,     /* O - if ASCII, width of field or unit string */
            int  *status)    /* IO - error status                           */
/*
  Get the 'equivalent' table column type. 

  This routine is similar to the ffgtcl routine (which returns the physical
  datatype of the column, as stored in the FITS file) except that if the
  TSCALn and TZEROn keywords are defined for the column, then it returns
  the 'equivalent' datatype.  Thus, if the column is defined as '1I'  (short
  integer) this routine may return the type as 'TUSHORT' or as 'TFLOAT'
  depending on the TSCALn and TZEROn values.
  
  Returns the datatype code of the column, as well as the vector
  repeat count and (if it is an ASCII character column) the
  width of the field or a unit string within the field.  This supports the
  TFORMn = 'rAw' syntax for specifying arrays of substrings, so
  if TFORMn = '60A12' then repeat = 60 and width = 12.
*/
{
    LONGLONG trepeat, twidth;
    
    ffeqtyll(fptr, colnum, typecode, &trepeat, &twidth, status);

    if (repeat)
        *repeat= (long) trepeat;

    if (width)
        *width = (long) twidth;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffeqtyll( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - column number                           */
            int *typecode,   /* O - datatype code (21 = short, etc)         */
            LONGLONG *repeat,    /* O - repeat count of field                   */
            LONGLONG *width,     /* O - if ASCII, width of field or unit string */
            int  *status)    /* IO - error status                           */
/*
  Get the 'equivalent' table column type. 

  This routine is similar to the ffgtcl routine (which returns the physical
  datatype of the column, as stored in the FITS file) except that if the
  TSCALn and TZEROn keywords are defined for the column, then it returns
  the 'equivalent' datatype.  Thus, if the column is defined as '1I'  (short
  integer) this routine may return the type as 'TUSHORT' or as 'TFLOAT'
  depending on the TSCALn and TZEROn values.
  
  Returns the datatype code of the column, as well as the vector
  repeat count and (if it is an ASCII character column) the
  width of the field or a unit string within the field.  This supports the
  TFORMn = 'rAw' syntax for specifying arrays of substrings, so
  if TFORMn = '60A12' then repeat = 60 and width = 12.
*/
{
    tcolumn *colptr;
    int hdutype, decims, tcode, effcode;
    double tscale, tzero, min_val, max_val;
    long lngscale, lngzero = 0, tmpwidth;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
        return(*status = BAD_COL_NUM);

    colptr = (fptr->Fptr)->tableptr;   /* pointer to first column */
    colptr += (colnum - 1);    /* offset to correct column */

    if (ffghdt(fptr, &hdutype, status) > 0)
        return(*status);

    if (hdutype == ASCII_TBL)
    {
      ffasfm(colptr->tform, typecode, &tmpwidth, &decims, status);
      if (width)
          *width = tmpwidth;

      if (repeat)
           *repeat = 1;
    }
    else
    {
      if (typecode)
          *typecode = colptr->tdatatype;

      if (width)
          *width = colptr->twidth;

      if (repeat)
          *repeat = colptr->trepeat;
    }

    /* return if caller is not interested in the typecode value */
    if (!typecode)
        return(*status);

    /* check if the tscale and tzero keywords are defined, which might
       change the effective datatype of the column  */

    tscale = colptr->tscale;
    tzero = colptr->tzero;

    if (tscale == 1.0 && tzero == 0.0)  /* no scaling */
        return(*status);
 
    tcode = abs(*typecode);

    switch (tcode)
    {
      case TBYTE:   /* binary table 'rB' column */
        min_val = 0.;
        max_val = 255.0;
        break;

      case TSHORT:
        min_val = -32768.0;
        max_val =  32767.0;
        break;
        
      case TLONG:

        min_val = -2147483648.0;
        max_val =  2147483647.0;
        break;
        
      case TLONGLONG:
        /*  simply reuse the TLONG values */
        min_val = -2147483648.0;
        max_val =  2147483647.0;
        break;
	
      default:  /* don't have to deal with other data types */
        return(*status);
    }

    if (tscale >= 0.) {
        min_val = tzero + tscale * min_val;
        max_val = tzero + tscale * max_val;
    } else {
        max_val = tzero + tscale * min_val;
        min_val = tzero + tscale * max_val;
    }
    if (tzero < 2147483648.)  /* don't exceed range of 32-bit integer */
       lngzero = (long) tzero;
    lngscale   = (long) tscale;

    if ((tzero != 2147483648.) && /* special value that exceeds integer range */
       (lngzero != tzero || lngscale != tscale)) { /* not integers? */
       /* floating point scaled values; just decide on required precision */
       if (tcode == TBYTE || tcode == TSHORT)
          effcode = TFLOAT;
       else
          effcode = TDOUBLE;

    /*
       In all the remaining cases, TSCALn and TZEROn are integers,
       and not equal to 1 and 0, respectively.  
    */

    } else if ((min_val == -128.) && (max_val == 127.)) {
        effcode = TSBYTE;
 
    } else if ((min_val >= -32768.0) && (max_val <= 32767.0)) {
        effcode = TSHORT;

    } else if ((min_val >= 0.0) && (max_val <= 65535.0)) {
        effcode = TUSHORT;

    } else if ((min_val >= -2147483648.0) && (max_val <= 2147483647.0)) {
        effcode = TLONG;

    } else if ((min_val >= 0.0) && (max_val < 4294967296.0)) {
        effcode = TULONG;

    } else {  /* exceeds the range of a 32-bit integer */
        effcode = TDOUBLE;
    }   

    /* return the effective datatype code (negative if variable length col.) */
    if (*typecode < 0)  /* variable length array column */
        *typecode = -effcode;
    else
        *typecode = effcode;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgncl( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  *ncols,     /* O - number of columns in the table          */
            int  *status)    /* IO - error status                           */
/*
  Get the number of columns in the table (= TFIELDS keyword)
*/
{
    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
        return(*status = NOT_TABLE);

    *ncols = (fptr->Fptr)->tfield;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgnrw( fitsfile *fptr,  /* I - FITS file pointer                       */
            long  *nrows,    /* O - number of rows in the table             */
            int  *status)    /* IO - error status                           */
/*
  Get the number of rows in the table (= NAXIS2 keyword)
*/
{
    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
        return(*status = NOT_TABLE);

    /* the NAXIS2 keyword may not be up to date, so use the structure value */
    *nrows = (long) (fptr->Fptr)->numrows;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgnrwll( fitsfile *fptr,  /* I - FITS file pointer                     */
            LONGLONG  *nrows,  /* O - number of rows in the table           */
            int  *status)      /* IO - error status                         */
/*
  Get the number of rows in the table (= NAXIS2 keyword)
*/
{
    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
        return(*status = NOT_TABLE);

    /* the NAXIS2 keyword may not be up to date, so use the structure value */
    *nrows = (fptr->Fptr)->numrows;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgacl( fitsfile *fptr,   /* I - FITS file pointer                      */
            int  colnum,      /* I - column number                          */
            char *ttype,      /* O - TTYPEn keyword value                   */
            long *tbcol,      /* O - TBCOLn keyword value                   */
            char *tunit,      /* O - TUNITn keyword value                   */
            char *tform,      /* O - TFORMn keyword value                   */
            double *tscal,    /* O - TSCALn keyword value                   */
            double *tzero,    /* O - TZEROn keyword value                   */
            char *tnull,      /* O - TNULLn keyword value                   */
            char *tdisp,      /* O - TDISPn keyword value                   */
            int  *status)     /* IO - error status                          */
/*
  get ASCII column keyword values
*/
{
    char name[FLEN_KEYWORD], comm[FLEN_COMMENT];
    tcolumn *colptr;
    int tstatus;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
        return(*status = BAD_COL_NUM);

    /* get what we can from the column structure */

    colptr = (fptr->Fptr)->tableptr;   /* pointer to first column */
    colptr += (colnum -1);     /* offset to correct column */

    if (ttype)
        strcpy(ttype, colptr->ttype);

    if (tbcol)
        *tbcol = (long) ((colptr->tbcol) + 1);  /* first col is 1, not 0 */

    if (tform)
        strcpy(tform, colptr->tform);

    if (tscal)
        *tscal = colptr->tscale;

    if (tzero)
        *tzero = colptr->tzero;

    if (tnull)
        strcpy(tnull, colptr->strnull);

    /* read keywords to get additional parameters */

    if (tunit)
    {
        ffkeyn("TUNIT", colnum, name, status);
        tstatus = 0;
        *tunit = '\0';
        ffgkys(fptr, name, tunit, comm, &tstatus);
    }

    if (tdisp)
    {
        ffkeyn("TDISP", colnum, name, status);
        tstatus = 0;
        *tdisp = '\0';
        ffgkys(fptr, name, tdisp, comm, &tstatus);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgbcl( fitsfile *fptr,   /* I - FITS file pointer                      */
            int  colnum,      /* I - column number                          */
            char *ttype,      /* O - TTYPEn keyword value                   */
            char *tunit,      /* O - TUNITn keyword value                   */
            char *dtype,      /* O - datatype char: I, J, E, D, etc.        */
            long *repeat,     /* O - vector column repeat count             */
            double *tscal,    /* O - TSCALn keyword value                   */
            double *tzero,    /* O - TZEROn keyword value                   */
            long *tnull,      /* O - TNULLn keyword value integer cols only */
            char *tdisp,      /* O - TDISPn keyword value                   */
            int  *status)     /* IO - error status                          */
/*
  get BINTABLE column keyword values
*/
{
    LONGLONG trepeat, ttnull;
    
    if (*status > 0)
        return(*status);

    ffgbclll(fptr, colnum, ttype, tunit, dtype, &trepeat, tscal, tzero,
             &ttnull, tdisp, status);

    if (repeat)
        *repeat = (long) trepeat;

    if (tnull)
        *tnull = (long) ttnull;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgbclll( fitsfile *fptr,   /* I - FITS file pointer                      */
            int  colnum,      /* I - column number                          */
            char *ttype,      /* O - TTYPEn keyword value                   */
            char *tunit,      /* O - TUNITn keyword value                   */
            char *dtype,      /* O - datatype char: I, J, E, D, etc.        */
            LONGLONG *repeat, /* O - vector column repeat count             */
            double *tscal,    /* O - TSCALn keyword value                   */
            double *tzero,    /* O - TZEROn keyword value                   */
            LONGLONG *tnull,  /* O - TNULLn keyword value integer cols only */
            char *tdisp,      /* O - TDISPn keyword value                   */
            int  *status)     /* IO - error status                          */
/*
  get BINTABLE column keyword values
*/
{
    char name[FLEN_KEYWORD], comm[FLEN_COMMENT];
    tcolumn *colptr;
    int tstatus;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
        return(*status = BAD_COL_NUM);

    /* get what we can from the column structure */

    colptr = (fptr->Fptr)->tableptr;   /* pointer to first column */
    colptr += (colnum -1);     /* offset to correct column */

    if (ttype)
        strcpy(ttype, colptr->ttype);

    if (dtype)
    {
        if (colptr->tdatatype < 0)  /* add the "P" prefix for */
            strcpy(dtype, "P");     /* variable length columns */
        else
            dtype[0] = 0;

        if      (abs(colptr->tdatatype) == TBIT)
            strcat(dtype, "X");
        else if (abs(colptr->tdatatype) == TBYTE)
            strcat(dtype, "B");
        else if (abs(colptr->tdatatype) == TLOGICAL)
            strcat(dtype, "L");
        else if (abs(colptr->tdatatype) == TSTRING)
            strcat(dtype, "A");
        else if (abs(colptr->tdatatype) == TSHORT)
            strcat(dtype, "I");
        else if (abs(colptr->tdatatype) == TLONG)
            strcat(dtype, "J");
        else if (abs(colptr->tdatatype) == TLONGLONG)
            strcat(dtype, "K");
        else if (abs(colptr->tdatatype) == TFLOAT)
            strcat(dtype, "E");
        else if (abs(colptr->tdatatype) == TDOUBLE)
            strcat(dtype, "D");
        else if (abs(colptr->tdatatype) == TCOMPLEX)
            strcat(dtype, "C");
        else if (abs(colptr->tdatatype) == TDBLCOMPLEX)
            strcat(dtype, "M");
    }

    if (repeat)
        *repeat = colptr->trepeat;

    if (tscal)
        *tscal  = colptr->tscale;

    if (tzero)
        *tzero  = colptr->tzero;

    if (tnull)
        *tnull  = colptr->tnull;

    /* read keywords to get additional parameters */

    if (tunit)
    {
        ffkeyn("TUNIT", colnum, name, status);
        tstatus = 0;
        *tunit = '\0';
        ffgkys(fptr, name, tunit, comm, &tstatus);
    }

    if (tdisp)
    {
        ffkeyn("TDISP", colnum, name, status);
        tstatus = 0;
        *tdisp = '\0';
        ffgkys(fptr, name, tdisp, comm, &tstatus);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghdn(fitsfile *fptr,   /* I - FITS file pointer                      */
            int *chdunum)    /* O - number of the CHDU; 1 = primary array  */
/*
  Return the number of the Current HDU in the FITS file.  The primary array
  is HDU number 1.  Note that this is one of the few cfitsio routines that
  does not return the error status value as the value of the function.
*/
{
    *chdunum = (fptr->HDUposition) + 1;
    return(*chdunum);
}
/*--------------------------------------------------------------------------*/
int ffghadll(fitsfile *fptr,     /* I - FITS file pointer                     */
            LONGLONG *headstart, /* O - byte offset to beginning of CHDU      */
            LONGLONG *datastart, /* O - byte offset to beginning of next HDU  */
            LONGLONG *dataend,   /* O - byte offset to beginning of next HDU  */
            int *status)         /* IO - error status     */
/*
  Return the address (= byte offset) in the FITS file to the beginning of
  the current HDU, the beginning of the data unit, and the end of the data unit.
*/
{
    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        if (ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status) > 0)
            return(*status);
    }
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
    {
        if (ffrdef(fptr, status) > 0)           /* rescan header */
            return(*status);
    }

    if (headstart)
        *headstart = (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu];       

    if (datastart)
        *datastart = (fptr->Fptr)->datastart;

    if (dataend)
        *dataend = (fptr->Fptr)->headstart[((fptr->Fptr)->curhdu) + 1];       

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghof(fitsfile *fptr,     /* I - FITS file pointer                     */
            OFF_T *headstart,  /* O - byte offset to beginning of CHDU      */
            OFF_T *datastart,  /* O - byte offset to beginning of next HDU  */
            OFF_T *dataend,    /* O - byte offset to beginning of next HDU  */
            int *status)       /* IO - error status     */
/*
  Return the address (= byte offset) in the FITS file to the beginning of
  the current HDU, the beginning of the data unit, and the end of the data unit.
*/
{
    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        if (ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status) > 0)
            return(*status);
    }
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
    {
        if (ffrdef(fptr, status) > 0)           /* rescan header */
            return(*status);
    }

    if (headstart)
        *headstart = (OFF_T) (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu];       

    if (datastart)
        *datastart = (OFF_T) (fptr->Fptr)->datastart;

    if (dataend)
        *dataend   = (OFF_T) (fptr->Fptr)->headstart[((fptr->Fptr)->curhdu) + 1];       

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghad(fitsfile *fptr,     /* I - FITS file pointer                     */
            long *headstart,  /* O - byte offset to beginning of CHDU      */
            long *datastart,  /* O - byte offset to beginning of next HDU  */
            long *dataend,    /* O - byte offset to beginning of next HDU  */
            int *status)       /* IO - error status     */
/*
  Return the address (= byte offset) in the FITS file to the beginning of
  the current HDU, the beginning of the data unit, and the end of the data unit.
*/
{
    LONGLONG shead, sdata, edata;

    if (*status > 0)
        return(*status);

    ffghadll(fptr, &shead, &sdata, &edata, status);

    if (headstart)
    {
        if (shead > LONG_MAX)
            *status = NUM_OVERFLOW;
        else
            *headstart = (long) shead;
    }

    if (datastart)
    {
        if (sdata > LONG_MAX)
            *status = NUM_OVERFLOW;
        else
            *datastart = (long) sdata;
    }

    if (dataend)
    {
        if (edata > LONG_MAX)
            *status = NUM_OVERFLOW;
        else
            *dataend = (long) edata;       
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffrhdu(fitsfile *fptr,    /* I - FITS file pointer */
           int *hdutype,      /* O - type of HDU       */
           int *status)       /* IO - error status     */
/*
  read the required keywords of the CHDU and initialize the corresponding
  structure elements that describe the format of the HDU
*/
{
    int ii, tstatus;
    char card[FLEN_CARD];
    char name[FLEN_KEYWORD], value[FLEN_VALUE], comm[FLEN_COMMENT];
    char xname[FLEN_VALUE], *xtension, urltype[20];

    if (*status > 0)
        return(*status);

    if (ffgrec(fptr, 1, card, status) > 0 )  /* get the 80-byte card */
    {
        ffpmsg("Cannot read first keyword in header (ffrhdu).");
        return(*status);
    }
    strncpy(name,card,8);  /* first 8 characters = the keyword name */
    name[8] = '\0';

    for (ii=7; ii >= 0; ii--)  /* replace trailing blanks with nulls */
    {
        if (name[ii] == ' ')
            name[ii] = '\0';
        else
            break;
    }

    if (ffpsvc(card, value, comm, status) > 0)   /* parse value and comment */
    {
        ffpmsg("Cannot read value of first  keyword in header (ffrhdu):");
        ffpmsg(card);
        return(*status);
    }

    if (!strcmp(name, "SIMPLE"))        /* this is the primary array */
    {

       ffpinit(fptr, status);           /* initialize the primary array */

       if (hdutype != NULL)
           *hdutype = 0;
    }

    else if (!strcmp(name, "XTENSION"))   /* this is an XTENSION keyword */
    {
        if (ffc2s(value, xname, status) > 0)  /* get the value string */
        {
            ffpmsg("Bad value string for XTENSION keyword:");
            ffpmsg(value);
            return(*status);
        }

        xtension = xname;
        while (*xtension == ' ')  /* ignore any leading spaces in name */
           xtension++;

        if (!strcmp(xtension, "TABLE"))
        {
            ffainit(fptr, status);       /* initialize the ASCII table */
            if (hdutype != NULL)
                *hdutype = 1;
        }

        else if (!strcmp(xtension, "BINTABLE") ||
                 !strcmp(xtension, "A3DTABLE") ||
                 !strcmp(xtension, "3DTABLE") )
        {
            ffbinit(fptr, status);       /* initialize the binary table */
            if (hdutype != NULL)
                *hdutype = 2;
        }

        else
        {
            tstatus = 0;
            ffpinit(fptr, &tstatus);       /* probably an IMAGE extension */

            if (tstatus == UNKNOWN_EXT && hdutype != NULL)
                *hdutype = -1;       /* don't recognize this extension type */
            else
            {
                *status = tstatus;
                if (hdutype != NULL)
                    *hdutype = 0;
            }
        }
    }

    else     /*  not the start of a new extension */
    {
        if (card[0] == 0  ||
            card[0] == 10)     /* some editors append this character to EOF */
        {           
            *status = END_OF_FILE;
        }
        else
        {
          *status = UNKNOWN_REC;  /* found unknown type of record */
          ffpmsg
        ("Extension doesn't start with SIMPLE or XTENSION keyword. (ffrhdu)");
        ffpmsg(card);
        }
    }

    /*  compare the starting position of the next HDU (if any) with the size */
    /*  of the whole file to see if this is the last HDU in the file */

    if ((fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1] < 
        (fptr->Fptr)->logfilesize )
    {
        (fptr->Fptr)->lasthdu = 0;  /* no, not the last HDU */
    }
    else
    {
        (fptr->Fptr)->lasthdu = 1;  /* yes, this is the last HDU */

        /* special code for mem:// type files (FITS file in memory) */
        /* Allocate enough memory to hold the entire HDU. */
        /* Without this code, CFITSIO would repeatedly realloc  memory */
        /* to incrementally increase the size of the file by 2880 bytes */
        /* at a time, until it reached the final size */
 
        ffurlt(fptr, urltype, status);
        if (!strcmp(urltype,"mem://") || !strcmp(urltype,"memkeep://"))
        {
            fftrun(fptr, (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1],
               status);
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpinit(fitsfile *fptr,      /* I - FITS file pointer */
           int *status)          /* IO - error status     */
/*
  initialize the parameters defining the structure of the primary array
  or an Image extension 
*/
{
    int groups, tstatus, simple, bitpix, naxis, extend, nspace;
    int ttype = 0, bytlen = 0, ii, ntilebins;
    long  pcount, gcount;
    LONGLONG naxes[999], npix, blank;
    double bscale, bzero;
    char comm[FLEN_COMMENT];
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    (fptr->Fptr)->hdutype = IMAGE_HDU; /* primary array or IMAGE extension  */
    (fptr->Fptr)->headend = (fptr->Fptr)->logfilesize;  /* set max size */

    groups = 0;
    tstatus = *status;

    /* get all the descriptive info about this HDU */
    ffgphd(fptr, 999, &simple, &bitpix, &naxis, naxes, &pcount, &gcount, 
           &extend, &bscale, &bzero, &blank, &nspace, status);

    if (*status == NOT_IMAGE)
        *status = tstatus;    /* ignore 'unknown extension type' error */
    else if (*status > 0)
        return(*status);

    /*
       the logical end of the header is 80 bytes before the current position, 
       minus any trailing blank keywords just before the END keyword.
    */
    (fptr->Fptr)->headend = (fptr->Fptr)->nextkey - (80 * (nspace + 1));

    /* the data unit begins at the beginning of the next logical block */
    (fptr->Fptr)->datastart = (((fptr->Fptr)->nextkey - 80) / 2880 + 1)
                              * 2880;

    if (naxis > 0 && naxes[0] == 0)  /* test for 'random groups' */
    {
        tstatus = 0;
        ffmaky(fptr, 2, status);         /* reset to beginning of header */

        if (ffgkyl(fptr, "GROUPS", &groups, comm, &tstatus))
            groups = 0;          /* GROUPS keyword not found */
    }

    if (bitpix == BYTE_IMG)   /* test  bitpix and set the datatype code */
    {
        ttype=TBYTE;
        bytlen=1;
    }
    else if (bitpix == SHORT_IMG)
    {
        ttype=TSHORT;
        bytlen=2;
    }
    else if (bitpix == LONG_IMG)
    {
        ttype=TLONG;
        bytlen=4;
    }
    else if (bitpix == LONGLONG_IMG)
    {
        ttype=TLONGLONG;
        bytlen=8;
    }
    else if (bitpix == FLOAT_IMG)
    {
        ttype=TFLOAT;
        bytlen=4;
    }
    else if (bitpix == DOUBLE_IMG)
    {
        ttype=TDOUBLE;
        bytlen=8;
    }
        
    /*   calculate the size of the primary array  */
    (fptr->Fptr)->imgdim = naxis;
    if (naxis == 0)
    {
        npix = 0;
    }
    else
    {
        if (groups)
        {
            npix = 1;  /* NAXIS1 = 0 is a special flag for 'random groups' */
        }
        else
        {
            npix = naxes[0];
        }

        (fptr->Fptr)->imgnaxis[0] = naxes[0];
        for (ii=1; ii < naxis; ii++)
        {
            npix = npix*naxes[ii];   /* calc number of pixels in the array */
            (fptr->Fptr)->imgnaxis[ii] = naxes[ii];
        }
    }

    /*
       now we know everything about the array; just fill in the parameters:
       the next HDU begins in the next logical block after the data
    */

    (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1] =
         (fptr->Fptr)->datastart + 
         ( ((LONGLONG) pcount + npix) * bytlen * gcount + 2879) / 2880 * 2880;

    /*
      initialize the fictitious heap starting address (immediately following
      the array data) and a zero length heap.  This is used to find the
      end of the data when checking the fill values in the last block. 
    */
    (fptr->Fptr)->heapstart = (npix + pcount) * bytlen * gcount;
    (fptr->Fptr)->heapsize = 0;

    (fptr->Fptr)->compressimg = 0;  /* this is not a compressed image */

    if (naxis == 0)
    {
        (fptr->Fptr)->rowlength = 0;    /* rows have zero length */
        (fptr->Fptr)->tfield = 0;       /* table has no fields   */

        /* free the tile-compressed image cache, if it exists */
        if ((fptr->Fptr)->tilerow) {
           ntilebins = 
	    (((fptr->Fptr)->znaxis[0] - 1) / ((fptr->Fptr)->tilesize[0])) + 1;

           for (ii = 0; ii < ntilebins; ii++) {
             if ((fptr->Fptr)->tiledata[ii]) {
	       free((fptr->Fptr)->tiledata[ii]);
             }

             if ((fptr->Fptr)->tilenullarray[ii]) {
	       free((fptr->Fptr)->tilenullarray[ii]);
             }
            }
	    
	    free((fptr->Fptr)->tileanynull);
	    free((fptr->Fptr)->tiletype);	   
	    free((fptr->Fptr)->tiledatasize);
	    free((fptr->Fptr)->tilenullarray);
	    free((fptr->Fptr)->tiledata);
	    free((fptr->Fptr)->tilerow);

	    (fptr->Fptr)->tileanynull = 0;
	    (fptr->Fptr)->tiletype = 0;	   
	    (fptr->Fptr)->tiledatasize = 0;
	    (fptr->Fptr)->tilenullarray = 0;
	    (fptr->Fptr)->tiledata = 0;
	    (fptr->Fptr)->tilerow = 0;
        }

        if ((fptr->Fptr)->tableptr)
           free((fptr->Fptr)->tableptr); /* free memory for the old CHDU */

        (fptr->Fptr)->tableptr = 0;     /* set a null table structure pointer */
        (fptr->Fptr)->numrows = 0;
        (fptr->Fptr)->origrows = 0;
    }
    else
    {
      /*
        The primary array is actually interpreted as a binary table.  There
        are two columns: the first column contains the group parameters if any.
        The second column contains the primary array of data as a single vector
        column element. In the case of 'random grouped' format, each group
        is stored in a separate row of the table.
      */
        /* the number of rows is equal to the number of groups */
        (fptr->Fptr)->numrows = gcount;
        (fptr->Fptr)->origrows = gcount;

        (fptr->Fptr)->rowlength = (npix + pcount) * bytlen; /* total size */
        (fptr->Fptr)->tfield = 2;  /* 2 fields: group params and the image */

        /* free the tile-compressed image cache, if it exists */
        if ((fptr->Fptr)->tilerow) {

           ntilebins = 
	    (((fptr->Fptr)->znaxis[0] - 1) / ((fptr->Fptr)->tilesize[0])) + 1;

           for (ii = 0; ii < ntilebins; ii++) {
             if ((fptr->Fptr)->tiledata[ii]) {
	       free((fptr->Fptr)->tiledata[ii]);
             }

             if ((fptr->Fptr)->tilenullarray[ii]) {
	       free((fptr->Fptr)->tilenullarray[ii]);
             }
            }
	    
	    free((fptr->Fptr)->tileanynull);
	    free((fptr->Fptr)->tiletype);	   
	    free((fptr->Fptr)->tiledatasize);
	    free((fptr->Fptr)->tilenullarray);
	    free((fptr->Fptr)->tiledata);
	    free((fptr->Fptr)->tilerow);

	    (fptr->Fptr)->tileanynull = 0;
	    (fptr->Fptr)->tiletype = 0;	   
	    (fptr->Fptr)->tiledatasize = 0;
	    (fptr->Fptr)->tilenullarray = 0;
	    (fptr->Fptr)->tiledata = 0;
	    (fptr->Fptr)->tilerow = 0;
        }

        if ((fptr->Fptr)->tableptr)
           free((fptr->Fptr)->tableptr); /* free memory for the old CHDU */

        colptr = (tcolumn *) calloc(2, sizeof(tcolumn) ) ;

        if (!colptr)
        {
          ffpmsg
          ("malloc failed to get memory for FITS array descriptors (ffpinit)");
          (fptr->Fptr)->tableptr = 0;  /* set a null table structure pointer */
          return(*status = ARRAY_TOO_BIG);
        }

        /* copy the table structure address to the fitsfile structure */
        (fptr->Fptr)->tableptr = colptr; 

        /* the first column represents the group parameters, if any */
        colptr->tbcol = 0;
        colptr->tdatatype = ttype;
        colptr->twidth = bytlen;
        colptr->trepeat = (LONGLONG) pcount;
        colptr->tscale = 1.;
        colptr->tzero = 0.;
        colptr->tnull = blank;

        colptr++;  /* increment pointer to the second column */

        /* the second column represents the image array */
        colptr->tbcol = pcount * bytlen; /* col starts after the group parms */
        colptr->tdatatype = ttype; 
        colptr->twidth = bytlen;
        colptr->trepeat = npix;
        colptr->tscale = bscale;
        colptr->tzero = bzero;
        colptr->tnull = blank;
    }

    /* reset next keyword pointer to the start of the header */
    (fptr->Fptr)->nextkey = (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu ];

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffainit(fitsfile *fptr,      /* I - FITS file pointer */
            int *status)         /* IO - error status     */
{
/*
  initialize the parameters defining the structure of an ASCII table 
*/
    int  ii, nspace, ntilebins;
    long tfield;
    LONGLONG pcount, rowlen, nrows, tbcoln;
    tcolumn *colptr = 0;
    char name[FLEN_KEYWORD], value[FLEN_VALUE], comm[FLEN_COMMENT];
    char message[FLEN_ERRMSG], errmsg[81];

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    (fptr->Fptr)->hdutype = ASCII_TBL;  /* set that this is an ASCII table */
    (fptr->Fptr)->headend = (fptr->Fptr)->logfilesize;  /* set max size */

    /* get table parameters and test that the header is a valid: */
    if (ffgttb(fptr, &rowlen, &nrows, &pcount, &tfield, status) > 0)  
       return(*status);

    if (pcount != 0)
    {
       ffpmsg("PCOUNT keyword not equal to 0 in ASCII table (ffainit).");
       sprintf(errmsg, "  PCOUNT = %ld", (long) pcount);
       ffpmsg(errmsg);
       return(*status = BAD_PCOUNT);
    }

    (fptr->Fptr)->rowlength = rowlen; /* store length of a row */
    (fptr->Fptr)->tfield = tfield; /* store number of table fields in row */

     /* free the tile-compressed image cache, if it exists */
     if ((fptr->Fptr)->tilerow) {

           ntilebins = 
	    (((fptr->Fptr)->znaxis[0] - 1) / ((fptr->Fptr)->tilesize[0])) + 1;

           for (ii = 0; ii < ntilebins; ii++) {
             if ((fptr->Fptr)->tiledata[ii]) {
	       free((fptr->Fptr)->tiledata[ii]);
             }

             if ((fptr->Fptr)->tilenullarray[ii]) {
	       free((fptr->Fptr)->tilenullarray[ii]);
             }
            }
	    
	    free((fptr->Fptr)->tileanynull);
	    free((fptr->Fptr)->tiletype);	   
	    free((fptr->Fptr)->tiledatasize);
	    free((fptr->Fptr)->tilenullarray);
	    free((fptr->Fptr)->tiledata);
	    free((fptr->Fptr)->tilerow);

	    (fptr->Fptr)->tileanynull = 0;
	    (fptr->Fptr)->tiletype = 0;	   
	    (fptr->Fptr)->tiledatasize = 0;
	    (fptr->Fptr)->tilenullarray = 0;
	    (fptr->Fptr)->tiledata = 0;
	    (fptr->Fptr)->tilerow = 0;
     }

    if ((fptr->Fptr)->tableptr)
       free((fptr->Fptr)->tableptr); /* free memory for the old CHDU */

    /* mem for column structures ; space is initialized = 0 */
    if (tfield > 0)
    {
      colptr = (tcolumn *) calloc(tfield, sizeof(tcolumn) );
      if (!colptr)
      {
        ffpmsg
        ("malloc failed to get memory for FITS table descriptors (ffainit)");
        (fptr->Fptr)->tableptr = 0;  /* set a null table structure pointer */
        return(*status = ARRAY_TOO_BIG);
      }
    }

    /* copy the table structure address to the fitsfile structure */
    (fptr->Fptr)->tableptr = colptr; 

    /*  initialize the table field parameters */
    for (ii = 0; ii < tfield; ii++, colptr++)
    {
        colptr->ttype[0] = '\0';  /* null column name */
        colptr->tscale = 1.;
        colptr->tzero  = 0.;
        colptr->strnull[0] = ASCII_NULL_UNDEFINED;  /* null value undefined */
        colptr->tbcol = -1;          /* initialize to illegal value */
        colptr->tdatatype = -9999;   /* initialize to illegal value */
    }

    /*
      Initialize the fictitious heap starting address (immediately following
      the table data) and a zero length heap.  This is used to find the
      end of the table data when checking the fill values in the last block. 
      There is no special data following an ASCII table.
    */
    (fptr->Fptr)->numrows = nrows;
    (fptr->Fptr)->origrows = nrows;
    (fptr->Fptr)->heapstart = rowlen * nrows;
    (fptr->Fptr)->heapsize = 0;

    (fptr->Fptr)->compressimg = 0;  /* this is not a compressed image */

    /* now search for the table column keywords and the END keyword */

    for (nspace = 0, ii = 8; 1; ii++)  /* infinite loop  */
    {
        ffgkyn(fptr, ii, name, value, comm, status);

        /* try to ignore minor syntax errors */
        if (*status == NO_QUOTE)
        {
            strcat(value, "'");
            *status = 0;
        }
        else if (*status == BAD_KEYCHAR)
        {
            *status = 0;
        }

        if (*status == END_OF_FILE)
        {
            ffpmsg("END keyword not found in ASCII table header (ffainit).");
            return(*status = NO_END);
        }
        else if (*status > 0)
            return(*status);

        else if (name[0] == 'T')   /* keyword starts with 'T' ? */
            ffgtbp(fptr, name, value, status); /* test if column keyword */

        else if (!FSTRCMP(name, "END"))  /* is this the END keyword? */
            break;

        if (!name[0] && !value[0] && !comm[0])  /* a blank keyword? */
            nspace++;

        else
            nspace = 0;
    }

    /* test that all required keywords were found and have legal values */
    colptr = (fptr->Fptr)->tableptr;
    for (ii = 0; ii < tfield; ii++, colptr++)
    {
        tbcoln = colptr->tbcol;  /* the starting column number (zero based) */

        if (colptr->tdatatype == -9999)
        {
            ffkeyn("TFORM", ii+1, name, status);  /* construct keyword name */
            sprintf(message,"Required %s keyword not found (ffainit).", name);
            ffpmsg(message);
            return(*status = NO_TFORM);
        }

        else if (tbcoln == -1)
        {
            ffkeyn("TBCOL", ii+1, name, status); /* construct keyword name */
            sprintf(message,"Required %s keyword not found (ffainit).", name);
            ffpmsg(message);
            return(*status = NO_TBCOL);
        }

        else if ((fptr->Fptr)->rowlength != 0 && 
                (tbcoln < 0 || tbcoln >= (fptr->Fptr)->rowlength ) )
        {
            ffkeyn("TBCOL", ii+1, name, status);  /* construct keyword name */
            sprintf(message,"Value of %s keyword out of range: %ld (ffainit).",
            name, (long) tbcoln);
            ffpmsg(message);
            return(*status = BAD_TBCOL);
        }

        else if ((fptr->Fptr)->rowlength != 0 && 
                 tbcoln + colptr->twidth > (fptr->Fptr)->rowlength )
        {
            sprintf(message,"Column %d is too wide to fit in table (ffainit)",
            ii+1);
            ffpmsg(message);
            sprintf(message, " TFORM = %s and NAXIS1 = %ld",
                    colptr->tform, (long) (fptr->Fptr)->rowlength);
            ffpmsg(message);
            return(*status = COL_TOO_WIDE);
        }
    }

    /*
      now we know everything about the table; just fill in the parameters:
      the 'END' record is 80 bytes before the current position, minus
      any trailing blank keywords just before the END keyword.
    */
    (fptr->Fptr)->headend = (fptr->Fptr)->nextkey - (80 * (nspace + 1));
 
    /* the data unit begins at the beginning of the next logical block */
    (fptr->Fptr)->datastart = (((fptr->Fptr)->nextkey - 80) / 2880 + 1) 
                              * 2880;

    /* the next HDU begins in the next logical block after the data  */
    (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1] =
         (fptr->Fptr)->datastart +
         ( ((LONGLONG)rowlen * nrows + 2879) / 2880 * 2880 );

    /* reset next keyword pointer to the start of the header */
    (fptr->Fptr)->nextkey = (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu ];

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffbinit(fitsfile *fptr,     /* I - FITS file pointer */
            int *status)        /* IO - error status     */
{
/*
  initialize the parameters defining the structure of a binary table 
*/
    int  ii, nspace, ntilebins;
    long tfield;
    LONGLONG pcount, rowlen, nrows, totalwidth;
    tcolumn *colptr = 0;
    char name[FLEN_KEYWORD], value[FLEN_VALUE], comm[FLEN_COMMENT];
    char message[FLEN_ERRMSG];

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    (fptr->Fptr)->hdutype = BINARY_TBL;  /* set that this is a binary table */
    (fptr->Fptr)->headend = (fptr->Fptr)->logfilesize;  /* set max size */

    /* get table parameters and test that the header is valid: */
    if (ffgttb(fptr, &rowlen, &nrows, &pcount, &tfield, status) > 0)
       return(*status);

    (fptr->Fptr)->rowlength =  rowlen; /* store length of a row */
    (fptr->Fptr)->tfield = tfield; /* store number of table fields in row */

     /* free the tile-compressed image cache, if it exists */
     if ((fptr->Fptr)->tilerow) {

           ntilebins = 
	    (((fptr->Fptr)->znaxis[0] - 1) / ((fptr->Fptr)->tilesize[0])) + 1;

           for (ii = 0; ii < ntilebins; ii++) {
             if ((fptr->Fptr)->tiledata[ii]) {
	       free((fptr->Fptr)->tiledata[ii]);
             }

             if ((fptr->Fptr)->tilenullarray[ii]) {
	       free((fptr->Fptr)->tilenullarray[ii]);
             }
            }
	    
	    free((fptr->Fptr)->tileanynull);
	    free((fptr->Fptr)->tiletype);	   
	    free((fptr->Fptr)->tiledatasize);
	    free((fptr->Fptr)->tilenullarray);
	    free((fptr->Fptr)->tiledata);
	    free((fptr->Fptr)->tilerow);

	    (fptr->Fptr)->tileanynull = 0;
	    (fptr->Fptr)->tiletype = 0;	   
	    (fptr->Fptr)->tiledatasize = 0;
	    (fptr->Fptr)->tilenullarray = 0;
	    (fptr->Fptr)->tiledata = 0;
	    (fptr->Fptr)->tilerow = 0;
     }

    if ((fptr->Fptr)->tableptr)
       free((fptr->Fptr)->tableptr); /* free memory for the old CHDU */

    /* mem for column structures ; space is initialized = 0  */
    if (tfield > 0)
    {
      colptr = (tcolumn *) calloc(tfield, sizeof(tcolumn) );
      if (!colptr)
      {
        ffpmsg
        ("malloc failed to get memory for FITS table descriptors (ffbinit)");
        (fptr->Fptr)->tableptr = 0;  /* set a null table structure pointer */
        return(*status = ARRAY_TOO_BIG);
      }
    }

    /* copy the table structure address to the fitsfile structure */
    (fptr->Fptr)->tableptr = colptr; 

    /* initialize the table field parameters */
    for (ii = 0; ii < tfield; ii++, colptr++)
    {
        colptr->ttype[0] = '\0';  /* null column name */
        colptr->tscale = 1.;
        colptr->tzero  = 0.;
        colptr->tnull  = NULL_UNDEFINED; /* (integer) null value undefined */
        colptr->tdatatype = -9999;   /* initialize to illegal value */
        colptr->trepeat = 1;
        colptr->strnull[0] = '\0'; /* for ASCII string columns (TFORM = rA) */
    }

    /*
      Initialize the heap starting address (immediately following
      the table data) and the size of the heap.  This is used to find the
      end of the table data when checking the fill values in the last block. 
    */
    (fptr->Fptr)->numrows = nrows;
    (fptr->Fptr)->origrows = nrows;
    (fptr->Fptr)->heapstart = rowlen * nrows;
    (fptr->Fptr)->heapsize = pcount;

    (fptr->Fptr)->compressimg = 0;  /* initialize as not a compressed image */

    /* now search for the table column keywords and the END keyword */

    for (nspace = 0, ii = 8; 1; ii++)  /* infinite loop  */
    {
        ffgkyn(fptr, ii, name, value, comm, status);

        /* try to ignore minor syntax errors */
        if (*status == NO_QUOTE)
        {
            strcat(value, "'");
            *status = 0;
        }
        else if (*status == BAD_KEYCHAR)
        {
            *status = 0;
        }

        if (*status == END_OF_FILE)
        {
            ffpmsg("END keyword not found in binary table header (ffbinit).");
            return(*status = NO_END);
        }
        else if (*status > 0)
            return(*status);

        else if (name[0] == 'T')   /* keyword starts with 'T' ? */
            ffgtbp(fptr, name, value, status); /* test if column keyword */

        else if (!FSTRCMP(name, "ZIMAGE"))
        {
            if (value[0] == 'T')
                (fptr->Fptr)->compressimg = 1; /* this is a compressed image */
        }
        else if (!FSTRCMP(name, "END"))  /* is this the END keyword? */
            break;


        if (!name[0] && !value[0] && !comm[0])  /* a blank keyword? */
            nspace++;

        else
            nspace = 0; /* reset number of consecutive spaces before END */
    }

    /* test that all the required keywords were found and have legal values */
    colptr = (fptr->Fptr)->tableptr;  /* set pointer to first column */

    for (ii = 0; ii < tfield; ii++, colptr++)
    {
        if (colptr->tdatatype == -9999)
        {
            ffkeyn("TFORM", ii+1, name, status);  /* construct keyword name */
            sprintf(message,"Required %s keyword not found (ffbinit).", name);
            ffpmsg(message);
            return(*status = NO_TFORM);
        }
    }

    /*
      now we know everything about the table; just fill in the parameters:
      the 'END' record is 80 bytes before the current position, minus
      any trailing blank keywords just before the END keyword.
    */

    (fptr->Fptr)->headend = (fptr->Fptr)->nextkey - (80 * (nspace + 1));
 
    /* the data unit begins at the beginning of the next logical block */
    (fptr->Fptr)->datastart = (((fptr->Fptr)->nextkey - 80) / 2880 + 1) 
                              * 2880;

    /* the next HDU begins in the next logical block after the data  */
    (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1] = 
         (fptr->Fptr)->datastart +
	 ( ((fptr->Fptr)->heapstart + (fptr->Fptr)->heapsize + 2879) / 2880 * 2880 );

    /* determine the byte offset to the beginning of each column */
    ffgtbc(fptr, &totalwidth, status);

    if (totalwidth != rowlen)
    {
        sprintf(message,
        "NAXIS1 = %ld is not equal to the sum of column widths: %ld", 
        (long) rowlen, (long) totalwidth);
        ffpmsg(message);
        *status = BAD_ROW_WIDTH;
    }

    /* reset next keyword pointer to the start of the header */
    (fptr->Fptr)->nextkey = (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu ];

    if ( (fptr->Fptr)->compressimg == 1) /*  Is this a compressed image */
        imcomp_get_compressed_image_par(fptr, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgabc(int tfields,     /* I - number of columns in the table           */
           char **tform,    /* I - value of TFORMn keyword for each column  */
           int space,       /* I - number of spaces to leave between cols   */
           long *rowlen,    /* O - total width of a table row               */
           long *tbcol,     /* O - starting byte in row for each column     */
           int *status)     /* IO - error status                            */
/*
  calculate the starting byte offset of each column of an ASCII table
  and the total length of a row, in bytes.  The input space value determines
  how many blank spaces to leave between each column (1 is recommended).
*/
{
    int ii, datacode, decims;
    long width;

    if (*status > 0)
        return(*status);

    *rowlen=0;

    if (tfields <= 0)
        return(*status);

    tbcol[0] = 1;

    for (ii = 0; ii < tfields; ii++)
    {
        tbcol[ii] = *rowlen + 1;    /* starting byte in row of column */

        ffasfm(tform[ii], &datacode, &width, &decims, status);

        *rowlen += (width + space);  /* total length of row */
    }

    *rowlen -= space;  /*  don't add space after the last field */

    return (*status);
}
/*--------------------------------------------------------------------------*/
int ffgtbc(fitsfile *fptr,    /* I - FITS file pointer          */
           LONGLONG *totalwidth,  /* O - total width of a table row */
           int *status)       /* IO - error status              */
{
/*
  calculate the starting byte offset of each column of a binary table.
  Use the values of the datatype code and repeat counts in the
  column structure. Return the total length of a row, in bytes.
*/
    int tfields, ii;
    LONGLONG nbytes;
    tcolumn *colptr;
    char message[FLEN_ERRMSG], *cptr;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    tfields = (fptr->Fptr)->tfield;
    colptr = (fptr->Fptr)->tableptr;  /* point to first column structure */

    *totalwidth = 0;

    for (ii = 0; ii < tfields; ii++, colptr++)
    {
        colptr->tbcol = *totalwidth;  /* byte offset in row to this column */

        if (colptr->tdatatype == TSTRING)
        {
            nbytes =  colptr->trepeat;   /* one byte per char */
        }
        else if (colptr->tdatatype == TBIT)
        {
            nbytes = ( colptr->trepeat + 7) / 8;
        }
        else if (colptr->tdatatype > 0)
        {
            nbytes =  colptr->trepeat * (colptr->tdatatype / 10);
        }
        else  {
	
	  cptr = colptr->tform;
	  while (isdigit(*cptr)) cptr++;
	
	  if (*cptr == 'P')  
	   /* this is a 'P' variable length descriptor (neg. tdatatype) */
            nbytes = colptr->trepeat * 8;
	  else if (*cptr == 'Q') 
	   /* this is a 'Q' variable length descriptor (neg. tdatatype) */
            nbytes = colptr->trepeat * 16;

	  else {
		sprintf(message,
		"unknown binary table column type: %s", colptr->tform);
		ffpmsg(message);
		*status = BAD_TFORM;
		return(*status);
	  }
 	}

       *totalwidth = *totalwidth + nbytes;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtbp(fitsfile *fptr,     /* I - FITS file pointer   */
           char *name,         /* I - name of the keyword */
           char *value,        /* I - value string of the keyword */
           int *status)        /* IO - error status       */
{
/*
  Get TaBle Parameter.  The input keyword name begins with the letter T.
  Test if the keyword is one of the table column definition keywords
  of an ASCII or binary table. If so, decode it and update the value 
  in the structure.
*/
    int tstatus, datacode, decimals;
    long width, repeat, nfield, ivalue;
    LONGLONG jjvalue;
    double dvalue;
    char tvalue[FLEN_VALUE], *loc;
    char message[FLEN_ERRMSG];
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    tstatus = 0;

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if(!FSTRNCMP(name + 1, "TYPE", 4) )
    {
        /* get the index number */
        if( ffc2ii(name + 5, &nfield, &tstatus) > 0) /* read index no. */
            return(*status);    /* must not be an indexed keyword */

        if (nfield < 1 || nfield > (fptr->Fptr)->tfield ) /* out of range */
            return(*status);

        colptr = (fptr->Fptr)->tableptr;        /* get pointer to columns */
        colptr = colptr + nfield - 1;   /* point to the correct column */

        if (ffc2s(value, tvalue, &tstatus) > 0)  /* remove quotes */
            return(*status);

        strcpy(colptr->ttype, tvalue);  /* copy col name to structure */
    }
    else if(!FSTRNCMP(name + 1, "FORM", 4) )
    {
        /* get the index number */
        if( ffc2ii(name + 5, &nfield, &tstatus) > 0) /* read index no. */
            return(*status);    /* must not be an indexed keyword */

        if (nfield < 1 || nfield > (fptr->Fptr)->tfield )  /* out of range */
            return(*status);

        colptr = (fptr->Fptr)->tableptr;        /* get pointer to columns */
        colptr = colptr + nfield - 1;   /* point to the correct column */

        if (ffc2s(value, tvalue, &tstatus) > 0)  /* remove quotes */
            return(*status);

        strncpy(colptr->tform, tvalue, 9);  /* copy TFORM to structure */
        colptr->tform[9] = '\0';            /* make sure it is terminated */

        if ((fptr->Fptr)->hdutype == ASCII_TBL)  /* ASCII table */
        {
          if (ffasfm(tvalue, &datacode, &width, &decimals, status) > 0)
              return(*status);  /* bad format code */

          colptr->tdatatype = TSTRING; /* store datatype code */
          colptr->trepeat = 1;      /* field repeat count == 1 */
          colptr->twidth = width;   /* the width of the field, in bytes */
        }
        else  /* binary table */
        {
          if (ffbnfm(tvalue, &datacode, &repeat, &width, status) > 0)
              return(*status);  /* bad format code */

          colptr->tdatatype = datacode; /* store datatype code */
          colptr->trepeat = (LONGLONG) repeat;     /* field repeat count  */

          /* Don't overwrite the unit string width if it was previously */
	  /* set by a TDIMn keyword and has a legal value */
          if (datacode == TSTRING) {
	    if (colptr->twidth == 0 || colptr->twidth > repeat)
              colptr->twidth = width;   /*  width of a unit string */

          } else {
              colptr->twidth = width;   /*  width of a unit value in chars */
          }
        }
    }
    else if(!FSTRNCMP(name + 1, "BCOL", 4) )
    {
        /* get the index number */
        if( ffc2ii(name + 5, &nfield, &tstatus) > 0) /* read index no. */
            return(*status);    /* must not be an indexed keyword */

        if (nfield < 1 || nfield > (fptr->Fptr)->tfield )  /* out of range */
            return(*status);

        colptr = (fptr->Fptr)->tableptr;        /* get pointer to columns */
        colptr = colptr + nfield - 1;   /* point to the correct column */

        if ((fptr->Fptr)->hdutype == BINARY_TBL)
            return(*status);  /* binary tables don't have TBCOL keywords */

        if (ffc2ii(value, &ivalue, status) > 0)
        {
            sprintf(message,
            "Error reading value of %s as an integer: %s", name, value);
            ffpmsg(message);
            return(*status);
        }
        colptr->tbcol = ivalue - 1; /* convert to zero base */
    }
    else if(!FSTRNCMP(name + 1, "SCAL", 4) )
    {
        /* get the index number */
        if( ffc2ii(name + 5, &nfield, &tstatus) > 0) /* read index no. */
            return(*status);    /* must not be an indexed keyword */

        if (nfield < 1 || nfield > (fptr->Fptr)->tfield )  /* out of range */
            return(*status);

        colptr = (fptr->Fptr)->tableptr;        /* get pointer to columns */
        colptr = colptr + nfield - 1;   /* point to the correct column */

        if (ffc2dd(value, &dvalue, &tstatus) > 0)
        {
            sprintf(message,
            "Error reading value of %s as a double: %s", name, value);
            ffpmsg(message);

            /* ignore this error, so don't return error status */
            return(*status);
        }
        colptr->tscale = dvalue;
    }
    else if(!FSTRNCMP(name + 1, "ZERO", 4) )
    {
        /* get the index number */
        if( ffc2ii(name + 5, &nfield, &tstatus) > 0) /* read index no. */
            return(*status);    /* must not be an indexed keyword */

        if (nfield < 1 || nfield > (fptr->Fptr)->tfield )  /* out of range */
            return(*status);

        colptr = (fptr->Fptr)->tableptr;        /* get pointer to columns */
        colptr = colptr + nfield - 1;   /* point to the correct column */

        if (ffc2dd(value, &dvalue, &tstatus) > 0)
        {
            sprintf(message,
            "Error reading value of %s as a double: %s", name, value);
            ffpmsg(message);

            /* ignore this error, so don't return error status */
            return(*status);
        }
        colptr->tzero = dvalue;
    }
    else if(!FSTRNCMP(name + 1, "NULL", 4) )
    {
        /* get the index number */
        if( ffc2ii(name + 5, &nfield, &tstatus) > 0) /* read index no. */
            return(*status);    /* must not be an indexed keyword */

        if (nfield < 1 || nfield > (fptr->Fptr)->tfield )  /* out of range */
            return(*status);

        colptr = (fptr->Fptr)->tableptr;        /* get pointer to columns */
        colptr = colptr + nfield - 1;   /* point to the correct column */

        if ((fptr->Fptr)->hdutype == ASCII_TBL)  /* ASCII table */
        {
            if (ffc2s(value, tvalue, &tstatus) > 0)  /* remove quotes */
                return(*status);

            strncpy(colptr->strnull, tvalue, 17);  /* copy TNULL string */
            colptr->strnull[17] = '\0';  /* terminate the strnull field */

        }
        else  /* binary table */
        {
            if (ffc2jj(value, &jjvalue, &tstatus) > 0) 
            {
                sprintf(message,
                "Error reading value of %s as an integer: %s", name, value);
                ffpmsg(message);

                /* ignore this error, so don't return error status */
                return(*status);
            }
            colptr->tnull = jjvalue; /* null value for integer column */
        }
    }
    else if(!FSTRNCMP(name + 1, "DIM", 3) )
    {
        if ((fptr->Fptr)->hdutype == ASCII_TBL)  /* ASCII table */
            return(*status);  /* ASCII tables don't support TDIMn keyword */ 

        /* get the index number */
        if( ffc2ii(name + 4, &nfield, &tstatus) > 0) /* read index no. */
            return(*status);    /* must not be an indexed keyword */

        if (nfield < 1 || nfield > (fptr->Fptr)->tfield )  /* out of range */
            return(*status);

        colptr = (fptr->Fptr)->tableptr;     /* get pointer to columns */
        colptr = colptr + nfield - 1;   /* point to the correct column */

        /* uninitialized columns have tdatatype set = -9999 */
        if (colptr->tdatatype != -9999 && colptr->tdatatype != TSTRING)
	    return(*status);     /* this is not an ASCII string column */
	   
        loc = strchr(value, '(' );  /* find the opening parenthesis */
        if (!loc)
            return(*status);   /* not a proper TDIM keyword */

        loc++;
        width = strtol(loc, &loc, 10);  /* read size of first dimension */
        if (colptr->trepeat != 1 && colptr->trepeat < width)
	    return(*status);  /* string length is greater than column width */

        colptr->twidth = width;   /* set width of a unit string in chars */
    }
    else if (!FSTRNCMP(name + 1, "HEAP", 4) )
    {
        if ((fptr->Fptr)->hdutype == ASCII_TBL)  /* ASCII table */
            return(*status);  /* ASCII tables don't have a heap */ 

        if (ffc2jj(value, &jjvalue, &tstatus) > 0) 
        {
            sprintf(message,
            "Error reading value of %s as an integer: %s", name, value);
            ffpmsg(message);

            /* ignore this error, so don't return error status */
            return(*status);
        }
        (fptr->Fptr)->heapstart = jjvalue; /* starting byte of the heap */
        return(*status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcprll( fitsfile *fptr, /* I - FITS file pointer                      */
        int colnum,     /* I - column number (1 = 1st column of table)      */
        LONGLONG firstrow,  /* I - first row (1 = 1st row of table)         */
        LONGLONG firstelem, /* I - first element within vector (1 = 1st)    */
        LONGLONG nelem, /* I - number of elements to read or write          */
        int writemode,  /* I - = 1 if writing data, = 0 if reading data     */
                        /*     If = 2, then writing data, but don't modify  */
                        /*     the returned values of repeat and incre.     */
                        /*     If = -1, then reading data in reverse        */
                        /*     direction.                                   */
        double *scale,  /* O - FITS scaling factor (TSCALn keyword value)   */
        double *zero,   /* O - FITS scaling zero pt (TZEROn keyword value)  */
        char *tform,    /* O - ASCII column format: value of TFORMn keyword */
        long *twidth,   /* O - width of ASCII column (characters)           */
        int *tcode,     /* O - column datatype code: I*4=41, R*4=42, etc    */
        int *maxelem,   /* O - max number of elements that fit in buffer    */
        LONGLONG *startpos,/* O - offset in file to starting row & column      */
        LONGLONG *elemnum, /* O - starting element number ( 0 = 1st element)   */
        long *incre,    /* O - byte offset between elements within a row    */
        LONGLONG *repeat,  /* O - number of elements in a row (vector column)  */
        LONGLONG *rowlen,  /* O - length of a row, in bytes                    */
        int  *hdutype,  /* O - HDU type: 0, 1, 2 = primary, table, bintable */
        LONGLONG *tnull,    /* O - null value for integer columns               */
        char *snull,    /* O - null value for ASCII table columns           */
        int *status)    /* IO - error status                                */
/*
  Get Column PaRameters, and test starting row and element numbers for 
  validity.  This is a workhorse routine that is call by nearly every
  other routine that reads or writes to FITS files.
*/
{
    int nulpos, rangecheck = 1, tstatus = 0;
    LONGLONG datastart, endpos;
    long nblock;
    LONGLONG heapoffset, lrepeat, endrow, nrows, tbcol;
    char message[81];
    tcolumn *colptr;

    if (fptr->HDUposition != (fptr->Fptr)->curhdu) {
        /* reset position to the correct HDU if necessary */
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    } else if ((fptr->Fptr)->datastart == DATA_UNDEFINED) {
        /* rescan header if data structure is undefined */
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    } else if (writemode > 0) {

	/* Only terminate the header with the END card if */
	/* writing to the stdout stream (don't have random access). */

	/* Initialize STREAM_DRIVER to be the device number for */
	/* writing FITS files directly out to the stdout stream. */
	/* This only needs to be done once and is thread safe. */
	if (STREAM_DRIVER <= 0 || STREAM_DRIVER > 40) {
            urltype2driver("stream://", &STREAM_DRIVER);
        }

        if ((fptr->Fptr)->driver == STREAM_DRIVER) {
	    if ((fptr->Fptr)->ENDpos != 
	       maxvalue((fptr->Fptr)->headend , (fptr->Fptr)->datastart -2880)) {
	           ffwend(fptr, status);
	    } 
	}
    }

    /* Do sanity check of input parameters */
    if (firstrow < 1)
    {
        if ((fptr->Fptr)->hdutype == IMAGE_HDU) /*  Primary Array or IMAGE */
        {
          sprintf(message, "Image group number is less than 1: %.0f",
                (double) firstrow);
          ffpmsg(message);
          return(*status = BAD_ROW_NUM);
        }
        else
        {
          sprintf(message, "Starting row number is less than 1: %.0f",
                (double) firstrow);
          ffpmsg(message);
          return(*status = BAD_ROW_NUM);
        }
    }
    else if ((fptr->Fptr)->hdutype != ASCII_TBL && firstelem < 1)
    {
        sprintf(message, "Starting element number less than 1: %ld",
               (long) firstelem);
        ffpmsg(message);
        return(*status = BAD_ELEM_NUM);
    }
    else if (nelem < 0)
    {
        sprintf(message, "Tried to read or write less than 0 elements: %.0f",
            (double) nelem);
        ffpmsg(message);
        return(*status = NEG_BYTES);
    }
    else if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
    {
        sprintf(message, "Specified column number is out of range: %d",
                colnum);
        ffpmsg(message);
        sprintf(message, "  There are %d columns in this table.",
                (fptr->Fptr)->tfield );
        ffpmsg(message);

        return(*status = BAD_COL_NUM);
    }

    /*  copy relevant parameters from the structure */

    *hdutype = (fptr->Fptr)->hdutype;    /* image, ASCII table, or BINTABLE  */
    *rowlen   = (fptr->Fptr)->rowlength; /* width of the table, in bytes     */
    datastart = (fptr->Fptr)->datastart; /* offset in file to start of table */

    colptr  = (fptr->Fptr)->tableptr;    /* point to first column */
    colptr += (colnum - 1);      /* offset to correct column structure */

    *scale    = colptr->tscale;  /* value scaling factor;    default = 1.0 */
    *zero     = colptr->tzero;   /* value scaling zeropoint; default = 0.0 */
    *tnull    = colptr->tnull;   /* null value for integer columns         */
    tbcol     = colptr->tbcol;   /* offset to start of column within row   */
    *twidth   = colptr->twidth;  /* width of a single datum, in bytes      */
    *incre    = colptr->twidth;  /* increment between datums, in bytes     */

    *tcode    = colptr->tdatatype;
    *repeat   = colptr->trepeat;

    strcpy(tform, colptr->tform);    /* value of TFORMn keyword            */
    strcpy(snull, colptr->strnull);  /* null value for ASCII table columns */

    if (*hdutype == ASCII_TBL && snull[0] == '\0')
    {
     /* In ASCII tables, a null value is equivalent to all spaces */

       strcpy(snull, "                 ");   /* maximum of 17 spaces */
       nulpos = minvalue(17, *twidth);      /* truncate to width of column */
       snull[nulpos] = '\0';
    }

    /* Special case:  interpret writemode = -1 as reading data, but */
    /* don't do error check for exceeding the range of pixels  */
    if (writemode == -1)
    {
      writemode = 0;
      rangecheck = 0;
    }

    /* Special case: interprete 'X' column as 'B' */
    if (abs(*tcode) == TBIT)
    {
        *tcode  = *tcode / TBIT * TBYTE;
        *repeat = (*repeat + 7) / 8;
    }

    /* Special case: support the 'rAw' format in BINTABLEs */
    if (*hdutype == BINARY_TBL && *tcode == TSTRING) {
       *repeat = *repeat / *twidth;  /* repeat = # of unit strings in field */
    }
    else if (*hdutype == BINARY_TBL && *tcode == -TSTRING) {
       /* variable length string */
       *incre = 1;
       *twidth = (long) nelem;
    }

    if (*hdutype == ASCII_TBL)
        *elemnum = 0;   /* ASCII tables don't have vector elements */
    else
        *elemnum = firstelem - 1;

    /* interprete complex and double complex as pairs of floats or doubles */
    if (abs(*tcode) >= TCOMPLEX)
    {
        if (*tcode > 0)
          *tcode = (*tcode + 1) / 2;
        else
          *tcode = (*tcode - 1) / 2;

        *repeat  = *repeat * 2;
        *twidth  = *twidth / 2;
        *incre   = *incre  / 2;
    }

    /* calculate no. of pixels that fit in buffer */
    /* allow for case where floats are 8 bytes long */
    if (abs(*tcode) == TFLOAT)
       *maxelem = DBUFFSIZE / sizeof(float);
    else if (abs(*tcode) == TDOUBLE)
       *maxelem = DBUFFSIZE / sizeof(double);
    else if (abs(*tcode) == TSTRING)
    {
       *maxelem = (DBUFFSIZE - 1)/ *twidth; /* leave room for final \0 */
       if (*maxelem == 0) {
            sprintf(message,
        "ASCII string column is too wide: %ld; max supported width is %d",
                   *twidth,  DBUFFSIZE - 1);
            ffpmsg(message);
            return(*status = COL_TOO_WIDE);
        }
    }
    else
       *maxelem = DBUFFSIZE / *twidth; 

    /* calc starting byte position to 1st element of col  */
    /*  (this does not apply to variable length columns)  */
    *startpos = datastart + ((LONGLONG)(firstrow - 1) * *rowlen) + tbcol;

    if (*hdutype == IMAGE_HDU && writemode) /*  Primary Array or IMAGE */
    { /*
        For primary arrays, set the repeat count greater than the total
        number of pixels to be written.  This prevents an out-of-range
        error message in cases where the final image array size is not
        yet known or defined.
      */
        if (*repeat < *elemnum + nelem)
            *repeat = *elemnum + nelem; 
    }
    else if (*tcode > 0)     /*  Fixed length table column  */
    {
        if (*elemnum >= *repeat)
        {
            sprintf(message,
        "First element to write is too large: %ld; max allowed value is %ld",
                   (long) ((*elemnum) + 1), (long) *repeat);
            ffpmsg(message);
            return(*status = BAD_ELEM_NUM);
        }

        /* last row number to be read or written */
        endrow = ((*elemnum + nelem - 1) / *repeat) + firstrow;

        if (writemode)
        {
            /* check if we are writing beyond the current end of table */
            if ((endrow > (fptr->Fptr)->numrows) && (nelem > 0) )
            {
                /* if there are more HDUs following the current one, or */
                /* if there is a data heap, then we must insert space */
                /* for the new rows.  */
                if ( !((fptr->Fptr)->lasthdu) || (fptr->Fptr)->heapsize > 0)
                {
                    nrows = endrow - ((fptr->Fptr)->numrows);
                    if (ffirow(fptr, (fptr->Fptr)->numrows, nrows, status) > 0)
                    {
                       sprintf(message,
                       "Failed to add space for %.0f new rows in table.",
                       (double) nrows);
                       ffpmsg(message);
                       return(*status);
                    }
                }
                else
                {
                  /* update heap starting address */
                  (fptr->Fptr)->heapstart += 
                  ((LONGLONG)(endrow - (fptr->Fptr)->numrows) * 
                          (fptr->Fptr)->rowlength );

                  (fptr->Fptr)->numrows = endrow; /* update number of rows */
                }
            }
        }
        else  /* reading from the file */
        {
          if ( endrow > (fptr->Fptr)->numrows && rangecheck)
          {
            if (*hdutype == IMAGE_HDU) /*  Primary Array or IMAGE */
            {
              if (firstrow > (fptr->Fptr)->numrows)
              {
                sprintf(message, 
                  "Attempted to read from group %ld of the HDU,", (long) firstrow);
                ffpmsg(message);

                sprintf(message, 
                  "however the HDU only contains %ld group(s).",
                   (long) ((fptr->Fptr)->numrows) );
                ffpmsg(message);
              }
              else
              {
                ffpmsg("Attempt to read past end of array:");
                sprintf(message, 
                  "  Image has  %ld elements;", (long) *repeat);
                ffpmsg(message);

                sprintf(message, 
                "  Tried to read %ld elements starting at element %ld.",
                (long) nelem, (long) firstelem);
                ffpmsg(message);
              }
            }
            else
            {
              ffpmsg("Attempt to read past end of table:");
              sprintf(message, 
                "  Table has %.0f rows with %.0f elements per row;",
                    (double) ((fptr->Fptr)->numrows), (double) *repeat);
              ffpmsg(message);

              sprintf(message, 
              "  Tried to read %.0f elements starting at row %.0f, element %.0f.",
              (double) nelem, (double) firstrow, (double) ((*elemnum) + 1));
              ffpmsg(message);

            }
            return(*status = BAD_ROW_NUM);
          }
        }

        if (*repeat == 1 && nelem > 1 && writemode != 2)
        { /*
            When accessing a scalar column, fool the calling routine into
            thinking that this is a vector column with very big elements.
            This allows multiple values (up to the maxelem number of elements
            that will fit in the buffer) to be read or written with a single
            routine call, which increases the efficiency.

            If writemode == 2, then the calling program does not want to
            have this efficiency trick applied.
          */           
            if (*rowlen <= LONG_MAX) {
                *incre = (long) *rowlen;
                *repeat = nelem;
            }
        }
    }
    else    /*  Variable length Binary Table column */
    {
      *tcode *= (-1);  

      if (writemode)    /* return next empty heap address for writing */
      {

        *repeat = nelem + *elemnum; /* total no. of elements in the field */

        /* first, check if we are overwriting an existing row, and */
        /* if so, if the existing space is big enough for the new vector */

        if ( firstrow <= (fptr->Fptr)->numrows )
        {
          ffgdesll(fptr, colnum, firstrow, &lrepeat, &heapoffset, &tstatus);
          if (!tstatus)
          {
            if (colptr->tdatatype <= -TCOMPLEX)
              lrepeat = lrepeat * 2;  /* no. of float or double values */
            else if (colptr->tdatatype == -TBIT)
              lrepeat = (lrepeat + 7) / 8;  /* convert from bits to bytes */

            if (lrepeat >= *repeat)  /* enough existing space? */
            {
              *startpos = datastart + heapoffset + (fptr->Fptr)->heapstart;

              /*  write the descriptor into the fixed length part of table */
              if (colptr->tdatatype <= -TCOMPLEX)
              {
                /* divide repeat count by 2 to get no. of complex values */
                ffpdes(fptr, colnum, firstrow, *repeat / 2, 
                      heapoffset, status);
              }
              else
              {
                ffpdes(fptr, colnum, firstrow, *repeat,
                      heapoffset, status);
              }
              return(*status);
            }
          }
        }

        /* Add more rows to the table, if writing beyond the end. */
        /* It is necessary to shift the heap down in this case */
        if ( firstrow > (fptr->Fptr)->numrows)
        {
            nrows = firstrow - ((fptr->Fptr)->numrows);
            if (ffirow(fptr, (fptr->Fptr)->numrows, nrows, status) > 0)
            {
                sprintf(message,
                "Failed to add space for %.0f new rows in table.",
                       (double) nrows);
                ffpmsg(message);
                return(*status);
            }
        }

        /*  calculate starting position (for writing new data) in the heap */
        *startpos = datastart + (fptr->Fptr)->heapstart + 
                    (fptr->Fptr)->heapsize;

        /*  write the descriptor into the fixed length part of table */
        if (colptr->tdatatype <= -TCOMPLEX)
        {
          /* divide repeat count by 2 to get no. of complex values */
          ffpdes(fptr, colnum, firstrow, *repeat / 2, 
                (fptr->Fptr)->heapsize, status);
        }
        else
        {
          ffpdes(fptr, colnum, firstrow, *repeat, (fptr->Fptr)->heapsize,
                 status);
        }

        /* If this is not the last HDU in the file, then check if */
        /* extending the heap would overwrite the following header. */
        /* If so, then have to insert more blocks. */
        if ( !((fptr->Fptr)->lasthdu) )
        {
            endpos = datastart + (fptr->Fptr)->heapstart + 
                     (fptr->Fptr)->heapsize + ( *repeat * (*incre));

            if (endpos > (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1])
            {
                /* calc the number of blocks that need to be added */
                nblock = (long) (((endpos - 1 - 
                         (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1] ) 
                         / 2880) + 1);

                if (ffiblk(fptr, nblock, 1, status) > 0) /* insert blocks */
                {
                  sprintf(message,
       "Failed to extend the size of the variable length heap by %ld blocks.",
                   nblock);
                   ffpmsg(message);
                   return(*status);
                }
            }
        }

        /* increment the address to the next empty heap position */
        (fptr->Fptr)->heapsize += ( *repeat * (*incre)); 
      }
      else    /*  get the read start position in the heap */
      {
        if ( firstrow > (fptr->Fptr)->numrows)
        {
            ffpmsg("Attempt to read past end of table");
            sprintf(message, 
                "  Table has %.0f rows and tried to read row %.0f.",
                (double) ((fptr->Fptr)->numrows), (double) firstrow);
            ffpmsg(message);
            return(*status = BAD_ROW_NUM);
        }

        ffgdesll(fptr, colnum, firstrow, &lrepeat, &heapoffset, status);
        *repeat = lrepeat;

        if (colptr->tdatatype <= -TCOMPLEX)
            *repeat = *repeat * 2;  /* no. of float or double values */
        else if (colptr->tdatatype == -TBIT)
            *repeat = (*repeat + 7) / 8;  /* convert from bits to bytes */

        if (*elemnum >= *repeat)
        {
            sprintf(message, 
         "Starting element to read in variable length column is too large: %ld",
                    (long) firstelem);
            ffpmsg(message);
            sprintf(message, 
         "  This row only contains %ld elements", (long) *repeat);
            ffpmsg(message);
            return(*status = BAD_ELEM_NUM);
        }

        *startpos = datastart + heapoffset + (fptr->Fptr)->heapstart;
      }
    }
    return(*status);
}
/*---------------------------------------------------------------------------*/
int fftheap(fitsfile *fptr, /* I - FITS file pointer                         */
           LONGLONG *heapsz,   /* O - current size of the heap               */
           LONGLONG *unused,   /* O - no. of unused bytes in the heap        */
           LONGLONG *overlap,  /* O - no. of bytes shared by > 1 descriptors */
           int  *valid,     /* O - are all the heap addresses valid?         */
           int *status)     /* IO - error status                             */
/*
  Tests the contents of the binary table variable length array heap.
  Returns the number of bytes that are currently not pointed to by any
  of the descriptors, and also the number of bytes that are pointed to
  by more than one descriptor.  It returns valid = FALSE if any of the
  descriptors point to addresses that are out of the bounds of the
  heap.
*/
{
    int jj, typecode, pixsize;
    long ii, kk, theapsz, nbytes;
    LONGLONG repeat, offset, tunused = 0, toverlap = 0;
    char *buffer, message[81];

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if ( fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* rescan header to make sure everything is up to date */
    else if ( ffrdef(fptr, status) > 0)               
        return(*status);

    if (valid) *valid = TRUE;
    if (heapsz) *heapsz = (fptr->Fptr)->heapsize;
    if (unused) *unused = 0;
    if (overlap) *overlap = 0;
    
    /* return if this is not a binary table HDU or if the heap is empty */
    if ( (fptr->Fptr)->hdutype != BINARY_TBL || (fptr->Fptr)->heapsize == 0 )
        return(*status);

    if ((fptr->Fptr)->heapsize > LONG_MAX) {
        ffpmsg("Heap is too big to test ( > 2**31 bytes). (fftheap)");
        return(*status = MEMORY_ALLOCATION);
    }

    theapsz = (long) (fptr->Fptr)->heapsize;
    buffer = calloc(1, theapsz);     /* allocate temp space */
    if (!buffer )
    {
        sprintf(message,"Failed to allocate buffer to test the heap");
        ffpmsg(message);
        return(*status = MEMORY_ALLOCATION);
    }

    /* loop over all cols */
    for (jj = 1; jj <= (fptr->Fptr)->tfield && *status <= 0; jj++)
    {
        ffgtcl(fptr, jj, &typecode, NULL, NULL, status);
        if (typecode > 0)
           continue;        /* ignore fixed length columns */

        pixsize = -typecode / 10;

        for (ii = 1; ii <= (fptr->Fptr)->numrows; ii++)
        {
            ffgdesll(fptr, jj, ii, &repeat, &offset, status);
            if (typecode == -TBIT)
                nbytes = (long) (repeat + 7) / 8;
            else
                nbytes = (long) repeat * pixsize;

            if (offset < 0 || offset + nbytes > theapsz)
            {
                if (valid) *valid = FALSE;  /* address out of bounds */
                sprintf(message,
                "Descriptor in row %ld, column %d has invalid heap address",
                ii, jj);
                ffpmsg(message);
            }
            else
            {
                for (kk = 0; kk < nbytes; kk++)
                    buffer[kk + offset]++;   /* increment every used byte */
            }
        }
    }

    for (kk = 0; kk < theapsz; kk++)
    {
        if (buffer[kk] == 0)
            tunused++;
        else if (buffer[kk] > 1)
            toverlap++;
    }

    if (heapsz) *heapsz = theapsz;
    if (unused) *unused = tunused;
    if (overlap) *overlap = toverlap;

    free(buffer);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffcmph(fitsfile *fptr,  /* I -FITS file pointer                         */
           int *status)     /* IO - error status                            */
/*
  compress the binary table heap by reordering the contents heap and
  recovering any unused space
*/
{
    fitsfile *tptr;
    int jj, typecode, pixsize, valid;
    long ii, buffsize = 10000, nblock, nbytes;
    LONGLONG  unused, overlap;
    LONGLONG repeat, offset;
    char *buffer, *tbuff, comm[FLEN_COMMENT];
    char message[81];
    LONGLONG pcount;
    LONGLONG readheapstart, writeheapstart, endpos, t1heapsize, t2heapsize;

    if (*status > 0)
        return(*status);

    /* get information about the current heap */
    fftheap(fptr, NULL, &unused, &overlap, &valid, status);

    if (!valid)
       return(*status = BAD_HEAP_PTR);  /* bad heap pointers */

    /* return if this is not a binary table HDU or if the heap is OK as is */
    if ( (fptr->Fptr)->hdutype != BINARY_TBL || (fptr->Fptr)->heapsize == 0 ||
         (unused == 0 && overlap == 0) || *status > 0 )
        return(*status);

    /* copy the current HDU to a temporary file in memory */
    if (ffinit( &tptr, "mem://tempheapfile", status) )
    {
        sprintf(message,"Failed to create temporary file for the heap");
        ffpmsg(message);
        return(*status);
    }
    if ( ffcopy(fptr, tptr, 0, status) )
    {
        sprintf(message,"Failed to create copy of the heap");
        ffpmsg(message);
        ffclos(tptr, status);
        return(*status);
    }

    buffer = (char *) malloc(buffsize);  /* allocate initial buffer */
    if (!buffer)
    {
        sprintf(message,"Failed to allocate buffer to copy the heap");
        ffpmsg(message);
        ffclos(tptr, status);
        return(*status = MEMORY_ALLOCATION);
    }
    
    readheapstart  = (tptr->Fptr)->datastart + (tptr->Fptr)->heapstart;
    writeheapstart = (fptr->Fptr)->datastart + (fptr->Fptr)->heapstart;

    t1heapsize = (fptr->Fptr)->heapsize;  /* save original heap size */
    (fptr->Fptr)->heapsize = 0;  /* reset heap to zero */

    /* loop over all cols */
    for (jj = 1; jj <= (fptr->Fptr)->tfield && *status <= 0; jj++)
    {
        ffgtcl(tptr, jj, &typecode, NULL, NULL, status);
        if (typecode > 0)
           continue;        /* ignore fixed length columns */

        pixsize = -typecode / 10;

        /* copy heap data, row by row */
        for (ii = 1; ii <= (fptr->Fptr)->numrows; ii++)
        {
            ffgdesll(tptr, jj, ii, &repeat, &offset, status);
            if (typecode == -TBIT)
                nbytes = (long) (repeat + 7) / 8;
            else
                nbytes = (long) repeat * pixsize;

            /* increase size of buffer if necessary to read whole array */
            if (nbytes > buffsize)
            {
                tbuff = realloc(buffer, nbytes);

                if (tbuff)
                {
                    buffer = tbuff;
                    buffsize = nbytes;
                }
                else
                    *status = MEMORY_ALLOCATION;
            }

            /* If this is not the last HDU in the file, then check if */
            /* extending the heap would overwrite the following header. */
            /* If so, then have to insert more blocks. */
            if ( !((fptr->Fptr)->lasthdu) )
            {
              endpos = writeheapstart + (fptr->Fptr)->heapsize + nbytes;

              if (endpos > (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1])
              {
                /* calc the number of blocks that need to be added */
                nblock = (long) (((endpos - 1 - 
                         (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1] ) 
                         / 2880) + 1);

                if (ffiblk(fptr, nblock, 1, status) > 0) /* insert blocks */
                {
                  sprintf(message,
       "Failed to extend the size of the variable length heap by %ld blocks.",
                   nblock);
                   ffpmsg(message);
                }
              }
            }

            /* read arrray of bytes from temporary copy */
            ffmbyt(tptr, readheapstart + offset, REPORT_EOF, status);
            ffgbyt(tptr, nbytes, buffer, status);

            /* write arrray of bytes back to original file */
            ffmbyt(fptr, writeheapstart + (fptr->Fptr)->heapsize, 
                    IGNORE_EOF, status);
            ffpbyt(fptr, nbytes, buffer, status);

            /* write descriptor */
            ffpdes(fptr, jj, ii, repeat, 
                   (fptr->Fptr)->heapsize, status);

            (fptr->Fptr)->heapsize += nbytes; /* update heapsize */

            if (*status > 0)
            {
               free(buffer);
               ffclos(tptr, status);
               return(*status);
            }
        }
    }

    free(buffer);
    ffclos(tptr, status);

    /* delete any empty blocks at the end of the HDU */
    nblock = (long) (( (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1] -
             (writeheapstart + (fptr->Fptr)->heapsize) ) / 2880);

    if (nblock > 0)
    {
       t2heapsize = (fptr->Fptr)->heapsize;  /* save new heap size */
       (fptr->Fptr)->heapsize = t1heapsize;  /* restore  original heap size */

       ffdblk(fptr, nblock, status);
       (fptr->Fptr)->heapsize = t2heapsize;  /* reset correct heap size */
    }

    /* update the PCOUNT value (size of heap) */
    ffmaky(fptr, 2, status);         /* reset to beginning of header */

    ffgkyjj(fptr, "PCOUNT", &pcount, comm, status);
    if ((fptr->Fptr)->heapsize != pcount)
    {
        ffmkyj(fptr, "PCOUNT", (fptr->Fptr)->heapsize, comm, status);
    }
    ffrdef(fptr, status);  /* rescan new HDU structure */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgdes(fitsfile *fptr, /* I - FITS file pointer                         */
           int colnum,     /* I - column number (1 = 1st column of table)   */
           LONGLONG rownum,    /* I - row number (1 = 1st row of table)         */
           long *length,   /* O - number of elements in the row             */
           long *heapaddr, /* O - heap pointer to the data                  */
           int *status)    /* IO - error status                             */
/*
  get (read) the variable length vector descriptor from the table.
*/
{
    LONGLONG lengthjj, heapaddrjj;
    
    if (ffgdesll(fptr, colnum, rownum, &lengthjj, &heapaddrjj, status) > 0)
        return(*status);

    /* convert the temporary 8-byte values to 4-byte values */
    /* check for overflow */
    if (length) {
        if (lengthjj > LONG_MAX)
	    *status = NUM_OVERFLOW;
	else
            *length = (long) lengthjj;
    }
    
    if (heapaddr) {
        if (heapaddrjj > LONG_MAX)
	    *status = NUM_OVERFLOW;
	else
            *heapaddr = (long) heapaddrjj;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgdesll(fitsfile *fptr,   /* I - FITS file pointer                       */
           int colnum,         /* I - column number (1 = 1st column of table) */
           LONGLONG rownum,        /* I - row number (1 = 1st row of table)       */
           LONGLONG *length,   /* O - number of elements in the row           */
           LONGLONG *heapaddr, /* O - heap pointer to the data                */
           int *status)        /* IO - error status                           */
/*
  get (read) the variable length vector descriptor from the binary table.
  This is similar to ffgdes, except it supports the full 8-byte range of the
  length and offset values in 'Q' columns, as well as 'P' columns.
*/
{
    LONGLONG bytepos;
    unsigned int descript4[2] = {0,0};
    LONGLONG descript8[2] = {0,0};
    tcolumn *colptr;

    if (*status > 0)
       return(*status);
       
    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    colptr = (fptr->Fptr)->tableptr;  /* point to first column structure */
    colptr += (colnum - 1);   /* offset to the correct column */

    if (colptr->tdatatype >= 0) {
        *status = NOT_VARI_LEN;
        return(*status);
    }

    bytepos = (fptr->Fptr)->datastart + 
                  ((fptr->Fptr)->rowlength * (rownum - 1)) +
                   colptr->tbcol;

    if (colptr->tform[0] == 'P' || colptr->tform[1] == 'P')
    {
        /* read 4-byte descriptor */
        if (ffgi4b(fptr, bytepos, 2, 4, (INT32BIT *) descript4, status) <= 0) 
        {
           if (length)
             *length = (LONGLONG) descript4[0];   /* 1st word is the length  */
           if (heapaddr)
             *heapaddr = (LONGLONG) descript4[1]; /* 2nd word is the address */
        }

    }
    else  /* this is for 'Q' columns */
    {
        /* read 8 byte descriptor */
        if (ffgi8b(fptr, bytepos, 2, 8, (long *) descript8, status) <= 0) 
        {
           if (length)
             *length = descript8[0];   /* 1st word is the length  */
           if (heapaddr)
             *heapaddr = descript8[1]; /* 2nd word is the address */
        }
    }     

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgdess(fitsfile *fptr, /* I - FITS file pointer                        */
           int colnum,     /* I - column number (1 = 1st column of table)   */
           LONGLONG firstrow,  /* I - first row  (1 = 1st row of table)         */
           LONGLONG nrows,     /* I - number or rows to read                    */
           long *length,   /* O - number of elements in the row             */
           long *heapaddr, /* O - heap pointer to the data                  */
           int *status)    /* IO - error status                             */
/*
  get (read) a range of variable length vector descriptors from the table.
*/
{
    LONGLONG rowsize, bytepos;
    long  ii;
    INT32BIT descript4[2] = {0,0};
    LONGLONG descript8[2] = {0,0};
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    colptr = (fptr->Fptr)->tableptr;  /* point to first column structure */
    colptr += (colnum - 1);   /* offset to the correct column */

    if (colptr->tdatatype >= 0) {
        *status = NOT_VARI_LEN;
        return(*status);
    }
    
    rowsize = (fptr->Fptr)->rowlength;
    bytepos = (fptr->Fptr)->datastart + 
                  (rowsize  * (firstrow - 1)) +
                  colptr->tbcol;

    if (colptr->tform[0] == 'P' || colptr->tform[1] == 'P')
    {
        /* read 4-byte descriptors */
        for (ii = 0; ii < nrows; ii++)
        {
	    /* read descriptors */
            if (ffgi4b(fptr, bytepos, 2, 4, descript4, status) <= 0)
	    { 
              if (length) {
                *length =   (long) descript4[0];   /* 1st word is the length  */
                length++;
	      }

              if (heapaddr) {
                *heapaddr = (long) descript4[1];   /* 2nd word is the address */
                heapaddr++;
	      }
              bytepos += rowsize;
	    }
	    else
	      return(*status);
        }
    }
    else  /* this is for 'Q' columns */
    {
        /* read 8-byte descriptors */
        for (ii = 0; ii < nrows; ii++)
        {
	    /* read descriptors */
            if (ffgi8b(fptr, bytepos, 2, 8, (long *) descript8, status) <= 0)
	    { 
              if (length) {
	        if (descript8[0] > LONG_MAX)*status = NUM_OVERFLOW;
                *length =   (long) descript8[0];   /* 1st word is the length  */
                length++;
	      }
              if (heapaddr) {
	        if (descript8[1] > LONG_MAX)*status = NUM_OVERFLOW;
                *heapaddr = (long) descript8[1];   /* 2nd word is the address */
                heapaddr++;
	      }
              bytepos += rowsize;
	    }
	    else
	      return(*status);
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgdessll(fitsfile *fptr, /* I - FITS file pointer                      */
           int colnum,     /* I - column number (1 = 1st column of table)   */
           LONGLONG firstrow,  /* I - first row  (1 = 1st row of table)         */
           LONGLONG nrows,     /* I - number or rows to read                    */
           LONGLONG *length,   /* O - number of elements in the row         */
           LONGLONG *heapaddr, /* O - heap pointer to the data              */
           int *status)    /* IO - error status                             */
/*
  get (read) a range of variable length vector descriptors from the table.
*/
{
    LONGLONG rowsize, bytepos;
    long  ii;
    unsigned int descript4[2] = {0,0};
    LONGLONG descript8[2] = {0,0};
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    colptr = (fptr->Fptr)->tableptr;  /* point to first column structure */
    colptr += (colnum - 1);           /* offset to the correct column */

    if (colptr->tdatatype >= 0) {
        *status = NOT_VARI_LEN;
        return(*status);
    }

    rowsize = (fptr->Fptr)->rowlength;
    bytepos = (fptr->Fptr)->datastart + 
                  (rowsize  * (firstrow - 1)) +
                  colptr->tbcol;

    if (colptr->tform[0] == 'P' || colptr->tform[1] == 'P')
    {
        /* read 4-byte descriptors */
        for (ii = 0; ii < nrows; ii++)
        {
	    /* read descriptors */
            if (ffgi4b(fptr, bytepos, 2, 4, (INT32BIT *) descript4, status) <= 0)
	    { 
              if (length) {
                *length =   (LONGLONG) descript4[0];   /* 1st word is the length  */
                length++;
	      }

              if (heapaddr) {
                *heapaddr = (LONGLONG) descript4[1];   /* 2nd word is the address */
                heapaddr++;
	      }
              bytepos += rowsize;
	    }
	    else
	      return(*status);
        }
    }
    else  /* this is for 'Q' columns */
    {
        /* read 8-byte descriptors */
        for (ii = 0; ii < nrows; ii++)
        {
	    /* read descriptors */
	    /* cast to type (long *) even though it is actually (LONGLONG *) */
            if (ffgi8b(fptr, bytepos, 2, 8, (long *) descript8, status) <= 0)
	    { 
              if (length) {
                *length =   descript8[0];   /* 1st word is the length  */
                length++;
	      }

              if (heapaddr) {
                *heapaddr = descript8[1];   /* 2nd word is the address */
                heapaddr++;
	      }
              bytepos += rowsize;
	    }
	    else
	      return(*status);
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpdes(fitsfile *fptr,  /* I - FITS file pointer                         */
           int colnum,      /* I - column number (1 = 1st column of table)   */
           LONGLONG rownum,     /* I - row number (1 = 1st row of table)         */
           LONGLONG length,    /* I - number of elements in the row             */
           LONGLONG heapaddr,  /* I - heap pointer to the data                  */
           int *status)     /* IO - error status                             */
/*
  put (write) the variable length vector descriptor to the table.
*/
{
    LONGLONG bytepos;
    unsigned int descript4[2];
    LONGLONG descript8[2];
    tcolumn *colptr;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    colptr = (fptr->Fptr)->tableptr;  /* point to first column structure */
    colptr += (colnum - 1);   /* offset to the correct column */

    if (colptr->tdatatype >= 0)
        *status = NOT_VARI_LEN;

    bytepos = (fptr->Fptr)->datastart + 
                  ((fptr->Fptr)->rowlength * (rownum - 1)) +
                  colptr->tbcol;

    ffmbyt(fptr, bytepos, IGNORE_EOF, status); /* move to element */

    if (colptr->tform[0] == 'P' || colptr->tform[1] == 'P')
    {
        if (length   > UINT_MAX || length   < 0 ||
            heapaddr > UINT_MAX || heapaddr < 0) {
            ffpmsg("P variable length column descriptor is out of range");
	    *status = NUM_OVERFLOW;
            return(*status);
        }
           
        descript4[0] = (unsigned int) length;   /* 1st word is the length  */
        descript4[1] = (unsigned int) heapaddr; /* 2nd word is the address */
 
        ffpi4b(fptr, 2, 4, (INT32BIT *) descript4, status); /* write the descriptor */
    }
    else /* this is a 'Q' descriptor column */
    {
        descript8[0] =  length;   /* 1st word is the length  */
        descript8[1] =  heapaddr; /* 2nd word is the address */
 
        ffpi8b(fptr, 2, 8, (long *) descript8, status); /* write the descriptor */
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffchdu(fitsfile *fptr,      /* I - FITS file pointer */
           int *status)         /* IO - error status     */
{
/*
  close the current HDU.  If we have write access to the file, then:
    - write the END keyword and pad header with blanks if necessary
    - check the data fill values, and rewrite them if not correct
*/
    char message[FLEN_ERRMSG];
    int ii, stdriver, ntilebins;

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
        /* no need to do any further updating of the HDU */
    }
    else if ((fptr->Fptr)->writemode == 1)
    {
        urltype2driver("stream://", &stdriver);

        /* don't rescan header in special case of writing to stdout */
        if (((fptr->Fptr)->driver != stdriver)) 
             ffrdef(fptr, status); 

        if ((fptr->Fptr)->heapsize > 0) {
          ffuptf(fptr, status);  /* update the variable length TFORM values */
        }
	
        ffpdfl(fptr, status);  /* insure correct data fill values */
    }

    if ((fptr->Fptr)->open_count == 1)
    {

    /* free memory for the CHDU structure only if no other files are using it */
        if ((fptr->Fptr)->tableptr)
        {
            free((fptr->Fptr)->tableptr);
           (fptr->Fptr)->tableptr = NULL;

          /* free the tile-compressed image cache, if it exists */
          if ((fptr->Fptr)->tilerow) {

           ntilebins = 
	    (((fptr->Fptr)->znaxis[0] - 1) / ((fptr->Fptr)->tilesize[0])) + 1;

           for (ii = 0; ii < ntilebins; ii++) {
             if ((fptr->Fptr)->tiledata[ii]) {
	       free((fptr->Fptr)->tiledata[ii]);
             }

             if ((fptr->Fptr)->tilenullarray[ii]) {
	       free((fptr->Fptr)->tilenullarray[ii]);
             }
            }
	    
	    free((fptr->Fptr)->tileanynull);
	    free((fptr->Fptr)->tiletype);	   
	    free((fptr->Fptr)->tiledatasize);
	    free((fptr->Fptr)->tilenullarray);
	    free((fptr->Fptr)->tiledata);
	    free((fptr->Fptr)->tilerow);

	    (fptr->Fptr)->tileanynull = 0;
	    (fptr->Fptr)->tiletype = 0;	   
	    (fptr->Fptr)->tiledatasize = 0;
	    (fptr->Fptr)->tilenullarray = 0;
	    (fptr->Fptr)->tiledata = 0;
	    (fptr->Fptr)->tilerow = 0;
          }
        }
    }

    if (*status > 0 && *status != NO_CLOSE_ERROR)
    {
        sprintf(message,
        "Error while closing HDU number %d (ffchdu).", (fptr->Fptr)->curhdu);
        ffpmsg(message);
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffuptf(fitsfile *fptr,      /* I - FITS file pointer */
           int *status)         /* IO - error status     */
/*
  Update the value of the TFORM keywords for the variable length array
  columns to make sure they all have the form 1Px(len) or Px(len) where
  'len' is the maximum length of the vector in the table (e.g., '1PE(400)')
*/
{
    int ii;
    long tflds;
    LONGLONG length, addr, maxlen, naxis2, jj;
    char comment[FLEN_COMMENT], keyname[FLEN_KEYWORD];
    char tform[FLEN_VALUE], newform[FLEN_VALUE], lenval[40];
    char card[FLEN_CARD];
    char message[FLEN_ERRMSG];
    char *tmp;

    ffmaky(fptr, 2, status);         /* reset to beginning of header */
    ffgkyjj(fptr, "NAXIS2", &naxis2, comment, status);
    ffgkyj(fptr, "TFIELDS", &tflds, comment, status);

    for (ii = 1; ii <= tflds; ii++)        /* loop over all the columns */
    {
      ffkeyn("TFORM", ii, keyname, status);          /* construct name */
      if (ffgkys(fptr, keyname, tform, comment, status) > 0)
      {
        sprintf(message,
        "Error while updating variable length vector TFORMn values (ffuptf).");
        ffpmsg(message);
        return(*status);
      }
      /* is this a variable array length column ? */
      if (tform[0] == 'P' || tform[1] == 'P' || tform[0] == 'Q' || tform[1] == 'Q')
      {
          /* get the max length */
          maxlen = 0;
          for (jj=1; jj <= naxis2; jj++)
          {
            ffgdesll(fptr, ii, jj, &length, &addr, status);

	    if (length > maxlen)
	         maxlen = length;
          }

          /* construct the new keyword value */
          strcpy(newform, "'");
          tmp = strchr(tform, '(');  /* truncate old length, if present */
          if (tmp) *tmp = 0;       
          strcat(newform, tform);

          /* print as double, because the string-to-64-bit */
          /* conversion is platform dependent (%lld, %ld, %I64d) */

          sprintf(lenval, "(%.0f)", (double) maxlen);

          strcat(newform,lenval);
          while(strlen(newform) < 9)
             strcat(newform," ");   /* append spaces 'till length = 8 */
          strcat(newform,"'" );     /* append closing parenthesis */
          /* would be simpler to just call ffmkyj here, but this */
          /* would force linking in all the modkey & putkey routines */
          ffmkky(keyname, newform, comment, card, status);  /* make new card */
          ffmkey(fptr, card, status);   /* replace last read keyword */
      }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffrdef(fitsfile *fptr,      /* I - FITS file pointer */
           int *status)         /* IO - error status     */
/*
  ReDEFine the structure of a data unit.  This routine re-reads
  the CHDU header keywords to determine the structure and length of the
  current data unit.  This redefines the start of the next HDU.
*/
{
    int dummy, tstatus = 0;
    LONGLONG naxis2;
    LONGLONG pcount;
    char card[FLEN_CARD], comm[FLEN_COMMENT], valstring[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
    else if ((fptr->Fptr)->writemode == 1) /* write access to the file? */
    {
        /* don't need to check NAXIS2 and PCOUNT if data hasn't been written */
        if ((fptr->Fptr)->datastart != DATA_UNDEFINED)
        {
          /* update NAXIS2 keyword if more rows were written to the table */
          /* and if the user has not explicitly reset the NAXIS2 value */
          if ((fptr->Fptr)->hdutype != IMAGE_HDU)
          {
            ffmaky(fptr, 2, status);
            if (ffgkyjj(fptr, "NAXIS2", &naxis2, comm, &tstatus) > 0)
            {
                /* Couldn't read NAXIS2 (odd!);  in certain circumstances */
                /* this may be normal, so ignore the error. */
                naxis2 = (fptr->Fptr)->numrows;
            }

            if ((fptr->Fptr)->numrows > naxis2
              && (fptr->Fptr)->origrows == naxis2)
              /* if origrows is not equal to naxis2, then the user must */
              /* have manually modified the NAXIS2 keyword value, and */
              /* we will assume that the current value is correct. */
            {
              /* would be simpler to just call ffmkyj here, but this */
              /* would force linking in all the modkey & putkey routines */

              /* print as double because the 64-bit int conversion */
              /* is platform dependent (%lld, %ld, %I64 )          */

              sprintf(valstring, "%.0f", (double) ((fptr->Fptr)->numrows));

              ffmkky("NAXIS2", valstring, comm, card, status);
              ffmkey(fptr, card, status);
            }
          }

          /* if data has been written to variable length columns in a  */
          /* binary table, then we may need to update the PCOUNT value */
          if ((fptr->Fptr)->heapsize > 0)
          {
            ffmaky(fptr, 2, status);
            ffgkyjj(fptr, "PCOUNT", &pcount, comm, status);
            if ((fptr->Fptr)->heapsize != pcount)
            {
              ffmkyj(fptr, "PCOUNT", (fptr->Fptr)->heapsize, comm, status);
            }
          }
        }

        if (ffwend(fptr, status) <= 0)     /* rewrite END keyword and fill */
        {
            ffrhdu(fptr, &dummy, status);  /* re-scan the header keywords  */
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffhdef(fitsfile *fptr,      /* I - FITS file pointer                    */
           int morekeys,        /* I - reserve space for this many keywords */
           int *status)         /* IO - error status                        */
/*
  based on the number of keywords which have already been written,
  plus the number of keywords to reserve space for, we then can
  define where the data unit should start (it must start at the
  beginning of a 2880-byte logical block).

  This routine will only have any effect if the starting location of the
  data unit following the header is not already defined.  In any case,
  it is always possible to add more keywords to the header even if the
  data has already been written.  It is just more efficient to reserve
  the space in advance.
*/
{
    LONGLONG delta;

    if (*status > 0 || morekeys < 1)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
    {
      ffrdef(fptr, status);

      /* ffrdef defines the offset to datastart and the start of */
      /* the next HDU based on the number of existing keywords. */
      /* We need to increment both of these values based on */
      /* the number of new keywords to be added.  */

      delta = (((fptr->Fptr)->headend + (morekeys * 80)) / 2880 + 1)
                                * 2880 - (fptr->Fptr)->datastart; 
              
      (fptr->Fptr)->datastart += delta;

      (fptr->Fptr)->headstart[ (fptr->Fptr)->curhdu + 1] += delta;

    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffwend(fitsfile *fptr,       /* I - FITS file pointer */
            int *status)         /* IO - error status     */
/*
  write the END card and following fill (space chars) in the current header
*/
{
    int ii, tstatus;
    LONGLONG endpos;
    long nspace;
    char blankkey[FLEN_CARD], endkey[FLEN_CARD], keyrec[FLEN_CARD] = "";

    if (*status > 0)
        return(*status);

    endpos = (fptr->Fptr)->headend;

    /* we assume that the HDUposition == curhdu in all cases */

    /*  calc the data starting position if not currently defined */
    if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        (fptr->Fptr)->datastart = ( endpos / 2880 + 1 ) * 2880;

    /* calculate the number of blank keyword slots in the header */
    nspace = (long) (( (fptr->Fptr)->datastart - endpos ) / 80);

    /* construct a blank and END keyword (80 spaces )  */
    strcpy(blankkey, "                                        ");
    strcat(blankkey, "                                        ");
    strcpy(endkey, "END                                     ");
    strcat(endkey, "                                        ");
  
    /* check if header is already correctly terminated with END and fill */
    tstatus=0;
    ffmbyt(fptr, endpos, REPORT_EOF, &tstatus); /* move to header end */
    for (ii=0; ii < nspace; ii++)
    {
        ffgbyt(fptr, 80, keyrec, &tstatus);  /* get next keyword */
        if (tstatus) break;
        if (strncmp(keyrec, blankkey, 80) && strncmp(keyrec, endkey, 80))
            break;
    }

    if (ii == nspace && !tstatus)
    {
        /* check if the END keyword exists at the correct position */
        endpos=maxvalue( endpos, ( (fptr->Fptr)->datastart - 2880 ) );
        ffmbyt(fptr, endpos, REPORT_EOF, &tstatus);  /* move to END position */
        ffgbyt(fptr, 80, keyrec, &tstatus); /* read the END keyword */
        if ( !strncmp(keyrec, endkey, 80) && !tstatus) {

            /* store this position, for later reference */
            (fptr->Fptr)->ENDpos = endpos;

            return(*status);    /* END card was already correct */
         }
    }

    /* header was not correctly terminated, so write the END and blank fill */
    endpos = (fptr->Fptr)->headend;
    ffmbyt(fptr, endpos, IGNORE_EOF, status); /* move to header end */
    for (ii=0; ii < nspace; ii++)
        ffpbyt(fptr, 80, blankkey, status);  /* write the blank keywords */

    /*
    The END keyword must either be placed immediately after the last
    keyword that was written (as indicated by the headend value), or
    must be in the first 80 bytes of the 2880-byte FITS record immediately 
    preceeding the data unit, whichever is further in the file. The
    latter will occur if space has been reserved for more header keywords
    which have not yet been written.
    */

    endpos=maxvalue( endpos, ( (fptr->Fptr)->datastart - 2880 ) );
    ffmbyt(fptr, endpos, REPORT_EOF, status);  /* move to END position */

    ffpbyt(fptr, 80, endkey, status); /*  write the END keyword to header */
    
    /* store this position, for later reference */
    (fptr->Fptr)->ENDpos = endpos;

    if (*status > 0)
        ffpmsg("Error while writing END card (ffwend).");

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpdfl(fitsfile *fptr,      /* I - FITS file pointer */
           int *status)         /* IO - error status     */
/*
  Write the Data Unit Fill values if they are not already correct.
  The fill values are used to fill out the last 2880 byte block of the HDU.
  Fill the data unit with zeros or blanks depending on the type of HDU
  from the end of the data to the end of the current FITS 2880 byte block
*/
{
    char chfill, fill[2880];
    LONGLONG fillstart;
    int nfill, tstatus, ii;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        return(*status);      /* fill has already been correctly written */

    if ((fptr->Fptr)->heapstart == 0)
        return(*status);      /* null data unit, so there is no fill */

    fillstart = (fptr->Fptr)->datastart + (fptr->Fptr)->heapstart +
                (fptr->Fptr)->heapsize;

    nfill = (long) ((fillstart + 2879) / 2880 * 2880 - fillstart);

    if ((fptr->Fptr)->hdutype == ASCII_TBL)
        chfill = 32;         /* ASCII tables are filled with spaces */
    else
        chfill = 0;          /* all other extensions are filled with zeros */

    tstatus = 0;

    if (!nfill)  /* no fill bytes; just check that entire table exists */
    {
        fillstart--;
        nfill = 1;
        ffmbyt(fptr, fillstart, REPORT_EOF, &tstatus); /* move to last byte */
        ffgbyt(fptr, nfill, fill, &tstatus);           /* get the last byte */

        if (tstatus == 0)
            return(*status);  /* no EOF error, so everything is OK */
    }
    else
    {
        ffmbyt(fptr, fillstart, REPORT_EOF, &tstatus); /* move to fill area */
        ffgbyt(fptr, nfill, fill, &tstatus);           /* get the fill bytes */

        if (tstatus == 0)
        {
            for (ii = 0; ii < nfill; ii++)
            {
                if (fill[ii] != chfill)
                    break;
            }

            if (ii == nfill)
                return(*status);   /* all the fill values were correct */
        }
    }

    /* fill values are incorrect or have not been written, so write them */

    memset(fill, chfill, nfill);  /* fill the buffer with the fill value */

    ffmbyt(fptr, fillstart, IGNORE_EOF, status); /* move to fill area */
    ffpbyt(fptr, nfill, fill, status); /* write the fill bytes */

    if (*status > 0)
        ffpmsg("Error writing Data Unit fill bytes (ffpdfl).");

    return(*status);
}
/**********************************************************************
   ffchfl : Check Header Fill values

      Check that the header unit is correctly filled with blanks from
      the END card to the end of the current FITS 2880-byte block

         Function parameters:
            fptr     Fits file pointer
            status   output error status

    Translated ftchfl into C by Peter Wilson, Oct. 1997
**********************************************************************/
int ffchfl( fitsfile *fptr, int *status)
{
   int nblank,i,gotend;
   LONGLONG endpos;
   char rec[FLEN_CARD];
   char *blanks="                                                                                ";  /*  80 spaces  */

   if( *status > 0 ) return (*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

   /*   calculate the number of blank keyword slots in the header  */

   endpos=(fptr->Fptr)->headend;
   nblank=(long) (((fptr->Fptr)->datastart-endpos)/80);

   /*   move the i/o pointer to the end of the header keywords   */

   ffmbyt(fptr,endpos,TRUE,status);

   /*   find the END card (there may be blank keywords perceeding it)   */

   gotend=FALSE;
   for(i=0;i<nblank;i++) {
      ffgbyt(fptr,80,rec,status);
      if( !strncmp(rec, "END     ", 8) ) {
         if( gotend ) {
            /*   There is a duplicate END record   */
            *status=BAD_HEADER_FILL;
            ffpmsg("Warning: Header fill area contains duplicate END card:");
         }
         gotend=TRUE;
         if( strncmp( rec+8, blanks+8, 72) ) {
            /*   END keyword has extra characters   */
            *status=END_JUNK;
            ffpmsg(
            "Warning: END keyword contains extraneous non-blank characters:");
         }
      } else if( gotend ) {
         if( strncmp( rec, blanks, 80 ) ) {
            /*   The fill area contains extraneous characters   */
            *status=BAD_HEADER_FILL;
            ffpmsg(
         "Warning: Header fill area contains extraneous non-blank characters:");
         }
      }

      if( *status > 0 ) {
         rec[FLEN_CARD - 1] = '\0';  /* make sure string is null terminated */
         ffpmsg(rec);
         return( *status );
      }
   }
   return( *status );
}

/**********************************************************************
   ffcdfl : Check Data Unit Fill values

      Check that the data unit is correctly filled with zeros or
      blanks from the end of the data to the end of the current
      FITS 2880 byte block

         Function parameters:
            fptr     Fits file pointer
            status   output error status

    Translated ftcdfl into C by Peter Wilson, Oct. 1997
**********************************************************************/
int ffcdfl( fitsfile *fptr, int *status)
{
   int nfill,i;
   LONGLONG filpos;
   char chfill,chbuff[2880];

   if( *status > 0 ) return( *status );

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

   /*   check if the data unit is null   */
   if( (fptr->Fptr)->heapstart==0 ) return( *status );

   /* calculate starting position of the fill bytes, if any */
   filpos = (fptr->Fptr)->datastart 
          + (fptr->Fptr)->heapstart 
          + (fptr->Fptr)->heapsize;

   /*   calculate the number of fill bytes   */
   nfill = (long) ((filpos + 2879) / 2880 * 2880 - filpos);
   if( nfill == 0 ) return( *status );

   /*   move to the beginning of the fill bytes   */
   ffmbyt(fptr, filpos, FALSE, status);

   if( ffgbyt(fptr, nfill, chbuff, status) > 0)
   {
      ffpmsg("Error reading data unit fill bytes (ffcdfl).");
      return( *status );
   }

   if( (fptr->Fptr)->hdutype==ASCII_TBL )
      chfill = 32;         /* ASCII tables are filled with spaces */
   else
      chfill = 0;          /* all other extensions are filled with zeros */
   
   /*   check for all zeros or blanks   */
   
   for(i=0;i<nfill;i++) {
      if( chbuff[i] != chfill ) {
         *status=BAD_DATA_FILL;
         if( (fptr->Fptr)->hdutype==ASCII_TBL )
            ffpmsg("Warning: remaining bytes following ASCII table data are not filled with blanks.");
         else
            ffpmsg("Warning: remaining bytes following data are not filled with zeros.");
         return( *status );
      }
   }
   return( *status );
}
/*--------------------------------------------------------------------------*/
int ffcrhd(fitsfile *fptr,      /* I - FITS file pointer */
           int *status)         /* IO - error status     */
/*
  CReate Header Data unit:  Create, initialize, and move the i/o pointer
  to a new extension appended to the end of the FITS file.
*/
{
    int  tstatus = 0;
    LONGLONG bytepos, *ptr;

    if (*status > 0)
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* If the current header is empty, we don't have to do anything */
    if ((fptr->Fptr)->headend == (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu] )
        return(*status);

    while (ffmrhd(fptr, 1, 0, &tstatus) == 0);  /* move to end of file */

    if ((fptr->Fptr)->maxhdu == (fptr->Fptr)->MAXHDU)
    {
        /* allocate more space for the headstart array */
        ptr = (LONGLONG*) realloc( (fptr->Fptr)->headstart,
                        ((fptr->Fptr)->MAXHDU + 1001) * sizeof(LONGLONG) );

        if (ptr == NULL)
           return (*status = MEMORY_ALLOCATION);
        else {
          (fptr->Fptr)->MAXHDU = (fptr->Fptr)->MAXHDU + 1000;
          (fptr->Fptr)->headstart = ptr;
        }
    }

    if (ffchdu(fptr, status) <= 0)  /* close the current HDU */
    {
      bytepos = (fptr->Fptr)->headstart[(fptr->Fptr)->maxhdu + 1]; /* last */
      ffmbyt(fptr, bytepos, IGNORE_EOF, status);  /* move file ptr to it */
      (fptr->Fptr)->maxhdu++;       /* increment the known number of HDUs */
      (fptr->Fptr)->curhdu = (fptr->Fptr)->maxhdu; /* set current HDU loc */
      fptr->HDUposition    = (fptr->Fptr)->maxhdu; /* set current HDU loc */
      (fptr->Fptr)->nextkey = bytepos;    /* next keyword = start of header */
      (fptr->Fptr)->headend = bytepos;          /* end of header */
      (fptr->Fptr)->datastart = DATA_UNDEFINED; /* start data unit undefined */

       /* any other needed resets */
       
       /* reset the dithering offset that may have been calculated for the */
       /* previous HDU back to the requested default value */
       (fptr->Fptr)->dither_seed = (fptr->Fptr)->request_dither_seed;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdblk(fitsfile *fptr,      /* I - FITS file pointer                    */
           long nblocks,        /* I - number of 2880-byte blocks to delete */
           int *status)         /* IO - error status                        */
/*
  Delete the specified number of 2880-byte blocks from the end
  of the CHDU by shifting all following extensions up this 
  number of blocks.
*/
{
    char buffer[2880];
    int tstatus, ii;
    LONGLONG readpos, writepos;

    if (*status > 0 || nblocks <= 0)
        return(*status);

    tstatus = 0;
    /* pointers to the read and write positions */

    readpos = (fptr->Fptr)->datastart + 
                   (fptr->Fptr)->heapstart + 
                   (fptr->Fptr)->heapsize;
    readpos = ((readpos + 2879) / 2880) * 2880; /* start of block */

/*  the following formula is wrong because the current data unit
    may have been extended without updating the headstart value
    of the following HDU.
    
    readpos = (fptr->Fptr)->headstart[((fptr->Fptr)->curhdu) + 1];
*/
    writepos = readpos - ((LONGLONG)nblocks * 2880);

    while ( !ffmbyt(fptr, readpos, REPORT_EOF, &tstatus) &&
            !ffgbyt(fptr, 2880L, buffer, &tstatus) )
    {
        ffmbyt(fptr, writepos, REPORT_EOF, status);
        ffpbyt(fptr, 2880L, buffer, status);

        if (*status > 0)
        {
           ffpmsg("Error deleting FITS blocks (ffdblk)");
           return(*status);
        }
        readpos  += 2880;  /* increment to next block to transfer */
        writepos += 2880;
    }

    /* now fill the last nblock blocks with zeros */
    memset(buffer, 0, 2880);
    ffmbyt(fptr, writepos, REPORT_EOF, status);

    for (ii = 0; ii < nblocks; ii++)
        ffpbyt(fptr, 2880L, buffer, status);

    /* move back before the deleted blocks, since they may be deleted */
    /*   and we do not want to delete the current active buffer */
    ffmbyt(fptr, writepos - 1, REPORT_EOF, status);

    /* truncate the file to the new size, if supported on this device */
    fftrun(fptr, writepos, status);

    /* recalculate the starting location of all subsequent HDUs */
    for (ii = (fptr->Fptr)->curhdu; ii <= (fptr->Fptr)->maxhdu; ii++)
         (fptr->Fptr)->headstart[ii + 1] -= ((LONGLONG)nblocks * 2880);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffghdt(fitsfile *fptr,      /* I - FITS file pointer             */
           int *exttype,        /* O - type of extension, 0, 1, or 2 */
                                /*  for IMAGE_HDU, ASCII_TBL, or BINARY_TBL */
           int *status)         /* IO - error status                 */
/*
  Return the type of the CHDU. This returns the 'logical' type of the HDU,
  not necessarily the physical type, so in the case of a compressed image
  stored in a binary table, this will return the type as an Image, not a
  binary table.
*/
{
    if (*status > 0)
        return(*status);

    if (fptr->HDUposition == 0 && (fptr->Fptr)->headend == 0) { 
         /* empty primary array is alway an IMAGE_HDU */
         *exttype = IMAGE_HDU;
    }
    else {
 
        /* reset position to the correct HDU if necessary */
        if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        {
            ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
        }
        else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        {
            /* rescan header if data structure is undefined */
            if ( ffrdef(fptr, status) > 0)               
                return(*status);
        }

        *exttype = (fptr->Fptr)->hdutype; /* return the type of HDU */

        /*  check if this is a compressed image */
        if ((fptr->Fptr)->compressimg)
            *exttype = IMAGE_HDU;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_is_reentrant(void)
/*
   Was CFITSIO compiled with the -D_REENTRANT flag?  1 = yes, 0 = no.
   Note that specifying the -D_REENTRANT flag is required, but may not be 
   sufficient, to ensure that CFITSIO can be safely used in a multi-threaded 
   environoment.
*/
{
#ifdef _REENTRANT
       return(1);
#else
       return(0);
#endif
}
/*--------------------------------------------------------------------------*/
int fits_is_compressed_image(fitsfile *fptr,  /* I - FITS file pointer  */
                 int *status)                 /* IO - error status      */
/*
   Returns TRUE if the CHDU is a compressed image, else returns zero.
*/
{
    if (*status > 0)
        return(0);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
    {
        /* rescan header if data structure is undefined */
        if ( ffrdef(fptr, status) > 0)               
            return(*status);
    }

    /*  check if this is a compressed image */
    if ((fptr->Fptr)->compressimg)
         return(1);

    return(0);
}
/*--------------------------------------------------------------------------*/
int ffgipr(fitsfile *infptr,   /* I - FITS file pointer                     */
        int maxaxis,           /* I - max number of axes to return          */
        int *bitpix,           /* O - image data type                       */
        int *naxis,            /* O - image dimension (NAXIS value)         */
        long *naxes,           /* O - size of image dimensions              */
        int *status)           /* IO - error status      */

/*
    get the datatype and size of the input image
*/
{

    if (*status > 0)
        return(*status);

    /* don't return the parameter if a null pointer was given */

    if (bitpix)
      fits_get_img_type(infptr, bitpix, status);  /* get BITPIX value */

    if (naxis)
      fits_get_img_dim(infptr, naxis, status);    /* get NAXIS value */

    if (naxes)
      fits_get_img_size(infptr, maxaxis, naxes, status); /* get NAXISn values */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgiprll(fitsfile *infptr,   /* I - FITS file pointer                   */
        int maxaxis,           /* I - max number of axes to return          */
        int *bitpix,           /* O - image data type                       */
        int *naxis,            /* O - image dimension (NAXIS value)         */
        LONGLONG *naxes,       /* O - size of image dimensions              */
        int *status)           /* IO - error status      */

/*
    get the datatype and size of the input image
*/
{

    if (*status > 0)
        return(*status);

    /* don't return the parameter if a null pointer was given */

    if (bitpix)
      fits_get_img_type(infptr, bitpix, status);  /* get BITPIX value */

    if (naxis)
      fits_get_img_dim(infptr, naxis, status);    /* get NAXIS value */

    if (naxes)
      fits_get_img_sizell(infptr, maxaxis, naxes, status); /* get NAXISn values */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgidt( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  *imgtype,   /* O - image data type                         */
            int  *status)    /* IO - error status                           */
/*
  Get the datatype of the image (= BITPIX keyword for normal image, or
  ZBITPIX for a compressed image)
*/
{
    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    /* reset to beginning of header */
    ffmaky(fptr, 1, status);  /* simply move to beginning of header */

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        ffgky(fptr, TINT, "BITPIX", imgtype, NULL, status);
    }
    else if ((fptr->Fptr)->compressimg)
    {
        /* this is a binary table containing a compressed image */
        ffgky(fptr, TINT, "ZBITPIX", imgtype, NULL, status);
    }
    else
    {
        *status = NOT_IMAGE;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgiet( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  *imgtype,   /* O - image data type                         */
            int  *status)    /* IO - error status                           */
/*
  Get the effective datatype of the image (= BITPIX keyword for normal image,
  or ZBITPIX for a compressed image)
*/
{
    int tstatus;
    long lngscale, lngzero = 0;
    double bscale, bzero, min_val, max_val;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    /* reset to beginning of header */
    ffmaky(fptr, 2, status);  /* simply move to beginning of header */

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        ffgky(fptr, TINT, "BITPIX", imgtype, NULL, status);
    }
    else if ((fptr->Fptr)->compressimg)
    {
        /* this is a binary table containing a compressed image */
        ffgky(fptr, TINT, "ZBITPIX", imgtype, NULL, status);
    }
    else
    {
        *status = NOT_IMAGE;
        return(*status);

    }

    /* check if the BSCALE and BZERO keywords are defined, which might
       change the effective datatype of the image  */
    tstatus = 0;
    ffgky(fptr, TDOUBLE, "BSCALE", &bscale, NULL, &tstatus);
    if (tstatus)
           bscale = 1.0;

    tstatus = 0;
    ffgky(fptr, TDOUBLE, "BZERO", &bzero, NULL, &tstatus);
    if (tstatus)
           bzero = 0.0;

    if (bscale == 1.0 && bzero == 0.0)  /* no scaling */
        return(*status);

    switch (*imgtype)
    {
      case BYTE_IMG:   /* 8-bit image */
        min_val = 0.;
        max_val = 255.0;
        break;

      case SHORT_IMG:
        min_val = -32768.0;
        max_val =  32767.0;
        break;
        
      case LONG_IMG:

        min_val = -2147483648.0;
        max_val =  2147483647.0;
        break;
        
      default:  /* don't have to deal with other data types */
        return(*status);
    }

    if (bscale >= 0.) {
        min_val = bzero + bscale * min_val;
        max_val = bzero + bscale * max_val;
    } else {
        max_val = bzero + bscale * min_val;
        min_val = bzero + bscale * max_val;
    }
    if (bzero < 2147483648.)  /* don't exceed range of 32-bit integer */
       lngzero = (long) bzero;
    lngscale = (long) bscale;

    if ((bzero != 2147483648.) && /* special value that exceeds integer range */
       (lngzero != bzero || lngscale != bscale)) { /* not integers? */
       /* floating point scaled values; just decide on required precision */
       if (*imgtype == BYTE_IMG || *imgtype == SHORT_IMG)
          *imgtype = FLOAT_IMG;
       else
         *imgtype = DOUBLE_IMG;

    /*
       In all the remaining cases, BSCALE and BZERO are integers,
       and not equal to 1 and 0, respectively.  
    */

    } else if ((min_val == -128.) && (max_val == 127.)) {
       *imgtype = SBYTE_IMG;

    } else if ((min_val >= -32768.0) && (max_val <= 32767.0)) {
       *imgtype = SHORT_IMG;

    } else if ((min_val >= 0.0) && (max_val <= 65535.0)) {
       *imgtype = USHORT_IMG;

    } else if ((min_val >= -2147483648.0) && (max_val <= 2147483647.0)) {
       *imgtype = LONG_IMG;

    } else if ((min_val >= 0.0) && (max_val < 4294967296.0)) {
       *imgtype = ULONG_IMG;

    } else {  /* exceeds the range of a 32-bit integer */
       *imgtype = DOUBLE_IMG;
    }   

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgidm( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  *naxis  ,   /* O - image dimension (NAXIS value)           */
            int  *status)    /* IO - error status                           */
/*
  Get the dimension of the image (= NAXIS keyword for normal image, or
  ZNAXIS for a compressed image)
  These values are cached for faster access.
*/
{
    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        *naxis = (fptr->Fptr)->imgdim;
    }
    else if ((fptr->Fptr)->compressimg)
    {
        *naxis = (fptr->Fptr)->zndim;
    }
    else
    {
        *status = NOT_IMAGE;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgisz( fitsfile *fptr,  /* I - FITS file pointer                       */
            int nlen,        /* I - number of axes to return                */
            long  *naxes,    /* O - size of image dimensions                */
            int  *status)    /* IO - error status                           */
/*
  Get the size of the image dimensions (= NAXISn keywords for normal image, or
  ZNAXISn for a compressed image)
  These values are cached for faster access.

*/
{
    int ii, naxis;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        naxis = minvalue((fptr->Fptr)->imgdim, nlen);
        for (ii = 0; ii < naxis; ii++)
        {
            naxes[ii] = (long) (fptr->Fptr)->imgnaxis[ii];
        }
    }
    else if ((fptr->Fptr)->compressimg)
    {
        naxis = minvalue( (fptr->Fptr)->zndim, nlen);
        for (ii = 0; ii < naxis; ii++)
        {
            naxes[ii] = (long) (fptr->Fptr)->znaxis[ii];
        }
    }
    else
    {
        *status = NOT_IMAGE;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgiszll( fitsfile *fptr,  /* I - FITS file pointer                     */
            int nlen,          /* I - number of axes to return              */
            LONGLONG  *naxes,  /* O - size of image dimensions              */
            int  *status)      /* IO - error status                         */
/*
  Get the size of the image dimensions (= NAXISn keywords for normal image, or
  ZNAXISn for a compressed image)
*/
{
    int ii, naxis;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);

    if ((fptr->Fptr)->hdutype == IMAGE_HDU)
    {
        naxis = minvalue((fptr->Fptr)->imgdim, nlen);
        for (ii = 0; ii < naxis; ii++)
        {
            naxes[ii] = (fptr->Fptr)->imgnaxis[ii];
        }
    }
    else if ((fptr->Fptr)->compressimg)
    {
        naxis = minvalue( (fptr->Fptr)->zndim, nlen);
        for (ii = 0; ii < naxis; ii++)
        {
            naxes[ii] = (fptr->Fptr)->znaxis[ii];
        }
    }
    else
    {
        *status = NOT_IMAGE;
    }

    return(*status);
}/*--------------------------------------------------------------------------*/
int ffmahd(fitsfile *fptr,      /* I - FITS file pointer             */
           int hdunum,          /* I - number of the HDU to move to  */
           int *exttype,        /* O - type of extension, 0, 1, or 2 */
           int *status)         /* IO - error status                 */
/*
  Move to Absolute Header Data unit.  Move to the specified HDU
  and read the header to initialize the table structure.  Note that extnum 
  is one based, so the primary array is extnum = 1.
*/
{
    int moveto, tstatus;
    char message[FLEN_ERRMSG];
    LONGLONG *ptr;

    if (*status > 0)
        return(*status);
    else if (hdunum < 1 )
        return(*status = BAD_HDU_NUM);
    else if (hdunum >= (fptr->Fptr)->MAXHDU )
    {
        /* allocate more space for the headstart array */
        ptr = (LONGLONG*) realloc( (fptr->Fptr)->headstart,
                        (hdunum + 1001) * sizeof(LONGLONG) ); 

        if (ptr == NULL)
           return (*status = MEMORY_ALLOCATION);
        else {
          (fptr->Fptr)->MAXHDU = hdunum + 1000; 
          (fptr->Fptr)->headstart = ptr;
        }
    }

    /* set logical HDU position to the actual position, in case they differ */
    fptr->HDUposition = (fptr->Fptr)->curhdu;

    while( ((fptr->Fptr)->curhdu) + 1 != hdunum) /* at the correct HDU? */
    {
        /* move directly to the extension if we know that it exists,
           otherwise move to the highest known extension.  */
        
        moveto = minvalue(hdunum - 1, ((fptr->Fptr)->maxhdu) + 1);

        /* test if HDU exists */
        if ((fptr->Fptr)->headstart[moveto] < (fptr->Fptr)->logfilesize )
        {
            if (ffchdu(fptr, status) <= 0)  /* close out the current HDU */
            {
                if (ffgext(fptr, moveto, exttype, status) > 0)
                {   /* failed to get the requested extension */

                    tstatus = 0;
                    ffrhdu(fptr, exttype, &tstatus); /* restore the CHDU */
                }
            }
        }
        else
            *status = END_OF_FILE;

        if (*status > 0)
        {
            if (*status != END_OF_FILE)
            {
                /* don't clutter up the message stack in the common case of */
                /* simply hitting the end of file (often an expected error) */

                sprintf(message,
                "Failed to move to HDU number %d (ffmahd).", hdunum);
                ffpmsg(message);
            }
            return(*status);
        }
    }

    /* return the type of HDU; tile compressed images which are stored */
    /* in a binary table will return exttype = IMAGE_HDU, not BINARY_TBL */
    if (exttype != NULL)
        ffghdt(fptr, exttype, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmrhd(fitsfile *fptr,      /* I - FITS file pointer                    */
           int hdumov,          /* I - rel. no. of HDUs to move by (+ or -) */ 
           int *exttype,        /* O - type of extension, 0, 1, or 2        */
           int *status)         /* IO - error status                        */
/*
  Move a Relative number of Header Data units.  Offset to the specified
  extension and read the header to initialize the HDU structure. 
*/
{
    int extnum;

    if (*status > 0)
        return(*status);

    extnum = fptr->HDUposition + 1 + hdumov;  /* the absolute HDU number */
    ffmahd(fptr, extnum, exttype, status);  /* move to the HDU */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffmnhd(fitsfile *fptr,      /* I - FITS file pointer                    */
           int exttype,         /* I - desired extension type               */
           char *hduname,       /* I - desired EXTNAME value for the HDU    */
           int hduver,          /* I - desired EXTVERS value for the HDU    */
           int *status)         /* IO - error status                        */
/*
  Move to the next HDU with a given extension type (IMAGE_HDU, ASCII_TBL,
  BINARY_TBL, or ANY_HDU), extension name (EXTNAME or HDUNAME keyword),
  and EXTVERS keyword values.  If hduvers = 0, then move to the first HDU
  with the given type and name regardless of EXTVERS value.  If no matching
  HDU is found in the file, then the current open HDU will remain unchanged.
*/
{
    char extname[FLEN_VALUE];
    int ii, hdutype, alttype, extnum, tstatus, match, exact;
    int slen, putback = 0, chopped = 0;
    long extver;

    if (*status > 0)
        return(*status);

    extnum = fptr->HDUposition + 1;  /* save the current HDU number */

    /*
       This is a kludge to deal with a special case where the
       user specified a hduname that ended with a # character, which
       CFITSIO previously interpreted as a flag to mean "don't copy any
       other HDUs in the file into the virtual file in memory.  If the
       remaining hduname does not end with a # character (meaning that
       the user originally entered a hduname ending in 2 # characters)
       then there is the possibility that the # character should be
       treated literally, if the actual EXTNAME also ends with a #.
       Setting putback = 1 means that we need to test for this case later on.
    */
        
    if ((fptr->Fptr)->only_one) {  /* if true, name orignally ended with a # */
       slen = strlen(hduname);
       if (hduname[slen - 1] != '#') /* This will fail if real EXTNAME value */
           putback = 1;              /*  ends with 2 # characters. */
    } 

    for (ii=1; 1; ii++)    /* loop over all HDUs until EOF */
    {
        tstatus = 0;
        if (ffmahd(fptr, ii, &hdutype, &tstatus))  /* move to next HDU */
        {
           ffmahd(fptr, extnum, 0, status); /* restore original file position */
           return(*status = BAD_HDU_NUM);   /* couldn't find desired HDU */
        }

        alttype = -1; 
        if (fits_is_compressed_image(fptr, status))
            alttype = BINARY_TBL;
        
        /* Does this HDU have a matching type? */
        if (exttype == ANY_HDU || hdutype == exttype || hdutype == alttype)
        {
          ffmaky(fptr, 2, status); /* reset to the 2nd keyword in the header */
          if (ffgkys(fptr, "EXTNAME", extname, 0, &tstatus) <= 0) /* get keyword */
          {
               if (putback) {          /* more of the kludge */
                   /* test if the EXTNAME value ends with a #;  if so, chop it  */
		   /* off before comparing the strings */
		   chopped = 0;
	           slen = strlen(extname);
		   if (extname[slen - 1] == '#') {
		       extname[slen - 1] = '\0'; 
                       chopped = 1;
                   }
               }

               /* see if the strings are an exact match */
               ffcmps(extname, hduname, CASEINSEN, &match, &exact);
          }

          /* if EXTNAME keyword doesn't exist, or it does not match, then try HDUNAME */
          if (tstatus || !exact)
	  {
               tstatus = 0;
               if (ffgkys(fptr, "HDUNAME", extname, 0, &tstatus) <= 0)
	       {
                   if (putback) {          /* more of the kludge */
		       chopped = 0;
	               slen = strlen(extname);
		       if (extname[slen - 1] == '#') {
		           extname[slen - 1] = '\0';  /* chop off the # */
                           chopped = 1;
                       }
                   }

                   /* see if the strings are an exact match */
                   ffcmps(extname, hduname, CASEINSEN, &match, &exact);
               }
          }

          if (!tstatus && exact)    /* found a matching name */
          {
             if (hduver)  /* need to check if version numbers match? */
             {
                if (ffgkyj(fptr, "EXTVER", &extver, 0, &tstatus) > 0)
                    extver = 1;  /* assume default EXTVER value */

                if ( (int) extver == hduver)
                {
                    if (chopped) {
                        /* The # was literally part of the name, not a flag */
	                (fptr->Fptr)->only_one = 0;  
                    }
                    return(*status);    /* found matching name and vers */
                }
             }
             else
             {
                 if (chopped) {
                     /* The # was literally part of the name, not a flag */
	            (fptr->Fptr)->only_one = 0;  
                 }
                 return(*status);    /* found matching name */
             }
          }  /* end of !tstatus && exact */

        }  /* end of matching HDU type */
    }  /* end of loop over HDUs */
}
/*--------------------------------------------------------------------------*/
int ffthdu(fitsfile *fptr,      /* I - FITS file pointer                    */
           int *nhdu,            /* O - number of HDUs in the file           */
           int *status)         /* IO - error status                        */
/*
  Return the number of HDUs that currently exist in the file.
*/
{
    int ii, extnum, tstatus;

    if (*status > 0)
        return(*status);

    extnum = fptr->HDUposition + 1;  /* save the current HDU number */
    *nhdu = extnum - 1;

    /* if the CHDU is empty or not completely defined, just return */
    if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        return(*status);

    tstatus = 0;

    /* loop until EOF */
    for (ii=extnum; ffmahd(fptr, ii, 0, &tstatus) <= 0; ii++)
    {
        *nhdu = ii;
    }

    ffmahd(fptr, extnum, 0, status);       /* restore orig file position */
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgext(fitsfile *fptr,      /* I - FITS file pointer                */
           int hdunum,          /* I - no. of HDU to move get (0 based) */ 
           int *exttype,        /* O - type of extension, 0, 1, or 2    */
           int *status)         /* IO - error status                    */
/*
  Get Extension.  Move to the specified extension and initialize the
  HDU structure.
*/
{
    int xcurhdu, xmaxhdu;
    LONGLONG xheadend;

    if (*status > 0)
        return(*status);

    if (ffmbyt(fptr, (fptr->Fptr)->headstart[hdunum], REPORT_EOF, status) <= 0)
    {
        /* temporarily save current values, in case of error */
        xcurhdu = (fptr->Fptr)->curhdu;
        xmaxhdu = (fptr->Fptr)->maxhdu;
        xheadend = (fptr->Fptr)->headend;

        /* set new parameter values */
        (fptr->Fptr)->curhdu = hdunum;
        fptr->HDUposition    = hdunum;
        (fptr->Fptr)->maxhdu = maxvalue((fptr->Fptr)->maxhdu, hdunum);
        (fptr->Fptr)->headend = (fptr->Fptr)->logfilesize; /* set max size */

        if (ffrhdu(fptr, exttype, status) > 0)
        {   /* failed to get the new HDU, so restore previous values */
            (fptr->Fptr)->curhdu = xcurhdu;
            fptr->HDUposition    = xcurhdu;
            (fptr->Fptr)->maxhdu = xmaxhdu;
            (fptr->Fptr)->headend = xheadend;
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffiblk(fitsfile *fptr,      /* I - FITS file pointer               */
           long nblock,         /* I - no. of blocks to insert         */ 
           int headdata,        /* I - insert where? 0=header, 1=data  */
                                /*     -1=beginning of file            */
           int *status)         /* IO - error status                   */
/*
   insert 2880-byte blocks at the end of the current header or data unit
*/
{
    int tstatus, savehdu, typhdu;
    LONGLONG insertpt, jpoint;
    long ii, nshift;
    char charfill;
    char buff1[2880], buff2[2880];
    char *inbuff, *outbuff, *tmpbuff;
    char card[FLEN_CARD];

    if (*status > 0 || nblock <= 0)
        return(*status);
        
    tstatus = *status;

    if (headdata == 0 || (fptr->Fptr)->hdutype == ASCII_TBL)
        charfill = 32;  /* headers and ASCII tables have space (32) fill */
    else
        charfill = 0;   /* images and binary tables have zero fill */

    if (headdata == 0)  
        insertpt = (fptr->Fptr)->datastart;  /* insert just before data, or */
    else if (headdata == -1)
    {
        insertpt = 0;
        strcpy(card, "XTENSION= 'IMAGE   '          / IMAGE extension");
    }
    else                                     /* at end of data, */
    {
        insertpt = (fptr->Fptr)->datastart + 
                   (fptr->Fptr)->heapstart + 
                   (fptr->Fptr)->heapsize;
        insertpt = ((insertpt + 2879) / 2880) * 2880; /* start of block */

       /* the following formula is wrong because the current data unit
          may have been extended without updating the headstart value
          of the following HDU.
       */
       /* insertpt = (fptr->Fptr)->headstart[(fptr->Fptr)->curhdu + 1]; */
    }

    inbuff  = buff1;   /* set pointers to input and output buffers */
    outbuff = buff2;

    memset(outbuff, charfill, 2880); /* initialize buffer with fill */

    if (nblock == 1)  /* insert one block */
    {
      if (headdata == -1)
        ffmrec(fptr, 1, card, status);    /* change SIMPLE -> XTENSION */

      ffmbyt(fptr, insertpt, REPORT_EOF, status);  /* move to 1st point */
      ffgbyt(fptr, 2880, inbuff, status);  /* read first block of bytes */

      while (*status <= 0)
      {
        ffmbyt(fptr, insertpt, REPORT_EOF, status);  /* insert point */
        ffpbyt(fptr, 2880, outbuff, status);  /* write the output buffer */

        if (*status > 0)
            return(*status);

        tmpbuff = inbuff;   /* swap input and output pointers */
        inbuff = outbuff;
        outbuff = tmpbuff;
        insertpt += 2880;  /* increment insert point by 1 block */

        ffmbyt(fptr, insertpt, REPORT_EOF, status);  /* move to next block */
        ffgbyt(fptr, 2880, inbuff, status);  /* read block of bytes */
      }

      *status = tstatus;  /* reset status value */
      ffmbyt(fptr, insertpt, IGNORE_EOF, status); /* move back to insert pt */
      ffpbyt(fptr, 2880, outbuff, status);  /* write the final block */
    }

    else   /*  inserting more than 1 block */

    {
        savehdu = (fptr->Fptr)->curhdu;  /* save the current HDU number */
        tstatus = *status;
        while(*status <= 0)  /* find the last HDU in file */
              ffmrhd(fptr, 1, &typhdu, status);

        if (*status == END_OF_FILE)
        {
            *status = tstatus;
        }

        ffmahd(fptr, savehdu + 1, &typhdu, status);  /* move back to CHDU */
        if (headdata == -1)
          ffmrec(fptr, 1, card, status); /* NOW change SIMPLE -> XTENSION */

        /* number of 2880-byte blocks that have to be shifted down */
        nshift = (long) (((fptr->Fptr)->headstart[(fptr->Fptr)->maxhdu + 1] - insertpt)
                 / 2880);
        /* position of last block in file to be shifted */
        jpoint =  (fptr->Fptr)->headstart[(fptr->Fptr)->maxhdu + 1] - 2880;

        /* move all the blocks starting at end of file working backwards */
        for (ii = 0; ii < nshift; ii++)
        {
            /* move to the read start position */
            if (ffmbyt(fptr, jpoint, REPORT_EOF, status) > 0)
                return(*status);

            ffgbyt(fptr, 2880, inbuff,status);  /* read one record */

            /* move forward to the write postion */
            ffmbyt(fptr, jpoint + ((LONGLONG) nblock * 2880), IGNORE_EOF, status);

            ffpbyt(fptr, 2880, inbuff, status);  /* write the record */

            jpoint -= 2880;
        }

        /* move back to the write start postion (might be EOF) */
        ffmbyt(fptr, insertpt, IGNORE_EOF, status);

        for (ii = 0; ii < nblock; ii++)   /* insert correct fill value */
             ffpbyt(fptr, 2880, outbuff, status);
    }

    if (headdata == 0)         /* update data start address */
      (fptr->Fptr)->datastart += ((LONGLONG) nblock * 2880);

    /* update following HDU addresses */
    for (ii = (fptr->Fptr)->curhdu; ii <= (fptr->Fptr)->maxhdu; ii++)
         (fptr->Fptr)->headstart[ii + 1] += ((LONGLONG) nblock * 2880);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgkcl(char *tcard)

/*
   Return the type classification of the input header record

   TYP_STRUC_KEY: SIMPLE, BITPIX, NAXIS, NAXISn, EXTEND, BLOCKED,
                  GROUPS, PCOUNT, GCOUNT, END
                  XTENSION, TFIELDS, TTYPEn, TBCOLn, TFORMn, THEAP,
                   and the first 4 COMMENT keywords in the primary array
                   that define the FITS format.

   TYP_CMPRS_KEY:
            The keywords used in the compressed image format
                  ZIMAGE, ZCMPTYPE, ZNAMEn, ZVALn, ZTILEn, 
                  ZBITPIX, ZNAXISn, ZSCALE, ZZERO, ZBLANK,
                  EXTNAME = 'COMPRESSED_IMAGE'
		  ZSIMPLE, ZTENSION, ZEXTEND, ZBLOCKED, ZPCOUNT, ZGCOUNT
		  ZQUANTIZ, ZDITHER0

   TYP_SCAL_KEY:  BSCALE, BZERO, TSCALn, TZEROn

   TYP_NULL_KEY:  BLANK, TNULLn

   TYP_DIM_KEY:   TDIMn

   TYP_RANG_KEY:  TLMINn, TLMAXn, TDMINn, TDMAXn, DATAMIN, DATAMAX

   TYP_UNIT_KEY:  BUNIT, TUNITn

   TYP_DISP_KEY:  TDISPn

   TYP_HDUID_KEY: EXTNAME, EXTVER, EXTLEVEL, HDUNAME, HDUVER, HDULEVEL

   TYP_CKSUM_KEY  CHECKSUM, DATASUM

   TYP_WCS_KEY:
           Primary array:
                  WCAXES, CTYPEn, CUNITn, CRVALn, CRPIXn, CROTAn, CDELTn
                  CDj_is, PVj_ms, LONPOLEs, LATPOLEs
  
           Pixel list:
                  TCTYPn, TCTYns, TCUNIn, TCUNns, TCRVLn, TCRVns, TCRPXn, TCRPks,
                  TCDn_k, TCn_ks, TPVn_m, TPn_ms, TCDLTn, TCROTn

           Bintable vector:
                  jCTYPn, jCTYns, jCUNIn, jCUNns, jCRVLn, jCRVns, iCRPXn, iCRPns,
                  jiCDn, jiCDns, jPVn_m, jPn_ms, jCDLTn, jCROTn
                
   TYP_REFSYS_KEY:
                   EQUINOXs, EPOCH, MJD-OBSs, RADECSYS, RADESYSs

   TYP_COMM_KEY:  COMMENT, HISTORY, (blank keyword)

   TYP_CONT_KEY:  CONTINUE

   TYP_USER_KEY:  all other keywords

*/ 
{
    char card[20], *card1, *card5;

    card[0] = '\0';
    strncat(card, tcard, 8);   /* copy the keyword name */
    strcat(card, "        "); /* append blanks to make at least 8 chars long */
    ffupch(card);  /* make sure it is in upper case */

    card1 = card + 1;  /* pointer to 2nd character */
    card5 = card + 5;  /* pointer to 6th character */

    /* the strncmp function is slow, so try to be more efficient */
    if (*card == 'Z')
    {
	if (FSTRNCMP (card1, "IMAGE  ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "CMPTYPE", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "NAME", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_CMPRS_KEY);
        }
	else if (FSTRNCMP (card1, "VAL", 3) == 0)
        {
            if (*(card + 4) >= '0' && *(card + 4) <= '9')
	        return (TYP_CMPRS_KEY);
        }
	else if (FSTRNCMP (card1, "TILE", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_CMPRS_KEY);
        }
	else if (FSTRNCMP (card1, "BITPIX ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "NAXIS", 5) == 0)
        {
            if ( ( *(card + 6) >= '0' && *(card + 6) <= '9' )
             || (*(card + 6) == ' ') )
	        return (TYP_CMPRS_KEY);
        }
	else if (FSTRNCMP (card1, "SCALE  ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "ZERO   ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "BLANK  ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "SIMPLE ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "TENSION", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "EXTEND ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "BLOCKED", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "PCOUNT ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "GCOUNT ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "QUANTIZ", 7) == 0)
	    return (TYP_CMPRS_KEY);
	else if (FSTRNCMP (card1, "DITHER0", 7) == 0)
	    return (TYP_CMPRS_KEY);
    }
    else if (*card == ' ')
    {
	return (TYP_COMM_KEY);
    }
    else if (*card == 'B')
    {
	if (FSTRNCMP (card1, "ITPIX  ", 7) == 0)
	    return (TYP_STRUC_KEY);
	if (FSTRNCMP (card1, "LOCKED ", 7) == 0)
	    return (TYP_STRUC_KEY);

	if (FSTRNCMP (card1, "LANK   ", 7) == 0)
	    return (TYP_NULL_KEY);

	if (FSTRNCMP (card1, "SCALE  ", 7) == 0)
	    return (TYP_SCAL_KEY);
	if (FSTRNCMP (card1, "ZERO   ", 7) == 0)
	    return (TYP_SCAL_KEY);

	if (FSTRNCMP (card1, "UNIT   ", 7) == 0)
	    return (TYP_UNIT_KEY);
    }
    else if (*card == 'C')
    {
	if (FSTRNCMP (card1, "OMMENT",6) == 0)
	{
          /* new comment string starting Oct 2001 */
	    if (FSTRNCMP (tcard, "COMMENT   and Astrophysics', volume 376, page 3",
              47) == 0)
	        return (TYP_STRUC_KEY);

         /* original COMMENT strings from 1993 - 2001 */
	    if (FSTRNCMP (tcard, "COMMENT   FITS (Flexible Image Transport System",
              47) == 0)
	        return (TYP_STRUC_KEY);
	    if (FSTRNCMP (tcard, "COMMENT   Astrophysics Supplement Series v44/p3",
              47) == 0)
	        return (TYP_STRUC_KEY);
	    if (FSTRNCMP (tcard, "COMMENT   Contact the NASA Science Office of St",
              47) == 0)
	        return (TYP_STRUC_KEY);
	    if (FSTRNCMP (tcard, "COMMENT   FITS Definition document #100 and oth",
              47) == 0)
	        return (TYP_STRUC_KEY);

            if (*(card + 7) == ' ')
	        return (TYP_COMM_KEY);
            else
                return (TYP_USER_KEY);
	}

	if (FSTRNCMP (card1, "HECKSUM", 7) == 0)
	    return (TYP_CKSUM_KEY);

	if (FSTRNCMP (card1, "ONTINUE", 7) == 0)
	    return (TYP_CONT_KEY);

	if (FSTRNCMP (card1, "TYPE",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "UNIT",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "RVAL",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "RPIX",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "ROTA",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "RDER",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "SYER",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "DELT",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (*card1 == 'D')
        {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
        }
    }
    else if (*card == 'D')
    {
	if (FSTRNCMP (card1, "ATASUM ", 7) == 0)
	    return (TYP_CKSUM_KEY);
	if (FSTRNCMP (card1, "ATAMIN ", 7) == 0)
	    return (TYP_RANG_KEY);
	if (FSTRNCMP (card1, "ATAMAX ", 7) == 0)
	    return (TYP_RANG_KEY);
	if (FSTRNCMP (card1, "ATE-OBS", 7) == 0)
	    return (TYP_REFSYS_KEY);    }
    else if (*card == 'E')
    {
	if (FSTRNCMP (card1, "XTEND  ", 7) == 0)
	    return (TYP_STRUC_KEY);
	if (FSTRNCMP (card1, "ND     ", 7) == 0)
	    return (TYP_STRUC_KEY);
	if (FSTRNCMP (card1, "XTNAME ", 7) == 0)
	{
            /* check for special compressed image value */
            if (FSTRNCMP(tcard, "EXTNAME = 'COMPRESSED_IMAGE'", 28) == 0)
	      return (TYP_CMPRS_KEY);
            else
	      return (TYP_HDUID_KEY);
	}
	if (FSTRNCMP (card1, "XTVER  ", 7) == 0)
	    return (TYP_HDUID_KEY);
	if (FSTRNCMP (card1, "XTLEVEL", 7) == 0)
	    return (TYP_HDUID_KEY);

	if (FSTRNCMP (card1, "QUINOX", 6) == 0)
	    return (TYP_REFSYS_KEY);
	if (FSTRNCMP (card1, "QUI",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_REFSYS_KEY);
        }
	if (FSTRNCMP (card1, "POCH   ", 7) == 0)
	    return (TYP_REFSYS_KEY);
    }
    else if (*card == 'G')
    {
	if (FSTRNCMP (card1, "COUNT  ", 7) == 0)
	    return (TYP_STRUC_KEY);
	if (FSTRNCMP (card1, "ROUPS  ", 7) == 0)
	    return (TYP_STRUC_KEY);
    }
    else if (*card == 'H')
    {
	if (FSTRNCMP (card1, "DUNAME ", 7) == 0)
	    return (TYP_HDUID_KEY);
	if (FSTRNCMP (card1, "DUVER  ", 7) == 0)
	    return (TYP_HDUID_KEY);
	if (FSTRNCMP (card1, "DULEVEL", 7) == 0)
	    return (TYP_HDUID_KEY);

	if (FSTRNCMP (card1, "ISTORY",6) == 0)
        {
            if (*(card + 7) == ' ')
	        return (TYP_COMM_KEY);
            else
                return (TYP_USER_KEY);
        }
    }
    else if (*card == 'L')
    {
	if (FSTRNCMP (card1, "ONPOLE",6) == 0)
	    return (TYP_WCS_KEY);
	if (FSTRNCMP (card1, "ATPOLE",6) == 0)
	    return (TYP_WCS_KEY);
	if (FSTRNCMP (card1, "ONP",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "ATP",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
    }
    else if (*card == 'M')
    {
	if (FSTRNCMP (card1, "JD-OBS ", 7) == 0)
	    return (TYP_REFSYS_KEY);
	if (FSTRNCMP (card1, "JDOB",4) == 0)
        {
            if (*(card+5) >= '0' && *(card+5) <= '9')
	        return (TYP_REFSYS_KEY);
        }
    }
    else if (*card == 'N')
    {
	if (FSTRNCMP (card1, "AXIS", 4) == 0)
        {
            if ((*card5 >= '0' && *card5 <= '9')
             || (*card5 == ' '))
	        return (TYP_STRUC_KEY);
        }
    }
    else if (*card == 'P')
    {
	if (FSTRNCMP (card1, "COUNT  ", 7) == 0)
	    return (TYP_STRUC_KEY);
	if (*card1 == 'C')
        {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (*card1 == 'V')
        {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (*card1 == 'S')
        {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
        }
    }
    else if (*card == 'R')
    {
	if (FSTRNCMP (card1, "ADECSYS", 7) == 0)
	    return (TYP_REFSYS_KEY);
	if (FSTRNCMP (card1, "ADESYS", 6) == 0)
	    return (TYP_REFSYS_KEY);
	if (FSTRNCMP (card1, "ADE",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_REFSYS_KEY);
        }
    }
    else if (*card == 'S')
    {
	if (FSTRNCMP (card1, "IMPLE  ", 7) == 0)
	    return (TYP_STRUC_KEY);
    }
    else if (*card == 'T')
    {
	if (FSTRNCMP (card1, "TYPE", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_STRUC_KEY);
        }
	else if (FSTRNCMP (card1, "FORM", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_STRUC_KEY);
        }
	else if (FSTRNCMP (card1, "BCOL", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_STRUC_KEY);
        }
	else if (FSTRNCMP (card1, "FIELDS ", 7) == 0)
	    return (TYP_STRUC_KEY);
	else if (FSTRNCMP (card1, "HEAP   ", 7) == 0)
	    return (TYP_STRUC_KEY);

	else if (FSTRNCMP (card1, "NULL", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_NULL_KEY);
        }

	else if (FSTRNCMP (card1, "DIM", 3) == 0)
        {
            if (*(card + 4) >= '0' && *(card + 4) <= '9')
 	        return (TYP_DIM_KEY);
        }

	else if (FSTRNCMP (card1, "UNIT", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_UNIT_KEY);
        }

	else if (FSTRNCMP (card1, "DISP", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_DISP_KEY);
        }

	else if (FSTRNCMP (card1, "SCAL", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_SCAL_KEY);
        }
	else if (FSTRNCMP (card1, "ZERO", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_SCAL_KEY);
        }

	else if (FSTRNCMP (card1, "LMIN", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_RANG_KEY);
        }
	else if (FSTRNCMP (card1, "LMAX", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_RANG_KEY);
        }
	else if (FSTRNCMP (card1, "DMIN", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_RANG_KEY);
        }
	else if (FSTRNCMP (card1, "DMAX", 4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_RANG_KEY);
        }

	else if (FSTRNCMP (card1, "CTYP",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CTY",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CUNI",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CUN",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRVL",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRV",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRPX",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRP",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CROT",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CDLT",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CDE",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRD",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CSY",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "WCS",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "C",1) == 0)
        {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "P",1) == 0)
        {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "V",1) == 0)
        {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "S",1) == 0)
        {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
        }
    }
    else if (*card == 'X')
    {
	if (FSTRNCMP (card1, "TENSION", 7) == 0)
	    return (TYP_STRUC_KEY);
    }
    else if (*card == 'W')
    {
	if (FSTRNCMP (card1, "CSAXES", 6) == 0)
	    return (TYP_WCS_KEY);
	if (FSTRNCMP (card1, "CSNAME", 6) == 0)
	    return (TYP_WCS_KEY);
	if (FSTRNCMP (card1, "CAX", 3) == 0)
	{
            if (*(card + 4) >= '0' && *(card + 4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CSN", 3) == 0)
	{
            if (*(card + 4) >= '0' && *(card + 4) <= '9')
	        return (TYP_WCS_KEY);
        }
    }
    
    else if (*card >= '0' && *card <= '9')
    {
      if (*card1 == 'C')
      {
        if (FSTRNCMP (card1, "CTYP",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CTY",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CUNI",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CUN",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRVL",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRV",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRPX",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRP",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CROT",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CDLT",4) == 0)
        {
            if (*card5 >= '0' && *card5 <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CDE",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CRD",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
	else if (FSTRNCMP (card1, "CSY",3) == 0)
        {
            if (*(card+4) >= '0' && *(card+4) <= '9')
	        return (TYP_WCS_KEY);
        }
      }
      else if (FSTRNCMP (card1, "V",1) == 0)
      {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
      }
      else if (FSTRNCMP (card1, "S",1) == 0)
      {
            if (*(card + 2) >= '0' && *(card + 2) <= '9')
	        return (TYP_WCS_KEY);
      }
      else if (*card1 >= '0' && *card1 <= '9')
      {   /* 2 digits at beginning of keyword */
	
	    if ( (*(card + 2) == 'P') && (*(card + 3) == 'C') )
	    {
               if (*(card + 4) >= '0' && *(card + 4) <= '9')
	        return (TYP_WCS_KEY);  /*  ijPCn keyword */
            }
	    else if ( (*(card + 2) == 'C') && (*(card + 3) == 'D') )
	    {
               if (*(card + 4) >= '0' && *(card + 4) <= '9')
	        return (TYP_WCS_KEY);  /*  ijCDn keyword */
            }
      }
      
    }
    
    return (TYP_USER_KEY);  /* by default all others are user keywords */
}
/*--------------------------------------------------------------------------*/
int ffdtyp(const char *cval,  /* I - formatted string representation of the value */
           char *dtype, /* O - datatype code: C, L, F, I, or X */
          int *status)  /* IO - error status */
/*
  determine implicit datatype of input string.
  This assumes that the string conforms to the FITS standard
  for keyword values, so may not detect all invalid formats.
*/
{

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (cval[0] == '\0')
        return(*status = VALUE_UNDEFINED);
    else if (cval[0] == '\'')
        *dtype = 'C';          /* character string starts with a quote */
    else if (cval[0] == 'T' || cval[0] == 'F')
        *dtype = 'L';          /* logical = T or F character */
    else if (cval[0] == '(')
        *dtype = 'X';          /* complex datatype "(1.2, -3.4)" */
    else if (strchr(cval,'.'))
        *dtype = 'F';          /* float usualy contains a decimal point */
    else if (strchr(cval,'E') || strchr(cval,'D') )
        *dtype = 'F';          /* exponential contains a E or D */
    else
        *dtype = 'I';          /* if none of the above assume it is integer */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffinttyp(char *cval,  /* I - formatted string representation of the integer */
           int *dtype, /* O - datatype code: TBYTE, TSHORT, TUSHORT, etc */
           int *negative, /* O - is cval negative? */
           int *status)  /* IO - error status */
/*
  determine implicit datatype of input integer string.
  This assumes that the string conforms to the FITS standard
  for integer keyword value, so may not detect all invalid formats.
*/
{
    int ii, len;
    char *p;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    *dtype = 0;  /* initialize to NULL */
    *negative = 0;
    p = cval;

    if (*p == '+') {
        p++;   /* ignore leading + sign */
    } else if (*p == '-') {
        p++;
	*negative = 1;   /* this is a negative number */
    }

    if (*p == '0') {
        while (*p == '0') p++;  /* skip leading zeros */

        if (*p == 0) {  /* the value is a string of 1 or more zeros */
           *dtype  = TSBYTE;
	   return(*status);
        }
    }

    len = strlen(p);
    for (ii = 0; ii < len; ii++)  {
        if (!isdigit(*(p+ii))) {
	    *status = BAD_INTKEY;
	    return(*status);
	}
    }

    /* check for unambiguous cases, based on length of the string */
    if (len == 0) {
        *status = VALUE_UNDEFINED;
    } else if (len < 3) {
        *dtype = TSBYTE;
    } else if (len == 4) {
	*dtype = TSHORT;
    } else if (len > 5 && len < 10) {
        *dtype = TINT;
    } else if (len > 10 && len < 19) {
        *dtype = TLONGLONG;
    } else if (len > 19) {
	*status = BAD_INTKEY;
    } else {
    
      if (!(*negative)) {  /* positive integers */
	if (len == 3) {
	    if (strcmp(p,"127") <= 0 ) {
	        *dtype = TSBYTE;
	    } else if (strcmp(p,"255") <= 0 ) {
	        *dtype = TBYTE;
	    } else {
	        *dtype = TSHORT;
	    }
	} else if (len == 5) {
 	    if (strcmp(p,"32767") <= 0 ) {
	        *dtype = TSHORT;
 	    } else if (strcmp(p,"65535") <= 0 ) {
	        *dtype = TUSHORT;
	    } else {
	        *dtype = TINT;
	    }
	} else if (len == 10) {
	    if (strcmp(p,"2147483647") <= 0 ) {
	        *dtype = TINT;
	    } else if (strcmp(p,"4294967295") <= 0 ) {
	        *dtype = TUINT;
	    } else {
	        *dtype = TLONGLONG;
	    }
	} else if (len == 19) {
	    if (strcmp(p,"9223372036854775807") <= 0 ) {
	        *dtype = TLONGLONG;
	    } else {
		*status = BAD_INTKEY;
	    }
	}

      } else {  /* negative integers */
	if (len == 3) {
	    if (strcmp(p,"128") <= 0 ) {
	        *dtype = TSBYTE;
	    } else {
	        *dtype = TSHORT;
	    }
	} else if (len == 5) {
 	    if (strcmp(p,"32768") <= 0 ) {
	        *dtype = TSHORT;
	    } else {
	        *dtype = TINT;
	    }
	} else if (len == 10) {
	    if (strcmp(p,"2147483648") <= 0 ) {
	        *dtype = TINT;
	    } else {
	        *dtype = TLONGLONG;
	    }
	} else if (len == 19) {
	    if (strcmp(p,"9223372036854775808") <= 0 ) {
	        *dtype = TLONGLONG;
	    } else {
		*status = BAD_INTKEY;
	    }
	}
      }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2x(const char *cval,   /* I - formatted string representation of the value */
          char *dtype,        /* O - datatype code: C, L, F, I or X  */

    /* Only one of the following will be defined, depending on datatype */
          long *ival,    /* O - integer value       */
          int *lval,     /* O - logical value       */
          char *sval,    /* O - string value        */
          double *dval,  /* O - double value        */

          int *status)   /* IO - error status */
/*
  high level routine to convert formatted character string to its
  intrinsic data type
*/
{
    ffdtyp(cval, dtype, status);     /* determine the datatype */

    if (*dtype == 'I')
        ffc2ii(cval, ival, status);
    else if (*dtype == 'F')
        ffc2dd(cval, dval, status);
    else if (*dtype == 'L')
        ffc2ll(cval, lval, status);
    else 
        ffc2s(cval, sval, status);   /* C and X formats */
        
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2xx(const char *cval,   /* I - formatted string representation of the value */
          char *dtype,         /* O - datatype code: C, L, F, I or X  */

    /* Only one of the following will be defined, depending on datatype */
          LONGLONG *ival, /* O - integer value       */
          int *lval,     /* O - logical value       */
          char *sval,    /* O - string value        */
          double *dval,  /* O - double value        */

          int *status)   /* IO - error status */
/*
  high level routine to convert formatted character string to its
  intrinsic data type
*/
{
    ffdtyp(cval, dtype, status);     /* determine the datatype */

    if (*dtype == 'I')
        ffc2jj(cval, ival, status);
    else if (*dtype == 'F')
        ffc2dd(cval, dval, status);
    else if (*dtype == 'L')
        ffc2ll(cval, lval, status);
    else 
        ffc2s(cval, sval, status);   /* C and X formats */
        
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2i(const char *cval,   /* I - string representation of the value */
          long *ival,         /* O - numerical value of the input string */
          int *status)        /* IO - error status */
/*
  convert formatted string to an integer value, doing implicit
  datatype conversion if necessary.
*/
{
    char dtype, sval[81], msg[81];
    int lval;
    double dval;
    
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (cval[0] == '\0')
        return(*status = VALUE_UNDEFINED);  /* null value string */
        
    /* convert the keyword to its native datatype */
    ffc2x(cval, &dtype, ival, &lval, sval, &dval, status);

    if (dtype == 'X' )
    {
            *status = BAD_INTKEY;
    }
    else if (dtype == 'C')
    {
            /* try reading the string as a number */
            if (ffc2dd(sval, &dval, status) <= 0)
            {
              if (dval > (double) LONG_MAX || dval < (double) LONG_MIN)
                *status = NUM_OVERFLOW;
              else
                *ival = (long) dval;
            }
    }
    else if (dtype == 'F')
    {
            if (dval > (double) LONG_MAX || dval < (double) LONG_MIN)
                *status = NUM_OVERFLOW;
            else
                *ival = (long) dval;
    }
    else if (dtype == 'L')
    {
            *ival = (long) lval;
    }

    if (*status > 0)
    {
            *ival = 0;
            strcpy(msg,"Error in ffc2i evaluating string as an integer: ");
            strncat(msg,cval,30);
            ffpmsg(msg);
            return(*status);
    }


    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2j(const char *cval,     /* I - string representation of the value */
          LONGLONG *ival,       /* O - numerical value of the input string */
          int *status)          /* IO - error status */
/*
  convert formatted string to a LONGLONG integer value, doing implicit
  datatype conversion if necessary.
*/
{
    char dtype, sval[81], msg[81];
    int lval;
    double dval;
    
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (cval[0] == '\0')
        return(*status = VALUE_UNDEFINED);  /* null value string */
        
    /* convert the keyword to its native datatype */
    ffc2xx(cval, &dtype, ival, &lval, sval, &dval, status);

    if (dtype == 'X' )
    {
            *status = BAD_INTKEY;
    }
    else if (dtype == 'C')
    {
            /* try reading the string as a number */
            if (ffc2dd(sval, &dval, status) <= 0)
            {
              if (dval > (double) LONGLONG_MAX || dval < (double) LONGLONG_MIN)
                *status = NUM_OVERFLOW;
              else
                *ival = (LONGLONG) dval;
            }
    }
    else if (dtype == 'F')
    {
            if (dval > (double) LONGLONG_MAX || dval < (double) LONGLONG_MIN)
                *status = NUM_OVERFLOW;
            else
                *ival = (LONGLONG) dval;
    }
    else if (dtype == 'L')
    {
            *ival = (LONGLONG) lval;
    }

    if (*status > 0)
    {
            *ival = 0;
            strcpy(msg,"Error in ffc2j evaluating string as a long integer: ");
            strncat(msg,cval,30);
            ffpmsg(msg);
            return(*status);
    }


    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2l(const char *cval,  /* I - string representation of the value */
         int *lval,          /* O - numerical value of the input string */
         int *status)        /* IO - error status */
/*
  convert formatted string to a logical value, doing implicit
  datatype conversion if necessary
*/
{
    char dtype, sval[81], msg[81];
    long ival;
    double dval;
    
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (cval[0] == '\0')
        return(*status = VALUE_UNDEFINED);  /* null value string */

    /* convert the keyword to its native datatype */
    ffc2x(cval, &dtype, &ival, lval, sval, &dval, status);

    if (dtype == 'C' || dtype == 'X' )
        *status = BAD_LOGICALKEY;

    if (*status > 0)
    {
            *lval = 0;
            strcpy(msg,"Error in ffc2l evaluating string as a logical: ");
            strncat(msg,cval,30);
            ffpmsg(msg);
            return(*status);
    }

    if (dtype == 'I')
    {
        if (ival)
            *lval = 1;
        else
            *lval = 0;
    }
    else if (dtype == 'F')
    {
        if (dval)
            *lval = 1;
        else
            *lval = 0;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2r(const char *cval,   /* I - string representation of the value */
          float *fval,        /* O - numerical value of the input string */
          int *status)        /* IO - error status */
/*
  convert formatted string to a real float value, doing implicit
  datatype conversion if necessary
*/
{
    char dtype, sval[81], msg[81];
    int lval;
    
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (cval[0] == '\0')
        return(*status = VALUE_UNDEFINED);  /* null value string */

    ffdtyp(cval, &dtype, status);     /* determine the datatype */

    if (dtype == 'I' || dtype == 'F')
        ffc2rr(cval, fval, status);
    else if (dtype == 'L')
    {
        ffc2ll(cval, &lval, status);
        *fval = (float) lval;
    }
    else if (dtype == 'C')
    {
        /* try reading the string as a number */
        ffc2s(cval, sval, status); 
        ffc2rr(sval, fval, status);
    }
    else 
        *status = BAD_FLOATKEY;

    if (*status > 0)
    {
            *fval = 0.;
            strcpy(msg,"Error in ffc2r evaluating string as a float: ");
            strncat(msg,cval,30);
            ffpmsg(msg);
            return(*status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2d(const char *cval,   /* I - string representation of the value */
          double *dval,       /* O - numerical value of the input string */
          int *status)        /* IO - error status */
/*
  convert formatted string to a double value, doing implicit
  datatype conversion if necessary
*/
{
    char dtype, sval[81], msg[81];
    int lval;
    
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (cval[0] == '\0')
        return(*status = VALUE_UNDEFINED);  /* null value string */

    ffdtyp(cval, &dtype, status);     /* determine the datatype */

    if (dtype == 'I' || dtype == 'F')
        ffc2dd(cval, dval, status);
    else if (dtype == 'L')
    {
        ffc2ll(cval, &lval, status);
        *dval = (double) lval;
    }
    else if (dtype == 'C')
    {
        /* try reading the string as a number */
        ffc2s(cval, sval, status); 
        ffc2dd(sval, dval, status);
    }
    else 
        *status = BAD_DOUBLEKEY;

    if (*status > 0)
    {
            *dval = 0.;
            strcpy(msg,"Error in ffc2d evaluating string as a double: ");
            strncat(msg,cval,30);
            ffpmsg(msg);
            return(*status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2ii(const char *cval,  /* I - string representation of the value */
          long *ival,         /* O - numerical value of the input string */
          int *status)        /* IO - error status */
/*
  convert null-terminated formatted string to an integer value
*/
{
    char *loc, msg[81];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    errno = 0;
    *ival = 0;
    *ival = strtol(cval, &loc, 10);  /* read the string as an integer */

    /* check for read error, or junk following the integer */
    if (*loc != '\0' && *loc != ' ' ) 
        *status = BAD_C2I;

    if (errno == ERANGE)
    {
        strcpy(msg,"Range Error in ffc2ii converting string to long int: ");
        strncat(msg,cval,25);
        ffpmsg(msg);

        *status = NUM_OVERFLOW;
        errno = 0;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2jj(const char *cval,  /* I - string representation of the value */
          LONGLONG *ival,     /* O - numerical value of the input string */
          int *status)        /* IO - error status */
/*
  convert null-terminated formatted string to an long long integer value
*/
{
    char *loc, msg[81];

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    errno = 0;
    *ival = 0;

#if defined(_MSC_VER)

    /* Microsoft Visual C++ 6.0 does not have the strtoll function */
    *ival =  _atoi64(cval);
    loc = (char *) cval;
    while (*loc == ' ') loc++;     /* skip spaces */
    if    (*loc == '-') loc++;     /* skip minus sign */
    if    (*loc == '+') loc++;     /* skip plus sign */
    while (isdigit(*loc)) loc++;   /* skip digits */

#elif (USE_LL_SUFFIX == 1)
    *ival = strtoll(cval, &loc, 10);  /* read the string as an integer */
#else
    *ival = strtol(cval, &loc, 10);  /* read the string as an integer */
#endif

    /* check for read error, or junk following the integer */
    if (*loc != '\0' && *loc != ' ' ) 
        *status = BAD_C2I;

    if (errno == ERANGE)
    {
        strcpy(msg,"Range Error in ffc2jj converting string to longlong int: ");
        strncat(msg,cval,25);
        ffpmsg(msg);

        *status = NUM_OVERFLOW;
        errno = 0;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2ll(const char *cval,  /* I - string representation of the value: T or F */
           int *lval,         /* O - numerical value of the input string: 1 or 0 */
           int *status)       /* IO - error status */
/*
  convert null-terminated formatted string to a logical value
*/
{
    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (cval[0] == 'T')
        *lval = 1;
    else                
        *lval = 0;        /* any character besides T is considered false */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2s(const char *instr,  /* I - null terminated quoted input string */
          char *outstr,       /* O - null terminated output string without quotes */
          int *status)        /* IO - error status */
/*
    convert an input quoted string to an unquoted string by removing
    the leading and trailing quote character.  Also, replace any
    pairs of single quote characters with just a single quote 
    character (FITS used a pair of single quotes to represent
    a literal quote character within the string).
*/
{
    int jj;
    size_t len, ii;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (instr[0] != '\'')
    {
        if (instr[0] == '\0') {
           outstr[0] = '\0';
           return(*status = VALUE_UNDEFINED);  /* null value string */
        } else {
          strcpy(outstr, instr);  /* no leading quote, so return input string */
          return(*status);
        }
    }

    len = strlen(instr);

    for (ii=1, jj=0; ii < len; ii++, jj++)
    {
        if (instr[ii] == '\'')  /*  is this the closing quote?  */
        {
            if (instr[ii+1] == '\'')  /* 2 successive quotes? */
                ii++;  /* copy only one of the quotes */
            else
                break;   /*  found the closing quote, so exit this loop  */
        }
        outstr[jj] = instr[ii];   /* copy the next character to the output */
    }

    outstr[jj] = '\0';             /*  terminate the output string  */

    if (ii == len)
    {
        ffpmsg("This string value has no closing quote (ffc2s):");
        ffpmsg(instr);
        return(*status = 205);
    }

    for (jj--; jj >= 0; jj--)  /* replace trailing blanks with nulls */
    {
        if (outstr[jj] == ' ')
            outstr[jj] = 0;
        else
            break;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2rr(const char *cval,   /* I - string representation of the value */
           float *fval,        /* O - numerical value of the input string */
           int *status)        /* IO - error status */
/*
  convert null-terminated formatted string to a float value
*/
{
    char *loc, msg[81], tval[73];
    struct lconv *lcc = 0;
    static char decimalpt = 0;
    short *sptr, iret;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (!decimalpt) { /* only do this once for efficiency */
       lcc = localeconv();   /* set structure containing local decimal point symbol */
       decimalpt = *(lcc->decimal_point);
    }

    errno = 0;
    *fval = 0.;

    if (strchr(cval, 'D') || decimalpt == ',')  {
        /* strtod expects a comma, not a period, as the decimal point */
        strcpy(tval, cval);

        /*  The C language does not support a 'D'; replace with 'E' */
        if ((loc = strchr(tval, 'D'))) *loc = 'E';

        if (decimalpt == ',')  {
            /* strtod expects a comma, not a period, as the decimal point */
            if ((loc = strchr(tval, '.')))  *loc = ',';   
        }

        *fval = (float) strtod(tval, &loc);  /* read the string as an float */
    } else {
        *fval = (float) strtod(cval, &loc);
    }

    /* check for read error, or junk following the value */
    if (*loc != '\0' && *loc != ' ' )
    {
        strcpy(msg,"Error in ffc2rr converting string to float: ");
        strncat(msg,cval,30);
        ffpmsg(msg);

        *status = BAD_C2F;   
    }

    sptr = (short *) fval;
#if BYTESWAPPED && MACHINE != VAXVMS && MACHINE != ALPHAVMS
    sptr++;       /* point to MSBs */
#endif
    iret = fnan(*sptr);  /* if iret == 1, then the float value is a NaN */

    if (errno == ERANGE || (iret == 1) )
    {
        strcpy(msg,"Error in ffc2rr converting string to float: ");
        strncat(msg,cval,30);
        ffpmsg(msg);
	*fval = 0.;

        *status = NUM_OVERFLOW;
        errno = 0;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffc2dd(const char *cval,   /* I - string representation of the value */
           double *dval,       /* O - numerical value of the input string */
           int *status)        /* IO - error status */
/*
  convert null-terminated formatted string to a double value
*/
{
    char *loc, msg[81], tval[73];
    struct lconv *lcc = 0;
    static char decimalpt = 0;
    short *sptr, iret;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    if (!decimalpt) { /* only do this once for efficiency */
       lcc = localeconv();   /* set structure containing local decimal point symbol */
       decimalpt = *(lcc->decimal_point);
    }
   
    errno = 0;
    *dval = 0.;

    if (strchr(cval, 'D') || decimalpt == ',') {
        /* need to modify a temporary copy of the string before parsing it */
        strcpy(tval, cval);
        /*  The C language does not support a 'D'; replace with 'E' */
        if ((loc = strchr(tval, 'D'))) *loc = 'E';

        if (decimalpt == ',')  {
            /* strtod expects a comma, not a period, as the decimal point */
            if ((loc = strchr(tval, '.')))  *loc = ',';   
        }
    
        *dval = strtod(tval, &loc);  /* read the string as an double */
    } else {
        *dval = strtod(cval, &loc);
    }

    /* check for read error, or junk following the value */
    if (*loc != '\0' && *loc != ' ' )
    {
        strcpy(msg,"Error in ffc2dd converting string to double: ");
        strncat(msg,cval,30);
        ffpmsg(msg);

        *status = BAD_C2D;   
    }

    sptr = (short *) dval;
#if BYTESWAPPED && MACHINE != VAXVMS && MACHINE != ALPHAVMS
    sptr += 3;       /* point to MSBs */
#endif
    iret = dnan(*sptr);  /* if iret == 1, then the double value is a NaN */

    if (errno == ERANGE || (iret == 1) )
    {
        strcpy(msg,"Error in ffc2dd converting string to double: ");
        strncat(msg,cval,30);
        ffpmsg(msg);
	*dval = 0.;

        *status = NUM_OVERFLOW;
        errno = 0;
    }

    return(*status);
}

/* ================================================================== */
/* A hack for nonunix machines, which lack strcasecmp and strncasecmp */
/* ================================================================== */

int fits_strcasecmp(const char *s1, const char *s2)
{
   char c1, c2;

   for (;;) {
      c1 = toupper( *s1 );
      c2 = toupper( *s2 );

      if (c1 < c2) return(-1);
      if (c1 > c2) return(1);
      if (c1 == 0) return(0);
      s1++;
      s2++;
   }
}

int fits_strncasecmp(const char *s1, const char *s2, size_t n)
{
   char c1, c2;

   for (; n-- ;) {
      c1 = toupper( *s1 );
      c2 = toupper( *s2 );

      if (c1 < c2) return(-1);
      if (c1 > c2) return(1);
      if (c1 == 0) return(0);
      s1++;
      s2++;
   }
   return(0);
}
