#include "erfa.h"

void eraAtic13(double ri, double di, double date1, double date2,
               double *rc, double *dc, double *eo)
/*
**  - - - - - - - - - -
**   e r a A t i c 1 3
**  - - - - - - - - - -
**
**  Transform star RA,Dec from geocentric CIRS to ICRS astrometric.
**
**  Given:
**     ri,di  double  CIRS geocentric RA,Dec (radians)
**     date1  double  TDB as a 2-part...
**     date2  double  ...Julian Date (Note 1)
**
**  Returned:
**     rc,dc  double  ICRS astrometric RA,Dec (radians)
**     eo     double  equation of the origins (ERA-GST, radians, Note 4)
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
**  2) Iterative techniques are used for the aberration and light
**     deflection corrections so that the functions eraAtic13 (or
**     eraAticq) and eraAtci13 (or eraAtciq) are accurate inverses;
**     even at the edge of the Sun's disk the discrepancy is only about
**     1 nanoarcsecond.
**
**  3) The available accuracy is better than 1 milliarcsecond, limited
**     mainly by the precession-nutation model that is used, namely
**     IAU 2000A/2006.  Very close to solar system bodies, additional
**     errors of up to several milliarcseconds can occur because of
**     unmodeled light deflection;  however, the Sun's contribution is
**     taken into account, to first order.  The accuracy limitations of
**     the ERFA function eraEpv00 (used to compute Earth position and
**     velocity) can contribute aberration errors of up to
**     5 microarcseconds.  Light deflection at the Sun's limb is
**     uncertain at the 0.4 mas level.
**
**  4) Should the transformation to (equinox based) J2000.0 mean place
**     be required rather than (CIO based) ICRS coordinates, subtract the
**     equation of the origins from the returned right ascension:
**     RA = RI - EO.  (The eraAnp function can then be applied, as
**     required, to keep the result in the conventional 0-2pi range.)
**
**  Called:
**     eraApci13    astrometry parameters, ICRS-CIRS, 2013
**     eraAticq     quick CIRS to ICRS astrometric
**
**  This revision:   2022 May 3
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Star-independent astrometry parameters */
   eraASTROM astrom;


/* Star-independent astrometry parameters. */
   eraApci13(date1, date2, &astrom, eo);

/* CIRS to ICRS astrometric. */
   eraAticq(ri, di, &astrom, rc, dc);

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
