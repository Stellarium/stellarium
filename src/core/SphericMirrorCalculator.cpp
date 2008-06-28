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

#include "SphericMirrorCalculator.hpp"
#include <QSettings>
#include <QDebug>

SphericMirrorCalculator::SphericMirrorCalculator(const QSettings& conf) {
  const Vec3d mirror_position(
conf.value("spheric_mirror/mirror_position_x",0.0).toDouble(),
conf.value("spheric_mirror/mirror_position_y",2.0).toDouble(),
conf.value("spheric_mirror/mirror_position_z",0.0).toDouble());
  const double mirror_radius(conf.value("spheric_mirror/mirror_radius",0.25).toDouble());
  DomeCenter = mirror_position * (-1.0/mirror_radius);
  const double dome_radius(conf.value("spheric_mirror/dome_radius",2.5).toDouble());
  DomeRadius = dome_radius / mirror_radius;
  const Vec3d projector_position(
conf.value("spheric_mirror/projector_position_x",0.0).toDouble(),
conf.value("spheric_mirror/projector_position_y",1.0).toDouble(),
conf.value("spheric_mirror/projector_position_z",-0.2).toDouble());
  P = (projector_position - mirror_position) * (1.0/mirror_radius);
  PP = P.lengthSquared();
  lP = sqrt(PP);
  p = P * (1.0/lP);
  double image_distance_div_height
			  = conf.value("spheric_mirror/image_distance_div_height",-1e100).toDouble();
  if (image_distance_div_height <= -1e100)
  { const double scaling_factor = conf.value("spheric_mirror/scaling_factor", 0.8).toDouble();
    image_distance_div_height = sqrt(PP-1.0) * scaling_factor;
    qDebug() << "INFO: spheric_mirror:scaling_factor is deprecated and may be removed in future versions.";
    qDebug() << "      In order to keep your setup unchanged, please use spheric_mirror:image_distance_div_height = "
             << image_distance_div_height << " instead";
  }
  horzZoomFactor = conf.value("spheric_mirror/flip_horz",true).toBool()
                   ? (-image_distance_div_height)
                   : image_distance_div_height;
  vertZoomFactor = conf.value("spheric_mirror/flip_vert",false).toBool()
                   ? (-image_distance_div_height)
                   : image_distance_div_height;

  const double alpha = conf.value("spheric_mirror/projector_alpha",0.0).toDouble() * (M_PI/180);
  const double phi = conf.value("spheric_mirror/projector_phi",0.0).toDouble() * (M_PI/180);
  double delta = conf.value("spheric_mirror/projector_delta",-1e100).toDouble();
  if (delta <= -1e100) {
    double x,y;
      // before calling transform() horzZoomFactor,vertZoomFactor,
      // alphaDeltaPhi must already be initialized
    initRotMatrix(0.0,0.0,0.0);
    transform(Vec3d(0,0,1),x,y);
	const double zenith_y(conf.value("spheric_mirror/zenith_y",0.125).toDouble());
    delta = -atan(y/image_distance_div_height)
          + atan(zenith_y/image_distance_div_height);
    qDebug() << "INFO: spheric_mirror:zenith_y is deprecated and may be removed in future versions.";
    qDebug() << "      In order to keep your setup unchanged, please use spheric_mirror:projector_delta = "
             << (delta*(180.0/M_PI)) << " instead";
  } else {
    delta *= (M_PI/180);
  }
  initRotMatrix(alpha,delta,phi);
}

void SphericMirrorCalculator::initRotMatrix(double alpha,
                                            double delta,
                                            double phi) {
  const double ca = cos(alpha);
  const double sa = sin(alpha);
  const double cd = cos(delta);
  const double sd = sin(delta);
  const double cp = cos(phi);
  const double sp = sin(phi);

  alphaDeltaPhi[0] =   ca*cp - sa*sd*sp;
  alphaDeltaPhi[1] = - sa*cp - ca*sd*sp;
  alphaDeltaPhi[2] =            - cd*sp;

  alphaDeltaPhi[3] =           sa*cd;
  alphaDeltaPhi[4] =           ca*cd;
  alphaDeltaPhi[5] =            - sd;
  
  alphaDeltaPhi[6] = - ca*sp - sa*sd*cp;
  alphaDeltaPhi[7] =   sa*sp - ca*sd*cp;
  alphaDeltaPhi[8] =            - cd*cp;
/*
    // check if alphaDeltaPhi is an orthogonal matrix:
  for (int i=0;i<3;i++) {
    for (int j=0;j<3;j++) {
      double prod0 = 0;
      double prod1 = 0;
      for (int k=0;k<3;k++) {
        prod0 += alphaDeltaPhi[3*i+k]*alphaDeltaPhi[3*j+k];
        prod1 += alphaDeltaPhi[i+3*k]*alphaDeltaPhi[j+3*k];
      }
      if (i==j) {
        prod0 -= 1.0;
        prod1 -= 1.0;
      }
      if (fabs(prod0)>1e-10) {
        qDebug() << "i: " << i << ", j: " << j
                 << ", prod0: " << prod0 << ", prod1: " << prod1;
        assert(0);
      }
      if (fabs(prod1)>1e-10) {
        qDebug << "i: " << i << ", j: " << j
               << ", prod0: " << prod0 << ", prod1: " << prod1;
        assert(0);
      }
    }
  }
*/
}


