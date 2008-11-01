/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#include <map>
#include "GLee.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#else
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif

#include "Projector.hpp"
#include "MappingClasses.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "SFont.hpp"

#include <QDebug>
#include <QString>
#include <QSettings>

#include <QPainter>

const QString Projector::maskTypeToString(ProjectorMaskType type)
{
	if (type == Disk )
		return "disk";
	else
		return "none";
}

Projector::ProjectorMaskType Projector::stringToMaskType(const QString &s)
{
	if (s=="disk")
		return Disk;
	return None;
}

void Projector::registerProjectionMapping(Mapping *c)
{
	if (c) projectionMapping[c->getId()] = c;
}

void Projector::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	// Video Section
	QString tmpstr = conf->value("projection/viewport").toString();
	const Projector::ProjectorMaskType projMaskType = Projector::stringToMaskType(tmpstr);
	setMaskType(projMaskType);
	const bool maximized = (tmpstr=="maximized");
	const int screen_w = conf->value("video/screen_w", 800).toInt();
	const int screen_h = conf->value("video/screen_h", 600).toInt();
	const int screen_min_wh = qMin(screen_w,screen_h);
	const int viewport_width = conf->value("projection/viewport_width", maximized ? screen_w : screen_min_wh).toInt();
	const int viewport_height = conf->value("projection/viewport_height", maximized ? screen_h : screen_min_wh).toInt();
	const int viewport_x = conf->value("projection/viewport_x", (screen_w-viewport_width)/2).toInt();
	const int viewport_y = conf->value("projection/viewport_y", (screen_h-viewport_height)/2).toInt();
	const double viewportCenterX = conf->value("projection/viewport_center_x",0.5*viewport_width).toDouble();
	const double viewportCenterY = conf->value("projection/viewport_center_y",0.5*viewport_height).toDouble();
	const double viewportFovDiameter = conf->value("projection/viewport_fov_diameter", qMin(viewport_width,viewport_height)).toDouble();
	setViewport(viewport_x,viewport_y,viewport_width,viewport_height, viewportCenterX,viewportCenterY, viewportFovDiameter);

	double overwrite_max_fov = conf->value("projection/equal_area_max_fov",0.0).toDouble();
	if (overwrite_max_fov > 360.0)
		overwrite_max_fov = 360.0;
	if (overwrite_max_fov > 0.0)
		MappingEqualArea::getMapping()->maxFov = overwrite_max_fov;
	overwrite_max_fov = conf->value("projection/stereographic_max_fov",0.0).toDouble();
	if (overwrite_max_fov > 359.999999)
		overwrite_max_fov = 359.999999;
	if (overwrite_max_fov > 0.0)
		MappingStereographic::getMapping()->maxFov = overwrite_max_fov;
	overwrite_max_fov = conf->value("projection/fisheye_max_fov",0.0).toDouble();
	if (overwrite_max_fov > 360.0)
		overwrite_max_fov = 360.0;
	if (overwrite_max_fov > 0.0)
		MappingFisheye::getMapping()->maxFov = overwrite_max_fov;
	overwrite_max_fov = conf->value("projection/cylinder_max_fov",0.0).toDouble();
	if (overwrite_max_fov > 540.0)
		overwrite_max_fov = 540.0;
	if (overwrite_max_fov > 0.0)
		MappingCylinder::getMapping()->maxFov = overwrite_max_fov;
	overwrite_max_fov = conf->value("projection/perspective_max_fov",0.0).toDouble();
	if (overwrite_max_fov > 179.999999)
		overwrite_max_fov = 179.999999;
	if (overwrite_max_fov > 0.0)
		MappingPerspective::getMapping()->maxFov = overwrite_max_fov;
	overwrite_max_fov = conf->value("projection/orthographic_max_fov",0.0).toDouble();
	if (overwrite_max_fov > 180.0)
		overwrite_max_fov = 180.0;
	if (overwrite_max_fov > 0.0)
		MappingOrthographic::getMapping()->maxFov = overwrite_max_fov;

	setFlagGravityLabels( conf->value("viewing/flag_gravity_labels").toBool() );

	// Register the default mappings
	registerProjectionMapping(MappingEqualArea::getMapping());
	registerProjectionMapping(MappingStereographic::getMapping());
	registerProjectionMapping(MappingFisheye::getMapping());
	registerProjectionMapping(MappingCylinder::getMapping());
	registerProjectionMapping(MappingMercator::getMapping());
	registerProjectionMapping(MappingPerspective::getMapping());
	registerProjectionMapping(MappingOrthographic::getMapping());

	tmpstr = conf->value("projection/type", "stereographic").toString();
	setCurrentMapping(tmpstr);

	setInitFov(conf->value("navigation/init_fov",60.).toDouble());
	setFov(initFov);

	//glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW);

	flagGlPointSprite = conf->value("projection/flag_use_gl_point_sprite",false).toBool();
	flagGlPointSprite = flagGlPointSprite && GLEE_ARB_point_sprite;
	if (flagGlPointSprite)
	{
		glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );
		glEnable(GL_POINT_SPRITE_ARB);
		glEnable(GL_POINT_SMOOTH);
		qDebug() << "INFO: using GL_ARB_point_sprite";
	} else {
		//qDebug() << "WARNING: GL_ARB_point_sprite not available";
	}
}

