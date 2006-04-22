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

#include "navigator.h"
#include "observator.h"
#include "projector.h"
#include "stel_object.h"
#include "hip_star_mgr.h"
#include "constellation_mgr.h"
#include "nebula_mgr.h"
#include "stel_atmosphere.h"
#include "tone_reproductor.h"
#include "solarsystem.h"
#include "stel_utility.h"
#include "init_parser.h"
#include "draw.h"
#include "landscape.h"
#include "meteor_mgr.h"
#include "sky_localizer.h"
#include "loadingbar.h"
#include "image_mgr.h"

//!  @brief Main class for stellarium core processing. 
//!  
//! Manage all the objects to be used in the program. 
//! This class is the main API of the program. It must be documented using doxygen.
class StelCore
{
// TODO : remove both
friend class StelCommandInterface;
public:
	//! Possible mount modes
	enum MOUNT_MODE { MOUNT_ALTAZIMUTAL, MOUNT_EQUATORIAL };

	// Inputs are the locale directory and root directory
    StelCore(const string& LDIR, const string& DATA_ROOT);
    virtual ~StelCore();
	
	//! Init and load all main core components from the passed config file.
	void init(const InitParser& conf);

	//! Update all the objects with respect to the time.
	//! @param delta_time the time increment in ms.
	void update(int delta_time);

	//! Execute all the drawing functions
	//! @param delta_time the time increment in ms.
	void draw(int delta_time);
	
	//! Get the name of the directory containing the data
	const string getDataDir(void) const {return dataRoot + "/data/";}
	
	//! Get the name of the root directory i.e the one containing the other main directories
	const string& getDataRoot() const {return dataRoot;}
	
	//! Set the sky culture from I18 name
	void setSkyCulture(const wstring& cultureName);
	
	//! Set the current sky culture from the passed directory
	void setSkyCultureDir(const string& culturedir);

	string getSkyCultureDir() {return skyCultureDir;}
		
	//! Get the current sky culture I18 name
	wstring getSkyCulture() const {return skyloc->directoryToSkyCultureI18(skyCultureDir);}
	
	//! Get the I18 available sky culture names
	wstring getSkyCultureListI18() const {return skyloc->getSkyCultureListI18();}
	
	//! Set the landscape
	void setLandscape(const string& new_landscape_name);

	//! @brief Set the sky language and reload the sky objects names with the new translation
	//! This function has no permanent effect on the global locale
	//!@param newSkyLocaleName The name of the locale (e.g fr) to use for sky object labels
	void setSkyLanguage(const string& newSkyLocaleName);
	
	//! Get the current sky language used for sky object labels
	//! @return The name of the locale (e.g fr)
	string getSkyLanguage() {return skyTranslator.getLocaleName();}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Navigation
	//! Set time speed in JDay/sec
	void setTimeSpeed(double ts) {navigation->set_time_speed(ts);}
	//! Get time speed in JDay/sec
	double getTimeSpeed(void) const {return navigation->get_time_speed();}
	
	//! Set the current date in Julian Day
	void setJDay(double JD) {navigation->set_JDay(JD);}
	//! Get the current date in Julian Day
	double getJDay(void) const {return navigation->get_JDay();}	
	
	//! Set object tracking
	void setFlagTraking(bool b) {navigation->set_flag_traking(b);}
	//! Get object tracking
	bool getFlagTraking(void) {return navigation->get_flag_traking();}
	
	//! Set whether sky position is to be locked
	void setFlagLockSkyPosition(bool b) {navigation->set_flag_lock_equ_pos(b);}
	//! Set whether sky position is locked
	bool getFlagLockSkyPosition(void) {return navigation->get_flag_lock_equ_pos();}
	
	//! Set current mount type
	void setMountMode(MOUNT_MODE m) {navigation->set_viewing_mode((m==MOUNT_ALTAZIMUTAL) ? Navigator::VIEW_HORIZON : Navigator::VIEW_EQUATOR);}
	//! Get current mount type
	MOUNT_MODE getMountMode(void) {return ((navigation->get_viewing_mode()==Navigator::VIEW_HORIZON) ? MOUNT_ALTAZIMUTAL : MOUNT_EQUATORIAL);}
	//! Toggle current mount mode between equatorial and altazimutal
	void toggleMountMode(void) {if (getMountMode()==MOUNT_ALTAZIMUTAL) setMountMode(MOUNT_EQUATORIAL); else setMountMode(MOUNT_ALTAZIMUTAL);}
	
