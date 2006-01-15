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

#define SCRIPT_REMOVEABLE_DISK "/tmp/scripts/"

#include <string>

#include "navigator.h"
#include "observator.h"
#include "projector.h"
#include "fisheye_projector.h"
#include "cylinder_projector.h"
#include "stel_object.h"
#include "hip_star_mgr.h"
#include "constellation_mgr.h"
#include "nebula_mgr.h"
#include "stel_atmosphere.h"
#include "tone_reproductor.h"
#include "s_gui.h"
#include "stel_command_interface.h"
#include "stel_ui.h"
#include "solarsystem.h"
#include "stel_utility.h"
#include "init_parser.h"
#include "draw.h"
#include "landscape.h"
#include "meteor_mgr.h"
#include "sky_localizer.h"
#include "script_mgr.h"
#include "image_mgr.h"
#include "loadingbar.h"

// Predeclaration of the StelCommandInterface class
class StelCommandInterface;
class ScriptMgr;
class StelUI;


/**
 *  @brief Main class for stellarium core processing. 
 *  
 * Manage all the objects to be used in the program. 
 * This class will be the main API of the program after the currently planned 
 * reorganization of the source code. It must be documented using doxygen.
 */
class StelCore
{
friend class StelUI;
friend class StelCommandInterface;
public:
	// Inputs are the main data, textures, configuration directories relatively to the DATA_ROOT directory
	// TODO : StelCore should not handle any directory
    StelCore(const string& DDIR, const string& TDIR, const string& CDIR, const string& DATA_ROOT);
    virtual ~StelCore();
	
	//! @brief Init and load all main core components.
	void init(void);

	//! @brief Update all the objects with respect to the time.
	//! @param delta_time the time increment in ms.
	void update(int delta_time);

	//! @brief Execute all the drawing functions
	//! @param delta_time the time increment in ms.
	void draw(int delta_time);

	// TODO : only viewport size is managed by the projection class
	// Actual screen size should be managed by the stel_app class
	int get_screen_W(void) const {return screen_W;}
	int get_screen_H(void) const {return screen_H;}
		
	// find and select the "nearest" object and retrieve his informations
	StelObject * find_stel_object(int x, int y) const;
	StelObject * find_stel_object(const Vec3d& pos) const;

	// Find and select in a "clever" way an object
	StelObject * clever_find(const Vec3d& pos) const;
	StelObject * clever_find(int x, int y) const;
	
	// ---------------------------------------------------------------
	// Interfaces for external controls (gui, tui or script facility)
	// Only the function listed here should be called by them
	// ---------------------------------------------------------------
	
	//! Zoom to the given FOV
	void zoomTo(double aim_fov, float move_duration = 1.) {projection->zoom_to(aim_fov, move_duration);}
	
	//! Go and zoom temporary to the selected object.
	void autoZoomIn(float move_duration = 1.f, bool allow_manual_zoom = 1);
	
	//! Unzoom to the previous position
	void autoZoomOut(float move_duration = 1.f, bool full = 0);
	
	//! Set the sky culture
	int setSkyCulture(string _culture_dir);
	
	//! Set the screen size
	void setScreenSize(int w, int h);
	
	//! Set the landscape
	void setLandscape(const string& new_landscape_name);
	
	//! Set time speed in JDay/sec
	void setTimeSpeed(double ts) {navigation->set_time_speed(ts);}
	//! Get time speed in JDay/sec
	double getTimeSpeed(void) const {return navigation->get_time_speed();}
	
	//! Set the current date in Julian Day
	void setJDay(double JD) {navigation->set_JDay(JD);}
	//! Get the current date in Julian Day
	double getJDay(void) const {return navigation->get_JDay();}
		
	//! Set display flag of constellation lines
	void constellation_set_flag_lines(bool b) {asterisms->set_flag_lines(b);}
	//! Get display flag of constellation lines
	bool constellation_get_flag_lines(void) {return asterisms->get_flag_lines();}
	
	//! Get display flag of constellation boundaries
	bool constellation_get_flag_boundaries(void) {return asterisms->get_flag_boundaries();}

	//! Set display flag of constellation art
	void constellation_set_flag_art(bool b) {asterisms->set_flag_art(b);}
	//! Get display flag of constellation art
	bool constellation_get_flag_art(void) {return asterisms->get_flag_art();}
	
	//! Set display flag of constellation names
	void constellation_set_flag_names(bool b) {asterisms->set_flag_names(b);}
	//! Get display flag of constellation names
	bool constellation_get_flag_names(void) {return asterisms->get_flag_names();}



