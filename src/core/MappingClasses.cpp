#include "MappingClasses.hpp"
#include "Translator.hpp"

QString ProjectorPerspective::getNameI18() const
{
	return q_("Perspective");
}

QString ProjectorPerspective::getDescriptionI18() const
{
	return q_("Perspective projection keeps the horizon a straight line. The mathematical name for this projection method is <i>gnomonic projection</i>.");
}

bool ProjectorPerspective::backward(Vec3d &v) const
{
	v[2] = std::sqrt(1.0/(1.0+v[0]*v[0]+v[1]*v[1]));
	v[0] *= v[2];
	v[1] *= v[2];
	v[2] = -v[2];
	return true;
}

double ProjectorPerspective::fovToViewScalingFactor(double fov) const
{
	return std::tan(fov);
}

double ProjectorPerspective::viewScalingFactorToFov(double vsf) const
{
	return std::atan(vsf);
}

double ProjectorPerspective::deltaZoom(double fov) const
{
    const double vsf = fovToViewScalingFactor(fov);
// deriv_viewScalingFactorToFov(vsf) = 1.0 / (1.0+vsf*vsf)
// return vsf*deriv_viewScalingFactorToFov(vsf))
	return vsf / (1.0+vsf*vsf);
}


QString ProjectorEqualArea::getNameI18() const
{
	return q_("Equal Area");
}

QString ProjectorEqualArea::getDescriptionI18() const
{
	return q_("The full name of this projection method is, <i>Lambert azimuthal equal-area projection</i>. It preserves the area but not the angle.");
}

bool ProjectorEqualArea::backward(Vec3d &v) const
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

double ProjectorEqualArea::fovToViewScalingFactor(double fov) const
{
	return 2.0 * std::sin(0.5 * fov);
}

double ProjectorEqualArea::viewScalingFactorToFov(double vsf) const
{
	return 2.0 * std::asin(0.5 * vsf);
}

double ProjectorEqualArea::deltaZoom(double fov) const
{
// deriv_viewScalingFactorToFov(vsf) = 2.0 / sqrt(4.0-vsf*vsf);
// const double vsf = fovToViewScalingFactor(fov);
// return vsf*deriv_viewScalingFactorToFov(vsf)) = 2.0*vsf / sqrt(4.0-vsf*vsf);
    return fov;
}

QString ProjectorStereographic::getNameI18() const
{
	return q_("Stereographic");
}

QString ProjectorStereographic::getDescriptionI18() const
{
	return q_("Stereographic projection is known since the antiquity and was originally known as the planisphere projection. It preserves the angles at which curves cross each other but it does not preserve area.");
}

bool ProjectorStereographic::backward(Vec3d &v) const
{
  const double lqq = 0.25*(v[0]*v[0] + v[1]*v[1]);
  v[2] = lqq - 1.0;
  v *= (1.0 / (lqq + 1.0));
  return true;
}

double ProjectorStereographic::fovToViewScalingFactor(double fov) const
{
	return 2.0 * std::tan(0.5 * fov);
}

double ProjectorStereographic::viewScalingFactorToFov(double vsf) const
{
	return 2.0 * std::atan(0.5 * vsf);
}

double ProjectorStereographic::deltaZoom(double fov) const
{
    const double vsf = fovToViewScalingFactor(fov);
// deriv_viewScalingFactorToFov(vsf) = 4.0 / (4.0+vsf*vsf)
// return vsf*deriv_viewScalingFactorToFov(vsf))
	return 4.0*vsf / (4.0+vsf*vsf);
}




QString ProjectorFisheye::getNameI18() const
{
	return q_("Fish-eye");
}

QString ProjectorFisheye::getDescriptionI18() const
{
	return q_("In fish-eye projection, or <i>azimuthal equidistant projection</i>, straight lines become curves when they appear a large angular distance from the centre of the field of view (like the distortions seen with very wide angle camera lenses).");
}

bool ProjectorFisheye::backward(Vec3d &v) const
{
	const double a = std::sqrt(v[0]*v[0]+v[1]*v[1]);
	const double f = (a > 0.0) ? (std::sin(a) / a) : 1.0;
	v[0] *= f;
	v[1] *= f;
	v[2] = -std::cos(a);
	return (a < M_PI);
}

double ProjectorFisheye::fovToViewScalingFactor(double fov) const
{
	return fov;
}

