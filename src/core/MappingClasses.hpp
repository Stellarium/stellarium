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
	bool forward(Vec3d &win) const;
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
	bool forward(Vec3d &win) const;
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
	bool forward(Vec3d &win) const;
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
