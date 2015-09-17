/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelObjectModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelProjector.hpp"
#include "StelMovementMgr.hpp"
#include "RefractionExtinction.hpp"
#include "StelSkyDrawer.hpp"

#include <QMouseEvent>
#include <QString>
#include <QDebug>
#include <QStringList>

StelObjectMgr::StelObjectMgr() : searchRadiusPixel(25.f), distanceWeight(1.f)
{
	setObjectName("StelObjectMgr");
	objectPointerVisibility = true;
}

StelObjectMgr::~StelObjectMgr()
{
}

/*************************************************************************
 Add a new StelObject manager into the list of supported modules.
*************************************************************************/
void StelObjectMgr::registerStelObjectMgr(StelObjectModule* mgr)
{
	objectsModule.push_back(mgr);
}


StelObjectP StelObjectMgr::searchByNameI18n(const QString &name) const
{
	StelObjectP rval;
	foreach (const StelObjectModule* m, objectsModule)
	{
		rval = m->searchByNameI18n(name);
		if (rval)
			return rval;
	}
	return rval;
}

//! Find any kind of object by its standard program name
StelObjectP StelObjectMgr::searchByName(const QString &name) const
{
	StelObjectP rval;
	foreach (const StelObjectModule* m, objectsModule)
	{
		rval = m->searchByName(name);
		if (rval)
			return rval;
	}
	return rval;
}


//! Find and select an object from its translated name
//! @param nameI18n the case sensitive object translated name
//! @return true if an object was found with the passed name
bool StelObjectMgr::findAndSelectI18n(const QString &nameI18n, StelModule::StelModuleSelectAction action)
{
	// Then look for another object
	StelObjectP obj = searchByNameI18n(nameI18n);
	if (!obj)
		return false;
	else
		return setSelectedObject(obj, action);
}

//! Find and select an object from its standard program name
bool StelObjectMgr::findAndSelect(const QString &name, StelModule::StelModuleSelectAction action)
{
	// Then look for another object
	StelObjectP obj = searchByName(name);
	if (!obj)
		return false;
	else
		return setSelectedObject(obj, action);
}


//! Find and select an object near given equatorial position
bool StelObjectMgr::findAndSelect(const StelCore* core, const Vec3d& pos, StelModule::StelModuleSelectAction action)
{
	StelObjectP tempselect = cleverFind(core, pos);
	return setSelectedObject(tempselect, action);
}

//! Find and select an object near given screen position
bool StelObjectMgr::findAndSelect(const StelCore* core, int x, int y, StelModule::StelModuleSelectAction action)
{
	StelObjectP tempselect = cleverFind(core, x, y);
	return setSelectedObject(tempselect, action);
}

// Find an object in a "clever" way, v in J2000 frame
StelObjectP StelObjectMgr::cleverFind(const StelCore* core, const Vec3d& v) const
{
	StelObjectP sobj;
	QList<StelObjectP> candidates;

	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	// Field of view for a searchRadiusPixel pixel diameter circle on screen
	float fov_around = core->getMovementMgr()->getCurrentFov()/qMin(prj->getViewportWidth(), prj->getViewportHeight()) * searchRadiusPixel;

	// Collect the objects inside the range
	foreach (const StelObjectModule* m, objectsModule)
		candidates += m->searchAround(v, fov_around, core);

	// GZ 2014-08-17: This should be exactly the sky's limit magnitude (or even more, but not less!), else visible stars cannot be clicked.
	float limitMag = core->getSkyDrawer()->getLimitMagnitude(); // -2.f;
	QList<StelObjectP> tmp;
	foreach (const StelObjectP& obj, candidates)
	{
		if (obj->getSelectPriority(core)<=limitMag)
			tmp.append(obj);
	}
	
	candidates = tmp;
	
	// Now select the object minimizing the function y = distance(in pixel) + magnitude
	Vec3d winpos;
	prj->project(v, winpos);
	float xpos = winpos[0];
	float ypos = winpos[1];

	float best_object_value;
	best_object_value = 100000.f;
	foreach (const StelObjectP& obj, candidates)
	{
		prj->project(obj->getJ2000EquatorialPos(core), winpos);
		float distance = std::sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1]))*distanceWeight;
		float priority =  obj->getSelectPriority(core);
		// qDebug() << (*iter).getShortInfoString(core) << ": " << priority << " " << distance;
		if (distance + priority < best_object_value)
		{
			best_object_value = distance + priority;
			sobj = obj;
		}
	}

	return sobj;
}

