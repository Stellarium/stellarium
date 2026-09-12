#include "erfa.h"
#include "erfam.h"

int eraUtctai(double utc1, double utc2, double *tai1, double *tai2)
/*
**  - - - - - - - - - -
**   e r a U t c t a i
**  - - - - - - - - - -
**
**  Time scale transformation:  Coordinated Universal Time, UTC, to
**  International Atomic Time, TAI.
**
**  Given:
**     utc1,utc2  double   UTC as a 2-part quasi Julian Date (Notes 1-4)
**
**  Returned:
**     tai1,tai2  double   TAI as a 2-part Julian Date (Note 5)
**
**  Returned (function value):
**                int      status: +1 = dubious year (Note 3)
**                                  0 = OK
**                                 -1 = unacceptable date
**
**  Notes:
**
**  1) utc1+utc2 is quasi Julian Date (see Note 2), apportioned in any
**     convenient way between the two arguments, for example where utc1
**     is the Julian Day Number and utc2 is the fraction of a day.
**
**  2) JD cannot unambiguously represent UTC during a leap second unless
**     special measures are taken.  The convention in the present
**     function is that the JD day represents UTC days whether the
**     length is 86399, 86400 or 86401 SI seconds.  In the 1960-1972 era
**     there were smaller jumps (in either direction) each time the
**     linear UTC(TAI) expression was changed, and these "mini-leaps"
**     are also included in the ERFA convention.
**
**  3) The warning status "dubious year" flags UTCs that predate the
**     introduction of the time scale or that are too far in the future
**     to be trusted.  See eraDat for further details.
**
**  4) The function eraDtf2d converts from calendar date and time of day
**     into 2-part Julian Date, and in the case of UTC implements the
**     leap-second-ambiguity convention described above.
**
**  5) The returned TAI1,TAI2 are such that their sum is the TAI Julian
**     Date.
**
**  Called:
**     eraJd2cal    JD to Gregorian calendar
**     eraDat       delta(AT) = TAI-UTC
**     eraCal2jd    Gregorian calendar to JD
**
**  References:
**
**     McCarthy, D. D., Petit, G. (eds.), IERS Conventions (2003),
**     IERS Technical Note No. 32, BKG (2004)
**
**     Explanatory Supplement to the Astronomical Almanac,
**     P. Kenneth Seidelmann (ed), University Science Books (1992)
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   int big1;
   int iy, im, id, j, iyt, imt, idt;
   double u1, u2, fd, dat0, dat12, w, dat24, dlod, dleap, z1, z2, a2;


/* Put the two parts of the UTC into big-first order. */
   big1 = ( fabs(utc1) >= fabs(utc2) );
   if ( big1 ) {
      u1 = utc1;
      u2 = utc2;
   } else {
      u1 = utc2;
      u2 = utc1;
   }

/* Get TAI-UTC at 0h today. */
   j = eraJd2cal(u1, u2, &iy, &im, &id, &fd);
   if ( j ) return j;
   j = eraDat(iy, im, id, 0.0, &dat0);
   if ( j < 0 ) return j;

/* Get TAI-UTC at 12h today (to detect drift). */
   j = eraDat(iy, im, id, 0.5, &dat12);
   if ( j < 0 ) return j;

/* Get TAI-UTC at 0h tomorrow (to detect jumps). */
   j = eraJd2cal(u1+1.5, u2-fd, &iyt, &imt, &idt, &w);
   if ( j ) return j;
   j = eraDat(iyt, imt, idt, 0.0, &dat24);
   if ( j < 0 ) return j;

/* Separate TAI-UTC change into per-day (DLOD) and any jump (DLEAP). */
   dlod = 2.0 * (dat12 - dat0);
   dleap = dat24 - (dat0 + dlod);

/* Remove any scaling applied to spread leap into preceding day. */
   fd *= (ERFA_DAYSEC+dleap)/ERFA_DAYSEC;

/* Scale from (pre-1972) UTC seconds to SI seconds. */
   fd *= (ERFA_DAYSEC+dlod)/ERFA_DAYSEC;

/* Today's calendar date to 2-part JD. */
   if ( eraCal2jd(iy, im, id, &z1, &z2) ) return -1;

/* Assemble the TAI result, preserving the UTC split and order. */
   a2 = z1 - u1;
   a2 += z2;
   a2 += fd + dat0/ERFA_DAYSEC;
   if ( big1 ) {
      *tai1 = u1;
      *tai2 = a2;
   } else {
      *tai1 = a2;
      *tai2 = u1;
   }

/* Status. */
   return j;

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
