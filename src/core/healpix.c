/*
 * Stellarium
 * Copyright (C) 2017 Guillaume Chereau
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

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

/* utab[m] = (short)(
      (m&0x1 )       | ((m&0x2 ) << 1) | ((m&0x4 ) << 2) | ((m&0x8 ) << 3)
    | ((m&0x10) << 4) | ((m&0x20) << 5) | ((m&0x40) << 6) | ((m&0x80) << 7)); */
static const short utab[]={
#define Z(a) 0x##a##0, 0x##a##1, 0x##a##4, 0x##a##5
#define Y(a) Z(a##0), Z(a##1), Z(a##4), Z(a##5)
#define X(a) Y(a##0), Y(a##1), Y(a##4), Y(a##5)
X(0),X(1),X(4),X(5)
#undef X
#undef Y
#undef Z
};

/* ctab[m] = (short)(
       (m&0x1 )       | ((m&0x2 ) << 7) | ((m&0x4 ) >> 1) | ((m&0x8 ) << 6)
    | ((m&0x10) >> 2) | ((m&0x20) << 5) | ((m&0x40) >> 3) | ((m&0x80) << 4)); */
static const short ctab[]={
#define Z(a) a,a+1,a+256,a+257
#define Y(a) Z(a),Z(a+2),Z(a+512),Z(a+514)
#define X(a) Y(a),Y(a+4),Y(a+1024),Y(a+1028)
X(0),X(8),X(2048),X(2056)
#undef X
#undef Y
#undef Z
};

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
    double sigma, xc;

    if (fabs(y) > M_PI / 4) {
        // Polar
        sigma = 2 - fabs(y * 4) / M_PI;
        *z = (y > 0 ? 1 : -1) * (1 - sigma * sigma / 3);
        xc = -M_PI + (2 * floor((x + M_PI) * 4 / (2 * M_PI)) + 1) * M_PI / 4;
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
