/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#ifndef _FISHEYE_PROJECTOR_H_
#define _FISHEYE_PROJECTOR_H_

#include "projector.h"

// Class which handle projection modes and projection matrix
// Overide some function usually handled by glu
class Fisheye_projector : public Projector
{
public:
	Fisheye_projector(int _screenW = 800, int _screenH = 600, double _fov = 175.);
	virtual ~Fisheye_projector();

	virtual void change_fov(double deltaFov);

	virtual void maximize_viewport(void);

	// Same function but using a custom modelview matrix
	virtual bool project_custom(const Vec3f& v, Vec3d& win, const Mat4d& mat) const;
	virtual bool project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const;
	virtual bool project_custom_check(const Vec3f& v, Vec3d& win, const Mat4d& mat) const;
	virtual void unproject_custom(double x, double y, Vec3d& v, const Mat4d& mat) const;

protected:

	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	virtual void init_project_matrix(void);

	// transformation from screen 2D point x,y to object
	// m is here the already inverted full tranfo matrix
	virtual void unproject(double x, double y, const Mat4d& m, Vec3d& v) const
	{
		v.set(	(x - vec_viewport[0]) * 2. / vec_viewport[2] - 1.0,
				(y - vec_viewport[1]) * 2. / vec_viewport[3] - 1.0,
				1.0);
		v.transfo4d(m);
	}

};


#endif // _FISHEYE_PROJECTOR_H_
