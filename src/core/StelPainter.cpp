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

#ifdef STELPAINTER_GL2
 QGLShaderProgram* StelPainter::colorShaderProgram=NULL;
 QGLShaderProgram* StelPainter::texturesShaderProgram=NULL;
 QGLShaderProgram* StelPainter::basicShaderProgram=NULL;
 QGLShaderProgram* StelPainter::texturesColorShaderProgram=NULL;
 StelPainter::BasicShaderVars StelPainter::basicShaderVars;
 StelPainter::TexturesShaderVars StelPainter::texturesShaderVars;
 StelPainter::TexturesColorShaderVars StelPainter::texturesColorShaderVars;
#endif

		//QPainter qPainter(glWidget);
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


static QVarLengthArray<Vec3f, 4096> polygonVertexArray;
static QVarLengthArray<Vec2f, 4096> polygonTextureCoordArray;
static QVarLengthArray<unsigned int, 4096> indexArray;


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

#ifdef STELPAINTER_GL2
void StelPainter::TEMPSpecifyShaders(QGLShaderProgram *plain,
                                     QGLShaderProgram *color,
                                     QGLShaderProgram *texture,
                                     QGLShaderProgram *colorTexture)
{
	basicShaderProgram = plain;
	basicShaderVars.projectionMatrix = basicShaderProgram->uniformLocation("projectionMatrix");
	basicShaderVars.globalColor = basicShaderProgram->uniformLocation("globalColor");
	basicShaderVars.vertex = basicShaderProgram->attributeLocation("vertex");

	colorShaderProgram = color;

	texturesShaderProgram = texture;
	texturesShaderVars.projectionMatrix = texturesShaderProgram->
	                                      uniformLocation("projectionMatrix");
	texturesShaderVars.texCoord = texturesShaderProgram->attributeLocation("texCoord");
	texturesShaderVars.vertex = texturesShaderProgram->attributeLocation("vertex");
	texturesShaderVars.globalColor = texturesShaderProgram->uniformLocation("globalColor");
	texturesShaderVars.texture = texturesShaderProgram->uniformLocation("tex");
	
	texturesColorShaderProgram = colorTexture;
	texturesColorShaderVars.projectionMatrix = texturesColorShaderProgram->
	                                           uniformLocation("projectionMatrix");
	texturesColorShaderVars.texCoord = texturesColorShaderProgram->
	                                   attributeLocation("texCoord");
	texturesColorShaderVars.vertex = texturesColorShaderProgram->attributeLocation("vertex");
	texturesColorShaderVars.color = texturesColorShaderProgram->attributeLocation("color");
	texturesColorShaderVars.texture = texturesColorShaderProgram->uniformLocation("tex");
}
#endif


void StelPainter::setArrays(const Vec3d* vertice, const Vec2f* texCoords, const Vec3f* colorArray, const Vec3f* normalArray)
{
	enableClientStates(vertice, texCoords, colorArray, normalArray);
	setVertexPointer(3, GL_DOUBLE, vertice);
	setTexCoordPointer(2, GL_FLOAT, texCoords);
	setColorPointer(3, GL_FLOAT, colorArray);
	setNormalPointer(GL_FLOAT, normalArray);
}

void StelPainter::enableClientStates(bool vertex, bool texture, bool color, bool normal)
{
	vertexArray.enabled = vertex;
	texCoordArray.enabled = texture;
	colorArray.enabled = color;
	normalArray.enabled = normal;
}

