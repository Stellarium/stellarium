//
// C++ Interface: stelapp
//
// Description: 
//
//
// Author: Fabien Chéreau <stellarium@free.fr>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef STELAPP_H
#define STELAPP_H

#include "s_gui.h"
#include "stel_command_interface.h"
#include "stel_ui.h"
#include "script_mgr.h"


// Predeclaration of some classes
class StelCommandInterface;
class ScriptMgr;
class StelUI;
class ViewportDistorter;

/**
@author Fabien Chereau
*/
class StelApp{
friend class StelUI;
friend class StelCommandInterface;
public:
	//! Possible drawing modes
	enum DRAWMODE { DM_NORMAL=0, DM_CHART, DM_NIGHT, DM_NIGHTCHART, DM_NONE };

    StelApp(const string& CDIR, const string& LDIR, const string& DATA_ROOT);

    ~StelApp();

	//! Initialize application and core
	void init(void);

	//! Update all object according to the delta time
	void update(int delta_time);

	//! Draw all
	// Return the max squared distance in pixels that any object has
	// travelled since the last update.
	double draw(int delta_time);

	// Start the main loop until the end of the execution
	void startMainLoop(void) {start_main_loop();}

	// n.b. - do not confuse this with sky time rate
	int getTimeMultiplier() { return time_multiplier; };

	// Handle mouse clics
	int handleClick(int x, int y, s_gui::S_GUI_VALUE button, s_gui::S_GUI_VALUE state);
	// Handle mouse move
	int handleMove(int x, int y);
	// Handle key press and release
	int handleKeys(Uint16 key, s_gui::S_GUI_VALUE state);

	const string getConfigDir(void) const {return configDir;}
	
	const float getMaxFPS(void) const {return maxfps;}
	const float getMinFPS(void) const {return minfps;}

	//! Quit the application
	void quit(void);

	void playStartupScript();

	//! @brief Set the application language
	//! This applies to GUI, console messages etc..
	//! This function has no permanent effect on the global locale
	//! @param newAppLocaleName The name of the language (e.g fr) to use for GUI, TUI and console messages etc..
	void setAppLanguage(const std::string& newAppLangName);
	string getAppLanguage() { return Translator::globalTranslator.getLocaleName(); }

	//! Set flag for activating night vision mode
	void setVisionModeNight(void);
	//! Get flag for activating night vision mode
	bool getVisionModeNight(void) const {return draw_mode==DM_NIGHT;}
	
	//! Set flag for activating chart vision mode
	void setVisionModeChart(void);
	//! Get flag for activating chart vision mode
	bool getVisionModeChart(void) const {return draw_mode==DM_CHART;}
	
	//! Set flag for activating chart vision mode 
	// ["color" section name used for easier backward compatibility for older configs - Rob]
	void setVisionModeNormal(void);
	//! Get flag for activating chart vision mode
	bool getVisionModeNormal(void) const {return draw_mode==DM_NORMAL;}

	void setViewPortDistorterType(const string &type);
	string getViewPortDistorterType(void) const;

	//! Return full path to config file
	const string getConfigFile(void) const {return getConfigDir() + "config.ini";}

	// for use by TUI
	void saveCurrentConfig(const string& confFile);

	double getMouseCursorTimeout();
	
	//! Return a list of working fullscreen hardware video modes (one per line)
	string getVideoModeList(void) const;

	//! Required because stelcore doesn't have access to the script manager anymore!
	//! Record a command if script recording is on
	void recordCommand(string commandline);
	
private:
	//! Screen size
	int screenW, screenH;

    //! Initialize openGL screen with SDL
	void initSDL(int w, int h, int bbpMode, bool fullScreen, string iconFile);
	
	//! Run the main program loop
	void start_main_loop(void);

	//! Terminate the application with SDL
	void terminateApplication(void);

	//! Set the drawing mode in 2D for drawing in the full screen
	void set2DfullscreenProjection(void) const;
	//! Restore previous projection mode
	void restoreFrom2DfullscreenProjection(void) const;

	//! The assicated StelCore instance
	StelCore* core;

	//Files location
	string configDir;
	
	// Script related
	string SelectedScript;  // script filename (without directory) selected in a UI to run when exit UI
	string SelectedScriptDirectory;  // script directory for same

	// Navigation
	string PositionFile;


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
};

#endif
