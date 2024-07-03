/*
Copyright (C) 2000 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2003 Fabien Chereau
Copyright (C) 2010 Georg Zotti (parts)

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
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef SIDEREAL_TIME_HPP
#define SIDEREAL_TIME_HPP


/* Calculate mean sidereal time (degrees) from date. */
double get_mean_sidereal_time (double JD, double JDE);

/* Calculate apparent sidereal time (degrees) from date. We need JD(UT) and JDE(TT) here to accurately compute nutation. */
double get_apparent_sidereal_time (double JD, double JDE);


#endif /* SIDEREAL_TIME_HPP */
