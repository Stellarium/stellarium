/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chéreau
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

#include <stdio.h>
#include "fisheye_projector.h"


Fisheye_projector::Fisheye_projector(int _screenW, int _screenH, double _fov,
	double _min_fov, double _max_fov) : Projector(800, 600, 180.f, _min_fov, _max_fov)
{
	change_fov(_fov);
	set_screen_size(_screenW,_screenH);
	mat_projection.set(1., 0., 0., 0.,
							0., 1., 0., 0.,
							0., 0., -1, 0.,
							0., 0., 0., 1.);
}

// For a fisheye, ratio is alway = 1
void Fisheye_projector::set_viewport(int x, int y, int w, int h)
{
	Projector::set_viewport(x, y, w, h);
	center.set(vec_viewport[0]+vec_viewport[2]/2,vec_viewport[1]+vec_viewport[3]/2,0);
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of glOrtho
void Fisheye_projector::init_project_matrix(void)
{
	double f = 1./tan(60.*M_PI/360.);
	mat_projection2 = Mat4d(f*ratio, 0., 0., 0.,
							0., f, 0., 0.,
							0., 0., (zFar + zNear)/(zNear - zFar), -1.,
							0., 0., (2.*zFar*zNear)/(zNear - zFar), 0.);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection2);
    glMatrixMode(GL_MODELVIEW);
}

void Fisheye_projector::update_openGL(void) const
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection2);
    glMatrixMode(GL_MODELVIEW);
	glViewport(vec_viewport[0], vec_viewport[1], vec_viewport[2], vec_viewport[3]);
}

bool Fisheye_projector::project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const
{
	static double z;
	static double a;

	win = v;
	win.transfo4d(mat);
	win.normalize();
	win.transfo4d(mat_projection);
	z = win[2];
	a = fabs(M_PI_2 - asin(win[2]));
	win[2] = 0.;
	win.normalize();
	win = center + win * (a/M_PI * 360./fov * MY_MIN(vec_viewport[2],vec_viewport[3])/2);
	win[2] = z;
	if (a<0.95*M_PI) return true;
	else return false;
}


void Fisheye_projector::unproject_custom(double x ,double y, Vec3d& v, const Mat4d& mat) const
{
	unproject(x, y, (mat_projection*mat).inverse(), v);
}

void Fisheye_projector::unproject(double x, double y, const Mat4d& m, Vec3d& v) const
{
	double d = MY_MIN(vec_viewport[2],vec_viewport[3])/2;
	static double length;
	v[0] = x - center[0];
	v[1] = y - center[1];
	v[2] = 0;
	length = v.length();

	double angle_center = length/d * fov/2*M_PI/180;
	double r = sin(angle_center);
	if (length!=0)
	{
		v.normalize();
		v*=r;
	}
	else
	{
		v.set(0.,0.,0.);
	}

	v[2] = sqrt(1.-(v[0]*v[0]+v[1]*v[1]));
	if (angle_center>M_PI_2) v[2] = -v[2];

	v.transfo4d(m);
}


// Override glVertex3f
// Here is the main trick for texturing in fisheye mode : The trick is to compute the
// new coordinate in orthographic projection which will simulate the fisheye projection.
void Fisheye_projector::sVertex3(double x, double y, double z, const Mat4d& mat) const
{
	static Vec3d win;
	static Vec3d v;
	v.set(x,y,z);
	project_custom(v, win, mat);
	// Can be optimized by avoiding matrix inversion if it's always the same
	gluUnProject(win[0],win[1],0.5,mat,mat_projection2,vec_viewport,&v[0],&v[1],&v[2]);
	glVertex3dv(v);
}

void Fisheye_projector::sSphere(GLdouble radius, GLint slices, GLint stacks, const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	static GLfloat rho, drho, theta, dtheta;
	static GLfloat x, y, z;
	static GLfloat s, t, ds, dt;
	static GLint i, j, imin, imax;
	static GLfloat nsign;

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
	for (i = imin; i < imax; i++)
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

// Reimplementation of gluCylinder : glu is overrided for non standard projection
void Fisheye_projector::sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
const Mat4d& mat, int orient_inside) const
{
	glPushMatrix();
	glLoadMatrixd(mat);

	static GLdouble da, r, dz;
	static GLfloat z, nsign;
	static GLint i, j;

	nsign = 1.0;
	if (orient_inside) glCullFace(GL_FRONT);
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
			x = sinf(0.0);
			y = cosf(0.0);
		}
		else
		{
			x = sinf(i * da);
			y = cosf(i * da);
		}
		glNormal3f(x * nsign, y * nsign, 0);
		glTexCoord2f(s, t);
		sVertex3(x * r, y * r, z, mat);
		glNormal3f(x * nsign, y * nsign, 0);
		glTexCoord2f(s, t + dt);
		sVertex3(x * r, y * r, z + dz, mat);
		s += ds;
	}			/* for slices */
	glEnd();
	t += dt;
	z += dz;
	}				/* for stacks */

	glPopMatrix();
	if (orient_inside) glCullFace(GL_BACK);
}
