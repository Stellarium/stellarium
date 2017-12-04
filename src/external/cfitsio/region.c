#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "fitsio2.h"
#include "region.h"
static int Pt_in_Poly( double x, double y, int nPts, double *Pts );

/*---------------------------------------------------------------------------*/
int fits_read_rgnfile( const char *filename,
            WCSdata    *wcs,
            SAORegion  **Rgn,
            int        *status )
/*  Read regions from either a FITS or ASCII region file and return the information     */
/*  in the "SAORegion" structure.  If it is nonNULL, use wcs to convert the  */
/*  region coordinates to pixels.  Return an error if region is in degrees   */
/*  but no WCS data is provided.                                             */
/*---------------------------------------------------------------------------*/
{
  fitsfile *fptr;
  int tstatus = 0;

  if( *status ) return( *status );

  /* try to open as a FITS file - if that doesn't work treat as an ASCII file */

  fits_write_errmark();
  if ( ffopen(&fptr, filename, READONLY, &tstatus) ) {
    fits_clear_errmark();
    fits_read_ascii_region(filename, wcs, Rgn, status);
  } else {
    fits_read_fits_region(fptr, wcs, Rgn, status);
  }

  return(*status);

}
/*---------------------------------------------------------------------------*/
int fits_read_ascii_region( const char *filename,
			    WCSdata    *wcs,
			    SAORegion  **Rgn,
			    int        *status )
