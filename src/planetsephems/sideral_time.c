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

#include <math.h>

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

#define TERMS 63
#define LN_NUTATION_EPOCH_THRESHOLD 0.1


/* Nutation is a period oscillation of the Earths rotational axis around it's mean position.*/

/*
 Contains Nutation in longitude, obliquity and ecliptic obliquity.
 Angles are expressed in degrees.
*/
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

/* arguments and coefficients taken from table 21A on page 133 */

static const struct nutation_arguments arguments[TERMS] = {
	{0.0,	0.0,	0.0,	0.0,	1.0},
	{-2.0,	0.0,	0.0,	2.0,	2.0},
	{0.0,	0.0,	0.0,	2.0,	2.0},
	{0.0,	0.0,	0.0,	0.0,	2.0},
	{0.0,	1.0,	0.0,	0.0,	0.0},
	{0.0,	0.0,	1.0,	0.0,	0.0},
	{-2.0,	1.0,	0.0,	2.0,	2.0},
	{0.0,	0.0,	0.0,	2.0,	1.0},
	{0.0,	0.0,	1.0,	2.0,	2.0},
	{-2.0,	-1.0,	0.0,	2.0,	2.0},
	{-2.0,	0.0,	1.0,	0.0,	0.0},
	{-2.0,	0.0,	0.0,	2.0,	1.0},
	{0.0,	0.0,	-1.0,	2.0,	2.0},
	{2.0,	0.0,	0.0,	0.0,	0.0},
	{0.0,	0.0,	1.0,	0.0,	1.0},
	{2.0,	0.0,	-1.0,	2.0,	2.0},
	{0.0,	0.0,	-1.0,	0.0,	1.0},
	{0.0,	0.0,	1.0,	2.0,	1.0},
	{-2.0,	0.0,	2.0,	0.0,	0.0},
	{0.0,	0.0,	-2.0,	2.0,	1.0},
	{2.0,	0.0,	0.0,	2.0,	2.0},
	{0.0,	0.0,	2.0,	2.0,	2.0},
	{0.0,	0.0,	2.0,	0.0,	0.0},
	{-2.0,	0.0,	1.0,	2.0,	2.0},
	{0.0,	0.0,	0.0,	2.0,	0.0},
	{-2.0,	0.0,	0.0,	2.0,	0.0},
	{0.0,	0.0,	-1.0,	2.0,	1.0},
	{0.0,	2.0,	0.0,	0.0,	0.0},
	{2.0,	0.0,	-1.0,	0.0,	1.0},
	{-2.0,	2.0,	0.0,	2.0,	2.0},
	{0.0,	1.0,	0.0,	0.0,	1.0},
	{-2.0,	0.0,	1.0,	0.0,	1.0},
	{0.0,	-1.0,	0.0,	0.0,	1.0},
	{0.0,	0.0,	2.0,	-2.0,	0.0},
	{2.0,	0.0,	-1.0,	2.0,	1.0},
	{2.0,	0.0,	1.0,	2.0,	2.0},
	{0.0,	1.0,	0.0,	2.0,	2.0},
	{-2.0,	1.0,	1.0,	0.0,	0.0},
	{0.0,	-1.0,	0.0,	2.0,	2.0},
	{2.0,	0.0,	0.0,	2.0,	1.0},
	{2.0,	0.0,	1.0,	0.0,	0.0},
	{-2.0,	0.0,	2.0,	2.0,	2.0},
	{-2.0,	0.0,	1.0,	2.0,	1.0},
	{2.0,	0.0,	-2.0,	0.0,	1.0},
	{2.0,	0.0,	0.0,	0.0,	1.0},
	{0.0,	-1.0,	1.0,	0.0,	0.0},
	{-2.0,	-1.0,	0.0,	2.0,	1.0},
	{-2.0,	0.0,	0.0,	0.0,	1.0},
	{0.0,	0.0,	2.0,	2.0,	1.0},
	{-2.0,	0.0,	2.0,	0.0,	1.0},
	{-2.0,	1.0,	0.0,	2.0,	1.0},
	{0.0,	0.0,	1.0,	-2.0,	0.0},
	{-1.0,	0.0,	1.0,	0.0,	0.0},
	{-2.0,	1.0,	0.0,	0.0,	0.0},
	{1.0,	0.0,	0.0,	0.0,	0.0},
	{0.0,	0.0,	1.0,	2.0,	0.0},
	{0.0,	0.0,	-2.0,	2.0,	2.0},
	{-1.0,	-1.0,	1.0,	0.0,	0.0},
	{0.0,	1.0,	1.0,	0.0,	0.0},
	{0.0,	-1.0,	1.0,	2.0,	2.0},
	{2.0,	-1.0,	-1.0,	2.0,	2.0},
	{0.0,	0.0,	3.0,	2.0,	2.0},
	{2.0,	-1.0,	0.0,	2.0,	2.0}};

