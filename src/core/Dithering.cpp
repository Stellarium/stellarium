/*
 * Stellarium
 * Copyright (C) 2018-2022 Ruslan Kabatsayev <b7.10110111@gmail.com>
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

#include "Dithering.hpp"
#include "BlueNoiseTriangleRemapped.hpp"

GLuint makeDitherPatternTexture(QOpenGLFunctions& gl)
{
	GLuint tex;
	gl.glGenTextures(1, &tex);
	gl.glBindTexture(GL_TEXTURE_2D, tex);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// OpenGL ES has different defined formats. However, the shader will use the red channel, so this should work:
	if(QOpenGLContext::currentContext()->isOpenGLES())
	{
		gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, std::size(blueNoiseTriangleRemapped), std::size(blueNoiseTriangleRemapped[0]),
						0, GL_LUMINANCE, GL_FLOAT, blueNoiseTriangleRemapped);
	}
	else
	{
		gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, std::size(blueNoiseTriangleRemapped), std::size(blueNoiseTriangleRemapped[0]),
						0, GL_RED, GL_FLOAT, blueNoiseTriangleRemapped);
	}
	return tex;
}

Vec3f calcRGBMaxValue(StelPainter::DitheringMode mode)
{
	switch(mode)
	{
		case StelPainter::DitheringMode::Color666:
			return Vec3f(63);
		case StelPainter::DitheringMode::Color565:
			return Vec3f(31,63,31);
		case StelPainter::DitheringMode::Color888:
			return Vec3f(255);
		case StelPainter::DitheringMode::Color101010:
			return Vec3f(1023);
		case StelPainter::DitheringMode::Disabled: // and
		default:
		    return Vec3f(0.);
	}
}

QString makeDitheringShader()
{
	return 1+R"(
uniform mediump vec3 rgbMaxValue;
uniform sampler2D ditherPattern;
mediump vec3 dither(mediump vec3 c)
{
	c = clamp(c, 0., 1.);

	if(rgbMaxValue.r==0.) return c;

	mediump vec3 noise=texture2D(ditherPattern,gl_FragCoord.xy/64.).rrr;

	{
		// Prevent undershoot (imperfect white) due to clipping of positive noise contributions
		mediump vec3 antiUndershootC = 1.+(0.5-sqrt(2.*rgbMaxValue*(1.-c)))/rgbMaxValue;
		mediump vec3 edge = 1.-1./(2.*rgbMaxValue);
		// Per-component version of: c = c > edge ? antiUndershootC : c;
		c = antiUndershootC + step(-edge, -c) * (c-antiUndershootC);
	}

	{
		// Prevent overshoot (imperfect black) due to clipping of negative noise contributions
		mediump vec3 antiOvershootC  = (-1.+sqrt(8.*rgbMaxValue*c))/(2.*rgbMaxValue);
		mediump vec3 edge = 1./(2.*rgbMaxValue);
		// Per-component version of: c = c < edge ? antiOvershootC : c;
		c = antiOvershootC + step(edge, c) * (c-antiOvershootC);
	}

	return c+noise/rgbMaxValue;
}
mediump vec4 dither(mediump vec4 c) { return vec4(dither(c.xyz),c.w); }
)";
}
