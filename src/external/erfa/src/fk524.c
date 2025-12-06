#include "erfa.h"
#include "erfam.h"

void eraFk524(double r2000, double d2000,
              double dr2000, double dd2000,
              double p2000, double v2000,
              double *r1950, double *d1950,
              double *dr1950, double *dd1950,
              double *p1950, double *v1950)
/*
**  - - - - - - - - -
**   e r a F k 5 2 4
**  - - - - - - - - -
**
**  Convert J2000.0 FK5 star catalog data to B1950.0 FK4.
**
**  Given: (all J2000.0, FK5)
**     r2000,d2000    double   J2000.0 RA,Dec (rad)
**     dr2000,dd2000  double   J2000.0 proper motions (rad/Jul.yr)
**     p2000          double   parallax (arcsec)
**     v2000          double   radial velocity (km/s, +ve = moving away)
**
**  Returned: (all B1950.0, FK4)
**     r1950,d1950    double   B1950.0 RA,Dec (rad)
**     dr1950,dd1950  double   B1950.0 proper motions (rad/trop.yr)
**     p1950          double   parallax (arcsec)
**     v1950          double   radial velocity (km/s, +ve = moving away)
**
**  Notes:
**
**  1) The proper motions in RA are dRA/dt rather than cos(Dec)*dRA/dt,
**     and are per year rather than per century.
**
**  2) The conversion is somewhat complicated, for several reasons:
**
**     . Change of standard epoch from J2000.0 to B1950.0.
**
**     . An intermediate transition date of 1984 January 1.0 TT.
**
**     . A change of precession model.
**
**     . Change of time unit for proper motion (Julian to tropical).
**
**     . FK4 positions include the E-terms of aberration, to simplify
**       the hand computation of annual aberration.  FK5 positions
**       assume a rigorous aberration computation based on the Earth's
**       barycentric velocity.
**
**     . The E-terms also affect proper motions, and in particular cause
**       objects at large distances to exhibit fictitious proper
**       motions.
**
**     The algorithm is based on Smith et al. (1989) and Yallop et al.
**     (1989), which presented a matrix method due to Standish (1982) as
**     developed by Aoki et al. (1983), using Kinoshita's development of
**     Andoyer's post-Newcomb precession.  The numerical constants from
**     Seidelmann (1992) are used canonically.
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
**  Called:
**     eraAnp       normalize angle into range 0 to 2pi
**     eraPdp       scalar product of two p-vectors
**     eraPm        modulus of p-vector
**     eraPmp       p-vector minus p-vector
**     eraPpp       p-vector pluus p-vector
**     eraPv2s      pv-vector to spherical coordinates
**     eraS2pv      spherical coordinates to pv-vector
**     eraSxp       multiply p-vector by scalar
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
**     Smith, C.A. et al., 1989, "The transformation of astrometric
**     catalog systems to the equinox J2000.0".  Astron.J. 97, 265.
**
**     Standish, E.M., 1982, "Conversion of positions and proper motions
**     from B1950.0 to the IAU system at J2000.0".  Astron.Astrophys.,
**     115, 1, 20-22.
**
**     Yallop, B.D. et al., 1989, "Transformation of mean star places
**     from FK4 B1950.0 to FK5 J2000.0 using matrices in 6-space".
**     Astron.J. 97, 274.
**
**  This revision:   2023 March 20
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Radians per year to arcsec per century */
   const double PMF = 100.0*ERFA_DR2AS;

/* Small number to avoid arithmetic problems */
   const double TINY = 1e-30;

/* Miscellaneous */
   double r, d, ur, ud, px, rv, pxvf, w, rd;
   int i, j, k, l;

/* Vectors, p and pv */
   double r0[2][3], r1[2][3], p1[3], p2[3], pv[2][3];

/*
** CANONICAL CONSTANTS (Seidelmann 1992)
*/

/* Km per sec to au per tropical century */
/* = 86400 * 36524.2198782 / 149597870.7 */
   const double VF = 21.095;

/* Constant pv-vector (cf. Seidelmann 3.591-2, vectors A and Adot) */
   static double a[2][3] = {
                      { -1.62557e-6, -0.31919e-6, -0.13843e-6 },
                      { +1.245e-3,   -1.580e-3,   -0.659e-3   }
                           };

/* 3x2 matrix of pv-vectors (cf. Seidelmann 3.592-1, matrix M^-1) */
   static double em[2][3][2][3] = {

    { { { +0.9999256795,     +0.0111814828,     +0.0048590039,    },
        { -0.00000242389840, -0.00000002710544, -0.00000001177742 } },

      { { -0.0111814828,     +0.9999374849,     -0.0000271771,    },
        { +0.00000002710544, -0.00000242392702, +0.00000000006585 } },

      { { -0.0048590040,     -0.0000271557,     +0.9999881946,    },
        { +0.00000001177742, +0.00000000006585, -0.00000242404995 } } },

    { { { -0.000551,         +0.238509,         -0.435614,        },
        { +0.99990432,       +0.01118145,       +0.00485852       } },

      { { -0.238560,         -0.002667,         +0.012254,        },
        { -0.01118145,       +0.99991613,       -0.00002717       } },

      { { +0.435730,         -0.008541,         +0.002117,        },
        { -0.00485852,       -0.00002716,       +0.99996684       } } }

                                  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* The FK5 data (units radians and arcsec per Julian century). */
   r = r2000;
   d = d2000;
   ur = dr2000*PMF;
   ud = dd2000*PMF;
   px = p2000;
   rv = v2000;

/* Express as a pv-vector. */
   pxvf = px * VF;
   w = rv * pxvf;
   eraS2pv(r, d, 1.0, ur, ud, w, r0);

/* Convert pv-vector to Bessel-Newcomb system (cf. Seidelmann 3.592-1). */
   for ( i = 0; i < 2; i++ ) {
      for ( j = 0; j < 3; j++ ) {
         w = 0.0;
         for ( k = 0; k < 2; k++ ) {
            for ( l = 0; l < 3; l++ ) {
               w += em[i][j][k][l] * r0[k][l];
            }
         }
         r1[i][j] = w;
      }
   }

/* Apply E-terms (equivalent to Seidelmann 3.592-3, one iteration). */

/* Direction. */
   w = eraPm(r1[0]);
   eraSxp(eraPdp(r1[0],a[0]), r1[0], p1);
   eraSxp(w, a[0], p2);
   eraPmp(p2, p1, p1);
   eraPpp(r1[0], p1, p1);

/* Recompute length. */
   w = eraPm(p1);

/* Direction. */
   eraSxp(eraPdp(r1[0],a[0]), r1[0], p1);
   eraSxp(w, a[0], p2);
   eraPmp(p2, p1, p1);
   eraPpp(r1[0], p1, pv[0]);

/* Derivative. */
   eraSxp(eraPdp(r1[0],a[1]), pv[0], p1);
   eraSxp(w, a[1], p2);
   eraPmp(p2, p1, p1);
   eraPpp(r1[1], p1, pv[1]);

/* Revert to catalog form. */
   eraPv2s(pv, &r, &d, &w, &ur, &ud, &rd);
   if ( px > TINY ) {
      rv = rd/pxvf;
      px = px/w;
   }

/* Return the results. */
   *r1950 = eraAnp(r);
   *d1950 = d;
   *dr1950 = ur/PMF;
   *dd1950 = ud/PMF;
   *p1950 = px;
   *v1950 = rv;

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
