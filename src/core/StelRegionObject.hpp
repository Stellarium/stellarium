/*
 * Copyright (C) 2009 Fabien Chereau
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

#ifndef STELREGIONOBJECT_HPP
#define STELREGIONOBJECT_HPP

#include "StelSphereGeometry.hpp"

#include <QSharedPointer>

//! Simple abstract class defining basic methods implemented by all objects that need
//! to be stored in a StelSphericalIndex.
class StelRegionObject
{
	//required for Q_FLAGS macro in some derived classes
	Q_GADGET
public:
	virtual ~StelRegionObject(void) {;}

	//! Return the spatial region of the object.
	virtual SphericalRegionP getRegion() const=0;
	
	//! Return the spatial region of the object.
	virtual Vec3d getPointInRegion() const=0;
};

//! @typedef StelRegionObjectP
//! Shared pointer on a StelRegionObject with smart pointers
typedef QSharedPointer<StelRegionObject> StelRegionObjectP;

#endif // STELGRIDOBJECT_HPP
