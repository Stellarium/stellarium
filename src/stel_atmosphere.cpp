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

	int limY;

	// Don't calc if under the ground
	if (ground_ON)
	{
		gluProject(1,0,0,M,P,V,objx,objy,objz);
		limY = (int)(sky_resolution-(float)(*objy)/stepY+3.);
		if (!(limY<sky_resolution+1)) limY = sky_resolution+1;
	}
	else
	{
		limY = sky_resolution+1;
	}

	Vec3d point;
	for (int x=0; x<sky_resolution+1; x++)
	{
		for(int y=0; y<limY; y++)
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
	// TODO : optimisation not to draw behind the ground
	float stepX = (float)du->screenW / sky_resolution;
	float stepY = (float)du->screenH / sky_resolution;
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	du->set_orthographic_projection();	// set 2D coordinate
	for (int y2=0; y2<sky_resolution-1+1; y2++)
	{
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
