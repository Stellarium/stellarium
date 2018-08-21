/*
 * Stellarium
 * Copyright (C) 2018 Guillaume Chereau
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

#ifndef SATURATION_SHADER_HPP
#define SATURATION_SHADER_HPP

#include <QString>

//! Return a GLSL function that allows to adjust the saturation of a color.
//! HSV/RGB conversion function from Sam Hocevar:
//! http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
static inline QString makeSaturationShader()
{
	return R"(
	mediump vec3 rgb2hsv(mediump vec3 c)
	{
		mediump vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
		mediump vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
		mediump vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);
		mediump float d = q.x - min(q.w, q.y);
		mediump float e = 1.0e-10;
		return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
	}

	mediump vec3 hsv2rgb(mediump vec3 c)
	{
		mediump vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
		mediump vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
		return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
	}

	mediump vec3 saturate(mediump vec3 c, mediump float x)
	{
		c = rgb2hsv(c);
		c.y *= x;
		return hsv2rgb(c);
	}

	mediump vec4 saturate(mediump vec4 c, mediump float x)
	{
		return vec4(saturate(c.rgb, x), c.a);
	}

	)";
}

#endif
