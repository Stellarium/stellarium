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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "GLee.h"
#include "fixx11h.h"

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QString>
#include <QSettings>
#include <QLinkedList>
#include <QPainter>
#include <QMutex>
#include <QVarLengthArray>
#include <QPaintEngine>
#include <QGLContext>

#ifndef NDEBUG
QMutex* StelPainter::globalMutex = new QMutex();
#endif

bool StelPainter::flagGlPointSprite = false;
QPainter* StelPainter::qPainter = NULL;

void StelPainter::setQPainter(QPainter* p)
{
	qPainter=p;
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

	switchToNativeOpenGLPainting();

	// Init GL viewport to current projector values
	glViewport(prj->viewportXywh[0], prj->viewportXywh[1], prj->viewportXywh[2], prj->viewportXywh[3]);
	initGlMatrixOrtho2d();

	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_MULTISAMPLE);
	glDisable(GL_DITHER);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_TEXTURE_2D);

	glFrontFace(prj->needGlFrontFaceCW()?GL_CW:GL_CCW);
}

StelPainter::~StelPainter()
{
	revertToQtPainting();

#ifndef NDEBUG
	// We are done with this StelPainter
	globalMutex->unlock();
#endif
}


void StelPainter::setFont(const QFont& font)
{
	Q_ASSERT(qPainter);
	qPainter->setFont(font);
}

QFontMetrics StelPainter::getFontMetrics() const
{
	Q_ASSERT(qPainter);
	return qPainter->fontMetrics();
}

//! Switch to native OpenGL painting, i.e not using QPainter
//! After this call revertToQtPainting MUST be called
void StelPainter::switchToNativeOpenGLPainting() const
{
	Q_ASSERT(qPainter);
	// Ensure that the current GL content is the one of our main GL window
	QGLWidget* w = dynamic_cast<QGLWidget*>(qPainter->device());
	if (w!=0)
	{
		Q_ASSERT(w->isValid());
		w->makeCurrent();
	}

	// Save openGL projection state
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	//glDisable(GL_MULTISAMPLE); doesn't work on win32
	glDisable(GL_DITHER);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}

//! Revert openGL state so that Qt painting works again
void StelPainter::revertToQtPainting() const
{
	Q_ASSERT(qPainter);
	// Ensure that the current GL content is the one of our main GL window
	QGLWidget* w = dynamic_cast<QGLWidget*>(qPainter->device());
	if (w!=0)
	{
		Q_ASSERT(w->isValid());
		w->makeCurrent();
	}

	// Restore openGL projection state for Qt drawings
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
	glPopClientAttrib();
#ifndef NDEBUG
	GLenum er = glGetError();
	if (er!=GL_NO_ERROR)
	{
		if (er==GL_INVALID_OPERATION)
			qFatal("Invalid openGL operation in StelPainter::revertToQtPainting()");
	}
#endif
}

void StelPainter::initSystemGLInfo()
{
	// TODO move that in static code
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	flagGlPointSprite = conf->value("projection/flag_use_gl_point_sprite",false).toBool();
	flagGlPointSprite = flagGlPointSprite && GLEE_ARB_point_sprite;
	if (flagGlPointSprite)
	{
		glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );
		glEnable(GL_POINT_SPRITE_ARB);
		glEnable(GL_POINT_SMOOTH);
		qDebug() << "INFO: using GL_ARB_point_sprite";
	}
	else
	{
			//qDebug() << "WARNING: GL_ARB_point_sprite not available";
	}
}

/*************************************************************************
 Init the real openGL Matrices to a 2d orthographic projection
*************************************************************************/
void StelPainter::initGlMatrixOrtho2d(void) const
{
	// Set the real openGL projection and modelview matrix to orthographic projection
	// thus we never need to change to 2dMode from now on before drawing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(prj->viewportXywh[0], prj->viewportXywh[0] + prj->viewportXywh[2],
			prj->viewportXywh[1], prj->viewportXywh[1] + prj->viewportXywh[3], -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}



///////////////////////////////////////////////////////////////////////////
// Standard methods for drawing primitives

// Fill with black around the circle
void StelPainter::drawViewportShape(void) const
{
	if (prj->maskType != StelProjector::MaskDisk)
		return;

	glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	glPushMatrix();
	glTranslated(prj->viewportCenter[0],prj->viewportCenter[1],0.0);

	// Got rid of GLU and therefore copied the code there
// 	GLUquadricObj * p = gluNewQuadric();
// 	gluDisk(p, 0.5*prj->viewportFovDiameter, , 256, 1);  // should always cover whole screen
// 	gluDeleteQuadric(p);

	GLfloat innerRadius = 0.5*prj->viewportFovDiameter;
	GLfloat outerRadius = prj->getViewportWidth()+prj->getViewportHeight();
	GLint slices = 256;
	GLfloat sweepAngle = 360.;

	GLfloat sinCache[240];
	GLfloat cosCache[240];
	GLfloat vertices[(240+1)*2][3];
	GLfloat deltaRadius;
	GLfloat radiusHigh;

	if (slices>=240)
	{
		slices=240-1;
	}

	if (outerRadius<=0.0 || innerRadius<0.0 ||innerRadius > outerRadius)
	{
		Q_ASSERT(0);
		return;
	}

	/* Compute length (needed for normal calculations) */
	deltaRadius=outerRadius-innerRadius;

	/* Cache is the vertex locations cache */
	for (int i=0; i<=slices; i++)
	{
		GLfloat angle=((M_PI*sweepAngle)/180.0f)*i/slices;
		sinCache[i]=(GLfloat)sin(angle);
		cosCache[i]=(GLfloat)cos(angle);
	}

	sinCache[slices]=sinCache[0];
	cosCache[slices]=cosCache[0];

	/* Enable arrays */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertices);

	radiusHigh=outerRadius-deltaRadius;
	for (int i=0; i<=slices; i++)
	{
		vertices[i*2][0]=outerRadius*sinCache[i];
		vertices[i*2][1]=outerRadius*cosCache[i];
		vertices[i*2][2]=0.0;
		vertices[i*2+1][0]=radiusHigh*sinCache[i];
		vertices[i*2+1][1]=radiusHigh*cosCache[i];
		vertices[i*2+1][2]=0.0;
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, ((slices+1)*2));

	glDisableClientState(GL_VERTEX_ARRAY);

	glPopMatrix();
}

#define MAX_STACKS 4096
static double cos_sin_rho[2*(MAX_STACKS+1)];
#define MAX_SLICES 4096
static double cos_sin_theta[2*(MAX_SLICES+1)];

static
void ComputeCosSinTheta(double phi,int segments) {
  double *cos_sin = cos_sin_theta;
  double *cos_sin_rev = cos_sin + 2*(segments+1);
  const double c = cos(phi);
  const double s = sin(phi);
  *cos_sin++ = 1.0;
  *cos_sin++ = 0.0;
  *--cos_sin_rev = -cos_sin[-1];
  *--cos_sin_rev =  cos_sin[-2];
  *cos_sin++ = c;
  *cos_sin++ = s;
  *--cos_sin_rev = -cos_sin[-1];
  *--cos_sin_rev =  cos_sin[-2];
  while (cos_sin < cos_sin_rev) {
	cos_sin[0] = cos_sin[-2]*c - cos_sin[-1]*s;
	cos_sin[1] = cos_sin[-2]*s + cos_sin[-1]*c;
	cos_sin += 2;
	*--cos_sin_rev = -cos_sin[-1];
	*--cos_sin_rev =  cos_sin[-2];
  }
}

