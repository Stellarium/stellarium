/*
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

Copyright (C) 2002 Liam Girdwood <liam@nova-ioe.org>

The functions in this file use the Lunar Solution ELP 2000-82B by
Michelle Chapront-Touze and Jean Chapront.

*/

#include "lunar.h"
#include <math.h>

/* AU in KM */
#define AU			149597870.691

/* sequence sizes */
#define ELP1_SIZE	1023  	  	/* Main problem. Longitude periodic terms (sine) */
#define ELP2_SIZE	918    		/* Main problem. Latitude (sine) */
#define ELP3_SIZE	704    		/* Main problem. Distance (cosine) */
#define ELP4_SIZE	347    		/* Earth figure perturbations. Longitude */
#define ELP5_SIZE	316   		/* Earth figure perturbations. Latitude */
#define ELP6_SIZE	237    		/* Earth figure perturbations. Distance */
#define ELP7_SIZE	14    		/* Earth figure perturbations. Longitude/t */
#define ELP8_SIZE	11    		/* Earth figure perturbations. Latitude/t */
#define ELP9_SIZE	8    		/* Earth figure perturbations. Distance/t */
#define ELP10_SIZE	14328    	/* Planetary perturbations. Table 1 Longitude */
#define ELP11_SIZE	5233    	/* Planetary perturbations. Table 1 Latitude */
#define ELP12_SIZE	6631    	/* Planetary perturbations. Table 1 Distance */
#define ELP13_SIZE	4384    	/* Planetary perturbations. Table 1 Longitude/t */
#define ELP14_SIZE	833    		/* Planetary perturbations. Table 1 Latitude/t */
#define ELP15_SIZE	1715    	/* Planetary perturbations. Table 1 Distance/t */
#define ELP16_SIZE	170    		/* Planetary perturbations. Table 2 Longitude */
#define ELP17_SIZE	150    		/* Planetary perturbations. Table 2 Latitude */
#define ELP18_SIZE	114    		/* Planetary perturbations. Table 2 Distance */
#define ELP19_SIZE	226    		/* Planetary perturbations. Table 2 Longitude/t */
#define ELP20_SIZE	188    		/* Planetary perturbations. Table 2 Latitude/t */
#define ELP21_SIZE	169    		/* Planetary perturbations. Table 2 Distance/t */
#define ELP22_SIZE	3    		/* Tidal effects. Longitude */
#define ELP23_SIZE	2    		/* Tidal effects. Latitude */
#define ELP24_SIZE	2    		/* Tidal effects. Distance */
#define ELP25_SIZE	6    		/* Tidal effects. Longitude/t */
#define ELP26_SIZE	4    		/* Tidal effects. Latitude/t */
#define ELP27_SIZE	5    		/* Tidal effects. Distance/t */
#define ELP28_SIZE	20    		/* Moon figure perturbations. Longitude */
#define ELP29_SIZE	12    		/* Moon figure perturbations. Latitude */
#define ELP30_SIZE	14    		/* Moon figure perturbations. Distance */
#define ELP31_SIZE	11    		/* Relativistic perturbations. Longitude */
#define ELP32_SIZE	4    		/* Relativistic perturbations. Latitude */
#define ELP33_SIZE	10    		/* Relativistic perturbations. Distance */
#define ELP34_SIZE	28    		/* Planetary perturbations - solar eccentricity. Longitude/t2 */
#define ELP35_SIZE	13    		/* Planetary perturbations - solar eccentricity. Latitude/t2 */
#define ELP36_SIZE	19    		/* Planetary perturbations - solar eccentricity. Distance/t2 */


/* Chapront theory lunar constants */
#define 	RAD		(648000.0 / M_PI)
#define		DEG		(M_PI / 180.0)
#define		M_PI2 	(2.0 * M_PI)
#define		PIS2	(M_PI / 2.0)
#define 	ATH		384747.9806743165
#define 	A0		384747.9806448954
#define		AM	 	0.074801329518
#define		ALPHA 	0.002571881335
#define		DTASM	(2.0 * ALPHA / (3.0 * AM))
#define		W12		(1732559343.73604 / RAD)
#define		PRECES	(5029.0966 / RAD)
#define		C1		60.0
#define		C2		3600.0

/* Corrections of the constants for DE200/LE200 */
#define		DELNU	((0.55604 / RAD) / W12)
#define		DELE	(0.01789 / RAD)
#define		DELG	(-0.08066 / RAD)
#define		DELNP	((-0.06424 / RAD) / W12)
#define 	DELEP	(-0.12879 / RAD)

/*     Precession matrix */
#define		P1		0.10180391e-4
#define		P2		0.47020439e-6
#define		P3		-0.5417367e-9
#define		P4		-0.2507948e-11
#define		P5		0.463486e-14
#define		Q1		-0.113469002e-3
#define		Q2		0.12372674e-6
#define		Q3		0.1265417e-8
#define		Q4		-0.1371808e-11
#define		Q5		-0.320334e-14


/* constants with corrections for DE200 / LE200 */
static const double W1[5] = 
{
	((218.0 + (18.0 / 60.0) + (59.95571 / 3600.0))) * DEG,
	1732559343.73604 / RAD,
	-5.8883 / RAD,
	0.006604 / RAD,
	-0.00003169 / RAD
}; 

static const double W2[5] = 
{
	((83.0 + (21.0 / 60.0) + (11.67475 / 3600.0))) * DEG,
	14643420.2632 / RAD,
	-38.2776 /  RAD,
	-0.045047 / RAD,
	0.00021301 / RAD
};

