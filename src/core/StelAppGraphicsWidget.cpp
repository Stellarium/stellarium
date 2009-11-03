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

StelAppGraphicsWidget::StelAppGraphicsWidget(int argc, char** argv)
	: paintState(0), useBuffers(false), backgroundBuffer(0), foregroundBuffer(0)
{
	StelApp::initStatic();
	previousPaintTime = StelApp::getTotalRunTime();
	setFocusPolicy(Qt::StrongFocus);
	stelApp = new StelApp(argc, argv);
}

StelAppGraphicsWidget::~StelAppGraphicsWidget()
{
	delete stelApp;
	if (backgroundBuffer)
		delete backgroundBuffer;
	if (foregroundBuffer)
		delete foregroundBuffer;
}

void StelAppGraphicsWidget::init()
{
	stelApp->init();
	QSettings* settings = StelApp::getInstance().getSettings();
	useBuffers = settings->value("video/use_buffers", false).toBool();
	if (useBuffers && !QGLFramebufferObject::hasOpenGLFramebufferObjects())
	{
		qDebug() << "Don't support OpenGL framebuffer objects";
		useBuffers = false;
	}
	if (useBuffers)
		qDebug() << "Use OpenGL framebuffer objects";
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


void StelAppGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	// Don't even try to draw if we don't have a core yet (fix a bug during splash screen)
	if (!stelApp->getCore()) return;

	if (painter->paintEngine()->type() != QPaintEngine::OpenGL
#if QT_VERSION>=0x040600
		   && painter->paintEngine()->type() != QPaintEngine::OpenGL2
#endif
	   )
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
		Q_ASSERT(backgroundBuffer->isBound());
		// If we are using the gui, then we try to have the best reactivity, even if we need to lower the fps for that.
		int minFps = StelApp::getInstance().getGui()->isCurrentlyUsed() ? 16 : 2;
		while (true)
		{
			bool keep = paintPartial();
			if (!keep) // The paint is done
			{
				backgroundBuffer->release();
				swapBuffers();
				break;
			}
			double spentTime = StelApp::getTotalRunTime() - previousPaintFrameTime;
			if (1. / spentTime <= minFps) // we spent too much time
			{
				// We stop the painting operation for now
				backgroundBuffer->release();
				break;
			}
		}
		// Paint the last completed painted buffer
		paintBuffer();
	}
	else
	{
		while (paintPartial()) {;}
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
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, foregroundBuffer->texture());
	sPainter.drawRect2d(0, 0, foregroundBuffer->size().width(), foregroundBuffer->size().height());
	glDisable(GL_TEXTURE_2D);
}

//! Initialize the opengl buffer objects.
void StelAppGraphicsWidget::initBuffers()
{
	Q_ASSERT(useBuffers);
	Q_ASSERT(QGLFramebufferObject::hasOpenGLFramebufferObjects());
	if (!backgroundBuffer)
	{
		qDebug() << "Create OpenGL framebuffers";
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
