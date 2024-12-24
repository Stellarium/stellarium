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

/* utab[m] = (short)(
      (m&0x1 )       | ((m&0x2 ) << 1) | ((m&0x4 ) << 2) | ((m&0x8 ) << 3)
    | ((m&0x10) << 4) | ((m&0x20) << 5) | ((m&0x40) << 6) | ((m&0x80) << 7)); */
static const short utab[] = {
    #define Z(a) 0x##a##0, 0x##a##1, 0x##a##4, 0x##a##5
    #define Y(a) Z(a##0), Z(a##1), Z(a##4), Z(a##5)
    #define X(a) Y(a##0), Y(a##1), Y(a##4), Y(a##5)
    X(0),
    X(1),
    X(4),
    X(5)
    #undef X
    #undef Y
    #undef Z
};

/* ctab[m] = (short)(
       (m&0x1 )       | ((m&0x2 ) << 7) | ((m&0x4 ) >> 1) | ((m&0x8 ) << 6)
    | ((m&0x10) >> 2) | ((m&0x20) << 5) | ((m&0x40) >> 3) | ((m&0x80) << 4)); */
static const short ctab[] = {
    #define Z(a) a, a + 1, a + 256, a + 257
    #define Y(a) Z(a), Z(a + 2), Z(a + 512), Z(a + 514)
    #define X(a) Y(a), Y(a + 4), Y(a + 1024), Y(a + 1028)
    X(0),
    X(8),
    X(2048),
    X(2056)
    #undef X
    #undef Y
    #undef Z
};

void healpix_pix2vec(int nside, int pix, double out[3]);
void healpix_get_mat3(int nside, int pix, double out[3][3]);
void healpix_xy2vec(const double xy[2], double out[3]);