/*  Read regions from a SAO-style region file and return the information     */
/*  in the "SAORegion" structure.  If it is nonNULL, use wcs to convert the  */
/*  region coordinates to pixels.  Return an error if region is in degrees   */
/*  but no WCS data is provided.                                             */
/*---------------------------------------------------------------------------*/
{
   char     *currLine;
   char     *namePtr, *paramPtr, *currLoc;
   char     *pX, *pY, *endp;
   long     allocLen, lineLen, hh, mm, dd;
   double   *coords, X, Y, x, y, ss, div, xsave= 0., ysave= 0.;
   int      nParams, nCoords, negdec;
   int      i, done;
   FILE     *rgnFile;
   coordFmt cFmt;
   SAORegion *aRgn;
   RgnShape *newShape, *tmpShape;

   if( *status ) return( *status );

   aRgn = (SAORegion *)malloc( sizeof(SAORegion) );
   if( ! aRgn ) {
      ffpmsg("Couldn't allocate memory to hold Region file contents.");
      return(*status = MEMORY_ALLOCATION );
   }
   aRgn->nShapes    =    0;
   aRgn->Shapes     = NULL;
   if( wcs && wcs->exists )
      aRgn->wcs = *wcs;
   else
      aRgn->wcs.exists = 0;

   cFmt = pixel_fmt; /* set default format */

   /*  Allocate Line Buffer  */

   allocLen = 512;
   currLine = (char *)malloc( allocLen * sizeof(char) );
   if( !currLine ) {
      free( aRgn );
      ffpmsg("Couldn't allocate memory to hold Region file contents.");
      return(*status = MEMORY_ALLOCATION );
   }

   /*  Open Region File  */

   if( (rgnFile = fopen( filename, "r" ))==NULL ) {
      sprintf(currLine,"Could not open Region file %s.",filename);
      ffpmsg( currLine );
      free( currLine );
      free( aRgn );
      return( *status = FILE_NOT_OPENED );
   }
   
   /*  Read in file, line by line  */
   /*  First, set error status in case file is empty */ 
   *status = FILE_NOT_OPENED;

   while( fgets(currLine,allocLen,rgnFile) != NULL ) {

      /* reset status if we got here */
      *status = 0;

      /*  Make sure we have a full line of text  */

      lineLen = strlen(currLine);
      while( lineLen==allocLen-1 && currLine[lineLen-1]!='\n' ) {
         currLoc = (char *)realloc( currLine, 2 * allocLen * sizeof(char) );
         if( !currLoc ) {
            ffpmsg("Couldn't allocate memory to hold Region file contents.");
            *status = MEMORY_ALLOCATION;
            goto error;
         } else {
            currLine = currLoc;
         }
         fgets( currLine+lineLen, allocLen+1, rgnFile );
         allocLen += allocLen;
         lineLen  += strlen(currLine+lineLen);
      }

      currLoc = currLine;
      if( *currLoc == '#' ) {

         /*  Look to see if it is followed by a format statement...  */
         /*  if not skip line                                        */

         currLoc++;
         while( isspace(*currLoc) ) currLoc++;
         if( !fits_strncasecmp( currLoc, "format:", 7 ) ) {
            if( aRgn->nShapes ) {
               ffpmsg("Format code encountered after reading 1 or more shapes.");
               *status = PARSE_SYNTAX_ERR;
               goto error;
            }
            currLoc += 7;
            while( isspace(*currLoc) ) currLoc++;
            if( !fits_strncasecmp( currLoc, "pixel", 5 ) ) {
               cFmt = pixel_fmt;
            } else if( !fits_strncasecmp( currLoc, "degree", 6 ) ) {
               cFmt = degree_fmt;
            } else if( !fits_strncasecmp( currLoc, "hhmmss", 6 ) ) {
               cFmt = hhmmss_fmt;
            } else if( !fits_strncasecmp( currLoc, "hms", 3 ) ) {
               cFmt = hhmmss_fmt;
            } else {
               ffpmsg("Unknown format code encountered in region file.");
               *status = PARSE_SYNTAX_ERR;
               goto error;
            }
         }

      } else if( !fits_strncasecmp( currLoc, "glob", 4 ) ) {
		  /* skip lines that begin with the word 'global' */

      } else {

         while( *currLoc != '\0' ) {

            namePtr  = currLoc;
            paramPtr = NULL;
            nParams  = 1;

            /*  Search for closing parenthesis  */

            done = 0;
            while( !done && !*status && *currLoc ) {
               switch (*currLoc) {
               case '(':
                  *currLoc = '\0';
                  currLoc++;
                  if( paramPtr )   /* Can't have two '(' in a region! */
                     *status = 1;
                  else
                     paramPtr = currLoc;
                  break;
               case ')':
                  *currLoc = '\0';
                  currLoc++;
                  if( !paramPtr )  /* Can't have a ')' without a '(' first */
                     *status = 1;
                  else
                     done = 1;
                  break;
               case '#':
               case '\n':
                  *currLoc = '\0';
                  if( !paramPtr )  /* Allow for a blank line */
                     done = 1;
                  break;
               case ':':  
                  currLoc++;
                  if ( paramPtr ) cFmt = hhmmss_fmt; /* set format if parameter has : */
                  break;
               case 'd':
                  currLoc++;
                  if ( paramPtr ) cFmt = degree_fmt; /* set format if parameter has d */  
                  break;
               case ',':
                  nParams++;  /* Fall through to default */
               default:
                  currLoc++;
                  break;
               }
            }
            if( *status || !done ) {
               ffpmsg( "Error reading Region file" );
               *status = PARSE_SYNTAX_ERR;
               goto error;
            }

            /*  Skip white space in region name  */

            while( isspace(*namePtr) ) namePtr++;

            /*  Was this a blank line? Or the end of the current one  */

            if( ! *namePtr && ! paramPtr ) continue;

            /*  Check for format code at beginning of the line */

            if( !fits_strncasecmp( namePtr, "image;", 6 ) ) {
				namePtr += 6;
				cFmt = pixel_fmt;
            } else if( !fits_strncasecmp( namePtr, "physical;", 9 ) ) {
                                namePtr += 9;
                                cFmt = pixel_fmt;
            } else if( !fits_strncasecmp( namePtr, "linear;", 7 ) ) {
                                namePtr += 7;
                                cFmt = pixel_fmt;
            } else if( !fits_strncasecmp( namePtr, "fk4;", 4 ) ) {
				namePtr += 4;
				cFmt = degree_fmt;
            } else if( !fits_strncasecmp( namePtr, "fk5;", 4 ) ) {
				namePtr += 4;
				cFmt = degree_fmt;
            } else if( !fits_strncasecmp( namePtr, "icrs;", 5 ) ) {
				namePtr += 5;
				cFmt = degree_fmt;

            /* the following 5 cases support region files created by POW 
	       (or ds9 Version 4.x) which
               may have lines containing  only a format code, not followed
               by a ';' (and with no region specifier on the line).  We use
               the 'continue' statement to jump to the end of the loop and
               then continue reading the next line of the region file. */

            } else if( !fits_strncasecmp( namePtr, "fk5", 3 ) ) {
				cFmt = degree_fmt;
                                continue;  /* supports POW region file format */
            } else if( !fits_strncasecmp( namePtr, "fk4", 3 ) ) {
				cFmt = degree_fmt;
                                continue;  /* supports POW region file format */
            } else if( !fits_strncasecmp( namePtr, "icrs", 4 ) ) {
				cFmt = degree_fmt;
                                continue;  /* supports POW region file format */
            } else if( !fits_strncasecmp( namePtr, "image", 5 ) ) {
				cFmt = pixel_fmt;
                                continue;  /* supports POW region file format */
            } else if( !fits_strncasecmp( namePtr, "physical", 8 ) ) {
				cFmt = pixel_fmt;
                                continue;  /* supports POW region file format */


            } else if( !fits_strncasecmp( namePtr, "galactic;", 9 ) ) {
               ffpmsg( "Galactic region coordinates not supported" );
               ffpmsg( namePtr );
               *status = PARSE_SYNTAX_ERR;
               goto error;
            } else if( !fits_strncasecmp( namePtr, "ecliptic;", 9 ) ) {
               ffpmsg( "ecliptic region coordinates not supported" );
               ffpmsg( namePtr );
               *status = PARSE_SYNTAX_ERR;
               goto error;
            }

            /**************************************************/
            /*  We've apparently found a region... Set it up  */
            /**************************************************/

            if( !(aRgn->nShapes % 10) ) {
               if( aRgn->Shapes )
                  tmpShape = (RgnShape *)realloc( aRgn->Shapes,
                                                  (10+aRgn->nShapes)
                                                  * sizeof(RgnShape) );
               else
                  tmpShape = (RgnShape *) malloc( 10 * sizeof(RgnShape) );
               if( tmpShape ) {
                  aRgn->Shapes = tmpShape;
               } else {
                  ffpmsg( "Failed to allocate memory for Region data");
                  *status = MEMORY_ALLOCATION;
                  goto error;
               }

            }
            newShape        = &aRgn->Shapes[aRgn->nShapes++];
            newShape->sign  = 1;
            newShape->shape = point_rgn;
	    for (i=0; i<8; i++) newShape->param.gen.p[i] = 0.0;
	    newShape->param.gen.a = 0.0;
	    newShape->param.gen.b = 0.0;
	    newShape->param.gen.sinT = 0.0;
	    newShape->param.gen.cosT = 0.0;

            while( isspace(*namePtr) ) namePtr++;
            
			/*  Check for the shape's sign  */

            if( *namePtr=='+' ) {
               namePtr++;
            } else if( *namePtr=='-' ) {
               namePtr++;
               newShape->sign = 0;
            }

            /* Skip white space in region name */

            while( isspace(*namePtr) ) namePtr++;
            if( *namePtr=='\0' ) {
               ffpmsg( "Error reading Region file" );
               *status = PARSE_SYNTAX_ERR;
               goto error;
            }
            lineLen = strlen( namePtr ) - 1;
            while( isspace(namePtr[lineLen]) ) namePtr[lineLen--] = '\0';

            /*  Now identify the region  */

            if(        !fits_strcasecmp( namePtr, "circle"  ) ) {
               newShape->shape = circle_rgn;
               if( nParams != 3 )
                  *status = PARSE_SYNTAX_ERR;
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "annulus" ) ) {
               newShape->shape = annulus_rgn;
               if( nParams != 4 )
                  *status = PARSE_SYNTAX_ERR;
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "ellipse" ) ) {
               if( nParams < 4 || nParams > 8 ) {
                  *status = PARSE_SYNTAX_ERR;
	       } else if ( nParams < 6 ) {
		 newShape->shape = ellipse_rgn;
		 newShape->param.gen.p[4] = 0.0;
	       } else {
		 newShape->shape = elliptannulus_rgn;
		 newShape->param.gen.p[6] = 0.0;
		 newShape->param.gen.p[7] = 0.0;
	       }
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "elliptannulus" ) ) {
               newShape->shape = elliptannulus_rgn;
               if( !( nParams==8 || nParams==6 ) )
                  *status = PARSE_SYNTAX_ERR;
               newShape->param.gen.p[6] = 0.0;
               newShape->param.gen.p[7] = 0.0;
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "box"    ) 
                    || !fits_strcasecmp( namePtr, "rotbox" ) ) {
	       if( nParams < 4 || nParams > 8 ) {
		 *status = PARSE_SYNTAX_ERR;
	       } else if ( nParams < 6 ) {
		 newShape->shape = box_rgn;
		 newShape->param.gen.p[4] = 0.0;
	       } else {
		  newShape->shape = boxannulus_rgn;
		  newShape->param.gen.p[6] = 0.0;
		  newShape->param.gen.p[7] = 0.0;
	       }
	       nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "rectangle"    )
                    || !fits_strcasecmp( namePtr, "rotrectangle" ) ) {
               newShape->shape = rectangle_rgn;
               if( nParams < 4 || nParams > 5 )
                  *status = PARSE_SYNTAX_ERR;
               newShape->param.gen.p[4] = 0.0;
               nCoords = 4;
            } else if( !fits_strcasecmp( namePtr, "diamond"    )
                    || !fits_strcasecmp( namePtr, "rotdiamond" )
                    || !fits_strcasecmp( namePtr, "rhombus"    )
                    || !fits_strcasecmp( namePtr, "rotrhombus" ) ) {
               newShape->shape = diamond_rgn;
               if( nParams < 4 || nParams > 5 )
                  *status = PARSE_SYNTAX_ERR;
               newShape->param.gen.p[4] = 0.0;
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "sector"  )
                    || !fits_strcasecmp( namePtr, "pie"     ) ) {
               newShape->shape = sector_rgn;
               if( nParams != 4 )
                  *status = PARSE_SYNTAX_ERR;
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "point"   ) ) {
               newShape->shape = point_rgn;
               if( nParams != 2 )
                  *status = PARSE_SYNTAX_ERR;
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "line"    ) ) {
               newShape->shape = line_rgn;
               if( nParams != 4 )
                  *status = PARSE_SYNTAX_ERR;
               nCoords = 4;
            } else if( !fits_strcasecmp( namePtr, "polygon" ) ) {
               newShape->shape = poly_rgn;
               if( nParams < 6 || (nParams&1) )
                  *status = PARSE_SYNTAX_ERR;
               nCoords = nParams;
            } else if( !fits_strcasecmp( namePtr, "panda" ) ) {
               newShape->shape = panda_rgn;
               if( nParams != 8 )
                  *status = PARSE_SYNTAX_ERR;
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "epanda" ) ) {
               newShape->shape = epanda_rgn;
               if( nParams < 10 || nParams > 11 )
                  *status = PARSE_SYNTAX_ERR;
               newShape->param.gen.p[10] = 0.0;
               nCoords = 2;
            } else if( !fits_strcasecmp( namePtr, "bpanda" ) ) {
               newShape->shape = bpanda_rgn;
               if( nParams < 10 || nParams > 11 )
                  *status = PARSE_SYNTAX_ERR;
               newShape->param.gen.p[10] = 0.0;
               nCoords = 2;
            } else {
               ffpmsg( "Unrecognized region found in region file:" );
               ffpmsg( namePtr );
               *status = PARSE_SYNTAX_ERR;
               goto error;
            }
            if( *status ) {
               ffpmsg( "Wrong number of parameters found for region" );
               ffpmsg( namePtr );
               goto error;
            }

            /*  Parse Parameter string... convert to pixels if necessary  */

            if( newShape->shape==poly_rgn ) {
               newShape->param.poly.Pts = (double *)malloc( nParams
                                                            * sizeof(double) );
               if( !newShape->param.poly.Pts ) {
                  ffpmsg(
                      "Could not allocate memory to hold polygon parameters" );
                  *status = MEMORY_ALLOCATION;
                  goto error;
               }
               newShape->param.poly.nPts = nParams;
               coords = newShape->param.poly.Pts;
            } else
               coords = newShape->param.gen.p;

            /*  Parse the initial "WCS?" coordinates  */
            for( i=0; i<nCoords; i+=2 ) {

               pX = paramPtr;
               while( *paramPtr!=',' ) paramPtr++;
               *(paramPtr++) = '\0';

               pY = paramPtr;
               while( *paramPtr!=',' && *paramPtr != '\0' ) paramPtr++;
               *(paramPtr++) = '\0';

               if( strchr(pX, ':' ) ) {
                  /*  Read in special format & convert to decimal degrees  */
                  cFmt = hhmmss_fmt;
                  mm = 0;
                  ss = 0.;
                  hh = strtol(pX, &endp, 10);
                  if (endp && *endp==':') {
                      pX = endp + 1;
                      mm = strtol(pX, &endp, 10);
                      if (endp && *endp==':') {
                          pX = endp + 1;
                          ss = atof( pX );
                      }
                  }
                  X = 15. * (hh + mm/60. + ss/3600.); /* convert to degrees */

                  mm = 0;
                  ss = 0.;
                  negdec = 0;

                  while( isspace(*pY) ) pY++;
                  if (*pY=='-') {
                      negdec = 1;
                      pY++;
                  }
                  dd = strtol(pY, &endp, 10);
                  if (endp && *endp==':') {
                      pY = endp + 1;
                      mm = strtol(pY, &endp, 10);
                      if (endp && *endp==':') {
                          pY = endp + 1;
                          ss = atof( pY );
                      }
                  }
                  if (negdec)
                     Y = -dd - mm/60. - ss/3600.; /* convert to degrees */
                  else
                     Y = dd + mm/60. + ss/3600.;

               } else {
                  X = atof( pX );
                  Y = atof( pY );
               }
               if (i==0) {   /* save 1st coord. in case needed later */
                   xsave = X;
                   ysave = Y;
               }

               if( cFmt!=pixel_fmt ) {
                  /*  Convert to pixels  */
                  if( wcs==NULL || ! wcs->exists ) {
                     ffpmsg("WCS information needed to convert region coordinates.");
                     *status = NO_WCS_KEY;
                     goto error;
                  }
                  
                  if( ffxypx(  X,  Y, wcs->xrefval, wcs->yrefval,
                                      wcs->xrefpix, wcs->yrefpix,
                                      wcs->xinc,    wcs->yinc,
                                      wcs->rot,     wcs->type,
                              &x, &y, status ) ) {
                     ffpmsg("Error converting region to pixel coordinates.");
                     goto error;
                  }
                  X = x; Y = y;
               }
               coords[i]   = X;
               coords[i+1] = Y;

            }

            /*  Read in remaining parameters...  */

            for( ; i<nParams; i++ ) {
               pX = paramPtr;
               while( *paramPtr!=',' && *paramPtr != '\0' ) paramPtr++;
               *(paramPtr++) = '\0';
               coords[i] = strtod( pX, &endp );

	       if (endp && (*endp=='"' || *endp=='\'' || *endp=='d') ) {
		  div = 1.0;
		  if ( *endp=='"' ) div = 3600.0;
		  if ( *endp=='\'' ) div = 60.0;
		  /* parameter given in arcsec so convert to pixels. */
		  /* Increment first Y coordinate by this amount then calc */
		  /* the distance in pixels from the original coordinate. */
		  /* NOTE: This assumes the pixels are square!! */
		  if (ysave < 0.)
		     Y = ysave + coords[i]/div;  /* don't exceed -90 */
		  else
		     Y = ysave - coords[i]/div;  /* don't exceed +90 */

		  X = xsave;
		  if( ffxypx(  X,  Y, wcs->xrefval, wcs->yrefval,
			       wcs->xrefpix, wcs->yrefpix,
			       wcs->xinc,    wcs->yinc,
			       wcs->rot,     wcs->type,
                               &x, &y, status ) ) {
		     ffpmsg("Error converting region to pixel coordinates.");
		     goto error;
		  }
		 
		  coords[i] = sqrt( pow(x-coords[0],2) + pow(y-coords[1],2) );

               }
            }

	    /* special case for elliptannulus and boxannulus if only one angle
	       was given */

	    if ( (newShape->shape == elliptannulus_rgn || 
		  newShape->shape == boxannulus_rgn ) && nParams == 7 ) {
	      coords[7] = coords[6];
	    }

            /* Also, correct the position angle for any WCS rotation:  */
            /*    If regions are specified in WCS coordintes, then the angles */
            /*    are relative to the WCS system, not the pixel X,Y system */

	    if( cFmt!=pixel_fmt ) {	    
	      switch( newShape->shape ) {
	      case sector_rgn:
	      case panda_rgn:
		coords[2] += (wcs->rot);
		coords[3] += (wcs->rot);
		break;
	      case box_rgn:
	      case rectangle_rgn:
	      case diamond_rgn:
	      case ellipse_rgn:
		coords[4] += (wcs->rot);
		break;
	      case boxannulus_rgn:
	      case elliptannulus_rgn:
		coords[6] += (wcs->rot);
		coords[7] += (wcs->rot);
		break;
	      case epanda_rgn:
	      case bpanda_rgn:
		coords[2] += (wcs->rot);
		coords[3] += (wcs->rot);
		coords[10] += (wcs->rot);
              default:
                break;
	      }
	    }

	    /* do some precalculations to speed up tests */

	    fits_setup_shape(newShape);

         }  /* End of while( *currLoc ) */
