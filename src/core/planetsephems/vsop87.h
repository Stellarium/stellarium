/************************************************************************

The PLANETARY SOLUTION VSOP87 by Bretagnon P. and Francou G. can be found at
ftp://ftp.imcce.fr/pub/ephem/planets/vsop87

I (Johannes Gajdosik) have just taken the data obtained from above
(VSOP87.mer,...,VSOP87.nep) and rearranged it into this piece of software.

I can neigther allow nor forbid the usage of VSOP87.
The copyright notice below covers not the work of Bretagnon P. and Francou G.
but just my work, that is the compilation of the VSOP87 data
into the software supplied in this file.


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

This is my implementation of the VSOP87 planetary solution.
I tried to optimize for speed by rearranging the terms so that
for a given touple of (a[0],a[1],...,a[11]) the values
  (cos,sin)(a[0]*lambda[0](T)+...a[11]*lambda[11](T))
have only to be calculated once.
Furthermore I used the addition formulas
  (cos,sin)(x+y) = ...
so that for given T the functions cos and sin have only to be called 12 times.

****************************************************************/


#ifndef _VSOP87_H_
#define _VSOP87_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VSOP87_MERCURY  0
#define VSOP87_VENUS    1
#define VSOP87_EMB      2
#define VSOP87_MARS     3
#define VSOP87_JUPITER  4
#define VSOP87_SATURN   5
#define VSOP87_URANUS   6
#define VSOP87_NEPTUNE  7

void GetVsop87Coor(double jd,int body,double *xyz);
  /* Return the rectangular coordinates of the given planet
     and the given julian date jd expressed in dynamical time (TAI+32.184s).
     The origin of the xyz-coordinates is the center of the sun.
     The reference frame is "dynamical equinox and ecliptic J2000",
     which is the reference frame in VSOP87 and VSOP87A.
  */

void GetVsop87OsculatingCoor(double jd0,double jd,int body,double *xyz);
  /* The oculating orbit of epoch jd0, evatuated at jd, is returned.
  */

#ifdef __cplusplus
}
#endif

#endif
