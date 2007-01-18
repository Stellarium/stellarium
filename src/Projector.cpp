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

#include <iostream>
#include <cstdio>
#include <cassert>
#include "Projector.hpp"
#include "FisheyeProjector.hpp"
#include "CylinderProjector.hpp"
#include "StereographicProjector.hpp"
#include "SphericMirrorProjector.hpp"
#include "InitParser.hpp"

const char *Projector::typeToString(PROJECTOR_TYPE type) {
  switch (type) {
    case PERSPECTIVE_PROJECTOR:    return "perspective";
    case FISHEYE_PROJECTOR:        return "fisheye";
    case CYLINDER_PROJECTOR:       return "cylinder";
    case STEREOGRAPHIC_PROJECTOR:  return "stereographic";
    case SPHERIC_MIRROR_PROJECTOR: return "spheric_mirror";
  }
  cerr << "fatal: Projector::typeToString(" << type << ") failed" << endl;
  assert(0);
    // just shutup the compiler, this point will never be reached
  return 0;
}

Projector::PROJECTOR_TYPE Projector::stringToType(const string &s) {
  if (s=="perspective")    return PERSPECTIVE_PROJECTOR;
  if (s=="fisheye")        return FISHEYE_PROJECTOR;
  if (s=="cylinder")       return CYLINDER_PROJECTOR;
  if (s=="stereographic")  return STEREOGRAPHIC_PROJECTOR;
  if (s=="spheric_mirror") return SPHERIC_MIRROR_PROJECTOR;
  cerr << "fatal: Projector::stringToType(" << s << ") failed" << endl;
  assert(0);
    // just shutup the compiler, this point will never be reached
  return PERSPECTIVE_PROJECTOR;
}

const char *Projector::maskTypeToString(PROJECTOR_MASK_TYPE type) {
  if(type == DISK ) return "disk";
  else return "none";
}

Projector::PROJECTOR_MASK_TYPE Projector::stringToMaskType(const string &s) {
  if (s=="disk")    return DISK;
  return NONE;
}


Projector *Projector::create(PROJECTOR_TYPE type,
                             const Vec4i& viewport,
                             double _fov) {
  Projector *rval = 0;
  switch (type) {
    case PERSPECTIVE_PROJECTOR:
      rval = new Projector(viewport,_fov);
      break;
    case FISHEYE_PROJECTOR:
      rval = new FisheyeProjector(viewport,_fov);
      break;
    case CYLINDER_PROJECTOR:
      rval = new CylinderProjector(viewport,_fov);
      break;
    case STEREOGRAPHIC_PROJECTOR:
      rval = new StereographicProjector(viewport,_fov);
      break;
    case SPHERIC_MIRROR_PROJECTOR:
      rval = new SphericMirrorProjector(viewport,_fov);
      break;
  }
  if (rval == 0) {
    cerr << "fatal: Projector::create(" << type << ") failed" << endl;
    exit(1);
  }
    // just shutup the compiler, this point will never be reached
  return rval;
}

void Projector::init(const InitParser& conf)
{
	// Video Section
	setViewportSize(conf.get_int("video:screen_w"), conf.get_int("video:screen_h"));
	setViewportPosX(conf.get_int    ("video:horizontal_offset"));
	setViewportPosY(conf.get_int    ("video:vertical_offset"));
	
	string tmpstr = conf.get_str("projection:viewport");
	const Projector::PROJECTOR_MASK_TYPE projMaskType = Projector::stringToMaskType(tmpstr);
	setMaskType(projMaskType);
	initFov	= conf.get_double ("navigation","init_fov",60.);
	setFov(initFov);
	setFlagGravityLabels( conf.get_boolean("viewing:flag_gravity_labels") );
	
	tmpstr = conf.get_str("projection:viewport");
	if (tmpstr=="maximized") setMaximizedViewport(getViewportWidth(),  getViewportHeight());
	else
		if (tmpstr=="square" || tmpstr=="disk")
		{
			setSquareViewport(getViewportWidth(), getViewportHeight(), conf.get_int("video:horizontal_offset"), conf.get_int("video:horizontal_offset"));
			if (tmpstr=="disk") setViewportMaskDisk();
		}
		else
		{
			cerr << "ERROR : Unknown viewport type : " << tmpstr << endl;
			exit(-1);
		}
}


