/*
 *  kelso.h     April 9 2001
 *
 *  Header file for kelso
 */

#ifndef KELSO_H
#define KELSO_H 1

#ifdef __cplusplus
extern "C" {
#endif
    
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
/* #include <unistd.h> */

/* from David Kaelbling <drk@sgi.com> */
#define select duplicate_select
#include <unistd.h>
#undef select



/** Type definitions **/

/* Two-line-element satellite orbital data */
typedef struct
{
  double
    epoch, xndt2o, xndd6o, bstar, 
    xincl, xnodeo, eo, omegao, xmo, xno;
  int
    catnr,  /* Catalogue Number  */
    elset,  /* Element Set       */
    revnum; /* Revolution Number */
  char
    sat_name[25], /* Satellite name string    */
    idesg[9];     /* International Designator */
  /* values needed for squint calculations */
  double xincl1, xnodeo1, omegao1;
} tle_t; 


/* Geodetic position structure */
typedef struct
{
  double
  lat, lon, alt, theta;
} geodetic_t;

/* General three-dimensional vector structure */
typedef struct
{
  double
  x, y, z, w;
} vector_t;

/* Common arguments between deep-space functions */
typedef struct
{
  /* Used by dpinit part of Deep() */
  double
    eosq,sinio,cosio,betao,aodp,theta2,sing,cosg,
    betao2,xmdot,omgdot,xnodot,xnodp;
  /* Used by dpsec and dpper parts of Deep() */
  double
    xll,omgadf,xnode,em,xinc,xn,t;
  /* Used by thetg and Deep() */
  double
    ds50;
} deep_arg_t;

/** Table of constant values **/
#define de2ra    1.74532925E-2   /* Degrees to Radians */
#define pi       3.1415926535898 /* Pi */
#define pio2     1.5707963267949 /* Pi/2 */
#define x3pio2   4.71238898      /* 3*Pi/2 */
#define twopi    6.2831853071796 /* 2*Pi  */
#define e6a      1.0E-6
#define tothrd   6.6666667E-1    /* 2/3 */
#define xj2      1.0826158E-3    /* J2 Harmonic */
#define xj3     -2.53881E-6      /* J3 Harmonic */   
#define xj4     -1.65597E-6      /* J4 Harmonic */
#define xke      7.43669161E-2
#define xkmper   6.378135E3      /* Earth radius km */
#define xmnpda   1.44E3          /* Minutes per day */
#define ae       1.0
#define ck2      5.413079E-4
#define ck4      6.209887E-7
#define __f      3.352779E-3
#define ge       3.986008E5 
#define __s__    1.012229
#define qoms2t   1.880279E-09
#define secday   8.6400E4        /* Seconds per day */
#define omega_E  1.0027379
#define omega_ER 6.3003879
#define zns      1.19459E-5
#define c1ss     2.9864797E-6
#define zes      1.675E-2
#define znl      1.5835218E-4
#define c1l      4.7968065E-7
#define zel      5.490E-2
#define zcosis   9.1744867E-1
#define zsinis   3.9785416E-1
#define zsings  -9.8088458E-1
#define zcosgs   1.945905E-1
#define zcoshs   1
#define zsinhs   0
#define q22      1.7891679E-6
#define q31      2.1460748E-6
#define q33      2.2123015E-7
#define g22      5.7686396
#define g32      9.5240898E-1
#define g44      1.8014998
#define g52      1.0508330
#define g54      4.4108898
#define root22   1.7891679E-6
#define root32   3.7393792E-7
#define root44   7.3636953E-9
#define root52   1.1428639E-7
#define root54   2.1765803E-9
#define thdt     4.3752691E-3
#define rho      1.5696615E-1
#define mfactor  7.292115E-5
#define __sr__       6.96000E5      /*Solar radius - kilometers (IAU 76)*/
#define SGPAU       1.49597870E8   /*Astronomical unit - kilometers (IAU 76)*/

/* Entry points of Deep() */
#define dpinit   1 /* Deep-space initialization code */
#define dpsec    2 /* Deep-space secular code        */
#define dpper    3 /* Deep-space periodic code       */

/* Carriage return and line feed */
#define CR  0x0A
#define LF  0x0D

/* Flow control flag definitions */
#define ALL_FLAGS              -1
#define  SGP_INITIALIZED_FLAG  0x000001
#define SGP4_INITIALIZED_FLAG  0x000002
#define SDP4_INITIALIZED_FLAG  0x000004
#define SGP8_INITIALIZED_FLAG  0x000008
#define SDP8_INITIALIZED_FLAG  0x000010
#define SIMPLE_FLAG            0x000020
#define DEEP_SPACE_EPHEM_FLAG  0x000040
#define LUNAR_TERMS_DONE_FLAG  0x000080
#define NEW_EPHEMERIS_FLAG     0x000100
#define DO_LOOP_FLAG           0x000200
#define RESONANCE_FLAG         0x000400
#define SYNCHRONOUS_FLAG       0x000800
#define EPOCH_RESTART_FLAG     0x001000
#define VISIBLE_FLAG           0x002000
#define SAT_ECLIPSED_FLAG      0x004000


/** Funtion prototypes **/

/* main.c */
/* int     main(void); */
/* sgp4sdp4.c */
void    SGP4(double tsince, tle_t *tle, vector_t *pos, vector_t *vel, double* phase);
void    SDP4(double tsince, tle_t *tle, vector_t *pos, vector_t *vel, double* phase);
void    Deep(int ientry, tle_t *tle, deep_arg_t *deep_arg);
int     isFlagSet(int flag);
int     isFlagClear(int flag);
void    SetFlag(int flag);
void    ClearFlag(int flag);
/* sgp_in.c */
int     Checksum_Good(char *tle_set);
int     Good_Elements(char *tle_set);
void    Convert_Satellite_Data(char *tle_set, tle_t *tle);
int     Get_Next_Tle_Set( char lines[3][80], tle_t *tle );
void    select_ephemeris(tle_t *tle);
/* sgp_math.c */
int     Sign(double arg);
double  Sqr(double arg);
double  Cube(double arg);
double  Radians(double arg);
double  Degrees(double arg);
double  ArcSin(double arg);
double  ArcCos(double arg);
void    SgpMagnitude(vector_t *v);
void    Vec_Add(vector_t *v1, vector_t *v2, vector_t *v3);
void    Vec_Sub(vector_t *v1, vector_t *v2, vector_t *v3);
void    Scalar_Multiply(double k, vector_t *v1, vector_t *v2);
void    Scale_Vector(double k, vector_t *v);
double  Dot(vector_t *v1, vector_t *v2);
double  Angle(vector_t *v1, vector_t *v2);
void    Cross(vector_t *v1, vector_t *v2, vector_t *v3);
void    Normalize(vector_t *v);
double  AcTan(double sinx, double cosx);
double  FMod2p(double x);
double  Modulus(double arg1, double arg2);
double  Frac(double arg);
int     Round(double arg);
double  Int(double arg);
void    Convert_Sat_State(vector_t *pos, vector_t *vel);
/* sgp_obs.c */
void    Calculate_User_PosVel(double _time, geodetic_t *geodetic,
                              vector_t *obs_pos, vector_t *obs_vel);
void    Calculate_LatLonAlt(double _time, vector_t *pos, geodetic_t *geodetic);
void    Calculate_Obs(double _time, vector_t *pos, vector_t *vel,
                      geodetic_t *geodetic, vector_t *obs_set);
void    Calculate_RADec(double _time, vector_t *pos, vector_t *vel,
                        geodetic_t *geodetic, vector_t *obs_set);
/* sgp_time.c */
double  Julian_Date_of_Epoch(double epoch);
double  Epoch_Time(double jd);
int     DOY(int yr, int mo, int dy);
double  Fraction_of_Day(int hr, int mi, int se);
void    Calendar_Date(double jd, struct tm *cdate);
void    Time_of_Day(double jd, struct tm *cdate);
double  Julian_Date(struct tm *cdate);
void    Date_Time(double jd, struct tm *cdate);
int     Check_Date(struct tm *cdate);
struct tm Time_to_UTC(struct tm *cdate);
struct tm Time_from_UTC(struct tm *cdate);
double  JD_to_UTC(double jt);
double  JD_from_UTC(double jt);
double  Delta_ET(double year);
double  Julian_Date_of_Year(double year);
double  ThetaG(double epoch, deep_arg_t *deep_arg);
double  ThetaG_JD(double jd);
void    UTC_Calendar_Now(struct tm *cdate);
/* solar.c */
void    Calculate_Solar_Position(double _time, vector_t *solar_vector);
int     Sat_Eclipsed(vector_t *pos, vector_t *sol, double *depth);

#ifdef __cplusplus
}
#endif

#endif
