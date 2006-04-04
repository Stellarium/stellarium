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

#include "spheric_mirror_calculator.h"
#include "init_parser.h"

// This code slow and ugly and I know it.
// Yet it might be useful for playing around.

SpericMirrorCalculator::SpericMirrorCalculator(void) {
  setParams(Vec3d(0,-2,15),Vec3d(0,0,20),1,25,0.0/8.0,1.0);
}

void SpericMirrorCalculator::init(const InitParser &conf) {
  const Vec3d projector_position(
                conf.get_double("spheric_mirror","projector_position_x"),
                conf.get_double("spheric_mirror","projector_position_z"),
                conf.get_double("spheric_mirror","projector_position_y"));
  const Vec3d mirror_position(
                conf.get_double("spheric_mirror","mirror_position_x"),
                conf.get_double("spheric_mirror","mirror_position_z"),
                conf.get_double("spheric_mirror","mirror_position_y"));
  const double mirror_radius(conf.get_double("spheric_mirror","mirror_radius"));
  const double dome_radius(conf.get_double("spheric_mirror","dome_radius"));
  const double zenith_y(conf.get_double("spheric_mirror","zenith_y"));
  const double scaling_factor(conf.get_double("spheric_mirror","scaling_factor"));
  setParams(projector_position,
            mirror_position,
            mirror_radius,
            dome_radius,
            zenith_y,
            scaling_factor);
}

void SpericMirrorCalculator::setParams(const Vec3d &projector_position,
               const Vec3d &mirror_position,
               double mirror_radius,
               double dome_radius,
               double zenith_y,
               double scaling_factor) {
  DomeCenter = (- mirror_position) * (1.0/mirror_radius);
  DomeRadius = dome_radius / mirror_radius;
  P = (projector_position - mirror_position) * (1.0/mirror_radius);
  PP = P.dot(P);
  zoom_factor = sqrt(PP-1.0) * scaling_factor;
  lP = sqrt(PP);
  p = P * (1.0/lP);
  cos_alpha = 1.0;
  sin_alpha = 0.0;
  double x,y;
  transform(Vec3d(0,1,0),x,y);
  double alpha = atan(y/zoom_factor) - atan(zenith_y/zoom_factor);
  cos_alpha = cos(alpha);
  sin_alpha = sin(alpha);
}

bool SpericMirrorCalculator::transform(const Vec3d &v,
                                       double &xb,double &yb) const {
  const Vec3d S = DomeCenter + (v * (DomeRadius/v.length()));
  const Vec3d SmP = S - P;
  const double P_SmP = P.dot(SmP);
  const bool rval = ( (PP-1.0)*SmP.dot(SmP) > P_SmP*P_SmP );
  
  const double lS = S.length();
  const Vec3d s(S/lS);
  double t_min = 0;
  double t_max = 1;
  Vec3d Q;
  for (int i=0;i<10;i++) {
    const double t = 0.5 * (t_min+t_max);
    Q = p*t + s*(1.0-t);
    Q.normalize();
    Vec3d Qp = P-Q;
    Qp.normalize();
    Vec3d Qs = S-Q;
    Qs.normalize();
    if ( (Qp-Qs).dot(Q) > 0.0 ) {
      t_max = t;
    } else {
      t_min = t;
    }
  }
  Vec3d x = Q-P;
  const double zb = (cos_alpha*x[2] + sin_alpha*x[1]);

    // rotate
  xb = zoom_factor * x[0]/zb;
  yb = zoom_factor * (cos_alpha*x[1] - sin_alpha*x[2])/zb;
  return rval;
}


bool SpericMirrorCalculator::retransform(double x,double y,Vec3d &v) const {
  x /= zoom_factor;
  y /= zoom_factor;
  v[0] = x;
  v[1] =   y*cos_alpha + sin_alpha;
  v[2] =  -y*sin_alpha + cos_alpha;
  const double vv = v.dot(v);
  const double Pv = P.dot(v);
  const double discr = Pv*Pv-(P.dot(P)-1.0)*vv;
  if (discr < 0) {
    return false;
  }
  const Vec3d Q = P + v*((-Pv-sqrt(discr))/vv);
  const Vec3d w = v - Q*(2*v.dot(Q));
  const Vec3d MQ = Q - DomeCenter;
  double f = -MQ.dot(w);
  f += sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
  const Vec3d S = Q + w*(f/vv);
  v = S - DomeCenter;
  v *= (1.0/DomeRadius);
  return true;
}

