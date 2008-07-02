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

#ifndef _STELAPPGRAPHICITEM_HPP_
#define _STELAPPGRAPHICITEM_HPP_

#include <cassert>
#include <QGraphicsRectItem>

class StelAppGraphicsItem : public QObject, public QGraphicsRectItem
{
	Q_OBJECT;
public:
	StelAppGraphicsItem();
	~StelAppGraphicsItem();

	//! Get the StelMainWindow singleton instance.
	//! @deprecated
	//! @return the StelMainWindow singleton instance
	static StelAppGraphicsItem& getInstance() {assert(singleton); return *singleton;}
	
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
	
	//! Paint the whole Core of stellarium
	//! This method is called automatically by the GraphicsView
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget=0);
	
	void glWindowHasBeenResized(int w, int h);
	
	int width() {return (int)(rect().width());}
	int height() {return (int)(rect().height());}
	
	//! Switch to native OpenGL painting, i.e not using QPainter
	//! After this call revertToQtPainting MUST be called
	void switchToNativeOpenGLPainting();

	//! Revert openGL state so that Qt painting works again
	//! @return a painter that can be used
	QPainter* revertToQtPainting();
	
	QPainter* switchToQPainting();
	void revertToOpenGL();
	
protected:
 	virtual bool sceneEvent(QEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	virtual void wheelEvent(QGraphicsSceneWheelEvent* event);
	virtual void focusOutEvent(QFocusEvent* event);
	virtual void focusInEvent(QFocusEvent* event);
	
private slots:
	//! Called when screen needs to be refreshed
	void recompute();
		
private:
	//! Notify that an event was handled by the program and therefore the 
	//! FPS should be maximized for a couple of seconds.
	void thereWasAnEvent();
	class QTimer* mainTimer;
	
	double previousTime;
	double lastEventTimeSec;
	
	// Main elements of the StelApp
	class ViewportDistorter *distorter;
	
	// The StelAppGraphicsItem singleton
	static StelAppGraphicsItem* singleton;
	
	QPainter* tempPainter;
};

#endif // _STELAPPGRAPHICITEM_HPP_

