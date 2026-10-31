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

#include "tests/testTimescales.hpp"
#include "HorizonsZones.hpp"

#include <QDateTime>

QTEST_GUILESS_MAIN(TestTimescales)

typedef HorizonsZones HZ;

namespace
{
// A zone 4 start well inside zone 3/4 of today, as StelCore would give it (0h UTC 2026-10-09 + 86 days)
const double ZONE4_START = 2461322.5 + HorizonsZones::DEFAULT_ZONE4_DAYS;

// TT is UT + about 69 s: irrelevant but for the dates tested against JD(TT)
HZ::Zone zone(double jdUT, double zone4Start = ZONE4_START)
{
	return HZ::zoneFor(jdUT, jdUT + 69.2 / 86400., zone4Start);
}
}

void TestTimescales::testZoneBoundaries()
{
	const double eps = 1e-6;	// 0.1 s
	// first date of zone 1 is included, and the dates around each limit
	QCOMPARE(zone(HZ::JD_FIRST), HZ::Zone::Zone1);
	QCOMPARE(zone(HZ::JD_ZONE1_END - eps), HZ::Zone::Zone1);
	QCOMPARE(zone(HZ::JD_ZONE1_END), HZ::Zone::Zone2);
	QCOMPARE(zone(HZ::JD_UTC_START - eps), HZ::Zone::Zone2);
	QCOMPARE(zone(HZ::JD_UTC_START), HZ::Zone::Zone3);
	QCOMPARE(zone(ZONE4_START - eps), HZ::Zone::Zone3);
	QCOMPARE(zone(ZONE4_START), HZ::Zone::Zone4);
	// a date in each zone
	QCOMPARE(zone(-1000000.), HZ::Zone::Zone1);
	QCOMPARE(zone(2300000.), HZ::Zone::Zone1);
	QCOMPARE(zone(2400000.), HZ::Zone::Zone2);
	QCOMPARE(zone(2451545.), HZ::Zone::Zone3);	// J2000
	QCOMPARE(zone(2461322.5), HZ::Zone::Zone3);	// 2026-10-09
	QCOMPARE(zone(2500000.), HZ::Zone::Zone4);
	QCOMPARE(zone(3000000.), HZ::Zone::Zone5);
	QCOMPARE(HZ::zoneNumber(HZ::Zone::Zone3), 3);
}

void TestTimescales::testZoneOutOfRange()
{
	QCOMPARE(zone(HZ::JD_FIRST - 1e-6), HZ::Zone::None);
	QCOMPARE(zone(-3000000.), HZ::Zone::None);
	QCOMPARE(zone(HZ::JD_LAST), HZ::Zone::Zone5);	// last date is included
	QCOMPARE(zone(HZ::JD_LAST + 1e-6), HZ::Zone::None);
	QCOMPARE(zone(6000000.), HZ::Zone::None);
}

void TestTimescales::testZone4Zone5UseTT()
{
	// The limit between zones 4 and 5 is tested on JD(TT): a date whose JD(UT) is just before it but whose JD(TT) is after is zone 5
	const double jdTTLimit = HZ::JD_ZONE5_TT;
	const double dt = 69.2 / 86400.;
	QCOMPARE(HZ::zoneFor(jdTTLimit - dt - 1e-6, jdTTLimit - 1e-6, ZONE4_START), HZ::Zone::Zone4);
	QCOMPARE(HZ::zoneFor(jdTTLimit - dt, jdTTLimit, ZONE4_START), HZ::Zone::Zone5);
	QCOMPARE(HZ::zoneFor(jdTTLimit - dt - 1e-6, jdTTLimit + 1e-6, ZONE4_START), HZ::Zone::Zone5);
}

void TestTimescales::testZone4StartMoves()
{
	// the start of zone 4 is a parameter: the same date changes zone with it
	const double jd = 2461322.5 + 80.;
	QCOMPARE(zone(jd, 2461322.5 + 78.), HZ::Zone::Zone4);
	QCOMPARE(zone(jd, 2461322.5 + 86.), HZ::Zone::Zone3);
}

void TestTimescales::testUtcDayStart()
{
	// 2026-10-09 0h UTC = 1791504000000 ms
	const qint64 day = 1791504000000LL;
	QCOMPARE(HZ::utcDayStartJD(day), 2461322.5);
	QCOMPARE(HZ::utcDayStartJD(day + 12LL * 3600000LL), 2461322.5);
	QCOMPARE(HZ::utcDayStartJD(day + 86400000LL - 1), 2461322.5);
	QCOMPARE(HZ::utcDayStartJD(day + 86400000LL), 2461323.5);
	QCOMPARE(HZ::utcDayStartJD(0), 2440587.5);		// 1970-01-01
	QCOMPARE(HZ::utcDayStartJD(-1), 2440586.5);		// 1969-12-31 23:59:59.999
	QCOMPARE(HZ::utcDayStartJD(-86400000LL), 2440586.5);
	QCOMPARE(HZ::utcDayStartJD(-86400001LL), 2440585.5);
	// agrees with Qt for the present day
	const QDateTime now = QDateTime::currentDateTimeUtc();
	const double jd = HZ::utcDayStartJD(now.toMSecsSinceEpoch());
	QCOMPARE(jd, double(now.date().toJulianDay()) - 0.5);
}
