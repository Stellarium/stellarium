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
#ifndef _PRECESSION_H_
#define _PRECESSION_H_

#ifdef __cplusplus
extern "C" {
#endif

//! Precession modelled from:
//! J. Vondrák, N. Capitaine, and P. Wallace: New precession expressions, valid for long time intervals
//! A&A (Astronomy&Astrophysics) 534, A22 (2011)
//! DOI: http://dx.doi.org/10.1051/0004-6361/201117274
//! This paper describes a precession model valid for +/-200.000 years from J2000.0 and consistent with P03 precession accepted as IAU2006 Precession.
//! Some better understanding of the angles can be found in:
//! + 1994AJ____108__711W J.G.Williams: Contributions to the Earth's Obliquity Rate, Precession and Nutation (Angles of eq. (35))
//! + A&A 459, 981–985 (2006) DOI: 10.1051/0004-6361:20065897: Wallace&Capitaine: Precession-nutation procedures consistent with IAU 2006 resolutions
//!
//! The angles computed therein are used to rotate the planet Earth, and also to rotate an "Ecliptic of Date", i.e. the current orbital plane of Earth.
//! Currently this is without Nutation.
//! Return values are in radians
//! TODO: Find IAU2000 Nutation and how this fits in here.
void getPrecessionAnglesVondrak(const double jde, double *epsilon_A, double *chi_A, double *omega_A, double *psi_A);

//! It seems the easiest solution is in fact the one also implemented in the paper,
//! combining matrix P from P_A, Q_A, X_A, Y_A and, for the ecliptic of date, rotate back by epsilon_A.
//! Return values are in radians
void getPrecessionAnglesVondrakPQXYe(const double jde, double *vP_A, double *vQ_A, double *vX_A, double *vY_A, double *vepsilon_A);

//! Just return (presumably precomputed) ecliptic obliquity. [radians]
double getPrecessionAngleVondrakEpsilon(const double jde);

//! Just return (previously computed) ecliptic obliquity. [radians]
double getPrecessionAngleVondrakCurrentEpsilonA(void);

#ifdef __cplusplus
}
#endif

#endif
