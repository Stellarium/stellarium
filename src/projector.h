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

#ifndef _PROJECTOR_H_
#define _PROJECTOR_H_

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#include "vecmath.h"

// Class which handle projection modes and projection matrix
// Overide some function usually handled by glu
class Projector
{
public:
	Projector(int _screenW = 800, int _screenH = 600, double _fov = 60.);
	virtual ~Projector();

	void set_fov(double f) {fov = f;}
	double get_fov(void) const {return fov;}
	void change_fov(double deltaFov);

	void set_screen_size(int w, int h);
	int scrW(void) const {return vec_viewport[2];}
	int scrH(void) const {return vec_viewport[3];}

	void set_clipping_planes(double znear, double zfar);

	// Return true if the 2D pos is inside the viewport
	inline bool check_in_screen(const Vec3d& pos) const;

	// Set the standard modelview matrices used for projection
	void set_modelview_matrices(const Mat4d& _mat_earth_equ_to_eye,
								const Mat4d& _mat_helio_to_eye,
								const Mat4d& _mat_local_to_eye);

	// Return in vector "win" the projection on the screen of point v in earth equatorial coordinate
	// according to the current modelview and projection matrices (reimplementation of gluProject)
	// Return true if the z screen coordinate is < 1, ie if it isn't behind the observer
	// except for the _check version which return true if the projected point is inside the screen
	bool project_earth_equ(const Vec3d& v, Vec3d& win) const;
	bool project_earth_equ(const Vec3f& v, Vec3d& win) const;
	bool project_earth_equ_check(const Vec3f& v, Vec3d& win) const;
	bool unproject_earth_equ(double x, double y, Vec3d& v) const;

	// Same function with input vector v in heliocentric coordinate
	bool project_helio(const Vec3f& v, Vec3d& win) const;
	bool project_helio_check(const Vec3f& v, Vec3d& win) const;
	bool unproject_helio(double x, double y, Vec3d& v) const;

	// Same function with input vector v in local coordinate
	bool project_local(const Vec3f& v, Vec3d& win) const;
	bool project_local(const Vec3d& v, Vec3d& win) const;
	bool project_local_check(const Vec3f& v, Vec3d& win) const;
	bool unproject_local(double x, double y, Vec3d& v) const;

	// Same function but using a custom modelview matrix
	bool project_custom(const Vec3f& v, Vec3d& win, const Mat4d& mat) const;
	bool project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const;
	bool project_custom_check(const Vec3f& v, Vec3d& win, const Mat4d& mat) const;
	bool unproject_custom(double x, double y, Vec3d& v, const Mat4d& mat) const;

	// Same function but using the currently openGL modelview matrix
	// This function replaces the gluProject() function
	bool project_current(const Vec3f& v, Vec3d& win);
	bool unproject_current(double x ,double y, Vec3d& v);

	// Set the drawing mode in 2D. Use reset_perspective_projection() to restore previous projection mode
	void set_orthographic_projection(void) const;

	// Restore the previous projection mode after a call to set_orthographic_projection()
	void reset_perspective_projection(void) const;

private:
	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	void init_project_matrix(void);

	// transformation from screen 2D point x,y to object
	// m is here the already inverted full tranfo matrix
	bool unproject(double x, double y, const Mat4d m, Vec3d& out) const;

	int screenW, screenH;
	double fov;					// Field of view
	float ratio;				// Screen ratio = screenW/screenH
	double zNear, zFar;			// Near and far clipping planes
	Vec4i vec_viewport;			// Viewport parameters
	Mat4d mat_projection;		// Projection matrix

	Mat4d mat_earth_equ_to_eye;	// Modelview Matrix for earth equatorial projection
	Mat4d mat_helio_to_eye;		// Modelview Matrix for earth equatorial projection
	Mat4d mat_local_to_eye;		// Modelview Matrix for earth equatorial projection
	Mat4d inv_mat_local_to_eye; // Inverse of mat_projection*mat_local_to_eye

	// Used to store temporary openGL modelview matrices
    GLdouble M[16];
};


#endif // _PROJECTOR_H_
