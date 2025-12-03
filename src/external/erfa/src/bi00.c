#include "erfa.h"
#include "erfam.h"

void eraBi00(double *dpsibi, double *depsbi, double *dra)
/*
**  - - - - - - - -
**   e r a B i 0 0
**  - - - - - - - -
**
**  Frame bias components of IAU 2000 precession-nutation models;  part
**  of the Mathews-Herring-Buffett (MHB2000) nutation series, with
**  additions.
**
**  Returned:
**     dpsibi,depsbi  double  longitude and obliquity corrections
**     dra            double  the ICRS RA of the J2000.0 mean equinox
**
**  Notes:
**
**  1) The frame bias corrections in longitude and obliquity (radians)
**     are required in order to correct for the offset between the GCRS
**     pole and the mean J2000.0 pole.  They define, with respect to the
**     GCRS frame, a J2000.0 mean pole that is consistent with the rest
**     of the IAU 2000A precession-nutation model.
**
**  2) In addition to the displacement of the pole, the complete
**     description of the frame bias requires also an offset in right
**     ascension.  This is not part of the IAU 2000A model, and is from
**     Chapront et al. (2002).  It is returned in radians.
**
**  3) This is a supplemented implementation of one aspect of the IAU
**     2000A nutation model, formally adopted by the IAU General
**     Assembly in 2000, namely MHB2000 (Mathews et al. 2002).
**
**  References:
**
**     Chapront, J., Chapront-Touze, M. & Francou, G., Astron.
**     Astrophys., 387, 700, 2002.
**
**     Mathews, P.M., Herring, T.A., Buffet, B.A., "Modeling of nutation
**     and precession:  New nutation series for nonrigid Earth and
**     insights into the Earth's interior", J.Geophys.Res., 107, B4,
**     2002.  The MHB2000 code itself was obtained on 2002 September 9
**     from ftp://maia.usno.navy.mil/conv2000/chapter5/IAU2000A.
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* The frame bias corrections in longitude and obliquity */
   const double DPBIAS = -0.041775  * ERFA_DAS2R,
                DEBIAS = -0.0068192 * ERFA_DAS2R;

/* The ICRS RA of the J2000.0 equinox (Chapront et al., 2002) */
   const double DRA0 = -0.0146 * ERFA_DAS2R;


/* Return the results (which are fixed). */
   *dpsibi = DPBIAS;
   *depsbi = DEBIAS;
   *dra = DRA0;

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
