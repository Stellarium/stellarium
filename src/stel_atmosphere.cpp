/*
 * Stellarium
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

/*
	Class which compute and display the daylight sky color using openGL
	the sky is computed with the skylight class.
*/

#include "stellarium.h"
#include "stel_utility.h"
#include "stel_atmosphere.h"

stel_atmosphere::stel_atmosphere() : sky_resolution(64), tab_sky(NULL)
{
	// Create the vector array used to store the sky color on the full field of view
	tab_sky = new (Vec3f*)[sky_resolution+1];
	for (int k=0; k<sky_resolution+1 ;k++)
	{
		tab_sky[k] = new Vec3f[sky_resolution+1];
	}
}

stel_atmosphere::~stel_atmosphere()
{
	for (int k=0; k<sky_resolution+1 ;k++)
	{
		if (tab_sky[k]) delete tab_sky[k];
	}
	if (tab_sky) delete tab_sky;
}

void stel_atmosphere::compute_color(Vec3d sunPos, Vec3d moonPos, float moon_phase,
	tone_reproductor * eye, Projector* prj)
{
	//Vec3d obj;
	skylight_struct2 b2;

	sunPos.normalize();
	moonPos.normalize();

	float sun_pos[3];
	sun_pos[0] = sunPos[0];
	sun_pos[1] = sunPos[1];
	sun_pos[2] = sunPos[2];

	float moon_pos[3];
	moon_pos[0] = moonPos[0];
	moon_pos[1] = moonPos[1];
	moon_pos[2] = moonPos[2];

	sky.set_paramsv(sun_pos,5.f);
	skyb.set_sun_moon(moon_pos[2], sun_pos[2]);
	skyb.set_date(2003, 07, moon_phase);

	float stepX = (float)prj->viewW() / sky_resolution;
	float stepY = (float)prj->viewH() / sky_resolution;
	float viewport_left = (float)prj->view_left();
	float viewport_bottom = (float)prj->view_bottom();

	Vec3d point(1., 0., 0.);

	// Variables used to compute the average sky luminance
	double sum_lum = 0.;
	unsigned int nb_lum = 0;

	// Compute the sky color for every point above the ground
	for (int x=0; x<=sky_resolution; ++x)
	{
		for(int y=0; y<=sky_resolution; ++y)
		{
			prj->unproject_local((double)viewport_left+x*stepX, (double)viewport_bottom+y*stepY,point);
			point.normalize();

			if (point[2]<=0)
			{
				point[2] = -point[2];
				// The sky bellow the ground is the symetric of the one above :
				// it looks nice and gives proper values for brightness estimation
			}

			b2.pos[0] = point[0]; b2.pos[1] = point[1]; b2.pos[2] = point[2];
			sky.get_xyY_valuev(&b2);

			// Make a smooth transition between the skylight.cpp and the skybright.cpp 's models.
			float s = (1000.f - b2.color[2])/1000.f;
			if (s>0)
			{
				b2.color[2]*=1.f-s;
				b2.color[2]+=s * skyb.get_luminance(moon_pos[0]*b2.pos[0]+moon_pos[1]*b2.pos[1]+
					moon_pos[2]*b2.pos[2] - 0.00000001, sun_pos[0]*b2.pos[0]+sun_pos[1]*b2.pos[1]+
					sun_pos[2]*b2.pos[2] - 0.00000001, b2.pos[2]);
			}

			sum_lum+=b2.color[2];
			++nb_lum;
			eye->xyY_to_RGB(b2.color);
			tab_sky[x][y].set(b2.color[0],b2.color[1],b2.color[2]);
		}
	}

	// Update world adaptation luminance from the previous values
	if (sum_lum/nb_lum<1.5f) eye->set_world_adaptation_luminance(3.75f);
	else eye->set_world_adaptation_luminance(sum_lum/nb_lum*2.5);

	sum_lum = 0.f;
	nb_lum = 0;
}



// Draw the atmosphere using the precalc values stored in tab_sky
void stel_atmosphere::draw(Projector* prj)
{
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
	
	float stepX = (float)prj->viewW() / sky_resolution;
	float stepY = (float)prj->viewH() / sky_resolution;
	float viewport_left = (float)prj->view_left();
	float view_bottom = (float)prj->view_bottom();

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	prj->set_orthographic_projection();	// set 2D coordinate
	for (int y2=0; y2<sky_resolution; y2++)
	{
		glBegin(GL_TRIANGLE_STRIP);
			for(int x2=0; x2<sky_resolution+1; x2++)
			{
				glColor3f(tab_sky[x2][y2][0],tab_sky[x2][y2][1],tab_sky[x2][y2][2]);
				glVertex2i((int)(viewport_left+x2*stepX),(int)(view_bottom+y2*stepY));
				glColor3f(tab_sky[x2][y2+1][0],tab_sky[x2][y2+1][1],tab_sky[x2][y2+1][2]);
				glVertex2i((int)(viewport_left+x2*stepX),(int)((view_bottom+y2+1)*stepY));
			}
		glEnd();
	}
	prj->reset_perspective_projection();
}
