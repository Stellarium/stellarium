#include "erfa.h"
#include "erfam.h"

double eraGmst06(double uta, double utb, double tta, double ttb)
/*
**  - - - - - - - - - -
**   e r a G m s t 0 6
**  - - - - - - - - - -
**
**  Greenwich mean sidereal time (consistent with IAU 2006 precession).
**
**  Given:
**     uta,utb    double    UT1 as a 2-part Julian Date (Notes 1,2)
**     tta,ttb    double    TT as a 2-part Julian Date (Notes 1,2)
**
**  Returned (function value):
**                double    Greenwich mean sidereal time (radians)
**
**  Notes:
**
**  1) The UT1 and TT dates uta+utb and tta+ttb respectively, are both
**     Julian Dates, apportioned in any convenient way between the
**     argument pairs.  For example, JD=2450123.7 could be expressed in
**     any of these ways, among others:
**
**            Part A        Part B
**
**         2450123.7           0.0       (JD method)
**         2451545.0       -1421.3       (J2000 method)
**         2400000.5       50123.2       (MJD method)
**         2450123.5           0.2       (date & time method)
**
**     The JD method is the most natural and convenient to use in
**     cases where the loss of several decimal digits of resolution
**     is acceptable (in the case of UT;  the TT is not at all critical
**     in this respect).  The J2000 and MJD methods are good compromises
**     between resolution and convenience.  For UT, the date & time
**     method is best matched to the algorithm that is used by the Earth
**     rotation angle function, called internally:  maximum precision is
**     delivered when the uta argument is for 0hrs UT1 on the day in
**     question and the utb argument lies in the range 0 to 1, or vice
**     versa.
**
**  2) Both UT1 and TT are required, UT1 to predict the Earth rotation
**     and TT to predict the effects of precession.  If UT1 is used for
**     both purposes, errors of order 100 microarcseconds result.
**
**  3) This GMST is compatible with the IAU 2006 precession and must not
**     be used with other precession models.
**
**  4) The result is returned in the range 0 to 2pi.
**
**  Called:
**     eraEra00     Earth rotation angle, IAU 2000
**     eraAnp       normalize angle into range 0 to 2pi
**
**  Reference:
**
**     Capitaine, N., Wallace, P.T. & Chapront, J., 2005,
**     Astron.Astrophys. 432, 355
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double t, gmst;


/* TT Julian centuries since J2000.0. */
   t = ((tta - ERFA_DJ00) + ttb) / ERFA_DJC;

/* Greenwich mean sidereal time, IAU 2006. */
   gmst = eraAnp(eraEra00(uta, utb) +
                  (    0.014506     +
                  (  4612.156534    +
                  (     1.3915817   +
                  (    -0.00000044  +
                  (    -0.000029956 +
                  (    -0.0000000368 )
          * t) * t) * t) * t) * t) * ERFA_DAS2R);

   return gmst;

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
