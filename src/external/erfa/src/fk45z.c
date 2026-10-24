#include "erfa.h"
#include "erfam.h"

void eraFk45z(double r1950, double d1950, double bepoch,
              double *r2000, double *d2000)
/*
**  - - - - - - - - -
**   e r a F k 4 5 z
**  - - - - - - - - -
**
**  Convert a B1950.0 FK4 star position to J2000.0 FK5, assuming zero
**  proper motion in the FK5 system.
**
**  This function converts a star's catalog data from the old FK4
**  (Bessel-Newcomb) system to the later IAU 1976 FK5 (Fricke) system,
**  in such a way that the FK5 proper motion is zero.  Because such a
**  star has, in general, a non-zero proper motion in the FK4 system,
**  the function requires the epoch at which the position in the FK4
**  system was determined.
**
**  Given:
**     r1950,d1950    double   B1950.0 FK4 RA,Dec at epoch (rad)
**     bepoch         double   Besselian epoch (e.g. 1979.3)
**
**  Returned:
**     r2000,d2000    double   J2000.0 FK5 RA,Dec (rad)
**
**  Notes:
**
**  1) The epoch bepoch is strictly speaking Besselian, but if a
**     Julian epoch is supplied the result will be affected only to a
**     negligible extent.
**
**  2) The method is from Appendix 2 of Aoki et al. (1983), but using
**     the constants of Seidelmann (1992).  See the function eraFk425
**     for a general introduction to the FK4 to FK5 conversion.
**
**  3) Conversion from equinox B1950.0 FK4 to equinox J2000.0 FK5 only
**     is provided for.  Conversions for different starting and/or
**     ending epochs would require additional treatment for precession,
**     proper motion and E-terms.
**
**  4) In the FK4 catalog the proper motions of stars within 10 degrees
**     of the poles do not embody differential E-terms effects and
**     should, strictly speaking, be handled in a different manner from
**     stars outside these regions.  However, given the general lack of
**     homogeneity of the star data available for routine astrometry,
**     the difficulties of handling positions that may have been
**     determined from astrometric fields spanning the polar and non-
**     polar regions, the likelihood that the differential E-terms
**     effect was not taken into account when allowing for proper motion
**     in past astrometry, and the undesirability of a discontinuity in
**     the algorithm, the decision has been made in this ERFA algorithm
**     to include the effects of differential E-terms on the proper
**     motions for all stars, whether polar or not.  At epoch J2000.0,
**     and measuring "on the sky" rather than in terms of RA change, the
**     errors resulting from this simplification are less than
**     1 milliarcsecond in position and 1 milliarcsecond per century in
**     proper motion.
**
**  References:
**
**     Aoki, S. et al., 1983, "Conversion matrix of epoch B1950.0
**     FK4-based positions of stars to epoch J2000.0 positions in
**     accordance with the new IAU resolutions".  Astron.Astrophys.
**     128, 263-267.
**
**     Seidelmann, P.K. (ed), 1992, "Explanatory Supplement to the
**     Astronomical Almanac", ISBN 0-935702-68-7.
**
**  Called:
**     eraAnp       normalize angle into range 0 to 2pi
**     eraC2s       p-vector to spherical
**     eraEpb2jd    Besselian epoch to Julian date
**     eraEpj       Julian date to Julian epoch
**     eraPdp       scalar product of two p-vectors
**     eraPmp       p-vector minus p-vector
**     eraPpsp      p-vector plus scaled p-vector
**     eraPvu       update a pv-vector
**     eraS2c       spherical to p-vector
**
**  This revision:   2023 March 4
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Radians per year to arcsec per century */
   const double PMF = 100.0*ERFA_DR2AS;

/* Position and position+velocity vectors */
   double r0[3], p[3], pv[2][3];

/* Miscellaneous */
   double w, djm0, djm;
   int i, j, k;

/*
** CANONICAL CONSTANTS (Seidelmann 1992)
*/

/* Vectors A and Adot (Seidelmann 3.591-2) */
   static double a[3]  = { -1.62557e-6, -0.31919e-6, -0.13843e-6 };
   static double ad[3] = { +1.245e-3,   -1.580e-3,   -0.659e-3   };

/* 3x2 matrix of p-vectors (cf. Seidelmann 3.591-4, matrix M) */
   static double em[2][3][3] = {
         { { +0.9999256782, -0.0111820611, -0.0048579477 },
           { +0.0111820610, +0.9999374784, -0.0000271765 },
           { +0.0048579479, -0.0000271474, +0.9999881997 } },
         { { -0.000551,     -0.238565,     +0.435739     },
           { +0.238514,     -0.002667,     -0.008541     },
           { -0.435623,     +0.012254,     +0.002117     } }
                               };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Spherical coordinates to p-vector. */
   eraS2c(r1950, d1950, r0);

/* Adjust p-vector A to give zero proper motion in FK5. */
   w  = (bepoch - 1950) / PMF;
   eraPpsp(a, w, ad, p);

/* Remove E-terms. */
   eraPpsp(p, -eraPdp(r0,p), r0, p);
   eraPmp(r0, p, p);

/* Convert to Fricke system pv-vector (cf. Seidelmann 3.591-3). */
   for ( i = 0; i < 2; i++ ) {
      for ( j = 0; j < 3; j++ ) {
         w = 0.0;
         for ( k = 0; k < 3; k++ ) {
            w += em[i][j][k] * p[k];
         }
         pv[i][j] = w;
      }
   }

/* Allow for fictitious proper motion. */
   eraEpb2jd(bepoch, &djm0, &djm);
   w = (eraEpj(djm0,djm)-2000.0) / PMF;
   eraPvu(w, pv, pv);

/* Revert to spherical coordinates. */
   eraC2s(pv[0], &w, d2000);
   *r2000 = eraAnp(w);

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
