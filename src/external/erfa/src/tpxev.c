#include "erfa.h"

int eraTpxev(double v[3], double v0[3], double *xi, double *eta)
/*
**  - - - - - - - - -
**   e r a T p x e v
**  - - - - - - - - -
**
**  In the tangent plane projection, given celestial direction cosines
**  for a star and the tangent point, solve for the star's rectangular
**  coordinates in the tangent plane.
**
**  Given:
**     v         double[3]  direction cosines of star (Note 4)
**     v0        double[3]  direction cosines of tangent point (Note 4)
**
**  Returned:
**     *xi,*eta  double     tangent plane coordinates of star
**
**  Returned (function value):
**               int        status: 0 = OK
**                                  1 = star too far from axis
**                                  2 = antistar on tangent plane
**                                  3 = antistar too far from axis
**
**  Notes:
**
**  1) The tangent plane projection is also called the "gnomonic
**     projection" and the "central projection".
**
**  2) The eta axis points due north in the adopted coordinate system.
**     If the direction cosines represent observed (RA,Dec), the tangent
**     plane coordinates (xi,eta) are conventionally called the
**     "standard coordinates".  If the direction cosines are with
**     respect to a right-handed triad, (xi,eta) are also right-handed.
**     The units of (xi,eta) are, effectively, radians at the tangent
**     point.
**
**  3) The method used is to extend the star vector to the tangent
**     plane and then rotate the triad so that (x,y) becomes (xi,eta).
**     Writing (a,b) for the celestial spherical coordinates of the
**     star, the sequence of rotations is (a+pi/2) around the z-axis
**     followed by (pi/2-b) around the x-axis.
**
**  4) If vector v0 is not of unit length, or if vector v is of zero
**     length, the results will be wrong.
**
**  5) If v0 points at a pole, the returned (xi,eta) will be based on
**     the arbitrary assumption that the longitude coordinate of the
**     tangent point is zero.
**
**  6) This function is a member of the following set:
**
**         spherical      vector         solve for
**
**         eraTpxes    > eraTpxev <       xi,eta
**         eraTpsts      eraTpstv          star
**         eraTpors      eraTporv         origin
**
**  References:
**
**     Calabretta M.R. & Greisen, E.W., 2002, "Representations of
**     celestial coordinates in FITS", Astron.Astrophys. 395, 1077
**
**     Green, R.M., "Spherical Astronomy", Cambridge University Press,
**     1987, Chapter 13.
**
**  This revision:   2018 January 2
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   const double TINY = 1e-6;
   int j;
   double x, y, z, x0, y0, z0, r2, r, w, d;


/* Star and tangent point. */
   x = v[0];
   y = v[1];
   z = v[2];
   x0 = v0[0];
   y0 = v0[1];
   z0 = v0[2];

/* Deal with polar case. */
   r2 = x0*x0 + y0*y0;
   r = sqrt(r2);
   if ( r == 0.0 ) {
      r = 1e-20;
      x0 = r;
   }

/* Reciprocal of star vector length to tangent plane. */
   w = x*x0 + y*y0;
   d = w + z*z0;

/* Check for error cases. */
   if ( d > TINY ) {
      j = 0;
   } else if ( d >= 0.0 ) {
      j = 1;
      d = TINY;
   } else if ( d > -TINY ) {
      j = 2;
      d = -TINY;
   } else {
      j = 3;
   }

/* Return the tangent plane coordinates (even in dubious cases). */
   d *= r;
   *xi = (y*x0 - x*y0) / d;
   *eta = (z*r2 - z0*w) / d;

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
