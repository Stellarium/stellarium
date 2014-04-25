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
			   double velocity,
			   double radiantAlpha,
			   double radiantDelta,
			   float pidx)
	: speed(velocity)
	, pidx(pidx)
	, minDist(0.)
	, distMultiplier(0.)
	, startH(0.)
	, endH(0.)
	, mag(1.f)
{
	// the meteor starts dead, after we'll calculate the position
	// if the position is within the bounds, this parameter will be changed to TRUE
	alive = false;

	double high_range = EARTH_RADIUS+HIGH_ALTITUDE;
	double low_range = EARTH_RADIUS+LOW_ALTITUDE;

	// view matrix of meteor model
	viewMatrix = Mat4d::zrotation(radiantAlpha) * Mat4d::yrotation(M_PI_2 - radiantDelta);

	// find observer position in meteor coordinate system
	obs = core->altAzToJ2000(Vec3d(0,0,EARTH_RADIUS));
	obs.transfo4d(viewMatrix.transpose());

	// select random trajectory using polar coordinates in XY plane, centered on observer
	xydistance = (double)rand() / ((double)RAND_MAX+1)*(VISIBLE_RADIUS);
	double angle = (double)rand() / ((double)RAND_MAX+1)*2*M_PI;

	// set meteor start x,y
	position[0] = posInternal[0] = posTrain[0] = xydistance*cos(angle) + obs[0];
	position[1] = posInternal[1] = posTrain[1] = xydistance*sin(angle) + obs[1];

	// D is distance from center of earth
	double D = sqrt(position[0]*position[0] + position[1]*position[1]);

	if (D > high_range)     // won't be visible, meteor still dead
	{
		return;
	}

	posTrain[2] = position[2] = startH = sqrt(high_range*high_range - D*D);

	// determine end of burn point, and nearest point to observer for distance mag calculation
	// mag should be max at nearest point still burning
	endH = -startH;  // earth grazing
	minDist = xydistance;
	if (D <= low_range)
	{
		endH = sqrt(low_range*low_range - D*D);
		minDist = sqrt(xydistance*xydistance + pow(endH - obs[2], 2));
	}

	if (minDist > VISIBLE_RADIUS)
	{
		// on average, not visible (although if were zoomed ...)
		return; //meteor still dead
	}

	// If everything is ok until here,
	alive = true;  //the meteor is alive

	// determine intensity [-3; 4.5]
	float Mag1 = (double)rand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag2 = (double)rand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag = (Mag1 + Mag2)/2.0f;

	// compute RMag and CMag
	RCMag rcMag;
	core->getSkyDrawer()->computeRCMag(Mag, &rcMag);
	mag = rcMag.luminance;

	// most visible meteors are under about 180km distant
	// scale max mag down if outside this range
	float scale = 1;
	if (minDist!=0)
	{
		scale = 180*180 / (minDist*minDist);
	}
	if (scale < 1)
	{
		mag *= scale;
	}

	// implements the population index
	float oneMag = -0.2; // negative, working in different scale ( 0 to 1 - where 1 is brighter)
	if (rand()%100 < 100.f/pidx) // probability
	{
		mag += oneMag;  // (m+1)
	}
}

MeteorStream::~MeteorStream()
{
}

// returns true if alive
bool MeteorStream::update(double deltaTime)
{
	if (!alive)
	{
		return false;
	}

	if (position[2] < endH)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out
		mag -= deltaTime/1000.0f;
		if(mag < 0)
		{
			alive = false;    // no longer visible
		}
	}

	// *** would need time direction multiplier to allow reverse time replay
	position[2] -= speed*deltaTime/1000.0f;

	// train doesn't extend beyond start of burn
	if (position[2] + speed*0.5f > startH)
	{
		posTrain[2] = startH ;
	}
	else
	{
		posTrain[2] -= speed*deltaTime/1000.0f;
	}

	return alive;
}


// returns true if visible
// Assumes that we are in local frame
void MeteorStream::draw(const StelCore* core, StelPainter& sPainter)
{
	if (!alive)
	{
		return;
	}

	Vec3d spos = position;
	Vec3d epos = posTrain;

	// convert to equ
	spos.transfo4d(viewMatrix);
	epos.transfo4d(viewMatrix);

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
		Vec3d posi = posInternal;
		posi[2] = posTrain[2] + i*(position[2] - posTrain[2])/segments;
		posi.transfo4d(viewMatrix);
		posi = core->j2000ToAltAz(posi);
		posi[2] -= EARTH_RADIUS;
		posi/=1216;

		colorArray.push_back(Vec4f(1,1,1,i*mag/segments));
		vertexArray.push_back(posi);
	}
	// first point - light
	colorArray.push_back(Vec4f(1,1,1,mag));
	vertexArray.push_back(spos);

	sPainter.setColorPointer(4, GL_FLOAT, colorArray.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArray.constData());
	sPainter.enableClientStates(true, false, true);
	sPainter.drawFromArray(StelPainter::LineStrip, vertexArray.size(), 0, true);
	sPainter.enableClientStates(false);
}
