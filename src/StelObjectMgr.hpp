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

#ifndef STELOBJECTMGR_HPP_
#define STELOBJECTMGR_HPP_

#include <vector>
#include "vecmath.h"
#include "StelModule.hpp"
#include "StelObject.hpp"

class StelObjectModule;
class StelCore;

//! @class StelObjectMgr 
//! Manage the selection and queries on one or more StelObjects.
//! When the user requests selection of an object, the selectedObjectChangeCallBack method
//! of all the StelModule which are registered is called.
//! Each module is then free to manage object selection as it wants.
class StelObjectMgr : public StelModule
{
public:
	StelObjectMgr();
	virtual ~StelObjectMgr();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb) {;}
	virtual string getModuleID() const {return "StelObjectMgr";}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye) {return 0.;}
	virtual void update(double deltaTime) {;}
	
	///////////////////////////////////////////////////////////////////////////
	
	//! Add a new StelObject manager into the list of supported modules.
	//! Registered modules can have selected objects 
	void registerStelObjectMgr(StelObjectModule* mgr);
	
	//! Find and select an object near given equatorial position.
	//! @param added add the found object to selection if true, replace if false
	//! @return true if a object was found at position (this does not necessarily means it is selected)
	bool findAndSelect(const StelCore* core, const Vec3d& pos, bool added=false);

	//! Find and select an object near given screen position.
	//! @param added add the found object to selection if true, replace if false
	//! @return true if a object was found at position (this does not necessarily means it is selected)
	bool findAndSelect(const StelCore* core, int x, int y, bool added=false);

	//! Find and select an object from its translated name.
	//! @param added add the found object to selection if true, replace if false
	//! @param nameI18n the case sensitive object translated name
	//! @return true if a object with the passed name was found
	bool findAndSelectI18n(const wstring &nameI18n, bool added=false);

	//! Find and select an object from its standard program name.
	//! @param added add the found object to selection if true, replace if false
	//! @param name the case sensitive object translated name
	//! @return true if a object with the passed name was found
	bool findAndSelect(const string &name, bool added=false);

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const;

	//! Return whether an object was selected during last selection related event.
	bool getWasSelected(void) const {return !lastSelectedObjects.empty();}

	//! Notify that we want to unselect any object.
	void unSelect(void);

	//! Notify that we want to select the given object.
	//! @param added add the object to selection if true, replace if false
	//! @return true if at least 1 object was sucessfully selected
	bool setSelectedObject(const StelObjectP obj, bool added=false);
	
	//! Notify that we want to select the given objects.
	//! @param objs a vector of objects to select
	//! @param added add the object to selection if true, replace if false
	//! @return true if at least 1 object was sucessfully selected
	bool setSelectedObject(const std::vector<StelObjectP>& objs, bool added=false);

	//! Get the list objects which was recently selected by the user.
	const std::vector<StelObjectP>& getSelectedObject() const {return lastSelectedObjects;}
	
	//! Return the list objects of type "withType" which was recently selected by the user.
	//! @param type return only objects of the given type 
	std::vector<StelObjectP> getSelectedObject(const std::string& type);

	//! Set whether a pointer is to be drawn over selected object.
	void setFlagSelectedObjectPointer(bool b) { object_pointer_visibility = b; }
	
private:
	std::vector<StelObjectModule*> objectsModule;	// The list of StelObjectModule that are referenced in Stellarium
	
	std::vector<StelObjectP> lastSelectedObjects;	// The last selected object in stellarium
	
	bool object_pointer_visibility;		// Should selected object pointer be drawn

	//! Find any kind of object by its translated name.
	StelObjectP searchByNameI18n(const wstring &name) const;

	//! Find any kind of object by its standard program name.
	StelObjectP searchByName(const string &name) const;

	//! Find in a "clever" way an object from its equatorial position.
	StelObjectP cleverFind(const StelCore* core, const Vec3d& pos) const;
	
	//! Find in a "clever" way an object from its screen position.
	StelObjectP cleverFind(const StelCore* core, int x, int y) const;	
};

#endif /*SELECTIONMGR_HPP_*/
