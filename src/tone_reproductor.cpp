/*
 * Copyright (C) 2003 Fabien Chéreau
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

#include <stdio.h>
#include <math.h>
#include "tone_reproductor.h"

// Set some values to prevent bugs in case of bad use
tone_reproductor::tone_reproductor() : Lda(50.f), Lwa(40000.f), MaxdL(100.f), gamma(2.3f)
{
	set_display_adaptation_luminance(Lda);
	set_world_adaptation_luminance(Lwa);
}

tone_reproductor::~tone_reproductor()
{
}

// Set the eye adaptation luminance for the display and precompute what can be
// Usual luminance range is 1-100 cd/m^2 for a CRT screen
void tone_reproductor::set_display_adaptation_luminance(float _Lda)
{
	static float log10Lda;
	Lda = _Lda;

	// Update alpha_da and beta_da values
	log10Lda = log10f(Lda);
	alpha_da = 0.4f * log10Lda + 1.519f;
	beta_da = -0.4f * log10Lda*log10Lda + 0.218f * log10Lda + 6.1642f;

	// Update terms
	alpha_wa_over_alpha_da = alpha_wa/alpha_da;
	term2 = powf(10.f, (beta_wa-beta_da)/alpha_da) / (M_PI*0.0001f);
}

// Set the eye adaptation luminance for the world and precompute what can be
void tone_reproductor::set_world_adaptation_luminance(float _Lwa)
{
	static float log10Lwa;
	Lwa = _Lwa;

	// Update alpha_da and beta_da values
	log10Lwa = log10f(Lwa);
	alpha_wa = 0.4f * log10Lwa + 1.519f;
	beta_wa = -0.4f * log10Lwa*log10Lwa + 0.218f * log10Lwa + 6.1642f;

	// Update terms
	alpha_wa_over_alpha_da = alpha_wa/alpha_da;
	term2 = powf(10.f, (beta_wa-beta_da)/alpha_da) / (M_PI*0.0001f);

}


// Convert from xyY color system to RGB according to the adaptation
// The Y component is in cd/m^2
void tone_reproductor::xyY_to_RGB(float* color)
{
	// 1. Hue conversion
	static float log10Y;
	log10Y = log10f(color[2]);
	// if log10Y>0.6, photopic vision only (with the cones, colors are seen)
	// else scotopic vision if log10Y<-2 (with the rods, no colors, everything blue),
	// else mesopic vision (with rods and cones, transition state)
	if (log10Y<0.6)
	{
		// Compute s, ratio between scotopic and photopic vision
		static float s;
		s = 0.f;
		if (log10Y > -2.f)
		{
			static float op;
			op = (log10Y + 2.f)/2.6f;
			s = 3.f * op * op - 2 * op * op * op;
		}

		// Do the blue shift for scotopic vision simulation (night vision) [3]
		// The "night blue" is x,y(0.25, 0.25)
		color[0] = (1.f - s) * 0.25f + s * color[0];	// Add scotopic + photopic components
		color[1] = (1.f - s) * 0.25f + s * color[1];	// Add scotopic + photopic components

		// Take into account the scotopic luminance approximated by V [3] [4]
		static float V;
		V = color[2] * (1.33f * (1.f + color[1] / color[0] + color[0] * (1.f - color[0] - color[1])) - 1.68f);
		color[2] = 0.4468f * (1.f - s) * V + s * color[2];
	}

	// 2. Adapt the luminance value and scale it to fit in the RGB range [2]
	color[2] = powf(adapt_luminance(color[2]) / MaxdL,1.f/gamma);

	// Convert from xyY to XZY
	static float X;
	static float Y;
	static float Z;
	X = color[0] * color[2] / color[1];
	Y = color[2];
	Z = (1.f - color[0] - color[1]) * color[2] / color[1];

	// Use a XYZ to Adobe RGB (1998) matrix which uses a D65 reference white
	color[0] = 2.04148f  *X - 0.564977f*Y - 0.344713f *Z;
	color[1] =-0.969258f *X + 1.87599f *Y + 0.0415557f*Z;
	color[2] = 0.0134455f*X - 0.118373f*Y + 1.01527f  *Z;
}

