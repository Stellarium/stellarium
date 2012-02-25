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

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelAppGraphicsWidget.hpp"
#include "StelPainter.hpp"
#include "StelGuiBase.hpp"
#include "StelViewportEffect.hpp"

#include <QPainter>
#include <QPaintEngine>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QGLFramebufferObject>
#include <QSettings>

StelAppGraphicsWidget::StelAppGraphicsWidget()
	: paintState(0), useBuffers(false), backgroundBuffer(NULL), foregroundBuffer(NULL), viewportEffect(NULL), doPaint(true)
{
	previousPaintTime = StelApp::getTotalRunTime();
	setFocusPolicy(Qt::StrongFocus);
	stelApp = new StelApp();
}

StelAppGraphicsWidget::~StelAppGraphicsWidget()
{
	delete stelApp;
	stelApp = NULL;
	if (backgroundBuffer) delete backgroundBuffer;
	backgroundBuffer = NULL;
	if (foregroundBuffer) delete foregroundBuffer;
	foregroundBuffer = NULL;
	if (viewportEffect) delete viewportEffect;
	viewportEffect = NULL;
	
	StelApp::deinitStatic();
}

void StelAppGraphicsWidget::init(QSettings* conf)
{
	stelApp->init(conf);
	Q_ASSERT(viewportEffect==NULL);
	setViewportEffect(conf->value("video/viewport_effect", "none").toString());
	
	//previousPaintTime needs to be updated after the time zone is set
	//in StelLocaleMgr::init(), otherwise this causes an invalid value of
	//deltaT the first time it is calculated in paintPartial(), which in
	//turn causes Stellarium to start with a wrong time.
	previousPaintTime = StelApp::getTotalRunTime();
}


void StelAppGraphicsWidget::setViewportEffect(const QString& name)
{
	if (viewportEffect)
	{
		if (viewportEffect->getName()==name)
			return;
		delete viewportEffect;
		viewportEffect=NULL;
	}
	if (name=="none")
	{
		useBuffers = false;
		return;
	}
	if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
	{
		qWarning() << "Don't support OpenGL framebuffer objects, can't use Viewport effect: " << name;
		useBuffers = false;
		return;
	}

	qDebug() << "Use OpenGL framebuffer objects for viewport effect: " << name;
	useBuffers = true;
	if (name == "framebufferOnly")
	{
		viewportEffect = new StelViewportEffect();
	}
	else if (name == "sphericMirrorDistorter")
	{
		viewportEffect = new StelViewportDistorterFisheyeToSphericMirror(size().width(), size().height());
	}
	else
	{
		qWarning() << "Unknown viewport effect name: " << name;
		useBuffers=false;
	}
}

QString StelAppGraphicsWidget::getViewportEffect() const
{
	if (viewportEffect)
		return viewportEffect->getName();
	return "none";
}

void StelAppGraphicsWidget::distortPos(QPointF* pos)
{
	if (!viewportEffect)
		return;
	float x = pos->x();
	float y = pos->y();
	y = size().height() - 1 - y;
	viewportEffect->distortXY(x,y);
	pos->setX(x);
	pos->setY(size().height() - 1 - y);
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

void StelAppGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	// Don't even try to draw if we don't have a core yet (fix a bug during splash screen)
	if (!stelApp || !stelApp->getCore() || !doPaint)
		return;
	
	StelPainter::setQPainter(painter);

	if (useBuffers)
	{
		StelPainter::makeMainGLContextCurrent();
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
		viewportEffect->paintViewportBuffer(foregroundBuffer);
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
	// Apply distortion on the mouse position.
	QPointF pos = event->scenePos();
	distortPos(&pos);
	pos.setY(scene()->height() - 1 - pos.y());
	stelApp->handleMove(pos.x(), pos.y(), event->buttons());
}

void StelAppGraphicsWidget::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	// Apply distortion on the mouse position.
	// Widget gets focus on clicked
	setFocus();
	QPointF pos = event->scenePos();
	distortPos(&pos);
	pos.setY(scene()->height() - 1 - pos.y());
	QMouseEvent newEvent(QEvent::MouseButtonPress, QPoint(pos.x(),pos.y()), event->button(), event->buttons(), event->modifiers());
	stelApp->handleClick(&newEvent);
}

void StelAppGraphicsWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	// Apply distortion on the mouse position.
	QPointF pos = event->scenePos();
	distortPos(&pos);
	pos.setY(scene()->height() - 1 - pos.y());
	QMouseEvent newEvent(QEvent::MouseButtonRelease, QPoint(pos.x(),pos.y()), event->button(), event->buttons(), event->modifiers());
	stelApp->handleClick(&newEvent);
}

void StelAppGraphicsWidget::wheelEvent(QGraphicsSceneWheelEvent* event)
{
	// Apply distortion on the mouse position.
	QPointF pos = event->scenePos();
	distortPos(&pos);
	pos.setY(scene()->height() - 1 - pos.y());
	QWheelEvent newEvent(QPoint(pos.x(),pos.y()), event->delta(), event->buttons(), event->modifiers(), event->orientation());
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
