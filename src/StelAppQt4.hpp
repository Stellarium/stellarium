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

#include <QMainWindow>
#include <cassert>

//! Implementation of StelApp based on a Qt4 main window.
//! The fullscreen mode is just resizing the window to screen dimension and hiding the decoration
class StelAppQt4 : public QMainWindow
{
	friend class GLWidget;
public:
	
	//! Constructor
	StelAppQt4();
	virtual ~StelAppQt4();

	//! Get the StelAppQt4 singleton instance.
	//! @return the StelAppQt4 singleton instance
	static StelAppQt4& getInstance() {assert(singleton); return *singleton;}
	
	///////////////////////////////////////////////////////////////////////////
	// Override events
	virtual void keyPressEvent(QKeyEvent*);
	virtual void keyReleaseEvent(QKeyEvent*);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods from StelApp
	//! Return the time since when stellarium is running in second
	double getTotalRunTime() const;
	
	//! Set mouse cursor display
	void showCursor(bool b);
	
	//! Swap GL buffer, should be called only for special condition
	void swapGLBuffers();
	
	//! Get the width of the openGL screen
	int getScreenW() const;
	
	//! Get the height of the openGL screen
	int getScreenH() const;
	
	//! Terminate the application
	void terminateApplication(void);
	
	//! Alternate fullscreen mode/windowed mode if possible
	void toggleFullScreen();
	
	//! Get whether fullscreen is activated or not
	bool getFullScreen() const;
	
	///////////////////////////////////////////////////////////////////////////
	// Specific methods
	//! Run the whole stellarium program and return at the end, i.e after deinitialization
	//! The caller doesn't have to do anything after this call.
	//! @param argc The number of command line parameters
	//! @param argv an array of char* command line arguments
	static void runStellarium(int argc, char **argv);

	//! Initialize openGL screen
	void initOpenGL(int w, int h, int bbpMode, bool fullScreen, const QString& iconFile);
	
	//! Save a screen shot.
	//! The format of the file, and hence the filename extension 
	//! depends on the architecture and build type.
	//! @arg filePrefix changes the beginning of the file name
	//! @arg shotDir changes the drectory where the screenshot is saved
	//! If shotDir is "" then StelFileMgr::getScreenshotDir() will be used
	void saveScreenShot(const QString& filePrefix="stellarium-", const QString& saveDir="") const;
	
	//! Call this when you want to make the window (not) resizable
	void setResizable(bool resizable);

private:
	
	// The StelApp singleton
	static StelAppQt4* singleton;
};

#endif /*STELAPPQT4_HPP_*/
