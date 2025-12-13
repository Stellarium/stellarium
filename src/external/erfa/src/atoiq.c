#include "erfa.h"

void eraAtoiq(const char *type,
              double ob1, double ob2, eraASTROM *astrom,
              double *ri, double *di)
/*
**  - - - - - - - - -
**   e r a A t o i q
**  - - - - - - - - -
**
**  Quick observed place to CIRS, given the star-independent astrometry
**  parameters.
**
**  Use of this function is appropriate when efficiency is important and
**  where many star positions are all to be transformed for one date.
**  The star-independent astrometry parameters can be obtained by
**  calling eraApio[13] or eraApco[13].
**
**  Given:
**     type   char[]     type of coordinates: "R", "H" or "A" (Note 1)
**     ob1    double     observed Az, HA or RA (radians; Az is N=0,E=90)
**     ob2    double     observed ZD or Dec (radians)
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
**     ri     double*    CIRS right ascension (CIO-based, radians)
**     di     double*    CIRS declination (radians)
**
**  Notes:
**
**  1) "Observed" Az,ZD means the position that would be seen by a
**     perfect geodetically aligned theodolite.  This is related to
**     the observed HA,Dec via the standard rotation, using the geodetic
**     latitude (corrected for polar motion), while the observed HA and
**     (CIO-based) RA are related simply through the Earth rotation
**     angle and the site longitude.  "Observed" RA,Dec or HA,Dec thus
**     means the position that would be seen by a perfect equatorial
**     with its polar axis aligned to the Earth's axis of rotation.
**
**  2) Only the first character of the type argument is significant.
**     "R" or "r" indicates that ob1 and ob2 are the observed right
**     ascension (CIO-based) and declination;  "H" or "h" indicates that
**     they are hour angle (west +ve) and declination;  anything else
**     ("A" or "a" is recommended) indicates that ob1 and ob2 are
**     azimuth (north zero, east 90 deg) and zenith distance.  (Zenith
**     distance is used rather than altitude in order to reflect the
**     fact that no allowance is made for depression of the horizon.)
**
**  3) The accuracy of the result is limited by the corrections for
**     refraction, which use a simple A*tan(z) + B*tan^3(z) model.
**     Providing the meteorological parameters are known accurately and
**     there are no gross local effects, the predicted intermediate
**     coordinates should be within 0.05 arcsec (optical) or 1 arcsec
**     (radio) for a zenith distance of less than 70 degrees, better
**     than 30 arcsec (optical or radio) at 85 degrees and better than
**     20 arcmin (optical) or 25 arcmin (radio) at the horizon.
**
**     Without refraction, the complementary functions eraAtioq and
**     eraAtoiq are self-consistent to better than 1 microarcsecond all
**     over the celestial sphere.  With refraction included, consistency
**     falls off at high zenith distances, but is still better than
**     0.05 arcsec at 85 degrees.
**
**  4) It is advisable to take great care with units, as even unlikely
**     values of the input parameters are accepted and processed in
**     accordance with the models used.
**
**  Called:
**     eraS2c       spherical coordinates to unit vector
**     eraC2s       p-vector to spherical
**     eraAnp       normalize angle into range 0 to 2pi
**
**  This revision:   2022 August 30
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Minimum sin(alt) for refraction purposes */
   const double SELMIN = 0.05;

   int c;
   double c1, c2, sphi, cphi, ce, xaeo, yaeo, zaeo, v[3],
          xmhdo, ymhdo, zmhdo, az, sz, zdo, refa, refb, tz, dref,
          zdt, xaet, yaet, zaet, xmhda, ymhda, zmhda,
          f, xhd, yhd, zhd, sx, cx, sy, cy, hma;


/* Coordinate type. */
   c = (int) type[0];

/* Coordinates. */
   c1 = ob1;
   c2 = ob2;

/* Sin, cos of latitude. */
   sphi = astrom->sphi;
   cphi = astrom->cphi;

/* Standardize coordinate type. */
   if ( c == 'r' || c == 'R' ) {
      c = 'R';
   } else if ( c == 'h' || c == 'H' ) {
      c = 'H';
   } else {
      c = 'A';
   }

/* If Az,ZD, convert to Cartesian (S=0,E=90). */
   if ( c == 'A' ) {
      ce = sin(c2);
      xaeo = - cos(c1) * ce;
      yaeo = sin(c1) * ce;
      zaeo = cos(c2);

   } else {

   /* If RA,Dec, convert to HA,Dec. */
      if ( c == 'R' ) c1 = astrom->eral - c1;

   /* To Cartesian -HA,Dec. */
      eraS2c ( -c1, c2, v );
      xmhdo = v[0];
      ymhdo = v[1];
      zmhdo = v[2];

   /* To Cartesian Az,El (S=0,E=90). */
      xaeo = sphi*xmhdo - cphi*zmhdo;
      yaeo = ymhdo;
      zaeo = cphi*xmhdo + sphi*zmhdo;
   }

/* Azimuth (S=0,E=90). */
   az = ( xaeo != 0.0 || yaeo != 0.0 ) ? atan2(yaeo,xaeo) : 0.0;

/* Sine of observed ZD, and observed ZD. */
   sz = sqrt ( xaeo*xaeo + yaeo*yaeo );
   zdo = atan2 ( sz, zaeo );

/*
** Refraction
** ----------
*/

/* Fast algorithm using two constant model. */
   refa = astrom->refa;
   refb = astrom->refb;
   tz = sz / ( zaeo > SELMIN ? zaeo : SELMIN );
   dref = ( refa + refb*tz*tz ) * tz;
   zdt = zdo + dref;

/* To Cartesian Az,ZD. */
   ce = sin(zdt);
   xaet = cos(az) * ce;
   yaet = sin(az) * ce;
   zaet = cos(zdt);

/* Cartesian Az,ZD to Cartesian -HA,Dec. */
   xmhda = sphi*xaet + cphi*zaet;
   ymhda = yaet;
   zmhda = - cphi*xaet + sphi*zaet;

/* Diurnal aberration. */
   f = ( 1.0 + astrom->diurab*ymhda );
   xhd = f * xmhda;
   yhd = f * ( ymhda - astrom->diurab );
   zhd = f * zmhda;

/* Polar motion. */
   sx = sin(astrom->xpl);
   cx = cos(astrom->xpl);
   sy = sin(astrom->ypl);
   cy = cos(astrom->ypl);
   v[0] = cx*xhd + sx*sy*yhd - sx*cy*zhd;
   v[1] = cy*yhd + sy*zhd;
   v[2] = sx*xhd - cx*sy*yhd + cx*cy*zhd;

/* To spherical -HA,Dec. */
   eraC2s(v, &hma, di);

/* Right ascension. */
   *ri = eraAnp(astrom->eral + hma);

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
