/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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

#ifndef _STELAPPGRAPHICSWIDGET_HPP_
#define _STELAPPGRAPHICSWIDGET_HPP_

#include <QGraphicsWidget>

//! @class StelAppGraphicsWidget
//! A QGraphicsWidget encapsulating all the Stellarium main sky view and the embedded GUI widgets
//! such as the moving button bars.
//! It manages redirection of users inputs to the core and GUI.
class StelAppGraphicsWidget : public QGraphicsWidget
{
	Q_OBJECT
public:
	StelAppGraphicsWidget(int argc, char** argv);
	~StelAppGraphicsWidget();

	//! Initialize the StelAppGraphicsWidget.
	void init();

	//! Paint the main sky view and the embedded GUI widgets such as the moving button bars.
	//! This method is called automatically by the GraphicsView.
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget=0);

	//! Iterate through the drawing sequence.
	bool paintPartial(QPainter* painter);

	//! Return whether we use opengl buffers.
	bool getUseBuffers() const {return useBuffers;}
	//! Set whether we use opengl buffers.
	void setUseBuffers(bool value) {useBuffers = value;}
	
protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);
	virtual void resizeEvent(QGraphicsSceneResizeEvent* event);
	
private:
	double previousPaintTime;
	//! The main application instance.
	class StelApp* stelApp;
	//! The state of paintPartial method
	int paintState;

	//! set to true to use buffers
	bool useBuffers;
	//! The framebuffer where we are currently drawing the scene
	class QGLFramebufferObject* backgroundBuffer;
	//! The framebuffer that we use while waiting for the drawing to be done
	class QGLFramebufferObject* foregroundBuffer;

	//! Initialize the opengl buffer objects.
	void initBuffers();
	//! Swap the buffers
	//! this should be called after we finish the paint
	void swapBuffers();
	//! Paint the foreground buffer.
	void paintBuffer(QPainter* painter);
};

#endif // _STELAPPGRAPHICSWIDGET_HPP_

