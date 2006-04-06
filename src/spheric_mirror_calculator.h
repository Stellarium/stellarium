/*
 * Stellarium
 * Copyright (C) Fabien Chereau
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

#ifndef _SPHERIC_MIRROR_CALCULATOR_H_
#define _SPHERIC_MIRROR_CALCULATOR_H_

#include "vecmath.h"

class InitParser;

class SpericMirrorCalculator {
public:
  SpericMirrorCalculator(void);
  void init(const InitParser &conf);
  void setParams(const Vec3d &projector_position,
                 const Vec3d &mirror_position,
                 double mirror_radius,
                 double dome_radius,
                 double zenith_y,
                 double scaling_factor);
  bool transform(const Vec3d &v,double &x,double &y) const;
  bool retransform(double x,double y,Vec3d &v) const;
    // for calculating partial derivatives:
  bool retransform(double x,double y,Vec3d &v,Vec3d &v_x,Vec3d &v_y) const;
private:
  Vec3d P;          // projector
  Vec3d DomeCenter;
  double DomeRadius;
  double PP;
  double lP;
  Vec3d p;
  double cos_alpha,sin_alpha;
  double zoom_factor;
};

#endif
