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

Copyright (C) 2002 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2003 Fabien Chéreau

*/

#ifndef LN_LUNAR_H
#define LN_LUNAR_H

/* used for elp1 - 3 */
struct main_problem
{
	signed char ilu[4];
	float A;
	float B[5];
};

/* used for elp 4 - 9 */
struct earth_pert
{
	signed char iz;
	signed char ilu[4];
	float O;
	float A;
	float P;
}; 

/* used for elp 10 - 21 */
struct planet_pert
{
	signed char ipla[11];
	float theta;
	float O;
	float P;
};

typedef struct earth_pert tidal_effects;
typedef struct earth_pert moon_pert;
typedef struct earth_pert rel_pert;
typedef struct earth_pert plan_sol_pert;
 

/* sum ELP series */
double sum_series_elp1 (double *t);
double sum_series_elp2 (double *t);
double sum_series_elp3 (double *t);
double sum_series_elp4 (double *t);
double sum_series_elp5 (double *t);
double sum_series_elp6 (double *t);
double sum_series_elp7 (double *t);
double sum_series_elp8 (double *t);
double sum_series_elp9 (double *t);
double sum_series_elp10 (double *t);
double sum_series_elp11 (double *t);
double sum_series_elp12 (double *t);
double sum_series_elp13 (double *t);
double sum_series_elp14 (double *t);
double sum_series_elp15 (double *t);
double sum_series_elp16 (double *t);
double sum_series_elp17 (double *t);
double sum_series_elp18 (double *t);
double sum_series_elp19 (double *t);
double sum_series_elp20 (double *t);
double sum_series_elp21 (double *t);
double sum_series_elp22 (double *t);
double sum_series_elp23 (double *t);
double sum_series_elp24 (double *t);
double sum_series_elp25 (double *t);
double sum_series_elp26 (double *t);
double sum_series_elp27 (double *t);
double sum_series_elp28 (double *t);
double sum_series_elp29 (double *t);
double sum_series_elp30 (double *t);
double sum_series_elp31 (double *t);
double sum_series_elp32 (double *t);
double sum_series_elp33 (double *t);
double sum_series_elp34 (double *t);
double sum_series_elp35 (double *t);
double sum_series_elp36 (double *t);

#endif
