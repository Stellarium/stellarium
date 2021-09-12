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

#ifndef SPORADICMETEOR_HPP
#define SPORADICMETEOR_HPP

#include "Meteor.hpp"

//! @class SporadicMeteor
//! Models a single sporadic meteor with a random color and a random radiant.
//! @author Marcos Cardinot <mcardinot@gmail.com>
class SporadicMeteor : public Meteor
{
public:
	//! Create a SporadicMeteor object.
	SporadicMeteor(const StelCore* core, const float& maxVel, const StelTextureSP& bolideTexture);
	virtual ~SporadicMeteor();

private:
	static QList<ColorPair> getRandColor();
	static const float _RAND_MAX;
	static const double _RAND_MAX_P1;
	static const float _RAND_MAX_P1_f;
};


#endif // SPORADICMETEOR_HPP
