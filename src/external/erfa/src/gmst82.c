#include "erfa.h"
#include "erfam.h"

double eraGmst82(double dj1, double dj2)
/*
**  - - - - - - - - - -
**   e r a G m s t 8 2
**  - - - - - - - - - -
**
**  Universal Time to Greenwich mean sidereal time (IAU 1982 model).
**
**  Given:
**     dj1,dj2    double    UT1 Julian Date (see note)
**
**  Returned (function value):
**                double    Greenwich mean sidereal time (radians)
**
**  Notes:
**
**  1) The UT1 date dj1+dj2 is a Julian Date, apportioned in any
**     convenient way between the arguments dj1 and dj2.  For example,
**     JD(UT1)=2450123.7 could be expressed in any of these ways,
**     among others:
**
**             dj1            dj2
**
**         2450123.7          0          (JD method)
**          2451545        -1421.3       (J2000 method)
**         2400000.5       50123.2       (MJD method)
**         2450123.5         0.2         (date & time method)
**
**     The JD method is the most natural and convenient to use in
**     cases where the loss of several decimal digits of resolution
**     is acceptable.  The J2000 and MJD methods are good compromises
**     between resolution and convenience.  The date & time method is
**     best matched to the algorithm used:  maximum accuracy (or, at
**     least, minimum noise) is delivered when the dj1 argument is for
**     0hrs UT1 on the day in question and the dj2 argument lies in the
**     range 0 to 1, or vice versa.
**
**  2) The algorithm is based on the IAU 1982 expression.  This is
**     always described as giving the GMST at 0 hours UT1.  In fact, it
**     gives the difference between the GMST and the UT, the steady
**     4-minutes-per-day drawing-ahead of ST with respect to UT.  When
**     whole days are ignored, the expression happens to equal the GMST
**     at 0 hours UT1 each day.
**
**  3) In this function, the entire UT1 (the sum of the two arguments
**     dj1 and dj2) is used directly as the argument for the standard
**     formula, the constant term of which is adjusted by 12 hours to
**     take account of the noon phasing of Julian Date.  The UT1 is then
**     added, but omitting whole days to conserve accuracy.
**
**  Called:
**     eraAnp       normalize angle into range 0 to 2pi
**
**  References:
**
**     Transactions of the International Astronomical Union,
**     XVIII B, 67 (1983).
**
**     Aoki et al., Astron.Astrophys., 105, 359-361 (1982).
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Coefficients of IAU 1982 GMST-UT1 model */
   double A = 24110.54841  -  ERFA_DAYSEC / 2.0;
   double B = 8640184.812866;
   double C = 0.093104;
   double D =  -6.2e-6;

/* The first constant, A, has to be adjusted by 12 hours because the */
/* UT1 is supplied as a Julian date, which begins at noon.           */

   double d1, d2, t, f, gmst;


/* Julian centuries since fundamental epoch. */
   if (dj1 < dj2) {
      d1 = dj1;
      d2 = dj2;
   } else {
      d1 = dj2;
      d2 = dj1;
   }
   t = (d1 + (d2 - ERFA_DJ00)) / ERFA_DJC;

/* Fractional part of JD(UT1), in seconds. */
   f = ERFA_DAYSEC * (fmod(d1, 1.0) + fmod(d2, 1.0));

/* GMST at this UT1. */
   gmst = eraAnp(ERFA_DS2R * ((A + (B + (C + D * t) * t) * t) + f));

   return gmst;

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
