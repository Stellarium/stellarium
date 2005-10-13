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

// Main class for stellarium
// Manage all the objects to be used in the program

#ifndef _STEL_CORE_H_
#define _STEL_CORE_H_


#define SCRIPT_REMOVEABLE_DISK "/tmp/scripts/"

#include <string>

#include "navigator.h"
#include "observator.h"
#include "projector.h"
#include "fisheye_projector.h"
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
class stel_ui;

class stel_core
{
friend class stel_ui;
friend class StelCommandInterface;
public:
	// Inputs are the main data, textures, configuration directories relatively to the DATA_ROOT directory
	// TODO : stel_core should not handle any directory
    stel_core(const string& DDIR, const string& TDIR, const string& CDIR, const string& DATA_ROOT);
    virtual ~stel_core();
	
	// Init and load all main core components
	void init(void);
	// Update all the objects in function of the time
	void update(int delta_time);
	// Execute all the drawing functions
	void draw(int delta_time);

	// TODO : only viewport size is managed by the projection class
	// Actual screen size should be managed by the stel_app class
	int get_screen_W(void) const {return screen_W;}
	int get_screen_H(void) const {return screen_H;}
		
	// find and select the "nearest" object and retrieve his informations
	stel_object * find_stel_object(int x, int y) const;
	stel_object * find_stel_object(const Vec3d& pos) const;

	// Find and select in a "clever" way an object
	stel_object * clever_find(const Vec3d& pos) const;
	stel_object * clever_find(int x, int y) const;
	
	// ---------------------------------------------------------------
	// Interfaces for external controls (gui, tui or script facility)
	// Only the function listed here should be called by them
	// ---------------------------------------------------------------
	
	//! Zoom to the given FOV
	void zoom_to(double aim_fov, float move_duration = 1.) {projection->zoom_to(aim_fov, move_duration);}
	
	//! Go and zoom temporary to the selected object.
	void auto_zoom_in(float move_duration = 1.f, bool allow_manual_zoom = 1);
	
	//! Unzoom to the previous position
	void auto_zoom_out(float move_duration = 1.f, bool full = 0);
	
	//! Set the sky culture
	int set_sky_culture(string _culture_dir);
	
	//! Set the locale type
	void set_system_locale_by_code(const string& _locale); // ie fr_FR
	void set_system_locale_by_name(const string& _locale); // ie fra

	//! Set the screen size
	void set_screen_size(int w, int h);
	
	//! Set the landscape
	void set_landscape(const string& new_landscape_name);
	
	//! Set/Get time speed in JDay/sec
	void set_time_speed(double ts) {navigation->set_time_speed(ts);}
	double get_time_speed(void) const {return navigation->get_time_speed();}
	
	//! Set/Get JDay
	void set_JDay(double JD) {navigation->set_JDay(JD);}
	double get_JDay(void) const {return navigation->get_JDay();}
		
	//! Set/Get display flag of constellation lines
	void constellation_set_flag_lines(bool b) {asterisms->set_flag_lines(b);}
	bool constellation_get_flag_lines(void) {return asterisms->get_flag_lines();}
	
	//! Set/Get display flag of constellation boundaries
	bool constellation_get_flag_boundaries(void) {return asterisms->get_flag_boundaries();}

	//! Set/Get display flag of constellation art
	void constellation_set_flag_art(bool b) {asterisms->set_flag_art(b);}
	bool constellation_get_flag_art(void) {return asterisms->get_flag_art();}
	
	//! Set/Get display flag of constellation names
	void constellation_set_flag_names(bool b) {asterisms->set_flag_names(b);}
	bool constellation_get_flag_names(void) {return asterisms->get_flag_names();}
		
	///////////////////////////////////////////////////////////////////////////////
	// Below this limit, all the function will end up in the stel_app class
	///////////////////////////////////////////////////////////////////////////////
	
	void load_config(void);
	void save_config(void);
	// Set the config file names.
	void set_config_files(const string& _config_file);	
	
