#ifndef MAPPING_HPP_
#define MAPPING_HPP_

#include "vecmath.h"

class Mapping
{
public:
	Mapping(void);
	virtual ~Mapping(void) {}
	virtual std::string getName(void) const = 0;

	//! Apply the transformation in the forward direction
	//! After transformation v[2] will always contain the length
	//! of the original v: sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])
	//! regardless of the projection type. This makes it possible to
	//! implement depth buffer testing in a way independent of the
	//! projection type. I would like to return the squared length
	//! instead of the length because of performance reasons.
	//! But then far away objects are not textured any more,
	//! perhaps because of a depth buffer overflow although
	//! the depth test is disabled?
	virtual bool forward(Vec3d &v) const = 0;
	virtual bool backward(Vec3d &v) const = 0;
	virtual double fovToViewScalingFactor(double fov) const = 0;
	virtual double viewScalingFactorToFov(double vsf) const = 0;
	virtual double deltaZoom(double fov) const = 0;
	//! Minimum FOV apperture in degree
	double minFov;
	//! Maximum FOV apperture in degree
	double maxFov;
};

#endif
