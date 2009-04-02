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

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#else
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelFont.hpp"

#include <QDebug>
#include <QString>
#include <QSettings>
#include <QLinkedList>
#include <QPainter>
#include <QMutex>

QMutex* StelPainter::globalMutex = new QMutex();
bool StelPainter::flagGlPointSprite = false;

StelPainter::StelPainter(const StelProjectorP& proj) : prj(proj)
{
	Q_ASSERT(proj);
	Q_ASSERT(globalMutex);
	
// 	GLenum er = glGetError();
// 	if (er!=GL_NO_ERROR)
// 	{
// 		if (er==GL_INVALID_OPERATION)
// 			qFatal("Invalid openGL operation. It is likely that you used openGL calls without having a valid instance of StelPainter");
// 	}
	
	// Lock the global mutex ensuring that no other instances of StelPainter are currently being used
	if (globalMutex->tryLock()==false)
	{
		qFatal("There can be only 1 instance of StelPainter at a given time");
	}
	
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
	
	// We are done with this StelPainter
	globalMutex->unlock();
}

//! Switch to native OpenGL painting, i.e not using QPainter
//! After this call revertToQtPainting MUST be called
void StelPainter::switchToNativeOpenGLPainting()
{
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
void StelPainter::revertToQtPainting()
{
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
	GLUquadricObj * p = gluNewQuadric();
	gluDisk(p, 0.5*prj->viewportFovDiameter, prj->getViewportWidth()+prj->getViewportHeight(), 256, 1);  // should always cover whole screen
	gluDeleteQuadric(p);
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
//  for (i=level-1;i>=0;i--) {
//    double f = ((i+1)/(double)(level+1));
//    rad[i] = radius*f*f;
//  }
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
	for (i=level;i>0;--i,slices_step<<=1)
	{
		for (j=0,cos_sin_theta_p=cos_sin_theta; j<slices; j+=slices_step,cos_sin_theta_p+=2*slices_step)
		{
			glBegin(GL_TRIANGLE_FAN);
			double x = rad[i]*cos_sin_theta_p[slices_step];
			double y = rad[i]*cos_sin_theta_p[slices_step+1];
			glTexCoord2d(0.5*(1.0+x/radius),0.5*(1.0+y/radius));
			drawVertex3(x,y,0);
		
			x = rad[i]*cos_sin_theta_p[2*slices_step];
			y = rad[i]*cos_sin_theta_p[2*slices_step+1];
			glTexCoord2d(0.5*(1.0+x/radius),0.5*(1.0+y/radius));
			drawVertex3(x,y,0);
		
			x = rad[i-1]*cos_sin_theta_p[2*slices_step];
			y = rad[i-1]*cos_sin_theta_p[2*slices_step+1];
			glTexCoord2d(0.5*(1.0+x/radius),0.5*(1.0+y/radius));
			drawVertex3(x,y,0);
		
			x = rad[i-1]*cos_sin_theta_p[0];
			y = rad[i-1]*cos_sin_theta_p[1];
			glTexCoord2d(0.5*(1.0+x/radius),0.5*(1.0+y/radius));
			drawVertex3(x,y,0);
		
			x = rad[i]*cos_sin_theta_p[0];
			y = rad[i]*cos_sin_theta_p[1];
			glTexCoord2d(0.5*(1.0+x/radius),0.5*(1.0+y/radius));
			drawVertex3(x,y,0);
			glEnd();
		}
	}
	// draw the inner polygon
	slices_step>>=1;
	glBegin(GL_POLYGON);
	for (j=0,cos_sin_theta_p=cos_sin_theta;	j<=slices; j+=slices_step,cos_sin_theta_p+=2*slices_step)
	{
		double x = rad[0]*cos_sin_theta_p[0];
		double y = rad[0]*cos_sin_theta_p[1];
		glTexCoord2d(0.5*(1.0+x/radius),0.5*(1.0+y/radius));
		drawVertex3(x,y,0);
	}
	glEnd();
}


// Draw a disk with a special texturing mode having texture center at disk center
void StelPainter::sDisk(GLdouble radius, GLint slices, GLint stacks, int orientInside) const
{
	GLint i,j;
	const GLfloat nsign = orientInside ? -1 : 1;
    double r;
	const double dr = radius / stacks;

	const double dtheta = 2.0 * M_PI / slices;
	if (slices < 0) slices = -slices;
	Q_ASSERT(slices<=MAX_SLICES);
	ComputeCosSinTheta(dtheta,slices);
	double *cos_sin_theta_p;

	// draw intermediate stacks as quad strips
	for (i = 0, r = 0.0; i < stacks; i++, r+=dr)
	{
		glBegin(GL_QUAD_STRIP);
		for (j = 0,cos_sin_theta_p = cos_sin_theta; j <= slices;
		     j++,cos_sin_theta_p+=2)
		{
			double x = r*cos_sin_theta_p[0];
			double y = r*cos_sin_theta_p[1];
			glNormal3f(0, 0, nsign);
			glTexCoord2d(0.5+0.5*x/radius, 0.5+0.5*y/radius);
			drawVertex3(x, y, 0);
			x = (r+dr)*cos_sin_theta_p[0];
			y = (r+dr)*cos_sin_theta_p[1];
			glNormal3f(0, 0, nsign);
			glTexCoord2d(0.5+0.5*x/radius, 0.5+0.5*y/radius);
			drawVertex3(x, y, 0);
		}
		glEnd();
	}
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


	// draw intermediate stacks as quad strips
	for (double r = rMin; r < rMax; r+=dr)
	{
		const double tex_r0 = (r-rMin)/(rMax-rMin);
		const double tex_r1 = (r+dr-rMin)/(rMax-rMin);
		glBegin(GL_QUAD_STRIP /*GL_TRIANGLE_STRIP*/);
		for (j=0,cos_sin_theta_p=cos_sin_theta;
		        j<=slices;
		        j++,cos_sin_theta_p+=2)
		{
			x = r*cos_sin_theta_p[0];
			y = r*cos_sin_theta_p[1];
			glNormal3d(0, 0, nsign);
			glTexCoord2d(tex_r0, 0.5);
			drawVertex3(x, y, 0);
			x = (r+dr)*cos_sin_theta_p[0];
			y = (r+dr)*cos_sin_theta_p[1];
			glNormal3d(0, 0, nsign);
			glTexCoord2d(tex_r1, 0.5);
			drawVertex3(x, y, 0);
		}
		glEnd();
	}
}

static void sSphereMapTexCoordFast(double rho_div_fov, double costheta, double sintheta)
{
	if (rho_div_fov>0.5)
		rho_div_fov=0.5;
	glTexCoord2d(0.5 + rho_div_fov * costheta,
	             0.5 + rho_div_fov * sintheta);
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

	// draw intermediate stacks as quad strips
	if (!orientInside) // nsign==1
	{
		for (i = 0,cos_sin_rho_p=cos_sin_rho,rho=0.0; i < imax; ++i,cos_sin_rho_p+=2,rho+=drho)
		{
			glBegin(GL_QUAD_STRIP);
			for (j=0,cos_sin_theta_p=cos_sin_theta;j<=slices;++j,cos_sin_theta_p+=2)
			{
				x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
				z = cos_sin_rho_p[0];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho, cos_sin_theta_p[0], cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);

				x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
				z = cos_sin_rho_p[2];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho + drho, cos_sin_theta_p[0], cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);
			}
			glEnd();
		}
	}
	else
	{
		for (i = 0,cos_sin_rho_p=cos_sin_rho,rho=0.0; i < imax; ++i,cos_sin_rho_p+=2,rho+=drho)
		{
			glBegin(GL_QUAD_STRIP);
			for (j=0,cos_sin_theta_p=cos_sin_theta;j<=slices;++j,cos_sin_theta_p+=2)
			{
				x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
				z = cos_sin_rho_p[2];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho + drho, cos_sin_theta_p[0], -cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);

				x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
				z = cos_sin_rho_p[0];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho, cos_sin_theta_p[0], -cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);
			}
			glEnd();
		}
	}
}

