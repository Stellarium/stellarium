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
		     const float& pidx, QList<Meteor::ColorPair> colors, const StelTextureSP& bolideTexture)
	: Meteor(core, bolideTexture)
{
	// if speed is zero, use a random value
	if (!speed)
	{
		speed = 11 + static_cast<int>(static_cast<float>(qrand()) / (static_cast<float>(RAND_MAX) + 1) * 61);  // abs range 11-72 km/s
	}

	// building meteor model
	init(radiantAlpha, radiantDelta, speed, colors);

	if (!isAlive())
	{
		return;
	}

	// implements the population index (pidx) - usually a decimal between 2 and 4
	if (pidx > 1.f)
	{
		// higher pidx implies a larger fraction of faint meteors than average
		float prob = static_cast<float>(qrand()) / (static_cast<float>(RAND_MAX) + 1);
		if (prob > 1.f / pidx)
		{
			// Increase the absolute magnitude ([-3; 4.5]) in 1.5!
			// As we are working on a 0-1 scale (where 1 is brighter),
			// more 1.5 means less 0.2!
			setAbsMag(absMag() - 0.2f);
		}
	}
}

MeteorObj::~MeteorObj()
{
}
