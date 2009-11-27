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

#include <QtOpenGL>

#include "StelProjector.hpp"
#include "StelProjectorClasses.hpp"
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

#if QT_VERSION<0x040600
#include <QGLContext>
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#ifndef NDEBUG
QMutex* StelPainter::globalMutex = new QMutex();
#endif

QPainter* StelPainter::qPainter = NULL;

//void StelPainter::init()
//{
//#ifdef USE_OPENGL_ES2
//	QGLShader* vShader = new QGLShader(QGLShader::Vertex);
//	vShader->compileSourceCode(
//					"uniform mat4 projectionMatrix;\n"
//					"attribute vec3 vertex;\n"
//					"attribute vec4 color;\n"
//					"varying vec4 outColor;\n"
//					"void main()\n"
//					"{	gl_Position = projectionMatrix*vec4(vertex, 1.);\n"
//					"	outColor = color;}");
//	noTexturesShaderProgram = new QGLShaderProgram();
//	Q_ASSERT(vShader->isCompiled())
//	if (!vShader->log().isEmpty())
//		qWarning() << "Warnings while compiling vertex shader: " << vShader->log();
//	QGLShader* fShader = new QGLShader(QGLShader::Fragment);
//	fShader->compileSourceCode(
//			"varying vec4 outColor;\n"
//			"void main(){gl_FragColor = outColor;}");
//	Q_ASSERT(fShader->isCompiled())
//	if (!fShader->log().isEmpty())
//		qWarning() << "Warnings while compiling fragment shader: " << vShader->log();
//	noTexturesShaderProgram->addShader(vShader);
//	noTexturesShaderProgram->addShader(fShader);
//	noTexturesShaderProgram->link();
//	Q_ASSERT(noTexturesShaderProgram->linked())
//	if (!starsShaderProgram->log().isEmpty())
//		qWarning() << "Warnings while linking shader: " << starsShaderProgram->log();
//#endif
//}

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

	Q_ASSERT(qPainter);

#if QT_VERSION>=0x040600
	qPainter->beginNativePainting();
#else
	// Ensure that the current GL content is the one of our main GL window
	QGLWidget* w = dynamic_cast<QGLWidget*>(qPainter->device());
	if (w!=0)
	{
		Q_ASSERT(w->isValid());
		w->makeCurrent();
	}
	qPainter->save();
#endif

	// Init GL viewport to current projector values
	glViewport(prj->viewportXywh[0], prj->viewportXywh[1], prj->viewportXywh[2], prj->viewportXywh[3]);

#ifndef USE_OPENGL_ES2
	// Save openGL projection state
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	// Set the real openGL projection and modelview matrix to 2d orthographic projection
	// thus we never need to change to 2dMode from now on before drawing
	glMultMatrixf(prj->getProjectionMatrix());
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_MULTISAMPLE);
	glDisable(GL_DITHER);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_TEXTURE_2D);
#endif
	glFrontFace(prj->needGlFrontFaceCW()?GL_CW:GL_CCW);
}

