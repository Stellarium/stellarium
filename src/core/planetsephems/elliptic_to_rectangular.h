/************************************************************************

The code in this file is heavily inspired by the TASS17 and GUST86 theories
found on
ftp://ftp.imcce.fr/pub/ephem/satel

I (Johannes Gajdosik) have just taken the Fortran code and data
obtained from above and rearranged it into this piece of software.

I can neither allow nor forbid the above theories.
The copyright notice below covers just my work,
that is the compilation of the data obtained from above
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

****************************************************************/

#ifndef ELLIPTIC_TO_RECTANGULAR_H
#define ELLIPTIC_TO_RECTANGULAR_H

#ifdef __cplusplus
extern "C" {
#endif

/*
   Given the orbital elements at some time t0 calculate the
   rectangular coordinates at time (t0+dt).
   
   mu = G*(m1+m2) .. gravitational constant of the two body problem
   a .. semi major axis
   n = mean motion = 2*M_PI/(orbit period)
   
   elem[0] .. either a (EllipticToRectangularA()) or n (EllipticToRectangularN())
   elem[1] .. L
   elem[2] .. K=e*cos(Omega+omega)
   elem[3] .. H=e*sin(Omega+omega)
   elem[4] .. Q=sin(i/2)*cos(Omega)
   elem[5] .. P=sin(i/2)*sin(Omega)
   
   Omega = longitude of ascending node
   omega = argument of pericenter
   L = mean longitude = Omega + omega + M
   M = mean anomaly
   i = inclination
   e = excentricity
   
   Units are suspected to be: Julian days, AU, rad

   Results:
   xyz[0,1,2]=Position [AU]
   xyz[3,4,5]=Velocity [AU/d]

*/

void EllipticToRectangularN(double mu,const double elem[6],double dt,
                            double xyz[]);
void EllipticToRectangularA(double mu,const double elem[6],double dt,
                            double xyz[]);

#ifdef __cplusplus
}
#endif

#endif
