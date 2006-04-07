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
                conf.get_double("spheric_mirror","projector_position_x",0.0),
                conf.get_double("spheric_mirror","projector_position_z",-0.2),
                conf.get_double("spheric_mirror","projector_position_y",1.0));
  const Vec3d mirror_position(
                conf.get_double("spheric_mirror","mirror_position_x",0.0),
                conf.get_double("spheric_mirror","mirror_position_z",0.0),
                conf.get_double("spheric_mirror","mirror_position_y",2.0));
  const double mirror_radius(conf.get_double("spheric_mirror",
                                             "mirror_radius",0.25));
  const double dome_radius(conf.get_double("spheric_mirror",
                                           "dome_radius",2.5));
  const double zenith_y(conf.get_double("spheric_mirror",
                                        "zenith_y",0.125));
  const double scaling_factor(conf.get_double("spheric_mirror",
                                              "scaling_factor",0.8));
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

bool SpericMirrorCalculator::retransform(double x,double y,
                                         Vec3d &v,Vec3d &v_x,Vec3d &v_y) const {
  x /= zoom_factor;
  const double dx = 1.0/zoom_factor;
  y /= zoom_factor;
  const double dy = 1.0/zoom_factor;

  v[0] = x;
  v[1] =   y*cos_alpha + sin_alpha;
  v[2] =  -y*sin_alpha + cos_alpha;

  v_x[0] = dx;
  v_x[1] = 0;
  v_x[2] = 0;

  v_y[0] = 0;
  v_y[1] =  dy*cos_alpha;
  v_y[2] = -dy*sin_alpha;

  const double vv = v.dot(v);
  const double vv_x = 2.0*v.dot(v_x);
  const double vv_y = 2.0*v.dot(v_y);
  
  const double Pv = P.dot(v);
  const double Pv_x = P.dot(v_x);
  const double Pv_y = P.dot(v_y);

  const double discr = Pv*Pv-(P.dot(P)-1.0)*vv;
  const double discr_x = 2.0*Pv*Pv_x-(P.dot(P)-1.0)*vv_x;
  const double discr_y = 2.0*Pv*Pv_y-(P.dot(P)-1.0)*vv_y;
  
  if (discr < 0) {
    return false;
  }
  const Vec3d Q = P + v*((-Pv-sqrt(discr))/vv);
  const Vec3d Q_x = v_x*((-Pv-sqrt(discr))/vv)
                  + v*( (vv*(-Pv_x-0.5*discr_x/sqrt(discr))
                        -vv_x*(-Pv-sqrt(discr))) /(vv*vv));
  const Vec3d Q_y = v_y*((-Pv-sqrt(discr))/vv)
                  + v*( (vv*(-Pv_y-0.5*discr_y/sqrt(discr))
                        -vv_y*(-Pv-sqrt(discr))) /(vv*vv));

  const Vec3d w = v - Q*(2*v.dot(Q));
  const Vec3d w_x = v_x - Q_x*(2*v.dot(Q)) - Q*(2*(v_x.dot(Q)+v.dot(Q_x)));
  const Vec3d w_y = v_y - Q_y*(2*v.dot(Q)) - Q*(2*(v_y.dot(Q)+v.dot(Q_y)));


  const Vec3d MQ = Q - DomeCenter;
  // MQ_x = Q_x
  // MQ_y = Q_y

  double f = -MQ.dot(w);
  double f_x = -Q_x.dot(w)-MQ.dot(w_x);
  double f_y = -Q_y.dot(w)-MQ.dot(w_y);

  double f1 = f + sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
  double f1_x = f_x + 0.5*(2*f*f_x - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv_x
                                   - 2*MQ.dot(Q_x)*vv )
              / sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
  double f1_y = f_y + 0.5*(2*f*f_y - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv_y
                                   - 2*MQ.dot(Q_y)*vv )
              / sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);

  const Vec3d S = Q + w*(f1/vv);
  const Vec3d S_x = Q_x + w*((vv*f1_x-vv_x*f1)/(vv*vv)) + w_x*(f1/vv);
  const Vec3d S_y = Q_y + w*((vv*f1_y-vv_y*f1)/(vv*vv)) + w_y*(f1/vv);

  v = S - DomeCenter;
  v_x = S_x;
  v_y = S_y;
  
  v *= (1.0/DomeRadius);
  v_x *= (1.0/DomeRadius);
  v_y *= (1.0/DomeRadius);
  return true;
}

