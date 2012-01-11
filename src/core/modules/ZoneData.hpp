/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
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

#ifndef _ZONEDATA_HPP_
#define _ZONEDATA_HPP_

// just for Vect3d.
// Take any Vector class instead, if you want to use the star feature in a
// different context.
#include "VecMath.hpp"
#include "StelObjectType.hpp"

class StelObject;

namespace BigStarCatalogExtension
{

//! @struct ZoneData
//! A single triangle. The stars are arranged in triangular zones according to
//! StelGeodesicGrid. Bright stars (Star1) are stored in zones with small level,
//! whereas fainter stars (Star2,Star3) are stored in zones with highter level.
//! A ZoneData contains stars of a given type (Star1,Star2 or Star3) of
//! a given triangle and level of the StelGeodesicGrid.
struct ZoneData
{
	//! Get number of stars in this triangle.
	int getNrOfStars(void) const
	{
		return size;
	}
	Vec3f center;
	Vec3f axis0;
	Vec3f axis1;
	int size;
	void *stars;
};

//! @struct SpecialZoneData
//! Wrapper struct around ZoneData.
//! @tparam Star either Star1, Star2 or Star3, depending on the brightness of
//! stars in this triangle
template <class Star>
struct SpecialZoneData : public ZoneData
{
	StelObjectP createStelObject(const Star *s) const
	{
		return s->createStelObject(*this);
	}

	//! Get array of stars in this zone.
	Star *getStars(void) const
	{
		return reinterpret_cast<Star*>(stars);
	}
};

} // namespace BigStarCatalogExtension

#endif // _ZONEDATA_HPP_
