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

#ifndef STELOBJECTMGR_H
#define STELOBJECTMGR_H

#include "stelmodule.h"
#include "stel_object.h"
#include "translator.h"

/**
 * Manage a collection of StelObject.
 * @author Fabien Chéreau <stellarium@free.fr>
 */

class StelObjectMgr : public StelModule
{
public:
    StelObjectMgr();

    ~StelObjectMgr();
	
	//! Search for StelObject in the disk of diameter limitFov centered on the given position
	//! @param v equatorial position at epoch J2000
	//! @param limitFov angular diameter of the searching zone in degree
	//! @return the list of all the objects contained in the defined zone
	virtual vector<StelObject> searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const = 0;
	
	//! Return the matching StelObject if exists or the empty StelObject if not found
	//! @param nameI18n the translated name for the current sky locale
	virtual StelObject searchByNameI18n(const wstring& nameI18n) const = 0;
	
	//! Find and return the list of at most maxNbItem objects auto-completing passed object I18 name
	//! @param objPrefix the first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing matches
	virtual vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const = 0;
	
	//! Update i18n names of the sky objects from english names according to passed translator
	//! The translation is done internally using gettext with translated strings defined in the .po files
	virtual void translateSkyNames(Translator& trans) = 0;
	
	//! Set the current sky culture from the passed directory
	//! Reload and translate any data file for the given sky culture/translation if necessary
	//! @param cultureDir the directory containing data files for the culture
	//! @return true if the data for the culture was successfuly found, false otherwise
	virtual bool setSkyCultureDir(const string& cultureDir) = 0;
};

#endif