void StelPainter::drawFromArray(DrawingMode mode, int count, int offset, bool doProj, const unsigned int* indices)
{
	ArrayDesc projectedVertexArray = vertexArray;
	if (doProj)
	{
		// Project the vertex array using current projection
		if (indices)
			projectedVertexArray = projectArray(vertexArray, 0, count, indices + offset);
		else
			projectedVertexArray = projectArray(vertexArray, offset, count, NULL);
	}

#ifndef STELPAINTER_GL2
	// Enable the client state and set the opengl array for each array
	Q_ASSERT(projectedVertexArray.enabled);
	Q_ASSERT(projectedVertexArray.pointer);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(projectedVertexArray.size, projectedVertexArray.type, 0, projectedVertexArray.pointer);
	if (texCoordArray.enabled)
	{
		Q_ASSERT(texCoordArray.pointer);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(texCoordArray.size, texCoordArray.type, 0, texCoordArray.pointer);
	}
	if (normalArray.enabled)
	{
		Q_ASSERT(normalArray.pointer);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(normalArray.type, 0, normalArray.pointer);
	}
	if (colorArray.enabled)
	{
		Q_ASSERT(colorArray.pointer);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(colorArray.size, colorArray.type, 0, colorArray.pointer);
	}

#else
	QGLShaderProgram* pr=NULL;

	const Mat4f& m = getProjector()->getProjectionMatrix();
	const QMatrix4x4 qMat(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);

	if (!texCoordArray.enabled && !colorArray.enabled && !normalArray.enabled)
	{
		pr = basicShaderProgram;
		pr->bind();
		pr->setAttributeArray(basicShaderVars.vertex, (const GLfloat*)projectedVertexArray.pointer, projectedVertexArray.size);
		pr->enableAttributeArray(basicShaderVars.vertex);
		pr->setUniformValue(basicShaderVars.projectionMatrix, qMat);
		pr->setUniformValue(basicShaderVars.globalColor, currentColor[0], currentColor[1], currentColor[2], currentColor[3]);
	}
	else if (texCoordArray.enabled && !colorArray.enabled && !normalArray.enabled)
	{
		pr = texturesShaderProgram;
		pr->bind();
		pr->setAttributeArray(texturesShaderVars.vertex, (const GLfloat*)projectedVertexArray.pointer, projectedVertexArray.size);
		pr->enableAttributeArray(texturesShaderVars.vertex);
		pr->setUniformValue(texturesShaderVars.projectionMatrix, qMat);
		pr->setUniformValue(texturesShaderVars.globalColor, currentColor[0], currentColor[1], currentColor[2], currentColor[3]);
		pr->setAttributeArray(texturesShaderVars.texCoord, (const GLfloat*)texCoordArray.pointer, 2);
		pr->enableAttributeArray(texturesShaderVars.texCoord);
		//pr->setUniformValue(texturesShaderVars.texture, 0);    // use texture unit 0
	}
	else if (texCoordArray.enabled && colorArray.enabled && !normalArray.enabled)
	{
		pr = texturesColorShaderProgram;
		pr->bind();
		pr->setAttributeArray(texturesColorShaderVars.vertex, (const GLfloat*)projectedVertexArray.pointer, projectedVertexArray.size);
		pr->enableAttributeArray(texturesColorShaderVars.vertex);
		pr->setUniformValue(texturesColorShaderVars.projectionMatrix, qMat);
		pr->setAttributeArray(texturesColorShaderVars.texCoord, (const GLfloat*)texCoordArray.pointer, 2);
		pr->enableAttributeArray(texturesColorShaderVars.texCoord);
		pr->setAttributeArray(texturesColorShaderVars.color, (const GLfloat*)colorArray.pointer, colorArray.size);
		pr->enableAttributeArray(texturesColorShaderVars.color);
		//pr->setUniformValue(texturesShaderVars.texture, 0);    // use texture unit 0
	}
	else
	{
		qDebug() << "Unhandled parameters." << texCoordArray.enabled << colorArray.enabled << normalArray.enabled;
		return;
	}
#endif
	if (indices)
		glDrawElements(mode, count, GL_UNSIGNED_INT, indices + offset);
	else
		glDrawArrays(mode, offset, count);
#ifndef STELPAINTER_GL2
	glDisableClientState(GL_VERTEX_ARRAY);
	if (texCoordArray.enabled)
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (normalArray.enabled)
		glDisableClientState(GL_NORMAL_ARRAY);
	if (colorArray.enabled)
		glDisableClientState(GL_COLOR_ARRAY);
#else
	if (pr==texturesColorShaderProgram)
	{
		pr->disableAttributeArray(texturesColorShaderVars.texCoord);
		pr->disableAttributeArray(texturesColorShaderVars.vertex);
		pr->disableAttributeArray(texturesColorShaderVars.color);
	}
	else if (pr==texturesShaderProgram)
	{
		pr->disableAttributeArray(texturesShaderVars.texCoord);
		pr->disableAttributeArray(texturesShaderVars.vertex);
	}
	else if (pr == texturesShaderProgram)
	{
		pr->disableAttributeArray(basicShaderVars.vertex);
	}
	if (pr)
		pr->release();
#endif
}


StelPainter::ArrayDesc StelPainter::projectArray(const StelPainter::ArrayDesc& array, int offset, int count, const unsigned int* indices)
{
	// XXX: we should use a more generic way to test whether or not to do the projection.
	if (dynamic_cast<StelProjector2d*>(prj.data()))
	{
		return array;
	}

	Q_ASSERT(array.size == 3);
	Q_ASSERT(array.type == GL_DOUBLE);
	Vec3d* vecArray = (Vec3d*)array.pointer;

	// We have two different cases :
	// 1) We are not using an indice array.  In that case the size of the array is known
	// 2) We are using an indice array.  In that case we have to find the max value by iterating through the indices.
	if (!indices)
	{
		polygonVertexArray.resize(offset + count);
		prj->project(count, vecArray + offset, polygonVertexArray.data() + offset);
	} else
	{
		// we need to find the max value of the indices !
		unsigned int max = 0;
		for (int i = offset; i < offset + count; ++i)
		{
			max = std::max(max, indices[i]);
		}
		polygonVertexArray.resize(max+1);
		prj->project(max + 1, vecArray + offset, polygonVertexArray.data() + offset);
	}

	ArrayDesc ret;
	ret.size = 3;
	ret.type = GL_FLOAT;
	ret.pointer = polygonVertexArray.constData();
	ret.enabled = array.enabled;
	return ret;
}
