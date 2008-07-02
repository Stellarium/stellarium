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

#ifndef _SPHERICMIRRORCALCULATOR_HPP_
#define _SPHERICMIRRORCALCULATOR_HPP_

#include "vecmath.h"

class QSettings;

class SphericMirrorCalculator {
public:
  SphericMirrorCalculator(const QSettings& conf);
  double getHorzZoomFactor(void) const {return horzZoomFactor;}
  double getVertZoomFactor(void) const {return vertZoomFactor;}
  bool transform(const Vec3d &v,double &x,double &y) const;
  bool retransform(double x,double y,Vec3d &v) const;
    // for calculating partial derivatives:
  bool retransform(double x,double y,Vec3d &v,Vec3d &vX,Vec3d &vY) const;
private:
  void initRotMatrix(double alpha,double delta,double phi);
private:
  Vec3d P;          // projector
  Vec3d DomeCenter;
  double DomeRadius;
  double PP;
  double lP;
  Vec3d p;
  double horzZoomFactor;
  double vertZoomFactor;
  double alphaDeltaPhi[9];
};

#endif // _SPHERICMIRRORCALCULATOR_HPP_
