/*****************************************************************************\
 * Pluto.cpp
 *
 * Pluto is a class that can calculate the orbit of Pluto.
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#include <math.h>
#include "AstroOps.h"
#include "Pluto.h"
#include "PlutoTerms.h"  // data extracted from vsop.bin file

// t is in julian centuries from J2000.0

void Pluto::calcAllLocs( double& lat, double& lon, double& rad, const double t) {

  // jupiter's mean longitude
  double mlJup =  Astro::toRadians(34.35 + 3034.9057 * t);

  // saturn's mean longitude
  double mlSat =  Astro::toRadians(50.08 + 1222.1138 * t);

  // pluto's mean longitude
  double mlPl = Astro::toRadians(238.96 +  144.9600 * t);

  // use local vars for retvals, hoping to encourage FP reg optimizations
  double lon_ = 238.956785 + 144.96 * t;
  double lat_ = -3.908202;
  double rad_ = 407.247248;  // temporarily in tenths of AUs; fixed at the end

  double arg;
  for( int ii=0; ii<7; ii++ ) {
    if( ii == 6)
      arg = mlJup - mlPl;
    else
      arg = (double)(ii + 1) * mlPl;

    double cosArg = cos(arg) * 1.e-6;
    double sinArg = sin(arg) * 1.e-6;
    long* plc = plutoLongCoeff[ii];

    lon_ += (double)(plc[0]) * sinArg + (double)(plc[1]) * cosArg;
    lat_ += (double)(plc[2]) * sinArg + (double)(plc[3]) * cosArg;
    rad_   += (double)(plc[4]) * sinArg + (double)(plc[5]) * cosArg;
  }

  PlutoCoeffs* pc = plutoCoeff;
  for( int i=0; i<N_COEFFS; i++ ) {
    if( pc->lon_a || pc->lon_b ||
        pc->lat_a || pc->lat_b ||
        pc->rad_a || pc->rad_b)
    {
      if( 0 == pc->j )
        arg = 0.;
      else
        arg = ((1 == pc->j) ? mlJup : mlJup * (double)pc->j);

      if( pc->s < 0)
        arg -= (-1 == pc->s) ? mlSat : mlSat + mlSat;

      if( pc->s > 0)
        arg += (1 == pc->s) ? mlSat : mlSat + mlSat;

      if( pc->p)
        arg += mlPl * (double)pc->p;

      double cosArg = cos(arg) * 1.e-6;
      double sinArg = sin(arg) * 1.e-6;
      lon_ += sinArg * (double)(pc->lon_a) + cosArg * (double)(pc->lon_b);
      lat_ += sinArg * (double)(pc->lat_a) + cosArg * (double)(pc->lat_b);
      rad_   += sinArg * (double)(pc->rad_a) + cosArg * (double)(pc->rad_b);
    }
    pc++;
  }
  lon = Astro::toRadians(lon_);
  lat = Astro::toRadians(lat_);
  rad = rad_ / 10.;                  // convert back to AUs
}