	// Increment/decrement smoothly the vision field and position
	void update_move(int delta_time);

	// Handle mouse clics
	int handle_clic(Uint16 x, Uint16 y, s_gui::S_GUI_VALUE button, s_gui::S_GUI_VALUE state);
	// Handle mouse move
	int handle_move(int x, int y);
	// Handle key press and release
	int handle_keys(Uint16 key, s_gui::S_GUI_VALUE state);

	int get_bppMode(void) const {return bppMode;}
	int get_Fullscreen(void) const {return Fullscreen;}

	const string& get_DataDir(void) const {return DataDir;}
	
	const float getMaxFPS(void) const {return maxfps;}

	int get_mouse_zoom(void) const {return MouseZoom;}

	// TODO move to stel_command_interface or get rid of this method
	void set_param(string& key, float value);

	// n.b. - do not confuse this with sky time rate
	int get_time_multiplier() { return time_multiplier; };

	//! Quit the application
	void quit(void);

	void set_object_pointer_visibility(bool _newval) { object_pointer_visibility = _newval; }
	void play_startup_script();

private:
	void set_system_locale(void);
	void set_sky_locale(void);
	string get_cursor_pos(int x, int y);


	// Main elements of the program
	navigator * navigation;				// Manage all navigation parameters, coordinate transformations etc..
	Observator * observatory;			// Manage observer position and locales for its country
	Projector * projection;				// Manage the projection mode and matrix
	stel_object * selected_object;		// The selected object in stellarium
	Hip_Star_mgr * hip_stars;			// Manage the hipparcos stars
	Constellation_mgr * asterisms;		// Manage constellations (boundaries, names etc..)
	Nebula_mgr * nebulas;				// Manage the nebulas
	SolarSystem* ssystem;				// Manage the solar system
	stel_atmosphere * atmosphere;		// Atmosphere
	SkyGrid * equ_grid;					// Equatorial grid
	SkyGrid * azi_grid;					// Azimutal grid
	SkyLine * equator_line;				// Celestial Equator line
	SkyLine * ecliptic_line;			// Ecliptic line
	SkyLine * meridian_line;			// Meridian line
	Cardinals * cardinals_points;		// Cardinals points
	MilkyWay * milky_way;				// Our galaxy
	Meteor_mgr * meteors;				// Manage meteor showers
	Landscape * landscape;				// The landscape ie the fog, the ground and "decor"
	tone_reproductor * tone_converter;	// Tones conversion between stellarium world and display device

	planet* selected_planet;

	// Projector
	PROJECTOR_TYPE ProjectorType;
	VIEWPORT_TYPE ViewportType;
	int DistortionFunction;
	
		
	///////////////////////////////////////////////////////////////////////////////
	// Below this limit, all the attributes will end up in the stel_app class
	///////////////////////////////////////////////////////////////////////////////
	
	void load_config_from(const string& confFile);
	void save_config_to(const string& confFile);

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
	stel_ui * ui;							// The main User Interface
	ImageMgr * script_images;               // for script loaded image display
	Sky_localizer *skyloc;					// for sky cultures and locales
	
	// localization
	string SkyCulture;  // the culture used for constellations
	string SkyLocale;   // the locale (usually will just be language code) used for object labels
	// these are separate so that, for example, someone could view polynesian constellations in Hawaiian 
	// OR english, french, etc.
	

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

	Vec3f AzimuthalColor;
	Vec3f EquatorialColor;
	Vec3f EquatorColor;
	Vec3f EclipticColor;

	Vec3f ConstLinesColor;
	Vec3f ConstNamesColor;
	Vec3f NebulaLabelColor;
	Vec3f NebulaCircleColor;
	Vec3f CardinalColor;
	Vec3f PlanetNamesColor;
	Vec3f PlanetOrbitsColor;
	Vec3f ObjectTrailsColor;

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
	bool FlagNight;

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
	string UILocale;                    // locale UI is currently using
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
