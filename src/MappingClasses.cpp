#include "MappingClasses.hpp"

MappingPerspective::MappingPerspective(void)
{
	maxFov = 150.0;
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
	fov *= (0.5*M_PI/180.0);
	return 0.5 / std::tan(fov);
}


MappingEqualArea::MappingEqualArea(void)
{
	maxFov = 360.0;
}

MappingEqualArea MappingEqualArea::instance;

bool MappingEqualArea::forward(Vec3d &v) const
{
	const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const double f = std::sqrt(2.0/(r*(r-v[2])));
	v[0] *= f;
	v[1] *= f;
	v[2] = r;
	return true;
}

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
	fov *= (0.5*M_PI/180.0);
	return 0.5 * 0.5/std::sin(0.5*fov);
}


MappingStereographic::MappingStereographic(void)
{
	maxFov = 330.0;
}

MappingStereographic MappingStereographic::instance;

bool MappingStereographic::forward(Vec3d &v) const
{
	const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	const double h = 0.5*(r-v[2]);
	if (h <= 0.0) {
		v[0] = 1e99;
		v[1] = 1e99;
		v[2] = -1e99;
		return false;
	}
	const double f = 1.0 / h;
	v[0] *= f;
	v[1] *= f;
	v[2] = r;
	return true;
}

bool MappingStereographic::backward(Vec3d &v) const
{
  const double lqq = 0.25*(v[0]*v[0] + v[1]*v[1]);
  v[2] = lqq - 1.0;
  v *= (1.0 / (lqq + 1.0));
  return true;
}

double MappingStereographic::fovToViewScalingFactor(double fov) const
{
	fov *= (0.5*M_PI/180.0);
	return 0.5 * 0.5/std::tan(0.5*fov);
}



MappingFisheye::MappingFisheye(void)
{
	maxFov = 180.00001;
}

MappingFisheye MappingFisheye::instance;

bool MappingFisheye::forward(Vec3d &v) const
{
	const double rq1 = v[0]*v[0] + v[1]*v[1];
	if (rq1 > 0.0) {
		const double h = std::sqrt(rq1);
		const double f = std::atan2(h,-v[2]) / h;
		v[0] *= f;
		v[1] *= f;
		v[2] = std::sqrt(rq1 + v[2]*v[2]);
		return true;
	}
	if (v[2] < 0.0) {
		v[0] = 0.0;
		v[1] = 0.0;
		v[2] = 1.0;
		return true;
	}
	v[0] = 1e99;
	v[1] = 1e99;
	v[2] = -1e99;
	return false;
}

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
	fov *= (0.5*M_PI/180.0);
	return 0.5 / fov;
}



MappingCylinder::MappingCylinder(void)
{
	// assume aspect ration of 4/3 for getting a full 360 degree horizon:
	maxFov = 180.0 * 4/3;
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
	const double cd = cos(v[1]);
	v[2] = - cd * std::cos(v[0]);
	v[0] = cd * std::sin(v[0]);
	v[1] = std::sin(v[1]);
	return true;
}

double MappingCylinder::fovToViewScalingFactor(double fov) const
{
	fov *= (0.5*M_PI/180.0);
	return 0.5 / fov;
}


MappingOrthographic::MappingOrthographic(void)
{
  maxFov = 180.0;
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
		h = 1.0/sqrt(dq);
		v[0] *= h;
		v[1] *= h;
		v[2] = 0.0;
		return false;
	}
	v[2] = -sqrt(h);
	return true;
}

double MappingOrthographic::fovToViewScalingFactor(double fov) const
{
	fov *= (0.5*M_PI/180.0);
	return 0.5 / std::sin(fov);
}

