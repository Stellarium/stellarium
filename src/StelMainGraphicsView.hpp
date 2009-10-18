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

class QGLWidget;
class QResizeEvent;
class StelGuiBase;

//! @class StelMainGraphicsView
//! Reimplement a QGraphicsView for Stellarium.
//! It is the class creating the singleton GL Widget, the main StelApp instance as well as the main GUI.
class StelMainGraphicsView : public QGraphicsView
{
Q_OBJECT
public:
	StelMainGraphicsView(QWidget* parent, int argc, char** argv);
	virtual ~StelMainGraphicsView();
	
	//! Start the main initialization of Stellarium
	void init();
	
	//! Get the StelMainGraphicsView singleton instance.
	static StelMainGraphicsView& getInstance() {Q_ASSERT(singleton); return *singleton;}
	
	//! Get the main QGLWidget
	//! @deprecated don't use that.
	QGLWidget* getOpenGLWin() {return glWidget;}
	
	//! Swap openGL buffer.
	//! @deprecated don't use that.
	void swapBuffer();

	//! Delete openGL textures (to call before the GLContext disappears)
	void deinitGL();
	
	//! Return the QGraphicsWidget encapsulating the Stellarium main sky view.
	//! Use its layout if you want to add widget on the top of the main sky view.
	class StelAppGraphicsWidget* getStelAppGraphicsWidget() {return mainSkyItem;}
	
	//! Return the top level QGraphicsWidget which contains the layout containing the Stellarium main sky view.
	//! Use its layout if you want to add widget on the side of the main sky view.
	QGraphicsWidget* getTopLevelGraphicsWidget() {return backItem;}
	
	//! Define the type of viewport distorter to use
	//! @param type can be only 'fisheye_to_spheric_mirror' or anything else for no distorter
	void setViewPortDistorterType(const QString& type);
	//! Get the type of viewport distorter currently used
	QString getViewPortDistorterType() const;
	
public slots:

	///////////////////////////////////////////////////////////////////////////
	// Specific methods
	//! Save a screen shot.
	//! The format of the file, and hence the filename extension 
	//! depends on the architecture and build type.
	//! @arg filePrefix changes the beginning of the file name
	//! @arg shotDir changes the directory where the screenshot is saved
	//! If shotDir is "" then StelFileMgr::getScreenshotDir() will be used
	void saveScreenShot(const QString& filePrefix="stellarium-", const QString& saveDir="");
	
	//! Get whether colors are inverted when saving screenshot
	bool getFlagInvertScreenShotColors() const {return flagInvertScreenShotColors;}
	//! Set whether colors should be inverted when saving screenshot
	void setFlagInvertScreenShotColors(bool b) {flagInvertScreenShotColors=b;}
			
	//! Get the state of the mouse cursor timeout flag
	bool getFlagCursorTimeout() {return flagCursorTimeout;}
	//! Get the mouse cursor timeout in seconds
	float getCursorTimeout() const {return cursorTimeout;}
	//! Get the state of the mouse cursor timeout flag
	void setFlagCursorTimeout(bool b) {flagCursorTimeout=b;}
	//! Set the mouse cursor timeout in seconds
	void setCursorTimeout(float t) {cursorTimeout=t;}
		
protected:
	virtual void resizeEvent(QResizeEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void wheelEvent(QWheelEvent* wheelEvent);
	
	//! Update the mouse pointer state and schedule next redraw.
	//! This method is called automatically by Qt.
	virtual void drawBackground(QPainter* painter, const QRectF &rect);
	virtual void drawForeground(QPainter* painter, const QRectF &rect);
	
signals:
	//! emitted when saveScreenShot is requested with saveScreenShot().
	//! doScreenshot() does the actual work (it has to do it in the main
	//! thread, where as saveScreenShot() might get called from another one.
	void screenshotRequested(void);

private slots:
	// Do the actual screenshot generation in the main thread with this method.
	void doScreenshot(void);
	
	void minFpsChanged();
	
private:
	//! Start the display loop
	void startMainLoop();
	
	//! The StelMainWindow singleton
	static StelMainGraphicsView* singleton;
	QGraphicsWidget* backItem;
	class StelAppGraphicsWidget* mainSkyItem;
	
	//! The openGL window
	QGLWidget* glWidget;
	
	StelGuiBase* gui;
	
	bool wasDeinit;
	
	bool flagInvertScreenShotColors;

	QString screenShotPrefix;
	QString screenShotDir;

	// Number of second before the mouse cursor disappears
	float cursorTimeout;
	bool flagCursorTimeout;
	
	//! Notify that an event was handled by the program and therefore the 
	//! FPS should be maximized for a couple of seconds.
	void thereWasAnEvent();
	
	//! Apply viewport distortions.
	void distortPos(QPoint* pos);
	
	double lastEventTimeSec;
	
	// The distorter currently activated
	class StelViewportDistorter *distorter;
	
	QTimer* minFpsTimer;
};


#endif // _STELMAINGRAPHICSVIEW_HPP_
