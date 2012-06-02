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
#include <QSettings>

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

int StelObjectMgr::getCustomInfoString(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	bool Name = conf->value("custom_selected_info/flag_show_name", false).toBool();
	bool CatalogNumber = conf->value("custom_selected_info/flag_show_catalognumber", false).toBool();
	bool Magnitude = conf->value("custom_selected_info/flag_show_magnitude", false).toBool();
	bool RaDecJ2000 = conf->value("custom_selected_info/flag_show_radecj2000", false).toBool();
	bool RaDecOfDate = conf->value("custom_selected_info/flag_show_radecofdate", false).toBool();
	bool AltAz = conf->value("custom_selected_info/flag_show_altaz", false).toBool();
	bool Distance = conf->value("custom_selected_info/flag_show_distance", false).toBool();
	bool Size = conf->value("custom_selected_info/flag_show_size", false).toBool();
	bool Extra1 = conf->value("custom_selected_info/flag_show_extra1", false).toBool();
	bool Extra2 = conf->value("custom_selected_info/flag_show_extra2", false).toBool();
	bool Extra3 = conf->value("custom_selected_info/flag_show_extra3", false).toBool();
	bool HourAngle = conf->value("custom_selected_info/flag_show_hourangle", false).toBool();
	bool AbsoluteMagnitude = conf->value("custom_selected_info/flag_show_absolutemagnitude", false).toBool();

	int OctZero = 0x00000000;

	int NameOct;
	if (Name)
	{
		NameOct = StelObject::Name;
	}
	else
	{
		NameOct = OctZero;
	}

	int CatalogNumberOct;
	if (CatalogNumber)
	{
		CatalogNumberOct = StelObject::CatalogNumber;
	}
	else
	{
		CatalogNumberOct = OctZero;
	}

	int MagnitudeOct;
	if (Magnitude)
	{
		MagnitudeOct = StelObject::Magnitude;
	}
	else
	{
		MagnitudeOct = OctZero;
	}

	int RaDecJ2000Oct;
	if (RaDecJ2000)
	{
		RaDecJ2000Oct = StelObject::RaDecJ2000;
	}
	else
	{
		RaDecJ2000Oct = OctZero;
	}

	int RaDecOfDateOct;
	if (RaDecOfDate)
	{
		RaDecOfDateOct = StelObject::RaDecOfDate;
	}
	else
	{
		RaDecOfDateOct = OctZero;
	}

	int AltAzOct;
	if (AltAz)
	{
		AltAzOct = StelObject::AltAzi;
	}
	else
	{
		AltAzOct = OctZero;
	}

	int DistanceOct;
	if (Distance)
	{
		DistanceOct = StelObject::Distance;
	}
	else
	{
		DistanceOct = OctZero;
	}

	int SizeOct;
	if (Size)
	{
		SizeOct = StelObject::Size;
	}
	else
	{
		SizeOct = OctZero;
	}

	int Extra1Oct;
	if (Extra1)
	{
		Extra1Oct = StelObject::Extra1;
	}
	else
	{
		Extra1Oct = OctZero;
	}

	int Extra2Oct;
	if (Extra2)
	{
		Extra2Oct = StelObject::Extra2;
	}
	else
	{
		Extra2Oct = OctZero;
	}

	int Extra3Oct;
	if (Extra3)
	{
		Extra3Oct = StelObject::Extra3;
	}
	else
	{
		Extra3Oct = OctZero;
	}

	int HourAngleOct;
	if (HourAngle)
	{
		HourAngleOct = StelObject::HourAngle;
	}
	else
	{
		HourAngleOct = OctZero;
	}

	int AbsoluteMagnitudeOct;
	if (AbsoluteMagnitude)
	{
		AbsoluteMagnitudeOct = StelObject::AbsoluteMagnitude;
	}
	else
	{
		AbsoluteMagnitudeOct = OctZero;
	}

	return (NameOct|CatalogNumberOct|MagnitudeOct|RaDecJ2000Oct|RaDecOfDateOct|AltAzOct|DistanceOct|SizeOct|Extra1Oct|Extra2Oct|Extra3Oct|HourAngleOct|AbsoluteMagnitudeOct);
}
