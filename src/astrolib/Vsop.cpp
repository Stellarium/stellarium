/*****************************************************************************\
 * Vsop.cpp
 *
 * The Vsop class wraps the VSOP87 data and provides VSOP support fns
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#include "Vsop.h"
#include "VsopData.h"
#include "AstroOps.h"
#include <math.h>

/*
 * This function, using the simplified VSOP87 data in Meeus, can compute
 * planetary positions in heliocentric ecliptic coordinates.
 *
 * 'planet' can run from 0=sun,  1=mercury,  ... 8=neptune.
 * (VSOP doesn't handle the moon or Pluto.)
 *
 * 'value' can be either
 *   0=ecliptic longitude, 1=ecliptic latitude, 2=distance from sun.
 *   (These are ecliptic coordinates _of date_,  by the way!)
 *
 * cen = (JD - 2451545.) / 36525. = difference from J2000,  in Julian
 * centuries.
 *
 * returns 0. if planet is <1 (mercury) or >8 (neptune)
 */
double Vsop::calcLoc(
    double t,         // time in decimal centuries
    PlanetNb planet,
    LocType ltype)
{
  double rval = 0.0;

  if( planet > SUN && planet < PLUTO ) {

    t /= 10.;          // convert to julian millenia
    double tPower = 1.0;

    const VsopTerms* pT;
    switch ( planet ) {
      case MERCURY: pT = &(MercuryTerms[ltype*6]); break;
      case VENUS:   pT = &(VenusTerms[ltype*6]);   break;
      case EARTH:   pT = &(EarthTerms[ltype*6]);   break;
      case MARS:    pT = &(MarsTerms[ltype*6]);    break;
      case JUPITER: pT = &(JupiterTerms[ltype*6]); break;
      case SATURN:  pT = &(SaturnTerms[ltype*6]);  break;
      case URANUS:  pT = &(UranusTerms[ltype*6]);  break;
      case NEPTUNE: pT = &(NeptuneTerms[ltype*6]); break;
      default: break;
    };

    // Always six series to calculate
    for( int i=0; i<6; i++ ) {
      double sum = 0.;
      const VsopSet* pv = pT->pTerms;

      // sum the term = A x cos( B + C x tc ) for each row
      for( unsigned j=0; j<pT->rows; j++ ) {
        sum += pv->A * cos( pv->B + pv->C * t );
        pv++;
      }
      // Add to series and bump multipler
      // i.e., L = L0*t + L1*t^2 + L2*t^3 + ...
      rval += sum * tPower;
      tPower *= t;
      pT++;
    }

    rval *= 1.e-8;  // rescale the term

    if( ECLIPTIC_LON == ltype )  /* ensure 0 < rval < 2PI  */
    {
      rval = AstroOps::normalizeRadians( rval );
    }
  }

  return rval;
}