static const double W3[5] = 
{
	(125.0 + (2.0 / 60.0) + (40.39816 / 3600.0)) * DEG,
	-6967919.3622 / RAD,
	6.3622 / RAD,
	0.007625 / RAD,
	-0.00003586 / RAD
};

static const double earth[5] = 
{
	(100.0 + (27.0 / 60.0) + (59.22059 / 3600.0)) * DEG,
	129597742.2758 / RAD,
	-0.0202 / RAD,
	0.000009 / RAD,
	0.00000015 / RAD	
};

static const double peri[5] =
{
	(102.0 + (56.0 / 60.0) + (14.42753 / 3600.0)) * DEG,
	1161.2283 / RAD,
	0.5327 / RAD,
	-0.000138 / RAD,
	0
};

/* Delaunay's arguments.*/
static const double del[4][5] = {
	{ 5.198466741027443, 7771.377146811758394, -0.000028449351621, 0.000000031973462, -0.000000000154365 },
	{ -0.043125180208125, 628.301955168488007, -0.000002680534843, 0.000000000712676, 0.000000000000727 },
	{ 2.355555898265799, 8328.691426955554562, 0.000157027757616, 0.000000250411114, -0.000000001186339 },
	{ 1.627905233371468, 8433.466158130539043, -0.000059392100004, -0.000000004949948, 0.000000000020217 }
};


static const double zeta[2] = 
{
	(218.0 + (18.0 / 60.0) + (59.95571 / 3600.0)) * DEG, 
	((1732559343.73604 / RAD) + PRECES)
};


/* Planetary arguments */
static const double p[8][2] = 
{
	{(252 + 15 / C1 + 3.25986 / C2 ) * DEG, 538101628.68898 / RAD },
	{(181 + 58 / C1 + 47.28305 / C2) * DEG, 210664136.43355 / RAD },
	{(100.0 + (27.0 / 60.0) + (59.22059 / 3600.0)) * DEG, 129597742.2758 / RAD},
	{(355 + 25 / C1 + 59.78866 / C2) * DEG, 68905077.59284 / RAD },
	{(34 + 21 / C1 + 5.34212 / C2) * DEG, 10925660.42861 / RAD },
	{(50 + 4 / C1 + 38.89694 / C2) * DEG, 4399609.65932 / RAD },
	{(314 + 3 / C1 + 18.01841 / C2) * DEG, 1542481.19393 / RAD },
	{(304 + 20 / C1 + 55.19575 / C2) * DEG, 786550.32074 / RAD }
};

/* precision */
static double pre[3];

#include "lunar_mainproblem.h"
#include "lunar_planetpert1.h"
#include "lunar_planetpert2.h"


static const tidal_effects tidal_effects_elp22 [ELP22_SIZE] = 
{
	{0,{1,1,-1,-1},192.936650,0.000040,0.075},
	{0,{1,1,0,-1},192.936650,0.000820,18.600},
	{0,{1,1,1,-1},192.936650,0.000040,0.076}
};


static const tidal_effects tidal_effects_elp23 [ELP23_SIZE] = 
{
	{0,{1,1,0,-2},192.936630,0.000040,0.074},
	{0,{1,1,0,0},192.936640,0.000040,0.075}
};

static const tidal_effects tidal_effects_elp24 [ELP24_SIZE] = 
{
	{0,{1,1,-1,-1},282.936650,0.000040,0.075},
	{0,{1,1,1,-1},102.936650,0.000040,0.076}
};

static const tidal_effects tidal_effects_elp25 [ELP25_SIZE] = 
{
	{0,{0,0,1,0},0.,0.000580,0.075},
	{0,{0,0,2,0},0.,0.000040,0.038},
	{0,{2,0,-2,0},0.,0.000020,0.564},
	{0,{2,0,-1,0},0.,0.000210,0.087},
	{0,{2,0,0,0},0.,0.000090,0.040},
	{0,{2,0,1,0},0.,0.000010,0.026}
};

static const tidal_effects tidal_effects_elp26 [ELP26_SIZE] = 
{
	{0,{0,0,0,1},180.00,0.000050,0.075},
	{0,{0,0,1,-1},0.,0.000030,5.997},
	{0,{0,0,1,1},0.,0.000030,0.037},
	{0,{2,0,0,-1},0.,0.000010,0.088}
};

static const tidal_effects tidal_effects_elp27 [ELP27_SIZE] = 
{
	{0,{0,0,0,0},90.00,0.003560,99999.999},
	{0,{0,0,1,0},270.00,0.000720,0.075},
	{0,{0,0,2,0},270.00,0.000030,0.038},
	{0,{2,0,-1,0},270.00,0.000190,0.087},
	{0,{2,0,0,0},270.00,0.000130,0.040}
};

