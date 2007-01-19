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
  max_fov = 270.0;
  setFov(_fov);
}

bool CylinderProjector::project_custom(const Vec3d &v,
                                       Vec3d &win,
                                       const Mat4d &mat) const {
  win = v;
  win.transfo4d(mat);
  const double win_length = win.length();
  const double alpha = atan2(win[0],-win[2]);
  const double delta = asin(win[1]/win_length);
  win[0] = center[0] + flip_horz * alpha*view_scaling_factor;
  win[1] = center[1] + flip_vert * delta*view_scaling_factor;
  win[2] = (win_length - zNear) / (zFar-zNear);
  return true;
}

void CylinderProjector::unproject(double x, double y,
                                  const Mat4d& m, Vec3d& v) const {
  const double alpha = flip_horz * (x - center[0]) / view_scaling_factor;
  const double delta = flip_vert * (y - center[1]) / view_scaling_factor;
  const double cd = cos(delta);
  v[0] = cd * sin(alpha);
  v[1] = sin(delta);
  v[2] = cd * cos(alpha); // why not minus ?
  v.transfo4d(m);
}

