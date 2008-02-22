#ifndef _MAPPING_CLASSES_HPP_
#define _MAPPING_CLASSES_HPP_

#include "Mapping.hpp"

class MappingPerspective : public Mapping
{
public:
    MappingPerspective(void);
	QString getName(void) const {return "perspective";} 
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingPerspective instance;
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class MappingEqualArea : public Mapping
{
public:
    MappingEqualArea(void);
	QString getName(void) const {return "equal_area";} 
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingEqualArea instance;
	bool forward(Vec3d &v) const
	{
		const double r = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		const double f = std::sqrt(2.0/(r*(r-v[2])));
		v[0] *= f;
		v[1] *= f;
		v[2] = r;
		return true;
	}
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class MappingStereographic : public Mapping
{
public:
    MappingStereographic(void);
	QString getName(void) const {return "stereographic";} 
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingStereographic instance;
	bool forward(Vec3d &v) const
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
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class MappingFisheye : public Mapping
{
public:
    MappingFisheye(void);
	QString getName(void) const {return "fisheye";} 
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingFisheye instance;
	bool forward(Vec3d &v) const
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
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class MappingCylinder : public Mapping
{
public:
    MappingCylinder(void);
	QString getName(void) const {return "cylinder";} 
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingCylinder instance;
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

class MappingOrthographic : public Mapping
{
public:
    MappingOrthographic(void);
	QString getName(void) const {return "orthographic";} 
	static Mapping *getMapping(void) {return &instance;}
private:
	static MappingOrthographic instance;
	bool forward(Vec3d &win) const;
	bool backward(Vec3d &v) const;
	double fovToViewScalingFactor(double fov) const;
	double viewScalingFactorToFov(double vsf) const;
	double deltaZoom(double fov) const;
};

#endif
