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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _ZONE_DATA_HPP_
#define _ZONE_DATA_HPP_

#include <boost/intrusive_ptr.hpp>

  // just for Vect3d.
  // Take any Vector class instead, if you want to use the star feature in a
  // different context.
#include "vecmath.h"

class StelObject;


namespace BigStarCatalogExtension {

// The stars are arranged in triangular zones according to GeodesicGrid.
// Bright stars (Star1) are stored in zones with small level, whereas
// fainter stars (Star2,Star3) are stored in zones with highter level.
// A ZoneData contains stars of a given type (Star1,Star2 or Star3) of
// a given triangle and level of the GeodesicGrid.

struct ZoneData { // a single Triangle
    // no virtual functions!
  int getNrOfStars(void) const {return size;}
  Vec3d center;
  Vec3d axis0;
  Vec3d axis1;
  int size;
  void *stars;
};

template <class Star>
struct SpecialZoneData : public ZoneData {
  boost::intrusive_ptr<StelObject> createStelObject(const Star *s) const
    {return s->createStelObject(*this);}
  Star *getStars(void) const {return reinterpret_cast<Star*>(stars);}
     // array of stars in this zone
};

} // namespace BigStarCatalogExtension

#endif
