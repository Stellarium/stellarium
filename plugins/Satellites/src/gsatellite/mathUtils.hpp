/***************************************************************************
 * Name: mathUtils.cpp
 *
 * Description: Mathematical utilities
 *              Implement some mathematical utilities to trigonometrical
 *              calc.
 *
 *
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2004 by J. L. Canales                                   *
 *   ph03696@homeserver                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.             *
 ***************************************************************************/

#ifndef MATHUTILS_HPP
#define MATHUTILS_HPP


//! Four-quadrant arctan function
//! @ingroup satellites
double AcTan(double sinx, double cosx);

//! Returns square of a double
//! @ingroup satellites
double Sqr(double arg);


#endif // MATHUTILS_HPP
