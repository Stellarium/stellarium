/*****************************************************************************\
 * PlanetData.cpp
 *
 * Handles planetary motion calculations and conversions
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#include "PlanetData.h"

#include "MathOps.h"
#include "Lunar.h"
#include "Pluto.h"
#include "Vsop.h"

#include <math.h>
#include <string.h>   // for memcpy()

// calculate the data for a given planet, jd, and location
//
void PlanetData::calc( PlanetNb planet, double jd, ObsInfo& oi )
{
  // There's a lot of calculating here, but the one we need most often
  // is the last one (AltAzLoc), which depends on all the previous
  // calculations
  //
  m_jd = jd;
  m_planet = planet;
  const double centuries = Astro::toMillenia( jd );

  // choose appropriate method, based on planet
  //
  if( LUNA == planet ) {          /* not VSOP */
    static Lunar luna;
    luna.calcAllLocs( m_eclipticLon, m_eclipticLat, m_r, centuries );
  }
  else if( PLUTO == planet ) {    /* not VSOP */
    Pluto::calcAllLocs( m_eclipticLon, m_eclipticLat, m_r, centuries );
  }
  else {
    Vsop::calcAllLocs( m_eclipticLon, m_eclipticLat, m_r, centuries, planet );
    if ( EARTH == planet ) {
      /*
       * What we _really_ want is the location of the sun as seen from
       * the earth (geocentric view).  VSOP gives us the opposite
       * (heliocentric) view, i.e., the earth as seen from the sun.
       * To work around this, we add PI to the longitude (rotate 180 degrees)
       * and negate the latitude.
       */
//      m_eclipticLon += Astro::PI; 
//      m_eclipticLat *= -1.;
    }
  }

  AstroVector tmp;
  MathOps::polar3ToCartesian( tmp, m_eclipticLon, m_eclipticLat );
  memcpy( m_eclipticLoc, tmp, sizeof(AstroVector) );

  // At this point,  eclipticLoc is a unit vector in ecliptic
  // coords of date.  Rotate it by 'obliquity' to get
  // a vector in equatorial coords of date:
  //
  const double obliquity = AstroOps::meanObliquity( centuries );
  MathOps::rotateVector( tmp, obliquity, 0 );
  memcpy( m_equatorialLoc, tmp, sizeof(AstroVector) );

  // The following two rotations take us from a vector in
  // equatorial coords of date to an alt/az vector:
  //
  const double localSiderealTime =
      AstroOps::greenwichSiderealTime(jd) + oi.longitude();
  MathOps::rotateVector( tmp, -localSiderealTime, 2 );

  m_hourAngle = atan2( tmp[1], tmp[0] );

  MathOps::rotateVector( tmp, oi.latitude() - Astro::PI_OVER_TWO, 1);
  memcpy( m_altazLoc, tmp, sizeof(AstroVector) );
}
