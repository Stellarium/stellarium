/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#include "StelPainter.hpp"
#include <QtOpenGL>

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelProjector.hpp"
#include "StelProjectorClasses.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QString>
#include <QSettings>
#include <QLinkedList>
#include <QPainter>
#include <QMutex>
#include <QVarLengthArray>
#include <QPaintEngine>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#ifndef NDEBUG
QMutex* StelPainter::globalMutex = new QMutex();
#endif

QPainter* StelPainter::qPainter = NULL;
QGLContext* StelPainter::glContext = NULL;

bool StelPainter::isNoPowerOfTwoAllowed;

void StelPainter::setQPainter(QPainter* p)
{
	qPainter=p;
	if (p==NULL)
		return;

	if (p->paintEngine()->type() != QPaintEngine::OpenGL && p->paintEngine()->type() != QPaintEngine::OpenGL2)
	{
		qCritical("StelPainter::setQPainter(): StelPainter needs a QGLWidget to be set as viewport on the graphics view");
		return;
	}
	QGLWidget* glwidget = dynamic_cast<QGLWidget*>(p->device());
	if (glwidget && glwidget->context()!=glContext)
	{
		qCritical("StelPainter::setQPainter(): StelPainter needs to paint on a GLWidget with the same GL context as the one used for initialization.");
		return;
	}
}

void StelPainter::makeMainGLContextCurrent()
{
	Q_ASSERT(glContext!=NULL);
	Q_ASSERT(glContext->isValid());
	glContext->makeCurrent();
}

StelPainter::StelPainter(const StelProjectorP& proj) : prj(proj)
{
	Q_ASSERT(proj);

#ifndef NDEBUG
	Q_ASSERT(globalMutex);
	GLenum er = glGetError();
	if (er!=GL_NO_ERROR)
	{
		if (er==GL_INVALID_OPERATION)
			qFatal("Invalid openGL operation. It is likely that you used openGL calls without having a valid instance of StelPainter");
	}

	// Lock the global mutex ensuring that no other instances of StelPainter are currently being used
	if (globalMutex->tryLock()==false)
	{
		qFatal("There can be only 1 instance of StelPainter at a given time");
	}
#endif

	Q_ASSERT(qPainter);
	qPainter->beginNativePainting();

#ifndef STELPAINTER_GL2
	// Save openGL projection state
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	

	glDisable(GL_LIGHTING);
	glDisable(GL_MULTISAMPLE);
	glDisable(GL_DITHER);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_LINE_SMOOTH);
#endif
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	// Fix some problem when using Qt OpenGL2 engine
	glStencilMask(0x11111111);
	// Deactivate drawing in depth buffer by default
	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	setProjector(proj);
}

void StelPainter::setProjector(const StelProjectorP& p)
{
	prj=p;
	// Init GL viewport to current projector values
	glViewport(prj->viewportXywh[0], prj->viewportXywh[1], prj->viewportXywh[2], prj->viewportXywh[3]);
	glFrontFace(prj->flipFrontBackFace()?GL_CW:GL_CCW);
#ifndef STELPAINTER_GL2
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Set the real openGL projection and modelview matrix to 2d orthographic projection
	// thus we never need to change to 2dMode from now on before drawing
	glMultMatrixf(prj->getProjectionMatrix());
	glMatrixMode(GL_MODELVIEW);
#endif
}

StelPainter::~StelPainter()
{
	Q_ASSERT(qPainter);

#ifndef STELPAINTER_GL2
	// Restore openGL projection state for Qt drawings
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#endif
#ifndef NDEBUG
	GLenum er = glGetError();
	if (er!=GL_NO_ERROR)
	{
		if (er==GL_INVALID_OPERATION)
			qFatal("Invalid openGL operation detected in ~StelPainter()");
	}
#endif
	qPainter->endNativePainting();

#ifndef NDEBUG
	// We are done with this StelPainter
	globalMutex->unlock();
#endif
}

// Draw the given SphericalRegion.
void StelPainter::drawSphericalRegion(const SphericalRegion* poly, SphericalPolygonDrawMode drawMode, const SphericalCap* clippingCap, bool doSubDivise, double maxSqDistortion)
{
	if (!prj->getBoundingCap().intersects(poly->getBoundingCap()))
		return;

	switch (drawMode)
	{
		case SphericalPolygonDrawModeBoundary:
			Q_ASSERT_X(false, Q_FUNC_INFO,
			           "GL-REFACTOR - TODO boundary draw mode based on StelCircleArcRenderer");
			break;
		case SphericalPolygonDrawModeFill:
		case SphericalPolygonDrawModeTextureFill:
			Q_ASSERT_X(false, Q_FUNC_INFO,
			           "GL-REFACTOR - fill draw modes were refactored already");
		default:
			Q_ASSERT(0);
	}
}

//GL-REFACTOR: This has been refactored into QGL2Renderer and will be removed after 
//the rest of StelPainter is refactored.
void StelPainter::initSystemGLInfo(QGLContext* ctx)
{
	
	Q_ASSERT(glContext==NULL);
	glContext = ctx;

	makeMainGLContextCurrent();
	isNoPowerOfTwoAllowed = QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_Version_2_0) || QGLFormat::openGLVersionFlags().testFlag(QGLFormat::OpenGL_ES_Version_2_0);
}
