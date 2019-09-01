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

// Class which compute the daylight sky color
// Fast implementation of the algorithm from the article
// "A Practical Analytic Model for Daylight" by A. J. Preetham, Peter Shirley and Brian Smits.

#include "Skylight.hpp"

#include <cmath>
#include <cstdlib>

Skylight::Skylight() :
	thetas(0.f),
	T(0.f),
	zenithLuminance(0.f),
	zenithColorX(0.f),
	zenithColorY(0.f),
	eyeLumConversion(0.f)
{
	AY = BY = CY = DY = EY = 0.f;
	Ax = Bx = Cx = Dx = Ex = 0.f;
	Ay = By = Cy = Dy = Ey = 0.f;
	term_x = term_y = term_Y = 0.f;
}

Skylight::~Skylight()
{
}


void Skylight::setParams(float _sunZenithAngle, float _turbidity)
{
	// Set the two main variables
	thetas = _sunZenithAngle;
	T = _turbidity;

	// Precomputation of the distribution coefficients and zenith luminances/color
	computeZenithLuminance();
	computeZenithColor();
	computeLuminanceDistributionCoefs();
	computeColorDistributionCoefs();

	// Precompute everything possible to increase the get_CIE_value() function speed
	float cos_thetas = std::cos(thetas);
	term_x = zenithColorX   / ((1.f + Ax * std::exp(Bx)) * (1.f + Cx * std::exp(Dx*thetas) + Ex * cos_thetas * cos_thetas));
	term_y = zenithColorY   / ((1.f + Ay * std::exp(By)) * (1.f + Cy * std::exp(Dy*thetas) + Ey * cos_thetas * cos_thetas));
	term_Y = zenithLuminance / ((1.f + AY * std::exp(BY)) * (1.f + CY * std::exp(DY*thetas) + EY * cos_thetas * cos_thetas));
}

void Skylight::setParamsv(const float * _sunPos, float _turbidity)
{
	// Store sun position
	sunPos[0] = _sunPos[0];
	sunPos[1] = _sunPos[1];
	sunPos[2] = _sunPos[2];

	// Set the two main variables
	thetas = M_PI_2f - asinf(sunPos[2]);
	T = _turbidity;

	// Precomputation of the distribution coefficients and zenith luminances/color
	computeZenithLuminance();
	computeZenithColor();
	computeLuminanceDistributionCoefs();
	computeColorDistributionCoefs();

	// Precompute everything possible to increase the get_CIE_value() function speed
	float cos_thetas = sunPos[2];
	term_x = zenithColorX   / ((1.f + Ax * std::exp(Bx)) * (1.f + Cx * std::exp(Dx*thetas) + Ex * cos_thetas * cos_thetas));
	term_y = zenithColorY   / ((1.f + Ay * std::exp(By)) * (1.f + Cy * std::exp(Dy*thetas) + Ey * cos_thetas * cos_thetas));
	term_Y = zenithLuminance / ((1.f + AY * std::exp(BY)) * (1.f + CY * std::exp(DY*thetas) + EY * cos_thetas * cos_thetas));
}