	/**
	 * @brief Set the application language
	 * This apply to GUI, console messages etc..
	 * This function has no permanent effect on the global locale
	 * @param newAppLocaleName The name of the locale (e.g fr_FR) to use for GUI, TUI and console messages etc..
	 */
	void StelCore::setAppLanguage(const std::string& newAppLocaleName);

	/** 
	 * @brief Set the sky language and reload the sky objects names with the new translation
	 * This function has no permanent effect on the global locale
	 * @param newSkyLocaleName The name of the locale (e.g fr_FR) to use for sky object labels
	 */
	void StelCore::setSkyLanguage(const std::string& newSkyLocaleName);

	///////////////////////////////////////////////////////////////////////////////
	// Below this limit, all the function will end up in the stel_app class
	///////////////////////////////////////////////////////////////////////////////
	
	void loadConfig(void);
	void saveConfig(void);
	// Set the config file names.
	void setConfigFiles(const string& _config_file);	
	
	// Increment/decrement smoothly the vision field and position
	void updateMove(int delta_time);

	// Handle mouse clics
	int handleClick(Uint16 x, Uint16 y, s_gui::S_GUI_VALUE button, s_gui::S_GUI_VALUE state);
	// Handle mouse move
	int handleMove(int x, int y);
	// Handle key press and release
	int handleKeys(Uint16 key, s_gui::S_GUI_VALUE state);

	int getBppMode(void) const {return bppMode;}
	int getFullscreen(void) const {return Fullscreen;}

	const string& getDataDir(void) const {return DataDir;}
	
	const float getMaxFPS(void) const {return maxfps;}

	int getMouseZoom(void) const {return MouseZoom;}

	// TODO move to stel_command_interface or get rid of this method
	void setParam(string& key, float value);

	// n.b. - do not confuse this with sky time rate
	int getTimeMultiplier() { return time_multiplier; };

	//! Quit the application
	void quit(void);

	void setObjectPointerVisibility(bool _newval) { object_pointer_visibility = _newval; }
	void playStartupScript();
	
private:
	TypeFace* UTFfont;

	wstring get_cursor_pos(int x, int y);

	static Translator skyTranslator;			// The translator used for astronomical object naming

	// Main elements of the program
	Navigator * navigation;				// Manage all navigation parameters, coordinate transformations etc..
	Observator * observatory;			// Manage observer position and locales for its country
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

	Planet* selected_planet;

	// Projector
	Projector::PROJECTOR_TYPE ProjectorType;
	Projector::VIEWPORT_TYPE ViewportType;
	int DistortionFunction;
	
		
	///////////////////////////////////////////////////////////////////////////////
	// Below this limit, all the attributes will end up in the stel_app class
	///////////////////////////////////////////////////////////////////////////////
	
	void loadConfigFrom(const string& confFile);
	void saveConfigTo(const string& confFile);

	// Big options
	int screen_W;
	int screen_H;
	int bppMode;
	int Fullscreen;
	int verticalOffset;
	int horizontalOffset;

    //Files location
	string TextureDir;
	string ConfigDir;
	string DataDir;
	string DataRoot;

	string config_file;

	// Main elements of the stel_app
	StelCommandInterface * commander;       // interface to perform all UI and scripting actions
	ScriptMgr * scripts;                    // manage playing and recording scripts
	StelUI * ui;							// The main User Interface
	ImageMgr * script_images;               // for script loaded image display
	SkyLocalizer *skyloc;					// for sky cultures and locales
	
	// localization
	string SkyCulture;  // the culture used for constellations

	// Script related
	string SelectedScript;  // script filename (without directory) selected in a UI to run when exit UI
	string SelectedScriptDirectory;  // script directory for same


	///////////////////////////////////////////////////////////////////////////////
	// Below this limit, all the attributes will be removed from the program and 
	// handled by the sub classes themselve
	///////////////////////////////////////////////////////////////////////////////
		
	// Stars
	int FlagStarName;
	int FlagStarSciName;
	float MaxMagStarName;
	float MaxMagStarSciName;
	float NebulaScale;
	float StarScale;
	float StarMagScale;
	float StarTwinkleAmount;
	int FlagStarTwinkle;
	int FlagPointStar;

