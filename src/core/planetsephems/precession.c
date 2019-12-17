/*
Copyright (C) 2015 Georg Zotti

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Library General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

/*
 * Precession solution following methods from:
 * J. Vondrak, N. Capitaine, P. Wallace
 * New precession expressions, valid for long time intervals
 * Astronomy&Astrophysics 534, A22 (2011)
 * DOI: 10.1051/0004-6361/201117274
 *   with correction from
 * J. Vondrak, N. Capitaine, P. Wallace
 * New precession expressions, valid for long time intervals (Corrigendum)
 * A&A 541, C1 (2012)
 * DOI: 10.1051/0004-6361/201117274e
 * The data are only applicable for a time range of 200.000 years around J2000.
 * This is by far enough for Stellarium as of 2015, but just to make sure I added a few asserts.
 */

#include <math.h>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

/* Interval threshold (days) for re-computing these values. with 1, compute only 1/day:  */
#define PRECESSION_EPOCH_THRESHOLD 1.0
/* Interval threshold (days) for re-computing nutation values. with 1/24, compute only every hour  */
#define NUTATION_EPOCH_THRESHOLD (1./24.)

/* cache results for retrieval if recomputation is not required */

static double c_psi_A=0.0, c_omega_A=0.0, c_chi_A=0.0, /*c_p_A=0.0, */ c_epsilon_A=0.0,
		c_Y_A=0.0, c_X_A=0.0, c_Q_A=0.0, c_P_A=0.0,
		c_lastJDE=-1e100;

static const double arcSec2Rad=M_PI*2.0/(360.0*3600.0);

static const double PQvals[8][5]=
{ //  1/Pn         P_A:Cn       Q_A:Cn        P_A:Sn        Q_A:Sn
  { 1.0/ 708.15, -5486.751211, -684.661560,   667.666730, -5523.863691 },
  { 1.0/2309.00,   -17.127623, 2446.283880, -2354.886252,  -549.747450 },
  { 1.0/1620.00,  -617.517403,  399.671049,  -428.152441,  -310.998056 },
  { 1.0/ 492.20,   413.442940, -356.652376,   376.202861,   421.535876 },
  { 1.0/1183.00,    78.614193, -186.387003,   184.778874,   -36.776172 },
  { 1.0/ 622.00,  -180.732815, -316.800070,   335.321713,  -145.278396 },
  { 1.0/ 882.00,   -87.676083,  198.296701,  -185.138669,   -34.744450 },
  { 1.0/ 547.00,    46.140315,  101.135679,  -120.972830,    22.885731 }};

static const double XYvals[14][5]=
{ //  1/Pn          Xa:Cn          Ya:Cn         Xa:Sn         Ya:Sn
  { 1.0/ 256.75,  -819.940624,  75004.344875, 81491.287984,  1558.515853 },
  { 1.0/ 708.15, -8444.676815,    624.033993,   787.163481,  7774.939698 },
  { 1.0/ 274.20,  2600.009459,   1251.136893,  1251.296102, -2219.534038 },
  { 1.0/ 241.45,  2755.175630,  -1102.212834, -1257.950837, -2523.969396 },
  { 1.0/2309.00,  -167.659835,  -2660.664980, -2966.799730,   247.850422 },
  { 1.0/ 492.20,   871.855056,    699.291817,   639.744522,  -846.485643 },
  { 1.0/ 396.10,    44.769698,    153.167220,   131.600209, -1393.124055 },
  { 1.0/ 288.90,  -512.313065,   -950.865637,  -445.040117,   368.526116 },
  { 1.0/ 231.10,  -819.415595,    499.754645,   584.522874,   749.045012 },
  { 1.0/1610.00,  -538.071099,   -145.188210,   -89.756563,   444.704518 },
  { 1.0/ 620.00,  -189.793622,    558.116553,   524.429630,   235.934465 },
  { 1.0/ 157.87,  -402.922932,    -23.923029,   -13.549067,   374.049623 },
  { 1.0/ 220.30,   179.516345,   -165.405086,  -210.157124,  -171.330180 },
  { 1.0/1200.00,    -9.814756,      9.344131,   -44.919798,   -22.899655 }};


