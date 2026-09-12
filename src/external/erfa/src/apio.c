#include "erfa.h"
#include "erfam.h"

void eraApio(double sp, double theta,
             double elong, double phi, double hm, double xp, double yp,
             double refa, double refb,
             eraASTROM *astrom)
/*
**  - - - - - - - -
**   e r a A p i o
**  - - - - - - - -
**
**  For a terrestrial observer, prepare star-independent astrometry
**  parameters for transformations between CIRS and observed
**  coordinates.  The caller supplies the Earth orientation information
**  and the refraction constants as well as the site coordinates.
**
**  Given:
**     sp     double      the TIO locator s' (radians, Note 1)
**     theta  double      Earth rotation angle (radians)
**     elong  double      longitude (radians, east +ve, Note 2)
**     phi    double      geodetic latitude (radians, Note 2)
**     hm     double      height above ellipsoid (m, geodetic Note 2)
**     xp,yp  double      polar motion coordinates (radians, Note 3)
**     refa   double      refraction constant A (radians, Note 4)
**     refb   double      refraction constant B (radians, Note 4)
**
**  Returned:
**     astrom eraASTROM*  star-independent astrometry parameters:
**      pmt    double       unchanged
**      eb     double[3]    unchanged
**      eh     double[3]    unchanged
**      em     double       unchanged
**      v      double[3]    unchanged
**      bm1    double       unchanged
**      bpn    double[3][3] unchanged
**      along  double       adjusted longitude (radians)
**      xpl    double       polar motion xp wrt local meridian (radians)
**      ypl    double       polar motion yp wrt local meridian (radians)
**      sphi   double       sine of geodetic latitude
**      cphi   double       cosine of geodetic latitude
**      diurab double       magnitude of diurnal aberration vector
**      eral   double       "local" Earth rotation angle (radians)
**      refa   double       refraction constant A (radians)
**      refb   double       refraction constant B (radians)
**
**  Notes:
**
**  1) sp, the TIO locator s', is a tiny quantity needed only by the
**     most precise applications.  It can either be set to zero or
**     predicted using the ERFA function eraSp00.
**
**  2) The geographical coordinates are with respect to the ERFA_WGS84
**     reference ellipsoid.  TAKE CARE WITH THE LONGITUDE SIGN:  the
**     longitude required by the present function is east-positive
**     (i.e. right-handed), in accordance with geographical convention.
**
**  3) The polar motion xp,yp can be obtained from IERS bulletins.  The
**     values are the coordinates (in radians) of the Celestial
**     Intermediate Pole with respect to the International Terrestrial
**     Reference System (see IERS Conventions 2003), measured along the
**     meridians 0 and 90 deg west respectively.  For many applications,
**     xp and yp can be set to zero.
**
**     Internally, the polar motion is stored in a form rotated onto the
**     local meridian.
**
**  4) The refraction constants refa and refb are for use in a
**     dZ = A*tan(Z)+B*tan^3(Z) model, where Z is the observed
**     (i.e. refracted) zenith distance and dZ is the amount of
**     refraction.
**
**  5) It is advisable to take great care with units, as even unlikely
**     values of the input parameters are accepted and processed in
**     accordance with the models used.
**
**  6) In cases where the caller does not wish to provide the Earth
**     rotation information and refraction constants, the function
**     eraApio13 can be used instead of the present function.  This
**     starts from UTC and weather readings etc. and computes suitable
**     values using other ERFA functions.
**
**  7) This is one of several functions that inserts into the astrom
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
**  8) The context structure astrom produced by this function is used by
**     eraAtioq and eraAtoiq.
**
**  Called:
**     eraIr        initialize r-matrix to identity
**     eraRz        rotate around Z-axis
**     eraRy        rotate around Y-axis
**     eraRx        rotate around X-axis
**     eraAnpm      normalize angle into range +/- pi
**     eraPvtob     position/velocity of terrestrial station
**
**  This revision:   2021 February 24
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double r[3][3], a, b, eral, c, pv[2][3];


/* Form the rotation matrix, CIRS to apparent [HA,Dec]. */
   eraIr(r);
   eraRz(theta+sp, r);
   eraRy(-xp, r);
   eraRx(-yp, r);
   eraRz(elong, r);

/* Solve for local Earth rotation angle. */
   a = r[0][0];
   b = r[0][1];
   eral = ( a != 0.0 || b != 0.0 ) ?  atan2(b, a) : 0.0;
   astrom->eral = eral;

/* Solve for polar motion [X,Y] with respect to local meridian. */
   a = r[0][0];
   c = r[0][2];
   astrom->xpl = atan2(c, sqrt(a*a+b*b));
   a = r[1][2];
   b = r[2][2];
   astrom->ypl = ( a != 0.0 || b != 0.0 ) ? -atan2(a, b) : 0.0;

/* Adjusted longitude. */
   astrom->along = eraAnpm(eral - theta);

/* Functions of latitude. */
   astrom->sphi = sin(phi);
   astrom->cphi = cos(phi);

/* Observer's geocentric position and velocity (m, m/s, CIRS). */
   eraPvtob(elong, phi, hm, xp, yp, sp, theta, pv);

/* Magnitude of diurnal aberration vector. */
   astrom->diurab = sqrt(pv[1][0]*pv[1][0]+pv[1][1]*pv[1][1]) / ERFA_CMPS;

/* Refraction constants. */
   astrom->refa = refa;
   astrom->refb = refb;

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