Projector::Projector(const Vec4i& viewport, double _fov)
          :maskType(NONE), fov(1.0), min_fov(0.0001), max_fov(100),
           zNear(0.1), zFar(10000),
           vec_viewport(viewport),
           gravityLabels(0)
{
	flip_horz = 1.0;
	flip_vert = 1.0;
	setViewport(viewport);
	setFov(_fov);
}

Projector::~Projector()
{
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of gluPerspective
void Projector::init_project_matrix(void)
{
	double f = 1./tan(fov*M_PI/360.);
	double ratio = (double)getViewportHeight()/getViewportWidth();
	mat_projection.set(	flip_horz*f*ratio, 0., 0., 0.,
							0., flip_vert*f, 0., 0.,
							0., 0., (zFar + zNear)/(zNear - zFar), -1.,
							0., 0., (2.*zFar*zNear)/(zNear - zFar), 0.);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection);
    glMatrixMode(GL_MODELVIEW);
}

void Projector::setViewport(int x, int y, int w, int h)
{
	vec_viewport[0] = x;
	vec_viewport[1] = y;
	vec_viewport[2] = w;
	vec_viewport[3] = h;
	center.set(vec_viewport[0]+vec_viewport[2]/2,vec_viewport[1]+vec_viewport[3]/2,0);
	view_scaling_factor = 1.0/fov*180./M_PI*MY_MIN(getViewportWidth(),getViewportHeight());
	glViewport(x, y, w, h);
	init_project_matrix();
}

void Projector::setFov(double f)
{
	fov = f;
	if (f>max_fov) fov = max_fov;
	if (f<min_fov) fov = min_fov;
	view_scaling_factor = 1.0/fov*180./M_PI*MY_MIN(getViewportWidth(),getViewportHeight());
	init_project_matrix();
}

// Get the largest possible displayed angle in radian
// This can for example be the angular distance between one corner of the screen to the oposite one
double Projector::getMaxDisplayedAngle() const
{
	Vec3d v1, v2;
	unproject_earth_equ(vec_viewport[0],vec_viewport[1], v1);
	unproject_earth_equ(vec_viewport[0]+vec_viewport[2],vec_viewport[1]+vec_viewport[3], v2);
	v1.normalize();
	v2.normalize();
	return v1.angle(v2);
}

void Projector::setMaxFov(double max) {
  if (fov > max) setFov(max);
  max_fov = max;
}

// Fill with black around the circle
void Projector::draw_viewport_shape(void)
{
	if (maskType != DISK) return;

	glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	set_orthographic_projection();
	glTranslatef(getViewportPosX()+getViewportWidth()/2,getViewportPosY()+getViewportHeight()/2,0.f);
	GLUquadricObj * p = gluNewQuadric();
	gluDisk(p, MY_MIN(getViewportWidth(),getViewportHeight())/2, getViewportWidth()+getViewportHeight(), 256, 1);  // should always cover whole screen
	gluDeleteQuadric(p);
	reset_perspective_projection();
}

void Projector::set_clipping_planes(double znear, double zfar)
{
	zNear = znear;
	zFar = zfar;
	init_project_matrix();
}

// Set the standard modelview matrices used for projection
void Projector::set_modelview_matrices(	const Mat4d& _mat_earth_equ_to_eye,
					const Mat4d& _mat_helio_to_eye,
					const Mat4d& _mat_local_to_eye,
					const Mat4d& _mat_j2000_to_eye)
{
	mat_earth_equ_to_eye = _mat_earth_equ_to_eye;
	mat_j2000_to_eye = _mat_j2000_to_eye;
	mat_helio_to_eye = _mat_helio_to_eye;
	mat_local_to_eye = _mat_local_to_eye;

	inv_mat_earth_equ_to_eye = (mat_projection*mat_earth_equ_to_eye).inverse();
	inv_mat_j2000_to_eye = (mat_projection*mat_j2000_to_eye).inverse();
	inv_mat_helio_to_eye = (mat_projection*mat_helio_to_eye).inverse();
	inv_mat_local_to_eye = (mat_projection*mat_local_to_eye).inverse();	
}


