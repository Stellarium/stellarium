#include "erfa.h"
#include "erfam.h"

void eraPr00(double date1, double date2, double *dpsipr, double *depspr)
/*
**  - - - - - - - -
**   e r a P r 0 0
**  - - - - - - - -
**
**  Precession-rate part of the IAU 2000 precession-nutation models
**  (part of MHB2000).
**
**  Given:
**     date1,date2    double  TT as a 2-part Julian Date (Note 1)
**
**  Returned:
**     dpsipr,depspr  double  precession corrections (Notes 2,3)
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
**  2) The precession adjustments are expressed as "nutation
**     components", corrections in longitude and obliquity with respect
**     to the J2000.0 equinox and ecliptic.
**
**  3) Although the precession adjustments are stated to be with respect
**     to Lieske et al. (1977), the MHB2000 model does not specify which
**     set of Euler angles are to be used and how the adjustments are to
**     be applied.  The most literal and straightforward procedure is to
**     adopt the 4-rotation epsilon_0, psi_A, omega_A, xi_A option, and
**     to add dpsipr to psi_A and depspr to both omega_A and eps_A.
**
**  4) This is an implementation of one aspect of the IAU 2000A nutation
**     model, formally adopted by the IAU General Assembly in 2000,
**     namely MHB2000 (Mathews et al. 2002).
**
**  References:
**
**     Lieske, J.H., Lederle, T., Fricke, W. & Morando, B., "Expressions
**     for the precession quantities based upon the IAU (1976) System of
**     Astronomical Constants", Astron.Astrophys., 58, 1-16 (1977)
**
**     Mathews, P.M., Herring, T.A., Buffet, B.A., "Modeling of nutation
**     and precession   New nutation series for nonrigid Earth and
**     insights into the Earth's interior", J.Geophys.Res., 107, B4,
**     2002.  The MHB2000 code itself was obtained on 9th September 2002
**     from ftp://maia.usno.navy.mil/conv2000/chapter5/IAU2000A.
**
**     Wallace, P.T., "Software for Implementing the IAU 2000
**     Resolutions", in IERS Workshop 5.1 (2002).
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   double t;

/* Precession and obliquity corrections (radians per century) */
   static const double PRECOR = -0.29965 * ERFA_DAS2R,
                       OBLCOR = -0.02524 * ERFA_DAS2R;


/* Interval between fundamental epoch J2000.0 and given date (JC). */
   t = ((date1 - ERFA_DJ00) + date2) / ERFA_DJC;

/* Precession rate contributions with respect to IAU 1976/80. */
   *dpsipr = PRECOR * t;
   *depspr = OBLCOR * t;

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
