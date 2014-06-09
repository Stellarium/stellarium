/*
 * Copyright (C) 2013 Marcos Cardinot
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

#include <cstdlib>

#include "MeteorStream.hpp"
#include "StelCore.hpp"
#include "StelToneReproducer.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"

MeteorStream::MeteorStream(const StelCore* core,
			   double speed,
			   double radiantAlpha,
			   double radiantDelta,
			   float pidx)
	: m_speed(speed)
	, m_pidx(pidx)
	, m_minDist(0.)
	, m_distMultiplier(0.)
	, m_startH(0.)
	, m_endH(0.)
	, m_mag(1.f)
{

	// if speed is zero, use a random value
	if (!speed)
	{
		m_speed = 11+(double)rand()/((double)RAND_MAX+1)*61;  // abs range 11-72 km/s
	}

	// the meteor starts dead, after we'll calculate the position
	// if the position is within the bounds, this parameter will be changed to TRUE
	m_alive = false;

	double high_range = EARTH_RADIUS+HIGH_ALTITUDE;
	double low_range = EARTH_RADIUS+LOW_ALTITUDE;

	// view matrix of meteor model
	m_viewMatrix = Mat4d::zrotation(radiantAlpha) * Mat4d::yrotation(M_PI_2 - radiantDelta);

	// find observer position in meteor coordinate system
	m_obs = core->altAzToJ2000(Vec3d(0,0,EARTH_RADIUS));
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
	m_endH = -m_startH;  // earth grazing
	m_minDist = m_xydistance;
	if (D <= low_range)
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
	if (m_minDist!=0)
	{
		scale = 180*180 / (m_minDist*m_minDist);
	}
	if (scale < 1)
	{
		m_mag *= scale;
	}

	// implements the population index
	float oneMag = -0.2; // negative, working in different scale ( 0 to 1 - where 1 is brighter)
	if (m_pidx) // is not zero
	{
		if (rand()%100 < 100.f/pidx) // probability
		{
			m_mag += oneMag;  // (m+1)
		}
	}
}

MeteorStream::~MeteorStream()
{
}

// returns true if alive
bool MeteorStream::update(double deltaTime)
{
	if (!m_alive)
	{
		return false;
	}

	if (m_position[2] < m_endH)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out
		m_mag -= deltaTime/1000.0f;
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

	return m_alive;
}


// returns true if visible
// Assumes that we are in local frame
void MeteorStream::draw(const StelCore* core, StelPainter& sPainter)
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

	// convert to local and correct for earth radius
	//[since equ and local coordinates in stellarium use same 0 point!]
	spos = core->j2000ToAltAz(spos);
	epos = core->j2000ToAltAz(epos);
	spos[2] -= EARTH_RADIUS;
	epos[2] -= EARTH_RADIUS;
	// 1216 is to scale down under 1 for desktop version
	spos/=1216;
	epos/=1216;

	QVector<Vec4f> colorArray;
	QVector<Vec3d> vertexArray;
	// last point - dark
	colorArray.push_back(Vec4f(0,0,0,0));
	vertexArray.push_back(epos);
	// compute intermediate points to curve along projection distortions
	int segments = 10;
	for (int i=1; i<segments; i++) {
		Vec3d posi = m_posTrain;
		posi[2] = m_posTrain[2] + i*(m_position[2] - m_posTrain[2])/segments;
		posi.transfo4d(m_viewMatrix);
		posi = core->j2000ToAltAz(posi);
		posi[2] -= EARTH_RADIUS;
		posi/=1216;

		colorArray.push_back(Vec4f(1,1,1,i*m_mag/segments));
		vertexArray.push_back(posi);
	}
	// first point - light
	colorArray.push_back(Vec4f(1,1,1,m_mag));
	vertexArray.push_back(spos);

	sPainter.setColorPointer(4, GL_FLOAT, colorArray.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArray.constData());
	sPainter.enableClientStates(true, false, true);
	sPainter.drawFromArray(StelPainter::LineStrip, vertexArray.size(), 0, true);
	sPainter.enableClientStates(false);
}
