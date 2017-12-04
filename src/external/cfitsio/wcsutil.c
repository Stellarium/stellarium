#include <math.h>
#include "fitsio2.h"
#define D2R 0.01745329252
#define TWOPI 6.28318530717959

/*--------------------------------------------------------------------------*/
int ffwldp(double xpix, double ypix, double xref, double yref,
      double xrefpix, double yrefpix, double xinc, double yinc, double rot,
      char *type, double *xpos, double *ypos, int *status)

/* This routine is based on the classic AIPS WCS routine. 

   It converts from pixel location to RA,Dec for 9 projective geometries:
   "-CAR", "-SIN", "-TAN", "-ARC", "-NCP", "-GLS", "-MER", "-AIT" and "-STG".
*/

/*-----------------------------------------------------------------------*/
/* routine to determine accurate position for pixel coordinates          */
/* returns 0 if successful otherwise:                                    */
/* 501 = angle too large for projection;                                 */
/* does: -CAR, -SIN, -TAN, -ARC, -NCP, -GLS, -MER, -AIT  -STG projections*/
/* Input:                                                                */
/*   f   xpix    x pixel number  (RA or long without rotation)           */
/*   f   ypiy    y pixel number  (dec or lat without rotation)           */
/*   d   xref    x reference coordinate value (deg)                      */
/*   d   yref    y reference coordinate value (deg)                      */
/*   f   xrefpix x reference pixel                                       */
/*   f   yrefpix y reference pixel                                       */
/*   f   xinc    x coordinate increment (deg)                            */
/*   f   yinc    y coordinate increment (deg)                            */
/*   f   rot     rotation (deg)  (from N through E)                      */
/*   c  *type    projection type code e.g. "-SIN";                       */
/* Output:                                                               */
/*   d   *xpos   x (RA) coordinate (deg)                                 */
/*   d   *ypos   y (dec) coordinate (deg)                                */
/*-----------------------------------------------------------------------*/
 {double cosr, sinr, dx, dy, dz, temp, x, y, z;
  double sins, coss, dect, rat, dt, l, m, mg, da, dd, cos0, sin0;
  double dec0, ra0;
  double geo1, geo2, geo3;
  double deps = 1.0e-5;
  char *cptr;
  
  if (*status > 0)
     return(*status);

/*   Offset from ref pixel  */
  dx = (xpix-xrefpix) * xinc;
  dy = (ypix-yrefpix) * yinc;

/*   Take out rotation  */
  cosr = cos(rot * D2R);
  sinr = sin(rot * D2R);
  if (rot != 0.0) {
     temp = dx * cosr - dy * sinr;
     dy = dy * cosr + dx * sinr;
     dx = temp;
  }

/* convert to radians  */
  ra0 = xref * D2R;
  dec0 = yref * D2R;

  l = dx * D2R;
  m = dy * D2R;
  sins = l*l + m*m;
  cos0 = cos(dec0);
  sin0 = sin(dec0);

  if (*type != '-') {  /* unrecognized projection code */
     return(*status = 504);
  }

    cptr = type + 1;

    if (*cptr == 'C') { /* linear -CAR */
      if (*(cptr + 1) != 'A' ||  *(cptr + 2) != 'R') {
         return(*status = 504);
      }
      rat =  ra0 + l;
      dect = dec0 + m;

    } else if (*cptr == 'T') {  /* -TAN */
      if (*(cptr + 1) != 'A' ||  *(cptr + 2) != 'N') {
         return(*status = 504);
      }
      x = cos0*cos(ra0) - l*sin(ra0) - m*cos(ra0)*sin0;
      y = cos0*sin(ra0) + l*cos(ra0) - m*sin(ra0)*sin0;
      z = sin0                       + m*         cos0;
      rat  = atan2( y, x );
      dect = atan ( z / sqrt(x*x+y*y) );

    } else if (*cptr == 'S') {

      if (*(cptr + 1) == 'I' &&  *(cptr + 2) == 'N') { /* -SIN */
          if (sins>1.0)
	    return(*status = 501);
          coss = sqrt (1.0 - sins);
          dt = sin0 * coss + cos0 * m;
          if ((dt>1.0) || (dt<-1.0))
	    return(*status = 501);
          dect = asin (dt);
          rat = cos0 * coss - sin0 * m;
          if ((rat==0.0) && (l==0.0))
	    return(*status = 501);
          rat = atan2 (l, rat) + ra0;

       } else if (*(cptr + 1) == 'T' &&  *(cptr + 2) == 'G') {  /* -STG Sterographic*/
          dz = (4.0 - sins) / (4.0 + sins);
          if (fabs(dz)>1.0)
	    return(*status = 501);
          dect = dz * sin0 + m * cos0 * (1.0+dz) / 2.0;
          if (fabs(dect)>1.0)
	    return(*status = 501);
          dect = asin (dect);
          rat = cos(dect);
          if (fabs(rat)<deps)
	    return(*status = 501);
          rat = l * (1.0+dz) / (2.0 * rat);
          if (fabs(rat)>1.0)
	    return(*status = 501);
          rat = asin (rat);
          mg = 1.0 + sin(dect) * sin0 + cos(dect) * cos0 * cos(rat);
          if (fabs(mg)<deps)
	    return(*status = 501);
          mg = 2.0 * (sin(dect) * cos0 - cos(dect) * sin0 * cos(rat)) / mg;
          if (fabs(mg-m)>deps)
	    rat = TWOPI /2.0 - rat;
          rat = ra0 + rat;
        } else  {
          return(*status = 504);
        }
 
    } else if (*cptr == 'A') {

      if (*(cptr + 1) == 'R' &&  *(cptr + 2) == 'C') { /* ARC */
          if (sins>=TWOPI*TWOPI/4.0)
	    return(*status = 501);
          sins = sqrt(sins);
          coss = cos (sins);
          if (sins!=0.0)
	    sins = sin (sins) / sins;
          else
	    sins = 1.0;
          dt = m * cos0 * sins + sin0 * coss;
          if ((dt>1.0) || (dt<-1.0))
	    return(*status = 501);
          dect = asin (dt);
          da = coss - dt * sin0;
          dt = l * sins * cos0;
          if ((da==0.0) && (dt==0.0))
	    return(*status = 501);
          rat = ra0 + atan2 (dt, da);

      } else if (*(cptr + 1) == 'I' &&  *(cptr + 2) == 'T') {  /* -AIT Aitoff */
          dt = yinc*cosr + xinc*sinr;
          if (dt==0.0)
	    dt = 1.0;
          dt = dt * D2R;
          dy = yref * D2R;
          dx = sin(dy+dt)/sqrt((1.0+cos(dy+dt))/2.0) -
	      sin(dy)/sqrt((1.0+cos(dy))/2.0);
          if (dx==0.0)
	    dx = 1.0;
          geo2 = dt / dx;
          dt = xinc*cosr - yinc* sinr;
          if (dt==0.0)
	    dt = 1.0;
          dt = dt * D2R;
          dx = 2.0 * cos(dy) * sin(dt/2.0);
          if (dx==0.0) dx = 1.0;
          geo1 = dt * sqrt((1.0+cos(dy)*cos(dt/2.0))/2.0) / dx;
          geo3 = geo2 * sin(dy) / sqrt((1.0+cos(dy))/2.0);
          rat = ra0;
          dect = dec0;
          if ((l != 0.0) || (m != 0.0)) {
            dz = 4.0 - l*l/(4.0*geo1*geo1) - ((m+geo3)/geo2)*((m+geo3)/geo2) ;
            if ((dz>4.0) || (dz<2.0)) return(*status = 501);
            dz = 0.5 * sqrt (dz);
            dd = (m+geo3) * dz / geo2;
            if (fabs(dd)>1.0) return(*status = 501);
            dd = asin (dd);
            if (fabs(cos(dd))<deps) return(*status = 501);
            da = l * dz / (2.0 * geo1 * cos(dd));
            if (fabs(da)>1.0) return(*status = 501);
            da = asin (da);
            rat = ra0 + 2.0 * da;
            dect = dd;
          }
        } else  {
          return(*status = 504);
        }
 
    } else if (*cptr == 'N') { /* -NCP North celestial pole*/
      if (*(cptr + 1) != 'C' ||  *(cptr + 2) != 'P') {
         return(*status = 504);
      }
      dect = cos0 - m * sin0;
      if (dect==0.0)
        return(*status = 501);
      rat = ra0 + atan2 (l, dect);
      dt = cos (rat-ra0);
      if (dt==0.0)
        return(*status = 501);
      dect = dect / dt;
      if ((dect>1.0) || (dect<-1.0))
        return(*status = 501);
      dect = acos (dect);
      if (dec0<0.0) dect = -dect;

    } else if (*cptr == 'G') {   /* -GLS global sinusoid */
      if (*(cptr + 1) != 'L' ||  *(cptr + 2) != 'S') {
         return(*status = 504);
      }
      dect = dec0 + m;
      if (fabs(dect)>TWOPI/4.0)
        return(*status = 501);
      coss = cos (dect);
      if (fabs(l)>TWOPI*coss/2.0)
        return(*status = 501);
      rat = ra0;
      if (coss>deps) rat = rat + l / coss;

    } else if (*cptr == 'M') {  /* -MER mercator*/
      if (*(cptr + 1) != 'E' ||  *(cptr + 2) != 'R') {
         return(*status = 504);
      }
      dt = yinc * cosr + xinc * sinr;
      if (dt==0.0) dt = 1.0;
      dy = (yref/2.0 + 45.0) * D2R;
      dx = dy + dt / 2.0 * D2R;
      dy = log (tan (dy));
      dx = log (tan (dx));
      geo2 = dt * D2R / (dx - dy);
      geo3 = geo2 * dy;
      geo1 = cos (yref*D2R);
      if (geo1<=0.0) geo1 = 1.0;
      rat = l / geo1 + ra0;
      if (fabs(rat - ra0) > TWOPI)
        return(*status = 501);
      dt = 0.0;
      if (geo2!=0.0) dt = (m + geo3) / geo2;
      dt = exp (dt);
      dect = 2.0 * atan (dt) - TWOPI / 4.0;

    } else  {
      return(*status = 504);
    }

  /*  correct for RA rollover  */
  if (rat-ra0>TWOPI/2.0) rat = rat - TWOPI;
  if (rat-ra0<-TWOPI/2.0) rat = rat + TWOPI;
  if (rat < 0.0) rat += TWOPI;

  /*  convert to degrees  */
  *xpos  = rat  / D2R;
  *ypos  = dect  / D2R;
  return(*status);
} 
/*--------------------------------------------------------------------------*/
int ffxypx(double xpos, double ypos, double xref, double yref, 
      double xrefpix, double yrefpix, double xinc, double yinc, double rot,
      char *type, double *xpix, double *ypix, int *status)

