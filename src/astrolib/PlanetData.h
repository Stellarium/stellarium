/*****************************************************************************\
 * PlanetData.h
 *
 * Handles planetary motion calculations and conversions
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#if !defined( PLANET_DATA__H )
#define PLANET_DATA__H

#include "AstroOps.h"
#include "Pluto.h"

class ObsInfo;

// bodies of interest
//
enum PlanetNb { NAP=-1, // NotAPlanet
        SUN=0, MERCURY=1, VENUS=2, EARTH=3, MARS=4, JUPITER=5,
        SATURN=6, URANUS=7, NEPTUNE=8, PLUTO=9, LUNA=10 };

class PlanetData {
public:
  PlanetData() : m_planet( NAP ) {}
  PlanetData( PlanetNb planet, double jd, ObsInfo& oi ) {
    calc( planet, jd, oi );
  }

  // Calculate the data for a given planet, jd, and location
  // This function must be called (directly or via c'tor) before calling
  // any of the other fns!
  //
  void calc( PlanetNb planet, double jd, ObsInfo& oi );

  PlanetNb planet() const { return m_planet; }
  double jd() const { return ( NAP == m_planet ) ? -1. : m_jd; }
  double hourAngle() const { return ( NAP == m_planet ) ? -1. : m_hourAngle; }

  double eclipticLon() const { return ( NAP == m_planet ) ? -1. : m_eclipticLon; }
  double eclipticLat() const { return ( NAP == m_planet ) ? -1. : m_eclipticLat; }
  double radius()      const { return ( NAP == m_planet ) ? -1. : m_r; }

  // The &3's will limit index GPFs with low perf. penalty
  const double eclipticLoc(int i) const {
      return ( NAP == m_planet ) ? -1. : m_eclipticLoc[i & 3];
  }
  const double equatorialLoc(int i) const {
      return ( NAP == m_planet ) ? -1. : m_equatorialLoc[i & 3];
  }
  const double altazLoc(int i) const {
      return ( NAP == m_planet ) ? -1. : m_altazLoc[i & 3];
  }

private:
  PlanetNb m_planet;

  double m_jd;
  double m_r;
  double m_eclipticLon;
  double m_eclipticLat;
  double m_hourAngle;

  AstroVector m_eclipticLoc;
  AstroVector m_equatorialLoc;
  AstroVector m_altazLoc;
};





#endif  /* #if !defined( PLANET_DATA__H ) */
