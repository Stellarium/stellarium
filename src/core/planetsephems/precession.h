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
//! DOI: https://doi.org/10.1051/0004-6361/201117274
//!    with correction from
//! J. Vondrak, N. Capitaine, P. Wallace
//! New precession expressions, valid for long time intervals (Corrigendum)
//! A&A 541, C1 (2012)
//! DOI: https://doi.org/10.1051/0004-6361/201117274e
//!
//! This paper describes a precession model valid for +/-200.000 years from J2000.0 and consistent with P03 precession accepted as IAU2006 Precession.
//! Some better understanding of the angles can be found in:
//! + 1994AJ____108__711W J.G.Williams: Contributions to the Earth's Obliquity Rate, Precession and Nutation (Angles of eq. (35))
//! + A&A 459, 981-985 (2006) DOI: 10.1051/0004-6361:20065897: Wallace&Capitaine: Precession-nutation procedures consistent with IAU 2006 resolutions
//!
//! The angles computed therein are used to rotate the planet Earth's axis, and also to rotate an "Ecliptic of Date", i.e. the current orbital plane of Earth.
//! Currently this is without Nutation.
//! Return values are in radians
void getPrecessionAnglesVondrak(const double jde, double *epsilon_A, double *chi_A, double *omega_A, double *psi_A);

//! Alternative solution, the one also implemented in the paper,
//! combining matrix P from P_A, Q_A, X_A, Y_A and, for the ecliptic of date, rotate back by epsilon_A.
//! Return values are in radians.
//! This solution is currently unused, it seems easier to use the Capitaine sequence above.
void getPrecessionAnglesVondrakPQXYe(const double jde, double *vP_A, double *vQ_A, double *vX_A, double *vY_A, double *vepsilon_A);

//! Return ecliptic obliquity. [radians]
double getPrecessionAngleVondrakEpsilon(const double jde);

//! Just return (previously computed) ecliptic obliquity. [radians]
double getPrecessionAngleVondrakCurrentEpsilonA(void);

// To complete the task of correct&accurate precession-nutation handling, we need fitting IAU-2000A or IAU-2000B Nutation.
// E.g. A&A 459, 981-985 (2006) P. T. Wallace and N. Capitaine: Precession-nutation procedures consistent with IAU 2006 resolutions. DOI: 10.1051/0004-6361:20065897
// IAU 2000A nutation has 1400 terms and goes into micro-arcseconds. All we ever aim for is sub-arcsecond, if at all, this is more than covered by IAU-2000B.

//! Compute and return nutation angles of the abridged IAU-2000B nutation.
//! @param JDE Julian day (TT)
//! @return deltaPsi, radians
//! @return deltaEps, radians
//! Ref: Dennis D. McCarthy and Brian J. Lizum: An Abridged Model of the Precession-Nutation of the Celestial Pole.
//! Celestial Mechanics and Dynamical Astronomy 85: 37-49, 2003.
//! This model provides accuracy better than 1 milli-arcsecond in the time 1995-2050.
//! TODO: find out drift rate behaviour e.g. in 17./18. century, maybe use nutation only e.g. 1610-2200?
void getNutationAngles(const double JDE, double *deltaPsi, double *deltaEpsilon);

#ifdef __cplusplus
}
#endif

#endif
