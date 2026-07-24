#include "erfa.h"

void eraAtciqz(double rc, double dc, eraASTROM *astrom,
               double *ri, double *di)
/*
**  - - - - - - - - - -
**   e r a A t c i q z
**  - - - - - - - - - -
**
**  Quick ICRS to CIRS transformation, given precomputed star-
**  independent astrometry parameters, and assuming zero parallax and
**  proper motion.
**
**  Use of this function is appropriate when efficiency is important and
**  where many star positions are to be transformed for one date.  The
**  star-independent parameters can be obtained by calling one of the
**  functions eraApci[13], eraApcg[13], eraApco[13] or eraApcs[13].
**
**  The corresponding function for the case of non-zero parallax and
**  proper motion is eraAtciq.
**
**  Given:
**     rc,dc  double     ICRS astrometric RA,Dec (radians)
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
**     ri,di  double     CIRS RA,Dec (radians)
**
**  Note:
**
**     All the vectors are with respect to BCRS axes.
**
**  References:
**
**     Urban, S. & Seidelmann, P. K. (eds), Explanatory Supplement to
**     the Astronomical Almanac, 3rd ed., University Science Books
**     (2013).
**
**     Klioner, Sergei A., "A practical relativistic model for micro-
**     arcsecond astrometry in space", Astr. J. 125, 1580-1597 (2003).
**
**  Called:
**     eraS2c       spherical coordinates to unit vector
**     eraLdsun     light deflection due to Sun
**     eraAb        stellar aberration
**     eraRxp       product of r-matrix and p-vector
**     eraC2s       p-vector to spherical
**     eraAnp       normalize angle into range +/- pi
**
**  This revision:   2013 October 9
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double pco[3], pnat[3], ppr[3], pi[3], w;


/* BCRS coordinate direction (unit vector). */
   eraS2c(rc, dc, pco);

/* Light deflection by the Sun, giving BCRS natural direction. */
   eraLdsun(pco, astrom->eh, astrom->em, pnat);

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