static const double precVals[18][7]=
{ // 1/Pn         psi_A:Cn       om_A:Cn       chi_A:Cn         psi_A:Sn       om_A:Sn       chi_A:Sn

  { 1.0/402.90,  -22206.325946,  1267.727824, -13765.924050,    -3243.236469, -8571.476251,  -2206.967126 },
  { 1.0/256.75,   12236.649447,  1702.324248,  13511.858383,    -3969.723769,  5309.796459,  -4186.752711 },
  { 1.0/292.00,   -1589.008343, -2970.553839,  -1455.229106,     7099.207893,  -610.393953,   6737.949677 },
  { 1.0/537.22,    2482.103195,   693.790312,   1054.394467,    -1903.696711,   923.201931,   -856.922846 },
  { 1.0/241.45,     150.322920,   -14.724451,      0.0     ,      146.435014,     3.759055,      0.0      },
  { 1.0/375.22,     -13.632066,  -516.649401,   -112.300144,     1300.630106,   -40.691114,    957.149088 },
  { 1.0/157.87,     389.437420,  -356.794454,    202.769908,     1727.498039,    80.437484,   1709.440735 },
  { 1.0/274.20,    2031.433792,  -129.552058,   1936.050095,      299.854055,   807.300668,    154.425505 },
  { 1.0/203.00,     363.748303,   256.129314,      0.0     ,    -1217.125982,    83.712326,      0.0      },
  { 1.0/440.00,    -896.747562,   190.266114,   -655.484214,     -471.367487,  -368.654854,   -243.520976 },
  { 1.0/170.72,    -926.995700,    95.103991,   -891.898637,     -441.682145,  -191.881064,   -406.539008 },
  { 1.0/713.37,      37.070667,  -332.907067,      0.0     ,      -86.169171,    -4.263770,      0.0      },
  { 1.0/313.00,    -597.682468,   131.337633,      0.0     ,     -308.320429,  -270.353691,      0.0      },
  { 1.0/128.38,      66.282812,    82.731919,   -333.322021,     -422.815629,    11.602861,   -446.656435 },
  { 1.0/202.00,       0.0     ,     0.0     ,    327.517465,        0.0     ,     0.0     ,  -1049.071786 },
  { 1.0/315.00,       0.0     ,     0.0     ,   -494.780332,        0.0     ,     0.0     ,   -301.504189 },
  { 1.0/136.32,       0.0     ,     0.0     ,    585.492621,        0.0     ,     0.0     ,     41.348740 },
  { 1.0/490.00,       0.0     ,     0.0     ,    110.512834,        0.0     ,     0.0     ,    142.525186 }};

static const double p_epsVals[10][5]=
{ //  1/Pn         p_A:Cn     eps_A:Cn        p_A:Sn      eps_A:Sn
  { 1.0/ 409.90, -6908.287473,  753.872780, -2845.175469, -1704.720302},
  { 1.0/ 396.15, -3198.706291, -247.805823,   449.844989,  -862.308358},
  { 1.0/ 537.22,  1453.674527,  379.471484, -1255.915323,   447.832178},
  { 1.0/ 402.90,  -857.748557,  -53.880558,   886.736783,  -889.571909},
  { 1.0/ 417.15,  1173.231614,  -90.109153,   418.887514,   190.402846},
  { 1.0/ 288.92,  -156.981465, -353.600190,   997.912441,   -56.564991},
  { 1.0/4043.00,   371.836550,  -63.115353,  -240.979710,  -296.222622},
  { 1.0/ 306.00,  -216.619040,  -28.248187,    76.541307,   -75.859952},
  { 1.0/ 277.00,   193.691479,   17.703387,   -36.788069,    67.473503},
  { 1.0/ 203.00,    11.891524,   38.911307,  -170.964086,     3.014055}};

