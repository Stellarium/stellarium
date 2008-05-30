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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <cmath>
#include <cassert>
# include <config.h>
#ifndef HAVE_POW10
# define HAVE_POW10 1
//# define pow10(x) pow(10,(x))
# define pow10(x) std::exp((x) * 2.3025850930)
#endif

#include "ToneReproducer.hpp"

/*********************************************************************
 Constructor: Set some default values to prevent bugs in case of bad use
*********************************************************************/
ToneReproducer::ToneReproducer() : Lda(50.f), Lwa(40000.f), oneOverMaxdL(1.f/100.f), sqrtOneOverMaxdL(1.f/10.f), oneOverGamma(1.f/2.3f)
{
	// Initialize  sensor
	setOutputScale();
	
	// Update alphaDa and betaDa values
	float log10Lwa = std::log10(Lwa);
	alphaWa = 0.4f * log10Lwa + 1.519f;
	betaWa = -0.4f * log10Lwa*log10Lwa + 0.218f * log10Lwa + 6.1642f;
	
	setDisplayAdaptationLuminance(Lda);
	setWorldAdaptationLuminance(Lwa);
}


/*********************************************************************
 Destructor
*********************************************************************/
ToneReproducer::~ToneReproducer()
{
}

/*********************************************************************
 Set the global scale
*********************************************************************/
void ToneReproducer::setOutputScale(float scale)
{
	outputScale=scale;
	sqrtOutputScale = std::sqrt(scale);
}
	
/*********************************************************************
 Set the eye adaptation luminance for the display (and precompute what can be)
*********************************************************************/
void ToneReproducer::setDisplayAdaptationLuminance(float _Lda)
{
	Lda = _Lda;

	// Update alphaDa and betaDa values
	float log10Lda = std::log10(Lda);
	alphaDa = 0.4f * log10Lda + 1.519f;
	betaDa = -0.4f * log10Lda*log10Lda + 0.218f * log10Lda + 6.1642f;

	// Update terms
	alphaWaOverAlphaDa = alphaWa/alphaDa;
	term2 = pow10((betaWa-betaDa)/alphaDa) / (M_PI*0.0001f);
	sqrtTerm2 = std::sqrt(term2);
	term2TimesOneOverMaxdLpOneOverGamma = std::pow(term2*oneOverMaxdL, oneOverGamma);
}

/*********************************************************************
 Set the eye adaptation luminance for the world (and precompute what can be)
*********************************************************************/
void ToneReproducer::setWorldAdaptationLuminance(float _Lwa)
{
	Lwa = _Lwa;

	// Update alphaDa and betaDa values
	float log10Lwa = std::log10(Lwa);
	alphaWa = 0.4f * log10Lwa + 1.519f;
	betaWa = -0.4f * log10Lwa*log10Lwa + 0.218f * log10Lwa + 6.1642f;

	// Update terms
	alphaWaOverAlphaDa = alphaWa/alphaDa;
	term2 = pow10((betaWa-betaDa)/alphaDa) / (M_PI*0.0001f);
	sqrtTerm2 = std::sqrt(term2);
	term2TimesOneOverMaxdLpOneOverGamma = std::pow(term2*oneOverMaxdL, oneOverGamma);
}


/*********************************************************************
 Convert from xyY color system to RGB according to the adaptation
 The Y component is in cd/m^2
*********************************************************************/
void ToneReproducer::xyYToRGB(float* color) const
{
	float log10Y;
	// 1. Hue conversion
	if (color[2] <= 0.f)
	{
		// qDebug() << "ToneReproducer::xyYToRGB: BIG WARNING: color[2]<=0: " << color[2];
		log10Y = -9e50;
	}
	else
	{
		log10Y = std::log10(color[2]);
	}

	// if log10Y>0.6, photopic vision only (with the cones, colors are seen)
	// else scotopic vision if log10Y<-2 (with the rods, no colors, everything blue),
	// else mesopic vision (with rods and cones, transition state)
	if (log10Y<0.6)
	{
		// Compute s, ratio between scotopic and photopic vision
		float s = 0.f;
		if (log10Y > -2.f)
		{
			const float op = (log10Y + 2.f)/2.6f;
			s = 3.f * op * op - 2 * op * op * op;
		}

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
	color[2] = std::pow((float)(color[2]*M_PI*0.0001f), alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;
	
	// Convert from xyY to XZY
	const float X = color[0] * color[2] / color[1];
	const float Y = color[2];
	const float Z = (1.f - color[0] - color[1]) * color[2] / color[1];

	// Use a XYZ to Adobe RGB (1998) matrix which uses a D65 reference white
	color[0] = 2.04148f  *X - 0.564977f*Y - 0.344713f *Z;
	color[1] =-0.969258f *X + 1.87599f *Y + 0.0415557f*Z;
	color[2] = 0.0134455f*X - 0.118373f*Y + 1.01527f  *Z;
}
