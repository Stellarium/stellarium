#include "erfa.h"

void eraEqec06(double date1, double date2, double dr, double dd,
               double *dl, double *db)
/*
**  - - - - - - - - - -
**   e r a E q e c 0 6
**  - - - - - - - - - -
**
**  Transformation from ICRS equatorial coordinates to ecliptic
**  coordinates (mean equinox and ecliptic of date) using IAU 2006
**  precession model.
**
**  Given:
**     date1,date2 double TT as a 2-part Julian date (Note 1)
**     dr,dd       double ICRS right ascension and declination (radians)
**
**  Returned:
**     dl,db       double ecliptic longitude and latitude (radians)
**
**  1) The TT date date1+date2 is a Julian Date, apportioned in any
**     convenient way between the two arguments.  For example,
**     JD(TT)=2450123.7 could be expressed in any of these ways,
**     among others:
**
**            date1          date2
**
**         2450123.7           0.0       (JD method)
**         2451545.0       -1421.3       (J2000 method)
**         2400000.5       50123.2       (MJD method)
**         2450123.5           0.2       (date & time method)
**
**     The JD method is the most natural and convenient to use in
**     cases where the loss of several decimal digits of resolution
**     is acceptable.  The J2000 method is best matched to the way
**     the argument is handled internally and will deliver the
**     optimum resolution.  The MJD method and the date & time methods
**     are both good compromises between resolution and convenience.
**
**  2) No assumptions are made about whether the coordinates represent
**     starlight and embody astrometric effects such as parallax or
**     aberration.
**
**  3) The transformation is approximately that from mean J2000.0 right
**     ascension and declination to ecliptic longitude and latitude
**     (mean equinox and ecliptic of date), with only frame bias (always
**     less than 25 mas) to disturb this classical picture.
**
**  Called:
**     eraS2c       spherical coordinates to unit vector
**     eraEcm06     J2000.0 to ecliptic rotation matrix, IAU 2006
**     eraRxp       product of r-matrix and p-vector
**     eraC2s       unit vector to spherical coordinates
**     eraAnp       normalize angle into range 0 to 2pi
**     eraAnpm      normalize angle into range +/- pi
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double rm[3][3], v1[3], v2[3], a, b;


/* Spherical to Cartesian. */
   eraS2c(dr, dd, v1);

/* Rotation matrix, ICRS equatorial to ecliptic. */
   eraEcm06(date1, date2, rm);

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
