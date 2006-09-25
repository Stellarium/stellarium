/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef STELAPP_H
#define STELAPP_H

#include "stellarium.h"
#include "s_gui.h"

// Predeclaration of some classes
class StelCore;
class StelCommandInterface;
class ScriptMgr;
class StelUI;
class ViewportDistorter;
class StelFontMgr;

//! Singleton main Stellarium application class.
//! @author Fabien Chereau
class StelApp
{
	friend class StelUI;
	friend class StelCommandInterface;
public:
	//! @brief Create and initialize the main Stellarium application.
	//! @param configDir the full path to the directory where config.ini is stored.
	//! @param localeDir the full path to the directory containing locale specific infos
	//! e.g. /usr/local/share/locale/. This directory should typically contain fr/LC_MESSAGES/stellarium.mo
	//! so that french translations work.
	//! @param dataRootDir the root data directory.
	StelApp(const string& configDir, const string& localeDir, const string& dataRootDir);

	//! Deinitialize and destroy the main Stellarium application.
	~StelApp();

	//! @brief Get the StelApp singleton instance
	//! @return the StelApp singleton instance
	static StelApp& getInstance(void) {assert(singleton); return *singleton;}

	//! @brief Start the main loop and return when the program ends.
	void startMainLoop(void);

	//! @brief Get the configuration file path.
	//! @return the full path to Stellarium's main config.ini file
	string getConfigFilePath(void) const;

	//! @brief Get the full path to a data file.
	//! This method will try to find the file in all valid data directories until it finds it.
	//! @param dataFileName the data file path relative to the main data directory (e.g font/myFont.ttf)
	//! @return the fullpath to the data file e.g /usr/local/share/stellarium/data/font/myFont.ttf
	string getDataFilePath(const string& dataFileName) const;
	
	//! @brief Get the locale data directory path
	//! @return the full path to the directory containing locale specific infos e.g. /usr/local/share/locale/.
	//! This directory should e.g. contain fr/LC_MESSAGES/stellarium.mo so that french translations work.
	const string& getLocaleDir() {return localeDir;}

	//! @brief Get the full path to a texture file.
	//! This method will try to find the file in all valid data directories until it finds it.
	//! @param textureFileName the texture file path relative to the main data directory (e.g jupiter.png)
	//! @return the fullpath to the texture file e.g /usr/local/share/stellarium/data/texture/jupiter.png.
	string getTextureFilePath(const string& textureFileName) const;

	//! @brief Set the application language. This applies to GUI etc..
	//! This function has no permanent effect on the global locale.
	//! @param newAppLocaleName the name of the language (e.g fr).
	void setAppLanguage(const string& newAppLangName);

	//! @brief Get the application language currently used for GUI etc..
	//! This function has no permanent effect on the global locale.
	//! @return the name of the language (e.g fr).
	string getAppLanguage() { return Translator::globalTranslator.getTrueLocaleName(); }

	//! @brief Get the font manager to use for loading fonts.
	//! @return the font manager to use for loading fonts.
	const StelFontMgr& getFontManager() { return *fontManager;}
	
	//! Set flag for activating night vision mode
	void setVisionModeNight(void);
	//! Get flag for activating night vision mode
	bool getVisionModeNight(void) const {return draw_mode==DM_NIGHT;}

	//! Set flag for activating chart vision mode
	// ["color" section name used for easier backward compatibility for older configs - Rob]
	void setVisionModeNormal(void);
	//! Get flag for activating chart vision mode
	bool getVisionModeNormal(void) const {return draw_mode==DM_NORMAL;}

	void setViewPortDistorterType(const string &type);
	string getViewPortDistorterType(void) const;

	//! Return a list of working fullscreen hardware video modes (one per line)
	string getVideoModeList(void) const;

	//! Required because stelcore doesn't have access to the script manager anymore!
	//! Record a command if script recording is on
	void recordCommand(string commandline);

	string get_time_format_str(void) const {return s_time_format_to_string(time_format);}
	void set_time_format_str(const string& tf) {time_format=string_to_s_time_format(tf);}
	string get_date_format_str(void) const {return s_date_format_to_string(date_format);}
	void set_date_format_str(const string& df) {date_format=string_to_s_date_format(df);}

	void setCustomTimezone(string _time_zone) { set_custom_tz_name(_time_zone); }

	//! Possible drawing modes
	enum DRAWMODE { DM_NORMAL=0, DM_CHART, DM_NIGHT, DM_NIGHTCHART, DM_NONE };

	enum S_TIME_FORMAT {S_TIME_24H,	S_TIME_12H,	S_TIME_SYSTEM_DEFAULT};
	enum S_DATE_FORMAT {S_DATE_MMDDYYYY, S_DATE_DDMMYYYY, S_DATE_SYSTEM_DEFAULT, S_DATE_YYYYMMDD};

