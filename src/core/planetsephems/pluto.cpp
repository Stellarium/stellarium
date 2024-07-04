/*
Copyright (C) 2001 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2003 Fabien Chereau
Copyright (C) 2024 Georg Zotti (C++ changes)

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

#include "pluto.hpp"
#include <array>
#include <cmath>
#include <execution>
#include <numeric>

#define PLUTO_COEFFS 43

/*
struct pluto_argument
{
    double J, S, P;
};

struct pluto_longitude
{
    double A,B;
};

struct pluto_latitude
{
    double A,B;
};

struct pluto_radius
{
    double A,B;
};
*/

// Combine all in one!
struct pluto_data
{
    double J, S, P, lngA, lngB, latA, latB, radA, radB;
};

static const std::array<struct pluto_data, PLUTO_COEFFS> data = {{
//   J  S   P     lngA,        lngB, latA, latB, radA, radB
    {0, 0,  1, -19799805, 19850055, -5452852, -14974862,  66865439, 68951812},
    {0, 0,  2,    897144, -4954829,  3527812,   1672790, -11827535,  -332538},
    {0, 0,  3,    611149,  1211027, -1050748,    327647,   1593179, -1438890},
    {0, 0,  4,   -341243,  -189585,   178690,   -292153,    -18444,   483220},
    {0, 0,  5,    129287,   -34992,    18650,    100340,    -65977,   -85431},
    {0, 0,  6,    -38164,    30893,   -30697,    -25823,     31174,    -6032},
    {0, 1, -1,     20442,    -9987,     4878,     11248,     -5794,    22161},
    {0, 1,  0,     -4063,    -5071,      226,       -64,      4601,     4032},
    {0, 1,  1,     -6016,    -3336,     2030,      -836,     -1729,      234},
    {0, 1,  2,     -3956,     3039,       69,      -604,      -415,      702},
    {0, 1,  3,      -667,     3572,     -247,      -567,       239,      723},
    {0, 2, -2,      1276,      501,      -57,         1,        67,      -67},
    {0, 2, -1,      1152,     -917,     -122,       175,      1034,     -451},
    {0, 2,  0,       630,    -1277,      -49,      -164,      -129,      504},
    {1, -1, 0,      2571,     -459,     -197,       199,       480,     -231},
    {1, -1, 1,       899,    -1449,      -25,       217,         2,     -441},
    {1, 0, -3,     -1016,     1043,      589,      -248,     -3359,      265},
    {1, 0, -2,     -2343,    -1012,     -269,       711,      7856,    -7832},
    {1, 0, -1,      7042,      788,      185,       193,        36,    45763},
    {1, 0,  0,      1199,     -338,      315,       807,      8663,     8547},
    {1, 0,  1,       418,      -67,     -130,       -43,      -809,     -769},
    {1, 0,  2,       120,     -274,        5,         3,       263,     -144},
    {1, 0,  3,       -60,     -159,        2,        17,      -126,       32},
    {1, 0,  4,       -82,      -29,        2,         5,       -35,      -16},
    {1, 1, -3,       -36,      -20,        2,         3,       -19,       -4},
    {1, 1, -2,       -40,        7,        3,         1,       -15,        8},
    {1, 1, -1,       -14,       22,        2,        -1,        -4,       12},
    {1, 1,  0,         4,       13,        1,        -1,         5,        6},
    {1, 1,  1,         5,        2,        0,        -1,         3,        1},
    {1, 1,  3,        -1,        0,        0,         0,         6,       -2},
    {2, 0, -6,         2,        0,        0,        -2,         2,        2},
    {2, 0, -5,        -4,        5,        2,         2,        -2,       -2},
    {2, 0, -4,         4,       -7,       -7,         0,        14,       13},
    {2, 0, -3,        14,       24,       10,        -8,       -63,       13},
    {2, 0, -2,       -49,      -34,       -3,        20,       136,     -236},
    {2, 0, -1,       163,      -48,        6,         5,       273,     1065},
    {2, 0,  0,         9,       24,       14,        17,       251,      149},
    {2, 0,  1,        -4,        1,       -2,         0,       -25,       -9},
    {2, 0,  2,        -3,        1,        0,         0,         9,       -2},
    {2, 0,  3,         1,        3,        0,         0,        -8,        7},
    {3, 0, -2,        -3,       -1,        0,         1,         2,      -10},
    {3, 0, -1,         5,       -3,        0,         0,        19,       35},
    {3, 0,  0,         0,        0,        1,         0,        10,        2}
}};


