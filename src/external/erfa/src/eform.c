#include "erfa.h"
#include "erfam.h"

int eraEform ( int n, double *a, double *f )
/*
**  - - - - - - - - -
**   e r a E f o r m
**  - - - - - - - - -
**
**  Earth reference ellipsoids.
**
**  Given:
**     n    int         ellipsoid identifier (Note 1)
**
**  Returned:
**     a    double      equatorial radius (meters, Note 2)
**     f    double      flattening (Note 2)
**
**  Returned (function value):
**          int         status:  0 = OK
**                              -1 = illegal identifier (Note 3)
**
**  Notes:
**
**  1) The identifier n is a number that specifies the choice of
**     reference ellipsoid.  The following are supported:
**
**        n    ellipsoid
**
**        1     ERFA_WGS84
**        2     ERFA_GRS80
**        3     ERFA_WGS72
**
**     The n value has no significance outside the ERFA software.  For
**     convenience, symbols ERFA_WGS84 etc. are defined in erfam.h.
**
**  2) The ellipsoid parameters are returned in the form of equatorial
**     radius in meters (a) and flattening (f).  The latter is a number
**     around 0.00335, i.e. around 1/298.
**
**  3) For the case where an unsupported n value is supplied, zero a and
**     f are returned, as well as error status.
**
**  References:
**
**     Department of Defense World Geodetic System 1984, National
**     Imagery and Mapping Agency Technical Report 8350.2, Third
**     Edition, p3-2.
**
**     Moritz, H., Bull. Geodesique 66-2, 187 (1992).
**
**     The Department of Defense World Geodetic System 1972, World
**     Geodetic System Committee, May 1974.
**
**     Explanatory Supplement to the Astronomical Almanac,
**     P. Kenneth Seidelmann (ed), University Science Books (1992),
**     p220.
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{

/* Look up a and f for the specified reference ellipsoid. */
   switch ( n ) {

   case ERFA_WGS84:
      *a = 6378137.0;
      *f = 1.0 / 298.257223563;
      break;

   case ERFA_GRS80:
      *a = 6378137.0;
      *f = 1.0 / 298.257222101;
      break;

   case ERFA_WGS72:
      *a = 6378135.0;
      *f = 1.0 / 298.26;
      break;

   default:

   /* Invalid identifier. */
      *a = 0.0;
      *f = 0.0;
      return -1;

   }

/* OK status. */
   return 0;

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
