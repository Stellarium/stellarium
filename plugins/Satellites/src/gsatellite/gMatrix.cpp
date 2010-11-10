/***************************************************************************
 * Name: GMatrix.cpp
 *
 * Date: 2009-12-31 16:34:55 +0100 (jue, 31 dic 2009)
 * Revision: 17
 * HeadURL: https://gsat.svn.sourceforge.net/svnroot/gsat/trunk/src/GMatrix.cpp
 *
 * Description: GMatrix Definition.
 *              GMatrix class define a bidimensional GDOUBLE elements matrix
 *              and the basic matrix operations.
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

// CMatrix
#include "gMatrix.hpp"
#include "gException.hpp"

#include <cassert>

// Class gMatrix
gMatrix::gMatrix() : gMatrixTempl<double>()
{
}

gMatrix::gMatrix(unsigned int ai_uiRowNumber, unsigned int ai_uiColumNumber)
	: gMatrixTempl<double>(ai_uiRowNumber, ai_uiColumNumber)
{

}


bool gMatrix::operator!=(const gMatrix &right) const
{
	if((*this)==(right))
	{
		return false;
	}
	else
	{
		return true;
	}
}



//## Other Operations (implementation)

gMatrix gMatrix::operator *(const gMatrix &ai_RightMatrix)
{

	unsigned int iRow;
	unsigned int iColumn;
	unsigned int uiRightColumn = ai_RightMatrix.Columns();
	unsigned int uiRightRow    = ai_RightMatrix.Rows();

	unsigned int Iterator;
	double dAuxValue = 0;
	gMatrix ReturnedMatrix(Rows(), uiRightColumn);


	if(Columns() != uiRightRow)
	{
		throw std::out_of_range(OPERATOR_X_INCOMPATIBLE_ORDER);
	}
	else
	{
		for(iRow = 0; iRow < Rows(); iRow++)
			for(iColumn = 0; iColumn < uiRightColumn; iColumn++)
			{
				// Product and addition of implied elements
				for(Iterator = 0; Iterator < uiRightRow; Iterator++)
				{
					dAuxValue += (operator[](iRow)[ Iterator] *
					              ai_RightMatrix[ Iterator][ iColumn]);
				}

				ReturnedMatrix[ iRow][ iColumn] = dAuxValue;
				dAuxValue = 0;
			}
	}

	return ReturnedMatrix;
}


gVector gMatrix::operator *(const gVector &ai_RightVector)
{

	unsigned int iRow;
	unsigned int iColumn;

	double dAuxValue = 0;
	gVector ReturnedVector(Rows());

	assert(Columns() == ai_RightVector.size());


	if(Columns() != ai_RightVector.size())
	{
		throw std::out_of_range(OPERATOR_X_INCOMPATIBLE_ORDER);
	}
	else
	{
		for(iRow = 0; iRow < Rows(); iRow++)
		{
			for(iColumn = 0; iColumn < Columns(); iColumn++)
			{
				// Product and addition of implied elements
				dAuxValue += (operator[](iRow)[ iColumn] *
				              ai_RightVector[ iColumn]);
			}

			ReturnedVector[ iRow] = dAuxValue;
			dAuxValue = 0;

		}
	}
	return ReturnedVector;
}


gMatrix gMatrix::operator *(double ai_dRight)
{
	unsigned int iRow;
	unsigned int iColumn;
	gMatrix ReturnedMatrix(Rows(), Columns());

	try
	{
		for(iRow = 0; iRow < Rows(); iRow++)
			for(iColumn = 0; iColumn < Columns(); iColumn ++)
				ReturnedMatrix[ iRow][ iColumn] = ((*this)[ iRow][ iColumn] * ai_dRight);
	}
	catch(...)
	{
		for(iRow = 0; iRow < Rows(); iRow++)
			for(iColumn = 0; iColumn < Columns(); iColumn ++)
				ReturnedMatrix[ iRow][ iColumn] = 0;

		throw;
	}

	return ReturnedMatrix;
}

gMatrix gMatrix::operator+(gMatrix &ai_RightMatrix) const
{

	gMatrix ReturnedMatrix(Rows(), Columns());

#ifdef ENABLE_DEBUG
	assert(Rows()==ai_RightMatrix.Rows() && Columns()==ai_RightMatrix.Columns());
#endif

	if(Rows()!=ai_RightMatrix.Rows() || Columns()!=ai_RightMatrix.Columns())
	{
		throw std::out_of_range(OPERATOR_ADD_INCOMPATIBLE_ORDER);
	}


	for(unsigned int i = 0; i<Rows(); i++)
		for(unsigned int j = 0; j<Columns(); j++)
			ReturnedMatrix[ i][ j] = operator[](i)[ j] + ai_RightMatrix[ i][ j];


	return ReturnedMatrix;

}

const gMatrix& gMatrix::operator+=(gMatrix &ai_RightMatrix)
{

#ifdef ENABLE_DEBUG
	assert(Rows()==ai_RightMatrix.Rows() && Columns()==ai_RightMatrix.Columns());
#endif

	if(Rows()!=ai_RightMatrix.Rows() || Columns()!=ai_RightMatrix.Columns())
	{
		throw std::out_of_range(OPERATOR_ADDEQUAL_INCOMPATIBLE_ORDER);
	}
	for(unsigned int i = 0; i<Rows(); i++)
		for(unsigned int j = 0; j<Columns(); j++)
			operator[](i)[ j] = operator[](i)[ j] + ai_RightMatrix[ i][ j];


	return *this;

}

gMatrix gMatrix::operator-(gMatrix &ai_RightMatrix) const
{

	gMatrix ReturnedMatrix(Rows(), Columns());

#ifdef ENABLE_DEBUG
	assert(Rows()==ai_RightMatrix.Rows() && Columns()==ai_RightMatrix.Columns());
#endif

	if(Rows()!=ai_RightMatrix.Rows() || Columns()!=ai_RightMatrix.Columns())
	{
		throw std::out_of_range(OPERATOR_DIFF_INCOMPATIBLE_ORDER);
	}
	for(unsigned int i = 0; i<Rows(); i++)
		for(unsigned int j = 0; j<Columns(); j++)
			ReturnedMatrix[ i][ j] = operator[](i)[ j] - ai_RightMatrix[ i][ j];


	return ReturnedMatrix;

}

const gMatrix& gMatrix::operator-=(gMatrix &ai_RightMatrix)
{

#ifdef ENABLE_DEBUG
	assert(Rows()==ai_RightMatrix.Rows() && Columns()==ai_RightMatrix.Columns());
#endif

	if(Rows()!=ai_RightMatrix.Rows() || Columns()!=ai_RightMatrix.Columns())
	{
		throw std::out_of_range(OPERATOR_DIFFEQUAL_INCOMPATIBLE_ORDER);
	}
	for(unsigned int i = 0; i<Rows(); i++)
		for(unsigned int j = 0; j<Columns(); j++)
			operator[](i)[ j] = operator[](i)[ j] - ai_RightMatrix[ i][ j];


	return *this;

}

double gMatrix::det()
{

	gMatrix aux=*this;
	double dDet;
	int k,l,i,j;
	int n = Rows();
	int m = n-1;


	try
	{
		if(Columns() != Rows())
		{
			throw std::invalid_argument(DET_INCOMPATIBLE_ORDER);
		}

		dDet=aux[0][0];
		for(k=0; k<m; k++)
		{
			l=k+1;
			for(i=l; i<n; i++)
			{
				for(j=l; j<n; j++)
				{
					aux[i][j] = (aux[k][k]*aux[i][j]-aux[k][j]*aux[i][k])/aux[k][k];
				}
			}
			dDet=dDet*aux[k+1][k+1];
		}

		return dDet;
	}
	catch(...)
	{
		throw;
	}
}

