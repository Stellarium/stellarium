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
#include "StelGuiBase.hpp"

#include <QPainter>
#include <QPaintEngine>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QGLFramebufferObject>
#include <QSettings>

StelAppGraphicsWidget::StelAppGraphicsWidget()
	: paintState(0), useBuffers(false), backgroundBuffer(0), foregroundBuffer(0)
{
	StelApp::initStatic();
	previousPaintTime = StelApp::getTotalRunTime();
	setFocusPolicy(Qt::StrongFocus);
	stelApp = new StelApp();
}

StelAppGraphicsWidget::~StelAppGraphicsWidget()
{
	delete stelApp;
	if (backgroundBuffer)
		delete backgroundBuffer;
	if (foregroundBuffer)
		delete foregroundBuffer;
}

void StelAppGraphicsWidget::init(QSettings* conf)
{

	useBuffers = conf->value("video/use_buffers", false).toBool();
	if (useBuffers && !QGLFramebufferObject::hasOpenGLFramebufferObjects())
	{
		qDebug() << "Don't support OpenGL framebuffer objects";
		useBuffers = false;
	}
#ifndef NDEBUG
	if (useBuffers)
		qDebug() << "Use OpenGL framebuffer objects";
#endif
	stelApp->init(conf);
}

//! Iterate through the drawing sequence.
bool StelAppGraphicsWidget::paintPartial()
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
		stelApp->update(dt);
		paintState = 1;
		return true;
	}
	if (paintState == 1)
	{
		// And draw them
		if (stelApp->drawPartial())
			return true;

		paintState = 0;
		return false;
	}
	Q_ASSERT(false);
	return false;
}

#include "StelUtils.hpp"
void StelAppGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	// Don't even try to draw if we don't have a core yet (fix a bug during splash screen)
	if (!stelApp || !stelApp->getCore())
		return;

	if (painter->paintEngine()->type() != QPaintEngine::OpenGL && painter->paintEngine()->type() != QPaintEngine::OpenGL2)
	{
		qWarning("StelAppGraphicsWidget: paint needs a QGLWidget to be set as viewport on the graphics view");
		return;
	}

	StelPainter::setQPainter(painter);

	if (useBuffers)
	{
		stelApp->makeMainGLContextCurrent();
		initBuffers();
		backgroundBuffer->bind();
		QPainter* pa = new QPainter(backgroundBuffer);
		StelPainter::setQPainter(pa);

		// If we are using the gui, then we try to have the best reactivity, even if we need to lower the fps for that.
		int minFps = StelApp::getInstance().getGui()->isCurrentlyUsed() ? 16 : 2;
		while (true)
		{
			bool keep = paintPartial();
			if (!keep) // The paint is done
			{
				delete pa;
				backgroundBuffer->release();
				swapBuffers();
				break;
			}
			double spentTime = StelApp::getTotalRunTime() - previousPaintFrameTime;
			if (1. / spentTime <= minFps) // we spent too much time
			{
				// We stop the painting operation for now
				delete pa;
				backgroundBuffer->release();
				break;
			}
		}
		Q_ASSERT(!backgroundBuffer->isBound());
		Q_ASSERT(!foregroundBuffer->isBound());
		// Paint the last completed painted buffer
		StelPainter::setQPainter(painter);
		paintBuffer();
	}
	else
	{
		while (paintPartial()) {;}

#if 0
		QVector<QVector<Vec3d> > contours;
		QVector<Vec3d> c1(4);
		StelUtils::spheToRect(-0.5, -0.5, c1[3]);
		StelUtils::spheToRect(0.5, -0.5, c1[2]);
		StelUtils::spheToRect(0.5, 0.5, c1[1]);
		StelUtils::spheToRect(-0.5, 0.5, c1[0]);
		contours.append(c1);
		QVector<Vec3d> c2(4);
		StelUtils::spheToRect(-0.2, 0.2, c2[3]);
		StelUtils::spheToRect(0.2, 0.2, c2[2]);
		StelUtils::spheToRect(0.2, -0.2, c2[1]);
		StelUtils::spheToRect(-0.2, -0.2, c2[0]);
		contours.append(c2);
		SphericalPolygon holySquare;
		holySquare.setContours(contours);
		StelPainter sPainter(stelApp->getCore()->getProjection(StelCore::FrameJ2000));
		sPainter.setColor(0,0,1);
		sPainter.drawSphericalRegion(&holySquare);
		sPainter.setColor(1,1,0);
		sPainter.drawSphericalRegion(&holySquare, StelPainter::SphericalPolygonDrawModeBoundary);
#endif
	}
	StelPainter::setQPainter(NULL);
	previousPaintFrameTime = StelApp::getTotalRunTime();
}

//! Swap the buffers
//! this should be called after we finish the paint
void StelAppGraphicsWidget::swapBuffers()
{
	Q_ASSERT(useBuffers);
	QGLFramebufferObject* tmp = backgroundBuffer;
	backgroundBuffer = foregroundBuffer;
	foregroundBuffer = tmp;
}

//! Paint the foreground buffer.
void StelAppGraphicsWidget::paintBuffer()
{
	Q_ASSERT(useBuffers);
	Q_ASSERT(foregroundBuffer);
	StelPainter sPainter(StelApp::getInstance().getCore()->getProjection2d());
	sPainter.setColor(1,1,1);
	sPainter.enableTexture2d(true);
	glBindTexture(GL_TEXTURE_2D, foregroundBuffer->texture());
	sPainter.drawRect2d(0, 0, foregroundBuffer->size().width(), foregroundBuffer->size().height());
	sPainter.enableTexture2d(false);
}

//! Initialize the opengl buffer objects.
void StelAppGraphicsWidget::initBuffers()
{
	Q_ASSERT(useBuffers);
	Q_ASSERT(QGLFramebufferObject::hasOpenGLFramebufferObjects());
	if (!backgroundBuffer)
	{
		backgroundBuffer = new QGLFramebufferObject(scene()->sceneRect().size().toSize(), QGLFramebufferObject::CombinedDepthStencil);
		foregroundBuffer = new QGLFramebufferObject(scene()->sceneRect().size().toSize(), QGLFramebufferObject::CombinedDepthStencil);
		Q_ASSERT(backgroundBuffer->isValid());
		Q_ASSERT(foregroundBuffer->isValid());
	}
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
	if (backgroundBuffer)
	{
		delete backgroundBuffer;
		backgroundBuffer = NULL;
	}
	if (foregroundBuffer)
	{
		delete foregroundBuffer;
		foregroundBuffer = NULL;
	}
}
