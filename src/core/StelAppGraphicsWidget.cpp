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
#include "StelRenderer.hpp"
#include "StelGuiBase.hpp"
#include "StelViewportEffect.hpp"

#include <QPainter>
#include <QPaintEngine>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QGLFramebufferObject>
#include <QSettings>

StelAppGraphicsWidget::StelAppGraphicsWidget(StelRenderer* renderer)
	: paintState(0), 
	  renderer(renderer),
	  viewportEffect(NULL), 
	  doPaint(true)
{
	previousPaintTime = StelApp::getTotalRunTime();
	setFocusPolicy(Qt::StrongFocus);
	stelApp = new StelApp();
}

StelAppGraphicsWidget::~StelAppGraphicsWidget()
{
	delete stelApp;
	stelApp = NULL;
	
	if (viewportEffect) delete viewportEffect;
	viewportEffect = NULL;
	
	StelApp::deinitStatic();
}

void StelAppGraphicsWidget::init(QSettings* conf)
{
	stelApp->init(conf);
	Q_ASSERT(viewportEffect==NULL);
	setViewportEffect(conf->value("video/viewport_effect", "none").toString());
	renderer->viewportHasBeenResized(scene()->sceneRect().size().toSize());
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
	
	if (name == "framebufferOnly" || name == "none")
	{
		qDebug() << "Setting default viewport effect";
		viewportEffect = new StelViewportEffect();
	}
	else if (name == "sphericMirrorDistorter")
	{
		qDebug() << "Setting sphericMirrorDistorter viewport effect";
		viewportEffect = new StelViewportDistorterFisheyeToSphericMirror(size().width(),
		                                                                 size().height(),
		                                                                 renderer);
	}
	else
	{
		qWarning() << "Unknown viewport effect name: " << name;
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
		
	renderer->setDefaultPainter(painter);
	renderer->startDrawing();
	
	
	// When using the GUI, try to have the best reactivity, 
	// even if we need to lower the FPS.
	int minFps = StelApp::getInstance().getGui()->isCurrentlyUsed() ? 16 : 2;
	while (true)
	{
		bool keep = paintPartial();
		if (!keep) // The paint is done
		{
			renderer->finishDrawing();
			break;
		}
		double spentTime = StelApp::getTotalRunTime() - previousPaintFrameTime;
		if (1. / spentTime <= minFps) // we spent too much time
		{
			// We stop the painting operation for now
			renderer->suspendDrawing();
			break;
		}
	}
	
	renderer->drawWindow(viewportEffect);
	renderer->setDefaultPainter(NULL);
	
	previousPaintFrameTime = StelApp::getTotalRunTime();
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
	const float w = geometry().width();
	const float h = geometry().height();
	const float x = scenePos().x();
	const float y = scene()->sceneRect().height() - (scenePos().y() + geometry().height());
	
	stelApp->glWindowHasBeenResized(x, y, w, h);
	renderer->viewportHasBeenResized(scene()->sceneRect().size().toSize());
}
