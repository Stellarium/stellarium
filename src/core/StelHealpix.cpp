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

// Position for poles
double healpix_sigma(double z)
{
    if (z < 0)
    {
        return -healpix_sigma(-z);
    }
    else
    {
        return 2 - sqrt(3 * (1 - z));
    }
}

double healpix_clip(double Z, double A, double B)
{
    if (Z < A)
    {
        return A;
    }
    else if (Z > B)
    {
        return B;
    }
    else
    {
        return Z;
    }
}

double healpix_wrap(double A, double B)
{
    if (A < 0)
    {
        return B - fmod(-A, B);
    }
    else
    {
        return fmod(A, B);
    }
}

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
	xc = -M_PI + (2 * std::floor((x + M_PI) * 4 / (2 * M_PI)) + 1) * M_PI / 4;
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

// HEALPix spherical projection
void za2tu(double z, double a, double *t, double *u)
{
    if (fabs(z) <= 2. / 3.)
    // equator
    {
        *t = a;
        *u = 3 * (M_PI / 8.) * z;
    }
    else
    // north/south poles
    {
        double sigma_z;
        sigma_z = healpix_sigma(z);
        *t = a - (fabs(sigma_z) - 1) * (fmod(a, M_PI / 2.) - (M_PI / 4.));
        *u = (M_PI / 4.) * sigma_z;
    }
}

// spherical projection to base pixel index
// f: base pixel index
// p: coord in north east axis of base pixel
// q: coord in north west axis of base pixel
void tu2fpq(double t, double u, long long int *f, double *p, double *q)
{
    double pp;
    int PP;
    double qq;
    int QQ;
    int V;
    t /= (M_PI / 4.);
    u /= (M_PI / 4.);
    t = healpix_wrap(t, 8.);
    t += -4.;
    u += 5.;
    pp = healpix_clip((u + t) / 2., 0., 5.);
    PP = floor(pp);
    qq = healpix_clip((u - t) / 2., 3. - PP, 6. - PP);
    QQ = floor(qq);
    V = 5 - (PP + QQ);
    if (V < 0)
    { // clip
        *f = 0;
        *p = 1.;
        *q = 1.;
    }
    else
    {
        *f = 4 * V + fmod(((PP - QQ + 4) >> 1), 4.);
        *p = fmod(pp, 1);
        *q = fmod(qq, 1);
    }
}

void tu2fxy(int nside, double t, double u, long long int *f, long long int *x, long long int *y)
{
    double p;
    double q;
    tu2fpq(t, u, f, &p, &q);
    *x = healpix_clip(floor(nside * p), 0, nside - 1);
    *y = healpix_clip(floor(nside * q), 0, nside - 1);
}

long long int za2pix_nest(int nside, double z, double a)
{
    double t;
    double u;
    long long int f;
    long long int x;
    long long int y;
    za2tu(z, a, &t, &u);
    tu2fxy(nside, t, u, &f, &x, &y);
    return healpix_xyf2nest(nside, x, y, f);
}

long long int healpix_ang2pix_nest(int nside, double theta, double phi)
{
    double z;
    // normalize coords
    z = cos(theta);

    return za2pix_nest(nside, z, phi);
}
