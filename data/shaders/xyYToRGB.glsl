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

uniform highp float alphaWaOverAlphaDa;
uniform highp float oneOverGamma;
uniform highp float term2TimesOneOverMaxdLpOneOverGamma;
uniform highp float brightnessScale; // Only the atmosphere fader value [0...1], i.e. drawn or not, or in transition.
uniform bool doSRGB;


vec3 xyYToRGB(highp float x, highp float y, highp float Y)
{
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
		return vec3(0.787077, 0.9898434, 1.9256125) * Y * brightnessScale;
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

//  HEAD of 2021:
//		// Convert from xyY to XYZ
//		highp vec3 XYZ = vec3(x * Y / y, Y, (1. - x - y) * Y / y);
//		// Use the XYZ to sRGB matrix which uses a D65 reference white
//		// Ref: http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
//		const highp mat3 XYZ2sRGBl=mat3(vec3(3.2404542,-0.9692660,0.0556434),
//										vec3(-1.5371385,1.8760108,-0.2040259),
//										vec3(-0.4985314,0.0415560,1.0572252));
//		return XYZ2sRGBl * XYZ * brightnessScale;
//  EXPERIMENTAL
		// Convert from xyY to XZY
		// Use a XYZ to Adobe RGB (1998) matrix which uses a D65 reference white

		mediump vec3 XYZ = vec3(x * Y / y, Y, (1. - x - y) * Y / y);
		if(!doSRGB)
		{
                    // Apparently AdobeRGB1998 transformation. Use values from same source:
                    resultSkyColor = vec3(2.0413690*XYZ.x -0.5649464*XYZ.y-0.3446944*XYZ.z,
                                         -0.9692660*XYZ.x +1.8760108*XYZ.y+0.0415560*XYZ.z,
                                          0.0134474*XYZ.x -0.1183897*XYZ.y+1.0154096*XYZ.z);
		}
		else
		{
                    // sRGB transformation. Recipe contains low-level linearity, however this causes a really ugly artifact.
                    // Matrix from WP: https://en.wikipedia.org/wiki/SRGB
                    resultSkyColor = vec3(3.2406 * XYZ.x -1.5372 * XYZ.y-0.4986 * XYZ.z,
                                         -0.9689 * XYZ.x +1.8758 * XYZ.y+0.0415 * XYZ.z,
                                          0.0557 * XYZ.x -0.2040 * XYZ.y+1.0570 * XYZ.z);
                    // This is now preliminary sRGB. We have to scale this with preset Gamma=2.4, channel-wise!
                    // This scaling causes a terrible visual problem. Disabling. 
                    //resultSkyColor.x=( resultSkyColor.x <= 0.0031308 ? 12.92*resultSkyColor.x : 1.055*pow(resultSkyColor.x, 1./2.4) - 0.055);
                    //resultSkyColor.y=( resultSkyColor.y <= 0.0031308 ? 12.92*resultSkyColor.y : 1.055*pow(resultSkyColor.y, 1./2.4) - 0.055);
                    //resultSkyColor.z=( resultSkyColor.z <= 0.0031308 ? 12.92*resultSkyColor.z : 1.055*pow(resultSkyColor.z, 1./2.4) - 0.055);
		}

        // final scaling. GZ: Not sure, maybe do this scale before the Gamma step just above.
		return resultSkyColor*brightnessScale;
// EXPERIMENT END 
	}
}