static const moon_pert moon_pert_elp28 [ELP28_SIZE] = 
{
	{0,{0,0,0,1},303.961850,0.000040,0.075},
	{0,{0,0,1,-1},259.883930,0.000160,5.997},
	{0,{0,0,2,-2},0.430200,0.000400,2.998},
	{0,{0,0,3,-2},0.433790,0.000020,0.077},
	{0,{0,1,-1,0},359.998580,0.000140,0.082},
	{0,{0,1,0,0},359.999820,0.002230,1.000},
	{0,{0,1,1,0},359.999610,0.000140,0.070},
	{0,{1,0,-1,0},359.993310,0.000090,1.127},
	{0,{1,0,0,0},359.995370,0.000010,0.081},
	{0,{1,1,-1,0},0.064180,0.000030,8.850},
	{0,{2,-1,-1,0},180.000950,0.000040,0.095},
	{0,{2,-1,0,0},180.000140,0.000030,0.042},
	{0,{2,0,-3,0},179.981260,0.000010,0.067},
	{0,{2,0,-2,0},179.983660,0.000250,0.564},
	{0,{2,0,-1,0},179.996380,0.000140,0.087},
	{0,{2,0,0,-2},179.958640,0.000030,0.474},
	{0,{2,0,0,0},179.999040,0.000020,0.040},
	{0,{2,1,-2,0},179.991840,0.000020,1.292},
	{0,{2,1,-1,0},0.003130,0.000020,0.080},
	{0,{2,1,0,0},359.999650,0.000020,0.039}
};


static const moon_pert moon_pert_elp29 [ELP29_SIZE] = 
{
	{0,{0,0,1,-1},0.021990,0.000030,5.997},
	{0,{0,0,1,0},245.990670,0.000010,0.075},
	{0,{0,0,1,1},0.005300,0.000010,0.037},
	{0,{0,0,2,-3},0.422830,0.000020,0.073},
	{0,{0,0,2,-1},0.745050,0.000010,0.076},
	{0,{0,1,-1,-1},359.999820,0.000010,0.039},
	{0,{0,1,0,-1},359.999820,0.000100,0.081},
	{0,{0,1,0,1},359.999820,0.000100,0.069},
	{0,{0,1,1,1},359.999820,0.000010,0.036},
	{0,{2,0,-2,-1},179.983560,0.000010,0.066},
	{0,{2,0,-2,1},179.983530,0.000010,0.086},
	{0,{2,0,0,-1},179.994780,0.000050,0.088}
};

static const moon_pert moon_pert_elp30 [ELP30_SIZE] = 
{
	{0,{0,0,0,0},90.00,0.001300,99999.999},
	{0,{0,0,0,1},213.957200,0.000030,0.075},
	{0,{0,0,0,2},270.037450,0.000020,0.037},
	{0,{0,0,1,0},90.075970,0.000040,0.075},
	{0,{0,0,3,-2},270.434290,0.000020,0.077},
	{0,{0,1,-1,0},89.999190,0.000130,0.082},
	{0,{0,1,0,0},270.000070,0.000220,1.000},
	{0,{0,1,1,0},269.999030,0.000110,0.070},
	{0,{2,-1,-1,0},89.998150,0.000020,0.095},
	{0,{2,-1,0,0},90.000520,0.000030,0.042},
	{0,{2,0,-2,0},269.985850,0.000050,0.564},
	{0,{2,0,-1,0},89.998630,0.000130,0.087},
	{0,{2,1,-1,0},269.999820,0.000020,0.080},
	{0,{2,1,0,0},269.999820,0.000030,0.039}
};


static const rel_pert rel_pert_elp31 [ELP31_SIZE] = 
{
	{0,{0,1,-1,0},179.934730,0.000060,0.082},
	{0,{0,1,0,0},179.985320,0.000810,1.000},
	{0,{0,1,1,0},179.963230,0.000050,0.070},
	{0,{1,0,0,0},0.000010,0.000130,0.081},
	{0,{1,1,0,0},180.022820,0.000010,0.075},
	{0,{2,-1,-1,0},0.022640,0.000020,0.095},
	{0,{2,0,-1,0},359.988260,0.000020,0.087},
	{0,{2,0,0,0},180.000190,0.000550,0.040},
	{0,{2,0,1,0},180.000170,0.000060,0.026},
	{0,{2,1,-1,0},180.749540,0.000010,0.080},
	{0,{4,0,-1,0},180.000350,0.000010,0.028}
};

static const rel_pert rel_pert_elp32 [ELP32_SIZE] = 
{
	{0,{0,1,0,-1},179.998030,0.000040,0.081},
	{0,{0,1,0,1},179.997980,0.000040,0.069},
	{0,{2,0,0,-1},359.998100,0.000020,0.088},
	{0,{2,0,0,1},180.000260,0.000020,0.026}
};

static const rel_pert rel_pert_elp33 [ELP33_SIZE] = 
{
	{0,{0,0,0,0},270.00,0.008280,99999.999},
	{0,{0,0,1,0},89.999940,0.000430,0.075},
	{0,{0,1,-1,0},269.932920,0.000050,0.082},
	{0,{0,1,0,0},270.009080,0.000090,1.000},
	{0,{0,1,1,0},89.957650,0.000050,0.070},
	{0,{1,0,0,0},270.000020,0.000060,0.081},
	{0,{2,-1,0,0},89.970710,0.000020,0.042},
	{0,{2,0,-1,0},269.993670,0.000030,0.087},
	{0,{2,0,0,0},90.000140,0.001060,0.040},
	{0,{2,0,1,0},90.000100,0.000080,0.026}
};


