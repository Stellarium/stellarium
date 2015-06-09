/*
 * Stellarium: Meteor Showers Plug-in
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

#include "MeteorShowers.hpp"
#include "MeteorStream.hpp"

MeteorStream::MeteorStream(const StelCore* core,
			   int speed,
			   float radiantAlpha,
			   float radiantDelta,
			   float pidx,
			   QList<MeteorShower::colorPair> colors)
	: m_speed(speed)
	, m_segments(10)
{
	qsrand (QDateTime::currentMSecsSinceEpoch());
	// if speed is zero, use a random value
	if (!speed)
	{
		m_speed = 11+(double)qrand()/((double)RAND_MAX+1)*61;  // abs range 11-72 km/s
	}

	// view matrix of meteor model
	m_viewMatrix = Mat4d::zrotation(radiantAlpha) * Mat4d::yrotation(M_PI_2 - radiantDelta);

	// building meteor model
	m_alive = Meteor::initMeteorModel(core, m_segments, meteor);
	if (!m_alive)
	{
		return;
	}

	// implements the population index
	float oneMag = -0.2; // negative, working in different scale ( 0 to 1 - where 1 is brighter)
	if (pidx) // is not zero
	{
		if (qrand()%100 < 100.f/pidx) // probability
		{
			meteor.mag += oneMag;  // (m+1)
		}
	}

	// determine the meteor color
	if (colors.isEmpty()) {
		colors.push_back(MeteorShower::colorPair("white", 100));
	} else {
		// handle cases when the total intensity is less than 100
		int totalIntensity = 0;
		int indexWhite = -1;
		for (int i=0; i<colors.size(); i++) {
			totalIntensity += colors.at(i).second;
			if (colors.at(i).first == "white") {
				indexWhite = i;
			}
		}

		int increaseWhite = 0;
		if (totalIntensity > 100) {
			qDebug() << "MeteorShowers plugin (showers.json): Total intensity must be less than 100";
			colors.clear();
			colors.push_back(MeteorShower::colorPair("white", 100));
		} else {
			increaseWhite = 100 - totalIntensity;
		}

		if (increaseWhite > 0) {
			if (indexWhite == -1) {
				colors.push_back(MeteorShower::colorPair("white", increaseWhite));
			} else {
				colors[indexWhite].second = increaseWhite;
			}
		}
	}

	// building lineColorArray and trainColorArray
	Meteor::buildColorArrays(m_segments, colors, m_lineColorArray, m_trainColorArray);
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

	m_alive = Meteor::updateMeteorModel(deltaTime, m_speed, meteor);

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

	float thickness, bolideSize;
	Meteor::calculateThickness(core, thickness, bolideSize);

	Meteor::drawTrain(core, sPainter, meteor, m_viewMatrix, thickness,
			  m_segments, m_lineColorArray, m_trainColorArray);

	Meteor::drawBolide(core, sPainter, meteor, m_viewMatrix, bolideSize);
}
