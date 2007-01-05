/*
 * Copyright (C) 2003 Fabien Chereau
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

#ifndef _STEL_CORE_H_
#define _STEL_CORE_H_

#include <string>

#include "StelObject.hpp"
#include "meteor_mgr.h"
#include "image_mgr.h"
#include "callbacks.hpp"


//!  @brief Main class for stellarium core processing.
//!
//! Manage all the base modules which must be present in stellarium
class StelCore
{
public:
	//! Possible mount modes
	enum MOUNT_MODE { MOUNT_ALTAZIMUTAL, MOUNT_EQUATORIAL };

	// Inputs are the locale directory and root directory and callback function for recording actions
    StelCore(const string& LDIR, const string& DATA_ROOT, const boost::callback <void, string> & recordCallback);
    virtual ~StelCore();

	//! Init and load all main core components from the passed config file.
	void init(const InitParser& conf, LoadingBar& lb);

	//! Init projection temp TODO remove
	void initProj(const InitParser& conf);

	//! Update all the objects with respect to the time.
	//! @param delta_time the time increment in ms.
	void update(int delta_time);

	//! Execute all the drawing functions
	//! @param delta_time the time increment in ms.
	//! @returns the max squared distance in pixels any single object has moved since the previous update.
	double draw(int delta_time);
	
	//! Update the sky culture for all the modules
	void updateSkyCulture();


	//! Get the current projector used in the core
	Projector* getProjection() {return projection;}
	
	//! Get the current navigation (manages frame transformation) used in the core
	Navigator* getNavigation() {return navigation;}

	//! Get the current tone converter used in the core
	ToneReproducer* getToneReproductor() {return tone_converter;}

	//! Set current mount type
	void setMountMode(MOUNT_MODE m) {navigation->setViewingMode((m==MOUNT_ALTAZIMUTAL) ? Navigator::VIEW_HORIZON : Navigator::VIEW_EQUATOR);}
	//! Get current mount type
	MOUNT_MODE getMountMode(void) {return ((navigation->getViewingMode()==Navigator::VIEW_HORIZON) ? MOUNT_ALTAZIMUTAL : MOUNT_EQUATORIAL);}
	//! Toggle current mount mode between equatorial and altazimutal
	void toggleMountMode(void) {if (getMountMode()==MOUNT_ALTAZIMUTAL) setMountMode(MOUNT_EQUATORIAL); else setMountMode(MOUNT_ALTAZIMUTAL);}

	//! Find and select an object near given equatorial position
	//! @return true if a object was found at position (this does not necessarily means it is selected)
	bool findAndSelect(const Vec3d& pos);

	//! Find and select an object near given screen position
	//! @return true if a object was found at position (this does not necessarily means it is selected)
	bool findAndSelect(int x, int y);

	//! Find and select an object from its translated name
	//! @param nameI18n the case sensitive object translated name
	//! @return true if a object was found with the passed name
	bool findAndSelectI18n(const wstring &nameI18n);

	//! Find and select an object based on selection type and standard name or number
	//! @return true if an object was selected
	bool selectObject(const string &type, const string &id);


	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const;

	///////////////////////////////////////////////////////////////////////////////////////
	// Selection things: TODO move into a new SelectionMgr class?
	
	//! Return whether an object is currently selected
	bool getFlagHasSelected(void) {return selected_object;}

	//! Deselect selected object if any
	//! Does not deselect selected constellation
    void unSelect(void);

	//! Deselect all selected object if any
	//! Does deselect selected constellations
    void deselect(void);

	//! Set whether a pointer is to be drawn over selected object
	void setFlagSelectedObjectPointer(bool b) { object_pointer_visibility = b; }

	//! Get a multiline string describing the currently selected object
	wstring getSelectedObjectInfo(void) const {return selected_object.getInfoString(navigation);}

	//! Get a 1 line string briefly describing the currently selected object
	wstring getSelectedObjectShortInfo(void) const {return selected_object.getShortInfoString(navigation);}

	//! Get a color used to display info about the currently selected object
	Vec3f getSelectedObjectInfoColor(void) const;

	//! Get the earth centered equatorial position of the currently selected object
	Vec3f getSelectedObjectEarthEquPos(void) const {return selected_object.get_earth_equ_pos(navigation);}

	//! Get the size of the FoV best suited for seeing the selected object from close view
	double getSelectedObjectCloseFov(void) const {return selected_object.get_close_fov(navigation);}

	//! Get the size of the FoV best suited for seeing the selected object + its satellites if any
	double getSelectedObjectSatelliteFov(void) const {return selected_object.get_satellites_fov(navigation);}
	
	//! Get the size of the FoV best suited for seeing the selected object + its parent satellites
	double getSelectedObjectParentSatelliteFov(void) const {return selected_object.get_parent_satellites_fov(navigation);}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Telescope things: TODO move into the TelescopeMgr class
	//! the telescope with the given number shall go to the selected object
	void telescopeGoto(int nr);

	///////////////////////////////////////////////////////////////////////////////////////
	// Planets flags
	bool setHomePlanet(string planet);


	///////////////////////////////////////////////////////////////////////////////////////
	// Projection
	//! Set the projection type
	void setProjectionType(const string& ptype);
	//! Get the projection type
	string getProjectionType(void) const {return Projector::typeToString(projection->getType());}


	///////////////////////////////////////////////////////////////////////////////////////
	// Observer
	//! Return the current observatory (as a const object)
	const Observer* getObservatory(void) {return observatory;}

	//! Move to a new latitude and longitude on home planet
	void moveObserver(double lat, double lon, double alt, int delay, const wstring& name)
	{
		observatory->moveTo(lat, lon, alt, delay, name);
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Others
	//! Load color scheme from the given ini file and section name
	void setColorScheme(const string& skinFile, const string& section);

	//! Return the current image manager which display users images
	ImageMgr* getImageMgr(void) const {return script_images;}

private:
	//! Callback to record actions
	boost::callback<void, string> recordActionCallback;

	//! Select passed object
	//! @return true if the object was selected (false if the same was already selected)
	bool selectObject(const StelObject &obj);

	//! Find any kind of object by the name
	StelObject searchByNameI18n(const wstring &name) const;

	//! Find in a "clever" way an object from its equatorial position
	StelObject clever_find(const Vec3d& pos) const;
	
	//! Find in a "clever" way an object from its screen position
	StelObject clever_find(int x, int y) const;	
		
	// Main elements of the program
	Navigator * navigation;				// Manage all navigation parameters, coordinate transformations etc..
	Observer * observatory;			// Manage observer position
	Projector * projection;				// Manage the projection mode and matrix
	StelObject selected_object;			// The selected object in stellarium
	class MovementMgr* movementMgr;
	class HipStarMgr * hip_stars;		// Manage the hipparcos stars
	class ConstellationMgr * asterisms;		// Manage constellations (boundaries, names etc..)
	class NebulaMgr * nebulas;				// Manage the nebulas
	class SolarSystem* ssystem;				// Manage the solar system

	class MilkyWay* milky_way;				// Our galaxy
	MeteorMgr * meteors;				// Manage meteor showers
	ToneReproducer * tone_converter;	// Tones conversion between stellarium world and display device
	ImageMgr * script_images;           // for script loaded image display
	class TelescopeMgr *telescope_mgr;
	class LandscapeMgr* landscape;
	class GeodesicGridDrawer* geoDrawer;
	class GridLinesMgr* gridLines;
	bool object_pointer_visibility;		// Should selected object pointer be drawn
};

#endif // _STEL_CORE_H_