void Projector::windowHasBeenResized(int width,int height)
{
	// Maximize display when resized since it invalidates previous options anyway
	setViewport(0,0,width,height, 0.5*width, 0.5*height,qMin(width,height));
}

Projector::Projector(const Vector4<GLint>& viewport, double _fov)
		:maskType(None), fov(_fov), minFov(0.0001), maxFov(100),
		zNear(0.1), zFar(10000),
		viewportXywh(viewport),
        viewportCenter(Vec2d(viewportXywh[0]+0.5*viewportXywh[2],
		                      viewportXywh[1]+0.5*viewportXywh[3])),
        viewportFovDiameter(qMin(viewportXywh[2],viewportXywh[3])),
		gravityLabels(0),
		mapping(NULL)
{
	flipHorz = 1.0;
	flipVert = 1.0;
	setViewport(viewportXywh[0],viewportXywh[1],
	            viewportXywh[2],viewportXywh[3],
	            viewportCenter[0]-viewportXywh[0],
                viewportCenter[1]-viewportXywh[1],
	            viewportFovDiameter);

	setFov(_fov);
}

Projector::~Projector()
{}


void Projector::setViewport(int x, int y, int w, int h,
                            double cx,double cy,double fovDiam)
{
	viewportXywh[0] = x;
	viewportXywh[1] = y;
	viewportXywh[2] = w;
	viewportXywh[3] = h;
	viewportCenter[0] = x+cx;
	viewportCenter[1] = y+cy;
	viewportFovDiameter = fovDiam;
	pixelPerRad = 0.5 * viewportFovDiameter
	  / (mapping ? mapping->fovToViewScalingFactor(fov*(M_PI/360.0)) : 1.0);
	glViewport(x, y, w, h);
	initGlMatrixOrtho2d();
}

/*************************************************************************
 Return a polygon matching precisely the real viewport defined by the area on the screen 
 where projection is valid.
*************************************************************************/
QList<Vec2d> Projector::getViewportVertices2d() const
{
	QList<Vec2d> result;
	result.push_back(Vec2d(viewportXywh[0], viewportXywh[1]));
	result.push_back(Vec2d(viewportXywh[0]+viewportXywh[2], viewportXywh[1]));
	result.push_back(Vec2d(viewportXywh[0]+viewportXywh[2], viewportXywh[1]+viewportXywh[3]));
	result.push_back(Vec2d(viewportXywh[0], viewportXywh[1]+viewportXywh[3]));
	return result;
}

