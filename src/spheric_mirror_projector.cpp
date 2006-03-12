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


// This code slow and ugly and I know it.
// I know that left-right must be switched.
// Yet it might be useful for playing around.


  // assume 1 = radius of mirror
  //       (0,0,0) = center of mirror
static const Vec3d P(0,-2,-5);          // center projector
static const double alpha = 28.74373*M_PI/180; // 24.045*M_PI/180;
static const Vec3d DomeCenter(0,0,-20);
static const double DomeRadius = 25.0;

static const double PP = P.dot(P);
static const double lP = P.length();
static const Vec3d p(P/lP);

static double cos_alpha,sin_alpha;

SphericMirrorProjector::SphericMirrorProjector(const Vec4i& viewport,
                                               double _fov)
                       :CustomProjector(viewport, _fov) {
  min_fov = 0.001;
  max_fov = 180.00001;
  set_fov(_fov);
  
  cos_alpha = cos(alpha);
  sin_alpha = sin(alpha);

    // we have a mirrored image:
  glFrontFace(GL_CW);
}

void SphericMirrorProjector::setViewport(int x, int y, int w, int h)
{
	Projector::setViewport(x, y, w, h);
	center.set(vec_viewport[0]+vec_viewport[2]/2,
               vec_viewport[1]+5*vec_viewport[3]/8,0);
}

bool SphericMirrorProjector::project_custom(const Vec3d &v,
                                            Vec3d &x,
                                            const Mat4d &mat) const {
  Vec3d S = v;
  S.transfo4d(mat);

  const double z = S[2];
  S[2] = S[1];
  S[1] = -z;

  const double R = S.length();
  S *= (DomeRadius/R);
  S += DomeCenter;
  
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
  
  x = Q-P;
    // rotate
  x.normalize();
  x[1] = cos_alpha*x[1] - sin_alpha*x[2];
  // x[2] does not matter:
  // x[2] = sin_alpha*x[1] + cos_alpha*x[2];

  x[0] = center[0] - x[0] * (view_scaling_factor*20);
  x[1] = center[1] + x[1] * (view_scaling_factor*20);
  x[2] = rval ? ((-z - zNear) / (zFar-zNear)) : -1000;
  return rval;
}


void SphericMirrorProjector::unproject(double x, double y,
                                       const Mat4d& m, Vec3d& v) const {
  x = - (x - center[0]) / (view_scaling_factor*20);
  y = (y - center[1]) / (view_scaling_factor*20);

  v[0] = x;
  v[1] =   y*cos_alpha + sin_alpha;
  v[2] =  -y*sin_alpha + cos_alpha;

  const double vv = v.dot(v);
  const double Pv = P.dot(v);
  const double discr = Pv*Pv-(P.dot(P)-1.0)*vv;
  if (discr < 0) {
//cout << "discr < 0" << endl;
  } else {
    const Vec3d Q = P + v*((-Pv-sqrt(discr))/vv);
    const Vec3d w = v - Q*(2*v.dot(Q));
    const Vec3d MQ = Q - DomeCenter;
    double f = -MQ.dot(w);
    f += sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
    const Vec3d S = Q + w*(f/vv);
    v = S - DomeCenter;

    v[2] = -v[2]; // why
    v *= (1.0/DomeRadius);

    x = v[1];
    v[1] = -v[2];
    v[2] = x;
//cout << "SphericMirrorProjector::unproject: before("
//     << v[0] << ',' << v[1] << ',' << v[2] << ')' << endl;
    v.transfo4d(m);
  }
}

