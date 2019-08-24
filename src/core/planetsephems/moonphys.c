/*
 * Stellarium
 * Copyright (c) 2019 Georg Zotti
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include "moonphys.h"


// Inclination of Moon's axis against its orbit.
static const double I =  (M_PI/180.0)* 1.54242;
static const double Table47A[60][6];
static const double Table47B[60][5];
/*
 * Compute angles related to Lunar physical ephemeris.
 * J. Meeus, Astronomical Algorithms (2nd. ed) 1998, chapter 47.
 */
#pragma warning(disable:4146)
void computeMoonAngles(const double JDE, double *Lp, double *D, double *M, double *Mp, double *E, double *F, double *Omega, double *lambda, double *beta, double *r, bool doLBR)
{
	const double T=(JDE-2451545.0)/36525.0;
	*Lp=(M_PI/180.0)* (((((-1.0/65194000.0)*T +(1.0/538841.0))*T -0.0015786)*T +481267.88123421)*T + 218.3164477);
	*D =(M_PI/180.0)* (((((-1.0/113065000.0)*T +(1.0/545868.0))*T -0.0018819)*T +445267.1114034)*T + 297.8501921);
	*M =(M_PI/180.0)* ( (((1.0/24490000)*T -0.0001536)*T +35999.0502909)*T + 357.5291092);
	*Mp=(M_PI/180.0)* (((((-1.0/14712000)*T +(1.0/69699.0))*T +0.0087414)*T +477198.8675055)*T + 134.9633964);
	*F =(M_PI/180.0)* ( ((((1.0/863310000.0)*T -(1.0/3526000.0))*T -0.0036539)*T +483202.0175233)*T + 93.2720950);
	*Omega = (M_PI/180.0)* (((((-1.0/60616000)*T + (1.0/467441))*T + 0.0020754)*T -1934.1362891)*T + 125.0445479);
	*E=(0.0000074 * T -0.002516)*T +1.0;
	if(doLBR){
		const double A1 =(M_PI/180.0)*  (131.849 * T + 119.75);
		const double A2 =(M_PI/180.0)*  (479264.290 * T + 53.09);
		const double A3 =(M_PI/180.0)*  (481266.484 * T + 313.45);
		double sumL=0.0;
		double sumR=0.0;
		for (unsigned int i=59; i!=-1u; i--)
		{
			assert(i>=0);
			const double arg=*D*Table47A[i][0] + *M*Table47A[i][1] + *Mp*Table47A[i][2] + *F*Table47A[i][3];
			double lterm=sin(arg)*Table47A[i][4];
			double rterm=cos(arg)*Table47A[i][5];
			if (Table47A[i][1] != 0.0)
			{
				const double Emul=pow(*E, fabs(Table47A[i][1]));
				lterm *= Emul;
				rterm *= Emul;
			}
			sumL += lterm;
			sumR += rterm;
		}
		sumL += 318.0*sin(A2) + 1962.0*sin(*Lp-*F) + 3958.0*sin(A1);
		double sumB=0.0;
		for (unsigned int i=59; i!=-1u; i--)
		{
			const double arg=*D*Table47B[i][0] + *M*Table47B[i][1] + *Mp*Table47B[i][2] + *F*Table47B[i][3];
			double bterm=sin(arg)*Table47B[i][4];
			if (Table47B[i][1] != 0.0)
			{
				const double Emul=pow(*E, fabs(Table47B[i][1]));
				bterm *= Emul;
			}
			sumB += bterm;
		}
		sumB += -115.0*sin(*Lp+*Mp) + 127.0*sin(*Lp-*Mp) + 175.0*(sin(A1+*F)+sin(A1-*F)) + 382.0*sin(A3) - 2235.0*sin(*Lp);
		*lambda = *Lp + (M_PI/180.0)* sumL*1.e-6;
		*beta = (M_PI/180.0)* sumB * 1.e-6;
		*r = sumR*0.001 + 385000.56;
	}
}
#pragma warning(enable:4146)