/*************************************************************************
 Return a convex polygon on the sphere which includes the viewport in the 
 current frame
*************************************************************************/
StelGeom::ConvexPolygon Projector::getViewportConvexPolygon(double marginX, double marginY) const
{
	Vec3d e0, e1, e2, e3;
	unProject(0-marginX,0-marginY,e0);
	unProject(getViewportWidth()+marginX,0-marginY,e1);
	unProject(getViewportWidth()+marginX,getViewportHeight()+marginY,e2);
	unProject(0-marginX,getViewportHeight()+marginY,e3);
	e0.normalize();
	e1.normalize();
	e2.normalize();
	e3.normalize();
	return needGlFrontFaceCW() ? StelGeom::ConvexPolygon(e1, e0, e3, e2) : StelGeom::ConvexPolygon(e0, e1, e2, e3);
}

StelGeom::ConvexS Projector::unprojectViewport(void) const {
    // This is quite ugly, but already better than nothing.
    // In fact this function should have different implementations
    // for the different mapping types. And maskType, viewportFovDiameter,
    // viewportCenter, viewportXywh must be taken into account, too.
    // Last not least all halfplanes n*x>d really should have d<=0
    // or at least very small d/n.length().
  if ((dynamic_cast<const MappingCylinder*>(mapping) == 0 || fov < 90) &&
      fov < 360.0) {
    Vec3d e0,e1,e2,e3;
    bool ok;
    if (maskType == Disk) {
      if (fov >= 120.0) {
        unProject(viewportCenter[0],viewportCenter[1],e0);
        StelGeom::ConvexS rval(1);
        rval[0].n = e0;
        rval[0].d = (fov<360.0) ? cos(fov*(M_PI/360.0)) : -1.0;
        return rval;
      }
	  ok  = unProject(viewportCenter[0] - 0.5*viewportFovDiameter,
                      viewportCenter[1] - 0.5*viewportFovDiameter,e0);
	  ok &= unProject(viewportCenter[0] + 0.5*viewportFovDiameter,
                      viewportCenter[1] + 0.5*viewportFovDiameter,e2);
	  if (needGlFrontFaceCW()) {
        ok &= unProject(viewportCenter[0] - 0.5*viewportFovDiameter,
                        viewportCenter[1] + 0.5*viewportFovDiameter,e3);
        ok &= unProject(viewportCenter[0] + 0.5*viewportFovDiameter,
                        viewportCenter[1] - 0.5*viewportFovDiameter,e1);
	  } else {
        ok &= unProject(viewportCenter[0] - 0.5*viewportFovDiameter,
                        viewportCenter[1] + 0.5*viewportFovDiameter,e1);
        ok &= unProject(viewportCenter[0] + 0.5*viewportFovDiameter,
                        viewportCenter[1] - 0.5*viewportFovDiameter,e3);
	  }
    } else {
      ok  = unProject(viewportXywh[0],viewportXywh[1],e0);
      ok &= unProject(viewportXywh[0]+viewportXywh[2],
                      viewportXywh[1]+viewportXywh[3],e2);
      if (needGlFrontFaceCW()) {
        ok &= unProject(viewportXywh[0],viewportXywh[1]+viewportXywh[3],e3);
        ok &= unProject(viewportXywh[0]+viewportXywh[2],viewportXywh[1],e1);
      } else {
        ok &= unProject(viewportXywh[0],viewportXywh[1]+viewportXywh[3],e1);
        ok &= unProject(viewportXywh[0]+viewportXywh[2],viewportXywh[1],e3);
      }
    }
    if (ok) {
      StelGeom::HalfSpace h0(e0^e1);
      StelGeom::HalfSpace h1(e1^e2);
      StelGeom::HalfSpace h2(e2^e3);
      StelGeom::HalfSpace h3(e3^e0);
      if (h0.contains(e2) && h0.contains(e3) &&
          h1.contains(e3) && h1.contains(e0) &&
          h2.contains(e0) && h2.contains(e1) &&
          h3.contains(e1) && h3.contains(e2)) {
        StelGeom::ConvexS rval(4);
        rval[0] = h3;
        rval[1] = h2;
        rval[2] = h1;
        rval[3] = h0;
        return rval;
      } else {
        Vec3d middle;
        if (unProject(viewportXywh[0]+0.5*viewportXywh[2],
                      viewportXywh[1]+0.5*viewportXywh[3],middle)) {
          double d = middle*e0;
          double h = middle*e1;
          if (d > h) d = h;
          h = middle*e2;
          if (d > h) d = h;
          h = middle*e3;
          if (d > h) d = h;
          StelGeom::ConvexS rval(1);
          rval[0].n = middle;
          rval[0].d = d;
          return rval;
        }
      }
    }
  }
  StelGeom::ConvexS rval(1);
  rval[0].n = Vec3d(1.0,0.0,0.0);
  rval[0].d = -2.0;
  return rval;
}

