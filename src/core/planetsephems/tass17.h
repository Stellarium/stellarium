/************************************************************************

The TASS 1.7 theory of the Saturnian satellites including HYPERION
by Alain VIENNE and Luc DURIEZ can be found at
ftp://ftp.imcce.fr/pub/ephem/satel/tass17

I (Johannes Gajdosik) have just taken the Fortran code and data
obtained from above and rearranged it into this piece of software.

I can neither allow nor forbid the usage of the TASS 1.7 theory.
The copyright notice below covers not the work of Alain VIENNE and Luc DURIEZ
but just my work, that is the compilation of the TASS 1.7 theory
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

This is an implementation of the TASS 1.7 theory.
My modifications are:
1) do not calculate constant terms at runtime but beforehand
2) unite terms with the same frequencies
3) rearrange the terms so that calculation of the argument becomes easier
4) substitute so that the independent variable becomes T=jd-2444240
   for all satellites (including hyperion)
5) do not calculate a satellites "rmu" at runtime but beforehand
6) use a rotation matrix for Transformation to J2000
   instead of AIA, OMA and inclination of earth axis
7) exchange indices of hyperion and iapetus
8) calculate the orbital elements not for every new jd but rather reuse
   the previousely calculated elements if possible

****************************************************************/


#ifndef TASS17_H
#define TASS17_H

#ifdef __cplusplus
extern "C" {
#endif

#define TASS17_MIMAS     0
#define TASS17_ENCELADUS 1
#define TASS17_TETHYS    2
#define TASS17_DIONE     3
#define TASS17_RHEA      4
#define TASS17_TITAN     5
#define TASS17_HYPERION  7
#define TASS17_IAPETUS   6

// xyz and xyzdot are 3-vectors (position&speed)
void GetTass17Coor(double jd, int body, double *xyz, double *xyzdot);
// xyz is a 6-vector (position&speed)
void GetTass17OsculatingCoor(const double jd0,const double jd, const int body,double *xyz);

#ifdef __cplusplus
}
#endif

#endif
