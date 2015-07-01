/*
 * Stellarium
 * Copyright (C) 2015 Marcos Cardinot
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

#include "SporadicMeteor.hpp"

SporadicMeteor::SporadicMeteor(const StelCore* core, const float& maxVel, const StelTextureSP& bolideTexture)
	: Meteor(core, bolideTexture)
{
	// meteor velocity
	// (see line 460 in StelApp.cpp)
	float speed = 11 + (maxVel - 11) * ((float) qrand() / ((float) RAND_MAX + 1)); // [11, maxVel]

	// radiant position
	float rAlpha = 2 * M_PI * ((float) qrand() / ((float) RAND_MAX + 1));  // [0, 2pi]
	float rDelta = M_PI_2 - M_PI * ((float) qrand() / ((double) RAND_MAX + 1));  // [-pi/2, pi/2]

	init(rAlpha, rDelta, speed, getRandColor());
}

SporadicMeteor::~SporadicMeteor()
{
}

QList<Meteor::colorPair> SporadicMeteor::getRandColor()
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
