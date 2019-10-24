/*
 * Copyright (C) 2003 Fabien Chereau
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

#include "StelToneReproducer.hpp"
#include "StelUtils.hpp"
#include <cmath>

/*********************************************************************
 Constructor: Set some default values to prevent bugs in case of bad use
*********************************************************************/
StelToneReproducer::StelToneReproducer() : Lda(50.f), Lwa(40000.f), oneOverMaxdL(1.f/100.f), lnOneOverMaxdL(std::log(1.f/100.f)), oneOverGamma(1.f/2.2222f)
{
	// Initialize  sensor
	setInputScale();
	
	// Update alphaDa and betaDa values
	float log10Lwa = std::log10(Lwa);
	alphaWa = 0.4f * log10Lwa + 1.619f;
	betaWa = -0.4f * log10Lwa*log10Lwa + 0.218f * log10Lwa + 6.1642f;
	
	setDisplayAdaptationLuminance(Lda);
	setWorldAdaptationLuminance(Lwa);
}


/*********************************************************************
 Destructor
*********************************************************************/
StelToneReproducer::~StelToneReproducer()
{
}

/*********************************************************************
 Set the global input scale
*********************************************************************/
void StelToneReproducer::setInputScale(float scale)
{
	inputScale=scale;
	lnInputScale = std::log(inputScale);
}
	
/*********************************************************************
 Set the eye adaptation luminance for the display (and precompute what can be)
*********************************************************************/
void StelToneReproducer::setDisplayAdaptationLuminance(float _Lda)
{
	Lda = _Lda;

	// Update alphaDa and betaDa values
	float log10Lda = std::log10(Lda);
	alphaDa = 0.4f * log10Lda + 1.619f;
	betaDa = -0.4f * log10Lda*log10Lda + 0.218f * log10Lda + 6.1642f;

	// Update terms
	alphaWaOverAlphaDa = alphaWa/alphaDa;
	term2 =(stelpow10f((betaWa-betaDa)/alphaDa) / (M_PIf*0.0001f));
	lnTerm2 = std::log(term2);
	term2TimesOneOverMaxdLpOneOverGamma = std::pow(term2*oneOverMaxdL, oneOverGamma);
}

/*********************************************************************
 Set the eye adaptation luminance for the world (and precompute what can be)
*********************************************************************/
void StelToneReproducer::setWorldAdaptationLuminance(float _Lwa)
{
	Lwa = _Lwa;

	// Update alphaDa and betaDa values
	float log10Lwa = std::log10(Lwa);
	alphaWa = 0.4f * log10Lwa + 1.619f;
	betaWa = -0.4f * log10Lwa*log10Lwa + 0.218f * log10Lwa + 6.1642f;

	// Update terms
	alphaWaOverAlphaDa = alphaWa/alphaDa;
	term2 = (stelpow10f((betaWa-betaDa)/alphaDa) / (M_PIf*0.0001f));
	lnTerm2 = std::log(term2);
	term2TimesOneOverMaxdLpOneOverGamma = std::pow(term2*oneOverMaxdL, oneOverGamma);
}


/*********************************************************************
 Convert from xyY color system to RGB according to the adaptation
 The Y component is in cd/m^2
*********************************************************************/
void StelToneReproducer::xyYToRGB(float* color) const
{
	// 1. Hue conversion
	// if log10Y>0.6, photopic vision only (with the cones, colors are seen)
	// else scotopic vision if log10Y<-2 (with the rods, no colors, everything blue),
	// else mesopic vision (with rods and cones, transition state)
	if (color[2] <= 0.01f)
	{
		// special case for s = 0 (x=0.25, y=0.25)
		color[2] *= 0.5121445f;
		color[2] = std::pow((color[2]*M_PIf*0.0001f), alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;
		color[0] = 0.787077f*color[2];
		color[1] = 0.9898434f*color[2];
		color[2] *= 1.9256125f;
		return;
	}
	
	if (color[2]<3.9810717055349722f)
	{
		// Compute s, ratio between scotopic and photopic vision
		const float op = (std::log10(color[2]) + 2.f)/2.6f;
		const float s = op * op *(3.f - 2.f * op);
		// Do the blue shift for scotopic vision simulation (night vision) [3]
		// The "night blue" is x,y(0.25, 0.25)
		color[0] = (1.f - s) * 0.25f + s * color[0];	// Add scotopic + photopic components
		color[1] = (1.f - s) * 0.25f + s * color[1];	// Add scotopic + photopic components
		// Take into account the scotopic luminance approximated by V [3] [4]
		const float V = color[2] * (1.33f * (1.f + color[1] / color[0] + color[0] * (1.f - color[0] - color[1])) - 1.68f);
		color[2] = 0.4468f * (1.f - s) * V + s * color[2];
	}

	// 2. Adapt the luminance value and scale it to fit in the RGB range [2]
	// color[2] = std::pow(adaptLuminanceScaled(color[2]), oneOverGamma);
	color[2] = std::pow((color[2]*M_PIf*0.0001f), alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;
	
	// Convert from xyY to XZY
	const float X = color[0] * color[2] / color[1];
	const float Y = color[2];
	const float Z = (1.f - color[0] - color[1]) * color[2] / color[1];

	// Use a XYZ to sRGB matrix which uses a D65 reference white
	color[0] = 3.2404542f  *X - 1.5371385f*Y - 0.4985314f *Z;
	color[1] =-0.9692660f *X + 1.8760108f *Y + 0.0415560f*Z;
	color[2] = 0.0556434f*X - 0.2040259f*Y + 1.0572252f  *Z;
}