// compute angles for the series we are in fact using.
// jde: date JD_TT
// 
void getPrecessionAnglesVondrak(const double jde, double *epsilon_A, double *chi_A, double *omega_A, double *psi_A)
{
	if (fabs(jde-c_lastJDE) > PRECESSION_EPOCH_THRESHOLD)
	{
		double T=(jde-2451545.0)* (1.0/36525.0); // Julian centuries from J2000.0
		assert(fabs(T)<=2000); // MAKES SURE YOU NEVER OVERSTRETCH THIS!
		double T2pi= T*(2.0*M_PI); // Julian centuries from J2000.0, premultiplied by 2Pi
		// these are actually small greek letters in the papers.
		double Psi_A=0.0;
		double Omega_A=0.0;
		double Chi_A=0.0;
		double Epsilon_A=0.0;
		//double p_A=0.0; // currently unused. The data don't disturb.
		int i;
		for (i=0; i<18; ++i)
		{
			double invP=precVals[i][0];
			double sin2piT_P, cos2piT_P;
#ifdef _GNU_SOURCE
			sincos(T2pi*invP, &sin2piT_P, &cos2piT_P);
#else
			double phase=T2pi*invP;
			sin2piT_P= sin(phase);
			cos2piT_P= cos(phase);
#endif
			Psi_A   += precVals[i][1]*cos2piT_P + precVals[i][4]*sin2piT_P;
			Omega_A += precVals[i][2]*cos2piT_P + precVals[i][5]*sin2piT_P;
			Chi_A   += precVals[i][3]*cos2piT_P + precVals[i][6]*sin2piT_P;
		}

		for (i=0; i<10; ++i)
		{
			double invP=p_epsVals[i][0];
			double sin2piT_P, cos2piT_P;
#ifdef _GNU_SOURCE
			sincos(T2pi*invP, &sin2piT_P, &cos2piT_P);
#else
			double phase=T2pi*invP;
			sin2piT_P= sin(phase);
			cos2piT_P= cos(phase);
#endif
			//p_A       += p_epsVals[i][1]*cos2piT_P + p_epsVals[i][3]*sin2piT_P;
			Epsilon_A += p_epsVals[i][2]*cos2piT_P + p_epsVals[i][4]*sin2piT_P;
		}

		Psi_A     += (( 289.e-9*T - 0.00740913)*T + 5042.7980307)*T +  8473.343527;
		Omega_A   += (( 151.e-9*T + 0.00000146)*T -    0.4436568)*T + 84283.175915;
		Chi_A     += (( -61.e-9*T + 0.00001472)*T +    0.0790159)*T -    19.657270;
		//p_A       += ((271.e-9*T - 0.00710733)*T + 5043.0520035)*T +  8134.017132;
		Epsilon_A += ((-110.e-9*T - 0.00004039)*T +    0.3624445)*T + 84028.206305;
		c_psi_A     = arcSec2Rad*Psi_A;
		c_omega_A   = arcSec2Rad*Omega_A;
		c_chi_A     = arcSec2Rad*Chi_A;
		// c_p_A     = arcSec2Rad*p_A;
		c_epsilon_A = arcSec2Rad*Epsilon_A;
	}
	*psi_A     = c_psi_A;
	*omega_A   = c_omega_A;
	*chi_A     = c_chi_A;
	*epsilon_A = c_epsilon_A;
}

