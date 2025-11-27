#include "erfa.h"
#include "erfam.h"

int eraStarpv(double ra, double dec,
              double pmr, double pmd, double px, double rv,
              double pv[2][3])
/*
**  - - - - - - - - - -
**   e r a S t a r p v
**  - - - - - - - - - -
**
**  Convert star catalog coordinates to position+velocity vector.
**
**  Given (Note 1):
**     ra     double        right ascension (radians)
**     dec    double        declination (radians)
**     pmr    double        RA proper motion (radians/year)
**     pmd    double        Dec proper motion (radians/year)
**     px     double        parallax (arcseconds)
**     rv     double        radial velocity (km/s, positive = receding)
**
**  Returned (Note 2):
**     pv     double[2][3]  pv-vector (au, au/day)
**
**  Returned (function value):
**            int           status:
**                              0 = no warnings
**                              1 = distance overridden (Note 6)
**                              2 = excessive speed (Note 7)
**                              4 = solution didn't converge (Note 8)
**                           else = binary logical OR of the above
**
**  Notes:
**
**  1) The star data accepted by this function are "observables" for an
**     imaginary observer at the solar-system barycenter.  Proper motion
**     and radial velocity are, strictly, in terms of barycentric
**     coordinate time, TCB.  For most practical applications, it is
**     permissible to neglect the distinction between TCB and ordinary
**     "proper" time on Earth (TT/TAI).  The result will, as a rule, be
**     limited by the intrinsic accuracy of the proper-motion and
**     radial-velocity data;  moreover, the pv-vector is likely to be
**     merely an intermediate result, so that a change of time unit
**     would cancel out overall.
**
**     In accordance with normal star-catalog conventions, the object's
**     right ascension and declination are freed from the effects of
**     secular aberration.  The frame, which is aligned to the catalog
**     equator and equinox, is Lorentzian and centered on the SSB.
**
**  2) The resulting position and velocity pv-vector is with respect to
**     the same frame and, like the catalog coordinates, is freed from
**     the effects of secular aberration.  Should the "coordinate
**     direction", where the object was located at the catalog epoch, be
**     required, it may be obtained by calculating the magnitude of the
**     position vector pv[0][0-2] dividing by the speed of light in
**     au/day to give the light-time, and then multiplying the space
**     velocity pv[1][0-2] by this light-time and adding the result to
**     pv[0][0-2].
**
**     Summarizing, the pv-vector returned is for most stars almost
**     identical to the result of applying the standard geometrical
**     "space motion" transformation.  The differences, which are the
**     subject of the Stumpff paper referenced below, are:
**
**     (i) In stars with significant radial velocity and proper motion,
**     the constantly changing light-time distorts the apparent proper
**     motion.  Note that this is a classical, not a relativistic,
**     effect.
**
**     (ii) The transformation complies with special relativity.
**
**  3) Care is needed with units.  The star coordinates are in radians
**     and the proper motions in radians per Julian year, but the
**     parallax is in arcseconds; the radial velocity is in km/s, but
**     the pv-vector result is in au and au/day.
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
**     of which is an arbitrary (large) value (see the constant PXMIN).
**     When the distance is overridden in this way, the status,
**     initially zero, has 1 added to it.
**
**  7) If the space velocity is a significant fraction of c (see the
**     constant VMAX), it is arbitrarily set to zero.  When this action
**     occurs, 2 is added to the status.
**
**  8) The relativistic adjustment involves an iterative calculation.
**     If the process fails to converge within a set number (IMAX) of
**     iterations, 4 is added to the status.
**
**  9) The inverse transformation is performed by the function
**     eraPvstar.
**
**  Called:
**     eraS2pv      spherical coordinates to pv-vector
**     eraPm        modulus of p-vector
**     eraZp        zero p-vector
**     eraPn        decompose p-vector into modulus and direction
**     eraPdp       scalar product of two p-vectors
**     eraSxp       multiply p-vector by scalar
**     eraPmp       p-vector minus p-vector
**     eraPpp       p-vector plus p-vector
**
**  Reference:
**
**     Stumpff, P., 1985, Astron.Astrophys. 144, 232-240.
**
**  This revision:  2023 May 4
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Smallest allowed parallax */
   static const double PXMIN = 1e-7;

/* Largest allowed speed (fraction of c) */
   static const double VMAX = 0.5;

/* Maximum number of iterations for relativistic solution */
   static const int IMAX = 100;

   int i, iwarn;
   double w, r, rd, rad, decd, v, pu[3], usr[3], ust[3],
          vsr, vst, betst, betsr, bett, betr,
          dd, ddel, ur[3], ut[3],
          d = 0.0, del = 0.0,       /* to prevent */
          odd = 0.0, oddel = 0.0,   /* compiler   */
          od = 0.0, odel = 0.0;     /* warnings   */


/* Distance (au). */
   if (px >= PXMIN) {
      w = px;
      iwarn = 0;
   } else {
      w = PXMIN;
      iwarn = 1;
   }
   r = ERFA_DR2AS / w;

/* Radial speed (au/day). */
   rd = ERFA_DAYSEC * rv * 1e3 / ERFA_DAU;

/* Proper motion (radian/day). */
   rad = pmr / ERFA_DJY;
   decd = pmd / ERFA_DJY;

/* To pv-vector (au,au/day). */
   eraS2pv(ra, dec, r, rad, decd, rd, pv);

/* If excessive velocity, arbitrarily set it to zero. */
   v = eraPm(pv[1]);
   if (v / ERFA_DC > VMAX) {
      eraZp(pv[1]);
      iwarn += 2;
   }

/* Isolate the radial component of the velocity (au/day). */
   eraPn(pv[0], &w, pu);
   vsr = eraPdp(pu, pv[1]);
   eraSxp(vsr, pu, usr);

/* Isolate the transverse component of the velocity (au/day). */
   eraPmp(pv[1], usr, ust);
   vst = eraPm(ust);

/* Special-relativity dimensionless parameters. */
   betsr = vsr / ERFA_DC;
   betst = vst / ERFA_DC;

/* Determine the observed-to-inertial correction terms. */
   bett = betst;
   betr = betsr;
   for (i = 0; i < IMAX; i++) {
      d = 1.0 + betr;
      w = betr*betr + bett*bett;
      del = - w / (sqrt(1.0 - w) + 1.0);
      betr = d * betsr + del;
      bett = d * betst;
      if (i > 0) {
         dd = fabs(d - od);
         ddel = fabs(del - odel);
         if ((i > 1) && (dd >= odd) && (ddel >= oddel)) break;
         odd = dd;
         oddel = ddel;
      }
      od = d;
      odel = del;
   }
   if (i >= IMAX) iwarn += 4;

/* Scale observed tangential velocity vector into inertial (au/d). */
   eraSxp(d, ust, ut);

/* Compute inertial radial velocity vector (au/d). */
   eraSxp(ERFA_DC*(d*betsr+del), pu, ur);

/* Combine the two to obtain the inertial space velocity vector. */
   eraPpp(ur, ut, pv[1]);

/* Return the status. */
   return iwarn;

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
