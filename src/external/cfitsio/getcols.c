/*  This file, getcols.c, contains routines that read data elements from   */
/*  a FITS image or table, with a character string datatype.               */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <stdlib.h>
#include <string.h>
/* stddef.h is apparently needed to define size_t */
#include <stddef.h>
#include <ctype.h>
#include "fitsio2.h"
/*--------------------------------------------------------------------------*/
int ffgcvs( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col)  */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)         */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st)  */
            LONGLONG  nelem,      /* I - number of strings to read               */
            char *nulval,     /* I - string for null pixels                  */
            char **array,     /* O - array of values that are read           */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of string values from a column in the current FITS HDU.
  Any undefined pixels will be set equal to the value of 'nulval' unless
  nulval = null in which case no checks for undefined pixels will be made.
*/
{
    char cdummy[2];

    ffgcls(fptr, colnum, firstrow, firstelem, nelem, 1, nulval,
           array, cdummy, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcfs( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col) */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)        */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st) */
            LONGLONG  nelem,      /* I - number of strings to read              */
            char **array,     /* O - array of values that are read           */
            char *nularray,   /* O - array of flags = 1 if nultyp = 2        */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of string values from a column in the current FITS HDU.
  Nularray will be set = 1 if the corresponding array pixel is undefined, 
  otherwise nularray will = 0.
*/
{
    char dummy[2];

    ffgcls(fptr, colnum, firstrow, firstelem, nelem, 2, dummy,
           array, nularray, anynul, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcls( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col) */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)        */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st) */
            LONGLONG  nelem,      /* I - number of strings to read              */
            int   nultyp,     /* I - null value handling code:               */
                              /*     1: set undefined pixels = nulval        */
                              /*     2: set nularray=1 for undefined pixels  */
            char  *nulval,    /* I - value for null pixels if nultyp = 1     */
            char **array,     /* O - array of values that are read           */
            char *nularray,   /* O - array of flags = 1 if nultyp = 2        */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of string values from a column in the current FITS HDU.
  Returns a formated string value, regardless of the datatype of the column
