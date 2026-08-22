#include "erfa.h"

void eraH2fk5(double rh, double dh,
              double drh, double ddh, double pxh, double rvh,
              double *r5, double *d5,
              double *dr5, double *dd5, double *px5, double *rv5)
/*
**  - - - - - - - - -
**   e r a H 2 f k 5
**  - - - - - - - - -
**
**  Transform Hipparcos star data into the FK5 (J2000.0) system.
**
**  Given (all Hipparcos, epoch J2000.0):
**     rh      double    RA (radians)
**     dh      double    Dec (radians)
**     drh     double    proper motion in RA (dRA/dt, rad/Jyear)
**     ddh     double    proper motion in Dec (dDec/dt, rad/Jyear)
**     pxh     double    parallax (arcsec)
**     rvh     double    radial velocity (km/s, positive = receding)
**
**  Returned (all FK5, equinox J2000.0, epoch J2000.0):
**     r5      double    RA (radians)
**     d5      double    Dec (radians)
**     dr5     double    proper motion in RA (dRA/dt, rad/Jyear)
**     dd5     double    proper motion in Dec (dDec/dt, rad/Jyear)
**     px5     double    parallax (arcsec)
**     rv5     double    radial velocity (km/s, positive = receding)
**
**  Notes:
**
**  1) This function transforms Hipparcos star positions and proper
**     motions into FK5 J2000.0.
**
**  2) The proper motions in RA are dRA/dt rather than
**     cos(Dec)*dRA/dt, and are per year rather than per century.
**
**  3) The FK5 to Hipparcos transformation is modeled as a pure
**     rotation and spin;  zonal errors in the FK5 catalog are not
**     taken into account.
**
**  4) See also eraFk52h, eraFk5hz, eraHfk5z.
**
**  Called:
**     eraStarpv    star catalog data to space motion pv-vector
**     eraFk5hip    FK5 to Hipparcos rotation and spin
**     eraRv2m      r-vector to r-matrix
**     eraRxp       product of r-matrix and p-vector
**     eraTrxp      product of transpose of r-matrix and p-vector
**     eraPxp       vector product of two p-vectors
**     eraPmp       p-vector minus p-vector
**     eraPvstar    space motion pv-vector to star catalog data
**
**  Reference:
**
**     F.Mignard & M.Froeschle, Astron.Astrophys., 354, 732-739 (2000).
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   int i;
   double pvh[2][3], r5h[3][3], s5h[3], sh[3], wxp[3], vv[3], pv5[2][3];


/* Hipparcos barycentric position/velocity pv-vector (normalized). */
   eraStarpv(rh, dh, drh, ddh, pxh, rvh, pvh);

/* FK5 to Hipparcos orientation matrix and spin vector. */
   eraFk5hip(r5h, s5h);

/* Make spin units per day instead of per year. */
   for ( i = 0; i < 3; s5h[i++] /= 365.25 );

/* Orient the spin into the Hipparcos system. */
   eraRxp(r5h, s5h, sh);

/* De-orient the Hipparcos position into the FK5 system. */
   eraTrxp(r5h, pvh[0], pv5[0]);

/* Apply spin to the position giving an extra space motion component. */
   eraPxp(pvh[0], sh, wxp);

/* Subtract this component from the Hipparcos space motion. */
   eraPmp(pvh[1], wxp, vv);

/* De-orient the Hipparcos space motion into the FK5 system. */
   eraTrxp(r5h, vv, pv5[1]);

/* FK5 pv-vector to spherical. */
   eraPvstar(pv5, r5, d5, dr5, dd5, px5, rv5);

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
