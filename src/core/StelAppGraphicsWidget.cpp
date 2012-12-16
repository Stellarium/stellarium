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

//! Provides access to paintPartial and drawing related classes to Renderer.
//!
//! This is basically just a callback object, it is created and passed 
//! on every frame.
//!
//! Renderer uses drawPartial to draw parts of the scene, allowing it 
//! to measure rendering time and suspend rendering if it's too slow.
//!
//! @see StelRenderClient
class StelAppGraphicsWidgetRenderClient : public StelRenderClient
{
public:
	//! Construct a StelAppGraphicsWidgetRenderClient providing access to specified widget.
	//!
	//! @param widget  Widget that manages drawing of the scene.
	//! @param painter Painter that paints on the widget.
	explicit StelAppGraphicsWidgetRenderClient(StelAppGraphicsWidget* widget,
	                                           QPainter* painter)
		: StelRenderClient()
		, widget(widget)
		, painter(painter)
	{
	}

	virtual bool drawPartial() {return widget->paintPartial();}

	virtual QPainter* getPainter() 
	{
		return painter;
	}

	virtual StelViewportEffect* getViewportEffect() {return widget->viewportEffect;}

private:
	//! Widget that manages drawing of the scene. 
	StelAppGraphicsWidget* const widget;

	//! QPainter that paints on the graphics widget.
	QPainter* const painter;
};

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
	stelApp->init(conf, renderer);
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
	
	if (name == "none")
	{
		qDebug() << "Not using any viewport effect";
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
		qWarning() << "Unknown viewport effect name (using no effect): " << name;
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
		if (stelApp->drawPartial(renderer))
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

	StelAppGraphicsWidgetRenderClient client(this, painter);

	// Renderer manages the rendering process in detail, e.g. 
	// deciding whether or not to suspend drawing because FPS is too low.
	renderer->renderFrame(client);
}

void StelAppGraphicsWidget::handleMouseMove(QPointF pos, Qt::MouseButtons buttons)
{
	// Apply distortion on the mouse position.
	distortPos(&pos);
	pos.setY(scene()->height() - 1 - pos.y());
	stelApp->handleMove(pos.x(), pos.y(), buttons);
}

void StelAppGraphicsWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QPointF pos = event->scenePos();
	handleMouseMove(pos, event->buttons());
}

void StelAppGraphicsWidget::handleMousePress(QPointF pos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
	// Apply distortion on the mouse position.
	// Widget gets focus on clicked
	setFocus();
	distortPos(&pos);
	pos.setY(scene()->height() - 1 - pos.y());
	QMouseEvent newEvent(QEvent::MouseButtonPress, QPoint(pos.x(),pos.y()), button, buttons, modifiers);
	stelApp->handleClick(&newEvent);
}

void StelAppGraphicsWidget::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	QPointF pos = event->scenePos();
	handleMousePress(pos, event->button(), event->buttons(), event->modifiers());
}

void StelAppGraphicsWidget::handleMouseRelease(QPointF pos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
	// Apply distortion on the mouse position.
	// Widget gets focus on clicked
	setFocus();
	distortPos(&pos);
	pos.setY(scene()->height() - 1 - pos.y());
	QMouseEvent newEvent(QEvent::MouseButtonRelease, QPoint(pos.x(),pos.y()), button, buttons, modifiers);
	stelApp->handleClick(&newEvent);
}

void StelAppGraphicsWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	QPointF pos = event->scenePos();
	handleMouseRelease(pos, event->button(), event->buttons(), event->modifiers());
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
	
	stelApp->windowHasBeenResized(x, y, w, h);
	renderer->viewportHasBeenResized(scene()->sceneRect().size().toSize());
}
