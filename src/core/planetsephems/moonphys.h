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

#ifndef MOONPHYS_H
#define MOONPHYS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compute angles related to Lunar position.
 * J. Meeus, Astronomical Algorithms (2nd. ed) 1998, chapter 47.
 * All resulting angles are in radians, r in km!
 */
void computeMoonAngles(const double JDE, double *Lp, double *D, double *M, double *Mp, double *E, double *F, double *Omega, double *lambda, double *beta, double *r, bool doLBR);

/*
 * Compute angles related to Lunar physical ephemeris.
 * J. Meeus, Astronomical Algorithms (2nd. ed) 1998, chapter 53.
 */

void computeLibrations(const double T, const double M, const double Mp, const double D, const double E, const double F, const double Omega,
		       const double lambda, const double dPsi, const double beta, const double alpha, const double epsilon,
		       double *W, double *lp, double *bp, double *lpp, double *bpp, double *PA);


#ifdef __cplusplus
}
#endif
#endif
