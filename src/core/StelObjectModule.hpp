/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
 * Copyright (C) 2016 Marcos Cardinot
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

#ifndef STELOBJECTMODULE_HPP
#define STELOBJECTMODULE_HPP

#include "StelModule.hpp"
#include "StelObjectType.hpp"
#include "VecMath.hpp"

#include <QList>
#include <QString>
#include <QStringList>

//! @class StelObjectModule
//! Specialization of StelModule which manages a collection of StelObject.
//! Instances deriving from the StelObjectModule class can be managed by the StelObjectMgr.
//! The class defines extra abstract functions for searching and listing StelObjects.
class StelObjectModule : public StelModule
{
	Q_OBJECT
public:
	StelObjectModule();
	~StelObjectModule();
	
	//! Search for StelObject in an area around a specified point.
	//! The function searches in a disk of diameter limitFov centered on v.
	//! Only visible objects (i.e. currently displayed on screen) should be returned.
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

	//! Return the StelObject with the given ID if exists or the empty StelObject if not found
	//! @param name the english object name
	virtual StelObjectP searchByID(const QString& id) const = 0;

	//! Find and return the list of at most maxNbItem objects auto-completing passed object name
	//! @param objPrefix the first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords decide if start of word is searched	
	//! @return a list of matching object name by order of relevance, or an empty list if nothing matches
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;

	//! List all StelObjects.
	//! @param inEnglish list names in English (true) or translated (false)
	//! @return a list of matching object name by order of relevance, or an empty list if nothing matches
	virtual QStringList listAllObjects(bool inEnglish) const = 0;

	//! List all StelObjects by type.
	//! @param objType object type
	//! @param inEnglish list translated names (false) or in English (true)
	//! @return a list of matching object name by order of relevance, or an empty list if nothing matches
	virtual QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const;

	//! Gets a user-displayable name of the object category
	virtual QString getName() const = 0;
	//! Returns the name that will be returned by StelObject::getType() for the objects this module manages
	virtual QString getStelObjectType() const = 0;

	//! Auxiliary method of listMatchingObjects()
	//! @param objName object name
	//! @param objPrefix the first letters of the searched object
	//! @param useStartOfWords decide if start of word is searched
	//! @return true if it matches
	bool matchObjectName(const QString& objName, const QString& objPrefix, bool useStartOfWords) const;
};

#endif // STELOBJECTMODULE_HPP