*/
{
    int tcode, hdutype, tstatus, scaled, intcol, dwidth, nulwidth, ll, dlen;
    int equivtype;
    long ii, jj;
    tcolumn *colptr;
    char message[FLEN_ERRMSG], *carray, keyname[FLEN_KEYWORD];
    char cform[20], dispfmt[20], tmpstr[400], *flgarray, tmpnull[80];
    unsigned char byteval;
    float *earray;
    double *darray, tscale = 1.0;
    LONGLONG *llarray;

    if (*status > 0 || nelem == 0)  /* inherit input status value if > 0 */
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    /* rescan header if data structure is undefined */
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
        if ( ffrdef(fptr, status) > 0)               
            return(*status);

    if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
    {
        sprintf(message, "Specified column number is out of range: %d",
                colnum);
        ffpmsg(message);
        return(*status = BAD_COL_NUM);
    }

    /* get equivalent dataype of column (only needed for TLONGLONG columns) */
    ffeqtyll(fptr, colnum, &equivtype, NULL, NULL, status);
    if (equivtype < 0) equivtype = abs(equivtype);
    
    colptr  = (fptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */
    tcode = abs(colptr->tdatatype);

    if (tcode == TSTRING)
    {
      /* simply call the string column reading routine */
      ffgcls2(fptr, colnum, firstrow, firstelem, nelem, nultyp, nulval,
           array, nularray, anynul, status);
    }
    else if (tcode == TLOGICAL)
    {
      /* allocate memory for the array of logical values */
      carray = (char *) malloc((size_t) nelem);

      /*  call the logical column reading routine */
      ffgcll(fptr, colnum, firstrow, firstelem, nelem, nultyp, *nulval,
           carray, nularray, anynul, status); 

      if (*status <= 0)
      {
         /* convert logical values to "T", "F", or "N" (Null) */
         for (ii = 0; ii < nelem; ii++)
         {
           if (carray[ii] == 1)
              strcpy(array[ii], "T");
           else if (carray[ii] == 0)
              strcpy(array[ii], "F");
           else  /* undefined values = 2 */
              strcpy(array[ii],"N");
         }
      }

      free(carray);  /* free the memory */
    }
    else if (tcode == TCOMPLEX)
    {
      /* allocate memory for the array of double values */
      earray = (float *) calloc((size_t) (nelem * 2), sizeof(float) );
      
      ffgcle(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2,
        1, 1, FLOATNULLVALUE, earray, nularray, anynul, status);

      if (*status <= 0)
      {

         /* determine the format for the output strings */

         ffgcdw(fptr, colnum, &dwidth, status);
         dwidth = (dwidth - 3) / 2;
 
         /* use the TDISPn keyword if it exists */
         ffkeyn("TDISP", colnum, keyname, status);
         tstatus = 0;
         cform[0] = '\0';

         if (ffgkys(fptr, keyname, dispfmt, NULL, &tstatus) == 0)
         {
             /* convert the Fortran style format to a C style format */
             ffcdsp(dispfmt, cform);
         }

         if (!cform[0])
             strcpy(cform, "%14.6E");

         /* write the formated string for each value:  "(real,imag)" */
         jj = 0;
         for (ii = 0; ii < nelem; ii++)
         {
           strcpy(array[ii], "(");

           /* test for null value */
           if (earray[jj] == FLOATNULLVALUE)
           {
             strcpy(tmpstr, "NULL");
             if (nultyp == 2)
                nularray[ii] = 1;
           }
           else
             sprintf(tmpstr, cform, earray[jj]);

           strncat(array[ii], tmpstr, dwidth);
           strcat(array[ii], ",");
           jj++;

           /* test for null value */
           if (earray[jj] == FLOATNULLVALUE)
           {
             strcpy(tmpstr, "NULL");
             if (nultyp == 2)
                nularray[ii] = 1;
           }
           else
             sprintf(tmpstr, cform, earray[jj]);

           strncat(array[ii], tmpstr, dwidth);
           strcat(array[ii], ")");
           jj++;
         }
      }

      free(earray);  /* free the memory */
    }
    else if (tcode == TDBLCOMPLEX)
    {
      /* allocate memory for the array of double values */
      darray = (double *) calloc((size_t) (nelem * 2), sizeof(double) );
      
      ffgcld(fptr, colnum, firstrow, (firstelem - 1) * 2 + 1, nelem * 2,
        1, 1, DOUBLENULLVALUE, darray, nularray, anynul, status);

      if (*status <= 0)
      {
         /* determine the format for the output strings */

         ffgcdw(fptr, colnum, &dwidth, status);
         dwidth = (dwidth - 3) / 2;

         /* use the TDISPn keyword if it exists */
         ffkeyn("TDISP", colnum, keyname, status);
         tstatus = 0;
         cform[0] = '\0';
 
         if (ffgkys(fptr, keyname, dispfmt, NULL, &tstatus) == 0)
         {
             /* convert the Fortran style format to a C style format */
             ffcdsp(dispfmt, cform);
         }

         if (!cform[0])
            strcpy(cform, "%23.15E");

         /* write the formated string for each value:  "(real,imag)" */
         jj = 0;
         for (ii = 0; ii < nelem; ii++)
         {
           strcpy(array[ii], "(");

           /* test for null value */
           if (darray[jj] == DOUBLENULLVALUE)
           {
             strcpy(tmpstr, "NULL");
             if (nultyp == 2)
                nularray[ii] = 1;
           }
           else
             sprintf(tmpstr, cform, darray[jj]);

           strncat(array[ii], tmpstr, dwidth);
           strcat(array[ii], ",");
           jj++;

           /* test for null value */
           if (darray[jj] == DOUBLENULLVALUE)
           {
             strcpy(tmpstr, "NULL");
             if (nultyp == 2)
                nularray[ii] = 1;
           }
           else
             sprintf(tmpstr, cform, darray[jj]);

           strncat(array[ii], tmpstr, dwidth);
           strcat(array[ii], ")");
           jj++;
         }
      }

      free(darray);  /* free the memory */
    }
    else if (tcode == TLONGLONG && equivtype == TLONGLONG)
    {
      /* allocate memory for the array of LONGLONG values */
      llarray = (LONGLONG *) calloc((size_t) nelem, sizeof(LONGLONG) );
      flgarray = (char *) calloc((size_t) nelem, sizeof(char) );
      dwidth = 20;  /* max width of displayed long long integer value */

      if (ffgcfjj(fptr, colnum, firstrow, firstelem, nelem,
            llarray, flgarray, anynul, status) > 0)
      {
         free(flgarray);
         free(llarray);
         return(*status);
      }

      /* write the formated string for each value */
      if (nulval) {
          strcpy(tmpnull, nulval);
          nulwidth = strlen(nulval);
      } else {
          strcpy(tmpnull, " ");
          nulwidth = 1;
      }

      for (ii = 0; ii < nelem; ii++)
      {
           if ( flgarray[ii] )
           {
              *array[ii] = '\0';
              if (dwidth < nulwidth)
                  strncat(array[ii], tmpnull, dwidth);
              else
                  sprintf(array[ii],"%*s",dwidth,tmpnull);
		  
              if (nultyp == 2)
	          nularray[ii] = 1;
           }
           else
           {	   

#if defined(_MSC_VER)
    /* Microsoft Visual C++ 6.0 uses '%I64d' syntax  for 8-byte integers */
        sprintf(tmpstr, "%20I64d", llarray[ii]);
#elif (USE_LL_SUFFIX == 1)
        sprintf(tmpstr, "%20lld", llarray[ii]);
#else
        sprintf(tmpstr, "%20ld", llarray[ii]);
#endif
              *array[ii] = '\0';
              strncat(array[ii], tmpstr, 20);
           }
      }

      free(flgarray);
      free(llarray);  /* free the memory */

    }
    else
    {
      /* allocate memory for the array of double values */
      darray = (double *) calloc((size_t) nelem, sizeof(double) );
      
      /* read all other numeric type columns as doubles */
      if (ffgcld(fptr, colnum, firstrow, firstelem, nelem, 1, nultyp, 
           DOUBLENULLVALUE, darray, nularray, anynul, status) > 0)
      {
         free(darray);
         return(*status);
      }

      /* determine the format for the output strings */

      ffgcdw(fptr, colnum, &dwidth, status);

      /* check if  column is scaled */
      ffkeyn("TSCAL", colnum, keyname, status);
      tstatus = 0;
      scaled = 0;
      if (ffgkyd(fptr, keyname, &tscale, NULL, &tstatus) == 0)
      {
            if (tscale != 1.0)
                scaled = 1;    /* yes, this is a scaled column */
      }

      intcol = 0;
      if (tcode <= TLONG && !scaled)
             intcol = 1;   /* this is an unscaled integer column */

      /* use the TDISPn keyword if it exists */
      ffkeyn("TDISP", colnum, keyname, status);
      tstatus = 0;
      cform[0] = '\0';

      if (ffgkys(fptr, keyname, dispfmt, NULL, &tstatus) == 0)
      {
           /* convert the Fortran style TDISPn to a C style format */
           ffcdsp(dispfmt, cform);
      }

      if (!cform[0])
      {
            /* no TDISPn keyword; use TFORMn instead */

            ffkeyn("TFORM", colnum, keyname, status);
            ffgkys(fptr, keyname, dispfmt, NULL, status);

            if (scaled && tcode <= TSHORT)
            {
                  /* scaled short integer column == float */
                  strcpy(cform, "%#14.6G");
            }
            else if (scaled && tcode == TLONG)
            {
                  /* scaled long integer column == double */
                  strcpy(cform, "%#23.15G");
            }
            else if (scaled && tcode == TLONGLONG)
            {
                  /* scaled long long integer column == double */
                  strcpy(cform, "%#23.15G");
            }
            else
            {
               ffghdt(fptr, &hdutype, status);
               if (hdutype == ASCII_TBL)
               {
                  /* convert the Fortran style TFORMn to a C style format */
                  ffcdsp(dispfmt, cform);
               }
               else
               {
                 /* this is a binary table, need to convert the format */
                  if (tcode == TBIT) {            /* 'X' */
                     strcpy(cform, "%4d");
                  } else if (tcode == TBYTE) {    /* 'B' */
                     strcpy(cform, "%4d");
                  } else if (tcode == TSHORT) {   /* 'I' */
                     strcpy(cform, "%6d");
                  } else if (tcode == TLONG) {    /* 'J' */
                     strcpy(cform, "%11.0f");
                     intcol = 0;  /* needed to support unsigned int */
                  } else if (tcode == TFLOAT) {   /* 'E' */
                     strcpy(cform, "%#14.6G");
                  } else if (tcode == TDOUBLE) {  /* 'D' */
                     strcpy(cform, "%#23.15G");
                  }
               }
            }
      } 

      if (nulval) {
          strcpy(tmpnull, nulval);
          nulwidth = strlen(nulval);
      } else {
          strcpy(tmpnull, " ");
          nulwidth = 1;
      }

      /* write the formated string for each value */
      for (ii = 0; ii < nelem; ii++)
      {
           if (tcode == TBIT)
           {
               byteval = (char) darray[ii];

               for (ll=0; ll < 8; ll++)
               {
                   if ( ((unsigned char) (byteval << ll)) >> 7 )
                       *(array[ii] + ll) = '1';
                   else
                       *(array[ii] + ll) = '0';
               }
               *(array[ii] + 8) = '\0';
           }
           /* test for null value */
           else if ( (nultyp == 1 && darray[ii] == DOUBLENULLVALUE) ||
                (nultyp == 2 && nularray[ii]) )
           {
              *array[ii] = '\0';
              if (dwidth < nulwidth)
                  strncat(array[ii], tmpnull, dwidth);
              else
                  sprintf(array[ii],"%*s",dwidth,tmpnull);
           }
           else
           {	   
              if (intcol) {
                sprintf(tmpstr, cform, (int) darray[ii]);
              } else {
                sprintf(tmpstr, cform, darray[ii]);
              }
	      
              /* fill field with '*' if number is too wide */
              dlen = strlen(tmpstr);
	      if (dlen > dwidth) {
	         memset(tmpstr, '*', dwidth);
              }

              *array[ii] = '\0';
              strncat(array[ii], tmpstr, dwidth);
           }
      }

      free(darray);  /* free the memory */
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcdw( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column (1 = 1st col)      */
            int  *width,      /* O - display width                       */
            int  *status)     /* IO - error status                           */
/*
  Get Column Display Width.
*/
{
    tcolumn *colptr;
    char *cptr;
    char message[FLEN_ERRMSG], keyname[FLEN_KEYWORD], dispfmt[20];
    int tcode, hdutype, tstatus, scaled;
    double tscale;

    if (*status > 0)  /* inherit input status value if > 0 */
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
    {
        sprintf(message, "Specified column number is out of range: %d",
                colnum);
        ffpmsg(message);
        return(*status = BAD_COL_NUM);
    }

    colptr  = (fptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */
    tcode = abs(colptr->tdatatype);

    /* use the TDISPn keyword if it exists */
    ffkeyn("TDISP", colnum, keyname, status);

    *width = 0;
    tstatus = 0;
    if (ffgkys(fptr, keyname, dispfmt, NULL, &tstatus) == 0)
    {
          /* parse TDISPn get the display width */
          cptr = dispfmt;
          while(*cptr == ' ') /* skip leading blanks */
              cptr++;

          if (*cptr == 'A' || *cptr == 'a' ||
              *cptr == 'I' || *cptr == 'i' ||
              *cptr == 'O' || *cptr == 'o' ||
              *cptr == 'Z' || *cptr == 'z' ||
              *cptr == 'F' || *cptr == 'f' ||
              *cptr == 'E' || *cptr == 'e' ||
              *cptr == 'D' || *cptr == 'd' ||
              *cptr == 'G' || *cptr == 'g')
          {

            while(!isdigit((int) *cptr) && *cptr != '\0') /* find 1st digit */
              cptr++;

            *width = atoi(cptr);
            if (tcode >= TCOMPLEX)
              *width = (2 * (*width)) + 3;
          }
    }

    if (*width == 0)
    {
        /* no valid TDISPn keyword; use TFORMn instead */

        ffkeyn("TFORM", colnum, keyname, status);
        ffgkys(fptr, keyname, dispfmt, NULL, status);

        /* check if  column is scaled */
        ffkeyn("TSCAL", colnum, keyname, status);
        tstatus = 0;
        scaled = 0;

        if (ffgkyd(fptr, keyname, &tscale, NULL, &tstatus) == 0)
        {
            if (tscale != 1.0)
                scaled = 1;    /* yes, this is a scaled column */
        }

        if (scaled && tcode <= TSHORT)
        {
            /* scaled short integer col == float; default format is 14.6G */
            *width = 14;
        }
        else if (scaled && tcode == TLONG)
        {
            /* scaled long integer col == double; default format is 23.15G */
            *width = 23;
        }
        else if (scaled && tcode == TLONGLONG)
        {
            /* scaled long long integer col == double; default format is 23.15G */
            *width = 23;
        }

        else
        {
           ffghdt(fptr, &hdutype, status);  /* get type of table */
           if (hdutype == ASCII_TBL)
           {
              /* parse TFORMn get the display width */
              cptr = dispfmt;
              while(!isdigit((int) *cptr) && *cptr != '\0') /* find 1st digit */
                 cptr++;

              *width = atoi(cptr);
           }
           else
           {
                 /* this is a binary table */
                  if (tcode == TBIT)           /* 'X' */
                     *width = 8;
                  else if (tcode == TBYTE)     /* 'B' */
                     *width = 4;
                  else if (tcode == TSHORT)    /* 'I' */
                     *width = 6;
                  else if (tcode == TLONG)     /* 'J' */
                     *width = 11;
                  else if (tcode == TLONGLONG) /* 'K' */
                     *width = 20;
                  else if (tcode == TFLOAT)    /* 'E' */
                     *width = 14;
                  else if (tcode == TDOUBLE)   /* 'D' */
                     *width = 23;
                  else if (tcode == TCOMPLEX)  /* 'C' */
                     *width = 31;
                  else if (tcode == TDBLCOMPLEX)  /* 'M' */
                     *width = 49;
                  else if (tcode == TLOGICAL)  /* 'L' */
                     *width = 1;
                  else if (tcode == TSTRING)   /* 'A' */
                  {
		    int typecode;
		    long int repeat = 0, rwidth = 0;
		    int gstatus = 0;

		    /* Deal with possible vector string with repeat / width  by parsing
		       the TFORM=rAw keyword */
		    if (ffgtcl(fptr, colnum, &typecode, &repeat, &rwidth, &gstatus) == 0 &&
			rwidth >= 1 && rwidth < repeat) {
		      *width = rwidth;

		    } else {
		      
		      /* Hmmm, we couldn't parse the TFORM keyword by standard, so just do
			 simple parsing */
		      cptr = dispfmt;
		      while(!isdigit((int) *cptr) && *cptr != '\0') 
			cptr++;
		      
		      *width = atoi(cptr);
		    }

                    if (*width < 1)
                         *width = 1;  /* default is at least 1 column */
                  }
            }
        }
    } 
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgcls2 ( fitsfile *fptr,   /* I - FITS file pointer                       */
            int  colnum,      /* I - number of column to read (1 = 1st col) */
            LONGLONG  firstrow,   /* I - first row to read (1 = 1st row)        */
            LONGLONG  firstelem,  /* I - first vector element to read (1 = 1st) */
            LONGLONG  nelem,      /* I - number of strings to read              */
            int   nultyp,     /* I - null value handling code:               */
                              /*     1: set undefined pixels = nulval        */
                              /*     2: set nularray=1 for undefined pixels  */
            char  *nulval,    /* I - value for null pixels if nultyp = 1     */
            char **array,     /* O - array of values that are read           */
            char *nularray,   /* O - array of flags = 1 if nultyp = 2        */
            int  *anynul,     /* O - set to 1 if any values are null; else 0 */
            int  *status)     /* IO - error status                           */
/*
  Read an array of string values from a column in the current FITS HDU.
*/
{
    double dtemp;
    long nullen; 
    int tcode, maxelem, hdutype, nulcheck;
    long twidth, incre;
    long ii, jj, ntodo;
    LONGLONG repeat, startpos, elemnum, readptr, tnull, rowlen, rownum, remain, next;
    double scale, zero;
    char tform[20];
    char message[FLEN_ERRMSG];
    char snull[20];   /*  the FITS null value  */
    tcolumn *colptr;

    double cbuff[DBUFFSIZE / sizeof(double)]; /* align cbuff on word boundary */
    char *buffer, *arrayptr;

    if (*status > 0 || nelem == 0)  /* inherit input status value if > 0 */
        return(*status);

    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);

    if (anynul)
        *anynul = 0;

    if (nultyp == 2)
        memset(nularray, 0, (size_t) nelem);   /* initialize nullarray */

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/
    if (colnum < 1 || colnum > (fptr->Fptr)->tfield)
    {
        sprintf(message, "Specified column number is out of range: %d",
                colnum);
        ffpmsg(message);
        return(*status = BAD_COL_NUM);
    }

    colptr  = (fptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */
    tcode = colptr->tdatatype;

    if (tcode == -TSTRING) /* variable length column in a binary table? */
    {
      /* only read a single string; ignore value of firstelem */

      if (ffgcprll( fptr, colnum, firstrow, 1, 1, 0, &scale, &zero,
        tform, &twidth, &tcode, &maxelem, &startpos,  &elemnum, &incre,
        &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0)
        return(*status);

      remain = 1;
      twidth = (long) repeat;  
    }
    else if (tcode == TSTRING)
    {
      if (ffgcprll( fptr, colnum, firstrow, firstelem, nelem, 0, &scale, &zero,
        tform, &twidth, &tcode, &maxelem, &startpos,  &elemnum, &incre,
        &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0)
        return(*status);

      /* if string length is greater than a FITS block (2880 char) then must */
      /* only read 1 string at a time, to force reading by ffgbyt instead of */
      /* ffgbytoff (ffgbytoff can't handle this case) */
      if (twidth > IOBUFLEN) {
        maxelem = 1;
        incre = twidth;
        repeat = 1;
      }   

      remain = nelem;
    }
    else
        return(*status = NOT_ASCII_COL);

    nullen = strlen(snull);   /* length of the undefined pixel string */
    if (nullen == 0)
        nullen = 1;
 
    /*------------------------------------------------------------------*/
    /*  Decide whether to check for null values in the input FITS file: */
    /*------------------------------------------------------------------*/
    nulcheck = nultyp; /* by default check for null values in the FITS file */

    if (nultyp == 1 && nulval == 0)
       nulcheck = 0;    /* calling routine does not want to check for nulls */

    else if (nultyp == 1 && nulval && nulval[0] == 0)
       nulcheck = 0;    /* calling routine does not want to check for nulls */

    else if (snull[0] == ASCII_NULL_UNDEFINED)
       nulcheck = 0;   /* null value string in ASCII table not defined */

    else if (nullen > twidth)
       nulcheck = 0;   /* null value string is longer than width of column  */
                       /* thus impossible for any column elements to = null */

    /*---------------------------------------------------------------------*/
    /*  Now read the strings one at a time from the FITS column.           */
    /*---------------------------------------------------------------------*/
    next = 0;                 /* next element in array to be read  */
    rownum = 0;               /* row number, relative to firstrow     */

    while (remain)
    {
      /* limit the number of pixels to process at one time to the number that
         will fit in the buffer space or to the number of pixels that remain
         in the current vector, which ever is smaller.
      */
      ntodo = (long) minvalue(remain, maxelem);      
      ntodo = (long) minvalue(ntodo, (repeat - elemnum));

      readptr = startpos + ((LONGLONG)rownum * rowlen) + (elemnum * incre);
      ffmbyt(fptr, readptr, REPORT_EOF, status);  /* move to read position */

      /* read the array of strings from the FITS file into the buffer */

      if (incre == twidth)
         ffgbyt(fptr, ntodo * twidth, cbuff, status);
      else
         ffgbytoff(fptr, twidth, ntodo, incre - twidth, cbuff, status);

      /* copy from the buffer into the user's array of strings */
      /* work backwards from last char of last string to 1st char of 1st */

      buffer = ((char *) cbuff) + (ntodo * twidth) - 1;

      for (ii = (long) (next + ntodo - 1); ii >= next; ii--)
      {
         arrayptr = array[ii] + twidth - 1;

         for (jj = twidth - 1; jj > 0; jj--)  /* ignore trailing blanks */
         {
            if (*buffer == ' ')
            {
              buffer--;
              arrayptr--;
            }
            else
              break;
         }
         *(arrayptr + 1) = 0;  /* write the string terminator */
         
         for (; jj >= 0; jj--)    /* copy the string itself */
         {
           *arrayptr = *buffer;
           buffer--;
           arrayptr--;
         }

         /* check if null value is defined, and if the   */
         /* column string is identical to the null string */
         if (nulcheck && !strncmp(snull, array[ii], nullen) )
         {
           *anynul = 1;   /* this is a null value */
           if (nultyp == 1) {
	   
	     if (nulval)
                strcpy(array[ii], nulval);
	     else
	        strcpy(array[ii], " ");
	     
           } else
             nularray[ii] = 1;
         }
      }
    
      if (*status > 0)  /* test for error during previous read operation */
      {
         dtemp = (double) next;
         sprintf(message,
          "Error reading elements %.0f thru %.0f of data array (ffpcls).",
             dtemp+1., dtemp+ntodo);

         ffpmsg(message);
         return(*status);
      }

      /*--------------------------------------------*/
      /*  increment the counters for the next loop  */
      /*--------------------------------------------*/
      next += ntodo;
      remain -= ntodo;
      if (remain)
      {
          elemnum += ntodo;
          if (elemnum == repeat)  /* completed a row; start on next row */
          {
              elemnum = 0;
              rownum++;
          }
      }
    }  /*  End of main while Loop  */

    return(*status);
}