static const struct nutation_coefficients coefficients[TERMS] = {
	{-171996.0,	-174.2,	92025.0,8.9},
	{-13187.0,	-1.6,  	5736.0,	-3.1},
	{-2274.0, 	 0.2,  	977.0,	-0.5},
	{2062.0,   	0.2,    -895.0,    0.5},
	{1426.0,    -3.4,    54.0,    -0.1},
	{712.0,    0.1,    -7.0,    0.0},
	{-517.0,    1.2,    224.0,    -0.6},
	{-386.0,    -0.4,    200.0,    0.0},
	{-301.0,    0.0,    129.0,    -0.1},
	{217.0,    -0.5,    -95.0,    0.3},
	{-158.0,    0.0,    0.0,    0.0},
	{129.0,	0.1,	-70.0,	0.0},
	{123.0,	0.0,	-53.0,	0.0},
	{63.0,	0.0,	0.0,	0.0},
	{63.0,	1.0,	-33.0,	0.0},
	{-59.0,	0.0,	26.0,	0.0},
	{-58.0,	-0.1,	32.0,	0.0},
	{-51.0,	0.0,	27.0,	0.0},
	{48.0,	0.0,	0.0,	0.0},
	{46.0,	0.0,	-24.0,	0.0},
	{-38.0,	0.0,	16.0,	0.0},
	{-31.0,	0.0,	13.0,	0.0},
	{29.0,	0.0,	0.0,	0.0},
	{29.0,	0.0,	-12.0,	0.0},
	{26.0,	0.0,	0.0,	0.0},
	{-22.0,	0.0,	0.0,	0.0},
	{21.0,	0.0,	-10.0,	0.0},
	{17.0,	-0.1,	0.0,	0.0},
	{16.0,	0.0,	-8.0,	0.0},
	{-16.0,	0.1,	7.0,	0.0},
	{-15.0,	0.0,	9.0,	0.0},
	{-13.0,	0.0,	7.0,	0.0},
	{-12.0,	0.0,	6.0,	0.0},
	{11.0,	0.0,	0.0,	0.0},
	{-10.0,	0.0,	5.0,	0.0},
	{-8.0,	0.0,	3.0,	0.0},
	{7.0,	0.0,	-3.0,	0.0},
	{-7.0,	0.0,	0.0,	0.0},
	{-7.0,	0.0,	3.0,	0.0},
	{-7.0,	0.0,	3.0,	0.0},
	{6.0,	0.0,	0.0,	0.0},
	{6.0,	0.0,	-3.0,	0.0},
	{6.0,	0.0,	-3.0,	0.0},
	{-6.0,	0.0,	3.0,	0.0},
	{-6.0,	0.0,	3.0,	0.0},
	{5.0,	0.0,	0.0,	0.0},
	{-5.0,	0.0,	3.0,	0.0},
	{-5.0,	0.0,	3.0,	0.0},
	{-5.0,	0.0,	3.0,	0.0},
	{4.0,	0.0,	0.0,	0.0},
	{4.0,	0.0,	0.0,	0.0},
	{4.0,	0.0,	0.0,	0.0},
	{-4.0,	0.0,	0.0,	0.0},
	{-4.0,	0.0,	0.0,	0.0},
	{-4.0,	0.0,	0.0,	0.0},
	{3.0,	0.0,	0.0,	0.0},
	{-3.0,	0.0,	0.0,	0.0},
	{-3.0,	0.0,	0.0,	0.0},
	{-3.0,	0.0,	0.0,	0.0},
	{-3.0,	0.0,	0.0,	0.0},
	{-3.0,	0.0,	0.0,	0.0},
	{-3.0,	0.0,	0.0,	0.0},
	{-3.0,	0.0,	0.0,	0.0}};

/* cache values */
static double c_JD = 0.0, c_longitude = 0.0, c_obliquity = 0.0, c_ecliptic = 0.0; 