/*
  if (coords)printf("%.8f %.8f %.8f %.8f %.8f\n",
   coords[0],coords[1],coords[2],coords[3],coords[4]); 
*/
      }  /* End of if...else parse line */
   }   /* End of while( fgets(rgnFile) ) */

   /* set up component numbers */

   fits_set_region_components( aRgn );

error:

   if( *status ) {
      fits_free_region( aRgn );
   } else {
      *Rgn = aRgn;
   }

   fclose( rgnFile );
   free( currLine );

   return( *status );
}

/*---------------------------------------------------------------------------*/
int fits_in_region( double    X,
            double    Y,
            SAORegion *Rgn )
/*  Test if the given point is within the region described by Rgn.  X and    */
/*  Y are in pixel coordinates.                                              */
/*---------------------------------------------------------------------------*/
{
   double x, y, dx, dy, xprime, yprime, r, th;
   RgnShape *Shapes;
   int i, cur_comp;
   int result, comp_result;

   Shapes = Rgn->Shapes;

   result = 0;
   comp_result = 0;
   cur_comp = Rgn->Shapes[0].comp;

   for( i=0; i<Rgn->nShapes; i++, Shapes++ ) {

     /* if this region has a different component number to the last one  */
     /*	then replace the accumulated selection logical with the union of */
     /*	the current logical and the total logical. Reinitialize the      */
     /* temporary logical.                                               */

     if ( i==0 || Shapes->comp != cur_comp ) {
       result = result || comp_result;
       cur_comp = Shapes->comp;
       /* if an excluded region is given first, then implicitly   */
       /* assume a previous shape that includes the entire image. */
       comp_result = !Shapes->sign;
     }

    /* only need to test if  */
    /*   the point is not already included and this is an include region, */
    /* or the point is included and this is an excluded region */

    if ( (!comp_result && Shapes->sign) || (comp_result && !Shapes->sign) ) { 

      comp_result = 1;

      switch( Shapes->shape ) {

      case box_rgn:
         /*  Shift origin to center of region  */
         xprime = X - Shapes->param.gen.p[0];
         yprime = Y - Shapes->param.gen.p[1];

         /*  Rotate point to region's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;

         dx = 0.5 * Shapes->param.gen.p[2];
         dy = 0.5 * Shapes->param.gen.p[3];
         if( (x < -dx) || (x > dx) || (y < -dy) || (y > dy) )
            comp_result = 0;
         break;

      case boxannulus_rgn:
         /*  Shift origin to center of region  */
         xprime = X - Shapes->param.gen.p[0];
         yprime = Y - Shapes->param.gen.p[1];

         /*  Rotate point to region's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;

         dx = 0.5 * Shapes->param.gen.p[4];
         dy = 0.5 * Shapes->param.gen.p[5];
         if( (x < -dx) || (x > dx) || (y < -dy) || (y > dy) ) {
	   comp_result = 0;
	 } else {
	   /* Repeat test for inner box */
	   x =  xprime * Shapes->param.gen.b + yprime * Shapes->param.gen.a;
	   y = -xprime * Shapes->param.gen.a + yprime * Shapes->param.gen.b;
	   
	   dx = 0.5 * Shapes->param.gen.p[2];
	   dy = 0.5 * Shapes->param.gen.p[3];
	   if( (x >= -dx) && (x <= dx) && (y >= -dy) && (y <= dy) )
	     comp_result = 0;
	 }
         break;

      case rectangle_rgn:
         /*  Shift origin to center of region  */
         xprime = X - Shapes->param.gen.p[5];
         yprime = Y - Shapes->param.gen.p[6];

         /*  Rotate point to region's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;

         dx = Shapes->param.gen.a;
         dy = Shapes->param.gen.b;
         if( (x < -dx) || (x > dx) || (y < -dy) || (y > dy) )
            comp_result = 0;
         break;

      case diamond_rgn:
         /*  Shift origin to center of region  */
         xprime = X - Shapes->param.gen.p[0];
         yprime = Y - Shapes->param.gen.p[1];

         /*  Rotate point to region's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;

         dx = 0.5 * Shapes->param.gen.p[2];
         dy = 0.5 * Shapes->param.gen.p[3];
         r  = fabs(x/dx) + fabs(y/dy);
         if( r > 1 )
            comp_result = 0;
         break;

      case circle_rgn:
         /*  Shift origin to center of region  */
         x = X - Shapes->param.gen.p[0];
         y = Y - Shapes->param.gen.p[1];

         r  = x*x + y*y;
         if ( r > Shapes->param.gen.a )
            comp_result = 0;
         break;

      case annulus_rgn:
         /*  Shift origin to center of region  */
         x = X - Shapes->param.gen.p[0];
         y = Y - Shapes->param.gen.p[1];

         r = x*x + y*y;
         if ( r < Shapes->param.gen.a || r > Shapes->param.gen.b )
            comp_result = 0;
         break;

      case sector_rgn:
         /*  Shift origin to center of region  */
         x = X - Shapes->param.gen.p[0];
         y = Y - Shapes->param.gen.p[1];

         if( x || y ) {
            r = atan2( y, x ) * RadToDeg;
            if( Shapes->param.gen.p[2] <= Shapes->param.gen.p[3] ) {
               if( r < Shapes->param.gen.p[2] || r > Shapes->param.gen.p[3] )
                  comp_result = 0;
            } else {
               if( r < Shapes->param.gen.p[2] && r > Shapes->param.gen.p[3] )
                  comp_result = 0;
            }
         }
         break;

      case ellipse_rgn:
         /*  Shift origin to center of region  */
         xprime = X - Shapes->param.gen.p[0];
         yprime = Y - Shapes->param.gen.p[1];

         /*  Rotate point to region's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;

         x /= Shapes->param.gen.p[2];
         y /= Shapes->param.gen.p[3];
         r = x*x + y*y;
         if( r>1.0 )
            comp_result = 0;
         break;

      case elliptannulus_rgn:
         /*  Shift origin to center of region  */
         xprime = X - Shapes->param.gen.p[0];
         yprime = Y - Shapes->param.gen.p[1];

         /*  Rotate point to outer ellipse's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;

         x /= Shapes->param.gen.p[4];
         y /= Shapes->param.gen.p[5];
         r = x*x + y*y;
         if( r>1.0 )
            comp_result = 0;
         else {
            /*  Repeat test for inner ellipse  */
            x =  xprime * Shapes->param.gen.b + yprime * Shapes->param.gen.a;
            y = -xprime * Shapes->param.gen.a + yprime * Shapes->param.gen.b;

            x /= Shapes->param.gen.p[2];
            y /= Shapes->param.gen.p[3];
            r = x*x + y*y;
            if( r<1.0 )
               comp_result = 0;
         }
         break;

      case line_rgn:
         /*  Shift origin to first point of line  */
         xprime = X - Shapes->param.gen.p[0];
         yprime = Y - Shapes->param.gen.p[1];

         /*  Rotate point to line's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;

         if( (y < -0.5) || (y >= 0.5) || (x < -0.5)
             || (x >= Shapes->param.gen.a) )
            comp_result = 0;
         break;

      case point_rgn:
         /*  Shift origin to center of region  */
         x = X - Shapes->param.gen.p[0];
         y = Y - Shapes->param.gen.p[1];

         if ( (x<-0.5) || (x>=0.5) || (y<-0.5) || (y>=0.5) )
            comp_result = 0;
         break;

      case poly_rgn:
         if( X<Shapes->xmin || X>Shapes->xmax
             || Y<Shapes->ymin || Y>Shapes->ymax )
            comp_result = 0;
         else
            comp_result = Pt_in_Poly( X, Y, Shapes->param.poly.nPts,
                                       Shapes->param.poly.Pts );
         break;

      case panda_rgn:
         /*  Shift origin to center of region  */
         x = X - Shapes->param.gen.p[0];
         y = Y - Shapes->param.gen.p[1];

         r = x*x + y*y;
         if ( r < Shapes->param.gen.a || r > Shapes->param.gen.b ) {
	   comp_result = 0;
	 } else {
	   if( x || y ) {
	     th = atan2( y, x ) * RadToDeg;
	     if( Shapes->param.gen.p[2] <= Shapes->param.gen.p[3] ) {
               if( th < Shapes->param.gen.p[2] || th > Shapes->param.gen.p[3] )
		 comp_result = 0;
	     } else {
               if( th < Shapes->param.gen.p[2] && th > Shapes->param.gen.p[3] )
		 comp_result = 0;
	     }
	   }
         }
         break;

      case epanda_rgn:
         /*  Shift origin to center of region  */
         xprime = X - Shapes->param.gen.p[0];
         yprime = Y - Shapes->param.gen.p[1];

         /*  Rotate point to region's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;
	 xprime = x;
	 yprime = y;

	 /* outer region test */
         x = xprime/Shapes->param.gen.p[7];
         y = yprime/Shapes->param.gen.p[8];
         r = x*x + y*y;
	 if ( r>1.0 )
	   comp_result = 0;
	 else {
	   /* inner region test */
	   x = xprime/Shapes->param.gen.p[5];
	   y = yprime/Shapes->param.gen.p[6];
	   r = x*x + y*y;
	   if ( r<1.0 )
	     comp_result = 0;
	   else {
	     /* angle test */
	     if( xprime || yprime ) {
	       th = atan2( yprime, xprime ) * RadToDeg;
	       if( Shapes->param.gen.p[2] <= Shapes->param.gen.p[3] ) {
		 if( th < Shapes->param.gen.p[2] || th > Shapes->param.gen.p[3] )
		   comp_result = 0;
	       } else {
		 if( th < Shapes->param.gen.p[2] && th > Shapes->param.gen.p[3] )
		   comp_result = 0;
	       }
	     }
	   }
	 }
         break;

      case bpanda_rgn:
         /*  Shift origin to center of region  */
         xprime = X - Shapes->param.gen.p[0];
         yprime = Y - Shapes->param.gen.p[1];

         /*  Rotate point to region's orientation  */
         x =  xprime * Shapes->param.gen.cosT + yprime * Shapes->param.gen.sinT;
         y = -xprime * Shapes->param.gen.sinT + yprime * Shapes->param.gen.cosT;

	 /* outer box test */
         dx = 0.5 * Shapes->param.gen.p[7];
         dy = 0.5 * Shapes->param.gen.p[8];
         if( (x < -dx) || (x > dx) || (y < -dy) || (y > dy) )
	   comp_result = 0;
	 else {
	   /* inner box test */
	   dx = 0.5 * Shapes->param.gen.p[5];
	   dy = 0.5 * Shapes->param.gen.p[6];
	   if( (x >= -dx) && (x <= dx) && (y >= -dy) && (y <= dy) )
	     comp_result = 0;
	   else {
	     /* angle test */
	     if( x || y ) {
	       th = atan2( y, x ) * RadToDeg;
	       if( Shapes->param.gen.p[2] <= Shapes->param.gen.p[3] ) {
		 if( th < Shapes->param.gen.p[2] || th > Shapes->param.gen.p[3] )
		   comp_result = 0;
	       } else {
		 if( th < Shapes->param.gen.p[2] && th > Shapes->param.gen.p[3] )
		   comp_result = 0;
	       }
	     }
	   }
	 }
         break;
      }

      if( !Shapes->sign ) comp_result = !comp_result;

     } 

   }

   result = result || comp_result;
   
   return( result );
}

