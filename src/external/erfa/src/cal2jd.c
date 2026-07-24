#include "erfa.h"
#include "erfam.h"

int eraCal2jd(int iy, int im, int id, double *djm0, double *djm)
/*
**  - - - - - - - - - -
**   e r a C a l 2 j d
**  - - - - - - - - - -
**
**  Gregorian Calendar to Julian Date.
**
**  Given:
**     iy,im,id  int     year, month, day in Gregorian calendar (Note 1)
**
**  Returned:
**     djm0      double  MJD zero-point: always 2400000.5
**     djm       double  Modified Julian Date for 0 hrs
**
**  Returned (function value):
**               int     status:
**                           0 = OK
**                          -1 = bad year   (Note 3: JD not computed)
**                          -2 = bad month  (JD not computed)
**                          -3 = bad day    (JD computed)
**
**  Notes:
**
**  1) The algorithm used is valid from -4800 March 1, but this
**     implementation rejects dates before -4799 January 1.
**
**  2) The Julian Date is returned in two pieces, in the usual ERFA
**     manner, which is designed to preserve time resolution.  The
**     Julian Date is available as a single number by adding djm0 and
**     djm.
**
**  3) In early eras the conversion is from the "Proleptic Gregorian
**     Calendar";  no account is taken of the date(s) of adoption of
**     the Gregorian Calendar, nor is the AD/BC numbering convention
**     observed.
**
**  Reference:
**
**     Explanatory Supplement to the Astronomical Almanac,
**     P. Kenneth Seidelmann (ed), University Science Books (1992),
**     Section 12.92 (p604).
**
**  This revision:  2021 May 11
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/
{
   int j, ly, my;
   long iypmy;

/* Earliest year allowed (4800BC) */
   const int IYMIN = -4799;

/* Month lengths in days */
   static const int mtab[]
                     = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


/* Preset status. */
   j = 0;

/* Validate year and month. */
   if (iy < IYMIN) return -1;
   if (im < 1 || im > 12) return -2;

/* If February in a leap year, 1, otherwise 0. */
   ly = ((im == 2) && !(iy%4) && (iy%100 || !(iy%400)));

/* Validate day, taking into account leap years. */
   if ( (id < 1) || (id > (mtab[im-1] + ly))) j = -3;

/* Return result. */
   my = (im - 14) / 12;
   iypmy = (long) (iy + my);
   *djm0 = ERFA_DJM0;
   *djm = (double)((1461L * (iypmy + 4800L)) / 4L
                 + (367L * (long) (im - 2 - 12 * my)) / 12L
                 - (3L * ((iypmy + 4900L) / 100L)) / 4L
                 + (long) id - 2432076L);

/* Return status. */
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
