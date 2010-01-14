/*
 * Unit SGP_Math
 *       Author:  Dr TS Kelso
 * Original Version:  1991 Oct 30
 * Current Revision:  1998 Mar 17
 *          Version:  3.00
 *        Copyright:  1991-1998, All Rights Reserved
 *
 *   ported to C by:  Neoklis Kyriazis  April 9 2001
 */

#include "sgp4sdp4.h"

/* Returns sign of a double */
int
Sign(double arg)
{
  if( arg > 0 )
    return( 1 );
  else if( arg < 0 )
    return( -1 );
  else
    return( 0 );
} /* Function Sign*/

/*------------------------------------------------------------------*/

/* Returns square of a double */
double
Sqr(double arg)
{
  return( arg*arg );
} /* Function Sqr */

/*------------------------------------------------------------------*/

/* Returns cube of a double */
double
Cube(double arg)
{
  return( arg*arg*arg );
} /*Function Cube*/

/*------------------------------------------------------------------*/

/* Returns angle in radians from arg id degrees */
double
Radians(double arg)
{
  return( arg*de2ra );
} /*Function Radians*/

/*------------------------------------------------------------------*/

/* Returns angle in degrees from arg in rads */
double
Degrees(double arg)
{
  return( arg/de2ra );
} /*Function Degrees*/

/*------------------------------------------------------------------*/

/* Returns the arcsine of the argument */
double
ArcSin(double arg)
{
  if( fabs(arg) >= 1 )
    return( Sign(arg)*pio2 );
  else
    return( atan(arg/sqrt(1-arg*arg)) );
} /*Function ArcSin*/

/*------------------------------------------------------------------*/

/* Returns orccosine of rgument */
double
ArcCos(double arg)
{
  return( pio2 - ArcSin(arg) );
} /*Function ArcCos*/

/*------------------------------------------------------------------*/

/* Calculates scalar magnitude of a vector_t argument */
void
SgpMagnitude(vector_t *v)
{
  v->w = sqrt(Sqr(v->x) + Sqr(v->y) + Sqr(v->z));
} /*Procedure SgpMagnitude*/

/*------------------------------------------------------------------*/

/* Adds vectors v1 and v2 together to produce v3 */
void
Vec_Add(vector_t *v1, vector_t *v2, vector_t *v3)
{
  v3->x = v1->x + v2->x;
  v3->y = v1->y + v2->y;
  v3->z = v1->z + v2->z;

  SgpMagnitude(v3);
} /*Procedure Vec_Add*/

/*------------------------------------------------------------------*/

/* Subtracts vector v2 from v1 to produce v3 */
void
Vec_Sub(vector_t *v1, vector_t *v2, vector_t *v3)
{
  v3->x = v1->x - v2->x;
  v3->y = v1->y - v2->y;
  v3->z = v1->z - v2->z;

  SgpMagnitude(v3);
} /*Procedure Vec_Sub*/

/*------------------------------------------------------------------*/

/* Multiplies the vector v1 by the scalar k to produce the vector v2 */
void
Scalar_Multiply(double k, vector_t *v1, vector_t *v2)
{
  v2->x = k * v1->x;
  v2->y = k * v1->y;
  v2->z = k * v1->z;
  v2->w = fabs(k) * v1->w;
} /*Procedure Scalar_Multiply*/

/*------------------------------------------------------------------*/

/* Multiplies the vector v1 by the scalar k */
void
Scale_Vector(double k, vector_t *v)
{ 
  v->x *= k;
  v->y *= k;
  v->z *= k;
  SgpMagnitude(v);
} /* Procedure Scale_Vector */

/*------------------------------------------------------------------*/

/* Returns the dot product of two vectors */
double
Dot(vector_t *v1, vector_t *v2)
{
  return( v1->x*v2->x + v1->y*v2->y + v1->z*v2->z );
}  /*Function Dot*/

/*------------------------------------------------------------------*/

/* Calculates the angle between vectors v1 and v2 */
double
Angle(vector_t *v1, vector_t *v2)
{
  SgpMagnitude(v1);
  SgpMagnitude(v2);
  return( ArcCos(Dot(v1,v2)/(v1->w*v2->w)) );
} /*Function Angle*/

/*------------------------------------------------------------------*/

/* Produces cross product of v1 and v2, and returns in v3 */
void
Cross(vector_t *v1, vector_t *v2 ,vector_t *v3)
{
  v3->x = v1->y*v2->z - v1->z*v2->y;
  v3->y = v1->z*v2->x - v1->x*v2->z;
  v3->z = v1->x*v2->y - v1->y*v2->x;
  SgpMagnitude(v3);
} /*Procedure Cross*/

/*------------------------------------------------------------------*/

/* Normalizes a vector */
void
Normalize( vector_t *v )
{
  v->x /= v->w;
  v->y /= v->w;
  v->z /= v->w;
} /*Procedure Normalize*/

/*------------------------------------------------------------------*/

/* Four-quadrant arctan function */
double
AcTan(double sinx, double cosx)
{
  if(cosx == 0)
    {
      if(sinx > 0)
	return (pio2);
      else
	return (x3pio2);
    }
  else
    {
      if(cosx > 0)
	{
	  if(sinx > 0)
	    return ( atan(sinx/cosx) );
	  else
	    return ( twopi + atan(sinx/cosx) );
	}
      else
	return ( pi + atan(sinx/cosx) );
    }

} /* Function AcTan */

/*------------------------------------------------------------------*/

/* Returns mod 2pi of argument */
double
FMod2p(double x)
{
  int i;
  double ret_val;

  ret_val = x;
  i = ret_val/twopi;
  ret_val -= i*twopi;
  if (ret_val < 0) ret_val += twopi;

  return (ret_val);
} /* fmod2p */

/*------------------------------------------------------------------*/

/* Returns arg1 mod arg2 */
double
Modulus(double arg1, double arg2)
{
  int i;
  double ret_val;

  ret_val = arg1;
  i = ret_val/arg2;
  ret_val -= i*arg2;
  if (ret_val < 0) ret_val += arg2;

  return (ret_val);
} /* modulus */

/*------------------------------------------------------------------*/

/* Returns fractional part of double argument */
double
Frac( double arg )
{
  return( arg - floor(arg) );
} /* Frac */

/*------------------------------------------------------------------*/

/* Returns argument rounded up to nearest integer */
int
Round( double arg )
{
  return( (int) floor(arg + 0.5) );
} /* Round */

/*------------------------------------------------------------------*/

/* Returns the floor integer of a double arguement, as double */
double
Int( double arg )
{
  return( floor(arg) );
} /* Int */

/*------------------------------------------------------------------*/

/* Converts the satellite's position and velocity  */
/* vectors from normalised values to km and km/sec */ 
void
Convert_Sat_State( vector_t *pos, vector_t *vel )
{
      Scale_Vector( xkmper, pos );
      Scale_Vector( xkmper*xmnpda/secday, vel );

} /* Procedure Convert_Sat_State */

/*------------------------------------------------------------------*/
