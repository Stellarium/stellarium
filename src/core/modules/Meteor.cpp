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
	: m_minDist(0.)
	, m_startH(0.)
	, m_endH(0.)
	, m_mag(1.)
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

	// determine number of intermediate segments (useful to curve along projection distortions)
	m_segments = 10;
	m_firstBrightSegment = (double)rand()/((double)RAND_MAX+1)*m_segments;
	// determine the meteor color
	m_colors.push_back(MeteorMgr::colorPair("white", 100));

	// building color arrays (line and prism)
	int totalOfSegments = 0;
	int currentSegment = 1+(double)rand()/((double)RAND_MAX+1)*(m_segments-1);
	for (int colorIndex=0; colorIndex<m_colors.size(); colorIndex++) {
		MeteorMgr::colorPair currentColor = m_colors[colorIndex];

		// segments which we'll paint with the current color
		int numOfSegments = m_segments*(currentColor.second / 100.f) + 0.4f; // +0.4 affect approximation
		if (colorIndex == m_colors.size()-1) {
			numOfSegments = m_segments - totalOfSegments;
		}

		totalOfSegments += numOfSegments;
		for (int i=0; i<numOfSegments; i++) {
			m_lineColorArray.insert(currentSegment, getColor(currentColor.first));
			m_trainColorArray.insert(currentSegment, getColor(currentColor.first));
			if (currentSegment >= m_segments-1) {
				currentSegment = 0;
			} else {
				currentSegment++;
			}
			m_trainColorArray.insert(currentSegment, getColor(currentColor.first));
		}
	}
}

Meteor::~Meteor()
{   
}

Vec4f Meteor::getColor(QString colorName) {
	int R, G, B; // 0-255
	if (colorName == "violet") { // Calcium
		R = 176;
		G = 67;
		B = 172;
	} else if (colorName == "blueGreen") { // Magnesium
		R = 0;
		G = 255;
		B = 152;
	} else if (colorName == "yellow") { // Iron
		R = 255;
		G = 255;
		B = 0;
	} else if (colorName == "orangeYellow") { // Sodium
		R = 255;
		G = 160;
		B = 0;
	} else if (colorName == "red") { // atmospheric nitrogen and oxygen
		R = 255;
		G = 30;
		B = 0;
	} else { // white
		R = 255;
		G = 255;
		B = 255;
	}

	return Vec4f(R/255.f, G/255.f, B/255.f, 1);
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

void Meteor::insertVertex(const StelCore* core, QVector<Vec3d> &vertexArray, Vec3d vertex) {
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
void Meteor::draw(const StelCore* core, StelPainter& sPainter)
{
	if (!m_alive)
	{
		return;
	}

	double maxFOV = core->getMovementMgr()->getMaxFov();
	double FOV = core->getMovementMgr()->getCurrentFov();
	double thickness = 2*log(FOV + 0.25)/(1.2*maxFOV - (FOV + 0.25)) + 0.01;
	if (FOV <= 0.5)
	{
		thickness = 0.013 * FOV; // decreasing faster
	}

	// train (triangular prism)
	//
	QVector<Vec3d> vertexArrayLine;
	QVector<Vec3d> vertexArrayL;
	QVector<Vec3d> vertexArrayR;
	QVector<Vec3d> vertexArrayTop;

	Vec3d posTrainB = m_posTrain;
	posTrainB[0] += thickness*0.7;
	posTrainB[1] += thickness*0.7;
	Vec3d posTrainL = m_posTrain;
	posTrainL[1] -= thickness;
	Vec3d posTrainR = m_posTrain;
	posTrainR[0] -= thickness;

	for (int i=0; i<m_segments; i++) {
		double mag = m_mag * i/(3* (m_segments-1));
		if (i > m_firstBrightSegment) {
			mag *= 12/5;
		}

		double height = m_posTrain[2] + i*(m_position[2] - m_posTrain[2])/(m_segments-1);
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

		m_lineColorArray[i][3] = mag;
		m_trainColorArray[i*2][3] = mag;
		m_trainColorArray[i*2+1][3] = mag;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.enableClientStates(true, false, true);
	sPainter.setColorPointer(4, GL_FLOAT, m_trainColorArray.toVector().constData());
	if (thickness) {
		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayL.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayL.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayR.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayR.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayTop.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayTop.size(), 0, true);
	}
	sPainter.setColorPointer(4, GL_FLOAT, m_lineColorArray.toVector().constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayLine.constData());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexArrayLine.size(), 0, true);

	glDisable(GL_BLEND);
	sPainter.enableClientStates(false);
}

bool Meteor::isAlive(void)
{
	return m_alive;
}
