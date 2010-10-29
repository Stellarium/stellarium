/***************************************************************************
 * Name: gMatrix.h
 *
 * Date: 2007-03-29 17:01:15 +0200 (jue, 29 mar 2007)
 * Revision: 16
 * HeadURL: https://gsat.svn.sourceforge.net/svnroot/gsat/trunk/src/GMatrix.h
 *
 * Description:  Fichero de descripci� de la clase GMatrix.
 *               Esta clase tiene como cometido la encapsulaci� de objetos
 *               tipo matriz bidimensional de elementos GDOUBLE junto con
 *               las operaciones de matrices realizables.
 *
 *
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2004 by JL Canales                                      *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _GMATRIX_HPP_
#define _GMATRIX_HPP_ 1

// MatrixContainer
#include "gMatrixTempl.hpp"
#include "gVector.hpp"

//! @class: gMatrix
//! This class implement the needed code to make matrix calculations.
//! The GMatrix class inherit from MatrixContainer class that is created
//! from the vector STL class givingt all the STL vector funcionality.
//! This class has not overlapped the = operator and the copy constructor
//! because this methods are given by the vector STL class.
class gMatrix : public gMatrixTempl<double>
{
public:
	//## Constructors (generated)
	gMatrix();

	//## Constructors (specified)
	//## Operation:gMatrix%950961952
	//	Constructor whit row and column dimensions.
	gMatrix ( unsigned int ai_uiRowNumber, unsigned int ai_uiColumNumber);

	//## Equality Operations (generated)
	bool operator!=( const gMatrix &right) const;

	//## Other Operations (specified)
	//## Operation: operator*%949869240
	//	This operators make the vectorial product calculation
	gMatrix operator* ( const gMatrix &ai_RightMatrix);

	//## Operation: operator*%949869240
	//	This operators make the vectorial product calculation
	gVector operator* ( const gVector &ai_RightVector);

	//## Operation: operator*%949869241
	//	This operator make the scalar product calculation.
	gMatrix operator* ( double ai_dRight);

	gMatrix operator+ ( gMatrix &ai_RightMatrix) const;
	const gMatrix& operator+=( gMatrix &ai_RightMatrix);

	gMatrix operator- ( gMatrix &ai_RightMatrix) const;
	const gMatrix& operator-=( gMatrix &ai_RightMatrix);

	// Determinant
	double det();
};

#endif //_GMATRIX_HPP_
