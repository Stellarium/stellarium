/*
Copyright (C) 2000 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2003 Fabien Chereau

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

#ifndef _STELLASTRO_H
#define _STELLASTRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Calculate mean sidereal time from date. */
double get_mean_sidereal_time (double JD);

/* Calculate apparent sidereal time from date.*/
double get_apparent_sidereal_time (double JD);

/* Calculate Earth globe centre distance. */
void get_earth_centre_dist (float height, double latitude, double * p_sin_o, double * p_cos_o);

/* Calculate the adjustment in altitude of a body due to atmospheric refraction. */
double get_refraction_adj (double altitude, double atm_pres, double temp);

#ifdef __cplusplus
}
#endif


#endif /* _STELLASTRO_H */