/*---------------------------------------------------------------------------*/
void fits_free_region( SAORegion *Rgn )
/*   Free up memory allocated to hold the region data.                       */
/*---------------------------------------------------------------------------*/
{
   int i;

   for( i=0; i<Rgn->nShapes; i++ )
      if( Rgn->Shapes[i].shape == poly_rgn )
         free( Rgn->Shapes[i].param.poly.Pts );
   if( Rgn->Shapes )
      free( Rgn->Shapes );
   free( Rgn );
}

/*---------------------------------------------------------------------------*/
static int Pt_in_Poly( double x,
                       double y,
                       int nPts,
                       double *Pts )
/*  Internal routine for testing whether the coordinate x,y is within the    */
/*  polygon region traced out by the array Pts.                              */
/*---------------------------------------------------------------------------*/
{
   int i, j, flag=0;
   double prevX, prevY;
   double nextX, nextY;
   double dx, dy, Dy;

   nextX = Pts[nPts-2];
   nextY = Pts[nPts-1];

   for( i=0; i<nPts; i+=2 ) {
      prevX = nextX;
      prevY = nextY;

      nextX = Pts[i];
      nextY = Pts[i+1];

      if( (y>prevY && y>=nextY) || (y<prevY && y<=nextY)
          || (x>prevX && x>=nextX) )
         continue;
      
      /* Check to see if x,y lies right on the segment */

      if( x>=prevX || x>nextX ) {
         dy = y - prevY;
         Dy = nextY - prevY;

         if( fabs(Dy)<1e-10 ) {
            if( fabs(dy)<1e-10 )
               return( 1 );
            else
               continue;
         }

         dx = prevX + ( (nextX-prevX)/(Dy) ) * dy - x;
         if( dx < -1e-10 )
            continue;
         if( dx <  1e-10 )
            return( 1 );
      }

      /* There is an intersection! Make sure it isn't a V point.  */

      if( y != prevY ) {
         flag = 1 - flag;
      } else {
         j = i+1;  /* Point to Y component */
         do {
            if( j>1 )
               j -= 2;
            else
               j = nPts-1;
         } while( y == Pts[j] );

         if( (nextY-y)*(y-Pts[j]) > 0 )
            flag = 1-flag;
      }

   }
   return( flag );
}
/*---------------------------------------------------------------------------*/
void fits_set_region_components ( SAORegion *aRgn )
{
/* 
   Internal routine to turn a collection of regions read from an ascii file into
   the more complex structure that is allowed by the FITS REGION extension with
   multiple components. Regions are anded within components and ored between them
   ie for a pixel to be selected it must be selected by at least one component
   and to be selected by a component it must be selected by all that component's
   shapes.

   The algorithm is to replicate every exclude region after every include
   region before it in the list. eg reg1, reg2, -reg3, reg4, -reg5 becomes
   (reg1, -reg3, -reg5), (reg2, -reg5, -reg3), (reg4, -reg5) where the
   parentheses designate components.
*/

  int i, j, k, icomp;

/* loop round shapes */

  i = 0;
  while ( i<aRgn->nShapes ) {

    /* first do the case of an exclude region */

    if ( !aRgn->Shapes[i].sign ) {

      /* we need to run back through the list copying the current shape as
	 required. start by findin the first include shape before this exclude */

      j = i-1;
      while ( j > 0 && !aRgn->Shapes[j].sign ) j--;

      /* then go back one more shape */

      j--;

      /* and loop back through the regions */

      while ( j >= 0 ) {

	/* if this is an include region then insert a copy of the exclude
	   region immediately after it */

	if ( aRgn->Shapes[j].sign ) {

	  aRgn->Shapes = (RgnShape *) realloc (aRgn->Shapes,(1+aRgn->nShapes)*sizeof(RgnShape));
	  aRgn->nShapes++;
	  for (k=aRgn->nShapes-1; k>j+1; k--) aRgn->Shapes[k] = aRgn->Shapes[k-1];

	  i++;
	  aRgn->Shapes[j+1] = aRgn->Shapes[i];

	}

	j--;

      }

    }

    i++;

  }

  /* now set the component numbers */

  icomp = 0;
  for ( i=0; i<aRgn->nShapes; i++ ) {
    if ( aRgn->Shapes[i].sign ) icomp++;
    aRgn->Shapes[i].comp = icomp;

    /*
    printf("i = %d, shape = %d, sign = %d, comp = %d\n", i, aRgn->Shapes[i].shape, aRgn->Shapes[i].sign, aRgn->Shapes[i].comp);
    */

  }

  return;

}

