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

#include "stellastro.h"
#include <time.h>

/* Calculate the julian day from a calendar day.
 * Valid for positive and negative years but not for negative JD.
 * Formula 7.1 on pg 61 */
double get_julian_day (struct ln_date * date)
{
    double days;
    int a,b;
    
    /* check for month = January or February */
    if (date->months < 3 )
    {
        date->years--;
	    date->months += 12;
	}
	
	a = date->years / 100;
	
	/* check for Julian or Gregorian calendar (starts Oct 4th 1582) */
	if (date->years > 1582 || (date->years == 1582 && (date->months > 10 || (date->months == 10 && date->days >= 4))))
	{
	    /* Gregorian calendar */    
	    b = 2 - a + (a / 4);
	}
	else
	{
	    /* Julian calendar */
	    b = 0;
	}
	
	/* add a fraction of hours, minutes and secs to days*/
	days = date->days + (double)(date->hours / 24.0) + (double)(date->minutes / 1440.0) + (double)(date->seconds /  86400.0);

	/* now get the JD */
	return (int)(365.25 * (date->years + 4716)) +
	    (int)(30.6001 * (date->months + 1)) + days + b - 1524.5;
}


/* Calculate the day of the week.
 * Returns 0 = Sunday .. 6 = Saturday */
unsigned int get_day_of_week (struct ln_date *date)
{
    unsigned int day;
    double JD;
    
    /* get julian day */
    JD = get_julian_day (date);
    
    JD += 1.5;
    
    day = (int)JD % 7; 
    
    return (day);
}	


/* Calculate the date from the Julian day
 * params : JD Julian day, date Pointer to new calendar date. */
void get_date (double JD, struct ln_date * date)
{
   int A,a,B,C,D,E;
   double F,Z;
   
   JD += 0.5;
   Z = (int) JD;
   F = JD - Z;
   
   if (Z < 2299161)
   {
       A = Z;
   }
   else
   {
       a = (int) ((Z - 1867216.25) / 36524.25);
       A = Z + 1 + a - (int)(a / 4);
   }
   
   B = A + 1524;
   C = (int) ((B - 122.1) / 365.25);
   D = (int) (365.25 * C);
   E = (int) ((B - D) / 30.6001);
   
   /* get the hms */
   date->hours = F * 24;
   F -= (double)date->hours / 24;
   date->minutes = F * 1440;
   F -= (double)date->minutes / 1440;
   date->seconds = F * 86400;
   
   /* get the day */
   date->days = B - D - (int)(30.6001 * E);
   
   /* get the month */
   if (E < 14)
   {
       date->months = E - 1;
   }
   else
   {
       date->months = E - 13;
   }
   
   /* get the year */
   if (date->months > 2)
   {
       date->years = C - 4716;
   }
   else
   {
       date->years = C - 4715;
   }    
}	


/* Calculate julian day from system time. */
double get_julian_from_sys ()
{
	double JD;
	struct ln_date date;
	
	/* get sys date */
	get_ln_date_from_sys (&date);
	
	JD = get_julian_day (&date);
	
	return (JD);
}


/* Calculate gmt date from system date.
 * param : date Pointer to store date. */
void get_ln_date_from_sys (struct ln_date * date)
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
