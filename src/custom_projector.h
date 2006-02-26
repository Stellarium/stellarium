/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _CUSTOM_PROJECTOR_H_
#define _CUSTOM_PROJECTOR_H_

#include "projector.h"

// Class which handle projection modes and projection matrix
// Overide some function usually handled by glu
class CustomProjector : public Projector
{
protected:
    CustomProjector(int _screenW = 800, int _screenH = 600, double _fov = 175.);
private:
	void setViewport(int x, int y, int w, int h);

	// Same function but using a custom modelview matrix
	void unproject_custom(double x, double y, Vec3d& v, const Mat4d& mat) const;

	// Reimplementation of gluSphere : glu is overrided for non standard projection
	void sSphere(GLdouble radius, GLdouble oblateness,
		GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	// Reimplementation of gluCylinder : glu is overrided for non standard projection
	void sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	void update_openGL(void) const;

	// Override glVertex3f and glVertex3d
	void sVertex3(double x, double y, double z, const Mat4d& mat) const;

	const Vec3d convert_pos(const Vec3d& v, const Mat4d& mat) const;
protected:

	Vec3d center;

	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	void init_project_matrix(void);

};


#endif // _FISHEYE_PROJECTOR_H_
