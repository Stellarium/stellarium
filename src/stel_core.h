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
#include "setting_mgr.h"

// Predeclaration of the StelCommandInterface class
class StelCommandInterface;
class ScriptMgr;



class stel_core
{
friend class stel_ui;
friend class StelCommandInterface;
public:
    stel_core();
    virtual ~stel_core();
	
	void init(void);
	void load_config(void);
	void save_config(void);

	// Set the main data, textures and configuration directories
	void set_directories(const string& DDIR, const string& TDIR, const string& CDIR, const string& DATA_ROOT);

	// Set the 2 config files names.
	void set_config_files(const string& _config_file);

	void update(int delta_time);		// Update all the objects in function of the time
	void draw(int delta_time);			// Execute all the drawing functions

	// Increment/decrement smoothly the vision field and position
	void update_move(int delta_time);

	// Handle mouse clics
	int handle_clic(Uint16 x, Uint16 y, Uint8 state, Uint8 button);
	// Handle mouse move
	int handle_move(int x, int y);
	// Handle key press and release
	int handle_keys(SDLKey key, s_gui::S_GUI_VALUE state);

	int get_screen_W(void) const {return screen_W;}
	int get_screen_H(void) const {return screen_H;}
	int get_bppMode(void) const {return bppMode;}
	int get_Fullscreen(void) const {return Fullscreen;}

	const string& get_DataDir(void) const {return DataDir;}
	
	const float getMaxFPS(void) const {return maxfps;}

	// find and select the "nearest" object and retrieve his informations
	stel_object * find_stel_object(int x, int y) const;
	stel_object * find_stel_object(const Vec3d& pos) const;

	// Find and select in a "clever" way an object
	stel_object * clever_find(const Vec3d& pos) const;
	stel_object * clever_find(int x, int y) const;

	int get_mouse_zoom(void) const {return MouseZoom;}


	// ---------------------------------------------------------------
	// Interfaces for external controls (gui, tui or script facility)
	// Only the function listed here should be called by them
	// ---------------------------------------------------------------
	
	// IDEA: replace flags and settings with a hash of structs or classes...
	int set_flag(string name, string value, bool &nweval, bool trusted);  // set a core flag
	// Set the value of the parameter TODO
	void set_param(string& key, float value);
	// Zoom to the given FOV
	void zoom_to(double aim_fov, float move_duration = 1.) {projection->zoom_to(aim_fov, move_duration);}
	// Go and zoom temporary to the selected object.
	void auto_zoom_in(float move_duration = 1.f);
	// Unzoom to the previous position
	void auto_zoom_out(float move_duration = 1.f);
	// Set the sky culture
	int set_sky_culture(string _culture_dir);
	// Set the locale type
	void set_sky_locale(string _locale);	
	// Set the screen size
	void set_screen_size(int w, int h);
	// Set the landscape
	void set_landscape(const string& new_landscape_name);
	// Set/Get time speed in JDay/sec
	void set_time_speed(double ts) {navigation->set_time_speed(ts);}
	double get_time_speed(void) const {return navigation->get_time_speed();}
	// Set/Get JDay
	void set_JDay(double JD) {navigation->set_JDay(JD);}
	double get_JDay(void) const {return navigation->get_JDay();}
	// Quit the application
	void quit(void);

	// Activate/deactivate constellation art
	void constellation_show_art(bool b) {asterisms->show_art(b);}
	
	//	void populate_setting_mgr(SettingMgr *smgr);  // TEMP until migrate to new config setup
	//      void load_config_from_setting_mgr(SettingMgr *smgr);  // TEMP until migrate to new config setup
	
private:

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

