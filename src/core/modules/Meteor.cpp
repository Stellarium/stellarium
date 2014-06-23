/*
 * Stellarium
 * Copyright (C) 2004 Robert Spearman
 * Copyright (C) 2014 Marcos Cardinot
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

#include "Meteor.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"

Meteor::Meteor(const StelCore* core, double v)
	: m_distMultiplier(0.)
	, m_segments(10)
{
	// determine meteor velocity
	// abs range 11-72 km/s by default (see line 427 in StelApp.cpp)
	m_speed = 11+(double)rand()/((double)RAND_MAX+1)*(v-11);

	// view matrix of sporadic meteors model
	double alpha = (double)rand()/((double)RAND_MAX+1)*2*M_PI;
	double delta = M_PI_2 - (double)rand()/((double)RAND_MAX+1)*M_PI;
	m_viewMatrix = Mat4d::zrotation(alpha) * Mat4d::yrotation(delta);

	// building meteor model
	m_alive = initMeteorModel(core, m_segments, m_viewMatrix, meteor);
	if (!m_alive)
	{
		return;
	}

	// determine the meteor color
	QList<colorPair> colors;
	colors.push_back(colorPair("white", 100));

	// building lineColorArray and trainColorArray
	buildColorArrays(m_segments, colors, m_lineColorArray, m_trainColorArray);
}

Meteor::~Meteor()
{
}

// returns true if alive
bool Meteor::initMeteorModel(const StelCore* core, const int segments, const Mat4d viewMatrix, MeteorModel &mm)
{
	double high_range = EARTH_RADIUS + HIGH_ALTITUDE;
	double low_range = EARTH_RADIUS + LOW_ALTITUDE;

	// find observer position in meteor coordinate system
	mm.obs = core->altAzToJ2000(Vec3d(0,0,EARTH_RADIUS));
	mm.obs.transfo4d(viewMatrix.transpose());

	// select random trajectory using polar coordinates in XY plane, centered on observer
	mm.xydistance = (double)rand() / ((double)RAND_MAX+1)*(VISIBLE_RADIUS);
	double angle = (double)rand() / ((double)RAND_MAX+1)*2*M_PI;

	// set meteor start x,y
	mm.position[0] = mm.posTrain[0] = mm.xydistance*cos(angle) + mm.obs[0];
	mm.position[1] = mm.posTrain[1] = mm.xydistance*sin(angle) + mm.obs[1];

	// D is distance from center of earth
	double D = sqrt(mm.position[0]*mm.position[0] + mm.position[1]*mm.position[1]);

	if (D > high_range)     // won't be visible, meteor still dead
	{
		return false;
	}

	mm.posTrain[2] = mm.position[2] = mm.startH = sqrt(high_range*high_range - D*D);

	// determine end of burn point, and nearest point to observer for distance mag calculation
	// mag should be max at nearest point still burning
	if (D > low_range)
	{
		mm.endH = -mm.startH;  // earth grazing
		mm.minDist = mm.xydistance;
	}
	else
	{
		mm.endH = sqrt(low_range*low_range - D*D);
		mm.minDist = sqrt(mm.xydistance*mm.xydistance + pow(mm.endH - mm.obs[2], 2));
	}

	if (mm.minDist > VISIBLE_RADIUS)
	{
		// on average, not visible (although if were zoomed ...)
		return false; //meteor still dead
	}

	// determine intensity [-3; 4.5]
	float Mag1 = (double)rand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag2 = (double)rand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag = (Mag1 + Mag2)/2.0f;

	// compute RMag and CMag
	RCMag rcMag;
	core->getSkyDrawer()->computeRCMag(Mag, &rcMag);
	mm.mag = rcMag.luminance;

	// most visible meteors are under about 180km distant
	// scale max mag down if outside this range
	float scale = 1;
	if (mm.minDist)
	{
		scale = 180*180 / (mm.minDist*mm.minDist);
	}
	if (scale < 1)
	{
		mm.mag *= scale;
	}

	mm.firstBrightSegment = (double)rand()/((double)RAND_MAX+1)*segments;

	// If everything is ok until here,
	return true;  //the meteor is alive
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

void Meteor::buildColorArrays(const int segments,
			      const QList<colorPair> colors,
			      QList<Vec4f> &lineColorArray,
			      QList<Vec4f> &trainColorArray)
{
	// building color arrays (line and prism)
	int totalOfSegments = 0;
	int currentSegment = 1+(double)rand()/((double)RAND_MAX+1)*(segments-1);
	for (int colorIndex=0; colorIndex<colors.size(); colorIndex++) {
		colorPair currentColor = colors[colorIndex];

		// segments which we'll paint with the current color
		int numOfSegments = segments*(currentColor.second / 100.f) + 0.4f; // +0.4 affect approximation
		if (colorIndex == colors.size()-1) {
			numOfSegments = segments - totalOfSegments;
		}

		totalOfSegments += numOfSegments;
		for (int i=0; i<numOfSegments; i++) {
			lineColorArray.insert(currentSegment, getColor(currentColor.first));
			trainColorArray.insert(currentSegment, getColor(currentColor.first));
			if (currentSegment >= segments-1) {
				currentSegment = 0;
			} else {
				currentSegment++;
			}
			trainColorArray.insert(currentSegment, getColor(currentColor.first));
		}
	}
}

bool Meteor::updateMeteorModel(double deltaTime, double speed, MeteorModel &mm)
{
	if (mm.position[2] < mm.endH)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out
		mm.mag -= deltaTime/500.0f;
		if(mm.mag < 0)
		{
			return false;    // no longer visible
		}
	}

	// *** would need time direction multiplier to allow reverse time replay
	double dt = 820+(double)rand()/((double)RAND_MAX+1)*185; // range 820-1005
	mm.position[2] -= speed*deltaTime/dt;

	// train doesn't extend beyond start of burn
	if (mm.position[2] + speed*0.5f > mm.startH)
	{
		mm.posTrain[2] = mm.startH;
	}
	else
	{
		mm.posTrain[2] -= speed*deltaTime/dt;
	}

	return true;
}

// returns true if alive
bool Meteor::update(double deltaTime)
{
	if (!m_alive)
	{
		return false;
	}

	m_alive = updateMeteorModel(deltaTime, m_speed, meteor);

	// determine visual magnitude based on distance to observer
	double dist = sqrt(meteor.xydistance*meteor.xydistance + pow(meteor.position[2]-meteor.obs[2], 2));
	if (dist == 0)
	{
		dist = .01;    // just to be cautious (meteor hits observer!)
	}
	m_distMultiplier = meteor.minDist*meteor.minDist / (dist*dist);

	return m_alive;
}

void Meteor::insertVertex(const StelCore* core, QVector<Vec3d> &vertexArray, Vec3d vertex) {
	vertex.transfo4d(m_viewMatrix);
	vertex = core->j2000ToAltAz(vertex);
	vertex[2] -= EARTH_RADIUS;
	vertex/=1216; // 1216 is to scale down under 1 for desktop version
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

	Vec3d posTrainB = meteor.posTrain;
	posTrainB[0] += thickness*0.7;
	posTrainB[1] += thickness*0.7;
	Vec3d posTrainL = meteor.posTrain;
	posTrainL[1] -= thickness;
	Vec3d posTrainR = meteor.posTrain;
	posTrainR[0] -= thickness;

	for (int i=0; i<m_segments; i++) {
		double mag = meteor.mag * i/(3* (m_segments-1));
		if (i > meteor.firstBrightSegment) {
			mag *= 12/5;
		}

		double height = meteor.posTrain[2] + i*(meteor.position[2] - meteor.posTrain[2])/(m_segments-1);
		Vec3d posi;

		posi = meteor.posTrain;
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