StelPainter::~StelPainter()
{
	Q_ASSERT(qPainter);

#ifndef USE_OPENGL_ES2
	// Restore openGL projection state for Qt drawings
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
	glPopClientAttrib();
#endif
#ifndef NDEBUG
	GLenum er = glGetError();
	if (er!=GL_NO_ERROR)
	{
		if (er==GL_INVALID_OPERATION)
			qFatal("Invalid openGL operation in StelPainter::revertToQtPainting()");
	}
#endif

#if QT_VERSION>=0x040600
	qPainter->endNativePainting();
#else
	qPainter->restore();
#endif

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

void StelPainter::setColor(float r, float g, float b, float a)
{
#ifndef USE_OPENGL_ES2
	glColor4f(r,g,b,a);
#else

#endif
}

QFontMetrics StelPainter::getFontMetrics() const
{
	Q_ASSERT(qPainter);
	return qPainter->fontMetrics();
}

void StelPainter::initSystemGLInfo()
{
}


///////////////////////////////////////////////////////////////////////////
// Standard methods for drawing primitives

// Fill with black around the circle
void StelPainter::drawViewportShape(void)
{
	if (prj->maskType != StelProjector::MaskDisk)
		return;

	glDisable(GL_BLEND);
	setColor(0.f,0.f,0.f);

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
	enableClientStates(true);
	setVertexPointer(3, GL_FLOAT, vertices);

	radiusHigh=outerRadius-deltaRadius;
	for (int i=0; i<=slices; i++)
	{
		vertices[i*2][0]= prj->viewportCenter[0] + outerRadius*sinCache[i];
		vertices[i*2][1]= prj->viewportCenter[1] + outerRadius*cosCache[i];
		vertices[i*2][2] = 0.0;
		vertices[i*2+1][0]= prj->viewportCenter[0] + radiusHigh*sinCache[i];
		vertices[i*2+1][1]= prj->viewportCenter[1] + radiusHigh*cosCache[i];
		vertices[i*2+1][2] = 0.0;
	}
	drawFromArray(TriangleStrip, (slices+1)*2, 0, false);
	enableClientStates(false);

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


void StelPainter::sFanDisk(double radius, int innerFanSlices, int level)
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
			setArrays((Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData());
			drawFromArray(TriangleFan, vertexArr.size()/3);
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
	setArrays((Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData());
	drawFromArray(TriangleFan, vertexArr.size()/3);
}

void StelPainter::sRing(double rMin, double rMax, int slices, int stacks, int orientInside)
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
		setArrays((Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData(), NULL, (Vec3f*)normalArr.constData());
		drawFromArray(TriangleStrip, vertexArr.size()/3);
	}
}

static void sSphereMapTexCoordFast(double rho_div_fov, double costheta, double sintheta, QVector<float>& out)
{
	if (rho_div_fov>0.5)
		rho_div_fov=0.5;
	out << 0.5 + rho_div_fov * costheta << 0.5 + rho_div_fov * sintheta;
}

void StelPainter::sSphereMap(double radius, int slices, int stacks, double textureFov, int orientInside)
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
			setArrays((Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData(), NULL, (Vec3f*)normalArr.constData());
			drawFromArray(TriangleStrip, vertexArr.size()/3);
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
			setArrays((Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData(), NULL, (Vec3f*)normalArr.constData());
			drawFromArray(TriangleStrip, vertexArr.size()/3);
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

void StelPainter::drawText(const Vec3d& v, const QString& str, float angleDeg, float xshift, float yshift, bool noGravity) const
{
	Vec3d win;
	prj->project(v, win);
	drawText(win[0], win[1], str, angleDeg, xshift, yshift, noGravity);
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
#ifndef USE_OPENGL_ES2
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	glPushAttrib(GL_ALL_ATTRIB_BITS);
#endif
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float color[4];
	glGetFloatv(GL_CURRENT_COLOR, color);

#if QT_VERSION>=0x040600
	qPainter->endNativePainting();
#endif

	qPainter->save();
	qPainter->resetTransform();
	qPainter->resetMatrix();
	qPainter->setRenderHints(QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);
	const QColor qCol=QColor::fromRgbF(qMin(1.f,color[0]), qMin(1.f,color[1]), qMin(1.f,color[2]), qMin(1.f,color[3]));
	qPainter->setPen(qCol);

	if (prj->gravityLabels && !noGravity)
	{
		drawTextGravity180(x, y, str, xshift, yshift);
	}
	else
	{
#if QT_VERSION>=0x040600
		qPainter->translate(x+xshift, prj->viewportXywh[3]-y-yshift);
#else
		qPainter->translate(x+xshift, y+yshift);
		qPainter->scale(1, -1);
#endif
		qPainter->drawText(0, 0, str);
	}
	qPainter->restore();

#if QT_VERSION>=0x040600
	qPainter->beginNativePainting();
#endif

#ifndef USE_OPENGL_ES2
	glPopClientAttrib();
	glPopAttrib();
#endif
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}

static QVector<Vec2f> tmpVertexArray;

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
QVector<Vec2f> StelPainter::smallCircleVertexArray;

void StelPainter::drawSmallCircleVertexArray()
{
	if (smallCircleVertexArray.isEmpty())
		return;

	Q_ASSERT(smallCircleVertexArray.size()>1);

	enableClientStates(true);
	setVertexPointer(2, GL_FLOAT, smallCircleVertexArray.constData());
	drawFromArray(LineStrip, smallCircleVertexArray.size(), 0, false);
	enableClientStates(false);
	smallCircleVertexArray.resize(0);
}

static Vec3d pt1, pt2;
void StelPainter::drawGreatCircleArc(const Vec3d& start, const Vec3d& stop, const SphericalCap* clippingCap,
	void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, void* userData), void* userData)
 {
	 if (clippingCap)
	 {
		 pt1=start;
		 pt2=stop;
		 if (clippingCap->clipGreatCircle(pt1, pt2))
		 {
			drawSmallCircleArc(pt1, pt2, Vec3d(0), viewportEdgeIntersectCallback, userData);
		 }
		 return;
	}
	drawSmallCircleArc(start, stop, Vec3d(0), viewportEdgeIntersectCallback, userData);
 }

/*************************************************************************
 Draw a small circle arc in the current frame
*************************************************************************/
void StelPainter::drawSmallCircleArc(const Vec3d& start, const Vec3d& stop, const Vec3d& rotCenter, void (*viewportEdgeIntersectCallback)(const Vec3d& screenPos, const Vec3d& direction, void* userData), void* userData)
{
	Q_ASSERT(smallCircleVertexArray.empty());

	QLinkedList<Vec3d> tessArc;	// Contains the list of projected points from the tesselated arc
	Vec3d win1, win2;
	win1[2] = prj->project(start, win1) ? 1.0 : -1.;
	win2[2] = prj->project(stop, win2) ? 1.0 : -1.;
	tessArc.append(win1);


	if (rotCenter.lengthSquared()<0.00000001)
	{
		// Great circle
		// Perform the tesselation of the arc in small segments in a way so that the lines look smooth
		fIter(prj, start, stop, win1, win2, tessArc, tessArc.insert(tessArc.end(), win2), 1, rotCenter);
	}
	else
	{
		Vec3d tmp = (rotCenter^start)/rotCenter.length();
		const double radius = fabs(tmp.length());
		// Perform the tesselation of the arc in small segments in a way so that the lines look smooth
		fIter(prj, start-rotCenter, stop-rotCenter, win1, win2, tessArc, tessArc.insert(tessArc.end(), win2), radius, rotCenter);
	}

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
			smallCircleVertexArray.append(Vec2f(p1[0], p1[1]));
			if (i+1==tessArc.end())
			{
				smallCircleVertexArray.append(Vec2f(p2[0], p2[1]));
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
				smallCircleVertexArray.append(Vec2f(p1[0], p1[1]));
			drawSmallCircleVertexArray();
		}
	}
	Q_ASSERT(smallCircleVertexArray.isEmpty());
}

// Project the passed triangle on the screen ensuring that it will look smooth, even for non linear distortion
// by splitting it into subtriangles.
void StelPainter::projectSphericalTriangle(const SphericalCap* clippingCap, const Vec3d* vertices, QVarLengthArray<Vec2f, 4096>* outVertices,
		const Vec2f* texturePos, QVarLengthArray<Vec2f, 4096>* outTexturePos,
		int nbI, bool checkDisc1, bool checkDisc2, bool checkDisc3) const
{
	Q_ASSERT(fabs(vertices[0].length()-1.)<0.00001);
	Q_ASSERT(fabs(vertices[1].length()-1.)<0.00001);
	Q_ASSERT(fabs(vertices[2].length()-1.)<0.00001);
	if (clippingCap && clippingCap->containsTriangle(vertices))
		clippingCap = NULL;
	if (clippingCap && !clippingCap->intersectsTriangle(vertices))
		return;
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
		outVertices->append(Vec2f(e0[0], e0[1])); outVertices->append(Vec2f(e1[0], e1[1])); outVertices->append(Vec2f(e2[0], e2[1]));
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
		outVertices->append(Vec2f(e0[0], e0[1])); outVertices->append(Vec2f(e1[0], e1[1])); outVertices->append(Vec2f(e2[0], e2[1]));
		if (outTexturePos)
			outTexturePos->append(texturePos,3);
		return;
	}

	// Recursively splits the triangle into sub triangles.
	// Depending on which combination of sides of the triangle has to be split a different strategy is used.
	Vec3d va[3];
	Vec2f ta[3];
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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, true, true, false);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, true, false, true);
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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, false, true, true);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, true, true, false);
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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, false, true, true);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, true, false, true);
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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, true, true, false);
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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);


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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, true, false, true);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1, false, true, true);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);

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
		projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);
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
	projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);

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
	projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);

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
	projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);

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
	projectSphericalTriangle(clippingCap, va, outVertices, ta, outTexturePos, nbI+1);

	return;
}

