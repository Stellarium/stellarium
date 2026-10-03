/*
Copyright (C) 2015 Georg Zotti
Copyright (C) 2026 Sylvain Simard

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
#ifndef PRECESSION_H
#define PRECESSION_H

#ifdef __cplusplus
extern "C" {
#endif

//=============== LONG-TERM PRECESSION VONDRAK (2011-2012)  ===============

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

//================= LONG-TERM PRECESSION OWEN (1990) ======================

//! Precession modelled from:
//! A Theory of the earth's precession relative to the invariable plane of the solar system
//! by: William Mann Owen Jr
//! PhD. dissertation
//! University of Florida
//! 1990
//!
//! This thesis describes a long-term precession theory of classical angles (chiA, OmegaA, PsiA, epsA) and new ones
//! (L, I and Delta) inferred at discrete time from a numerical integration over a time spanned of +/- 5000 centuries
//! from J2000. Chebyshev polynomials fitted to these values achieve standard deviation of the difference between the
//! tabular values and the polynomial approximations better than 0.013" for classical angles and an order of magnitude
//! better for new angles.
//!
//! While the thesis provides 125 intervals of 80 centuries each of Chebyshev coefficients for eight (8) precession
//! parameters, this implementation currently only uses 5 intervals centered on J2000 to cover a Time range of
//! +/- 200 centuries centered around J2000. Additional intervals could be added as required.
//!
//! This first method computes the classical precession angles chi_A, omega_A, psi_A and epsilon_A
//!
//! A Precession matrix can be built from them using standard rotation mattrices
//!
//!                              P = Rz(chi_A) x Rx(-omega_A) x Rz(-psi_A) x Rx(eps0)
//!
//! This is the same standard form from Capitaine et al, expressed in the order of row-major matrices
//!
//! Return values are in radians
void getPrecessionAnglesOwenClassic(const double jde, double *epsilon_A, double *chi_A, double *omega_A, double *psi_A);

//! This second method computes the new precession angles L, I and Delta valid when refered to the
//! invariable plane of the solar system.
//!
//! A Precession matrix can be built from them using standard rotation mattrices
//!
//!                         P = Rz(-L) x Rx(-I) x Rz(Delta) x Rx(I0) x Rz(L0)
//!
//! This is the form used by Owen(1990), expressed in the order of row-major matrices
//!
//! Return values are in radians
void getPrecessionAnglesOwenLIDelta(const double jde, double *L, double *I, double *Delta);

//! Just return the ecliptic obliquity from the Owen long term theory
double getPrecessionAngleOwenEpsilon(const double jde);

//! Just return the invariable plane angles L0 at J2000
double getPrecessionAnglesOwenL0(void);

//! Just return the invariable plane angles I0 at J2000
double getPrecessionAnglesOwenI0(void);

//========================= NUTATION IAU-2000B ============================

// To complete the task of correct&accurate precession-nutation handling, we need fitting IAU-2000A or IAU-2000B Nutation.
// E.g. A&A 459, 981-985 (2006) P. T. Wallace and N. Capitaine: Precession-nutation procedures consistent with IAU 2006 resolutions. DOI: 10.1051/0004-6361:20065897
// IAU 2000A nutation has 1400 terms and goes into micro-arcseconds. All we ever aim for is sub-arcsecond, if at all, this is more than covered by IAU-2000B.

//! Compute and return nutation angles of the abridged IAU-2000B nutation.
//! @param JDE Julian day (TT)
//! @return deltaPsi, radians
//! @return deltaEps, radians
//! Ref: Dennis D. McCarthy and Brian J. Luzum: An Abridged Model of the Precession-Nutation of the Celestial Pole.
//! Celestial Mechanics and Dynamical Astronomy 85: 37-49, 2003.
//! This model provides accuracy better than 1 milli-arcsecond in the time 1995-2050.
//! TODO: find out drift rate behaviour e.g. in 17./18. century, maybe use nutation only e.g. 1610-2200?
void getNutationAngles(const double JDE, double *deltaPsi, double *deltaEpsilon);

#ifdef __cplusplus
}
#endif

#endif