/* Calculate nutation of longitude and obliquity in degrees from Julian Ephemeris Day
* params : JD Julian Day, nutation Pointer to store nutation.
* Chapter 21 pg 131-134 Using Table 21A */
void get_nutation (double JD, struct ln_nutation * nutation)
{

	double D,M,MM,F,O,T,T2,T3;
	double coeff_sine, coeff_cos;
	int i;

	/* should we bother recalculating nutation */
	if (fabs(JD - c_JD) > LN_NUTATION_EPOCH_THRESHOLD)
	{
		/* set the new epoch */
		c_JD = JD;

		/* set ecliptic */
		c_ecliptic = 23.0 + 26.0 / 60.0 + 27.407 / 3600.0;

		/* calc T */
		T = (JD - 2451545.0)/36525;
		T2 = T * T;
		T3 = T2 * T;

		/* calculate D,M,M',F and Omega */
		D = 297.85036 + 445267.111480 * T - 0.0019142 * T2 + T3 / 189474.0;
		M = 357.52772 + 35999.050340 * T - 0.0001603 * T2 - T3 / 300000.0;
		MM = 134.96298 + 477198.867398 * T + 0.0086972 * T2 + T3 / 56250.0;
		F = 93.2719100 + 483202.017538 * T - 0.0036825 * T2 + T3 / 327270.0;
		O = 125.04452 - 1934.136261 * T + 0.0020708 * T2 + T3 / 450000.0;

		/* convert to radians */
		D *= M_PI/180.;
		M *= M_PI/180.;
		MM *= M_PI/180.;
		F *= M_PI/180.;
		O *= M_PI/180.;

		/* calc sum of terms in table 21A */
		for (i=0; i< TERMS; i++)
		{
			/* calc coefficients of sine and cosine */
			coeff_sine = coefficients[i].longitude1 + (coefficients[i].longitude2 * T);
			coeff_cos = coefficients[i].obliquity1 + (coefficients[i].obliquity2 * T);
	
			/* sum the arguments */
			if (arguments[i].D != 0)
			{
				c_longitude += coeff_sine * (sin (arguments[i].D * D));
				c_obliquity += coeff_cos * (cos (arguments[i].D * D));
			}
			if (arguments[i].M != 0)
			{
				c_longitude += coeff_sine * (sin (arguments[i].M * M));
				c_obliquity += coeff_cos * (cos (arguments[i].M * M));
			}
			if (arguments[i].MM != 0)
			{
				c_longitude += coeff_sine * (sin (arguments[i].MM * MM));
				c_obliquity += coeff_cos * (cos (arguments[i].MM * MM));
			}
			if (arguments[i].F != 0)
			{
				c_longitude += coeff_sine * (sin (arguments[i].F * F));
				c_obliquity += coeff_cos * (cos (arguments[i].F * F));
			}
			if (arguments[i].O != 0)
			{
				c_longitude += coeff_sine * (sin (arguments[i].O * O));
				c_obliquity += coeff_cos * (cos (arguments[i].O * O));
			}
		}    

		/* change to degrees */
		c_longitude /= 36000000.;
		c_obliquity /= 36000000.;

		c_ecliptic += c_obliquity;
	}

	/* return results */
	nutation->longitude = c_longitude;
	nutation->obliquity = c_obliquity;
	nutation->ecliptic = c_ecliptic;
}

/* Calculate the mean sidereal time at the meridian of Greenwich of a given date.
 * returns apparent sidereal time (degree).
 * Formula 11.1, 11.4 pg 83 */
double get_mean_sidereal_time (double JD)
{
    double sidereal;
    double T;
    
    T = (JD - 2451545.0) / 36525.0;
        
    /* calc mean angle */
    sidereal = 280.46061837 + (360.98564736629 * (JD - 2451545.0)) + (0.000387933 * T * T) - (T * T * T / 38710000.0);
    
    /* add a convenient multiple of 360 degrees */
    sidereal = range_degrees (sidereal);

    return sidereal;
} 


/* Calculate the apparent sidereal time at the meridian of Greenwich of a given date.
 * returns apparent sidereal time (degree).
 * Formula 11.1, 11.4 pg 83 */
double get_apparent_sidereal_time (double JD)
{
   double correction, sidereal;
   struct ln_nutation nutation;  
   
   /* get the mean sidereal time */
   sidereal = get_mean_sidereal_time (JD);
        
   /* add corrections for nutation in longitude and for the true obliquity of 
   the ecliptic */   
   get_nutation (JD, &nutation); 
    
   correction = (nutation.longitude * cos (nutation.obliquity*M_PI/180.));

   sidereal += correction;
   
   return (sidereal);
}
