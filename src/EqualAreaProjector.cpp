/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Author 2007 Johannes Gajdosik
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

#include "EqualAreaProjector.hpp"


EqualAreaProjector::EqualAreaProjector(const Vec4i& viewport, double _fov)
                   :CustomProjector(viewport, _fov)
{
	min_fov = 0.0001;
	max_fov = 225.0;
	setFov(_fov);
}

bool EqualAreaProjector::project_custom(const Vec3d &v,Vec3d &win,
                                      const Mat4d &mat) const {
  win[0] = mat.r[0]*v[0] + mat.r[4]*v[1] +  mat.r[8]*v[2] + mat.r[12];
  win[1] = mat.r[1]*v[0] + mat.r[5]*v[1] +  mat.r[9]*v[2] + mat.r[13];
  win[2] = mat.r[2]*v[0] + mat.r[6]*v[1] + mat.r[10]*v[2] + mat.r[14];
  const double r = win.length();
  win *= (1.0/r);
  const double z1 = win[2] + 1.0;
  const double f = view_scaling_factor
                 * sqrt(1.0 + (z1*z1) / (win[0]*win[0]+win[1]*win[1]));
  win[0] = center[0] + flip_horz * win[0] * f;
  win[1] = center[1] + flip_vert * win[1] * f;
  win[2] = (r - zNear) / (zFar-zNear);
  return true;
}

/*
bool EqualAreaProjector::project_custom(const Vec3d &v,Vec3d &win,
                                      const Mat4d &mat) const {
  double x = mat.r[0]*v[0] + mat.r[4]*v[1] +  mat.r[8]*v[2] + mat.r[12];
  double y = mat.r[1]*v[0] + mat.r[5]*v[1] +  mat.r[9]*v[2] + mat.r[13];
  double z = mat.r[2]*v[0] + mat.r[6]*v[1] + mat.r[10]*v[2] + mat.r[14];
  const double r = sqrt(x*x+y*y+z*z);
  const double z1 = z + r;
  const double f = view_scaling_factor * sqrt(1.0 + (z1*z1) / (x*x+y*y));
  win[0] = center[0] + flip_horz * x * f;
  win[1] = center[1] + flip_vert * y * f;
  win[2] = (r - zNear) / (zFar-zNear);
  return true;
}
*/


void EqualAreaProjector::unproject(double x, double y, const Mat4d& m, Vec3d& v) const
{
  x = flip_horz * (x - center[0]) / view_scaling_factor;
  y = flip_vert * (y - center[1]) / view_scaling_factor;
  const double dq = x*x + y*y;
  double l = 1.0 - 0.25*dq;
  if (l < 0) {
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 1.0;
  } else {
    l = sqrt(l);
    v[0] = x * l;
    v[1] = y * l;
    v[2] = -(0.5*dq - 1.0); // why minus ?
  }
  v.transfo4d(m);
}


