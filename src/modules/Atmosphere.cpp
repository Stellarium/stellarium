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

#include <QDebug>

#include "GLee.h"
#include "Atmosphere.hpp"
#include "StelUtils.hpp"
#include "Projector.hpp"
#include "ToneReproducer.hpp"
#include "StelCore.hpp"

// Uncomment to try out vertex buffers
//#define USE_VERTEX_BUFFERS 1

Atmosphere::Atmosphere(void) :viewport(0,0,0,0),sky_resolution_y(44), posGrid(NULL), colorGrid(NULL), indices(NULL),
            averageLuminance(0.f), eclipseFactor(1.), lightPollutionLuminance(0)
{
	setFadeDuration(3.f);
}

Atmosphere::~Atmosphere(void)
{
	if (posGrid)
	{
		delete[] posGrid;
		posGrid = NULL;
	}
	if (colorGrid)
	{
		delete[] colorGrid;
		colorGrid = NULL;
	}
	if (indices)
	{
		delete[] indices;
		indices = NULL;
	}
}

void Atmosphere::compute_color(double JD, Vec3d sunPos, Vec3d moonPos, float moon_phase,
                               ToneReproducer * eye, Projector* prj,
                               float latitude, float altitude, float temperature, float relative_humidity)
{
	if (viewport != prj->getViewport())
	{
		// The viewport changed: update the number of point of the grid
		viewport = prj->getViewport();
		if (posGrid)
			delete[] posGrid;
		if (colorGrid)
			delete[] colorGrid;
		if (indices)
			delete[] indices;
		sky_resolution_x = (int)floor(0.5+sky_resolution_y*(0.5*sqrt(3.0))*prj->getViewportWidth()/prj->getViewportHeight());
		posGrid = new Vec2f[(1+sky_resolution_x)*(1+sky_resolution_y)];
		colorGrid = new Vec3f[(1+sky_resolution_x)*(1+sky_resolution_y)];
		float stepX = (float)prj->getViewportWidth() / (sky_resolution_x-0.5);
		float stepY = (float)prj->getViewportHeight() / sky_resolution_y;
		float viewport_left = (float)prj->getViewportPosX();
		float viewport_bottom = (float)prj->getViewportPosY();
		for (int x=0; x<=sky_resolution_x; ++x)
		{
			for(int y=0; y<=sky_resolution_y; ++y)
			{
				Vec2f &v(posGrid[y*(1+sky_resolution_x)+x]);
				v[0] = viewport_left + ((x == 0) ? 0.f : 
						(x == sky_resolution_x) ? (float)prj->getViewportWidth() : (x-0.5*(y&1))*stepX);
				v[1] = viewport_bottom+y*stepY;
			}
		}
		
		// Generate the indices used to draw the quads
		indices = new GLushort[sky_resolution_x*sky_resolution_y*4];
		int i=0;
		for (int y2=0; y2<sky_resolution_y; ++y2)
		{
			GLushort g0 = y2*(1+sky_resolution_x);
			GLushort g1 = g0;
			if (y2&1)
			{
				g1+=(1+sky_resolution_x);
			}
			else
			{
				g0+=(1+sky_resolution_x);
			}
			
			for (int x2=0; x2<sky_resolution_x; ++x2)
			{
				indices[i++]=g0;
				indices[i++]=g1;
				indices[i++]=++g1;
				indices[i++]=++g0;
			}
		}
		
#ifdef USE_VERTEX_BUFFERS
		// Load the data on the GPU using vertex buffers
		glGenBuffersARB(1, &vertexBufferId);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBufferId);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, (1+sky_resolution_x)*(1+sky_resolution_y)*2*sizeof(float), posGrid, GL_STATIC_DRAW_ARB);
		glGenBuffersARB(1, &indicesBufferId);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indicesBufferId);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sky_resolution_x*sky_resolution_y*4*sizeof(GLushort), indices, GL_STATIC_DRAW_ARB);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
