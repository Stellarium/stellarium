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
}

Fisheye_projector::~Fisheye_projector()
{
}

// For a fisheye, ratio is alway = 1
void Fisheye_projector::maximize_viewport()
{
	vec_viewport[0] = 0;
	vec_viewport[1] = 0;
	vec_viewport[2] = screenW;
	vec_viewport[3] = screenH;
	glViewport(0, 0, screenW, screenH);
	center.set((vec_viewport[2]-vec_viewport[0])/2,(vec_viewport[3]-vec_viewport[1])/2,0);
	init_project_matrix();
}

void Fisheye_projector::set_viewport(int x, int y, int w, int h)
{
	vec_viewport[0] = x;
	vec_viewport[1] = y;
	vec_viewport[2] = w;
	vec_viewport[3] = h;
	glViewport(x, y, w, h);
	center.set(x+w/2,y+h/2,0);
}

void Fisheye_projector::change_fov(double deltaFov)
{
	Projector::change_fov(deltaFov);
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of glOrtho
void Fisheye_projector::init_project_matrix(void)
{
	// Simplest orthographic projection
	mat_projection = Mat4d(	1., 0., 0., 0.,
							0., 1., 0., 0.,
							0., 0., -1, 0.,
							0., 0., 0., 1.);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection);
    glMatrixMode(GL_MODELVIEW);
}

bool Fisheye_projector::project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const
{
	static Vec3d w;
	w = v;
	w.transfo4d(mat);
	w.normalize();
	w.transfo4d(mat_projection);
	static double z;
	z = w[2];
	double a = fabs(M_PI_2 - asin(w[2]));
	w[2] = 0;
	w.normalize();
	win = center + w * a/M_PI * 360./fov * MY_MIN(vec_viewport[2],vec_viewport[3])/2;
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
	v[0] = x - center[0];
	v[1] = y - center[1];
	v[2] = 0;
	double angle_center = v.length()/d * fov/2*M_PI/180;
	double r = sin(angle_center);
	v.normalize();
	v*=r;

	v[2] = sqrt(1.-(v[0]*v[0]+v[1]*v[1]));
	if (angle_center>M_PI_2) v[2] = -v[2];

	v.transfo4d(m);
}

// Override glVertex3f
// Here is the main trick for texturing in fisheye mode : The trick is to compute the
// new coordinate in orthographic projection which will simulate the fisheye projection.
void Fisheye_projector::sVertex3f(double x, double y, double z, const Mat4d& mat) const
{
	static Vec3d win;
	static Vec3d v;
	project_custom(Vec3d(x,y,z), win, mat);
	gluUnProject(win[0],win[1],win[2],mat,mat_projection,vec_viewport,&v[0],&v[1],&v[2]);
	glVertex3f(v[0],v[1],v[2]);
}

void Fisheye_projector::sSphere(GLdouble radius, GLint slices, GLint stacks, const Mat4d& mat, int orient_inside) const
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
			sVertex3f(x * radius, y * radius, z * radius, mat);
			x = -sin(theta) * sin(rho + drho);
			y = cos(theta) * sin(rho + drho);
			z = nsign * cos(rho + drho);
			glNormal3f(x * nsign, y * nsign, z * nsign);
			glTexCoord2f(s, t - dt);
			s += ds;
			sVertex3f(x * radius, y * radius, z * radius, mat);
		}
		glEnd();
		t -= dt;
	}
	glPopMatrix();
}
