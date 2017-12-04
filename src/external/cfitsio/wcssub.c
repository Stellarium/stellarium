#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "fitsio2.h"

/*--------------------------------------------------------------------------*/
int fits_read_wcstab(
   fitsfile   *fptr, /* I - FITS file pointer           */
   int  nwtb,        /* Number of arrays to be read from the binary table(s) */
   wtbarr *wtb,      /* Address of the first element of an array of wtbarr
                         typedefs.  This wtbarr typedef is defined below to
                         match the wtbarr struct defined in WCSLIB.  An array
                         of such structs returned by the WCSLIB function
                         wcstab(). */
   int  *status)

/*
*   Author: Mark Calabretta, Australia Telescope National Facility
*   http://www.atnf.csiro.au/~mcalabre/index.html
*
*   fits_read_wcstab() extracts arrays from a binary table required in
*   constructing -TAB coordinates.  This helper routine is intended for
*   use by routines in the WCSLIB library when dealing with the -TAB table
*   look up WCS convention.
*/

{
   int  anynul, colnum, hdunum, iwtb, m, naxis, nostat;
   long *naxes = 0, nelem;
   wtbarr *wtbp;


   if (*status) return *status;

   if (fptr == 0) {
      return (*status = NULL_INPUT_PTR);
   }

   if (nwtb == 0) return 0;

   /* Zero the array pointers. */
   wtbp = wtb;
   for (iwtb = 0; iwtb < nwtb; iwtb++, wtbp++) {
     *wtbp->arrayp = 0x0;
   }

   /* Save HDU number so that we can move back to it later. */
   fits_get_hdu_num(fptr, &hdunum);

   wtbp = wtb;
   for (iwtb = 0; iwtb < nwtb; iwtb++, wtbp++) {
      /* Move to the required binary table extension. */
      if (fits_movnam_hdu(fptr, BINARY_TBL, (char *)(wtbp->extnam),
          wtbp->extver, status)) {
         goto cleanup;
      }

      /* Locate the table column. */
      if (fits_get_colnum(fptr, CASEINSEN, (char *)(wtbp->ttype), &colnum,
          status)) {
         goto cleanup;
      }

      /* Get the array dimensions and check for consistency. */
      if (wtbp->ndim < 1) {
         *status = NEG_AXIS;
         goto cleanup;
      }

      if (!(naxes = calloc(wtbp->ndim, sizeof(long)))) {
         *status = MEMORY_ALLOCATION;
         goto cleanup;
      }

      if (fits_read_tdim(fptr, colnum, wtbp->ndim, &naxis, naxes, status)) {
         goto cleanup;
      }

      if (naxis != wtbp->ndim) {
         if (wtbp->kind == 'c' && wtbp->ndim == 2) {
            /* Allow TDIMn to be omitted for degenerate coordinate arrays. */
            naxis = 2;
            naxes[1] = naxes[0];
            naxes[0] = 1;
         } else {
            *status = BAD_TDIM;
            goto cleanup;
         }
      }

      if (wtbp->kind == 'c') {
         /* Coordinate array; calculate the array size. */
         nelem = naxes[0];
         for (m = 0; m < naxis-1; m++) {
            *(wtbp->dimlen + m) = naxes[m+1];
            nelem *= naxes[m+1];
         }
      } else {
         /* Index vector; check length. */
         if ((nelem = naxes[0]) != *(wtbp->dimlen)) {
            /* N.B. coordinate array precedes the index vectors. */
            *status = BAD_TDIM;
            goto cleanup;
         }
      }

      free(naxes);
      naxes = 0;

      /* Allocate memory for the array. */
      if (!(*wtbp->arrayp = calloc((size_t)nelem, sizeof(double)))) {
         *status = MEMORY_ALLOCATION;
         goto cleanup;
      }

      /* Read the array from the table. */
      if (fits_read_col_dbl(fptr, colnum, wtbp->row, 1L, nelem, 0.0,
          *wtbp->arrayp, &anynul, status)) {
         goto cleanup;
      }
   }

cleanup:
   /* Move back to the starting HDU. */
   nostat = 0;
   fits_movabs_hdu(fptr, hdunum, 0, &nostat);

   /* Release allocated memory. */
   if (naxes) free(naxes);
   if (*status) {
      wtbp = wtb;
      for (iwtb = 0; iwtb < nwtb; iwtb++, wtbp++) {
         if (*wtbp->arrayp) free(*wtbp->arrayp);
      }
   }

   return *status;
}
/*--------------------------------------------------------------------------*/
int ffgiwcs(fitsfile *fptr,  /* I - FITS file pointer                    */
           char **header,   /* O - pointer to the WCS related keywords  */
           int *status)     /* IO - error status                        */
