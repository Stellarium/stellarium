/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

StelObjectMgr::StelObjectMgr() : objectPointerVisibility(true), searchRadiusPixel(25.f), distanceWeight(1.f)
{
	setObjectName("StelObjectMgr");
}

StelObjectMgr::~StelObjectMgr()
{
}

/*************************************************************************
 Add a new StelObject manager into the list of supported modules.
*************************************************************************/
void StelObjectMgr::registerStelObjectMgr(StelObjectModule* m)
{
	objectsModule.push_back(m);
	typeToModuleMap.insert(m->getStelObjectType(),m);

	objModulesMap.insert(m->objectName(), m->getName());

	//TODO: there should probably be a better way to specify the sub-types
	// instead of hardcoding them here

	// Celestial objects from Solar system by type
	if (m->objectName()=="SolarSystem")
	{
		objModulesMap["SolarSystem:planet"] = "Planets";
		objModulesMap["SolarSystem:moon"] = "Moons";
		objModulesMap["SolarSystem:asteroid"] = "Asteroids";
		objModulesMap["SolarSystem:comet"] = "Comets";
		objModulesMap["SolarSystem:plutino"] = "Plutinos";
		objModulesMap["SolarSystem:cubewano"] = "Cubewanos";
		objModulesMap["SolarSystem:dwarf planet"] = "Dwarf planets";
		objModulesMap["SolarSystem:scattered disc object"] = "Scattered disc objects";
		objModulesMap["SolarSystem:Oort cloud object"] = "Oort cloud objects";
		objModulesMap["SolarSystem:sednoid"] = "Sednoids";
	}
	// Deep-sky objects by type + amateur catalogues
	if (m->objectName()=="NebulaMgr")
	{
		objModulesMap["NebulaMgr:0"] = "Bright galaxies";
		objModulesMap["NebulaMgr:1"] = "Active galaxies";
		objModulesMap["NebulaMgr:2"] = "Radio galaxies";
		objModulesMap["NebulaMgr:3"] = "Interacting galaxies";
		objModulesMap["NebulaMgr:4"] = "Bright quasars";
		objModulesMap["NebulaMgr:5"] = "Star clusters";
		objModulesMap["NebulaMgr:6"] = "Open star clusters";
		objModulesMap["NebulaMgr:7"] = "Globular star clusters";
		objModulesMap["NebulaMgr:8"] = "Stellar associations";
		objModulesMap["NebulaMgr:9"] = "Star clouds";
		objModulesMap["NebulaMgr:10"] = "Nebulae";
		objModulesMap["NebulaMgr:11"] = "Planetary nebulae";
		objModulesMap["NebulaMgr:12"] = "Dark nebulae";
		objModulesMap["NebulaMgr:13"] = "Reflection nebulae";
		objModulesMap["NebulaMgr:14"] = "Bipolar nebulae";
		objModulesMap["NebulaMgr:15"] = "Emission nebulae";
		objModulesMap["NebulaMgr:16"] = "Clusters associated with nebulosity";
		objModulesMap["NebulaMgr:17"] = "HII regions";
		objModulesMap["NebulaMgr:18"] = "Supernova remnants";
		objModulesMap["NebulaMgr:19"] = "Interstellar matter";
		objModulesMap["NebulaMgr:20"] = "Emission objects";
		objModulesMap["NebulaMgr:21"] = "BL Lac objects";
		objModulesMap["NebulaMgr:22"] = "Blazars";
		objModulesMap["NebulaMgr:23"] = "Molecular Clouds";
		objModulesMap["NebulaMgr:24"] = "Young Stellar Objects";
		objModulesMap["NebulaMgr:25"] = "Possible Quasars";
		objModulesMap["NebulaMgr:26"] = "Possible Planetary Nebulae";
		objModulesMap["NebulaMgr:27"] = "Protoplanetary Nebulae";
		objModulesMap["NebulaMgr:29"] = "Symbiotic stars";
		objModulesMap["NebulaMgr:30"] = "Emission-line stars";
		objModulesMap["NebulaMgr:31"] = "Supernova candidates";
		objModulesMap["NebulaMgr:32"] = "Supernova remnant candidates";
		objModulesMap["NebulaMgr:33"] = "Clusters of galaxies";
		objModulesMap["NebulaMgr:100"] = "Messier Catalogue";
		objModulesMap["NebulaMgr:101"] = "Caldwell Catalogue";
		objModulesMap["NebulaMgr:102"] = "Barnard Catalogue";
		objModulesMap["NebulaMgr:103"] = "Sharpless Catalogue";
		objModulesMap["NebulaMgr:104"] = "Van den Bergh Catalogue";
		objModulesMap["NebulaMgr:105"] = "The Catalogue of Rodgers, Campbell, and Whiteoak";
		objModulesMap["NebulaMgr:106"] = "Collinder Catalogue";
		objModulesMap["NebulaMgr:107"] = "Melotte Catalogue";
		objModulesMap["NebulaMgr:108"] = "New General Catalogue";
		objModulesMap["NebulaMgr:109"] = "Index Catalogue";
		objModulesMap["NebulaMgr:110"] = "Lynds' Catalogue of Bright Nebulae";
		objModulesMap["NebulaMgr:111"] = "Lynds' Catalogue of Dark Nebulae";
		objModulesMap["NebulaMgr:112"] = "Principal Galaxy Catalog";
		objModulesMap["NebulaMgr:113"] = "The Uppsala General Catalogue of Galaxies";
		objModulesMap["NebulaMgr:114"] = "Cederblad Catalog";
		objModulesMap["NebulaMgr:115"] = "The Catalogue of Peculiar Galaxies";
		objModulesMap["NebulaMgr:116"] = "The Catalogue of Interacting Galaxies";
		objModulesMap["NebulaMgr:117"] = "The Catalogue of Galactic Planetary Nebulae";
		objModulesMap["NebulaMgr:118"] = "The Strasbourg-ESO Catalogue of Galactic Planetary Nebulae";
		objModulesMap["NebulaMgr:119"] = "A catalogue of Galactic supernova remnants";
		objModulesMap["NebulaMgr:120"] = "A Catalog of Rich Clusters of Galaxies";
		objModulesMap["NebulaMgr:150"] = "Dwarf galaxies";
		objModulesMap["NebulaMgr:151"] = "Herschel 400 Catalogue";
	}
	// Interesting stars
	if (m->objectName()=="StarMgr")
	{
		objModulesMap["StarMgr:0"] = "Interesting double stars";
		objModulesMap["StarMgr:1"] = "Interesting variable stars";
		objModulesMap["StarMgr:2"] = "Bright double stars";
		objModulesMap["StarMgr:3"] = "Bright variable stars";
		objModulesMap["StarMgr:4"] = "Bright stars with high proper motion";
	}
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

StelObjectP StelObjectMgr::searchByID(const QString &type, const QString &id) const
{
	QMap<QString, StelObjectModule*>::const_iterator it = typeToModuleMap.constFind(type);
	if(it!=typeToModuleMap.constEnd())
	{
		return (*it)->searchByID(id);;
	}
	qWarning()<<"StelObject type"<<type<<"unknown";
	return Q_NULLPTR;
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
	if(!lastSelectedObjects.isEmpty())
	{
		lastSelectedObjects.clear();
		emit(selectedObjectChanged(StelModule::RemoveFromSelection));
	}
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

/*****************************************************************************************
 Find and return the list of at most maxNbItem objects auto-completing passed object name
*******************************************************************************************/
QStringList StelObjectMgr::listMatchingObjects(const QString& objPrefix, unsigned int maxNbItem, bool useStartOfWords, bool inEnglish) const
{
	QStringList result;
	if (maxNbItem <= 0)
	{
		return result;
	}

	// For all StelObjectmodules..
	foreach (const StelObjectModule* m, objectsModule)
	{
		// Get matching object for this module
		QStringList matchingObj = m->listMatchingObjects(objPrefix, maxNbItem, useStartOfWords, inEnglish);
		result += matchingObj;
		maxNbItem-=matchingObj.size();
	}

	result.sort();
	return result;
}

QStringList StelObjectMgr::listAllModuleObjects(const QString &moduleId, bool inEnglish) const
{
	// search for module
	StelObjectModule* module = Q_NULLPTR;
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
	if (module == Q_NULLPTR)
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
	return objModulesMap;
}

QVariantMap StelObjectMgr::getObjectInfo(const StelObjectP obj)
{
	QVariantMap map;
	if (!obj)
	{
		qDebug() << "getObjectInfo WARNING - object not found";
		map.insert("found", false);
	}
	else
	{
		map=obj->getInfoMap(StelApp::getInstance().getCore());
		map.insert("found", true);
	}
	return map;
}