void Projector::setFov(double f)
{
	fov = f;
	if (f>maxFov)
		fov = maxFov;
	if (f<minFov)
		fov = minFov;
	pixelPerRad = 0.5 * viewportFovDiameter / (mapping ? mapping->fovToViewScalingFactor(fov*(M_PI/360.0)) : 1.0);
}


void Projector::setMaxFov(double max)
{
	maxFov = max;
	if (fov > max)
	{
		setFov(max);
	}
}

void Projector::setClippingPlanes(double znear, double zfar)
{
	zNear = znear;
	zFar = zfar;
}

// Set the standard modelview matrices used for projection
void Projector::setModelviewMatrices(	const Mat4d& _matEarthEquToEye,
                                        const Mat4d& _matHeliocentricEclipticToEye,
                                        const Mat4d& _matAltAzToEye,
                                        const Mat4d& _matJ2000ToEye)
{
	matEarthEquToEye = _matEarthEquToEye;
	matJ2000ToEye = _matJ2000ToEye;
	matHeliocentricEclipticToEye = _matHeliocentricEclipticToEye;
	matAltAzToEye = _matAltAzToEye;
}


/*************************************************************************
 Set the frame in which we want to draw from now on
*************************************************************************/
void Projector::setCurrentFrame(FrameType frameType) const
{
	switch (frameType)
	{
	case FrameLocal:
		setCustomFrame(matAltAzToEye);
		break;
	case FrameHelio:
		setCustomFrame(matHeliocentricEclipticToEye);
		break;
	case FrameEarthEqu:
		setCustomFrame(matEarthEquToEye);
		break;
	case FrameJ2000:
		setCustomFrame(matJ2000ToEye);
		break;
	default:
		qDebug() << "Unknown reference frame type: " << (int)frameType << ".";
	}
}

/*************************************************************************
 Set a custom model view matrix, it is valid until the next call to 
 setCurrentFrame or setCustomFrame
*************************************************************************/
void Projector::setCustomFrame(const Mat4d& m) const
{
	modelViewMatrix = m;
}

