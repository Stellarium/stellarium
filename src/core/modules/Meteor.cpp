/*
 * Stellarium
 * Copyright (C) 2014-2015 Marcos Cardinot
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

#include <QtMath>

#include "Meteor.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"
#include "StelTexture.hpp"

StelTextureSP Meteor::bolideTexture;

Meteor::Meteor(const StelCore* core, float v)
	: m_segments(10)
{
	qsrand(QDateTime::currentMSecsSinceEpoch());

	//
	// Sporadic Meteors Model
	//

	// meteor velocity
	// (see line 460 in StelApp.cpp)
	m_speed = 11 + (v - 11) * ((float) qrand() / ((float) RAND_MAX + 1)); // [11, v]

	// rotation matrix
	float alpha = 2 * M_PI * ((float) qrand() / ((float) RAND_MAX + 1));  // [0, 2pi]
	float delta = M_PI_2 - M_PI * ((float) qrand() / ((double) RAND_MAX + 1));  // [-pi/2, pi/2]
	m_viewMatrix = Mat4d::zrotation(alpha) * Mat4d::yrotation(delta);

	// building meteor model
	m_alive = initMeteorModel(core, m_segments, m_viewMatrix, meteor);
	if (!m_alive)
	{
		return;
	}

	// building lineColorArray and trainColorArray
	buildColorArrays(m_segments, getRandColor(), m_lineColorArray, m_trainColorArray);
}

Meteor::~Meteor()
{
}

// returns true if alive
bool Meteor::initMeteorModel(const StelCore* core, const int segments, const Mat4d viewMatrix, MeteorModel &mm)
{
	// initial meteor altitude above the earth surface
	mm.initialAlt = MIN_ALTITUDE + (MAX_ALTITUDE - MIN_ALTITUDE) * ((float) qrand() / ((float) RAND_MAX + 1));

	// meteor zenith angle (angle from radiant to meteor)
	float zenithAngle = M_PI_2 * ((double) qrand() / ((double) RAND_MAX + 1)); // [0, pi/2]

	// distance between the observer and the meteor
	float distance;
	if (zenithAngle > 1.134f) // > 65 degrees?
	{
		distance = qSqrt(EARTH_RADIUS2 * qPow(zenithAngle, 2)
				 + 2 * EARTH_RADIUS * mm.initialAlt
				 + qPow(mm.initialAlt, 2));
		distance -= EARTH_RADIUS * qCos(zenithAngle);
	}
	else
	{
		// (first order approximation)
		distance = mm.initialAlt / qCos(zenithAngle);
	}

	// meteor trajectory
	mm.xyDist = distance * qSin(zenithAngle);
	float angle = 2 * M_PI * ((double) qrand() / ((double) RAND_MAX + 1)); // [0, 2pi]

	// initial meteor coordinates
	mm.position[0] = mm.xyDist * qCos(angle);
	mm.position[1] = mm.xyDist * qSin(angle);
	mm.position[2] = mm.initialAlt;
	mm.posTrain = mm.position;

	// find meteor position in horizontal coordinate system
	Vec3d positionAltAz = meteorToAltAz(core, viewMatrix, mm.position);
	float meteorLat = qAsin(positionAltAz[2] / positionAltAz.length());

	// below the horizon
	if (meteorLat < 0.0f)
	{
		return false;
	}

	// final meteor altitude (end of burn point)
	if (mm.xyDist > MIN_ALTITUDE)
	{
		mm.finalAlt = -mm.initialAlt;  // earth grazing
	}
	else
	{
		mm.finalAlt = qSqrt(MIN_ALTITUDE*MIN_ALTITUDE - mm.xyDist*mm.xyDist);
	}

	// determine intensity [-3; 4.5]
	float Mag1 = (double)qrand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag2 = (double)qrand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag = (Mag1 + Mag2)/2.0f;

	// compute RMag and CMag
	RCMag rcMag;
	core->getSkyDrawer()->computeRCMag(Mag, &rcMag);
	mm.mag = rcMag.luminance;

	// most visible meteors are under about 184km distant
	// scale max mag down if outside this range
	float scale = qPow(184, 2) / qPow(distance, 2);
	if (scale < 1.0f)
	{
		mm.mag *= scale;
	}

	mm.firstBrightSegment = segments * ((float) qrand() / ((float) RAND_MAX + 1));

	// If everything is ok until here,
	return true;  //the meteor is alive
}

Vec4f Meteor::getColorFromName(QString colorName)
{
	int R, G, B; // 0-255
	if (colorName == "violet")
	{ // Calcium
		R = 176;
		G = 67;
		B = 172;
	}
	else if (colorName == "blueGreen")
	{ // Magnesium
		R = 0;
		G = 255;
		B = 152;
	}
	else if (colorName == "yellow")
	{ // Iron
		R = 255;
		G = 255;
		B = 0;
	}
	else if (colorName == "orangeYellow")
	{ // Sodium
		R = 255;
		G = 160;
		B = 0;
	}
	else if (colorName == "red")
	{ // atmospheric nitrogen and oxygen
		R = 255;
		G = 30;
		B = 0;
	}
	else
	{ // white
		R = 255;
		G = 255;
		B = 255;
	}

	return Vec4f(R/255.f, G/255.f, B/255.f, 1);
}

QList<Meteor::colorPair> Meteor::getRandColor()
{
	QList<colorPair> colors;
	float prob = (double)qrand()/((double)RAND_MAX+1);
	if (prob > 0.9)
	{
		colors.push_back(Meteor::colorPair("white", 70));
		colors.push_back(Meteor::colorPair("orangeYellow", 10));
		colors.push_back(Meteor::colorPair("yellow", 10));
		colors.push_back(Meteor::colorPair("blueGreen", 10));
	}
	else if (prob > 0.85)
	{
		colors.push_back(Meteor::colorPair("white", 80));
		colors.push_back(Meteor::colorPair("violet", 20));
	}
	else if (prob > 0.80)
	{
		colors.push_back(Meteor::colorPair("white", 80));
		colors.push_back(Meteor::colorPair("orangeYellow", 20));
	}
	else
	{
		colors.push_back(Meteor::colorPair("white", 100));
	}

	return colors;
}

void Meteor::buildColorArrays(const int segments,
			      const QList<colorPair> colors,
			      QList<Vec4f> &lineColorArray,
			      QList<Vec4f> &trainColorArray)
{
	// building color arrays (line and prism)
	int totalOfSegments = 0;
	int currentSegment = 1+(double)qrand()/((double)RAND_MAX+1)*(segments-1);
	for (int colorIndex=0; colorIndex<colors.size(); colorIndex++)
	{
		colorPair currentColor = colors[colorIndex];

		// segments which we'll paint with the current color
		int numOfSegments = segments*(currentColor.second / 100.f) + 0.4f; // +0.4 affect approximation
		if (colorIndex == colors.size()-1)
		{
			numOfSegments = segments - totalOfSegments;
		}

		totalOfSegments += numOfSegments;
		for (int i=0; i<numOfSegments; i++)
		{
			lineColorArray.insert(currentSegment, getColorFromName(currentColor.first));
			trainColorArray.insert(currentSegment, getColorFromName(currentColor.first));
			if (currentSegment >= segments-1)
			{
				currentSegment = 0;
			}
			else
			{
				currentSegment++;
			}
			trainColorArray.insert(currentSegment, getColorFromName(currentColor.first));
		}
	}
}

bool Meteor::updateMeteorModel(double deltaTime, double speed, MeteorModel &mm)
{
	if (mm.position[2] < mm.finalAlt)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out
		mm.mag -= deltaTime/500.0f;
		if(mm.mag < 0)
		{
			return false;    // no longer visible
		}
	}

	mm.position[2] -= speed * deltaTime / 1000.0f;

	// train doesn't extend beyond start of burn
	if (mm.position[2] + speed * 0.5f > mm.initialAlt)
	{
		mm.posTrain[2] = mm.initialAlt;
	}
	else
	{
		mm.posTrain[2] -= speed * deltaTime / 1000.0f;
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

	return m_alive;
}

Vec3d Meteor::meteorToAltAz(const StelCore* core, const Mat4d& viewMatrix, Vec3d position)
{
	position.transfo4d(viewMatrix);
	position = core->j2000ToAltAz(position);
	position/=1216; // 1216 is to scale down under 1 for desktop version
	return position;
}

void Meteor::insertVertex(const StelCore* core, const Mat4d& viewMatrix, QVector<Vec3d> &vertexArray, Vec3d vertex)
{
	vertexArray.push_back(meteorToAltAz(core, viewMatrix, vertex));
}

void Meteor::calculateThickness(const StelCore* core, float& thickness, float& bolideSize)
{
	float maxFOV = core->getMovementMgr()->getMaxFov();
	float FOV = core->getMovementMgr()->getCurrentFov();
	thickness = 2*log(FOV + 0.25)/(1.2*maxFOV - (FOV + 0.25)) + 0.01;
	if (FOV <= 0.5)
	{
		thickness = 0.013 * FOV; // decreasing faster
	}
	else if (FOV > 100.0)
	{
		thickness = 0; // remove prism
	}
	bolideSize = thickness*3;
}

void Meteor::drawBolide(const StelCore* core, StelPainter& sPainter, const MeteorModel& mm,
			const Mat4d& viewMatrix, const float bolideSize)
{
	if (!bolideSize)
	{
		return;
	}

	// bolide
	//
	QVector<Vec3d> vertexArrayBolide;
	QVector<Vec4f> colorArrayBolide;
	Vec4f bolideColor = Vec4f(1,1,1,mm.mag);

	Vec3d topLeft = mm.position;
	topLeft[1] -= bolideSize;
	insertVertex(core, viewMatrix, vertexArrayBolide, topLeft);
	colorArrayBolide.push_back(bolideColor);

	Vec3d topRight = mm.position;
	topRight[0] -= bolideSize;
	insertVertex(core, viewMatrix, vertexArrayBolide, topRight);
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomRight = mm.position;
	bottomRight[1] += bolideSize;
	insertVertex(core, viewMatrix, vertexArrayBolide, bottomRight);
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomLeft = mm.position;
	bottomLeft[0] += bolideSize;
	insertVertex(core, viewMatrix, vertexArrayBolide, bottomLeft);
	colorArrayBolide.push_back(bolideColor);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	sPainter.enableClientStates(true, true, true);
	Meteor::bolideTexture->bind();
	static const float texCoordData[] = {1.,0., 0.,0., 0.,1., 1.,1.};
	sPainter.setTexCoordPointer(2, GL_FLOAT, texCoordData);
	sPainter.setColorPointer(4, GL_FLOAT, colorArrayBolide.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayBolide.constData());
	sPainter.drawFromArray(StelPainter::TriangleFan, vertexArrayBolide.size(), 0, true);

	glDisable(GL_BLEND);
	sPainter.enableClientStates(false);
}

void Meteor::drawTrain(const StelCore *core, StelPainter& sPainter, const MeteorModel& mm,
		       const Mat4d& viewMatrix, const float thickness, const int segments,
		       QList<Vec4f> lineColorArray, QList<Vec4f> trainColorArray)
{
	if (segments != lineColorArray.size() || 2*segments != trainColorArray.size())
	{
		qWarning() << "Meteor: color arrays have an inconsistent size!";
		return;
	}

	// train (triangular prism)
	//
	QVector<Vec3d> vertexArrayLine;
	QVector<Vec3d> vertexArrayL;
	QVector<Vec3d> vertexArrayR;
	QVector<Vec3d> vertexArrayTop;

	Vec3d posTrainB = mm.posTrain;
	posTrainB[0] += thickness*0.7;
	posTrainB[1] += thickness*0.7;
	Vec3d posTrainL = mm.posTrain;
	posTrainL[1] -= thickness;
	Vec3d posTrainR = mm.posTrain;
	posTrainR[0] -= thickness;

	for (int i=0; i<segments; i++)
	{
		float mag = mm.mag * i/(3* (segments-1));
		if (i > mm.firstBrightSegment)
		{
			mag *= 12./5.;
		}

		double height = mm.posTrain[2] + i*(mm.position[2] - mm.posTrain[2])/(segments-1);
		Vec3d posi;

		posi = mm.posTrain;
		posi[2] = height;
		insertVertex(core, viewMatrix, vertexArrayLine, posi);

		posi = posTrainB;
		posi[2] = height;
		insertVertex(core, viewMatrix, vertexArrayL, posi);
		insertVertex(core, viewMatrix, vertexArrayR, posi);

		posi = posTrainL;
		posi[2] = height;
		insertVertex(core, viewMatrix, vertexArrayL, posi);
		insertVertex(core, viewMatrix, vertexArrayTop, posi);

		posi = posTrainR;
		posi[2] = height;
		insertVertex(core, viewMatrix, vertexArrayR, posi);
		insertVertex(core, viewMatrix, vertexArrayTop, posi);

		lineColorArray[i][3] = mag;
		trainColorArray[i*2][3] = mag;
		trainColorArray[i*2+1][3] = mag;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.enableClientStates(true, false, true);
	if (thickness)
	{
		sPainter.setColorPointer(4, GL_FLOAT, trainColorArray.toVector().constData());

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayL.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayL.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayR.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayR.size(), 0, true);

		sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayTop.constData());
		sPainter.drawFromArray(StelPainter::TriangleStrip, vertexArrayTop.size(), 0, true);
	}
	sPainter.setColorPointer(4, GL_FLOAT, lineColorArray.toVector().constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayLine.constData());
	sPainter.drawFromArray(StelPainter::LineStrip, vertexArrayLine.size(), 0, true);

	glDisable(GL_BLEND);
	sPainter.enableClientStates(false);
}

// returns true if visible
// Assumes that we are in local frame
void Meteor::draw(const StelCore* core, StelPainter& sPainter)
{
	if (!m_alive)
	{
		return;
	}

	float thickness, bolideSize;
	calculateThickness(core, thickness, bolideSize);

	drawTrain(core, sPainter, meteor, m_viewMatrix, thickness,
		  m_segments, m_lineColorArray, m_trainColorArray);

	drawBolide(core, sPainter, meteor, m_viewMatrix, bolideSize);
}
