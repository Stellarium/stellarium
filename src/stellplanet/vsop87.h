/*

Copyright 2000 Liam Girdwood
Modified 2003 Fabien Chéreau

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

*/

#ifndef LN_VSOP87_H
#define LN_VSOP87_H

struct vsop
{
	float A;
	float B;
	float C;
};

double calc_series (const struct vsop * data, int terms, double t);
void vsop87_to_fk5 (double *L, double *B, double JD);

#endif
