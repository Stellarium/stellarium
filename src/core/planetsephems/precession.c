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
 * The data are only applicable for a time range of 200.000 years around J2000.
 * This is by far enough for Stellarium as of 2015, but just to make sure I added a few asserts.
 */

#include <math.h>
#include <assert.h>

/* Interval threshold (days) for re-computing these values. with 1, compute only 1/day:  */
#define PRECESSION_EPOCH_THRESHOLD 1.0

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
  { 1.0/ 882.00,   -87.676083,  198.296071,  -185.138669,   -34.744450 },
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