void StelPainter::drawTextGravity180(const StelFont* font, float x, float y, const QString& ws,
                                   bool speedOptimize, float xshift, float yshift) const
{
	static float dx, dy, d, theta, psi;
	dx = x - prj->viewportCenter[0];
	dy = y - prj->viewportCenter[1];
	d = sqrt(dx*dx + dy*dy);

	// If the text is too far away to be visible in the screen return
	if (d>qMax(prj->viewportXywh[3], prj->viewportXywh[2])*2)
		return;

	theta = M_PI + std::atan2(dx, dy - 1);
	psi = std::atan2((float)font->getStrLen(ws)/ws.length(),d + 1) * 180./M_PI;

	if (psi>5)
		psi = 5;
	
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushMatrix();
	glTranslatef(x,y,0);
	if (prj->gravityLabels)
		glRotatef(theta*180./M_PI,0,0,-1);
	glTranslatef(xshift, -yshift, 0);
	glScalef(1, -1, 1);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	for (int i=0;i<ws.length();++i)
	{
		if( !speedOptimize )
		{
			font->printCharOutlined(ws[i]);
		}
		else
		{
			font->printChar(ws[i]);
		}

		// with typeface need to manually advance
		// TODO, absolute rotation would be better than relative
		// TODO: would look better with kerning information...
		glTranslatef(font->getStrLen(ws.mid(i,1)) * 1.05, 0, 0);

		if( !speedOptimize )
		{
			psi = std::atan2((float)font->getStrLen(ws.mid(i,1))*1.05f,(float)d) * 180./M_PI;
			if (psi>5)
				psi = 5;
		}
		
		glRotatef(psi,0,0,-1);
	}
	glPopMatrix();
	glPopAttrib();
}