	// TODO!
	void loadObservatory();
	
	//! Go to the selected object
	void gotoSelectedObject(void) {if (selected_object) navigation->move_to(selected_object->get_earth_equ_pos(navigation), auto_move_duration);}
	
	//! Set automove duration in seconds
	void setAutomoveDuration(float f) {auto_move_duration = f;}
	//! Get automove duration in seconds
	float getAutomoveDuration(void) {return auto_move_duration;}
	
	//! Zoom to the given FOV (in degree)
	void zoomTo(double aim_fov, float move_duration = 1.) {projection->zoom_to(aim_fov, move_duration);}
	
	//! Get current FOV (in degree)
	float getFov(void) const {return projection->get_fov();}
	
    //! If is currently zooming, return the target FOV, otherwise return current FOV
    double getAimFov(void) {return projection->getAimFov();}	
	
	//! Set the current FOV (in degree)
	void setFov(double f) {projection->set_fov(f);}
	
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
	
	//! Return whether an object is currently selected
	bool getFlagHasSelected(void) {return selected_object!=NULL;}
	
	//! Deselect selected object if any
	void unSelect(void) {selected_object=NULL; asterisms->setSelected(NULL); ssystem->setSelected(NULL);}
	
	//! Set whether a pointer is to be drawn over selected object
	void setFlagSelectedObjectPointer(bool b) { object_pointer_visibility = b; }
	
	//! Get a multiline string describing the currently selected object
	wstring getSelectedObjectInfo(void) const {if (selected_object==NULL) return L""; else return selected_object->getInfoString(navigation);}

	//! Get a 1 line string briefly describing the currently selected object
	wstring getSelectedObjectShortInfo(void) const {if (selected_object==NULL) return L""; else return selected_object->getShortInfoString(navigation);}

	//! Get a color used to display info about the currently selected object
	Vec3f getSelectedObjectInfoColor(void) const;
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Constellations methods
	//! Set display flag of constellation lines
	void setFlagConstellationLines(bool b) {asterisms->setFlagLines(b);}
	//! Get display flag of constellation lines
	bool getFlagConstellationLines(void) {return asterisms->getFlagLines();}
	
	//! Set display flag of constellation art
	void setFlagConstellationArt(bool b) {asterisms->setFlagArt(b);}
	//! Get display flag of constellation art
	bool getFlagConstellationArt(void) {return asterisms->getFlagArt();}
	
	//! Set display flag of constellation names
	void setFlagConstellationNames(bool b) {asterisms->setFlagNames(b);}
	//! Get display flag of constellation names
	bool getFlagConstellationNames(void) {return asterisms->getFlagNames();}

	//! Set display flag of constellation boundaries
	void setFlagConstellationBoundaries(bool b) {asterisms->setFlagBoundaries(b);}
	//! Get display flag of constellation boundaries
	bool getFlagConstellationBoundaries(void) {return asterisms->getFlagBoundaries();}
	
	//! Set constellation art intensity
	void setConstellationArtIntensity(float f) {asterisms->setArtIntensity(f);}
	//! Get constellation art intensity
	float getConstellationArtIntensity(void) const {return asterisms->getArtIntensity();}	
	
	//! Set constellation art intensity
	void setConstellationArtFadeDuration(float f) {asterisms->setArtFadeDuration(f);}
	//! Get constellation art intensity
	float getConstellationArtFadeDuration(void) const {return asterisms->getArtFadeDuration();}		
	
	//! Set whether selected constellation is drawn alone
	void setFlagConstellationIsolateSelected(bool b) {asterisms->setFlagIsolateSelected(b);}
	//! Get whether selected constellation is drawn alone 
	bool getFlagConstellationIsolateSelected(void) {return asterisms->getFlagIsolateSelected();}
	
	//! Set constellation font size 
	void setConstellationFontSize(float f) {asterisms->setFont(f, baseFontFile);}
	
