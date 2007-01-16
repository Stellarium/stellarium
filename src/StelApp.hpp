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

#include <string>
#include <cassert>
#include "SDL.h"	// SDL is used only for the key codes types
#include "stellarium.h"

#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelSkyCultureMgr.hpp"

// Predeclaration of some classes
class StelCore;
class StelCommandInterface;
class ScriptMgr;
class StelUI;
class ViewportDistorter;
class SkyLocalizer;
class StelTextureMgr;
class StelObjectMgr;

using namespace std;

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
	string getConfigFilePath() const;

	//! @brief Get the (writable) .stellarium/ directory path, usually $HOME/.stellarium/
	//! @return the full path to the (writable) .stellarium/ directory path
	const std::string& getDotStellariumDir() const {return dotStellariumDir;}

	//! @brief Get the full path to a file.
	//! This method will try to find the file in all valid data directories until it finds it.
	//! @return the fullpath to the file.
	string getFilePath(const string& fileName) const;

	//! @brief Get the full path to a data file.
	//! This method will try to find the file in all valid data directories until it finds it.
	//! @param dataFileName the data file path relative to the main data directory (e.g image/myImage.png)
	//! @return the fullpath to the data file e.g /usr/local/share/stellarium/data/image/myImage.png
	string getDataFilePath(const string& dataFileName) const;

	//! @brief Get the full path to a texture file.
	//! This method will try to find the file in all valid data directories until it finds it.
	//! @param textureFileName the texture file path relative to the main data directory (e.g jupiter.png)
	//! @return the fullpath to the texture file e.g /usr/local/share/stellarium/data/texture/jupiter.png.
	string getTextureFilePath(const string& textureFileName) const;
	
	//! @brief Get the locale data directory path
	//! @return the full path to the directory containing locale specific infos e.g. /usr/local/share/locale/.
	//! This directory should e.g. contain fr/LC_MESSAGES/stellarium.mo so that french translations work.
	const string& getLocaleDir() {return localeDir;}

	//! @brief Get the module manager to use for accessing any module loaded in the application
	//! @return the module manager.
	StelModuleMgr& getModuleMgr() {return *moduleMgr;}
	
	//! @brief Get the locale manager to use for i18n & date/time localization
	//! @return the font manager to use for loading fonts.
	StelLocaleMgr& getLocaleMgr() {return *localeMgr;}
	
	//! @brief Get the font manager to use for loading fonts.
	//! @return the font manager to use for loading fonts.
	StelFontMgr& getFontManager() {return *fontManager;}
	
	//! @brief Get the sky cultures manager
	//! @return the sky cultures manager
	StelSkyCultureMgr& getSkyCultureMgr() {return *skyCultureMgr;}
	
	//! @brief Get the texture manager to use for loading textures.
	//! @return the texture manager to use for loading textures.
	StelTextureMgr& getTextureManager() {return *textureMgr;}
	
	//! @brief Get the StelObject manager to use for querying from all stellarium objects
	//! @return the StelObject manager to use for querying from all stellarium objects
	StelObjectMgr& getStelObjectMgr() {return *stelObjectMgr;}
	
	//! @brief Get the core of the program. It is the one which provide the projection, navigation and tone converter.
	//! @return the StelCore instance of the program
	StelCore* getCore() {return core;}
	
	//! Update translations and font everywhere in the program
	void updateAppLanguage();
	
	//! Update translations and font for sky everywhere in the program
	void updateSkyLanguage();	
	
	//! Update and reload sky culture informations everywhere in the program
	void updateSkyCulture();	
	
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

	//! Return the time since when stellarium is running in second
	double getTotalRunTime() const {return totalRunTime;}
	
	//! Required because stelcore doesn't have access to the script manager anymore!
	//! Record a command if script recording is on
	void recordCommand(string commandline);

	// n.b. - do not confuse this with sky time rate
	int getTimeMultiplier() { return time_multiplier; }
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
	int handleClick(int x, int y, Uint8 button, Uint8 state);
	// Handle mouse move
	int handleMove(int x, int y);
	// Handle key press and release
	int handleKeys(SDLKey key, SDLMod mod, Uint16 unicode, Uint8 state);

	// Set the colorscheme for all the modules
	void setColorScheme(const std::string& fileName, const std::string& section);

	// Initialize openGL screen with SDL
	void initSDL(int w, int h, int bbpMode, bool fullScreen, string iconFile);

	// Save a screen shot in the HOME directory
	void saveScreenShot() const;

	//! Terminate the application with SDL
	void terminateApplication(void);

	//! Set the drawing mode in 2D for drawing in the full screen
	void set2DfullscreenProjection(void) const;
	//! Restore previous projection mode
	void restoreFrom2DfullscreenProjection(void) const;

	// The StelApp singleton
	static StelApp* singleton;

	// Screen size
	int screenW, screenH;

	// The assicated StelCore instance
	StelCore* core;

	// Full path to config dir
	string dotStellariumDir;
	// Full path to locale dir
	string localeDir;
	// Full path to data dir
	string dataDir;
	// Full path to root dir
	string rootDir;

	// Module manager for the application
	StelModuleMgr* moduleMgr;
	
	// Font manager for the application
	StelFontMgr* fontManager;
	
	// Locale manager for the application
	StelLocaleMgr* localeMgr;
	
	// Sky cultures manager for the application
	StelSkyCultureMgr* skyCultureMgr;
	
	// Textures manager for the application
	StelTextureMgr* textureMgr;
	
	// Manager for all the StelObjects of the program
	StelObjectMgr* stelObjectMgr;

	int frame, timefr, timeBase;		// Used for fps counter
	float fps;
	float minfps, maxfps;
	
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

	double totalRunTime;	// Total stellarium run time in second

	int time_multiplier;  // used for adjusting delta_time for script speeds
	
	//! Possible drawing modes
	enum DRAWMODE { DM_NORMAL=0, DM_NIGHT};
	DRAWMODE draw_mode;					// Current draw mode
};

#endif