void getPrecessionAnglesVondrakPQXYe(const double jde, double *vP_A, double *vQ_A, double *vX_A, double *vY_A, double *vepsilon_A)
{
	if (fabs(jde-c_lastJDE) > PRECESSION_EPOCH_THRESHOLD)
	{
		double T=(jde-2451545.0)* (1.0/36525.0);
		assert(fabs(T)<=2000); // MAKES SURE YOU NEVER OVERSTRETCH THIS!
		double T2pi= T*(2.0*M_PI); // Julian centuries from J2000.0, premultiplied by 2Pi
		// these are actually small greek letters in the papers.
		double P_A=0.0;
		double Q_A=0.0;
		double X_A=0.0;
		double Y_A=0.0;
		double Epsilon_A=0.0;
		int i;
		for (i=0; i<8; ++i)
		{
			double invP=PQvals[i][0];
			double sin2piT_P, cos2piT_P;
#ifdef _GNU_SOURCE
			sincos(T2pi*invP, &sin2piT_P, &cos2piT_P);
#else
			double phase=T2pi*invP;
			sin2piT_P= sin(phase);
			cos2piT_P= cos(phase);
#endif
			P_A += PQvals[i][1]*cos2piT_P + PQvals[i][3]*sin2piT_P;
			Q_A += PQvals[i][2]*cos2piT_P + PQvals[i][4]*sin2piT_P;
		}
		for (i=0; i<14; ++i)
		{
			double invP=XYvals[i][0];
			double sin2piT_P, cos2piT_P;
#ifdef _GNU_SOURCE
			sincos(T2pi*invP, &sin2piT_P, &cos2piT_P);
#else
			double phase=T2pi*invP;
			sin2piT_P= sin(phase);
			cos2piT_P= cos(phase);
#endif
			X_A += XYvals[i][1]*cos2piT_P + XYvals[i][3]*sin2piT_P;
			Y_A += XYvals[i][2]*cos2piT_P + XYvals[i][4]*sin2piT_P;
		}
		for (i=0; i<10; ++i)
		{
			double invP=p_epsVals[i][0];
			double sin2piT_P, cos2piT_P;
#ifdef _GNU_SOURCE
			sincos(T2pi*invP, &sin2piT_P, &cos2piT_P);
#else
			double phase=T2pi*invP;
			sin2piT_P= sin(phase);
			cos2piT_P= cos(phase);
#endif
			//p_A       += p_epsVals[i][1]*cos2piT_P + p_epsVals[i][3]*sin2piT_P;
			Epsilon_A += p_epsVals[i][2]*cos2piT_P + p_epsVals[i][4]*sin2piT_P;
		}

		// Now the polynomial terms in T. Horner's scheme is best again.
		P_A       += (( 110.e-9*T - 0.00028913)*T -    0.1189000)*T +  5851.607687;
		Q_A       += ((-437.e-9*T - 0.00000020)*T +    1.1689818)*T -  1600.886300;
		X_A       += ((-152.e-9*T - 0.00037173)*T +    0.4252841)*T +  5453.282155;
		Y_A       += ((+231.e-9*T - 0.00018725)*T -    0.7675452)*T - 73750.930350;
		Epsilon_A += (( 110.e-9*T - 0.00004039)*T +    0.3624445)*T + 84028.206305;
		c_P_A       = arcSec2Rad*P_A;
		c_Q_A       = arcSec2Rad*Q_A;
		c_X_A       = arcSec2Rad*X_A;
		c_Y_A       = arcSec2Rad*Y_A;
		c_epsilon_A = arcSec2Rad*Epsilon_A;
	}
	*vP_A       = c_P_A;
	*vQ_A       = c_Q_A;
	*vX_A       = c_X_A;
	*vY_A       = c_Y_A;
	*vepsilon_A = c_epsilon_A;

}

//! Just return (presumably precomputed) ecliptic obliquity.
double getPrecessionAngleVondrakEpsilon(const double jde)
{
	double epsilon_A, dummy_chi_A, dummy_omega_A, dummy_psi_A;
	getPrecessionAnglesVondrak(jde, &epsilon_A, &dummy_chi_A, &dummy_omega_A, &dummy_psi_A);
	return epsilon_A;
}
//! Just return (presumably precomputed) ecliptic obliquity.
double getPrecessionAngleVondrakCurrentEpsilonA(void)
{
	return c_epsilon_A;
}

// ====================== NUTATION IAU-2000B below.


struct nut2000B
{
	double l_factor;     // multiplier for lunar mean anomaly l
	double ls_factor;    // multiplier for solar mean anomaly
	double F_factor;     // multiplier for F=L-Omega where L s mean longitude of the moon
	double D_factor;     // mean elongatin of the moon from the sun
	double Omega_factor; // mean longitude of lunar ascending node
	double period;       // days
	double A;            // A  [0.1 mas]
	double Ap;           // A' [0.1 mas]
	double B;            // B  [0.1 mas]
	double Bp;           // B' [0.1 mas]
	double App;          // A''[0.1 mas]
	double Bpp;          // B''[0.1 mas]
};