static QVarLengthArray<Vec2f, 4096> polygonVertexArray;
static QVarLengthArray<Vec2f, 4096> polygonTextureCoordArray;
// XXX: We should change the type to unsigned int
static QVarLengthArray<unsigned short, 4096> indexArray;

void StelPainter::drawGreatCircleArcs(const StelVertexArray& va, const SphericalCap* clippingCap)
{
	Q_ASSERT(va.vertex.size()!=1);
	switch (va.primitiveType)
	{
		case StelVertexArray::Lines:
			Q_ASSERT(va.vertex.size()%2==0);
			for (int i=0;i<va.vertex.size();i+=2)
				drawGreatCircleArc(va.vertex.at(i), va.vertex.at(i+1), clippingCap);
			return;
		case StelVertexArray::LineStrip:
			for (int i=0;i<va.vertex.size()-1;++i)
				drawGreatCircleArc(va.vertex.at(i), va.vertex.at(i+1), clippingCap);
			return;
		case StelVertexArray::LineLoop:
			for (int i=0;i<va.vertex.size()-1;++i)
				drawGreatCircleArc(va.vertex.at(i), va.vertex.at(i+1), clippingCap);
			drawGreatCircleArc(va.vertex.last(), va.vertex.first(), clippingCap);
			return;
		default:
			Q_ASSERT(0); // Unsupported primitive yype
	}
}

