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

/**
@author Fabien Chereau
*/
class StelApp{
friend class StelUI;
friend class StelCommandInterface;
public:
    StelApp(const string& CDIR, const string& LDIR, const string& DATA_ROOT);

    ~StelApp();

	//! Initialize application and core
	void init(void);

	//! Update all object according to the delta time
	void update(int delta_time);

	//! Draw all
	void draw(int delta_time);

	// Start the main loop until the end of the execution
	void startMainLoop(void) {start_main_loop();}

	// n.b. - do not confuse this with sky time rate
	int getTimeMultiplier() { return time_multiplier; };

	// Handle mouse clics
	int handleClick(Uint16 x, Uint16 y, s_gui::S_GUI_VALUE button, s_gui::S_GUI_VALUE state);
	// Handle mouse move
	int handleMove(int x, int y);
	// Handle key press and release
	int handleKeys(Uint16 key, s_gui::S_GUI_VALUE state);

	const string getConfigDir(void) const {return configDir;}
	
	const float getMaxFPS(void) const {return maxfps;}

	int getMouseZoom(void) const {return MouseZoom;}

	//! Quit the application
	void quit(void);

	void playStartupScript();

	//! @brief Set the application language
	//! This applies to GUI, console messages etc..
	//! This function has no permanent effect on the global locale
	//! @param newAppLocaleName The name of the language (e.g fr) to use for GUI, TUI and console messages etc..
	void setAppLanguage(const std::string& newAppLangName);

private:
    //! Initialize openGL screen with SDL
	void initSDL(int w, int h, int bbpMode, bool fullScreen, string iconFile);
	
	//! Run the main program loop
	void start_main_loop(void);

	//! Terminate the application with SDL
	void terminateApplication(void);

	StelCore* core;

	//Files location
	string configDir;
	
	void loadConfigFrom(const string& confFile);

	string config_file;
	
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
	float maxfps;

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
		
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// SDL related function and variables
	static SDL_Cursor *create_cursor(const char *image[]);

	// SDL managment variables
	SDL_Surface *Screen;// The Screen
    SDL_Event	E;		// And Event Used In The Polling Process
    Uint32	TickCount;	// Used For The Tick Counter
    Uint32	LastCount;	// Used For The Tick Counter
    SDL_Cursor *Cursor;
};

#endif
