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
        Fisheye_projector(int _screenW = 800, int _screenH = 600, double _fov = 175., int _distortion_function = -1,
		    double _min_fov = 0.001, double _max_fov = 300.);

                    // distortion_function can be used to select between different projection distortions
                    // currently not used by standard stellarium version, but needed by Digitalis

	Fisheye_projector(const Projector&);

	virtual PROJECTOR_TYPE get_type(void) const {return FISHEYE_PROJECTOR;}

	virtual void set_viewport(int x, int y, int w, int h);

	// Same function but using a custom modelview matrix
	virtual bool project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const;
	virtual void unproject_custom(double x, double y, Vec3d& v, const Mat4d& mat) const;

	// Reimplementation of gluSphere : glu is overrided for non standard projection
	void sSphere(GLdouble radius, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	// Reimplementation of gluCylinder : glu is overrided for non standard projection
	virtual void sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	void update_openGL(void) const;

	// Override glVertex3f and glVertex3d
	void sVertex3(double x, double y, double z, const Mat4d& mat) const;

	const Vec3d convert_pos(const Vec3d& v, const Mat4d& mat) const;
protected:

	Vec3d center;

	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	virtual void init_project_matrix(void);

	// transformation from screen 2D point x,y to object
	// m is here the already inverted full tranfo matrix
	virtual void unproject(double x, double y, const Mat4d& m, Vec3d& v) const;
	void unproject(const Vec3d& u, const Mat4d& m, Vec3d& v) const;

	Mat4d mat_projection2;		// orthographic projection matrix used for 3d vertex conversion
};


#endif // _FISHEYE_PROJECTOR_H_
