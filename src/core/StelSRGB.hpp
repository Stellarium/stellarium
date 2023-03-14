/*
 * Stellarium
 * Copyright (C) 2023 Ruslan Kabatsayev <b7.10110111@gmail.com>
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

#ifndef INCLUDE_ONCE_E0C038F5_4B24_4B2C_9AFC_B1B8F0BC4BAF
#define INCLUDE_ONCE_E0C038F5_4B24_4B2C_9AFC_B1B8F0BC4BAF

#include <cmath>
#include <QByteArray>
#include "VecMath.hpp"

template<typename F>
F srgbToLinear(const F c)
{
	return c > F(0.04045) ? std::pow((c+F(0.055))/F(1.055), F(2.4))
						  : c / F(12.92);
}

template<typename F>
Vector3<F> srgbToLinear(const Vector3<F>& srgb)
{
	return {srgbToLinear(srgb[0]), srgbToLinear(srgb[1]), srgbToLinear(srgb[2])};
}

inline QByteArray makeSRGBUtilsShader()
{
	static const QByteArray src = 1+R"(
#line 1 108
vec3 srgbToLinear(vec3 srgb)
{
	vec3 s = step(vec3(0.04045), srgb);
	vec3 d = vec3(1) - s;
	return s * pow((srgb+0.055)/1.055, vec3(2.4)) +
		   d * srgb/12.92;
}

vec3 linearToSRGB(vec3 lin)
{
	vec3 s = step(vec3(0.0031308), lin);
	vec3 d = vec3(1) - s;
	return s * (1.055*pow(lin, vec3(1./2.4))-0.055) +
		   d *  12.92*lin;
}
#line 1 0
)";

	return src;
}

#endif
