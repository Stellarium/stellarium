#include "erfa.h"

void eraLteqec(double epj, double dr, double dd, double *dl, double *db)
/*
**  - - - - - - - - - -
**   e r a L t e q e c
**  - - - - - - - - - -
**
**  Transformation from ICRS RA,Dec to ecliptic coordinates (mean equinox
**  and ecliptic of date), using a long-term precession model.
**
**  Given:
**     epj     double     Julian epoch (TT)
**     dr,dd   double     ICRS right ascension and declination (radians)
**
**  Returned:
**     dl,db   double     ecliptic longitude and latitude (radians)
**
**  1) No assumptions are made about whether the coordinates represent
**     starlight and embody astrometric effects such as parallax or
**     aberration.
**
**  2) The transformation is approximately that from mean J2000.0 right
**     ascension and declination to ecliptic longitude and latitude
**     (mean equinox and ecliptic of date), with only frame bias (always
**     less than 25 mas) to disturb this classical picture.
**
**  3) The Vondrak et al. (2011, 2012) 400 millennia precession model
**     agrees with the IAU 2006 precession at J2000.0 and stays within
**     100 microarcseconds during the 20th and 21st centuries.  It is
**     accurate to a few arcseconds throughout the historical period,
**     worsening to a few tenths of a degree at the end of the
**     +/- 200,000 year time span.
**
**  Called:
**     eraS2c       spherical coordinates to unit vector
**     eraLtecm     J2000.0 to ecliptic rotation matrix, long term
**     eraRxp       product of r-matrix and p-vector
**     eraC2s       unit vector to spherical coordinates
**     eraAnp       normalize angle into range 0 to 2pi
**     eraAnpm      normalize angle into range +/- pi
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
**  This revision:  2023 March 18
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double rm[3][3], v1[3], v2[3], a, b;


/* Spherical to Cartesian. */
   eraS2c(dr, dd, v1);

/* Rotation matrix, ICRS equatorial to ecliptic. */
   eraLtecm(epj, rm);

/* The transformation from ICRS to ecliptic. */
   eraRxp(rm, v1, v2);

/* Cartesian to spherical. */
   eraC2s(v2, &a, &b);

/* Express in conventional ranges. */
   *dl = eraAnp(a);
   *db = eraAnpm(b);

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
