#include "erfa.h"
#include "erfam.h"
#include <string.h>

int eraD2dtf(const char *scale, int ndp, double d1, double d2,
             int *iy, int *im, int *id, int ihmsf[4])
/*
**  - - - - - - - - -
**   e r a D 2 d t f
**  - - - - - - - - -
**
**  Format for output a 2-part Julian Date (or in the case of UTC a
**  quasi-JD form that includes special provision for leap seconds).
**
**  Given:
**     scale     char[]  time scale ID (Note 1)
**     ndp       int     resolution (Note 2)
**     d1,d2     double  time as a 2-part Julian Date (Notes 3,4)
**
**  Returned:
**     iy,im,id  int     year, month, day in Gregorian calendar (Note 5)
**     ihmsf     int[4]  hours, minutes, seconds, fraction (Note 1)
**
**  Returned (function value):
**               int     status: +1 = dubious year (Note 5)
**                                0 = OK
**                               -1 = unacceptable date (Note 6)
**
**  Notes:
**
**  1) scale identifies the time scale.  Only the value "UTC" (in upper
**     case) is significant, and enables handling of leap seconds (see
**     Note 4).
**
**  2) ndp is the number of decimal places in the seconds field, and can
**     have negative as well as positive values, such as:
**
**     ndp         resolution
**     -4            1 00 00
**     -3            0 10 00
**     -2            0 01 00
**     -1            0 00 10
**      0            0 00 01
**      1            0 00 00.1
**      2            0 00 00.01
**      3            0 00 00.001
**
**     The limits are platform dependent, but a safe range is -5 to +9.
**
**  3) d1+d2 is Julian Date, apportioned in any convenient way between
**     the two arguments, for example where d1 is the Julian Day Number
**     and d2 is the fraction of a day.  In the case of UTC, where the
**     use of JD is problematical, special conventions apply:  see the
**     next note.
**
**  4) JD cannot unambiguously represent UTC during a leap second unless
**     special measures are taken.  The ERFA internal convention is that
**     the quasi-JD day represents UTC days whether the length is 86399,
**     86400 or 86401 SI seconds.  In the 1960-1972 era there were
**     smaller jumps (in either direction) each time the linear UTC(TAI)
**     expression was changed, and these "mini-leaps" are also included
**     in the ERFA convention.
**
**  5) The warning status "dubious year" flags UTCs that predate the
**     introduction of the time scale or that are too far in the future
**     to be trusted.  See eraDat for further details.
**
**  6) For calendar conventions and limitations, see eraCal2jd.
**
**  Called:
**     eraJd2cal    JD to Gregorian calendar
**     eraD2tf      decompose days to hms
**     eraDat       delta(AT) = TAI-UTC
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   int leap;
   char s;
   int iy1, im1, id1, js, iy2, im2, id2, ihmsf1[4], i;
   double a1, b1, fd, dat0, dat12, w, dat24, dleap;


/* The two-part JD. */
   a1 = d1;
   b1 = d2;

/* Provisional calendar date. */
   js = eraJd2cal(a1, b1, &iy1, &im1, &id1, &fd);
   if ( js ) return -1;

/* Is this a leap second day? */
   leap = 0;
   if ( ! strcmp(scale,"UTC") ) {

   /* TAI-UTC at 0h today. */
      js = eraDat(iy1, im1, id1, 0.0, &dat0);
      if ( js < 0 ) return -1;

   /* TAI-UTC at 12h today (to detect drift). */
      js = eraDat(iy1, im1, id1, 0.5, &dat12);
      if ( js < 0 ) return -1;

   /* TAI-UTC at 0h tomorrow (to detect jumps). */
      js = eraJd2cal(a1+1.5, b1-fd, &iy2, &im2, &id2, &w);
      if ( js ) return -1;
      js = eraDat(iy2, im2, id2, 0.0, &dat24);
      if ( js < 0 ) return -1;

   /* Any sudden change in TAI-UTC (seconds). */
      dleap = dat24 - (2.0*dat12 - dat0);

   /* If leap second day, scale the fraction of a day into SI. */
      leap = (fabs(dleap) > 0.5);
      if (leap) fd += fd * dleap/ERFA_DAYSEC;
   }