static const plan_sol_pert plan_sol_pert_elp34 [ELP34_SIZE] = 
{
	{0,{0,1,-2,0},0.,0.000070,0.039},
	{0,{0,1,-1,0},0.,0.001080,0.082},
	{0,{0,1,0,0},0.,0.004870,1.000},
	{0,{0,1,1,0},0.,0.000800,0.070},
	{0,{0,1,2,0},0.,0.000060,0.036},
	{0,{0,2,-1,0},0.,0.000040,0.089},
	{0,{0,2,0,0},0.,0.000110,0.500},
	{0,{0,2,1,0},0.,0.000020,0.066},
	{0,{1,1,0,0},180.00,0.000130,0.075},
	{0,{2,-2,-1,0},180.00,0.000110,0.105},
	{0,{2,-2,0,0},180.00,0.000120,0.044},
	{0,{2,-2,1,0},180.00,0.000010,0.028},
	{0,{2,-1,-2,0},180.00,0.000060,0.360},
	{0,{2,-1,-1,0},180.00,0.001500,0.095},
	{0,{2,-1,0,-2},180.00,0.000020,0.322},
	{0,{2,-1,0,0},180.00,0.001200,0.042},
	{0,{2,-1,1,0},180.00,0.000110,0.027},
	{0,{2,0,-1,0},0.,0.000020,0.087},
	{0,{2,0,0,0},0.,0.000030,0.040},
	{0,{2,1,-2,0},180.00,0.000020,1.292},
	{0,{2,1,-1,0},0.,0.000210,0.080},
	{0,{2,1,0,-2},0.,0.000010,0.903},
	{0,{2,1,0,0},0.,0.000180,0.039},
	{0,{2,1,1,0},0.,0.000020,0.026},
	{0,{2,2,-1,0},0.,0.000040,0.074},
	{0,{4,-1,-2,0},180.00,0.000020,0.046},
	{0,{4,-1,-1,0},180.00,0.000030,0.028},
	{0,{4,-1,0,0},180.00,0.000010,0.021}
};

static const plan_sol_pert plan_sol_pert_elp35 [ELP35_SIZE] = 
{
	{0,{0,1,-1,-1},0.,0.000050,0.039},
	{0,{0,1,-1,1},0.,0.000040,0.857},
	{0,{0,1,0,-1},0.,0.000040,0.081},
	{0,{0,1,0,1},0.,0.000050,0.069},
	{0,{0,1,1,-1},0.,0.000040,1.200},
	{0,{0,1,1,1},0.,0.000040,0.036},
	{0,{2,-2,0,-1},180.00,0.000020,0.107},
	{0,{2,-1,-1,-1},180.00,0.000050,0.340},
	{0,{2,-1,-1,1},180.00,0.000060,0.042},
	{0,{2,-1,0,-1},180.00,0.000220,0.097},
	{0,{2,-1,0,1},180.00,0.000060,0.027},
	{0,{2,-1,1,-1},180.00,0.000010,0.042},
	{0,{2,1,0,-1},0.,0.000090,0.081}
};

static const plan_sol_pert plan_sol_pert_elp36 [ELP36_SIZE] = 
{
	{0,{0,1,-2,0},90.00,0.000050,0.039},
	{0,{0,1,-1,0},90.00,0.000950,0.082},
	{0,{0,1,0,0},270.00,0.000360,1.000},
	{0,{0,1,1,0},270.00,0.000770,0.070},
	{0,{0,1,2,0},270.00,0.000040,0.036},
	{0,{0,2,-1,0},90.00,0.000030,0.089},
	{0,{1,1,0,0},90.00,0.000120,0.075},
	{0,{2,-2,-1,0},90.00,0.000070,0.105},
	{0,{2,-2,0,0},90.00,0.000140,0.044},
	{0,{2,-1,-2,0},270.00,0.000070,0.360},
	{0,{2,-1,-1,0},90.00,0.001110,0.095},
	{0,{2,-1,0,0},90.00,0.001490,0.042},
	{0,{2,-1,1,0},90.00,0.000090,0.027},
	{0,{2,0,0,0},270.00,0.000040,0.040},
	{0,{2,1,-1,0},270.00,0.000180,0.080},
	{0,{2,1,0,0},270.00,0.000230,0.039},
	{0,{2,1,1,0},270.00,0.000020,0.026},
	{0,{2,2,-1,0},270.00,0.000030,0.074},
	{0,{4,-1,-1,0},90.00,0.000030,0.028}
};

