#include "MappingClasses.hpp"
#include "Translator.hpp"

MappingPerspective::MappingPerspective(void)
{
	maxFov = 150.0;
}

QString MappingPerspective::getNameI18() const
{
	return q_("Perspective");
}

QString MappingPerspective::getDescriptionI18() const
{
	return q_("Perspective projection keeps the horizon a straight line. The mathematical name for this projection method is <i>gnomonic projection</i>.");
}

MappingPerspective MappingPerspective::instance;

bool MappingPerspective::forward(Vec3d &v) const
{
	const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (v[2] < 0) {
		v[0] /= (-v[2]);
		v[1] /= (-v[2]);
		v[2] = r;
		return true;
	}
	if (v[2] > 0) {
		v[0] /= v[2];
		v[1] /= v[2];
		v[2] = r;
		return false;
	}
	v[0] *= 1e99;
	v[1] *= 1e99;
	v[2] = r;
	return false;
}

bool MappingPerspective::backward(Vec3d &v) const
{
	v[2] = std::sqrt(1.0/(1.0+v[0]*v[0]+v[1]*v[1]));
	v[0] *= v[2];
	v[1] *= v[2];
	v[2] = -v[2];
	return true;
}

double MappingPerspective::fovToViewScalingFactor(double fov) const
{
	return std::tan(fov);
}

double MappingPerspective::viewScalingFactorToFov(double vsf) const
{
	return std::atan(vsf);
}

double MappingPerspective::deltaZoom(double fov) const
{
    const double vsf = fovToViewScalingFactor(fov);
// deriv_viewScalingFactorToFov(vsf) = 1.0 / (1.0+vsf*vsf)
// return vsf*deriv_viewScalingFactorToFov(vsf))
	return vsf / (1.0+vsf*vsf);
}


MappingEqualArea::MappingEqualArea(void)
{
	maxFov = 360.0;
}

QString MappingEqualArea::getNameI18() const
{
	return q_("Equal Area");
}

QString MappingEqualArea::getDescriptionI18() const
{
	return q_("The full name of this projection method is, <i>Lambert azimuthal equal-area projection</i>.");
}

MappingEqualArea MappingEqualArea::instance;

bool MappingEqualArea::backward(Vec3d &v) const
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

double MappingEqualArea::fovToViewScalingFactor(double fov) const
{
	return 2.0 * std::sin(0.5 * fov);
}

double MappingEqualArea::viewScalingFactorToFov(double vsf) const
{
	return 2.0 * std::asin(0.5 * vsf);
}

double MappingEqualArea::deltaZoom(double fov) const
{
// deriv_viewScalingFactorToFov(vsf) = 2.0 / sqrt(4.0-vsf*vsf);
// const double vsf = fovToViewScalingFactor(fov);
// return vsf*deriv_viewScalingFactorToFov(vsf)) = 2.0*vsf / sqrt(4.0-vsf*vsf);
    return fov;
}


MappingStereographic::MappingStereographic(void)
{
	maxFov = 235.0;
}

QString MappingStereographic::getNameI18() const
{
	return q_("Stereographic");
}

QString MappingStereographic::getDescriptionI18() const
{
	return q_("This mode is similar to fish-eye projection mode.");
}

MappingStereographic MappingStereographic::instance;


bool MappingStereographic::backward(Vec3d &v) const
{
  const double lqq = 0.25*(v[0]*v[0] + v[1]*v[1]);
  v[2] = lqq - 1.0;
  v *= (1.0 / (lqq + 1.0));
  return true;
}

double MappingStereographic::fovToViewScalingFactor(double fov) const
{
	return 2.0 * std::tan(0.5 * fov);
}

double MappingStereographic::viewScalingFactorToFov(double vsf) const
{
	return 2.0 * std::atan(0.5 * vsf);
}

double MappingStereographic::deltaZoom(double fov) const
{
    const double vsf = fovToViewScalingFactor(fov);
// deriv_viewScalingFactorToFov(vsf) = 4.0 / (4.0+vsf*vsf)
// return vsf*deriv_viewScalingFactorToFov(vsf))
	return 4.0*vsf / (4.0+vsf*vsf);
}


MappingFisheye::MappingFisheye(void)
{
	maxFov = 180.00001;
}

QString MappingFisheye::getNameI18() const
{
	return q_("Fish-eye");
}

QString MappingFisheye::getDescriptionI18() const
{
	return q_("In fish-eye projection, or <i>azimuthal equidistant projection</i>, straight lines become curves when they appear a large angular distance from the centre of the field of view (like the distortions seen with very wide angle camera lenses).");
}

MappingFisheye MappingFisheye::instance;

bool MappingFisheye::backward(Vec3d &v) const
{
	const double a = std::sqrt(v[0]*v[0]+v[1]*v[1]);
	const double f = (a > 0.0) ? (std::sin(a) / a) : 1.0;
	v[0] *= f;
	v[1] *= f;
	v[2] = -std::cos(a);
	return (a < M_PI);
}

double MappingFisheye::fovToViewScalingFactor(double fov) const
{
	return fov;
}

double MappingFisheye::viewScalingFactorToFov(double vsf) const
{
	return vsf;
}

double MappingFisheye::deltaZoom(double fov) const
{
	return fov;
}



MappingCylinder::MappingCylinder(void)
{
	// assume aspect ration of 4/3 for getting a full 360 degree horizon:
	maxFov = 175.0 * 4/3;
}

QString MappingCylinder::getNameI18() const
{
	return q_("Cylinder");
}

QString MappingCylinder::getDescriptionI18() const
{
	return q_("The full name of this projection mode is <i>cylindrical equidistant projection</i>.");
}

MappingCylinder MappingCylinder::instance;

bool MappingCylinder::forward(Vec3d &v) const
{
	const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const double alpha = std::atan2(v[0],-v[2]);
	const double delta = std::asin(v[1]/r);
	v[0] = alpha;
	v[1] = delta;
	v[2] = r;
	return true;
}

bool MappingCylinder::backward(Vec3d &v) const
{
	const double cd = std::cos(v[1]);
	v[2] = - cd * std::cos(v[0]);
	v[0] = cd * std::sin(v[0]);
	v[1] = std::sin(v[1]);
	return true;
}

double MappingCylinder::fovToViewScalingFactor(double fov) const
{
	return fov;
}

double MappingCylinder::viewScalingFactorToFov(double vsf) const
{
	return vsf;
}

double MappingCylinder::deltaZoom(double fov) const
{
	return fov;
}


MappingOrthographic::MappingOrthographic(void)
{
  maxFov = 180.0;
}

QString MappingOrthographic::getNameI18() const
{
	return q_("Orthographic");
}

QString MappingOrthographic::getDescriptionI18() const
{
	return q_("Orthographic projection is related to perspective projection, but the point of perspective is set to an infinite distance.");
}

MappingOrthographic MappingOrthographic::instance;

bool MappingOrthographic::forward(Vec3d &v) const
{
	const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const double h = 1.0/r;
	v[0] *= h;
	v[1] *= h;
	const bool rval = (v[2] <= 0.0);
	v[2] = r;
	return rval;
}

bool MappingOrthographic::backward(Vec3d &v) const
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

double MappingOrthographic::fovToViewScalingFactor(double fov) const
{
	return std::sin(fov);
}

double MappingOrthographic::viewScalingFactorToFov(double vsf) const
{
	return std::asin(vsf);
}

double MappingOrthographic::deltaZoom(double fov) const
{
	return fov;
}
