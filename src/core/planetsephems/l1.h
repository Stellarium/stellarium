/************************************************************************

The L1 theory of the galilean satellites
by Valery Lainey can be found at
ftp://ftp.imcce.fr/pub/ephem/satel/galilean/L1

I (Johannes Gajdosik) have just taken the Fortran code and data
obtained from above and rearranged it into this piece of software.

I can neigther allow nor forbid the usage of the L1 theory.
The copyright notice below covers not the work of Valery Lainey
but just my work, that is the compilation of the L1 theory
into the software supplied in this file.


Copyright (c) 2005 Johannes Gajdosik

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

This is an implementation of the L1 theory, V1.1.
My modifications are:
1) do not calculate constant terms at runtime but beforehand
2) unite terms with the same frequencies
3) rearrange the terms so that calculation of the argument becomes easier
4) substitute so that the independent variable becomes T=jd-2433282.5
5) artificially blend out the usage of polynomials outside [-695+t0,695+t0]
   in order to avoid a discontinuity

****************************************************************/

#ifndef _L1_H_
#define _L1_H_

#ifdef __cplusplus
extern "C" {
#endif

#define L1_IO            0
#define L1_EUROPA        1
#define L1_GANYMEDE      2
#define L1_CALLISTO      3

void GetL1Coor(double jd,int body,double *xyz);
  /* Return the rectangular coordinates of the given satellite
     and the given julian date jd expressed in dynamical time (TAI+32.184s).
     The origin of the xyz-coordinates is the center of the planet.
     The reference frame is "dynamical equinox and ecliptic J2000",
     which is the reference frame in VSOP87 and VSOP87A.
  */

void GetL1OsculatingCoor(double jd0,double jd,int body,double *xyz);
  /* The oculating orbit of epoch jd0, evatuated at jd, is returned.
  */


#ifdef __cplusplus
}
#endif

#endif
