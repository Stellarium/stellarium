/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#ifndef STELAPPQT4_HPP_
#define STELAPPQT4_HPP_

#include "StelApp.hpp"

#ifdef USE_QT4

class GLWidget;

//! Implementation of StelApp based on a Qt4 main window.
//! The fullscreen mode is just resizing the window to screen dimension and hiding the decoration
class StelAppQt4 : public StelApp
{
	friend class GLWidget;
	friend class StelMainWindow;
public:
	StelAppQt4(int argc, char **argv);
	virtual ~StelAppQt4();

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

	//! Empty in the case of StelAppQt4
	virtual void init(void) {;}
	
	//! Get the width of the openGL screen
	virtual int getScreenW() const;
	
	//! Get the height of the openGL screen
	virtual int getScreenH() const;
	
	//! Terminate the application
	virtual void terminateApplication(void);
	
	//! Alternate fullscreen mode/windowed mode if possible
	virtual void toggleFullScreen();
	
	//! Get whether fullscreen is activated or not
	bool getFullScreen() const;
	
	//! Return the instance of the main window of the program
	class StelMainWindow* getMainWindow() {return mainWindow;}

protected:

	//! Initialize openGL screen
	virtual void initOpenGL(int w, int h, int bbpMode, bool fullScreen, string iconFile);
	
	//! Save a screen shot.
	//! The format of the file, and hence the filename extension 
	//! depends on the architecture and build type.
	//! @arg filePrefix changes the beginning of the file name
	//! @arg shotDir changes the drectory where the screenshot is saved
	//! If shotDir is "" then StelFileMgr::getScreenshotDir() will be used
	virtual void saveScreenShot(const string& filePrefix="stellarium-", const string& saveDir="") const;
	
	//! Call this when you want to make the window (not) resizable
	virtual void setResizable(bool resizable);

private:
	class StelMainWindow* mainWindow;
	class GLWidget* winOpenGL;
};

#endif // USE_QT4

#endif /*STELAPPQT4_HPP_*/
