/*
 
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

Copyright 2000 Liam Girdwood
Modified 2003 Fabien Chéreau
 
*/

#include "vsop87.h"
#include <math.h>


double calc_series (const struct vsop * data, int terms, double t)
{
	static double value;
	static int i;
	value = 0.;

	for (i=0; i<terms; ++i)
	{
		value += data->A * cos(data->B + data->C * t);
		++data;
	}
	return value;
}



// Transform from VSOP87 to FK5 reference frame.
// Equation 31.3 Pg 207.
// param Latitude, Longitude in radian
// param JD Julian day
void vsop87_to_fk5 (double *L, double *B, double JD)
{
	static double LL, cosLL, sinLL, T, delta_L, delta_B;

	cosLL = cos(LL);
	sinLL = sin(LL);

	/* get julian centuries from 2000 */
	T = (JD - 2451545.0)/ 36525.0;
	
	LL = *L - 0.02438225 * T - 0.00000541052 * T * T;

	delta_L = (-0.09033 / 3600.0) + (0.03916 / 3600.0) * (cosLL + sinLL) * tan(*B);
	delta_B = (0.03916 / 3600.0) * (cosLL - sinLL);
	
	*L += delta_L;
	*B += delta_B;
}
