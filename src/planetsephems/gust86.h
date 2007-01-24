/************************************************************************

COMPUTATION OF THE COORDINATES OF THE URANIAN SATELLITES (GUST86),
version 0.1 (1988,1995) by LASKAR J. and JACOBSON, R. can be found at
ftp://ftp.imcce.fr/pub/ephem/satel/gust86

I (Johannes Gajdosik) have just taken the Fortran code and data
obtained from above and rearranged it into this piece of software.

I can neigther allow nor forbid the usage of the GUST86 theory.
The copyright notice below covers not the works of LASKAR J. and JACOBSON, R.,
but just my work, that is the compilation of the GUST86 theory
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

My implementation of GUST86 has the following modifications:
1) Rotate results to "dynamical equinox and ecliptic J2000",
   the reference frame of VSOP87 and VSOP87A:
   The rotation matrix Gust86ToJ2000 can be derived from gust86.f,
   the rotation J2000ToVsop87 can be derived from vsop87.doc.
2) units used in calculations: julian day, AU, rad
3) use the same function EllipticToRectangular that I use in TASS17.
4) calculate the orbital elements not for every new jd but rather reuse
   the previousely calculated elements if possible

****************************************************************/


#ifndef _GUST86_H_
#define _GUST86_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GUST86_MIRANDA   0
#define GUST86_ARIEL     1
#define GUST86_UMBRIEL   2
#define GUST86_TITANIA   3
#define GUST86_OBERON    4

void GetGust86Coor(double jd,int body,double *xyz);
  /* Return the rectangular coordinates of the given satellite
     and the given julian date jd expressed in dynamical time (TAI+32.184s).
     The origin of the xyz-coordinates is the center of the planet.
     The reference frame is "dynamical equinox and ecliptic J2000",
     which is the reference frame in VSOP87 and VSOP87A.

     According to vsop87.doc VSOP87 coordinates can be transformed to
     FK5 (=J2000=ICRF for our accuracy requirements) by
       X       cos(psi) -sin(psi) 0   1        0         0   X
       Y     = sin(psi)  cos(psi) 0 * 0 cos(eps) -sin(eps) * Y
       Z FK5          0         0 1   0 sin(eps)  cos(eps)   Z VSOP87
     with psi = -0.0000275 degrees = -0.099 arcsec and
     eps = 23.4392803055556 degrees = 23d26m21.4091sec.

     http://ssd.jpl.nasa.gov/horizons_doc.html#frames says:
       "J2000" selects an Earth Mean-Equator and dynamical Equinox of
       Epoch J2000.0 inertial reference system, where the Epoch of J2000.0
       is the Julian date 2451545.0. "Mean" indicates nutation effects are
       ignored in the frame definition. The system is aligned with the
       IAU-sponsored J2000 frame of the Radio Source Catalog of the
       International Earth Rotational Service (ICRF).
       The ICRF is thought to differ from FK5 by at most 0.01 arcsec.

     From this I conclude that in the context of stellarium
     ICRF, J2000 and FK5 are the same, while the transformation
     ICRF <-> VSOP87 must be done with the matrix given above.
   */
     
void GetGust86OsculatingCoor(double jd0,double jd,int body,double *xyz);
  /* The oculating orbit of epoch jd0, evatuated at jd, is returned.
  */

#ifdef __cplusplus
}
#endif

#endif
