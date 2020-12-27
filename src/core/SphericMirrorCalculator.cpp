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

#include "SphericMirrorCalculator.hpp"
#include <QSettings>
#include <QDebug>
#include "StelUtils.hpp"

SphericMirrorCalculator::SphericMirrorCalculator(const QSettings& conf)
{
	const Vec3f mirror_position(
				conf.value("spheric_mirror/mirror_position_x",0.0).toFloat(),
				conf.value("spheric_mirror/mirror_position_y",2.0).toFloat(),
				conf.value("spheric_mirror/mirror_position_z",0.0).toFloat());
	const float mirror_radius(conf.value("spheric_mirror/mirror_radius",0.25).toFloat());
	DomeCenter = mirror_position * (-1.0f/mirror_radius);
	const float dome_radius(conf.value("spheric_mirror/dome_radius",2.5).toFloat());
	DomeRadius = dome_radius / mirror_radius;
	const Vec3f projector_position(
				conf.value("spheric_mirror/projector_position_x",0.0).toFloat(),
				conf.value("spheric_mirror/projector_position_y",1.0).toFloat(),
				conf.value("spheric_mirror/projector_position_z",-0.2).toFloat());
	P = (projector_position - mirror_position) * (1.0f/mirror_radius);
	PP = P.lengthSquared();
	lP = std::sqrt(PP);
	p = P * (1.0f/lP);
	float image_distance_div_height = conf.value("spheric_mirror/image_distance_div_height",-1e38f).toFloat();
	if (image_distance_div_height <= -1e38f)
	{
		const float scaling_factor = conf.value("spheric_mirror/scaling_factor", 0.8).toFloat();
		image_distance_div_height = std::sqrt(PP-1.0f) * scaling_factor;
		qDebug() << "INFO: spheric_mirror:scaling_factor is deprecated and may be removed in future versions.";
		qDebug() << "      In order to keep your setup unchanged, please use spheric_mirror:image_distance_div_height = "
				<< image_distance_div_height << " instead";
	}
	horzZoomFactor = conf.value("spheric_mirror/flip_horz",true).toBool() ? (-image_distance_div_height) : image_distance_div_height;
	vertZoomFactor = conf.value("spheric_mirror/flip_vert",false).toBool() ? (-image_distance_div_height) : image_distance_div_height;

	const float alpha = conf.value("spheric_mirror/projector_alpha",0.0f).toFloat() * (M_PI_180f);
	const float phi = conf.value("spheric_mirror/projector_phi",0.0f).toFloat() * (M_PI_180f);
	float delta = conf.value("spheric_mirror/projector_delta",-1e38f).toFloat();
	if (delta <= -1e38f)
	{
		float x,y;
		// before calling transform() horzZoomFactor,vertZoomFactor,
		// alphaDeltaPhi must already be initialized
		initRotMatrix(0.0,0.0,0.0);
		transform(Vec3f(0,0,1),x,y);
		const float zenith_y(conf.value("spheric_mirror/zenith_y",0.125).toFloat());
		delta = -atan(y/image_distance_div_height) + atan(zenith_y/image_distance_div_height);
		qDebug() << "INFO: spheric_mirror:zenith_y is deprecated and may be removed in future versions.";
		qDebug() << "      In order to keep your setup unchanged, please use spheric_mirror:projector_delta = "
				<< (delta*M_180_PIf) << " instead";
	}
	else
		delta *= M_PI_180f;

	initRotMatrix(alpha,delta,phi);
}

void SphericMirrorCalculator::initRotMatrix(float alpha, float delta, float phi)
{
	const float ca = cos(alpha);
	const float sa = sin(alpha);
	const float cd = cos(delta);
	const float sd = sin(delta);
	const float cp = cos(phi);
	const float sp = sin(phi);

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
	  float prod0 = 0;
	  float prod1 = 0;
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
	Q_ASSERT(0);
      }
      if (fabs(prod1)>1e-10) {
	qDebug << "i: " << i << ", j: " << j
	       << ", prod0: " << prod0 << ", prod1: " << prod1;
	Q_ASSERT(0);
      }
    }
  }
*/
}


bool SphericMirrorCalculator::transform(const Vec3f &v, float &xb,float &yb) const
{
	const Vec3f S = DomeCenter + (v * (DomeRadius/v.length()));
	const Vec3f SmP = S - P;
	const float P_SmP = P.dot(SmP);
	const bool rval = ( (PP-1.0f)*SmP.dot(SmP) > P_SmP*P_SmP );

	const float lS = S.length();
	const Vec3f s(S/lS);
	float t_min = 0;
	float t_max = 1;
	Vec3f Q;
	// more iterations would be more accurate,
	// but I keep this number of iterations for exact zenith_y compatibility:
	for (int i=0;i<10;i++)
	{
		const float t = 0.5f * (t_min+t_max);
		Q = p*t + s*(1.0f-t);
		Q.normalize();
		Vec3f Qp = P-Q;
		Qp.normalize();
		Vec3f Qs = S-Q;
		Qs.normalize();
		if ( (Qp-Qs).dot(Q) > 0.0f )
			t_max = t;
		else
			t_min = t;
	}
	Vec3f x = Q-P;

	// rotate
	const float zb = alphaDeltaPhi[1]*x[0] + alphaDeltaPhi[4]*x[1] + alphaDeltaPhi[7]*x[2];
	xb = (horzZoomFactor/zb) * (alphaDeltaPhi[0]*x[0] + alphaDeltaPhi[3]*x[1] + alphaDeltaPhi[6]*x[2]);
	yb = (vertZoomFactor/zb) * (alphaDeltaPhi[2]*x[0] + alphaDeltaPhi[5]*x[1] + alphaDeltaPhi[8]*x[2]);

	return rval;
}


