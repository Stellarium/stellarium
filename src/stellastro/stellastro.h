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

#ifndef LN_LIBNOVA_H
#define LN_LIBNOVA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>	
	

// General Calendar Functions

// Human readable (easy printf) date format
struct ln_date
{
    int years; 		/*!< Years. All values are valid */
    int months;		/*!< Months. Valid values : 1 (January) - 12 (December) */
    int days; 		/*!< Days. Valid values 1 - 28,29,30,31 Depends on month.*/
    int hours; 		/*!< Hours. Valid values 0 - 23. */
    int minutes; 	/*!< Minutes. Valid values 0 - 59. */
    double seconds;	/*!< Seconds. Valid values 0 - 59.99999.... */
};

/* Calculate the julian day from date.*/
double get_julian_day (struct ln_date * date);

/* Calculate the date from the julian day. */
void get_date (double JD, struct ln_date * date);

/* Calculate day of the week. 0 = Sunday .. 6 = Saturday */
unsigned int get_day_of_week (struct ln_date *date);
	
/* Calculate julian day from system time. */
double get_julian_from_sys ();

/* Calculate date from system date */
void get_ln_date_from_sys (struct ln_date * date);
	
/* Calculate julian day from time_t */
double get_julian_from_timet (time_t * time);

/* Calculate time_t from julian day */
void get_timet_from_julian (double JD, time_t * time);


/* Obtains Latitude, Longitude, RA or Declination from a string. */
double get_dec_location(char *s);


/* Obtains a human readable location in the form: ddºmm'ss.ss" */
char *get_humanr_location(double location);


/* Calculate mean sidereal time from date. */
double get_mean_sidereal_time (double JD);


/* Calculate apparent sidereal time from date.*/
double get_apparent_sidereal_time (double JD);


double range_radians(double angle);


/* Calculate Earth globe centre distance. */
void get_earth_centre_dist (float height, double latitude, double * p_sin_o, double * p_cos_o);


/* Calculate the adjustment in altitude of a body due to atmospheric refraction. */
double get_refraction_adj (double altitude, double atm_pres, double temp);


#ifdef __cplusplus
};
#endif


#endif
