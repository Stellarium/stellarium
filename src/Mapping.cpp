#include "Mapping.hpp"

Mapping::Mapping() : minFov(0.0001), maxFov(100.)
{;}

Mapping MappingEqualArea::getMapping() const
{
	Mapping m;
	m.mapForward = boost::callback<bool, Vec3d&>(forward);
	m.mapBackward = boost::callback<bool, Vec3d&>(backward);
	m.minFov = 0.0001;
	m.maxFov = 225.0;
	return m;
}

bool MappingEqualArea::forward(Vec3d &v)
{
	const double r = v.length();
	v *= (1.0/r);
	const double z1 = v[2] + 1.0;
	const double f = std::sqrt(1.0 + (z1*z1) / (v[0]*v[0]+v[1]*v[1]));
	v[0] *= f;
	v[1] *= f;
	v[2] = r;
	return true;
}

bool MappingEqualArea::backward(Vec3d& v)
{
	const double dq = v[0]*v[0] + v[1]*v[1];
	double l = 1.0 - 0.25*dq;
	if (l < 0)
	{
		v[0] = 0.0;
		v[1] = 0.0;
		v[2] = -1.0;
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

Mapping MappingStereographic::getMapping() const
{
	Mapping m;
	m.mapForward = boost::callback<bool, Vec3d&>(forward);
	m.mapBackward = boost::callback<bool, Vec3d&>(backward);
	m.minFov = 0.001;
	m.maxFov = 480.0;
	return m;
}

bool MappingStereographic::forward(Vec3d &v)
{
	const double r = std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
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

bool MappingStereographic::backward(Vec3d &v)
{
  const double lqq = 0.25*(v[0]*v[0] + v[1]*v[1]);
  v[2] = lqq - 1.0;
  v *= (1.0 / (lqq + 1.0));
  return true;
}

Mapping MappingFisheye::getMapping() const
{
	Mapping m;
	m.mapForward = boost::callback<bool, Vec3d&>(forward);
	m.mapBackward = boost::callback<bool, Vec3d&>(backward);
	m.minFov = 0.0001;
	m.maxFov = 180.00001;
	return m;
}

bool MappingFisheye::forward(Vec3d &v)
{
//	const double l = v.length();
	const double oneoverh = 1./std::sqrt(v[0]*v[0]+v[1]*v[1]);
	double a = M_PI_2 + std::atan(v[2]*oneoverh);
	const double f = a * oneoverh;
	v[0] *= f;
	v[1] *= f;
	v[2] = std::fabs(v[2]);
	return true;
}

bool MappingFisheye::backward(Vec3d &v)
{
  const double a = std::sqrt(v[0]*v[0]+v[1]*v[1]);
  const double f = std::sin(a) / a;
  v[0] *= f;
  v[1] *= f;
  v[2] = -std::cos(a);
  return true;
}

Mapping MappingCylinder::getMapping() const
{
	Mapping m;
	m.mapForward = boost::callback<bool, Vec3d&>(forward);
	m.mapBackward = boost::callback<bool, Vec3d&>(backward);
	m.minFov = 0.001;
	m.maxFov = 270.0;
	return m;
}

bool MappingCylinder::forward(Vec3d &v)
{
  const double win_length = v.length();
  const double alpha = std::atan2(v[0],-v[2]);
  const double delta = std::asin(v[1]/win_length);
  v[0] = alpha;
  v[1] = delta;
  v[2] = win_length;
  return true;
}

bool MappingCylinder::backward(Vec3d &v)
{
  const double cd = cos(v[1]);
  v[2] = - cd * std::cos(v[0]);
  v[0] = cd * std::sin(v[0]);
  v[1] = std::sin(v[1]);
  return true;
}

Mapping MappingPerspective::getMapping() const
{
	Mapping m;
	m.mapForward = boost::callback<bool, Vec3d&>(forward);
	m.mapBackward = boost::callback<bool, Vec3d&>(backward);
	m.minFov = 0.0001;
	m.maxFov = 150.0;
	return m;
}

bool MappingPerspective::forward(Vec3d &v)
{
  v[2] = -v[2];
  if (v[2] > 0) {
    v[0] /= v[2];
    v[1] /= v[2];
    return true;
  }
  if (v[2] < 0) {
    v[0] /= (-v[2]);
    v[1] /= (-v[2]);
    return false;
  }
  v[0] *= 1e99;
  v[1] *= 1e99;
  return false;
}

bool MappingPerspective::backward(Vec3d &v)
{
  v[2] = std::sqrt(1.0/(1.0+v[0]*v[0]+v[1]*v[1]));
  v[0] *= v[2];
  v[1] *= v[2];
  v[2] = -v[2];
  return true;
}

Mapping MappingOrthographic::getMapping() const
{
  Mapping m;
  m.mapForward = boost::callback<bool, Vec3d&>(forward);
  m.mapBackward = boost::callback<bool, Vec3d&>(backward);
  m.minFov = 0.0001;
  m.maxFov = 120.0;
  return m;
}

bool MappingOrthographic::forward(Vec3d &v)
{
  const double h = 1.0/v.length();
  v[0] *= h;
  v[1] *= h;
  v[2] = -v[2];
  return (v[2] >= 0.0);
}

bool MappingOrthographic::backward(Vec3d &v)
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
