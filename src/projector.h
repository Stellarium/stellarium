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

#include "stellarium.h"
#include "vecmath.h"

// Class which handle projection modes and projection matrix
// Overide some function usually handled by glu
class Projector
{
public:
	Projector(int _screenW = 800, int _screenH = 600, double _fov = 60.,
		double _min_fov = 0.001, double _max_fov = 100);
	virtual ~Projector();

	void set_fov(double f);
	double get_fov(void) const {return fov;}
	virtual void change_fov(double deltaFov);

	void set_screen_size(int w, int h);
	void maximize_viewport(void) {set_viewport(0,0,screenW,screenH);}
	void set_square_viewport(void);
	void set_disk_viewport(void);
	virtual void set_viewport(int x, int y, int w, int h);

	int viewW(void) const {return vec_viewport[2];}
	int viewH(void) const {return vec_viewport[3];}

	void set_clipping_planes(double znear, double zfar);

	// Return true if the 2D pos is inside the viewport
	bool check_in_viewport(const Vec3d& pos) const
	{	return 	pos[1]>vec_viewport[1] && pos[1]<vec_viewport[3] &&
				pos[0]>vec_viewport[0] && pos[0]<vec_viewport[2];}

	// Set the standard modelview matrices used for projection
	void set_modelview_matrices(const Mat4d& _mat_earth_equ_to_eye,
								const Mat4d& _mat_helio_to_eye,
								const Mat4d& _mat_local_to_eye);

	// Return in vector "win" the projection on the screen of point v in earth equatorial coordinate
	// according to the current modelview and projection matrices (reimplementation of gluProject)
	// Return true if the z screen coordinate is < 1, ie if it isn't behind the observer
	// except for the _check version which return true if the projected point is inside the screen
	bool project_earth_equ(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_earth_equ_to_eye);}

	bool project_earth_equ_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_earth_equ_to_eye);}

	void unproject_earth_equ(double x, double y, Vec3d& v) const
		{unproject(x, y, inv_mat_earth_equ_to_eye, v);}


	// Same function with input vector v in heliocentric coordinate
	bool project_helio_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_helio_to_eye);}

	void unproject_helio(double x, double y, Vec3d& v) const
		{return unproject(x, y, inv_mat_helio_to_eye, v);}


	// Same function with input vector v in local coordinate
	bool project_local(const Vec3d& v, Vec3d& win) const
		{return project_custom(v, win, mat_local_to_eye);}

	bool project_local_check(const Vec3d& v, Vec3d& win) const
		{return project_custom_check(v, win, mat_local_to_eye);}

	void unproject_local(double x, double y, Vec3d& v) const
		{unproject(x, y, inv_mat_local_to_eye, v);}

	// Same function but using a custom modelview matrix
	virtual bool project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const;
	virtual bool project_custom_check(const Vec3f& v, Vec3d& win, const Mat4d& mat) const
		{return project_custom(v, win, mat) && check_in_viewport(win);}

	virtual void unproject_custom(double x, double y, Vec3d& v, const Mat4d& mat) const;

	// Set the drawing mode in 2D for drawing in the full screen
	// Use restore_from_2Dfullscreen_projection() to restore previous projection mode
	void set_2Dfullscreen_projection(void) const;
	void restore_from_2Dfullscreen_projection(void) const;

	// Set the drawing mode in 2D for drawing inside the viewport only.
	// Use reset_perspective_projection() to restore previous projection mode
	void set_orthographic_projection(void) const;

	// Restore the previous projection mode after a call to set_orthographic_projection()
	void reset_perspective_projection(void) const;

	// Reimplementation of gluSphere : glu is overrided for non standard projection
	virtual void sSphere(GLdouble radius, GLint slices, GLint stacks,
		const Mat4d& mat, int orient_inside = 0) const;

	void sVertex3(float x, float y, float z, const Mat4d& mat) const
	{glVertex3f(x,y,z);}

	void sVertex3(double x, double y, double z, const Mat4d& mat) const
	{glVertex3d(x,y,z);}

	void update_openGL(void) const;

	const Vec3d convert_pos(const Vec3d& v, const Mat4d& mat) const {return v;}

protected:
	// Init the viewing matrix from the fov, the clipping planes and screen ratio
	// The function is a reimplementation of gluPerspective
	virtual void init_project_matrix(void);

	int screenW, screenH;
	double fov;					// Field of view
	double min_fov;				// Minimum fov
	double max_fov;				// Maximum fov
	float ratio;				// Screen ratio = screenW/screenH
	double zNear, zFar;			// Near and far clipping planes
	Vec4i vec_viewport;			// Viewport parameters
	Mat4d mat_projection;		// Projection matrix

	Mat4d mat_earth_equ_to_eye;		// Modelview Matrix for earth equatorial projection
	Mat4d mat_helio_to_eye;			// Modelview Matrix for earth equatorial projection
	Mat4d mat_local_to_eye;			// Modelview Matrix for earth equatorial projection
	Mat4d inv_mat_earth_equ_to_eye;	// Inverse of mat_projection*mat_earth_equ_to_eye
	Mat4d inv_mat_helio_to_eye;		// Inverse of mat_projection*mat_helio_to_eye
	Mat4d inv_mat_local_to_eye;		// Inverse of mat_projection*mat_local_to_eye

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


#endif // _PROJECTOR_H_
