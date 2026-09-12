#include "erfa.h"
#include "erfam.h"

void eraApcs(double date1, double date2, double pv[2][3],
             double ebpv[2][3], double ehp[3],
             eraASTROM *astrom)
/*
**  - - - - - - - -
**   e r a A p c s
**  - - - - - - - -
**
**  For an observer whose geocentric position and velocity are known,
**  prepare star-independent astrometry parameters for transformations
**  between ICRS and GCRS.  The Earth ephemeris is supplied by the
**  caller.
**
**  The parameters produced by this function are required in the space
**  motion, parallax, light deflection and aberration parts of the
**  astrometric transformation chain.
**
**  Given:
**     date1  double       TDB as a 2-part...
**     date2  double       ...Julian Date (Note 1)
**     pv     double[2][3] observer's geocentric pos/vel (m, m/s)
**     ebpv   double[2][3] Earth barycentric PV (au, au/day)
**     ehp    double[3]    Earth heliocentric P (au)
**
**  Returned:
**     astrom eraASTROM*   star-independent astrometry parameters:
**      pmt    double       PM time interval (SSB, Julian years)
**      eb     double[3]    SSB to observer (vector, au)
**      eh     double[3]    Sun to observer (unit vector)
**      em     double       distance from Sun to observer (au)
**      v      double[3]    barycentric observer velocity (vector, c)
**      bm1    double       sqrt(1-|v|^2): reciprocal of Lorenz factor
**      bpn    double[3][3] bias-precession-nutation matrix
**      along  double       unchanged
**      xpl    double       unchanged
**      ypl    double       unchanged
**      sphi   double       unchanged
**      cphi   double       unchanged
**      diurab double       unchanged
**      eral   double       unchanged
**      refa   double       unchanged
**      refb   double       unchanged
**
**  Notes:
**
**  1) The TDB date date1+date2 is a Julian Date, apportioned in any
**     convenient way between the two arguments.  For example,
**     JD(TDB)=2450123.7 could be expressed in any of these ways, among
**     others:
**
**            date1          date2
**
**         2450123.7           0.0       (JD method)
**         2451545.0       -1421.3       (J2000 method)
**         2400000.5       50123.2       (MJD method)
**         2450123.5           0.2       (date & time method)
**
**     The JD method is the most natural and convenient to use in cases
**     where the loss of several decimal digits of resolution is
**     acceptable.  The J2000 method is best matched to the way the
**     argument is handled internally and will deliver the optimum
**     resolution.  The MJD method and the date & time methods are both
**     good compromises between resolution and convenience.  For most
**     applications of this function the choice will not be at all
**     critical.
**
**     TT can be used instead of TDB without any significant impact on
**     accuracy.
**
**  2) All the vectors are with respect to BCRS axes.
**
**  3) Providing separate arguments for (i) the observer's geocentric
**     position and velocity and (ii) the Earth ephemeris is done for
**     convenience in the geocentric, terrestrial and Earth orbit cases.
**     For deep space applications it maybe more convenient to specify
**     zero geocentric position and velocity and to supply the
**     observer's position and velocity information directly instead of
**     with respect to the Earth.  However, note the different units:
**     m and m/s for the geocentric vectors, au and au/day for the
**     heliocentric and barycentric vectors.
**
**  4) In cases where the caller does not wish to provide the Earth
**     ephemeris, the function eraApcs13 can be used instead of the
**     present function.  This computes the Earth ephemeris using the
**     ERFA function eraEpv00.
**
**  5) This is one of several functions that inserts into the astrom
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
**  6) The context structure astrom produced by this function is used by
**     eraAtciq* and eraAticq*.
**
**  Called:
**     eraCp        copy p-vector
**     eraPm        modulus of p-vector
**     eraPn        decompose p-vector into modulus and direction
**     eraIr        initialize r-matrix to identity
**
**  This revision:   2021 February 24
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* au/d to m/s */
   const double AUDMS = ERFA_DAU/ERFA_DAYSEC;

/* Light time for 1 au (day) */
   const double CR = ERFA_AULT/ERFA_DAYSEC;

   int i;
   double dp, dv, pb[3], vb[3], ph[3], v2, w;


/* Time since reference epoch, years (for proper motion calculation). */
   astrom->pmt = ( (date1 - ERFA_DJ00) + date2 ) / ERFA_DJY;

/* Adjust Earth ephemeris to observer. */
   for (i = 0; i < 3; i++) {
      dp = pv[0][i] / ERFA_DAU;
      dv = pv[1][i] / AUDMS;
      pb[i] = ebpv[0][i] + dp;
      vb[i] = ebpv[1][i] + dv;
      ph[i] = ehp[i] + dp;
   }

/* Barycentric position of observer (au). */
   eraCp(pb, astrom->eb);

/* Heliocentric direction and distance (unit vector and au). */
   eraPn(ph, &astrom->em, astrom->eh);

/* Barycentric vel. in units of c, and reciprocal of Lorenz factor. */
   v2 = 0.0;
   for (i = 0; i < 3; i++) {
      w = vb[i] * CR;
      astrom->v[i] = w;
      v2 += w*w;
   }
   astrom->bm1 = sqrt(1.0 - v2);

/* Reset the NPB matrix. */
   eraIr(astrom->bpn);

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
