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

#include "Meteor.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"
#include "StelTexture.hpp"

#include <QtMath>

Meteor::Meteor(const StelCore* core)
	: m_core(core),
	  m_alive(false),
	  m_segments(10)
{
	qsrand(QDateTime::currentMSecsSinceEpoch());
}

Meteor::~Meteor()
{
}

void Meteor::init(const float& radiantAlpha, const float& radiantDelta, const float& speed,
		  const QList<colorPair> colors, const StelTextureSP& bolideTexture)
{
	// bolide texture
	m_bolideTexture = bolideTexture;

	// meteor velocity in km/s
	m_speed = speed;

	// initial meteor altitude above the earth surface
	m_initialAlt = MIN_ALTITUDE + (MAX_ALTITUDE - MIN_ALTITUDE) * ((float) qrand() / ((float) RAND_MAX + 1));

	// angle from radiant to meteor
	float radiantAngle = M_PI_2 * ((double) qrand() / ((double) RAND_MAX + 1)); // [0, pi/2]

	// distance between the observer and the meteor
	float distance;
	if (radiantAngle > 1.134f) // > 65 degrees?
	{
		distance = qSqrt(EARTH_RADIUS2 * qPow(radiantAngle, 2)
				 + 2 * EARTH_RADIUS * m_initialAlt
				 + qPow(m_initialAlt, 2));
		distance -= EARTH_RADIUS * qCos(radiantAngle);
	}
	else
	{
		// (first order approximation)
		distance = m_initialAlt / qCos(radiantAngle);
	}

	// meteor trajectory
	m_xyDist = distance * qSin(radiantAngle);
	float angle = 2 * M_PI * ((double) qrand() / ((double) RAND_MAX + 1)); // [0, 2pi]

	// initial meteor coordinates
	m_position[0] = m_xyDist * qCos(angle);
	m_position[1] = m_xyDist * qSin(angle);
	m_position[2] = m_initialAlt;
	m_posTrain = m_position;

	// determine rotation matrix based on radiant
	m_rotationMatrix = Mat4d::zrotation(radiantAlpha) * Mat4d::yrotation(M_PI_2 - radiantDelta);

	// find meteor position in horizontal coordinate system
	Vec3d positionAltAz = meteorToAltAz(m_core, m_rotationMatrix, m_position);
	float meteorLat = qAsin(positionAltAz[2] / positionAltAz.length());

	// below the horizon
	if (meteorLat < 0.0f)
	{
		return;
	}

	// final meteor altitude (end of burn point)
	if (m_xyDist > MIN_ALTITUDE)
	{
		m_finalAlt = -m_initialAlt;  // earth grazing
	}
	else
	{
		m_finalAlt = qSqrt(MIN_ALTITUDE*MIN_ALTITUDE - m_xyDist*m_xyDist);
	}

	// determine intensity [-3; 4.5]
	float Mag1 = (double)qrand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag2 = (double)qrand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag = (Mag1 + Mag2)/2.0f;

	// compute RMag and CMag
	RCMag rcMag;
	m_core->getSkyDrawer()->computeRCMag(Mag, &rcMag);
	m_mag = rcMag.luminance;

	// most visible meteors are under about 184km distant
	// scale max mag down if outside this range
	float scale = qPow(184, 2) / qPow(distance, 2);
	if (scale < 1.0f)
	{
		m_mag *= scale;
	}

	m_firstBrightSegment = m_segments * ((float) qrand() / ((float) RAND_MAX + 1));

	// build the color arrays
	buildColorArrays(colors);

	m_alive = true;
}

bool Meteor::update(double deltaTime)
{
	if (!m_alive)
	{
		return false;
	}

	if (m_position[2] < m_finalAlt)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out
		m_mag -= deltaTime/500.0f;
		if(m_mag < 0)
		{
			m_alive = false;
			return false;    // no longer visible
		}
	}

	m_position[2] -= m_speed * deltaTime / 1000.0f;

	// train doesn't extend beyond start of burn
	if (m_position[2] + m_speed * 0.5f > m_initialAlt)
	{
		m_posTrain[2] = m_initialAlt;
	}
	else
	{
		m_posTrain[2] -= m_speed * deltaTime / 1000.0f;
	}

	return true;
}