void computeLibrations(const double T, const double M, const double Mp, const double D, const double E, const double F, const double Omega,
		       const double lambda, const double dPsi, const double beta, const double alpha, const double epsilon,
		       double *W, double *lp, double *bp, double *lpp, double *bpp, double *PA)
{
	*W = lambda-dPsi-Omega;
	const double A = atan2(sin(*W)*cos(beta)*cos(I) - sin(beta)*sin(I), cos(*W)*cos(beta));
	*lp = A-F;
	*bp = asin(-sin(*W)*cos(beta)*sin(I) - sin(beta)*cos(I));

	const double K1 = (M_PI/180.0)* (T*131.849+119.75);
	const double K2 = (M_PI/180.0)* (T*20.186+72.56);
	const double rho = (M_PI/180.0)*0.00014 * cos(Mp+2.0*(F-D))
			-  (M_PI/180.0)*0.00020 * (cos(Mp-F) + cos (Mp+2.0*F) + sin(Mp+F))
			-  (M_PI/180.0)*0.00054 * cos(Mp-2.0*D)
			-  (M_PI/180.0)*0.00085 * cos(2.0*(F-D))
			-  (M_PI/180.0)*0.00293 * cos(2.0*F)
			+  (M_PI/180.0)*0.00684 * cos(Mp-2.0*F)
			-  (M_PI/180.0)*0.02245 * sin(F)
			-  (M_PI/180.0)*0.02752 * cos(Mp);
	const double sigma = -(M_PI/180.0)*0.00010 * cos(Mp-3.0*F)
			+     (M_PI/180.0)*0.00013 * sin(Mp+2.0*(F-D))
			+     (M_PI/180.0)*0.00019 * sin(Mp-F)
			+     (M_PI/180.0)*0.00020 * cos (Mp-F)
			-     (M_PI/180.0)*0.00023 * sin(Mp+2.0*F)
			-     (M_PI/180.0)*0.00025 * sin(2.0*Mp)
			+     (M_PI/180.0)*0.00040 * cos(Mp+F)
			+     (M_PI/180.0)*0.00069 * sin(Mp-2.0*D)
			-     (M_PI/180.0)*0.00083 * sin(2.0*(F-D))
			-     (M_PI/180.0)*0.00279 * sin(2.0*F)
			-     (M_PI/180.0)*0.00682 * sin(Mp-2.0*F)
			+     (M_PI/180.0)*0.02244 * cos(F)
			-     (M_PI/180.0)*0.02816 * sin(Mp);
	const double tau = (M_PI/180.0)*0.00011 * sin(2.0*(Mp-M-D))
			-  (M_PI/180.0)*0.00012 * ( sin(2.0*Mp) + sin(Mp-2.0*F) )
			+  (M_PI/180.0)*0.00014 * ( cos(2.0*(Mp-F)) - sin(2.0*D))
			+  (M_PI/180.0)*0.00023 * sin(K2)
			+  (M_PI/180.0)*0.00027 * sin(2.0*(Mp-D)-M)
			-  (M_PI/180.0)*0.00032 * sin(Mp-M-D)
			-  (M_PI/180.0)*0.00039 * sin(Mp-F)
			+  (M_PI/180.0)*0.00046 * sin(2.0*(F-D))
			-  (M_PI/180.0)*0.00096 * sin(Mp-D)
			+  (M_PI/180.0)*0.00115 * sin(Mp-2.0*D)
			-  (M_PI/180.0)*0.00183 * cos(Mp-F)
			+  (M_PI/180.0)*0.00196 * sin(Omega)
			+  (M_PI/180.0)*0.00276 * sin(2.0*(Mp-D))
			+  (M_PI/180.0)*0.00396 * sin(K1)
			-  (M_PI/180.0)*0.00467 * sin(Mp)
			+  (M_PI/180.0)*0.00473 * sin(2.0*(Mp-F))
			+  (M_PI/180.0)*0.02520 * E * sin(M);

	*lpp = -tau + (rho * cos(A) + sigma * sin(A))*tan(*bp);
	*bpp = sigma*cos(A) - rho*sin(A);
	const double V = Omega+dPsi+sigma/sin(I);
	const double X = sin(I+rho)*sin(V);
	const double Y = sin(I+rho)*cos(V)*cos(epsilon) - cos(I+rho)*sin(epsilon);
	const double omega = atan2(X, Y);
	*PA=asin((sqrt(X*X+Y*Y) * cos(alpha-omega)) / cos(*bp+*bpp));
}

