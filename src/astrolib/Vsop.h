/*****************************************************************************\
 * Vsop.h
 *
 * The Vsop class wraps the VSOP87 data and provides VSOP support fns
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#if !defined( VSOP__H )
#define VSOP__H

#include "PlanetData.h"  // for enum PlanetNB

// * * * * * simple support structs * * * * *

// One VSOP term

struct VsopSet {
  double A;
  double B;
  double C;
};

// A set of VSOP terms

struct VsopTerms {
  unsigned rows;          // number of term sets
  const VsopSet* pTerms;  // pointer to start of data

  VsopTerms() : rows(0), pTerms(0) {}
  VsopTerms( unsigned r, const VsopSet* p ) : rows(r), pTerms(p) {}
};

// A complete collection of VSOP terms (6 Lat, 6 Lon, 6 Rad )

typedef /*const*/ VsopTerms AstroTerms[3*6];

// The main VSOP support class

class Vsop {
public:
  // location elements (longitude, latitide, distance)
  //
  enum LocType { ECLIPTIC_LON = 0, ECLIPTIC_LAT = 1, RADIUS = 2 };

  // calculate the spec'd location element of the spec'd body at the given time
  //
  static double calcLoc(
      double cen,             // time in decimal centuries
      PlanetNb planet,          // must be in the range SUN...NEPTUNE
      LocType value);         // 0=ecliptic lon, 1=ecliptic lat, 2=radius

  // calculate all three location elements of the spec'd body at the given time
  //
  static void calcAllLocs(
      double& lon,            // returned longitude
      double& lat,            // returned latitude
      double& rad,            // returned radius vector
      double cen,             // time in decimal centuries
      PlanetNb planet)          // must be in the range SUN...NEPTUNE
  {
    lon = calcLoc( cen, planet, ECLIPTIC_LON );
    lat = calcLoc( cen, planet, ECLIPTIC_LAT );
    rad = calcLoc( cen, planet, RADIUS );
  }

};  // end class Vsop

#endif  /* #if !defined( VSOP__H ) */
