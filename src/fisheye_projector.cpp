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
#include "fisheye_projector.h"


Fisheye_projector::Fisheye_projector(int _screenW, int _screenH, double _fov) : Projector(800, 600, 175.f)
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
	int c = screenH;
	if (screenH>screenW) c=screenW;
	int marginw = (screenW-c)/2;
	int marginh = (screenH-c)/2;
	vec_viewport[0] = marginw;
	vec_viewport[1] = marginh;
	vec_viewport[2] = c;
	vec_viewport[3] = c;
	glViewport(marginw, marginh, c, c);
}

void Fisheye_projector::change_fov(double deltaFov)
{
    // if we are zooming in or out
    if (deltaFov)
    {
		if (fov+deltaFov>0.001 && fov+deltaFov<250.)  fov+=deltaFov;
		if (fov+deltaFov>250.) fov=250.;
		if (fov+deltaFov<0.001) fov=0.001;
		init_project_matrix();
    }
}

// Init the viewing matrix, setting the field of view, the clipping planes, and screen ratio
// The function is a reimplementation of gluPerspective
void Fisheye_projector::init_project_matrix(void)
{
	double f = 1./tan(fov*M_PI/360.);
	mat_projection = Mat4d(	f, 0., 0., 0.,
							0., f, 0., 0.,
							0., 0., (zFar + zNear)/(zNear - zFar), -1.,
							0., 0., (2.*zFar*zNear)/(zNear - zFar), 0.);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(mat_projection);
    glMatrixMode(GL_MODELVIEW);
}


bool Fisheye_projector::project_custom(const Vec3f& v, Vec3d& win, const Mat4d& mat) const
{
    gluProject(v[0],v[1],v[2],mat,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1.);
}
bool Fisheye_projector::project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const
{
    gluProject(v[0],v[1],v[2],mat,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1.);
}
bool Fisheye_projector::project_custom_check(const Vec3f& v, Vec3d& win, const Mat4d& mat) const
{
    gluProject(v[0],v[1],v[2],mat,mat_projection,vec_viewport,&win[0],&win[1],&win[2]);
	return (win[2]<1. && check_in_viewport(win));
}
void Fisheye_projector::unproject_custom(double x ,double y, Vec3d& v, const Mat4d& mat) const
{
	gluUnProject(x,y,1.,mat,mat_projection,vec_viewport,&v[0],&v[1],&v[2]);
}
