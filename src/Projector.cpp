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
#include <sstream>
#include <cstdio>
#include <cassert>
#include "Projector.hpp"
#include "InitParser.hpp"
#include "Mapping.hpp"

const char *Projector::maskTypeToString(PROJECTOR_MASK_TYPE type)
{
	if(type == DISK )
		return "disk";
	else
		return "none";
}

Projector::PROJECTOR_MASK_TYPE Projector::stringToMaskType(const string &s)
{
	if (s=="disk")
		return DISK;
	return NONE;
}


void Projector::init(const InitParser& conf)
{
	// Video Section
	setViewport(conf.get_int("video:horizontal_offset"), conf.get_int("video:vertical_offset"),
	            conf.get_int("video:screen_w"), conf.get_int("video:screen_h"));

	string tmpstr = conf.get_str("projection:viewport");
	const Projector::PROJECTOR_MASK_TYPE projMaskType = Projector::stringToMaskType(tmpstr);
	setMaskType(projMaskType);
	initFov	= conf.get_double ("navigation","init_fov",60.);
	setFov(initFov);
	setFlagGravityLabels( conf.get_boolean("viewing:flag_gravity_labels") );

	tmpstr = conf.get_str("projection:viewport");
	if (tmpstr=="maximized")
		setMaximizedViewport(getViewportWidth(),  getViewportHeight());
	else
		if (tmpstr=="square" || tmpstr=="disk")
		{
			setSquareViewport(getViewportWidth(), getViewportHeight(), conf.get_int("video:horizontal_offset"), conf.get_int("video:horizontal_offset"));
			if (tmpstr=="disk")
				setViewportMaskDisk();
		}
		else
		{
			cerr << "ERROR : Unknown viewport type : " << tmpstr << endl;
			exit(-1);
		}

	// Register the default mappings
	registerProjectionMapping(MappingEqualArea());
	registerProjectionMapping(MappingStereographic());
	registerProjectionMapping(MappingFisheye());
	registerProjectionMapping(MappingCylinder());
	registerProjectionMapping(MappingPerspective());
	registerProjectionMapping(MappingOrthographic());

	tmpstr = conf.get_str("projection:type");
	setCurrentProjection(tmpstr);

	glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW);
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
{}


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

std::vector<Vec2d> Projector::getViewportVertices() const
{
	std::vector<Vec2d> result;
	result.push_back(Vec2d(vec_viewport[0], vec_viewport[1]));
	result.push_back(Vec2d(vec_viewport[0]+vec_viewport[2], vec_viewport[1]));
	result.push_back(Vec2d(vec_viewport[0]+vec_viewport[2], vec_viewport[1]+vec_viewport[3]));
	result.push_back(Vec2d(vec_viewport[0], vec_viewport[1]+vec_viewport[3]));
	return result;
}

void Projector::setFov(double f)
{
	fov = f;
	if (f>max_fov)
		fov = max_fov;
	if (f<min_fov)
		fov = min_fov;
	view_scaling_factor = 1.0/fov*180./M_PI*MY_MIN(getViewportWidth(),getViewportHeight());
}


void Projector::setMaxFov(double max)
{
	if (fov > max)
		setFov(max);
	max_fov = max;
}

void Projector::set_clipping_planes(double znear, double zfar)
{
	zNear = znear;
	zFar = zfar;
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

	// TODO: Find out what is the reason of this matrix
	static const Mat4d mat(1., 0., 0., 0.,
	                       0., 1., 0., 0.,
	                       0., 0., -1, 0.,
	                       0., 0., 0., 1.);

	inv_mat_earth_equ_to_eye = (mat*mat_earth_equ_to_eye).inverse();
	inv_mat_j2000_to_eye = (mat*mat_j2000_to_eye).inverse();
	inv_mat_helio_to_eye = (mat*mat_helio_to_eye).inverse();
	inv_mat_local_to_eye = (mat*mat_local_to_eye).inverse();
}