// The function object that we use as an interface between VertexArray::foreachTriangle and
// StelPainter::projectSphericalTriangle.
//
// This is used by drawSphericalTriangles to project all the triangles coordinates in a StelVertexArray into our global
// vertex array buffer.
class VertexArrayProjector
{
public:
	VertexArrayProjector(const StelVertexArray& ar, StelPainter* apainter, const SphericalCap* aclippingCap,
						 QVarLengthArray<Vec2f, 4096>* aoutVertices, QVarLengthArray<Vec2f, 4096>* aoutTexturePos=NULL)
		   : vertexArray(ar), painter(apainter), clippingCap(aclippingCap), outVertices(aoutVertices),
			 outTexturePos(aoutTexturePos)
	{
	}

	// Project a single triangle and add it into the output arrays
	inline void operator()(const Vec3d* v0, const Vec3d* v1, const Vec3d* v2,
						   const Vec2f* t0, const Vec2f* t1, const Vec2f* t2,
						   unsigned int i0, unsigned int i1, unsigned i2)
	{
		// XXX: we may optimize more by putting the declaration and the test outside of this method.
		const Vec3d tmpVertex[3] = {*v0, *v1, *v2};
		if (outTexturePos)
		{
			const Vec2f tmpTexture[3] = {*t0, *t1, *t2};
			painter->projectSphericalTriangle(clippingCap, tmpVertex, outVertices, tmpTexture, outTexturePos);
		}
		else
			painter->projectSphericalTriangle(clippingCap, tmpVertex, outVertices, NULL, NULL);
	}

