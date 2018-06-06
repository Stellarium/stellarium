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

#include <limits>

QString StelProjectorPerspective::getNameI18() const
{
	return q_("Perspective");
}

QString StelProjectorPerspective::getDescriptionI18() const
{
	return q_("Perspective projection maps the horizon and other great circles like equator, ecliptic, hour lines, etc. into straight lines. The mathematical name for this projection method is <i>gnomonic projection</i>.");
}

bool StelProjectorPerspective::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (v[2] < 0) {
		v[0] *= (-widthStretch/v[2]);
		v[1] /= (-v[2]);
		v[2] = r;
		return true;
	}
	if (v[2] > 0) {
		v[0] *= widthStretch/v[2];
		v[1] /= v[2];
		v[2] = -std::numeric_limits<float>::max();
		return false;
	}
	v[0] = std::numeric_limits<float>::max();
	v[1] = std::numeric_limits<float>::max();
	v[2] = -std::numeric_limits<float>::max();
	return false;
}

bool StelProjectorPerspective::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
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
	return q_("The full name of this projection method is <i>Lambert azimuthal equal-area projection</i>. It preserves the area but not the angle.");
}

bool StelProjectorEqualArea::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const float f = std::sqrt(2.f/(r*(r-v[2])));
	v[0] *= f*widthStretch;
	v[1] *= f;
	v[2] = r;
	return true;
}

bool StelProjectorEqualArea::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
	// FIXME: for high FoV, return false but don't cause crash with Mouse Pointer Coordinates.
	const double dq = v[0]*v[0] + v[1]*v[1];
	double l = 1.0 - 0.25*dq;
	if (l < 0)
	{
		v[0] = 0.0;
		v[1] = 0.0;
		v[2] = 1.0;
		//return false; // GZ tentative fix for projecting invalid outlying screen point. CAUSES CRASH SOMETIMES!?
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
	return q_("Stereographic projection is known since antiquity and was originally known as the planisphere projection. It preserves the angles at which curves cross each other but it does not preserve area.");
}

bool StelProjectorStereographic::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const float h = 0.5f*(r-v[2]);
	if (h <= 0.f) {
		v[0] = std::numeric_limits<float>::max();
		v[1] = std::numeric_limits<float>::max();
		v[2] = -std::numeric_limits<float>::min();
		return false;
	}
	const float f = 1.f / h;
	v[0] *= f*widthStretch;
	v[1] *= f;
	v[2] = r;
	return true;
}

bool StelProjectorStereographic::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
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

bool StelProjectorFisheye::forward(Vec3f &v) const
{
	const float rq1 = v[0]*v[0] + v[1]*v[1];
	if (rq1 > 0.f) {
		const float h = std::sqrt(rq1);
		const float f = std::atan2(h,-v[2]) / h;
		v[0] *= f*widthStretch;
		v[1] *= f;
		v[2] = std::sqrt(rq1 + v[2]*v[2]);
		return true;
	}
	if (v[2] < 0.f) {
		v[0] = 0.f;
		v[1] = 0.f;
		v[2] = 1.f;
		return true;
	}
	v[0] = std::numeric_limits<float>::max();
	v[1] = std::numeric_limits<float>::max();
	v[2] = std::numeric_limits<float>::min();
	return false;
}

bool StelProjectorFisheye::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
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

bool StelProjectorHammer::forward(Vec3f &v) const
{
	// Hammer Aitoff
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const float alpha = std::atan2(v[0],-v[2]);
	const float cosDelta = std::sqrt(1.f-v[1]*v[1]/(r*r));
	float z = std::sqrt(1.+cosDelta*std::cos(alpha/2.f));
	v[0] = 2.f*M_SQRT2*cosDelta*std::sin(alpha/2.f)/z * widthStretch;
	v[1] = M_SQRT2*v[1]/r/z;
	v[2] = r;
	return true;
}

bool StelProjectorHammer::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
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
	v[0] = alpha*widthStretch;
	v[1] = delta;
	v[2] = r;
	return rval;
}

