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

#include "stellastro.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>

/* Calculate the julian day from a calendar day.
 * Valid for positive and negative years but not for negative JD.
 * Formula 7.1 on pg 61 */
 // Code originally from libnova which appeared to be totally wrong... New code from celestia
double get_julian_day (const ln_date * cdate)
{
	ln_date date;
	int y, m, B;

	date = *cdate;

    y = date.years;
	m = date.months;
    if (date.months <= 2)
    {
        y = date.years - 1;
        m = date.months + 12;
    }

    // Correct for the lost days in Oct 1582 when the Gregorian calendar
    // replaced the Julian calendar.
    B = -2;
    if (date.years > 1582 || (date.years == 1582 && (date.months > 10 || (date.months == 10 && date.days >= 15))))
    {
        B = y / 400 - y / 100;
    }

    return (floor(365.25 * y) +
            floor(30.6001 * (m + 1)) + B + 1720996.5 +
            date.days + date.hours / 24.0 + date.minutes / 1440.0 + date.seconds / 86400.0);
}


/* Calculate the day of the week.
 * Returns 0 = Sunday .. 6 = Saturday */
unsigned int get_day_of_week (const ln_date *date)
{
    double JD;
    /* get julian day */
    JD = get_julian_day(date) + 1.5;
    return (int)JD % 7;
}

// Calculate tm struct from julian day
void get_tm_from_julian(double JD, struct tm * tm_time)
{
	ln_date date;
	get_date(JD, &date);
	tm_time->tm_sec = date.seconds;
	tm_time->tm_min = date.minutes;
	tm_time->tm_hour = date.hours;
	tm_time->tm_mday = date.days;
	tm_time->tm_mon = date.months - 1;
	tm_time->tm_year = date.years - 1900;
	tm_time->tm_isdst = -1;
}

/* Calculate the date from the Julian day
 * params : JD Julian day, date Pointer to new calendar date. */
 // Code originally from libnova which appeared to be totally wrong... New code from celestia
void get_date(double jd, ln_date * date)
{
    int a, d, e, f;
	double c, b;
	double dday, dhour, dminute;

	a = (int) (jd + 0.5);

    if (a < 2299161)
    {
        c = a + 1524;
    }
    else
    {
        b = (int) ((a - 1867216.25) / 36524.25);
        c = a + b - (int) (b / 4) + 1525;
    }

    d = (int) ((c - 122.1) / 365.25);
    e = (int) (365.25 * d);
    f = (int) ((c - e) / 30.6001);

    dday = c - e - (int) (30.6001 * f) + ((jd + 0.5) - (int) (jd + 0.5));

    /* This following used to be 14.0, but gcc was computing it incorrectly, so
       it was changed to 14 */
    date->months = f - 1 - 12 * (int) (f / 14);
    date->years = d - 4715 - (int) ((7.0 + date->months) / 10.0);
    date->days = (int) dday;

    dhour = (dday - date->days) * 24;
    date->hours = (int) dhour;

    dminute = (dhour - date->hours) * 60;
    date->minutes = (int) dminute;

    date->seconds = (dminute - date->minutes) * 60;
}	


/* Calculate julian day from system time. */
double get_julian_from_sys(void)
{
	ln_date date;
	/* get sys date */
	get_ln_date_from_sys(&date);
	return get_julian_day(&date);
}


/* Calculate gmt date from system date.
 * param : date Pointer to store date. */
void get_ln_date_from_sys(ln_date * date)
{
	time_t rawtime;
	struct tm * ptm;

	/* get current time */
	time ( &rawtime );

	/* convert to gmt time representation */
    ptm = gmtime ( &rawtime );

	/* fill in date struct */
	date->seconds = ptm->tm_sec;
	date->minutes = ptm->tm_min;
	date->hours = ptm->tm_hour;
	date->days = ptm->tm_mday;
	date->months = ptm->tm_mon + 1;
	date->years = ptm->tm_year + 1900;
}


// Calculate time_t from julian day
time_t get_time_t_from_julian(double JD)
{
	struct tm loctime;
	ln_date date;

	get_date(JD, &date);

	loctime.tm_sec = date.seconds;
	loctime.tm_min = date.minutes;
	loctime.tm_hour = date.hours;
	loctime.tm_mday =date.days;
	loctime.tm_mon = date.months -1;
	loctime.tm_year = date.years - 1900;
	loctime.tm_isdst = -1;

	return mktime(&loctime);
}