bool SphericMirrorCalculator::transform(const Vec3d &v,
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
    // more iterations would be more accurate,
    // but I keep this number of iterations for exact zenith_y compatibility:
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

    // rotate
  const double zb =
    alphaDeltaPhi[1]*x[0] + alphaDeltaPhi[4]*x[1] + alphaDeltaPhi[7]*x[2];
  xb = (horzZoomFactor/zb) *
   (alphaDeltaPhi[0]*x[0] + alphaDeltaPhi[3]*x[1] + alphaDeltaPhi[6]*x[2]);
  yb = (vertZoomFactor/zb) *
   (alphaDeltaPhi[2]*x[0] + alphaDeltaPhi[5]*x[1] + alphaDeltaPhi[8]*x[2]);

  return rval;
}


bool SphericMirrorCalculator::retransform(double x,double y,Vec3d &v) const {
  x /= horzZoomFactor;
  y /= vertZoomFactor;
  v[0] = alphaDeltaPhi[0]*x + alphaDeltaPhi[1] + alphaDeltaPhi[2]*y;
  v[1] = alphaDeltaPhi[3]*x + alphaDeltaPhi[4] + alphaDeltaPhi[5]*y;
  v[2] = alphaDeltaPhi[6]*x + alphaDeltaPhi[7] + alphaDeltaPhi[8]*y;
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

bool SphericMirrorCalculator::retransform(double x,double y,
                                          Vec3d &v,
                                          Vec3d &vX,Vec3d &vY) const {
  x /= horzZoomFactor;
  const double dx = 1.0/horzZoomFactor;
  y /= vertZoomFactor;
  const double dy = 1.0/vertZoomFactor;

  v[0] = alphaDeltaPhi[0]*x + alphaDeltaPhi[1] + alphaDeltaPhi[2]*y;
  v[1] = alphaDeltaPhi[3]*x + alphaDeltaPhi[4] + alphaDeltaPhi[5]*y;
  v[2] = alphaDeltaPhi[6]*x + alphaDeltaPhi[7] + alphaDeltaPhi[8]*y;

  vX[0] = alphaDeltaPhi[0]*dx;
  vX[1] = alphaDeltaPhi[3]*dx;
  vX[2] = alphaDeltaPhi[6]*dx;

  vY[0] = alphaDeltaPhi[2]*dy;
  vY[1] = alphaDeltaPhi[5]*dy;
  vY[2] = alphaDeltaPhi[8]*dy;

  const double vv = v.dot(v);
  const double vvX = 2.0*v.dot(vX);
  const double vvY = 2.0*v.dot(vY);
  
  const double Pv = P.dot(v);
  const double PvX = P.dot(vX);
  const double PvY = P.dot(vY);

  const double discr = Pv*Pv-(P.dot(P)-1.0)*vv;
  const double discr_x = 2.0*Pv*PvX-(P.dot(P)-1.0)*vvX;
  const double discr_y = 2.0*Pv*PvY-(P.dot(P)-1.0)*vvY;
  
  if (discr < 0) {
    return false;
  }
  const Vec3d Q = P + v*((-Pv-sqrt(discr))/vv);
  const Vec3d Q_x = vX*((-Pv-sqrt(discr))/vv)
                  + v*( (vv*(-PvX-0.5*discr_x/sqrt(discr))
                        -vvX*(-Pv-sqrt(discr))) /(vv*vv));
  const Vec3d Q_y = vY*((-Pv-sqrt(discr))/vv)
                  + v*( (vv*(-PvY-0.5*discr_y/sqrt(discr))
                        -vvY*(-Pv-sqrt(discr))) /(vv*vv));

  const Vec3d w = v - Q*(2*v.dot(Q));
  const Vec3d w_x = vX - Q_x*(2*v.dot(Q)) - Q*(2*(vX.dot(Q)+v.dot(Q_x)));
  const Vec3d w_y = vY - Q_y*(2*v.dot(Q)) - Q*(2*(vY.dot(Q)+v.dot(Q_y)));


  const Vec3d MQ = Q - DomeCenter;
  // MQ_x = Q_x
  // MQ_y = Q_y

  double f = -MQ.dot(w);
  double f_x = -Q_x.dot(w)-MQ.dot(w_x);
  double f_y = -Q_y.dot(w)-MQ.dot(w_y);

  double f1 = f + sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
  double f1_x = f_x + 0.5*(2*f*f_x - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vvX
                                   - 2*MQ.dot(Q_x)*vv )
              / sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
  double f1_y = f_y + 0.5*(2*f*f_y - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vvY
                                   - 2*MQ.dot(Q_y)*vv )
              / sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);

  const Vec3d S = Q + w*(f1/vv);
  const Vec3d S_x = Q_x + w*((vv*f1_x-vvX*f1)/(vv*vv)) + w_x*(f1/vv);
  const Vec3d S_y = Q_y + w*((vv*f1_y-vvY*f1)/(vv*vv)) + w_y*(f1/vv);

  v = S - DomeCenter;
  vX = S_x;
  vY = S_y;
  
  v *= (1.0/DomeRadius);
  vX *= (1.0/DomeRadius);
  vY *= (1.0/DomeRadius);
  return true;
}

