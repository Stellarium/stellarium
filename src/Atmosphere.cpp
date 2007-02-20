/*
 * Stellarium
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

//	Class which compute and display the daylight sky color using openGL
//	the sky is computed with the Skylight class.

// TODO : Adaptative resolution for optimization

#include "Atmosphere.hpp"
#include "stellarium.h"
#include "StelUtils.hpp"
#include "Projector.hpp"

Atmosphere::Atmosphere(void)
           :viewport(0,0,0,0),sky_resolution_y(44), grid(0),
            world_adaptation_luminance(0.f), atm_intensity(0)
{
	setFadeDuration(3.f);
}

Atmosphere::~Atmosphere(void)
{
	if (grid) {delete[] grid;grid = 0;}
}

void Atmosphere::compute_color(double JD, Vec3d sunPos, Vec3d moonPos, float moon_phase,
                               ToneReproducer * eye, Projector* prj,
                               float latitude, float altitude, float temperature, float relative_humidity)
{
	if (viewport != prj->getViewport()) {
		viewport = prj->getViewport();
		if (grid) {
			delete grid;
			grid = 0;
		}
		sky_resolution_x = (int)floor(0.5+sky_resolution_y*(0.5*sqrt(3.0))
		                                 *prj->getViewportWidth()
		                                 /prj->getViewportHeight());
		grid = new GridPoint[(1+sky_resolution_x)*(1+sky_resolution_y)];
//cout << "Atmosphere::compute_color: "
//     << (1+sky_resolution_x)*(1+sky_resolution_y)
//     << " Gridpoints instead of " << (48*48)
//     << endl;
		float stepX = (float)prj->getViewportWidth() / (sky_resolution_x-0.5);
		float stepY = (float)prj->getViewportHeight() / sky_resolution_y;
		float viewport_left = (float)prj->getViewportPosX();
		float viewport_bottom = (float)prj->getViewportPosY();
		for (int x=0; x<=sky_resolution_x; ++x)
			for(int y=0; y<=sky_resolution_y; ++y) {
				GridPoint &g(grid[y*(1+sky_resolution_x)+x]);
				g.pos_2d[0] = viewport_left
				            + ((x == 0) ? 0.f :
				               (x == sky_resolution_x) ? (float)prj->getViewportWidth() :
				               (x-0.5*(y&1))*stepX);
				g.pos_2d[1] = viewport_bottom+y*stepY;
			}
	}

	float min_mw_lum = 0.13;

	// no need to calculate if not visible
	if(!fader.getInterstate())
	{
		atm_intensity = 0;
		world_adaptation_luminance = 3.75f;
		milkyway_adaptation_luminance = min_mw_lum;  // brighter than without atm, since no drawing addition of atm brightness
		return;
	}
	else
	{
		atm_intensity = fader.getInterstate();
	}


	//Vec3d obj;
	skylight_struct2 b2;

	// these are for radii
	double sun_angular_size = atan(696000./AU/sunPos.length());
	double moon_angular_size = atan(1738./AU/moonPos.length());

	double touch_angle = sun_angular_size + moon_angular_size;
	double dark_angle = moon_angular_size - sun_angular_size;

	sunPos.normalize();
	moonPos.normalize();

	// determine luminance falloff during solar eclipses
	double separation_angle = acos( sunPos.dot( moonPos ));  // angle between them

	//	printf("touch at %f\tnow at %f (%f)\n", touch_angle, separation_angle, separation_angle/touch_angle);

	// bright stars should be visible at total eclipse
	// TODO: correct for atmospheric diffusion
	// TODO: use better coverage function (non-linear)
	// because of above issues, this algorithm darkens more quickly than reality
	if( separation_angle < touch_angle)
	{
		float min;
		if(dark_angle < 0)
		{
			// annular eclipse
			float asun = sun_angular_size*sun_angular_size;
			min = (asun - moon_angular_size*moon_angular_size)/asun;  // minimum proportion of sun uncovered
			dark_angle *= -1;
		}
		else min = 0.004;  // so bright stars show up at total eclipse

		if(separation_angle < dark_angle) atm_intensity = min;
		else atm_intensity *= min + (1.-min)*(separation_angle-dark_angle)/(touch_angle-dark_angle);

		//		printf("atm int %f (min %f)\n", atm_intensity, min);
	}

	float sun_pos[3];
	sun_pos[0] = sunPos[0];
	sun_pos[1] = sunPos[1];
	sun_pos[2] = sunPos[2];

	float moon_pos[3];
	moon_pos[0] = moonPos[0];
	moon_pos[1] = moonPos[1];
	moon_pos[2] = moonPos[2];

	sky.set_paramsv(sun_pos, 5.f);

	skyb.set_loc(latitude * M_PI/180., altitude, temperature, relative_humidity);
	skyb.set_sun_moon(moon_pos[2], sun_pos[2]);

	// Calculate the date from the julian day.
	ln_date date;
	get_date(JD, &date);

	skyb.set_date(date.years, date.months, moon_phase);

	Vec3d point(1., 0., 0.);

	// Variables used to compute the average sky luminance
	double sum_lum = 0.;
	unsigned int nb_lum = 0;

	prj->setCurrentFrame(Projector::FRAME_LOCAL);

	// Compute the sky color for every point above the ground
	for (int x=0; x<=sky_resolution_x; ++x)
	{
		for(int y=0; y<=sky_resolution_y; ++y)
		{
            GridPoint &g(grid[y*(1+sky_resolution_x)+x]);
			prj->unProject(g.pos_2d[0],g.pos_2d[1],point);

			assert(fabs(point.lengthSquared()-1.0) < 1e-10);

			if (point[2]<=0)
			{
				point[2] = -point[2];
				// The sky below the ground is the symetric of the one above :
				// it looks nice and gives proper values for brightness estimation
			}

			b2.pos[0] = point[0];
			b2.pos[1] = point[1];
			b2.pos[2] = point[2];

			// Use the Skylight model for the color
			sky.get_xyY_valuev(b2);

			// Use the Skybright.cpp 's models for brightness which gives better results.
			b2.color[2] = skyb.get_luminance(moon_pos[0]*b2.pos[0]+moon_pos[1]*b2.pos[1]+
			                                 moon_pos[2]*b2.pos[2], sun_pos[0]*b2.pos[0]+sun_pos[1]*b2.pos[1]+
			                                 sun_pos[2]*b2.pos[2], b2.pos[2]);


			sum_lum+=b2.color[2];
			++nb_lum;
			eye->xyY_to_RGB(b2.color);
			grid[y*(1+sky_resolution_x)+x].color.set(
			              atm_intensity*b2.color[0],
			              atm_intensity*b2.color[1],
			              atm_intensity*b2.color[2]);
		}
	}

	world_adaptation_luminance = 3.75f + 3.5*sum_lum/nb_lum*atm_intensity;
	milkyway_adaptation_luminance = min_mw_lum*(1-atm_intensity) + 30*sum_lum/nb_lum*atm_intensity;

	sum_lum = 0.f;
	nb_lum = 0;
}



// Draw the atmosphere using the precalc values stored in tab_sky
void Atmosphere::draw(Projector* prj)
{
	if(fader.getInterstate())
	{
		// printf("Atm int: %f\n", atm_intensity);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		for (int y2=0; y2<sky_resolution_y; ++y2)
		{
			const GridPoint *g0 = grid + y2*(1+sky_resolution_x);
			const GridPoint *g1 = g0;
			if (y2&1) g1+=(1+sky_resolution_x);
			else g0+=(1+sky_resolution_x);
			glBegin(GL_TRIANGLE_STRIP);
			for(int x2=0; x2<=sky_resolution_x; ++x2,g0++,g1++)
			{
				glColor3fv(g0->color);
				glVertex2fv(g0->pos_2d);
				glColor3fv(g1->color);
				glVertex2fv(g1->pos_2d);
			}
			glEnd();
		}
	}
}
