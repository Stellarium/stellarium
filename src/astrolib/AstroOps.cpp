/*****************************************************************************\
 * AstroOps.cpp
 *
 * AstroOps is a 'catch-all' class that performs misc. handy calculations
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#include "AstroOps.h"

#include <math.h>

// In-class constants cannot be initialized in the class declaration. Sigh.
//
const double Astro::PI_ASTRO = PI_DEF;
const double Astro::TWO_PI = PI_DEF + PI_DEF;
const double Astro::PI_OVER_TWO = PI_DEF / 2.;
const double Astro::TWO_OVER_PI = 2. / PI_DEF;

const double Astro::DEG_PER_CIRCLE = 360.;
const double Astro::MINUTES_PER_DEGREE = 60.;
const double Astro::SECONDS_PER_DEGREE = 3600.;

const double Astro::TO_RADS = PI_DEF / 180.;
const double Astro::TO_CENTURIES = 36525.;

const double Astro::J2000 = 2451545.0;

const double Astro::HOURS_PER_DAY = 24.;
const int    Astro::IHOURS_PER_DAY = 24;

/*----------------------------------------------------------------------------
 * The obliquity formula (and all the magic numbers below) come from Meeus,
 * Astro Algorithms.
 *
 * Input t is time in julian centuries from 2000.
 * Valid range is the years -8000 to +12000 (t = -100 to 100).
 *
 * return value is mean obliquity (epsilon sub 0) in radians.
 */

double AstroOps::meanObliquity( double t )
{
   double u, u0;
   static double t0 = 30000.;
   static double rval = 0.;
   static const double rvalStart = 23. * Astro::SECONDS_PER_DEGREE +
                             26. * Astro::MINUTES_PER_DEGREE +
                             21.448;
   static const int OBLIQ_COEFFS = 10;
   static const double coeffs[ 10 ] = {
         -468093.,  -155.,  199925.,  -5138.,  -24967.,
         -3905.,    712.,   2787.,    579.,    245.
   };

   if( t0 != t ) {

      t0 = t;
      u = u0 = t / 100.;     // u is in julian 10000's of years
      rval = rvalStart;

      for( int i=0; i<OBLIQ_COEFFS; i++ )
      {
          rval += u * coeffs[i] / 100.;
          u *= u0;
      }
      // convert from seconds to radians
      rval = Astro::toRadians( rval / Astro::SECONDS_PER_DEGREE );
   }
   return rval;
}

/*----------------------------------------------------------------------------
 * The nutation formula (and all the magic numbers below) come from
 * p 132-5, Meeus,  Astro Algorithms
 *
 * input is time in julian centuries from 2000.
 *
 * *pDPhi is nutation (delta phi) in arcseconds
 * *pDEpsilon is nutation (delta epsilon) in arcseconds
 *
 * Either pointer can be NULL,  in which case that value is
 * not computed.  (we added this because sometimes,  you want
 * only pDPhi or pDEpsilon;  in such cases,  computing _both_
 * is a waste of perfectly good CPU time)
 */