/*************************************************************************
 Draw the string at the given position and angle with the given font
*************************************************************************/
void StelPainter::drawText(const StelFont* font, float x, float y, const QString& str, float angleDeg, float xshift, float yshift, bool noGravity) const
{
	if (prj->gravityLabels && !noGravity)
	{
		drawTextGravity180(font, x, y, str, true, xshift, yshift);
		return;
	}
	
// 	QPainter* painter = StelAppGraphicsScene::getInstance().switchToQPainting();
// 	if (painter==NULL)
// 	{
// 		StelAppGraphicsScene::getInstance().revertToOpenGL();
// 		qDebug() << "NULL Painter";
// 		return;
// 	}
// 	painter->setRenderHints(QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);
// 	painter->translate(QPointF(x+xshift, y+yshift));
// 	//painter->scale(1, -1.00001);
// 	painter->setBrush(QColor(255,255,0));
// 	painter->setPen(QColor(255,255,0));
// 	QPainterPath text;
// 	text.addText(QPointF(0,0), QFont(), str);
// 	painter->drawPath(text);
// 	//painter->drawText(QPointF(xshift, -yshift), str);
// 	StelAppGraphicsScene::getInstance().revertToOpenGL();
	
	glPushMatrix();
	glTranslatef(x,y,0);
	glRotatef(angleDeg,0,0,1);
	glTranslatef(0,font->getLineHeight(),0);
	font->print(xshift, yshift, str);
	glPopMatrix();
}