	// Draw the resulting arrays
	void drawResult()
	{
		painter->setVertexPointer(2, GL_FLOAT, outVertices->constData());
		if (outTexturePos)
			painter->setTexCoordPointer(2, GL_FLOAT, outTexturePos->constData());
		painter->enableClientStates(true, outTexturePos != NULL);
		painter->drawFromArray(StelPainter::Triangles, outVertices->size(), 0, false);
		painter->enableClientStates(false);
	}

private:
	const StelVertexArray& vertexArray;
	StelPainter* painter;
	const SphericalCap* clippingCap;
	QVarLengthArray<Vec2f, 4096>* outVertices;
	QVarLengthArray<Vec2f, 4096>* outTexturePos;
};


// The function object that can be used by the StelVertexArray::foreachTriangle method.  It removes all the triangles
// that are intersecting a discontinuity of the projection.
//
// Since we don't modify the vertex (no projection is done), we only put the indices of the 'safe' triangle into the
// outIndice array.
//
// We also store the remaining triangles so that we can eventually draw them afterward.
class RemoveDiscontinuousTriangles
{
public:
	RemoveDiscontinuousTriangles(const StelVertexArray& ar, StelPainter* apainter,
								 QVarLengthArray<unsigned short, 4096>* aoutIndices, bool isTextured)
		: vertexArray(ar), painter(apainter), outIndices(aoutIndices), textured(isTextured)
	{
	}

	inline void operator()(const Vec3d* v0, const Vec3d* v1, const Vec3d* v2,
						   const Vec2f* t0, const Vec2f* t1, const Vec2f* t2,
						   unsigned int i0, unsigned int i1, unsigned i2)
	{
		StelProjector* prj = painter->getProjector().data();
		if (prj->intersectViewportDiscontinuity(*v0, *v1) ||
			prj->intersectViewportDiscontinuity(*v1, *v2) ||
			prj->intersectViewportDiscontinuity(*v2, *v0))
		{
			remainingTriangles << i0 << i1 << i2;
			return;
		}

		outIndices->append(i0);
		outIndices->append(i1);
		outIndices->append(i2);
	}

	void drawResult()
	{
		painter->setVertexPointer(3, GL_DOUBLE, vertexArray.vertex.constData());
		painter->setTexCoordPointer(2, GL_FLOAT, vertexArray.texCoords.constData());
		painter->enableClientStates(true, textured);
		StelPainter::DrawingMode mode = (StelPainter::DrawingMode)vertexArray.primitiveType;
		painter->drawFromArray(mode, outIndices->constData(), 0, outIndices->size());
		painter->enableClientStates(false);
	}

private:
	const StelVertexArray& vertexArray;
	StelPainter* painter;
	QVarLengthArray<unsigned short, 4096>* outIndices;
	QList<unsigned short> remainingTriangles;
	bool textured;
};


void StelPainter::drawSphericalTriangles(const StelVertexArray& va, bool textured, const SphericalCap* clippingCap, bool doSubDivide)
{
	if (va.vertex.isEmpty())
		return;

	// Never need to do clipping if the projection doesn't have a discontinuity
	const bool doClip = prj->hasDiscontinuity();

	Q_ASSERT(va.vertex.size()>2);
#ifndef USE_OPENGL_ES2
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
	polygonVertexArray.clear();
	polygonTextureCoordArray.clear();
	indexArray.clear();

	// The simplest case, we don't need to iterate through the triangles at all.
	if (!doClip && !doSubDivide)
	{
		Q_ASSERT_X(va.indices.isEmpty(), Q_FUNC_INFO, "indexed vertex array not done yet");

		setVertexPointer(3, GL_DOUBLE, va.vertex.constData());
		setTexCoordPointer(2, GL_FLOAT, va.texCoords.constData());

		enableClientStates(true, textured);
		drawFromArray((DrawingMode)va.primitiveType, va.vertex.size());
		enableClientStates(false);
		return;
	}
	if (doClip && !doSubDivide)
	{
		RemoveDiscontinuousTriangles result = va.foreachTriangle(RemoveDiscontinuousTriangles(va, this, &indexArray, textured));
		result.drawResult();
		return;
	}

	// the last case.  It is the slowest, it process the triangles one by one.
	{
		// Project all the triangles of the VertexArray into our buffer arrays.
		VertexArrayProjector result = va.foreachTriangle(VertexArrayProjector(va, this, clippingCap, &polygonVertexArray, textured ? &polygonTextureCoordArray : NULL));
		result.drawResult();
		return;
	}
}

