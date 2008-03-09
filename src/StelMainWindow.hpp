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

#ifndef STELMAINWINDOW_HPP_
#define STELMAINWINDOW_HPP_

#include <QMainWindow>
#include <cassert>

class QGLWidget;
class QGraphicsView;

//! The main Stellarium window. It creates an instance of StelGLWidget and starts it
//! The fullscreen mode is just resizing the window to screen dimension and hiding the decoration
class StelMainWindow : public QMainWindow
{
	Q_OBJECT;
public:
	//! Constructor
	StelMainWindow();
	virtual ~StelMainWindow();
	
	//! Init the application
	void init(int argc, char** argv);
			
	//! Get the StelMainWindow singleton instance.
	//! @deprecated
	//! @return the StelMainWindow singleton instance
	static StelMainWindow& getInstance() {assert(singleton); return *singleton;}
	
	//! Get the main QGLWidget
	QGLWidget* getOpenGLWin() {return glWidget;}
	
	//! Get the main QGraphicsView
	QGraphicsView* getGraphicsView() {return view;}
	
public slots:
	//! Alternate fullscreen mode/windowed mode if possible
	void toggleFullScreen();
	
	//! Get whether fullscreen is activated or not
	bool getFullScreen() const;
	//! Set whether fullscreen is activated or not
	void setFullScreen(bool);
	
	///////////////////////////////////////////////////////////////////////////
	// Specific methods
	//! Save a screen shot.
	//! The format of the file, and hence the filename extension 
	//! depends on the architecture and build type.
	//! @arg filePrefix changes the beginning of the file name
	//! @arg shotDir changes the drectory where the screenshot is saved
	//! If shotDir is "" then StelFileMgr::getScreenshotDir() will be used
	void saveScreenShot(const QString& filePrefix="stellarium-", const QString& saveDir="") const;
	
private:
	//! The openGL window
	QGLWidget* glWidget;
	QGraphicsView* view;
	
	// The StelMainWindow singleton
	static StelMainWindow* singleton;
};

#endif /*STELMAINWINDOW_HPP_*/
