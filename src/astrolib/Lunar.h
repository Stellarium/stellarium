/*****************************************************************************\
 * Lunar.h
 *
 * Lunar is a class that can calculate lunar fundmentals for any reasonable
 * time.
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#if !defined( LUNAR__H )
#define LUNAR__H

#include <math.h>
#include "AstroOps.h"

// A struct to hold the fundmental elements
// The member names should be familiar to Meeus fans ;-)
//
struct LunarFundamentals {
  double Lp;
  double D;
  double M;
  double Mp;
  double F;
  double A1;
  double A2;
  double A3;
  double T;

  LunarFundamentals():Lp(0.),D(0.),M(0.),Mp(0.),F(0.),A1(0.),A2(0.),A3(0.),T(0.) {}
};

// terms for longitude & radius
//
static const int N_LTERM1 = 60;
struct LunarTerms1 {
  char d, m, mp, f;
  long sl, sr;
};

// terms for latitude
//
static const int N_LTERM2 = 60;
struct LunarTerms2 {
  char d, m, mp, f;
  long sb;
};

// our main class -- calculates Lunar fundamentals, lat, lon & distance
//
class Lunar {
public:
  // default c'tor
  Lunar() : m_initialized( false ), m_lon(-1.), m_lat(-1.), m_r(-1.)
  {}

  // data & time c'tor
  // t = decimal julian centuries
  Lunar( double t )
  {
     calcFundamentals( t );
  }

  // calculates the fundamanentals given the time
  // t = decimal julian centuries
  //
  void calcFundamentals( double t );

  //
  // NOTE: calcFundamentals() must be called before calling the functions
  //       below, or an invalid result (-1.) will be returned.
  //

  // returns lunar latitude
  double latitude();         // returns -1 if not initialized
  double latitudeRadians() {  // returns -1 if not initialized
    return ( m_initialized ) ? Astro::toRadians( latitude() ) : -1.;
  }

  // returns lunar longitude
  double longitude()         // returns -1 if not initialized
  {
    if ( m_lon < 0. )
      calcLonRad();
    return m_lon;
  }

  double longitudeRadians()  // returns -1 if not initialized
  {
    return ( m_initialized ) ? Astro::toRadians( longitude() ) : -1.;
  }

  // returns lunar distance
  double radius()     // returns -1 if not initialized
  {
    if ( m_r < 0. )
      calcLonRad();
    return m_r;
  }

  // calculate all three location elements of the spec'd body at the given time
  //
  void calcAllLocs(
      double& lon,            // returned longitude
      double& lat,            // returned latitude
      double& rad,            // returned radius vector
      double t)               // time in decimal centuries
  {
    calcFundamentals( t );
    lon = longitudeRadians();
    lat = latitudeRadians();
    rad = radius();
  }

private:
  // reduce (0 < d < 360) a positive angle and convert to radians
  //
  double normalize( double d ) {
    return Astro::toRadians( AstroOps::normalizeDegrees( d ) );
  }

  // calculate an individual fundimental
  //  tptr - points to array of doubles
  //  t - time in decimal Julian centuries
  //
  double getFund( const double* tptr, double t );

  // calculate longitude and radius
  //
  // NOTE: calcFundamentals() must have been called first
  //
  void calcLonRad();

  // ***** data  *****

  // our calculated fundmentals
  //
  LunarFundamentals m_f;

  // true if calcFundamentals has been called
  bool m_initialized;

  // longitude, latitude, and radius (stored in _degrees_)
  double m_lon, m_lat, m_r;
};

#endif  /* #if !defined( LUNAR__H ) */
