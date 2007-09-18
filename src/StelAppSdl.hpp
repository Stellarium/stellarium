#ifndef STELAPPSDL_HPP_
#define STELAPPSDL_HPP_

#ifdef USE_SDL

#include "SDL/SDL.h"
#include "StelApp.hpp"

class StelAppSdl : public StelApp
{
public:
	StelAppSdl(int argc, char** argv) :
		StelApp(argc, argv) {;}

	virtual ~StelAppSdl() {deInit();}

	///////////////////////////////////////////////////////////////////////////
	// Methods from StelApp
	///////////////////////////////////////////////////////////////////////////	
	
	//! Start the main loop and return when the program ends.
	virtual void startMainLoop(void);
	
	//! Return a list of working fullscreen hardware video modes (one per line)
	virtual string getVideoModeList(void) const;
	
	//! Return the time since when stellarium is running in second
	virtual double getTotalRunTime() const;
	
	//! Set mouse cursor display
	virtual void showCursor(bool b);
	
	//! De-init SDL / QT related stuff
	virtual void deInit();
	
	//! Swap GL buffer, should be called only for special condition
	virtual void swapGLBuffers();
	
	//! Get the width of the openGL screen
	virtual int getScreenW() const {return screenW;}
	
	//! Get the height of the openGL screen
	virtual int getScreenH() const {return screenH;}
	
	//! Terminate the application
	virtual void terminateApplication();
	
	//! Alternate fullscreen mode/windowed mode if possible
	virtual void toggleFullScreen();
	
	//! Return whether we are in fullscreen mode
	virtual bool getFullScreen() const;
	
protected:
	// Initialize openGL screen
	virtual void initOpenGL(int w, int h, int bbpMode, bool fullScreen, string iconFile);
	
	//! Save a screen shot.
	//! The format of the file, and hence the filename extension 
	//! depends on the architecture and build type.
	//! @arg filePrefix changes the beginning of the file name
	//! @arg shotDir changes the drectory where the screenshot is saved
	//! If shotDir is "" then StelFileMgr::getScreenshotDir() will be used
	virtual void saveScreenShot(const string& filePrefix="stellarium-", const string& saveDir="") const;
	
	virtual void setResizable(bool resizable);
	
private:
	// Screen size
	int screenW, screenH;
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// SDL related function and variables
	static SDL_Cursor *create_cursor(const char *image[]);

	// SDL managment variables
	SDL_Surface *Screen;// The Screen
	Uint32	TickCount;	// Used For The Tick Counter
	Uint32	LastCount;	// Used For The Tick Counter
	SDL_Cursor *Cursor;
	bool fullScreenMode;    // Are we currently in full screen mode?
};

#endif // USE_SDL

#endif /*STELAPPSDL_HPP_*/
