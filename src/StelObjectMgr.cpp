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

StelObjectMgr::StelObjectMgr() : selectedObject(NULL)
{
	object_pointer_visibility = true;
}

StelObjectMgr::~StelObjectMgr()
{
	// release the previous StelObject:
	selectedObject = StelObject();
}
		
/*************************************************************************
 Add a new StelObject manager into the list of supported modules.
*************************************************************************/
void StelObjectMgr::registerStelObjectMgr(StelObjectModule* mgr)
{
	objectsModule.push_back(mgr);
}


StelObject StelObjectMgr::searchByNameI18n(const wstring &name) const
{
	StelObject rval;
	std::vector<StelObjectModule*>::const_iterator iter;
	for (iter=objectsModule.begin();iter!=objectsModule.end();++iter)
	{
		rval = (*iter)->searchByNameI18n(name);
		if (rval)
			return rval;
	}
	return rval;
}

//! Find and select an object from its translated name
//! @param nameI18n the case sensitive object translated name
//! @return true if an object was found with the passed name
bool StelObjectMgr::findAndSelectI18n(const wstring &nameI18n)
{
	// Then look for another object
	StelObject obj = searchByNameI18n(nameI18n);
	if (!obj)
		return false;
	else
		return setSelectedObject(obj);
}


//! Find and select an object based on selection type and standard name or number
//! @return true if an object was selected
//
//bool StelObjectMgr::setSelectedObject(const string &type, const string &id)
//{
//	/*
//	  std::wostringstream oss;
//	  oss << id.c_str();
//	  return findAndSelectI18n(oss.str());
//	*/
//	if(type=="hp")
//	{
//		unsigned int hpnum;
//		std::istringstream istr(id);
//		istr >> hpnum;
//		selectedObject = hip_stars->searchHP(hpnum);
//		asterisms->setSelected(selectedObject);
//		ssystem->setSelected("");
//
//	}
//	else if(type=="star")
//	{
//		selectedObject = hip_stars->search(id);
//		asterisms->setSelected(selectedObject);
//		ssystem->setSelected("");
//
//	}
//	else if(type=="planet")
//	{
//		ssystem->setSelected(id);
//		selectedObject = ssystem->getSelected();
//		asterisms->setSelected(StelObject());
//
//	}
//	else if(type=="nebula")
//	{
//		selectedObject = nebulas->search(id);
//		ssystem->setSelected("");
//		asterisms->setSelected(StelObject());
//
//	}
//	else if(type=="constellation")
//	{
//		// Select only constellation, nothing else
//		asterisms->setSelected(id);
//		selectedObject = NULL;
//		ssystem->setSelected("");
//	}
//	else if(type=="constellation_star")
//	{
//		// For Find capability, select a star in constellation so can center view on constellation
//		selectedObject = asterisms->setSelectedStar(id);
//		ssystem->setSelected("");
//	}
//	else
//	{
//		cerr << "Invalid selection type specified: " << type << endl;
//		return 0;
//	}
//
//
//	if (selectedObject)
//	{
//		if (movementMgr->getFlagTracking())
//			movementMgr->setFlagLockEquPos(true);
//		movementMgr->setFlagTracking(false);
//
//		return 1;
//	}
//
//	return 0;
//}


//! Find and select an object near given equatorial position
bool StelObjectMgr::findAndSelect(const StelCore* core, const Vec3d& pos)
{
	StelObject tempselect = cleverFind(core, pos);
	return setSelectedObject(tempselect);
}

//! Find and select an object near given screen position
bool StelObjectMgr::findAndSelect(const StelCore* core, int x, int y)
{
	StelObject tempselect = cleverFind(core, x, y);
	return setSelectedObject(tempselect);
}

// Find an object in a "clever" way, v in J2000 frame
StelObject StelObjectMgr::cleverFind(const StelCore* core, const Vec3d& v) const
{
	StelObject sobj;
	vector<StelObject> candidates;
	vector<StelObject> temp;

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
	vector<StelObject>::iterator iter = candidates.begin();
	while (iter != candidates.end())
	{
		core->getProjection()->project((*iter).get_earth_equ_pos(core->getNavigation()), winpos);
		float distance = sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1]));
		float priority =  (*iter).getSelectPriority(core->getNavigation());
		if (distance + priority < best_object_value)
		{
			best_object_value = distance + priority;
			sobj = *iter;
		}
		++iter;
	}

	return sobj;
}

StelObject StelObjectMgr::cleverFind(const StelCore* core, int x, int y) const
{
	Vec3d v;
	core->getProjection()->setCurrentFrame(Projector::FRAME_J2000);
	core->getProjection()->unProject(x,core->getProjection()->getViewportHeight()-y,v);
	return cleverFind(core, v);
}

//! Deselect selected object if any
void StelObjectMgr::unSelect(void)
{
	selectedObject = NULL;
	
	// Send the event to every StelModule
	StelModuleMgr& mmgr = StelApp::getInstance().getModuleMgr();
	for (StelModuleMgr::Iterator iter=mmgr.begin();iter!=mmgr.end();++iter)
	{
		(*iter)->selectedObjectChangeCallBack();
	}
}

//! Select passed object
//! @return true if the object was selected (false if the same was already selected)
bool StelObjectMgr::setSelectedObject(const StelObject &obj)
{
	// Unselect if it is the same object
	if (obj && selectedObject==obj)
	{
		unSelect();
		return true;
	}

	selectedObject = obj;

	// If an object has been found
	if (selectedObject)
	{
		// potentially record this action
		//if (!recordActionCallback.empty())
		//	recordActionCallback("select " + selectedObject.getEnglishName());

		// Send the event to every StelModule
		StelModuleMgr& mmgr = StelApp::getInstance().getModuleMgr();
		for (StelModuleMgr::Iterator iter=mmgr.begin();iter!=mmgr.end();++iter)
		{
			(*iter)->selectedObjectChangeCallBack();
		}
		return true;
	}
	else
	{
		unSelect();
		return false;
	}

	assert(0);	// Non reachable code
}


//! Find and return the list of at most maxNbItem objects auto-completing passed object I18 name
//! @param objPrefix the first letters of the searched object
//! @param maxNbItem the maximum number of returned object names
//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
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
