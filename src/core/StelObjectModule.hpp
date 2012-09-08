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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELOBJECTMODULE_HPP_
#define _STELOBJECTMODULE_HPP_

#include <QList>
#include <QString>
#include <QStringList>
#include "StelModule.hpp"
#include "StelObjectType.hpp"
#include "VecMath.hpp"

//! @class StelObjectModule
//! Specialization of StelModule which manages a collection of StelObject.
//! Instances deriving from the StelObjectModule class can be managed by the StelObjectMgr.
//! The class defines extra abstract functions for searching and listing StelObjects.
class StelObjectModule : public StelModule
{
public:
	StelObjectModule();
	~StelObjectModule();
	
	//! Search for StelObject in an area around a specifid point.
	//! The function searches in a disk of diameter limitFov centered on v.
	//! Only visible objects (i.e curretly displayed on screen) should be returned.
	//! @param v equatorial position at epoch J2000.
	//! @param limitFov angular diameter of the searching zone in degree.
	//! @param core the core instance to use.
	//! @return the list of all the displayed objects contained in the defined zone.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const = 0;
	
	//! Find a StelObject by name.
	//! @param nameI18n The translated name for the current sky locale.
	//! @return The matching StelObject if exists or the empty StelObject if not found.
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const = 0;

	//! Return the matching StelObject if exists or the empty StelObject if not found
	//! @param name the english object name
	virtual StelObjectP searchByName(const QString& name) const = 0;
	
	//! Find and return the list of at most maxNbItem objects auto-completing passed object I18 name
	//! @param objPrefix the first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing matches
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const = 0;

	virtual QStringList listAllObjects(bool inEnglish) const = 0;

	virtual QString getName() const = 0;
};

#endif // _STELOBJECTMODULE_HPP_
