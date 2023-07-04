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
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"

SporadicMeteor::SporadicMeteor(const StelCore* core, const float& maxVel, const StelTextureSP& bolideTexture)
	: Meteor(core, bolideTexture)
{
	// meteor velocity
	// (see line 460 in StelApp.cpp)
	float speed = 11 + (maxVel - 11) * static_cast<float>(StelApp::getInstance().getRandF()); // [11, maxVel]

	// select a random radiant in a visible area
	double rAlt = M_PI_2  * static_cast<double>(StelApp::getInstance().getRandF());  // [0, pi/2]
	double rAz = 2 * M_PI * static_cast<double>(StelApp::getInstance().getRandF());  // [0, 2pi]
	Vec3d pos;
	StelUtils::spheToRect(rAz, rAlt, pos);

	// convert to J2000
	float rAlpha, rDelta;
	pos = core->altAzToJ2000(pos);
	StelUtils::rectToSphe(&rAlpha, &rDelta, pos);

	// initialize the meteor model
	init(rAlpha, rDelta, speed, getRandColor());
}

SporadicMeteor::~SporadicMeteor()
{
}

QList<Meteor::ColorPair> SporadicMeteor::getRandColor()
{
	QList<ColorPair> colors;
	float prob = StelApp::getInstance().getRandF();
	if (prob > 0.9f)
	{
		colors.push_back(Meteor::ColorPair("white", 70));
		colors.push_back(Meteor::ColorPair("orangeYellow", 10));
		colors.push_back(Meteor::ColorPair("yellow", 10));
		colors.push_back(Meteor::ColorPair("blueGreen", 10));
	}
	else if (prob > 0.85f)
	{
		colors.push_back(Meteor::ColorPair("white", 80));
		colors.push_back(Meteor::ColorPair("violet", 20));
	}
	else if (prob > 0.80f)
	{
		colors.push_back(Meteor::ColorPair("white", 80));
		colors.push_back(Meteor::ColorPair("orangeYellow", 20));
	}
	else
	{
		colors.push_back(Meteor::ColorPair("white", 100));
	}

	return colors;
}
