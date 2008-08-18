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

#include <config.h>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include "fixx11h.h"
#include "StelAppGraphicsItem.hpp"
#include "ViewportDistorter.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainGraphicsView.hpp"

#include <QtOpenGL>
#include <QTimer>

StelAppGraphicsItem* StelAppGraphicsItem::singleton = NULL;
 
StelAppGraphicsItem::StelAppGraphicsItem() : tempPainter(NULL)
{
	assert(!singleton);
	singleton = this;
	
	distorter = ViewportDistorter::create("none",800,600,NULL);
	lastEventTimeSec = StelApp::getInstance().getTotalRunTime();
	previousTime = lastEventTimeSec;
}

StelAppGraphicsItem::~StelAppGraphicsItem()
{
	if (distorter)
	{
		delete distorter;
		distorter=NULL;
	}
}

void StelAppGraphicsItem::init()
{
	setViewPortDistorterType(StelApp::getInstance().getSettings()->value("video/distorter","none").toString());
}

void StelAppGraphicsItem::glWindowHasBeenResized(int w, int h)
{
	if (!distorter || (distorter && distorter->getType() == "none"))
	{
		//StelApp::getInstance().glWindowHasBeenResized(w, h);
	}
}

//! Switch to native OpenGL painting, i.e not using QPainter
//! After this call revertToQtPainting MUST be called
void StelAppGraphicsItem::switchToNativeOpenGLPainting()
{
	// Save openGL projection state
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_MULTISAMPLE);
	glDisable(GL_DITHER);
	glDisable(GL_ALPHA_TEST);
}

//! Revert openGL state so that Qt painting works again
//! @return a painter that can be used
QPainter* StelAppGraphicsItem::revertToQtPainting()
{
	// Restore openGL projection state for Qt drawings
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
	return tempPainter;
}


QPainter* StelAppGraphicsItem::switchToQPainting()
{
	if (tempPainter)
	{
		tempPainter->save();
		tempPainter->resetTransform();
	}
	// Save openGL projection state
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	if (!tempPainter)
		return NULL;
	return tempPainter;
}

//! Revert openGL state so that Qt painting works again
//! @return a painter that can be used
void StelAppGraphicsItem::revertToOpenGL()
{
	if (tempPainter)
		tempPainter->restore();
	// Restore openGL projection state for Qt drawings
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}

void StelAppGraphicsItem::drawBackground(QPainter *painter, const QRectF &)
{
	if (painter->paintEngine()->type() != QPaintEngine::OpenGL) {
		qWarning("StelAppGraphicsItem: drawBackground needs a QGLWidget to be set as viewport on the graphics view");
		return;
	}

	const double now = StelApp::getInstance().getTotalRunTime();
	double dt = now-previousTime;
	previousTime = now;
	if (dt<0)	// This fix the star scale bug!!
		return;
	
	// Update the core and all modules
	StelApp::getInstance().update(dt);
	
	switchToNativeOpenGLPainting();
	StelApp::getInstance().glWindowHasBeenResized((int)(width()), (int)(height()));
	
	// And draw them
	distorter->prepare();
	StelApp::getInstance().draw();
	distorter->distort();
	
	revertToQtPainting();
	
	double duration = 1./StelApp::getInstance().minfps;
	if (now-lastEventTimeSec<2.5)
		duration = 1./StelApp::getInstance().maxfps;
	int dur = (int)(duration*1000);
	QTimer::singleShot(dur<5 ? 5 : dur, this, SLOT(update()));
}

void StelAppGraphicsItem::thereWasAnEvent()
{
	lastEventTimeSec = StelApp::getInstance().getTotalRunTime();
}

void StelAppGraphicsItem::setViewPortDistorterType(const QString &type)
{
	if (type != getViewPortDistorterType())
	{
		if (type == "none")
		{
			StelMainGraphicsView::getInstance().getOpenGLWin()->setMaximumSize(10000,10000);
		}
		else
		{
			StelMainGraphicsView::getInstance().getOpenGLWin()->setFixedSize((int)(width()), (int)(height()));
		}
	}
	if (distorter)
	{
		delete distorter;
		distorter = NULL;
	}
	distorter = ViewportDistorter::create(type,(int)width(),(int)height(),StelApp::getInstance().getCore()->getProjection());
}

QString StelAppGraphicsItem::getViewPortDistorterType() const
{
	if (distorter)
		return distorter->getType();
	return "none";
}

// Set mouse cursor display
void StelAppGraphicsItem::showCursor(bool b)
{
//	setCursor(b ? Qt::ArrowCursor : Qt::BlankCursor);
}

void StelAppGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsScene::mouseMoveEvent(event);
	if (event->isAccepted())
		return;
	
	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = (int)height() - 1 - y;
	distorter->distortXY(x,y);
	StelApp::getInstance().handleMove(x, y, event->buttons());
	event->accept();
	
	thereWasAnEvent(); // Refresh screen ASAP
}

void StelAppGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	QGraphicsScene::mousePressEvent(event);
	if (event->isAccepted())
		return;

	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = (int)height() - 1 - y;
	distorter->distortXY(x,y);
	QMouseEvent newEvent(QEvent::MouseButtonPress, QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleClick(&newEvent);
	event->accept();
	
	thereWasAnEvent(); // Refresh screen ASAP
}

void StelAppGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	QGraphicsScene::mouseReleaseEvent(event);
	if (event->isAccepted())
		return;

	int x = (int)event->scenePos().x();
	int y = (int)event->scenePos().y();
	y = (int)height() - 1 - y;
	distorter->distortXY(x,y);
	QMouseEvent newEvent(QEvent::MouseButtonRelease, QPoint(x,y), event->button(), event->buttons(), event->modifiers());
	StelApp::getInstance().handleClick(&newEvent);
	event->accept();
	
	thereWasAnEvent(); // Refresh screen ASAP
}

void StelAppGraphicsItem::wheelEvent(QGraphicsSceneWheelEvent* event)
{
	QGraphicsScene::wheelEvent(event);
	if (event->isAccepted())
		return;
	
	int x = (int)event->pos().x();
	int y = (int)event->pos().y();
	y = (int)height() - 1 - y;
	distorter->distortXY(x,y);
	QWheelEvent newEvent(QPoint(x,y), event->delta(), event->buttons(), event->modifiers(), event->orientation());
	StelApp::getInstance().handleWheel(&newEvent);
	event->accept();

	thereWasAnEvent(); // Refresh screen ASAP
}

void StelAppGraphicsItem::keyPressEvent(QKeyEvent* event)
{
	QGraphicsScene::keyPressEvent(event);
	if (event->isAccepted())
		return;
	
	StelApp::getInstance().handleKeys(event);

	thereWasAnEvent(); // Refresh screen ASAP
}

void StelAppGraphicsItem::keyReleaseEvent(QKeyEvent* event)
{
	QGraphicsScene::keyReleaseEvent(event);
	if (event->isAccepted())
		return;
	
	StelApp::getInstance().handleKeys(event);

	thereWasAnEvent(); // Refresh screen ASAP
}
