
#include "erfa.h"

void eraG2icrs ( double dl, double db, double *dr, double *dd )
/*
**  - - - - - - - - - -
**   e r a G 2 i c r s
**  - - - - - - - - - -
**
**  Transformation from Galactic coordinates to ICRS.
**
**  Given:
**     dl     double      Galactic longitude (radians)
**     db     double      Galactic latitude (radians)
**
**  Returned:
**     dr     double      ICRS right ascension (radians)
**     dd     double      ICRS declination (radians)
**
**  Notes:
**
**  1) The IAU 1958 system of Galactic coordinates was defined with
**     respect to the now obsolete reference system FK4 B1950.0.  When
**     interpreting the system in a modern context, several factors have
**     to be taken into account:
**
**     . The inclusion in FK4 positions of the E-terms of aberration.
**
**     . The distortion of the FK4 proper motion system by differential
**       Galactic rotation.
**
**     . The use of the B1950.0 equinox rather than the now-standard
**       J2000.0.
**
**     . The frame bias between ICRS and the J2000.0 mean place system.
**
**     The Hipparcos Catalogue (Perryman & ESA 1997) provides a rotation
**     matrix that transforms directly between ICRS and Galactic
**     coordinates with the above factors taken into account.  The
**     matrix is derived from three angles, namely the ICRS coordinates
**     of the Galactic pole and the longitude of the ascending node of
**     the Galactic equator on the ICRS equator.  They are given in
**     degrees to five decimal places and for canonical purposes are
**     regarded as exact.  In the Hipparcos Catalogue the matrix
**     elements are given to 10 decimal places (about 20 microarcsec).
**     In the present ERFA function the matrix elements have been
**     recomputed from the canonical three angles and are given to 30
**     decimal places.
**
**  2) The inverse transformation is performed by the function eraIcrs2g.
**
**  Called:
**     eraAnp       normalize angle into range 0 to 2pi
**     eraAnpm      normalize angle into range +/- pi
**     eraS2c       spherical coordinates to unit vector
**     eraTrxp      product of transpose of r-matrix and p-vector
**     eraC2s       p-vector to spherical
**
**  Reference:
**     Perryman M.A.C. & ESA, 1997, ESA SP-1200, The Hipparcos and Tycho
**     catalogues.  Astrometric and photometric star catalogues
**     derived from the ESA Hipparcos Space Astrometry Mission.  ESA
**     Publications Division, Noordwijk, Netherlands.
**
**  This revision:   2023 April 16
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double v1[3], v2[3];

/*
**  L2,B2 system of Galactic coordinates in the form presented in the
**  Hipparcos Catalogue.  In degrees:
**
**  P = 192.85948    right ascension of the Galactic north pole in ICRS
**  Q =  27.12825    declination of the Galactic north pole in ICRS
**  R =  32.93192    Galactic longitude of the ascending node of
**                   the Galactic equator on the ICRS equator
**
**  ICRS to Galactic rotation matrix, obtained by computing
**  R_3(-R) R_1(pi/2-Q) R_3(pi/2+P) to the full precision shown:
*/
   double r[3][3] = { { -0.054875560416215368492398900454,
                        -0.873437090234885048760383168409,
                        -0.483835015548713226831774175116 },
                      { +0.494109427875583673525222371358,
                        -0.444829629960011178146614061616,
                        +0.746982244497218890527388004556 },
                      { -0.867666149019004701181616534570,
                        -0.198076373431201528180486091412,
                        +0.455983776175066922272100478348 } };


/* Spherical to Cartesian. */
   eraS2c(dl, db, v1);

/* Galactic to ICRS. */
   eraTrxp(r, v1, v2);

/* Cartesian to spherical. */
   eraC2s(v2, dr, dd);

/* Express in conventional ranges. */
   *dr = eraAnp(*dr);
   *dd = eraAnpm(*dd);

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
