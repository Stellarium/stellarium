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

stel_atmosphere::stel_atmosphere() : sky_resolution(32), tab_sky(NULL)
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
}

void stel_atmosphere::compute_color(int ground_ON, Vec3d sunPos, tone_reproductor * eye, draw_utility * du)
{
	static    GLdouble M[16];
	static    GLdouble P[16];
	static    GLdouble objx[1];
	static    GLdouble objy[1];
	static    GLdouble objz[1];
	static    GLint V[4];

	skylight_struct b;

	sunPos.normalize();

	sky.set_params(M_PI_2-asinf(sunPos[2]),5.f);

	// Convert x,y screen pos in 3D vector
	glGetDoublev(GL_MODELVIEW_MATRIX,M);
	glGetDoublev(GL_PROJECTION_MATRIX,P);
	glGetIntegerv(GL_VIEWPORT,V);

	float stepX = (float)du->screenW / sky_resolution;
	float stepY = (float)du->screenH / sky_resolution;

	Vec3d point;

	// Find which row is the first one not bellow the ground
	gluProject(1.,0.,0.,M,P,V,objx,objy,objz);
	double yHoriz = *objy;
	startY = (int)floor(yHoriz/stepY);

	// The sky is totally covered by the ground
	if (startY>sky_resolution)
	{
		for (int x=0; x<=sky_resolution; x++)
		{
			for(int y=0; y<=sky_resolution; y++)
			{
				tab_sky[x][y].set(0.f,0.f,-10.f);
			}
		}
		return;
	}

	// If the horizon limit is visible on the screen
	if (startY>=0)
	{
		// Init the values bellow the ground
		for (int x=0; x<=sky_resolution; x++)
		{
			for(int y=0; y<startY; y++)
			{
				tab_sky[x][y].set(0.f,0.f,-10.f);
			}
		}

		// Set the values of the first line bellow the ground
		for (int x=0; x<=sky_resolution; x++)
		{
			gluUnProject(x*stepX,yHoriz,1,M,P,V,objx,objy,objz);
			point.set(*objx,*objy,*objz);
			point.normalize();
			b.zenith_angle = M_PI_2-0.1;
			b.dist_sun = acosf(point.dot(sunPos));
			if (b.dist_sun<0) b.dist_sun=-b.dist_sun;
			sky.get_xyY_value(&b);
			eye->xyY_to_RGB(b.color);
			tab_sky[x][startY].set(b.color[0],b.color[1],b.color[2]);
		}
	}
	else
	{
		startY=-1;
	}

	// Compute the sky color for every point above the ground
	for (int x=0; x<=sky_resolution; x++)
	{
		for(int y=startY+1; y<=sky_resolution; y++)
		{
			gluUnProject(x*stepX,y*stepY,1,M,P,V,objx,objy,objz);
			point.set(*objx,*objy,*objz);
			point.normalize();
			b.zenith_angle = M_PI_2-asinf(point[2]);
			b.dist_sun = acosf(point.dot(sunPos));
			if (b.dist_sun<0) b.dist_sun=-b.dist_sun;
			sky.get_xyY_value(&b);
			eye->xyY_to_RGB(b.color);
			tab_sky[x][y].set(b.color[0],b.color[1],b.color[2]);
		}
	}
}



// Draw the atmosphere using the precalc values stored in tab_sky
void stel_atmosphere::draw(draw_utility * du)
{
	int startYtemp = startY;
	if (startYtemp>sky_resolution) return;
	if (startYtemp-1<0) startYtemp = 1;
	float stepX = (float)du->screenW / sky_resolution;
	float stepY = (float)du->screenH / sky_resolution;
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	du->set_orthographic_projection();	// set 2D coordinate
	for (int y2=startYtemp-1; y2<sky_resolution; y2++)
	{
		if (tab_sky[0][y2+1][2]==-10.f) continue;	// Don't draw if value == -10 i.e. under ground
		glBegin(GL_TRIANGLE_STRIP);
			for(int x2=0; x2<sky_resolution+1; x2++)
			{
				glColor3f(tab_sky[x2][y2][0],tab_sky[x2][y2][1],tab_sky[x2][y2][2]);
				glVertex2i((int)(x2*stepX),(int)(y2*stepY));
				glColor3f(tab_sky[x2][y2+1][0],tab_sky[x2][y2+1][1],tab_sky[x2][y2+1][2]);
				glVertex2i((int)(x2*stepX),(int)((y2+1)*stepY));
			}
		glEnd();
	}
	du->reset_perspective_projection();
}
