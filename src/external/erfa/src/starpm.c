#include "erfa.h"
#include "erfam.h"

int eraStarpm(double ra1, double dec1,
              double pmr1, double pmd1, double px1, double rv1,
              double ep1a, double ep1b, double ep2a, double ep2b,
              double *ra2, double *dec2,
              double *pmr2, double *pmd2, double *px2, double *rv2)
/*
**  - - - - - - - - - -
**   e r a S t a r p m
**  - - - - - - - - - -
**
**  Star proper motion:  update star catalog data for space motion.
**
**  Given:
**     ra1    double     right ascension (radians), before
**     dec1   double     declination (radians), before
**     pmr1   double     RA proper motion (radians/year), before
**     pmd1   double     Dec proper motion (radians/year), before
**     px1    double     parallax (arcseconds), before
**     rv1    double     radial velocity (km/s, +ve = receding), before
**     ep1a   double     "before" epoch, part A (Note 1)
**     ep1b   double     "before" epoch, part B (Note 1)
**     ep2a   double     "after" epoch, part A (Note 1)
**     ep2b   double     "after" epoch, part B (Note 1)
**
**  Returned:
**     ra2    double     right ascension (radians), after
**     dec2   double     declination (radians), after
**     pmr2   double     RA proper motion (radians/year), after
**     pmd2   double     Dec proper motion (radians/year), after
**     px2    double     parallax (arcseconds), after
**     rv2    double     radial velocity (km/s, +ve = receding), after
**
**  Returned (function value):
**            int        status:
**                          -1 = system error (should not occur)
**                           0 = no warnings or errors
**                           1 = distance overridden (Note 6)
**                           2 = excessive velocity (Note 7)
**                           4 = solution didn't converge (Note 8)
**                        else = binary logical OR of the above warnings
**
**  Notes:
**
**  1) The starting and ending TDB dates ep1a+ep1b and ep2a+ep2b are
**     Julian Dates, apportioned in any convenient way between the two
**     parts (A and B).  For example, JD(TDB)=2450123.7 could be
**     expressed in any of these ways, among others:
**
**            epNa            epNb
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
**     good compromises between resolution and convenience.
**
**  2) In accordance with normal star-catalog conventions, the object's
**     right ascension and declination are freed from the effects of
**     secular aberration.  The frame, which is aligned to the catalog
**     equator and equinox, is Lorentzian and centered on the SSB.
**
**     The proper motions are the rate of change of the right ascension
**     and declination at the catalog epoch and are in radians per TDB
**     Julian year.
**
**     The parallax and radial velocity are in the same frame.
**
**  3) Care is needed with units.  The star coordinates are in radians
**     and the proper motions in radians per Julian year, but the
**     parallax is in arcseconds.
**
**  4) The RA proper motion is in terms of coordinate angle, not true
**     angle.  If the catalog uses arcseconds for both RA and Dec proper
**     motions, the RA proper motion will need to be divided by cos(Dec)
**     before use.
**
**  5) Straight-line motion at constant speed, in the inertial frame,
**     is assumed.
**
**  6) An extremely small (or zero or negative) parallax is interpreted
**     to mean that the object is on the "celestial sphere", the radius
**     of which is an arbitrary (large) value (see the eraStarpv
**     function for the value used).  When the distance is overridden in
**     this way, the status, initially zero, has 1 added to it.
**
**  7) If the space velocity is a significant fraction of c (see the
**     constant VMAX in the function eraStarpv), it is arbitrarily set
**     to zero.  When this action occurs, 2 is added to the status.
**
**  8) The relativistic adjustment carried out in the eraStarpv function
**     involves an iterative calculation.  If the process fails to
**     converge within a set number of iterations, 4 is added to the
**     status.
**
**  Called:
**     eraStarpv    star catalog data to space motion pv-vector
**     eraPvu       update a pv-vector
**     eraPdp       scalar product of two p-vectors
**     eraPvstar    space motion pv-vector to star catalog data
**
**  This revision:  2023 May 3
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double pv1[2][3], tl1, dt, pv[2][3], r2, rdv, v2, c2mv2, tl2,
          pv2[2][3];
   int j1, j2, j;


/* RA,Dec etc. at the "before" epoch to space motion pv-vector. */
   j1 = eraStarpv(ra1, dec1, pmr1, pmd1, px1, rv1, pv1);

/* Light time when observed (days). */
   tl1 = eraPm(pv1[0]) / ERFA_DC;

/* Time interval, "before" to "after" (days). */
   dt = (ep2a - ep1a) + (ep2b - ep1b);

/* Move star along track from the "before" observed position to the */
/* "after" geometric position. */
   eraPvu(dt + tl1, pv1, pv);

/* From this geometric position, deduce the observed light time (days) */
/* at the "after" epoch (with theoretically unneccessary error check). */
   r2 = eraPdp(pv[0], pv[0]);
   rdv = eraPdp(pv[0], pv[1]);
   v2 = eraPdp(pv[1], pv[1]);
   c2mv2 = ERFA_DC*ERFA_DC - v2;
   if (c2mv2 <= 0.0) return -1;
   tl2 = (-rdv + sqrt(rdv*rdv + c2mv2*r2)) / c2mv2;

/* Move the position along track from the observed place at the */
/* "before" epoch to the observed place at the "after" epoch. */
   eraPvu(dt + (tl1 - tl2), pv1, pv2);

/* Space motion pv-vector to RA,Dec etc. at the "after" epoch. */
   j2 = eraPvstar(pv2, ra2, dec2, pmr2, pmd2, px2, rv2);

/* Final status. */
   j = (j2 == 0) ? j1 : -1;

   return j;

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
