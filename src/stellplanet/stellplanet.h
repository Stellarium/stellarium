/*
Copyright (C) 2000 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2003 Fabien Chéreau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _STELLPLANET_H_
#define _STELLPLANET_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Chapter 31 Pg 206-207 Equ 31.1 31.2 , 31.3 using VSOP 87
 * Calculate planets rectangular heliocentric ecliptical coordinates
 * for given julian day. Values are in UA.
 * params : Julian day, rect coords */
void get_mercury_helio_coords(double JD, double * X, double * Y, double * Z);
void get_venus_helio_coords(double JD, double * X, double * Y, double * Z);
void get_earth_helio_coords(double JD, double * X, double * Y, double * Z);
void get_mars_helio_coords(double JD, double * X, double * Y, double * Z);
void get_jupiter_helio_coords(double JD, double * X, double * Y, double * Z);
void get_saturn_helio_coords(double JD, double * X, double * Y, double * Z);
void get_uranus_helio_coords(double JD, double * X, double * Y, double * Z);
void get_neptune_helio_coords(double JD, double * X, double * Y, double * Z);
void get_pluto_helio_coords(double JD, double * X, double * Y, double * Z);

// Return 0 of course...
void get_sun_helio_coords(double JD, double * X, double * Y, double * Z) {*X=0.; *Y=0.; *Y=0.;};

/* Calculate the rectangular geocentric lunar coordinates to the inertial mean
 * ecliptic and equinox of J2000.
 * The geocentric coordinates returned are in units of UA.
 * The truncation level of the series in radians for longitude
 * and latitude and in km for distance. (Valid range 0 - 0.01, 0 being highest accuracy)
 * This function is based upon the Lunar Solution ELP2000-82B by
 * Michelle Chapront-Touze and Jean Chapront of the Bureau des Longitudes,
 * Paris. ELP 2000-82B theory
 * param JD Julian day, rect pos, precision */
void get_lunar_geo_posn_prec(double JD, double * X, double * Y, double * Z, double prcision);

/* Same with lowest precision by default */
void get_lunar_geo_posn(double JD, double * X, double * Y, double * Z);

#ifdef __cplusplus
};
#endif


#endif /* _STELLPLANET_H_ */
