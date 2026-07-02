#include "erfa.h"
#include "erfam.h"

void eraNut80(double date1, double date2, double *dpsi, double *deps)
/*
**  - - - - - - - - -
**   e r a N u t 8 0
**  - - - - - - - - -
**
**  Nutation, IAU 1980 model.
**
**  Given:
**     date1,date2   double    TT as a 2-part Julian Date (Note 1)
**
**  Returned:
**     dpsi          double    nutation in longitude (radians)
**     deps          double    nutation in obliquity (radians)
**
**  Notes:
**
**  1) The TT date date1+date2 is a Julian Date, apportioned in any
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
**  2) The nutation components are with respect to the ecliptic of
**     date.
**
**  Called:
**     eraAnpm      normalize angle into range +/- pi
**
**  Reference:
**
**     Explanatory Supplement to the Astronomical Almanac,
**     P. Kenneth Seidelmann (ed), University Science Books (1992),
**     Section 3.222 (p111).
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double t, el, elp, f, d, om, dp, de, arg, s, c;
   int j;

/* Units of 0.1 milliarcsecond to radians */
   const double U2R = ERFA_DAS2R / 1e4;

/* ------------------------------------------------ */
/* Table of multiples of arguments and coefficients */
/* ------------------------------------------------ */

/* The units for the sine and cosine coefficients are 0.1 mas and */
/* the same per Julian century */

   static const struct {
      int nl,nlp,nf,nd,nom; /* coefficients of l,l',F,D,Om */
      double sp,spt;        /* longitude sine, 1 and t coefficients */
      double ce,cet;        /* obliquity cosine, 1 and t coefficients */
   } x[] = {

   /* 1-10 */
      {  0,  0,  0,  0,  1, -171996.0, -174.2,  92025.0,    8.9 },
      {  0,  0,  0,  0,  2,    2062.0,    0.2,   -895.0,    0.5 },
      { -2,  0,  2,  0,  1,      46.0,    0.0,    -24.0,    0.0 },
      {  2,  0, -2,  0,  0,      11.0,    0.0,      0.0,    0.0 },
      { -2,  0,  2,  0,  2,      -3.0,    0.0,      1.0,    0.0 },
      {  1, -1,  0, -1,  0,      -3.0,    0.0,      0.0,    0.0 },
      {  0, -2,  2, -2,  1,      -2.0,    0.0,      1.0,    0.0 },
      {  2,  0, -2,  0,  1,       1.0,    0.0,      0.0,    0.0 },
      {  0,  0,  2, -2,  2,  -13187.0,   -1.6,   5736.0,   -3.1 },
      {  0,  1,  0,  0,  0,    1426.0,   -3.4,     54.0,   -0.1 },

   /* 11-20 */
      {  0,  1,  2, -2,  2,    -517.0,    1.2,    224.0,   -0.6 },
      {  0, -1,  2, -2,  2,     217.0,   -0.5,    -95.0,    0.3 },
      {  0,  0,  2, -2,  1,     129.0,    0.1,    -70.0,    0.0 },
      {  2,  0,  0, -2,  0,      48.0,    0.0,      1.0,    0.0 },
      {  0,  0,  2, -2,  0,     -22.0,    0.0,      0.0,    0.0 },
      {  0,  2,  0,  0,  0,      17.0,   -0.1,      0.0,    0.0 },
      {  0,  1,  0,  0,  1,     -15.0,    0.0,      9.0,    0.0 },
      {  0,  2,  2, -2,  2,     -16.0,    0.1,      7.0,    0.0 },
      {  0, -1,  0,  0,  1,     -12.0,    0.0,      6.0,    0.0 },
      { -2,  0,  0,  2,  1,      -6.0,    0.0,      3.0,    0.0 },

   /* 21-30 */
      {  0, -1,  2, -2,  1,      -5.0,    0.0,      3.0,    0.0 },
      {  2,  0,  0, -2,  1,       4.0,    0.0,     -2.0,    0.0 },
      {  0,  1,  2, -2,  1,       4.0,    0.0,     -2.0,    0.0 },
      {  1,  0,  0, -1,  0,      -4.0,    0.0,      0.0,    0.0 },
      {  2,  1,  0, -2,  0,       1.0,    0.0,      0.0,    0.0 },
      {  0,  0, -2,  2,  1,       1.0,    0.0,      0.0,    0.0 },
      {  0,  1, -2,  2,  0,      -1.0,    0.0,      0.0,    0.0 },
      {  0,  1,  0,  0,  2,       1.0,    0.0,      0.0,    0.0 },
      { -1,  0,  0,  1,  1,       1.0,    0.0,      0.0,    0.0 },
      {  0,  1,  2, -2,  0,      -1.0,    0.0,      0.0,    0.0 },

   /* 31-40 */
      {  0,  0,  2,  0,  2,   -2274.0,   -0.2,    977.0,   -0.5 },
      {  1,  0,  0,  0,  0,     712.0,    0.1,     -7.0,    0.0 },
      {  0,  0,  2,  0,  1,    -386.0,   -0.4,    200.0,    0.0 },
      {  1,  0,  2,  0,  2,    -301.0,    0.0,    129.0,   -0.1 },
      {  1,  0,  0, -2,  0,    -158.0,    0.0,     -1.0,    0.0 },
      { -1,  0,  2,  0,  2,     123.0,    0.0,    -53.0,    0.0 },
      {  0,  0,  0,  2,  0,      63.0,    0.0,     -2.0,    0.0 },
      {  1,  0,  0,  0,  1,      63.0,    0.1,    -33.0,    0.0 },
      { -1,  0,  0,  0,  1,     -58.0,   -0.1,     32.0,    0.0 },
      { -1,  0,  2,  2,  2,     -59.0,    0.0,     26.0,    0.0 },

   /* 41-50 */
      {  1,  0,  2,  0,  1,     -51.0,    0.0,     27.0,    0.0 },
      {  0,  0,  2,  2,  2,     -38.0,    0.0,     16.0,    0.0 },
      {  2,  0,  0,  0,  0,      29.0,    0.0,     -1.0,    0.0 },
      {  1,  0,  2, -2,  2,      29.0,    0.0,    -12.0,    0.0 },
      {  2,  0,  2,  0,  2,     -31.0,    0.0,     13.0,    0.0 },
      {  0,  0,  2,  0,  0,      26.0,    0.0,     -1.0,    0.0 },
      { -1,  0,  2,  0,  1,      21.0,    0.0,    -10.0,    0.0 },
      { -1,  0,  0,  2,  1,      16.0,    0.0,     -8.0,    0.0 },
      {  1,  0,  0, -2,  1,     -13.0,    0.0,      7.0,    0.0 },
      { -1,  0,  2,  2,  1,     -10.0,    0.0,      5.0,    0.0 },

   /* 51-60 */
      {  1,  1,  0, -2,  0,      -7.0,    0.0,      0.0,    0.0 },
      {  0,  1,  2,  0,  2,       7.0,    0.0,     -3.0,    0.0 },
      {  0, -1,  2,  0,  2,      -7.0,    0.0,      3.0,    0.0 },
      {  1,  0,  2,  2,  2,      -8.0,    0.0,      3.0,    0.0 },
      {  1,  0,  0,  2,  0,       6.0,    0.0,      0.0,    0.0 },
      {  2,  0,  2, -2,  2,       6.0,    0.0,     -3.0,    0.0 },
      {  0,  0,  0,  2,  1,      -6.0,    0.0,      3.0,    0.0 },
      {  0,  0,  2,  2,  1,      -7.0,    0.0,      3.0,    0.0 },
      {  1,  0,  2, -2,  1,       6.0,    0.0,     -3.0,    0.0 },
      {  0,  0,  0, -2,  1,      -5.0,    0.0,      3.0,    0.0 },

   /* 61-70 */
      {  1, -1,  0,  0,  0,       5.0,    0.0,      0.0,    0.0 },
      {  2,  0,  2,  0,  1,      -5.0,    0.0,      3.0,    0.0 },
      {  0,  1,  0, -2,  0,      -4.0,    0.0,      0.0,    0.0 },
      {  1,  0, -2,  0,  0,       4.0,    0.0,      0.0,    0.0 },
      {  0,  0,  0,  1,  0,      -4.0,    0.0,      0.0,    0.0 },
      {  1,  1,  0,  0,  0,      -3.0,    0.0,      0.0,    0.0 },
      {  1,  0,  2,  0,  0,       3.0,    0.0,      0.0,    0.0 },
      {  1, -1,  2,  0,  2,      -3.0,    0.0,      1.0,    0.0 },
      { -1, -1,  2,  2,  2,      -3.0,    0.0,      1.0,    0.0 },
      { -2,  0,  0,  0,  1,      -2.0,    0.0,      1.0,    0.0 },

   /* 71-80 */
      {  3,  0,  2,  0,  2,      -3.0,    0.0,      1.0,    0.0 },
      {  0, -1,  2,  2,  2,      -3.0,    0.0,      1.0,    0.0 },
      {  1,  1,  2,  0,  2,       2.0,    0.0,     -1.0,    0.0 },
      { -1,  0,  2, -2,  1,      -2.0,    0.0,      1.0,    0.0 },
      {  2,  0,  0,  0,  1,       2.0,    0.0,     -1.0,    0.0 },
      {  1,  0,  0,  0,  2,      -2.0,    0.0,      1.0,    0.0 },
      {  3,  0,  0,  0,  0,       2.0,    0.0,      0.0,    0.0 },
      {  0,  0,  2,  1,  2,       2.0,    0.0,     -1.0,    0.0 },
      { -1,  0,  0,  0,  2,       1.0,    0.0,     -1.0,    0.0 },
      {  1,  0,  0, -4,  0,      -1.0,    0.0,      0.0,    0.0 },

   /* 81-90 */
      { -2,  0,  2,  2,  2,       1.0,    0.0,     -1.0,    0.0 },
      { -1,  0,  2,  4,  2,      -2.0,    0.0,      1.0,    0.0 },
      {  2,  0,  0, -4,  0,      -1.0,    0.0,      0.0,    0.0 },
      {  1,  1,  2, -2,  2,       1.0,    0.0,     -1.0,    0.0 },
      {  1,  0,  2,  2,  1,      -1.0,    0.0,      1.0,    0.0 },
      { -2,  0,  2,  4,  2,      -1.0,    0.0,      1.0,    0.0 },
      { -1,  0,  4,  0,  2,       1.0,    0.0,      0.0,    0.0 },
      {  1, -1,  0, -2,  0,       1.0,    0.0,      0.0,    0.0 },
      {  2,  0,  2, -2,  1,       1.0,    0.0,     -1.0,    0.0 },
      {  2,  0,  2,  2,  2,      -1.0,    0.0,      0.0,    0.0 },

   /* 91-100 */
      {  1,  0,  0,  2,  1,      -1.0,    0.0,      0.0,    0.0 },
      {  0,  0,  4, -2,  2,       1.0,    0.0,      0.0,    0.0 },
      {  3,  0,  2, -2,  2,       1.0,    0.0,      0.0,    0.0 },
      {  1,  0,  2, -2,  0,      -1.0,    0.0,      0.0,    0.0 },
      {  0,  1,  2,  0,  1,       1.0,    0.0,      0.0,    0.0 },
      { -1, -1,  0,  2,  1,       1.0,    0.0,      0.0,    0.0 },
      {  0,  0, -2,  0,  1,      -1.0,    0.0,      0.0,    0.0 },
      {  0,  0,  2, -1,  2,      -1.0,    0.0,      0.0,    0.0 },
      {  0,  1,  0,  2,  0,      -1.0,    0.0,      0.0,    0.0 },
      {  1,  0, -2, -2,  0,      -1.0,    0.0,      0.0,    0.0 },

   /* 101-106 */
      {  0, -1,  2,  0,  1,      -1.0,    0.0,      0.0,    0.0 },
      {  1,  1,  0, -2,  1,      -1.0,    0.0,      0.0,    0.0 },
      {  1,  0, -2,  2,  0,      -1.0,    0.0,      0.0,    0.0 },
      {  2,  0,  0,  2,  0,       1.0,    0.0,      0.0,    0.0 },
      {  0,  0,  2,  4,  2,      -1.0,    0.0,      0.0,    0.0 },
      {  0,  1,  0,  1,  0,       1.0,    0.0,      0.0,    0.0 }
   };

