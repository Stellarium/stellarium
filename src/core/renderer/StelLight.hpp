#ifndef _STELLIGHT_HPP_
#define _STELLIGHT_HPP_

#include "VecMath.hpp"

//! Basic light source.
//!
//! Used for lighting calculation when generating a lit sphere.
struct StelLight
{
	//! Position of the light.
	Vec3f position;
	//! Difuse light color.
	Vec4f diffuse;
	//! Ambient light color.
	Vec4f ambient;
};

#endif // _STELLIGHT_HPP_
