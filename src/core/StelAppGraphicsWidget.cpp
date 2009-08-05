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

#include <config.h>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelAppGraphicsWidget.hpp"
#include "StelPainter.hpp"

#include <QtOpenGL>

StelAppGraphicsWidget::StelAppGraphicsWidget()
{
	previousTime = StelApp::getTotalRunTime();
	setFocusPolicy(Qt::StrongFocus);
}

StelAppGraphicsWidget::~StelAppGraphicsWidget()
{
}

void StelAppGraphicsWidget::init()
{
}

void StelAppGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if (painter->paintEngine()->type() != QPaintEngine::OpenGL)
	{
		qWarning("StelAppGraphicsWidget: drawBackground needs a QGLWidget to be set as viewport on the graphics view");
		return;
	}
	
	const double now = StelApp::getTotalRunTime();
	double dt = now-previousTime;
	previousTime = now;
	if (dt<0)	// This fix the star scale bug!!
		return;
	
	// Update the core and all modules
	StelApp::getInstance().update(dt);

	painter->save();
	
	StelPainter::setQPainter(painter);
	
	// And draw them
	StelApp::getInstance().draw();

	StelPainter::setQPainter(NULL);
	
	painter->restore();
}


void StelAppGraphicsWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = scene()->height() - 1 - y;
	StelApp::getInstance().handleMove(x, y, event->buttons());
}

void StelAppGraphicsWidget::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = scene()->height() - 1 - y;
	QMouseEvent newEvent(QEvent::MouseButtonPress, QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleClick(&newEvent);
}

void StelAppGraphicsWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = scene()->height() - 1 - y;
	QMouseEvent newEvent(QEvent::MouseButtonRelease, QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleClick(&newEvent);
}

void StelAppGraphicsWidget::wheelEvent(QGraphicsSceneWheelEvent* event)
{
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = scene()->height() - 1 - y;
	QWheelEvent newEvent(QPoint(x,y), event->delta(), event->buttons(), event->modifiers(), event->orientation());
	StelApp::getInstance().handleWheel(&newEvent);
}

void StelAppGraphicsWidget::keyPressEvent(QKeyEvent* event)
{
	StelApp::getInstance().handleKeys(event);
}

void StelAppGraphicsWidget::keyReleaseEvent(QKeyEvent* event)
{
	StelApp::getInstance().handleKeys(event);
}

void StelAppGraphicsWidget::resizeEvent(QGraphicsSceneResizeEvent* event)
{
	QGraphicsWidget::resizeEvent(event);
	StelApp::getInstance().glWindowHasBeenResized(scenePos().x(), scene()->sceneRect().height()-(scenePos().y()+geometry().height()), geometry().width(), geometry().height());
}
