#include "erfa.h"
#include "erfam.h"

int eraAtco13(double rc, double dc,
              double pr, double pd, double px, double rv,
              double utc1, double utc2, double dut1,
              double elong, double phi, double hm, double xp, double yp,
              double phpa, double tc, double rh, double wl,
              double *aob, double *zob, double *hob,
              double *dob, double *rob, double *eo)
/*
**  - - - - - - - - - -
**   e r a A t c o 1 3
**  - - - - - - - - - -
**
**  ICRS RA,Dec to observed place.  The caller supplies UTC, site
**  coordinates, ambient air conditions and observing wavelength.
**
**  ERFA models are used for the Earth ephemeris, bias-precession-
**  nutation, Earth orientation and refraction.
**
**  Given:
**     rc,dc  double   ICRS right ascension at J2000.0 (radians, Note 1)
**     pr     double   RA proper motion (radians/year, Note 2)
**     pd     double   Dec proper motion (radians/year)
**     px     double   parallax (arcsec)
**     rv     double   radial velocity (km/s, +ve if receding)
**     utc1   double   UTC as a 2-part...
**     utc2   double   ...quasi Julian Date (Notes 3-4)
**     dut1   double   UT1-UTC (seconds, Note 5)
**     elong  double   longitude (radians, east +ve, Note 6)
**     phi    double   latitude (geodetic, radians, Note 6)
**     hm     double   height above ellipsoid (m, geodetic, Notes 6,8)
**     xp,yp  double   polar motion coordinates (radians, Note 7)
**     phpa   double   pressure at the observer (hPa = mB, Note 8)
**     tc     double   ambient temperature at the observer (deg C)
**     rh     double   relative humidity at the observer (range 0-1)
**     wl     double   wavelength (micrometers, Note 9)
**
**  Returned:
**     aob    double*  observed azimuth (radians: N=0,E=90)
**     zob    double*  observed zenith distance (radians)
**     hob    double*  observed hour angle (radians)
**     dob    double*  observed declination (radians)
**     rob    double*  observed right ascension (CIO-based, radians)
**     eo     double*  equation of the origins (ERA-GST, radians)
**
**  Returned (function value):
**            int      status: +1 = dubious year (Note 4)
**                              0 = OK
**                             -1 = unacceptable date
**
**  Notes:
**
**  1)  Star data for an epoch other than J2000.0 (for example from the
**      Hipparcos catalog, which has an epoch of J1991.25) will require
**      a preliminary call to eraPmsafe before use.
**
**  2)  The proper motion in RA is dRA/dt rather than cos(Dec)*dRA/dt.
**
**  3)  utc1+utc2 is quasi Julian Date (see Note 2), apportioned in any
**      convenient way between the two arguments, for example where utc1
**      is the Julian Day Number and utc2 is the fraction of a day.
**
**      However, JD cannot unambiguously represent UTC during a leap
**      second unless special measures are taken.  The convention in the
**      present function is that the JD day represents UTC days whether
**      the length is 86399, 86400 or 86401 SI seconds.
**
**      Applications should use the function eraDtf2d to convert from
**      calendar date and time of day into 2-part quasi Julian Date, as
**      it implements the leap-second-ambiguity convention just
**      described.
**
**  4)  The warning status "dubious year" flags UTCs that predate the
**      introduction of the time scale or that are too far in the
**      future to be trusted.  See eraDat for further details.
**
**  5)  UT1-UTC is tabulated in IERS bulletins.  It increases by exactly
**      one second at the end of each positive UTC leap second,
**      introduced in order to keep UT1-UTC within +/- 0.9s.  n.b. This
**      practice is under review, and in the future UT1-UTC may grow
**      essentially without limit.
**
**  6)  The geographical coordinates are with respect to the ERFA_WGS84
**      reference ellipsoid.  TAKE CARE WITH THE LONGITUDE SIGN:  the
**      longitude required by the present function is east-positive
**      (i.e. right-handed), in accordance with geographical convention.
**
**  7)  The polar motion xp,yp can be obtained from IERS bulletins.  The
**      values are the coordinates (in radians) of the Celestial
**      Intermediate Pole with respect to the International Terrestrial
**      Reference System (see IERS Conventions 2003), measured along the
**      meridians 0 and 90 deg west respectively.  For many
**      applications, xp and yp can be set to zero.
**
**  8)  If hm, the height above the ellipsoid of the observing station
**      in meters, is not known but phpa, the pressure in hPa (=mB),
**      is available, an adequate estimate of hm can be obtained from
**      the expression
**
**            hm = -29.3 * tsl * log ( phpa / 1013.25 );
**
**      where tsl is the approximate sea-level air temperature in K
**      (See Astrophysical Quantities, C.W.Allen, 3rd edition, section
**      52).  Similarly, if the pressure phpa is not known, it can be
**      estimated from the height of the observing station, hm, as
**      follows:
**
**            phpa = 1013.25 * exp ( -hm / ( 29.3 * tsl ) );
**
**      Note, however, that the refraction is nearly proportional to
**      the pressure and that an accurate phpa value is important for
**      precise work.
**
**  9)  The argument wl specifies the observing wavelength in
**      micrometers.  The transition from optical to radio is assumed to
**      occur at 100 micrometers (about 3000 GHz).
**
**  10) The accuracy of the result is limited by the corrections for
**      refraction, which use a simple A*tan(z) + B*tan^3(z) model.
**      Providing the meteorological parameters are known accurately and
**      there are no gross local effects, the predicted observed
**      coordinates should be within 0.05 arcsec (optical) or 1 arcsec
**      (radio) for a zenith distance of less than 70 degrees, better
**      than 30 arcsec (optical or radio) at 85 degrees and better
**      than 20 arcmin (optical) or 30 arcmin (radio) at the horizon.
**
**      Without refraction, the complementary functions eraAtco13 and
**      eraAtoc13 are self-consistent to better than 1 microarcsecond
**      all over the celestial sphere.  With refraction included,
**      consistency falls off at high zenith distances, but is still
**      better than 0.05 arcsec at 85 degrees.
**
**  11) "Observed" Az,ZD means the position that would be seen by a
**      perfect geodetically aligned theodolite.  (Zenith distance is
**      used rather than altitude in order to reflect the fact that no
**      allowance is made for depression of the horizon.)  This is
**      related to the observed HA,Dec via the standard rotation, using
**      the geodetic latitude (corrected for polar motion), while the
**      observed HA and RA are related simply through the Earth rotation
**      angle and the site longitude.  "Observed" RA,Dec or HA,Dec thus
**      means the position that would be seen by a perfect equatorial
**      with its polar axis aligned to the Earth's axis of rotation.
**
**  12) It is advisable to take great care with units, as even unlikely
**      values of the input parameters are accepted and processed in
**      accordance with the models used.
**
**  Called:
**     eraApco13    astrometry parameters, ICRS-observed, 2013
**     eraAtciq     quick ICRS to CIRS
**     eraAtioq     quick CIRS to observed
**
**  This revision:   2022 May 3
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   int j;
   eraASTROM astrom;
   double ri, di;


/* Star-independent astrometry parameters. */
   j = eraApco13(utc1, utc2, dut1, elong, phi, hm, xp, yp,
                 phpa, tc, rh, wl, &astrom, eo);

/* Abort if bad UTC. */
   if ( j < 0 ) return j;

/* Transform ICRS to CIRS. */
   eraAtciq(rc, dc, pr, pd, px, rv, &astrom, &ri, &di);

/* Transform CIRS to observed. */
   eraAtioq(ri, di, &astrom, aob, zob, hob, dob, rob);

/* Return OK/warning status. */
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
