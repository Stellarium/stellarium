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

#ifndef _STELMAINGRAPHICSVIEW_HPP_
#define _STELMAINGRAPHICSVIEW_HPP_

#include <QGraphicsView>
#include <QResizeEvent>

class QGLWidget;
class QGraphicsScene;

class StelMainGraphicsView : public QGraphicsView
{
Q_OBJECT;
public:
	StelMainGraphicsView(QWidget* parent, int argc, char** argv);
	virtual ~StelMainGraphicsView();
	
	//! Start the main initialization of Stellarium
	void init();
	
	//! Get the StelMainGraphicsView singleton instance.
	//! @return the StelMainGraphicsView singleton instance
	static StelMainGraphicsView& getInstance() {Q_ASSERT(singleton); return *singleton;}
	
	//! Get the main QGLWidget
	QGLWidget* getOpenGLWin() {return glWidget;}

	//! Delete openGL textures (to call before the GLContext disappears)
	void deinitGL();
	
	//! Add a new progress bar in the lower right corner of the screen.
	//! When the progress bar is not used anymore, just delete it.
	//! @return a pointer to the progress bar
	class QProgressBar* addProgessBar();
	
	//! Activate all the QActions associated to the widget
	void activateKeyActions(bool b);

public slots:

	///////////////////////////////////////////////////////////////////////////
	// Specific methods
	//! Save a screen shot.
	//! The format of the file, and hence the filename extension 
	//! depends on the architecture and build type.
	//! @arg filePrefix changes the beginning of the file name
	//! @arg shotDir changes the directory where the screenshot is saved
	//! If shotDir is "" then StelFileMgr::getScreenshotDir() will be used
	void saveScreenShot(const QString& filePrefix="stellarium-", const QString& saveDir="") const;
	
protected:
	virtual void resizeEvent(QResizeEvent* event);
	
private:
	
	//! The StelMainWindow singleton
	static StelMainGraphicsView* singleton;
	
	//! The openGL window
	QGLWidget* glWidget;
	class StelApp* stelApp;
	
	bool wasDeinit;
};


#endif // _STELMAINGRAPHICSVIEW_HPP_
