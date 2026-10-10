#include "erfa.h"
#include "erfam.h"

double eraEect00(double date1, double date2)
/*
**  - - - - - - - - - -
**   e r a E e c t 0 0
**  - - - - - - - - - -
**
**  Equation of the equinoxes complementary terms, consistent with
**  IAU 2000 resolutions.
**
**  Given:
**     date1,date2  double   TT as a 2-part Julian Date (Note 1)
**
**  Returned (function value):
**                  double   complementary terms (Note 2)
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
**  2) The "complementary terms" are part of the equation of the
**     equinoxes (EE), classically the difference between apparent and
**     mean Sidereal Time:
**
**        GAST = GMST + EE
**
**     with:
**
**        EE = dpsi * cos(eps)
**
**     where dpsi is the nutation in longitude and eps is the obliquity
**     of date.  However, if the rotation of the Earth were constant in
**     an inertial frame the classical formulation would lead to
**     apparent irregularities in the UT1 timescale traceable to side-
**     effects of precession-nutation.  In order to eliminate these
**     effects from UT1, "complementary terms" were introduced in 1994
**     (IAU, 1994) and took effect from 1997 (Capitaine and Gontier,
**     1993):
**
**        GAST = GMST + CT + EE
**
**     By convention, the complementary terms are included as part of
**     the equation of the equinoxes rather than as part of the mean
**     Sidereal Time.  This slightly compromises the "geometrical"
**     interpretation of mean sidereal time but is otherwise
**     inconsequential.
**
**     The present function computes CT in the above expression,
**     compatible with IAU 2000 resolutions (Capitaine et al., 2002, and
**     IERS Conventions 2003).
**
**  Called:
**     eraFal03     mean anomaly of the Moon
**     eraFalp03    mean anomaly of the Sun
**     eraFaf03     mean argument of the latitude of the Moon
**     eraFad03     mean elongation of the Moon from the Sun
**     eraFaom03    mean longitude of the Moon's ascending node
**     eraFave03    mean longitude of Venus
**     eraFae03     mean longitude of Earth
**     eraFapa03    general accumulated precession in longitude
**
**  References:
**
**     Capitaine, N. & Gontier, A.-M., Astron.Astrophys., 275,
**     645-650 (1993)
**
**     Capitaine, N., Wallace, P.T. and McCarthy, D.D., "Expressions to
**     implement the IAU 2000 definition of UT1", Astron.Astrophys., 406,
**     1135-1149 (2003)
**
**     IAU Resolution C7, Recommendation 3 (1994)
**
**     McCarthy, D. D., Petit, G. (eds.), IERS Conventions (2003),
**     IERS Technical Note No. 32, BKG (2004)
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Time since J2000.0, in Julian centuries */
   double t;

/* Miscellaneous */
   int i, j;
   double a, s0, s1;

/* Fundamental arguments */
   double fa[14];

/* Returned value. */
   double eect;

/* ----------------------------------------- */
/* The series for the EE complementary terms */
/* ----------------------------------------- */

   typedef struct {
      int nfa[8];      /* coefficients of l,l',F,D,Om,LVe,LE,pA */
      double s, c;     /* sine and cosine coefficients */
   } TERM;

