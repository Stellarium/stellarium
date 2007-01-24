/************************************************************************

The Ephemerides of the Martian satellites
(adjustement from 1877 to 2005, Version 1.0)
by Valery Lainey can be obtained from Valery Lainey: 

               V.Lainey (Lainey@oma.be)
ROB- 3, Avenue Circulaire, B-1180 Bruxelles (Belgium)
IMCCE - 77, Avenue Denfert-Rochereau 75014 Paris (France)

-----------------------------------------------------------------------

I (Johannes Gajdosik) have just taken Valery Laineys Fortran code,
MarsSatV1-0.f, which he kindly supplied, and rearranged it into
this piece of software.

I can neigther allow nor forbid the usage of Valery Laineys
Ephemerides of the Martian satellites.
The copyright notice below covers not the work of Valery Lainey
but just my work, that is the compilation of Valery Laineys
Ephemerides of the Martian satellites into the software supplied in this file.


Copyright (c) 2006 Johannes Gajdosik

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

My modifications to the "Ephemerides of the Martian satellites" as implemented
in MarsSatV1-0.f are
1) do not calculate constant terms at runtime but beforehand
2) unite terms with the same frequencies

****************************************************************/

#ifndef _MARS_SAT_H_
#define _MARS_SAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MARS_SAT_PHOBOS 0
#define MARS_SAT_DEIMOS 1

void GetMarsSatCoor(double jd,int body,double *xyz);
  /* Return the rectangular coordinates of the given satellite
     and the given julian date jd expressed in dynamical time (TAI+32.184s).
     The origin of the xyz-coordinates is the center of the planet.
     The reference frame is "dynamical equinox and ecliptic J2000",
     which is the reference frame in VSOP87 and VSOP87A.
  */

void GetMarsSatOsculatingCoor(double jd0,double jd,int body,double *xyz);
  /* The oculating orbit of epoch jd0, evatuated at jd, is returned.
  */

#ifdef __cplusplus
}
#endif

#endif