// Draw the given SphericalPolygon.
void StelPainter::drawSphericalRegion(const SphericalRegion* poly, SphericalPolygonDrawMode drawMode, const SphericalCap* clippingCap)
{
	switch (drawMode)
	{
		case SphericalPolygonDrawModeBoundary:
			drawGreatCircleArcs(poly->getOutlineVertexArray(), clippingCap);
			break;
		case SphericalPolygonDrawModeFill:
		case SphericalPolygonDrawModeTextureFill:
			glEnable(GL_CULL_FACE);
			// Assumes the polygon is already tesselated as triangles
			drawSphericalTriangles(poly->getFillVertexArray(), drawMode==SphericalPolygonDrawModeTextureFill, clippingCap);
			glDisable(GL_CULL_FACE);
			break;
		default:
			Q_ASSERT(0);
	}
}


/*************************************************************************
 draw a simple circle, 2d viewport coordinates in pixel
*************************************************************************/
void StelPainter::drawCircle(double x,double y,double r)
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
	static QVarLengthArray<Vec3f, 180> circleVertexArray(180);

	for (int i=0;i<segments;i++)
	{
		circleVertexArray[i].set(x+dx,y+dy,0);
		r = dx*cp-dy*sp;
		dy = dx*sp+dy*cp;
		dx = r;
	}
	enableClientStates(true);
	setVertexPointer(3, GL_FLOAT, circleVertexArray.data());
	drawFromArray(LineLoop, 180, 0, false);
	enableClientStates(false);
}


void StelPainter::drawSprite2dMode(double x, double y, float radius)
{
	static float vertexData[] = {-10.,-10.,10.,-10., 10.,10., -10.,10.};
	static const float texCoordData[] = {0.,0., 1.,0., 0.,1., 1.,1.};
	vertexData[0]=x-radius; vertexData[1]=y-radius;
	vertexData[2]=x+radius; vertexData[3]=y-radius;
	vertexData[4]=x-radius; vertexData[5]=y+radius;
	vertexData[6]=x+radius; vertexData[7]=y+radius;
	enableClientStates(true, true);
	setVertexPointer(2, GL_FLOAT, vertexData);
	setTexCoordPointer(2, GL_FLOAT, texCoordData);
	drawFromArray(TriangleStrip, 4, 0, false);
	enableClientStates(false);
}

void StelPainter::drawSprite2dMode(const Vec3d& v, float radius)
{
	Vec3d win;
	prj->project(v, win);
	drawSprite2dMode(win[0], win[1], radius);
}

void StelPainter::drawRect2d(float x, float y, float width, float height)
{
	static float vertexData[] = {-10.,-10.,10.,-10., 10.,10., -10.,10.};
	static const float texCoordData[] = {0.,0., 1.,0., 0.,1., 1.,1.};
	vertexData[0]=x; vertexData[1]=y;
	vertexData[2]=x+width; vertexData[3]=y;
	vertexData[4]=x; vertexData[5]=y+height;
	vertexData[6]=x+width; vertexData[7]=y+height;

	enableClientStates(true, true);
	setVertexPointer(2, GL_FLOAT, vertexData);
	setTexCoordPointer(2, GL_FLOAT, texCoordData);
	drawFromArray(TriangleStrip, 4, 0, false);
	enableClientStates(false);
}