/* This routine is based on the classic AIPS WCS routine. 

   It converts from RA,Dec to pixel location to for 9 projective geometries:
   "-CAR", "-SIN", "-TAN", "-ARC", "-NCP", "-GLS", "-MER", "-AIT" and "-STG".
*/
/*-----------------------------------------------------------------------*/
/* routine to determine accurate pixel coordinates for an RA and Dec     */
/* returns 0 if successful otherwise:                                    */
/* 501 = angle too large for projection;                                 */
/* 502 = bad values                                                      */
/* does: -SIN, -TAN, -ARC, -NCP, -GLS, -MER, -AIT projections            */
/* anything else is linear                                               */
/* Input:                                                                */
/*   d   xpos    x (RA) coordinate (deg)                                 */
/*   d   ypos    y (dec) coordinate (deg)                                */
/*   d   xref    x reference coordinate value (deg)                      */
/*   d   yref    y reference coordinate value (deg)                      */
/*   f   xrefpix x reference pixel                                       */
/*   f   yrefpix y reference pixel                                       */
/*   f   xinc    x coordinate increment (deg)                            */
/*   f   yinc    y coordinate increment (deg)                            */
/*   f   rot     rotation (deg)  (from N through E)                      */
/*   c  *type    projection type code e.g. "-SIN";                       */
/* Output:                                                               */
/*   f  *xpix    x pixel number  (RA or long without rotation)           */
/*   f  *ypiy    y pixel number  (dec or lat without rotation)           */
/*-----------------------------------------------------------------------*/
 {
  double dx, dy, dz, r, ra0, dec0, ra, dec, coss, sins, dt, da, dd, sint;
  double l, m, geo1, geo2, geo3, sinr, cosr, cos0, sin0;
  double deps=1.0e-5;
  char *cptr;

  if (*type != '-') {  /* unrecognized projection code */
     return(*status = 504);
  }

  cptr = type + 1;

  dt = (xpos - xref);
  if (dt >  180) xpos -= 360;
  if (dt < -180) xpos += 360;
  /* NOTE: changing input argument xpos is OK (call-by-value in C!) */

  /* default values - linear */
  dx = xpos - xref;
  dy = ypos - yref;

  /*  Correct for rotation */
  r = rot * D2R;
  cosr = cos (r);
  sinr = sin (r);
  dz = dx*cosr + dy*sinr;
  dy = dy*cosr - dx*sinr;
  dx = dz;

  /*     check axis increments - bail out if either 0 */
  if ((xinc==0.0) || (yinc==0.0)) {*xpix=0.0; *ypix=0.0;
    return(*status = 502);}

  /*     convert to pixels  */
  *xpix = dx / xinc + xrefpix;
  *ypix = dy / yinc + yrefpix;

  if (*cptr == 'C') { /* linear -CAR */
      if (*(cptr + 1) != 'A' ||  *(cptr + 2) != 'R') {
         return(*status = 504);
      }

      return(*status);  /* done if linear */
  }

  /* Non linear position */
  ra0 = xref * D2R;
  dec0 = yref * D2R;
  ra = xpos * D2R;
  dec = ypos * D2R;

  /* compute direction cosine */
  coss = cos (dec);
  sins = sin (dec);
  cos0 = cos (dec0);
  sin0 = sin (dec0);
  l = sin(ra-ra0) * coss;
  sint = sins * sin0 + coss * cos0 * cos(ra-ra0);

    /* process by case  */
    if (*cptr == 'T') {  /* -TAN tan */
         if (*(cptr + 1) != 'A' ||  *(cptr + 2) != 'N') {
           return(*status = 504);
         }

         if (sint<=0.0)
	   return(*status = 501);
         if( cos0<0.001 ) {
            /* Do a first order expansion around pole */
            m = (coss * cos(ra-ra0)) / (sins * sin0);
            m = (-m + cos0 * (1.0 + m*m)) / sin0;
         } else {
            m = ( sins/sint - sin0 ) / cos0;
         }
	 if( fabs(sin(ra0)) < 0.3 ) {
	    l  = coss*sin(ra)/sint - cos0*sin(ra0) + m*sin(ra0)*sin0;
	    l /= cos(ra0);
	 } else {
	    l  = coss*cos(ra)/sint - cos0*cos(ra0) + m*cos(ra0)*sin0;
	    l /= -sin(ra0);
	 }

    } else if (*cptr == 'S') {

      if (*(cptr + 1) == 'I' &&  *(cptr + 2) == 'N') { /* -SIN */
         if (sint<0.0)
	   return(*status = 501);
         m = sins * cos(dec0) - coss * sin(dec0) * cos(ra-ra0);

      } else if (*(cptr + 1) == 'T' &&  *(cptr + 2) == 'G') {  /* -STG Sterographic*/
         da = ra - ra0;
         if (fabs(dec)>TWOPI/4.0)
	   return(*status = 501);
         dd = 1.0 + sins * sin(dec0) + coss * cos(dec0) * cos(da);
         if (fabs(dd)<deps)
	   return(*status = 501);
         dd = 2.0 / dd;
         l = l * dd;
         m = dd * (sins * cos(dec0) - coss * sin(dec0) * cos(da));

        } else  {
          return(*status = 504);
        }
 
    } else if (*cptr == 'A') {

      if (*(cptr + 1) == 'R' &&  *(cptr + 2) == 'C') { /* ARC */
         m = sins * sin(dec0) + coss * cos(dec0) * cos(ra-ra0);
         if (m<-1.0) m = -1.0;
         if (m>1.0) m = 1.0;
         m = acos (m);
         if (m!=0) 
            m = m / sin(m);
         else
            m = 1.0;
         l = l * m;
         m = (sins * cos(dec0) - coss * sin(dec0) * cos(ra-ra0)) * m;

      } else if (*(cptr + 1) == 'I' &&  *(cptr + 2) == 'T') {  /* -AIT Aitoff */
         da = (ra - ra0) / 2.0;
         if (fabs(da)>TWOPI/4.0)
	     return(*status = 501);
         dt = yinc*cosr + xinc*sinr;
         if (dt==0.0) dt = 1.0;
         dt = dt * D2R;
         dy = yref * D2R;
         dx = sin(dy+dt)/sqrt((1.0+cos(dy+dt))/2.0) -
             sin(dy)/sqrt((1.0+cos(dy))/2.0);
         if (dx==0.0) dx = 1.0;
         geo2 = dt / dx;
         dt = xinc*cosr - yinc* sinr;
         if (dt==0.0) dt = 1.0;
         dt = dt * D2R;
         dx = 2.0 * cos(dy) * sin(dt/2.0);
         if (dx==0.0) dx = 1.0;
         geo1 = dt * sqrt((1.0+cos(dy)*cos(dt/2.0))/2.0) / dx;
         geo3 = geo2 * sin(dy) / sqrt((1.0+cos(dy))/2.0);
         dt = sqrt ((1.0 + cos(dec) * cos(da))/2.0);
         if (fabs(dt)<deps)
	     return(*status = 503);
         l = 2.0 * geo1 * cos(dec) * sin(da) / dt;
         m = geo2 * sin(dec) / dt - geo3;

        } else  {
          return(*status = 504);
        }
 
    } else if (*cptr == 'N') { /* -NCP North celestial pole*/
         if (*(cptr + 1) != 'C' ||  *(cptr + 2) != 'P') {
             return(*status = 504);
         }

         if (dec0==0.0) 
	     return(*status = 501);  /* can't stand the equator */
         else
	   m = (cos(dec0) - coss * cos(ra-ra0)) / sin(dec0);

    } else if (*cptr == 'G') {   /* -GLS global sinusoid */
         if (*(cptr + 1) != 'L' ||  *(cptr + 2) != 'S') {
             return(*status = 504);
         }

         dt = ra - ra0;
         if (fabs(dec)>TWOPI/4.0)
	   return(*status = 501);
         if (fabs(dec0)>TWOPI/4.0)
	   return(*status = 501);
         m = dec - dec0;
         l = dt * coss;

    } else if (*cptr == 'M') {  /* -MER mercator*/
         if (*(cptr + 1) != 'E' ||  *(cptr + 2) != 'R') {
             return(*status = 504);
         }

         dt = yinc * cosr + xinc * sinr;
         if (dt==0.0) dt = 1.0;
         dy = (yref/2.0 + 45.0) * D2R;
         dx = dy + dt / 2.0 * D2R;
         dy = log (tan (dy));
         dx = log (tan (dx));
         geo2 = dt * D2R / (dx - dy);
         geo3 = geo2 * dy;
         geo1 = cos (yref*D2R);
         if (geo1<=0.0) geo1 = 1.0;
         dt = ra - ra0;
         l = geo1 * dt;
         dt = dec / 2.0 + TWOPI / 8.0;
         dt = tan (dt);
         if (dt<deps)
	   return(*status = 502);
         m = geo2 * log (dt) - geo3;

    } else  {
      return(*status = 504);
    }

    /*   convert to degrees  */
    dx = l / D2R;
    dy = m / D2R;

    /*  Correct for rotation */
    dz = dx*cosr + dy*sinr;
    dy = dy*cosr - dx*sinr;
    dx = dz;

    /*     convert to pixels  */
    *xpix = dx / xinc + xrefpix;
    *ypix = dy / yinc + yrefpix;
    return(*status);
}