	wstring get_printable_date_UTC(double JD) const;
	wstring get_printable_date_local(double JD) const;
	wstring get_printable_time_UTC(double JD) const;
	wstring get_printable_time_local(double JD) const;

	enum S_TZ_FORMAT
	{
	    S_TZ_CUSTOM,
	    S_TZ_GMT_SHIFT,
	    S_TZ_SYSTEM_DEFAULT
	};

	//! Return the current time shift at observator time zone with respect to GMT time
	void set_GMT_shift(int t) {GMT_shift=t;}
	float get_GMT_shift(double JD = 0, bool _local=0) const;
	void set_custom_tz_name(const string& tzname);
	string get_custom_tz_name(void) const {return custom_tz_name;}
	S_TZ_FORMAT get_tz_format(void) const {return time_zone_mode;}

	// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
	string get_ISO8601_time_local(double JD) const;

private:
	//! Initialize application and core
	void init(void);

	//! Update all object according to the delta time
	void update(int delta_time);

	//! Draw all
	// Return the max squared distance in pixels that any object has
	// travelled since the last update.
	double draw(int delta_time);

	//! Quit the application
	void quit(void);

	// Handle mouse clics
	int handleClick(int x, int y, s_gui::S_GUI_VALUE button, s_gui::S_GUI_VALUE state);
	// Handle mouse move
	int handleMove(int x, int y);
	// Handle key press and release
	int handleKeys(SDLKey key, SDLMod mod,
	               Uint16 unicode, s_gui::S_GUI_VALUE state);

	// n.b. - do not confuse this with sky time rate
	int getTimeMultiplier() { return time_multiplier; }
	;

	// Initialize openGL screen with SDL
	void initSDL(int w, int h, int bbpMode, bool fullScreen, string iconFile);

	//! Terminate the application with SDL
	void terminateApplication(void);

	//! Set the drawing mode in 2D for drawing in the full screen
	void set2DfullscreenProjection(void) const;
	//! Restore previous projection mode
	void restoreFrom2DfullscreenProjection(void) const;

	// for use by TUI
	void saveCurrentConfig(const string& confFile);

	// The StelApp singleton
	static StelApp* singleton;

	// Screen size
	int screenW, screenH;

	// The assicated StelCore instance
	StelCore* core;

	// Full path to config dir
	string configDir;
	// Full path to locale dir
	string localeDir;
	// Full path to data dir
	string dataDir;
	// Full path to root dir
	string rootDir;

	// Script related
	string SelectedScript;  // script filename (without directory) selected in a UI to run when exit UI
	string SelectedScriptDirectory;  // script directory for same

	// Navigation
	string PositionFile;

	// Font manager for the application
	StelFontMgr* fontManager;
	
	int FlagEnableMoveMouse;  // allow mouse at edge of screen to move view

	double PresetSkyTime;
	string StartupTimeMode;

	int MouseZoom;

	int frame, timefr, timeBase;		// Used for fps counter
	float fps;
	float minfps, maxfps;

	int FlagTimePause;
	double temp_time_velocity;			// Used to store time speed while in pause

	// Flags for mouse movements
	bool is_mouse_moving_horiz;
	bool is_mouse_moving_vert;
	int time_multiplier;  // used for adjusting delta_time for script speeds

	// Main elements of the stel_app
	StelCommandInterface * commander;       // interface to perform all UI and scripting actions
	ScriptMgr * scripts;                    // manage playing and recording scripts
	StelUI * ui;							// The main User Interface
	ViewportDistorter *distorter;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// SDL related function and variables
	static SDL_Cursor *create_cursor(const char *image[]);

	// SDL managment variables
	SDL_Surface *Screen;// The Screen
	SDL_Event	E;		// And Event Used In The Polling Process
	Uint32	TickCount;	// Used For The Tick Counter
	Uint32	LastCount;	// Used For The Tick Counter
	SDL_Cursor *Cursor;

	DRAWMODE draw_mode;					// Current draw mode
	bool initialized;  // has the init method been called yet?

	// Date and time variables
	S_TIME_FORMAT time_format;
	S_DATE_FORMAT date_format;
	S_TZ_FORMAT time_zone_mode;		// Can be the system default or a user defined value

	string custom_tz_name;			// Something like "Europe/Paris"
	float GMT_shift;				// Time shift between GMT time and local time in hour. (positive for Est of GMT)

	// Convert the time format enum to its associated string and reverse
	S_TIME_FORMAT string_to_s_time_format(const string&) const;
	string s_time_format_to_string(S_TIME_FORMAT) const;

	// Convert the date format enum to its associated string and reverse
	S_DATE_FORMAT string_to_s_date_format(const string& df) const;
	string s_date_format_to_string(S_DATE_FORMAT df) const;
};

#endif