/*************************************************************************
 Same function but with a rotation angle
*************************************************************************/
void StelPainter::drawSprite2dMode(double x, double y, float radius, float rotation)
{
	glPushMatrix();
	glTranslatef(x, y, 0.0);
	glRotatef(rotation,0.,0.,1.);
	static float vertexData[] = {-10.,-10.,10.,-10., 10.,10., -10.,10.};
	static const float texCoordData[] = {0.,0., 1.,0., 0.,1., 1.,1.};
	vertexData[0]=-radius; vertexData[1]=-radius;
	vertexData[2]=radius; vertexData[3]=-radius;
	vertexData[4]=-radius; vertexData[5]=radius;
	vertexData[6]=radius; vertexData[7]=radius;

	enableClientStates(true, true);
	setVertexPointer(2, GL_FLOAT, vertexData);
	setTexCoordPointer(2, GL_FLOAT, texCoordData);
	drawFromArray(TriangleStrip, 4, 0, false);
	enableClientStates(false);

	glPopMatrix();

}

/*************************************************************************
 Draw a GL_POINT at the given position
*************************************************************************/
void StelPainter::drawPoint2d(double x, double y)
{
	static float vertexData[] = {0.,0.};
	vertexData[0]=x;
	vertexData[1]=y;

	enableClientStates(true);
	setVertexPointer(2, GL_FLOAT, vertexData);
	drawFromArray(Points, 1, 0, false);
	enableClientStates(false);
}


/*************************************************************************
 Draw a line between the 2 points.
*************************************************************************/
void StelPainter::drawLine2d(double x1, double y1, double x2, double y2)
{
	static float vertexData[] = {0.,0.,0.,0.};
	vertexData[0]=x1;
	vertexData[1]=y1;
	vertexData[2]=x2;
	vertexData[3]=y2;

	enableClientStates(true);
	setVertexPointer(2, GL_FLOAT, vertexData);
	drawFromArray(Lines, 2, 0, false);
	enableClientStates(false);
}

///////////////////////////////////////////////////////////////////////////
// Drawing methods for general (non-linear) mode

void StelPainter::sSphere(double radius, double oneMinusOblateness, int slices, int stacks, int orientInside, bool flipTexture)
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
	// If the texture is flipped, we iterate the coordinates backward.
	const GLfloat ds = ((flipTexture) ? -1 : 1) * 1.0 / slices;
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
		s = (!flipTexture) ? 0.0 : 1.0;
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
			setArrays((Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData(), (Vec3f*)colorArr.constData());
		else
			setArrays((Vec3d*)vertexArr.constData(), (Vec2f*)texCoordArr.constData());
		drawFromArray(TriangleStrip, vertexArr.size()/3);
		t -= dt;
	}

	if (isLightOn)
		glEnable(GL_LIGHTING);
}

// Reimplementation of gluCylinder : glu is overrided for non standard projection
void StelPainter::sCylinder(double radius, double height, int slices, int stacks, int orientInside)
{
		double da, r, dz;
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
		setArrays((Vec3d*)vertexArray.constData(), (Vec2f*)texCoordArray.constData(), NULL,  (Vec3f*)normalArray.constData());
		drawFromArray(TriangleStrip, vertexArray.size()/3);
		t += dt;
		z += dz;
	}				/* for stacks */

	if (orientInside)
		glCullFace(GL_BACK);
}


void StelPainter::setPointSize(qreal size)
{
#ifndef USE_OPENGL_ES2
	glPointSize(size);
#else
#error GL ES2 to be done
#endif
}

void StelPainter::setArrays(const Vec3d* vertice, const Vec2f* texCoords, const Vec3f* colorArray, const Vec3f* normalArray) {
	enableClientStates(vertice, texCoords, colorArray, normalArray);
	setVertexPointer(3, GL_DOUBLE, vertice);
	setTexCoordPointer(2, GL_FLOAT, texCoords);
	setColorPointer(3, GL_FLOAT, colorArray);
	setNormalPointer(GL_FLOAT, normalArray);
}

