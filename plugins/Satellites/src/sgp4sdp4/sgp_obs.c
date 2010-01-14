/*
 * Unit SGP_Obs
 *           Author:  Dr TS Kelso 
 * Original Version:  1992 Jun 02 
 * Current Revision:  1992 Sep 28 
 *          Version:  1.40 
 *        Copyright:  1992, All Rights Reserved 
 *
 *   Ported to C by:  Neoklis Kyriazis  April 9 2001
 */

#include "sgp4sdp4.h"

/* Procedure Calculate_User_PosVel passes the user's geodetic position */
/* and the time of interest and returns the ECI position and velocity  */
/* of the observer. The velocity calculation assumes the geodetic      */
/* position is stationary relative to the earth's surface.             */
void
Calculate_User_PosVel(double _time,
                      geodetic_t *geodetic,
                      vector_t *obs_pos,
                      vector_t *obs_vel)
{
/* Reference:  The 1992 Astronomical Almanac, page K11. */

  double c,sq,achcp;

  geodetic->theta = FMod2p(ThetaG_JD(_time) + geodetic->lon);/*LMST*/
  c = 1/sqrt(1 + __f*(__f - 2)*Sqr(sin(geodetic->lat)));
  sq = Sqr(1 - __f)*c;
  achcp = (xkmper*c + geodetic->alt)*cos(geodetic->lat);
  obs_pos->x = achcp*cos(geodetic->theta);/*kilometers*/
  obs_pos->y = achcp*sin(geodetic->theta);
  obs_pos->z = (xkmper*sq + geodetic->alt)*sin(geodetic->lat);
  obs_vel->x = -mfactor*obs_pos->y;/*kilometers/second*/
  obs_vel->y =  mfactor*obs_pos->x;
  obs_vel->z =  0;
  SgpMagnitude(obs_pos);
  SgpMagnitude(obs_vel);
} /*Procedure Calculate_User_PosVel*/

/*------------------------------------------------------------------*/

/* Procedure Calculate_LatLonAlt will calculate the geodetic  */
/* position of an object given its ECI position pos and time. */
/* It is intended to be used to determine the ground track of */
/* a satellite.  The calculations  assume the earth to be an  */
/* oblate spheroid as defined in WGS '72.                     */
void
Calculate_LatLonAlt(double _time, vector_t *pos,  geodetic_t *geodetic)
{
  /* Reference:  The 1992 Astronomical Almanac, page K12. */

  double r,e2,phi,c;

  geodetic->theta = AcTan(pos->y,pos->x);/*radians*/
  geodetic->lon = FMod2p(geodetic->theta - ThetaG_JD(_time));/*radians*/
  r = sqrt(Sqr(pos->x) + Sqr(pos->y));
  e2 = __f*(2 - __f);
  geodetic->lat = AcTan(pos->z,r);/*radians*/

  do
    {
      phi = geodetic->lat;
      c = 1/sqrt(1 - e2*Sqr(sin(phi)));
      geodetic->lat = AcTan(pos->z + xkmper*c*e2*sin(phi),r);
    }
  while(fabs(geodetic->lat - phi) >= 1E-10);

  geodetic->alt = r/cos(geodetic->lat) - xkmper*c;/*kilometers*/

  if( geodetic->lat > pio2 ) geodetic->lat -= twopi;
  
} /*Procedure Calculate_LatLonAlt*/

/*------------------------------------------------------------------*/

/* The procedures Calculate_Obs and Calculate_RADec calculate         */
/* the *topocentric* coordinates of the object with ECI position,     */
/* {pos}, and velocity, {vel}, from location {geodetic} at {time}.    */
/* The {obs_set} returned for Calculate_Obs consists of azimuth,      */
/* elevation, range, and range rate (in that order) with units of     */
/* radians, radians, kilometers, and kilometers/second, respectively. */
/* The WGS '72 geoid is used and the effect of atmospheric refraction */
/* (under standard temperature and pressure) is incorporated into the */
/* elevation calculation; the effect of atmospheric refraction on     */
/* range and range rate has not yet been quantified.                  */

/* The {obs_set} for Calculate_RADec consists of right ascension and  */
/* declination (in that order) in radians.  Again, calculations are   */
/* based on *topocentric* position using the WGS '72 geoid and        */
/* incorporating atmospheric refraction.                              */

