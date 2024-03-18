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

#include "StelSRGB.hpp"
#include "StelMainView.hpp"

QByteArray makeSRGBUtilsShader()
{
	static const QByteArray src = 1+R"(
#line 1 108
float srgbToLinear(float srgb)
{
	float s = step(float(0.04045), srgb);
	float d = 1. - s;
	return s * pow((srgb+0.055)/1.055, float(2.4)) +
	       d * srgb/12.92;
}

float linearToSRGB(float lin)
{
	float s = step(float(0.0031308), lin);
	float d = 1. - s;
	return s * (1.055*pow(lin, float(1./2.4))-0.055) +
	       d *  12.92*lin;
}

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
