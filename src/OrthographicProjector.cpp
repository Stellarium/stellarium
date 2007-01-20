/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Author of this file: 2007 Johannes Gajdosik
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

#include "OrthographicProjector.hpp"

#include <math.h>

OrthographicProjector::OrthographicProjector(const Vec4i& viewport,
                                             double _fov)
                      :CustomProjector(viewport, _fov) {
  min_fov = 0.001;
  max_fov = 120.0;
  setFov(_fov);
}

bool OrthographicProjector::project_custom(const Vec3d &v,
                                           Vec3d &win,
                                           const Mat4d &mat) const {
  win = v;
  win.transfo4d(mat);
  const double f = view_scaling_factor / win.length();
  win[0] = center[0] + flip_horz * win[0] * f;
  win[1] = center[1] + flip_vert * win[1] * f;
  win[2] = (-win[2] - zNear) / (zFar-zNear);
  return (win[2] >= 0.0);
}

void OrthographicProjector::unproject(double x, double y,
                                      const Mat4d& m, Vec3d& v) const {
  v[0] = flip_horz * (x - center[0]) / view_scaling_factor;
  v[1] = flip_vert * (y - center[1]) / view_scaling_factor;
  const double h = 1.0 - v[0]*v[0] - v[1]*v[1];
  v[2] = (h < 0) ? 1.0 : sqrt(h);  // why not minus ?
  v.transfo4d(m);
}

