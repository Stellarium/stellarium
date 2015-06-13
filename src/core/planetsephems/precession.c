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
 */

#include <math.h>

static double precVals[18][7]=
{ // 1/Pn     psi_A:Cn       om_A:Cn       chi_A:Cn         psi_A:Sn       om_A:Sn       chi_A:Sn

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



// compute angles for the series we are in fact using.
// TODO: Decide whether we buffer the time with some static vars including lastJD...
// jd: date JD_TT  
// 
void getPrecessionAnglesVondrak(const double jd, double *chi_A, double *omega_A, double *psi_A)
{
	double T=(jd-2451545.0)* (1.0/36525.0);
	double T2pi= T*(2.0*M_PI); // Julian centuries from J2000.0, premultiplied by 2Pi
	double Psi_A=0.0;
	double Omega_A=0.0;
	double Chi_A=0.0;
	int i;
	for (i=0; i<18; ++i)
	{
		double invP=precVals[i][0];
		double sin2piT_P, cos2piT_P;
#ifdef _GNU_SOURCE
		sincos(T2pi/P, &sin2piT_P, &cos2piT_P);
#else
		double phase=T2pi*invP;
		sin2piT_P= sin(phase);
		cos2piT_P= cos(phase);
#endif
		Psi_A   += precVals[i][1]*cos2piT_P + precVals[i][4]*sin2piT_P;
		Omega_A += precVals[i][2]*cos2piT_P + precVals[i][5]*sin2piT_P;
		Chi_A   += precVals[i][3]*cos2piT_P + precVals[i][6]*sin2piT_P;
	}
	Psi_A   += ((289.e-9*T - 0.00740913)*T +5042.7980307)*T +  8473.343527;
	Omega_A += ((151.e-9*T + 0.00000146)*T -   0.4436568)*T + 84283.175915;
	Chi_A   += ((-61.e-9*T + 0.00001472)*T +   0.0790159)*T -    19.657270;

	const double arcSec2Rad=M_PI*2.0/(360.0*3600.0);
	*psi_A   = arcSec2Rad*Psi_A;
	*omega_A = arcSec2Rad*Omega_A;
	*chi_A   = arcSec2Rad*Chi_A;
}
