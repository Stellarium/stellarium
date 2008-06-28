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

// Class which compute the daylight sky color
// Fast implementation of the algorithm from the article
// "A Practical Analytic Model for Daylight" by A. J. Preetham, Peter Shirley and Brian Smits.

#include "Skylight.hpp"

#include <cmath>
#include <cstdlib>

/// Compute acos(x)
inline float fastAcos(float x)
{
	return M_PI/2.f - (
					   x + 1.f/6.f * x*x*x + 3.f/40.f * x*x*x*x*x +
			5.f/112.f * x*x*x*x*x*x*x
					  );
}

Skylight::Skylight() : thetas(0.f), T(0.f)
{
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
	thetas = M_PI_2 - std::asin((float)sunPos[2]);
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

// Compute the sky color at the given position in the CIE color system and store it in p.color
// p.color[0] is CIE x color component
// p.color[1] is CIE y color component
// p.color[2] is CIE Y color component (luminance)
// void Skylight::getxyYValue(skylightStruct * p)
// {
// 	float cosDistSun = std::cos(p->distSun);
// 	float oneOverCosZenithAngle = 1.f/std::cos(p->zenithAngle);
// 	p->color[0] = term_x * (1.f + Ax * std::exp(Bx * oneOverCosZenithAngle)) * (1.f + Cx * std::exp(Dx*p->distSun) +
// 		Ex * cosDistSun * cosDistSun);
// 	p->color[1] = term_y * (1.f + Ay * std::exp(By * oneOverCosZenithAngle)) * (1.f + Cy * std::exp(Dy*p->distSun) +
// 		Ey * cosDistSun * cosDistSun);
// 	p->color[2] = term_Y * (1.f + AY * std::exp(BY * oneOverCosZenithAngle)) * (1.f + CY * std::exp(DY*p->distSun) +
// 		EY * cosDistSun * cosDistSun);
// }

// Compute the sky color at the given position in the CIE color system and store it in p.color
// p.color[0] is CIE x color component
// p.color[1] is CIE y color component
// p.color[2] is CIE Y color component (luminance)
void Skylight::getxyYValuev(skylightStruct2& p) const
{
	float cosDistSun = sunPos[0]*p.pos[0]
                       + sunPos[1]*p.pos[1]
                       + sunPos[2]*p.pos[2];
	float distSun;
	if (cosDistSun <= -1.f ) {cosDistSun = -1.f;distSun = M_PI;}
	else if (cosDistSun >= 1.f ) {cosDistSun = 1.f;distSun = 0.f;}
	else distSun = fastAcos(cosDistSun);
	const float cosDistSun_q = cosDistSun*cosDistSun;

	float Fx = 0.f;
	float Fy = 0.f;
	float FY = 0.f;

	if (p.pos[2] > 0.f) {
	  float oneOverCosZenithAngle = 1.f / p.pos[2];
	  Fx = std::exp(Bx*oneOverCosZenithAngle);
	  Fy = std::exp(By*oneOverCosZenithAngle);
	  FY = std::exp(BY*oneOverCosZenithAngle);
	}

	p.color[0] = term_x * (1.f + Ax * Fx)
	                    * (1.f + Cx * std::exp(Dx*distSun) + Ex * cosDistSun_q);

	p.color[1] = term_y * (1.f + Ay * Fy)
	                    * (1.f + Cy * std::exp(Dy*distSun) + Ey * cosDistSun_q);

	p.color[2] = term_Y * (1.f + AY * FY)
	                    * (1.f + CY * std::exp(DY*distSun) + EY * cosDistSun_q);


	if (p.color[2] < 0 || p.color[0] < 0 || p.color[1] < 0)
	{
		p.color[0] = 0.25;
		p.color[1] = 0.25;
		p.color[2] = 0;
	}
}