void Meteor::draw(const StelCore* core, StelPainter& sPainter)
{
	if (!m_alive)
	{
		return;
	}

	float thickness;
	float bolideSize;
	calculateThickness(core, thickness, bolideSize);

	drawTrain(core, sPainter, thickness);

	drawBolide(core, sPainter, bolideSize);
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

void Meteor::buildColorArrays(const QList<colorPair> colors)
{
	// building color arrays (line and prism)
	int totalOfSegments = 0;
	int currentSegment = 1 + (m_segments - 1) * ((float) qrand() / ((float) RAND_MAX + 1));
	for (int colorIndex = 0; colorIndex < colors.size(); colorIndex++)
	{
		colorPair currentColor = colors[colorIndex];

		// segments which we'll paint with the current color
		int numOfSegments = m_segments * (currentColor.second / 100.f) + 0.4f; // +0.4 affect approximation
		if (colorIndex == colors.size()-1)
		{
			numOfSegments = m_segments - totalOfSegments;
		}

		totalOfSegments += numOfSegments;
		for (int i=0; i<numOfSegments; i++)
		{
			m_lineColorArray.insert(currentSegment, getColorFromName(currentColor.first));
			m_trainColorArray.insert(currentSegment, getColorFromName(currentColor.first));
			if (currentSegment >= m_segments-1)
			{
				currentSegment = 0;
			}
			else
			{
				currentSegment++;
			}
			m_trainColorArray.insert(currentSegment, getColorFromName(currentColor.first));
		}
	}
}

Vec3d Meteor::meteorToAltAz(const StelCore* core, const Mat4d& rotationMatrix, Vec3d position)
{
	position.transfo4d(rotationMatrix);
	position = core->j2000ToAltAz(position);
	position /= 10000.0; // 10000 is to scale down under 1
	return position;
}

void Meteor::insertVertex(const StelCore* core, const Mat4d& rotationMatrix, QVector<Vec3d> &vertexArray, Vec3d vertex)
{
	vertexArray.push_back(meteorToAltAz(core, rotationMatrix, vertex));
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

void Meteor::drawBolide(const StelCore* core, StelPainter& sPainter, const float& bolideSize)
{
	if (!bolideSize || !m_bolideTexture)
	{
		return;
	}

	// bolide
	//
	QVector<Vec3d> vertexArrayBolide;
	QVector<Vec4f> colorArrayBolide;
	Vec4f bolideColor = Vec4f(1,1,1,m_mag);

	Vec3d topLeft = m_position;
	topLeft[1] -= bolideSize;
	insertVertex(core, m_rotationMatrix, vertexArrayBolide, topLeft);
	colorArrayBolide.push_back(bolideColor);

	Vec3d topRight = m_position;
	topRight[0] -= bolideSize;
	insertVertex(core, m_rotationMatrix, vertexArrayBolide, topRight);
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomRight = m_position;
	bottomRight[1] += bolideSize;
	insertVertex(core, m_rotationMatrix, vertexArrayBolide, bottomRight);
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomLeft = m_position;
	bottomLeft[0] += bolideSize;
	insertVertex(core, m_rotationMatrix, vertexArrayBolide, bottomLeft);
	colorArrayBolide.push_back(bolideColor);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	sPainter.enableClientStates(true, true, true);
	m_bolideTexture->bind();
	static const float texCoordData[] = {1.,0., 0.,0., 0.,1., 1.,1.};
	sPainter.setTexCoordPointer(2, GL_FLOAT, texCoordData);
	sPainter.setColorPointer(4, GL_FLOAT, colorArrayBolide.constData());
	sPainter.setVertexPointer(3, GL_DOUBLE, vertexArrayBolide.constData());
	sPainter.drawFromArray(StelPainter::TriangleFan, vertexArrayBolide.size(), 0, true);

	glDisable(GL_BLEND);
	sPainter.enableClientStates(false);
}

void Meteor::drawTrain(const StelCore *core, StelPainter& sPainter, const float& thickness)
{
	if (m_segments != m_lineColorArray.size() || 2*m_segments != m_trainColorArray.size())
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

	Vec3d posTrainB = m_posTrain;
	posTrainB[0] += thickness*0.7;
	posTrainB[1] += thickness*0.7;
	Vec3d posTrainL = m_posTrain;
	posTrainL[1] -= thickness;
	Vec3d posTrainR = m_posTrain;
	posTrainR[0] -= thickness;

	for (int i=0; i<m_segments; i++)
	{
		float mag = m_mag * i/(3* (m_segments-1));
		if (i > m_firstBrightSegment)
		{
			mag *= 12./5.;
		}

		double height = m_posTrain[2] + i*(m_position[2] - m_posTrain[2])/(m_segments-1);
		Vec3d posi;

		posi = m_posTrain;
		posi[2] = height;
		insertVertex(core, m_rotationMatrix, vertexArrayLine, posi);

		posi = posTrainB;
		posi[2] = height;
		insertVertex(core, m_rotationMatrix, vertexArrayL, posi);
		insertVertex(core, m_rotationMatrix, vertexArrayR, posi);

		posi = posTrainL;
		posi[2] = height;
		insertVertex(core, m_rotationMatrix, vertexArrayL, posi);
		insertVertex(core, m_rotationMatrix, vertexArrayTop, posi);

		posi = posTrainR;
		posi[2] = height;
		insertVertex(core, m_rotationMatrix, vertexArrayR, posi);
		insertVertex(core, m_rotationMatrix, vertexArrayTop, posi);

		m_lineColorArray[i][3] = mag;
		m_trainColorArray[i*2][3] = mag;
		m_trainColorArray[i*2+1][3] = mag;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	sPainter.enableClientStates(true, false, true);
	if (thickness)
	{
		sPainter.setColorPointer(4, GL_FLOAT, m_trainColorArray.toVector().constData());

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