/* Terms of order t^0 */
   static const TERM e0[] = {

   /* 1-10 */
      {{ 0,  0,  0,  0,  1,  0,  0,  0}, 2640.96e-6, -0.39e-6 },
      {{ 0,  0,  0,  0,  2,  0,  0,  0},   63.52e-6, -0.02e-6 },
      {{ 0,  0,  2, -2,  3,  0,  0,  0},   11.75e-6,  0.01e-6 },
      {{ 0,  0,  2, -2,  1,  0,  0,  0},   11.21e-6,  0.01e-6 },
      {{ 0,  0,  2, -2,  2,  0,  0,  0},   -4.55e-6,  0.00e-6 },
      {{ 0,  0,  2,  0,  3,  0,  0,  0},    2.02e-6,  0.00e-6 },
      {{ 0,  0,  2,  0,  1,  0,  0,  0},    1.98e-6,  0.00e-6 },
      {{ 0,  0,  0,  0,  3,  0,  0,  0},   -1.72e-6,  0.00e-6 },
      {{ 0,  1,  0,  0,  1,  0,  0,  0},   -1.41e-6, -0.01e-6 },
      {{ 0,  1,  0,  0, -1,  0,  0,  0},   -1.26e-6, -0.01e-6 },

   /* 11-20 */
      {{ 1,  0,  0,  0, -1,  0,  0,  0},   -0.63e-6,  0.00e-6 },
      {{ 1,  0,  0,  0,  1,  0,  0,  0},   -0.63e-6,  0.00e-6 },
      {{ 0,  1,  2, -2,  3,  0,  0,  0},    0.46e-6,  0.00e-6 },
      {{ 0,  1,  2, -2,  1,  0,  0,  0},    0.45e-6,  0.00e-6 },
      {{ 0,  0,  4, -4,  4,  0,  0,  0},    0.36e-6,  0.00e-6 },
      {{ 0,  0,  1, -1,  1, -8, 12,  0},   -0.24e-6, -0.12e-6 },
      {{ 0,  0,  2,  0,  0,  0,  0,  0},    0.32e-6,  0.00e-6 },
      {{ 0,  0,  2,  0,  2,  0,  0,  0},    0.28e-6,  0.00e-6 },
      {{ 1,  0,  2,  0,  3,  0,  0,  0},    0.27e-6,  0.00e-6 },
      {{ 1,  0,  2,  0,  1,  0,  0,  0},    0.26e-6,  0.00e-6 },

   /* 21-30 */
      {{ 0,  0,  2, -2,  0,  0,  0,  0},   -0.21e-6,  0.00e-6 },
      {{ 0,  1, -2,  2, -3,  0,  0,  0},    0.19e-6,  0.00e-6 },
      {{ 0,  1, -2,  2, -1,  0,  0,  0},    0.18e-6,  0.00e-6 },
      {{ 0,  0,  0,  0,  0,  8,-13, -1},   -0.10e-6,  0.05e-6 },
      {{ 0,  0,  0,  2,  0,  0,  0,  0},    0.15e-6,  0.00e-6 },
      {{ 2,  0, -2,  0, -1,  0,  0,  0},   -0.14e-6,  0.00e-6 },
      {{ 1,  0,  0, -2,  1,  0,  0,  0},    0.14e-6,  0.00e-6 },
      {{ 0,  1,  2, -2,  2,  0,  0,  0},   -0.14e-6,  0.00e-6 },
      {{ 1,  0,  0, -2, -1,  0,  0,  0},    0.14e-6,  0.00e-6 },
      {{ 0,  0,  4, -2,  4,  0,  0,  0},    0.13e-6,  0.00e-6 },

   /* 31-33 */
      {{ 0,  0,  2, -2,  4,  0,  0,  0},   -0.11e-6,  0.00e-6 },
      {{ 1,  0, -2,  0, -3,  0,  0,  0},    0.11e-6,  0.00e-6 },
      {{ 1,  0, -2,  0, -1,  0,  0,  0},    0.11e-6,  0.00e-6 }
   };

/* Terms of order t^1 */
   static const TERM e1[] = {
      {{ 0,  0,  0,  0,  1,  0,  0,  0},    -0.87e-6,  0.00e-6 }
   };

/* Number of terms in the series */
   const int NE0 = (int) (sizeof e0 / sizeof (TERM));
   const int NE1 = (int) (sizeof e1 / sizeof (TERM));

/* ------------------------------------------------------------------ */

/* Interval between fundamental epoch J2000.0 and current date (JC). */
   t = ((date1 - ERFA_DJ00) + date2) / ERFA_DJC;

/* Fundamental Arguments (from IERS Conventions 2003) */

/* Mean anomaly of the Moon. */
   fa[0] = eraFal03(t);

/* Mean anomaly of the Sun. */
   fa[1] = eraFalp03(t);

/* Mean longitude of the Moon minus that of the ascending node. */
   fa[2] = eraFaf03(t);

/* Mean elongation of the Moon from the Sun. */
   fa[3] = eraFad03(t);

/* Mean longitude of the ascending node of the Moon. */
   fa[4] = eraFaom03(t);

/* Mean longitude of Venus. */
   fa[5] = eraFave03(t);

/* Mean longitude of Earth. */
   fa[6] = eraFae03(t);

/* General precession in longitude. */
   fa[7] = eraFapa03(t);

/* Evaluate the EE complementary terms. */
   s0 = 0.0;
   s1 = 0.0;

   for (i = NE0-1; i >= 0; i--) {
      a = 0.0;
      for (j = 0; j < 8; j++) {
         a += (double)(e0[i].nfa[j]) * fa[j];
      }
      s0 += e0[i].s * sin(a) + e0[i].c * cos(a);
   }

   for (i = NE1-1; i >= 0; i--) {
      a = 0.0;
      for (j = 0; j < 8; j++) {
         a += (double)(e1[i].nfa[j]) * fa[j];
      }
      s1 += e1[i].s * sin(a) + e1[i].c * cos(a);
   }

   eect = (s0 + s1 * t ) * ERFA_DAS2R;

   return eect;

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
