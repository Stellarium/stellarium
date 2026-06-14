#include "erfa.h"
#include "erfam.h"

void eraHd2ae (double ha, double dec, double phi,
               double *az, double *el)
/*
**  - - - - - - - - -
**   e r a H d 2 a e
**  - - - - - - - - -
**
**  Equatorial to horizon coordinates:  transform hour angle and
**  declination to azimuth and altitude.
**
**  Given:
**     ha       double       hour angle (local)
**     dec      double       declination
**     phi      double       site latitude
**
**  Returned:
**     *az      double       azimuth
**     *el      double       altitude (informally, elevation)
**
**  Notes:
**
**  1)  All the arguments are angles in radians.
**
**  2)  Azimuth is returned in the range 0-2pi;  north is zero, and east
**      is +pi/2.  Altitude is returned in the range +/- pi/2.
**
**  3)  The latitude phi is pi/2 minus the angle between the Earth's
**      rotation axis and the adopted zenith.  In many applications it
**      will be sufficient to use the published geodetic latitude of the
**      site.  In very precise (sub-arcsecond) applications, phi can be
**      corrected for polar motion.
**
**  4)  The returned azimuth az is with respect to the rotational north
**      pole, as opposed to the ITRS pole, and for sub-arcsecond
**      accuracy will need to be adjusted for polar motion if it is to
**      be with respect to north on a map of the Earth's surface.
**
**  5)  Should the user wish to work with respect to the astronomical
**      zenith rather than the geodetic zenith, phi will need to be
**      adjusted for deflection of the vertical (often tens of
**      arcseconds), and the zero point of the hour angle ha will also
**      be affected.
**
**  6)  The transformation is the same as Vh = Rz(pi)*Ry(pi/2-phi)*Ve,
**      where Vh and Ve are lefthanded unit vectors in the (az,el) and
**      (ha,dec) systems respectively and Ry and Rz are rotations about
**      first the y-axis and then the z-axis.  (n.b. Rz(pi) simply
**      reverses the signs of the x and y components.)  For efficiency,
**      the algorithm is written out rather than calling other utility
**      functions.  For applications that require even greater
**      efficiency, additional savings are possible if constant terms
**      such as functions of latitude are computed once and for all.
**
**  7)  Again for efficiency, no range checking of arguments is carried
**      out.
**
**  Last revision:   2021 February 24
**
**  ERFA release 2023-10-11
**
**  Copyright (C) 2023 IAU ERFA Board.  See notes at end.
*/
{
   double sh, ch, sd, cd, sp, cp, x, y, z, r, a;


/* Useful trig functions. */
   sh = sin(ha);
   ch = cos(ha);
   sd = sin(dec);
   cd = cos(dec);
   sp = sin(phi);
   cp = cos(phi);

/* Az,Alt unit vector. */
   x = - ch*cd*sp + sd*cp;
   y = - sh*cd;
   z = ch*cd*cp + sd*sp;

/* To spherical. */
   r = sqrt(x*x + y*y);
   a = (r != 0.0) ? atan2(y,x) : 0.0;
   *az = (a < 0.0) ? a+ERFA_D2PI : a;
   *el = atan2(z,r);

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