/* sum lunar elp1 series */
double sum_series_elp1 (double* t)
{
	double result = 0;
	double x,y;
	double tgv;
	int i,j,k;
	
  	for (j=0; j< ELP1_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(main_elp1[j].A) > pre[0])
		{
			/* derivatives of A */
			tgv = main_elp1[j].B[0] + DTASM * main_elp1[j].B[4];
			x = main_elp1[j].A + tgv * (DELNP - AM * DELNU) + 
				main_elp1[j].B[1] * DELG + main_elp1[j].B[2] * 
				DELE + main_elp1[j].B[3] * DELEP;
		
			y = 0;
			for (k = 0; k < 5; k++)
			{
				for (i = 0; i < 4; i++) 
				{
					y += main_elp1[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += x * sin (y);
	}
	}
	return (result);
}

/* sum lunar elp2 series */
double sum_series_elp2 (double* t)
{
	double result = 0;
	double x,y;
	double tgv;
	int i,j,k;
	
  	for (j=0; j< ELP2_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(main_elp2[j].A) > pre[1])
		{
			/* derivatives of A */
			tgv = main_elp2[j].B[0] + DTASM * main_elp2[j].B[4];
			x = main_elp2[j].A + tgv * (DELNP - AM * DELNU) + 
				main_elp2[j].B[1] * DELG + main_elp2[j].B[2] * 
				DELE + main_elp2[j].B[3] * DELEP;
		
			y = 0;
			for (k = 0; k < 5; k++)
			{
				for (i = 0; i < 4; i++) 
				{
					y += main_elp2[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += x * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp3 series */
double sum_series_elp3 (double* t)
{
	double result = 0;
	double x,y;
	double tgv;
	int i,j,k;
	
  	for (j=0; j< ELP3_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(main_elp3[j].A) > pre[2])
		{
			/* derivatives of A */
			tgv = main_elp3[j].B[0] + DTASM * main_elp3[j].B[4];
			x = main_elp3[j].A + tgv * (DELNP - AM * DELNU) + 
				main_elp3[j].B[1] * DELG + main_elp3[j].B[2] * 
				DELE + main_elp3[j].B[3] * DELEP;
		
			y = 0;
			for (k = 0; k < 5; k++)
			{
				for (i = 0; i < 4; i++) 
				{
					y += main_elp3[j].ilu[i] * del[i][k] * t[k];
				}
			}
			y += (M_PI_2);

			result += x * sin (y);
		}
	}
	return (result);
}


/* sum lunar elp4 series */
double sum_series_elp4 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP4_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(earth_pert_elp4[j].A) > pre[0])
		{
			y = earth_pert_elp4[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += earth_pert_elp4[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += earth_pert_elp4[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += earth_pert_elp4[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp5 series */
double sum_series_elp5 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP5_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(earth_pert_elp5[j].A) > pre[1])
		{
			y = earth_pert_elp5[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += earth_pert_elp5[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += earth_pert_elp5[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += earth_pert_elp5[j].A * sin (y);
		}
	}
	return (result);
}


/* sum lunar elp6 series */
double sum_series_elp6 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP6_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(earth_pert_elp6[j].A) > pre[2])
		{
			y = earth_pert_elp6[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += earth_pert_elp6[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += earth_pert_elp6[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += earth_pert_elp6[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp7 series */
double sum_series_elp7 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP7_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(earth_pert_elp7[j].A) > pre[0])
		{
			A = earth_pert_elp7[j].A * t[1];
			y = earth_pert_elp7[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += earth_pert_elp7[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += earth_pert_elp7[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp8 series */
double sum_series_elp8 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP8_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(earth_pert_elp8[j].A) > pre[1])
		{
			y = earth_pert_elp8[j].O * DEG;
			A = earth_pert_elp8[j].A * t[1];
			for (k = 0; k < 2; k++) 
			{
				y += earth_pert_elp8[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += earth_pert_elp8[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp9 series */
double sum_series_elp9 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP9_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(earth_pert_elp9[j].A) > pre[2])
		{
			A = earth_pert_elp9[j].A * t[1];
			y = earth_pert_elp9[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += earth_pert_elp9[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++)
				{
					y += earth_pert_elp9[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp10 series */
double sum_series_elp10 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP10_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp10[j].O) > pre[0])
		{
			y = plan_pert_elp10[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += (plan_pert_elp10[j].ipla[8] * del[0][k] 
				+ plan_pert_elp10[j].ipla[9] * del[2][k]
				+ plan_pert_elp10[j].ipla[10] * del [3][k]) * t[k];
				for (i = 0; i < 8; i++) 
				{
					y += plan_pert_elp10[j].ipla[i] * p[i][k] * t[k];
				}
			}

			result += plan_pert_elp10[j].O * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp11 series */
double sum_series_elp11 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP11_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp11[j].O) > pre[1])
		{
			y = plan_pert_elp11[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += (plan_pert_elp11[j].ipla[8] * del[0][k] 
				+ plan_pert_elp11[j].ipla[9] * del[2][k] 
				+ plan_pert_elp11[j].ipla[10] * del [3][k]) * t[k];
				for (i = 0; i < 8; i++) 
				{
					y += plan_pert_elp11[j].ipla[i] * p[i][k] * t[k];
				}
			}

			result += plan_pert_elp11[j].O * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp12 series */
double sum_series_elp12 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP12_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp12[j].O) > pre[2])
		{
			y = plan_pert_elp12[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += (plan_pert_elp12[j].ipla[8] * del[0][k] 
				+ plan_pert_elp12[j].ipla[9] * del[2][k] 
				+ plan_pert_elp12[j].ipla[10] * del [3][k]) * t[k];
				for (i = 0; i < 8; i++) 
				{
					y += plan_pert_elp12[j].ipla[i] * p[i][k] * t[k];
				}
			}

			result += plan_pert_elp12[j].O * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp13 series */
double sum_series_elp13 (double *t)
{
	double result = 0;
	int i,j,k;
	double y,x;
	
	for (j=0; j< ELP13_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp13[j].O) > pre[0])
		{
			y = plan_pert_elp13[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += (plan_pert_elp13[j].ipla[8] * del[0][k] 
				+ plan_pert_elp13[j].ipla[9] * del[2][k] 
				+ plan_pert_elp13[j].ipla[10] * del [3][k]) * t[k];
				for (i = 0; i < 8; i++) 
				{
					y += plan_pert_elp13[j].ipla[i] * p[i][k] * t[k];
				}
			}

			x = plan_pert_elp13[j].O * t[1];
			result += x * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp14 series */
double sum_series_elp14 (double *t)
{
	double result = 0;
	int i,j,k;
	double y,x;
	
	for (j=0; j< ELP14_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp14[j].O) > pre[1])
		{
			y = plan_pert_elp14[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += (plan_pert_elp14[j].ipla[8] * del[0][k] 
				+ plan_pert_elp14[j].ipla[9] * del[2][k] 
				+ plan_pert_elp14[j].ipla[10] * del [3][k]) * t[k];
				for (i = 0; i < 8; i++) 
				{
					y += plan_pert_elp14[j].ipla[i] * p[i][k] * t[k];
				}
			}

			x = plan_pert_elp14[j].O * t[1];
			result += x * sin (y);
		}
	}
	return (result);
}


/* sum lunar elp15 series */
double sum_series_elp15 (double *t)
{
	double result = 0;
	int i,j,k;
	double y,x;
	
	for (j=0; j< ELP15_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp15[j].O) > pre[2])
		{
			y = plan_pert_elp15[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += (plan_pert_elp15[j].ipla[8] * del[0][k] 
				+ plan_pert_elp15[j].ipla[9] * del[2][k] 
				+ plan_pert_elp15[j].ipla[10] * del [3][k]) * t[k];
				for (i = 0; i < 8; i++) 
				{
					y += plan_pert_elp15[j].ipla[i] * p[i][k] * t[k];
				}
			}

			x = plan_pert_elp15[j].O * t[1];
			result += x * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp16 series */
double sum_series_elp16 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP16_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp16[j].O) > pre[0])
		{
			y = plan_pert_elp16[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				for (i = 0; i < 4; i++) 
				{
					y += plan_pert_elp16[j].ipla[i + 7] * del[i][k] * t[k];
				}
				for (i = 0; i < 7; i++) 
				{
					y += plan_pert_elp16[j].ipla[i] * p[i][k] * t[k];
				}
			}

			result += plan_pert_elp16[j].O * sin (y);
		}
	}
	return (result);
}

double sum_series_elp17 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP17_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp17[j].O) > pre[1])
		{
			y = plan_pert_elp17[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				for (i = 0; i < 4; i++) 
				{
					y += plan_pert_elp17[j].ipla[i + 7] * del[i][k] * t[k];
				}
				for (i = 0; i < 7; i++) 
				{
					y += plan_pert_elp17[j].ipla[i] * p[i][k] * t[k];
				}
			}

			result += plan_pert_elp17[j].O * sin (y);
		}
	}
	return (result);
}

double sum_series_elp18 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP18_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp18[j].O) > pre[2])
		{
			y = plan_pert_elp18[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				for (i = 0; i < 4; i++) 
				{
					y += plan_pert_elp18[j].ipla[i + 7] * del[i][k] * t[k];
				}
				for (i = 0; i < 7; i++) 
				{
					y += plan_pert_elp18[j].ipla[i] * p[i][k] * t[k];
				}
			}

			result += plan_pert_elp18[j].O * sin (y);
		}
	}
	return (result);
}

double sum_series_elp19 (double *t)
{
	double result = 0;
	int i,j,k;
	double y,x;
	
	for (j=0; j< ELP19_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp19[j].O) > pre[0])
		{
			y = plan_pert_elp19[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				for (i = 0; i < 4; i++) 
				{
					y += plan_pert_elp19[j].ipla[i + 7] * del[i][k] * t[k];
				}
				for (i = 0; i < 7; i++) 
				{
					y += plan_pert_elp19[j].ipla[i] * p[i][k] * t[k];
				}
			}

			x = plan_pert_elp19[j].O * t[1];
			result += x * sin (y);
		}
	}
	return (result);
}

double sum_series_elp20 (double *t)
{
	double result = 0;
	int i,j,k;
	double y,x;
	
	for (j=0; j< ELP20_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp20[j].O) > pre[1])
		{
			y = plan_pert_elp20[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				for (i = 0; i < 4; i++) 
				{
					y += plan_pert_elp20[j].ipla[i + 7] * del[i][k] * t[k];
				}
				for (i = 0; i < 7; i++) 
				{
					y += plan_pert_elp20[j].ipla[i] * p[i][k] * t[k];
				}
			}

			x = plan_pert_elp20[j].O * t[1];
			result += x * sin (y);
		}
	}
	return (result);
}

double sum_series_elp21 (double *t)
{
	double result = 0;
	int i,j,k;
	double y,x;
	
	for (j=0; j< ELP21_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_pert_elp21[j].O) > pre[2])
		{
			y = plan_pert_elp21[j].theta * DEG;
			for (k = 0; k < 2; k++) 
			{
				for (i = 0; i < 4; i++) 
				{
					y += plan_pert_elp21[j].ipla[i + 7] * del[i][k] * t[k];
				}
				for (i = 0; i < 7; i++) 
				{
					y += plan_pert_elp21[j].ipla[i] * p[i][k] * t[k];
				}
			}

			x = plan_pert_elp21[j].O * t[1];
			result += x * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp22 series */
double sum_series_elp22 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP22_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(tidal_effects_elp22[j].A) > pre[0])
		{
			y = tidal_effects_elp22[j].O * DEG;
			for (k = 0; k < 2; k++)
			{
				y += tidal_effects_elp22[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += tidal_effects_elp22[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += tidal_effects_elp22[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp23 series */
double sum_series_elp23 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP23_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(tidal_effects_elp23[j].A) > pre[1])
		{
			y = tidal_effects_elp23[j].O * DEG;
			for (k = 0; k < 2; k++)
			{
				y += tidal_effects_elp23[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += tidal_effects_elp23[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += tidal_effects_elp23[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp24 series */
double sum_series_elp24 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP24_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(tidal_effects_elp24[j].A) > pre[2])
		{
			y = tidal_effects_elp24[j].O * DEG;
			for (k = 0; k < 2; k++)
			{
				y += tidal_effects_elp24[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += tidal_effects_elp24[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += tidal_effects_elp24[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp25 series */
double sum_series_elp25 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP25_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(tidal_effects_elp25[j].A) > pre[0])
		{
			A = tidal_effects_elp25[j].A * t[1];
			y = tidal_effects_elp25[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += tidal_effects_elp25[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += tidal_effects_elp25[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp26 series */
double sum_series_elp26 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP26_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(tidal_effects_elp26[j].A) > pre[1])
		{
			A = tidal_effects_elp26[j].A * t[1];
			y = tidal_effects_elp26[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += tidal_effects_elp26[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += tidal_effects_elp26[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp27 series */
double sum_series_elp27 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP27_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(tidal_effects_elp27[j].A) > pre[2])
		{
			A = tidal_effects_elp27[j].A * t[1];
			y = tidal_effects_elp27[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += tidal_effects_elp27[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += tidal_effects_elp27[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp28 series */
double sum_series_elp28 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP28_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(moon_pert_elp28[j].A) > pre[0])
		{
			y = moon_pert_elp28[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += moon_pert_elp28[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += moon_pert_elp28[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += moon_pert_elp28[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp29 series */
double sum_series_elp29 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP29_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(moon_pert_elp29[j].A) > pre[1])
		{
			y = moon_pert_elp29[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += moon_pert_elp29[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += moon_pert_elp29[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += moon_pert_elp29[j].A * sin (y);
		}
	}
	return (result);
}


/* sum lunar elp30 series */
double sum_series_elp30 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP30_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(moon_pert_elp30[j].A) > pre[2])
		{
			y = moon_pert_elp30[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += moon_pert_elp30[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += moon_pert_elp30[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += moon_pert_elp30[j].A * sin (y);
		}
	}
	return (result);
}


/* sum lunar elp31 series */
double sum_series_elp31 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP31_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(rel_pert_elp31[j].A) > pre[0])
		{
			y = rel_pert_elp31[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += rel_pert_elp31[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += rel_pert_elp31[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += rel_pert_elp31[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp32 series */
double sum_series_elp32 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP32_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(rel_pert_elp32[j].A) > pre[1])
		{
			y = rel_pert_elp32[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += rel_pert_elp32[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += rel_pert_elp32[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += rel_pert_elp32[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp33 series */
double sum_series_elp33 (double *t)
{
	double result = 0;
	int i,j,k;
	double y;
	
	for (j=0; j< ELP33_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(rel_pert_elp33[j].A) > pre[2])
		{
			y = rel_pert_elp33[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += rel_pert_elp33[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += rel_pert_elp33[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += rel_pert_elp33[j].A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp34 series */
double sum_series_elp34 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP34_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_sol_pert_elp34[j].A) > pre[0])
		{
			A = plan_sol_pert_elp34[j].A * t[2];
			y = plan_sol_pert_elp34[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += plan_sol_pert_elp34[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += plan_sol_pert_elp34[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}
/* sum lunar elp35 series */
double sum_series_elp35 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP35_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_sol_pert_elp35[j].A) > pre[1])
		{
			A = plan_sol_pert_elp35[j].A * t[2];
			y = plan_sol_pert_elp35[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += plan_sol_pert_elp35[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += plan_sol_pert_elp35[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}

/* sum lunar elp36 series */
double sum_series_elp36 (double *t)
{
	double result = 0;
	int i,j,k;
	double y, A;
	
	for (j=0; j< ELP36_SIZE; j++)
	{
		/* do we need to calc this value */
		if (fabs(plan_sol_pert_elp36[j].A) > pre[2])
		{
			A = plan_sol_pert_elp36[j].A * t[2];
			y = plan_sol_pert_elp36[j].O * DEG;
			for (k = 0; k < 2; k++) 
			{
				y += plan_sol_pert_elp36[j].iz * zeta[k] * t[k];
				for (i = 0; i < 4; i++) 
				{
					y += plan_sol_pert_elp36[j].ilu[i] * del[i][k] * t[k];
				}
			}

			result += A * sin (y);
		}
	}
	return (result);
}


/*----------------------------------------------------------------------------
 * The obliquity formula (and all the magic numbers below) come from Meeus,
 * Astro Algorithms.
 *
 * Input t is time in julian day.
 * Valid range is the years -8000 to +12000 (t = -100 to 100).
 *
 * return value is mean obliquity (epsilon sub 0) in degrees.
 */
double pget_mean_obliquity( double t )
{
	double u, u0;
	static double t0 = 30000.;
	static double rval = 0.;
	static const double rvalStart = 23. * 3600. + 26. * 60. + 21.448;
	static const int OBLIQ_COEFFS = 10;
	static const double coeffs[ 10 ] = {
         -468093.,  -155.,  199925.,  -5138.,  -24967.,
         -3905.,    712.,   2787.,    579.,    245.};
	int i;
	t = ( t - 2451545.0 ) / 36525.; // Convert time in centuries

	if( t0 != t )
	{
		t0 = t;
    	u = u0 = t / 100.;     // u is in julian 10000's of years
    	rval = rvalStart;
		for( i=0; i<OBLIQ_COEFFS; i++ )
      	{
        	rval += u * coeffs[i] / 100.;
        	u *= u0;
		}
      	// convert from seconds to degree
      	rval /= 3600.;
	}
	return rval;
}



/* Calculate the rectangular geocentric lunar coordinates to the inertial mean
 * ecliptic and equinox of J2000.
 * The geocentric coordinates returned are in units of UA.
 * The truncation level of the series in radians for longitude
 * and latitude and in km for distance. (Valid range 0 - 0.01, 0 being highest accuracy)
 * This function is based upon the Lunar Solution ELP2000-82B by
 * Michelle Chapront-Touze and Jean Chapront of the Bureau des Longitudes,
 * Paris. ELP 2000-82B theory
 * param JD Julian day, rect pos, precision */
void get_lunar_geo_posn_prec(double JD, double * X, double * Y, double * Z, double precision)
{
	double t[5];
	double elp[36];
	double a,b,c;
	double x,y,z;
	double pw,qw, pwqw, pw2, qw2, ra;

	double rotc, rots, obl;

	/* is precision too low ? */
	if (precision > 0.01)
		precision = 0.01;

	/* calc julian centuries */
	t[0] = 1.0;
	t[1] = (JD - 2451545.0) / 36525.0;
	t[2] = t[1] * t[1];
	t[3] = t[2] * t[1];
	t[4] = t[3] * t[1];
	
	/* calc precision */
	pre[0] = precision * RAD;
	pre[1] = precision * RAD;
	pre[2] = precision * ATH;
	
	/* sum elp series */
	elp[0] = sum_series_elp1(t);
	elp[1] = sum_series_elp2(t);
	elp[2] = sum_series_elp3(t);
	elp[3] = sum_series_elp4(t);
	elp[4] = sum_series_elp5(t);
	elp[5] = sum_series_elp6(t);
	elp[6] = sum_series_elp7(t);
	elp[7] = sum_series_elp8(t);
	elp[8] = sum_series_elp9(t);
	elp[9] = sum_series_elp10(t);
	elp[10] = sum_series_elp11(t);
	elp[11] = sum_series_elp12(t);
	elp[12] = sum_series_elp13(t);
	elp[13] = sum_series_elp14(t);
	elp[14] = sum_series_elp15(t);
	elp[15] = sum_series_elp16(t);
	elp[16] = sum_series_elp17(t);
	elp[17] = sum_series_elp18(t);
	elp[18] = sum_series_elp19(t);
	elp[19] = sum_series_elp20(t);
	elp[20] = sum_series_elp21(t);
	elp[21] = sum_series_elp22(t);
	elp[22] = sum_series_elp23(t);
	elp[23] = sum_series_elp24(t);
	elp[24] = sum_series_elp25(t);
	elp[25] = sum_series_elp26(t);
	elp[26] = sum_series_elp27(t);
	elp[27] = sum_series_elp28(t);
	elp[28] = sum_series_elp29(t);
	elp[29] = sum_series_elp30(t);
	elp[30] = sum_series_elp31(t);
	elp[31] = sum_series_elp32(t);
	elp[32] = sum_series_elp33(t);
	elp[33] = sum_series_elp34(t);
	elp[34] = sum_series_elp35(t);
	elp[35] = sum_series_elp36(t);
	
	a = elp[0] + elp[3] + elp[6] + elp[9] + elp[12] +
		elp[15] + elp[18] + elp[21] + elp[24] +
		elp[27] + elp[30] + elp[33];
	b = elp[1] + elp[4] + elp[7] + elp[10] + elp[13] +
		elp[16] + elp[19] + elp[22] + elp[25] +
		elp[28] + elp[31] + elp[34];
	c = elp[2] + elp[5] + elp[8] + elp[11] + elp[14] +
		elp[17] + elp[20] + elp[23] + elp[26] +
		elp[29] + elp[32] + elp[35];
		
	/* calculate geocentric coords */	
	a = a / RAD + W1[0] + W1[1] * t[1] + W1[2] * t[2] + W1[3] * t[3] + W1[4] * t[4];
	b = b / RAD;
	c = c * A0 / ATH;
	
	x = c * cos(b);
	y = x * sin(a);
	x = x * cos(a);
	z = c * sin(b);
	
	/* Laskars series */
	pw = (P1 + P2 * t[1] + P3 * t[2] + P4 * t[3] + P5 * t[4]) * t[1];
	qw = (Q1 + Q2 * t[1] + Q3 * t[2] + Q4 * t[3] + Q5 * t[4]) * t[1];
	ra = 2.0 * sqrt(1 - pw * pw - qw * qw);
	pwqw = 2.0 * pw * qw;
	pw2 = 1 - 2.0 * pw * pw;
	qw2 = 1 - 2.0 * qw * qw;
	pw = pw * ra;
	qw = qw * ra;
	a = pw2 * x + pwqw * y + pw * z;
	b = pwqw * x + qw2 * y - qw * z;
	c = -pw * x + qw * y + (pw2 + qw2 -1) * z;

	a /= AU;
	b /= AU;
	c /= AU;

    /* Convert in rectangular geocentric equatorial coordinate */
    obl = pget_mean_obliquity(JD)*M_PI/180.;
    rotc = cos(obl);
    rots = sin(obl);

    *X = a;
    *Y = rotc * b - rots * c;
    *Z = rots * b + rotc * c;
}

/* Same with lowest precision by default */
void get_lunar_geo_posn(double JD, double * X, double * Y, double * Z)
{
	get_lunar_geo_posn_prec(JD, X, Y, Z, 0.0001);
}

