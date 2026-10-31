/*
 * Stellarium
 * Copyright (C) 2026 Sylvain Simard
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

#ifndef HORIZONSPRECNUT_HPP
#define HORIZONSPRECNUT_HPP

#include "HorizonsZones.hpp"
#include "VecMath.hpp"

//! (SS) 2026-10-09 Precession and nutation of the JPL Horizons path: the matrix P3 that takes a J2000 equatorial vector (p2)
//! to the true equator and equinox of date (p3), and the true obliquity for the ecliptic of date.
//!
//! The models depend on the TIME ZONE of the date (see HorizonsZones):
//!  - zones 1 and 5: Owen (1990) classic angles (epsA, chiA, omegaA, psiA), IAU 1980 nutation, obliquity of J2000 84381.448"
//!  - zones 2, 3 and 4: IAU 1976 precession, IAU 1980 nutation and mean obliquity
//! Zone 1 and 2 use JD(TDB), zones 3, 4 and 5 use JD(TT). The IAU 1980 nutation is corrected by the EOP (dPsi, dEps) in zones 3
//! (interpolated), 4 and 5 (last value) and by the empirical offsets in dPsi that match JPL Horizons (see dpsiOffsetMas()).
//!
//! Conventions. The matrices computed here are ERFA matrices: row-major r[3][3], used as p3 = r * p2 on column vectors, and the
//! rotations are those of ERFA (eraRx(a) rotates the frame by a). Stellarium's Mat4d is a column-major 4x4 that is also
//! multiplied on column vectors, so the same mathematical matrix is obtained by copying the elements to the right places
//! (toMat4d()). Do NOT build these matrices with Mat4d::xrotation()/zrotation(): they have the opposite sign of eraRx()/eraRz().
//!
//! No cache and no static variable are used (the cached functions of precession.c have a threshold of one day, about 138 mas).
//! This class does not use StelApp, StelCore or any other Stellarium singleton (unit testable).
class HorizonsPrecNut
{
public:
	//! Mean obliquity of J2000 (Lieske et al. 1977, also Owen 1990) in arcseconds, used by Horizons for the ecliptic of J2000
	static constexpr double EPS0_ARCSEC = 84381.448;
	static constexpr double ARCSEC2RAD  = 4.848136811095359935899141e-6;
	static constexpr double EPS0_RAD    = EPS0_ARCSEC * ARCSEC2RAD;

	struct Params
	{
		double jdUT = 0.;        //!< JD(UT1) before 1962-01-20, JD(UTC) after (StelCore::getJD() with the JPL Horizons Delta-T)
		double jdTDB = 0.;       //!< JD(TDB), time argument of zones 1 and 2
		double jdTT = 0.;        //!< JD(TT), time argument of zones 3, 4 and 5
		double zone4Start = 0.;  //!< JD(UTC) where zone 3 ends (StelCore::getHorizonsZone4Start())
		bool useNutation = true; //!< false: precession only (mean equator and equinox of date), no EOP, no offset
		double eopDpsi = 0.;     //!< [arcsec] zone 3: dPsi interpolated at jdUT. Zones 4 and 5: last dPsi. Not used in zones 1 and 2.
		double eopDeps = 0.;     //!< [arcsec] same for dEps
		bool forMoon = false;    //!< the offsets in dPsi of zones 4 and 5 are not the same for the Moon
	};

	struct Result
	{
		bool ok = false;                              //!< false when the date is outside of the range of Horizons
		HorizonsZones::Zone zone = HorizonsZones::Zone::None;
		double r[3][3] = {{1,0,0},{0,1,0},{0,0,1}};  //!< P3, J2000 equatorial -> true equator and equinox of date (ERFA convention)
		double epsMean = 0.;                          //!< mean obliquity of date [rad], Owen (zones 1, 5) or IAU 1980
		double dpsi = 0., deps = 0.;                  //!< nutation including EOP and offset [rad]
		double epsTrue = 0.;                          //!< epsMean + deps [rad]: obliquity for the apparent ecliptic of date
	};

	static Result compute(const Params& p);

	//! Offset added to dPsi (milliarcseconds) in a zone, to match JPL Horizons. Depends on the object in zones 4 and 5.
	static double dpsiOffsetMas(HorizonsZones::Zone z, bool forMoon);

	//! Copy an ERFA matrix to a Mat4d (same mathematical matrix, column-major storage)
	static Mat4d toMat4d(const double r[3][3])
	{
		return Mat4d(r[0][0], r[1][0], r[2][0], 0.,
		             r[0][1], r[1][1], r[2][1], 0.,
		             r[0][2], r[1][2], r[2][2], 0.,
		             0., 0., 0., 1.);
	}
	//! Copy the transpose of an ERFA matrix to a Mat4d
	static Mat4d toMat4dTransposed(const double r[3][3])
	{
		return Mat4d(r[0][0], r[0][1], r[0][2], 0.,
		             r[1][0], r[1][1], r[1][2], 0.,
		             r[2][0], r[2][1], r[2][2], 0.,
		             0., 0., 0., 1.);
	}
};

#endif // HORIZONSPRECNUT_HPP