	//! Get constellation line color
	Vec3f getColorConstellationLine() const {return asterisms->getLineColor();}
	//! Set constellation line color
	void setColorConstellationLine(const Vec3f& v) {asterisms->setLineColor(v);}
	
	//! Get constellation names color
	Vec3f getColorConstellationNames() const {return asterisms->getLabelColor();}
	//! Set constellation names color
	void setColorConstellationNames(const Vec3f& v) {asterisms->setLabelColor(v);}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Stars methods
	//! Set display flag for Stars 
	void setFlagStars(bool b) {hip_stars->setFlagStars(b);}
	//! Get display flag for Stars 
	bool getFlagStars(void) const {return hip_stars->getFlagStars();}
		
	//! Set display flag for Star names. Also make sure that stars are on if want labels 
	void setFlagStarName(bool b) {hip_stars->setFlagStarName(b);}
	//! Get display flag for Star names 
	bool getFlagStarName(void) const {return hip_stars->getFlagStarName();}
	
	//! Set display flag for Star Scientific names 
	void setFlagStarSciName(bool b) {hip_stars->setFlagStarSciName(b);}
	//! Get display flag for Star Scientific names 
	bool getFlagStarSciName(void) const {return hip_stars->getFlagStarSciName();}	
	
	//! Set flag for Star twinkling 
	void setFlagStarTwinkle(bool b) {hip_stars->setFlagStarTwinkle(b);}
	//! Get flag for Star twinkling 
	bool getFlagStarTwinkle(void) const {return hip_stars->getFlagStarTwinkle();}	
		
	//! Set flag for displaying Star as GLpoints (faster but not so nice) 
	void setFlagPointStar(bool b) {hip_stars->setFlagPointStar(b);}
	//! Get flag for displaying Star as GLpoints (faster but not so nice) 
	bool getFlagPointStar(void) const {return hip_stars->getFlagPointStar();}	
	
	//! Set maximum magnitude at which stars names are displayed 
	void setMaxMagStarName(float f) {hip_stars->setMaxMagStarName(f);}
	//! Get maximum magnitude at which stars names are displayed 
	float getMaxMagStarName(void) const {return hip_stars->getMaxMagStarName();}	
	
	//! Set maximum magnitude at which stars scientific names are displayed 
	void setMaxMagStarSciName(float f) {hip_stars->setMaxMagStarSciName(f);}
	//! Get maximum magnitude at which stars scientific names are displayed 
	float getMaxMagStarSciName(void) const {return hip_stars->getMaxMagStarSciName();}	
		
	//! Set base stars display scaling factor 
	void setStarScale(float f) {hip_stars->setStarScale(f);}
	//! Get base stars display scaling factor 
	float getStarScale(void) const {return hip_stars->getStarScale();}			
	
	//! Set stars display scaling factor wrt magnitude 
	void setStarMagScale(float f) {hip_stars->setStarMagScale(f);}
	//! Get base stars display scaling factor wrt magnitude 
	float getStarMagScale(void) const {return hip_stars->getStarMagScale();}

	//! Set stars twinkle amount 
	void setStarTwinkleAmount(float f) {hip_stars->setStarTwinkleAmount(f);}
	//! Get stars twinkle amount 
	float getStarTwinkleAmount(void) const {return hip_stars->getStarTwinkleAmount();}
	
	//! Set stars limiting display magnitude 
	void setStarLimitingMag(float f) {hip_stars->setStarLimitingMag(f);}
	//! Get stars limiting display magnitude 
	float getStarLimitingMag(void) const {return hip_stars->getStarLimitingMag();}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Planets flags
	//! Set flag for displaying Planets
	void setFlagPlanets(bool b) {ssystem->setFlagPlanets(b);}
	//! Get flag for displaying Planets
	bool getFlagPlanets(void) const {return ssystem->getFlagPlanets();}		
	
	//! Set flag for displaying Planets Trails
	void setFlagPlanetsTrails(bool b) {ssystem->setFlagTrails(b);}
	//! Get flag for displaying Planets Trails
	bool getFlagPlanetsTrails(void) const {return ssystem->getFlagTrails();}	
	