	// Gui
	int FlagShowTopBar;
    int FlagShowFps;
	int FlagShowTime;
	int FlagShowDate;
	int FlagShowAppName;
	int FlagShowFov;
    int FlagMenu;
    int FlagHelp;
    int FlagInfos;
    int FlagConfig;
    int FlagSearch;
	int FlagShowSelectedObjectInfo;
	Vec3f GuiBaseColor;
	Vec3f GuiTextColor;
	Vec3f GuiBaseColorr;
	Vec3f GuiTextColorr;
	float BaseFontSize;
	string BaseFontName;

	float BaseCFontSize;
	string BaseCFontName;
	
	float MapFontSize;
	float ConstellationFontSize;
	float StarFontSize;
	float PlanetFontSize;
	float NebulaFontSize;
	float GridFontSize;
	float CardinalsFontSize;
	float LoadingBarFontSize;

	Vec3f AzimuthalColor[3];
	Vec3f EquatorialColor[3];
	Vec3f EquatorColor[3];
	Vec3f EclipticColor[3];

	Vec3f ConstLinesColor[3];
	Vec3f ConstBoundaryColor[3];
	Vec3f ConstNamesColor[3];
	Vec3f NebulaLabelColor[3];
	Vec3f NebulaCircleColor[3];
	Vec3f StarLabelColor[3];
	Vec3f StarCircleColor[3];
	Vec3f CardinalColor[3];
	Vec3f PlanetNamesColor[3];
	Vec3f PlanetOrbitsColor[3];
	Vec3f ObjectTrailsColor[3];
	Vec3f ChartColor[3];
	Vec3f MilkyWayColor[3];

	double MouseCursorTimeout;  // seconds to hide cursor when not used.  0 means no timeout

	// Text UI
	bool FlagEnableTuiMenu;
	bool FlagShowGravityUi;
	bool FlagShowTuiMenu;
	bool FlagShowTuiDateTime;
	bool FlagShowTuiShortObjInfo;

	// Astro
    bool FlagStars;
    bool FlagPlanets;
    bool FlagPlanetsHints;
    bool FlagPlanetsOrbits;
    bool FlagObjectTrails;
    bool FlagNebula;
    bool FlagNebulaLongName;
    float MaxMagNebulaName;
    bool FlagNebulaCircle;
    bool FlagMilkyWay;
    float MilkyWayIntensity;
    bool FlagBrightNebulae;
	
	// Landscape
	bool FlagLandscape;
    bool FlagFog;
    bool FlagAtmosphere;
	float sky_brightness;
	float AtmosphereFadeDuration;

	// Viewing
    bool FlagAzimutalGrid;
    bool FlagEquatorialGrid;
    bool FlagEquatorLine;
    bool FlagEclipticLine;
    bool FlagMeridianLine;
	bool FlagGravityLabels;
	float MoonScale;
	bool FlagMoonScaled;
	float ConstellationArtIntensity;
	float ConstellationArtFadeDuration;
	//bool FlagUseCommonNames;
	bool FlagChart;
	bool FlagNight;
	bool ColorSchemeChanged;
	inline void SetDrawMode(void) { draw_mode = (int)FlagChart;	if (FlagChart) draw_mode += (int)FlagNight; ColorSchemeChanged = true;}
	void ChangeColorScheme(void);
	void draw_chart_background(void);

	// Navigation
	string PositionFile;
	int FlagEnableZoomKeys;
	int FlagManualZoom;
	int FlagShowScriptBar;
	int FlagEnableMoveKeys;
	int FlagEnableMoveMouse;  // allow mouse at edge of screen to move view
	float InitFov;
	double PresetSkyTime;
	string StartupTimeMode;
	Vec3d InitViewPos;
	int MouseZoom;

	// Locale
	int FlagUTC_Time;					// if true display UTC time

	int frame, timefr, timeBase;		// Used for fps counter
	float fps;
	float maxfps;
	
	double deltaFov,deltaAlt,deltaAz;	// View movement
	double move_speed, zoom_speed;		// Speed of movement and zooming
	float auto_move_duration;			// Duration of movement for the auto move to a selected object

	int FlagTimePause;
	double temp_time_velocity;			// Used to store time speed while in pause

	// Viewing direction function : 1 move, 0 stop.
	void turn_right(int);
	void turn_left(int);
	void turn_up(int);
	void turn_down(int);
	void zoom_in(int);
	void zoom_out(int);
	// Flags for mouse movements
	bool is_mouse_moving_horiz;
	bool is_mouse_moving_vert;
	int time_multiplier;  // used for adjusting delta_time for script speeds

	bool object_pointer_visibility;  // should selected object pointer be drawn
};

#endif // _STEL_CORE_H_
