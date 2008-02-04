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

#ifndef STELGLWIDGET_HPP_
#define STELGLWIDGET_HPP_

#include <QTime>
#include <cassert>
#include <QGraphicsView>
#include <QGLWidget>

class QGraphicsScene;

class StelGLWidget : public QGraphicsView
{
	Q_OBJECT;
public:
	StelGLWidget(QWidget *parent);
	~StelGLWidget();
	
	///////////////////////////////////////////////////////////////////////////
	// Override virtual methods
	virtual void initializeGL();

	//! Get the StelGLWidget singleton instance.
	//! @return the StelGLWidget singleton instance
	static StelGLWidget& getInstance() {assert(singleton); return *singleton;}
	
	void init();

	//! Define the type of viewport distorter to use
	//! @param type can be only 'fisheye_to_spheric_mirror' or anything else for no distorter
	void setViewPortDistorterType(const QString& type);
	//! Get the type of viewport distorter currently used
	QString getViewPortDistorterType() const;
	
	//! Set mouse cursor display
	void showCursor(bool b);
	
	//! Start the main drawing loop
	void startDrawingLoop();
	
	//! Get the main QGraphicsScene containing all the items managed by the QGraphicsView
	QGraphicsScene* getScene() {return scene();}
	
	class QGLWidget* glWidget;
	
protected:
	virtual void resizeEvent(QResizeEvent * event);
 	virtual void paintEvent(QPaintEvent *event);
	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void wheelEvent(QWheelEvent*);
	virtual void keyPressEvent(QKeyEvent*);
	virtual void keyReleaseEvent(QKeyEvent*);
		
private slots:
	//! Called when screen needs to be refreshed
	void recompute();
		
private:
	//! Notify that an event was handled by the program and therefore the FPS should be maximized for a couple of seconds
	void thereWasAnEvent();
	class QTimer* mainTimer;
	
	double previousTime;
	double lastEventTimeSec;
	
	// Main elements of the stel_app
	class ViewportDistorter *distorter;
	
	// The StelGLWidget singleton
	static StelGLWidget* singleton;

	class StelUI* ui;
};

#endif /*STELGLWIDGET_HPP_*/