#endif
		
	}

	// Update the eclipse intensity factor to apply on atmosphere model
	// these are for radii
	const double sun_angular_size = atan(696000./AU/sunPos.length());
	const double moon_angular_size = atan(1738./AU/moonPos.length());
	const double touch_angle = sun_angular_size + moon_angular_size;

	// determine luminance falloff during solar eclipses
	sunPos.normalize();
	moonPos.normalize();
	double separation_angle = std::acos(sunPos.dot(moonPos));  // angle between them
	// qDebug("touch at %f\tnow at %f (%f)\n", touch_angle, separation_angle, separation_angle/touch_angle);
	// bright stars should be visible at total eclipse
	// TODO: correct for atmospheric diffusion
	// TODO: use better coverage function (non-linear)
	// because of above issues, this algorithm darkens more quickly than reality
	if( separation_angle < touch_angle)
	{
		double dark_angle = moon_angular_size - sun_angular_size;
		float min;
		if(dark_angle < 0)
		{
			// annular eclipse
			float asun = sun_angular_size*sun_angular_size;
			min = (asun - moon_angular_size*moon_angular_size)/asun;  // minimum proportion of sun uncovered
			dark_angle *= -1;
		}
		else min = 0.0001;  // so bright stars show up at total eclipse

		if (separation_angle < dark_angle)
			eclipseFactor = min;
		else
			eclipseFactor = min + (1.-min)*(separation_angle-dark_angle)/(touch_angle-dark_angle);
	}
	else
		eclipseFactor = 1.;
		

	// No need to calculate if not visible
	if (!fader.getInterstate())
	{
		averageLuminance = 0.001 + lightPollutionLuminance;
		return;
	}

	// Calculate the atmosphere RGB for each point of the grid
	
	float sun_pos[3];
	sun_pos[0] = sunPos[0];
	sun_pos[1] = sunPos[1];
	sun_pos[2] = sunPos[2];

	float moon_pos[3];
	moon_pos[0] = moonPos[0];
	moon_pos[1] = moonPos[1];
	moon_pos[2] = moonPos[2];

	sky.set_paramsv(sun_pos, 5.f);

	skyb.setLocation(latitude * M_PI/180., altitude, temperature, relative_humidity);
	skyb.setSunMoon(moon_pos[2], sun_pos[2]);

	// Calculate the date from the julian day.
	int year, month, day;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	skyb.setDate(year, month, moon_phase);

	// Variables used to compute the average sky luminance
	double sum_lum = 0.;
	unsigned int nb_lum = 0;
	
	Vec3d point(1., 0., 0.);
	skylight_struct2 b2;
	float lumi;
	
	prj->setCurrentFrame(Projector::FrameLocal);
	
	// Compute the sky color for every point above the ground
	for (int i=0; i<(1+sky_resolution_x)*(1+sky_resolution_y); ++i)
	{
		Vec2f &v(posGrid[i]);
		prj->unProject(v[0],v[1],point);
		
		assert(fabs(point.lengthSquared()-1.0) < 1e-10);
		
		if (point[2]<=0)
		{
			point[2] = -point[2];
			// The sky below the ground is the symetric of the one above :
			// it looks nice and gives proper values for brightness estimation
		}
		
		// Use the Skybright.cpp 's models for brightness which gives better results.
		lumi = skyb.getLuminance(moon_pos[0]*point[0]+moon_pos[1]*point[1]+
				moon_pos[2]*point[2], sun_pos[0]*point[0]+sun_pos[1]*point[1]+
				sun_pos[2]*point[2], point[2]);
		lumi *= eclipseFactor;
		// Add star background luminance
		lumi += 0.0001;
		// Multiply by the input scale of the ToneConverter (is not done automatically by the xyYtoRGB method called later)
		//lumi*=eye->getInputScale();
		
		// Add the light pollution luminance AFTER the scaling to avoid scaling it because it is the cause
		// of the scaling itself
		lumi += lightPollutionLuminance;
		
		// Store for later statistics
		sum_lum+=lumi;
		++nb_lum;
		
		if (lumi>0.01)
		{
			b2.pos[0] = point[0];
			b2.pos[1] = point[1];
			b2.pos[2] = point[2];
			// Use the Skylight model for the color
			sky.get_xyY_valuev(b2);
		}
		else
		{
			// Too dark to see atmosphere color, don't bother computing it
			b2.color[0]=0.;
			b2.color[1]=0.;
		}
		b2.color[2] = lumi;
		
		colorGrid[i].set(b2.color[0], b2.color[1], b2.color[2]);
	}
			
	// Update average luminance
	averageLuminance = sum_lum/nb_lum;
}



// Draw the atmosphere using the precalc values stored in tab_sky
void Atmosphere::draw(StelCore* core)
{
	ToneReproducer* eye = core->getToneReproducer();
	
	if (fader.getInterstate())
	{
		const float atm_intensity = fader.getInterstate();
		
		glBlendFunc(GL_ONE, GL_ONE);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glShadeModel(GL_SMOOTH);
		
		// Adapt luminance at this point to avoid a mismatch with the adaption value
		for (int i=0;i<(1+sky_resolution_x)*(1+sky_resolution_y);++i)
		{
			Vec3f& c = colorGrid[i];
			eye->xyYToRGB(c);
			c*=atm_intensity;
		}
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		
		// Load the color components
		glColorPointer(3, GL_FLOAT, 0, colorGrid);
		
#ifdef USE_VERTEX_BUFFERS
		// Bind the vertex and indices buffer
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vertexBufferId);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indicesBufferId);
		glVertexPointer(2, GL_FLOAT, 0, 0);
		// And draw everything at once
		glDrawElements(GL_QUADS, sky_resolution_x*sky_resolution_y*4, GL_UNSIGNED_SHORT, 0);
		// Unbind buffers
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
#else
		// Load the vertex array
		glVertexPointer(2, GL_FLOAT, 0, posGrid);
		// And draw everything at once
		glDrawElements(GL_QUADS, sky_resolution_x*sky_resolution_y*4, GL_UNSIGNED_SHORT, indices);
#endif
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		
		glShadeModel(GL_FLAT);
	}
}
