/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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


#include "MeteorObj.hpp"

MeteorObj::MeteorObj(const StelCore* core, int speed, const float& radiantAlpha, const float& radiantDelta,
		     const float& pidx, QList<Meteor::colorPair> colors, const StelTextureSP& bolideTexture)
	: Meteor(core, bolideTexture)
{
	// if speed is zero, use a random value
	if (!speed)
	{
		speed = 11 + (double)qrand() / ((double)RAND_MAX + 1) * 61;  // abs range 11-72 km/s
	}

	// determine the meteor color
	if (colors.isEmpty()) {
		colors.push_back(Meteor::colorPair("white", 100));
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
			colors.push_back(Meteor::colorPair("white", 100));
		} else {
			increaseWhite = 100 - totalIntensity;
		}

		if (increaseWhite > 0) {
			if (indexWhite == -1) {
				colors.push_back(Meteor::colorPair("white", increaseWhite));
			} else {
				colors[indexWhite].second = increaseWhite;
			}
		}
	}

	// building meteor model
	init(radiantAlpha, radiantDelta, speed, colors);

	if (!isAlive())
	{
		return;
	}

	// implements the population index
	float oneMag = -0.2; // negative, working in different scale ( 0 to 1 - where 1 is brighter)
	if (pidx) // is not zero
	{
		if (qrand()%100 < 100.f/pidx) // probability
		{
			setAbsMag(absMag() + oneMag);  // (m+1)
		}
	}
}

MeteorObj::~MeteorObj()
{
}
