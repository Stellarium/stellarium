/*
* Copyright (C) 1999, 2000 Juan Carlos Remis
* Copyright (C) 2002 Liam Girdwood
* Copyright (C) 2003 Fabien Chéreau
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stellastro.h"

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


/*----------------------------------------------------------------------------
 * The obliquity formula (and all the magic numbers below) come from Meeus,
 * Astro Algorithms.
 *
 * Input t is time in julian day.
 * Valid range is the years -8000 to +12000 (t = -100 to 100).
 *
 * return value is mean obliquity (epsilon sub 0) in degrees.
 */

double get_mean_obliquity( double t )
{
	double u, u0;
	static double t0 = 30000.;
	static double rval = 0.;
	static const double rvalStart = 23. * 3600. + 26. * 60. + 21.448;
	static const int OBLIQ_COEFFS = 10;
	static const double coeffs[ 10 ] = {
         -468093.,  -155.,  199925.,  -5138.,  -24967.,
         -3905.,    712.,   2787.,    579.,    245.};
	int i;
	t = ( t - J2000 ) / 36525.; // Convert time in centuries

	if( t0 != t )
	{
		t0 = t;
    	u = u0 = t / 100.;     // u is in julian 10000's of years
    	rval = rvalStart;
		for( i=0; i<OBLIQ_COEFFS; i++ )
      	{
        	rval += u * coeffs[i] / 100.;
        	u *= u0;
		}
      	// convert from seconds to degree
      	rval /= 3600.;
	}
	return rval;
}


// Obtains a ln_date from 2 strings s1 and s2 for date and time
// with the form dd/mm/yyyy for s1 and hh:mm:ss.s for s2.
// Returns NULL if s1 or s2 is not valid.
// Uses the current date if s1 is "today" and current time if s2 is "now"
const struct ln_date * str_to_date(const char * s1, const char * s2)
{
	static struct ln_date date;
	if (s1==NULL || s2==NULL) return NULL;
    if (!strcmp(s1,"today"))
	{
		struct ln_date tempDate;
		get_ln_date_from_sys(&tempDate);
		date.days = tempDate.days;
		date.months = tempDate.months;
		date.years = tempDate.years;
	}
	else
	{
		if (sscanf(s1,"%d:%d:%d",&(date.days),&(date.months),&(date.years))!=3)
			return NULL;
	}

    if (!strcmp(s2,"now"))
	{
		struct ln_date tempDate2;
		get_ln_date_from_sys(&tempDate2);
		date.hours = tempDate2.hours;
		date.minutes = tempDate2.minutes;
		date.seconds = tempDate2.seconds;
	}
	else
	{
		if (sscanf(s2,"%d:%d:%lf\n",&(date.hours),&(date.minutes),&(date.seconds)) != 3)
			return NULL;
	}

    if ( date.months>12 || date.months<1 || date.days<1 || date.days>31 ||
			date.hours>23 || date.hours<0 || date.minutes<0 || date.minutes>59 ||
			 date.seconds<0 || date.seconds>=60 )
    {
        return NULL;
    }
	return &date;
}
