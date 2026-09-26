#include "erfa.h"

double eraEors(double rnpb[3][3], double s)
/*
**  - - - - - - - -
**   e r a E o r s
**  - - - - - - - -
**
**  Equation of the origins, given the classical NPB matrix and the
**  quantity s.
**
**  Given:
**     rnpb  double[3][3]  classical nutation x precession x bias matrix
**     s     double        the quantity s (the CIO locator) in radians
**
**  Returned (function value):
**           double        the equation of the origins in radians
**
**  Notes:
**
**  1)  The equation of the origins is the distance between the true
**      equinox and the celestial intermediate origin and, equivalently,
**      the difference between Earth rotation angle and Greenwich
**      apparent sidereal time (ERA-GST).  It comprises the precession
**      (since J2000.0) in right ascension plus the equation of the
**      equinoxes (including the small correction terms).
**
**  2)  The algorithm is from Wallace & Capitaine (2006).
**
** References:
**
**     Capitaine, N. & Wallace, P.T., 2006, Astron.Astrophys. 450, 855
**
**     Wallace, P. & Capitaine, N., 2006, Astron.Astrophys. 459, 981
**
**  This revision:  2023 May 6
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double x, ax, xs, ys, zs, p, q, eo;


/* Evaluate Wallace & Capitaine (2006) expression (16). */
   x = rnpb[2][0];
   ax = x / (1.0 + rnpb[2][2]);
   xs = 1.0 - ax * x;
   ys = -ax * rnpb[2][1];
   zs = -x;
   p = rnpb[0][0] * xs + rnpb[0][1] * ys + rnpb[0][2] * zs;
   q = rnpb[1][0] * xs + rnpb[1][1] * ys + rnpb[1][2] * zs;
   eo = ((p != 0.0) || (q != 0.0)) ? s - atan2(q, p) : s;

   return eo;

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
