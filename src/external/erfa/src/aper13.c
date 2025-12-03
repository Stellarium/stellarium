#include "erfa.h"

void eraAper13(double ut11, double ut12, eraASTROM *astrom)
/*
**  - - - - - - - - - -
**   e r a A p e r 1 3
**  - - - - - - - - - -
**
**  In the star-independent astrometry parameters, update only the
**  Earth rotation angle.  The caller provides UT1, (n.b. not UTC).
**
**  Given:
**     ut11    double      UT1 as a 2-part...
**     ut12    double      ...Julian Date (Note 1)
**     astrom  eraASTROM*  star-independent astrometry parameters:
**      pmt    double       not used
**      eb     double[3]    not used
**      eh     double[3]    not used
**      em     double       not used
**      v      double[3]    not used
**      bm1    double       not used
**      bpn    double[3][3] not used
**      along  double       longitude + s' (radians)
**      xpl    double       not used
**      ypl    double       not used
**      sphi   double       not used
**      cphi   double       not used
**      diurab double       not used
**      eral   double       not used
**      refa   double       not used
**      refb   double       not used
**
**  Returned:
**     astrom  eraASTROM*  star-independent astrometry parameters:
**      pmt    double       unchanged
**      eb     double[3]    unchanged
**      eh     double[3]    unchanged
**      em     double       unchanged
**      v      double[3]    unchanged
**      bm1    double       unchanged
**      bpn    double[3][3] unchanged
**      along  double       unchanged
**      xpl    double       unchanged
**      ypl    double       unchanged
**      sphi   double       unchanged
**      cphi   double       unchanged
**      diurab double       unchanged
**      eral   double       "local" Earth rotation angle (radians)
**      refa   double       unchanged
**      refb   double       unchanged
**
**  Notes:
**
**  1) The UT1 date (n.b. not UTC) ut11+ut12 is a Julian Date,
**     apportioned in any convenient way between the arguments ut11 and
**     ut12.  For example, JD(UT1)=2450123.7 could be expressed in any
**     of these ways, among others:
**
**            ut11           ut12
**
**         2450123.7           0.0       (JD method)
**         2451545.0       -1421.3       (J2000 method)
**         2400000.5       50123.2       (MJD method)
**         2450123.5           0.2       (date & time method)
**
**     The JD method is the most natural and convenient to use in cases
**     where the loss of several decimal digits of resolution is
**     acceptable.  The J2000 and MJD methods are good compromises
**     between resolution and convenience.  The date & time method is
**     best matched to the algorithm used:  maximum precision is
**     delivered when the ut11 argument is for 0hrs UT1 on the day in
**     question and the ut12 argument lies in the range 0 to 1, or vice
**     versa.
**
**  2) If the caller wishes to provide the Earth rotation angle itself,
**     the function eraAper can be used instead.  One use of this
**     technique is to substitute Greenwich apparent sidereal time and
**     thereby to support equinox based transformations directly.
**
**  3) This is one of several functions that inserts into the astrom
**     structure star-independent parameters needed for the chain of
**     astrometric transformations ICRS <-> GCRS <-> CIRS <-> observed.
**
**     The various functions support different classes of observer and
**     portions of the transformation chain:
**
**          functions         observer        transformation
**
**       eraApcg eraApcg13    geocentric      ICRS <-> GCRS
**       eraApci eraApci13    terrestrial     ICRS <-> CIRS
**       eraApco eraApco13    terrestrial     ICRS <-> observed
**       eraApcs eraApcs13    space           ICRS <-> GCRS
**       eraAper eraAper13    terrestrial     update Earth rotation
**       eraApio eraApio13    terrestrial     CIRS <-> observed
**
**     Those with names ending in "13" use contemporary ERFA models to
**     compute the various ephemerides.  The others accept ephemerides
**     supplied by the caller.
**
**     The transformation from ICRS to GCRS covers space motion,
**     parallax, light deflection, and aberration.  From GCRS to CIRS
**     comprises frame bias and precession-nutation.  From CIRS to
**     observed takes account of Earth rotation, polar motion, diurnal
**     aberration and parallax (unless subsumed into the ICRS <-> GCRS
**     transformation), and atmospheric refraction.
**
**  Called:
**     eraAper      astrometry parameters: update ERA
**     eraEra00     Earth rotation angle, IAU 2000
**
**  This revision:   2013 September 25
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   eraAper(eraEra00(ut11,ut12), astrom);

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
