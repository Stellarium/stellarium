/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
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

// This is an ad hoc meteor model
// Could use a simple ablation physics model in the future

/*
NOTE: Here the radiant is always along the ecliptic at the apex of the Earth's way.
In reality, individual meteor streams have varying velocity vectors and therefore radiants
which are generally not at the apex of the Earth's way, such as the Perseids shower.
*/

// Improved realism and efficiency 2004-12

#include <cstdlib>
#include "Meteor.hpp"
#include "StelCore.hpp"

#include "StelToneReproducer.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"

Meteor::Meteor(const StelCore* core, double v)
	: m_mag(1.)
	, m_startH(0.)
	, m_endH(0.)
	, m_minDist(0.)
	, m_distMultiplier(0.)
{
	m_speed = 11+(double)rand()/((double)RAND_MAX+1)*(v-11);  // abs range 11-72 km/s by default (see line 427 in StelApp.cpp)

	// the meteor starts dead, after we'll calculate the position
	// if the position is within the bounds, this parameter will be changed to TRUE
	m_alive = false;

	double high_range = EARTH_RADIUS + HIGH_ALTITUDE;
	double low_range = EARTH_RADIUS + LOW_ALTITUDE;

	// view matrix of sporadic meteors model
	double alpha = (double)rand()/((double)RAND_MAX+1)*2*M_PI;
	double delta = M_PI_2 - (double)rand()/((double)RAND_MAX+1)*M_PI;
	m_viewMatrix = Mat4d::zrotation(alpha) * Mat4d::yrotation(delta);

	// find observer position in meteor coordinate system
	m_obs = core->altAzToEquinoxEqu(Vec3d(0,0,EARTH_RADIUS));
	m_obs.transfo4d(m_viewMatrix.transpose());

	// select random trajectory using polar coordinates in XY plane, centered on observer
	m_xydistance = (double)rand() / ((double)RAND_MAX+1)*(VISIBLE_RADIUS);
	double angle = (double)rand() / ((double)RAND_MAX+1)*2*M_PI;

	// set meteor start x,y
	m_position[0] = m_posTrain[0] = m_xydistance*cos(angle) + m_obs[0];
	m_position[1] = m_posTrain[1] = m_xydistance*sin(angle) + m_obs[1];

	// D is distance from center of earth
	double D = sqrt(m_position[0]*m_position[0] + m_position[1]*m_position[1]);

	if (D > high_range)     // won't be visible, meteor still dead
	{
		return;
	}

	m_posTrain[2] = m_position[2] = m_startH = sqrt(high_range*high_range - D*D);

	// determine end of burn point, and nearest point to observer for distance mag calculation
	// mag should be max at nearest point still burning
	if (D > low_range)
	{
		m_endH = -m_startH;  // earth grazing
		m_minDist = m_xydistance;
	}
	else
	{
		m_endH = sqrt(low_range*low_range - D*D);
		m_minDist = sqrt(m_xydistance*m_xydistance + pow(m_endH - m_obs[2], 2));
	}

	if (m_minDist > VISIBLE_RADIUS)
	{
		// on average, not visible (although if were zoomed ...)
		return; //meteor still dead
	}

	// If everything is ok until here,
	m_alive = true;  //the meteor is alive

	// determine intensity [-3; 4.5]
	float Mag1 = (double)rand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag2 = (double)rand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag = (Mag1 + Mag2)/2.0f;

	// compute RMag and CMag
	RCMag rcMag;
	core->getSkyDrawer()->computeRCMag(Mag, &rcMag);
	m_mag = rcMag.luminance;

	// most visible meteors are under about 180km distant
	// scale max mag down if outside this range
	float scale = 1;
	if (m_minDist)
	{
		scale = 180*180 / (m_minDist*m_minDist);
	}
	if (scale < 1)
	{
		m_mag *= scale;
	}
}

Meteor::~Meteor()
{   
}

// returns true if alive
bool Meteor::update(double deltaTime)
{
	if (!m_alive)
	{
		return false;
	}

	if (m_position[2] < m_endH)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out
		m_mag -= deltaTime/500.0f;
		if(m_mag < 0)
		{
			m_alive = false;    // no longer visible
		}
	}

	// *** would need time direction multiplier to allow reverse time replay
	m_position[2] -= m_speed*deltaTime/1000.0f;

	// train doesn't extend beyond start of burn
	if (m_position[2] + m_speed*0.5f > m_startH)
	{
		m_posTrain[2] = m_startH;
	}
	else
	{
		m_posTrain[2] -= m_speed*deltaTime/1000.0f;
	}

	// determine visual magnitude based on distance to observer
	double dist = sqrt(m_xydistance*m_xydistance + pow(m_position[2]-m_obs[2], 2));
	if (dist == 0)
	{
		dist = .01;    // just to be cautious (meteor hits observer!)
	}
	m_distMultiplier = m_minDist*m_minDist / (dist*dist);

	return m_alive;
}

// returns true if visible
// Assumes that we are in local frame
void Meteor::draw(const StelCore* core, StelPainter& sPainter)
{
	if (!m_alive)
	{
		return;
	}

	Vec3d spos = m_position;
	Vec3d epos = m_posTrain;

	// convert to equ
	spos.transfo4d(m_viewMatrix);
	epos.transfo4d(m_viewMatrix);

	// convert to local and correct for earth radius [since equ and local coordinates in stellarium use same 0 point!] 
	spos = core->equinoxEquToAltAz(spos);
	epos = core->equinoxEquToAltAz(epos);
	spos[2] -= EARTH_RADIUS;
	epos[2] -= EARTH_RADIUS;
	// 1216 is to scale down under 1 for desktop version
	spos/=1216;
	epos/=1216;

	// connect this point with last drawn point
	double tmag = m_mag*m_distMultiplier;

	// compute an intermediate point so can curve slightly along projection distortions
	Vec3d posi = m_posTrain;
	posi[2] = m_posTrain[2] + (m_position[2] - m_posTrain[2])/2;
	posi.transfo4d(m_viewMatrix);
	posi = core->equinoxEquToAltAz(posi);
	posi[2] -= EARTH_RADIUS;
	posi/=1216;

	// draw dark to light
	Vec4f colorArray[3];
	colorArray[0].set(0,0,0,0);
	colorArray[1].set(1,1,1,tmag*0.5);
	colorArray[2].set(1,1,1,tmag);
	Vec3d vertexArray[3];
	vertexArray[0]=epos;
	vertexArray[1]=posi;
	vertexArray[2]=spos;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.enableClientStates(true, false, true);
	sPainter.setColorPointer(4, GL_FLOAT, colorArray);
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArray);
	sPainter.drawFromArray(StelPainter::LineStrip, 3, 0, true);
	sPainter.enableClientStates(false);
}

bool Meteor::isAlive(void)
{
	return m_alive;
}