	// Main elements of the program
	StelCommandInterface * commander;       // interface to perform all UI and scripting actions
	ScriptMgr * scripts;                    // manage playing and recording scripts
	navigator * navigation;			// Manage all navigation parameters, coordinate transformations etc..
	Observator * observatory;		// Manage observer position and locales for its country
	Projector * projection;			// Manage the projection mode and matrix
	stel_object * selected_object;		// The selected object in stellarium
	Hip_Star_mgr * hip_stars;		// Manage the hipparcos stars
	Constellation_mgr * asterisms;		// Manage constellations (boundaries, names etc..)
	Nebula_mgr * nebulas;			// Manage the nebulas
	SolarSystem* ssystem;			// Manage the solar system
	stel_atmosphere * atmosphere;		// Atmosphere
	SkyGrid * equ_grid;			// Equatorial grid
	SkyGrid * azi_grid;			// Azimutal grid
	SkyLine * equator_line;			// Celestial Equator line
	SkyLine * ecliptic_line;		// Eclptic line
	Cardinals * cardinals_points;		// Cardinals points
	MilkyWay * milky_way;			// Our galaxy
	Meteor_mgr * meteors;
	Landscape * landscape;			// The landscape ie the fog, the ground and "decor"
	tone_reproductor * tone_converter;	// Tones conversion between stellarium world and display device
	stel_ui * ui;				// The main User Interface
	ImageMgr * script_images;               // for script loaded image display

	planet* selected_planet;

	// Projector
	PROJECTOR_TYPE ProjectorType;
	VIEWPORT_TYPE ViewportType;
	int DistortionFunction;

	// localization
	string SkyCulture;  // the culture used for constellations
	string SkyLocale;   // the locale (usually will just be language code) used for object labels
	// these are separate so that, for example, someone could view polynesian constellations in Hawaiian 
	// OR english, french, etc.
	

	// Script related
	string SelectedScript;  // script filename (without directory) selected in a UI to run when exit UI
	string SelectedScriptDirectory;  // script directory for same
	bool ScriptRemoveableDiskMounted;  // is disk for scripts mounted?

	// Stars
	int FlagStarName;
	int FlagStarSciName;
	float MaxMagStarName;
	float MaxMagStarSciName;
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
	int FlagShowSelectedObjectInfo;
	Vec3f GuiBaseColor;
	Vec3f GuiTextColor;
	float BaseFontSize;

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

	// Text UI
	int FlagEnableTuiMenu;
	int FlagShowGravityUi;
	int FlagShowTuiMenu;
	int FlagShowTuiDateTime;
	int FlagShowTuiShortObjInfo;

	// Astro
    int FlagStars;
    int FlagPlanets;
    int FlagPlanetsHints;
    int FlagPlanetsOrbits;
    int FlagObjectTrails;
    int FlagNebula;
    int FlagNebulaName;
    float MaxMagNebulaName;
    int FlagNebulaCircle;
    int FlagMilkyWay;
    int FlagBrightNebulae;
	
	// Landscape
    int FlagGround;
    int FlagHorizon;
    int FlagFog;
    int FlagAtmosphere;
	float sky_brightness;
	float AtmosphereFadeDuration;

	// Viewing
    int FlagConstellationDrawing;
    int FlagConstellationName;
	int FlagConstellationPick;
	int FlagConstellationArt;
    int FlagAzimutalGrid;
    int FlagEquatorialGrid;
    int FlagEquatorLine;
    int FlagEclipticLine;
    int FlagCardinalPoints;
	int FlagGravityLabels;
	float MoonScale;
	int FlagInitMoonScaled;
	float ConstellationArtIntensity;
	float ConstellationArtFadeDuration;

	// Navigation
	string PositionFile;
	int FlagEnableZoomKeys;
	int FlagManualZoom;
	int FlagEnableMoveKeys;
	int FlagEnableMoveMouse;  // allow mouse at edge of screen to move view
	float InitFov;
	double PresetSkyTime;
	string StartupTimeMode;
	Vec3d InitViewPos;
	VIEWING_MODE_TYPE ViewingMode;
	int MouseZoom;

	// Locale
	int FlagUTC_Time;					// if true display UTC time

	int frame, timefr, timeBase;		// Used for fps counter
	float fps;
	float maxfps;
	
	double deltaFov,deltaAlt,deltaAz;	// View movement
	double move_speed;					// Speed of movement
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
	
	Sky_localizer *skyloc;  // for sky cultures and locales
};

#endif // _STEL_CORE_H_