	//! Set flag for displaying Planets Hints
	void setFlagPlanetsHints(bool b) {ssystem->setFlagHints(b);}
	//! Get flag for displaying Planets Hints
	bool getFlagPlanetsHints(void) const {return ssystem->getFlagHints();}	
		
	//! Set flag for displaying Planets Orbits
	void setFlagPlanetsOrbits(bool b) {ssystem->setFlagOrbits(b);}
	//! Get flag for displaying Planets Orbits
	bool getFlagPlanetsOrbits(void) const {return ssystem->getFlagOrbits();}			
	
	//! Start/stop displaying planets Trails
	void startPlanetsTrails(bool b) {ssystem->startTrails(b);}

	//! Set base planets display scaling factor 
	void setPlanetsScale(float f) {ssystem->setScale(f);}
	//! Get base planets display scaling factor 
	float getPlanetsScale(void) const {return ssystem->getScale();}	
	
	//! Set selected planets by englishName
	//! @param englishName The planet name or "" to select no planet
	void setPlanetsSelected(const string& englishName) {ssystem->setSelected(englishName);}
	
	wstring getPlanetHashString(void);

	bool setHomePlanet(string planet);

	//! Set flag for displaying a scaled Moon
	void setFlagMoonScaled(bool b) {ssystem->setFlagMoonScale(b);}
	//! Get flag for displaying a scaled Moon
	bool getFlagMoonScaled(void) const {return ssystem->getFlagMoonScale();}
	
	//! Set Moon scale
	void setMoonScale(float f) { if (f<0) ssystem->setMoonScale(1.); else ssystem->setMoonScale(f);}
	//! Get Moon scale
	float getMoonScale(void) const {return ssystem->getMoonScale();}	
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Grid and lines
	//! Set flag for displaying Azimutal Grid
	void setFlagAzimutalGrid(bool b) {azi_grid->setFlagshow(b);}
	//! Get flag for displaying Azimutal Grid
	bool getFlagAzimutalGrid(void) const {return azi_grid->getFlagshow();}

	//! Set flag for displaying Equatorial Grid
	void setFlagEquatorGrid(bool b) {equ_grid->setFlagshow(b);}
	//! Get flag for displaying Equatorial Grid
	bool getFlagEquatorGrid(void) const {return equ_grid->getFlagshow();}
	
	//! Set flag for displaying Equatorial Line
	void setFlagEquatorLine(bool b) {equator_line->setFlagshow(b);}
	//! Get flag for displaying Equatorial Line
	bool getFlagEquatorLine(void) const {return equator_line->getFlagshow();}	
	
	//! Set flag for displaying Ecliptic Line
	void setFlagEclipticLine(bool b) {ecliptic_line->setFlagshow(b);}
	//! Get flag for displaying Ecliptic Line
	bool getFlagEclipticLine(void) const {return ecliptic_line->getFlagshow();}	
		
	//! Set flag for displaying Meridian Line
	void setFlagMeridianLine(bool b) {meridian_line->setFlagshow(b);}
	//! Get flag for displaying Meridian Line
	bool getFlagMeridianLine(void) const {return meridian_line->getFlagshow();}	
	
	//! Set flag for displaying Cardinals Points
	void setFlagCardinalsPoints(bool b) {cardinals_points->setFlagShow(b);}
	//! Get flag for displaying Cardinals Points
	bool getFlagCardinalsPoints(void) const {return cardinals_points->getFlagShow();}
	
	//! Set Cardinals Points color
	void setColorCardinalPoints(const Vec3f& v) {cardinals_points->set_color(v);}
	//! Get Cardinals Points color
	Vec3f getColorCardinalPoints(void) const {return cardinals_points->get_color();}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Projection	
	//! Set the horizontal viewport offset in pixels 
	void setViewportHorizontalOffset(int hoff) const {projection->setViewportPosX(hoff);}	
	//! Get the horizontal viewport offset in pixels 
	int getViewportHorizontalOffset(void) const {return projection->getViewportPosX();}
	
	//! Set the vertical viewport offset in pixels 
	void setViewportVerticalOffset(int voff) const {projection->setViewportPosY(voff);}
	//! Get the vertical viewport offset in pixels 
	int getViewportVerticalOffset(void) const {return projection->getViewportPosY();}
	
