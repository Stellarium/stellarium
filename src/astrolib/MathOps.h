/*****************************************************************************\
 * MathOps.h
 *
 * MathOps contains misc. vector and matrix operations
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#if !defined( MATH_OPS__H )
#define MATH_OPS__H

#include <math.h>
#include "AstroOps.h"

class MathOps {

public:

  static double acose( double arg ) {
    if( arg >= 1. )
      return( 0. );
    else if( arg <= -1. )
      return( Astro::PI_ASTRO );
    else
      return( acos( arg ) );
  }

  static double asine( double arg ) {
    if( arg >= 1. )
      return( Astro::PI_ASTRO / 2. );
    else if( arg <= -1. )
      return( -Astro::PI_ASTRO / 2. );
    else
      return( asin( arg) );
  }

  static void setIdentityMatrix( AstroMatrix matrix ) {
    for( int i=0; i<9; i++ )
      matrix[i] = ( (i & 3) ? 0. : 1. );
  }

  // inline swap - 'tmp' is only there to avoid a local var
  static void swapd( double& a, double& b, double tmp )
      { tmp = a;  a = b;  b = tmp; }

  static void rotateVector( AstroVector& v, double angle, int axis );

  static void polar3ToCartesian( AstroVector& vect, double lon, double lat );

  static void invertOrthonormalMatrix( AstroMatrix& matrix );

  static void preSpinMatrix( AstroMatrix& v1, AstroMatrix& v2, double angle );

  static void spinMatrix( AstroMatrix& v1, AstroMatrix& v2, double angle );
};

#endif  /* #if !defined( MATH_OPS__H ) */