double ProjectorFisheye::viewScalingFactorToFov(double vsf) const
{
	return vsf;
}

double ProjectorFisheye::deltaZoom(double fov) const
{
	return fov;
}





QString ProjectorCylinder::getNameI18() const
{
	return q_("Cylinder");
}

QString ProjectorCylinder::getDescriptionI18() const
{
	return q_("The full name of this projection mode is <i>cylindrical equidistant projection</i>. With this projection all parallels are equally spaced.");
}

bool ProjectorCylinder::forward(Vec3d &v) const
{
	const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const bool rval = (-r < v[1] && v[1] < r);
	const double alpha = std::atan2(v[0],-v[2]);
	const double delta = std::asin(v[1]/r);
	v[0] = alpha;
	v[1] = delta;
	v[2] = r;
	return rval;
}

bool ProjectorCylinder::backward(Vec3d &v) const
{
	const double cd = std::cos(v[1]);
	v[2] = - cd * std::cos(v[0]);
	v[0] = cd * std::sin(v[0]);
	v[1] = std::sin(v[1]);
	return true;
}

double ProjectorCylinder::fovToViewScalingFactor(double fov) const
{
	return fov;
}

double ProjectorCylinder::viewScalingFactorToFov(double vsf) const
{
	return vsf;
}

double ProjectorCylinder::deltaZoom(double fov) const
{
	return fov;
}



QString ProjectorMercator::getNameI18() const
{
	return q_("Mercator");
}

QString ProjectorMercator::getDescriptionI18() const
{
	return q_("The mercator projection is one of the most used world map projection. It preserves direction and shapes but distorts size, in an increasing degree away from the equator.");
}

bool ProjectorMercator::forward(Vec3d &v) const
{
	const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const bool rval = (-r < v[1] && v[1] < r);
	const double sin_delta = v[1]/r;
	v[0] = std::atan2(v[0],-v[2]);
	v[1] = 0.5*std::log((1.0+sin_delta)/(1.0-sin_delta));
	v[2] = r;
	return rval;
}


bool ProjectorMercator::backward(Vec3d &v) const
{
	const double E = std::exp(v[1]);
	const double h = E*E;
	const double h1 = 1.0/(1.0+h);
	const double sin_delta = (h-1.0)*h1;
	const double cos_delta = 2.0*E*h1;
	v[2] = - cos_delta * std::cos(v[0]);
	v[0] = cos_delta * std::sin(v[0]);
	v[1] = sin_delta;
	return true;
}

double ProjectorMercator::fovToViewScalingFactor(double fov) const
{
	return fov;
}

double ProjectorMercator::viewScalingFactorToFov(double vsf) const
{
	return vsf;
}

double ProjectorMercator::deltaZoom(double fov) const
{
	return fov;
}



QString ProjectorOrthographic::getNameI18() const
{
	return q_("Orthographic");
}

QString ProjectorOrthographic::getDescriptionI18() const
{
	return q_("Orthographic projection is related to perspective projection, but the point of perspective is set to an infinite distance.");
}

bool ProjectorOrthographic::forward(Vec3d &v) const
{
	const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const double h = 1.0/r;
	v[0] *= h;
	v[1] *= h;
	const bool rval = (v[2] <= 0.0);
	v[2] = r;
	return rval;
}

bool ProjectorOrthographic::backward(Vec3d &v) const
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

double ProjectorOrthographic::fovToViewScalingFactor(double fov) const
{
	return std::sin(fov);
}

double ProjectorOrthographic::viewScalingFactorToFov(double vsf) const
{
	return std::asin(vsf);
}

double ProjectorOrthographic::deltaZoom(double fov) const
{
	return fov;
}



QString Projector2d::getNameI18() const
{
	return "2d";
}

QString Projector2d::getDescriptionI18() const
{
	return "Simple 2d projection for internal use.";
}

bool Projector2d::forward(Vec3d &v) const
{
	Q_ASSERT(0);
	return false;
}

bool Projector2d::backward(Vec3d &v) const
{
	Q_ASSERT(0);
	return false;
}

double Projector2d::fovToViewScalingFactor(double fov) const
{
	return 1.;
}

double Projector2d::viewScalingFactorToFov(double vsf) const
{
	return 1.;
}

double Projector2d::deltaZoom(double fov) const
{
	Q_ASSERT(0);
	return fov;
}
