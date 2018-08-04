/*
Copyright (C) 2001 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2003 Fabien Chereau

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Libary General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef PLUTO_H
#define PLUTO_H

#ifdef __cplusplus
extern "C" {
#endif


struct pluto_argument
{
	double J, S, P;
};

struct pluto_longitude
{
	double A,B;
};

struct pluto_latitude
{
	double A,B;
};

struct pluto_radius
{
	double A,B;
};

void get_pluto_helio_coords (double JD, double * X, double * Y, double * Z);

#ifdef __cplusplus
}
#endif


#endif