static
void ComputeCosSinRho(double phi,int segments) {
  double *cos_sin = cos_sin_rho;
  double *cos_sin_rev = cos_sin + 2*(segments+1);
  const double c = cos(phi);
  const double s = sin(phi);
  *cos_sin++ = 1.0;
  *cos_sin++ = 0.0;
  *--cos_sin_rev =  cos_sin[-1];
  *--cos_sin_rev = -cos_sin[-2];
  *cos_sin++ = c;
  *cos_sin++ = s;
  *--cos_sin_rev =  cos_sin[-1];
  *--cos_sin_rev = -cos_sin[-2];
  while (cos_sin < cos_sin_rev) {
	cos_sin[0] = cos_sin[-2]*c - cos_sin[-1]*s;
	cos_sin[1] = cos_sin[-2]*s + cos_sin[-1]*c;
	cos_sin += 2;
	*--cos_sin_rev =  cos_sin[-1];
	*--cos_sin_rev = -cos_sin[-2];
	segments--;
  }
}


void StelPainter::sFanDisk(double radius, int innerFanSlices, int level) const
{
	Q_ASSERT(level<64);
	double rad[64];
	int i,j;
	rad[level] = radius;
	for (i=level-1;i>=0;--i)
	{
		rad[i] = rad[i+1]*(1.0-M_PI/(innerFanSlices<<(i+1)))*2.0/3.0;
	}
	int slices = innerFanSlices<<level;
	const double dtheta = 2.0 * M_PI / slices;
	Q_ASSERT(slices<=MAX_SLICES);
	ComputeCosSinTheta(dtheta,slices);
	double *cos_sin_theta_p;
	int slices_step = 2;
	static QVector<double> vertexArr;
	static QVector<float> texCoordArr;
	double x,y;

	for (i=level;i>0;--i,slices_step<<=1)
	{
		for (j=0,cos_sin_theta_p=cos_sin_theta; j<slices; j+=slices_step,cos_sin_theta_p+=2*slices_step)
		{
			vertexArr.resize(0);
			texCoordArr.resize(0);
			x = rad[i]*cos_sin_theta_p[slices_step];
			y = rad[i]*cos_sin_theta_p[slices_step+1];
			texCoordArr << 0.5*(1.0+x/radius) << 0.5*(1.0+y/radius);
			vertexArr << x << y << 0;

			x = rad[i]*cos_sin_theta_p[2*slices_step];
			y = rad[i]*cos_sin_theta_p[2*slices_step+1];
			texCoordArr << 0.5*(1.0+x/radius) << 0.5*(1.0+y/radius);
			vertexArr << x << y << 0;

			x = rad[i-1]*cos_sin_theta_p[2*slices_step];
			y = rad[i-1]*cos_sin_theta_p[2*slices_step+1];
			texCoordArr << 0.5*(1.0+x/radius) << 0.5*(1.0+y/radius);
			vertexArr << x << y << 0;

			x = rad[i-1]*cos_sin_theta_p[0];
			y = rad[i-1]*cos_sin_theta_p[1];
			texCoordArr << 0.5*(1.0+x/radius) << 0.5*(1.0+y/radius);
			vertexArr << x << y << 0;

			x = rad[i]*cos_sin_theta_p[0];
			y = rad[i]*cos_sin_theta_p[1];
			texCoordArr << 0.5*(1.0+x/radius) << 0.5*(1.0+y/radius);
			vertexArr << x << y << 0;
			drawArrays(GL_TRIANGLE_FAN, vertexArr.size()/3, (Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData());
		}
	}
	// draw the inner polygon
	slices_step>>=1;
	vertexArr.resize(0);
	texCoordArr.resize(0);
	texCoordArr << 0.5 << 0.5;
	vertexArr << 0 << 0 << 0;
	for (j=0,cos_sin_theta_p=cos_sin_theta;	j<=slices; j+=slices_step,cos_sin_theta_p+=2*slices_step)
	{
		x = rad[0]*cos_sin_theta_p[0];
		y = rad[0]*cos_sin_theta_p[1];
		texCoordArr << 0.5*(1.0+x/radius) << 0.5*(1.0+y/radius);
		vertexArr << x << y << 0;
	}
	drawArrays(GL_TRIANGLE_FAN, vertexArr.size()/3, (Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData());
}

void StelPainter::sRing(GLdouble rMin, GLdouble rMax, GLint slices, GLint stacks, int orientInside) const
{
	double x,y;
	int j;

	const double nsign = (orientInside)?-1.0:1.0;

	const double dr = (rMax-rMin) / stacks;
	const double dtheta = 2.0 * M_PI / slices;
	if (slices < 0) slices = -slices;
	Q_ASSERT(slices<=MAX_SLICES);
	ComputeCosSinTheta(dtheta,slices);
	double *cos_sin_theta_p;

	static QVector<double> vertexArr;
	static QVector<float> texCoordArr;
	static QVector<float> normalArr;

	// draw intermediate stacks as quad strips
	for (double r = rMin; r < rMax; r+=dr)
	{
		const double tex_r0 = (r-rMin)/(rMax-rMin);
		const double tex_r1 = (r+dr-rMin)/(rMax-rMin);
		vertexArr.resize(0);
		texCoordArr.resize(0);
		normalArr.resize(0);
		for (j=0,cos_sin_theta_p=cos_sin_theta; j<=slices; ++j,cos_sin_theta_p+=2)
		{
			x = r*cos_sin_theta_p[0];
			y = r*cos_sin_theta_p[1];
			normalArr << 0 << 0 << nsign;
			texCoordArr << tex_r0 << 0.5;
			vertexArr << x << y << 0;
			x = (r+dr)*cos_sin_theta_p[0];
			y = (r+dr)*cos_sin_theta_p[1];
			normalArr << 0 << 0 << nsign;
			texCoordArr << tex_r1 << 0.5;
			vertexArr << x << y << 0;
		}
		drawArrays(GL_TRIANGLE_STRIP, vertexArr.size()/3, (Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData(), NULL, (Vec3f*)normalArr.constData());
	}
}

static void sSphereMapTexCoordFast(double rho_div_fov, double costheta, double sintheta, QVector<float>& out)
{
	if (rho_div_fov>0.5)
		rho_div_fov=0.5;
	out << 0.5 + rho_div_fov * costheta << 0.5 + rho_div_fov * sintheta;
}

void StelPainter::sSphereMap(GLdouble radius, GLint slices, GLint stacks, double textureFov, int orientInside) const
{
	double rho,x,y,z;
	int i, j;
	const double nsign = orientInside?-1:1;

	double drho = M_PI / stacks;
	Q_ASSERT(stacks<=MAX_STACKS);
	ComputeCosSinRho(drho,stacks);
	double *cos_sin_rho_p;

	const double dtheta = 2.0 * M_PI / slices;
	Q_ASSERT(slices<=MAX_SLICES);

	ComputeCosSinTheta(dtheta,slices);
	double *cos_sin_theta_p;

	drho/=textureFov;

	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
	// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)

	const int imax = stacks;

	static QVector<double> vertexArr;
	static QVector<float> texCoordArr;
	static QVector<float> normalArr;

	// draw intermediate stacks as quad strips
	if (!orientInside) // nsign==1
	{
		for (i = 0,cos_sin_rho_p=cos_sin_rho,rho=0.0; i < imax; ++i,cos_sin_rho_p+=2,rho+=drho)
		{
			vertexArr.resize(0);
			texCoordArr.resize(0);
			normalArr.resize(0);
			for (j=0,cos_sin_theta_p=cos_sin_theta;j<=slices;++j,cos_sin_theta_p+=2)
			{
				x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
				z = cos_sin_rho_p[0];
				normalArr << x*nsign <<  y*nsign << z*nsign;
				sSphereMapTexCoordFast(rho, cos_sin_theta_p[0], cos_sin_theta_p[1], texCoordArr);
				vertexArr << x*radius << y*radius << z*radius;

				x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
				z = cos_sin_rho_p[2];
				normalArr << x*nsign <<  y*nsign << z*nsign;
				sSphereMapTexCoordFast(rho + drho, cos_sin_theta_p[0], cos_sin_theta_p[1], texCoordArr);
				vertexArr << x*radius << y*radius << z*radius;
			}
			drawArrays(GL_TRIANGLE_STRIP, vertexArr.size()/3, (Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData(), NULL, (Vec3f*)normalArr.constData());
		}
	}
	else
	{
		for (i = 0,cos_sin_rho_p=cos_sin_rho,rho=0.0; i < imax; ++i,cos_sin_rho_p+=2,rho+=drho)
		{
			vertexArr.resize(0);
			texCoordArr.resize(0);
			normalArr.resize(0);
			for (j=0,cos_sin_theta_p=cos_sin_theta;j<=slices;++j,cos_sin_theta_p+=2)
			{
				x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
				z = cos_sin_rho_p[2];
				normalArr << x*nsign <<  y*nsign << z*nsign;
				sSphereMapTexCoordFast(rho + drho, cos_sin_theta_p[0], -cos_sin_theta_p[1], texCoordArr);
				vertexArr << x*radius << y*radius << z*radius;

				x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
				z = cos_sin_rho_p[0];
				normalArr << x*nsign <<  y*nsign << z*nsign;
				sSphereMapTexCoordFast(rho, cos_sin_theta_p[0], -cos_sin_theta_p[1], texCoordArr);
				vertexArr << x*radius << y*radius << z*radius;
			}
			drawArrays(GL_TRIANGLE_STRIP, vertexArr.size()/3, (Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData(), NULL, (Vec3f*)normalArr.constData());
		}
	}
}

void StelPainter::drawTextGravity180(float x, float y, const QString& ws, float xshift, float yshift) const
{
	float dx, dy, d, theta, psi;
	dx = x - prj->viewportCenter[0];
	dy = y - prj->viewportCenter[1];
	d = std::sqrt(dx*dx + dy*dy);

	// If the text is too far away to be visible in the screen return
	if (d>qMax(prj->viewportXywh[3], prj->viewportXywh[2])*2)
		return;
	theta = M_PI + std::atan2(dx, dy - 1);
	psi = std::atan2((float)qPainter->fontMetrics().width(ws)/ws.length(),d + 1) * 180./M_PI;
	if (psi>5)
		psi = 5;

	qPainter->translate(x, y);
	if (prj->gravityLabels)
		qPainter->rotate(-theta*180./M_PI);
	qPainter->translate(xshift, yshift);
	qPainter->scale(1, -1);
	for (int i=0;i<ws.length();++i)
	{
		qPainter->drawText(0,0,ws[i]);

		// with typeface need to manually advance
		// TODO, absolute rotation would be better than relative
		// TODO: would look better with kerning information...
		qPainter->translate((float)qPainter->fontMetrics().width(ws.mid(i,1)) * 1.05, 0);
		qPainter->rotate(-psi);
	}
}

/*************************************************************************
 Draw the string at the given position and angle with the given font
*************************************************************************/
void StelPainter::drawText(float x, float y, const QString& str, float angleDeg, float xshift, float yshift, bool noGravity) const
{
	if (!qPainter)
		return;
	Q_ASSERT(qPainter);

	// Save openGL state
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	qPainter->save();

	qPainter->setRenderHints(QPainter::TextAntialiasing/* | QPainter::HighQualityAntialiasing*/);
	float color[4];
	glGetFloatv(GL_CURRENT_COLOR, color);
	const QColor qCol=QColor::fromRgbF(qMin(1.f,color[0]), qMin(1.f,color[1]), qMin(1.f,color[2]), qMin(1.f,color[3]));
	qPainter->setPen(qCol);

	if (prj->gravityLabels && !noGravity)
	{
		drawTextGravity180(x, y, str, xshift, yshift);
	}
	else
	{
		qPainter->translate(x, y);
		qPainter->translate(xshift, yshift);
		qPainter->scale(1, -1);
		qPainter->drawText(0, 0, str);
	}

	qPainter->restore();

	glPopClientAttrib();
	glPopAttrib();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

#if 0
	glPushMatrix();
	glTranslatef(x,y,0);
	glRotatef(angleDeg,0,0,1);
	glTranslatef(0,font->getLineHeight(),0);
	font->print(xshift, yshift, str);
	glPopMatrix();
#endif
}


/*************************************************************************
 Draw a gl array with 3D vertex position and optional 2D texture position.
*************************************************************************/
void StelPainter::drawArrays(GLenum mode, GLsizei count, Vec3d* vertice, const Vec2f* texCoords, const Vec3f* colorArray, const Vec3f* normalArray) const
{
	Q_ASSERT(vertice);
	// Project all the vertice
	for (int i=0;i<count;++i)
		prj->projectInPlace(vertice[i]);
	if (texCoords==NULL && colorArray==NULL && normalArray==NULL)
	{
		glInterleavedArrays(GL_V3F ,0, vertice);
		glDrawArrays(mode, 0, count);
		return;
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_DOUBLE, 0, vertice);

	if (texCoords)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	}
	if (colorArray)
	{
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT, 0, colorArray);
	}
	if (normalArray)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, normalArray);
	}
	glDrawArrays(mode, 0, count);
	glDisableClientState(GL_VERTEX_ARRAY);
	if (texCoords)
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (colorArray)
		glDisableClientState(GL_COLOR_ARRAY);
	if (normalArray)
		glDisableClientState(GL_NORMAL_ARRAY);
}


// Recursive method cutting a small circle in small segments
inline void fIter(const StelProjectorP& prj, const Vec3d& p1, const Vec3d& p2, Vec3d& win1, Vec3d& win2, QLinkedList<Vec3d>& vertexList, const QLinkedList<Vec3d>::iterator& iter, double radius, const Vec3d& center, int nbI=0, bool checkCrossDiscontinuity=true)
{
	const bool crossDiscontinuity = checkCrossDiscontinuity && prj->intersectViewportDiscontinuity(p1+center, p2+center);
	if (crossDiscontinuity && nbI>=10)
	{
		win1[2]=-2.;
		win2[2]=-2.;
		vertexList.insert(iter, win1);
		vertexList.insert(iter, win2);
		return;
	}

	Vec3d newVertex(p1); newVertex+=p2;
	newVertex.normalize();
	newVertex*=radius;
	Vec3d win3(newVertex[0]+center[0], newVertex[1]+center[1], newVertex[2]+center[2]);
	const bool isValidVertex = prj->projectInPlace(win3);

	const double v10=win1[0]-win3[0];
	const double v11=win1[1]-win3[1];
	const double v20=win2[0]-win3[0];
	const double v21=win2[1]-win3[1];

	const double dist = std::sqrt((v10*v10+v11*v11)*(v20*v20+v21*v21));
	const double cosAngle = (v10*v20+v11*v21)/dist;
	if ((cosAngle>-0.999 || dist>50*50 || crossDiscontinuity) && nbI<10)
	{
		// Use the 3rd component of the vector to store whether the vertex is valid
		win3[2]= isValidVertex ? 1.0 : -1.;
		fIter(prj, p1, newVertex, win1, win3, vertexList, vertexList.insert(iter, win3), radius, center, nbI+1, crossDiscontinuity || dist>50*50);
		fIter(prj, newVertex, p2, win3, win2, vertexList, iter, radius, center, nbI+1, crossDiscontinuity || dist>50*50 );
	}
}

// Used by the method below
static QVector<Vec3d> smallCircleVertexArray;

void drawSmallCircleVertexArray()
{
	if (smallCircleVertexArray.isEmpty())
		return;

	Q_ASSERT(smallCircleVertexArray.size()>1);

	glEnableClientState(GL_VERTEX_ARRAY);
	// Load the vertex array
	glVertexPointer(3, GL_DOUBLE, 0, smallCircleVertexArray.constData());
	// And draw everything at once
	glDrawArrays(GL_LINE_STRIP, 0, smallCircleVertexArray.size());
	glDisableClientState(GL_VERTEX_ARRAY);
	smallCircleVertexArray.resize(0);
}

/*************************************************************************
 Draw a small circle arc in the current frame
*************************************************************************/
void StelPainter::drawSmallCircleArc(const Vec3d& start, const Vec3d& stop, const Vec3d& rotCenter, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, const void* userData), const void* userData) const
{
	Q_ASSERT(smallCircleVertexArray.empty());

	QLinkedList<Vec3d> tessArc;	// Contains the list of projected points from the tesselated arc
	Vec3d win1, win2;
	win1[2] = prj->project(start, win1) ? 1.0 : -1.;
	win2[2] = prj->project(stop, win2) ? 1.0 : -1.;
	tessArc.append(win1);

	double radius;
	if (rotCenter==Vec3d(0,0,0))
	{
		// Great circle
		radius=1;
	}
	else
	{
		Vec3d tmp = (rotCenter^start)/rotCenter.length();
		radius = fabs(tmp.length());
	}

	// Perform the tesselation of the arc in small segments in a way so that the lines look smooth
	fIter(prj, start-rotCenter, stop-rotCenter, win1, win2, tessArc, tessArc.insert(tessArc.end(), win2), radius, rotCenter);

	// And draw.
	QLinkedList<Vec3d>::ConstIterator i = tessArc.begin();
	while (i+1 != tessArc.end())
	{
		const Vec3d& p1 = *i;
		const Vec3d& p2 = *(++i);
		const bool p1InViewport = prj->checkInViewport(p1);
		const bool p2InViewport = prj->checkInViewport(p2);
		if ((p1[2]>0 && p1InViewport) || (p2[2]>0 && p2InViewport))
		{
			smallCircleVertexArray.append(p1);
			if (i+1==tessArc.end())
			{
				smallCircleVertexArray.append(p2);
				drawSmallCircleVertexArray();
			}
			if (viewportEdgeIntersectCallback && p1InViewport!=p2InViewport)
			{
				// We crossed the edge of the view port
				if (p1InViewport)
					viewportEdgeIntersectCallback(prj->viewPortIntersect(p1, p2), p2-p1, userData);
				else
					viewportEdgeIntersectCallback(prj->viewPortIntersect(p2, p1), p1-p2, userData);
			}
		}
		else
		{
			// Break the line, draw the stored vertex and flush the list
			if (!smallCircleVertexArray.isEmpty())
				smallCircleVertexArray.append(p1);
			drawSmallCircleVertexArray();
		}
	}
	Q_ASSERT(smallCircleVertexArray.isEmpty());
}

