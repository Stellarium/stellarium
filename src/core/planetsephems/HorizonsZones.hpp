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

#ifndef HORIZONSZONES_HPP
#define HORIZONSZONES_HPP

#include <QString>
#include <QtGlobal>

//! (SS) 2026-10-09 The five TIME ZONES (epoch regimes) of the JPL Horizons precession-nutation path.
//!
//! JPL Horizons does not use one precession-nutation model for all dates. The model, the time argument given to it and the
//! offsets that have to be added to match its output depend on the date:
//!  - Zone 1: from 9999 BC to 1799-01-01 12h, dates in UT1.
//!  - Zone 2: from 1799-01-01 12h to 1962-01-20, dates in UT1.
//!  - Zone 3: from 1962-01-20 (start of UTC) to the start of the "constant EOP" regime, dates in UTC. The EOP are interpolated.
//!  - Zone 4: from the start of the constant EOP regime (about 80 days after today) to 2202-01-01 12h TT. The last EOP is kept.
//!  - Zone 5: from 2202-01-01 12h TT to 9999-12-30.
//! The names of the time scales (UT, UTC, UT1, TT) are those of the JPL Horizons documentation. In Stellarium, JD(UT or UTC) is
//! StelCore::getJD(), JD(TDB) is StelCore::getJDE() when the Delta-T algorithm is JPL Horizons, and JD(TT) is StelCore::getJDTT().
//!
//! This class does not use StelApp, StelCore or any other Stellarium singleton (unit testable). The word "zone" has nothing
//! to do with the time zones of the observer (StelCore::getUTCOffset()).
class HorizonsZones
{
public:
	enum class Zone { None = 0, Zone1, Zone2, Zone3, Zone4, Zone5 };

	static constexpr double JD_FIRST      = -1930633.5;  //!< start of zone 1, 9999 BC Mar 20 0h
	static constexpr double JD_ZONE1_END  =  2378132.0;  //!< 1799-01-01 12h UT1, start of zone 2
	static constexpr double JD_UTC_START  =  2437684.5;  //!< 1962-01-20 0h, start of zone 3 (as in the standalone program)
	//! 1962-01-19 12h: from there on StelUtils::getDeltaTByJPLHorizons() gives TDB-UTC (TDB-UT1 before), so JD(UT) is a JD(UTC).
	//! It was moved from 2437684.5 to be consistent with the JPL Horizons app, and is not the start of zone 3.
	static constexpr double JD_DELTAT_UTC_START = 2437684.0;
	static constexpr double JD_ZONE5_TT   =  2525324.0;  //!< 2202-01-01 12h TT, start of zone 5 (tested on JD(TT))
	static constexpr double JD_LAST       =  5373482.5;  //!< end of zone 5, 9999-12-30 0h
	static constexpr int    DEFAULT_ZONE4_DAYS = 86;     //!< days after 0h UTC of today at which zone 4 starts (the manual of Horizons says about 78)

	//! Zone of a date.
	//! @param jdUT JD(UT1) before 1962-01-20, JD(UTC) after (this is StelCore::getJD() with the JPL Horizons Delta-T)
	//! @param jdTT JD(TT), only used for the end of zone 4 and the start of zone 5
	//! @param zone4Start JD(UTC) where zone 3 ends and zone 4 starts
	//! @return Zone::None outside of the range of Horizons (before 9999 BC Mar 20 or after 9999-12-30)
	static Zone zoneFor(double jdUT, double jdTT, double zone4Start);

	//! JD of 0h UTC of the day that holds the given instant (Unix time in milliseconds). Always ends with .5.
	static double utcDayStartJD(qint64 msSinceEpoch);

	static int zoneNumber(Zone z) { return static_cast<int>(z); }
	static QString zoneName(Zone z);
};

#endif // HORIZONSZONES_HPP
