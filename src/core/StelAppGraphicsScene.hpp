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

#ifndef _STELAPPGRAPHICSSCENE_HPP_
#define _STELAPPGRAPHICSSCENE_HPP_

#include <QGraphicsScene>

class QTimer;

//! A special QGraphicsScene for use in Stellarium.
//! It manages initialization of the program and redirects users inputs to the core and GUI
class StelAppGraphicsScene : public QGraphicsScene
{
	Q_OBJECT;
public:
	StelAppGraphicsScene();
	~StelAppGraphicsScene();

	//! Get the StelMainWindow singleton instance.
	//! @return the StelMainWindow singleton instance
	static StelAppGraphicsScene& getInstance() {Q_ASSERT(singleton); return *singleton;}
	
	//! Initialize the StelAppGraphicsScene
	void init();

	//! Start the display loop
	void startMainLoop();
			
	//! Define the type of viewport distorter to use
	//! @param type can be only 'fisheye_to_spheric_mirror' or anything else for no distorter
	void setViewPortDistorterType(const QString& type);
	//! Get the type of viewport distorter currently used
	QString getViewPortDistorterType() const;
	
	void glWindowHasBeenResized(int w, int h);
	
	//! Get the state of the mouse cursor timeout flag
	bool getFlagCursorTimeout() {return flagCursorTimeout;}
	//! Get the mouse cursor timeout in seconds
	float getCursorTimeout() const {return cursorTimeout;}
	//! Get the state of the mouse cursor timeout flag
	void setFlagCursorTimeout(bool b) {flagCursorTimeout=b;}
	//! Set the mouse cursor timeout in seconds
	void setCursorTimeout(float t) {cursorTimeout=t;}

public slots:
	void minFpsChanged();

protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);
	
	//! Paint the whole Core of stellarium.
	//! This method is called automatically by the GraphicsView
	void drawBackground(QPainter *painter, const QRectF &rect);
	
private:
	
	//! Notify that an event was handled by the program and therefore the 
	//! FPS should be maximized for a couple of seconds.
	void thereWasAnEvent();
	
	double previousTime;
	double lastEventTimeSec;
	
	// Main elements of the StelApp
	class StelViewportDistorter *distorter;
	
	// The StelAppGraphicsScene singleton
	static StelAppGraphicsScene* singleton;
	
	// Number of second before the mouse cursor disappears
	float cursorTimeout;
	bool flagCursorTimeout;

	QTimer* minFpsTimer;
};

#endif // _STELAPPGRAPHICSSCENE_HPP_