static const struct nut2000B nut2000Btable[78] = {
{  0,  0,  0,  0,  1, -6798.35, -172064161, -174666, 92052331,  9086,  33386, 15377},
{  0,  0,  2, -2,  2,   182.62,  -13170906,   -1675,  5730336, -3015, -13696, -4587},
{  0,  0,  2,  0,  2,    13.66,   -2276413,    -234,   978459,  -485,   2796,  1374},
{  0,  0,  0,  0,  2, -3399.18,    2074554,     207,  -897492,   470,   -698,  -291},
{  0,  1,  0,  0,  0,   365.26,    1475877,   -3633,    73871,  -184,  11817, -1924},
{  0,  1,  2, -2,  2,   121.75,    -516821,    1226,   224386,  -677,   -524,  -174},
{  1,  0,  0,  0,  0,    27.55,     711159,      73,    -6750,     0,   -872,   358},
{  0,  0,  2,  0,  1,    13.63,    -387298,    -367,   200728,    18,    380,   318},
{  1,  0,  2,  0,  2,     9.13,    -301461,     -36,   129025,   -63,    816,   367},
{  0, -1,  2, -2,  2,   365.22,     215829,    -494,   -95929,   299,    111,   132},
{  0,  0,  2, -2,  1,   177.84,     128227,     137,   -68982,    -9,    181,    39},
{ -1,  0,  2,  0,  2,    27.09,     123457,      11,   -53311,    32,     19,    -4},
{ -1,  0,  0,  2,  0,    31.81,     156994,      10,    -1235,     0,   -168,    82},
{  1,  0,  0,  0,  1,    27.67,      63110,      63,   -33228,     0,     27,    -9},
{ -1,  0,  0,  0,  1,   -27.44,     -57976,     -63,    31429,     0,   -189,   -75},
{ -1,  0,  2,  2,  2,     9.56,     -59641,     -11,    25543,   -11,    149,    66},
{  1,  0,  2,  0,  1,     9.12,     -51613,     -42,    26366,     0,    129,    78},
{ -2,  0,  2,  0,  1,  1305.48,      45893,      50,   -24236,   -10,     31,    20},
{  0,  0,  0,  2,  0,    14.77,      63384,      11,    -1220,     0,   -150,    29},
{  0,  0,  2,  2,  2,     7.10,     -38571,      -1,    16452,   -11,    158,    68},
{ -2,  0,  0,  2,  0,  -205.89,     -47722,       0,      477,     0,    -18,   -25},

{  2,  0,  2,  0,  2,     6.86,     -31046,      -1,    13238,   -11,    131,    59},
{  1,  0,  2, -2,  2,    23.94,      28593,       0,   -12338,    10,     -1,    -3},
{ -1,  0,  2,  0,  1,    26.98,      20441,      21,   -10758,     0,     10,    -3},
{  2,  0,  0,  0,  0,    13.78,      29243,       0,     -609,     0,    -74,    13},
{  0,  0,  2,  0,  0,    13.61,      25887,       0,     -550,     0,    -66,    11},
{  0,  1,  0,  0,  1,   386.00,     -14053,     -25,     8551,    -2,     79,   -45},
{ -1,  0,  0,  2,  1,    31.96,      15164,      10,    -8001,     0,     11,    -1},
{  0,  2,  2, -2,  2,    91.31,     -15794,      72,     6850,   -42,    -16,    -5},
{  0,  0, -2,  2,  0,  -173.31,      21783,       0,     -167,     0,     13,    13},
{  1,  0,  0, -2,  1,   -31.66,     -12873,     -10,     6953,     0,    -37,   -14},
{  0, -1,  0,  0,  1,  -346.64,     -12654,      11,     6415,     0,     63,    26},
{ -1,  0,  2,  2,  1,     9.54,     -10204,       0,     5222,     0,     25,    15},
{  0,  2,  0,  0,  0,   182.63,      16707,     -85,      168,    -1,    -10,    10},
{  1,  0,  2,  2,  2,     5.64,      -7691,       0,     3268,     0,     44,    19},
{ -2,  0,  2,  0,  0,  1095.18,     -11024,       0,      104,     0,    -14,     2},
{  0,  1,  2,  0,  2,    13.17,       7566,     -21,    -3250,     0,    -11,    -5},
{  0,  0,  2,  2,  1,     7.09,      -6637,     -11,     3353,     0,     25,    14},
{  0, -1,  2,  0,  2,    14.19,      -7141,      21,     3070,     0,      8,     4},
{  0,  0,  0,  2,  1,    14.80,      -6302,     -11,     3272,     0,      2,     4},
{  1,  0,  2, -2,  1,    23.86,       5800,      10,    -3045,     0,      2,    -1},
{  2,  0,  2, -2,  2,    12.81,       6443,       0,    -2768,     0,     -7,    -4},

{ -2,  0,  0,  2,  1,  -199.84,      -5774,     -11,     3041,     0,    -15,    -5},
{  2,  0,  2,  0,  1,     6.85,      -5350,       0,     2695,     0,     21,    12},
{  0, -1,  2, -2,  1,   346.60,      -4752,     -11,     2719,     0,     -3,    -3},
{  0,  0,  0, -2,  1,   -14.73,      -4940,     -11,     2720,     0,    -21,    -9},
{ -1, -1,  0,  2,  0,    34.85,       7350,       0,      -51,     0,     -8,     4},
{  2,  0,  0, -2,  1,   212.32,       4065,       0,    -2206,     0,      6,     1},
{  1,  0,  0,  2,  0,     9.61,       6579,       0,     -199,     0,    -24,     2},
{  0,  1,  2, -2,  1,   119.61,       3579,       0,    -1900,     0,      5,     1},
{  1, -1,  0,  0,  0,    29.80,       4725,       0,      -41,     0,     -6,     3},
{ -2,  0,  2,  0,  2,  1615.76,      -3075,       0,     1313,     0,     -2,    -1},
{  3,  0,  2,  0,  2,     5.49,      -2904,       0,     1233,     0,     15,     7},
{  0, -1,  0,  2,  0,    15.39,       4348,       0,      -81,     0,    -10,     2},
{  1, -1,  2,  0,  2,     9.37,      -2878,       0,     1232,     0,      8,     4},
{  0,  0,  0,  1,  0,    29.53,      -4230,       0,      -20,     0,      5,    -2},
{ -1, -1,  2,  2,  2,     9.81,      -2819,       0,     1207,     0,      7,     3},
{ -1,  0,  2,  0,  0,    26.88,      -4056,       0,       40,     0,      5,    -2},
{  0, -1,  2,  2,  2,     7.24,      -2647,       0,     1129,     0,     11,     5},
{ -2,  0,  0,  0,  1,   -13.75,      -2294,       0,     1266,     0,    -10,    -4},
{  1,  1,  2,  0,  2,     8.91,       2481,       0,    -1062,     0,     -7,    -3},
{  2,  0,  0,  0,  1,    13.81,       2179,       0,    -1129,     0,     -2,    -2},
{ -1,  1,  0,  1,  0,  3232.87,       3276,       0,       -9,     0,      1,     0},

{  1,  1,  0,  0,  0,    25.62,      -3389,       0,       35,     0,      5,    -2},
{  1,  0,  2,  0,  0,     9.11,       3339,       0,     -107,     0,    -13,     1},
{ -1,  0,  2, -2,  1,   -32.61,      -1987,       0,     1073,     0,     -6,    -2},
{  1,  0,  0,  0,  2,    27.78,      -1981,       0,      854,     0,      0,     0},
{ -1,  0,  0,  1,  0,  -411.78,       4026,       0,     -553,     0,   -353,  -139},
{  0,  0,  2,  1,  2,     9.34,       1660,       0,     -710,     0,     -5,    -2},
{ -1,  0,  2,  4,  2,     5.80,      -1521,       0,      647,     0,      9,     4},
{ -1,  1,  0,  1,  1,  6146.17,       1314,       0,     -700,     0,      0,     0},
{  0, -2,  2, -2,  1,  6786.31,      -1283,       0,      672,     0,      0,     0},
{  1,  0,  2,  2,  1,     5.64,      -1331,       0,      663,     0,      8,     4},
{ -2,  0,  2,  2,  2,    14.63,       1383,       0,     -594,     0,     -2,    -2},
{ -1,  0,  0,  0,  2,   -27.33,       1405,       0,     -610,     0,      4,     2},
{  1,  1,  2, -2,  2,    22.47,       1290,       0,     -556,     0,      0,     0},
{ -2,  0,  2,  4,  2,     7.35,      -1214,       0,      518,     0,      5,     2},
{ -1,  0,  4,  0,  2,     9.06,       1146,       0,     -490,     0,     -3,    -1}};

