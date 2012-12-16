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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELAPPGRAPHICSWIDGET_HPP_
#define _STELAPPGRAPHICSWIDGET_HPP_

#include <QGraphicsWidget>

class StelViewportEffect;

//! @class StelAppGraphicsWidget
//! A QGraphicsWidget encapsulating all the Stellarium main sky view and the embedded GUI widgets
//! such as the moving button bars.
//! It manages redirection of users inputs to the core and GUI.
class StelAppGraphicsWidget : public QGraphicsWidget
{
	Q_OBJECT
public:
	StelAppGraphicsWidget(class StelRenderer* renderer);
	~StelAppGraphicsWidget();

	//! Initialize the StelAppGraphicsWidget.
	void init(class QSettings* conf);

	//! Paint the main sky view and the embedded GUI widgets such as the moving button bars.
	//! This method is called automatically by the GraphicsView.
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget=0);

	//! Define the type of viewport effect to use
	//! @param effectName must be one of 'none', 'framebufferOnly', 'sphericMirrorDistorter'
	void setViewportEffect(const QString& effectName);
	//! Get the type of viewport effect currently used
	QString getViewportEffect() const;

	//! Set whether widget repaint are necessary.
	void setDoPaint(bool b) {doPaint=b;}

	void handleMousePress(QPointF pos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
	void handleMouseRelease(QPointF pos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
	void handleMouseMove(QPointF pos, Qt::MouseButtons buttons);

protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);
	virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

private:
	//! Provides Renderer with access to paintPartial.
	friend class StelAppGraphicsWidgetRenderClient;

	double previousPaintTime;
	//! The main application instance.
	class StelApp* stelApp;
	//! The state of paintPartial method
	int paintState;
	
	//! Used for all graphics functionality.
	class StelRenderer* renderer;

	//! Iterate through the drawing sequence.
	bool paintPartial();

	StelViewportEffect* viewportEffect;
	void distortPos(QPointF* pos);
	
	bool doPaint;
};

#endif // _STELAPPGRAPHICSWIDGET_HPP_

