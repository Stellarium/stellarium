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

#ifndef _STELLASTRO_H
#define _STELLASTRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#ifndef J2000
# define J2000 2451545.0
#endif

// General Calendar Functions

// Human readable (easy printf) date format
typedef struct
{
    int years; 		/*!< Years. All values are valid */
    int months;		/*!< Months. Valid values : 1 (January) - 12 (December) */
    int days; 		/*!< Days. Valid values 1 - 28,29,30,31 Depends on month.*/
    int hours; 		/*!< Hours. Valid values 0 - 23. */
    int minutes; 	/*!< Minutes. Valid values 0 - 59. */
    double seconds;	/*!< Seconds. Valid values 0 - 59.99999.... */
}ln_date;


/* Calculate the julian day from date.*/
double get_julian_day(const ln_date * date);

/* Calculate the date from the julian day. */
void get_date(double JD, ln_date * date);

/* Calculate day of the week. 0 = Sunday .. 6 = Saturday */
unsigned int get_day_of_week (const ln_date *date);
	
/* Calculate julian day from system time. */
double get_julian_from_sys();

/* Calculate gmt date from system date */
void get_ln_date_from_sys(ln_date * date);

// Obtains a ln_date from 2 strings s1 and s2 for date and time
// with the form dd/mm/yyyy for s1 and hh:mm:ss.s for s2.
// Returns NULL if s1 or s2 is not valid.
// Uses the current date if s1 is "today" and current time if s2 is "now"
const ln_date * str_to_date(const char * s1, const char * s2);

// Calculate tm struct from julian day
void get_tm_from_julian(double JD, struct tm * tm_time);

// Return the number of hours to add to gmt time to get the local time
// taking the parameters from system. This takes into account the daylight saving
// time if there is.
int get_gmt_shift_from_system(void);

/* puts a large angle in the correct range 0 - 360 degrees */
double range_degrees (double angle);

/* puts a large angle in the correct range 0 - 2PI radians */
double range_radians (double angle);


/* Calculate mean sidereal time from date. */
double get_mean_sidereal_time (double JD);

/* Calculate apparent sidereal time from date.*/
double get_apparent_sidereal_time (double JD);

/* Input t is time in julian days.
 * Valid range is the years -8000 to +12000 (t = -100 to 100).
 * return mean obliquity (epsilon sub 0) in degrees.*/
double get_mean_obliquity( double t );

/* Calculate Earth globe centre distance. */
void get_earth_centre_dist (float height, double latitude, double * p_sin_o, double * p_cos_o);


/* Calculate the adjustment in altitude of a body due to atmospheric refraction. */
double get_refraction_adj (double altitude, double atm_pres, double temp);


#ifdef __cplusplus
};
#endif


#endif /* _STELLASTRO_H */
