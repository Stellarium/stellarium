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
	static constexpr unsigned char bayerPattern[] =
	{
		// 8x8 Bayer ordered dithering pattern.
		 0*255/64,0,0, 32*255/64,0,0,  8*255/64,0,0, 40*255/64,0,0,  2*255/64,0,0, 34*255/64,0,0, 10*255/64,0,0, 42*255/64,0,0,
		48*255/64,0,0, 16*255/64,0,0, 56*255/64,0,0, 24*255/64,0,0, 50*255/64,0,0, 18*255/64,0,0, 58*255/64,0,0, 26*255/64,0,0,
		12*255/64,0,0, 44*255/64,0,0,  4*255/64,0,0, 36*255/64,0,0, 14*255/64,0,0, 46*255/64,0,0,  6*255/64,0,0, 38*255/64,0,0,
		60*255/64,0,0, 28*255/64,0,0, 52*255/64,0,0, 20*255/64,0,0, 62*255/64,0,0, 30*255/64,0,0, 54*255/64,0,0, 22*255/64,0,0,
		 3*255/64,0,0, 35*255/64,0,0, 11*255/64,0,0, 43*255/64,0,0,  1*255/64,0,0, 33*255/64,0,0,  9*255/64,0,0, 41*255/64,0,0,
		51*255/64,0,0, 19*255/64,0,0, 59*255/64,0,0, 27*255/64,0,0, 49*255/64,0,0, 17*255/64,0,0, 57*255/64,0,0, 25*255/64,0,0,
		15*255/64,0,0, 47*255/64,0,0,  7*255/64,0,0, 39*255/64,0,0, 13*255/64,0,0, 45*255/64,0,0,  5*255/64,0,0, 37*255/64,0,0,
		63*255/64,0,0, 31*255/64,0,0, 55*255/64,0,0, 23*255/64,0,0, 61*255/64,0,0, 29*255/64,0,0, 53*255/64,0,0, 21*255/64,0,0
	};
	gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB , 8, 8, 0, GL_RGB , GL_UNSIGNED_BYTE, bayerPattern);
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
