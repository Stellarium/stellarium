/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Author 2006 Johannes Gajdosik
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

#include "StereographicProjector.hpp"


StereographicProjector::StereographicProjector(const Vec4i& viewport,
                                               double _fov)
                       :CustomProjector(viewport, _fov) {
  min_fov = 0.001;
  max_fov = 270.00001;
  set_fov(_fov);
}

bool StereographicProjector::project_custom(const Vec3d &v,
                                            Vec3d &win,
                                            const Mat4d &mat) const {
  const double x = mat.r[0]*v[0] + mat.r[4]*v[1] +  mat.r[8]*v[2] + mat.r[12];
  const double y = mat.r[1]*v[0] + mat.r[5]*v[1] +  mat.r[9]*v[2] + mat.r[13];
  const double z = mat.r[2]*v[0] + mat.r[6]*v[1] + mat.r[10]*v[2] + mat.r[14];
  const double r = sqrt(x*x+y*y+z*z);
  const double h = 0.5*(r-z);
  if (h <= 0.0) return false;
  const double f = view_scaling_factor / h;
  win[0] = center[0] + flip_horz * x * f;
  win[1] = center[1] + flip_vert * y * f;
  win[2] = (r - zNear) / (zFar-zNear);
  return true;
}


void StereographicProjector::unproject(double x, double y,
                                       const Mat4d& m, Vec3d& v) const {
  x = flip_horz * (x - center[0]) / (view_scaling_factor*2);
  y = flip_vert * (y - center[1]) / (view_scaling_factor*2);
  const double lq = x*x + y*y;
  v[0] = 2.0 * x;
  v[1] = 2.0 * y;
  v[2] = - (lq - 1.0); // why minus ?
  v *= (1.0 / (lq + 1.0));
//cout << "StereographicProjector::unproject: before("
//     << v[0] << ',' << v[1] << ',' << v[2] << ')' << endl;
  v.transfo4d(m);
//cout << "StereographicProjector::unproject: after ("
//     << v[0] << ',' << v[1] << ',' << v[2] << ')' << endl;
}

