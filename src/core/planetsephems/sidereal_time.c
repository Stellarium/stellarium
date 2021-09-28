/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Library General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA. 

Copyright (C) 2000 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2015 Georg Zotti (deactivated old IAU-1980 Nutation functions, update to IAU-2000B)

*/

#include <math.h>
#include <assert.h>
#include "precession.h"

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

/* puts a large angle in the correct range 0 - 360 degrees */
double range_degrees(double d)
{
	d = fmod( d, 360.);
	if(d<0.) d += 360.;
	return d;
}

/* puts a large angle in the correct range 0 - 2PI radians */
double range_radians (double r)
{
	r = fmod( r, 2.*M_PI );
	if (r<0.) r += 2.*M_PI;
	return r;
}

/* Calculate the mean sidereal time at the meridian of Greenwich (GMST) of a given date.
 * returns mean sidereal time (degrees).
 * Meeus, Astr. Algorithms, Formula 11.1, 11.4 pg 83. (or 2nd ed. 1998, 12.1, 12.4 pg.87)
 * MAKE SURE argument JD is UT, not TT!
 * GZfix for V0.14: Replace by expression (43) given in:
 * N. Capitaine, P.T.Wallace, J. Chapront: Expressions for IAU 2000 precession quantities.
 * A&A412, 567-586 (2003)
 * DOI: 10.1051/0004-6361:20031539
 */
double get_mean_sidereal_time (double JD, double JDE)
{
//	double sidereal;
//	double T, T1;
// 	T1 = (JD - 2451545.0);
//	T= T1 * (1.0/ 36525.0);
//
//	// calc mean angle. This is Meeus 11.4 (IAU 1982)
//      sidereal = 280.46061837 + (360.98564736629 * (JD - 2451545.0)) + (0.000387933 * T * T) - (T * T * T / 38710000.0);

	// TODO GZ: verify that GMST1980 is still OK
	//
	double sidereal, UT1, t, tu;

	UT1=(JD-floor(JD) +0.5) * 86400.; // time in seconds
	t=(JDE-2451545.)/36525.;
	tu=(JD-2451545.)/36525.;

	sidereal= (((-0.000000002454*t-0.00000199708)*t-0.0000002926)*t+0.092772110)*t*t;
	sidereal += (t-tu)*307.4771013;
	sidereal += 8640184.79447825*tu + 24110.5493771;
	sidereal += UT1;

	// this is expressed in seconds. We need degrees.
	// 1deg=4tempMin=240tempSec
	sidereal *= 1./240.;

	/* add again a convenient multiple of 360 degrees */
	sidereal = range_degrees (sidereal);

	return sidereal;
} 


/* Calculate the apparent sidereal time at the meridian of Greenwich of a given date.
 * returns apparent sidereal time (degrees).
 * Formula 11.1, 11.4 pg 83
 * GZ modified for V0.14 to use nutation IAU-2000B
 */
double get_apparent_sidereal_time (double JD, double JDE)
{
	double meanSidereal = get_mean_sidereal_time (JD, JDE);
        
	// add corrections for nutation in longitude and for the true obliquity of the ecliptic
	double deltaPsi, deltaEps;
	getNutationAngles(JDE, &deltaPsi, &deltaEps);

	return meanSidereal+ (deltaPsi*cos(getPrecessionAngleVondrakEpsilon(JDE) + deltaEps))*180./M_PI;
}

