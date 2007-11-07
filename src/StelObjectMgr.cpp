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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelObjectModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "Projector.hpp"

StelObjectMgr::StelObjectMgr()
{
	setObjectName("StelObjectMgr");
	object_pointer_visibility = true;
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


StelObjectP StelObjectMgr::searchByNameI18n(const wstring &name) const
{
	StelObjectP rval;
	std::vector<StelObjectModule*>::const_iterator iter;
	for (iter=objectsModule.begin();iter!=objectsModule.end();++iter)
	{
		rval = (*iter)->searchByNameI18n(name);
		if (rval)
			return rval;
	}
	return rval;
}

//! Find any kind of object by its standard program name
StelObjectP StelObjectMgr::searchByName(const string &name) const
{
	StelObjectP rval;
	std::vector<StelObjectModule*>::const_iterator iter;
	for (iter=objectsModule.begin();iter!=objectsModule.end();++iter)
	{
		rval = (*iter)->searchByName(name);
		if (rval)
			return rval;
	}
	return rval;
}


//! Find and select an object from its translated name
//! @param nameI18n the case sensitive object translated name
//! @return true if an object was found with the passed name
bool StelObjectMgr::findAndSelectI18n(const wstring &nameI18n, bool added)
{
	// Then look for another object
	StelObjectP obj = searchByNameI18n(nameI18n);
	if (!obj)
		return false;
	else
		return setSelectedObject(obj, added);
}

//! Find and select an object from its standard program name
bool StelObjectMgr::findAndSelect(const string &name, bool added)
{
	// Then look for another object
	StelObjectP obj = searchByName(name);
	if (!obj)
		return false;
	else
		return setSelectedObject(obj, added);
}


//! Find and select an object near given equatorial position
bool StelObjectMgr::findAndSelect(const StelCore* core, const Vec3d& pos, bool added)
{
	StelObjectP tempselect = cleverFind(core, pos);
	return setSelectedObject(tempselect, added);
}

//! Find and select an object near given screen position
bool StelObjectMgr::findAndSelect(const StelCore* core, int x, int y, bool added)
{
	StelObjectP tempselect = cleverFind(core, x, y);
	return setSelectedObject(tempselect, added);
}

// Find an object in a "clever" way, v in J2000 frame
StelObjectP StelObjectMgr::cleverFind(const StelCore* core, const Vec3d& v) const
{
	StelObjectP sobj;
	vector<StelObjectP> candidates;
	vector<StelObjectP> temp;

	// Field of view for a 30 pixel diameter circle on screen
	float fov_around = core->getProjection()->getFov()/MY_MIN(core->getProjection()->getViewportWidth(), core->getProjection()->getViewportHeight()) * 30.f;

	// Collect the objects inside the range
	std::vector<StelObjectModule*>::const_iterator iteromgr;
	for (iteromgr=objectsModule.begin();iteromgr!=objectsModule.end();++iteromgr)
	{
		temp = (*iteromgr)->searchAround(v, fov_around, core->getNavigation(), core->getProjection());
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}
	
	// Now select the object minimizing the function y = distance(in pixel) + magnitude
	Vec3d winpos;
	core->getProjection()->setCurrentFrame(Projector::FRAME_J2000);
	core->getProjection()->project(v, winpos);
	float xpos = winpos[0];
	float ypos = winpos[1];

	core->getProjection()->setCurrentFrame(Projector::FRAME_EARTH_EQU);
	float best_object_value;
	best_object_value = 100000.f;
	vector<StelObjectP>::iterator iter = candidates.begin();
	while (iter != candidates.end())
	{
		core->getProjection()->project((*iter)->getEarthEquatorialPos(core->getNavigation()), winpos);
		float distance = sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1]));
		float priority =  (*iter)->getSelectPriority(core->getNavigation());
//		cerr << StelUtils::wstringToString((*iter).getShortInfoString(core->getNavigation())) << ": " << priority << " " << distance << endl;
		if (distance + priority < best_object_value)
		{
			best_object_value = distance + priority;
			sobj = *iter;
		}
		++iter;
	}

	return sobj;
}

/*************************************************************************
 Find in a "clever" way an object from its equatorial position
*************************************************************************/
StelObjectP StelObjectMgr::cleverFind(const StelCore* core, int x, int y) const
{
	Vec3d v;
	core->getProjection()->setCurrentFrame(Projector::FRAME_J2000);
	core->getProjection()->unProject(x,y,v);
	return cleverFind(core, v);
}

/*************************************************************************
 Notify that we want to unselect any object
*************************************************************************/
void StelObjectMgr::unSelect(void)
{
	lastSelectedObjects.clear();
	
	// Send the event to every StelModule
	foreach (StelModule* iter, StelApp::getInstance().getModuleMgr().getAllModules())
	{
		iter->selectedObjectChangeCallBack(false);
	}
}

/*************************************************************************
 Notify that we want to select the given object
*************************************************************************/
bool StelObjectMgr::setSelectedObject(const StelObjectP obj, bool added)
{
	if (!obj)
	{
		unSelect();
		return false;
	}
	
	// An object has been found
	std::vector<StelObjectP> objs;
	objs.push_back(obj);
	return setSelectedObject(objs, added);
}

/*************************************************************************
 Notify that we want to select the given objects
*************************************************************************/
bool StelObjectMgr::setSelectedObject(const std::vector<StelObjectP>& objs, bool added)
{
	lastSelectedObjects=objs;

	// Send the event to every StelModule
	foreach (StelModule* iter, StelApp::getInstance().getModuleMgr().getAllModules())
	{
		iter->selectedObjectChangeCallBack(added);
	}
	return true;
}

/*************************************************************************
 Return the list objects of type "withType" which was recently selected by
  the user
*************************************************************************/
std::vector<StelObjectP> StelObjectMgr::getSelectedObject(const std::string& type)
{
	std::vector<StelObjectP> result;
	for (std::vector<StelObjectP>::iterator iter=lastSelectedObjects.begin();iter!=lastSelectedObjects.end();++iter)
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
vector<wstring> StelObjectMgr::listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem) const
{
	vector<wstring> result;
	vector <wstring>::const_iterator iter;

	// For all StelObjectmodules..
	std::vector<StelObjectModule*>::const_iterator iteromgr;
	for (iteromgr=objectsModule.begin();iteromgr!=objectsModule.end();++iteromgr)
	{
		// Get matching object for this module
		vector<wstring> matchingObj = (*iteromgr)->listMatchingObjectsI18n(objPrefix, maxNbItem);
		for (iter = matchingObj.begin(); iter != matchingObj.end(); ++iter)
			result.push_back(*iter);
		maxNbItem-=matchingObj.size();
	}

	sort(result.begin(), result.end());

	return result;
}
