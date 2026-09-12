#include "erfa.h"

void eraAtccq(double rc, double dc,
              double pr, double pd, double px, double rv,
              eraASTROM *astrom, double *ra, double *da)
/*
**  - - - - - - - - -
**   e r a A t c c q
**  - - - - - - - - -
**
**  Quick transformation of a star's ICRS catalog entry (epoch J2000.0)
**  into ICRS astrometric place, given precomputed star-independent
**  astrometry parameters.
**
**  Use of this function is appropriate when efficiency is important and
**  where many star positions are to be transformed for one date.  The
**  star-independent parameters can be obtained by calling one of the
**  functions eraApci[13], eraApcg[13], eraApco[13] or eraApcs[13].
**
**  If the parallax and proper motions are zero the transformation has
**  no effect.
**
**  Given:
**     rc,dc  double     ICRS RA,Dec at J2000.0 (radians)
**     pr     double     RA proper motion (radians/year, Note 3)
**     pd     double     Dec proper motion (radians/year)
**     px     double     parallax (arcsec)
**     rv     double     radial velocity (km/s, +ve if receding)
**     astrom eraASTROM* star-independent astrometry parameters:
**      pmt    double       PM time interval (SSB, Julian years)
**      eb     double[3]    SSB to observer (vector, au)
**      eh     double[3]    Sun to observer (unit vector)
**      em     double       distance from Sun to observer (au)
**      v      double[3]    barycentric observer velocity (vector, c)
**      bm1    double       sqrt(1-|v|^2): reciprocal of Lorenz factor
**      bpn    double[3][3] bias-precession-nutation matrix
**      along  double       longitude + s' (radians)
**      xpl    double       polar motion xp wrt local meridian (radians)
**      ypl    double       polar motion yp wrt local meridian (radians)
**      sphi   double       sine of geodetic latitude
**      cphi   double       cosine of geodetic latitude
**      diurab double       magnitude of diurnal aberration vector
**      eral   double       "local" Earth rotation angle (radians)
**      refa   double       refraction constant A (radians)
**      refb   double       refraction constant B (radians)
**
**  Returned:
**     ra,da  double*    ICRS astrometric RA,Dec (radians)
**
**  Notes:
**
**  1) All the vectors are with respect to BCRS axes.
**
**  2) Star data for an epoch other than J2000.0 (for example from the
**     Hipparcos catalog, which has an epoch of J1991.25) will require a
**     preliminary call to eraPmsafe before use.
**
**  3) The proper motion in RA is dRA/dt rather than cos(Dec)*dRA/dt.
**
**  Called:
**     eraPmpx      proper motion and parallax
**     eraC2s       p-vector to spherical
**     eraAnp       normalize angle into range 0 to 2pi
**
**  This revision:   2021 April 18
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double p[3], w;


/* Proper motion and parallax, giving BCRS coordinate direction. */
   eraPmpx(rc, dc, pr, pd, px, rv, astrom->pmt, astrom->eb, p);

/* ICRS astrometric RA,Dec. */
   eraC2s(p, &w, da);
   *ra = eraAnp(w);

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
