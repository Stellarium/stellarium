#include "erfa.h"
#include "erfam.h"

void eraLtpecl(double epj, double vec[3])
/*
**  - - - - - - - - - -
**   e r a L t p e c l
**  - - - - - - - - - -
**
**  Long-term precession of the ecliptic.
**
**  Given:
**     epj     double         Julian epoch (TT)
**
**  Returned:
**     vec     double[3]      ecliptic pole unit vector
**
**  Notes:
**
**  1) The returned vector is with respect to the J2000.0 mean equator
**     and equinox.
**
**  2) The Vondrak et al. (2011, 2012) 400 millennia precession model
**     agrees with the IAU 2006 precession at J2000.0 and stays within
**     100 microarcseconds during the 20th and 21st centuries.  It is
**     accurate to a few arcseconds throughout the historical period,
**     worsening to a few tenths of a degree at the end of the
**     +/- 200,000 year time span.
**
**  References:
**
**    Vondrak, J., Capitaine, N. and Wallace, P., 2011, New precession
**    expressions, valid for long time intervals, Astron.Astrophys. 534,
**    A22
**
**    Vondrak, J., Capitaine, N. and Wallace, P., 2012, New precession
**    expressions, valid for long time intervals (Corrigendum),
**    Astron.Astrophys. 541, C1
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Obliquity at J2000.0 (radians). */
   static const double eps0 = 84381.406 * ERFA_DAS2R;

/* Polynomial coefficients */
   enum { NPOL = 4 };
   static const double pqpol[2][NPOL] = {
      { 5851.607687,
          -0.1189000,
          -0.00028913,
           0.000000101},
      {-1600.886300,
           1.1689818,
          -0.00000020,
          -0.000000437}
   };

/* Periodic coefficients */
   static const double pqper[][5] = {
      { 708.15,-5486.751211,-684.661560,  667.666730,-5523.863691},
      {2309.00,  -17.127623,2446.283880,-2354.886252, -549.747450},
      {1620.00, -617.517403, 399.671049, -428.152441, -310.998056},
      { 492.20,  413.442940,-356.652376,  376.202861,  421.535876},
      {1183.00,   78.614193,-186.387003,  184.778874,  -36.776172},
      { 622.00, -180.732815,-316.800070,  335.321713, -145.278396},
      { 882.00,  -87.676083, 198.296701, -185.138669,  -34.744450},
      { 547.00,   46.140315, 101.135679, -120.972830,   22.885731}
   };
   static const int NPER = (int) ( sizeof pqper / 5 / sizeof (double) );

/* Miscellaneous */
   int i;
   double t, p, q, w, a, s, c;


/* Centuries since J2000. */
   t  = ( epj - 2000.0 ) / 100.0;

/* Initialize P_A and Q_A accumulators. */
   p = 0.0;
   q = 0.0;

/* Periodic terms. */
   w = ERFA_D2PI*t;
   for ( i = 0; i < NPER; i++ ) {
      a = w/pqper[i][0];
      s = sin(a);
      c = cos(a);
      p += c*pqper[i][1] + s*pqper[i][3];
      q += c*pqper[i][2] + s*pqper[i][4];
   }

/* Polynomial terms. */
   w = 1.0;
   for ( i = 0; i < NPOL; i++ ) {
      p += pqpol[0][i]*w;
      q += pqpol[1][i]*w;
      w *= t;
   }

/* P_A and Q_A (radians). */
   p *= ERFA_DAS2R;
   q *= ERFA_DAS2R;

/* Form the ecliptic pole vector. */
   w = 1.0 - p*p - q*q;
   w = w < 0.0 ? 0.0 : sqrt(w);
   s = sin(eps0);
   c = cos(eps0);
   vec[0] = p;
   vec[1] = - q*c - w*s;
   vec[2] = - q*s + w*c;

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