static const double Table47A[60][6]=
{
	{0,  0,  1,  0, 6288774, -20905355},
	{2,  0, -1,  0, 1274027,  -3699111},
	{2,  0,  0,  0,  658314,  -2955968},
	{0,  0,  2,  0,  213618,   -569925},
	{0,  1,  0,  0, -185116,     48888},
	{0,  0,  0,  2, -114332,     -3149},
	{2,  0, -2,  0,   58793,    246158},
	{2, -1, -1,  0,   57066,   -152138},
	{2,  0,  1,  0,   53322,   -170733},
	{2, -1,  0,  0,   45758,   -204586},
	{0,  1, -1,  0,  -40923,   -129620},
	{1,  0,  0,  0,  -34720,    108743},
	{0,  1,  1,  0,  -30383,    104755},
	{2,  0,  0, -2,   15327,     10321},
	{0,  0,  1,  2,  -12528,         0},
	{0,  0,  1, -2,   10980,     79661},
	{4,  0, -1,  0,   10675,    -34782},
	{0,  0,  3,  0,   10034,    -23210},
	{4,  0, -2,  0,    8548,    -21636},
	{2,  1, -1,  0,   -7888,     24208},
	{2,  1,  0,  0,   -6766,     30824},
	{1,  0, -1,  0,   -5163,     -8379},
	{1,  1,  0,  0,    4987,    -16675},
	{2, -1,  1,  0,    4036,    -12831},
	{2,  0,  2,  0,    3994,    -10445},
	{4,  0,  0,  0,    3861,    -11650},
	{2,  0, -3,  0,    3665,     14403},
	{0,  1, -2,  0,   -2689,     -7003},
	{2,  0, -1,  2,   -2602,         0},
	{2, -1, -2,  0,    2390,     10056},
	{1,  0,  1,  0,   -2348,      6322},
	{2, -2,  0,  0,    2236,     -9884},
	{0,  1,  2,  0,   -2120,      5751},
	{0,  2,  0,  0,   -2069,         0},
	{2, -2, -1,  0,    2048,     -4950},
	{2,  0,  1, -2,   -1773,      4130},
	{2,  0,  0,  2,   -1595,         0},
	{4, -1, -1,  0,    1215,     -3958},
	{0,  0,  2,  2,   -1110,         0},
	{3,  0, -1,  0,    -892,      3258},
	{2,  1,  1,  0,    -810,      2616},
	{4, -1, -2,  0,     759,     -1897},
	{0,  2, -1,  0,    -713,     -2117},
	{2,  2, -1,  0,    -700,      2354},
	{2,  1, -2,  0,     691,         0},
	{2, -1,  0, -2,     596,         0},
	{4,  0,  1,  0,     549,     -1423},
	{0,  0,  4,  0,     537,     -1117},
	{4, -1,  0,  0,     520,     -1571},
	{1,  0, -2, -0,    -487,     -1739},
	{2,  1,  0, -2,    -399,         0},
	{0,  0,  2, -2,    -381,     -4421},
	{1,  1,  1,  0,     351,         0},
	{3,  0, -2,  0,    -340,         0},
	{4,  0, -3,  0,     330,         0},
	{2, -1,  2,  0,     327,         0},
	{0,  2,  1,  0,    -323,      1165},
	{1,  1, -1,  0,     299,         0},
	{2,  0,  3,  0,     294,         0},
	{2,  0, -1, -2,       0,      8752}
};
static const double Table47B[60][5]=
{
	{0,  0,  0,  1, 5128122},
	{0,  0,  1,  1,  280602},
	{0,  0,  1, -1,  277693},
	{2,  0,  0, -1,  173237},
	{2,  0, -1,  1,   55413},
	{2,  0, -1, -1,   46271},
	{2,  0,  0,  1,   32573},
	{0,  0,  2,  1,   17198},
	{2,  0,  1, -1,    9266},
	{0,  0,  2, -1,    8822},
	{2, -1,  0, -1,    8216},
	{2,  0, -2, -1,    4324},
	{2,  0,  1,  1,    4200},
	{2,  1,  0, -1,   -3359},
	{2, -1, -1,  0,    2463},
	{2, -1,  0,  1,    2211},
	{2, -1, -1, -1,    2065},
	{0,  1, -1, -1,   -1870},
	{4,  0, -1, -1,    1828},
	{0,  1,  0,  1,   -1794},
	{0,  0,  0,  3,   -1749},
	{0,  1, -1,  1,   -1565},
	{1,  0,  0,  1,   -1491},
	{0,  1,  1,  1,   -1475},
	{0,  1,  1, -1,   -1410},
	{0,  1,  0, -1,   -1344},
	{1,  0,  0, -1,   -1335},
	{0,  0,  3,  1,    1107},
	{4,  0,  0, -1,    1021},
	{4,  0, -1,  1,     833},
	{0,  0,  1, -3,     777},
	{4,  0, -2,  1,     671},
	{2,  0,  0, -3,     607},
	{2,  0,  2, -1,     596},
	{2, -1,  1, -1,     491},
	{2,  0, -2,  1,    -451},
	{0,  0,  3, -1,     439},
	{2,  0,  2,  1,     422},
	{2,  0, -3, -1,     421},
	{2,  1, -1,  1,    -366},
	{2,  1,  0,  1,    -351},
	{4,  0,  0,  1,     331},
	{2, -1,  1,  1,     315},
	{2, -2,  0, -1,     302},
	{0,  0,  1,  3,    -283},
	{2,  1,  1, -1,    -229},
	{1,  1,  0, -1,     223},
	{1,  1,  0,  1,     223},
	{0,  1, -2, -1,    -220},
	{2,  1, -1, -1,    -220},
	{1,  0,  1,  1,    -185},
	{2, -1, -2, -1,     181},
	{0,  1,  2,  1,    -177},
	{4,  0, -2, -1,     176},
	{4, -1, -1, -1,     166},
	{1,  0,  1, -1,    -164},
	{4,  0,  1, -1,     132},
	{1,  0, -1, -1,    -119},
	{4, -1,  0, -1,     115},
	{2, -2,  0,  1,     107}
};
