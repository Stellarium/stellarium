#include "erfa.h"

void eraAtciqn(double rc, double dc, double pr, double pd,
               double px, double rv, eraASTROM *astrom,
               int n, eraLDBODY b[], double *ri, double *di)
/*
**  - - - - - - - - - -
**   e r a A t c i q n
**  - - - - - - - - - -
**
**  Quick ICRS, epoch J2000.0, to CIRS transformation, given precomputed
**  star-independent astrometry parameters plus a list of light-
**  deflecting bodies.
**
**  Use of this function is appropriate when efficiency is important and
**  where many star positions are to be transformed for one date.  The
**  star-independent parameters can be obtained by calling one of the
**  functions eraApci[13], eraApcg[13], eraApco[13] or eraApcs[13].
**
**
**  If the only light-deflecting body to be taken into account is the
**  Sun, the eraAtciq function can be used instead.  If in addition the
**  parallax and proper motions are zero, the eraAtciqz function can be
**  used.
**
**  Given:
**     rc,dc  double       ICRS RA,Dec at J2000.0 (radians)
**     pr     double       RA proper motion (radians/year, Note 3)
**     pd     double       Dec proper motion (radians/year)
**     px     double       parallax (arcsec)
**     rv     double       radial velocity (km/s, +ve if receding)
**     astrom eraASTROM*   star-independent astrometry parameters:
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
**     n      int          number of bodies (Note 3)
**     b      eraLDBODY[n] data for each of the n bodies (Notes 3,4):
**      bm     double       mass of the body (solar masses, Note 5)
**      dl     double       deflection limiter (Note 6)
**      pv     [2][3]       barycentric PV of the body (au, au/day)
**
**  Returned:
**     ri,di   double    CIRS RA,Dec (radians)
**
**  Notes:
**
**  1) Star data for an epoch other than J2000.0 (for example from the
**     Hipparcos catalog, which has an epoch of J1991.25) will require a
**     preliminary call to eraPmsafe before use.
**
**  2) The proper motion in RA is dRA/dt rather than cos(Dec)*dRA/dt.
**
**  3) The struct b contains n entries, one for each body to be
**     considered.  If n = 0, no gravitational light deflection will be
**     applied, not even for the Sun.
**
**  4) The struct b should include an entry for the Sun as well as for
**     any planet or other body to be taken into account.  The entries
**     should be in the order in which the light passes the body.
**
**  5) In the entry in the b struct for body i, the mass parameter
**     b[i].bm can, as required, be adjusted in order to allow for such
**     effects as quadrupole field.
**
**  6) The deflection limiter parameter b[i].dl is phi^2/2, where phi is
**     the angular separation (in radians) between star and body at
**     which limiting is applied.  As phi shrinks below the chosen
**     threshold, the deflection is artificially reduced, reaching zero
**     for phi = 0.   Example values suitable for a terrestrial
**     observer, together with masses, are as follows:
**
**        body i     b[i].bm        b[i].dl
**
**        Sun        1.0            6e-6
**        Jupiter    0.00095435     3e-9
**        Saturn     0.00028574     3e-10
**
**  7) For efficiency, validation of the contents of the b array is
**     omitted.  The supplied masses must be greater than zero, the
**     position and velocity vectors must be right, and the deflection
**     limiter greater than zero.
**
**  Called:
**     eraPmpx      proper motion and parallax
**     eraLdn       light deflection by n bodies
**     eraAb        stellar aberration
**     eraRxp       product of r-matrix and pv-vector
**     eraC2s       p-vector to spherical
**     eraAnp       normalize angle into range 0 to 2pi
**
**  This revision:   2021 April 3
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double pco[3], pnat[3], ppr[3], pi[3], w;


/* Proper motion and parallax, giving BCRS coordinate direction. */
   eraPmpx(rc, dc, pr, pd, px, rv, astrom->pmt, astrom->eb, pco);

/* Light deflection, giving BCRS natural direction. */
   eraLdn(n, b, astrom->eb, pco, pnat);

/* Aberration, giving GCRS proper direction. */
   eraAb(pnat, astrom->v, astrom->em, astrom->bm1, ppr);

/* Bias-precession-nutation, giving CIRS proper direction. */
   eraRxp(astrom->bpn, ppr, pi);

/* CIRS RA,Dec. */
   eraC2s(pi, &w, di);
   *ri = eraAnp(w);

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
