#ifndef MAPPING_HPP_
#define MAPPING_HPP_

#include "callbacks.hpp"
#include "vecmath.h"

class Mapping
{
public:
	Mapping();

	//! Apply the transformation in the forward direction
	boost::callback<bool, Vec3d&> mapForward;
	//! Apply the transformation in the backward direction
	boost::callback<bool, Vec3d&> mapBackward;
	
	//! Minimum FOV apperture in degree
	double minFov;
	//! Maximum FOV apperture in degree
	double maxFov;
};

class MappingPerspective
{
public:
	std::string getName() const {return "perspective";} 
	Mapping getMapping() const;

private:
	static bool forward(Vec3d& win);
	static bool backward(Vec3d& v);
};

class MappingEqualArea
{
public:
	std::string getName() const {return "equal_area";} 
	Mapping getMapping() const;

private:
	static bool forward(Vec3d& win);
	static bool backward(Vec3d& v);
};

class MappingStereographic
{
public:
	std::string getName() const {return "stereographic";} 
	Mapping getMapping() const;

private:
	static bool forward(Vec3d& win);
	static bool backward(Vec3d& v);
};

class MappingFisheye
{
public:
	std::string getName() const {return "fisheye";} 
	Mapping getMapping() const;

private:
	static bool forward(Vec3d& win);
	static bool backward(Vec3d& v);
};

class MappingCylinder
{
public:
	std::string getName() const {return "cylinder";} 
	Mapping getMapping() const;

private:
	static bool forward(Vec3d& win);
	static bool backward(Vec3d& v);
};

class MappingOrthographic
{
public:
	std::string getName() const {return "orthographic";} 
	Mapping getMapping() const;

private:
	static bool forward(Vec3d& win);
	static bool backward(Vec3d& v);
};

#endif /*MAPPING_HPP_*/
