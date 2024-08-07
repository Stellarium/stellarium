/*
Copyright (C) 2001 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2003 Fabien Chereau
Copyright (C) 2024 Georg Zotti (C++ changes)

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

#ifndef PLUTO_HPP
#define PLUTO_HPP

//! Meeus, Astron. Algorithms 2nd ed (1998). Chap 37. Equ 37.1
//! @param in: JDE Julian day (ephemeris time),
//! @param out: X, Y, Z in AU.
//!
//! Calculate Pluto heliocentric rectangular ecliptical coordinates for given julian day.
//! This function is accurate to within 0.07" in longitude, 0.02" in latitude
//! and 0.000006 AU in radius.
//! @note: This function is not valid outside the period of 1885-2099!
//!

void get_pluto_helio_coords (double JDE, double * X, double * Y, double * Z);

#endif
