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
//! This paper describes a precession model valid for +/-200.000 years from J2000.0.
void getPrecessionAnglesVondrak(const double jd, double *chi_A, double *omega_A, double *psi_A);

#ifdef __cplusplus
}
#endif

#endif
