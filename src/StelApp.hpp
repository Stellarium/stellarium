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
#include <vector>
#include "StelKey.hpp"
#include "StelTypes.hpp"

using namespace std;

// Predeclaration of some classes
class StelCore;
class StelCommandInterface;
class ScriptMgr;
class StelUI;
class ViewportDistorter;
class SkyLocalizer;
class StelTextureMgr;
class StelObjectMgr;
class StelFontMgr;
class StelLocaleMgr;
class StelModuleMgr;
class StelSkyCultureMgr;
class StelFileMgr;

using namespace std;

//! @brief Singleton main Stellarium application class.
//!
//! This is the central class of Stellarium.
//! Only one singleton instance of this class is created and can be accessed from anywhere else.
//! This class is the access point to several "Manager" class which provide application-wide services for managment of font,
//! textures, localization, sky culture, and in theory all other services used by the other part of the program.
//! The StelApp class is also the one managing the StelModule in a generic maner by calling their update,
//! drawing and other methods when needed.
//! @author Fabien Chereau
class StelApp
{
	friend class StelUI;
	friend class StelCommandInterface;
public:
	//! @brief Create and initialize the main Stellarium application.
	//! @param argc The number of command line paremeters
	//! @param argv an array of char* command line arguments
	//! The configFile will be search for in the search path by the StelFileMgr,
	//! it is therefor possible to specify either just a file name or path within the
	//! search path, or use a full path or even a relative path to an existing file
	StelApp(int argc, char** argv);

	//! Deinitialize and destroy the main Stellarium application.
	virtual ~StelApp();

	//! Return the full name of stellarium, i.e. "Stellarium 0.9.0"
	static string getApplicationName();

	//! Initialize application and core
	virtual void init();

	//! @brief Get the StelApp singleton instance
	//! @return the StelApp singleton instance
	static StelApp& getInstance() {assert(singleton); return *singleton;}

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
	
	//! @brief Get the StelFileMgr for performing file operations
	//! @return the StelFileMgr manager to use for performing file operations
	StelFileMgr& getFileMgr() {return *stelFileMgr;}
	
	//! @brief Get the core of the program. It is the one which provide the projection, navigation and tone converter.
	//! @return the StelCore instance of the program
	StelCore* getCore() {return core;}
	
	//! @brief Get the old-style home-made GUI. This method will be suppressed when the StelUI is made as a real StelModule
	//! @return a pointer on the old-style home-made GUI.
	StelUI* getStelUI() {return ui;}
	
	//! Update translations and font everywhere in the program
	void updateAppLanguage();
	
	//! Update translations and font for sky everywhere in the program
	void updateSkyLanguage();	
	
	//! Update and reload sky culture informations everywhere in the program
	void updateSkyCulture();	
	
	//! Set flag for activating night vision mode
	void setVisionModeNight();
	//! Get flag for activating night vision mode
	bool getVisionModeNight() const {return draw_mode==DM_NIGHT;}

	//! Set flag for activating chart vision mode
	// ["color" section name used for easier backward compatibility for older configs - Rob]
	void setVisionModeNormal();
	//! Get flag for activating chart vision mode
	bool getVisionModeNormal() const {return draw_mode==DM_NORMAL;}

	void setViewPortDistorterType(const string &type);
	string getViewPortDistorterType() const;
	
	//! Required because stelcore doesn't have access to the script manager anymore!
	//! Record a command if script recording is on
	void recordCommand(string commandline);

	//! Set the time multiplier used when fast forwarding scripts
	//! n.b. - do not confuse this with sky time rate
	//! @param multiplier new value for the time multiplier
	void setTimeMultiplier(const int multiplier) { time_multiplier = multiplier; }

	//! Get the time multiplier used when fast forwarding scripts
	//! n.b. - do not confuse this with sky time rate
	//! @return the integer time multiplier
	const int getTimeMultiplier() { return time_multiplier; }
	
	//! Set the drawing mode in 2D for drawing in the full screen
	void set2DfullscreenProjection() const;
	//! Restore previous projection mode
	void restoreFrom2DfullscreenProjection() const;
	
