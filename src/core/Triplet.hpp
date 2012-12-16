/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _TRIPLET_HPP_
#define _TRIPLET_HPP_

//! A simple struct holding a triplet of values of some type.
//!
//! Safer and more readable than using fixed-size arrays and passing them as pointers.
//!
//! Used e.g. for triangles. 
template<class T>
struct Triplet
{
	//! First, second, and third element of the triplet.
	T a, b, c;

	//! Default constructor for collections.
	Triplet(){}

	//! Construct a triplet from three values.
	Triplet(const T& a, const T& b, const T& c) : a(a), b(b), c(c){}
};

#endif // _TRIPLET_HPP_
