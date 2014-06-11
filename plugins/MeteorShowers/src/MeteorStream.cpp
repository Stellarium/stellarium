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
#include "StelTexture.hpp"

StelTextureSP MeteorStream::bolideTexture;

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

	// determine number of intermediate segments (useful to curve along projection distortions)
	m_segments = 10;
	m_firstBrightSegment = (double)rand()/((double)RAND_MAX+1)*m_segments;
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
		double dt = 820+(double)rand()/((double)RAND_MAX+1)*185; // range 820-1005
		m_posTrain[2] -= m_speed*deltaTime/dt;
	}

	return m_alive;
}

void MeteorStream::insertVertex(const StelCore* core, QVector<Vec3d> &vertexArray, Vec3d vertex) {
	// convert to equ
	vertex.transfo4d(m_viewMatrix);
	// convert to local and correct for earth radius
	//[since equ and local coordinates in stellarium use same 0 point!]
	vertex = core->j2000ToAltAz(vertex);
	vertex[2] -= EARTH_RADIUS;
	// 1216 is to scale down under 1 for desktop version
	vertex/=1216;

	vertexArray.push_back(vertex);
}

// returns true if visible
// Assumes that we are in local frame
void MeteorStream::draw(const StelCore* core, StelPainter& sPainter)
{
	if (!m_alive)
	{
		return;
	}

	double maxFOV = core->getMovementMgr()->getMaxFov();
	double FOV = core->getMovementMgr()->getCurrentFov();
	double thickness = log(FOV)/(1.2*maxFOV - FOV) + 0.01;
	double bolideSize = thickness*3;

	// train
	//
	QVector<Vec4f> colorArrayLine;
	QVector<Vec3d> vertexArrayLine;
	// triangular prism
	QVector<Vec4f> colorArray;
	QVector<Vec3d> vertexArrayL;
	QVector<Vec3d> vertexArrayR;
	QVector<Vec3d> vertexArrayTop;

	// last segment - dark
	insertVertex(core, vertexArrayLine, m_posTrain);
	Vec3d posTrainB = m_posTrain;
	posTrainB[0] += thickness*0.7;
	posTrainB[1] += thickness*0.7;

	Vec3d posTrainL = m_posTrain;
	posTrainL[1] -= thickness;
	insertVertex(core, vertexArrayL, posTrainB);
	insertVertex(core, vertexArrayL, posTrainL);

	Vec3d posTrainR = m_posTrain;
	posTrainR[0] -= thickness;
	insertVertex(core, vertexArrayR, posTrainB);
	insertVertex(core, vertexArrayR, posTrainR);

	insertVertex(core, vertexArrayTop, posTrainL);
	insertVertex(core, vertexArrayTop, posTrainR);

	Vec4f lastColor = Vec4f(1,1,1,0);
	colorArray.push_back(lastColor);
	colorArray.push_back(lastColor);
	colorArrayLine.push_back(lastColor);

	// compute intermediate segments
	for (int i=1; i<=m_segments; i++) {
		double mag = m_mag * i/(3*m_segments);
		if (i > m_firstBrightSegment) {
			mag *= 12/5;
		}

		double height = m_posTrain[2] + i*(m_position[2] - m_posTrain[2])/m_segments;
		Vec3d posi;

		posi = m_posTrain;
		posi[2] = height;
		insertVertex(core, vertexArrayLine, posi);

		posi = posTrainB;
		posi[2] = height;
		insertVertex(core, vertexArrayL, posi);
		insertVertex(core, vertexArrayR, posi);

		posi = posTrainL;
		posi[2] = height;
		insertVertex(core, vertexArrayL, posi);
		insertVertex(core, vertexArrayTop, posi);

		posi = posTrainR;
		posi[2] = height;
		insertVertex(core, vertexArrayR, posi);
		insertVertex(core, vertexArrayTop, posi);

		Vec4f internalColor = Vec4f(1,1,1,mag);
		colorArray.push_back(internalColor);
		colorArray.push_back(internalColor);
		colorArrayLine.push_back(internalColor);
	}

	// drawing all segments
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.enableClientStates(true, false, true);
	sPainter.setColorPointer(4, GL_FLOAT, colorArray.constData());
	if (thickness) {
		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayL.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayL.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayR.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayR.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayTop.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayTop.size(), 0, true);
	}
	sPainter.setColorPointer(4, GL_FLOAT, colorArrayLine.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayLine.constData());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexArrayLine.size(), 0, true);

	// bolide
	//
	QVector<Vec3d> vertexArrayBolide;
	QVector<Vec4f> colorArrayBolide;
	Vec4f bolideColor = Vec4f(1,1,1,m_mag);

	Vec3d topLeft = m_position;
	topLeft[1] -= bolideSize;
	insertVertex(core, vertexArrayBolide, topLeft);
	colorArrayBolide.push_back(bolideColor);

	Vec3d topRight = m_position;
	topRight[0] -= bolideSize;
	insertVertex(core, vertexArrayBolide, topRight);
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomRight = m_position;
	bottomRight[1] += bolideSize;
	insertVertex(core, vertexArrayBolide, bottomRight);
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomLeft = m_position;
	bottomLeft[0] += bolideSize;
	insertVertex(core, vertexArrayBolide, bottomLeft);
	colorArrayBolide.push_back(bolideColor);

	glBlendFunc(GL_ONE, GL_ONE);

	sPainter.enableClientStates(true, true, true);
	MeteorStream::bolideTexture->bind();

	static const float texCoordData[] = {1.,0., 0.,0., 0.,1., 1.,1.};
	sPainter.setTexCoordPointer(2, GL_FLOAT, texCoordData);
	sPainter.setColorPointer(4, GL_FLOAT, colorArrayBolide.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayBolide.constData());
	sPainter.drawFromArray(StelPainter::TriangleFan, vertexArrayBolide.size(), 0, true);

	glDisable(GL_BLEND);
	sPainter.enableClientStates(false);
}