void StelPainter::enableClientStates(bool vertex, bool texture, bool color, bool normal) {
	vertexArray.enabled = vertex;
	texCoordArray.enabled = texture;
	colorArray.enabled = color;
	normalArray.enabled = normal;
}

void StelPainter::prepareDrawFromArray(int count, int index, const unsigned short* indices, bool doProj)
{
#ifndef USE_OPENGL_ES2
	// Project the vertex array using current projection
	ArrayDesc vertexArray = (doProj)? projectArray(this->vertexArray, index, count, indices) : this->vertexArray;
	// Enable the client state and set the opengl array for each array
	prepareArray(vertexArray, GL_VERTEX_ARRAY);
	prepareArray(texCoordArray, GL_TEXTURE_COORD_ARRAY);
	prepareArray(normalArray, GL_NORMAL_ARRAY);
	prepareArray(colorArray, GL_COLOR_ARRAY);
#else
#error GL ES2 to be done
#endif
}

void StelPainter::prepareArray(const ArrayDesc& array, int cap)
{
#ifndef USE_OPENGL_ES2
	if (!array.enabled)
	{
		glDisableClientState(cap);
		return;
	}
	glEnableClientState(cap);
	switch (cap)
	{
	case GL_VERTEX_ARRAY:
		glVertexPointer(array.size, array.type, 0, array.pointer);
		break;
	case GL_TEXTURE_COORD_ARRAY:
		glTexCoordPointer(array.size, array.type, 0, array.pointer);
		break;
	case GL_NORMAL_ARRAY:
		glNormalPointer(array.type, 0, array.pointer);
		break;
	case GL_COLOR_ARRAY:
		glColorPointer(array.size, array.type, 0, array.pointer);
		break;
	default:
		Q_ASSERT(0);
	}
#else
	Q_ASSERT(0);
#endif
}

void StelPainter::drawFromArray(DrawingMode mode, int count, int index, bool doProj)
{
#ifndef USE_OPENGL_ES2
	prepareDrawFromArray(count, index, NULL, doProj);
	glDrawArrays(mode, index, count);
#else
#error GL ES2 to be done
#endif
}

void StelPainter::drawFromArray(DrawingMode mode, const unsigned short* indices, int offset, int count, bool doProj)
{
#ifndef USE_OPENGL_ES2
	prepareDrawFromArray(count, 0, indices + offset, doProj);
	glDrawElements(mode, count, GL_UNSIGNED_SHORT, indices + offset);
#else
#error GL ES2 to be done
#endif
}

StelPainter::ArrayDesc StelPainter::projectArray(const StelPainter::ArrayDesc& array, int index, int count, const unsigned short* indices)
{
	// XXX: we should use a more generic way to test whether or not to do the projection.
	if (dynamic_cast<StelProjector2d*>(prj.data()))
	{
		return array;
	}

	Q_ASSERT(array.size == 3);
	Q_ASSERT(array.type == GL_DOUBLE);
	Vec3d* vecArray = (Vec3d*)array.pointer;
	Vec3d win;

	// We have two different cases :
	// 1) We are not using an indice array.  In that case the size of the array is known
	// 2) We are using an indice array.  In that case we have to find the max value by iterating through the indices.
	if (!indices)
	{
		tmpVertexArray.resize(index + count);
		for (int i=index; i< index + count; ++i)
		{
			prj->project(vecArray[i], win);
			tmpVertexArray[i].set(win[0], win[1]);
		}
	} else
	{
		// we need to find the max value of the indices !
		unsigned short max = 0;
		for (int i = index; i < index + count; ++i)
		{
			max = std::max(max, indices[i]);
		}
		tmpVertexArray.resize(max+1);
		for (int i=index; i< index + count; ++i)
		{
			prj->project(vecArray[indices[i]], win);
			tmpVertexArray[indices[i]].set(win[0], win[1]);
		}
	}

	ArrayDesc ret;
	ret.size = 2;
	ret.type = GL_FLOAT;
	ret.pointer = tmpVertexArray.constData();
	ret.enabled = array.enabled;
	return ret;
}