/*************************************************************************
 Find in a "clever" way an object from its equatorial position
*************************************************************************/
StelObjectP StelObjectMgr::cleverFind(const StelCore* core, int x, int y) const
{
	Vec3d v;
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	if (prj->unProject(x,y,v))
	{
		// Nick Fedoseev patch: improve click match for refracted coordinates
		Vec3d win;
		prj->project(v,win);
		float dx = x - win.v[0];
		float dy = y - win.v[1];
		prj->unProject(x+dx, y+dy, v);

		return cleverFind(core, v);
	}
	return StelObjectP();
}

/*************************************************************************
 Notify that we want to unselect any object
*************************************************************************/
void StelObjectMgr::unSelect(void)
{
	lastSelectedObjects.clear();
	emit(selectedObjectChanged(StelModule::RemoveFromSelection));
}

/*************************************************************************
 Notify that we want to select the given object
*************************************************************************/
bool StelObjectMgr::setSelectedObject(const StelObjectP obj, StelModule::StelModuleSelectAction action)
{
	if (!obj)
	{
		unSelect();
		return false;
	}

	// An object has been found
	QList<StelObjectP> objs;
	objs.push_back(obj);
	return setSelectedObject(objs, action);
}

/*************************************************************************
 Notify that we want to select the given objects
*************************************************************************/
bool StelObjectMgr::setSelectedObject(const QList<StelObjectP>& objs, StelModule::StelModuleSelectAction action)
{
	if (action==StelModule::AddToSelection)
		lastSelectedObjects.append(objs);
	else
		lastSelectedObjects = objs;
	emit(selectedObjectChanged(action));
	return true;
}

/*************************************************************************
 Return the list objects of type "type" which was recently selected by
  the user
*************************************************************************/
QList<StelObjectP> StelObjectMgr::getSelectedObject(const QString& type)
{
	QList<StelObjectP> result;
	for (QList<StelObjectP>::iterator iter=lastSelectedObjects.begin();iter!=lastSelectedObjects.end();++iter)
	{
		if ((*iter)->getType()==type)
			result.push_back(*iter);
	}
	return result;
}


/*************************************************************************
 Find and return the list of at most maxNbItem objects auto-completing
 passed object I18 name
*************************************************************************/
QStringList StelObjectMgr::listMatchingObjectsI18n(const QString& objPrefix, unsigned int maxNbItem, bool useStartOfWords) const
{
	QStringList result;

	// For all StelObjectmodules..
	foreach (const StelObjectModule* m, objectsModule)
	{
		// Get matching object for this module
		QStringList matchingObj = m->listMatchingObjectsI18n(objPrefix, maxNbItem, useStartOfWords);
		result += matchingObj;
		maxNbItem-=matchingObj.size();
	}

	result.sort();
	return result;
}

/*************************************************************************
 Find and return the list of at most maxNbItem objects auto-completing
 passed object English name
*************************************************************************/
QStringList StelObjectMgr::listMatchingObjects(const QString& objPrefix, unsigned int maxNbItem, bool useStartOfWords) const
{
	QStringList result;

	// For all StelObjectmodules..
	foreach (const StelObjectModule* m, objectsModule)
	{
		// Get matching object for this module
		QStringList matchingObj = m->listMatchingObjects(objPrefix, maxNbItem, useStartOfWords);
		result += matchingObj;
		maxNbItem-=matchingObj.size();
	}

	result.sort();
	return result;
}

