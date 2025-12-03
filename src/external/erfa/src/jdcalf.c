#include "erfa.h"
#include "erfam.h"

int eraJdcalf(int ndp, double dj1, double dj2, int iymdf[4])
/*
**  - - - - - - - - - -
**   e r a J d c a l f
**  - - - - - - - - - -
**
**  Julian Date to Gregorian Calendar, expressed in a form convenient
**  for formatting messages:  rounded to a specified precision.
**
**  Given:
**     ndp       int      number of decimal places of days in fraction
**     dj1,dj2   double   dj1+dj2 = Julian Date (Note 1)
**
**  Returned:
**     iymdf     int[4]   year, month, day, fraction in Gregorian
**                        calendar
**
**  Returned (function value):
**               int      status:
**                          -1 = date out of range
**                           0 = OK
**                          +1 = ndp not 0-9 (interpreted as 0)
**
**  Notes:
**
**  1) The Julian Date is apportioned in any convenient way between
**     the arguments dj1 and dj2.  For example, JD=2450123.7 could
**     be expressed in any of these ways, among others:
**
**             dj1            dj2
**
**         2450123.7           0.0       (JD method)
**         2451545.0       -1421.3       (J2000 method)
**         2400000.5       50123.2       (MJD method)
**         2450123.5           0.2       (date & time method)
**
**  2) In early eras the conversion is from the "Proleptic Gregorian
**     Calendar";  no account is taken of the date(s) of adoption of
**     the Gregorian Calendar, nor is the AD/BC numbering convention
**     observed.
**
**  3) See also the function eraJd2cal.
**
**  4) The number of decimal places ndp should be 4 or less if internal
**     overflows are to be avoided on platforms which use 16-bit
**     integers.
**
**  Called:
**     eraJd2cal    JD to Gregorian calendar
**
**  Reference:
**
**     Explanatory Supplement to the Astronomical Almanac,
**     P. Kenneth Seidelmann (ed), University Science Books (1992),
**     Section 12.92 (p604).
**
**  This revision:  2023 January 16
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   int j, js;
   double denom, d1, d2, f1, f2, d, djd, f, rf;


/* Denominator of fraction (e.g. 100 for 2 decimal places). */
   if ((ndp >= 0) && (ndp <= 9)) {
      j = 0;
      denom = pow(10.0, ndp);
   } else {
      j = 1;
      denom = 1.0;
   }

/* Copy the date, big then small. */
   if (fabs(dj1) >= fabs(dj2)) {
      d1 = dj1;
      d2 = dj2;
   } else {
      d1 = dj2;
      d2 = dj1;
   }

/* Realign to midnight (without rounding error). */
   d1 -= 0.5;

/* Separate day and fraction (as precisely as possible). */
   d = ERFA_DNINT(d1);
   f1 = d1 - d;
   djd = d;
   d = ERFA_DNINT(d2);
   f2 = d2 - d;
   djd += d;
   d = ERFA_DNINT(f1 + f2);
   f = (f1 - d) + f2;
   if (f < 0.0) {
       f += 1.0;
       d -= 1.0;
   }
   djd += d;

/* Round the total fraction to the specified number of places. */
   rf = ERFA_DNINT(f*denom) / denom;

/* Re-align to noon. */
   djd += 0.5;

/* Convert to Gregorian calendar. */
   js = eraJd2cal(djd, rf, &iymdf[0], &iymdf[1], &iymdf[2], &f);
   if (js == 0) {
      iymdf[3] = (int) ERFA_DNINT(f * denom);
   } else {
      j = js;
   }

/* Return the status. */
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