	//! Maximize viewport according to passed screen values
	void setMaximizedViewport(int screenW, int screenH) {projection->setViewport(0, 0, screenW, screenH);}
	
	//! Set a centered squared viewport with passed vertical and horizontal offset
	void setSquareViewport(int screenW, int screenH, int hoffset, int voffset)
	{
		int m = MY_MIN(screenW, screenH);
		projection->setViewport((screenW-m)/2+hoffset, (screenH-m)/2+voffset, m, m);
	}

	//! Set whether a disk mask must be drawn over the viewport
	void setViewportMaskDisk(void) {projection->setMaskType(Projector::DISK);}
	//! Get whether a disk mask must be drawn over the viewport
	bool getViewportMaskDisk(void) {return projection->getMaskType()==Projector::DISK;}
	
	//! Set whether no mask must be drawn over the viewport
	void setViewportMaskNone(void) {projection->setMaskType(Projector::NONE);}
	
	//! Set the projection type
	void setProjectionType(const string& ptype);
	//! Get the projection type
	string getProjectionType(void) {return Projector::typeToString(projection->getType());}
	
	//! Set flag for enabling gravity labels
	void setFlagGravityLabels(bool b) {projection->setFlagGravityLabels(b);}
	//! Get flag for enabling gravity labels
	bool getFlagGravityLabels(void) const {return projection->getFlagGravityLabels();}	
	
	//! Get viewport width
	int getViewportWidth(void) const {return projection->getViewportWidth();}
	
	//! Get viewport height
	int getViewportHeight(void) const {return projection->getViewportHeight();}
	
	//! Get viewport X position
	int getViewportPosX(void) const {return projection->getViewportPosX();}
	
	//! Get viewport Y position
	int getViewportPosY(void) const {return projection->getViewportPosY();}
	
	//! Set the viewport width and height
	void setViewportSize(int w, int h);
	
	//! Print the passed wstring so that it is oriented in the drection of the gravity
	void printGravity(s_font* font, float x, float y, const wstring& str, bool speedOptimize = 1, 
			float xshift = 0, float yshift = 0) const
		{projection->print_gravity180(font, x, y, str, speedOptimize, xshift, yshift);}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Landscape
	//! Set flag for displaying Landscape
	void setFlagLandscape(bool b) {landscape->setFlagShow(b);}
	//! Get flag for displaying Landscape
	bool getFlagLandscape(void) const {return landscape->getFlagShow();}	
		
	//! Set flag for displaying Fog
	void setFlagFog(bool b) {landscape->setFlagShowFog(b);}
	//! Get flag for displaying Fog
	bool getFlagFog(void) const {return landscape->getFlagShowFog();}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Atmosphere
	//! Set flag for displaying Atmosphere
	void setFlagAtmosphere(bool b) {atmosphere->setFlagShow(b);}
	//! Get flag for displaying Atmosphere
	bool getFlagAtmosphere(void) const {return atmosphere->getFlagShow();}	
	
	//! Set atmosphere fade duration in s
	void setAtmosphereFadeDuration(float f) {atmosphere->setFadeDuration(f);}
	//! Get atmosphere fade duration in s
	float getAtmosphereFadeDuration(void) const {return atmosphere->getFadeDuration();}	
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Milky Way
	//! Set flag for displaying Milky Way
	void setFlagMilkyWay(bool b) {milky_way->setFlagShow(b);}
	//! Get flag for displaying Milky Way
	bool getFlagMilkyWay(void) const {return milky_way->getFlagShow();}
	
	//! Set Milky Way intensity
	void setMilkyWayIntensity(float f) {milky_way->set_intensity(f);}
	//! Get Milky Way intensity
	float getMilkyWayIntensity(void) const {return milky_way->get_intensity();}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Nebulae
	//! Set flag for displaying Nebulae
	void setFlagNebula(bool b) {nebulas->setFlagShow(b);}
	//! Get flag for displaying Nebulae
	bool getFlagNebula(void) const {return nebulas->getFlagShow();}
	