// Project the passed triangle on the screen ensuring that it will look smooth, even for non linear distortion
// by splitting it into subtriangles.
void StelPainter::projectSphericalTriangle(const Vec3d* vertices, QVarLengthArray<Vec3d, 4096>* outVertices,
		const bool* edgeFlags, QVarLengthArray<bool, 4096>* outEdgeFlags,
		const Vec2f* texturePos, QVarLengthArray<Vec2f, 4096>* outTexturePos,
		int nbI, bool checkDisc1, bool checkDisc2, bool checkDisc3) const
{
	Q_ASSERT(fabs(vertices[0].length()-1.)<0.00001);
	Q_ASSERT(fabs(vertices[1].length()-1.)<0.00001);
	Q_ASSERT(fabs(vertices[2].length()-1.)<0.00001);
	bool cDiscontinuity1 = checkDisc1 && prj->intersectViewportDiscontinuity(vertices[0], vertices[1]);
	bool cDiscontinuity2 = checkDisc2 && prj->intersectViewportDiscontinuity(vertices[1], vertices[2]);
	bool cDiscontinuity3 = checkDisc3 && prj->intersectViewportDiscontinuity(vertices[0], vertices[2]);
	const bool cd1=cDiscontinuity1;
	const bool cd2=cDiscontinuity2;
	const bool cd3=cDiscontinuity3;

	Vec3d e0=vertices[0];
	Vec3d e1=vertices[1];
	Vec3d e2=vertices[2];
	bool valid = prj->projectInPlace(e0);
	valid = prj->projectInPlace(e1) || valid;
	valid = prj->projectInPlace(e2) || valid;
	// Clip polygons behind the viewer
	if (!valid)
		return;

	static const double maxSqDistortion = 5.;
	if (checkDisc1 && cDiscontinuity1==false)
	{
		// If the distortion at segment e0,e1 is too big, flags it for subdivision
		Vec3d win3 = vertices[0]; win3+=vertices[1];
		prj->projectInPlace(win3);
		win3[0]-=(e0[0]+e1[0])*0.5; win3[1]-=(e0[1]+e1[1])*0.5;
		cDiscontinuity1 = (win3[0]*win3[0]+win3[1]*win3[1])>maxSqDistortion;
	}
	if (checkDisc2 && cDiscontinuity2==false)
	{
		// If the distortion at segment e1,e2 is too big, flags it for subdivision
		Vec3d win3 = vertices[1]; win3+=vertices[2];
		prj->projectInPlace(win3);
		win3[0]-=(e2[0]+e1[0])*0.5; win3[1]-=(e2[1]+e1[1])*0.5;
		cDiscontinuity2 = (win3[0]*win3[0]+win3[1]*win3[1])>maxSqDistortion;
	}
	if (checkDisc3 && cDiscontinuity3==false)
	{
		// If the distortion at segment e2,e0 is too big, flags it for subdivision
		Vec3d win3 = vertices[2]; win3+=vertices[0];
		prj->projectInPlace(win3);
		win3[0] -= (e0[0]+e2[0])*0.5;
		win3[1] -= (e0[1]+e2[1])*0.5;
		cDiscontinuity3 = (win3[0]*win3[0]+win3[1]*win3[1])>maxSqDistortion;
	}

	if (!cDiscontinuity1 && !cDiscontinuity2 && !cDiscontinuity3)
	{
		// The triangle is clean, appends it
		outVertices->append(e0); outVertices->append(e1); outVertices->append(e2);
		if (outEdgeFlags)
			outEdgeFlags->append(edgeFlags, 3);
		if (outTexturePos)
			outTexturePos->append(texturePos,3);
		return;
	}

	if (nbI > 4)
	{
		// If we reached the limit number of iterations and still have a discontinuity,
		// discards the triangle.
		if (cd1 || cd2 || cd3)
			return;

		// Else display it, it will be suboptimal though.
		outVertices->append(e0); outVertices->append(e1); outVertices->append(e2);
		if (outEdgeFlags)
			outEdgeFlags->append(edgeFlags, 3);
		if (outTexturePos)
			outTexturePos->append(texturePos,3);
		return;
	}

	// Recursively splits the triangle into sub triangles.
	// Depending on which combination of sides of the triangle has to be split a different strategy is used.
	Vec3d va[3];
	Vec2f ta[3];
	bool ba[3];
	// Only 1 side has to be split: split the triangle in 2
	if (cDiscontinuity1 && !cDiscontinuity2 && !cDiscontinuity3)
	{
		va[0]=vertices[0];
		va[1]=vertices[0];va[1]+=vertices[1];
		va[1].normalize();
		va[2]=vertices[2];
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[0]+texturePos[1])*0.5;
			ta[2]=texturePos[2];
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=false;
			ba[2]=edgeFlags[2];
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, true, true, false);

		//va[0]=vertices[0]+vertices[1];
		//va[0].normalize();
		va[0]=va[1];
		va[1]=vertices[1];
		va[2]=vertices[2];
		if (outTexturePos)
		{
			ta[0]=(texturePos[0]+texturePos[1])*0.5;
			ta[1]=texturePos[1];
			ta[2]=texturePos[2];
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=edgeFlags[1];
			ba[2]=false;
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, true, false, true);
		return;
	}

	if (!cDiscontinuity1 && cDiscontinuity2 && !cDiscontinuity3)
	{
		va[0]=vertices[0];
		va[1]=vertices[1];
		va[2]=vertices[1];va[2]+=vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=texturePos[1];
			ta[2]=(texturePos[1]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=edgeFlags[1];
			ba[2]=false;
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, false, true, true);

		va[0]=vertices[0];
		//va[1]=vertices[1]+vertices[2];
		//va[1].normalize();
		va[1]=va[2];
		va[2]=vertices[2];
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[1]+texturePos[2])*0.5;
			ta[2]=texturePos[2];
		}
		if (outEdgeFlags)
		{
			ba[0]=false;
			ba[1]=edgeFlags[1];
			ba[2]=edgeFlags[2];
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, true, true, false);
		return;
	}

	if (!cDiscontinuity1 && !cDiscontinuity2 && cDiscontinuity3)
	{
		va[0]=vertices[0];
		va[1]=vertices[1];
		va[2]=vertices[0];va[2]+=vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=texturePos[1];
			ta[2]=(texturePos[0]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=false;
			ba[2]=edgeFlags[2];
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, false, true, true);

		//va[0]=vertices[0]+vertices[2];
		//va[0].normalize();
		va[0]=va[2];
		va[1]=vertices[1];
		va[2]=vertices[2];
		if (outTexturePos)
		{
			ta[0]=(texturePos[0]+texturePos[2])*0.5;
			ta[1]=texturePos[1];
			ta[2]=texturePos[2];
		}
		if (outEdgeFlags)
		{
			ba[0]=false;
			ba[1]=edgeFlags[1];
			ba[2]=edgeFlags[2];
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, true, false, true);
		return;
	}

	// 2 sides have to be split: split the triangle in 3
	if (cDiscontinuity1 && cDiscontinuity2 && !cDiscontinuity3)
	{
		va[0]=vertices[0];
		va[1]=vertices[0];va[1]+=vertices[1];
		va[1].normalize();
		va[2]=vertices[1];va[2]+=vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[0]+texturePos[1])*0.5;
			ta[2]=(texturePos[1]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=false;
			ba[2]=false;
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);

		//va[0]=vertices[0]+vertices[1];
		//va[0].normalize();
		va[0]=va[1];
		va[1]=vertices[1];
		//va[2]=vertices[1]+vertices[2];
		//va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=(texturePos[0]+texturePos[1])*0.5;
			ta[1]=texturePos[1];
			ta[2]=(texturePos[1]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=edgeFlags[1];
			ba[2]=false;
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);

		va[0]=vertices[0];
		//va[1]=vertices[1]+vertices[2];
		//va[1].normalize();
		va[1]=va[2];
		va[2]=vertices[2];
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[1]+texturePos[2])*0.5;
			ta[2]=texturePos[2];
		}
		if (outEdgeFlags)
		{
			ba[0]=false;
			ba[1]=edgeFlags[1];
			ba[2]=edgeFlags[2];
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, true, true, false);
		return;
	}
	if (cDiscontinuity1 && !cDiscontinuity2 && cDiscontinuity3)
	{
		va[0]=vertices[0];
		va[1]=vertices[0];va[1]+=vertices[1];
		va[1].normalize();
		va[2]=vertices[0];va[2]+=vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[0]+texturePos[1])*0.5;
			ta[2]=(texturePos[0]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=false;
			ba[2]=edgeFlags[2];
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);

		//va[0]=vertices[0]+vertices[1];
		//va[0].normalize();
		va[0]=va[1];
		va[1]=vertices[2];
		//va[2]=vertices[0]+vertices[2];
		//va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=(texturePos[0]+texturePos[1])*0.5;
			ta[1]=texturePos[2];
			ta[2]=(texturePos[0]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=false;
			ba[1]=edgeFlags[2];
			ba[2]=false;
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);


		//va[0]=vertices[0]+vertices[1];
		//va[0].normalize();
		va[1]=vertices[1];
		va[2]=vertices[2];
		if (outTexturePos)
		{
			ta[0]=(texturePos[0]+texturePos[1])*0.5;
			ta[1]=texturePos[1];
			ta[2]=texturePos[2];
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=edgeFlags[1];
			ba[2]=false;
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, true, false, true);

		return;
	}
	if (!cDiscontinuity1 && cDiscontinuity2 && cDiscontinuity3)
	{
		va[0]=vertices[0];
		va[1]=vertices[1];
		va[2]=vertices[1];va[2]+=vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=texturePos[1];
			ta[2]=(texturePos[1]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[0];
			ba[1]=edgeFlags[1];
			ba[2]=false;
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1, false, true, true);

		//va[0]=vertices[1]+vertices[2];
		//va[0].normalize();
		va[0]=va[2];
		va[1]=vertices[2];
		va[2]=vertices[0];va[2]+=vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=(texturePos[1]+texturePos[2])*0.5;
			ta[1]=texturePos[2];
			ta[2]=(texturePos[0]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=edgeFlags[1];
			ba[1]=edgeFlags[2];
			ba[2]=false;
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);

		va[1]=va[0];
		va[0]=vertices[0];
		//va[1]=vertices[1]+vertices[2];
		//va[1].normalize();
		//va[2]=vertices[0]+vertices[2];
		//va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[1]+texturePos[2])*0.5;
			ta[2]=(texturePos[0]+texturePos[2])*0.5;
		}
		if (outEdgeFlags)
		{
			ba[0]=false;
			ba[1]=false;
			ba[2]=edgeFlags[2];
		}
		projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);
		return;
	}

	// Last case: the 3 sides have to be split: cut in 4 triangles a' la HTM
	va[0]=vertices[0];va[0]+=vertices[1];
	va[0].normalize();
	va[1]=vertices[1];va[1]+=vertices[2];
	va[1].normalize();
	va[2]=vertices[0];va[2]+=vertices[2];
	va[2].normalize();
	if (outTexturePos)
	{
		ta[0]=(texturePos[0]+texturePos[1])*0.5;
		ta[1]=(texturePos[1]+texturePos[2])*0.5;
		ta[2]=(texturePos[0]+texturePos[2])*0.5;
	}
	if (outEdgeFlags)
	{
		ba[0]=false;
		ba[1]=false;
		ba[2]=false;
	}
	projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);

	va[1]=va[0];
	va[0]=vertices[0];
	//va[1]=vertices[0]+vertices[1];
	//va[1].normalize();
	//va[2]=vertices[0]+vertices[2];
	//va[2].normalize();
	if (outTexturePos)
	{
		ta[0]=texturePos[0];
		ta[1]=(texturePos[0]+texturePos[1])*0.5;
		ta[2]=(texturePos[0]+texturePos[2])*0.5;
	}
	if (outEdgeFlags)
	{
		ba[0]=edgeFlags[0];
		ba[1]=false;
		ba[2]=edgeFlags[2];
	}
	projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);

	//va[0]=vertices[0]+vertices[1];
	//va[0].normalize();
	va[0]=va[1];
	va[1]=vertices[1];
	va[2]=vertices[1];va[2]+=vertices[2];
	va[2].normalize();
	if (outTexturePos)
	{
		ta[0]=(texturePos[0]+texturePos[1])*0.5;
		ta[1]=texturePos[1];
		ta[2]=(texturePos[1]+texturePos[2])*0.5;
	}
	if (outEdgeFlags)
	{
		ba[0]=edgeFlags[0];
		ba[1]=edgeFlags[1];
		ba[2]=false;
	}
	projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);

	va[0]=vertices[0];va[0]+=vertices[2];
	va[0].normalize();
	//va[1]=vertices[1]+vertices[2];
	//va[1].normalize();
	va[1]=va[2];
	va[2]=vertices[2];
	if (outTexturePos)
	{
		ta[0]=(texturePos[0]+texturePos[2])*0.5;
		ta[1]=(texturePos[1]+texturePos[2])*0.5;
		ta[2]=texturePos[2];
	}
	if (outEdgeFlags)
	{
		ba[0]=false;
		ba[1]=edgeFlags[1];
		ba[2]=edgeFlags[2];
	}
	projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);

	return;
}

