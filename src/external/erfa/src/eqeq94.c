#include "erfa.h"
#include "erfam.h"

double eraEqeq94(double date1, double date2)
/*
**  - - - - - - - - - -
**   e r a E q e q 9 4
**  - - - - - - - - - -
**
**  Equation of the equinoxes, IAU 1994 model.
**
**  Given:
**     date1,date2   double     TDB date (Note 1)
**
**  Returned (function value):
**                   double     equation of the equinoxes (Note 2)
**
**  Notes:
**
**  1) The date date1+date2 is a Julian Date, apportioned in any
**     convenient way between the two arguments.  For example,
**     JD(TT)=2450123.7 could be expressed in any of these ways,
**     among others:
**
**            date1          date2
**
**         2450123.7           0.0       (JD method)
**         2451545.0       -1421.3       (J2000 method)
**         2400000.5       50123.2       (MJD method)
**         2450123.5           0.2       (date & time method)
**
**     The JD method is the most natural and convenient to use in
**     cases where the loss of several decimal digits of resolution
**     is acceptable.  The J2000 method is best matched to the way
**     the argument is handled internally and will deliver the
**     optimum resolution.  The MJD method and the date & time methods
**     are both good compromises between resolution and convenience.
**
**  2) The result, which is in radians, operates in the following sense:
**
**        Greenwich apparent ST = GMST + equation of the equinoxes
**
**  Called:
**     eraAnpm      normalize angle into range +/- pi
**     eraNut80     nutation, IAU 1980
**     eraObl80     mean obliquity, IAU 1980
**
**  References:
**
**     IAU Resolution C7, Recommendation 3 (1994).
**
**     Capitaine, N. & Gontier, A.-M., 1993, Astron.Astrophys., 275,
**     645-650.
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double t,  om,  dpsi,  deps,  eps0, ee;


/* Interval between fundamental epoch J2000.0 and given date (JC). */
   t = ((date1 - ERFA_DJ00) + date2) / ERFA_DJC;

/* Longitude of the mean ascending node of the lunar orbit on the */
/* ecliptic, measured from the mean equinox of date. */
   om = eraAnpm((450160.280 + (-482890.539
           + (7.455 + 0.008 * t) * t) * t) * ERFA_DAS2R
           + fmod(-5.0 * t, 1.0) * ERFA_D2PI);

/* Nutation components and mean obliquity. */
   eraNut80(date1, date2, &dpsi, &deps);
   eps0 = eraObl80(date1, date2);

/* Equation of the equinoxes. */
   ee = dpsi*cos(eps0) + ERFA_DAS2R*(0.00264*sin(om) + 0.000063*sin(om+om));

   return ee;

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
