/*
 * Stellarium
 * Copyright (C) 2017 Guillaume Chereau
 * Copyright (C) 2024 Henry Leung
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelHealpix.hpp"
#include <cmath>

// Position of the healpix faces.
static const int FACES[12][2] = {{1,  0}, {3,  0}, {5,  0}, {7,  0},
				 {0, -1}, {2, -1}, {4, -1}, {6, -1},
				 {1, -2}, {3, -2}, {5, -2}, {7, -2}};

int healpix_xyf2nest(int nside, int ix, int iy, int face_num)
{
  return (face_num*nside*nside) +
      (utab[ix&0xff] | (utab[ix>>8]<<16)
    | (utab[iy&0xff]<<1) | (utab[iy>>8]<<17));
}

static void nest2xyf(int nside, int pix, int *ix, int *iy, int *face_num)
{
    int npface_ = nside * nside, raw;
    *face_num = pix / npface_;
    pix &= (npface_ - 1);
    raw = (pix & 0x5555) | ((pix & 0x55550000) >> 15);
    *ix = ctab[raw & 0xff] | (ctab[raw >> 8] << 4);
    pix >>= 1;
    raw = (pix & 0x5555) | ((pix & 0x55550000) >> 15);
    *iy = ctab[raw & 0xff] | (ctab[raw >> 8] << 4);
}

// Create a 3x3 mat that transforms uv texture position to healpix xy
// coordinates.
void healpix_get_mat3(int nside, int pix, double out[3][3])
{
    int ix, iy, face;
    nest2xyf(nside, pix, &ix, &iy, &face);
    out[0][0] = +M_PI / 4 / nside;
    out[0][1] = +M_PI / 4 / nside;
    out[0][2] = 0;
    out[1][0] = -M_PI / 4 / nside;
    out[1][1] = +M_PI / 4 / nside;
    out[1][2] = 0;
    out[2][0] = (FACES[face][0] + (ix - iy + 0.0) / nside) * M_PI / 4;
    out[2][1] = (FACES[face][1] + (ix + iy + 0.0) / nside) * M_PI / 4;
    out[2][2] = 1;
}

static void healpix_xy2_z_phi(const double xy[2], double *z, double *phi)
{
    double x = xy[0], y = xy[1];

    if (fabs(y) > M_PI / 4) {
	// Polar
	double sigma = 2 - fabs(y * 4) / M_PI;
	*z = (y > 0 ? 1 : -1) * (1 - sigma * sigma / 3);
	double xc = -M_PI + (2 * std::floor((x + M_PI) * 4 / (2 * M_PI)) + 1) * M_PI / 4;
	*phi = sigma ? (xc + (x - xc) / sigma) : x;
    } else {
	// Equatorial
	*phi = x;
	*z = y * 8 / (M_PI * 3);
    }
}

void healpix_xy2ang(const double xy[2], double *theta, double *phi)
{
    double z;
    healpix_xy2_z_phi(xy, &z, phi);
    *theta = acos(z);
}

void healpix_xy2vec(const double xy[2], double out[3])
{
    double z, phi, stheta;
    healpix_xy2_z_phi(xy, &z, &phi);
    stheta = sqrt((1 - z) * (1 + z));
    out[0] = stheta * cos(phi);
    out[1] = stheta * sin(phi);
    out[2] = z;
}

void healpix_pix2vec(int nside, int pix, double out[3])
{
    int ix, iy, face;
    double xy[2];
    nest2xyf(nside, pix, &ix, &iy, &face);
    xy[0] = (FACES[face][0] + (ix - iy + 0.0) / nside) * M_PI / 4;
    xy[1] = (FACES[face][1] + (ix + iy + 1.0) / nside) * M_PI / 4;
    healpix_xy2vec(xy, out);
}

void healpix_pix2ang(int nside, int pix, double *theta, double *phi)
{
    int ix, iy, face;
    double xy[2];
    nest2xyf(nside, pix, &ix, &iy, &face);
    xy[0] = (FACES[face][0] + (ix - iy + 0.0) / nside) * M_PI / 4;
    xy[1] = (FACES[face][1] + (ix + iy + 1.0) / nside) * M_PI / 4;
    healpix_xy2ang(xy, theta, phi);
}

long long int healpix_ang2pix_nest(int nside, double theta, double phi)
{
	const double z = cos(theta);
	const double za = fabs(z);
	double tt = phi * 2.0 / M_PI;
	tt -= std::floor(tt / 4.0) * 4.0; // normalize to [0, 4)

	int ix, iy, face_num;

	if (za <= 2.0 / 3.0)
	{
		// Equatorial region
		const double temp1 = nside * (0.5 + tt);
		const double temp2 = nside * (z * 0.75);
		const int jp = static_cast<int>(std::floor(temp1 - temp2));
		const int jm = static_cast<int>(std::floor(temp1 + temp2));
		const int ifp = jp / nside;
		const int ifm = jm / nside;

		if (ifp == ifm)
			face_num = ifp | 4;
		else if (ifp < ifm)
			face_num = ifp;
		else
			face_num = ifm + 8;

		ix = jm & (nside - 1);
		iy = nside - (jp & (nside - 1)) - 1;
	}
	else
	{
		// Polar region
		const int ntt = static_cast<int>(tt);
		const double tp = tt - ntt;
		const double tmp = nside * sqrt(3.0 * (1.0 - za));
		int jp = static_cast<int>(std::floor(tp * tmp));
		int jm = static_cast<int>(std::floor((1.0 - tp) * tmp));

		if (jp > nside - 1) jp = nside - 1;
		if (jm > nside - 1) jm = nside - 1;

		if (z >= 0.0)
		{
			ix = nside - jm - 1;
			iy = nside - jp - 1;
			face_num = ntt;
		}
		else
		{
			ix = jp;
			iy = jm;
			face_num = ntt + 8;
		}
	}

	return static_cast<long long int>(healpix_xyf2nest(nside, ix, iy, face_num));
}
