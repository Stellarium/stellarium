/*
 * Stellarium
 * Copyright (C) 2018 Ruslan Kabatsayev <b7.10110111@gmail.com>
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

#ifndef DITHERING_HPP
#define DITHERING_HPP

#include "StelOpenGL.hpp"
#include "StelPainter.hpp"
#include "VecMath.hpp"

inline GLuint makeBayerPatternTexture(QOpenGLFunctions& gl)
{
	GLuint tex;
	gl.glGenTextures(1, &tex);
	gl.glBindTexture(GL_TEXTURE_2D, tex);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	static constexpr float bayerPattern[8*8] =
	{
		// 8x8 Bayer ordered dithering pattern.
		0/64.f, 32/64.f,  8/64.f, 40/64.f,  2/64.f, 34/64.f, 10/64.f, 42/64.f,
		48/64.f, 16/64.f, 56/64.f, 24/64.f, 50/64.f, 18/64.f, 58/64.f, 26/64.f,
		12/64.f, 44/64.f,  4/64.f, 36/64.f, 14/64.f, 46/64.f,  6/64.f, 38/64.f,
		60/64.f, 28/64.f, 52/64.f, 20/64.f, 62/64.f, 30/64.f, 54/64.f, 22/64.f,
		3/64.f, 35/64.f, 11/64.f, 43/64.f,  1/64.f, 33/64.f,  9/64.f, 41/64.f,
		51/64.f, 19/64.f, 59/64.f, 27/64.f, 49/64.f, 17/64.f, 57/64.f, 25/64.f,
		15/64.f, 47/64.f,  7/64.f, 39/64.f, 13/64.f, 45/64.f,  5/64.f, 37/64.f,
		63/64.f, 31/64.f, 55/64.f, 23/64.f, 61/64.f, 29/64.f, 53/64.f, 21/64.f
	};
	gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 8, 8, 0, GL_RED, GL_FLOAT, bayerPattern);
	return tex;
}

inline Vec3f calcRGBMaxValue(StelPainter::DitheringMode mode)
{
	switch(mode)
	{
		default:
		case StelPainter::DitheringMode::Disabled:
			return Vec3f(0.);
		case StelPainter::DitheringMode::Color666:
			return Vec3f(63);
		case StelPainter::DitheringMode::Color565:
			return Vec3f(31,63,31);
		case StelPainter::DitheringMode::Color888:
			return Vec3f(255);
		case StelPainter::DitheringMode::Color101010:
			return Vec3f(1023);
	}
}

inline QString makeDitheringShader()
{
	return
			R"(uniform mediump vec3 rgbMaxValue;
			uniform sampler2D bayerPattern;
			mediump vec3 dither(mediump vec3 c)
			{
			if(rgbMaxValue.r==0.) return c;
			mediump float bayer=texture2D(bayerPattern,gl_FragCoord.xy/8.).r;

			mediump vec3 rgb=c*rgbMaxValue;
			mediump vec3 head=floor(rgb);
			mediump vec3 tail=rgb-head;
			return (head+1.-step(tail,vec3(bayer)))/rgbMaxValue;
			}
			mediump vec4 dither(mediump vec4 c) { return vec4(dither(c.xyz),c.w); }
			)";
}

#endif
