/*****************************************************************************\
 * AstroOps
 *
 * Astro     defines misc. handy constants & fns
 * ObsInfo   holds latitude, longitude and TZ for an observing location
 * AstroOps  is a 'catch-all' class that performs odd handy calculations
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#if !defined( ASTRO_OPS__H )
#define ASTRO_OPS__H

// * * * * * Generic Handy Constants and Conversions * * * * *

#define PI_DEF 3.14159265358979323846

struct Astro {
   // these are initialized in AstroOps.cpp

   static const double PI_ASTRO;
   static const double TWO_PI;
   static const double PI_OVER_TWO;
   static const double TWO_OVER_PI;
   static const double DEG_PER_CIRCLE;
   static const double MINUTES_PER_DEGREE;
   static const double SECONDS_PER_DEGREE;

   static const double TO_RADS;
   static const double TO_CENTURIES;

   static const double HOURS_PER_DAY;
   static const int IHOURS_PER_DAY;

   static const double J2000;

   // convert degrees to radians
   static double toRadians( double deg ) { return deg * TO_RADS; }

   // convert radians to degrees
   static double toDegrees( double rad ) { return rad / TO_RADS; }

   // convert hour to decimal day, e.g., 12h -> .5d
   static double toDays( int hours ) { return (double)hours / HOURS_PER_DAY; }

   // convert a JD to Julian Millenia referenced to epoch J2000
   static double toMillenia( double jd ) { return ( jd - J2000 ) / TO_CENTURIES; }
};

// * * * * * Handy Type Definitions * * * * *

typedef double AstroVector[3];
typedef double AstroMatrix[9];

// * * * * * Observer's Location Info * * * * *

class ObsInfo {
public:
  // c'tor: lon & lat are passed in DEGREES
  ObsInfo( double lon = 0, double lat = 0, int tz = 0 ) :
      m_longitude( Astro::toRadians(lon) ),
      m_latitude( Astro::toRadians(lat) ),
      m_timeZone( tz )
  {}

  double longitude() const { return m_longitude; }
  double degLatitude() const { return Astro::toDegrees(m_latitude); }
  // set: lon passed in DEGREES
  void setLongitude(double lon) { m_longitude = Astro::toRadians(lon); }

  double latitude() const { return m_latitude;  }
  double degLongitude() const { return Astro::toDegrees(m_longitude); }
  // set: lat passed in DEGREES
  void setLatitude(double lat) { m_latitude = Astro::toRadians(lat); }

  int timeZone() const { return m_timeZone;  }
  void setTimeZone(int tz) { m_timeZone = tz; }

private:
  double m_longitude;   // in radians, N positive
  double m_latitude;    // in radians, E positive
  int    m_timeZone;    // +/- 12 viz. UT
};

// * * * * * Handy functions that didn't fit anywhere else * * * * *

class AstroOps {
public:
  static double meanObliquity( double t );
  static void   nutation( double t, double* d_lon, double* d_obliq );
  static double greenwichSiderealTime( double jd );

  static double normalizeDegrees( double d);
  static double normalizeRadians( double d);
};

#endif  /* #if !defined( ASTRO_OPS__H ) */


