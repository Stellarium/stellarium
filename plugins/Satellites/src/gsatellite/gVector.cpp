/***************************************************************************
 * Name: gVector.hpp
 *
 * Description: GVector class envelop the STL vertor class to add
 *              some error control.
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2006 by J. L. Canales                                   *
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

#include "gVector.hpp"
#include <cassert>
#include <math.h>
gVector::gVector()
	: br_stl::gVectorTempl<double>()
{

}

gVector::gVector(unsigned int ai_uiElementsNumber)
	: br_stl::gVectorTempl<double>(ai_uiElementsNumber)
{

}

gVector gVector::operator- (gVector &ai_rightVector) const
{

	assert(size() == ai_rightVector.size());

	gVector retVector(size());

	for(unsigned int i=0; i<size(); i++)
		retVector[ i] = operator[](i) - ai_rightVector[i];
	return retVector;

}
const gVector& gVector::operator-=(gVector &ai_rightVector)
{
	assert(size() == ai_rightVector.size());

	for(unsigned int i=0; i<size(); i++)
		operator[](i) -= ai_rightVector[i];
	return *this;
}

gVector gVector::operator+ (gVector &ai_rightVector) const
{

	assert(size() == ai_rightVector.size());

	gVector retVector(size());

	for(unsigned int i=0; i<size(); i++)
		retVector[ i] = operator[](i) + ai_rightVector[i];
	return retVector;

}

const gVector& gVector::operator+=(gVector &ai_rightVector)
{
	assert(size() == ai_rightVector.size());

	for(unsigned int i=0; i<size(); i++)
		operator[](i) += ai_rightVector[i];
	return *this;
}

double gVector::Dot(const gVector& ai_rightVector) const
{
	assert(size() == ai_rightVector.size());

	double dot = 0;
	for(unsigned int i=0; i<size(); i++)
		dot += operator[](i) * ai_rightVector[i];
	return dot;
}

double gVector::Magnitude() const
{

	double magnitude=0;

	for(unsigned int i=0; i<size(); i++)
		magnitude += (operator[](i) * operator[](i));

	return sqrt(magnitude);
}
