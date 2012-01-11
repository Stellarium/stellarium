/*
 * Stellarium
 * Copyright (C) 2008 Stellarium Developers
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

#include "StelProjectorClasses.hpp"
#include "StelTranslator.hpp"

QString StelProjectorPerspective::getNameI18() const
{
	return q_("Perspective");
}

QString StelProjectorPerspective::getDescriptionI18() const
{
	return q_("Perspective projection keeps the horizon a straight line. The mathematical name for this projection method is <i>gnomonic projection</i>.");
}

bool StelProjectorPerspective::backward(Vec3d &v) const
{
	v[2] = std::sqrt(1.0/(1.0+v[0]*v[0]+v[1]*v[1]));
	v[0] *= v[2];
	v[1] *= v[2];
	v[2] = -v[2];
	return true;
}

float StelProjectorPerspective::fovToViewScalingFactor(float fov) const
{
	return std::tan(fov);
}

float StelProjectorPerspective::viewScalingFactorToFov(float vsf) const
{
	return std::atan(vsf);
}

float StelProjectorPerspective::deltaZoom(float fov) const
{
	const float vsf = fovToViewScalingFactor(fov);
// deriv_viewScalingFactorToFov(vsf) = 1.0 / (1.0+vsf*vsf)
// return vsf*deriv_viewScalingFactorToFov(vsf))
	return vsf / (1.f+vsf*vsf);
}


QString StelProjectorEqualArea::getNameI18() const
{
	return q_("Equal Area");
}

QString StelProjectorEqualArea::getDescriptionI18() const
{
	return q_("The full name of this projection method is, <i>Lambert azimuthal equal-area projection</i>. It preserves the area but not the angle.");
}

bool StelProjectorEqualArea::backward(Vec3d &v) const
{
	const double dq = v[0]*v[0] + v[1]*v[1];
	double l = 1.0 - 0.25*dq;
	if (l < 0)
	{
		v[0] = 0.0;
		v[1] = 0.0;
		v[2] = 1.0;
	}
	else
	{
		l = std::sqrt(l);
		v[0] *= l;
		v[1] *= l;
		v[2] = 0.5*dq - 1.0;
	}
	return true;
}

float StelProjectorEqualArea::fovToViewScalingFactor(float fov) const
{
	return 2.f * std::sin(0.5f * fov);
}

float StelProjectorEqualArea::viewScalingFactorToFov(float vsf) const
{
	return 2.f * std::asin(0.5f * vsf);
}

float StelProjectorEqualArea::deltaZoom(float fov) const
{
	return fov;
}

QString StelProjectorStereographic::getNameI18() const
{
	return q_("Stereographic");
}

QString StelProjectorStereographic::getDescriptionI18() const
{
	return q_("Stereographic projection is known since the antiquity and was originally known as the planisphere projection. It preserves the angles at which curves cross each other but it does not preserve area.");
}

bool StelProjectorStereographic::backward(Vec3d &v) const
{
  const double lqq = 0.25*(v[0]*v[0] + v[1]*v[1]);
  v[2] = lqq - 1.0;
  v *= (1.0 / (lqq + 1.0));
  return true;
}

float StelProjectorStereographic::fovToViewScalingFactor(float fov) const
{
	return 2.f * std::tan(0.5f * fov);
}

float StelProjectorStereographic::viewScalingFactorToFov(float vsf) const
{
	return 2.f * std::atan(0.5f * vsf);
}

float StelProjectorStereographic::deltaZoom(float fov) const
{
	const float vsf = fovToViewScalingFactor(fov);
	return 4.f*vsf / (4.f+vsf*vsf);
}




QString StelProjectorFisheye::getNameI18() const
{
	return q_("Fish-eye");
}

QString StelProjectorFisheye::getDescriptionI18() const
{
	return q_("In fish-eye projection, or <i>azimuthal equidistant projection</i>, straight lines become curves when they appear a large angular distance from the centre of the field of view (like the distortions seen with very wide angle camera lenses).");
}

bool StelProjectorFisheye::backward(Vec3d &v) const
{
	const double a = std::sqrt(v[0]*v[0]+v[1]*v[1]);
	const double f = (a > 0.0) ? (std::sin(a) / a) : 1.0;
	v[0] *= f;
	v[1] *= f;
	v[2] = -std::cos(a);
	return (a < M_PI);
}

float StelProjectorFisheye::fovToViewScalingFactor(float fov) const
{
	return fov;
}

float StelProjectorFisheye::viewScalingFactorToFov(float vsf) const
{
	return vsf;
}

float StelProjectorFisheye::deltaZoom(float fov) const
{
	return fov;
}



QString StelProjectorHammer::getNameI18() const
{
	return q_("Hammer-Aitoff");
}

QString StelProjectorHammer::getDescriptionI18() const
{
	return q_("The Hammer projection is an equal-area map projection, described by Ernst Hammer in 1892 and directly inspired by the Aitoff projection.");
}

bool StelProjectorHammer::backward(Vec3d &v) const
{
	const double zsq = 1.-0.25*0.25*v[0]*v[0]-0.5*0.5*v[1]*v[1];
	const double z = zsq<0. ? 0. : std::sqrt(zsq);
	const bool ret = 0.25*v[0]*v[0]+v[1]*v[1]<2.0; // This is stolen from glunatic
	const double alpha = 2.*std::atan2(z*v[0],(2.*(2.*zsq-1.)));
	const double delta = std::asin(v[1]*z);
	const double cd = std::cos(delta);
	v[2] = - cd * std::cos(alpha);
	v[0] = cd * std::sin(alpha);
	v[1] = v[1]*z;
	return ret;
}

float StelProjectorHammer::fovToViewScalingFactor(float fov) const
{
	return fov;
}

float StelProjectorHammer::viewScalingFactorToFov(float vsf) const
{
	return vsf;
}

float StelProjectorHammer::deltaZoom(float fov) const
{
	return fov;
}



QString StelProjectorCylinder::getNameI18() const
{
	return q_("Cylinder");
}

QString StelProjectorCylinder::getDescriptionI18() const
{
	return q_("The full name of this projection mode is <i>cylindrical equidistant projection</i>. With this projection all parallels are equally spaced.");
}

bool StelProjectorCylinder::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const bool rval = (-r < v[1] && v[1] < r);
	const float alpha = std::atan2(v[0],-v[2]);
	const float delta = std::asin(v[1]/r);
	v[0] = alpha;
	v[1] = delta;
	v[2] = r;
	return rval;
}

bool StelProjectorCylinder::backward(Vec3d &v) const
{
	const bool rval = v[1]<M_PI_2 && v[1]>-M_PI_2 && v[0]>-M_PI && v[0]<M_PI;
	const double cd = std::cos(v[1]);
	v[2] = - cd * std::cos(v[0]);
	v[0] = cd * std::sin(v[0]);
	v[1] = std::sin(v[1]);
	return rval;
}

float StelProjectorCylinder::fovToViewScalingFactor(float fov) const
{
	return fov;
}

float StelProjectorCylinder::viewScalingFactorToFov(float vsf) const
{
	return vsf;
}

float StelProjectorCylinder::deltaZoom(float fov) const
{
	return fov;
}

QString StelProjectorMercator::getNameI18() const
{
	return q_("Mercator");
}

QString StelProjectorMercator::getDescriptionI18() const
{
	return q_("The mercator projection is one of the most used world map projection. It preserves direction and shapes but distorts size, in an increasing degree away from the equator.");
}

bool StelProjectorMercator::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const bool rval = (-r < v[1] && v[1] < r);
	const float sin_delta = v[1]/r;
	v[0] = std::atan2(v[0],-v[2]);
	v[1] = 0.5f*std::log((1.f+sin_delta)/(1.f-sin_delta));
	v[2] = r;
	return rval;
}


bool StelProjectorMercator::backward(Vec3d &v) const
{
	const bool rval = v[1]<M_PI_2 && v[1]>-M_PI_2 && v[0]>-M_PI && v[0]<M_PI;
	const double E = std::exp(v[1]);
	const double h = E*E;
	const double h1 = 1.0/(1.0+h);
	const double sin_delta = (h-1.0)*h1;
	const double cos_delta = 2.0*E*h1;
	v[2] = - cos_delta * std::cos(v[0]);
	v[0] = cos_delta * std::sin(v[0]);
	v[1] = sin_delta;
	return rval;
}

float StelProjectorMercator::fovToViewScalingFactor(float fov) const
{
	return fov;
}

float StelProjectorMercator::viewScalingFactorToFov(float vsf) const
{
	return vsf;
}

float StelProjectorMercator::deltaZoom(float fov) const
{
	return fov;
}


QString StelProjectorOrthographic::getNameI18() const
{
	return q_("Orthographic");
}

QString StelProjectorOrthographic::getDescriptionI18() const
{
	return q_("Orthographic projection is related to perspective projection, but the point of perspective is set to an infinite distance.");
}

bool StelProjectorOrthographic::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const float h = 1.f/r;
	v[0] *= h;
	v[1] *= h;
	const bool rval = (v[2] <= 0.f);
	v[2] = r;
	return rval;
}

bool StelProjectorOrthographic::backward(Vec3d &v) const
{
	const double dq = v[0]*v[0] + v[1]*v[1];
	double h = 1.0 - dq;
	if (h < 0) {
		h = 1.0/std::sqrt(dq);
		v[0] *= h;
		v[1] *= h;
		v[2] = 0.0;
		return false;
	}
	v[2] = -std::sqrt(h);
	return true;
}

float StelProjectorOrthographic::fovToViewScalingFactor(float fov) const
{
	return std::sin(fov);
}

float StelProjectorOrthographic::viewScalingFactorToFov(float vsf) const
{
	return std::asin(vsf);
}

float StelProjectorOrthographic::deltaZoom(float fov) const
{
	return fov;
}



QString StelProjector2d::getNameI18() const
{
	return "2d";
}

QString StelProjector2d::getDescriptionI18() const
{
	return "Simple 2d projection for internal use.";
}

bool StelProjector2d::forward(Vec3f &) const
{
	Q_ASSERT(0);
	return false;
}

bool StelProjector2d::backward(Vec3d &) const
{
	Q_ASSERT(0);
	return false;
}

float StelProjector2d::fovToViewScalingFactor(float) const
{
	return 1.f;
}

float StelProjector2d::viewScalingFactorToFov(float) const
{
	return 1.f;
}

float StelProjector2d::deltaZoom(float fov) const
{
	Q_ASSERT(0);
	return fov;
}