/* Provisional time of day. */
   eraD2tf ( ndp, fd, &s, ihmsf1 );

/* Has the (rounded) time gone past 24h? */
   if ( ihmsf1[0] > 23 ) {

   /* Yes.  We probably need tomorrow's calendar date. */
      js = eraJd2cal(a1+1.5, b1-fd, &iy2, &im2, &id2, &w);
      if ( js ) return -1;

   /* Is today a leap second day? */
      if ( ! leap ) {

      /* No.  Use 0h tomorrow. */
         iy1 = iy2;
         im1 = im2;
         id1 = id2;
         ihmsf1[0] = 0;
         ihmsf1[1] = 0;
         ihmsf1[2] = 0;

      } else {

      /* Yes.  Are we past the leap second itself? */
         if ( ihmsf1[2] > 0 ) {

         /* Yes.  Use tomorrow but allow for the leap second. */
            iy1 = iy2;
            im1 = im2;
            id1 = id2;
            ihmsf1[0] = 0;
            ihmsf1[1] = 0;
            ihmsf1[2] = 0;

         } else {

         /* No.  Use 23 59 60... today. */
            ihmsf1[0] = 23;
            ihmsf1[1] = 59;
            ihmsf1[2] = 60;
         }

      /* If rounding to 10s or coarser always go up to new day. */
         if ( ndp < 0 && ihmsf1[2] == 60 ) {
            iy1 = iy2;
            im1 = im2;
            id1 = id2;
            ihmsf1[0] = 0;
            ihmsf1[1] = 0;
            ihmsf1[2] = 0;
         }
      }
   }

/* Results. */
   *iy = iy1;
   *im = im1;
   *id = id1;
   for ( i = 0; i < 4; i++ ) {
      ihmsf[i] = ihmsf1[i];
   }

/* Status. */
   return js;

/* Finished. */

}
/*----------------------------------------------------------------------
**  
**  
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  All rights reserved.
**  
**  This library is derived, with permission, from the International
**  Astronomical Union's "Standards of Fundamental Astronomy" library,
**  available from http://www.iausofa.org.
**  
**  The ERFA version is intended to retain identical functionality to
**  the SOFA library, but made distinct through different function and
**  file names, as set out in the SOFA license conditions.  The SOFA
**  original has a role as a reference standard for the IAU and IERS,
**  and consequently redistribution is permitted only in its unaltered
**  state.  The ERFA version is not subject to this restriction and
**  therefore can be included in distributions which do not support the
**  concept of "read only" software.
**  
**  Although the intent is to replicate the SOFA API (other than
**  replacement of prefix names) and results (with the exception of
**  bugs;  any that are discovered will be fixed), SOFA is not
**  responsible for any errors found in this version of the library.
**  
**  If you wish to acknowledge the SOFA heritage, please acknowledge
**  that you are using a library derived from SOFA, rather than SOFA
**  itself.
**  
**  
**  TERMS AND CONDITIONS
**  
**  Redistribution and use in source and binary forms, with or without
**  modification, are permitted provided that the following conditions
**  are met:
**  
**  1 Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**  
**  2 Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in
**    the documentation and/or other materials provided with the
**    distribution.
**  
**  3 Neither the name of the Standards Of Fundamental Astronomy Board,
**    the International Astronomical Union nor the names of its
**    contributors may be used to endorse or promote products derived
**    from this software without specific prior written permission.
**  
**  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
**  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
**  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
**  FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
**  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
**  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
**  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
**  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
**  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
**  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
**  POSSIBILITY OF SUCH DAMAGE.
**  
*/