/*
  int fits_get_image_wcs_keys 
  return a string containing all the image WCS header keywords.
  This string is then used as input to the wcsinit WCSlib routine.
  
  THIS ROUTINE IS DEPRECATED. USE fits_hdr2str INSTEAD
*/
{
    int hdutype;

    if (*status > 0)
        return(*status);

    fits_get_hdu_type(fptr, &hdutype, status);
    if (hdutype != IMAGE_HDU)
    {
      ffpmsg(
     "Error in ffgiwcs. This HDU is not an image. Can't read WCS keywords");
      return(*status = NOT_IMAGE);
    }

    /* read header keywords into a long string of chars */
    if (ffh2st(fptr, header, status) > 0)
    {
        ffpmsg("error creating string of image WCS keywords (ffgiwcs)");
        return(*status);
    }

    return(*status);
}

/*--------------------------------------------------------------------------*/
int ffgics(fitsfile *fptr,    /* I - FITS file pointer           */
           double *xrval,     /* O - X reference value           */
           double *yrval,     /* O - Y reference value           */
           double *xrpix,     /* O - X reference pixel           */
           double *yrpix,     /* O - Y reference pixel           */
           double *xinc,      /* O - X increment per pixel       */
           double *yinc,      /* O - Y increment per pixel       */
           double *rot,       /* O - rotation angle (degrees)    */
           char *type,        /* O - type of projection ('-tan') */
           int *status)       /* IO - error status               */
