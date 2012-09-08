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

StelObjectMgr::StelObjectMgr() : searchRadiusPixel(30.f), distanceWeight(1.f)
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
		float distance = sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1]))*distanceWeight;
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
	if (core->getProjection(StelCore::FrameJ2000)->unProject(x,y,v))
	{
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
 Return the list objects of type "withType" which was recently selected by
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
QStringList StelObjectMgr::listMatchingObjectsI18n(const QString& objPrefix, unsigned int maxNbItem) const
{
	QStringList result;

	// For all StelObjectmodules..
	foreach (const StelObjectModule* m, objectsModule)
	{
		// Get matching object for this module
		QStringList matchingObj = m->listMatchingObjectsI18n(objPrefix, maxNbItem);
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
	foreach(StelObjectModule* m, objectsModule)
	{
		if (m->objectName() == moduleId)
		{
			module = m;
			break;
		}
	}
	if (module == NULL)
	{
		qWarning() << "Can't find module with id " << moduleId;
		return QStringList();
	}
	return module->listAllObjects(inEnglish);
}

QMap<QString, QString> StelObjectMgr::objectModulesMap() const
{
	QMap<QString, QString> result;
	foreach(const StelObjectModule* m, objectsModule)
	{
		result[m->objectName()] = m->getName();
	}
	return result;
}