QStringList StelObjectMgr::listAllModuleObjects(const QString &moduleId, bool inEnglish) const
{
	// search for module
	StelObjectModule* module = NULL;
	QStringList result, list;
	QString objModule, objType;
	bool subSet = false;
	if (moduleId.contains(":"))
	{
		subSet = true;
		list = moduleId.split(":", QString::SkipEmptyParts);
		objModule = list.at(0);
		objType = list.at(1);
	}
	else
		objModule = moduleId;
	foreach(StelObjectModule* m, objectsModule)
	{
		if (m->objectName() == objModule)
		{
			module = m;
			break;
		}
	}
	if (module == NULL)
	{
		qWarning() << "Can't find module with id " << objModule;
		return QStringList();
	}
	if (subSet)
		result = module->listAllObjectsByType(objType, inEnglish);
	else
		result = module->listAllObjects(inEnglish);

	return result;
}

QMap<QString, QString> StelObjectMgr::objectModulesMap() const
{
	QMap<QString, QString> result;
	foreach(const StelObjectModule* m, objectsModule)
	{
		result[m->objectName()] = m->getName();
		// Celestial objects from Solar system by type
		if (m->objectName()=="SolarSystem")
		{
			result["SolarSystem:planet"] = "Planets";
			result["SolarSystem:moon"] = "Moons";
			result["SolarSystem:asteroid"] = "Asteroids";
			result["SolarSystem:comet"] = "Comets";
			result["SolarSystem:plutino"] = "Plutinos";
			result["SolarSystem:cubewano"] = "Cubewanos";
			result["SolarSystem:dwarf planet"] = "Dwarf planets";
			result["SolarSystem:scattered disc object"] = "Scattered disc objects";
			result["SolarSystem:Oort cloud object"] = "Oort cloud objects";
		}
		// Deep-sky objects by type + amateur catalogues
		if (m->objectName()=="NebulaMgr")
		{
			result["NebulaMgr:0"] = "Bright galaxies";
			result["NebulaMgr:1"] = "Active galaxies";
			result["NebulaMgr:2"] = "Radio galaxies";
			result["NebulaMgr:3"] = "Interacting galaxies";
			result["NebulaMgr:4"] = "Bright quasars";
			result["NebulaMgr:5"] = "Star clusters";
			result["NebulaMgr:6"] = "Open star clusters";
			result["NebulaMgr:7"] = "Globular star clusters";
			result["NebulaMgr:8"] = "Stellar associations";
			result["NebulaMgr:9"] = "Star clouds";
			result["NebulaMgr:10"] = "Nebulae";
			result["NebulaMgr:11"] = "Planetary nebulae";
			result["NebulaMgr:12"] = "Dark nebulae";
			result["NebulaMgr:13"] = "Reflection nebulae";
			result["NebulaMgr:14"] = "Bipolar nebulae";
			result["NebulaMgr:15"] = "Emission nebulae";
			result["NebulaMgr:16"] = "Clusters associated with nebulosity";
			result["NebulaMgr:17"] = "HII regions";			
			result["NebulaMgr:18"] = "Supernova remnants";
			result["NebulaMgr:19"] = "Interstellar matter";
			result["NebulaMgr:20"] = "Emission objects";
			result["NebulaMgr:21"] = "BL Lac objects";
			result["NebulaMgr:22"] = "Blazars";
			result["NebulaMgr:23"] = "Molecular Clouds";
			result["NebulaMgr:24"] = "Young Stellar Objects";
			result["NebulaMgr:25"] = "Possible Quasars";
			result["NebulaMgr:26"] = "Possible Planetary Nebulae";
			result["NebulaMgr:27"] = "Protoplanetary Nebulae";
			result["NebulaMgr:100"] = "Messier Catalogue";
			result["NebulaMgr:101"] = "Caldwell Catalogue";
			result["NebulaMgr:102"] = "Barnard Catalogue";
			result["NebulaMgr:103"] = "Sharpless Catalogue";
			result["NebulaMgr:104"] = "Van den Bergh Catalogue";
			result["NebulaMgr:105"] = "The Catalogue of Rodgers, Campbell, and Whiteoak";
			result["NebulaMgr:106"] = "Collinder Catalogue";
			result["NebulaMgr:107"] = "Melotte Catalogue";
		}
		// Interesting stars
		if (m->objectName()=="StarMgr")
		{
			result["StarMgr:0"] = "Interesting double stars";
			result["StarMgr:1"] = "Interesting variable stars";
		}
	}
	return result;
}