void
Calculate_Obs(double _time,
                vector_t *pos,
                vector_t *vel,
                geodetic_t *geodetic,
                vector_t *obs_set)
  {
   double
     sin_lat,cos_lat,
     sin_theta,cos_theta,
     el,azim,
     top_s,top_e,top_z;

   vector_t
     obs_pos,obs_vel,range,rgvel;

  Calculate_User_PosVel(_time, geodetic, &obs_pos, &obs_vel);

    range.x = pos->x - obs_pos.x;
    range.y = pos->y - obs_pos.y;
    range.z = pos->z - obs_pos.z;

    rgvel.x = vel->x - obs_vel.x;
    rgvel.y = vel->y - obs_vel.y;
    rgvel.z = vel->z - obs_vel.z;

  SgpMagnitude(&range);

  sin_lat = sin(geodetic->lat);
  cos_lat = cos(geodetic->lat);
  sin_theta = sin(geodetic->theta);
  cos_theta = cos(geodetic->theta);
  top_s = sin_lat*cos_theta*range.x
         + sin_lat*sin_theta*range.y
         - cos_lat*range.z;
  top_e = -sin_theta*range.x
         + cos_theta*range.y;
  top_z = cos_lat*cos_theta*range.x
         + cos_lat*sin_theta*range.y
         + sin_lat*range.z;
  azim = atan(-top_e/top_s); /*Azimuth*/
  if( top_s > 0 ) 
    azim = azim + pi;
  if( azim < 0 )
    azim = azim + twopi;
  el = ArcSin(top_z/range.w);
  obs_set->x = azim;      /* Azimuth (radians)  */
  obs_set->y = el;        /* Elevation (radians)*/
  obs_set->z = range.w; /* Range (kilometers) */

 /*Range Rate (kilometers/second)*/
  obs_set->w = Dot(&range, &rgvel)/range.w;

/* Corrections for atmospheric refraction */
/* Reference:  Astronomical Algorithms by Jean Meeus, pp. 101-104    */
/* Correction is meaningless when apparent elevation is below horizon */
  obs_set->y = obs_set->y + Radians((1.02/tan(Radians(Degrees(el)+
               10.3/(Degrees(el)+5.11))))/60);
  if( obs_set->y >= 0 )
    SetFlag(VISIBLE_FLAG);
  else
    {
    obs_set->y = el;  /*Reset to true elevation*/
    ClearFlag(VISIBLE_FLAG);
    } /*else*/
  } /*Procedure Calculate_Obs*/

/*------------------------------------------------------------------*/

void
Calculate_RADec( double _time,
                 vector_t *pos,
                 vector_t *vel,
                 geodetic_t *geodetic,
                 vector_t *obs_set)
{
/* Reference:  Methods of Orbit Determination by  */
/*                Pedro Ramon Escobal, pp. 401-402 */

double
    phi,theta,sin_theta,cos_theta,sin_phi,cos_phi,
    az,el,Lxh,Lyh,Lzh,Sx,Ex,Zx,Sy,Ey,Zy,Sz,Ez,Zz,
    Lx,Ly,Lz,cos_delta,sin_alpha,cos_alpha;

  Calculate_Obs(_time,pos,vel,geodetic,obs_set);

/*  if( isFlagSet(VISIBLE_FLAG) )
    {*/
    az = obs_set->x;
    el = obs_set->y;
    phi   = geodetic->lat;
    theta = FMod2p(ThetaG_JD(_time) + geodetic->lon);
    sin_theta = sin(theta);
    cos_theta = cos(theta);
    sin_phi = sin(phi);
    cos_phi = cos(phi);
    Lxh = -cos(az)*cos(el);
    Lyh =  sin(az)*cos(el);
    Lzh =  sin(el);
    Sx = sin_phi*cos_theta;
    Ex = -sin_theta;
    Zx = cos_theta*cos_phi;
    Sy = sin_phi*sin_theta;
    Ey = cos_theta;
    Zy = sin_theta*cos_phi;
    Sz = -cos_phi;
    Ez = 0;
    Zz = sin_phi;
    Lx = Sx*Lxh + Ex*Lyh + Zx*Lzh;
    Ly = Sy*Lxh + Ey*Lyh + Zy*Lzh;
    Lz = Sz*Lxh + Ez*Lyh + Zz*Lzh;
    obs_set->y = ArcSin(Lz);  /*Declination (radians)*/
    cos_delta = sqrt(1 - Sqr(Lz));
    sin_alpha = Ly/cos_delta;
    cos_alpha = Lx/cos_delta;
    obs_set->x = AcTan(sin_alpha,cos_alpha); /*Right Ascension (radians)*/
    obs_set->x = FMod2p(obs_set->x);
    /*}*/  /*if*/
  } /* Procedure Calculate_RADec */

/*------------------------------------------------------------------*/