/*---------------------------------------------------------------------------*/
void fits_setup_shape ( RgnShape *newShape)
{
/* Perform some useful calculations now to speed up filter later             */

  double X, Y, R;
  double *coords;
  int i;

  if ( newShape->shape == poly_rgn ) {
    coords = newShape->param.poly.Pts;
  } else {
    coords = newShape->param.gen.p;
  }

  switch( newShape->shape ) {
  case circle_rgn:
    newShape->param.gen.a = coords[2] * coords[2];
    break;
  case annulus_rgn:
    newShape->param.gen.a = coords[2] * coords[2];
    newShape->param.gen.b = coords[3] * coords[3];
    break;
  case sector_rgn:
    while( coords[2]> 180.0 ) coords[2] -= 360.0;
    while( coords[2]<=-180.0 ) coords[2] += 360.0;
    while( coords[3]> 180.0 ) coords[3] -= 360.0;
    while( coords[3]<=-180.0 ) coords[3] += 360.0;
    break;
  case ellipse_rgn:
    newShape->param.gen.sinT = sin( myPI * (coords[4] / 180.0) );
    newShape->param.gen.cosT = cos( myPI * (coords[4] / 180.0) );
    break;
  case elliptannulus_rgn:
    newShape->param.gen.a    = sin( myPI * (coords[6] / 180.0) );
    newShape->param.gen.b    = cos( myPI * (coords[6] / 180.0) );
    newShape->param.gen.sinT = sin( myPI * (coords[7] / 180.0) );
    newShape->param.gen.cosT = cos( myPI * (coords[7] / 180.0) );
    break;
  case box_rgn:
    newShape->param.gen.sinT = sin( myPI * (coords[4] / 180.0) );
    newShape->param.gen.cosT = cos( myPI * (coords[4] / 180.0) );
    break;
  case boxannulus_rgn:
    newShape->param.gen.a    = sin( myPI * (coords[6] / 180.0) );
    newShape->param.gen.b    = cos( myPI * (coords[6] / 180.0) );
    newShape->param.gen.sinT = sin( myPI * (coords[7] / 180.0) );
    newShape->param.gen.cosT = cos( myPI * (coords[7] / 180.0) );
    break;
  case rectangle_rgn:
    newShape->param.gen.sinT = sin( myPI * (coords[4] / 180.0) );
    newShape->param.gen.cosT = cos( myPI * (coords[4] / 180.0) );
    X = 0.5 * ( coords[2]-coords[0] );
    Y = 0.5 * ( coords[3]-coords[1] );
    newShape->param.gen.a = fabs( X * newShape->param.gen.cosT
				  + Y * newShape->param.gen.sinT );
    newShape->param.gen.b = fabs( Y * newShape->param.gen.cosT
				  - X * newShape->param.gen.sinT );
    newShape->param.gen.p[5] = 0.5 * ( coords[2]+coords[0] );
    newShape->param.gen.p[6] = 0.5 * ( coords[3]+coords[1] );
    break;
  case diamond_rgn:
    newShape->param.gen.sinT = sin( myPI * (coords[4] / 180.0) );
    newShape->param.gen.cosT = cos( myPI * (coords[4] / 180.0) );
    break;
  case line_rgn:
    X = coords[2] - coords[0];
    Y = coords[3] - coords[1];
    R = sqrt( X*X + Y*Y );
    newShape->param.gen.sinT = ( R ? Y/R : 0.0 );
    newShape->param.gen.cosT = ( R ? X/R : 1.0 );
    newShape->param.gen.a    = R + 0.5;
    break;
  case panda_rgn:
    while( coords[2]> 180.0 ) coords[2] -= 360.0;
    while( coords[2]<=-180.0 ) coords[2] += 360.0;
    while( coords[3]> 180.0 ) coords[3] -= 360.0;
    while( coords[3]<=-180.0 ) coords[3] += 360.0;
    newShape->param.gen.a = newShape->param.gen.p[5]*newShape->param.gen.p[5];
    newShape->param.gen.b = newShape->param.gen.p[6]*newShape->param.gen.p[6];
    break;
  case epanda_rgn:
  case bpanda_rgn:
    while( coords[2]> 180.0 ) coords[2] -= 360.0;
    while( coords[2]<=-180.0 ) coords[2] += 360.0;
    while( coords[3]> 180.0 ) coords[3] -= 360.0;
    while( coords[3]<=-180.0 ) coords[3] += 360.0;
    newShape->param.gen.sinT = sin( myPI * (coords[10] / 180.0) );
    newShape->param.gen.cosT = cos( myPI * (coords[10] / 180.0) );
    break;
  default:
    break;
  }

  /*  Set the xmin, xmax, ymin, ymax elements of the RgnShape structure */

  /* For everything which has first two parameters as center position just */
  /* find a circle that encompasses the region and use it to set the       */
  /* bounding box                                                          */

  R = -1.0;

  switch ( newShape->shape ) {

  case circle_rgn:
    R = coords[2];
    break;

  case annulus_rgn:
    R = coords[3];
    break;

  case ellipse_rgn:
    if ( coords[2] > coords[3] ) {
      R = coords[2];
    } else {
      R = coords[3];
    }
    break;

  case elliptannulus_rgn:
    if ( coords[4] > coords[5] ) {
      R = coords[4];
    } else {
      R = coords[5];
    }
    break;

  case box_rgn:
    R = sqrt(coords[2]*coords[2]+
	     coords[3]*coords[3])/2.0;
    break;

  case boxannulus_rgn:
    R = sqrt(coords[4]*coords[5]+
	     coords[4]*coords[5])/2.0;
    break;

  case diamond_rgn:
    if ( coords[2] > coords[3] ) {
      R = coords[2]/2.0;
    } else {
      R = coords[3]/2.0;
    }
    break;
    
  case point_rgn:
    R = 1.0;
    break;

  case panda_rgn:
    R = coords[6];
    break;

  case epanda_rgn:
    if ( coords[7] > coords[8] ) {
      R = coords[7];
    } else {
      R = coords[8];
    }
    break;

  case bpanda_rgn:
    R = sqrt(coords[7]*coords[8]+
	     coords[7]*coords[8])/2.0;
    break;

  default:
    break;
  }

  if ( R > 0.0 ) {

    newShape->xmin = coords[0] - R;
    newShape->xmax = coords[0] + R;
    newShape->ymin = coords[1] - R;
    newShape->ymax = coords[1] + R;

    return;

  }

  /* Now do the rest of the shapes that require individual methods */

  switch ( newShape->shape ) {

  case rectangle_rgn:
    R = sqrt((coords[5]-coords[0])*(coords[5]-coords[0])+
	     (coords[6]-coords[1])*(coords[6]-coords[1]));
    newShape->xmin = coords[5] - R;
    newShape->xmax = coords[5] + R;
    newShape->ymin = coords[6] - R;
    newShape->ymax = coords[6] + R;
    break;

  case poly_rgn:
    newShape->xmin = coords[0];
    newShape->xmax = coords[0];
    newShape->ymin = coords[1];
    newShape->ymax = coords[1];
    for( i=2; i < newShape->param.poly.nPts; ) {
      if( newShape->xmin > coords[i] ) /* Min X */
	newShape->xmin = coords[i];
      if( newShape->xmax < coords[i] ) /* Max X */
	newShape->xmax = coords[i];
      i++;
      if( newShape->ymin > coords[i] ) /* Min Y */
	newShape->ymin = coords[i];
      if( newShape->ymax < coords[i] ) /* Max Y */
	newShape->ymax = coords[i];
      i++;
    }
    break;

  case line_rgn:
    if ( coords[0] > coords[2] ) {
      newShape->xmin = coords[2];
      newShape->xmax = coords[0];
    } else {
      newShape->xmin = coords[0];
      newShape->xmax = coords[2];
    }
    if ( coords[1] > coords[3] ) {
      newShape->ymin = coords[3];
      newShape->ymax = coords[1];
    } else {
      newShape->ymin = coords[1];
      newShape->ymax = coords[3];
    }

    break;

    /* sector doesn't have min and max so indicate by setting max < min */

  case sector_rgn:
    newShape->xmin = 1.0;
    newShape->xmax = -1.0;
    newShape->ymin = 1.0;
    newShape->ymax = -1.0;
    break;

  default:
    break;
  }

  return;

}

