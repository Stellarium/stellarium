/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _PROJECTORTYPE_HPP_
#define _PROJECTORTYPE_HPP_

#include <boost/shared_ptr.hpp>
#include <config.h>

class Projector;

namespace boost
{
	//! @class shared_ptr
	//! Boost shared pointer class. See http://www.boost.org/doc/libs/1_37_0/libs/smart_ptr/shared_ptr.htm
	template<class T> class shared_ptr;
};

//! @typedef ProjectorP
//! Shared pointer on a Projector instance (implement reference counting)
typedef boost::shared_ptr<Projector> ProjectorP;

#endif // _PROJECTORTYPE_HPP_