/* Number of terms in the series */
   const int NT = (int) (sizeof x / sizeof x[0]);

/* ------------------------------------------------------------------ */

/* Interval between fundamental epoch J2000.0 and given date (JC). */
   t = ((date1 - ERFA_DJ00) + date2) / ERFA_DJC;

/* --------------------- */
/* Fundamental arguments */
/* --------------------- */

/* Mean longitude of Moon minus mean longitude of Moon's perigee. */
   el = eraAnpm(
        (485866.733 + (715922.633 + (31.310 + 0.064 * t) * t) * t)
        * ERFA_DAS2R + fmod(1325.0 * t, 1.0) * ERFA_D2PI);

/* Mean longitude of Sun minus mean longitude of Sun's perigee. */
   elp = eraAnpm(
         (1287099.804 + (1292581.224 + (-0.577 - 0.012 * t) * t) * t)
         * ERFA_DAS2R + fmod(99.0 * t, 1.0) * ERFA_D2PI);

/* Mean longitude of Moon minus mean longitude of Moon's node. */
   f = eraAnpm(
       (335778.877 + (295263.137 + (-13.257 + 0.011 * t) * t) * t)
       * ERFA_DAS2R + fmod(1342.0 * t, 1.0) * ERFA_D2PI);

/* Mean elongation of Moon from Sun. */
   d = eraAnpm(
       (1072261.307 + (1105601.328 + (-6.891 + 0.019 * t) * t) * t)
       * ERFA_DAS2R + fmod(1236.0 * t, 1.0) * ERFA_D2PI);

/* Longitude of the mean ascending node of the lunar orbit on the */
/* ecliptic, measured from the mean equinox of date. */
   om = eraAnpm(
        (450160.280 + (-482890.539 + (7.455 + 0.008 * t) * t) * t)
        * ERFA_DAS2R + fmod(-5.0 * t, 1.0) * ERFA_D2PI);

/* --------------- */
/* Nutation series */
/* --------------- */

/* Initialize nutation components. */
   dp = 0.0;
   de = 0.0;

/* Sum the nutation terms, ending with the biggest. */
   for (j = NT-1; j >= 0; j--) {

   /* Form argument for current term. */
      arg = (double)x[j].nl  * el
          + (double)x[j].nlp * elp
          + (double)x[j].nf  * f
          + (double)x[j].nd  * d
          + (double)x[j].nom * om;

   /* Accumulate current nutation term. */
      s = x[j].sp + x[j].spt * t;
      c = x[j].ce + x[j].cet * t;
      if (s != 0.0) dp += s * sin(arg);
      if (c != 0.0) de += c * cos(arg);
   }

/* Convert results from 0.1 mas units to radians. */
   *dpsi = dp * U2R;
   *deps = de * U2R;

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
