/*
 * Unit Solar
 *           Author:  Dr TS Kelso
 * Original Version:  1990 Jul 29
 * Current Revision:  1999 Nov 27
 *          Version:  1.30
 *        Copyright:  1990-1999, All Rights Reserved
 *
 *   Ported to C by: Neoklis Kyriazis  April 1 2001
 */

#include "sgp4sdp4.h"

/* Calculates solar position vector */
void
Calculate_Solar_Position(double _time, vector_t *solar_vector)
{
  double mjd,year,T,M,L,e,C,O,Lsa,nu,R,eps;

  mjd = _time - 2415020.0;
  year = 1900 + mjd/365.25;
  T = (mjd + Delta_ET(year)/secday)/36525.0;
  M = Radians(Modulus(358.47583 + Modulus(35999.04975*T,360.0)
		      - (0.000150 + 0.0000033*T)*Sqr(T),360.0));
  L = Radians(Modulus(279.69668 + Modulus(36000.76892*T,360.0)
		      + 0.0003025*Sqr(T),360.0));
  e = 0.01675104 - (0.0000418 + 0.000000126*T)*T;
  C = Radians((1.919460 - (0.004789 + 0.000014*T)*T)*sin(M)
	      + (0.020094 - 0.000100*T)*sin(2*M) + 0.000293*sin(3*M));
  O = Radians(Modulus(259.18 - 1934.142*T,360.0));
  Lsa = Modulus(L + C - Radians(0.00569 - 0.00479*sin(O)),twopi);
  nu = Modulus(M + C,twopi);
  R = 1.0000002*(1 - Sqr(e))/(1 + e*cos(nu));
  eps = Radians(23.452294 - (0.0130125 + (0.00000164 -
		0.000000503*T)*T)*T + 0.00256*cos(O));
  R = SGPAU*R;
  solar_vector->x = R*cos(Lsa);
  solar_vector->y = R*sin(Lsa)*cos(eps);
  solar_vector->z = R*sin(Lsa)*sin(eps);
  solar_vector->w = R;
} /*Procedure Calculate_Solar_Position*/

/*------------------------------------------------------------------*/

/* Calculates stellite's eclipse status and depth */
int
Sat_Eclipsed(vector_t *pos, vector_t *sol, double *depth)
{
  double sd_sun, sd_earth, delta;
  vector_t Rho, earth;

  /* Determine partial eclipse */
  sd_earth = ArcSin(xkmper/pos->w);
  Vec_Sub(sol,pos,&Rho);
  sd_sun = ArcSin(__sr__/Rho.w);
  Scalar_Multiply(-1,pos,&earth);
  delta = Angle(sol,&earth);
  *depth = sd_earth - sd_sun - delta;
  if( sd_earth < sd_sun )
    return( 0 );
  else
    if( *depth >= 0 )
      return( 1 );
    else
      return( 0 );

} /*Function Sat_Eclipsed*/

/*------------------------------------------------------------------*/
