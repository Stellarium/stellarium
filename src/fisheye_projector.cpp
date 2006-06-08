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

#include "fisheye_projector.h"


FisheyeProjector::FisheyeProjector(const Vec4i& viewport, double _fov)
                 :CustomProjector(viewport, _fov)
{
	min_fov = 0.0001;
	max_fov = 180.00001;
	set_fov(_fov);
}
/*
bool FisheyeProjector::project_custom(const Vec3d &v,Vec3d &win,
                                      const Mat4d &mat) const
{
	double z;
	double a;

	win = v;
	win.transfo4d(mat);
	z = (win.length() - zNear) / (zFar-zNear);
	win.normalize();
	//win.transfo4d(mat_projection);
	a = fabs(M_PI_2 + asin(win[2]));
	win[2] = 0.;
	win.normalize();
	win = center + win * (a/fov * 180./M_PI * MY_MIN(vec_viewport[2],vec_viewport[3]));
	win[2] = z;
	return (a<0.9*M_PI) ? true : false;
}
*/

/*
bool FisheyeProjector::project_custom(const Vec3d &v,Vec3d &win,
                                      const Mat4d &mat) const {
  win = v;
  win.transfo4d(mat);
  const double win_length = win.length();
  const double a = M_PI_2 + asin(win[2]/win_length);
  const double f = (a*view_scaling_factor) / sqrt(win[0]*win[0]+win[1]*win[1]);
  win[0] = center[0] + win[0] * f;
  win[1] = center[1] + win[1] * f;
  win[2] = (win_length - zNear) / (zFar-zNear);
  return (a<0.9*M_PI) ? true : false;
}
*/

bool FisheyeProjector::project_custom(const Vec3d &v,Vec3d &win,
                                      const Mat4d &mat) const {
    // optimization by
    // 1) calling atan instead of asin (very good on Intel CPUs)
    // 2) calling sqrt only once
    // Interestingly on my Amd64 asin works slightly faster than atan
    // (although it is done in software!),
    // but the omitted sqrt is still worth it.
    // I think that for calculating win[2] we need no sqrt.
    // Johannes.
	// Fab) Removed one division
  win[0] = mat.r[0]*v[0] + mat.r[4]*v[1] +  mat.r[8]*v[2] + mat.r[12];
  win[1] = mat.r[1]*v[0] + mat.r[5]*v[1] +  mat.r[9]*v[2] + mat.r[13];
  win[2] = mat.r[2]*v[0] + mat.r[6]*v[1] + mat.r[10]*v[2] + mat.r[14];
  const double oneoverh = 1./sqrt(win[0]*win[0]+win[1]*win[1]);
  const double a = M_PI_2 + atan(win[2]*oneoverh);
//  modified fisheye
//  if (a > 0.5*M_PI) a = 0.25*(M_PI*M_PI)/(M_PI-a);
  const double f = (a*view_scaling_factor) * oneoverh;
  win[0] = center[0] + flip_horz * win[0] * f;
  win[1] = center[1] + flip_vert * win[1] * f;
  win[2] = (fabs(win[2]) - zNear) / (zFar-zNear);
  return (a<0.9*M_PI) ? true : false;
}


/*
more accurate version with atan2 instead of atan
bool FisheyeProjector::project_custom(const Vec3d &v,Vec3d &win,
                                      const Mat4d &mat) const {
  const double x = mat.r[0]*v[0] + mat.r[4]*v[1] +  mat.r[8]*v[2] + mat.r[12];
  const double y = mat.r[1]*v[0] + mat.r[5]*v[1] +  mat.r[9]*v[2] + mat.r[13];
  const double z = mat.r[2]*v[0] + mat.r[6]*v[1] + mat.r[10]*v[2] + mat.r[14];
  const double xyq = x*x+y*y;
  const double h = sqrt(xyq);
  double a = atan2(h,-z);
//  modified fisheye
//  if (a > 0.5*M_PI) a = 0.25*(M_PI*M_PI)/(M_PI-a);
  const double f = (a*view_scaling_factor) / h;
  win[0] = center[0] + flip_horz * x * f;
  win[1] = center[1] + flip_vert * y * f;
  win[2] = (sqrt(xyq+z*z) - zNear) / (zFar-zNear);
  return true;
  return (a<0.9*M_PI) ? true : false;
}
*/




void FisheyeProjector::unproject(double x, double y, const Mat4d& m, Vec3d& v) const
{
	double d = MY_MIN(vec_viewport[2],vec_viewport[3])/2;
	static double length;
	v[0] = flip_horz * (x - center[0]);
	v[1] = flip_vert * (y - center[1]);
	v[2] = 0;
	length = v.length();

	double angle_center = length/d * fov/2*M_PI/180;
// modified fisheye
//	if (angle_center > 0.5*M_PI) {
//		angle_center = M_PI*(1.0-M_PI/(4.0*angle_center));
//	}
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


