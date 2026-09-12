#include "erfa.h"

void eraAe2hd (double az, double el, double phi,
               double *ha, double *dec)
/*
**  - - - - - - - - -
**   e r a A e 2 h d
**  - - - - - - - - -
**
**  Horizon to equatorial coordinates:  transform azimuth and altitude
**  to hour angle and declination.
**
**  Given:
**     az       double       azimuth
**     el       double       altitude (informally, elevation)
**     phi      double       site latitude
**
**  Returned:
**     ha       double       hour angle (local)
**     dec      double       declination
**
**  Notes:
**
**  1)  All the arguments are angles in radians.
**
**  2)  The sign convention for azimuth is north zero, east +pi/2.
**
**  3)  HA is returned in the range +/-pi.  Declination is returned in
**      the range +/-pi/2.
**
**  4)  The latitude phi is pi/2 minus the angle between the Earth's
**      rotation axis and the adopted zenith.  In many applications it
**      will be sufficient to use the published geodetic latitude of the
**      site.  In very precise (sub-arcsecond) applications, phi can be
**      corrected for polar motion.
**
**  5)  The azimuth az must be with respect to the rotational north pole,
**      as opposed to the ITRS pole, and an azimuth with respect to north
**      on a map of the Earth's surface will need to be adjusted for
**      polar motion if sub-arcsecond accuracy is required.
**
**  6)  Should the user wish to work with respect to the astronomical
**      zenith rather than the geodetic zenith, phi will need to be
**      adjusted for deflection of the vertical (often tens of
**      arcseconds), and the zero point of ha will also be affected.
**
**  7)  The transformation is the same as Ve = Ry(phi-pi/2)*Rz(pi)*Vh,
**      where Ve and Vh are lefthanded unit vectors in the (ha,dec) and
**      (az,el) systems respectively and Rz and Ry are rotations about
**      first the z-axis and then the y-axis.  (n.b. Rz(pi) simply
**      reverses the signs of the x and y components.)  For efficiency,
**      the algorithm is written out rather than calling other utility
**      functions.  For applications that require even greater
**      efficiency, additional savings are possible if constant terms
**      such as functions of latitude are computed once and for all.
**
**  8)  Again for efficiency, no range checking of arguments is carried
**      out.
**
**  Last revision:   2017 September 12
**
**  ERFA release 2023-10-11
**
**  Copyright (C) 2023 IAU ERFA Board.  See notes at end.
*/
{
   double sa, ca, se, ce, sp, cp, x, y, z, r;


/* Useful trig functions. */
   sa = sin(az);
   ca = cos(az);
   se = sin(el);
   ce = cos(el);
   sp = sin(phi);
   cp = cos(phi);

/* HA,Dec unit vector. */
   x = - ca*ce*sp + se*cp;
   y = - sa*ce;
   z = ca*ce*cp + se*sp;

/* To spherical. */
   r = sqrt(x*x + y*y);
   *ha = (r != 0.0) ? atan2(y,x) : 0.0;
   *dec = atan2(z,r);

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
