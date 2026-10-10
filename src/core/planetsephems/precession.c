/*
Copyright (C) 2015 Georg Zotti
Copyright (C) 2026 Sylvain Simard

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

#ifndef J2000
# define J2000 2451545.0
#endif

#ifndef DJC
# define DJC 36525.0
#endif

/* Degrees to radians */
#ifndef DD2R
# define DD2R (1.745329251994329576923691e-2)
#endif

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
		c_L = 0.0, c_I = 0.0, c_Delta = 0.0,
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
void getPrecessionAnglesVondrak(const double jde, double *epsilon_A, double *chi_A, double *omega_A, double *psi_A)
{
	if (fabs(jde-c_lastJDE) > PRECESSION_EPOCH_THRESHOLD)
	{
		c_lastJDE=jde;
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

// (SS) 2026-06-10 : I corrected two mistakes in the polynomial terms of the original code. The T^3 term of P_A was corrected
// from 110.e-9 to 101.e-9 and the T^3 term of Epsilon_A was corrected from 110.e-9 to -110.e-9.
void getPrecessionAnglesVondrakPQXYe(const double jde, double *vP_A, double *vQ_A, double *vX_A, double *vY_A, double *vepsilon_A)
{
	if (fabs(jde-c_lastJDE) > PRECESSION_EPOCH_THRESHOLD)
	{
		c_lastJDE=jde;
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
		P_A		  += ((+101.e-9*T - 0.00028913)*T -	   0.1189000)*T +  5851.607687; // (SS) 2026-06-10 : corrected the value of the T^3 term from 110.e-9 to 101.e-9
		Q_A       += ((-437.e-9*T - 0.00000020)*T +    1.1689818)*T -  1600.886300;
		X_A       += ((-152.e-9*T - 0.00037173)*T +    0.4252841)*T +  5453.282155;
		Y_A       += ((+231.e-9*T - 0.00018725)*T -    0.7675452)*T - 73750.930350;
		Epsilon_A += ((-110.e-9*T - 0.00004039)*T +    0.3624445)*T + 84028.206305; // (SS) 2026-06-10 : corrected the value of the T^3 term from 110.e-9 to -110.e-9
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

//! Just return (often cached) ecliptic obliquity. [radians]
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

// ================= OWEN(90) LONG TERM PRECESSION THEORY =================

// MACRO defining the range of the Owen(90) long-term theory intervals being implemented in this limited
// version of the full theory. The full theory contains 125 intervals of 80 centuries each. In this implementation
// we are using only 5 intervals centered on J2000 to cover a Time range of +/- 200 centuries. If additional intervals
// of Chebyshev coefficients are added to these 7 tables in order to cover a larger range you must modify these MACROs accordingly
#define iC_START 61
#define iC_CENTRAL 63
#define iC_END 65
#define LIMITS ((iC_CENTRAL - iC_START) * 80 + 40)

// Delta is the angle(deg), measured along the invariable plane, from the intersection
// of the invariable plane and the mean equator of J2000 to the intersection of the
// invariable plane and the mean equator of date
static const double DELTA_DEG[10][iC_END - iC_START + 1] = {
	//                                                   Delta (deg) Chebyshev Coefficients
	//       iC = 61, Tc = -160        iC = 62, Tc = -80         Ic = 63, Tc = 0	    iC = 64, Tc = +80      iC = 65, Tc = +160
	//           iCmap = 0                Icmap = 1                iCmap = 2                iCmap = 3               iCmap = 4
	//	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		{ -212.53649464223264    , -108.25884858484431    ,  6.2615291005289149E-1 ,  117.09044608667767    ,  233.15704592339831    },
		{   52.601179136870115   ,  52.349027226042447    ,  56.771213709867143    ,  59.060661124248742    ,  56.853168369398135    },
		{ -4.5828988085167103E-1 ,  3.4893381308555388E-1 ,  6.1835281382298593E-1 , -1.1131569628378585E-1 , -3.0539872356159629E-1 },
		{  8.0588172629387582E-2 ,  5.3833786104036534E-2 , -2.5128752776055107E-2 , -5.9650051230704656E-2 ,  1.4060654910629863E-2 },
		{ -1.5193849105236105E-3 , -1.9954650556701174E-3 , -7.7222489343378689E-3 ,  5.3930420949652662E-3 ,  1.1109823645741077E-3 },
		{ -1.4488131973252688E-4 , -1.5268691030085015E-4 , -3.4208049847247442E-6 ,  4.7847834528190912E-4 , -3.7130414834222326E-4 },
		{  4.5296047939943364E-5 , -2.4190557815900544E-5 ,  7.7231678081801024E-5 , -9.0393920612077870E-5 ,  2.2973779820402573E-5 },
		{ -6.1646187803246070E-6 , -7.0673251488384895E-7 ,  5.3289056505918707E-6 , -2.1155140324960377E-6 ,  1.4971679533428247E-6 },
		{  4.2022248222522481E-7 ,  1.2570991731562238E-7 , -6.1561748589433599E-7 ,  1.2181773104988007E-6 , -2.6022651367242367E-7 },
		{ -6.9101125561169980E-9 ,  1.4907548140220873E-8 , -1.0593850942161246E-7 , -2.9359115676942150E-8 ,  1.5190509398213489E-8 }
};

// I is the angle (deg) of the inclination of the invariable plane to the mean equator of date.
static const double I_DEG[10][iC_END - iC_START + 1] = {
	//                                                   I (deg) Chebyshev Coefficients
	//       iC = 61, Tc = -160        iC = 62, Tc = -80         Ic = 63, Tc = 0	    iC = 64, Tc = +80      iC = 65, Tc = +160
	//           iCmap = 0                Icmap = 1                iCmap = 2                iCmap = 3               iCmap = 4
	//	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		{  25.113293357688533    ,  25.675797175777276    ,   23.114556988665967   ,  21.662229672877546    ,  22.614813166560928    },
		{  1.4548120719905701    , -8.5759662165341842E-1 ,  -1.4162765443398956   ,  3.1936441409272980E-2 ,  7.6653571384938099E-1 },
		{ -2.7313930989863579E-1 , -2.3425450978840602E-1 ,  1.0404207539493511E-1 ,  1.8546649576387279E-1 ,  6.2866045410161332E-3 },
		{ -1.8698492046169593E-2 ,  2.1572234877403812E-2 ,  2.6315522885852885E-2 , -1.2926246425312430E-2 , -9.5364769546644450E-3 },
		{  3.1828200121792599E-3 ,  1.7385086245295523E-3 , -1.6218541171038529E-3 , -1.6598870615338844E-3 ,  1.1751452872783874E-3 },
		{ -5.0231835441076053E-5 , -9.4433708492985720E-5 , -2.1656331976588425E-4 ,  2.2905728674660311E-4 , -1.7726246438498393E-5 },
		{ -4.2588784801858854E-6 , -3.8153398284061494E-6 ,  5.0785607890280076E-6 ,  7.2898885304771316E-6 , -1.2197719270429934E-5 },
		{  7.0016072001988432E-7 , -4.0978220999957025E-7 ,  1.8483549770338827E-6 , -2.3050414336141165E-6 ,  6.9895194269472982E-7 },
		{ -9.8027621808098010E-8 , -1.8054745227662793E-8 ,  7.4962416484795782E-8 ,  1.0207190221594289E-8 ,  3.7435936837838101E-8 },
		{  9.5163891399824242E-9 ,  2.8035012061081817E-9 , -1.3869774297054672E-8 ,  2.1267933912813754E-8 , -3.6671444523199732E-9 }
};

// L is the angle (deg) measured along the equator of date, from the mean vernal equinox
// of date to the intersection of the invariable plane and the equator of date; the right
// ascension of this point.
static const double L_DEG[10][iC_END - iC_START + 1] = {
	//                                                   L (deg) Chebyshev Coefficients
	//       iC = 61, Tc = -160        iC = 62, Tc = -80         Ic = 63, Tc = 0	    iC = 64, Tc = +80      iC = 65, Tc = +160
	//           iCmap = 0                Icmap = 1                iCmap = 2                iCmap = 3               iCmap = 4
	//	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		{ -3.5954857535537836    ,  2.0020744183260302    ,  3.4380222430370099    ,  9.5437065030987305E-3 , -1.9320943594035367    },
		{  2.7893472819993091    ,  2.3353002424891801    , -9.8690360675407852E-1 , -1.9176927322805243    , -6.9879024986415078E-2 },
		{  2.5680743608790162E-1 , -3.1342008401332741E-1 , -4.0859346678513061E-1 ,  1.9333186102851091E-1 ,  1.5320475639045154E-1 },
		{ -6.5588744937483328E-2 , -3.2738907275091193E-2 ,  2.9203376233848717E-2 ,  3.7496899517722484E-2 , -2.5440846435685551E-2 },
		{  2.6350864315171937E-3 ,  1.6910241935722453E-3 ,  5.8849879702565663E-3 , -6.0027489222746884E-3 ,  4.5995057921348084E-4 },
		{  4.6607869327948861E-5 ,  7.9103449586165224E-5 , -6.2994538219749156E-5 , -3.1915287891149424E-4 ,  4.1426663618896325E-4 },
		{ -3.9809272397874433E-5 ,  2.3872301666332189E-5 , -7.1439474118375438E-5 ,  9.0034532891154015E-5 , -3.0181311030904185E-5 },
		{  6.2024562749848758E-6 ,  8.7654392349113181E-7 , -4.2764500331174845E-6 ,  1.1080885332971737E-6 , -1.8296328903418022E-6 },
		{ -5.3075941703763470E-7 , -1.4963759356787623E-7 ,  6.4051126256827420E-7 , -1.1372233207379882E-6 ,  2.6920761034203793E-7 },
		{  1.1169782184523872E-8 , -2.1528281786737335E-8 ,  9.1918098138014373E-8 ,  3.7803189897816002E-8 , -6.2874055435225260E-9 }
};

// EPS_A is the angle (deg) of the obliquity of the ecliptic; the inclination of the ecliptic
// of date to the equator of date.
static const double EPS_A_DEG[10][iC_END - iC_START + 1] = {
	//                                                   EPS (deg) Chebyshev Coefficients
	//       iC = 61, Tc = -160        iC = 62, Tc = -80         Ic = 63, Tc = 0	    iC = 64, Tc = +80      iC = 65, Tc = +160
	//           iCmap = 0                Icmap = 1                iCmap = 2                iCmap = 3               iCmap = 4
	//	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		{  23.699391439256386    ,  24.124759551704588    ,  23.439103144206208    ,  22.724671295125046    ,  22.914636050333696    },
		{  5.2330816033981775E-1 , -1.2094875596566286E-1 , -4.9386077073143590E-1 , -1.6041813558650337E-1 ,  3.2123508304962416E-1 },
		{ -5.6259493384864815E-2 , -8.3914869653015218E-2 , -2.3965445283267805E-4 ,  7.0646783888132504E-2 ,  3.6633220173792710E-2 },
		{ -8.2033318431602032E-3 ,  3.5357075322387405E-3 ,  8.6637485629656489E-3 ,  1.4967806745062837E-3 , -5.9228324767696043E-3 },
		{  6.6774163554156385E-4 ,  6.4557467824807032E-4 , -5.2828151901367600E-5 , -6.6857270989190734E-4 , -1.8823791073793285E-4 },
		{  2.4931584012812606E-5 , -2.5092064378707704E-5 , -4.3951004595359217E-5 ,  5.7578378071604775E-6 ,  3.2274552870236244E-5 },
		{ -3.1313623302407878E-6 , -1.7631607274450848E-6 , -1.1058785949914705E-6 ,  3.3738508454638728E-6 ,  4.9052463646336507E-7 },
		{  2.0343814827951515E-7 ,  1.3363622791424094E-7 ,  6.2431490022621172E-8 , -2.2917813537654764E-7 , -5.9064298731578425E-8 },
		{  2.9182026615852936E-9 ,  1.5577817511054047E-8 ,  3.4725376218710764E-8 , -2.1019907929218137E-8 , -2.0485712675098837E-8 },
		{ -4.1118760893281951E-9 , -2.4613907093017122E-9 ,  1.3658853127005757E-9 ,  4.3139832091694682E-9 , -6.2163304813908160E-10}
};

// PSI_A is the angle (deg) measured along the ecliptic of J2000, from the mean vernal equinox
// of J2000 to the intersection of the ecliptic of J2000 and the equator of date
static const double PSI_A_DEG[10][iC_END - iC_START + 1] = {
	//                                                  PSI_A (deg) Chebyshev Coefficients
	//       iC = 61, Tc = -160        iC = 62, Tc = -80         Ic = 63, Tc = 0	    iC = 64, Tc = +80      iC = 65, Tc = +160
	//           iCmap = 0                Icmap = 1                iCmap = 2                iCmap = 3               iCmap = 4
	//	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		{ -218.57864954903122    , -111.94350527506128    , -2.0414520115294410E-1 ,  111.61366860604471    ,  228.40683531269390    },
		{  51.752257487741612    ,  55.175558131675861    ,  55.969995858494106    ,  56.404525305162447    ,  60.056143904919826    },
		{  1.3304715765661958E-1 ,  4.7366115762797613E-1 , -1.9295093699770936E-1 ,  4.4403302410703782E-1 ,  2.9583200718478960E-2 },
		{  9.2048123521890745E-2 , -4.7701750975398538E-2 , -5.6819574830421158E-3 ,  7.1490030578883907E-2 , -1.5710838319490748E-1 },
		{ -6.0877528127241278E-3 , -9.2445765329325809E-3 ,  1.1073687302518981E-2 , -4.9184559079790816E-3 , -7.0017356811600801E-3 },
		{ -7.0013893644531700E-5 ,  7.0962838707454917E-4 , -9.0868489896815619E-5 , -1.3912698949042046E-3 ,  3.3009615142224537E-3 },
		{ -4.9217728385458495E-5 ,  1.5140455277814658E-4 , -1.1999773777895820E-4 , -6.8490613661884005E-5 ,  2.0318123852537664E-4 },
		{ -1.8578234189053723E-6 , -7.7813159018954928E-7 ,  9.9748697306154409E-6 ,  1.2394328562905297E-6 , -6.5840216067828310E-5 },
		{  7.4396426162029877E-7 , -2.4729402281953378E-6 ,  5.7911493603430550E-7 ,  1.7719847841480384E-6 , -5.9077673352976155E-6 },
		{ -5.9157528981843864E-9 , -1.0898887008726418E-7 , -2.3647526839778175E-7 ,  2.4889095220628068E-7 ,  1.3983942185303064E-6 }
};

// OMEGA_A is the angle (deg) of the inclination of the ecliptic of J2000 to the mean equator of date
static const double OMEGA_A_DEG[10][iC_END - iC_START + 1] = {
	//                                                 OMEGA_A (deg) Chebyshev Coefficients
	//       iC = 61, Tc = -160        iC = 62, Tc = -80         Ic = 63, Tc = 0	    iC = 64, Tc = +80      iC = 65, Tc = +160
	//           iCmap = 0                Icmap = 1                iCmap = 2                iCmap = 3               iCmap = 4
	//	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		{  25.541291140949806    ,  24.429357654237926    ,  23.450465062489337    ,  22.581778052947806    ,  21.518861835737142    },
		{  2.3778895112721623E-1 , -9.5205745947740161E-1 , -9.7259278279739817E-2 , -8.7069701538602037E-1 ,  2.0494789509441385E-1 },
		{ -3.7337334723142133E-1 ,  8.6738296270534816E-2 ,  1.1082286925130981E-2 , -9.8140710050197307E-2 ,  3.5193604846503161E-1 },
		{  2.4579295485161534E-2 ,  3.0061543426062955E-2 , -3.1469883339372219E-2 ,  2.6025931340678079E-2 ,  1.5305977982348925E-2 },
		{  4.3840999514263623E-3 , -4.1532480523019988E-3 , -1.0041906996819648E-4 ,  4.8165322168786755E-3 , -7.5015367726336455E-3 },
		{ -3.1126873333599556E-4 , -3.7920928393860939E-4 ,  5.6455168475133958E-4 , -1.9065587721933634E-4 , -4.0322553186065610E-4 },
		{ -9.8443045771748915E-6 ,  3.5117012399609737E-5 , -8.4403910211030209E-6 , -4.6838759635421777E-5 ,  1.0655320434844041E-4 },
		{ -7.9403103080496923E-7 ,  4.6811877283079217E-6 , -3.8269157371098435E-6 , -1.6608525315998471E-6 ,  7.1792339586935752E-6 },
		{  1.0840116743893556E-9 , -8.1836046585546861E-8 ,  3.1422585261198437E-7 , -3.2347811293516124E-8 , -1.6038746975430208E-6 },
		{  9.2865105216887919E-9 , -6.1803706664211173E-8 ,  9.3481729116773404E-9 ,  2.8104728109642000E-8 , -1.6135634628135124E-7 }
};

// CHI_A is the angle (deg) measured along the equator of date, from the vernal equinox
// of date to the intersection of the mean equator of date and the ecliptic of J2000
static const double CHI_A_DEG[10][iC_END - iC_START + 1] = {
	//                                                 CHI_A (deg) Chebyshev Coefficients
	//       iC = 61, Tc = -160        iC = 62, Tc = -80         Ic = 63, Tc = 0	    iC = 64, Tc = +80      iC = 65, Tc = +160
	//           iCmap = 0                Icmap = 1                iCmap = 2                iCmap = 3               iCmap = 4
	//	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		{  8.2378850337329404E-1 ,  -2.1726062070318606   , -4.8518673570735556E-1 , -2.0950740076326087    ,  6.3315163285678715E-1 },
		{ -3.7443109739678667    ,  7.8470515033132925E-1 ,  1.0016737299946743E-1 , -9.4447359463206877E-1 ,  3.5241082918420464    },
		{  4.0143936898854026E-1 ,  4.4044931004195718E-1 , -4.7074888613099918E-1 ,  4.0940512860493755E-1 ,  2.1223076605364606E-1 },
		{  8.1822830214590811E-2 , -8.0671247169971653E-2 , -5.8604054305076092E-3 ,  1.0261699700263508E-1 , -1.5648122502767368E-1 },
		{ -8.5978790792656293E-3 , -8.9672662444325007E-3 ,  1.4300208240553435E-2 , -5.3133241571955160E-3 , -9.1964075390801980E-3 },
		{ -2.8350488448426132E-5 ,  9.2248978383109719E-4 , -6.7127991650300028E-5 , -1.6634631550720911E-3 ,  3.3896161239812411E-3 },
		{ -4.2474671728156727E-5 ,  1.5143472266372874E-4 , -1.3703764889645475E-4 , -5.9477519536647907E-5 ,  2.1485178626085787E-4 },
		{ -1.6214840884656678E-6 , -1.6387009056475679E-6 ,  9.0505213684444634E-6 ,  2.9651387319208926E-6 , -6.6261759864793735E-5 },
		{  7.8560442001953050E-7 , -2.4405558979328144E-6 ,  6.0368690647808607E-7 ,  1.6434499452070584E-6 , -5.9257969712852667E-6 },
		{ -1.0320166416967075E-8 , -1.0148113464009015E-7 , -2.2135404747652171E-7 ,  2.3720647656961084E-7 ,  1.3918759086160525E-6 }
};

// compute classical angles epsilon_A, chi_A, omega_A, psi_A using Owen(90) long-term precession theory.
// jde: date JD_TT
void getPrecessionAnglesOwenClassic(const double jde, double *epsilon_A, double *chi_A, double *omega_A, double *psi_A)
{
	if (fabs(jde - c_lastJDE) > PRECESSION_EPOCH_THRESHOLD)
	{
		c_lastJDE = jde;

		// Convert jde to Julian centuries since J2000.0
		double T = (jde - 2451545.0) * (1.0 / 36525.0);
		assert(fabs(T) <= LIMITS);

		// Calculate the ordinal number of the set of Chebyshev coefficients to be used
		// for finding precession angles at a given date in the long-term theory.
		// Each set of Chebyshev coefficients covers a time interval of 80 Julian centuries.
		// We currently use 5 sets of Chebyshev coefficients from Owen's long-term theory
		// to cover the time spanned between +/- 200 centuries from J2000.0.
		// The index computed by Owen are 61, 62, 63, 64 and 65.
		// Index 61 : 18,000BC to 10,000BC
		// Index 62 : 10,000BC to  2,000BC
		// Index 63 :  2,000BC to  6,000AD
		// Index 64 :  6,000AD to 14,000AD
		// Index 65 : 14,000AD to 22,000AD
		// Since we are using five sets of Chebyshev coefficients, we have to map these five index values
		// to 0, 1, 2, 3 and 4 for our implementation. We can do this by subtracting macro iC_START from the computed index value.
		double index = (T + 5080) / 80;
		int iC       = (int)index;

		// Note : As the end of an interval is equivalent to the beginning of the next interval, we could compute the end of the
		// very last interval (i.e. at the LIMITS of Time) by stepping back one count of the index to use Chebyshev coefficients
		// of the last interval. Otherwise the index would point beyond in the no man's land...
		if (T >= LIMITS) iC--;

		int iCmap = iC - iC_START;

		// Calculate The central time in a time interval of the table of long-term Chebyshev coefficients.
		// Since we are using only five sets of Chebyshev coefficients from Owen long-term theory,
		// the only five values of Tc are used: -160, -80, 0, +80, +160. Tc = 0 corresponds to J2000.0, or jde = 2451545.0
		int Tc = 80 * iC - 5040;

		// Calculate the dimensionless time argument to Chebyshev polynomials.
		// This is the fractional time interval between the central time of the record and the given date
		// normalized so that it is between -1 and 1. For example, if the given date is exactly at the central time
		// of the record, then t = 0. If the given date is at the beginning of the record, then t = -1.
		// If the given date is at the end of the record, then t = +1.
		double t = (T - Tc) / 40.0;

		// Then Compute the Chebyshev polynomials Tn(t)
		// Owen's long-term precession theory uses Chebyshev polynomials to represent the precession angles
		// as a function of time. Owen uses Chebyshev polynomials of 10 terms, which are defined by the
		// following recursive equations: Set polynomial degree 0 & 1 term as starting conditions
		double Tn[10] = {0.0};
		Tn[0]         = 1.0;
		Tn[1]         = t;
		for (int i = 2; i < 10; i++)
			Tn[i] = 2 * t * Tn[i - 1] - Tn[i - 2];

		// We now need to compute all four classic precession angles (epsilon_A, chi_A, omega_A, psi_A) using the
		// Chebyshev polynomials and the corresponding coefficients from their respective table. The value
		// obtained is in degree and need to be converted to radians for the next steps of the calculation.
		double epsilon_A_deg = 0.0;
		double chi_A_deg     = 0.0;
		double omega_A_deg   = 0.0;
		double psi_A_deg     = 0.0;

		for (int i = 0; i < 10; i++)
		{
			epsilon_A_deg += EPS_A_DEG[i][iCmap] * Tn[i];
			chi_A_deg += CHI_A_DEG[i][iCmap] * Tn[i];
			omega_A_deg += OMEGA_A_DEG[i][iCmap] * Tn[i];
			psi_A_deg += PSI_A_DEG[i][iCmap] * Tn[i];
		}

		c_epsilon_A = epsilon_A_deg * DD2R;
		c_chi_A     = chi_A_deg * DD2R;
		c_omega_A   = omega_A_deg * DD2R;
		c_psi_A     = psi_A_deg * DD2R;
	}
	*epsilon_A = c_epsilon_A;
	*chi_A     = c_chi_A;
	*omega_A   = c_omega_A;
	*psi_A     = c_psi_A;
}

// compute invariable plane angles L, I, Delta, using the Owen(90) long term precession theory.
// jde: date JD_TT
void getPrecessionAnglesOwenLIDelta(const double jde, double *L, double *I, double *Delta)
{
	if (fabs(jde - c_lastJDE) > PRECESSION_EPOCH_THRESHOLD)
	{
		c_lastJDE = jde;

		// Convert jde to Julian centuries since J2000.0
		double T = (jde - 2451545.0) * (1.0 / 36525.0);
		assert(fabs(T) <= LIMITS); // MAKES SURE YOU REMAIN WITHIN THE LIMIT +/- 200 centuries ... FOR NOW

		// Calculate the ordinal number of the set of Chebyshev coefficients to be used
		// for finding precession angles at a given date in the long-term theory.
		// Each set of Chebyshev coefficients covers a time interval of 80 Julian centuries.
		// We currently use 5 sets of Chebyshev coefficients from Owen's long-term theory
		// to cover the time spanned between +/- 200 centuries from J2000.0.
		// The index computed by Owen are 61, 62, 63, 64 and 65.
		// Index 61 : 18,000BC to 10,000BC
		// Index 62 : 10,000BC to  2,000BC
		// Index 63 :  2,000BC to  6,000AD
		// Index 64 :  6,000AD to 14,000AD
		// Index 65 : 14,000AD to 22,000AD
		// Since we are using five sets of Chebyshev coefficients, we have to map these five index values
		// to 0, 1, 2, 3 and 4 for our implementation. We can do this by subtracting macro iC_START from the computed index value.
		double index = (T + 5080) / 80;
		int iC       = (int)index;

		// Note : As the end of an interval is equivalent to the beginning of the next interval, we could compute the end of the
		// very last interval (i.e. at the LIMITS of Time) by stepping back one count of the index to use Chebyshev coefficients
		// of the last interval. Otherwise the index would point beyond in the no man's land...
		if (T >= LIMITS) iC--;

		int iCmap = iC - iC_START;

		// Calculate The central time in a time interval of the table of long-term Chebyshev coefficients.
		// Since we are using only five sets of Chebyshev coefficients from Owen long-term theory,
		// the only five values of Tc are used: -160, -80, 0, +80, +160. Tc = 0 corresponds to J2000.0, or jde = 2451545.0
		int Tc = 80 * iC - 5040;

		// Calculate The dimensionless time argument to Chebyshev polynomials.
		// This is the fractional time interval between the central time of the record and the given date
		// normalized so that it is between -1 and 1. For example, if the given date is exactly at the central time
		// of the record, then t = 0. If the given date is at the beginning of the record, then t = -1.
		// If the given date is at the end of the record, then t = +1.
		double t = (T - Tc) / 40.0;

		// Then Compute the Chebyshev polynomials Tn(t)
		// Owen's long-term precession theory uses Chebyshev polynomials to represent the precession angles as a function of time.
		// Owen uses Chebyshev polynomials of 10 terms, which are defined by the following recursive equations:
		// Set polynomial degree 0 & 1 term as starting conditions
		double Tn[10] = {0.0};
		Tn[0]         = 1.0;
		Tn[1]         = t;
		for (int i = 2; i < 10; i++)
			Tn[i] = 2 * t * Tn[i - 1] - Tn[i - 2];

		// We now need to compute all three precession angles (L, I and Delta) using the Chebyshev polynomials
		// and the corresponding coefficients from their respective table. The value obtained is in degree
		// and we need to convert it to radians for the next steps of the calculation.
		double L_deg     = 0.0;
		double I_deg     = 0.0;
		double Delta_deg = 0.0;

		for (int i = 0; i < 10; i++)
		{
			L_deg += L_DEG[i][iCmap] * Tn[i];
			I_deg += I_DEG[i][iCmap] * Tn[i];
			Delta_deg += DELTA_DEG[i][iCmap] * Tn[i];
		}

		c_L     = L_deg * DD2R;
		c_I     = I_deg * DD2R;
		c_Delta = Delta_deg * DD2R;
	}
	*L     = c_L;
	*I     = c_I;
	*Delta = c_Delta;
}

//! Just return (presumably precomputed) ecliptic obliquity.
double getPrecessionAngleOwenEpsilon(const double jde)
{
	double epsilon_A, dummy_chi_A, dummy_omega_A, dummy_psi_A;
	// We have to call this twice with different dates to avoid returning the cached value.
	// However we must pick a Julian Day that is within the allowed range of +/- LIMITS centuries around J2000.0
	// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
	// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
	// of JulianDay, we use the maximum Julian Day minus 2 days.
	getPrecessionAnglesOwenClassic(J2000 + LIMITS * DJC - 2, &epsilon_A, &dummy_chi_A, &dummy_omega_A, &dummy_psi_A);
	getPrecessionAnglesOwenClassic(jde, &epsilon_A, &dummy_chi_A, &dummy_omega_A, &dummy_psi_A);
	return epsilon_A;
}

// Return the invariable plane angles L0 at J2000
double getPrecessionAnglesOwenL0(void)
{
	double L0, dummy_I0, dummy_Delta;
	// We have to call this twice with different dates to avoid returning the cached value.
	// However we must pick a Julian Day that is within the allowed range of +/- LIMITS centuries around J2000.0
	// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
	// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
	// of JulianDay, we use the maximum Julian Day minus 2 days.
	getPrecessionAnglesOwenLIDelta(J2000 + LIMITS * DJC - 2, &L0, &dummy_I0, &dummy_Delta);
	getPrecessionAnglesOwenLIDelta(J2000, &L0, &dummy_I0, &dummy_Delta);
	return L0;
}

// Return the invariable plane angles I0 at J2000
double getPrecessionAnglesOwenI0(void)
{
	double dummy_L0, I0, dummy_Delta;
	// We have to call this twice with different dates to avoid returning the cached value.
	// However we must pick a Julian Day that is within the allowed range of +/- LIMITS centuries around J2000.0
	// and also that we will not come closer than the PRECESSION_EPOCH_THRESHOLD set in precession.c.
	// Currently PRECESSION_EPOCH_THRESHOLD is set to 1 day, so to allow verification of the largest value
	// of JulianDay, we use the maximum Julian Day minus 2 days.
	getPrecessionAnglesOwenLIDelta(J2000 + LIMITS * DJC - 2, &dummy_L0, &I0, &dummy_Delta);
	getPrecessionAnglesOwenLIDelta(J2000, &dummy_L0, &I0, &dummy_Delta);
	return I0;
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
//! Ref: Dennis D. McCarthy and Brian J. Luzum: An Abridged Model of the Precession-Nutation of the Celestial Pole.
//! Celestial Mechanics and Dynamical Astronomy 85: 37-49, 2003.
//! This model provides accuracy better than 1 milli-arcsecond in the time 1995-2050.
//! TODO: find out drift rate behaviour e.g. in 17./18. century, maybe use nutation only e.g. 1610-2200?
//! @return deltaPsi, deltaEps [radians]
//! @param JDE Julian Day, TT
//! @note The model promises mas accuracy in the present era but gives no comment on long-time effects. Given that nutation was discovered in the early 18th century,
//! we used to set the returned values to zero before 1500 and after 2500. However, for better comparison with reference values,
//! we now provide non-zero results for the time range -4000...+8000.
//! To avoid a jump, a linear fade-in/fade-out is applied within 100 days before and after the limit dates.
void getNutationAngles(const double JDE, double *deltaPsi, double *deltaEpsilon)
{
// 1.1.1500
//#define NUT_BEGIN 2268932.5
// 1.1.-4000
#define NUT_BEGIN 260057.5
// 1.1.2500
//#define NUT_END 2634166.5
// 1.1.8000
#define NUT_END	4642999.5
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
