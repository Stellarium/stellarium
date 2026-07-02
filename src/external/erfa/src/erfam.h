#ifndef ERFAMHDEF
#define ERFAMHDEF

/*
**  - - - - - - - -
**   e r f a m . h
**  - - - - - - - -
**
**  Macros used by ERFA library.
**
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  Derived, with permission, from the SOFA library.  See notes at end of file.
*/

/* Pi */
#define ERFA_DPI (3.141592653589793238462643)

/* 2Pi */
#define ERFA_D2PI (6.283185307179586476925287)

/* Radians to degrees */
#define ERFA_DR2D (57.29577951308232087679815)

/* Degrees to radians */
#define ERFA_DD2R (1.745329251994329576923691e-2)

/* Radians to arcseconds */
#define ERFA_DR2AS (206264.8062470963551564734)

/* Arcseconds to radians */
#define ERFA_DAS2R (4.848136811095359935899141e-6)

/* Seconds of time to radians */
#define ERFA_DS2R (7.272205216643039903848712e-5)

/* Arcseconds in a full circle */
#define ERFA_TURNAS (1296000.0)

/* Milliarcseconds to radians */
#define ERFA_DMAS2R (ERFA_DAS2R / 1e3)

/* Length of tropical year B1900 (days) */
#define ERFA_DTY (365.242198781)

/* Seconds per day. */
#define ERFA_DAYSEC (86400.0)

/* Days per Julian year */
#define ERFA_DJY (365.25)

/* Days per Julian century */
#define ERFA_DJC (36525.0)

/* Days per Julian millennium */
#define ERFA_DJM (365250.0)

/* Reference epoch (J2000.0), Julian Date */
#define ERFA_DJ00 (2451545.0)

/* Julian Date of Modified Julian Date zero */
#define ERFA_DJM0 (2400000.5)

/* Reference epoch (J2000.0), Modified Julian Date */
#define ERFA_DJM00 (51544.5)

/* 1977 Jan 1.0 as MJD */
#define ERFA_DJM77 (43144.0)

/* TT minus TAI (s) */
#define ERFA_TTMTAI (32.184)

/* Astronomical unit (m, IAU 2012) */
#define ERFA_DAU (149597870.7e3)

/* Speed of light (m/s) */
#define ERFA_CMPS 299792458.0

/* Light time for 1 au (s) */
#define ERFA_AULT (ERFA_DAU/ERFA_CMPS)

/* Speed of light (au per day) */
#define ERFA_DC (ERFA_DAYSEC/ERFA_AULT)

/* L_G = 1 - d(TT)/d(TCG) */
#define ERFA_ELG (6.969290134e-10)

/* L_B = 1 - d(TDB)/d(TCB), and TDB (s) at TAI 1977/1/1.0 */
#define ERFA_ELB (1.550519768e-8)
#define ERFA_TDB0 (-6.55e-5)

/* Schwarzschild radius of the Sun (au) */
/* = 2 * 1.32712440041e20 / (2.99792458e8)^2 / 1.49597870700e11 */
#define ERFA_SRS 1.97412574336e-8

/* ERFA_DINT(A) - truncate to nearest whole number towards zero (double) */
#define ERFA_DINT(A) ((A)<0.0?ceil(A):floor(A))

/* ERFA_DNINT(A) - round to nearest whole number (double) */
#define ERFA_DNINT(A) (fabs(A)<0.5?0.0\
                                :((A)<0.0?ceil((A)-0.5):floor((A)+0.5)))

/* ERFA_DSIGN(A,B) - magnitude of A with sign of B (double) */
#define ERFA_DSIGN(A,B) ((B)<0.0?-fabs(A):fabs(A))

/* max(A,B) - larger (most +ve) of two numbers (generic) */
#define ERFA_GMAX(A,B) (((A)>(B))?(A):(B))

/* min(A,B) - smaller (least +ve) of two numbers (generic) */
#define ERFA_GMIN(A,B) (((A)<(B))?(A):(B))

/* Reference ellipsoids */
#define ERFA_WGS84 1
#define ERFA_GRS80 2
#define ERFA_WGS72 3

#endif


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
