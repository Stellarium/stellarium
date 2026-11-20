#include "erfa.h"

void eraTpstv(double xi, double eta, double v0[3], double v[3])
/*
**  - - - - - - - - -
**   e r a T p s t v
**  - - - - - - - - -
**
**  In the tangent plane projection, given the star's rectangular
**  coordinates and the direction cosines of the tangent point, solve
**  for the direction cosines of the star.
**
**  Given:
**     xi,eta  double     rectangular coordinates of star image (Note 2)
**     v0      double[3]  tangent point's direction cosines
**
**  Returned:
**     v       double[3]  star's direction cosines
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
**  3) The method used is to complete the star vector in the (xi,eta)
**     based triad and normalize it, then rotate the triad to put the
**     tangent point at the pole with the x-axis aligned to zero
**     longitude.  Writing (a0,b0) for the celestial spherical
**     coordinates of the tangent point, the sequence of rotations is
**     (b-pi/2) around the x-axis followed by (-a-pi/2) around the
**     z-axis.
**
**  4) If vector v0 is not of unit length, the returned vector v will
**     be wrong.
**
**  5) If vector v0 points at a pole, the returned vector v will be
**     based on the arbitrary assumption that the longitude coordinate
**     of the tangent point is zero.
**
**  6) This function is a member of the following set:
**
**         spherical      vector         solve for
**
**         eraTpxes      eraTpxev         xi,eta
**         eraTpsts    > eraTpstv <        star
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
   double x, y, z, f, r;


/* Tangent point. */
   x = v0[0];
   y = v0[1];
   z = v0[2];

/* Deal with polar case. */
   r = sqrt(x*x + y*y);
   if ( r == 0.0 ) {
      r = 1e-20;
      x = r;
   }

/* Star vector length to tangent plane. */
   f = sqrt(1.0 + xi*xi + eta*eta);

/* Apply the transformation and normalize. */
   v[0] = (x - (xi*y + eta*x*z) / r) / f;
   v[1] = (y + (xi*x - eta*y*z) / r) / f;
   v[2] = (z + eta*r) / f;

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
