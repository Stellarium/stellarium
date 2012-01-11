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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _SPHERICMIRRORCALCULATOR_HPP_
#define _SPHERICMIRRORCALCULATOR_HPP_

#include "VecMath.hpp"

class QSettings;

class SphericMirrorCalculator {
public:
  SphericMirrorCalculator(const QSettings& conf);
  float getHorzZoomFactor() const {return horzZoomFactor;}
  float getVertZoomFactor() const {return vertZoomFactor;}
  bool transform(const Vec3f &v,float &x,float &y) const;
  bool retransform(float x,float y,Vec3f &v) const;
    // for calculating partial derivatives:
  bool retransform(float x,float y, Vec3f& v, Vec3f& vX, Vec3f& vY) const;
private:
  void initRotMatrix(float alpha,float delta,float phi);
private:
  Vec3f P;          // projector
  Vec3f DomeCenter;
  float DomeRadius;
  float PP;
  float lP;
  Vec3f p;
  float horzZoomFactor;
  float vertZoomFactor;
  float alphaDeltaPhi[9];
};

#endif // _SPHERICMIRRORCALCULATOR_HPP_
