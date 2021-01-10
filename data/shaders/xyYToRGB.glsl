/*
 * Stellarium
 * Copyright (C) 2002-2018 Fabien Chereau and Stellarium contributors
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

const highp float pi = 3.1415926535897931;
const highp float ln10 = 2.3025850929940459;

// Variable for the xyYTo RGB conversion
uniform highp float alphaWaOverAlphaDa;
uniform highp float oneOverGamma;
uniform highp float term2TimesOneOverMaxdLpOneOverGamma;
uniform highp float brightnessScale;

// Variables for the color computation
uniform highp vec3 sunPos;
uniform mediump float term_x, Ax, Bx, Cx, Dx, Ex;
uniform mediump float term_y, Ay, By, Cy, Dy, Ey;

// The current projection matrix
uniform mediump mat4 projectionMatrix;

// Contains the 2d position of the point on the screen (before multiplication by the projection matrix)
attribute mediump vec2 skyVertex;

// Contains the r,g,b,Y (luminosity) components.
attribute highp vec4 skyColor;

// The output variable passed to the fragment shader
varying mediump vec3 resultSkyColor;

void main()
{
	gl_Position = projectionMatrix*vec4(skyVertex, 0., 1.);

	///////////////////////////////////////////////////////////////////////////
	// First compute the xy color component
	// skyColor contains the unprojected vertex position in r,g,b
	// + the Y (luminance) component of the color in the alpha channel
	highp vec3 unprojectedVertex = skyColor.xyz;
	highp float Y = skyColor.w;
	highp float x,y;
	if (Y>0.01)
	{
		highp float cosDistSunq = dot(sunPos,unprojectedVertex);
		highp float distSun=acos(cosDistSunq);
		highp float oneOverCosZenithAngle = (unprojectedVertex.z==0.) ? 9999999999999. : 1. / unprojectedVertex.z;

		cosDistSunq*=cosDistSunq;
		x = term_x * (1. + Ax * exp(Bx*oneOverCosZenithAngle))* (1. + Cx * exp(Dx*distSun) + Ex * cosDistSunq);
		y = term_y * (1. + Ay * exp(By*oneOverCosZenithAngle))* (1. + Cy * exp(Dy*distSun) + Ey * cosDistSunq);
		if (x < 0. || y < 0.)
		{
			x = 0.25;
			y = 0.25;
		}
	}
	else
	{
		x = 0.25;
		y = 0.25;
	}


	///////////////////////////////////////////////////////////////////////////
	// Now we have the xyY components, need to convert to RGB

	// 1. Hue conversion
	// if log10Y>0.6, photopic vision only (with the cones, colors are seen)
	// else scotopic vision if log10Y<-2 (with the rods, no colors, everything blue),
	// else mesopic vision (with rods and cones, transition state)
	if (Y <= 0.01)
	{
		// special case for s = 0 (x=0.25, y=0.25)
		Y *= 0.5121445;
		Y = pow(abs(Y*pi*1e-4), alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;
		resultSkyColor = vec3(0.787077, 0.9898434, 1.9256125) * Y * brightnessScale;
	}
	else
	{
		if (Y<3.9810717055349722)
		{
			// Compute s, ratio between scotopic and photopic vision
			float op = (log(Y)/ln10 + 2.)/2.6;
			float s = op * op *(3. - 2. * op);
			// Do the blue shift for scotopic vision simulation (night vision) [3]
			// The "night blue" is x,y(0.25, 0.25)
			x = (1. - s) * 0.25 + s * x;	// Add scotopic + photopic components
			y = (1. - s) * 0.25 + s * y;	// Add scotopic + photopic components
			// Take into account the scotopic luminance approximated by V [3] [4]
			float V = Y * (1.33 * (1. + y / x + x * (1. - x - y)) - 1.68);
			Y = 0.4468 * (1. - s) * V + s * Y;
		}

		// 2. Adapt the luminance value and scale it to fit in the RGB range [2]
		// Y = std::pow(adaptLuminanceScaled(Y), oneOverGamma);
		Y = pow(abs(Y*pi*1e-4), alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;

		// Convert from xyY to XYZ
		highp vec3 XYZ = vec3(x * Y / y, Y, (1. - x - y) * Y / y);
		// Use the XYZ to sRGB matrix which uses a D65 reference white
		// Ref: http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
		const highp mat3 XYZ2sRGBl=mat3(vec3(3.2404542,-0.9692660,0.0556434),
										vec3(-1.5371385,1.8760108,-0.2040259),
										vec3(-0.4985314,0.0415560,1.0572252));
		resultSkyColor = XYZ2sRGBl * XYZ;
		resultSkyColor *= brightnessScale;
	}
}
