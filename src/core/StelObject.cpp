/*
 * Stellarium
 * Copyright (C) 2006 Johannes Gajdosik
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


#include "StelObject.hpp"
#include "Navigator.hpp"

void intrusive_ptr_add_ref(StelObject* p)
{
	p->retain();
}

void intrusive_ptr_release(StelObject* p)
{
	p->release();
}

Vec3d StelObject::getEarthEquatorialPos(const Navigator * nav) const
{
	return nav->j2000_to_earth_equ(getObsJ2000Pos(nav));
}

