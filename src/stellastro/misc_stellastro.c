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


/* puts a large angle in the correct range 0 - 360 degrees */
double range_degrees(double angle)
{
    double temp;

    if (angle >= 0.0 && angle < 360.0)
    	return(angle);

	temp = (int)(angle / 360);

	if ( angle < 0.0 )
	   	temp --;

    temp *= 360;

    return (angle - temp);
}

/* puts a large angle in the correct range 0 - 2PI radians */
double range_radians (double angle)
{
    double temp;

    if (angle >= 0.0 && angle < (2.0 * M_PI))
    	return(angle);

	temp = (int)(angle / (M_PI * 2.0));

	if ( angle < 0.0 )
		temp --;
	temp *= (M_PI * 2.0);

	return (angle - temp);
}
