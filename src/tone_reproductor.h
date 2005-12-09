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

// Class which converts tones in function of the eye adaptation to luminance.
// The aim is to get on the screen something which is perceptualy accurate,
// ie. to compress high dynamic range luminance to CRT display range.
// The class perform mainly a fast implementation of the algorithm from the
// paper [1], with more accurate values from [2]. The blue shift formula is taken
// from [3] and combined with the Scotopic vision formula from [4].

// Important : you may call set_display_adaptation_luminance()
// and set_world_adaptation_luminance() before any call to xyY_to_RGB()
// or adapt_luminance otherwise the default values will be used. (they are
// appropriate for a daylight sky luminance)

// REFERENCES :
// Thanks to all the authors of the following papers i used for providing
// their work freely online.
//
// [1] "Tone Reproduction for Realistic Images", Tumblin and Rushmeier,
// IEEE Computer Graphics & Application, November 1993
//
// [2] "Tone Reproduction and Physically Based Spectral Rendering",
// Devlin, Chalmers, Wilkie and Purgathofer in EUROGRAPHICS 2002
//
// [3] "Night Rendering", H. Wann Jensen, S. Premoze, P. Shirley,
// W.B. Thompson, J.A. Ferwerda, M.M. Stark
//
// [4] "A Visibility Matching Tone Reproduction Operator for High Dynamic
// Range Scenes", G.W. Larson, H. Rushmeier, C. Piatko


#ifndef _TONE_REPRODUCTOR_H_
#define _TONE_REPRODUCTOR_H_

#include "fmath.h"

class ToneReproductor
{
public:
    ToneReproductor();
    virtual ~ToneReproductor();

	// Set the eye adaptation luminance for the display (and precompute what can be)
	// Usual luminance range is 1-100 cd/m^2 for a CRT screen
	// default value = 50 cd/m^2
	void set_display_adaptation_luminance(float display_adaptation_luminance);

	// Set the eye adaptation luminance for the world (and precompute what can be)
	// default value = 40000 cd/m^2 for Skylight
	// Star Light      : 0.001  cd/m^2
	// Moon Light      : 0.1    cd/m^2
	// Indoor Lighting : 100    cd/m^2
	// Sun Light       : 100000 cd/m^2
	void set_world_adaptation_luminance(float world_adaptation_luminance);

	// Set the maximum display luminance : default value = 100 cd/m^2
	// This value is used to scale the RGB range
	void set_max_display_luminance(float _MaxdL) {MaxdL = MaxdL;}

	// Set the display gamma : default value = 2.3
	void set_display_gamma(float _gamma) {gamma = _gamma;}

	// Return adapted luminance from world to display
	float adapt_luminance(float world_luminance) const
	{
		return powf((float)(world_luminance*M_PI*0.0001f),alpha_wa_over_alpha_da) * term2;
	}

	// Convert from xyY color system to RGB
	void xyY_to_RGB(float*);

private:
	float Lda;		// Display luminance adaptation (in cd/m^2)
	float Lwa;		// World   luminance adaptation (in cd/m^2)
	float MaxdL;	// Display maximum luminance (in cd/m^2)
	float gamma;	// Screen gamma value

	// Precomputed variables
	float alpha_da;
	float beta_da;
	float alpha_wa;
	float beta_wa;
	float alpha_wa_over_alpha_da;
	float term2;
};

#endif // _TONE_REPRODUCTOR_H_
