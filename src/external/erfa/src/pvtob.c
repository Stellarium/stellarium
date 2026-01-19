#include "erfa.h"
#include "erfam.h"

void eraPvtob(double elong, double phi, double hm,
              double xp, double yp, double sp, double theta,
              double pv[2][3])
/*
**  - - - - - - - - -
**   e r a P v t o b
**  - - - - - - - - -
**
**  Position and velocity of a terrestrial observing station.
**
**  Given:
**     elong   double       longitude (radians, east +ve, Note 1)
**     phi     double       latitude (geodetic, radians, Note 1)
**     hm      double       height above ref. ellipsoid (geodetic, m)
**     xp,yp   double       coordinates of the pole (radians, Note 2)
**     sp      double       the TIO locator s' (radians, Note 2)
**     theta   double       Earth rotation angle (radians, Note 3)
**
**  Returned:
**     pv      double[2][3] position/velocity vector (m, m/s, CIRS)
**
**  Notes:
**
**  1) The terrestrial coordinates are with respect to the ERFA_WGS84
**     reference ellipsoid.
**
**  2) xp and yp are the coordinates (in radians) of the Celestial
**     Intermediate Pole with respect to the International Terrestrial
**     Reference System (see IERS Conventions), measured along the
**     meridians 0 and 90 deg west respectively.  sp is the TIO locator
**     s', in radians, which positions the Terrestrial Intermediate
**     Origin on the equator.  For many applications, xp, yp and
**     (especially) sp can be set to zero.
**
**  3) If theta is Greenwich apparent sidereal time instead of Earth
**     rotation angle, the result is with respect to the true equator
**     and equinox of date, i.e. with the x-axis at the equinox rather
**     than the celestial intermediate origin.
**
**  4) The velocity units are meters per UT1 second, not per SI second.
**     This is unlikely to have any practical consequences in the modern
**     era.
**
**  5) No validation is performed on the arguments.  Error cases that
**     could lead to arithmetic exceptions are trapped by the eraGd2gc
**     function, and the result set to zeros.
**
**  References:
**
**     McCarthy, D. D., Petit, G. (eds.), IERS Conventions (2003),
**     IERS Technical Note No. 32, BKG (2004)
**
**     Urban, S. & Seidelmann, P. K. (eds), Explanatory Supplement to
**     the Astronomical Almanac, 3rd ed., University Science Books
**     (2013), Section 7.4.3.3.
**
**  Called:
**     eraGd2gc     geodetic to geocentric transformation
**     eraPom00     polar motion matrix
**     eraTrxp      product of transpose of r-matrix and p-vector
**
**  This revision:   2021 February 24
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Earth rotation rate in radians per UT1 second */
   const double OM = 1.00273781191135448 * ERFA_D2PI / ERFA_DAYSEC;

   double xyzm[3], rpm[3][3], xyz[3], x, y, z, s, c;


/* Geodetic to geocentric transformation (ERFA_WGS84). */
   (void) eraGd2gc(1, elong, phi, hm, xyzm);

/* Polar motion and TIO position. */
   eraPom00(xp, yp, sp, rpm);
   eraTrxp(rpm, xyzm, xyz);
   x = xyz[0];
   y = xyz[1];
   z = xyz[2];

/* Functions of ERA. */
   s = sin(theta);
   c = cos(theta);

/* Position. */
   pv[0][0] = c*x - s*y;
   pv[0][1] = s*x + c*y;
   pv[0][2] = z;

/* Velocity. */
   pv[1][0] = OM * ( -s*x - c*y );
   pv[1][1] = OM * (  c*x - s*y );
   pv[1][2] = 0.0;

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