// Set the drawing mode in 2D. Use unset2dDrawMode() to reset previous projection mode
void Projector::set2dDrawMode(void) const
{
	glMatrixMode(GL_PROJECTION);		// projection matrix mode
	glPushMatrix();						// store previous matrix
	glLoadIdentity();
	glOrtho(vec_viewport[0], vec_viewport[0] + vec_viewport[2], vec_viewport[1], vec_viewport[1] + vec_viewport[3], -1, 1);
	glMatrixMode(GL_MODELVIEW);			// modelview matrix mode
	glPushMatrix();
	glLoadIdentity();
}

// Reset the previous projection mode after a call to set2dDrawMode()
void Projector::unset2dDrawMode(void) const
{
	glMatrixMode(GL_PROJECTION);		// Restore previous matrix
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


/*************************************************************************
 Set the frame in which we want to draw from now on
*************************************************************************/
void Projector::setCurrentFrame(FRAME_TYPE frameType) const
{
	switch (frameType)
	{
	case FRAME_LOCAL:
		setCustomFrame(mat_local_to_eye);
		break;
	case FRAME_HELIO:
		setCustomFrame(mat_helio_to_eye);
		break;
	case FRAME_EARTH_EQU:
		setCustomFrame(mat_earth_equ_to_eye);
		break;
	case FRAME_J2000:
		setCustomFrame(mat_j2000_to_eye);
		break;
	default:
		cerr << "Unknown reference frame type: " << frameType << "." << endl;
	}
}

/*************************************************************************
 Set a custom model view matrix, it is valid until the next call to 
 setCurrentFrame or setCustomFrame
*************************************************************************/
void Projector::setCustomFrame(const Mat4d& m) const
{
	static const Mat4d mat(1., 0., 0., 0.,
	                       0., 1., 0., 0.,
	                       0., 0., -1, 0.,
	                       0., 0., 0., 1.);

	modelViewMatrix = m;
	inverseModelViewMatrix = (mat*m).inverse();
}

/*************************************************************************
Init the real openGL Matrices
*************************************************************************/
void Projector::init_project_matrix(void)
{
	//	double f = 1./tan(fov*M_PI/360.);
	//	double ratio = (double)getViewportHeight()/getViewportWidth();
	//	mat_projection.set(	flip_horz*f*ratio, 0., 0., 0.,
	//							0., flip_vert*f, 0., 0.,
	//							0., 0., (zFar + zNear)/(zNear - zFar), -1.,
	//							0., 0., (2.*zFar*zNear)/(zNear - zFar), 0.);
	//	glMatrixMode(GL_PROJECTION);
	//	glLoadMatrixd(mat_projection);
	//  glMatrixMode(GL_MODELVIEW);

	// Set the real openGL projection and modelview matrix to orthographic projection
	// thus we never need to change to 2dMode from now on before drawing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(vec_viewport[0], vec_viewport[0] + vec_viewport[2], vec_viewport[1], vec_viewport[1] + vec_viewport[3], -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/*************************************************************************
 Set the current projection mapping to use
*************************************************************************/
void Projector::setCurrentProjection(const std::string& projectionName)
{
	if (currentProjectionType==projectionName)
		return;

	std::map<std::string, Mapping>::const_iterator i = projectionMapping.find(projectionName);
	if (i!=projectionMapping.end())
	{
		currentProjectionType = projectionName;

		// Redefine the projection functions
		projectForward = i->second.mapForward;
		projectBackward = i->second.mapBackward;
		min_fov = i->second.minFov;
		max_fov = i->second.maxFov;
		setFov(fov);
		init_project_matrix();
	}
	else
	{
		cerr << "Unknown projection type: " << projectionName << "." << endl;
	}
}

/*************************************************************************
 Project the vector v from the current frame into the viewport
*************************************************************************/
bool Projector::project(const Vec3d &v, Vec3d &win) const
{
	//	win=v;
	//	win.transfo4d(modelViewMatrix);

	// really important speedup:
	win[0] = modelViewMatrix.r[0]*v[0] + modelViewMatrix.r[4]*v[1]
	         + modelViewMatrix.r[8]*v[2] + modelViewMatrix.r[12];
	win[1] = modelViewMatrix.r[1]*v[0] + modelViewMatrix.r[5]*v[1]
	         + modelViewMatrix.r[9]*v[2] + modelViewMatrix.r[13];
	win[2] = modelViewMatrix.r[2]*v[0] + modelViewMatrix.r[6]*v[1]
	         + modelViewMatrix.r[10]*v[2] + modelViewMatrix.r[14];
	const bool rval = projectForward(win);
	  // very important: even when the projected point comes from an
	  // invisible region of the sky (rval=false), we must finish
	  // reprojecting, so that OpenGl can successfully eliminate
	  // polygons by culling.
	win[0] = center[0] + flip_horz * view_scaling_factor * win[0];
	win[1] = center[1] + flip_vert * view_scaling_factor * win[1];
	win[2] = (win[2] - zNear) / (zFar-zNear);
	return rval;
}

/*************************************************************************
 Project the vector v from the viewport frame into the current frame 
*************************************************************************/
bool Projector::unProject(double x, double y, Vec3d &v) const
{
	v[0] = flip_horz * (x - center[0]) / view_scaling_factor;
	v[1] = flip_vert * (y - center[1]) / view_scaling_factor;
	v[2] = 0;
	if (!projectBackward(v))
		return false;

	// modelViewMapper.backward(v);
	v.transfo4d(inverseModelViewMatrix);
	return true;
}




///////////////////////////////////////////////////////////////////////////
// Standard methods for drawing primitives

// Fill with black around the circle
void Projector::draw_viewport_shape(void)
{
	if (maskType != DISK)
		return;

	glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	set2dDrawMode();
	glTranslatef(getViewportPosX()+getViewportWidth()/2,getViewportPosY()+getViewportHeight()/2,0.f);
	GLUquadricObj * p = gluNewQuadric();
	gluDisk(p, MY_MIN(getViewportWidth(),getViewportHeight())/2, getViewportWidth()+getViewportHeight(), 256, 1);  // should always cover whole screen
	gluDeleteQuadric(p);
	unset2dDrawMode();
}

// Reimplementation of gluSphere : glu is overrided for non standard projection
void Projector::sSphereLinear(GLdouble radius, GLdouble one_minus_oblateness,
                              GLint slices, GLint stacks, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(modelViewMatrix);

	if (one_minus_oblateness == 1.0)
	{ // gluSphere seems to have hardware acceleration
		GLUquadricObj * p = gluNewQuadric();
		gluQuadricTexture(p,GL_TRUE);
		if (orient_inside)
			gluQuadricOrientation(p, GLU_INSIDE);
		gluSphere(p, radius, slices, stacks);
		gluDeleteQuadric(p);
	}
	else
	{
		//GLfloat rho, theta;
		GLfloat x, y, z;
		GLfloat s, t, ds, dt;
		GLint i, j;
		GLfloat nsign;

		if (orient_inside)
			nsign = -1.0;
		else
			nsign = 1.0;

		const GLfloat drho = M_PI / (GLfloat) stacks;

#if defined(__sun) || defined(__sun__)
		// in Sun C/C++ on Solaris 8 VLAs are not allowed, so let's use new double[]
		double* cos_sin_rho = new double[2 * (stacks + 1)];
#else

		double cos_sin_rho[2*(stacks+1)];
#endif

		double *cos_sin_rho_p = cos_sin_rho;
		for (i = 0; i <= stacks; i++)
		{
			double rho = i * drho;
			*cos_sin_rho_p++ = cos(rho);
			*cos_sin_rho_p++ = sin(rho);
		}

		const GLfloat dtheta = 2.0 * M_PI / (GLfloat) slices;
		double cos_sin_theta[2*(slices+1)];
		double *cos_sin_theta_p = cos_sin_theta;
		for (i = 0; i <= slices; i++)
		{
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
				glVertex3d(x * radius,
				           y * radius,
				           one_minus_oblateness * z * radius);
				x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
				z = nsign * cos_sin_rho_p[2];
				glNormal3f(x * one_minus_oblateness * nsign,
				           y * one_minus_oblateness * nsign,
				           z * nsign);
				glTexCoord2f(s, t - dt);
				s += ds;
				glVertex3d(x * radius,
				           y * radius,
				           one_minus_oblateness * z * radius);
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

// Draw a disk with a special texturing mode having texture center at disk center
void Projector::sDisk(GLdouble radius, GLint slices, GLint stacks, int orient_inside) const
{
	GLfloat r, dr, theta, dtheta;
	GLfloat x, y;
	GLint j;
	GLfloat nsign;

	if (orient_inside)
		nsign = -1.0;
	else
		nsign = 1.0;

	dr = radius / (GLfloat) stacks;
	dtheta = 2.0 * M_PI / (GLfloat) slices;
	if (slices < 0)
		slices = -slices;

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
			drawVertex3(x, y, 0);
			x = (r+dr)*cos(theta);
			y = (r+dr)*sin(theta);
			glNormal3f(0, 0, nsign);
			glTexCoord2f(0.5+x/2/radius, 0.5+y/2/radius);
			drawVertex3(x, y, 0);
		}
		glEnd();
	}
}

void Projector::sRing(GLdouble r_min, GLdouble r_max, GLint slices, GLint stacks, int orient_inside) const
{
	double theta;
	double x,y;
	int j;

	const double nsign = (orient_inside)?-1.0:1.0;

	const double dr = (r_max-r_min) / stacks;
	const double dtheta = 2.0 * M_PI / slices;
	if (slices < 0)
		slices = -slices;

#if defined(__sun) || defined(__sun__)
	//in Sun C/C++ on Solaris 8 VLAs are not allowed, so let's use new double[]
	double *cos_sin_theta = new double[2*(slices+1)];
#else

	double cos_sin_theta[2*(slices+1)];
#endif

	double *cos_sin_theta_p = cos_sin_theta;
	for (j = 0; j <= slices; j++)
	{
		const double theta = (j == slices) ? 0.0 : j * dtheta;
		*cos_sin_theta_p++ = cos(theta);
		*cos_sin_theta_p++ = sin(theta);
	}


	// draw intermediate stacks as quad strips
	for (double r = r_min; r < r_max; r+=dr)
	{
		const double tex_r0 = (r-r_min)/(r_max-r_min);
		const double tex_r1 = (r+dr-r_min)/(r_max-r_min);
		glBegin(GL_QUAD_STRIP /*GL_TRIANGLE_STRIP*/);
		for (j=0,cos_sin_theta_p=cos_sin_theta;
		        j<=slices;
		        j++,cos_sin_theta_p+=2)
		{
			theta = (j == slices) ? 0.0 : j * dtheta;
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
#if defined(__sun) || defined(__sun__)
	delete[] cos_sin_theta;
#endif
}

static
inline void sSphereMapTexCoordFast(double rho_div_fov, double costheta, double sintheta)
{
	if (rho_div_fov>0.5)
		rho_div_fov=0.5;
	glTexCoord2d(0.5 + rho_div_fov * costheta,
	             0.5 + rho_div_fov * sintheta);
}

void Projector::sSphere_map(GLdouble radius, GLint slices, GLint stacks, double texture_fov, int orient_inside) const
{
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
	for (i = 0; i <= stacks; i++)
	{
		const double rho = i * drho;
		*cos_sin_rho_p++ = cos(rho);
		*cos_sin_rho_p++ = sin(rho);
	}

	const double dtheta = 2.0 * M_PI / slices;
	double cos_sin_theta[2*(slices+1)];
	double *cos_sin_theta_p = cos_sin_theta;
	for (i = 0; i <= slices; i++)
	{
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
				drawVertex3(x * radius, y * radius, z * radius);

				x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
				z = cos_sin_rho_p[2];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast((rho + drho)/texture_fov,
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
				sSphereMapTexCoordFast((rho + drho)/texture_fov,
				                       cos_sin_theta_p[0],
				                       -cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);

				x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
				y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
				z = cos_sin_rho_p[0];
				glNormal3d(x * nsign, y * nsign, z * nsign);
				sSphereMapTexCoordFast(rho/texture_fov,
				                       cos_sin_theta_p[0],
				                       -cos_sin_theta_p[1]);
				drawVertex3(x * radius, y * radius, z * radius);
			}
			glEnd();
		}
	}
#if defined(__sun) || defined(__sun__)
	delete[] cos_sin_rho;
#endif
}


// Reimplementation of gluCylinder : glu is overrided for non standard projection
void Projector::sCylinderLinear(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(modelViewMatrix);
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


void Projector::drawTextGravity180(SFont* font, float x, float y, const wstring& ws,
                                   bool speed_optimize, float xshift, float yshift) const
{
	static float dx, dy, d, theta, psi;
	dx = x - (vec_viewport[0] + vec_viewport[2]/2);
	dy = y - (vec_viewport[1] + vec_viewport[3]/2);
	d = sqrt(dx*dx + dy*dy);

	// If the text is too far away to be visible in the screen return
	if (d>MY_MAX(vec_viewport[3], vec_viewport[2])*2)
		return;


	theta = M_PI + std::atan2(dx, dy - 1);
	psi = std::atan2((float)font->getStrLen(ws)/ws.length(),d + 1) * 180./M_PI;

	if (psi>5)
		psi = 5;
	set2dDrawMode();
	glTranslatef(x,y,0);
	if(gravityLabels)
		glRotatef(theta*180./M_PI,0,0,-1);
	glTranslatef(xshift, -yshift, 0);
	glScalef(1, -1, 1);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	for (unsigned int i=0;i<ws.length();++i)
	{

		if (ws[i]>=16 && ws[i]<=18)
		{
			// handle hilight colors (TUI)

			// Note: this is hard coded - not generalized

			if( ws[i]==17 )
				glColor3f(0.5,1,0.5);  // normal
			if( ws[i]==18 )
				glColor3f(1,1,1);    // hilight

		}
		else
		{

			if( !speed_optimize )
			{
				font->print_char_outlined(ws[i]);
			}
			else
			{
				font->print_char(ws[i]);
			}

			// with typeface need to manually advance
			// TODO, absolute rotation would be better than relative
			// TODO: would look better with kerning information...
			glTranslatef(font->getStrLen(ws.substr(i,1)) * 1.05, 0, 0);

			if( !speed_optimize )
			{
				psi = std::atan2((float)font->getStrLen(ws.substr(i,1))*1.05f,(float)d) * 180./M_PI;
				if (psi>5)
					psi = 5;
			}

			// keep text horizontal if gravity labels off
			if(gravityLabels)
				glRotatef(psi,0,0,-1);
		}

	}
	unset2dDrawMode();
}

/*************************************************************************
 Draw the string at the given position and angle with the given font
*************************************************************************/
void Projector::drawText(const SFont* font, float x, float y, const wstring& str, float angleDeg, float xshift, float yshift) const
{
	glPushMatrix();
	glTranslatef(x,y,0);
	glRotatef(angleDeg,0,0,1);
	glTranslatef(0,font->getLineHeight(),0);
	font->print(xshift, yshift, str);
	glPopMatrix();
}

/*************************************************************************
 Draw a parallel arc in the current frame
*************************************************************************/
void Projector::drawParallel(const Vec3d& start, double length, bool labelAxis, const SFont* font, int nbSeg) const
{
	if (nbSeg==-1)
		nbSeg = 4 + (int)(length*44./(2.*M_PI));
	const Mat4d dRa = Mat4d::zrotation(length/nbSeg);

	Vec3d v(start);
	Vec3d pt1;
	glBegin(GL_LINE_STRIP);
	for (int i=0;i<=nbSeg;++i)
	{
		project(v, pt1);
		glVertex2f(pt1[0],pt1[1]);
		dRa.transfo(v);
	}
	glEnd();

	// Draw label if needed
	if (labelAxis)
	{
		double lon, lat;
		StelUtils::rect_to_sphe(&lon, &lat, start);
		Vec3d win0, win1;
		Vec3d v1, v2;
		StelUtils::sphe_to_rect(lon+0.0000001, lat, v2);
		project(start, win0);
		project(v2, win1);
		double angleDeg = std::atan2(win1[1]-win0[1], win1[0]-win0[0])*180./M_PI;
		const wstring str = StelUtils::printAngleDMS(lat);
		float xshift=5;
		if (angleDeg>90. || angleDeg<-90.)
		{
			angleDeg+=180.;
			xshift=-font->getStrLen(str)-5.f;
		}
		drawText(font, win1[0], win1[1], str, angleDeg, xshift, 3);

		// Label at end of the arc
		StelUtils::sphe_to_rect(lon+length-0.0000001, lat, v2);
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
	}
}

/*************************************************************************
 Draw a meridian arc in the current frame
*************************************************************************/
void Projector::drawMeridian(const Vec3d& start, double length, bool labelAxis, const SFont* font, int nbSeg) const
{
	if (nbSeg==-1)
		nbSeg = 4 + (int)(length*54./(2.*M_PI));
	static const Vec3d oneZ(0,0,1);
	const Mat4d dDe = Mat4d::rotation(start^oneZ, (start[1]>=0 ? 1.:-1.) * length/nbSeg);

	Vec3d v(start);
	Vec3d pt1;
	glBegin(GL_LINE_STRIP);
	for (int i=0;i<=nbSeg;++i)
	{
		project(v, pt1);
		glVertex2f(pt1[0],pt1[1]);
		dDe.transfo(v);
	}
	glEnd();

	// Draw label if needed
	if (labelAxis)
	{
		double lon, lat;
		StelUtils::rect_to_sphe(&lon, &lat, start);
		Vec3d win0, win1;
		Vec3d v2;
		StelUtils::sphe_to_rect(lon, lat+0.0000001, v2);
		project(start, win0);
		project(v2, win1);
		double angleDeg = std::atan2(win1[1]-win0[1], win1[0]-win0[0])*180./M_PI;
		const wstring str = StelUtils::printAngleHMS(lon);
		float xshift=20;
		if (angleDeg>90. || angleDeg<-90.)
		{
			angleDeg+=180.;
			xshift=-font->getStrLen(str)-20.f;
		}
		drawText(font, win1[0], win1[1], str, angleDeg, xshift, 3);

		// Label at end of the arc
		StelUtils::sphe_to_rect(lon, lat+length-0.0000001, v2);
		project(v, win0);
		project(v2, win1);
		angleDeg = std::atan2(win1[1]-win0[1], win1[0]-win0[0])*180./M_PI;
		xshift=20;
		if (angleDeg>90. || angleDeg<-90.)
		{
			angleDeg+=180.;
			xshift=-font->getStrLen(str)-20.f;
		}
		drawText(font, win1[0], win1[1], str, angleDeg, xshift, 3);
	}
}

/*************************************************************************
 Draw a square using the current texture at the given position
*************************************************************************/
void Projector::drawSprite(const Vec3d& pos, double size) const
{
	Vec3d win;
	project(pos, win);
	drawSprite2dMode(win[0], win[1], size);
}

/*************************************************************************
 Same function but gives the already projected 2d position in input
*************************************************************************/
inline void Projector::drawSprite2dMode(double x, double y, double size) const
{
	// TODO use GL extension if available
	//	glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );
	//	glEnable(GL_POINT_SPRITE_ARB);
	//	glPointSize(size);
	//	glBegin(GL_POINTS);
	//		glVertex2f(x,y);
	//	glEnd();

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
 Generalisation of glVertex3v. This method assumes that we are in orthographic 
 projection mode, which is true for special projections.
*************************************************************************/
void Projector::drawVertex3v(const Vec3d& v) const
{
	Vec3d win;
	project(v, win);
	glVertex2f(win[0],win[1]);
//	glVertex3dv(win.v);
}

///////////////////////////////////////////////////////////////////////////
// Drawing methods for general (non-linear) mode

// Here is the main trick for texturing in fisheye mode : The trick is to compute the
// new coordinate in orthographic projection which will simulate the fisheye projection.
//void Projector::sVertex3(double x, double y, double z, const Mat4d& mat) const
//{
//	Vec3d win;
//	Vec3d v(x,y,z);
//	project(v, win);
//
//	// Can be optimized by avoiding matrix inversion if it's always the same
//	gluUnProject(win[0],win[1],win[2],modelViewMatrix,mat_projection,vec_viewport,&v[0],&v[1],&v[2]);
//	glVertex3dv(v);
//}


void Projector::sSphere(GLdouble radius, GLdouble one_minus_oblateness,
                        GLint slices, GLint stacks, int orient_inside) const
{
	// It is really good for performance to have Vec4f,Vec3f objects
	// static rather than on the stack. But why?
	// Is the constructor/destructor so expensive?
	static Vec4f lightPos4;
	static Vec3f lightPos3;
	GLboolean isLightOn;
	static Vec3f transNorm;
	float c;

	static Vec4f ambientLight;
	static Vec4f diffuseLight;

	glGetBooleanv(GL_LIGHTING, &isLightOn);

	if (isLightOn)
	{
		glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos4);
		lightPos3 = lightPos4;
		lightPos3 -= modelViewMatrix * Vec3d(0.,0.,0.); // -posCenterEye
		lightPos3.normalize();
		glGetLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
		glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
		glDisable(GL_LIGHTING);
	}

	GLfloat x, y, z;
	GLfloat s, t;
	GLint i, j;
	GLfloat nsign;

	if (orient_inside)
	{
		nsign = -1.0;
		t=0.0; // from inside texture is reversed
	}
	else
	{
		nsign = 1.0;
		t=1.0;
	}

	const GLfloat drho = M_PI / (GLfloat) stacks;
#if defined(__sun) || defined(__sun__)
	// in Sun C/C++ on Solaris 8 VLAs are not allowed, so let's use new double[]
	double *cos_sin_rho = new double[2*(stacks+1)];
#else

	double cos_sin_rho[2*(stacks+1)];
#endif

	double *cos_sin_rho_p = cos_sin_rho;
	for (i = 0; i <= stacks; i++)
	{
		double rho = i * drho;
		*cos_sin_rho_p++ = cos(rho);
		*cos_sin_rho_p++ = sin(rho);
	}

	const GLfloat dtheta = 2.0 * M_PI / (GLfloat) slices;
	double cos_sin_theta[2*(slices+1)];
	double *cos_sin_theta_p = cos_sin_theta;
	for (i = 0; i <= slices; i++)
	{
		double theta = (i == slices) ? 0.0 : i * dtheta;
		*cos_sin_theta_p++ = cos(theta);
		*cos_sin_theta_p++ = sin(theta);
	}

#if defined(__sun) || defined(__sun__)
	delete[] cos_sin_rho;
#endif

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
		for (j = 0,cos_sin_theta_p = cos_sin_theta; j <= slices;
		        j++,cos_sin_theta_p+=2)
		{
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[1];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[1];
			z = nsign * cos_sin_rho_p[0];
			glTexCoord2f(s, t);
			if (isLightOn)
			{
				transNorm = modelViewMatrix.multiplyWithoutTranslation(
				                Vec3d(x * one_minus_oblateness * nsign,
				                      y * one_minus_oblateness * nsign,
				                      z * nsign));
				c = lightPos3.dot(transNorm);
				if (c<0)
					c=0;
				glColor3f(c*diffuseLight[0] + ambientLight[0],
				          c*diffuseLight[1] + ambientLight[1],
				          c*diffuseLight[2] + ambientLight[2]);
			}
			drawVertex3(x * radius, y * radius, z * one_minus_oblateness * radius);
			x = -cos_sin_theta_p[1] * cos_sin_rho_p[3];
			y = cos_sin_theta_p[0] * cos_sin_rho_p[3];
			z = nsign * cos_sin_rho_p[2];
			glTexCoord2f(s, t - dt);
			if (isLightOn)
			{
				transNorm = modelViewMatrix.multiplyWithoutTranslation(
				                Vec3d(x * one_minus_oblateness * nsign,
				                      y * one_minus_oblateness * nsign,
				                      z * nsign));
				c = lightPos3.dot(transNorm);
				if (c<0)
					c=0;
				glColor3f(c*diffuseLight[0] + ambientLight[0],
				          c*diffuseLight[1] + ambientLight[1],
				          c*diffuseLight[2] + ambientLight[2]);
			}
			drawVertex3(x * radius, y * radius, z * one_minus_oblateness * radius);
			s += ds;
		}
		glEnd();
		t -= dt;
	}

	if (isLightOn)
		glEnable(GL_LIGHTING);
}

// Reimplementation of gluCylinder : glu is overrided for non standard projection
void Projector::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orient_inside) const
{
	static GLdouble da, r, dz;
	static GLfloat z, nsign;
	static GLint i, j;

	nsign = 1.0;
	if (orient_inside)
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

	if (orient_inside)
		glCullFace(GL_BACK);
}
