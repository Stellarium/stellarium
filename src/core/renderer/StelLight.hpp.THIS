/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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

#ifndef _STELLIGHT_HPP_
#define _STELLIGHT_HPP_

#include "VecMath.hpp"

//! Basic light source.
//!
//! Used for lighting calculation when generating a lit sphere, and to 
//! pass light parameters to shaders.
//!
//! (There is no builtin light functionality in StelRenderer - lights are 
//! done in shaders, or by manually setting vertex colors)
struct StelLight
{
	//! Position of the light.
	Vec3f position;
	//! Difuse light color.
	Vec4f diffuse;
	//! Ambient light color.
	Vec4f ambient;

	//! Is this light identical to another light?
	bool operator == (const StelLight& rhs) const
	{
		return position == rhs.position && diffuse == rhs.diffuse && ambient == rhs.ambient;
	}
};

#endif // _STELLIGHT_HPP_
