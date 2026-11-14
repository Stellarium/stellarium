#include "erfa.h"

void eraPb06(double date1, double date2,
             double *bzeta, double *bz, double *btheta)
/*
**  - - - - - - - -
**   e r a P b 0 6
**  - - - - - - - -
**
**  This function forms three Euler angles which implement general
**  precession from epoch J2000.0, using the IAU 2006 model.  Frame
**  bias (the offset between ICRS and mean J2000.0) is included.
**
**  Given:
**     date1,date2  double   TT as a 2-part Julian Date (Note 1)
**
**  Returned:
**     bzeta        double   1st rotation: radians cw around z
**     bz           double   3rd rotation: radians cw around z
**     btheta       double   2nd rotation: radians ccw around y
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
**  2) The traditional accumulated precession angles zeta_A, z_A,
**     theta_A cannot be obtained in the usual way, namely through
**     polynomial expressions, because of the frame bias.  The latter
**     means that two of the angles undergo rapid changes near this
**     date.  They are instead the results of decomposing the
**     precession-bias matrix obtained by using the Fukushima-Williams
**     method, which does not suffer from the problem.  The
**     decomposition returns values which can be used in the
**     conventional formulation and which include frame bias.
**
**  3) The three angles are returned in the conventional order, which
**     is not the same as the order of the corresponding Euler
**     rotations.  The precession-bias matrix is
**     R_3(-z) x R_2(+theta) x R_3(-zeta).
**
**  4) Should zeta_A, z_A, theta_A angles be required that do not
**     contain frame bias, they are available by calling the ERFA
**     function eraP06e.
**
**  Called:
**     eraPmat06    PB matrix, IAU 2006
**     eraRz        rotate around Z-axis
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double r[3][3], y, x;


/* Precession matrix via Fukushima-Williams angles. */
   eraPmat06(date1, date2, r);

/* Solve for z, choosing the +/- pi alternative. */
   y = r[1][2];
   x = -r[0][2];
   if ( x < 0.0 ) {
      y = -y;
      x = -x;
   }
   *bz = ( x != 0.0 || y != 0.0 ) ? - atan2(y,x) : 0.0;

/* Derotate it out of the matrix. */
   eraRz ( *bz, r );

/* Solve for the remaining two angles. */
   y = r[0][2];
   x = r[2][2];
   *btheta = ( x != 0.0 || y != 0.0 ) ? - atan2(y,x) : 0.0;

   y = -r[1][0];
   x = r[1][1];
   *bzeta = ( x != 0.0 || y != 0.0 ) ? - atan2(y,x) : 0.0;

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
