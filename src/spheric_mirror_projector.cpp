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

#include "spheric_mirror_projector.h"

SphericMirrorProjector::SphericMirrorProjector(const Vec4i& viewport,
                                               double _fov)
                       :CustomProjector(viewport, _fov) {
  min_fov = 0.001;
  max_fov = 180.00001;
  set_fov(_fov);
}

void SphericMirrorProjector::setViewport(int x, int y, int w, int h)
{
	Projector::setViewport(x, y, w, h);
	center.set(vec_viewport[0]+vec_viewport[2]/2,
               vec_viewport[1]+4*vec_viewport[3]/8,0);
}

bool SphericMirrorProjector::project_custom(const Vec3d &v,
                                            Vec3d &x,
                                            const Mat4d &mat) const {
  Vec3d S = v;
  S.transfo4d(mat);

  const double z = S[2];
  S[2] = S[1];
  S[1] = -z;
  
  const bool rval = calc.transform(S,x[0],x[1]);

  x[0] = center[0] - flip_horz * x[0] * (5*view_scaling_factor);
  x[1] = center[1] + flip_vert * x[1] * (5*view_scaling_factor);
  x[2] = rval ? ((-z - zNear) / (zFar-zNear)) : -1000;
  return rval;
}


void SphericMirrorProjector::unproject(double x, double y,
                                       const Mat4d& m, Vec3d& v) const {
  x = - flip_horz * (x - center[0]) / (5*view_scaling_factor);
  y = flip_vert * (y - center[1]) / (5*view_scaling_factor);

  calc.retransform(x,y,v);

  v[2] = -v[2]; // why

  x = v[1];
  v[1] = -v[2];
  v[2] = x;
  v.transfo4d(m);
}

