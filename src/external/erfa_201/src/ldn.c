#include "erfa.h"
#include "erfam.h"

void eraLdn(int n, eraLDBODY b[], double ob[3], double sc[3],
            double sn[3])
/*+
**  - - - - - - -
**   e r a L d n
**  - - - - - - -
**
**  For a star, apply light deflection by multiple solar-system bodies,
**  as part of transforming coordinate direction into natural direction.
**
**  Given:
**     n    int           number of bodies (note 1)
**     b    eraLDBODY[n]  data for each of the n bodies (Notes 1,2):
**      bm   double         mass of the body (solar masses, Note 3)
**      dl   double         deflection limiter (Note 4)
**      pv   [2][3]         barycentric PV of the body (au, au/day)
**     ob   double[3]     barycentric position of the observer (au)
**     sc   double[3]     observer to star coord direction (unit vector)
**
**  Returned:
**     sn    double[3]      observer to deflected star (unit vector)
**
**  1) The array b contains n entries, one for each body to be
**     considered.  If n = 0, no gravitational light deflection will be
**     applied, not even for the Sun.
**
**  2) The array b should include an entry for the Sun as well as for
**     any planet or other body to be taken into account.  The entries
**     should be in the order in which the light passes the body.
**
**  3) In the entry in the b array for body i, the mass parameter
**     b[i].bm can, as required, be adjusted in order to allow for such
**     effects as quadrupole field.
**
**  4) The deflection limiter parameter b[i].dl is phi^2/2, where phi is
**     the angular separation (in radians) between star and body at
**     which limiting is applied.  As phi shrinks below the chosen
**     threshold, the deflection is artificially reduced, reaching zero
**     for phi = 0.   Example values suitable for a terrestrial
**     observer, together with masses, are as follows:
**
**        body i     b[i].bm        b[i].dl
**
**        Sun        1.0            6e-6
**        Jupiter    0.00095435     3e-9
**        Saturn     0.00028574     3e-10
**
**  5) For cases where the starlight passes the body before reaching the
**     observer, the body is placed back along its barycentric track by
**     the light time from that point to the observer.  For cases where
**     the body is "behind" the observer no such shift is applied.  If
**     a different treatment is preferred, the user has the option of
**     instead using the eraLd function.  Similarly, eraLd can be used
**     for cases where the source is nearby, not a star.
**
**  6) The returned vector sn is not normalized, but the consequential
**     departure from unit magnitude is always negligible.
**
**  7) The arguments sc and sn can be the same array.
**
**  8) For efficiency, validation is omitted.  The supplied masses must
**     be greater than zero, the position and velocity vectors must be
**     right, and the deflection limiter greater than zero.
**
**  Reference:
**
**     Urban, S. & Seidelmann, P. K. (eds), Explanatory Supplement to
**     the Astronomical Almanac, 3rd ed., University Science Books
**     (2013), Section 7.2.4.
**
**  Called:
**     eraCp        copy p-vector
**     eraPdp       scalar product of two p-vectors
**     eraPmp       p-vector minus p-vector
**     eraPpsp      p-vector plus scaled p-vector
**     eraPn        decompose p-vector into modulus and direction
**     eraLd        light deflection by a solar-system body
**
**  This revision:   2021 February 24
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
/* Light time for 1 au (days) */
   const double CR = ERFA_AULT/ERFA_DAYSEC;

   int i;
   double  v[3], dt, ev[3], em, e[3];


/* Star direction prior to deflection. */
   eraCp(sc, sn);

/* Body by body. */
   for ( i = 0; i < n; i++ ) {

   /* Body to observer vector at epoch of observation (au). */
      eraPmp ( ob, b[i].pv[0], v );

   /* Minus the time since the light passed the body (days). */
      dt = eraPdp(sn,v) * CR;

   /* Neutralize if the star is "behind" the observer. */
      dt = ERFA_GMIN(dt, 0.0);

   /* Backtrack the body to the time the light was passing the body. */
      eraPpsp(v, -dt, b[i].pv[1], ev);

   /* Body to observer vector as magnitude and direction. */
      eraPn(ev, &em, e);

   /* Apply light deflection for this body. */
      eraLd ( b[i].bm, sn, sn, e, em, b[i].dl, sn );

   /* Next body. */
   }

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
