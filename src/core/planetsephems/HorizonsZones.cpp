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

#include "HorizonsZones.hpp"

HorizonsZones::Zone HorizonsZones::zoneFor(double jdUT, double jdTT, double zone4Start)
{
	if (jdUT < JD_FIRST || jdUT > JD_LAST)
		return Zone::None;
	if (jdUT < JD_ZONE1_END)
		return Zone::Zone1;
	if (jdUT < JD_UTC_START)
		return Zone::Zone2;
	if (jdUT < zone4Start)
		return Zone::Zone3;
	// From here jdUT >= zone4Start. The end of zone 4 is tested on JD(TT), as in the standalone program.
	if (jdTT < JD_ZONE5_TT)
		return Zone::Zone4;
	return Zone::Zone5;
}

double HorizonsZones::utcDayStartJD(qint64 msSinceEpoch)
{
	const qint64 msPerDay = 86400000LL;
	// floor division, also right for instants before 1970
	qint64 days = msSinceEpoch / msPerDay;
	if (msSinceEpoch % msPerDay < 0)
		--days;
	return 2440587.5 + static_cast<double>(days);	// JD of 1970-01-01 0h UTC
}

QString HorizonsZones::zoneName(Zone z)
{
	switch (z)
	{
		case Zone::Zone1: return QStringLiteral("Zone 1 (to 1799, UT1)");
		case Zone::Zone2: return QStringLiteral("Zone 2 (1799 to 1962, UT1)");
		case Zone::Zone3: return QStringLiteral("Zone 3 (1962 to now + constant EOP delay, UTC)");
		case Zone::Zone4: return QStringLiteral("Zone 4 (constant EOP to 2202, UTC)");
		case Zone::Zone5: return QStringLiteral("Zone 5 (2202 to 9999, UTC)");
		default:          return QStringLiteral("outside of the range of JPL Horizons");
	}
}