/*---------------------------------------------------------------------------*/
int fits_read_fits_region ( fitsfile *fptr, 
			    WCSdata *wcs, 
			    SAORegion **Rgn, 
			    int *status)
/*  Read regions from a FITS region extension and return the information     */
/*  in the "SAORegion" structure.  If it is nonNULL, use wcs to convert the  */
/*  region coordinates to pixels.  Return an error if region is in degrees   */
/*  but no WCS data is provided.                                             */
/*---------------------------------------------------------------------------*/
{

  int i, j, icol[6], idum, anynul, npos;
  int dotransform, got_component = 1, tstatus;
  long icsize[6];
  double X, Y, Theta, Xsave = 0, Ysave = 0, Xpos, Ypos;
  double *coords;
  char *cvalue, *cvalue2;
  char comment[FLEN_COMMENT];
  char colname[6][FLEN_VALUE] = {"X", "Y", "SHAPE", "R", "ROTANG", "COMPONENT"};
  char shapename[17][FLEN_VALUE] = {"POINT","CIRCLE","ELLIPSE","ANNULUS",
				    "ELLIPTANNULUS","BOX","ROTBOX","BOXANNULUS",
				    "RECTANGLE","ROTRECTANGLE","POLYGON","PIE",
				    "SECTOR","DIAMOND","RHOMBUS","ROTDIAMOND",
				    "ROTRHOMBUS"};
  int shapetype[17] = {point_rgn, circle_rgn, ellipse_rgn, annulus_rgn, 
		       elliptannulus_rgn, box_rgn, box_rgn, boxannulus_rgn, 
		       rectangle_rgn, rectangle_rgn, poly_rgn, sector_rgn, 
		       sector_rgn, diamond_rgn, diamond_rgn, diamond_rgn, 
		       diamond_rgn};
  SAORegion *aRgn;
  RgnShape *newShape;
  WCSdata *regwcs = 0;

  if ( *status ) return( *status );

  aRgn = (SAORegion *)malloc( sizeof(SAORegion) );
  if( ! aRgn ) {
    ffpmsg("Couldn't allocate memory to hold Region file contents.");
    return(*status = MEMORY_ALLOCATION );
  }
  aRgn->nShapes    =    0;
  aRgn->Shapes     = NULL;
  if( wcs && wcs->exists )
    aRgn->wcs = *wcs;
  else
    aRgn->wcs.exists = 0;

  /* See if we are already positioned to a region extension, else */
  /* move to the REGION extension (file is already open). */

  tstatus = 0;
  for (i=0; i<5; i++) {
    ffgcno(fptr, CASEINSEN, colname[i], &icol[i], &tstatus);
  }

  if (tstatus) {
    /* couldn't find the required columns, so search for "REGION" extension */
    if ( ffmnhd(fptr, BINARY_TBL, "REGION", 1, status) ) {
      ffpmsg("Could not move to REGION extension.");
      goto error;
    }
  }

  /* get the number of shapes and allocate memory */

  if ( ffgky(fptr, TINT, "NAXIS2", &aRgn->nShapes, comment, status) ) {
    ffpmsg("Could not read NAXIS2 keyword.");
    goto error;
  }

  aRgn->Shapes = (RgnShape *) malloc(aRgn->nShapes * sizeof(RgnShape));
  if ( !aRgn->Shapes ) {
    ffpmsg( "Failed to allocate memory for Region data");
    *status = MEMORY_ALLOCATION;
    goto error;
  }

  /* get the required column numbers */

  for (i=0; i<5; i++) {
    if ( ffgcno(fptr, CASEINSEN, colname[i], &icol[i], status) ) {
      ffpmsg("Could not find column.");
      goto error;
    }
  }

  /* try to get the optional column numbers */

  if ( ffgcno(fptr, CASEINSEN, colname[5], &icol[5], status) ) {
       got_component = 0;
  }

  /* if there was input WCS then read the WCS info for the region in case they */
  /* are different and we have to transform */

  dotransform = 0;
  if ( aRgn->wcs.exists ) {
    regwcs = (WCSdata *) malloc ( sizeof(WCSdata) );
    if ( !regwcs ) {
      ffpmsg( "Failed to allocate memory for Region WCS data");
      *status = MEMORY_ALLOCATION;
      goto error;
    }

    regwcs->exists = 1;
    if ( ffgtcs(fptr, icol[0], icol[1], &regwcs->xrefval,  &regwcs->yrefval,
		&regwcs->xrefpix, &regwcs->yrefpix, &regwcs->xinc, &regwcs->yinc,
		&regwcs->rot, regwcs->type, status) ) {
      regwcs->exists = 0;
      *status = 0;
    }

    if ( regwcs->exists && wcs->exists ) {
      if ( fabs(regwcs->xrefval-wcs->xrefval) > 1.0e-6 ||
	   fabs(regwcs->yrefval-wcs->yrefval) > 1.0e-6 ||
	   fabs(regwcs->xrefpix-wcs->xrefpix) > 1.0e-6 ||
	   fabs(regwcs->yrefpix-wcs->yrefpix) > 1.0e-6 ||
	   fabs(regwcs->xinc-wcs->xinc) > 1.0e-6 ||
	   fabs(regwcs->yinc-wcs->yinc) > 1.0e-6 ||
	   fabs(regwcs->rot-wcs->rot) > 1.0e-6 ||
	   !strcmp(regwcs->type,wcs->type) ) dotransform = 1;
    }
  }

  /* get the sizes of the X, Y, R, and ROTANG vectors */

  for (i=0; i<6; i++) {
    if ( ffgtdm(fptr, icol[i], 1, &idum, &icsize[i], status) ) {
      ffpmsg("Could not find vector size of column.");
      goto error;
    }
  }

  cvalue = (char *) malloc ((FLEN_VALUE+1)*sizeof(char));

  /* loop over the shapes - note 1-based counting for rows in FITS files */

  for (i=1; i<=aRgn->nShapes; i++) {

    newShape = &aRgn->Shapes[i-1];
    for (j=0; j<8; j++) newShape->param.gen.p[j] = 0.0;
    newShape->param.gen.a = 0.0;
    newShape->param.gen.b = 0.0;
    newShape->param.gen.sinT = 0.0;
    newShape->param.gen.cosT = 0.0;

    /* get the shape */

    if ( ffgcvs(fptr, icol[2], i, 1, 1, " ", &cvalue, &anynul, status) ) {
      ffpmsg("Could not read shape.");
      goto error;
    }

    /* set include or exclude */

    newShape->sign = 1;
    cvalue2 = cvalue;
    if ( !strncmp(cvalue,"!",1) ) {
      newShape->sign = 0;
      cvalue2++;
    }

    /* set the shape type */

    for (j=0; j<17; j++) {
      if ( !strcmp(cvalue2, shapename[j]) ) newShape->shape = shapetype[j];
    }

    /* allocate memory for polygon case and set coords pointer */

    if ( newShape->shape == poly_rgn ) {
      newShape->param.poly.Pts = (double *) calloc (2*icsize[0], sizeof(double));
      if ( !newShape->param.poly.Pts ) {
	ffpmsg("Could not allocate memory to hold polygon parameters" );
	*status = MEMORY_ALLOCATION;
	goto error;
      }
      newShape->param.poly.nPts = 2*icsize[0];
      coords = newShape->param.poly.Pts;
    } else {
      coords = newShape->param.gen.p;
    }


  /* read X and Y. Polygon and Rectangle require special cases */

    npos = 1;
    if ( newShape->shape == poly_rgn ) npos = newShape->param.poly.nPts/2;
    if ( newShape->shape == rectangle_rgn ) npos = 2;

    for (j=0; j<npos; j++) {
      if ( ffgcvd(fptr, icol[0], i, j+1, 1, DOUBLENULLVALUE, coords, &anynul, status) ) {
	ffpmsg("Failed to read X column for polygon region");
	goto error;
      }
      if (*coords == DOUBLENULLVALUE) {  /* check for null value end of array marker */
        npos = j;
	newShape->param.poly.nPts = npos * 2;
	break;
      }
      coords++;
      
      if ( ffgcvd(fptr, icol[1], i, j+1, 1, DOUBLENULLVALUE, coords, &anynul, status) ) {
	ffpmsg("Failed to read Y column for polygon region");
	goto error;
      }
      if (*coords == DOUBLENULLVALUE) { /* check for null value end of array marker */
        npos = j;
	newShape->param.poly.nPts = npos * 2;
        coords--;
	break;
      }
      coords++;
 
      if (j == 0) {  /* save the first X and Y coordinate */
        Xsave = *(coords - 2);
	Ysave = *(coords - 1);
      } else if ((Xsave == *(coords - 2)) && (Ysave == *(coords - 1)) ) {
        /* if point has same coordinate as first point, this marks the end of the array */
        npos = j + 1;
	newShape->param.poly.nPts = npos * 2;
	break;
      }
    }

    /* transform positions if the region and input wcs differ */

    if ( dotransform ) {

      coords -= npos*2;
      Xsave = coords[0];
      Ysave = coords[1];
      for (j=0; j<npos; j++) {
	ffwldp(coords[2*j], coords[2*j+1], regwcs->xrefval, regwcs->yrefval, regwcs->xrefpix,
	       regwcs->yrefpix, regwcs->xinc, regwcs->yinc, regwcs->rot,
	       regwcs->type, &Xpos, &Ypos, status);
	ffxypx(Xpos, Ypos, wcs->xrefval, wcs->yrefval, wcs->xrefpix,
	       wcs->yrefpix, wcs->xinc, wcs->yinc, wcs->rot,
	       wcs->type, &coords[2*j], &coords[2*j+1], status);
	if ( *status ) {
	  ffpmsg("Failed to transform coordinates");
	  goto error;
	}
      }
      coords += npos*2;
    }

  /* read R. Circle requires one number; Box, Diamond, Ellipse, Annulus, Sector 
     and Panda two; Boxannulus and Elliptannulus four; Point, Rectangle and 
     Polygon none. */

    npos = 0;
    switch ( newShape->shape ) {
    case circle_rgn: 
      npos = 1;
      break;
    case box_rgn:
    case diamond_rgn:
    case ellipse_rgn:
    case annulus_rgn:
    case sector_rgn:
      npos = 2;
      break;
    case boxannulus_rgn:
    case elliptannulus_rgn:
      npos = 4;
      break;
    default:
      break;
    }

    if ( npos > 0 ) {
      if ( ffgcvd(fptr, icol[3], i, 1, npos, 0.0, coords, &anynul, status) ) {
	ffpmsg("Failed to read R column for region");
	goto error;
      }

    /* transform lengths if the region and input wcs differ */

      if ( dotransform ) {
	for (j=0; j<npos; j++) {
	  Y = Ysave + (*coords);
	  X = Xsave;
	  ffwldp(X, Y, regwcs->xrefval, regwcs->yrefval, regwcs->xrefpix,
		 regwcs->yrefpix, regwcs->xinc, regwcs->yinc, regwcs->rot,
		 regwcs->type, &Xpos, &Ypos, status);
	  ffxypx(Xpos, Ypos, wcs->xrefval, wcs->yrefval, wcs->xrefpix,
		 wcs->yrefpix, wcs->xinc, wcs->yinc, wcs->rot,
		 wcs->type, &X, &Y, status);
	  if ( *status ) {
	    ffpmsg("Failed to transform coordinates");
	    goto error;
	  }
	  *(coords++) = sqrt(pow(X-newShape->param.gen.p[0],2)+pow(Y-newShape->param.gen.p[1],2));
	}
      } else {
	coords += npos;
      }
    }

  /* read ROTANG. Requires two values for Boxannulus, Elliptannulus, Sector, 
     Panda; one for Box, Diamond, Ellipse; and none for Circle, Point, Annulus, 
     Rectangle, Polygon */

    npos = 0;
    switch ( newShape->shape ) {
    case box_rgn:
    case diamond_rgn:
    case ellipse_rgn:
      npos = 1;
      break;
    case boxannulus_rgn:
    case elliptannulus_rgn:
    case sector_rgn:
      npos = 2;
      break;
    default:
     break;
    }

    if ( npos > 0 ) {
      if ( ffgcvd(fptr, icol[4], i, 1, npos, 0.0, coords, &anynul, status) ) {
	ffpmsg("Failed to read ROTANG column for region");
	goto error;
      }

    /* transform angles if the region and input wcs differ */

      if ( dotransform ) {
	Theta = (wcs->rot) - (regwcs->rot);
	for (j=0; j<npos; j++) *(coords++) += Theta;
      } else {
	coords += npos;
      }
    }

  /* read the component number */

    if (got_component) {
      if ( ffgcv(fptr, TINT, icol[5], i, 1, 1, 0, &newShape->comp, &anynul, status) ) {
        ffpmsg("Failed to read COMPONENT column for region");
        goto error;
      }
    } else {
      newShape->comp = 1;
    }


    /* do some precalculations to speed up tests */

    fits_setup_shape(newShape);

    /* end loop over shapes */

  }

error:

   if( *status )
      fits_free_region( aRgn );
   else
      *Rgn = aRgn;

   ffclos(fptr, status);

   return( *status );
}