// Set the drawing mode in 2D. Use reset_perspective_projection() to reset
// previous projection mode
void Projector::set_orthographic_projection(void) const
{
	glMatrixMode(GL_PROJECTION);		// projection matrix mode
    glPushMatrix();						// store previous matrix
    glLoadIdentity();
    gluOrtho2D(	vec_viewport[0], vec_viewport[0] + vec_viewport[2],
				vec_viewport[1], vec_viewport[1] + vec_viewport[3]);	// set a 2D orthographic projection
	glMatrixMode(GL_MODELVIEW);			// modelview matrix mode
    glPushMatrix();
    glLoadIdentity();
}

// Reset the previous projection mode after a call to set_orthographic_projection()
void Projector::reset_perspective_projection(void) const
{
    glMatrixMode(GL_PROJECTION);		// Restore previous matrix
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}



// Reimplementation of gluSphere : glu is overrided for non standard projection

void Projector::sSphere(GLdouble radius, GLdouble one_minus_oblateness,
                        GLint slices, GLint stacks,
                        const Mat4d& mat, int orient_inside) const {
  glPushMatrix();
  glLoadMatrixd(mat);

  if (one_minus_oblateness == 1.0) { // gluSphere seems to have hardware acceleration
    GLUquadricObj * p = gluNewQuadric();
    gluQuadricTexture(p,GL_TRUE);
    if (orient_inside) gluQuadricOrientation(p, GLU_INSIDE);
    gluSphere(p, radius, slices, stacks);
    gluDeleteQuadric(p);
  } else {
    //GLfloat rho, theta;
    GLfloat x, y, z;
    GLfloat s, t, ds, dt;
    GLint i, j;
    GLfloat nsign;

    if (orient_inside) nsign = -1.0;
    else nsign = 1.0;

    const GLfloat drho = M_PI / (GLfloat) stacks;

#if defined(__sun) || defined(__sun__)
	// in Sun C/C++ on Solaris 8 VLAs are not allowed, so let's use new double[]
    double* cos_sin_rho = new double[2 * (stacks + 1)];
#else
    double cos_sin_rho[2*(stacks+1)];
#endif
    double *cos_sin_rho_p = cos_sin_rho;
    for (i = 0; i <= stacks; i++) {
      double rho = i * drho;
      *cos_sin_rho_p++ = cos(rho);
      *cos_sin_rho_p++ = sin(rho);
    }

    const GLfloat dtheta = 2.0 * M_PI / (GLfloat) slices;
    double cos_sin_theta[2*(slices+1)];
    double *cos_sin_theta_p = cos_sin_theta;
    for (i = 0; i <= slices; i++) {
      double theta = (i == slices) ? 0.0 : i * dtheta;
      *cos_sin_theta_p++ = cos(theta);
      *cos_sin_theta_p++ = sin(theta);
    }
    // texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
    // t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
    // cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
    ds = 1.0 / slices;
    dt = 1.0 / stacks;
    t = 1.0;            // because loop now runs from 0

    // draw intermediate stacks as quad strips
    for (i = 0,cos_sin_rho_p = cos_sin_rho; i < stacks;
         i++,cos_sin_rho_p+=2)
    {
        glBegin(GL_QUAD_STRIP);
        s = 0.0;
        for (j = 0,cos_sin_theta_p = cos_sin_theta; j <= slices;
             j++,cos_sin_theta_p+=2)
        {
            x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
            y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
            z = nsign * cos_sin_rho_p[0];
            glNormal3f(x * one_minus_oblateness * nsign,
                       y * one_minus_oblateness * nsign,
                       z * nsign);
            glTexCoord2f(s, t);
            sVertex3(x * radius,
                     y * radius,
                     one_minus_oblateness * z * radius, mat);
            x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
            y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
            z = nsign * cos_sin_rho_p[2];
            glNormal3f(x * one_minus_oblateness * nsign,
                       y * one_minus_oblateness * nsign,
                       z * nsign);
            glTexCoord2f(s, t - dt);
            s += ds;
            sVertex3(x * radius,
                     y * radius,
                     one_minus_oblateness * z * radius, mat);
        }
        glEnd();
        t -= dt;
    }
#if defined(__sun) || defined(__sun__)
    delete[] cos_sin_rho;
#endif
  }

  glPopMatrix();
}


