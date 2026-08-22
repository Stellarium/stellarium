#include "erfa.h"
#include "erfam.h"

int eraGd2gc ( int n, double elong, double phi, double height,
               double xyz[3] )
/*
**  - - - - - - - - -
**   e r a G d 2 g c
**  - - - - - - - - -
**
**  Transform geodetic coordinates to geocentric using the specified
**  reference ellipsoid.
**
**  Given:
**     n       int        ellipsoid identifier (Note 1)
**     elong   double     longitude (radians, east +ve, Note 3)
**     phi     double     latitude (geodetic, radians, Note 3)
**     height  double     height above ellipsoid (geodetic, Notes 2,3)
**
**  Returned:
**     xyz     double[3]  geocentric vector (Note 2)
**
**  Returned (function value):
**             int        status:  0 = OK
**                                -1 = illegal identifier (Note 3)
**                                -2 = illegal case (Note 3)
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
**  2) The height (height, given) and the geocentric vector (xyz,
**     returned) are in meters.
**
**  3) No validation is performed on the arguments elong, phi and
**     height.  An error status -1 means that the identifier n is
**     illegal.  An error status -2 protects against cases that would
**     lead to arithmetic exceptions.  In all error cases, xyz is set
**     to zeros.
**
**  4) The inverse transformation is performed in the function eraGc2gd.
**
**  Called:
**     eraEform     Earth reference ellipsoids
**     eraGd2gce    geodetic to geocentric transformation, general
**     eraZp        zero p-vector
**
**  This revision:  2023 March 9
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   int j;
   double a, f;


/* Obtain reference ellipsoid parameters. */
   j = eraEform ( n, &a, &f );

/* If OK, transform longitude, geodetic latitude, height to x,y,z. */
   if ( j == 0 ) {
      j = eraGd2gce ( a, f, elong, phi, height, xyz );
      if ( j != 0 ) j = -2;
   }

/* Deal with any errors. */
   if ( j != 0 ) eraZp ( xyz );

/* Return the status. */
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
