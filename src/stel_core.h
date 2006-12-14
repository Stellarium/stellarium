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

#include "stel_object.h"
#include "draw.h"
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
	void init(const InitParser& conf);

	//! Update all the objects with respect to the time.
	//! @param delta_time the time increment in ms.
	void update(int delta_time);

	//! Execute all the drawing functions
	//! @param delta_time the time increment in ms.
	//! @returns the max squared distance in pixels any single object has moved since the previous update.
	double draw(int delta_time);
	
	//! Update the sky culture for all the modules
	void updateSkyCulture();
	
	//! Set the sky locale and update the sky objects names for all the modules
	void updateSkyLanguage();

	//! Get the current projector used in the core
	Projector* getProjection() {return projection;}
	
	//! Get the current navigation (manages frame transformation) used in the core
	Navigator* getNavigation() {return navigation;}

	//! Get the current tone converter used in the core
	ToneReproductor* getToneReproductor() {return tone_converter;}


	///////////////////////////////////////////////////////////////////////////////////////
	// Navigation
	//! Set stellarium time to current real world time
	void setTimeNow();
	//! Get wether the current stellarium time is the real world time
	bool getIsTimeNow(void) const;

	//! Set object tracking
	void setFlagTracking(bool b);
	//! Get object tracking
	bool getFlagTracking(void) {return navigation->getFlagTraking();}

	//! Set whether sky position is to be locked
	void setFlagLockSkyPosition(bool b) {navigation->setFlagLockEquPos(b);}
	//! Set whether sky position is locked
	bool getFlagLockSkyPosition(void) {return navigation->getFlagLockEquPos();}

	//! Set current mount type
	void setMountMode(MOUNT_MODE m) {navigation->setViewingMode((m==MOUNT_ALTAZIMUTAL) ? Navigator::VIEW_HORIZON : Navigator::VIEW_EQUATOR);}
	//! Get current mount type
	MOUNT_MODE getMountMode(void) {return ((navigation->getViewingMode()==Navigator::VIEW_HORIZON) ? MOUNT_ALTAZIMUTAL : MOUNT_EQUATORIAL);}
	//! Toggle current mount mode between equatorial and altazimutal
	void toggleMountMode(void) {if (getMountMode()==MOUNT_ALTAZIMUTAL) setMountMode(MOUNT_EQUATORIAL); else setMountMode(MOUNT_ALTAZIMUTAL);}


	//! Go to the selected object
    void gotoSelectedObject(void) {
      if (selected_object)
        navigation->moveTo(
          selected_object.get_earth_equ_pos(navigation),
          auto_move_duration);
    }

	//! Move view in alt/az (or equatorial if in that mode) coordinates
	void panView(double delta_az, double delta_alt)	{
		setFlagTracking(0);
		navigation->updateMove(delta_az, delta_alt);
	}

	//! Set automove duration in seconds
	void setAutomoveDuration(float f) {auto_move_duration = f;}
	//! Get automove duration in seconds
	float getAutomoveDuration(void) const {return auto_move_duration;}

	//! Zoom to the given FOV (in degree)
	void zoomTo(double aim_fov, float move_duration = 1.) {projection->zoom_to(aim_fov, move_duration);}

	//! Get current FOV (in degree)
	float getFov(void) const {return projection->get_fov();}

    //! If is currently zooming, return the target FOV, otherwise return current FOV
	double getAimFov(void) const {return projection->getAimFov();}

	//! Set the current FOV (in degree)
	void setFov(double f) {projection->set_fov(f);}

	//! Set the maximum FOV (in degree)
	void setMaxFov(double f) {projection->setMaxFov(f);}

	//! Go and zoom temporarily to the selected object.
	void autoZoomIn(float move_duration = 1.f, bool allow_manual_zoom = 1);

	//! Unzoom to the previous position
	void autoZoomOut(float move_duration = 1.f, bool full = 0);

	//! Set whether auto zoom can go further than normal
	void setFlagManualAutoZoom(bool b) {FlagManualZoom = b;}
	//! Get whether auto zoom can go further than normal
	bool getFlagManualAutoZoom(void) {return FlagManualZoom;}

	// Viewing direction function : 1 move, 0 stop.
	void turn_right(int);
	void turn_left(int);
	void turn_up(int);
	void turn_down(int);
	void zoom_in(int);
	void zoom_out(int);

	//! Make the first screen position correspond to the second (useful for mouse dragging)
	void dragView(int x1, int y1, int x2, int y2);

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

	//! Set display flag of telescopes
	void setFlagTelescopes(bool b);
	//! Get display flag of telescopes
	bool getFlagTelescopes(void) const;

	//! Set display flag of telescope names
	void setFlagTelescopeName(bool b);
	//! Get display flag of telescope names
	bool getFlagTelescopeName(void) const;

	//! the telescope with the given number shall go to the selected object
	void telescopeGoto(int nr);

	///////////////////////////////////////////////////////////////////////////////////////
	// Planets flags
	bool setHomePlanet(string planet);

	///////////////////////////////////////////////////////////////////////////////////////
	// Grid and lines
	//! Set flag for displaying Azimutal Grid
	void setFlagAzimutalGrid(bool b) {azi_grid->setFlagshow(b);}
	//! Get flag for displaying Azimutal Grid
	bool getFlagAzimutalGrid(void) const {return azi_grid->getFlagshow();}
	Vec3f getColorAzimutalGrid(void) const {return azi_grid->getColor();}

	//! Set flag for displaying Equatorial Grid
	void setFlagEquatorGrid(bool b) {equ_grid->setFlagshow(b);}
	//! Get flag for displaying Equatorial Grid
	bool getFlagEquatorGrid(void) const {return equ_grid->getFlagshow();}
	Vec3f getColorEquatorGrid(void) const {return equ_grid->getColor();}

	//! Set flag for displaying Equatorial Line
	void setFlagEquatorLine(bool b) {equator_line->setFlagshow(b);}
	//! Get flag for displaying Equatorial Line
	bool getFlagEquatorLine(void) const {return equator_line->getFlagshow();}
	Vec3f getColorEquatorLine(void) const {return equator_line->getColor();}

	//! Set flag for displaying Ecliptic Line
	void setFlagEclipticLine(bool b) {ecliptic_line->setFlagshow(b);}
	//! Get flag for displaying Ecliptic Line
	bool getFlagEclipticLine(void) const {return ecliptic_line->getFlagshow();}
	Vec3f getColorEclipticLine(void) const {return ecliptic_line->getColor();}


	//! Set flag for displaying Meridian Line
	void setFlagMeridianLine(bool b) {meridian_line->setFlagshow(b);}
	//! Get flag for displaying Meridian Line
	bool getFlagMeridianLine(void) const {return meridian_line->getFlagshow();}
	Vec3f getColorMeridianLine(void) const {return meridian_line->getColor();}

	void setColorAzimutalGrid(const Vec3f& v) { azi_grid->setColor(v);}
	void setColorEquatorGrid(const Vec3f& v) { equ_grid->setColor(v);}
	void setColorEquatorLine(const Vec3f& v) { equator_line->setColor(v);}
	void setColorEclipticLine(const Vec3f& v) { ecliptic_line->setColor(v);}
	void setColorMeridianLine(const Vec3f& v) { meridian_line->setColor(v);}

	///////////////////////////////////////////////////////////////////////////////////////
	// Projection
	//! Set the projection type
	void setProjectionType(const string& ptype);
	//! Get the projection type
	string getProjectionType(void) const {return Projector::typeToString(projection->getType());}


	///////////////////////////////////////////////////////////////////////////////////////
	// Observator
	//! Return the current observatory (as a const object)
	const Observator* getObservatory(void) {return observatory;}

	//! Move to a new latitude and longitude on home planet
	void moveObserver(double lat, double lon, double alt, int delay, const wstring& name)
	{
		observatory->moveTo(lat, lon, alt, delay, name);
	}

	//! Set Meteor Rate in number per hour
	void setMeteorsRate(int f) {meteors->set_ZHR(f);}
	//! Get Meteor Rate in number per hour
	int getMeteorsRate(void) const {return meteors->get_ZHR();}

	///////////////////////////////////////////////////////////////////////////////////////
	// Others
	//! Load color scheme from the given ini file and section name
	void setColorScheme(const string& skinFile, const string& section);

	//! Return the current image manager which display users images
	ImageMgr* getImageMgr(void) const {return script_images;}

	double getZoomSpeed() {return zoom_speed;}
	float getAutoMoveDuration() {return auto_move_duration;}
	
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
	Observator * observatory;			// Manage observer position
	Projector * projection;				// Manage the projection mode and matrix
	StelObject selected_object;			// The selected object in stellarium
	class HipStarMgr * hip_stars;		// Manage the hipparcos stars
	class ConstellationMgr * asterisms;		// Manage constellations (boundaries, names etc..)
	class NebulaMgr * nebulas;				// Manage the nebulas
	class SolarSystem* ssystem;				// Manage the solar system
	SkyGrid * equ_grid;					// Equatorial grid
	SkyGrid * azi_grid;					// Azimutal grid
	SkyLine * equator_line;				// Celestial Equator line
	SkyLine * ecliptic_line;			// Ecliptic line
	SkyLine * meridian_line;			// Meridian line

	MilkyWay * milky_way;				// Our galaxy
	MeteorMgr * meteors;				// Manage meteor showers
	ToneReproductor * tone_converter;	// Tones conversion between stellarium world and display device
	ImageMgr * script_images;           // for script loaded image display
	class TelescopeMgr *telescope_mgr;
	class LandscapeMgr* landscape;
	class GeodesicGridDrawer* geoDrawer;

	bool object_pointer_visibility;		// Should selected object pointer be drawn

	// Increment/decrement smoothly the vision field and position
	void updateMove(int delta_time);
	int FlagEnableZoomKeys;
	int FlagEnableMoveKeys;

	double deltaFov,deltaAlt,deltaAz;	// View movement
	double move_speed, zoom_speed;		// Speed of movement and zooming

	float InitFov;						// Default viewing FOV
	int FlagManualZoom;					// Define whether auto zoom can go further
	float auto_move_duration;			// Duration of movement for the auto move to a selected objectin seconds

};

#endif // _STEL_CORE_H_