// Draw a half sphere
void Projector::sHalfSphere(GLdouble radius, GLint slices, GLint stacks,
	const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	GLfloat rho, drho, theta, dtheta;
	GLfloat x, y, z;
	GLfloat s, t, ds, dt;
	GLint i, j, imin, imax;
	GLfloat nsign;

	if (orient_inside) nsign = -1.0;
	else nsign = 1.0;

	drho = M_PI / (GLfloat) stacks;
	dtheta = 2.0 * M_PI / (GLfloat) slices;

	// texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
	// t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
	// cannot use triangle fan on texturing (s coord. at top/bottom tip varies)
	ds = 1.0 / slices;
	dt = 1.0 / stacks;
	t = 1.0;			// because loop now runs from 0
	imin = 0;
	imax = stacks;

	// draw intermediate stacks as quad strips
	for (i = imin; i < imax/2; i++)
	{
		rho = i * drho;
		glBegin(GL_QUAD_STRIP);
		s = 0.0;
		for (j = 0; j <= slices; j++)
		{
			theta = (j == slices) ? 0.0 : j * dtheta;
			x = -sin(theta) * sin(rho);
			y = cos(theta) * sin(rho);
			z = nsign * cos(rho);
			glNormal3f(x * nsign, y * nsign, z * nsign);
			glTexCoord2f(s, t);
			sVertex3(x * radius, y * radius, z * radius, mat);
			x = -sin(theta) * sin(rho + drho);
			y = cos(theta) * sin(rho + drho);
			z = nsign * cos(rho + drho);
			glNormal3f(x * nsign, y * nsign, z * nsign);
			glTexCoord2f(s, t - dt);
			s += ds;
			sVertex3(x * radius, y * radius, z * radius, mat);
		}
		glEnd();
		t -= dt;
	}
	glPopMatrix();
}

// Draw a disk with a special texturing mode having texture center at disk center
void Projector::sDisk(GLdouble radius, GLint slices, GLint stacks,
	const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	GLfloat r, dr, theta, dtheta;
	GLfloat x, y;
	GLint j;
	GLfloat nsign;

	if (orient_inside) nsign = -1.0;
	else nsign = 1.0;

	dr = radius / (GLfloat) stacks;
	dtheta = 2.0 * M_PI / (GLfloat) slices;
	if (slices < 0) slices = -slices;

	// draw intermediate stacks as quad strips
	for (r = 0; r < radius; r+=dr)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (j = 0; j <= slices; j++)
		{
			theta = (j == slices) ? 0.0 : j * dtheta;
			x = r*cos(theta);
			y = r*sin(theta);
			glNormal3f(0, 0, nsign);
			glTexCoord2f(0.5+x/2/radius, 0.5+y/2/radius);
			sVertex3(x, y, 0, mat);
			x = (r+dr)*cos(theta);
			y = (r+dr)*sin(theta);
			glNormal3f(0, 0, nsign);
			glTexCoord2f(0.5+x/2/radius, 0.5+y/2/radius);
			sVertex3(x, y, 0, mat);
		}
		glEnd();
	}
	glPopMatrix();
}

