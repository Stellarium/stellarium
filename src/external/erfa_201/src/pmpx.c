#include "erfa.h"
#include "erfam.h"

void eraPmpx(double rc, double dc, double pr, double pd,
             double px, double rv, double pmt, double pob[3],
             double pco[3])
/*
**  - - - - - - - -
**   e r a P m p x
**  - - - - - - - -
**
**  Proper motion and parallax.
**
**  Given:
**     rc,dc  double     ICRS RA,Dec at catalog epoch (radians)
**     pr     double     RA proper motion (radians/year, Note 1)
**     pd     double     Dec proper motion (radians/year)
**     px     double     parallax (arcsec)
**     rv     double     radial velocity (km/s, +ve if receding)
**     pmt    double     proper motion time interval (SSB, Julian years)
**     pob    double[3]  SSB to observer vector (au)
**
**  Returned:
**     pco    double[3]  coordinate direction (BCRS unit vector)
**
**  Notes:
**
**  1) The proper motion in RA is dRA/dt rather than cos(Dec)*dRA/dt.
**
**  2) The proper motion time interval is for when the starlight
**     reaches the solar system barycenter.
**
**  3) To avoid the need for iteration, the Roemer effect (i.e. the
**     small annual modulation of the proper motion coming from the
**     changing light time) is applied approximately, using the
**     direction of the star at the catalog epoch.
**
**  References:
**
**     1984 Astronomical Almanac, pp B39-B41.
**
**     Urban, S. & Seidelmann, P. K. (eds), Explanatory Supplement to
**     the Astronomical Almanac, 3rd ed., University Science Books
**     (2013), Section 7.2.
**
**  Called:
**     eraPdp       scalar product of two p-vectors
**     eraPn        decompose p-vector into modulus and direction
**
**  This revision:   2021 April 3
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Km/s to au/year */
   const double VF = ERFA_DAYSEC*ERFA_DJM/ERFA_DAU;

/* Light time for 1 au, Julian years */
   const double AULTY = ERFA_AULT/ERFA_DAYSEC/ERFA_DJY;

   int i;
   double sr, cr, sd, cd, x, y, z, p[3], dt, pxr, w, pdz, pm[3];


/* Spherical coordinates to unit vector (and useful functions). */
   sr = sin(rc);
   cr = cos(rc);
   sd = sin(dc);
   cd = cos(dc);
   p[0] = x = cr*cd;
   p[1] = y = sr*cd;
   p[2] = z = sd;

/* Proper motion time interval (y) including Roemer effect. */
   dt = pmt + eraPdp(p,pob)*AULTY;

/* Space motion (radians per year). */
   pxr = px * ERFA_DAS2R;
   w = VF * rv * pxr;
   pdz = pd * z;
   pm[0] = - pr*y - pdz*cr + w*x;
   pm[1] =   pr*x - pdz*sr + w*y;
   pm[2] =   pd*cd + w*z;

/* Coordinate direction of star (unit vector, BCRS). */
   for (i = 0; i < 3; i++) {
      p[i] += dt*pm[i] - pxr*pob[i];
   }
   eraPn(p, &w, pco);

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
