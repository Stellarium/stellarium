/*****************************************************************************\
 * MathOps.cpp
 *
 * MathOps contains misc. vector and matrix operations
 *
 * author: mark huss (mark@mhuss.com)
 * Based on Bill Gray's open-source code at projectpluto.com
 *
\*****************************************************************************/

#include "MathOps.h"

// rotate a vector along the spec'd axis
//
void MathOps::rotateVector( AstroVector& v, double angle, int axis )
{
  const double sinAng = sin( angle );
  const double cosAng = cos( angle );
  const int a = (axis + 1) % 3;
  const int b = (axis + 2) % 3;

  double temp = v[a] * cosAng - v[b] * sinAng;
  v[b] = v[b] * cosAng + v[a] * sinAng;
  v[a] = temp;
}

// convert polar coords to cartesian
//
void MathOps::polar3ToCartesian( AstroVector& v, double lon, double lat )
{
  const double cosLat = cos( lat );

  v[0] = cos( lon ) * cosLat;
  v[1] = sin( lon ) * cosLat;
  v[2] = sin( lat );
}

// Inverting an orthonormal matrix is simple:
// swap rows and columns, and you're done
//
void MathOps::invertOrthonormalMatrix( AstroMatrix& matrix )
{
   double temp=0;

   swapd( matrix[1], matrix[3], temp );
   swapd( matrix[2], matrix[6], temp );
   swapd( matrix[5], matrix[7], temp );
}

void MathOps::preSpinMatrix( AstroMatrix& m1, AstroMatrix& m2, double angle )
{
   const double sinAng = sin( angle );
   const double cosAng = cos( angle );
   double tval;

   for( int i=0; i<9; i+=3)
   {
      tval  = m1[i] * cosAng - m2[i] * sinAng;
      m2[i] = m2[i] * cosAng + m1[i] * sinAng;
      m1[i] = tval;
   }
}

void MathOps::spinMatrix( AstroMatrix& m1, AstroMatrix& m2, double angle )
{
   const double sinAng = sin( angle );
   const double cosAng = cos( angle );
   double tval;

   for( int i=0; i<3; i++ )
   {
      tval  = m1[i] * cosAng - m2[i] * sinAng;
      m2[i] = m2[i] * cosAng + m1[i] * sinAng;
      m1[i] = tval;
   }
}