	//! Set flag for displaying Nebulae Hints
	void setFlagNebulaHints(bool b) {nebulas->setFlagHints(b);}
	//! Get flag for displaying Nebulae Hints
	bool getFlagNebulaHints(void) const {return nebulas->getFlagHints();}	
	
	//! Set Nebulae Hints circle scale
	void setNebulaCircleScale(float f) {nebulas->setNebulaCircleScale(f);}
	//! Get Nebulae Hints circle scale
	float getNebulaCircleScale(void) const {return nebulas->getNebulaCircleScale();}
	
	//! Set flag for displaying Nebulae as bright
	void setFlagNebulaBright(bool b) {nebulas->setFlagBright(b);}
	//! Get flag for displaying Nebulae as brigth
	bool getFlagNebulaBright(void) const {return nebulas->getFlagBright();}	
	
	//! Set maximum magnitude at which nebulae hints are displayed
	void setNebulaMaxMagHints(float f) {nebulas->setMaxMagHints(f);}
	//! Get maximum magnitude at which nebulae hints are displayed
	float getNebulaMaxMagHints(void) const {return nebulas->getMaxMagHints();}
	
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Observator
	//! Return the current observatory (as a const object)
	Observator& getObservatory(void) {return *observatory;}
	
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

	//! Save all current settings to config
	void saveCurrentConfig(const string& confFile);

	
private:
	//! Find in a "clever" way an object from its equatorial position
	StelObject * clever_find(const Vec3d& pos) const;
	
	//! Find in a "clever" way an object from its screen position
	StelObject * clever_find(int x, int y) const;	
	
	string baseFontFile;				// The font file used by default during initialization

	string dataRoot;					// The root directory where the data is
	string localeDir;					// The directory containing the translation .mo file
	string skyCultureDir;				// The directory containing data for the culture used for constellations, etc.. 
	Translator skyTranslator;			// The translator used for astronomical object naming
		
	// Main elements of the program
	Navigator * navigation;				// Manage all navigation parameters, coordinate transformations etc..
	Observator * observatory;			// Manage observer position
	Projector * projection;				// Manage the projection mode and matrix
	StelObject * selected_object;		// The selected object in stellarium
	HipStarMgr * hip_stars;				// Manage the hipparcos stars
	ConstellationMgr * asterisms;		// Manage constellations (boundaries, names etc..)
	NebulaMgr * nebulas;				// Manage the nebulas
	SolarSystem* ssystem;				// Manage the solar system
	Atmosphere * atmosphere;			// Atmosphere
	SkyGrid * equ_grid;					// Equatorial grid
	SkyGrid * azi_grid;					// Azimutal grid
	SkyLine * equator_line;				// Celestial Equator line
	SkyLine * ecliptic_line;			// Ecliptic line
	SkyLine * meridian_line;			// Meridian line
	Cardinals * cardinals_points;		// Cardinals points
	MilkyWay * milky_way;				// Our galaxy
	MeteorMgr * meteors;				// Manage meteor showers
	Landscape * landscape;				// The landscape ie the fog, the ground and "decor"
	ToneReproductor * tone_converter;	// Tones conversion between stellarium world and display device
	SkyLocalizer *skyloc;				// for sky cultures and locales
	ImageMgr * script_images;           // for script loaded image display
		
	float sky_brightness;				// Current sky Brightness in ?
	bool object_pointer_visibility;		// Should selected object pointer be drawn
	void drawChartBackground(void);
	wstring get_cursor_pos(int x, int y);
		
	// Increment/decrement smoothly the vision field and position
	void updateMove(int delta_time);		
	int FlagEnableZoomKeys;
	int FlagEnableMoveKeys;
	
	double deltaFov,deltaAlt,deltaAz;	// View movement
	double move_speed, zoom_speed;		// Speed of movement and zooming
	
	float InitFov;						// Default viewing FOV
	Vec3d InitViewPos;					// Default viewing direction
	int FlagManualZoom;					// Define whether auto zoom can go further
	Vec3f chartColor;					// ?
	float auto_move_duration;			// Duration of movement for the auto move to a selected objectin seconds

	bool firstTime;                     // For init to track if reload or first time setup
};

#endif // _STEL_CORE_H_
