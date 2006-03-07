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
 
class StelUI;

class StelCore
{
// TODO : remove both
friend class StelUI;
friend class StelCommandInterface;
public:

	//! Possible drawing modes
	enum DRAWMODE { DM_NORMAL=0, DM_CHART, DM_NIGHT, DM_NIGHTCHART };

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
	
	//! Set the sky culture
	int setSkyCulture(string _culture_dir);
	
	//! Set the landscape
	void setLandscape(const string& new_landscape_name);

	//! @brief Set the sky language and reload the sky objects names with the new translation
	//! This function has no permanent effect on the global locale
	//!@param newSkyLocaleName The name of the locale (e.g fr) to use for sky object labels
	void setSkyLanguage(const std::string& newSkyLocaleName);

	void setObjectPointerVisibility(bool b) { object_pointer_visibility = b; }
	
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
	
	// TODO!
	void loadObservatory();
	
	//! Go to the selected object
	void gotoSelectedObject(void) {if (selected_object) navigation->move_to(selected_object->get_earth_equ_pos(navigation), auto_move_duration);}
	
	//! Zoom to the given FOV
	void zoomTo(double aim_fov, float move_duration = 1.) {projection->zoom_to(aim_fov, move_duration);}
	
	//! Get current FOV
	float getFov(void) const {return projection->get_fov();}
	
	//! Go and zoom temporarily to the selected object.
	void autoZoomIn(float move_duration = 1.f, bool allow_manual_zoom = 1);
	
	//! Unzoom to the previous position
	void autoZoomOut(float move_duration = 1.f, bool full = 0);
	
	// Viewing direction function : 1 move, 0 stop.
	void turn_right(int);
	void turn_left(int);
	void turn_up(int);
	void turn_down(int);
	void zoom_in(int);
	void zoom_out(int);	
	
	
	
	//! Find in a "clever" way an object from its equatorial position
	StelObject * clever_find(const Vec3d& pos) const;
	
	//! Find in a "clever" way an object from its screen position
	StelObject * clever_find(int x, int y) const;	
	
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
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Projection	
	//! Set the horizontal viewport offset in pixels 
	void setViewportHorizontalOffset(int hoff) const {projection->setViewportHorizontalOffset(hoff);}	
	//! Get the horizontal viewport offset in pixels 
	int getViewportHorizontalOffset(void) const {return projection->getViewportHorizontalOffset();}
	
	//! Set the vertical viewport offset in pixels 
	void setViewportVerticalOffset(int voff) const {projection->setViewportVerticalOffset(voff);}
	//! Get the vertical viewport offset in pixels 
	int getViewportVerticalOffset(void) const {return projection->getViewportVerticalOffset();}
	
	//! Set the viewport type
	void setViewportType(Projector::VIEWPORT_TYPE vType) {projection->setViewportType(vType);}
	//! Get the viewport type
	Projector::VIEWPORT_TYPE getViewportType(void) {return projection->getViewportType();}
	
	//! Set the projection type
	void setProjectionType(Projector::PROJECTOR_TYPE pType);
	//! Get the projection type
	Projector::PROJECTOR_TYPE getProjectionType(void) {return projection->getType();}
	
	//! Set flag for enabling gravity labels
	void setFlagGravityLabels(bool b) {projection->setFlagGravityLabels(b);}
	//! Get flag for enabling gravity labels
	bool getFlagGravityLabels(void) const {return projection->getFlagGravityLabels();}	
	
	//! Get viewport width
	int getViewportW(void) const {return projection->get_screenW();}
	
	//! Get viewport height
	int getViewportH(void) const {return projection->get_screenH();}
	
	//! Set the viewport width and height
	void setScreenSize(int w, int h);
	
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
	// Solar System
	//! Set flag for displaying a scaled Moon
	void setFlagMoonScaled(bool b) {ssystem->setFlagMoonScale(b);}
	//! Get flag for displaying a scaled Moon
	bool getFlagMoonScaled(void) const {return ssystem->getFlagMoonScale();}
	
	//! Set Moon scale
	void setMoonScale(float f) { if (f<0) ssystem->setMoonScale(1.); else ssystem->setMoonScale(f);}
	//! Get Moon scale
	float getMoonScale(void) const {return ssystem->getMoonScale();}
	
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Observator
	
	
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Others
	//! Load color scheme from the given ini file and section name
	void setColorScheme(const string& skinFile, const string& section);
	
	//! Set flag for activating night vision mode
	void setVisionModeNight(void) {if (!getVisionModeNight()) setColorScheme(getDataDir() + "default_config.ini", "colorr");draw_mode=DM_NIGHT;}
	//! Get flag for activating night vision mode
	bool getVisionModeNight(void) const {return draw_mode==DM_NIGHT;}
	
	//! Set flag for activating chart vision mode
	void setVisionModeChart(void){if (!getVisionModeChart()) setColorScheme(getDataDir() + "default_config.ini", "colorc");draw_mode=DM_CHART; }
	//! Get flag for activating chart vision mode
	bool getVisionModeChart(void) const {return draw_mode==DM_CHART;}
	
	//! Set flag for activating chart vision mode
	void setVisionModeNormal(void){if (!getVisionModeNormal()) setColorScheme(getDataDir() + "default_config.ini", "color");draw_mode=DM_NORMAL;}	
	//! Get flag for activating chart vision mode
	bool getVisionModeNormal(void) const {return draw_mode==DM_NORMAL;}

	//! Return the current image manager which display users images
	ImageMgr* getImageMgr(void) const {return script_images;}
	
private:
	string baseFontFile;				// The font file used by default during initialization

	string dataRoot;					// The root directory where the data is
	string localeDir;					// The directory containing the translation .mo file
	string skyCulture;					// the culture used for constellations, etc.. It is also the name of the directory
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
	void draw_chart_background(void);
	wstring get_cursor_pos(int x, int y);
		
	// Increment/decrement smoothly the vision field and position
	void updateMove(int delta_time);		
	int FlagEnableZoomKeys;
	int FlagEnableMoveKeys;
	
	double deltaFov,deltaAlt,deltaAz;	// View movement
	double move_speed, zoom_speed;		// Speed of movement and zooming
	
	float InitFov;						// Default viewing FOV
	Vec3d InitViewPos;					// Default viewing direction
	int FlagManualZoom;					// ?	
	Vec3f chartColor;					// ?
	float auto_move_duration;			// Duration of movement for the auto move to a selected object
	

	DRAWMODE draw_mode;					
};

#endif // _STEL_CORE_H_