// Recursive method cutting a small circle in small segments
void fIter(const StelProjectorP& prj, const Vec3d& p1, const Vec3d& p2, Vec3d& win1, Vec3d& win2, QLinkedList<Vec3d>& vertexList, const QLinkedList<Vec3d>::iterator& iter, double radius, const Vec3d& center, int nbI=0, bool checkCrossDiscontinuity=true)
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
	
	Vec3d win3;
	Vec3d newVertex(p1+p2);
	newVertex.normalize();
	newVertex*=radius;
	const bool isValidVertex = prj->project(newVertex+center, win3);
	
	const Vec3d v1(win1[0]-win3[0], win1[1]-win3[1], 0);
	const Vec3d v2(win2[0]-win3[0], win2[1]-win3[1], 0);
	
	const double dist = std::sqrt((v1[0]*v1[0]+v1[1]*v1[1])*(v2[0]*v2[0]+v2[1]*v2[1]));
	const double cosAngle = (v1[0]*v2[0]+v1[1]*v2[1])/dist;
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
	smallCircleVertexArray.clear();
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
void StelPainter::projectSphericalTriangle(const Vec3d* vertices, QVector<Vec3d>* outVertices,
		const bool* edgeFlags, QVector<bool>* outEdgeFlags,
  		const Vec2d* texturePos, QVector<Vec2d>* outTexturePos,
		int nbI, bool checkDisc1, bool checkDisc2, bool checkDisc3) const
{
	bool cDiscontinuity1 = checkDisc1 && prj->intersectViewportDiscontinuity(vertices[0], vertices[1]);
	bool cDiscontinuity2 = checkDisc2 && prj->intersectViewportDiscontinuity(vertices[1], vertices[2]);
	bool cDiscontinuity3 = checkDisc3 && prj->intersectViewportDiscontinuity(vertices[0], vertices[2]);
	const bool cd1=cDiscontinuity1;
	const bool cd2=cDiscontinuity2;
	const bool cd3=cDiscontinuity3;
	
	Vec3d e0, e1, e2, win3;
	prj->project(vertices[0], e0);
	prj->project(vertices[1], e1);
	prj->project(vertices[2], e2);
	
	if (!cd1 && !cd2 && !cd3 && qMax((e2-e1).lengthSquared(), qMax((e0-e1).lengthSquared(), (e0-e2).lengthSquared())) < 50)
	{
		// The triangle is small enough, appends it
		outVertices->append(e0);
		outVertices->append(e1);
		outVertices->append(e2);
		if (outEdgeFlags)
		{
			outEdgeFlags->append(edgeFlags[0]);
			outEdgeFlags->append(edgeFlags[1]);
			outEdgeFlags->append(edgeFlags[2]);
		}
		if (outTexturePos)
		{
			outTexturePos->append(texturePos[0]);
			outTexturePos->append(texturePos[1]);
			outTexturePos->append(texturePos[2]);
		}
		return;
	}
	
	Vec2d v1, v2;
	if (checkDisc1 && cDiscontinuity1==false)
	{
		// If the distortion at segment e0,e1 is too big, flags it for subdivision
		prj->project(vertices[0]+vertices[1], win3);
		v1.set(e0[0]-win3[0], e0[1]-win3[1]);
		v2.set(e1[0]-win3[0], e1[1]-win3[1]);
		const double dist = std::sqrt((v1[0]*v1[0]+v1[1]*v1[1])*(v2[0]*v2[0]+v2[1]*v2[1]));
		const double cosAngle = (v1[0]*v2[0]+v1[1]*v2[1])/dist;
		cDiscontinuity1 = cosAngle>-0.998;
	}
	if (checkDisc2 && cDiscontinuity2==false)
	{
		// If the distortion at segment e1,e2 is too big, flags it for subdivision
		prj->project(vertices[1]+vertices[2], win3);
		v1.set(e1[0]-win3[0], e1[1]-win3[1]);
		v2.set(e2[0]-win3[0], e2[1]-win3[1]);
		const double dist = std::sqrt((v1[0]*v1[0]+v1[1]*v1[1])*(v2[0]*v2[0]+v2[1]*v2[1]));
		const double cosAngle = (v1[0]*v2[0]+v1[1]*v2[1])/dist;
		cDiscontinuity2 = cosAngle>-0.998;
	}
	if (checkDisc3 && cDiscontinuity3==false)
	{
		// If the distortion at segment e2,e0 is too big, flags it for subdivision
		prj->project(vertices[2]+vertices[0], win3);
		v1.set(e2[0]-win3[0], e2[1]-win3[1]);
		v2.set(e0[0]-win3[0], e0[1]-win3[1]);
		const double dist = std::sqrt((v1[0]*v1[0]+v1[1]*v1[1])*(v2[0]*v2[0]+v2[1]*v2[1]));
		const double cosAngle = (v1[0]*v2[0]+v1[1]*v2[1])/dist;
		cDiscontinuity3 = cosAngle>-0.998;
	}
	
	if (!cDiscontinuity1 && !cDiscontinuity2 && !cDiscontinuity3)
	{
		// The triangle is clean, appends it
		outVertices->append(e0);
		outVertices->append(e1);
		outVertices->append(e2);
		if (outEdgeFlags)
		{
			outEdgeFlags->append(edgeFlags[0]);
			outEdgeFlags->append(edgeFlags[1]);
			outEdgeFlags->append(edgeFlags[2]);
		}
		if (outTexturePos)
		{
			outTexturePos->append(texturePos[0]);
			outTexturePos->append(texturePos[1]);
			outTexturePos->append(texturePos[2]);
		}
		return;
	}
	
	if (nbI > 5)
	{
		// If we reached the limit number of iterations and still have a discontinuity,
		// discard the triangle.
		if (cd1 || cd2 || cd3)
		{
			// TODO, try to display an approximated triangle by computing the intersection
			// of the sides with the discontinuity.
			return;
		}
		// Else display it, it will be suboptimal though.
		outVertices->append(e0);
		outVertices->append(e1);
		outVertices->append(e2);
		if (outEdgeFlags)
		{
			outEdgeFlags->append(edgeFlags[0]);
			outEdgeFlags->append(edgeFlags[1]);
			outEdgeFlags->append(edgeFlags[2]);
		}
		if (outTexturePos)
		{
			outTexturePos->append(texturePos[0]);
			outTexturePos->append(texturePos[1]);
			outTexturePos->append(texturePos[2]);
		}
		return;
	}
	
	// Recursively splits the triangle into sub triangles.
	// Depending on which combination of sides of the triangle has to be split a different strategy is used.
	Vec3d va[3];
	Vec2d ta[3];
	bool ba[3];
	// Only 1 side has to be split: split the triangle in 2
	if (cDiscontinuity1 && !cDiscontinuity2 && !cDiscontinuity3)
	{
		va[0]=vertices[0];
		va[1]=vertices[0]+vertices[1];
		va[1].normalize();
		va[2]=vertices[2];
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[0]+texturePos[1])/2.;
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
			ta[0]=(texturePos[0]+texturePos[1])/2.;
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
		va[2]=vertices[1]+vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=texturePos[1];
			ta[2]=(texturePos[1]+texturePos[2])/2.;
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
			ta[1]=(texturePos[1]+texturePos[2])/2.;
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
		va[2]=vertices[0]+vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=texturePos[1];
			ta[2]=(texturePos[0]+texturePos[2])/2.;
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
			ta[0]=(texturePos[0]+texturePos[2])/2.;
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
		va[1]=vertices[0]+vertices[1];
		va[1].normalize();
		va[2]=vertices[1]+vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[0]+texturePos[1])/2.;
			ta[2]=(texturePos[1]+texturePos[2])/2.;
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
			ta[0]=(texturePos[0]+texturePos[1])/2.;
			ta[1]=texturePos[1];
			ta[2]=(texturePos[1]+texturePos[2])/2.;
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
			ta[1]=(texturePos[1]+texturePos[2])/2.;
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
		va[1]=vertices[0]+vertices[1];
		va[1].normalize();
		va[2]=vertices[0]+vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=(texturePos[0]+texturePos[1])/2.;
			ta[2]=(texturePos[0]+texturePos[2])/2.;
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
			ta[0]=(texturePos[0]+texturePos[1])/2.;
			ta[1]=texturePos[2];
			ta[2]=(texturePos[0]+texturePos[2])/2.;
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
			ta[0]=(texturePos[0]+texturePos[1])/2.;
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
		va[2]=vertices[1]+vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=texturePos[0];
			ta[1]=texturePos[1];
			ta[2]=(texturePos[1]+texturePos[2])/2.;
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
		va[2]=vertices[0]+vertices[2];
		va[2].normalize();
		if (outTexturePos)
		{
			ta[0]=(texturePos[1]+texturePos[2])/2.;
			ta[1]=texturePos[2];
			ta[2]=(texturePos[0]+texturePos[2])/2.;
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
			ta[1]=(texturePos[1]+texturePos[2])/2.;
			ta[2]=(texturePos[0]+texturePos[2])/2.;
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
	
	va[0]=vertices[0]+vertices[1];
	va[0].normalize();
	va[1]=vertices[1]+vertices[2];
	va[1].normalize();
	va[2]=vertices[0]+vertices[2];
	va[2].normalize();
	if (outTexturePos)
	{
		ta[0]=(texturePos[0]+texturePos[1])/2.;
		ta[1]=(texturePos[1]+texturePos[2])/2.;
		ta[2]=(texturePos[0]+texturePos[2])/2.;
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
		ta[1]=(texturePos[0]+texturePos[1])/2.;
		ta[2]=(texturePos[0]+texturePos[2])/2.;
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
	va[2]=vertices[1]+vertices[2];
	va[2].normalize();
	if (outTexturePos)
	{
		ta[0]=(texturePos[0]+texturePos[1])/2.;
		ta[1]=texturePos[1];
		ta[2]=(texturePos[1]+texturePos[2])/2.;
	}
	if (outEdgeFlags)
	{
		ba[0]=edgeFlags[0];
		ba[1]=edgeFlags[1];
		ba[2]=false;
	}
	projectSphericalTriangle(va, outVertices, ba, outEdgeFlags, ta, outTexturePos, nbI+1);
	
	va[0]=vertices[0]+vertices[2];
	va[0].normalize();
	//va[1]=vertices[1]+vertices[2];
	//va[1].normalize();
	va[1]=va[2];
	va[2]=vertices[2];
	if (outTexturePos)
	{
		ta[0]=(texturePos[0]+texturePos[2])/2.;
		ta[1]=(texturePos[1]+texturePos[2])/2.;
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

static QVector<Vec3d> polygonVertexArray;
static QVector<bool> polygonEdgeFlagArray;
static QVector<Vec2d> polygonTextureCoordArray;

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
		// Initilize to a point inside the convex polygon.
		triangle[2]= (a.size() == 3) ? a.at(0) + a.at(1) + a.at(2) : a.at(0) + a.at(a.size()/2);
		const bool tmpEdges[3] = {true, false, false};
		for (int i=0;i<a.size()-1;++i)
		{
			triangle[0]=a.at(i);
			triangle[1]=a.at(i+1);
			projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray);
		}
		// Last triangle
		triangle[0]=a.last();
		triangle[1]=a.first();
		projectSphericalTriangle(triangle, &polygonVertexArray, tmpEdges, &polygonEdgeFlagArray);
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
	
	// Load the textureCoordinates if any
	if (drawMode==SphericalPolygonDrawModeTextureFill || drawMode==SphericalPolygonDrawModeTextureFillAndBoundary)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_DOUBLE, 0, polygonTextureCoordArray.constData());
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
		//QVector<bool> temp;
		//temp.fill(true, polygonEdgeFlagArray.size());
		//glEdgeFlagPointer(0, temp.constData());
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
	
	glDisableClientState(GL_EDGE_FLAG_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
	glBegin(GL_LINE_LOOP);
	for (int i=0;i<=segments;i++)
	{
		glVertex2d(x+dx,y+dy);
		r = dx*cp-dy*sp;
		dy = dx*sp+dy*cp;
		dx = r;
	}
	glEnd();
}



/*************************************************************************
 Same function but gives the already projected 2d position in input
*************************************************************************/
void StelPainter::drawSprite2dMode(double x, double y, double size) const
{
	// Use GL_POINT_SPRITE_ARB extension if available
	if (flagGlPointSprite)
	{
		glPointSize(size);
		glBegin(GL_POINTS);
		glVertex2f(x,y);
		glEnd();
		return;
	}

	const double radius = size*0.5;
	glBegin(GL_QUADS );
	glTexCoord2i(0,0);
	glVertex2f(x-radius,y-radius);
	glTexCoord2i(1,0);
	glVertex2f(x+radius,y-radius);
	glTexCoord2i(1,1);
	glVertex2f(x+radius,y+radius);
	glTexCoord2i(0,1);
	glVertex2f(x-radius,y+radius);
	glEnd();
}

/*************************************************************************
 Same function but with a rotation angle
*************************************************************************/
void StelPainter::drawSprite2dMode(double x, double y, double size, double rotation) const
{
	glPushMatrix();
	glTranslatef(x, y, 0.0);
	glRotatef(rotation,0.,0.,1.);
	const double radius = size*0.5;
	glBegin(GL_QUADS );
	glTexCoord2i(0,0);
	glVertex2f(-radius,-radius);
	glTexCoord2i(1,0);
	glVertex2f(+radius,-radius);
	glTexCoord2i(1,1);
	glVertex2f(+radius,+radius);
	glTexCoord2i(0,1);
	glVertex2f(-radius,+radius);
	glEnd();
	glPopMatrix();
}

/*************************************************************************
 Draw a GL_POINT at the given position
*************************************************************************/
void StelPainter::drawPoint2d(double x, double y) const
{
	if (flagGlPointSprite)
	{
		glDisable(GL_POINT_SPRITE_ARB);
		glBegin(GL_POINTS);
			glVertex2f(x, y);
		glEnd();
		glEnable(GL_POINT_SPRITE_ARB);
		return;
	}
	
	glBegin(GL_POINTS);
		glVertex2f(x, y);
	glEnd();
}

/*************************************************************************
 Draw a rotated rectangle using the current texture at the given position
*************************************************************************/
void StelPainter::drawRectSprite2dMode(double x, double y, double sizex, double sizey, double rotation) const
{
	glPushMatrix();
	glTranslatef(x, y, 0.0);
	glRotatef(rotation,0.,0.,1.);
	const double radiusx = sizex*0.5;
	const double radiusy = sizey*0.5;
	glBegin(GL_QUADS );
		glTexCoord2i(0,0);
		glVertex2f(-radiusx,-radiusy);
		glTexCoord2i(1,0);
		glVertex2f(+radiusx,-radiusy);
		glTexCoord2i(1,1);
		glVertex2f(+radiusx,+radiusy);
		glTexCoord2i(0,1);
		glVertex2f(-radiusx,+radiusy);
	glEnd();
	glPopMatrix();
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
	for (i = 0,cos_sin_rho_p = cos_sin_rho; i < stacks; ++i,cos_sin_rho_p+=2)
	{
		glBegin(GL_QUAD_STRIP);
		s = 0.0;
		for (j = 0,cos_sin_theta_p = cos_sin_theta; j <= slices;++j,cos_sin_theta_p+=2)
		{
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
			z = nsign * cos_sin_rho_p[0];
			glTexCoord2f(s, t);
			if (isLightOn)
			{
				c = nsign * lightPos3.dot(Vec3f(x * oneMinusOblateness, y * oneMinusOblateness, z));
				if (c<0) {c=0;}
				glColor3f(c*diffuseLight[0] + ambientLight[0],
				          c*diffuseLight[1] + ambientLight[1],
				          c*diffuseLight[2] + ambientLight[2]);
			}
			drawVertex3(x * radius, y * radius, z * oneMinusOblateness * radius);
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
			z = nsign * cos_sin_rho_p[2];
			glTexCoord2f(s, t - dt);
			if (isLightOn)
			{
				c = nsign * lightPos3.dot(Vec3f(x * oneMinusOblateness,y * oneMinusOblateness,z));
				if (c<0) {c=0;}
				glColor3f(c*diffuseLight[0] + ambientLight[0],
				          c*diffuseLight[1] + ambientLight[1],
				          c*diffuseLight[2] + ambientLight[2]);
			}
			drawVertex3(x * radius, y * radius, z * oneMinusOblateness * radius);
			s += ds;
		}
		glEnd();
		t -= dt;
	}

	if (isLightOn)
		glEnable(GL_LIGHTING);
}

// Reimplementation of gluCylinder : glu is overrided for non standard projection
void StelPainter::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orientInside) const
{
	static GLdouble da, r, dz;
	static GLfloat z, nsign;
	static GLint i, j;

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
	for (j = 0; j < stacks; j++)
	{
		GLfloat s = 0.0;
		glBegin(GL_QUAD_STRIP);
		for (i = 0; i <= slices; i++)
		{
			GLfloat x, y;
			if (i == slices)
			{
				x = std::sin(0.0f);
				y = std::cos(0.0f);
			}
			else
			{
				x = std::sin(i * da);
				y = std::cos(i * da);
			}
			glNormal3f(x * nsign, y * nsign, 0);
			glTexCoord2f(s, t);
			drawVertex3(x * r, y * r, z);
			glNormal3f(x * nsign, y * nsign, 0);
			glTexCoord2f(s, t + dt);
			drawVertex3(x * r, y * r, z + dz);
			s += ds;
		}			/* for slices */
		glEnd();
		t += dt;
		z += dz;
	}				/* for stacks */

	if (orientInside)
		glCullFace(GL_BACK);
}