/* cache results for retrieval if recomputation is not required */
static double c_deltaEps=0.0;
static double c_deltaPsi=0.0;
static double c_jdeLastNut=-1e-100;


//! Compute and return nutation angles of the abridged IAU-2000B nutation.
//! Ref: Dennis D. McCarthy and Brian J. Lizum: An Abridged Model of the Precession-Nutation of the Celestial Pole.
//! Celestial Mechanics and Dynamical Astronomy 85: 37-49, 2003.
//! This model provides accuracy better than 1 milli-arcsecond in the time 1995-2050.
//! TODO: find out drift rate behaviour e.g. in 17./18. century, maybe use nutation only e.g. 1610-2200?
//! @return deltaPsi, deltaEps [radians]
//! @param JDE Julian Day, TT
//! @note The model promises mas accuracy in the present era but gives no comment on long-time effects. Given that nutation was discovered in the early 18th century,
//! it seems wise to set the returned values to zero before 1500 and after 2500. To avoid a jump, a linear fade-in/fade-out is applied within 100 days before 1500 and after 2500.
void getNutationAngles(const double JDE, double *deltaPsi, double *deltaEpsilon)
{	
// 1.1.1500
#define NUT_BEGIN 2268932.5
// 1.1.2500
#define NUT_END	2634166.5
#define NUT_TRANSITION 100.0
	if ((JDE<=NUT_BEGIN-NUT_TRANSITION ) || (JDE>=NUT_END + NUT_TRANSITION))
	{
			*deltaPsi=0.0;
			*deltaEpsilon=0.0;
			return;
	}

	if (fabs(JDE-c_jdeLastNut)>NUTATION_EPOCH_THRESHOLD)
	{
		c_jdeLastNut=JDE;
		double t=(JDE-2451545.0)/36525.0;
		// F1 : l = mean anomaly of the Moon ['']
		double     l  =  (485868.249036 + 1717915923.2178*t);//*arcSec2Rad;
		// F2 : l' = mean anomaly of the Sun ['']
		double     ls = (1287104.79305 + 129596581.0481*t);//*arcSec2Rad;
		// F3 : F = L - Omega (L is the mean longitude of the Moon)
		double      F = (335779.526232 + 1739527262.8478*t);//*arcSec2Rad;
		// F4 : D = mean elongation of the Moon from the Sun
		double      D =  (1072260.70369 + 1602961601.2090*t);//*arcSec2Rad;
		// F5 : Omega = mean longitude of the ascending node of the lunar orbit
		double Omega  = (450160.398036 - 6962890.5431*t);//*arcSec2Rad;

		double deltaEps=0.0, deltaPsi=0.0; // lgtm [cpp/declaration-hides-parameter]
		int i;
		for (i=0; i<78; ++i)
		{
			const struct nut2000B *nut=&nut2000Btable[i];
			double theta=nut->l_factor*l + nut->ls_factor*ls + nut->F_factor*F + nut->D_factor*D + nut->Omega_factor*Omega;
			theta *=arcSec2Rad;
			double sinTheta=sin(theta);
			double cosTheta=cos(theta);
			deltaPsi+=(nut->A + nut->Ap*t)*sinTheta + nut->App*cosTheta;
			deltaEps+=(nut->B + nut->Bp*t)*cosTheta + nut->Bpp*sinTheta;
		}
		deltaPsi *= 1e-7; // convert from units of 0.1uas to arcsec. (The paper says mas, but this is an error!)
		deltaEps *= 1e-7;
		deltaPsi -= (0.29965*t + 0.0417750 + 0.0015835);
		deltaEps -= (0.02524*t + 0.0068192 - 0.0016339);
		c_deltaPsi = deltaPsi * arcSec2Rad;
		c_deltaEps = deltaEps * arcSec2Rad;
	}
	double limiter=1.0;
	if (JDE<NUT_BEGIN)
	{
		limiter=1.-(NUT_BEGIN-JDE)/NUT_TRANSITION;
	}
	if (JDE>NUT_END)
	{
		limiter=1.-(JDE-NUT_END)/NUT_TRANSITION;
	}

	*deltaPsi=c_deltaPsi*limiter;
	*deltaEpsilon=c_deltaEps*limiter;
}
