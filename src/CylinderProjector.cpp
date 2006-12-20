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

#include "CylinderProjector.hpp"


CylinderProjector::CylinderProjector(const Vec4i& viewport,
                                     double _fov)
                  :CustomProjector(viewport, _fov) {
  min_fov = 0.001;
  max_fov = 500.00001;
  set_fov(_fov);
}

/*
bool CylinderProjector::project_custom(const Vec3d& v, Vec3d& win, const Mat4d& mat) const
{
	double z;
	double alpha, delta;

	win = v;
	win.transfo4d(mat);
	z = (win.length() - zNear) / (zFar-zNear);
	win.normalize();

	alpha = asin(win[1]);
	delta = atan2(win[2],win[0]);

	win[0] = center[0] + delta/fov * 180./M_PI * MY_MIN(vec_viewport[2],vec_viewport[3]);
	win[1] = center[1] + alpha/fov * 180./M_PI * MY_MIN(vec_viewport[2],vec_viewport[3]);
	win[2] = z;
	return true;//(a<0.9*M_PI) ? true : false;
}
*/

bool CylinderProjector::project_custom(const Vec3d &v,
                                       Vec3d &win,
                                       const Mat4d &mat) const {
  win = v;
  win.transfo4d(mat);
  const double win_length = win.length();
  const double alpha = asin(win[1]/win_length);
  const double delta = atan2(win[2],win[0]);
  win[0] = center[0] + flip_horz * delta*view_scaling_factor;
  win[1] = center[1] + flip_vert * alpha*view_scaling_factor;
  win[2] = (win_length - zNear) / (zFar-zNear);
  return true;
}

void CylinderProjector::unproject(double x, double y,
                                  const Mat4d& m, Vec3d& v) const {
  const double d = flip_horz * (x - center[0]) / view_scaling_factor;
  const double a = flip_vert * (y - center[1]) / view_scaling_factor;
  v[0] = cos(a) * cos(d);
  v[1] = sin(a);
  v[2] = - cos(a) * sin(d); // why minus ?
  v.transfo4d(m);
}


