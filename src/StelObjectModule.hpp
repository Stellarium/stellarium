/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef STELOBJECTMODULE_H
#define STELOBJECTMODULE_H

#include <boost/intrusive_ptr.hpp>
#include "StelModule.hpp"
#include "StelObject.hpp"

/**
 * Manage a collection of StelObject.
 * @author Fabien Chereau <stellarium@free.fr>
 */

class StelObjectModule : public StelModule
{
public:
    StelObjectModule();

    ~StelObjectModule();
	
	//! Search for StelObject in the disk of diameter limitFov centered on the given position
	//! Only visible objects (i.e curretly displayed on screen should be returned)
	//! @param v equatorial position at epoch J2000
	//! @param limitFov angular diameter of the searching zone in degree
	//! @return the list of all the displayed objects contained in the defined zone
	virtual vector<StelObjectP> searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const = 0;
	
	//! Return the matching StelObject if exists or the empty StelObject if not found
	//! @param nameI18n the translated name for the current sky locale
	virtual StelObjectP searchByNameI18n(const wstring& nameI18n) const = 0;
	
	//! Find and return the list of at most maxNbItem objects auto-completing passed object I18 name
	//! @param objPrefix the first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing matches
	virtual vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const = 0;
};

#endif
