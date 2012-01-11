/***************************************************************************
 * Name: GVector.h
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

#ifndef _GVECTORTEMPL_H_
#define _GVECTORTEMPL_H_ 1


#include <cassert>
#include <vector> //libreria stl de vectores
#include <iostream> // for operator<<(), see below

namespace br_stl
{

template<class T>
class gVectorTempl : public std::vector<T>
{

public:

	//inherited types
	typedef typename gVectorTempl::size_type size_type;
	typedef typename gVectorTempl::iterator iterator;
	typedef typename gVectorTempl::difference_type difference_type;
	typedef typename gVectorTempl::reference reference;
	typedef typename gVectorTempl::const_reference const_reference;


	gVectorTempl()
	{
	}

	gVectorTempl(const gVectorTempl &right)
		:std::vector<T>((std::vector<T>) right)
	{

	}
	gVectorTempl(size_type n, const T& value=T())
		:std::vector<T>(n, value)
	{
	}

	gVectorTempl(iterator i, iterator j)
		:std::vector<T>(i, j)
	{
	}

	reference  operator [](difference_type index)
	{
		assert(index >=0 && index < static_cast<difference_type>(gVectorTempl::size()));


		return std::vector<T>::operator[](index);
	}

	const_reference operator [](difference_type index) const
	{
		assert(index >=0 && index < static_cast<difference_type>(gVectorTempl::size()));

		return std::vector<T>::operator[](index);
	}


};


template<class T>
inline std::ostream& operator<<(std::ostream& s, const gVectorTempl<T>& m)
{
	typedef typename gVectorTempl<T>::size_type size_type;

	for(size_type i = 0; i < m.size(); ++i)
	{
		s << std::endl << i <<" : ";
		s << m[i] <<" ";
	}
	s << std::endl;
	return s;
}

}
#endif // _GVECTORTEMPL_H_

