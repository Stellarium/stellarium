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

#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
/* puts a large angle in the correct range 0 - 360 degrees */
double range_degrees(double angle)
{
    double temp;
    double range;


    if (angle >= 0.0 && angle < 360.0)
    	return(angle);

	temp = (int)(angle / 360);

	if ( angle < 0.0 )
	   	temp --;

    temp *= 360;
	range = angle - temp;
    return (range);
}

/* puts a large angle in the correct range 0 - 2PI radians */
double range_radians (double angle)
{
    double temp;
    double range;

    if (angle >= 0.0 && angle < (2.0 * M_PI))
    	return(angle);

	temp = (int)(angle / (M_PI * 2.0));

	if ( angle < 0.0 )
		temp --;
	temp *= (M_PI * 2.0);
	range = angle - temp;

	return (range);
}

/* local types and macros */
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define iswhite(c)  ((c)== ' ' || (c)=='\t')

/*
[]------------------------------------------------------------------------[]
|  trim() & strip()                                                        |
|                                                                          |
|  strips trailing whitespaces from buf.                                   |
|                                                                          |
[]------------------------------------------------------------------------[]
*/
static char *trim(char *x)
{
    char *y;

    if(!x)
        return(x);
    y = x + strlen(x)-1;
    while (y >= x && iswhite(*y))
        *y-- = 0; /* skip white space */
    return x;
}


/*
[]------------------------------------------------------------------------[]
|                                                                          |
|   skipwhite()                                                            |
|   salta espacios en blanco                                               |
|                                                                          |
[]------------------------------------------------------------------------[]
*/
static void skipwhite(char **s)
{
   while(iswhite(**s))
        (*s)++;
}


/*! \fn double get_dec_location(char * s)
* \param s Location string
* \return angle in degrees
*
* Obtains Latitude, Longitude, RA or Declination from a string.
*
*  If the last char is N/S doesn't accept more than 90 degrees.            
*  If it is E/W doesn't accept more than 180 degrees.                      
*  If they are hours don't accept more than 24:00                          
*                                                                          
*  Any position can be expressed as follows:                               
*  (please use a 8 bits charset if you want                                
*  to view the degrees separator char '0xba')                              
*
*  42.30.35,53                                                             
*  90º0'0,01 W                                                             
*  42º30'35.53 N                                                           
*  42º30'35.53S                                                            
*  42º30'N                                                                 
*  - 42.30.35.53                                                           
*   42:30:35.53 S                                                          
*  + 42.30.35.53                                                           
*  +42º30 35,53                                                            
*   23h36'45,0                                                             
*                                                                          
*                                                                          
*  42:30:35.53 S = -42º30'35.53"                                           
*  + 42 30.35.53 S the same previous position, the plus (+) sign is        
*  considered like an error, the last 'S' has precedence over the sign     
*                                                                          
*  90º0'0,01 N ERROR: +- 90º0'00.00" latitude limit                        
*
*/
double get_dec_location(char *s)
{

	char *ptr, *dec, *hh;
	BOOL negative = FALSE;
	char delim1[] = " :.,;ºDdHhMm'\n\t";
	char delim2[] = " NSEWnsew\"\n\t";
	int dghh = 0, minutes = 0;
	double seconds = 0.0, pos;
        short count;

	enum _type{
    	    HOURS, DEGREES, LAT, LONG
	}type;

	if (s == NULL || !*s)
		return(-0.0);
	count = strlen(s) + 1;
	if ((ptr = (char *) alloca(count)) == NULL)
		return (-0.0);
	memcpy(ptr, s, count);
	trim(ptr);
	skipwhite(&ptr);
        
        /* the last letter has precedence over the sign */
	if (strpbrk(ptr,"SsWw") != NULL) 
		negative = TRUE;

	if (*ptr == '+' || *ptr == '-')
		negative = (char) (*ptr++ == '-' ? TRUE : negative);	
	skipwhite(&ptr);
	if ((hh = strpbrk(ptr,"Hh")) != NULL && hh < ptr + 3)
            type = HOURS;
        else 
            if (strpbrk(ptr,"SsNn") != NULL)
		type = LAT;
	    else 
 	        type = DEGREES; /* unspecified, the caller must control it */

	if ((ptr = strtok(ptr,delim1)) != NULL)
		dghh = atoi (ptr);
	else
		return (-0.0);

	if ((ptr = strtok(NULL,delim1)) != NULL) {
		minutes = atoi (ptr);
		if (minutes > 59)
		    return (-0.0);
	}else
		return (-0.0);

	if ((ptr = strtok(NULL,delim2)) != NULL) {
		if ((dec = strchr(ptr,',')) != NULL)
			*dec = '.';
		seconds = strtod (ptr, NULL);
		if (seconds > 59)
			return (-0.0);
	}
	
	if ((ptr = strtok(NULL," \n\t")) != NULL) {
		skipwhite(&ptr);
		if (*ptr == 'S' || *ptr == 'W' || *ptr == 's' || *ptr == 'W')
			    negative = TRUE;
	}
        pos = dghh + minutes /60.0 + seconds / 3600.0;

	if (type == HOURS && pos > 24.0)
		return (-0.0);
	if (type == LAT && pos > 90.0)
		return (-0.0);
	else
	    if (pos > 180.0)
		return (-0.0);

	if (negative)
		pos = 0.0 - pos;

	return (pos);

}


/*! \fn char * get_humanr_location(double location)    
* \param location Location angle in degress
* \return Angle string
*
* Obtains a human readable location in the form: ddºmm'ss.ss"             
*/
char *get_humanr_location(double location)
{
    static char buf[16];
    double deg = 0.0;
    double min = 0.0;
    double sec = 0.0;
    *buf = 0;
    sec = 60.0 * (modf(location, &deg));
    if (sec < 0.0)
        sec *= -1;
    sec = 60.0 * (modf(sec, &min));
    sprintf(buf,"%+dº%d'%.2f\"",(int)deg, (int) min, sec);
    return buf;
}
