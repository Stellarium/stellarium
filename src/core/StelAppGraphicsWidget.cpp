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

#include <QPainter>
#include <QPaintEngine>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>

StelAppGraphicsWidget::StelAppGraphicsWidget(int argc, char** argv)
	: paintState(0)
{
	StelApp::initStatic();
	previousPaintTime = StelApp::getTotalRunTime();
	setFocusPolicy(Qt::StrongFocus);
	stelApp = new StelApp(argc, argv);
}

StelAppGraphicsWidget::~StelAppGraphicsWidget()
{
	delete stelApp;
}

void StelAppGraphicsWidget::init()
{
	stelApp->init();
}

//! Iterate through the drawing sequence.
bool StelAppGraphicsWidget::paintPartial(QPainter* painter)
{
	// qDebug() << "paintPartial" << paintState;
	if (paintState == 0)
	{
		const double now = StelApp::getTotalRunTime();
		double dt = now-previousPaintTime;
		previousPaintTime = now;
		if (dt<0)       // This fix the star scale bug!!
			return false;

		// Update the core and all modules
		StelApp::getInstance().update(dt);
		paintState = 1;
		return true;
	}
	if (paintState == 1)
	{
		// And draw them
		StelPainter::setQPainter(painter);

		if (stelApp->drawPartial())
			return true;

		paintState = 0;
		return false;
	}
	Q_ASSERT(false);
	return false;
}


void StelAppGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if (painter->paintEngine()->type() != QPaintEngine::OpenGL
#if QT_VERSION>=0x040600
		   && painter->paintEngine()->type() != QPaintEngine::OpenGL2
#endif
	   )
	{
		qWarning("StelAppGraphicsWidget: paint needs a QGLWidget to be set as viewport on the graphics view");
		return;
	}

	while (true)
	{
		bool keep = paintPartial(painter);
		if (!keep)
			break;
	}

	StelPainter::setQPainter(NULL);
}


void StelAppGraphicsWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = scene()->height() - 1 - y;
	stelApp->handleMove(x, y, event->buttons());
}

void StelAppGraphicsWidget::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = scene()->height() - 1 - y;
	QMouseEvent newEvent(QEvent::MouseButtonPress, QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	stelApp->handleClick(&newEvent);
}

void StelAppGraphicsWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = scene()->height() - 1 - y;
	QMouseEvent newEvent(QEvent::MouseButtonRelease, QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	stelApp->handleClick(&newEvent);
}

void StelAppGraphicsWidget::wheelEvent(QGraphicsSceneWheelEvent* event)
{
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = scene()->height() - 1 - y;
	QWheelEvent newEvent(QPoint(x,y), event->delta(), event->buttons(), event->modifiers(), event->orientation());
	stelApp->handleWheel(&newEvent);
}

void StelAppGraphicsWidget::keyPressEvent(QKeyEvent* event)
{
	stelApp->handleKeys(event);
}

void StelAppGraphicsWidget::keyReleaseEvent(QKeyEvent* event)
{
	stelApp->handleKeys(event);
}

void StelAppGraphicsWidget::resizeEvent(QGraphicsSceneResizeEvent* event)
{
	QGraphicsWidget::resizeEvent(event);
	stelApp->glWindowHasBeenResized(scenePos().x(), scene()->sceneRect().height()-(scenePos().y()+geometry().height()), geometry().width(), geometry().height());
}