/*************************************************************************
 Init the real openGL Matrices to a 2d orthographic projection
*************************************************************************/
void Projector::initGlMatrixOrtho2d(void) const
{
	// Set the real openGL projection and modelview matrix to orthographic projection
	// thus we never need to change to 2dMode from now on before drawing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(viewportXywh[0], viewportXywh[0] + viewportXywh[2],
	        viewportXywh[1], viewportXywh[1] + viewportXywh[3], -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


/*************************************************************************
 Set the current projection mapping to use
*************************************************************************/
void Projector::setCurrentMapping(const QString& mappingId)
{
	if (currentProjectionType==mappingId)
		return;

	QMap<QString,const Mapping*>::const_iterator i(projectionMapping.find(mappingId));
	if (i!=projectionMapping.end())
	{
		currentProjectionType = mappingId;

		// Redefine the projection functions
		mapping = i.value();
		minFov = mapping->minFov;
		maxFov = mapping->maxFov;

		setFov(fov);
		initGlMatrixOrtho2d();
	}
	else
	{
		qWarning() << "Unknown projection type: " << qPrintable(mappingId) << "setting \"stereographic\" instead";
		setCurrentMapping("stereographic");
	}
}


/*************************************************************************
 Project the vector v from the viewport frame into the current frame 
*************************************************************************/
bool Projector::unProject(double x, double y, Vec3d &v) const
{
	v[0] = flipHorz * (x - viewportCenter[0]) / pixelPerRad;
	v[1] = flipVert * (y - viewportCenter[1]) / pixelPerRad;
	v[2] = 0;
	const bool rval = mapping->backward(v);
	  // Even when the reprojected point comes from an region of the screen,
	  // where nothing is projected to (rval=false), we finish reprojecting.
	  // This looks good for atmosphere rendering, and it helps avoiding
	  // discontinuities when dragging around with the mouse.

	x = v[0] - modelViewMatrix.r[12];
	y = v[1] - modelViewMatrix.r[13];
	const double z = v[2] - modelViewMatrix.r[14];
	v[0] = modelViewMatrix.r[0]*x + modelViewMatrix.r[1]*y + modelViewMatrix.r[2]*z;
	v[1] = modelViewMatrix.r[4]*x + modelViewMatrix.r[5]*y + modelViewMatrix.r[6]*z;
	v[2] = modelViewMatrix.r[8]*x + modelViewMatrix.r[9]*y + modelViewMatrix.r[10]*z;

// Johannes: Get rid of the inverseModelViewMatrix.
// We need no matrix inversion because we always work with orthogonal matrices
// (where the transposed is the inverse).
//	v.transfo4d(inverseModelViewMatrix);
	return rval;
}




///////////////////////////////////////////////////////////////////////////
// Standard methods for drawing primitives

// Fill with black around the circle
void Projector::drawViewportShape(void)
{
	if (maskType != Disk)
		return;

	glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	glPushMatrix();
	glTranslated(viewportCenter[0],viewportCenter[1],0.0);
	GLUquadricObj * p = gluNewQuadric();
	gluDisk(p, 0.5*viewportFovDiameter,
               getViewportWidth()+getViewportHeight(), 256, 1);  // should always cover whole screen
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


void Projector::sFanDisk(double radius,int innerFanSlices,int level) const {
  Q_ASSERT(level<64);
  double rad[64];
  int i,j;
  rad[level] = radius;
//  for (i=level-1;i>=0;i--) {
//    double f = ((i+1)/(double)(level+1));
//    rad[i] = radius*f*f;
//  }
  for (i=level-1;i>=0;i--) {
    rad[i] = rad[i+1]*(1.0-M_PI/(innerFanSlices<<(i+1)))*2.0/3.0;
  }
  int slices = innerFanSlices<<level;
  const double dtheta = 2.0 * M_PI / slices;
  Q_ASSERT(slices<=MAX_SLICES);
  ComputeCosSinTheta(dtheta,slices);
  double *cos_sin_theta_p;
  int slices_step = 2;
  for (i=level;i>0;i--,slices_step<<=1) {
    for (j=0,cos_sin_theta_p=cos_sin_theta;
         j<slices;
         j+=slices_step,cos_sin_theta_p+=2*slices_step) {
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
  for (j=0,cos_sin_theta_p=cos_sin_theta;
       j<=slices;
       j+=slices_step,cos_sin_theta_p+=2*slices_step) {
    double x = rad[0]*cos_sin_theta_p[0];
    double y = rad[0]*cos_sin_theta_p[1];
    glTexCoord2d(0.5*(1.0+x/radius),0.5*(1.0+y/radius));
    drawVertex3(x,y,0);
  }
  glEnd();
}


// Draw a disk with a special texturing mode having texture center at disk center
void Projector::sDisk(GLdouble radius, GLint slices, GLint stacks, int orientInside) const
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

void Projector::sRing(GLdouble rMin, GLdouble rMax, GLint slices, GLint stacks, int orientInside) const
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

void Projector::sSphereMap(GLdouble radius, GLint slices, GLint stacks,
                            double textureFov, int orientInside) const
{
	double rho,x,y,z;
	int i, j;
	const double nsign = orientInside?-1:1;

	const double drho = M_PI / stacks;
	Q_ASSERT(stacks<=MAX_STACKS);
	ComputeCosSinRho(drho,stacks);
	double *cos_sin_rho_p;

	const double dtheta = 2.0 * M_PI / slices;
	Q_ASSERT(slices<=MAX_SLICES);

	ComputeCosSinTheta(dtheta,slices);
	double *cos_sin_theta_p;


	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
	// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)

	const int imax = stacks;

	// draw intermediate stacks as quad strips
	if (!orientInside) // nsign==1
	{
		for (i = 0,cos_sin_rho_p=cos_sin_rho,rho=0.0;
		        i < imax; ++i,cos_sin_rho_p+=2,rho+=drho)
		{
			glBegin(GL_QUAD_STRIP);
			for (j=0,cos_sin_theta_p=cos_sin_theta;
			     j<=slices;++j,cos_sin_theta_p+=2)
			{
				x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
				z = cos_sin_rho_p[0];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho/textureFov,
				                       cos_sin_theta_p[0],
				                       cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);

				x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
				z = cos_sin_rho_p[2];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast((rho + drho)/textureFov,
				                       cos_sin_theta_p[0],
				                       cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);
			}
			glEnd();
		}
	}
	else
	{
		for (i = 0,cos_sin_rho_p=cos_sin_rho,rho=0.0;
		        i < imax; ++i,cos_sin_rho_p+=2,rho+=drho)
		{
			glBegin(GL_QUAD_STRIP);
			for (j=0,cos_sin_theta_p=cos_sin_theta;
			     j<=slices;++j,cos_sin_theta_p+=2)
			{
				x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
				z = cos_sin_rho_p[2];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast((rho + drho)/textureFov,
				                       cos_sin_theta_p[0],
				                       -cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);

				x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
				z = cos_sin_rho_p[0];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho/textureFov,
				                       cos_sin_theta_p[0],
				                       -cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);
			}
			glEnd();
		}
	}
}

void Projector::drawTextGravity180(const SFont* font, float x, float y, const QString& ws,
                                   bool speedOptimize, float xshift, float yshift) const
{
	static float dx, dy, d, theta, psi;
	dx = x - viewportCenter[0];
	dy = y - viewportCenter[1];
	d = sqrt(dx*dx + dy*dy);

	// If the text is too far away to be visible in the screen return
	if (d>qMax(viewportXywh[3], viewportXywh[2])*2)
		return;

	theta = M_PI + std::atan2(dx, dy - 1);
	psi = std::atan2((float)font->getStrLen(ws)/ws.length(),d + 1) * 180./M_PI;

	if (psi>5)
		psi = 5;
	
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushMatrix();
	glTranslatef(x,y,0);
	if(gravityLabels)
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
void Projector::drawText(const SFont* font, float x, float y, const QString& str, float angleDeg, float xshift, float yshift, bool noGravity) const
{
	if (gravityLabels && !noGravity)
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

/*************************************************************************
 Draw a small circle arc in the current frame
*************************************************************************/
void Projector::drawSmallCircleArc(const Vec3d& start, const Vec3d& rotAxis, double length, void (*viewportEdgeIntersectCallback)(double angleVal, const Vec3d& screenPos, const Vec3d& direction, bool enters))
{
	Q_ASSERT(viewportEdgeIntersectCallback==NULL); // TODO
	const double nbSeg = 4 + (int)(length*44./(2.*M_PI));
	const Mat4d& dRot = Mat4d::rotation(rotAxis, length/nbSeg);
	Vec3d v(start);
	Vec3d pt1;
	bool res1;
	bool res2=true;
	glBegin(GL_LINE_STRIP);
	for (int i=0;i<=nbSeg;++i)
	{
		res1 = project(v, pt1);
		const bool toDraw = !(res2==false && res1==false);
		res2=res1;
		if (toDraw)
			glVertex2f(pt1[0],pt1[1]);
		else
		{
			glEnd();
			glBegin(GL_LINE_STRIP);
		}
		dRot.transfo(v);
	}
	glEnd();
}

/*************************************************************************
 Draw a parallel arc in the current frame
*************************************************************************/
void Projector::drawParallel(const Vec3d& start, double length, bool labelAxis, const SFont* font, const Vec4f* textColor, int nbSeg) const
{
	if (nbSeg==-1)
		nbSeg = 4 + (int)(length*44./(2.*M_PI));
	const Mat4d dRa = Mat4d::zrotation(length/nbSeg);

	Vec3d v(start);
	Vec3d pt1;
	bool res1;
	bool res2=true;
	glBegin(GL_LINE_STRIP);
	for (int i=0;i<=nbSeg;++i)
	{
		res1 = project(v, pt1);
		const bool toDraw = !(res2==false && res1==false);
		res2=res1;
		if (toDraw)
			glVertex2f(pt1[0],pt1[1]);
		else
		{
			glEnd();
			glBegin(GL_LINE_STRIP);
		}
		dRa.transfo(v);
	}
	glEnd();

	// Draw label if needed
	if (labelAxis)
	{
		static GLfloat tmpColor[4];
		if (textColor)
		{
			glGetFloatv(GL_CURRENT_COLOR, tmpColor);
			glColor4fv(*textColor);
		}
		double lon, lat;
		StelUtils::rectToSphe(&lon, &lat, start);
		Vec3d win0, win1;
		Vec3d v1, v2;
		StelUtils::spheToRect(lon+0.0000001, lat, v2);
		project(start, win0);
		project(v2, win1);
		double angleDeg = std::atan2(win1[1]-win0[1], win1[0]-win0[0])*180./M_PI;
		const QString str = StelUtils::radToDmsStrAdapt(lat);
		float xshift=5;
		if (angleDeg>90. || angleDeg<-90.)
		{
			angleDeg+=180.;
			xshift=-font->getStrLen(str)-5.f;
		}
		drawText(font, win1[0], win1[1], str, angleDeg, xshift, 3);

		// Label at end of the arc
		StelUtils::spheToRect(lon+length-0.0000001, lat, v2);
		project(v, win0);
		project(v2, win1);
		angleDeg = std::atan2(win1[1]-win0[1], win1[0]-win0[0])*180./M_PI;
		xshift=5;
		if (angleDeg>90. || angleDeg<-90.)
		{
			angleDeg+=180.;
			xshift=-font->getStrLen(str)-5.f;
		}
		drawText(font, win1[0], win1[1], str, angleDeg, xshift, 3);
		
		if (textColor)
			glColor4fv(tmpColor);
	}
}

/*************************************************************************
 Draw a meridian arc in the current frame
*************************************************************************/
void Projector::drawMeridian(const Vec3d& start, double length, bool labelAxis, const SFont* font, const Vec4f* textColor, int nbSeg, bool useDMS) const
{
	if (nbSeg==-1)
		nbSeg = 4 + (int)(length*54./(2.*M_PI));
	static const Vec3d oneZ(0,0,1);
	const Mat4d dDe = Mat4d::rotation(start^oneZ, (start[1]>=0 ? 1.:-1.) * length/nbSeg);

	Vec3d v(start);
	Vec3d pt1;
	bool res1;
	bool res2=true;
	glBegin(GL_LINE_STRIP);
	for (int i=0;i<=nbSeg;++i)
	{
		res1 = project(v, pt1);
		const bool toDraw = !(res2==false && res1==false);
		res2=res1;
		if (toDraw)
			glVertex2f(pt1[0],pt1[1]);
		else
		{
			glEnd();
			glBegin(GL_LINE_STRIP);
		}
		dDe.transfo(v);
	}
	glEnd();

	// Draw label if needed
	if (labelAxis)
	{
		static GLfloat tmpColor[4];
		if (textColor)
		{
			glGetFloatv(GL_CURRENT_COLOR, tmpColor);
			glColor4fv(*textColor);
		}
		double lon, lat;
		StelUtils::rectToSphe(&lon, &lat, start);
		Vec3d win0, win1;
		Vec3d v2;
		StelUtils::spheToRect(lon, lat+0.0000001*(start[1]>=0 ? 1.:-1.), v2);
		project(start, win0);
		project(v2, win1);
		double angleDeg = std::atan2(win1[1]-win0[1], win1[0]-win0[0])*180./M_PI;
		//angleDeg += start[1]>=0 ? 1.:180.;
		QString str = useDMS ? StelUtils::radToDmsStrAdapt(M_PI-lon) : StelUtils::radToHmsStrAdapt(lon);
		float xshift=20;
		if (angleDeg>90. || angleDeg<-90.)
		{
			angleDeg+=180.;
			xshift=-font->getStrLen(str)-20.f;
		}
		drawText(font, win1[0], win1[1], str, angleDeg, xshift, 3);

		// Label at end of the arc
		StelUtils::spheToRect(lon, lat+(length-0.0000001)*(start[1]>=0 ? 1.:-1.), v2);
		project(v, win0);
		project(v2, win1);
		angleDeg = std::atan2(win1[1]-win0[1], win1[0]-win0[0])*180./M_PI;
		xshift=20;
		if (angleDeg>90. || angleDeg<-90.)
		{
			angleDeg+=180.;
			xshift=-font->getStrLen(str)-20.f;
		}
		StelUtils::rectToSphe(&lon, &lat, v);
		str = useDMS ? StelUtils::radToDmsStrAdapt(M_PI-lon) : StelUtils::radToHmsStrAdapt(lon);
		drawText(font, win1[0], win1[1], str, angleDeg, xshift, 3);
		
		if (textColor)
			glColor4fv(tmpColor);
	}
}


/*************************************************************************
 draw a simple circle, 2d viewport coordinates in pixel
*************************************************************************/
void Projector::drawCircle(double x,double y,double r) const {
  if (r <= 1.0) return;
  const Vec2d center(x,y);
  const Vec2d v_center(0.5*viewportXywh[2],
                       0.5*viewportXywh[3]);
  const double R = v_center.length();
  const double d = (v_center-center).length();
  if (d > r+R || d < r-R) return;
  const int segments = 180;
  const double phi = 2.0*M_PI/segments;
  const double cp = cos(phi);
  const double sp = sin(phi);
  double dx = r;
  double dy = 0;
  glBegin(GL_LINE_LOOP);
  for (int i=0;i<=segments;i++) {
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
void Projector::drawSprite2dMode(double x, double y, double size) const
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
void Projector::drawSprite2dMode(double x, double y, double size, double rotation) const
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
 Draw the given polygon
*************************************************************************/
void Projector::drawPolygon(const StelGeom::Polygon& poly) const
{
	const int size = poly.size();
	if (size<3)
		return;

	Vec3d win;	
	glBegin(GL_LINE_LOOP);
	for (int i=0;i<size;++i)
	{
		project(poly[i], win);
		glVertex3dv(win);
	}
	glEnd();
}
	

/*************************************************************************
 Draw a GL_POINT at the given position
*************************************************************************/
void Projector::drawPoint2d(double x, double y) const
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
void Projector::drawRectSprite2dMode(double x, double y, double sizex, double sizey, double rotation) const
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

void Projector::sSphere(GLdouble radius, GLdouble oneMinusOblateness,
                        GLint slices, GLint stacks, int orientInside) const
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
		lightPos3 -= modelViewMatrix * Vec3d(0.,0.,0.); // -posCenterEye
		lightPos3 = modelViewMatrix.transpose().multiplyWithoutTranslation(lightPos3);
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
	for (i = 0,cos_sin_rho_p = cos_sin_rho; i < stacks;
	     i++,cos_sin_rho_p+=2)
	{
		glBegin(GL_QUAD_STRIP);
		s = 0.0;
		for (j = 0,cos_sin_theta_p = cos_sin_theta; j <= slices;j++,cos_sin_theta_p+=2)
		{
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
			z = nsign * cos_sin_rho_p[0];
			glTexCoord2f(s, t);
			if (isLightOn)
			{
				c = nsign * lightPos3.dot(Vec3f(x * oneMinusOblateness,
				                                y * oneMinusOblateness,
				                                z));
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
				c = nsign * lightPos3.dot(Vec3f(x * oneMinusOblateness,
				                                y * oneMinusOblateness,
				                                z));
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
void Projector::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orientInside) const
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
