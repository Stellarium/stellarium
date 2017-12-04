/*   Globally defined histogram parameters */
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include "fitsio2.h"

typedef struct {  /*  Structure holding all the histogramming information   */
   union {        /*  the iterator work functions (ffwritehist, ffcalchist) */
      char   *b;  /*  need to do their job... passed via *userPointer.      */
      short  *i;
      int    *j;
      float  *r;
      double *d;
   } hist;

   fitsfile *tblptr;

   int   haxis, hcolnum[4], himagetype;
   long  haxis1, haxis2, haxis3, haxis4;
   float amin1, amin2, amin3, amin4;
   float maxbin1, maxbin2, maxbin3, maxbin4;
   float binsize1, binsize2, binsize3, binsize4;
   int   wtrecip, wtcolnum;
   float weight;
   char  *rowselector;

} histType;

/*--------------------------------------------------------------------------*/
int ffbins(char *binspec,   /* I - binning specification */
                   int *imagetype,      /* O - image type, TINT or TSHORT */
                   int *histaxis,       /* O - no. of axes in the histogram */
                   char colname[4][FLEN_VALUE],  /* column name for axis */
                   double *minin,        /* minimum value for each axis */
                   double *maxin,        /* maximum value for each axis */
                   double *binsizein,    /* size of bins on each axis */
                   char minname[4][FLEN_VALUE],  /* keyword name for min */
                   char maxname[4][FLEN_VALUE],  /* keyword name for max */
                   char binname[4][FLEN_VALUE],  /* keyword name for binsize */
                   double *wt,          /* weighting factor          */
                   char *wtname,        /* keyword or column name for weight */
                   int *recip,          /* the reciprocal of the weight? */
                   int *status)
{
/*
   Parse the input binning specification string, returning the binning
   parameters.  Supports up to 4 dimensions.  The binspec string has
   one of these forms:

   bin binsize                  - 2D histogram with binsize on each axis
   bin xcol                     - 1D histogram on column xcol
   bin (xcol, ycol) = binsize   - 2D histogram with binsize on each axis
   bin x=min:max:size, y=min:max:size, z..., t... 
   bin x=:max, y=::size
   bin x=size, y=min::size

   most other reasonable combinations are supported.        
*/
    int ii, slen, defaulttype;
    char *ptr, tmpname[30], *file_expr = NULL;
    double  dummy;

    if (*status > 0)
         return(*status);

    /* set the default values */
    *histaxis = 2;
    *imagetype = TINT;
    defaulttype = 1;
    *wt = 1.;
    *recip = 0;
    *wtname = '\0';

    /* set default values */
    for (ii = 0; ii < 4; ii++)
    {
        *colname[ii] = '\0';
        *minname[ii] = '\0';
        *maxname[ii] = '\0';
        *binname[ii] = '\0';
        minin[ii] = DOUBLENULLVALUE;  /* undefined values */
        maxin[ii] = DOUBLENULLVALUE;
        binsizein[ii] = DOUBLENULLVALUE;
    }

    ptr = binspec + 3;  /* skip over 'bin' */

    if (*ptr == 'i' )  /* bini */
    {
        *imagetype = TSHORT;
        defaulttype = 0;
        ptr++;
    }
    else if (*ptr == 'j' )  /* binj; same as default */
    {
        defaulttype = 0;
        ptr ++;
    }
    else if (*ptr == 'r' )  /* binr */
    {
        *imagetype = TFLOAT;
        defaulttype = 0;
        ptr ++;
    }
    else if (*ptr == 'd' )  /* bind */
    {
        *imagetype = TDOUBLE;
        defaulttype = 0;
        ptr ++;
    }
    else if (*ptr == 'b' )  /* binb */
    {
        *imagetype = TBYTE;
        defaulttype = 0;
        ptr ++;
    }

    if (*ptr == '\0')  /* use all defaults for other parameters */
        return(*status);
    else if (*ptr != ' ')  /* must be at least one blank */
    {
        ffpmsg("binning specification syntax error:");
        ffpmsg(binspec);
        return(*status = URL_PARSE_ERROR);
    }

    while (*ptr == ' ')  /* skip over blanks */
           ptr++;

    if (*ptr == '\0')   /* no other parameters; use defaults */
        return(*status);

    /* Check if need to import expression from a file */

    if( *ptr=='@' ) {
       if( ffimport_file( ptr+1, &file_expr, status ) ) return(*status);
       ptr = file_expr;
       while (*ptr == ' ')
               ptr++;       /* skip leading white space... again */
    }

    if (*ptr == '(' )
    {
        /* this must be the opening parenthesis around a list of column */
        /* names, optionally followed by a '=' and the binning spec. */

        for (ii = 0; ii < 4; ii++)
        {
            ptr++;               /* skip over the '(', ',', or ' ') */
            while (*ptr == ' ')  /* skip over blanks */
                ptr++;

            slen = strcspn(ptr, " ,)");
            strncat(colname[ii], ptr, slen); /* copy 1st column name */

            ptr += slen;
            while (*ptr == ' ')  /* skip over blanks */
                ptr++;

            if (*ptr == ')' )   /* end of the list of names */
            {
                *histaxis = ii + 1;
                break;
            }
        }

        if (ii == 4)   /* too many names in the list , or missing ')'  */
        {
            ffpmsg(
 "binning specification has too many column names or is missing closing ')':");
            ffpmsg(binspec);
	    if( file_expr ) free( file_expr );
            return(*status = URL_PARSE_ERROR);
        }

        ptr++;  /* skip over the closing parenthesis */
        while (*ptr == ' ')  /* skip over blanks */
            ptr++;

        if (*ptr == '\0') {
	    if( file_expr ) free( file_expr );
            return(*status);  /* parsed the entire string */
	}

        else if (*ptr != '=')  /* must be an equals sign now*/
        {
            ffpmsg("illegal binning specification in URL:");
            ffpmsg(" an equals sign '=' must follow the column names");
            ffpmsg(binspec);
	    if( file_expr ) free( file_expr );
            return(*status = URL_PARSE_ERROR);
        }

        ptr++;  /* skip over the equals sign */
        while (*ptr == ' ')  /* skip over blanks */
            ptr++;

        /* get the single range specification for all the columns */
        ffbinr(&ptr, tmpname, minin,
                                     maxin, binsizein, minname[0],
                                     maxname[0], binname[0], status);
        if (*status > 0)
        {
            ffpmsg("illegal binning specification in URL:");
            ffpmsg(binspec);
	    if( file_expr ) free( file_expr );
            return(*status);
        }

        for (ii = 1; ii < *histaxis; ii++)
        {
            minin[ii] = minin[0];
            maxin[ii] = maxin[0];
            binsizein[ii] = binsizein[0];
            strcpy(minname[ii], minname[0]);
            strcpy(maxname[ii], maxname[0]);
            strcpy(binname[ii], binname[0]);
        }

        while (*ptr == ' ')  /* skip over blanks */
            ptr++;

        if (*ptr == ';')
            goto getweight;   /* a weighting factor is specified */

        if (*ptr != '\0')  /* must have reached end of string */
        {
            ffpmsg("illegal syntax after binning range specification in URL:");
            ffpmsg(binspec);
	    if( file_expr ) free( file_expr );
            return(*status = URL_PARSE_ERROR);
        }

        return(*status);
    }             /* end of case with list of column names in ( )  */

    /* if we've reached this point, then the binning specification */
    /* must be of the form: XCOL = min:max:binsize, YCOL = ...     */
    /* where the column name followed by '=' are optional.         */
    /* If the column name is not specified, then use the default name */

    for (ii = 0; ii < 4; ii++) /* allow up to 4 histogram dimensions */
    {
        ffbinr(&ptr, colname[ii], &minin[ii],
                                     &maxin[ii], &binsizein[ii], minname[ii],
                                     maxname[ii], binname[ii], status);

        if (*status > 0)
        {
            ffpmsg("illegal syntax in binning range specification in URL:");
            ffpmsg(binspec);
	    if( file_expr ) free( file_expr );
            return(*status);
        }

        if (*ptr == '\0' || *ptr == ';')
            break;        /* reached the end of the string */

        if (*ptr == ' ')
        {
            while (*ptr == ' ')  /* skip over blanks */
                ptr++;

            if (*ptr == '\0' || *ptr == ';')
                break;        /* reached the end of the string */

            if (*ptr == ',')
                ptr++;  /* comma separates the next column specification */
        }
        else if (*ptr == ',')
        {          
            ptr++;  /* comma separates the next column specification */
        }
        else
        {
            ffpmsg("illegal characters following binning specification in URL:");
            ffpmsg(binspec);
	    if( file_expr ) free( file_expr );
            return(*status = URL_PARSE_ERROR);
        }
    }

    if (ii == 4)
    {
        /* there are yet more characters in the string */
        ffpmsg("illegal binning specification in URL:");
        ffpmsg("apparently greater than 4 histogram dimensions");
        ffpmsg(binspec);
        return(*status = URL_PARSE_ERROR);
    }
    else
        *histaxis = ii + 1;

    /* special case: if a single number was entered it should be      */
    /* interpreted as the binning factor for the default X and Y axes */

    if (*histaxis == 1 && *colname[0] == '\0' && 
         minin[0] == DOUBLENULLVALUE && maxin[0] == DOUBLENULLVALUE)
    {
        *histaxis = 2;
        binsizein[1] = binsizein[0];
    }

getweight:
    if (*ptr == ';')  /* looks like a weighting factor is given */
    {
        ptr++;
       
        while (*ptr == ' ')  /* skip over blanks */
            ptr++;

        recip = 0;
        if (*ptr == '/')
        {
            *recip = 1;  /* the reciprocal of the weight is entered */
            ptr++;

            while (*ptr == ' ')  /* skip over blanks */
                ptr++;
        }

        /* parse the weight as though it were a binrange. */
        /* either a column name or a numerical value will be returned */

        ffbinr(&ptr, wtname, &dummy, &dummy, wt, tmpname,
                                     tmpname, tmpname, status);

        if (*status > 0)
        {
            ffpmsg("illegal binning weight specification in URL:");
            ffpmsg(binspec);
	    if( file_expr ) free( file_expr );
            return(*status);
        }

        /* creat a float datatype histogram by default, if weight */
        /* factor is not = 1.0  */

        if ( (defaulttype && *wt != 1.0) || (defaulttype && *wtname) )
            *imagetype = TFLOAT;
    }

    while (*ptr == ' ')  /* skip over blanks */
         ptr++;

    if (*ptr != '\0')  /* should have reached the end of string */
    {
        ffpmsg("illegal syntax after binning weight specification in URL:");
        ffpmsg(binspec);
        *status = URL_PARSE_ERROR;
    }

    if( file_expr ) free( file_expr );
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffbinr(char **ptr, 
                   char *colname, 
                   double *minin,
                   double *maxin, 
                   double *binsizein,
                   char *minname,
                   char *maxname,
                   char *binname,
                   int *status)
/*
   Parse the input binning range specification string, returning 
   the column name, histogram min and max values, and bin size.
*/
{
    int slen, isanumber;
    char token[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    slen = fits_get_token(ptr, " ,=:;", token, &isanumber); /* get 1st token */

    if (slen == 0 && (**ptr == '\0' || **ptr == ',' || **ptr == ';') )
        return(*status);   /* a null range string */

    if (!isanumber && **ptr != ':')
    {
        /* this looks like the column name */

        if (token[0] == '#' && isdigit((int) token[1]) )
        {
            /* omit the leading '#' in the column number */
            strcpy(colname, token+1);
        }
        else
            strcpy(colname, token);

        while (**ptr == ' ')  /* skip over blanks */
             (*ptr)++;

        if (**ptr != '=')
            return(*status);  /* reached the end */

        (*ptr)++;   /* skip over the = sign */

        while (**ptr == ' ')  /* skip over blanks */
             (*ptr)++;

        slen = fits_get_token(ptr, " ,:;", token, &isanumber); /* get token */
    }

    if (**ptr != ':')
    {
        /* this is the first token, and since it is not followed by */
        /* a ':' this must be the binsize token */
        if (!isanumber)
            strcpy(binname, token);
        else
            *binsizein =  strtod(token, NULL);

        return(*status);  /* reached the end */
    }
    else
    {
        /* the token contains the min value */
        if (slen)
        {
            if (!isanumber)
                strcpy(minname, token);
            else
                *minin = strtod(token, NULL);
        }
    }

    (*ptr)++;  /* skip the colon between the min and max values */
    slen = fits_get_token(ptr, " ,:;", token, &isanumber); /* get token */

    /* the token contains the max value */
    if (slen)
    {
        if (!isanumber)
            strcpy(maxname, token);
        else
            *maxin = strtod(token, NULL);
    }

    if (**ptr != ':')
        return(*status);  /* reached the end; no binsize token */

    (*ptr)++;  /* skip the colon between the max and binsize values */
    slen = fits_get_token(ptr, " ,:;", token, &isanumber); /* get token */

    /* the token contains the binsize value */
    if (slen)
    {
        if (!isanumber)
            strcpy(binname, token);
        else
            *binsizein = strtod(token, NULL);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffhist2(fitsfile **fptr,  /* IO - pointer to table with X and Y cols;    */
                             /*     on output, points to histogram image    */
           char *outfile,    /* I - name for the output histogram file      */
           int imagetype,    /* I - datatype for image: TINT, TSHORT, etc   */
           int naxis,        /* I - number of axes in the histogram image   */
           char colname[4][FLEN_VALUE],   /* I - column names               */
           double *minin,     /* I - minimum histogram value, for each axis */
           double *maxin,     /* I - maximum histogram value, for each axis */
           double *binsizein, /* I - bin size along each axis               */
           char minname[4][FLEN_VALUE], /* I - optional keywords for min    */
           char maxname[4][FLEN_VALUE], /* I - optional keywords for max    */
           char binname[4][FLEN_VALUE], /* I - optional keywords for binsize */
           double weightin,        /* I - binning weighting factor          */
           char wtcol[FLEN_VALUE], /* I - optional keyword or col for weight*/
           int recip,              /* I - use reciprocal of the weight?     */
           char *selectrow,        /* I - optional array (length = no. of   */
                             /* rows in the table).  If the element is true */
                             /* then the corresponding row of the table will*/
                             /* be included in the histogram, otherwise the */
                             /* row will be skipped.  Ingnored if *selectrow*/
                             /* is equal to NULL.                           */
           int *status)
{
    fitsfile *histptr;
    int   bitpix, colnum[4], wtcolnum;
    long haxes[4];
    float amin[4], amax[4], binsize[4],  weight;

    if (*status > 0)
        return(*status);

    if (naxis > 4)
    {
        ffpmsg("histogram has more than 4 dimensions");
        return(*status = BAD_DIMEN);
    }

    /* reset position to the correct HDU if necessary */
    if ((*fptr)->HDUposition != ((*fptr)->Fptr)->curhdu)
        ffmahd(*fptr, ((*fptr)->HDUposition) + 1, NULL, status);

    if (imagetype == TBYTE)
        bitpix = BYTE_IMG;
    else if (imagetype == TSHORT)
        bitpix = SHORT_IMG;
    else if (imagetype == TINT)
        bitpix = LONG_IMG;
    else if (imagetype == TFLOAT)
        bitpix = FLOAT_IMG;
    else if (imagetype == TDOUBLE)
        bitpix = DOUBLE_IMG;
    else
        return(*status = BAD_DATATYPE);

    
    /*    Calculate the binning parameters:    */
    /*   columm numbers, axes length, min values,  max values, and binsizes.  */

    if (fits_calc_binning(
      *fptr, naxis, colname, minin, maxin, binsizein, minname, maxname, binname,
      colnum,  haxes, amin, amax, binsize, status) > 0)
    {
        ffpmsg("failed to determine binning parameters");
        return(*status);
    }
 
    /* get the histogramming weighting factor, if any */
    if (*wtcol)
    {
        /* first, look for a keyword with the weight value */
        if (ffgky(*fptr, TFLOAT, wtcol, &weight, NULL, status) )
        {
            /* not a keyword, so look for column with this name */
            *status = 0;

            /* get the column number in the table */
            if (ffgcno(*fptr, CASEINSEN, wtcol, &wtcolnum, status) > 0)
            {
               ffpmsg(
               "keyword or column for histogram weights doesn't exist: ");
               ffpmsg(wtcol);
               return(*status);
            }

            weight = FLOATNULLVALUE;
        }
    }
    else
        weight = (float) weightin;

    if (weight <= 0. && weight != FLOATNULLVALUE)
    {
        ffpmsg("Illegal histogramming weighting factor <= 0.");
        return(*status = URL_PARSE_ERROR);
    }

    if (recip && weight != FLOATNULLVALUE)
       /* take reciprocal of weight */
       weight = (float) (1.0 / weight);

    /* size of histogram is now known, so create temp output file */
    if (fits_create_file(&histptr, outfile, status) > 0)
    {
        ffpmsg("failed to create temp output file for histogram");
        return(*status);
    }

    /* create output FITS image HDU */
    if (ffcrim(histptr, bitpix, naxis, haxes, status) > 0)
    {
        ffpmsg("failed to create output histogram FITS image");
        return(*status);
    }

    /* copy header keywords, converting pixel list WCS keywords to image WCS form */
    if (fits_copy_pixlist2image(*fptr, histptr, 9, naxis, colnum, status) > 0)
    {
        ffpmsg("failed to copy pixel list keywords to new histogram header");
        return(*status);
    }

    /* if the table columns have no WCS keywords, then write default keywords */
    fits_write_keys_histo(*fptr, histptr, naxis, colnum, status);
    
    /* update the WCS keywords for the ref. pixel location, and pixel size */
    fits_rebin_wcs(histptr, naxis, amin, binsize,  status);      
    
    /* now compute the output image by binning the column values */
    if (fits_make_hist(*fptr, histptr, bitpix, naxis, haxes, colnum, amin, amax,
        binsize, weight, wtcolnum, recip, selectrow, status) > 0)
    {
        ffpmsg("failed to calculate new histogram values");
        return(*status);
    }
              
    /* finally, close the original file and return ptr to the new image */
    ffclos(*fptr, status);
    *fptr = histptr;

    return(*status);
}
/*--------------------------------------------------------------------------*/

/* ffhist3: same as ffhist2, but does not close the original file */
/*  and/or replace the original file pointer */
fitsfile *ffhist3(fitsfile *fptr, /* I - ptr to table with X and Y cols*/
           char *outfile,    /* I - name for the output histogram file      */
           int imagetype,    /* I - datatype for image: TINT, TSHORT, etc   */
           int naxis,        /* I - number of axes in the histogram image   */
           char colname[4][FLEN_VALUE],   /* I - column names               */
           double *minin,     /* I - minimum histogram value, for each axis */
           double *maxin,     /* I - maximum histogram value, for each axis */
           double *binsizein, /* I - bin size along each axis               */
           char minname[4][FLEN_VALUE], /* I - optional keywords for min    */
           char maxname[4][FLEN_VALUE], /* I - optional keywords for max    */
           char binname[4][FLEN_VALUE], /* I - optional keywords for binsize */
           double weightin,        /* I - binning weighting factor          */
           char wtcol[FLEN_VALUE], /* I - optional keyword or col for weight*/
           int recip,              /* I - use reciprocal of the weight?     */
           char *selectrow,        /* I - optional array (length = no. of   */
                             /* rows in the table).  If the element is true */
                             /* then the corresponding row of the table will*/
                             /* be included in the histogram, otherwise the */
                             /* row will be skipped.  Ingnored if *selectrow*/
                             /* is equal to NULL.                           */
           int *status)
{
    fitsfile *histptr;
    int   bitpix, colnum[4], wtcolnum;
    long haxes[4];
    float amin[4], amax[4], binsize[4],  weight;

    if (*status > 0)
        return(NULL);

    if (naxis > 4)
    {
        ffpmsg("histogram has more than 4 dimensions");
	*status = BAD_DIMEN;
        return(NULL);
    }

    /* reset position to the correct HDU if necessary */
    if ((fptr)->HDUposition != ((fptr)->Fptr)->curhdu)
        ffmahd(fptr, ((fptr)->HDUposition) + 1, NULL, status);

    if (imagetype == TBYTE)
        bitpix = BYTE_IMG;
    else if (imagetype == TSHORT)
        bitpix = SHORT_IMG;
    else if (imagetype == TINT)
        bitpix = LONG_IMG;
    else if (imagetype == TFLOAT)
        bitpix = FLOAT_IMG;
    else if (imagetype == TDOUBLE)
        bitpix = DOUBLE_IMG;
    else{
        *status = BAD_DATATYPE;
        return(NULL);
    }
    
    /*    Calculate the binning parameters:    */
    /*   columm numbers, axes length, min values,  max values, and binsizes.  */

    if (fits_calc_binning(
      fptr, naxis, colname, minin, maxin, binsizein, minname, maxname, binname,
      colnum, haxes, amin, amax, binsize, status) > 0)
    {
       ffpmsg("failed to determine binning parameters");
        return(NULL);
    }
 
    /* get the histogramming weighting factor, if any */
    if (*wtcol)
    {
        /* first, look for a keyword with the weight value */
        if (fits_read_key(fptr, TFLOAT, wtcol, &weight, NULL, status) )
        {
            /* not a keyword, so look for column with this name */
            *status = 0;

            /* get the column number in the table */
            if (ffgcno(fptr, CASEINSEN, wtcol, &wtcolnum, status) > 0)
            {
               ffpmsg(
               "keyword or column for histogram weights doesn't exist: ");
               ffpmsg(wtcol);
               return(NULL);
            }

            weight = FLOATNULLVALUE;
        }
    }
    else
        weight = (float) weightin;

    if (weight <= 0. && weight != FLOATNULLVALUE)
    {
        ffpmsg("Illegal histogramming weighting factor <= 0.");
	*status = URL_PARSE_ERROR;
        return(NULL);
    }

    if (recip && weight != FLOATNULLVALUE)
       /* take reciprocal of weight */
       weight = (float) (1.0 / weight);

    /* size of histogram is now known, so create temp output file */
    if (fits_create_file(&histptr, outfile, status) > 0)
    {
        ffpmsg("failed to create temp output file for histogram");
        return(NULL);
    }

    /* create output FITS image HDU */
    if (ffcrim(histptr, bitpix, naxis, haxes, status) > 0)
    {
        ffpmsg("failed to create output histogram FITS image");
        return(NULL);
    }

    /* copy header keywords, converting pixel list WCS keywords to image WCS */
    if (fits_copy_pixlist2image(fptr, histptr, 9, naxis, colnum, status) > 0)
    {
        ffpmsg("failed to copy pixel list keywords to new histogram header");
        return(NULL);
    }

    /* if the table columns have no WCS keywords, then write default keywords */
    fits_write_keys_histo(fptr, histptr, naxis, colnum, status);
    
    /* update the WCS keywords for the ref. pixel location, and pixel size */
    fits_rebin_wcs(histptr, naxis, amin, binsize,  status);      
    
    /* now compute the output image by binning the column values */
    if (fits_make_hist(fptr, histptr, bitpix, naxis, haxes, colnum, amin, amax,
        binsize, weight, wtcolnum, recip, selectrow, status) > 0)
    {
        ffpmsg("failed to calculate new histogram values");
        return(NULL);
    }
              
    return(histptr);
}
/*--------------------------------------------------------------------------*/
int ffhist(fitsfile **fptr,  /* IO - pointer to table with X and Y cols;    */
                             /*     on output, points to histogram image    */
           char *outfile,    /* I - name for the output histogram file      */
           int imagetype,    /* I - datatype for image: TINT, TSHORT, etc   */
           int naxis,        /* I - number of axes in the histogram image   */
           char colname[4][FLEN_VALUE],   /* I - column names               */
           double *minin,     /* I - minimum histogram value, for each axis */
           double *maxin,     /* I - maximum histogram value, for each axis */
           double *binsizein, /* I - bin size along each axis               */
           char minname[4][FLEN_VALUE], /* I - optional keywords for min    */
           char maxname[4][FLEN_VALUE], /* I - optional keywords for max    */
           char binname[4][FLEN_VALUE], /* I - optional keywords for binsize */
           double weightin,        /* I - binning weighting factor          */
           char wtcol[FLEN_VALUE], /* I - optional keyword or col for weight*/
           int recip,              /* I - use reciprocal of the weight?     */
           char *selectrow,        /* I - optional array (length = no. of   */
                             /* rows in the table).  If the element is true */
                             /* then the corresponding row of the table will*/
                             /* be included in the histogram, otherwise the */
                             /* row will be skipped.  Ingnored if *selectrow*/
                             /* is equal to NULL.                           */
           int *status)
{
    int ii, datatype, repeat, imin, imax, ibin, bitpix, tstatus, use_datamax = 0;
    long haxes[4];
    fitsfile *histptr;
    char errmsg[FLEN_ERRMSG], keyname[FLEN_KEYWORD], card[FLEN_CARD];
    tcolumn *colptr;
    iteratorCol imagepars[1];
    int n_cols = 1, nkeys;
    long  offset = 0;
    long n_per_loop = -1;  /* force whole array to be passed at one time */
    histType histData;    /* Structure holding histogram info for iterator */
    
    float amin[4], amax[4], binsize[4], maxbin[4];
    float datamin = FLOATNULLVALUE, datamax = FLOATNULLVALUE;
    char svalue[FLEN_VALUE];
    double dvalue;
    char cpref[4][FLEN_VALUE];
    char *cptr;

    if (*status > 0)
        return(*status);

    if (naxis > 4)
    {
        ffpmsg("histogram has more than 4 dimensions");
        return(*status = BAD_DIMEN);
    }

    /* reset position to the correct HDU if necessary */
    if ((*fptr)->HDUposition != ((*fptr)->Fptr)->curhdu)
        ffmahd(*fptr, ((*fptr)->HDUposition) + 1, NULL, status);

    histData.tblptr     = *fptr;
    histData.himagetype = imagetype;
    histData.haxis      = naxis;
    histData.rowselector = selectrow;

    if (imagetype == TBYTE)
        bitpix = BYTE_IMG;
    else if (imagetype == TSHORT)
        bitpix = SHORT_IMG;
    else if (imagetype == TINT)
        bitpix = LONG_IMG;
    else if (imagetype == TFLOAT)
        bitpix = FLOAT_IMG;
    else if (imagetype == TDOUBLE)
        bitpix = DOUBLE_IMG;
    else
        return(*status = BAD_DATATYPE);

    /* The CPREF keyword, if it exists, gives the preferred columns. */
    /* Otherwise, assume "X", "Y", "Z", and "T"  */

    tstatus = 0;
    ffgky(*fptr, TSTRING, "CPREF", cpref[0], NULL, &tstatus);

    if (!tstatus)
    {
        /* Preferred column names are given;  separate them */
        cptr = cpref[0];

        /* the first preferred axis... */
        while (*cptr != ',' && *cptr != '\0')
           cptr++;

        if (*cptr != '\0')
        {
           *cptr = '\0';
           cptr++;
           while (*cptr == ' ')
               cptr++;

           strcpy(cpref[1], cptr);
           cptr = cpref[1];

          /* the second preferred axis... */
          while (*cptr != ',' && *cptr != '\0')
             cptr++;

          if (*cptr != '\0')
          {
             *cptr = '\0';
             cptr++;
             while (*cptr == ' ')
                 cptr++;

             strcpy(cpref[2], cptr);
             cptr = cpref[2];

            /* the third preferred axis... */
            while (*cptr != ',' && *cptr != '\0')
               cptr++;

            if (*cptr != '\0')
            {
               *cptr = '\0';
               cptr++;
               while (*cptr == ' ')
                   cptr++;

               strcpy(cpref[3], cptr);

            }
          }
        }
    }

    for (ii = 0; ii < naxis; ii++)
    {

      /* get the min, max, and binsize values from keywords, if specified */

      if (*minname[ii])
      {
         if (ffgky(*fptr, TDOUBLE, minname[ii], &minin[ii], NULL, status) )
         {
             ffpmsg("error reading histogramming minimum keyword");
             ffpmsg(minname[ii]);
             return(*status);
         }
      }

      if (*maxname[ii])
      {
         if (ffgky(*fptr, TDOUBLE, maxname[ii], &maxin[ii], NULL, status) )
         {
             ffpmsg("error reading histogramming maximum keyword");
             ffpmsg(maxname[ii]);
             return(*status);
         }
      }

      if (*binname[ii])
      {
         if (ffgky(*fptr, TDOUBLE, binname[ii], &binsizein[ii], NULL, status) )
         {
             ffpmsg("error reading histogramming binsize keyword");
             ffpmsg(binname[ii]);
             return(*status);
         }
      }

      if (binsizein[ii] == 0.)
      {
        ffpmsg("error: histogram binsize = 0");
        return(*status = ZERO_SCALE);
      }

      if (*colname[ii] == '\0')
      {
         strcpy(colname[ii], cpref[ii]); /* try using the preferred column */
         if (*colname[ii] == '\0')
         {
           if (ii == 0)
              strcpy(colname[ii], "X");
           else if (ii == 1)
              strcpy(colname[ii], "Y");
           else if (ii == 2)
              strcpy(colname[ii], "Z");
           else if (ii == 3)
              strcpy(colname[ii], "T");
         }
      }

      /* get the column number in the table */
      if (ffgcno(*fptr, CASEINSEN, colname[ii], histData.hcolnum+ii, status)
              > 0)
      {
        strcpy(errmsg, "column for histogram axis doesn't exist: ");
        strcat(errmsg, colname[ii]);
        ffpmsg(errmsg);
        return(*status);
      }

      colptr = ((*fptr)->Fptr)->tableptr;
      colptr += (histData.hcolnum[ii] - 1);

      repeat = (int) colptr->trepeat;  /* vector repeat factor of the column */
      if (repeat > 1)
      {
        strcpy(errmsg, "Can't bin a vector column: ");
        strcat(errmsg, colname[ii]);
        ffpmsg(errmsg);
        return(*status = BAD_DATATYPE);
      }

      /* get the datatype of the column */
      fits_get_coltype(*fptr, histData.hcolnum[ii], &datatype,
         NULL, NULL, status);

      if (datatype < 0 || datatype == TSTRING)
      {
        strcpy(errmsg, "Inappropriate datatype; can't bin this column: ");
        strcat(errmsg, colname[ii]);
        ffpmsg(errmsg);
        return(*status = BAD_DATATYPE);
      }

      /* use TLMINn and TLMAXn keyword values if min and max were not given */
      /* else use actual data min and max if TLMINn and TLMAXn don't exist */
 
      if (minin[ii] == DOUBLENULLVALUE)
      {
        ffkeyn("TLMIN", histData.hcolnum[ii], keyname, status);
        if (ffgky(*fptr, TFLOAT, keyname, amin+ii, NULL, status) > 0)
        {
            /* use actual data minimum value for the histogram minimum */
            *status = 0;
            if (fits_get_col_minmax(*fptr, histData.hcolnum[ii], amin+ii, &datamax, status) > 0)
            {
                strcpy(errmsg, "Error calculating datamin and datamax for column: ");
                strcat(errmsg, colname[ii]);
                ffpmsg(errmsg);
                return(*status);
            }
         }
      }
      else
      {
        amin[ii] = (float) minin[ii];
      }

      if (maxin[ii] == DOUBLENULLVALUE)
      {
        ffkeyn("TLMAX", histData.hcolnum[ii], keyname, status);
        if (ffgky(*fptr, TFLOAT, keyname, &amax[ii], NULL, status) > 0)
        {
          *status = 0;
          if(datamax != FLOATNULLVALUE)  /* already computed max value */
          {
             amax[ii] = datamax;
          }
          else
          {
             /* use actual data maximum value for the histogram maximum */
             if (fits_get_col_minmax(*fptr, histData.hcolnum[ii], &datamin, &amax[ii], status) > 0)
             {
                 strcpy(errmsg, "Error calculating datamin and datamax for column: ");
                 strcat(errmsg, colname[ii]);
                 ffpmsg(errmsg);
                 return(*status);
             }
          }
        }
        use_datamax = 1;  /* flag that the max was determined by the data values */
                          /* and not specifically set by the calling program */
      }
      else
      {
        amax[ii] = (float) maxin[ii];
      }

      /* use TDBINn keyword or else 1 if bin size is not given */
      if (binsizein[ii] == DOUBLENULLVALUE)
      {
         tstatus = 0;
         ffkeyn("TDBIN", histData.hcolnum[ii], keyname, &tstatus);

         if (ffgky(*fptr, TDOUBLE, keyname, binsizein + ii, NULL, &tstatus) > 0)
         {
	    /* make at least 10 bins */
            binsizein[ii] = (amax[ii] - amin[ii]) / 10. ;
            if (binsizein[ii] > 1.)
                binsizein[ii] = 1.;  /* use default bin size */
         }
      }

      if ( (amin[ii] > amax[ii] && binsizein[ii] > 0. ) ||
           (amin[ii] < amax[ii] && binsizein[ii] < 0. ) )
          binsize[ii] = (float) -binsizein[ii];  /* reverse the sign of binsize */
      else
          binsize[ii] =  (float) binsizein[ii];  /* binsize has the correct sign */

      ibin = (int) binsize[ii];
      imin = (int) amin[ii];
      imax = (int) amax[ii];

      /* Determine the range and number of bins in the histogram. This  */
      /* depends on whether the input columns are integer or floats, so */
      /* treat each case separately.                                    */

      if (datatype <= TLONG && (float) imin == amin[ii] &&
                               (float) imax == amax[ii] &&
                               (float) ibin == binsize[ii] )
      {
        /* This is an integer column and integer limits were entered. */
        /* Shift the lower and upper histogramming limits by 0.5, so that */
        /* the values fall in the center of the bin, not on the edge. */

        haxes[ii] = (imax - imin) / ibin + 1;  /* last bin may only */
                                               /* be partially full */
        maxbin[ii] = (float) (haxes[ii] + 1.);  /* add 1. instead of .5 to avoid roundoff */

        if (amin[ii] < amax[ii])
        {
          amin[ii] = (float) (amin[ii] - 0.5);
          amax[ii] = (float) (amax[ii] + 0.5);
        }
        else
        {
          amin[ii] = (float) (amin[ii] + 0.5);
          amax[ii] = (float) (amax[ii] - 0.5);
        }
      }
      else if (use_datamax)  
      {
        /* Either the column datatype and/or the limits are floating point, */
        /* and the histogram limits are being defined by the min and max */
        /* values of the array.  Add 1 to the number of histogram bins to */
        /* make sure that pixels that are equal to the maximum or are */
        /* in the last partial bin are included.  */

        maxbin[ii] = (amax[ii] - amin[ii]) / binsize[ii]; 
        haxes[ii] = (long) (maxbin[ii] + 1);
      }
      else  
      {
        /*  float datatype column and/or limits, and the maximum value to */
        /*  include in the histogram is specified by the calling program. */
        /*  The lower limit is inclusive, but upper limit is exclusive    */
        maxbin[ii] = (amax[ii] - amin[ii]) / binsize[ii];
        haxes[ii] = (long) maxbin[ii];

        if (amin[ii] < amax[ii])
        {
          if (amin[ii] + (haxes[ii] * binsize[ii]) < amax[ii])
            haxes[ii]++;   /* need to include another partial bin */
        }
        else
        {
          if (amin[ii] + (haxes[ii] * binsize[ii]) > amax[ii])
            haxes[ii]++;   /* need to include another partial bin */
        }
      }
    }

       /* get the histogramming weighting factor */
    if (*wtcol)
    {
        /* first, look for a keyword with the weight value */
        if (ffgky(*fptr, TFLOAT, wtcol, &histData.weight, NULL, status) )
        {
            /* not a keyword, so look for column with this name */
            *status = 0;

            /* get the column number in the table */
            if (ffgcno(*fptr, CASEINSEN, wtcol, &histData.wtcolnum, status) > 0)
            {
               ffpmsg(
               "keyword or column for histogram weights doesn't exist: ");
               ffpmsg(wtcol);
               return(*status);
            }

            histData.weight = FLOATNULLVALUE;
        }
    }
    else
        histData.weight = (float) weightin;

    if (histData.weight <= 0. && histData.weight != FLOATNULLVALUE)
    {
        ffpmsg("Illegal histogramming weighting factor <= 0.");
        return(*status = URL_PARSE_ERROR);
    }

    if (recip && histData.weight != FLOATNULLVALUE)
       /* take reciprocal of weight */
       histData.weight = (float) (1.0 / histData.weight);

    histData.wtrecip = recip;
        
    /* size of histogram is now known, so create temp output file */
    if (ffinit(&histptr, outfile, status) > 0)
    {
        ffpmsg("failed to create temp output file for histogram");
        return(*status);
    }

    if (ffcrim(histptr, bitpix, histData.haxis, haxes, status) > 0)
    {
        ffpmsg("failed to create primary array histogram in temp file");
        ffclos(histptr, status);
        return(*status);
    }

    /* copy all non-structural keywords from the table to the image */
    fits_get_hdrspace(*fptr, &nkeys, NULL, status);
    for (ii = 1; ii <= nkeys; ii++)
    {
       fits_read_record(*fptr, ii, card, status);
       if (fits_get_keyclass(card) >= 120)
           fits_write_record(histptr, card, status);
    }           

    /* Set global variables with histogram parameter values.    */
    /* Use separate scalar variables rather than arrays because */
    /* it is more efficient when computing the histogram.       */

    histData.amin1 = amin[0];
    histData.maxbin1 = maxbin[0];
    histData.binsize1 = binsize[0];
    histData.haxis1 = haxes[0];

    if (histData.haxis > 1)
    {
      histData.amin2 = amin[1];
      histData.maxbin2 = maxbin[1];
      histData.binsize2 = binsize[1];
      histData.haxis2 = haxes[1];

      if (histData.haxis > 2)
      {
        histData.amin3 = amin[2];
        histData.maxbin3 = maxbin[2];
        histData.binsize3 = binsize[2];
        histData.haxis3 = haxes[2];

        if (histData.haxis > 3)
        {
          histData.amin4 = amin[3];
          histData.maxbin4 = maxbin[3];
          histData.binsize4 = binsize[3];
          histData.haxis4 = haxes[3];
        }
      }
    }

    /* define parameters of image for the iterator function */
    fits_iter_set_file(imagepars, histptr);        /* pointer to image */
    fits_iter_set_datatype(imagepars, imagetype);  /* image datatype   */
    fits_iter_set_iotype(imagepars, OutputCol);    /* image is output  */

    /* call the iterator function to write out the histogram image */
    if (fits_iterate_data(n_cols, imagepars, offset, n_per_loop,
                          ffwritehisto, (void*)&histData, status) )
         return(*status);

    /* write the World Coordinate System (WCS) keywords */
    /* create default values if WCS keywords are not present in the table */
    for (ii = 0; ii < histData.haxis; ii++)
    {
     /*  CTYPEn  */
       tstatus = 0;
       ffkeyn("TCTYP", histData.hcolnum[ii], keyname, &tstatus);
       ffgky(*fptr, TSTRING, keyname, svalue, NULL, &tstatus);
       if (tstatus)
       {               /* just use column name as the type */
          tstatus = 0;
          ffkeyn("TTYPE", histData.hcolnum[ii], keyname, &tstatus);
          ffgky(*fptr, TSTRING, keyname, svalue, NULL, &tstatus);
       }

       if (!tstatus)
       {
        ffkeyn("CTYPE", ii + 1, keyname, &tstatus);
        ffpky(histptr, TSTRING, keyname, svalue, "Coordinate Type", &tstatus);
       }
       else
          tstatus = 0;

     /*  CUNITn  */
       ffkeyn("TCUNI", histData.hcolnum[ii], keyname, &tstatus);
       ffgky(*fptr, TSTRING, keyname, svalue, NULL, &tstatus);
       if (tstatus)
       {         /* use the column units */
          tstatus = 0;
          ffkeyn("TUNIT", histData.hcolnum[ii], keyname, &tstatus);
          ffgky(*fptr, TSTRING, keyname, svalue, NULL, &tstatus);
       }

       if (!tstatus)
       {
        ffkeyn("CUNIT", ii + 1, keyname, &tstatus);
        ffpky(histptr, TSTRING, keyname, svalue, "Coordinate Units", &tstatus);
       }
       else
         tstatus = 0;

     /*  CRPIXn  - Reference Pixel  */
       ffkeyn("TCRPX", histData.hcolnum[ii], keyname, &tstatus);
       ffgky(*fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus);
       if (tstatus)
       {
         dvalue = 1.0; /* choose first pixel in new image as ref. pix. */
         tstatus = 0;
       }
       else
       {
           /* calculate locate of the ref. pix. in the new image */
           dvalue = (dvalue - amin[ii]) / binsize[ii] + .5;
       }

       ffkeyn("CRPIX", ii + 1, keyname, &tstatus);
       ffpky(histptr, TDOUBLE, keyname, &dvalue, "Reference Pixel", &tstatus);

     /*  CRVALn - Value at the location of the reference pixel */
       ffkeyn("TCRVL", histData.hcolnum[ii], keyname, &tstatus);
       ffgky(*fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus);
       if (tstatus)
       {
         /* calculate value at ref. pix. location (at center of 1st pixel) */
         dvalue = amin[ii] + binsize[ii]/2.;
         tstatus = 0;
       }

       ffkeyn("CRVAL", ii + 1, keyname, &tstatus);
       ffpky(histptr, TDOUBLE, keyname, &dvalue, "Reference Value", &tstatus);

     /*  CDELTn - unit size of pixels  */
       ffkeyn("TCDLT", histData.hcolnum[ii], keyname, &tstatus);
       ffgky(*fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus);
       if (tstatus)
       {
         dvalue = 1.0;  /* use default pixel size */
         tstatus = 0;
       }

       dvalue = dvalue * binsize[ii];
       ffkeyn("CDELT", ii + 1, keyname, &tstatus);
       ffpky(histptr, TDOUBLE, keyname, &dvalue, "Pixel size", &tstatus);

     /*  CROTAn - Rotation angle (degrees CCW)  */
     /*  There should only be a CROTA2 keyword, and only for 2+ D images */
       if (ii == 1)
       {
         ffkeyn("TCROT", histData.hcolnum[ii], keyname, &tstatus);
         ffgky(*fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus);
         if (!tstatus && dvalue != 0.)  /* only write keyword if angle != 0 */
         {
           ffkeyn("CROTA", ii + 1, keyname, &tstatus);
           ffpky(histptr, TDOUBLE, keyname, &dvalue,
                 "Rotation angle", &tstatus);
         }
         else
         {
            /* didn't find CROTA for the 2nd axis, so look for one */
            /* on the first axis */
           tstatus = 0;
           ffkeyn("TCROT", histData.hcolnum[0], keyname, &tstatus);
           ffgky(*fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus);
           if (!tstatus && dvalue != 0.)  /* only write keyword if angle != 0 */
           {
             dvalue *= -1.;   /* negate the value, because mirror image */
             ffkeyn("CROTA", ii + 1, keyname, &tstatus);
             ffpky(histptr, TDOUBLE, keyname, &dvalue,
                   "Rotation angle", &tstatus);
           }
         }
       }
    }

    /* convert any TPn_k keywords to PCi_j; the value remains unchanged */
    /* also convert any TCn_k to CDi_j; the value is modified by n binning size */
    /* This is a bit of a kludge, and only works for 2D WCS */

    if (histData.haxis == 2) {

      /* PC1_1 */
      tstatus = 0;
      ffkeyn("TP", histData.hcolnum[0], card, &tstatus);
      strcat(card,"_");
      ffkeyn(card, histData.hcolnum[0], keyname, &tstatus);
      ffgky(*fptr, TDOUBLE, keyname, &dvalue, card, &tstatus);
      if (!tstatus) 
         ffpky(histptr, TDOUBLE, "PC1_1", &dvalue, card, &tstatus);

      tstatus = 0;
      keyname[1] = 'C';
      ffgky(*fptr, TDOUBLE, keyname, &dvalue, card, &tstatus);
      if (!tstatus) {
         dvalue *=  binsize[0];
         ffpky(histptr, TDOUBLE, "CD1_1", &dvalue, card, &tstatus);
      }

      /* PC1_2 */
      tstatus = 0;
      ffkeyn("TP", histData.hcolnum[0], card, &tstatus);
      strcat(card,"_");
      ffkeyn(card, histData.hcolnum[1], keyname, &tstatus);
      ffgky(*fptr, TDOUBLE, keyname, &dvalue, card, &tstatus);
      if (!tstatus) 
         ffpky(histptr, TDOUBLE, "PC1_2", &dvalue, card, &tstatus);
 
      tstatus = 0;
      keyname[1] = 'C';
      ffgky(*fptr, TDOUBLE, keyname, &dvalue, card, &tstatus);
      if (!tstatus) {
        dvalue *=  binsize[0];
        ffpky(histptr, TDOUBLE, "CD1_2", &dvalue, card, &tstatus);
      }
       
      /* PC2_1 */
      tstatus = 0;
      ffkeyn("TP", histData.hcolnum[1], card, &tstatus);
      strcat(card,"_");
      ffkeyn(card, histData.hcolnum[0], keyname, &tstatus);
      ffgky(*fptr, TDOUBLE, keyname, &dvalue, card, &tstatus);
      if (!tstatus) 
         ffpky(histptr, TDOUBLE, "PC2_1", &dvalue, card, &tstatus);
 
      tstatus = 0;
      keyname[1] = 'C';
      ffgky(*fptr, TDOUBLE, keyname, &dvalue, card, &tstatus);
      if (!tstatus) {
         dvalue *=  binsize[1];
         ffpky(histptr, TDOUBLE, "CD2_1", &dvalue, card, &tstatus);
      }
       
       /* PC2_2 */
      tstatus = 0;
      ffkeyn("TP", histData.hcolnum[1], card, &tstatus);
      strcat(card,"_");
      ffkeyn(card, histData.hcolnum[1], keyname, &tstatus);
      ffgky(*fptr, TDOUBLE, keyname, &dvalue, card, &tstatus);
      if (!tstatus) 
         ffpky(histptr, TDOUBLE, "PC2_2", &dvalue, card, &tstatus);
        
      tstatus = 0;
      keyname[1] = 'C';
      ffgky(*fptr, TDOUBLE, keyname, &dvalue, card, &tstatus);
      if (!tstatus) {
         dvalue *=  binsize[1];
         ffpky(histptr, TDOUBLE, "CD2_2", &dvalue, card, &tstatus);
      }
    }   
       
    /* finally, close the original file and return ptr to the new image */
    ffclos(*fptr, status);
    *fptr = histptr;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_calc_binning(
      fitsfile *fptr,  /* IO - pointer to table to be binned      ;       */
      int naxis,       /* I - number of axes/columns in the binned image  */
      char colname[4][FLEN_VALUE],   /* I - optional column names         */
      double *minin,     /* I - optional lower bound value for each axis  */
      double *maxin,     /* I - optional upper bound value, for each axis */
      double *binsizein, /* I - optional bin size along each axis         */
      char minname[4][FLEN_VALUE], /* I - optional keywords for min       */
      char maxname[4][FLEN_VALUE], /* I - optional keywords for max       */
      char binname[4][FLEN_VALUE], /* I - optional keywords for binsize   */

    /* The returned parameters for each axis of the n-dimensional histogram are */

      int *colnum,     /* O - column numbers, to be binned */
      long *haxes,     /* O - number of bins in each histogram axis */
      float *amin,     /* O - lower bound of the histogram axes */
      float *amax,     /* O - upper bound of the histogram axes */
      float *binsize,  /* O - width of histogram bins/pixels on each axis */
      int *status)
/*_
    Calculate the actual binning parameters, based on various user input
    options.
*/
{
    tcolumn *colptr;
    char *cptr, cpref[4][FLEN_VALUE];
    char errmsg[FLEN_ERRMSG], keyname[FLEN_KEYWORD];
    int tstatus, ii;
    int datatype, repeat, imin, imax, ibin,  use_datamax = 0;
    float datamin, datamax;

    /* check inputs */
    
    if (*status > 0)
        return(*status);

    if (naxis > 4)
    {
        ffpmsg("histograms with more than 4 dimensions are not supported");
        return(*status = BAD_DIMEN);
    }

    /* reset position to the correct HDU if necessary */
    if ((fptr)->HDUposition != ((fptr)->Fptr)->curhdu)
        ffmahd(fptr, ((fptr)->HDUposition) + 1, NULL, status);
    
    /* ============================================================= */
    /* The CPREF keyword, if it exists, gives the preferred columns. */
    /* Otherwise, assume "X", "Y", "Z", and "T"  */

    *cpref[0] = '\0';
    *cpref[1] = '\0';
    *cpref[2] = '\0';
    *cpref[3] = '\0';

    tstatus = 0;
    ffgky(fptr, TSTRING, "CPREF", cpref[0], NULL, &tstatus);

    if (!tstatus)
    {
        /* Preferred column names are given;  separate them */
        cptr = cpref[0];

        /* the first preferred axis... */
        while (*cptr != ',' && *cptr != '\0')
           cptr++;

        if (*cptr != '\0')
        {
           *cptr = '\0';
           cptr++;
           while (*cptr == ' ')
               cptr++;

           strcpy(cpref[1], cptr);
           cptr = cpref[1];

          /* the second preferred axis... */
          while (*cptr != ',' && *cptr != '\0')
             cptr++;

          if (*cptr != '\0')
          {
             *cptr = '\0';
             cptr++;
             while (*cptr == ' ')
                 cptr++;

             strcpy(cpref[2], cptr);
             cptr = cpref[2];

            /* the third preferred axis... */
            while (*cptr != ',' && *cptr != '\0')
               cptr++;

            if (*cptr != '\0')
            {
               *cptr = '\0';
               cptr++;
               while (*cptr == ' ')
                   cptr++;

               strcpy(cpref[3], cptr);

            }
          }
        }
    }

    /* ============================================================= */
    /* Main Loop for calculating parameters for each column          */

    for (ii = 0; ii < naxis; ii++)
    {

      /* =========================================================== */
      /* Determine column Number, based on, in order of priority,
         1  input column name, or
	 2  name given by CPREF keyword, or
	 3  assume X, Y, Z and T for the name
      */
	  
      if (*colname[ii] == '\0')
      {
         strcpy(colname[ii], cpref[ii]); /* try using the preferred column */
         if (*colname[ii] == '\0')
         {
           if (ii == 0)
              strcpy(colname[ii], "X");
           else if (ii == 1)
              strcpy(colname[ii], "Y");
           else if (ii == 2)
              strcpy(colname[ii], "Z");
           else if (ii == 3)
              strcpy(colname[ii], "T");
         }
      }

      /* get the column number in the table */
      if (ffgcno(fptr, CASEINSEN, colname[ii], colnum+ii, status)
              > 0)
      {
          strcpy(errmsg, "column for histogram axis doesn't exist: ");
          strcat(errmsg, colname[ii]);
          ffpmsg(errmsg);
          return(*status);
      }

      /* ================================================================ */
      /* check tha column is not a vector or a string                     */

      colptr = ((fptr)->Fptr)->tableptr;
      colptr += (colnum[ii] - 1);

      repeat = (int) colptr->trepeat;  /* vector repeat factor of the column */
      if (repeat > 1)
      {
        strcpy(errmsg, "Can't bin a vector column: ");
        strcat(errmsg, colname[ii]);
        ffpmsg(errmsg);
        return(*status = BAD_DATATYPE);
      }

      /* get the datatype of the column */
      fits_get_coltype(fptr, colnum[ii], &datatype,
         NULL, NULL, status);

      if (datatype < 0 || datatype == TSTRING)
      {
        strcpy(errmsg, "Inappropriate datatype; can't bin this column: ");
        strcat(errmsg, colname[ii]);
        ffpmsg(errmsg);
        return(*status = BAD_DATATYPE);
      }

      /* ================================================================ */
      /* get the minimum value */

      datamin = FLOATNULLVALUE;
      datamax = FLOATNULLVALUE;
      
      if (*minname[ii])
      {
         if (ffgky(fptr, TDOUBLE, minname[ii], &minin[ii], NULL, status) )
         {
             ffpmsg("error reading histogramming minimum keyword");
             ffpmsg(minname[ii]);
             return(*status);
         }
      }

      if (minin[ii] != DOUBLENULLVALUE)
      {
        amin[ii] = (float) minin[ii];
      }
      else
      {
        ffkeyn("TLMIN", colnum[ii], keyname, status);
        if (ffgky(fptr, TFLOAT, keyname, amin+ii, NULL, status) > 0)
        {
            /* use actual data minimum value for the histogram minimum */
            *status = 0;
            if (fits_get_col_minmax(fptr, colnum[ii], amin+ii, &datamax, status) > 0)
            {
                strcpy(errmsg, "Error calculating datamin and datamax for column: ");
                strcat(errmsg, colname[ii]);
                ffpmsg(errmsg);
                return(*status);
            }
         }
      }

      /* ================================================================ */
      /* get the maximum value */

      if (*maxname[ii])
      {
         if (ffgky(fptr, TDOUBLE, maxname[ii], &maxin[ii], NULL, status) )
         {
             ffpmsg("error reading histogramming maximum keyword");
             ffpmsg(maxname[ii]);
             return(*status);
         }
      }

      if (maxin[ii] != DOUBLENULLVALUE)
      {
        amax[ii] = (float) maxin[ii];
      }
      else
      {
        ffkeyn("TLMAX", colnum[ii], keyname, status);
        if (ffgky(fptr, TFLOAT, keyname, &amax[ii], NULL, status) > 0)
        {
          *status = 0;
          if(datamax != FLOATNULLVALUE)  /* already computed max value */
          {
             amax[ii] = datamax;
          }
          else
          {
             /* use actual data maximum value for the histogram maximum */
             if (fits_get_col_minmax(fptr, colnum[ii], &datamin, &amax[ii], status) > 0)
             {
                 strcpy(errmsg, "Error calculating datamin and datamax for column: ");
                 strcat(errmsg, colname[ii]);
                 ffpmsg(errmsg);
                 return(*status);
             }
          }
        }
        use_datamax = 1;  /* flag that the max was determined by the data values */
                          /* and not specifically set by the calling program */
      }


      /* ================================================================ */
      /* determine binning size and range                                 */

      if (*binname[ii])
      {
         if (ffgky(fptr, TDOUBLE, binname[ii], &binsizein[ii], NULL, status) )
         {
             ffpmsg("error reading histogramming binsize keyword");
             ffpmsg(binname[ii]);
             return(*status);
         }
      }

      if (binsizein[ii] == 0.)
      {
        ffpmsg("error: histogram binsize = 0");
        return(*status = ZERO_SCALE);
      }

      /* use TDBINn keyword or else 1 if bin size is not given */
      if (binsizein[ii] != DOUBLENULLVALUE)
      { 
         binsize[ii] = (float) binsizein[ii];
      }
      else
      {
         tstatus = 0;
         ffkeyn("TDBIN", colnum[ii], keyname, &tstatus);

         if (ffgky(fptr, TDOUBLE, keyname, binsizein + ii, NULL, &tstatus) > 0)
         {
	    /* make at least 10 bins */
            binsize[ii] = (amax[ii] - amin[ii]) / 10.F ;
            if (binsize[ii] > 1.)
                binsize[ii] = 1.;  /* use default bin size */
         }
      }

      /* ================================================================ */
      /* if the min is greater than the max, make the binsize negative */
      if ( (amin[ii] > amax[ii] && binsize[ii] > 0. ) ||
           (amin[ii] < amax[ii] && binsize[ii] < 0. ) )
          binsize[ii] =  -binsize[ii];  /* reverse the sign of binsize */


      ibin = (int) binsize[ii];
      imin = (int) amin[ii];
      imax = (int) amax[ii];

      /* Determine the range and number of bins in the histogram. This  */
      /* depends on whether the input columns are integer or floats, so */
      /* treat each case separately.                                    */

      if (datatype <= TLONG && (float) imin == amin[ii] &&
                               (float) imax == amax[ii] &&
                               (float) ibin == binsize[ii] )
      {
        /* This is an integer column and integer limits were entered. */
        /* Shift the lower and upper histogramming limits by 0.5, so that */
        /* the values fall in the center of the bin, not on the edge. */

        haxes[ii] = (imax - imin) / ibin + 1;  /* last bin may only */
                                               /* be partially full */
        if (amin[ii] < amax[ii])
        {
          amin[ii] = (float) (amin[ii] - 0.5);
          amax[ii] = (float) (amax[ii] + 0.5);
        }
        else
        {
          amin[ii] = (float) (amin[ii] + 0.5);
          amax[ii] = (float) (amax[ii] - 0.5);
        }
      }
      else if (use_datamax)  
      {
        /* Either the column datatype and/or the limits are floating point, */
        /* and the histogram limits are being defined by the min and max */
        /* values of the array.  Add 1 to the number of histogram bins to */
        /* make sure that pixels that are equal to the maximum or are */
        /* in the last partial bin are included.  */

        haxes[ii] = (long) (((amax[ii] - amin[ii]) / binsize[ii]) + 1.); 
      }
      else  
      {
        /*  float datatype column and/or limits, and the maximum value to */
        /*  include in the histogram is specified by the calling program. */
        /*  The lower limit is inclusive, but upper limit is exclusive    */
        haxes[ii] = (long) ((amax[ii] - amin[ii]) / binsize[ii]);

        if (amin[ii] < amax[ii])
        {
          if (amin[ii] + (haxes[ii] * binsize[ii]) < amax[ii])
            haxes[ii]++;   /* need to include another partial bin */
        }
        else
        {
          if (amin[ii] + (haxes[ii] * binsize[ii]) > amax[ii])
            haxes[ii]++;   /* need to include another partial bin */
        }
      }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_write_keys_histo(
      fitsfile *fptr,   /* I - pointer to table to be binned              */
      fitsfile *histptr,  /* I - pointer to output histogram image HDU      */
      int naxis,        /* I - number of axes in the histogram image      */
      int *colnum,      /* I - column numbers (array length = naxis)      */
      int *status)     
{      
   /*  Write default WCS keywords in the output histogram image header */
   /*  if the keywords do not already exist.   */

    int ii, tstatus;
    char keyname[FLEN_KEYWORD], svalue[FLEN_VALUE];
    double dvalue;
    
    if (*status > 0)
        return(*status);

    for (ii = 0; ii < naxis; ii++)
    {
     /*  CTYPEn  */
       tstatus = 0;
       ffkeyn("CTYPE", ii+1, keyname, &tstatus);
       ffgky(histptr, TSTRING, keyname, svalue, NULL, &tstatus);
       
       if (!tstatus) continue;  /* keyword already exists, so skip to next axis */
       
       /* use column name as the axis name */
       tstatus = 0;
       ffkeyn("TTYPE", colnum[ii], keyname, &tstatus);
       ffgky(fptr, TSTRING, keyname, svalue, NULL, &tstatus);

       if (!tstatus)
       {
         ffkeyn("CTYPE", ii + 1, keyname, &tstatus);
         ffpky(histptr, TSTRING, keyname, svalue, "Coordinate Type", &tstatus);
       }

       /*  CUNITn,  use the column units */
       tstatus = 0;
       ffkeyn("TUNIT", colnum[ii], keyname, &tstatus);
       ffgky(fptr, TSTRING, keyname, svalue, NULL, &tstatus);

       if (!tstatus)
       {
         ffkeyn("CUNIT", ii + 1, keyname, &tstatus);
         ffpky(histptr, TSTRING, keyname, svalue, "Coordinate Units", &tstatus);
       }

       /*  CRPIXn  - Reference Pixel choose first pixel in new image as ref. pix. */
       dvalue = 1.0;
       tstatus = 0;
       ffkeyn("CRPIX", ii + 1, keyname, &tstatus);
       ffpky(histptr, TDOUBLE, keyname, &dvalue, "Reference Pixel", &tstatus);

       /*  CRVALn - Value at the location of the reference pixel */
       dvalue = 1.0;
       tstatus = 0;
       ffkeyn("CRVAL", ii + 1, keyname, &tstatus);
       ffpky(histptr, TDOUBLE, keyname, &dvalue, "Reference Value", &tstatus);

       /*  CDELTn - unit size of pixels  */
       dvalue = 1.0;  
       tstatus = 0;
       dvalue = 1.;
       ffkeyn("CDELT", ii + 1, keyname, &tstatus);
       ffpky(histptr, TDOUBLE, keyname, &dvalue, "Pixel size", &tstatus);

    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_rebin_wcs(
      fitsfile *fptr,   /* I - pointer to table to be binned           */
      int naxis,        /* I - number of axes in the histogram image   */
      float *amin,        /* I - first pixel include in each axis        */
      float *binsize,     /* I - binning factor for each axis            */
      int *status)      
{      
   /*  Update the  WCS keywords that define the location of the reference */
   /*  pixel, and the pixel size, along each axis.   */

    int ii, jj, tstatus, reset ;
    char keyname[FLEN_KEYWORD], svalue[FLEN_VALUE];
    double dvalue;
    
    if (*status > 0)
        return(*status);
  
    for (ii = 0; ii < naxis; ii++)
    {
       reset = 0;  /* flag to reset the reference pixel */
       tstatus = 0;
       ffkeyn("CRVAL", ii + 1, keyname, &tstatus);
       /* get previous (pre-binning) value */
       ffgky(fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus); 
       if (!tstatus && dvalue == 1.0)
           reset = 1;

       tstatus = 0;
       /*  CRPIXn - update location of the ref. pix. in the binned image */
       ffkeyn("CRPIX", ii + 1, keyname, &tstatus);

       /* get previous (pre-binning) value */
       ffgky(fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus); 

       if (!tstatus)
       {
           if (dvalue != 1.0)
	      reset = 0;

           /* updated value to give pixel location after binning */
           dvalue = (dvalue - amin[ii]) / ((double) binsize[ii]) + .5;  

           fits_modify_key_dbl(fptr, keyname, dvalue, -14, NULL, &tstatus);
       } else {
          reset = 0;
       }

       /*  CDELTn - update unit size of pixels  */
       tstatus = 0;
       ffkeyn("CDELT", ii + 1, keyname, &tstatus);

       /* get previous (pre-binning) value */
       ffgky(fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus); 

       if (!tstatus)
       {
           if (dvalue != 1.0)
	      reset = 0;

           /* updated to give post-binning value */
           dvalue = dvalue * binsize[ii];  

           fits_modify_key_dbl(fptr, keyname, dvalue, -14, NULL, &tstatus);
       }
       else
       {   /* no CDELTn keyword, so look for a CDij keywords */
          reset = 0;

          for (jj = 0; jj < naxis; jj++)
	  {
             tstatus = 0;
             ffkeyn("CD", jj + 1, svalue, &tstatus);
	     strcat(svalue,"_");
	     ffkeyn(svalue, ii + 1, keyname, &tstatus);

             /* get previous (pre-binning) value */
             ffgky(fptr, TDOUBLE, keyname, &dvalue, NULL, &tstatus); 

             if (!tstatus)
             {
                /* updated to give post-binning value */
               dvalue = dvalue * binsize[ii];  

               fits_modify_key_dbl(fptr, keyname, dvalue, -14, NULL, &tstatus);
             }
	  }
       }

       if (reset) {
          /* the original CRPIX, CRVAL, and CDELT keywords were all = 1.0 */
	  /* In this special case, reset the reference pixel to be the */
	  /* first pixel in the array (instead of possibly far off the array) */
 
           dvalue = 1.0;
           ffkeyn("CRPIX", ii + 1, keyname, &tstatus);
           fits_modify_key_dbl(fptr, keyname, dvalue, -14, NULL, &tstatus);

           ffkeyn("CRVAL", ii + 1, keyname, &tstatus);
	   dvalue = amin[ii] + (binsize[ii] / 2.0);	  
           fits_modify_key_dbl(fptr, keyname, dvalue, -14, NULL, &tstatus);
	}

    }
    return(*status);
}
/*--------------------------------------------------------------------------*/

int fits_make_hist(fitsfile *fptr, /* IO - pointer to table with X and Y cols; */
    fitsfile *histptr, /* I - pointer to output FITS image      */
    int bitpix,       /* I - datatype for image: 16, 32, -32, etc    */
    int naxis,        /* I - number of axes in the histogram image   */
    long *naxes,      /* I - size of axes in the histogram image   */
    int *colnum,    /* I - column numbers (array length = naxis)   */
    float *amin,     /* I - minimum histogram value, for each axis */
    float *amax,     /* I - maximum histogram value, for each axis */
    float *binsize, /* I - bin size along each axis               */
    float weight,        /* I - binning weighting factor          */
    int wtcolnum, /* I - optional keyword or col for weight*/
    int recip,              /* I - use reciprocal of the weight?     */
    char *selectrow,        /* I - optional array (length = no. of   */
                             /* rows in the table).  If the element is true */
                             /* then the corresponding row of the table will*/
                             /* be included in the histogram, otherwise the */
                             /* row will be skipped.  Ingnored if *selectrow*/
                             /* is equal to NULL.                           */
    int *status)
{		  
    int ii, imagetype, datatype;
    int n_cols = 1;
    long imin, imax, ibin;
    long  offset = 0;
    long n_per_loop = -1;  /* force whole array to be passed at one time */
    float taxes[4], tmin[4], tmax[4], tbin[4], maxbin[4];
    histType histData;    /* Structure holding histogram info for iterator */
    iteratorCol imagepars[1];

    /* check inputs */
    
    if (*status > 0)
        return(*status);

    if (naxis > 4)
    {
        ffpmsg("histogram has more than 4 dimensions");
        return(*status = BAD_DIMEN);
    }

    if   (bitpix == BYTE_IMG)
         imagetype = TBYTE;
    else if (bitpix == SHORT_IMG)
         imagetype = TSHORT;
    else if (bitpix == LONG_IMG)
         imagetype = TINT;    
    else if (bitpix == FLOAT_IMG)
         imagetype = TFLOAT;    
    else if (bitpix == DOUBLE_IMG)
         imagetype = TDOUBLE;    
    else
        return(*status = BAD_DATATYPE);

    /* reset position to the correct HDU if necessary */
    if ((fptr)->HDUposition != ((fptr)->Fptr)->curhdu)
        ffmahd(fptr, ((fptr)->HDUposition) + 1, NULL, status);

    histData.weight     = weight;
    histData.wtcolnum   = wtcolnum;
    histData.wtrecip    = recip;
    histData.tblptr     = fptr;
    histData.himagetype = imagetype;
    histData.haxis      = naxis;
    histData.rowselector = selectrow;

    for (ii = 0; ii < naxis; ii++)
    {
      taxes[ii] = (float) naxes[ii];
      tmin[ii] = amin[ii];
      tmax[ii] = amax[ii];
      if ( (amin[ii] > amax[ii] && binsize[ii] > 0. ) ||
           (amin[ii] < amax[ii] && binsize[ii] < 0. ) )
          tbin[ii] =  -binsize[ii];  /* reverse the sign of binsize */
      else
          tbin[ii] =   binsize[ii];  /* binsize has the correct sign */
          
      imin = (long) tmin[ii];
      imax = (long) tmax[ii];
      ibin = (long) tbin[ii];
    
      /* get the datatype of the column */
      fits_get_coltype(fptr, colnum[ii], &datatype, NULL, NULL, status);

      if (datatype <= TLONG && (float) imin == tmin[ii] &&
                               (float) imax == tmax[ii] &&
                               (float) ibin == tbin[ii] )
      {
        /* This is an integer column and integer limits were entered. */
        /* Shift the lower and upper histogramming limits by 0.5, so that */
        /* the values fall in the center of the bin, not on the edge. */

        maxbin[ii] = (taxes[ii] + 1.F);  /* add 1. instead of .5 to avoid roundoff */

        if (tmin[ii] < tmax[ii])
        {
          tmin[ii] = tmin[ii] - 0.5F;
          tmax[ii] = tmax[ii] + 0.5F;
        }
        else
        {
          tmin[ii] = tmin[ii] + 0.5F;
          tmax[ii] = tmax[ii] - 0.5F;
        }
      } else {  /* not an integer column with integer limits */
          maxbin[ii] = (tmax[ii] - tmin[ii]) / tbin[ii]; 
      }
    }

    /* Set global variables with histogram parameter values.    */
    /* Use separate scalar variables rather than arrays because */
    /* it is more efficient when computing the histogram.       */

    histData.hcolnum[0]  = colnum[0];
    histData.amin1 = tmin[0];
    histData.maxbin1 = maxbin[0];
    histData.binsize1 = tbin[0];
    histData.haxis1 = (long) taxes[0];

    if (histData.haxis > 1)
    {
      histData.hcolnum[1]  = colnum[1];
      histData.amin2 = tmin[1];
      histData.maxbin2 = maxbin[1];
      histData.binsize2 = tbin[1];
      histData.haxis2 = (long) taxes[1];

      if (histData.haxis > 2)
      {
        histData.hcolnum[2]  = colnum[2];
        histData.amin3 = tmin[2];
        histData.maxbin3 = maxbin[2];
        histData.binsize3 = tbin[2];
        histData.haxis3 = (long) taxes[2];

        if (histData.haxis > 3)
        {
          histData.hcolnum[3]  = colnum[3];
          histData.amin4 = tmin[3];
          histData.maxbin4 = maxbin[3];
          histData.binsize4 = tbin[3];
          histData.haxis4 = (long) taxes[3];
        }
      }
    }

    /* define parameters of image for the iterator function */
    fits_iter_set_file(imagepars, histptr);        /* pointer to image */
    fits_iter_set_datatype(imagepars, imagetype);  /* image datatype   */
    fits_iter_set_iotype(imagepars, OutputCol);    /* image is output  */

    /* call the iterator function to write out the histogram image */
    fits_iterate_data(n_cols, imagepars, offset, n_per_loop,
                          ffwritehisto, (void*)&histData, status);
       
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_get_col_minmax(fitsfile *fptr, int colnum, float *datamin, 
                     float *datamax, int *status)
/* 
   Simple utility routine to compute the min and max value in a column
*/
{
    int anynul;
    long nrows, ntodo, firstrow, ii;
    float array[1000], nulval;

    ffgky(fptr, TLONG, "NAXIS2", &nrows, NULL, status); /* no. of rows */

    firstrow = 1;
    nulval = FLOATNULLVALUE;
    *datamin =  9.0E36F;
    *datamax = -9.0E36F;

    while(nrows)
    {
        ntodo = minvalue(nrows, 100);
        ffgcv(fptr, TFLOAT, colnum, firstrow, 1, ntodo, &nulval, array,
              &anynul, status);

        for (ii = 0; ii < ntodo; ii++)
        {
            if (array[ii] != nulval)
            {
                *datamin = minvalue(*datamin, array[ii]);
                *datamax = maxvalue(*datamax, array[ii]);
            }
        }

        nrows -= ntodo;
        firstrow += ntodo;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffwritehisto(long totaln, long pixoffset, long firstn, long nvalues,
             int narrays, iteratorCol *imagepars, void *userPointer)
/*
   Interator work function that writes out the histogram.
   The histogram values are calculated by another work function, ffcalchisto.
   This work function only gets called once, and totaln = nvalues.
*/
{
    iteratorCol colpars[5];
    int ii, status = 0, ncols;
    long rows_per_loop = 0, offset = 0;
    histType *histData;

    histData = (histType *)userPointer;

    /* store pointer to the histogram array, and initialize to zero */

    switch( histData->himagetype ) {
    case TBYTE:
       histData->hist.b = (char *  ) fits_iter_get_array(imagepars);
       break;
    case TSHORT:
       histData->hist.i = (short * ) fits_iter_get_array(imagepars);
       break;
    case TINT:
       histData->hist.j = (int *   ) fits_iter_get_array(imagepars);
       break;
    case TFLOAT:
       histData->hist.r = (float * ) fits_iter_get_array(imagepars);
       break;
    case TDOUBLE:
       histData->hist.d = (double *) fits_iter_get_array(imagepars);
       break;
    }

    /* set the column parameters for the iterator function */
    for (ii = 0; ii < histData->haxis; ii++)
    {
      fits_iter_set_by_num(&colpars[ii], histData->tblptr,
			   histData->hcolnum[ii], TFLOAT, InputCol);
    }
    ncols = histData->haxis;

    if (histData->weight == FLOATNULLVALUE)
    {
      fits_iter_set_by_num(&colpars[histData->haxis], histData->tblptr,
			   histData->wtcolnum, TFLOAT, InputCol);
      ncols = histData->haxis + 1;
    }

    /* call iterator function to calc the histogram pixel values */

    /* must lock this call in multithreaded environoments because */
    /* the ffcalchist work routine uses static vaiables that would */
    /* get clobbered if multiple threads were running at the same time */
    FFLOCK;
    fits_iterate_data(ncols, colpars, offset, rows_per_loop,
                          ffcalchist, (void*)histData, &status);
    FFUNLOCK;

    return(status);
}
/*--------------------------------------------------------------------------*/
int ffcalchist(long totalrows, long offset, long firstrow, long nrows,
             int ncols, iteratorCol *colpars, void *userPointer)
/*
   Interator work function that calculates values for the 2D histogram.
*/
{
    long ii, ipix, iaxisbin;
    float pix, axisbin;
    static float *col1, *col2, *col3, *col4; /* static to preserve values */
    static float *wtcol;
    static long incr2, incr3, incr4;
    static histType histData;
    static char *rowselect;

    /*  Initialization procedures: execute on the first call  */
    if (firstrow == 1)
    {

      /*  Copy input histogram data to static local variable so we */
      /*  don't have to constantly dereference it.                 */

      histData = *(histType*)userPointer;
      rowselect = histData.rowselector;

      /* assign the input array pointers to local pointers */
      col1 = (float *) fits_iter_get_array(&colpars[0]);
      if (histData.haxis > 1)
      {
        col2 = (float *) fits_iter_get_array(&colpars[1]);
        incr2 = histData.haxis1;

        if (histData.haxis > 2)
        {
          col3 = (float *) fits_iter_get_array(&colpars[2]);
          incr3 = incr2 * histData.haxis2;

          if (histData.haxis > 3)
          {
            col4 = (float *) fits_iter_get_array(&colpars[3]);
            incr4 = incr3 * histData.haxis3;
          }
        }
      }

      if (ncols > histData.haxis)  /* then weights are give in a column */
      {
        wtcol = (float *) fits_iter_get_array(&colpars[histData.haxis]);
      }
    }   /* end of Initialization procedures */

    /*  Main loop: increment the histogram at position of each event */
    for (ii = 1; ii <= nrows; ii++) 
    {
        if (rowselect)     /* if a row selector array is supplied... */
        {
           if (*rowselect)
           {
               rowselect++;   /* this row is included in the histogram */
           }
           else
           {
               rowselect++;   /* this row is excluded from the histogram */
               continue;
           }
        }

        if (col1[ii] == FLOATNULLVALUE)  /* test for null value */
            continue;

        pix = (col1[ii] - histData.amin1) / histData.binsize1;
        ipix = (long) (pix + 1.); /* add 1 because the 1st pixel is the null value */

	/* test if bin is within range */
        if (ipix < 1 || ipix > histData.haxis1 || pix > histData.maxbin1)
            continue;

        if (histData.haxis > 1)
        {
          if (col2[ii] == FLOATNULLVALUE)
              continue;

          axisbin = (col2[ii] - histData.amin2) / histData.binsize2;
          iaxisbin = (long) axisbin;

          if (axisbin < 0. || iaxisbin >= histData.haxis2 || axisbin > histData.maxbin2)
              continue;

          ipix += (iaxisbin * incr2);

          if (histData.haxis > 2)
          {
            if (col3[ii] == FLOATNULLVALUE)
                continue;

            axisbin = (col3[ii] - histData.amin3) / histData.binsize3;
            iaxisbin = (long) axisbin;
            if (axisbin < 0. || iaxisbin >= histData.haxis3 || axisbin > histData.maxbin3)
                continue;

            ipix += (iaxisbin * incr3);
 
            if (histData.haxis > 3)
            {
              if (col4[ii] == FLOATNULLVALUE)
                  continue;

              axisbin = (col4[ii] - histData.amin4) / histData.binsize4;
              iaxisbin = (long) axisbin;
              if (axisbin < 0. || iaxisbin >= histData.haxis4 || axisbin > histData.maxbin4)
                  continue;

              ipix += (iaxisbin * incr4);

            }  /* end of haxis > 3 case */
          }    /* end of haxis > 2 case */
        }      /* end of haxis > 1 case */

        /* increment the histogram pixel */
        if (histData.weight != FLOATNULLVALUE) /* constant weight factor */
        {
            if (histData.himagetype == TINT)
              histData.hist.j[ipix] += (int) histData.weight;
            else if (histData.himagetype == TSHORT)
              histData.hist.i[ipix] += (short) histData.weight;
            else if (histData.himagetype == TFLOAT)
              histData.hist.r[ipix] += histData.weight;
            else if (histData.himagetype == TDOUBLE)
              histData.hist.d[ipix] += histData.weight;
            else if (histData.himagetype == TBYTE)
              histData.hist.b[ipix] += (char) histData.weight;
        }
        else if (histData.wtrecip) /* use reciprocal of the weight */
        {
            if (histData.himagetype == TINT)
              histData.hist.j[ipix] += (int) (1./wtcol[ii]);
            else if (histData.himagetype == TSHORT)
              histData.hist.i[ipix] += (short) (1./wtcol[ii]);
            else if (histData.himagetype == TFLOAT)
              histData.hist.r[ipix] += (float) (1./wtcol[ii]);
            else if (histData.himagetype == TDOUBLE)
              histData.hist.d[ipix] += 1./wtcol[ii];
            else if (histData.himagetype == TBYTE)
              histData.hist.b[ipix] += (char) (1./wtcol[ii]);
        }
        else   /* no weights */
        {
            if (histData.himagetype == TINT)
              histData.hist.j[ipix] += (int) wtcol[ii];
            else if (histData.himagetype == TSHORT)
              histData.hist.i[ipix] += (short) wtcol[ii];
            else if (histData.himagetype == TFLOAT)
              histData.hist.r[ipix] += wtcol[ii];
            else if (histData.himagetype == TDOUBLE)
              histData.hist.d[ipix] += wtcol[ii];
            else if (histData.himagetype == TBYTE)
              histData.hist.b[ipix] += (char) wtcol[ii];
        }

    }  /* end of main loop over all rows */

    return(0);
}