/* Transform spheric coordinate in rectangular */
void sphe_to_rect(const double lng, const double lat, const double r, double *x, double *y, double *z)
{
	const double cosLat = cos(lat);
	(*x) = r * cos(lng) * cosLat;
	(*y) = r * sin(lng) * cosLat;
	(*z) = r * sin(lat);
}

/*
 Meeus, Astron. Algorithms 2nd ed (1998). Chap 37. Equ 37.1
 params : Julian day, Longitude, Latitude, Radius

 Calculate Pluto heliocentric ecliptical coordinates for given julian day.
 This function is accurate to within 0.07" in longitude, 0.02" in latitude
 and 0.000006 AU in radius.
 Note: This function is not valid outside the period of 1885-2099.
 Longitude and Latitude are in radians, radius in AU.
*/
void get_pluto_helio_coords (double JD, double * X, double * Y, double * Z)
{
    /* get julian centuries since J2000 */
    const double t = (JD - 2451545) / 36525;

    /* calculate mean longitudes for jupiter, saturn and pluto */
    const double J =  34.35 + 3034.9057 * t;
    const double S =  50.08 + 1222.1138 * t;
    const double P = 238.96 +  144.9600 * t;

    /* calc periodic terms in table 37.A */
    //  double sum_longitude = 0, sum_latitude = 0, sum_radius = 0;
    //	for (int i=0; i < PLUTO_COEFFS; i++)
    //	{
    //		const double a = argument[i].J * J + argument[i].S * S + argument[i].P * P;
    //		const double sin_a = sin(PI/180.*a);
    //		const double cos_a = cos(PI/180.*a);
    //
    //		/* longitude */
    //		sum_longitude += longitude[i].A * sin_a + longitude[i].B * cos_a;
    //
    //		/* latitude */
    //		sum_latitude += latitude[i].A * sin_a + latitude[i].B * cos_a;
    //
    //		/* radius */
    //		sum_radius += radius[i].A * sin_a + radius[i].B * cos_a;
    //	}
    std::array<double, 3>sum_lon_at_rad =
            std::transform_reduce(std::execution::par,
                                  data.begin(), data.end(), std::array<double, 3>({0.0, 0.0, 0.0}),
                                  [](const std::array<double, 3>&sum, const std::array<double, 3>&addon){
        return std::array<double, 3>{sum[0] + addon[0], sum[1] + addon[1], sum[2] + addon[2]};},
    [=](const struct pluto_data &d){
        const double a = d.J * J + d.S * S + d.P * P;
        const double sin_a = sin(M_PI/180.*a);
        const double cos_a = cos(M_PI/180.*a);
        return std::array<double, 3>{
            d.lngA * sin_a + d.lngB * cos_a,
                    d.latA * sin_a + d.latB * cos_a,
                    d.radA * sin_a + d.radB * cos_a};
    });

    /* calc L, B, R */
    const double L = M_PI/180. * (238.958116 + 144.96 * t + sum_lon_at_rad[0] * 0.000001);
    const double B = M_PI/180. * (-3.908239 + sum_lon_at_rad[1] * 0.000001);
    const double R = (40.7241346 + sum_lon_at_rad[2] * 0.0000001);

    /* convert to rectangular coord */
    sphe_to_rect(L, B, R, X, Y, Z);
}
