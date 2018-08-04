/*
 * Stellarium
 * Copyright (C) 2018 Fabien Chereau
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

/*
 * L1.2 theory of the galilan satellites,
 * by Valery LAINEY, Alain VIENNE and Luc DURIEZ.
 * ftp://ftp.imcce.fr/pub/ephem/satel/galilean/L1/L1.2/
 *
 * This is a port of the original FORTRAN code to C.
 */

#ifndef L12_H
#define L12_H

#ifdef __cplusplus
extern "C" {
#endif

#define L12_IO            0
#define L12_EUROPA        1
#define L12_GANYMEDE      2
#define L12_CALLISTO      3

/*
 * Galilean satellites positions using l1.2 semi-analytic theory by
 * by Valery LAINEY, Alain VIENNE and Luc DURIEZ.
 * ftp://ftp.imcce.fr/pub/ephem/satel/galilean/L1/L1.2/
 *
 * @param jd Input time in TT JD.
 * @param body Moon index from 0 (Io) to 3 (Callisto).
 * @param p Get the position of the body in Cartesian coordinates with origin the center of Jupiter
 *          and in the VSOP87 plan, in AU.
 * @param v Get the speed of the body in Cartesian coordinates with origin the center of Jupiter
 *          and in the VSOP87 plan. in AU/d.
 */
void GetL12Coor(double jd, int body, double p[3], double v[3]);

#ifdef __cplusplus
}
#endif

#endif
