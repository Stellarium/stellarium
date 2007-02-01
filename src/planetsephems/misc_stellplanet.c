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

/* Transform spheric coordinate in rectangular */
void sphe_to_rect(double lng, double lat, double r, double *x, double *y, double *z)
{
	double cosLat = cos(lat);
    (*x) = cos(lng) * cosLat;
    (*y) = sin(lng) * cosLat;
	(*z) = sin(lat);
	(*x)*=r;
	(*y)*=r;
	(*z)*=r;
}
