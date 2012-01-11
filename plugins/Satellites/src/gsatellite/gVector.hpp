/***************************************************************************
 * Name: gVector.hpp
 *
 * Description: GVector class envelop the STL vertor class to add
 *              some error control.
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2006 by J.L. Canales                                    *
 *   jlcanales@users.sourceforge.net                                       *
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

#ifndef _GVECTOR_HPP_
#define _GVECTOR_HPP_ 1

#include "gVectorTempl.hpp"

//## Class: gVector
//	This class implement the needed code to make vector calculations.
//	The gVector class inherit from gVectorTempl class that is created
//	from the vector STL class giving all the STL vector funcionality.
//
//	This class has not overlapped the = operator and the copy constructor
//	because this methods are given by the vector STL class.
class gVector : public br_stl::gVectorTempl<double>
{

public:
	gVector();
	gVector(unsigned int ai_uiElementsNumber);

	virtual ~gVector() {};

	//## Other Operations (specified)
	//## Operation: operator*
	//	This operators make the vectorial product calculation
	// gVector operator* ( const gVector &ai_rightVector);

	//## Operation: operator*
	//	This operator make the scalar product calculation.
	//gVector operator* ( double ai_escalar);

	gVector operator+ (gVector &ai_rightVector) const;
	const gVector& operator+=(gVector &ai_rightVector);

	gVector operator- (gVector &ai_rightVector) const;
	const gVector& operator-=(gVector &ai_rightVector);

	//## Operation: Dot
	//   This method compute the dot vector between two vectors
	double Dot(const gVector& ai_rightVector) const;

	//## Operation: Angle
	//	This method compute the angle angle between two vectors
	//double Angle(const gVector&) const;

	//## Operation: Magnitude
	//	This method compute the vector magnitude
	double Magnitude() const;

};

#endif // _GVECTOR_HPP_