bool StelProjectorCylinder::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
	const bool rval = v[1]<M_PI_2 && v[1]>-M_PI_2 && v[0]>-M_PI && v[0]<M_PI;
	const double cd = std::cos(v[1]);
	const double alpha=v[0];
	v[2] = - cd * std::cos(alpha);
	v[0] = cd * std::sin(alpha);
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
	return q_("The Mercator projection is one of the most used world map projections. It preserves direction and shapes but distorts size, in an increasing degree away from the equator.");
}

bool StelProjectorMercator::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const bool rval = (-r < v[1] && v[1] < r);
	const float sin_delta = v[1]/r;
	v[0] = std::atan2(v[0],-v[2]) *widthStretch;
	v[1] = 0.5f*std::log((1.f+sin_delta)/(1.f-sin_delta));
	v[2] = r;
	return rval;
}


bool StelProjectorMercator::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
	const bool rval = v[0]>-M_PI && v[0]<M_PI;
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
	v[0] *= h *widthStretch;
	v[1] *= h;
	const bool rval = (v[2] <= 0.f);
	v[2] = r;
	return rval;
}

bool StelProjectorOrthographic::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
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

QString StelProjectorSinusoidal::getNameI18() const
{
	return q_("Sinusoidal");
}

QString StelProjectorSinusoidal::getDescriptionI18() const
{
	return q_("The sinusoidal projection is a <i>pseudocylindrical equal-area map projection</i>, sometimes called the Sanson-Flamsteed or the Mercator equal-area projection.");
}

bool StelProjectorSinusoidal::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const bool rval = (-r < v[1] && v[1] < r);
	const float alpha = std::atan2(v[0],-v[2]);
	const float delta = std::asin(v[1]/r);	
	v[0] = alpha*std::cos(delta) *widthStretch;
	v[1] = delta;
	v[2] = r;
	return rval;
}

bool StelProjectorSinusoidal::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
	const bool rval = v[1]<M_PI_2 && v[1]>-M_PI_2 && v[0]>-M_PI && v[0]<M_PI;
	const double cd = std::cos(v[1]);
	const double pcd = v[0]/cd;
	if (v[0]<-M_PI*cd || v[0]>M_PI*cd)
	{
		v[0] = -cd;
		v[1] = 1.0;
		// FIXME: It is unclear what happens to v[2] here.
		v.normalize(); // make sure the length test in Atmosphere.cpp work.
		return false;
	}
	v[2] = -cd * std::cos(pcd);
	v[0] = cd * std::sin(pcd);
	v[1] = std::sin(v[1]);
	return rval;
}

QString StelProjectorMiller::getNameI18() const
{
	return q_("Miller cylindrical");
}

QString StelProjectorMiller::getDescriptionI18() const
{
	return q_("The Miller cylindrical projection is a modified Mercator projection, proposed by Osborn Maitland Miller (1897-1979) in 1942. The poles are no longer mapped to infinity.");
}

bool StelProjectorMiller::forward(Vec3f &v) const
{
	const float r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const bool rval = (-r < v[1] && v[1] < r);
	const float sin_delta = v[1]/r;
	const float delta=asin(sin_delta);
	v[0] = std::atan2(v[0],-v[2]) * widthStretch;
	v[1] = 1.25f*asinh(tan(0.8f*delta));
	v[2] = r;
	return rval;
}

bool StelProjectorMiller::backward(Vec3d &v) const
{
	v[0] /= widthStretch;
	const double yMax=1.25f*asinh(tan(M_PI*2.0/5.0));
	const bool rval = v[1]<yMax && v[1]>-yMax && v[0]>-M_PI && v[0]<M_PI;
	const double lat = 1.25*atan(sinh(0.8*v[1]));
	const double lng = v[0];
	const double cos_lat=cos(lat);
	v[0] = cos_lat*sin(lng);
	v[1] = sin(lat);
	v[2]= -cos_lat*cos(lng);
	return rval;
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

