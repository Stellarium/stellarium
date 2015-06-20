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
			   QList<MeteorShower::colorPair> colors,
			   StelTextureSP bolideTexture)
{
	qsrand (QDateTime::currentMSecsSinceEpoch());

	// if speed is zero, use a random value
	if (!speed)
	{
		speed = 11 + (double)qrand() / ((double)RAND_MAX + 1) * 61;  // abs range 11-72 km/s
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
			qWarning() << "MeteorShowers plugin (showers.json): Total intensity must be less than 100";
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

	// building meteor model
	m_meteor = new Meteor(core);
	m_meteor->init(radiantAlpha, radiantDelta, speed, colors, bolideTexture);
	if (!m_meteor->isAlive())
	{
		return;
	}

	// implements the population index
	float oneMag = -0.2; // negative, working in different scale ( 0 to 1 - where 1 is brighter)
	if (pidx) // is not zero
	{
		if (qrand()%100 < 100.f/pidx) // probability
		{
			m_meteor->setMag(m_meteor->mag() + oneMag);  // (m+1)
		}
	}
}

MeteorStream::~MeteorStream()
{
}

bool MeteorStream::update(double deltaTime)
{
	return m_meteor->update(deltaTime);
}

void MeteorStream::draw(const StelCore* core, StelPainter& sPainter)
{
	m_meteor->draw(core, sPainter);
}