void AstroOps::nutation( double t, double* pDPhi, double* pDEpsilon )
{
  static const double linearPart[5] = {
      445267.111480, 35999.050340, 477198.867398,
      483202.017538, -1934.136261 };

  static const double coefficients[5][3] = {
      { 29785036., -19142.,  189474. },
      { 35752772., - 1603., -300000. },
      { 13496298.,  86972., 56250. },
      {  9327191., -36825., 327270. },
      { 12504452.,  20708., 450000. }
  };

  static const int N_NUTATION_COEFFS = 62;

  static const int args[62][3] = {
      {  324,-13187,5736 },
      { 1574, -2274, 977 },
      { 1564,  2062,-895 },
      { 1687,  1426,  54 },
      { 1587,   712,  -7 },
      {  449,  -517, 224 },
      { 1573,  -386, 200 },
      { 1599,  -301, 129 },
      {  199,   217, -95 },
      {  337,  -158,   0 },
      {  323,   129, -70 },
      { 1549,   123, -53 },
      { 2812,    63,   0 },
      { 1588,    63, -33 },
      { 2799,   -59,  26 },
      { 1538,   -58,  32 },
      { 1598,   -51,  27 },
      {  362,    48,   0 },
      { 1523,    46, -24 },
      { 2824,   -38,  16 },
      { 1624,   -31,  13 },
      { 1612,    29,   0 },
      {  349,    29, -12 },
      { 1572,    26,   0 },
      {  322,   -22,   0 },
      { 1548,    21, -10 },
      { 1812,    17,   0 },
      { 2788,    16,  -8 },
      {  574,   -16,   7 },
      { 1688,   -15,   9 },
      {  338,   -13,   7 },
      { 1438,   -12,   6 },
      { 1602,    11,   0 },
      { 2798,   -10,   5 },
      { 2849,    -8,   3 },
      { 1699,     7,  -3 },
      {  462,    -7,   0 },
      { 1449,    -7,   3 },
      { 2823,    -7,   3 },
      { 2837,     6,   0 },
      {  374,     6,  -3 },
      {  348,     6,  -3 },
      { 2763,    -6,   3 },
      { 2813,    -6,   3 },
      { 1462,     5,   0 },
      {  198,    -5,   3 },
      {  313,    -5,   3 },
      { 1623,    -5,   3 },
      { 1524,    -3,   0 },
      {  363,     4,   0 },
      {  448,     4,   0 },
      { 1474,    -3,   0 },
      { 2674,    -3,   0 },
      { 1649,    -3,   0 },
      { 2699,    -3,   0 },
      {  837,    -3,   0 },
      {  962,    -4,   0 },
      {  437,    -4,   0 },
      { 1577,     4,   0 },
      { 2187,    -4,   0 },
      { 1712,    -3,   0 },
      { 1597,     3,   0 }
  };

  static const int timeDependent[16 + 9] = {
      -16, -2, 2, -34, 1, 12, -4, 0, -5, 0, 1, 0, 0, 1, 0, -1,
      -31, -5, 5, -1,  0, -6, 0, -1, 3 };

  double terms[5];

  double t2 = t * t;
  double t3 = t2 * t;

  for( int ii=0; ii<5; ii++ ) {
    terms[ii] = linearPart[ii] * t + coefficients[ii][0] / 100000.;
    terms[ii] += t2 * coefficients[ii][1] * 1.e-7;
    terms[ii] += t3 / coefficients[ii][2];
    terms[ii] *= Astro::TO_RADS;
  }

   // The largest term in pDPhi won't fit into a short int.
   // Everything else does,  luckily.
   //
  if( pDPhi )
    *pDPhi = (-171996. - 174.2 * t) * sin( terms[4] );

  if( pDEpsilon )
    *pDEpsilon = (92025. + 8.9 * t) * cos( terms[4] );

  for( int i=0; i<N_NUTATION_COEFFS; i++ ) {
    double totalArg = 0.;
    int mult = args[i][0];

    for( int j=4; j>=0; j--) {
      if( mult % 5 != 2)
        totalArg += (double)( mult % 5 - 2) * terms[j];
      mult /= 5;
    }

    double coeff = (double)(args[i][1]);
    if( i < 16 && 0 != timeDependent[i] )
      coeff += (double)(timeDependent[i]) * t / 10.;
    else if( 26 == i || 28 == i )
      coeff += (double)(27 - i) * t / 10.;

    if( pDPhi )
      *pDPhi += coeff * sin(totalArg);

    if( 0 != args[i][2] ) {
      coeff = (double)(args[i][2]);

      if( i < 9 && 0 != timeDependent[i + 16] )
        coeff += (double)(timeDependent[i + 16]) * t / 10.;

      if( pDEpsilon)
        *pDEpsilon += coeff * cos( totalArg );
    }
  }
  if( pDPhi )
    *pDPhi *= .0001;

  if( pDEpsilon )
    *pDEpsilon *= .0001;
}

/*----------------------------------------------------------------------------
 *  See p 84,  in Meeus:
 * The following returns apparent Greenwich sidereal time for the given jd
 */
double AstroOps::greenwichSiderealTime( double jd )
{
  jd -= Astro::J2000;      /* set relative to 2000.0 */
  double jdm = jd / Astro::TO_CENTURIES;  /* convert jd to julian centuries */
  double intPart = floor( jd );
  jd -= intPart;
  double rval = 280.46061837 +
                360.98564736629 * jd +
                .98564736629 * intPart +
                jdm * jdm * ( 3.87933e-4 - jdm / 38710000. );

  return Astro::toRadians( rval );
}

//----------------------------------------------------------------------------
// reduce an angle in degrees to (0 <= d < 360)
//
double AstroOps::normalizeDegrees(double d)
{
  d = fmod( d, Astro::DEG_PER_CIRCLE);
  if( d < 0.)
    d += Astro::DEG_PER_CIRCLE;

  return d;
}

//----------------------------------------------------------------------------
// reduce an angle in radians to (0 <= r < 2PI)
//
double AstroOps::normalizeRadians(double r)
{
  r = fmod( r, Astro::TWO_PI );
  if( r < 0. )
    r += Astro::TWO_PI;

  return r;
}

//----------------------------------------------------------------------------
