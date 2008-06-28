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


void Skylight::set_params(float _sun_zenith_angle, float _turbidity)
{
	// Set the two main variables
	thetas = _sun_zenith_angle;
	T = _turbidity;

	// Precomputation of the distribution coefficients and zenith luminances/color
	compute_zenith_luminance();
	compute_zenith_color();
	compute_luminance_distribution_coefs();
	computeColorDistributionCoefs();

	// Precompute everything possible to increase the get_CIE_value() function speed
	float cos_thetas = std::cos(thetas);
	term_x = zenith_color_x   / ((1.f + Ax * std::exp(Bx)) * (1.f + Cx * std::exp(Dx*thetas) + Ex * cos_thetas * cos_thetas));
	term_y = zenith_color_y   / ((1.f + Ay * std::exp(By)) * (1.f + Cy * std::exp(Dy*thetas) + Ey * cos_thetas * cos_thetas));
	term_Y = zenith_luminance / ((1.f + AY * std::exp(BY)) * (1.f + CY * std::exp(DY*thetas) + EY * cos_thetas * cos_thetas));

}

void Skylight::set_paramsv(const float * _sun_pos, float _turbidity)
{
	// Store sun position
	sun_pos[0] = _sun_pos[0];
	sun_pos[1] = _sun_pos[1];
	sun_pos[2] = _sun_pos[2];

	// Set the two main variables
	thetas = M_PI_2 - std::asin((float)sun_pos[2]);
	T = _turbidity;

	// Precomputation of the distribution coefficients and zenith luminances/color
	compute_zenith_luminance();
	compute_zenith_color();
	compute_luminance_distribution_coefs();
	computeColorDistributionCoefs();

	// Precompute everything possible to increase the get_CIE_value() function speed
	float cos_thetas = sun_pos[2];
	term_x = zenith_color_x   / ((1.f + Ax * std::exp(Bx)) * (1.f + Cx * std::exp(Dx*thetas) + Ex * cos_thetas * cos_thetas));
	term_y = zenith_color_y   / ((1.f + Ay * std::exp(By)) * (1.f + Cy * std::exp(Dy*thetas) + Ey * cos_thetas * cos_thetas));
	term_Y = zenith_luminance / ((1.f + AY * std::exp(BY)) * (1.f + CY * std::exp(DY*thetas) + EY * cos_thetas * cos_thetas));
}

// Compute the sky color at the given position in the CIE color system and store it in p.color
// p.color[0] is CIE x color component
// p.color[1] is CIE y color component
// p.color[2] is CIE Y color component (luminance)
// void Skylight::get_xyY_value(skylight_struct * p)
// {
// 	float cos_dist_sun = std::cos(p->dist_sun);
// 	float one_over_cos_zenith_angle = 1.f/std::cos(p->zenith_angle);
// 	p->color[0] = term_x * (1.f + Ax * std::exp(Bx * one_over_cos_zenith_angle)) * (1.f + Cx * std::exp(Dx*p->dist_sun) +
// 		Ex * cos_dist_sun * cos_dist_sun);
// 	p->color[1] = term_y * (1.f + Ay * std::exp(By * one_over_cos_zenith_angle)) * (1.f + Cy * std::exp(Dy*p->dist_sun) +
// 		Ey * cos_dist_sun * cos_dist_sun);
// 	p->color[2] = term_Y * (1.f + AY * std::exp(BY * one_over_cos_zenith_angle)) * (1.f + CY * std::exp(DY*p->dist_sun) +
// 		EY * cos_dist_sun * cos_dist_sun);
// }

// Compute the sky color at the given position in the CIE color system and store it in p.color
// p.color[0] is CIE x color component
// p.color[1] is CIE y color component
// p.color[2] is CIE Y color component (luminance)
void Skylight::get_xyY_valuev(skylight_struct2& p) const
{
	float cos_dist_sun = sun_pos[0]*p.pos[0]
                       + sun_pos[1]*p.pos[1]
                       + sun_pos[2]*p.pos[2];
	float dist_sun;
	if (cos_dist_sun <= -1.f ) {cos_dist_sun = -1.f;dist_sun = M_PI;}
	else if (cos_dist_sun >= 1.f ) {cos_dist_sun = 1.f;dist_sun = 0.f;}
	else dist_sun = fastAcos(cos_dist_sun);
	const float cos_dist_sun_q = cos_dist_sun*cos_dist_sun;

	float Fx = 0.f;
	float Fy = 0.f;
	float FY = 0.f;

	if (p.pos[2] > 0.f) {
	  float one_over_cos_zenith_angle = 1.f / p.pos[2];
	  Fx = std::exp(Bx*one_over_cos_zenith_angle);
	  Fy = std::exp(By*one_over_cos_zenith_angle);
	  FY = std::exp(BY*one_over_cos_zenith_angle);
	}

	p.color[0] = term_x * (1.f + Ax * Fx)
	                    * (1.f + Cx * std::exp(Dx*dist_sun) + Ex * cos_dist_sun_q);

	p.color[1] = term_y * (1.f + Ay * Fy)
	                    * (1.f + Cy * std::exp(Dy*dist_sun) + Ey * cos_dist_sun_q);

	p.color[2] = term_Y * (1.f + AY * FY)
	                    * (1.f + CY * std::exp(DY*dist_sun) + EY * cos_dist_sun_q);


	if (p.color[2] < 0 || p.color[0] < 0 || p.color[1] < 0)
	{
		p.color[0] = 0.25;
		p.color[1] = 0.25;
		p.color[2] = 0;
	}
}

