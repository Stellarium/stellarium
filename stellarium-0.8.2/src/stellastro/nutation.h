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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

Copyright (C) 2000 Liam Girdwood <liam@nova-ioe.org>

*/

/* taken from table 21.A pg 133 */

#ifndef LN_NUTATION_H
#define LN_NUTATION_H

 /* Nutation is a period oscillation of the Earths rotational axis around it's mean position.*/

// Contains Nutation in longitude, obliquity and ecliptic obliquity.
// Angles are expressed in degrees.
struct ln_nutation
{
	double longitude;	/*!< Nutation in longitude */
	double obliquity;	/*!< Nutation in obliquity */
	double ecliptic;	/*!< Obliquity of the ecliptic */
};

struct nutation_arguments
{
    double D;
    double M;
    double MM;
    double F;
    double O;
};

struct nutation_coefficients
{
    double longitude1;
    double longitude2;
    double obliquity1;
    double obliquity2;
};

/* Calculate nutation of longitude and obliquity in degrees from Julian Ephemeris Day
 * params : JD Julian Day, nutation Pointer to store nutation.
 * Chapter 21 pg 131-134 Using Table 21A */
void get_nutation (double JD, struct ln_nutation * nutation);


#endif
