#include "erfa.h"
#include "erfam.h"

void eraPmat76(double date1, double date2, double rmatp[3][3])
/*
**  - - - - - - - - - -
**   e r a P m a t 7 6
**  - - - - - - - - - -
**
**  Precession matrix from J2000.0 to a specified date, IAU 1976 model.
**
**  Given:
**     date1,date2 double       ending date, TT (Note 1)
**
**  Returned:
**     rmatp       double[3][3] precession matrix, J2000.0 -> date1+date2
**
**  Notes:
**
**  1) The TT date date1+date2 is a Julian Date, apportioned in any
**     convenient way between the two arguments.  For example,
**     JD(TT)=2450123.7 could be expressed in any of these ways,
**     among others:
**
**            date1          date2
**
**         2450123.7           0.0       (JD method)
**         2451545.0       -1421.3       (J2000 method)
**         2400000.5       50123.2       (MJD method)
**         2450123.5           0.2       (date & time method)
**
**     The JD method is the most natural and convenient to use in
**     cases where the loss of several decimal digits of resolution
**     is acceptable.  The J2000 method is best matched to the way
**     the argument is handled internally and will deliver the
**     optimum resolution.  The MJD method and the date & time methods
**     are both good compromises between resolution and convenience.
**
**  2) The matrix operates in the sense V(date) = RMATP * V(J2000),
**     where the p-vector V(J2000) is with respect to the mean
**     equatorial triad of epoch J2000.0 and the p-vector V(date)
**     is with respect to the mean equatorial triad of the given
**     date.
**
**  3) Though the matrix method itself is rigorous, the precession
**     angles are expressed through canonical polynomials which are
**     valid only for a limited time span.  In addition, the IAU 1976
**     precession rate is known to be imperfect.  The absolute accuracy
**     of the present formulation is better than 0.1 arcsec from
**     1960AD to 2040AD, better than 1 arcsec from 1640AD to 2360AD,
**     and remains below 3 arcsec for the whole of the period
**     500BC to 3000AD.  The errors exceed 10 arcsec outside the
**     range 1200BC to 3900AD, exceed 100 arcsec outside 4200BC to
**     5600AD and exceed 1000 arcsec outside 6800BC to 8200AD.
**
**  Called:
**     eraPrec76    accumulated precession angles, IAU 1976
**     eraIr        initialize r-matrix to identity
**     eraRz        rotate around Z-axis
**     eraRy        rotate around Y-axis
**     eraCr        copy r-matrix
**
**  References:
**
**     Lieske, J.H., 1979, Astron.Astrophys. 73, 282.
**      equations (6) & (7), p283.
**
**     Kaplan,G.H., 1981. USNO circular no. 163, pA2.
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double zeta, z, theta, wmat[3][3];


/* Precession Euler angles, J2000.0 to specified date. */
   eraPrec76(ERFA_DJ00, 0.0, date1, date2, &zeta, &z, &theta);

/* Form the rotation matrix. */
   eraIr(  wmat);
   eraRz( -zeta, wmat);
   eraRy(  theta, wmat);
   eraRz( -z, wmat);
   eraCr( wmat, rmatp);

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