static QVarLengthArray<Vec3d, 4096> polygonVertexArray;
static QVarLengthArray<bool, 4096> polygonEdgeFlagArray;
static QVarLengthArray<Vec2f, 4096> polygonTextureCoordArray;

// Draw the given SphericalPolygon.
void StelPainter::drawSphericalPolygon(const SphericalPolygonBase* poly, SphericalPolygonDrawMode drawMode, const Vec4f* boundaryColor) const
{
	polygonVertexArray.clear();
	polygonEdgeFlagArray.clear();
	polygonTextureCoordArray.clear();

	// Use a special algo for convex polygons which are not pre-tesselated.
	const SphericalConvexPolygon* cvx = dynamic_cast<const SphericalConvexPolygon*>(poly);
	if (cvx!=NULL)
	{
		// Tesselate the convex polygon into a triangle fan.
		const QVector<Vec3d>& a = cvx->getConvexContour();
		Vec3d triangle[3];
		bool tmpEdges[3] = {true, true, false};

		if (drawMode==SphericalPolygonDrawModeTextureFillAndBoundary || drawMode==SphericalPolygonDrawModeTextureFill)
		{
			Q_ASSERT(dynamic_cast<const SphericalTexturedConvexPolygon*>(cvx)!=NULL);
			// Need to compute textures coordinates
			Vec2f texCoord[3];
			const QVector<Vec2f>& tex = cvx->getTextureCoordArray();
			triangle[0]=a.at(0);
			triangle[1]=a.at(1);
			triangle[2]=a.at(2);
			texCoord[0]=tex.at(0);
			texCoord[1]=tex.at(1);
			texCoord[2]=tex.at(2);
			projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray, texCoord, &polygonTextureCoordArray);
			tmpEdges[0]=false;
			for (int i=2;i<a.size()-2;++i)
			{
				triangle[1]=a.at(i);
				triangle[2]=a.at(i+1);
				texCoord[1]=tex.at(i);
				texCoord[2]=tex.at(i+1);
				projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray, texCoord, &polygonTextureCoordArray);
			}
			tmpEdges[2]=true;
			// Last triangle
			triangle[1]=a.at(a.size()-2);
			triangle[2]=a.last();
			texCoord[1]=tex.at(a.size()-2);
			texCoord[2]=tex.last();
			projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray, texCoord, &polygonTextureCoordArray);
		}
		else
		{
			// No need for textures coordinates
			triangle[0]=a.at(0);
			triangle[1]=a.at(1);
			triangle[2]=a.at(2);
			if (a.size()==3)
			{
				tmpEdges[2]=true;
				projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray);
			}
			else
			{
				projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray);
				tmpEdges[0]=false;
				for (int i=2;i<a.size()-2;++i)
				{
					triangle[1]=a.at(i);
					triangle[2]=a.at(i+1);
					projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray);
				}
				tmpEdges[2]=true;
				// Last triangle
				triangle[1]=a.at(a.size()-2);
				triangle[2]=a.last();
				projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray);
			}
		}
	}
	else
	{
		// Normal faster behaviour for already tesselated polygon
		const QVector<Vec3d>& a = poly->getVertexArray();
		if (a.isEmpty())
			return;
		if (drawMode==SphericalPolygonDrawModeTextureFill)
		{
			// Only texured fill, don't bother with edge flags
			for (int i=0;i<a.size()/3;++i)
				projectSphericalTriangle(a.constData()+i*3, &polygonVertexArray, NULL, NULL, poly->getTextureCoordArray().constData()+i*3, &polygonTextureCoordArray);
			Q_ASSERT(polygonVertexArray.size()==polygonTextureCoordArray.size());
		}
		else if (drawMode==SphericalPolygonDrawModeTextureFillAndBoundary)
		{
			// Texured fill and edge flags
			for (int i=0;i<a.size()/3;++i)
				projectSphericalTriangle(a.constData()+i*3, &polygonVertexArray, poly->getEdgeFlagArray().constData()+i*3, &polygonEdgeFlagArray, poly->getTextureCoordArray().constData()+i*3, &polygonTextureCoordArray);
			Q_ASSERT(polygonVertexArray.size()==polygonTextureCoordArray.size());
		}
		else if (drawMode==SphericalPolygonDrawModeBoundary || drawMode==SphericalPolygonDrawModeFillAndBoundary)
		{
			for (int i=0;i<a.size()/3;++i)
				projectSphericalTriangle(a.constData()+i*3, &polygonVertexArray, poly->getEdgeFlagArray().constData()+i*3, &polygonEdgeFlagArray, NULL, NULL);
		}
		else
		{
			// Only plain fill, don't bother with edge flags and textures coordinates
			for (int i=0;i<a.size()/3;++i)
				projectSphericalTriangle(a.constData()+i*3, &polygonVertexArray, NULL, NULL, NULL, NULL);
		}
	}

	// Load the vertex array
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_DOUBLE, 0, polygonVertexArray.constData());
	glEnable(GL_CULL_FACE);
	// Load the textureCoordinates if any
	if (drawMode==SphericalPolygonDrawModeTextureFill || drawMode==SphericalPolygonDrawModeTextureFillAndBoundary)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, polygonTextureCoordArray.constData());
	}
	// Draw the fill part
	if (drawMode!=SphericalPolygonDrawModeBoundary)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLES, 0, polygonVertexArray.size());
	}
	// Draw the boundary part
	if (drawMode==SphericalPolygonDrawModeBoundary || drawMode==SphericalPolygonDrawModeFillAndBoundary || drawMode==SphericalPolygonDrawModeTextureFillAndBoundary)
	{
		// Load the edge flags
		glEdgeFlagPointer(0, polygonEdgeFlagArray.constData());
		glEnableClientState(GL_EDGE_FLAG_ARRAY);

		// Draw the boundary part, and use the extra color if defined
		GLfloat tmpColor[4];
		if (boundaryColor!=NULL)
		{
			glGetFloatv(GL_CURRENT_COLOR, tmpColor);
			glColor4fv((float*)boundaryColor);
		}
		if (drawMode==SphericalPolygonDrawModeTextureFillAndBoundary)
		{
			glDisable(GL_TEXTURE_2D);
		}
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, polygonVertexArray.size());
		if (boundaryColor!=NULL)
		{
			// Revert previous color
			glColor4fv(tmpColor);
		}
		if (drawMode==SphericalPolygonDrawModeTextureFillAndBoundary)
		{
			glEnable(GL_TEXTURE_2D);
		}
	}

	glDisable(GL_CULL_FACE);
	glDisableClientState(GL_EDGE_FLAG_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void StelPainter::drawSphericalRegion(const SphericalRegion* region, SphericalPolygonDrawMode drawMode, const Vec4f* boundaryColor) const
{
	const SphericalPolygonBase* poly = dynamic_cast<const SphericalPolygonBase*>(region);
	if (poly)
	{
		return drawSphericalPolygon(poly, drawMode, boundaryColor);
	}
	else
	{
		SphericalPolygon pol = region->toSphericalPolygon();
		drawSphericalPolygon(&pol, drawMode, boundaryColor);
	}
}

/*************************************************************************
 draw a simple circle, 2d viewport coordinates in pixel
*************************************************************************/
void StelPainter::drawCircle(double x,double y,double r) const
{
	if (r <= 1.0)
		return;
	const Vec2d center(x,y);
	const Vec2d v_center(0.5*prj->viewportXywh[2],0.5*prj->viewportXywh[3]);
	const double R = v_center.length();
	const double d = (v_center-center).length();
	if (d > r+R || d < r-R)
		return;
	const int segments = 180;
	const double phi = 2.0*M_PI/segments;
	const double cp = cos(phi);
	const double sp = sin(phi);
	double dx = r;
	double dy = 0;
	static QVarLengthArray<Vec3d, 180> circleVertexArray(180);

	for (int i=0;i<segments;i++)
	{
		circleVertexArray[i].set(x+dx,y+dy,0);
		r = dx*cp-dy*sp;
		dy = dx*sp+dy*cp;
		dx = r;
	}
	//drawArrays(GL_LINE_LOOP, 180, circleVertexArray.data());//drawArrays() calls prj->projectInPlace() for each vertex
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_DOUBLE, 0, circleVertexArray.data());
	glDrawArrays(GL_LINE_LOOP, 0, 180);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void StelPainter::drawSprite2dMode(double x, double y, float radius) const
{
	// Use GL_POINT_SPRITE_ARB extension if available
	if (flagGlPointSprite)
	{
		glPointSize(radius*2.);
		static float vertexData[] = {0.,0.};
		vertexData[0]=x;
		vertexData[1]=y;
		glInterleavedArrays(GL_V2F ,0, vertexData);
		glDrawArrays(GL_POINTS, 0, 1);
		return;
	}

	static float vertexData[] = {0.,0.,-10.,-10.,0.,   1.,0.,10.,-10.,0.,  0.,1.,10.,10.,0,   1.,1.,-10.,10.,0.};
	vertexData[2]=x-radius; vertexData[3]=y-radius;
	vertexData[7]=x+radius; vertexData[8]=y-radius;
	vertexData[12]=x-radius; vertexData[13]=y+radius;
	vertexData[17]=x+radius; vertexData[18]=y+radius;
	glInterleavedArrays(GL_T2F_V3F ,0, vertexData);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void StelPainter::drawRect2d(float x, float y, float width, float height) const
{
	static float vertexData[] = {0.,0.,-10.,-10.,0.,   1.,0.,10.,-10.,0.,  0.,1.,10.,10.,0,   1.,1.,-10.,10.,0.};
	vertexData[2]=x; vertexData[3]=y;
	vertexData[7]=x+width; vertexData[8]=y;
	vertexData[12]=x; vertexData[13]=y+height;
	vertexData[17]=x+width; vertexData[18]=y+height;
	glInterleavedArrays(GL_T2F_V3F ,0, vertexData);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/*************************************************************************
 Same function but with a rotation angle
*************************************************************************/
void StelPainter::drawSprite2dMode(double x, double y, float radius, float rotation) const
{
	glPushMatrix();
	glTranslatef(x, y, 0.0);
	glRotatef(rotation,0.,0.,1.);

	static float vertexData[] = {0.,0.,-10.,-10.,0.,   1.,0.,10.,-10.,0.,  0.,1.,10.,10.,0,   1.,1.,-10.,10.,0.};
	vertexData[2]=-radius; vertexData[3]=-radius;
	vertexData[7]=radius; vertexData[8]=-radius;
	vertexData[12]=-radius; vertexData[13]=radius;
	vertexData[17]=radius; vertexData[18]=radius;
	glInterleavedArrays(GL_T2F_V3F ,0, vertexData);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glPopMatrix();
}

/*************************************************************************
 Draw a GL_POINT at the given position
*************************************************************************/
void StelPainter::drawPoint2d(double x, double y) const
{
	static float vertexData[] = {0.,0.};
	vertexData[0]=x;
	vertexData[1]=y;
	glInterleavedArrays(GL_V2F ,0, vertexData);
	if (flagGlPointSprite)
	{
		glDisable(GL_POINT_SPRITE_ARB);
		glDrawArrays(GL_POINTS, 0, 1);
		glEnable(GL_POINT_SPRITE_ARB);
		return;
	}
	glDrawArrays(GL_POINTS, 0, 1);
}


/*************************************************************************
 Draw a line between the 2 points.
*************************************************************************/
void StelPainter::drawLine2d(double x1, double y1, double x2, double y2) const
{
	static float vertexData[] = {0.,0.,0.,0.};
	vertexData[0]=x1;
	vertexData[1]=y1;
	vertexData[2]=x2;
	vertexData[3]=y2;
	glInterleavedArrays(GL_V2F ,0, vertexData);
	glDrawArrays(GL_LINES, 0, 2);
}

///////////////////////////////////////////////////////////////////////////
// Drawing methods for general (non-linear) mode

void StelPainter::sSphere(GLdouble radius, GLdouble oneMinusOblateness, GLint slices, GLint stacks, int orientInside) const
{
	// It is really good for performance to have Vec4f,Vec3f objects
	// static rather than on the stack. But why?
	// Is the constructor/destructor so expensive?
	static Vec3f lightPos3;
	GLboolean isLightOn;
	float c;

	static Vec4f ambientLight;
	static Vec4f diffuseLight;

	glGetBooleanv(GL_LIGHTING, &isLightOn);

	if (isLightOn)
	{
		glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos3);
		lightPos3 -= prj->modelViewMatrix * Vec3d(0.,0.,0.); // -posCenterEye
		lightPos3 = prj->modelViewMatrix.transpose().multiplyWithoutTranslation(lightPos3);
		lightPos3.normalize();
		glGetLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
		glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
		glDisable(GL_LIGHTING);
	}

	GLfloat x, y, z;
	GLfloat s, t;
	GLint i, j;
	GLfloat nsign;

	if (orientInside)
	{
		nsign = -1.0;
		t=0.0; // from inside texture is reversed
	}
	else
	{
		nsign = 1.0;
		t=1.0;
	}

	const double drho = M_PI / stacks;
	Q_ASSERT(stacks<=MAX_STACKS);
	ComputeCosSinRho(drho,stacks);
	double *cos_sin_rho_p;

	const double dtheta = 2.0 * M_PI / slices;
	Q_ASSERT(slices<=MAX_SLICES);
	ComputeCosSinTheta(dtheta,slices);
	const double *cos_sin_theta_p;

	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
	// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
	const GLfloat ds = 1.0 / slices;
	const GLfloat dt = nsign / stacks; // from inside texture is reversed

	// draw intermediate  as quad strips
	static QVector<double> vertexArr;
	static QVector<float> texCoordArr;
	static QVector<float> colorArr;
	for (i = 0,cos_sin_rho_p = cos_sin_rho; i < stacks; ++i,cos_sin_rho_p+=2)
	{
		texCoordArr.resize(0);
		vertexArr.resize(0);
		colorArr.resize(0);
		s = 0.0;
		for (j = 0,cos_sin_theta_p = cos_sin_theta; j <= slices;++j,cos_sin_theta_p+=2)
		{
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
			z = nsign * cos_sin_rho_p[0];
			texCoordArr << s << t;
			if (isLightOn)
			{
				c = nsign * (lightPos3[0]*x*oneMinusOblateness + lightPos3[1]*y*oneMinusOblateness + lightPos3[2]*z);
				if (c<0) {c=0;}
				colorArr << c*diffuseLight[0] + ambientLight[0] << c*diffuseLight[1] + ambientLight[1] << c*diffuseLight[2] + ambientLight[2];
			}
			vertexArr << x * radius << y * radius << z * oneMinusOblateness * radius;
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
			z = nsign * cos_sin_rho_p[2];
			texCoordArr << s << t - dt;
			if (isLightOn)
			{
				c = nsign * (lightPos3[0]*x*oneMinusOblateness + lightPos3[1]*y*oneMinusOblateness + lightPos3[2]*z);
				if (c<0) {c=0;}
				colorArr << c*diffuseLight[0] + ambientLight[0] << c*diffuseLight[1] + ambientLight[1] << c*diffuseLight[2] + ambientLight[2];
			}
			vertexArr << x * radius << y * radius << z * oneMinusOblateness * radius;
			s += ds;
		}
		// Draw the array now
		if (isLightOn)
			drawArrays(GL_TRIANGLE_STRIP, vertexArr.size()/3, (Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData(), (Vec3f*)colorArr.constData());
		else
			drawArrays(GL_TRIANGLE_STRIP, vertexArr.size()/3, (Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData());
		t -= dt;
	}

	if (isLightOn)
		glEnable(GL_LIGHTING);
}

// Reimplementation of gluCylinder : glu is overrided for non standard projection
void StelPainter::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orientInside) const
{
	GLdouble da, r, dz;
	GLfloat z, nsign;
	GLint i, j;

	nsign = 1.0;
	if (orientInside)
		glCullFace(GL_FRONT);
	//nsign = -1.0;
	//else nsign = 1.0;

	da = 2.0 * M_PI / slices;
	dz = height / stacks;

	GLfloat ds = 1.0 / slices;
	GLfloat dt = 1.0 / stacks;
	GLfloat t = 0.0;
	z = 0.0;
	r = radius;
	QVector<float> normalArray;
	QVector<float> texCoordArray;
	QVector<double> vertexArray;
	for (j = 0; j < stacks; j++)
	{
		normalArray.resize(0);
		texCoordArray.resize(0);
		vertexArray.resize(0);
		GLfloat s = 0.0;
		for (i = 0; i <= slices; i++)
		{
			GLfloat x, y;
			if (i == slices)
			{
				x = 0.f;
				y = 1.f;
			}
			else
			{
				x = std::sin(i * da);
				y = std::cos(i * da);
			}
			normalArray << x * nsign << y * nsign << 0.f;
			texCoordArray << s << t;
			vertexArray << x*r << y*r << z;
			normalArray << x * nsign << y * nsign << 0.f;
			texCoordArray << s << t + dt;
			vertexArray << x*r << y*r << z+dz;
			s += ds;
		} // for slices

		drawArrays(GL_TRIANGLE_STRIP, vertexArray.size()/3, (Vec3d*)vertexArray.constData(), (Vec2f*)texCoordArray.constData(), NULL,  (Vec3f*)normalArray.constData());
		t += dt;
		z += dz;
	}				/* for stacks */

	if (orientInside)
		glCullFace(GL_BACK);
}
