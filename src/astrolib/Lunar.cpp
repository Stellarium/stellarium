/*****************************************************************************\
 * Lunar.cpp
 *
 * Lunar is a class that can calculate lunar fundmentals for any reasonable
 *   time.
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#include "Lunar.h"
#include "LunarTerms.h"  // data extracted from vsop.bin file

#include "MathOps.h"
#include "AstroOps.h"

#include <stdlib.h>

//----------------------------------------------------------------------------
// calculate an individual fundimental
//  tptr - points to array of doubles
//  t - time in decimal Julian centuries
//
double Lunar::getFund( const double* tptr, double t )
{
  double d = *tptr++;
  double tpow = t;
  for( int i=4; i!=0; i-- ) {
    d += tpow * (*tptr++);
    tpow *= t;
  }
  return normalize( d );
}

//----------------------------------------------------------------------------
// calculate the fundamanentals given the vsop.bin data and a time
//   ad has vsop.bin data
//   t = decimal julian centuries
//
void Lunar::calcFundamentals( double t )
{
  m_f.Lp = getFund( LunarFundimentals_Lp, t );
  m_f.D = getFund( LunarFundimentals_D, t );
  m_f.M = getFund( LunarFundimentals_M, t );
  m_f.Mp = getFund( LunarFundimentals_Mp, t );
  m_f.F = getFund( LunarFundimentals_F, t );

  m_f.A1 = normalize( 119.75 + 131.849 * t );
  m_f.A2 = normalize( 53.09 + 479264.290 * t );
  m_f.A3 = normalize( 313.45 + 481266.484 * t );
  m_f.T  = normalize( t );

  // indicate values need to be recalculated
  m_lat = m_lon = m_r = -1.;

  // set init'd flag to true
  m_initialized = true;
}

//----------------------------------------------------------------------------
// calculate longitude and radius
//
// NOTE: calcFundamentals() must have been called first
//
void Lunar::calcLonRad()
{
  if ( !m_initialized ) {
    m_r = m_lon = -1.;
  }
  else {
    const LunarTerms1* tptr = LunarLonRad;

    double sl = 0., sr = 0.;
    const double e = 1. - .002516 * m_f.T - .0000074 * m_f.T * m_f.T;

    for( int i=N_LTERM1; i!=0; i-- ) {
      if( labs( tptr->sl ) > 0 || labs( tptr->sr ) > 0 ) {
        double arg;

        switch( tptr->d )
        {
          case  1:   arg = m_f.D;          break;
          case -1:   arg =-m_f.D;          break;
          case  2:   arg = m_f.D+m_f.D;    break;
          case -2:   arg =-m_f.D-m_f.D;    break;
          case  0:   arg = 0.;             break;
          default:   arg = (double)(tptr->d) * m_f.D;  break;
        }

        switch( tptr->m )
        {
          case  1:   arg += m_f.M;         break;
          case -1:   arg -= m_f.M;         break;
          case  2:   arg += m_f.M+m_f.M;   break;
          case -2:   arg -= m_f.M+m_f.M;   break;
          case  0:           ;             break;
          default:   arg += (double)(tptr->m) * m_f.M;  break;
        }

        switch( tptr->mp )
        {
          case  1:   arg += m_f.Mp;        break;
          case -1:   arg -= m_f.Mp;        break;
          case  2:   arg += m_f.Mp+m_f.Mp; break;
          case -2:   arg -= m_f.Mp+m_f.Mp; break;
          case  0:           ;             break;
          default:   arg += (double)(tptr->mp) * m_f.Mp;  break;
        }

        switch( tptr->f )
        {
          case  1:   arg += m_f.F;         break;
          case -1:   arg -= m_f.F;         break;
          case  2:   arg += m_f.F+m_f.F;   break;
          case -2:   arg -= m_f.F+m_f.F;   break;
          case  0:           ;             break;
          default:   arg += (double)(tptr->f) * m_f.F;  break;
        }

        if( tptr->sl )
        {
          double term = (double)(tptr->sl) * sin(arg);
          for( int j=abs(tptr->m); j!=0; j-- )
            term *= e;
          sl += term;
        }

        if( tptr->sr )
        {
          double term = (double)(tptr->sr) * cos(arg);
          for( int j=abs(tptr->m); j!=0; j-- )
            term *= e;
          sr += term;
        }
      }
      tptr++;
    }

    sl += 3958. * sin( m_f.A1 ) +
          1962. * sin( m_f.Lp - m_f.F ) +
          318.  * sin( m_f.A2 );

    m_lon = (m_f.Lp * 180. / Astro::PI_ASTRO) + sl * 1.e-6;

    // reduce signed angle to ( 0 < m_lon < 360 )
    m_lon = AstroOps::normalizeDegrees( m_lon );
    m_r = 385000.56 + sr / 1000.;
  }
}

//----------------------------------------------------------------------------
// calculate (or return prev. calculated) latitude
//
// NOTE: calcFundamentals() must have been called first
//
double Lunar::latitude()
{
  if ( !m_initialized )
    return -1.;

  if ( m_lat < 0. ) {
    const LunarTerms2* tptr = LunarLat;
    double rval = 0.;

    const double e = 1. - .002516 * m_f.T - .0000074 * m_f.T * m_f.T;

    for( int i=N_LTERM2; i!=0; i-- ) {

      if( labs( tptr->sb ) > 0. ) {
        double arg;

        switch( tptr->d )
        {
          case  1:   arg = m_f.D;          break;
          case -1:   arg =-m_f.D;          break;
          case  2:   arg = m_f.D+m_f.D;    break;
          case -2:   arg =-m_f.D-m_f.D;    break;
          case  0:   arg = 0.;             break;
          default:   arg = (double)(tptr->d) * m_f.D;  break;
        }

        switch( tptr->m )
        {
          case  1:   arg += m_f.M;         break;
          case -1:   arg -= m_f.M;         break;
          case  2:   arg += m_f.M+m_f.M;   break;
          case -2:   arg -= m_f.M+m_f.M;   break;
          case  0:           ;             break;
          default:   arg += (double)(tptr->m) * m_f.M;  break;
        }

        switch( tptr->mp )
        {
          case  1:   arg += m_f.Mp;        break;
          case -1:   arg -= m_f.Mp;        break;
          case  2:   arg += m_f.Mp+m_f.Mp; break;
          case -2:   arg -= m_f.Mp+m_f.Mp; break;
          case  0:           ;             break;
          default:   arg += (double)(tptr->mp) * m_f.Mp;  break;
        }

        switch( tptr->f )
        {
          case  1:   arg += m_f.F;         break;
          case -1:   arg -= m_f.F;         break;
          case  2:   arg += m_f.F+m_f.F;   break;
          case -2:   arg -= m_f.F+m_f.F;   break;
          case  0:           ;             break;
          default:   arg += (double)(tptr->f) * m_f.F;  break;
        }

        double term = (double)(tptr->sb) * sin( arg );
        for( int j = abs(tptr->m); j!=0; j-- )
          term *= e;

        rval += term;
      }
      rval += -2235. * sin( m_f.Lp ) +
               382.  * sin( m_f.A3 ) +
               175.  * sin( m_f.A1 - m_f.F ) +
               175.  * sin( m_f.A1 + m_f.F ) +
               127.  * sin( m_f.Lp - m_f.Mp ) -
               115.  * sin( m_f.Lp + m_f.Mp );

      tptr++;
    }
    m_lat = rval * 1.e-6;
  }
  return m_lat;
}

