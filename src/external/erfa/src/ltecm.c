#include "erfa.h"
#include "erfam.h"

void eraLtecm(double epj, double rm[3][3])
/*
**  - - - - - - - - -
**   e r a L t e c m
**  - - - - - - - - -
**
**  ICRS equatorial to ecliptic rotation matrix, long-term.
**
**  Given:
**     epj     double         Julian epoch (TT)
**
**  Returned:
**     rm      double[3][3]   ICRS to ecliptic rotation matrix
**
**  Notes:
**
**  1) The matrix is in the sense
**
**        E_ep = rm x P_ICRS,
**
**     where P_ICRS is a vector with respect to ICRS right ascension
**     and declination axes and E_ep is the same vector with respect to
**     the (inertial) ecliptic and equinox of epoch epj.
**
**  2) P_ICRS is a free vector, merely a direction, typically of unit
**     magnitude, and not bound to any particular spatial origin, such
**     as the Earth, Sun or SSB.  No assumptions are made about whether
**     it represents starlight and embodies astrometric effects such as
**     parallax or aberration.  The transformation is approximately that
**     between mean J2000.0 right ascension and declination and ecliptic
**     longitude and latitude, with only frame bias (always less than
**     25 mas) to disturb this classical picture.
**
**  3) The Vondrak et al. (2011, 2012) 400 millennia precession model
**     agrees with the IAU 2006 precession at J2000.0 and stays within
**     100 microarcseconds during the 20th and 21st centuries.  It is
**     accurate to a few arcseconds throughout the historical period,
**     worsening to a few tenths of a degree at the end of the
**     +/- 200,000 year time span.
**
**  Called:
**     eraLtpequ    equator pole, long term
**     eraLtpecl    ecliptic pole, long term
**     eraPxp       vector product
**     eraPn        normalize vector
**
**  References:
**
**    Vondrak, J., Capitaine, N. and Wallace, P., 2011, New precession
**    expressions, valid for long time intervals, Astron.Astrophys. 534,
**    A22
**
**    Vondrak, J., Capitaine, N. and Wallace, P., 2012, New precession
**    expressions, valid for long time intervals (Corrigendum),
**    Astron.Astrophys. 541, C1
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Frame bias (IERS Conventions 2010, Eqs. 5.21 and 5.33) */
   const double dx = -0.016617 * ERFA_DAS2R,
                de = -0.0068192 * ERFA_DAS2R,
                dr = -0.0146 * ERFA_DAS2R;

   double p[3], z[3], w[3], s, x[3], y[3];


/* Equator pole. */
   eraLtpequ(epj, p);

/* Ecliptic pole (bottom row of equatorial to ecliptic matrix). */
   eraLtpecl(epj, z);

/* Equinox (top row of matrix). */
   eraPxp(p, z, w);
   eraPn(w, &s, x);

/* Middle row of matrix. */
   eraPxp(z, x, y);

/* Combine with frame bias. */
   rm[0][0] =   x[0]    - x[1]*dr + x[2]*dx;
   rm[0][1] =   x[0]*dr + x[1]    + x[2]*de;
   rm[0][2] = - x[0]*dx - x[1]*de + x[2];
   rm[1][0] =   y[0]    - y[1]*dr + y[2]*dx;
   rm[1][1] =   y[0]*dr + y[1]    + y[2]*de;
   rm[1][2] = - y[0]*dx - y[1]*de + y[2];
   rm[2][0] =   z[0]    - z[1]*dr + z[2]*dx;
   rm[2][1] =   z[0]*dr + z[1]    + z[2]*de;
   rm[2][2] = - z[0]*dx - z[1]*de + z[2];

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
