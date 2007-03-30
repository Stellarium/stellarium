#ifndef STELAPPSDL_HPP_
#define STELAPPSDL_HPP_

#ifdef USE_SDL

#include "SDL.h"
#include "StelApp.hpp"

class StelAppSdl : public StelApp
{
public:
	StelAppSdl(const string& configDir, const string& localeDir, const string& dataRootDir) :
		StelApp(configDir, localeDir, dataRootDir) {;}

	virtual ~StelAppSdl() {deInit();}

	///////////////////////////////////////////////////////////////////////////
	// Methods from StelApp
	///////////////////////////////////////////////////////////////////////////	
	
	//! @brief Start the main loop and return when the program ends.
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
	virtual int getScreenH() const = {return screenH;}
protected:

	// Initialize openGL screen
	virtual void initOpenGL(int w, int h, int bbpMode, bool fullScreen, string iconFile);
	
	// Save a screen shot in the HOME directory
	virtual void saveScreenShot() const;

	//! Terminate the application
	virtual void terminateApplication(void);
	
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
};

#endif // USE_SDL

#endif /*STELAPPSDL_HPP_*/
