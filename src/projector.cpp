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

#include "stdio.h"
#include "projector.h"


// transformation from screen 2D point x,y to object
// m is here the already inverted full tranfo matrix
bool Projector::unproject(double x, double y, const Mat4d m, Vec3d& out) const
{
	// transformation coordonnees normalisees entre -1 et 1
	out[0] = (x - vec_viewport[0]) * 2. / vec_viewport[2] - 1.0;
	out[1] = (y - vec_viewport[1]) * 2. / vec_viewport[3] - 1.0;
	out[2] = 1.0;
	out = m*out;
	return true;
}

Projector::Projector(int _screenW, int _screenH, double _fov) : ratio(0.75f), zNear(0.1), zFar(10000)
{
	change_fov(_fov);
	set_screen_size(_screenW,_screenH);
}

Projector::~Projector()
{
}

void Projector::set_screen_size(int w, int h)
{
	screenW = w;
	screenH = h;
	vec_viewport[2] = w;
	vec_viewport[3] = h;
	glViewport(0, 0, w, h);
	ratio = (float)h/w;
	init_project_matrix();
}

void Projector::set_clipping_planes(double znear, double zfar)
{
	zNear = znear;
	zFar = zfar;
	init_project_matrix();
}

void Projector::change_fov(double deltaFov)
{
    // if we are zooming in or out
    if (deltaFov)
    {
		if (fov+deltaFov>0.001 && fov+deltaFov<100.)  fov+=deltaFov;
		if (fov+deltaFov>100) fov=100.;
		if (fov+deltaFov<0.001) fov=0.001;
		init_project_matrix();
    }
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of gluPerspective
void Projector::init_project_matrix(void)
{
	double f = 1./tan(fov*M_PI/360.);
	mat_projection = Mat4d(	f*ratio, 0., 0., 0.,
							0., f, 0., 0.,
							0., 0., (zFar + zNear)/(zNear - zFar), -1.,
							0., 0., (2.*zFar*zNear)/(zNear - zFar), 0.);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection);
    glMatrixMode(GL_MODELVIEW);
}

bool Projector::check_in_screen(const Vec3d& pos) const
{
	return 	pos[1]>vec_viewport[1] &&
			pos[1]<vec_viewport[3] &&
			pos[0]>vec_viewport[0] &&
			pos[0]<vec_viewport[2];
}


// Set the standard modelview matrices used for projection
void Projector::set_modelview_matrices(	const Mat4d& _mat_earth_equ_to_eye,
										const Mat4d& _mat_helio_to_eye,
										const Mat4d& _mat_local_to_eye)
{
	mat_earth_equ_to_eye = _mat_earth_equ_to_eye;
	mat_helio_to_eye = _mat_helio_to_eye;
	mat_local_to_eye = _mat_local_to_eye;

	inv_mat_local_to_eye = (mat_projection*mat_local_to_eye).inverse();
}


bool Projector::project_earth_equ(const Vec3f& v, Vec3d& win) const
{
    return project_custom(v, win, mat_earth_equ_to_eye);
}
bool Projector::project_earth_equ(const Vec3d& v, Vec3d& win) const
{
    return project_custom(v, win, mat_earth_equ_to_eye);
}
bool Projector::project_earth_equ_check(const Vec3f& v, Vec3d& win) const
{
    return project_custom_check(v, win, mat_earth_equ_to_eye);
}
bool Projector::unproject_earth_equ(double x ,double y, Vec3d& v) const
{
	return unproject_custom(x ,y, v, mat_earth_equ_to_eye);
}


bool Projector::project_helio(const Vec3f& v, Vec3d& win) const
{
    return project_custom(v, win, mat_helio_to_eye);
}
bool Projector::project_helio_check(const Vec3f& v, Vec3d& win) const
{
	return project_custom_check(v, win, mat_helio_to_eye);
}
bool Projector::unproject_helio(double x ,double y, Vec3d& v) const
{
	return unproject_custom(x ,y, v, mat_helio_to_eye);
}


bool Projector::project_local(const Vec3f& v, Vec3d& win) const
{
    return project_custom(v, win, mat_local_to_eye);
}
bool Projector::project_local(const Vec3d& v, Vec3d& win) const
{
    return project_custom(v, win, mat_local_to_eye);
}
bool Projector::project_local_check(const Vec3f& v, Vec3d& win) const
{
    return project_custom_check(v, win, mat_local_to_eye);
}
bool Projector::unproject_local(double x ,double y, Vec3d& v) const
{
	v[0] = (x - vec_viewport[0]) * 2. / vec_viewport[2] - 1.0;
	v[1] = (y - vec_viewport[1]) * 2. / vec_viewport[3] - 1.0;
	v[2] = 1.0;
	v.transfo4d(inv_mat_local_to_eye);
	return true;
	//return unproject(x, y, inv_mat_local_to_eye, v);
}


bool Projector::project_custom(const Vec3f& v, Vec3d& win, const Mat4d& mat) const
{
    gluProject(v[0],v[1],v[2],mat,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1.);
}
bool Projector::project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const
{
    gluProject(v[0],v[1],v[2],mat,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1.);
}
bool Projector::project_custom_check(const Vec3f& v, Vec3d& win, const Mat4d& mat) const
{
    gluProject(v[0],v[1],v[2],mat,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1. && check_in_screen(win));
}
bool Projector::unproject_custom(double x ,double y, Vec3d& v, const Mat4d& mat) const
{
	return gluUnProject(x,y,1.,mat,mat_projection,vec_viewport,&v[0],&v[1],&v[2]);
}


bool Projector::project_current(const Vec3f& v, Vec3d& win)
{
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
    gluProject(v[0],v[1],v[2],M,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1.);
}
bool Projector::unproject_current(double x ,double y, Vec3d& v)
{
    glGetDoublev(GL_MODELVIEW_MATRIX,M);
	return gluUnProject(x,y,1.,M,mat_projection,vec_viewport,&v[0],&v[1],&v[2]);
}

// Set the drawing mode in 2D. Use reset_perspective_projection() to reset
// previous projection mode
void Projector::set_orthographic_projection(void) const
{
	glMatrixMode(GL_PROJECTION);		// projection matrix mode
    glPushMatrix();						// store previous matrix
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);	// set a 2D orthographic projection
	glMatrixMode(GL_MODELVIEW);			// modelview matrix mode

	// TODO : shall i keep that ?
    glPushMatrix();
    glLoadIdentity();
}

// Reset the previous projection mode after a call to set_orthographic_projection()
void Projector::reset_perspective_projection(void) const
{
    glMatrixMode(GL_PROJECTION);		// Restore previous matrix
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

	// TODO : shall i keep that ?
    glPopMatrix();
}

/* transformation du point ecran (winx,winy,winz) en point objet
bool Projector::unproject(double x, double y, Mat4d m, Vec3d& out)
{
   static Vec4d out4(out);

   // transformation coordonnees normalisees entre -1 et 1
   out4[0] = (x - vec_viewport[0]) * 2. / vec_viewport[2] - 1.0;
   out4[1] = (y - vec_viewport[1]) * 2. / vec_viewport[3] - 1.0;
   out4[2] = 1.0;
   out4[3] = 1.0;

   // d'ou les coordonnees objets
   out4 = m*out4;
   if (out4[3] == 0.0) return false;
   out[0] = out4[0] / out4[3];
   out[1] = out4[1] / out4[3];
   out[2] = out4[2] / out4[3];
	return true;
}*/