	//! Sets the name of the configuration file
	//! It is possible to set the configuration by passing either a full path
	//! a relative path of an existing file, or path segment which will be appended
	//! to the serach path.  The configuration file must be writable, or there will
	//! be trouble!
	//! @param configName the name or full path of the configuration file
	void setConfigFile(const string& configName);
	
	//! Retrieve the full path of the current configuration file
	//! @return the full path of the configuration file
	const string& getConfigFilePath() { return configFile; }
		
	//! Copies the default configuration file
	void copyDefaultConfigFile();
	
	//! Get the width of the openGL screen
	//! @return width of the openGL screen in pixels
	virtual int getScreenW() const = 0;
	
	//! Get the height of the openGL screen
	//! @return height of the openGL screen in pixels
	virtual int getScreenH() const = 0;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods overidded for SDL / QT
	///////////////////////////////////////////////////////////////////////////	
	
	//! Start the main loop and return when the program ends.
	virtual void startMainLoop() = 0;
	
	//! Return a list of working fullscreen hardware video modes (one per line)
	virtual string getVideoModeList() const = 0;
	
	//! Return the time since when stellarium is running in second
	virtual double getTotalRunTime() const = 0;
	
	//! Set mouse cursor display
	virtual void showCursor(bool b) = 0;
	
	//! De-init SDL / QT related stuff
	virtual void deInit() = 0;
	
	//! Swap GL buffer, should be called only for special condition
	virtual void swapGLBuffers() = 0;
	
	//! Terminate the application
	virtual void terminateApplication() = 0;
	
	//! Alternate fullscreen mode/windowed mode if possible
	virtual void toggleFullScreen() = 0;
	
	//! Return whether we are in fullscreen mode
	virtual bool getFullScreen() const = 0;
	
protected:
	//! Update all object according to the delta time
	void update(int delta_time);

	//! Draw all registered StelModule in the order defined by the order lists.
	//! @return the max squared distance in pixels that any object has travelled since the last update.
	double draw(int delta_time);
	
	//! Handle mouse clics
	int handleClick(int x, int y, Uint8 button, Uint8 state, StelMod mod);
	//! Handle mouse move
	int handleMove(int x, int y, StelMod mod);
	//! Handle key press and release
	int handleKeys(StelKey key, StelMod mod, Uint16 unicode, Uint8 state);

	//! The minimum desired frame rate in frame per second
	float minfps;
	//! The maximum desired frame rate in frame per second
	float maxfps;

	///////////////////////////////////////////////////////////////////////////
	// Methods overidded for SDL / QT
	///////////////////////////////////////////////////////////////////////////	
	
	//! Initialize openGL screen
	virtual void initOpenGL(int w, int h, int bbpMode, bool fullScreen, string iconFile) =0;
	
	//! Save a screen shot to StelFileMgr::getScreenshotDir()
	virtual void saveScreenShot() const =0;
	
	//! Call this when the size of the GL window has changed
	void glWindowHasBeenResized(int w, int h);

	//! Call this when you want to make the window (not) resizable
	virtual void setResizable(bool resizable) = 0;
	
private:
	// C++-ized version of argc & argv
	vector<string> argList;
	
	// Set the colorscheme for all the modules
	void setColorScheme(const std::string& fileName, const std::string& section);

	// The StelApp singleton
	static StelApp* singleton;

	// The associated StelCore instance
	StelCore* core;

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

	// Utility class for file operations, mainly locating files by name
	StelFileMgr* stelFileMgr;

	float fps;
	int frame, timefr, timeBase;		// Used for fps counter
	
	// Main elements of the stel_app
	StelCommandInterface * commander;       // interface to perform all UI and scripting actions
	ScriptMgr * scripts;                    // manage playing and recording scripts
	StelUI * ui;							// The main User Interface
	ViewportDistorter *distorter;

	int time_multiplier;	// used for adjusting delta_time for script speeds
	
	//! Possible drawing modes
	enum DRAWMODE { DM_NORMAL=0, DM_NIGHT};
	DRAWMODE draw_mode;					// Current draw mode
	
	string configFile;

};

#endif
