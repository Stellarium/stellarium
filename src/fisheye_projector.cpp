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


FisheyeProjector::FisheyeProjector(int _screenW, int _screenH, double _fov)
                 :CustomProjector(_screenW, _screenH, _fov)
{
	min_fov = 0.0001;
	max_fov = 175.00001;
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
  win = v;
  win.transfo4d(mat);
  const double h = sqrt(win[0]*win[0]+win[1]*win[1]);
  const double a = M_PI_2 + atan(win[2]/h);
  const double f = (a*view_scaling_factor) / h;
  win[0] = center[0] + win[0] * f;
  win[1] = center[1] + win[1] * f;
  win[2] = (fabs(win[2]) - zNear) / (zFar-zNear);
  return (a<0.9*M_PI) ? true : false;
}

void FisheyeProjector::unproject(double x, double y, const Mat4d& m, Vec3d& v) const
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


