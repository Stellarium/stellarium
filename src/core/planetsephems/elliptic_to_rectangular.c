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

#include "elliptic_to_rectangular.h"

#include <math.h>

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif


/*
   Given the orbital elements at some time t0 calculate the
   rectangular coordinates at time (t0+dt).

   mu = G*(m1+m2) .. gravitational constant of the two body problem
   a .. semi major axis
   n = mean motion = 2*M_PI/(orbit period)

   elem[0] .. irrelevant (either a (called by EllipticToRectangularA()) or n (called by EllipticToRectangularN())
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
*/
static void
EllipticToRectangular(const double a,const double n,
                      const double elem[6],const double dt,double xyz[]) {
  const double L = fmod(elem[1]+n*dt,2.0*M_PI);
    /* solve Keplers equation
        x = L - elem[2]*sin(x) + elem[3]*cos(x)
      not by trivially iterating
        x_0 = L
        x_{j+1} = L - elem[2]*sin(x_j) + elem[3]*cos(x_j)
      but instead by Newton approximation:
        0 = f(x) = x - L - elem[2]*sin(x) + elem[3]*cos(x)
        f'(x) = 1 - elem[2]*cos(x) - elem[3]*sin(x)
        x_0 = L or whatever, perhaps first step of trivial iteration
        x_{j+1} = x_j - f(x_j)/f'(x_j)
    */
  double Le = L - elem[2]*sin(L) + elem[3]*cos(L);
  for (;;) {
    const double cLe = cos(Le);
    const double sLe = sin(Le);
      /* for excentricity < 1 we have denominator > 0 */
    const double dLe = (L - Le + elem[2]*sLe - elem[3]*cLe)
                     / (1.0    - elem[2]*cLe - elem[3]*sLe);
    Le += dLe;
    if (fabs(dLe) <= 1e-14) break; /* L1: <1e-12 */
  }

  {
    const double cLe = cos(Le);
    const double sLe = sin(Le);

    const double dlf = -elem[2]*sLe + elem[3]*cLe;
    const double phi = sqrt(1.0 - elem[2]*elem[2] - elem[3]*elem[3]);
    const double psi = 1.0 / (1.0 + phi);

    const double x1 = a * (cLe - elem[2] - psi*dlf*elem[3]);
    const double y1 = a * (sLe - elem[3] + psi*dlf*elem[2]);

    const double elem_4q = elem[4] * elem[4]; // Q²
    const double elem_5q = elem[5] * elem[5]; // P²
    const double dwho = 2.0 * sqrt(1.0 - elem_4q - elem_5q);
    const double rtp = 1.0 - elem_5q - elem_5q;
    const double rtq = 1.0 - elem_4q - elem_4q;
    const double rdg = 2.0 * elem[5] * elem[4];

    xyz[0] = x1 * rtp + y1 * rdg;
    xyz[1] = x1 * rdg + y1 * rtq;
    xyz[2] = (-x1 * elem[5] + y1 * elem[4]) * dwho;

// /* GZ 2017-11: Re-enable these lines, they seem to be velocity!
    const double rsam1 = -elem[2]*cLe - elem[3]*sLe;
    const double h = a*n / (1.0 + rsam1);
    const double vx1 = h * (-sLe - psi*rsam1*elem[3]);
    const double vy1 = h * ( cLe + psi*rsam1*elem[2]);

    xyz[3] = vx1 * rtp + vy1 * rdg;
    xyz[4] = vx1 * rdg + vy1 * rtq;
    xyz[5] = (-vx1 * elem[5] + vy1 * elem[4]) * dwho;
// */
  }
}

void EllipticToRectangularN(double mu,const double elem[6],double dt,
                            double xyz[]) {
  const double n = elem[0];
#if defined __USE_MISC || defined __USE_XOPEN_EXTENDED || defined __USE_ISOC99
    /* linux math.h declares cbrt: */
  const double a = cbrt(mu/(n*n));
#else
  const double a = exp(log(mu/(n*n))/3.0);
#endif
  EllipticToRectangular(a,n,elem,dt,xyz);
}

void EllipticToRectangularA(double mu,const double elem[6],double dt,
                            double xyz[]) {
  const double a = elem[0];
  const double n = sqrt(mu/(a*a*a)); // mean motion
  EllipticToRectangular(a,n,elem,dt,xyz);
}