bool SphericMirrorCalculator::retransform(float x,float y, Vec3f &v) const
{
	x /= horzZoomFactor;
	y /= vertZoomFactor;
	v[0] = alphaDeltaPhi[0]*x + alphaDeltaPhi[1] + alphaDeltaPhi[2]*y;
	v[1] = alphaDeltaPhi[3]*x + alphaDeltaPhi[4] + alphaDeltaPhi[5]*y;
	v[2] = alphaDeltaPhi[6]*x + alphaDeltaPhi[7] + alphaDeltaPhi[8]*y;
	const float vv = v.dot(v);
	const float Pv = P.dot(v);
	const float discr = Pv*Pv-(P.dot(P)-1.0f)*vv;
	if (discr < 0)
		return false;

	const Vec3f Q = P + v*((-Pv-std::sqrt(discr))/vv);
	const Vec3f w = v - Q*(2*v.dot(Q));
	const Vec3f MQ = Q - DomeCenter;
	float f = -MQ.dot(w);
	f += std::sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
	const Vec3f S = Q + w*(f/vv);
	v = S - DomeCenter;
	v *= (1.0f/DomeRadius);
	return true;
}

bool SphericMirrorCalculator::retransform(float x,float y, Vec3f &v, Vec3f &vX,Vec3f &vY) const
{
	x /= horzZoomFactor;
	const float dx = 1.0f/horzZoomFactor;
	y /= vertZoomFactor;
	const float dy = 1.0f/vertZoomFactor;

	v[0] = alphaDeltaPhi[0]*x + alphaDeltaPhi[1] + alphaDeltaPhi[2]*y;
	v[1] = alphaDeltaPhi[3]*x + alphaDeltaPhi[4] + alphaDeltaPhi[5]*y;
	v[2] = alphaDeltaPhi[6]*x + alphaDeltaPhi[7] + alphaDeltaPhi[8]*y;

	vX[0] = alphaDeltaPhi[0]*dx;
	vX[1] = alphaDeltaPhi[3]*dx;
	vX[2] = alphaDeltaPhi[6]*dx;

	vY[0] = alphaDeltaPhi[2]*dy;
	vY[1] = alphaDeltaPhi[5]*dy;
	vY[2] = alphaDeltaPhi[8]*dy;

	const float vv = v.dot(v);
	const float vvX = 2.0f*v.dot(vX);
	const float vvY = 2.0f*v.dot(vY);

	const float Pv = P.dot(v);
	const float PvX = P.dot(vX);
	const float PvY = P.dot(vY);

	const float discr = Pv*Pv-(P.dot(P)-1.0f)*vv;
	const float discr_x = 2.0f*Pv*PvX-(P.dot(P)-1.0f)*vvX;
	const float discr_y = 2.0f*Pv*PvY-(P.dot(P)-1.0f)*vvY;

	if (discr < 0)
		return false;

	const Vec3f Q = P + v*((-Pv-std::sqrt(discr))/vv);
	const Vec3f Q_x = vX*((-Pv-std::sqrt(discr))/vv) + v*( (vv*(-PvX-0.5f*discr_x/std::sqrt(discr)) - vvX*(-Pv-std::sqrt(discr))) /(vv*vv));
	const Vec3f Q_y = vY*((-Pv-std::sqrt(discr))/vv) + v*( (vv*(-PvY-0.5f*discr_y/std::sqrt(discr)) - vvY*(-Pv-std::sqrt(discr))) /(vv*vv));

	const Vec3f w = v - Q*(2*v.dot(Q));
	const Vec3f w_x = vX - Q_x*(2*v.dot(Q)) - Q*(2*(vX.dot(Q)+v.dot(Q_x)));
	const Vec3f w_y = vY - Q_y*(2*v.dot(Q)) - Q*(2*(vY.dot(Q)+v.dot(Q_y)));

	const Vec3f MQ = Q - DomeCenter;
	// MQ_x = Q_x
	// MQ_y = Q_y

	float f = -MQ.dot(w);
	float f_x = -Q_x.dot(w)-MQ.dot(w_x);
	float f_y = -Q_y.dot(w)-MQ.dot(w_y);

	float f1   = f + std::sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
	float f1_x = f_x + 0.5f*(2*f*f_x - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vvX - 2.f*MQ.dot(Q_x)*vv )	/ std::sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);
	float f1_y = f_y + 0.5f*(2*f*f_y - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vvY - 2.f*MQ.dot(Q_y)*vv )	/ std::sqrt(f*f - (MQ.dot(MQ)-DomeRadius*DomeRadius)*vv);

	const Vec3f S = Q + w*(f1/vv);
	const Vec3f S_x = Q_x + w*((vv*f1_x-vvX*f1)/(vv*vv)) + w_x*(f1/vv);
	const Vec3f S_y = Q_y + w*((vv*f1_y-vvY*f1)/(vv*vv)) + w_y*(f1/vv);

	v  = S - DomeCenter;
	vX = S_x;
	vY = S_y;

	v  *= (1.0f/DomeRadius);
	vX *= (1.0f/DomeRadius);
	vY *= (1.0f/DomeRadius);
	return true;
}
