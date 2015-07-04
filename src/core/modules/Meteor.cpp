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
#include "StelUtils.hpp"

#include <QtMath>

Meteor::Meteor(const StelCore* core, const StelTextureSP& bolideTexture)
	: m_core(core),
	  m_alive(false),
	  m_bolideTexture(bolideTexture),
	  m_segments(10)
{
	qsrand(QDateTime::currentMSecsSinceEpoch());
}

Meteor::~Meteor()
{
	m_bolideTexture.clear();
}

void Meteor::init(const float& radiantAlpha, const float& radiantDelta,
		  const float& speed, const QList<colorPair> colors)
{
	// meteor velocity in km/s
	m_speed = speed;

	// find the radiant in horizontal coordinates
	Vec3d radiantAltAz;
	StelUtils::spheToRect(radiantAlpha, radiantDelta, radiantAltAz);
	radiantAltAz = m_core->j2000ToAltAz(radiantAltAz);
	float radiantAlt, radiantAz;
	StelUtils::rectToSphe(&radiantAz, &radiantAlt, radiantAltAz);
	// N is zero, E is 90 degrees
	radiantAz = 3. * M_PI - radiantAz;
	if (radiantAz > M_PI*2)
	{
		radiantAz -= M_PI*2;
	}

	// meteors won't be visible if radiant is below 0degrees
	if (radiantAlt < 0.f)
	{
		return;
	}

	// determine the rotation matrix to align z axis with radiant
	m_matAltAzToRadiant = Mat4d::yrotation(M_PI_2 - radiantAlt) * Mat4d::zrotation(radiantAz);

	// select a random initial meteor altitude [MIN_ALTITUDE, MAX_ALTITUDE]
	float initialAlt = MIN_ALTITUDE + (MAX_ALTITUDE - MIN_ALTITUDE) * ((float) qrand() / ((float) RAND_MAX + 1));

	// determine the initial distance from observer to radiant
	m_initialDist = meteorDistance(M_PI_2 - radiantAlt, initialAlt);

	// meteor trajectory
	m_xyDist = VISIBLE_RADIUS * ((double) qrand() / ((double) RAND_MAX + 1)); // [0, visibleRadius]
	float angle = 2 * M_PI * ((double) qrand() / ((double) RAND_MAX + 1)); // [0, 2pi]

	// initial meteor coordinates
	m_position[0] = m_xyDist * qCos(angle);
	m_position[1] = m_xyDist * qSin(angle);
	m_position[2] = m_initialDist;
	m_posTrain = m_position;

	// find the angle from horizon to meteor
	Vec3d positionAltAz = radiantToAltAz(m_position);
	float meteorLat = qAsin(positionAltAz[2] / positionAltAz.length());

	// below the horizon ?
	if (meteorLat < 0.f)
	{
		return;
	}

	// determine the final distance from observer to meteor
	if (radiantAlt < 0.0174532925f) // (<1 degrees) earth grazing meteor ?
	{
		m_finalDist = -m_initialDist;
		// a meteor cannot hit the observer
		if (m_xyDist < BURN_ALTITUDE)
		{
			return;
		}
	}
	else
	{
		m_finalDist = meteorDistance(M_PI_2 - meteorLat, MIN_ALTITUDE);
	}

	// determine intensity [-3; 4.5]
	float Mag1 = (double)qrand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag2 = (double)qrand()/((double)RAND_MAX+1)*7.5f - 3;
	float Mag = (Mag1 + Mag2)/2.0f;

	// compute RMag and CMag
	RCMag rcMag;
	m_core->getSkyDrawer()->computeRCMag(Mag, &rcMag);
	m_absMag = rcMag.luminance;

	// most visible meteors are under about 184km distant
	// scale max mag down if outside this range
	float scale = qPow(184, 2) / qPow(m_initialDist, 2);
	if (scale < 1.0f)
	{
		m_absMag *= scale;
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

	if (m_position[2] < m_finalDist)
	{
		// burning has stopped so magnitude fades out
		// assume linear fade out
		m_absMag -= deltaTime/500.0f;
		m_aptMag = m_absMag;
		if(m_absMag < 0)
		{
			m_alive = false;
			return false;    // no longer visible
		}
	}
	else
	{
		// update apparent magnitude based on distance to observer
		m_aptMag = m_absMag * 0.5f;
		if (m_finalDist == -m_initialDist) // earth-grazer?
		{
			float mult = qPow(m_xyDist, 2) / qPow(m_position[2], 2);
			m_aptMag += 0.5f * mult > 1.f? 1.f : mult;
		}
		else
		{
			m_aptMag += 0.5f * (qPow(m_finalDist, 2) / qPow(m_position[2], 2));
		}
	}

	m_position[2] -= m_speed * deltaTime / 1000.0f;

	// train doesn't extend beyond start of burn
	if (m_position[2] + m_speed * 0.5f > m_initialDist)
	{
		m_posTrain[2] = m_initialDist;
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

float Meteor::meteorDistance(float zenithAngle, float altitude)
{
	float distance;

	if (zenithAngle > 1.13446401f) // > 65 degrees?
	{
		float zcos = qCos(zenithAngle);
		distance = qSqrt(EARTH_RADIUS2 * qPow(zcos, 2)
				 + 2 * EARTH_RADIUS * altitude
				 + qPow(altitude, 2));
		distance -= EARTH_RADIUS * zcos;
	}
	else
	{
		// (first order approximation)
		distance = altitude / qCos(zenithAngle);
	}

	return distance;
}

Vec3d Meteor::altAzToRadiant(Vec3d position)
{
	position.transfo4d(m_matAltAzToRadiant);
	position *= 1242;
	return position;
}

Vec3d Meteor::radiantToAltAz(Vec3d position)
{
	position /= 1242; // 1242 to scale down under 1
	position.transfo4d(m_matAltAzToRadiant.transpose());
	return position;
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
	if (!bolideSize || !m_bolideTexture || m_position[2] > VISIBLE_RADIUS)
	{
		return;
	}

	// bolide
	//
	QVector<Vec3d> vertexArrayBolide;
	QVector<Vec4f> colorArrayBolide;
	Vec4f bolideColor = Vec4f(1, 1, 1, m_aptMag);

	Vec3d topLeft = m_position;
	topLeft[1] -= bolideSize;
	vertexArrayBolide.push_back(radiantToAltAz(topLeft));
	colorArrayBolide.push_back(bolideColor);

	Vec3d topRight = m_position;
	topRight[0] -= bolideSize;
	vertexArrayBolide.push_back(radiantToAltAz(topRight));
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomRight = m_position;
	bottomRight[1] += bolideSize;
	vertexArrayBolide.push_back(radiantToAltAz(bottomRight));
	colorArrayBolide.push_back(bolideColor);

	Vec3d bottomLeft = m_position;
	bottomLeft[0] += bolideSize;
	vertexArrayBolide.push_back(radiantToAltAz(bottomLeft));
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
		float mag = m_aptMag * i/(3* (m_segments-1));
		if (i > m_firstBrightSegment)
		{
			mag *= 12./5.;
		}

		double height = m_posTrain[2] + i*(m_position[2] - m_posTrain[2])/(m_segments-1);
		Vec3d posi;

		posi = m_posTrain;
		posi[2] = height;
		vertexArrayLine.push_back(radiantToAltAz(posi));

		posi = posTrainB;
		posi[2] = height;
		vertexArrayL.push_back(radiantToAltAz(posi));
		vertexArrayR.push_back(radiantToAltAz(posi));

		posi = posTrainL;
		posi[2] = height;
		vertexArrayL.push_back(radiantToAltAz(posi));
		vertexArrayTop.push_back(radiantToAltAz(posi));

		posi = posTrainR;
		posi[2] = height;
		vertexArrayR.push_back(radiantToAltAz(posi));
		vertexArrayTop.push_back(radiantToAltAz(posi));

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
