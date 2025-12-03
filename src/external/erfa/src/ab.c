#include "erfa.h"
#include "erfam.h"

void eraAb(double pnat[3], double v[3], double s, double bm1,
           double ppr[3])
/*
**  - - - - - -
**   e r a A b
**  - - - - - -
**
**  Apply aberration to transform natural direction into proper
**  direction.
**
**  Given:
**    pnat    double[3]   natural direction to the source (unit vector)
**    v       double[3]   observer barycentric velocity in units of c
**    s       double      distance between the Sun and the observer (au)
**    bm1     double      sqrt(1-|v|^2): reciprocal of Lorenz factor
**
**  Returned:
**    ppr     double[3]   proper direction to source (unit vector)
**
**  Notes:
**
**  1) The algorithm is based on Expr. (7.40) in the Explanatory
**     Supplement (Urban & Seidelmann 2013), but with the following
**     changes:
**
**     o  Rigorous rather than approximate normalization is applied.
**
**     o  The gravitational potential term from Expr. (7) in
**        Klioner (2003) is added, taking into account only the Sun's
**        contribution.  This has a maximum effect of about
**        0.4 microarcsecond.
**
**  2) In almost all cases, the maximum accuracy will be limited by the
**     supplied velocity.  For example, if the ERFA eraEpv00 function is
**     used, errors of up to 5 microarcseconds could occur.
**
**  References:
**
**     Urban, S. & Seidelmann, P. K. (eds), Explanatory Supplement to
**     the Astronomical Almanac, 3rd ed., University Science Books
**     (2013).
**
**     Klioner, Sergei A., "A practical relativistic model for micro-
**     arcsecond astrometry in space", Astr. J. 125, 1580-1597 (2003).
**
**  Called:
**     eraPdp       scalar product of two p-vectors
**
**  This revision:   2021 February 24
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   int i;
   double pdv, w1, w2, r2, w, p[3], r;


   pdv = eraPdp(pnat, v);
   w1 = 1.0 + pdv/(1.0 + bm1);
   w2 = ERFA_SRS/s;
   r2 = 0.0;
   for (i = 0; i < 3; i++) {
      w = pnat[i]*bm1 + w1*v[i] + w2*(v[i] - pdv*pnat[i]);
      p[i] = w;
      r2 = r2 + w*w;
   }
   r = sqrt(r2);
   for (i = 0; i < 3; i++) {
      ppr[i] = p[i]/r;
   }

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
