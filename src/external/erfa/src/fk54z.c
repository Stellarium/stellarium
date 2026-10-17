#include "erfa.h"

void eraFk54z(double r2000, double d2000, double bepoch,
              double *r1950, double *d1950,
              double *dr1950, double *dd1950)
/*
**  - - - - - - - - -
**   e r a F k 5 4 z
**  - - - - - - - - -
**
**  Convert a J2000.0 FK5 star position to B1950.0 FK4, assuming zero
**  proper motion in FK5 and parallax.
**
**  Given:
**     r2000,d2000    double   J2000.0 FK5 RA,Dec (rad)
**     bepoch         double   Besselian epoch (e.g. 1950.0)
**
**  Returned:
**     r1950,d1950    double   B1950.0 FK4 RA,Dec (rad) at epoch BEPOCH
**     dr1950,dd1950  double   B1950.0 FK4 proper motions (rad/trop.yr)
**
**  Notes:
**
**  1) In contrast to the eraFk524 function, here the FK5 proper
**     motions, the parallax and the radial velocity are presumed zero.
**
**  2) This function converts a star position from the IAU 1976 FK5
**    (Fricke) system to the former FK4 (Bessel-Newcomb) system, for
**     cases such as distant radio sources where it is presumed there is
**     zero parallax and no proper motion.  Because of the E-terms of
**     aberration, such objects have (in general) non-zero proper motion
**     in FK4, and the present function returns those fictitious proper
**     motions.
**
**  3) Conversion from J2000.0 FK5 to B1950.0 FK4 only is provided for.
**     Conversions involving other equinoxes would require additional
**     treatment for precession.
**
**  4) The position returned by this function is in the B1950.0 FK4
**     reference system but at Besselian epoch bepoch.  For comparison
**     with catalogs the bepoch argument will frequently be 1950.0. (In
**     this context the distinction between Besselian and Julian epoch
**     is insignificant.)
**
**  5) The RA component of the returned (fictitious) proper motion is
**     dRA/dt rather than cos(Dec)*dRA/dt.
**
**  Called:
**     eraAnp       normalize angle into range 0 to 2pi
**     eraC2s       p-vector to spherical
**     eraFk524     FK4 to FK5
**     eraS2c       spherical to p-vector
**
**  This revision:   2023 March 5
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double r, d, pr, pd, px, rv, p[3], w, v[3];
   int i;


/* FK5 equinox J2000.0 to FK4 equinox B1950.0. */
   eraFk524(r2000, d2000, 0.0, 0.0, 0.0, 0.0,
            &r, &d, &pr, &pd, &px, &rv);

/* Spherical to Cartesian. */
   eraS2c(r, d, p);

/* Fictitious proper motion (radians per year). */
   v[0] = - pr*p[1] - pd*cos(r)*sin(d);
   v[1] =   pr*p[0] - pd*sin(r)*sin(d);
   v[2] =             pd*cos(d);

/* Apply the motion. */
   w = bepoch - 1950.0;
   for ( i = 0; i < 3; i++ ) {
      p[i] += w*v[i];
   }

/* Cartesian to spherical. */
   eraC2s(p, &w, d1950);
   *r1950 = eraAnp(w);

/* Fictitious proper motion. */
   *dr1950 = pr;
   *dd1950 = pd;

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