void Projector::sRing(GLdouble r_min, GLdouble r_max,
                      GLint slices, GLint stacks,
                      const Mat4d& mat, int orient_inside) const {
  glPushMatrix();
  glLoadMatrixd(mat);

  double theta;
  double x,y;
  int j;

  const double nsign = (orient_inside)?-1.0:1.0;

  const double dr = (r_max-r_min) / stacks;
  const double dtheta = 2.0 * M_PI / slices;
  if (slices < 0) slices = -slices;

#if defined(__sun) || defined(__sun__)
  //in Sun C/C++ on Solaris 8 VLAs are not allowed, so let's use new double[]
  double *cos_sin_theta = new double[2*(slices+1)];
#else
  double cos_sin_theta[2*(slices+1)];
#endif
  double *cos_sin_theta_p = cos_sin_theta;
  for (j = 0; j <= slices; j++) {
    const double theta = (j == slices) ? 0.0 : j * dtheta;
    *cos_sin_theta_p++ = cos(theta);
    *cos_sin_theta_p++ = sin(theta);
  }


  // draw intermediate stacks as quad strips
  for (double r = r_min; r < r_max; r+=dr) {
    const double tex_r0 = (r-r_min)/(r_max-r_min);
    const double tex_r1 = (r+dr-r_min)/(r_max-r_min);
    glBegin(GL_QUAD_STRIP /*GL_TRIANGLE_STRIP*/);
    for (j=0,cos_sin_theta_p=cos_sin_theta;
         j<=slices;
         j++,cos_sin_theta_p+=2) {
      theta = (j == slices) ? 0.0 : j * dtheta;
      x = r*cos_sin_theta_p[0];
      y = r*cos_sin_theta_p[1];
      glNormal3d(0, 0, nsign);
      glTexCoord2d(tex_r0, 0.5);
      sVertex3(x, y, 0, mat);
      x = (r+dr)*cos_sin_theta_p[0];
      y = (r+dr)*cos_sin_theta_p[1];
      glNormal3d(0, 0, nsign);
      glTexCoord2d(tex_r1, 0.5);
      sVertex3(x, y, 0, mat);
    }
    glEnd();
  }
#if defined(__sun) || defined(__sun__)
    delete[] cos_sin_theta;
#endif  
  
  glPopMatrix();
}

static
inline void sSphereMapTexCoordFast(double rho_div_fov,
                                   double costheta, double sintheta)
{
	if (rho_div_fov>0.5) rho_div_fov=0.5;
	glTexCoord2d(0.5 + rho_div_fov * costheta,
                 0.5 + rho_div_fov * sintheta);
}

void Projector::sSphere_map(GLdouble radius, GLint slices, GLint stacks,
                            const Mat4d& mat, double texture_fov,
                            int orient_inside) const
{
    glPushMatrix();
    glLoadMatrixd(mat);

    double rho,x,y,z;
    int i, j;
    const double nsign = orient_inside?-1:1;

    const double drho = M_PI / stacks;

#if defined(__sun) || defined(__sun__)
	// in Sun C/C++ on Solaris 8 VLAs are not allowed, so let's use new double[]    
	double *cos_sin_rho = new double[2*(stacks+1)];
#else
    double cos_sin_rho[2*(stacks+1)];
#endif	
    double *cos_sin_rho_p = cos_sin_rho;
    for (i = 0; i <= stacks; i++) {
      const double rho = i * drho;
      *cos_sin_rho_p++ = cos(rho);
      *cos_sin_rho_p++ = sin(rho);
    }

    const double dtheta = 2.0 * M_PI / slices;
    double cos_sin_theta[2*(slices+1)];
    double *cos_sin_theta_p = cos_sin_theta;
    for (i = 0; i <= slices; i++) {
      const double theta = (i == slices) ? 0.0 : i * dtheta;
      *cos_sin_theta_p++ = cos(theta);
      *cos_sin_theta_p++ = sin(theta);
    }


    // texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y axis
    // t goes from -1.0/+1.0 at z = -radius/+radius (linear along longitudes)
    // cannot use triangle fan on texturing (s coord. at top/bottom tip varies)

    const int imax = stacks;

    // draw intermediate stacks as quad strips
    if (!orient_inside) // nsign==1
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
                sSphereMapTexCoordFast(rho/texture_fov,
                                       cos_sin_theta_p[0],
                                       cos_sin_theta_p[1]);
                sVertex3(x * radius, y * radius, z * radius, mat);

                x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
                y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
                z = cos_sin_rho_p[2];
                glNormal3d(x * nsign, y * nsign, z * nsign);
                sSphereMapTexCoordFast((rho + drho)/texture_fov,
                                       cos_sin_theta_p[0],
                                       cos_sin_theta_p[1]);
                sVertex3(x * radius, y * radius, z * radius, mat);
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
                sSphereMapTexCoordFast((rho + drho)/texture_fov,
                                       cos_sin_theta_p[0],
                                       -cos_sin_theta_p[1]);
                sVertex3(x * radius, y * radius, z * radius, mat);

                x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
                y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
                z = cos_sin_rho_p[0];
                glNormal3d(x * nsign, y * nsign, z * nsign);
                sSphereMapTexCoordFast(rho/texture_fov,
                                       cos_sin_theta_p[0],
                                       -cos_sin_theta_p[1]);
                sVertex3(x * radius, y * radius, z * radius, mat);
            }
            glEnd();
        }
    }