/*
       read the values of the celestial coordinate system keywords.
       These values may be used as input to the subroutines that
       calculate celestial coordinates. (ffxypx, ffwldp)

       Modified in Nov 1999 to convert the CD matrix keywords back
       to the old CDELTn form, and to swap the axes if the dec-like
       axis is given first, and to assume default values if any of the
       keywords are not present.
*/
{
    int tstat = 0, cd_exists = 0, pc_exists = 0;
    char ctype[FLEN_VALUE];
    double cd11 = 0.0, cd21 = 0.0, cd22 = 0.0, cd12 = 0.0;
    double pc11 = 1.0, pc21 = 0.0, pc22 = 1.0, pc12 = 0.0;
    double pi =  3.1415926535897932;
    double phia, phib, temp;
    double toler = .0002;  /* tolerance for angles to agree (radians) */
                           /*   (= approximately 0.01 degrees) */

    if (*status > 0)
       return(*status);

    tstat = 0;
    if (ffgkyd(fptr, "CRVAL1", xrval, NULL, &tstat))
       *xrval = 0.;

    tstat = 0;
    if (ffgkyd(fptr, "CRVAL2", yrval, NULL, &tstat))
       *yrval = 0.;

    tstat = 0;
    if (ffgkyd(fptr, "CRPIX1", xrpix, NULL, &tstat))
        *xrpix = 0.;

    tstat = 0;
    if (ffgkyd(fptr, "CRPIX2", yrpix, NULL, &tstat))
        *yrpix = 0.;

    /* look for CDELTn first, then CDi_j keywords */
    tstat = 0;
    if (ffgkyd(fptr, "CDELT1", xinc, NULL, &tstat))
    {
        /* CASE 1: no CDELTn keyword, so look for the CD matrix */
        tstat = 0;
        if (ffgkyd(fptr, "CD1_1", &cd11, NULL, &tstat))
            tstat = 0;  /* reset keyword not found error */
        else
            cd_exists = 1;  /* found at least 1 CD_ keyword */

        if (ffgkyd(fptr, "CD2_1", &cd21, NULL, &tstat))
            tstat = 0;  /* reset keyword not found error */
        else
            cd_exists = 1;  /* found at least 1 CD_ keyword */

        if (ffgkyd(fptr, "CD1_2", &cd12, NULL, &tstat))
            tstat = 0;  /* reset keyword not found error */
        else
            cd_exists = 1;  /* found at least 1 CD_ keyword */

        if (ffgkyd(fptr, "CD2_2", &cd22, NULL, &tstat))
            tstat = 0;  /* reset keyword not found error */
        else
            cd_exists = 1;  /* found at least 1 CD_ keyword */

        if (cd_exists)  /* convert CDi_j back to CDELTn */
        {
            /* there are 2 ways to compute the angle: */
            phia = atan2( cd21, cd11);
            phib = atan2(-cd12, cd22);

            /* ensure that phia <= phib */
            temp = minvalue(phia, phib);
            phib = maxvalue(phia, phib);
            phia = temp;

            /* there is a possible 180 degree ambiguity in the angles */
            /* so add 180 degress to the smaller value if the values  */
            /* differ by more than 90 degrees = pi/2 radians.         */
            /* (Later, we may decide to take the other solution by    */
            /* subtracting 180 degrees from the larger value).        */

            if ((phib - phia) > (pi / 2.))
               phia += pi;

            if (fabs(phia - phib) > toler) 
            {
               /* angles don't agree, so looks like there is some skewness */
               /* between the axes.  Return with an error to be safe. */
               *status = APPROX_WCS_KEY;
            }
      
            phia = (phia + phib) /2.;  /* use the average of the 2 values */
            *xinc = cd11 / cos(phia);
            *yinc = cd22 / cos(phia);
            *rot = phia * 180. / pi;

            /* common usage is to have a positive yinc value.  If it is */
            /* negative, then subtract 180 degrees from rot and negate  */
            /* both xinc and yinc.  */

            if (*yinc < 0)
            {
                *xinc = -(*xinc);
                *yinc = -(*yinc);
                *rot = *rot - 180.;
            }
        }
        else   /* no CD matrix keywords either */
        {
            *xinc = 1.;

            /* there was no CDELT1 keyword, but check for CDELT2 just in case */
            tstat = 0;
            if (ffgkyd(fptr, "CDELT2", yinc, NULL, &tstat))
                *yinc = 1.;

            tstat = 0;
            if (ffgkyd(fptr, "CROTA2", rot, NULL, &tstat))
                *rot=0.;
        }
    }
    else  /* Case 2: CDELTn + optional PC matrix */
    {
        if (ffgkyd(fptr, "CDELT2", yinc, NULL, &tstat))
            *yinc = 1.;

        tstat = 0;
        if (ffgkyd(fptr, "CROTA2", rot, NULL, &tstat))
        {
            *rot=0.;

            /* no CROTA2 keyword, so look for the PC matrix */
            tstat = 0;
            if (ffgkyd(fptr, "PC1_1", &pc11, NULL, &tstat))
                tstat = 0;  /* reset keyword not found error */
            else
                pc_exists = 1;  /* found at least 1 PC_ keyword */

            if (ffgkyd(fptr, "PC2_1", &pc21, NULL, &tstat))
                tstat = 0;  /* reset keyword not found error */
            else
                pc_exists = 1;  /* found at least 1 PC_ keyword */

            if (ffgkyd(fptr, "PC1_2", &pc12, NULL, &tstat))
                tstat = 0;  /* reset keyword not found error */
            else
                pc_exists = 1;  /* found at least 1 PC_ keyword */

            if (ffgkyd(fptr, "PC2_2", &pc22, NULL, &tstat))
                tstat = 0;  /* reset keyword not found error */
            else
                pc_exists = 1;  /* found at least 1 PC_ keyword */

            if (pc_exists)  /* convert PCi_j back to CDELTn */
            {
                /* there are 2 ways to compute the angle: */
                phia = atan2( pc21, pc11);
                phib = atan2(-pc12, pc22);

                /* ensure that phia <= phib */
                temp = minvalue(phia, phib);
                phib = maxvalue(phia, phib);
                phia = temp;

                /* there is a possible 180 degree ambiguity in the angles */
                /* so add 180 degress to the smaller value if the values  */
                /* differ by more than 90 degrees = pi/2 radians.         */
                /* (Later, we may decide to take the other solution by    */
                /* subtracting 180 degrees from the larger value).        */

                if ((phib - phia) > (pi / 2.))
                   phia += pi;

                if (fabs(phia - phib) > toler) 
                {
                  /* angles don't agree, so looks like there is some skewness */
                  /* between the axes.  Return with an error to be safe. */
                  *status = APPROX_WCS_KEY;
                }
      
                phia = (phia + phib) /2.;  /* use the average of the 2 values */
                *rot = phia * 180. / pi;
            }
        }
    }

    /* get the type of projection, if any */
    tstat = 0;
    if (ffgkys(fptr, "CTYPE1", ctype, NULL, &tstat))
         type[0] = '\0';
    else
    {
        /* copy the projection type string */
        strncpy(type, &ctype[4], 4);
        type[4] = '\0';

        /* check if RA and DEC are inverted */
        if (!strncmp(ctype, "DEC-", 4) || !strncmp(ctype+1, "LAT", 3))
        {
            /* the latitudinal axis is given first, so swap them */

/*
 this case was removed on 12/9.  Apparently not correct.

            if ((*xinc / *yinc) < 0. )  
                *rot = -90. - (*rot);
            else
*/
            *rot = 90. - (*rot);

            /* Empirical tests with ds9 show the y-axis sign must be negated */
            /* and the xinc and yinc values must NOT be swapped. */
            *yinc = -(*yinc);

            temp = *xrval;
            *xrval = *yrval;
            *yrval = temp;
        }   
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgicsa(fitsfile *fptr,    /* I - FITS file pointer           */
           char version,      /* I - character code of desired version */
	                      /*     A - Z or blank */
           double *xrval,     /* O - X reference value           */
           double *yrval,     /* O - Y reference value           */
           double *xrpix,     /* O - X reference pixel           */
           double *yrpix,     /* O - Y reference pixel           */
           double *xinc,      /* O - X increment per pixel       */
           double *yinc,      /* O - Y increment per pixel       */
           double *rot,       /* O - rotation angle (degrees)    */
           char *type,        /* O - type of projection ('-tan') */
           int *status)       /* IO - error status               */
/*
       read the values of the celestial coordinate system keywords.
       These values may be used as input to the subroutines that
       calculate celestial coordinates. (ffxypx, ffwldp)

       Modified in Nov 1999 to convert the CD matrix keywords back
       to the old CDELTn form, and to swap the axes if the dec-like
       axis is given first, and to assume default values if any of the
       keywords are not present.
*/
{
    int tstat = 0, cd_exists = 0, pc_exists = 0;
    char ctype[FLEN_VALUE], keyname[FLEN_VALUE], alt[2];
    double cd11 = 0.0, cd21 = 0.0, cd22 = 0.0, cd12 = 0.0;
    double pc11 = 1.0, pc21 = 0.0, pc22 = 1.0, pc12 = 0.0;
    double pi =  3.1415926535897932;
    double phia, phib, temp;
    double toler = .0002;  /* tolerance for angles to agree (radians) */
                           /*   (= approximately 0.01 degrees) */

    if (*status > 0)
       return(*status);

    if (version == ' ') {
      ffgics(fptr, xrval, yrval, xrpix, yrpix, xinc, yinc, rot, type, status);
      return (*status);
    }

    if (version > 'Z' || version < 'A') {
      ffpmsg("ffgicsa: illegal WCS version code (must be A - Z or blank)");
      return(*status = WCS_ERROR);
    }

    alt[0] = version;
    alt[1] = '\0';
    
    tstat = 0;
    strcpy(keyname, "CRVAL1");
    strcat(keyname, alt);
    if (ffgkyd(fptr, keyname, xrval, NULL, &tstat))
       *xrval = 0.;

    tstat = 0;
    strcpy(keyname, "CRVAL2");
    strcat(keyname, alt);
    if (ffgkyd(fptr, keyname, yrval, NULL, &tstat))
       *yrval = 0.;

    tstat = 0;
    strcpy(keyname, "CRPIX1");
    strcat(keyname, alt);
    if (ffgkyd(fptr, keyname, xrpix, NULL, &tstat))
        *xrpix = 0.;

    tstat = 0;
    strcpy(keyname, "CRPIX2");
    strcat(keyname, alt);
     if (ffgkyd(fptr, keyname, yrpix, NULL, &tstat))
        *yrpix = 0.;

    /* look for CDELTn first, then CDi_j keywords */
    tstat = 0;
    strcpy(keyname, "CDELT1");
    strcat(keyname, alt);
    if (ffgkyd(fptr, keyname, xinc, NULL, &tstat))
    {
        /* CASE 1: no CDELTn keyword, so look for the CD matrix */
        tstat = 0;
        strcpy(keyname, "CD1_1");
        strcat(keyname, alt);
        if (ffgkyd(fptr, keyname, &cd11, NULL, &tstat))
            tstat = 0;  /* reset keyword not found error */
        else
            cd_exists = 1;  /* found at least 1 CD_ keyword */

        strcpy(keyname, "CD2_1");
        strcat(keyname, alt);
        if (ffgkyd(fptr, keyname, &cd21, NULL, &tstat))
            tstat = 0;  /* reset keyword not found error */
        else
            cd_exists = 1;  /* found at least 1 CD_ keyword */

        strcpy(keyname, "CD1_2");
        strcat(keyname, alt);
        if (ffgkyd(fptr, keyname, &cd12, NULL, &tstat))
            tstat = 0;  /* reset keyword not found error */
        else
            cd_exists = 1;  /* found at least 1 CD_ keyword */

        strcpy(keyname, "CD2_2");
        strcat(keyname, alt);
        if (ffgkyd(fptr, keyname, &cd22, NULL, &tstat))
            tstat = 0;  /* reset keyword not found error */
        else
            cd_exists = 1;  /* found at least 1 CD_ keyword */

        if (cd_exists)  /* convert CDi_j back to CDELTn */
        {
            /* there are 2 ways to compute the angle: */
            phia = atan2( cd21, cd11);
            phib = atan2(-cd12, cd22);

            /* ensure that phia <= phib */
            temp = minvalue(phia, phib);
            phib = maxvalue(phia, phib);
            phia = temp;

            /* there is a possible 180 degree ambiguity in the angles */
            /* so add 180 degress to the smaller value if the values  */
            /* differ by more than 90 degrees = pi/2 radians.         */
            /* (Later, we may decide to take the other solution by    */
            /* subtracting 180 degrees from the larger value).        */

            if ((phib - phia) > (pi / 2.))
               phia += pi;

            if (fabs(phia - phib) > toler) 
            {
               /* angles don't agree, so looks like there is some skewness */
               /* between the axes.  Return with an error to be safe. */
               *status = APPROX_WCS_KEY;
            }
      
            phia = (phia + phib) /2.;  /* use the average of the 2 values */
            *xinc = cd11 / cos(phia);
            *yinc = cd22 / cos(phia);
            *rot = phia * 180. / pi;

            /* common usage is to have a positive yinc value.  If it is */
            /* negative, then subtract 180 degrees from rot and negate  */
            /* both xinc and yinc.  */

            if (*yinc < 0)
            {
                *xinc = -(*xinc);
                *yinc = -(*yinc);
                *rot = *rot - 180.;
            }
        }
        else   /* no CD matrix keywords either */
        {
            *xinc = 1.;

            /* there was no CDELT1 keyword, but check for CDELT2 just in case */
            tstat = 0;
            strcpy(keyname, "CDELT2");
            strcat(keyname, alt);
            if (ffgkyd(fptr, keyname, yinc, NULL, &tstat))
                *yinc = 1.;

            tstat = 0;
            strcpy(keyname, "CROTA2");
            strcat(keyname, alt);
            if (ffgkyd(fptr, keyname, rot, NULL, &tstat))
                *rot=0.;
        }
    }
    else  /* Case 2: CDELTn + optional PC matrix */
    {
        strcpy(keyname, "CDELT2");
        strcat(keyname, alt);
        if (ffgkyd(fptr, keyname, yinc, NULL, &tstat))
            *yinc = 1.;

        tstat = 0;
        strcpy(keyname, "CROTA2");
        strcat(keyname, alt);
        if (ffgkyd(fptr, keyname, rot, NULL, &tstat))
        {
            *rot=0.;

            /* no CROTA2 keyword, so look for the PC matrix */
            tstat = 0;
            strcpy(keyname, "PC1_1");
            strcat(keyname, alt);
            if (ffgkyd(fptr, keyname, &pc11, NULL, &tstat))
                tstat = 0;  /* reset keyword not found error */
            else
                pc_exists = 1;  /* found at least 1 PC_ keyword */

            strcpy(keyname, "PC2_1");
            strcat(keyname, alt);
            if (ffgkyd(fptr, keyname, &pc21, NULL, &tstat))
                tstat = 0;  /* reset keyword not found error */
            else
                pc_exists = 1;  /* found at least 1 PC_ keyword */

            strcpy(keyname, "PC1_2");
            strcat(keyname, alt);
            if (ffgkyd(fptr, keyname, &pc12, NULL, &tstat))
                tstat = 0;  /* reset keyword not found error */
            else
                pc_exists = 1;  /* found at least 1 PC_ keyword */

            strcpy(keyname, "PC2_2");
            strcat(keyname, alt);
            if (ffgkyd(fptr, keyname, &pc22, NULL, &tstat))
                tstat = 0;  /* reset keyword not found error */
            else
                pc_exists = 1;  /* found at least 1 PC_ keyword */

            if (pc_exists)  /* convert PCi_j back to CDELTn */
            {
                /* there are 2 ways to compute the angle: */
                phia = atan2( pc21, pc11);
                phib = atan2(-pc12, pc22);

                /* ensure that phia <= phib */
                temp = minvalue(phia, phib);
                phib = maxvalue(phia, phib);
                phia = temp;

                /* there is a possible 180 degree ambiguity in the angles */
                /* so add 180 degress to the smaller value if the values  */
                /* differ by more than 90 degrees = pi/2 radians.         */
                /* (Later, we may decide to take the other solution by    */
                /* subtracting 180 degrees from the larger value).        */

                if ((phib - phia) > (pi / 2.))
                   phia += pi;

                if (fabs(phia - phib) > toler) 
                {
                  /* angles don't agree, so looks like there is some skewness */
                  /* between the axes.  Return with an error to be safe. */
                  *status = APPROX_WCS_KEY;
                }
      
                phia = (phia + phib) /2.;  /* use the average of the 2 values */
                *rot = phia * 180. / pi;
            }
        }
    }

    /* get the type of projection, if any */
    tstat = 0;
    strcpy(keyname, "CTYPE1");
    strcat(keyname, alt);
    if (ffgkys(fptr, keyname, ctype, NULL, &tstat))
         type[0] = '\0';
    else
    {
        /* copy the projection type string */
        strncpy(type, &ctype[4], 4);
        type[4] = '\0';

        /* check if RA and DEC are inverted */
        if (!strncmp(ctype, "DEC-", 4) || !strncmp(ctype+1, "LAT", 3))
        {
            /* the latitudinal axis is given first, so swap them */

            *rot = 90. - (*rot);

            /* Empirical tests with ds9 show the y-axis sign must be negated */
            /* and the xinc and yinc values must NOT be swapped. */
            *yinc = -(*yinc);

            temp = *xrval;
            *xrval = *yrval;
            *yrval = temp;
        }   
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtcs(fitsfile *fptr,    /* I - FITS file pointer           */
           int xcol,          /* I - column containing the RA coordinate  */
           int ycol,          /* I - column containing the DEC coordinate */
           double *xrval,     /* O - X reference value           */
           double *yrval,     /* O - Y reference value           */
           double *xrpix,     /* O - X reference pixel           */
           double *yrpix,     /* O - Y reference pixel           */
           double *xinc,      /* O - X increment per pixel       */
           double *yinc,      /* O - Y increment per pixel       */
           double *rot,       /* O - rotation angle (degrees)    */
           char *type,        /* O - type of projection ('-sin') */
           int *status)       /* IO - error status               */
/*
       read the values of the celestial coordinate system keywords
       from a FITS table where the X and Y or RA and DEC coordinates
       are stored in separate column.  Do this by converting the
       table to a temporary FITS image, then reading the keywords
       from the image file.
       These values may be used as input to the subroutines that
       calculate celestial coordinates. (ffxypx, ffwldp)
*/
{
    int colnum[2];
    long naxes[2];
    fitsfile *tptr;

    if (*status > 0)
       return(*status);

    colnum[0] = xcol;
    colnum[1] = ycol;
    naxes[0] = 10;
    naxes[1] = 10;

    /* create temporary  FITS file, in memory */
    ffinit(&tptr, "mem://", status);
    
    /* create a temporary image; the datatype and size are not important */
    ffcrim(tptr, 32, 2, naxes, status);
    
    /* now copy the relevant keywords from the table to the image */
    fits_copy_pixlist2image(fptr, tptr, 9, 2, colnum, status);

    /* write default WCS keywords, if they are not present */
    fits_write_keys_histo(fptr, tptr, 2, colnum, status);

    if (*status > 0)
       return(*status);
         
    /* read the WCS keyword values from the temporary image */
    ffgics(tptr, xrval, yrval, xrpix, yrpix, xinc, yinc, rot, type, status); 

    if (*status > 0)
    {
      ffpmsg
      ("ffgtcs could not find all the celestial coordinate keywords");
      return(*status = NO_WCS_KEY); 
    }

    /* delete the temporary file */
    fits_delete_file(tptr, status);
    
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtwcs(fitsfile *fptr,  /* I - FITS file pointer              */
           int xcol,        /* I - column number for the X column  */
           int ycol,        /* I - column number for the Y column  */
           char **header,   /* O - string of all the WCS keywords  */
           int *status)     /* IO - error status                   */
/*
  int fits_get_table_wcs_keys
  Return string containing all the WCS keywords appropriate for the 
  pair of X and Y columns containing the coordinate
  of each event in an event list table.  This string may then be passed
  to Doug Mink's WCS library wcsinit routine, to create and initialize the
  WCS structure.  The calling routine must free the header character string
  when it is no longer needed. 

  THIS ROUTINE IS DEPRECATED. USE fits_hdr2str INSTEAD
*/
{
    int hdutype, ncols, tstatus, length;
    int naxis1 = 1, naxis2 = 1;
    long tlmin, tlmax;
    char keyname[FLEN_KEYWORD];
    char valstring[FLEN_VALUE];
    char comm[2];
    char *cptr;
    /*  construct a string of 80 blanks, for adding fill to the keywords */
                 /*  12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
    char blanks[] = "                                                                                ";

    if (*status > 0)
        return(*status);

    fits_get_hdu_type(fptr, &hdutype, status);
    if (hdutype == IMAGE_HDU)
    {
        ffpmsg("Can't read table WSC keywords. This HDU is not a table");
        return(*status = NOT_TABLE);
    }

    fits_get_num_cols(fptr, &ncols, status);
    
    if (xcol < 1 || xcol > ncols)
    {
        ffpmsg("illegal X axis column number in fftwcs");
        return(*status = BAD_COL_NUM);
    }

    if (ycol < 1 || ycol > ncols)
    {
        ffpmsg("illegal Y axis column number in fftwcs");
        return(*status = BAD_COL_NUM);
    }

    /* allocate character string for all the WCS keywords */
    *header = calloc(1, 2401);  /* room for up to 30 keywords */
    if (*header == 0)
    {
        ffpmsg("error allocating memory for WCS header keywords (fftwcs)");
        return(*status = MEMORY_ALLOCATION);
    }

    cptr = *header;
    comm[0] = '\0';
    
    tstatus = 0;
    ffkeyn("TLMIN",xcol,keyname,status);
    ffgkyj(fptr,keyname, &tlmin,NULL,&tstatus);

    if (!tstatus)
    {
        ffkeyn("TLMAX",xcol,keyname,status);
        ffgkyj(fptr,keyname, &tlmax,NULL,&tstatus);
    }

    if (!tstatus)
    {
        naxis1 = tlmax - tlmin + 1;
    }

    tstatus = 0;
    ffkeyn("TLMIN",ycol,keyname,status);
    ffgkyj(fptr,keyname, &tlmin,NULL,&tstatus);

    if (!tstatus)
    {
        ffkeyn("TLMAX",ycol,keyname,status);
        ffgkyj(fptr,keyname, &tlmax,NULL,&tstatus);
    }

    if (!tstatus)
    {
        naxis2 = tlmax - tlmin + 1;
    }

    /*            123456789012345678901234567890    */
    strcat(cptr, "NAXIS   =                    2");
    strncat(cptr, blanks, 50);
    cptr += 80;

    ffi2c(naxis1, valstring, status);   /* convert to formatted string */
    ffmkky("NAXIS1", valstring, comm, cptr, status);  /* construct the keyword*/
    strncat(cptr, blanks, 50);  /* pad with blanks */
    cptr += 80;

    strcpy(keyname, "NAXIS2");
    ffi2c(naxis2, valstring, status);   /* convert to formatted string */
    ffmkky(keyname, valstring, comm, cptr, status);  /* construct the keyword*/
    strncat(cptr, blanks, 50);  /* pad with blanks */
    cptr += 80;

    /* read the required header keywords (use defaults if not found) */

    /*  CTYPE1 keyword */
    tstatus = 0;
    ffkeyn("TCTYP",xcol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) )
       valstring[0] =  '\0';
    ffmkky("CTYPE1", valstring, comm, cptr, status);  /* construct the keyword*/
    length = strlen(cptr);
    strncat(cptr, blanks, 80 - length);  /* pad with blanks */
    cptr += 80;

    /*  CTYPE2 keyword */
    tstatus = 0;
    ffkeyn("TCTYP",ycol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) )
       valstring[0] =  '\0';
    ffmkky("CTYPE2", valstring, comm, cptr, status);  /* construct the keyword*/
    length = strlen(cptr);
    strncat(cptr, blanks, 80 - length);  /* pad with blanks */
    cptr += 80;

    /*  CRPIX1 keyword */
    tstatus = 0;
    ffkeyn("TCRPX",xcol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) )
       strcpy(valstring, "1");
    ffmkky("CRPIX1", valstring, comm, cptr, status);  /* construct the keyword*/
    strncat(cptr, blanks, 50);  /* pad with blanks */
    cptr += 80;

    /*  CRPIX2 keyword */
    tstatus = 0;
    ffkeyn("TCRPX",ycol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) )
       strcpy(valstring, "1");
    ffmkky("CRPIX2", valstring, comm, cptr, status);  /* construct the keyword*/
    strncat(cptr, blanks, 50);  /* pad with blanks */
    cptr += 80;

    /*  CRVAL1 keyword */
    tstatus = 0;
    ffkeyn("TCRVL",xcol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) )
       strcpy(valstring, "1");
    ffmkky("CRVAL1", valstring, comm, cptr, status);  /* construct the keyword*/
    strncat(cptr, blanks, 50);  /* pad with blanks */
    cptr += 80;

    /*  CRVAL2 keyword */
    tstatus = 0;
    ffkeyn("TCRVL",ycol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) )
       strcpy(valstring, "1");
    ffmkky("CRVAL2", valstring, comm, cptr, status);  /* construct the keyword*/
    strncat(cptr, blanks, 50);  /* pad with blanks */
    cptr += 80;

    /*  CDELT1 keyword */
    tstatus = 0;
    ffkeyn("TCDLT",xcol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) )
       strcpy(valstring, "1");
    ffmkky("CDELT1", valstring, comm, cptr, status);  /* construct the keyword*/
    strncat(cptr, blanks, 50);  /* pad with blanks */
    cptr += 80;

    /*  CDELT2 keyword */
    tstatus = 0;
    ffkeyn("TCDLT",ycol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) )
       strcpy(valstring, "1");
    ffmkky("CDELT2", valstring, comm, cptr, status);  /* construct the keyword*/
    strncat(cptr, blanks, 50);  /* pad with blanks */
    cptr += 80;

    /* the following keywords may not exist */

    /*  CROTA2 keyword */
    tstatus = 0;
    ffkeyn("TCROT",ycol,keyname,status);
    if (ffgkey(fptr, keyname, valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("CROTA2", valstring, comm, cptr, status);  /* construct keyword*/
        strncat(cptr, blanks, 50);  /* pad with blanks */
        cptr += 80;
    }

    /*  EPOCH keyword */
    tstatus = 0;
    if (ffgkey(fptr, "EPOCH", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("EPOCH", valstring, comm, cptr, status);  /* construct keyword*/
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    /*  EQUINOX keyword */
    tstatus = 0;
    if (ffgkey(fptr, "EQUINOX", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("EQUINOX", valstring, comm, cptr, status); /* construct keyword*/
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    /*  RADECSYS keyword */
    tstatus = 0;
    if (ffgkey(fptr, "RADECSYS", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("RADECSYS", valstring, comm, cptr, status); /*construct keyword*/
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    /*  TELESCOPE keyword */
    tstatus = 0;
    if (ffgkey(fptr, "TELESCOP", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("TELESCOP", valstring, comm, cptr, status); 
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    /*  INSTRUME keyword */
    tstatus = 0;
    if (ffgkey(fptr, "INSTRUME", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("INSTRUME", valstring, comm, cptr, status);  
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    /*  DETECTOR keyword */
    tstatus = 0;
    if (ffgkey(fptr, "DETECTOR", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("DETECTOR", valstring, comm, cptr, status);  
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    /*  MJD-OBS keyword */
    tstatus = 0;
    if (ffgkey(fptr, "MJD-OBS", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("MJD-OBS", valstring, comm, cptr, status);  
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    /*  DATE-OBS keyword */
    tstatus = 0;
    if (ffgkey(fptr, "DATE-OBS", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("DATE-OBS", valstring, comm, cptr, status);  
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    /*  DATE keyword */
    tstatus = 0;
    if (ffgkey(fptr, "DATE", valstring, NULL, &tstatus) == 0 )
    {
        ffmkky("DATE", valstring, comm, cptr, status);  
        length = strlen(cptr);
        strncat(cptr, blanks, 80 - length);  /* pad with blanks */
        cptr += 80;
    }

    strcat(cptr, "END");
    strncat(cptr, blanks, 77);

    return(*status);
}
