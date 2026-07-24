#include "erfa.h"
#include "erfam.h"

int eraGd2gce ( double a, double f, double elong, double phi,
                double height, double xyz[3] )
/*
**  - - - - - - - - - -
**   e r a G d 2 g c e
**  - - - - - - - - - -
**
**  Transform geodetic coordinates to geocentric for a reference
**  ellipsoid of specified form.
**
**  Given:
**     a       double     equatorial radius (Notes 1,3,4)
**     f       double     flattening (Notes 2,4)
**     elong   double     longitude (radians, east +ve, Note 4)
**     phi     double     latitude (geodetic, radians, Note 4)
**     height  double     height above ellipsoid (geodetic, Notes 3,4)
**
**  Returned:
**     xyz     double[3]  geocentric vector (Note 3)
**
**  Returned (function value):
**             int        status:  0 = OK
**                                -1 = illegal case (Note 4)
**  Notes:
**
**  1) The equatorial radius, a, can be in any units, but meters is
**     the conventional choice.
**
**  2) The flattening, f, is (for the Earth) a value around 0.00335,
**     i.e. around 1/298.
**
**  3) The equatorial radius, a, and the height, height, must be
**     given in the same units, and determine the units of the
**     returned geocentric vector, xyz.
**
**  4) No validation is performed on individual arguments.  The error
**     status -1 protects against (unrealistic) cases that would lead
**     to arithmetic exceptions.  If an error occurs, xyz is unchanged.
**
**  5) The inverse transformation is performed in the function
**     eraGc2gde.
**
**  6) The transformation for a standard ellipsoid (such as ERFA_WGS84) can
**     more conveniently be performed by calling eraGd2gc,  which uses a
**     numerical code to identify the required a and f values.
**
**  References:
**
**     Green, R.M., Spherical Astronomy, Cambridge University Press,
**     (1985) Section 4.5, p96.
**
**     Explanatory Supplement to the Astronomical Almanac,
**     P. Kenneth Seidelmann (ed), University Science Books (1992),
**     Section 4.22, p202.
**
**  This revision:  2023 March 10
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double sp, cp, w, d, ac, as, r;


/* Functions of geodetic latitude. */
   sp = sin(phi);
   cp = cos(phi);
   w = 1.0 - f;
   w = w * w;
   d = cp*cp + w*sp*sp;
   if ( d <= 0.0 ) return -1;
   ac = a / sqrt(d);
   as = w * ac;

/* Geocentric vector. */
   r = (ac + height) * cp;
   xyz[0] = r * cos(elong);
   xyz[1] = r * sin(elong);
   xyz[2] = (as + height) * sp;

/* Success. */
   return 0;

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