#if defined(__sun) || defined(__sun__)
    delete[] cos_sin_rho;
#endif  
	
    glPopMatrix();
}


// Reimplementation of gluCylinder : glu is overrided for non standard projection
void Projector::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);
	GLUquadricObj * p = gluNewQuadric();
	gluQuadricTexture(p,GL_TRUE);
	if (orient_inside)
	{
		glCullFace(GL_FRONT);
	}
	gluCylinder(p, radius, radius, height, slices, stacks);
	gluDeleteQuadric(p);
	glPopMatrix();
	if (orient_inside)
	{
		glCullFace(GL_BACK);
	}
}


void Projector::print_gravity180(SFont* font, float x, float y, const wstring& ws,
				 bool speed_optimize, float xshift, float yshift) const
{
	static float dx, dy, d, theta, psi;
	dx = x - (vec_viewport[0] + vec_viewport[2]/2);
	dy = y - (vec_viewport[1] + vec_viewport[3]/2);
	d = sqrt(dx*dx + dy*dy);
	
	// If the text is too far away to be visible in the screen return
	if (d>MY_MAX(vec_viewport[3], vec_viewport[2])*2) return;


	theta = M_PI + std::atan2(dx, dy - 1);
	psi = std::atan2((float)font->getStrLen(ws)/ws.length(),d + 1) * 180./M_PI;

	if (psi>5) psi = 5;
	set_orthographic_projection();
	glTranslatef(x,y,0);
	if(gravityLabels) glRotatef(theta*180./M_PI,0,0,-1);
	glTranslatef(xshift, -yshift, 0);
	glScalef(1, -1, 1);
	
    glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	for (unsigned int i=0;i<ws.length();++i)
	{

		if (ws[i]>=16 && ws[i]<=18) {
			// handle hilight colors (TUI)

			// Note: this is hard coded - not generalized

			if( ws[i]==17 ) glColor3f(0.5,1,0.5);  // normal
			if( ws[i]==18 ) glColor3f(1,1,1);    // hilight

		} else {

			if( !speed_optimize ) {
				font->print_char_outlined(ws[i]);
			} else {
				font->print_char(ws[i]);
			}

			// with typeface need to manually advance
			// TODO, absolute rotation would be better than relative
			// TODO: would look better with kerning information...
			glTranslatef(font->getStrLen(ws.substr(i,1)) * 1.05, 0, 0);

			if( !speed_optimize ) {
				psi = std::atan2((float)font->getStrLen(ws.substr(i,1))*1.05f,(float)d) * 180./M_PI;
				if (psi>5) psi = 5;
			}

			// keep text horizontal if gravity labels off
			if(gravityLabels) glRotatef(psi,0,0,-1);	
		}

	}
	reset_perspective_projection();
}

