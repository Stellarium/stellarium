/***************************************************************************
 * Name: gMatrixTempl.h
 *
 * Description: MatrixContainer is a template to build
 *   			 bidimensional objects arrays.
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2006 by j. l. Canales   *
 *   jlcanales@users.sourceforge.net   *
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

#ifndef _GMATRIXTEMPL_HPP_
#define _GMATRIXTEMPL_HPP_ 1

#include "gVectorTempl.hpp"  // checked vector
#include <iostream>          // for operator<<(), see below

// matrix as vector of vectors
template<class T>
class gMatrixTempl : public br_stl::gVectorTempl<
	br_stl::gVectorTempl<T> >
{
public:
	typedef typename std::vector<T>::size_type size_type;

	gMatrixTempl(size_type x = 0, size_type y = 0)
		: br_stl::gVectorTempl< br_stl::gVectorTempl<T> >(x, br_stl::gVectorTempl<T>(y)), rows(x), columns(y)
	{
	}

	gMatrixTempl(const gMatrixTempl<T> &right)
		: br_stl::gVectorTempl< br_stl::gVectorTempl<T> >(right.rows, br_stl::gVectorTempl<T>(right.columns)), rows(right.rows), columns(right.columns)
	{

		for(size_type i = 0; i < rows; ++i)
			for(size_type j = 0; j < columns ; ++j)
				operator[](i)[j] = right[ i][ j];
	}

	size_type Rows() const
	{
		return rows;
	}

	size_type Columns() const
	{
		return columns;
	}

	void SetSize(size_type x, size_type y)
	{

		br_stl::gVectorTempl< br_stl::gVectorTempl<T> >::resize(x);
		for(size_type index = 0; index < x; index++)
			operator[](index).resize(y);

		rows    = x;
		columns = y;
	}


	void init(const T& Value)
	{
		for(size_type i = 0; i < rows; ++i)
			for(size_type j = 0; j < columns ; ++j)
				operator[](i)[j] = Value; // that is, (*this)[i][j]
	}

	// create identity matrix
	gMatrixTempl<T>& I()
	{
		for(size_type i = 0; i < rows; ++i)
			for(size_type j = 0; j < columns ; ++j)
				operator[](i)[j] = (i==j) ? T(1) : T(0);
		return *this;
	}

protected:
	size_type rows;
	size_type columns;


}; // class Matrix


template<class T>
inline std::ostream& operator<<(std::ostream& s, const gMatrixTempl<T>& m)
{
	typedef typename gMatrixTempl<T>::size_type size_type;

	for(size_type i = 0; i < m.Rows(); ++i)
	{
		s << std::endl << i <<" : ";
		for(size_type j = 0; j < m.Columns(); ++j)
			s << m[i][j] <<" ";
	}
	s << std::endl;
	return s;
}

#endif // _GMATRIXTEMPL_HPP_
