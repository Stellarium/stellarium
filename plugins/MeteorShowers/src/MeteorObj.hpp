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

#ifndef METEOROBJ_HPP
#define METEOROBJ_HPP

#include "Meteor.hpp"
#include "StelCore.hpp"

//! @class MeteorObj
//! Models a single meteor.
//! @author Marcos Cardinot <mcardinot@gmail.com>
//! @ingroup meteorShowers
class MeteorObj : public Meteor
{
public:
	//! Create a Meteor object.
	//! @param core StelCore instance.
	//! @param speed Meteor speed in km/s.
	//! @param radiantAlpha The radiant alpha in rad.
	//! @param radiantDelta The radiant delta in rad.
	//! @param pidx Population index.
	//! @param colors Meteor color.
	//! @param bolideTexture Bolide texture.
	MeteorObj(const StelCore*, int speed, const float& radiantAlpha, const float& radiantDelta,
		  const float& pidx, QList<Meteor::ColorPair> colors, const StelTextureSP& bolideTexture);
	virtual ~MeteorObj();
};

#endif // METEOROBJ_HPP
