/*
 * Stellarium
 * Copyright (C) 2026 Aaditya Sah
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

#include "tests/testStelMovementMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelUtils.hpp"

QTEST_GUILESS_MAIN(TestStelMovementMgr)

static const double EPS = 1e-12;

// For all directions: unit length, perpendicular to the view direction,
// not tilted (no component out of the plane of view direction and pole),
// and pointing to the side of the pole.
void TestStelMovementMgr::testUpVectorProperties()
{
	const Vec3d pole(0., 0., 1.);
	for (int i = 0; i < 24; ++i)
	{
		for (int j = -9; j <= 9; ++j)
		{
			const double lon = i * 15. * M_PI_180;
			const double lat = j * 10. * M_PI_180;
			Vec3d dir;
			StelUtils::spheToRect(lon, lat, dir);
			const Vec3d up = StelMovementMgr::upVectorFromLonLat(lon, lat);

			QVERIFY(qAbs(up.norm() - 1.) < EPS);
			QVERIFY(qAbs(up.dot(dir)) < EPS);
			QVERIFY(qAbs((dir ^ pole).dot(up)) < EPS);
			QVERIFY(up.dot(pole) > -EPS);
		}
	}
}

// At the horizon, up is straight up to the pole.
void TestStelMovementMgr::testUpVectorAtHorizon()
{
	for (int i = 0; i < 24; ++i)
	{
		const Vec3d up = StelMovementMgr::upVectorFromLonLat(i * 15. * M_PI_180, 0.);
		QVERIFY((up - Vec3d(0., 0., 1.)).norm() < EPS);
	}
}

// Exactly in the pole the result is still defined: the bottom of the screen points to the given longitude.
void TestStelMovementMgr::testUpVectorAtPoles()
{
	for (int i = 0; i < 24; ++i)
	{
		const double lon = i * 15. * M_PI_180;
		const Vec3d upZenith = StelMovementMgr::upVectorFromLonLat(lon, M_PI_2);
		QVERIFY((upZenith - Vec3d(-cos(lon), -sin(lon), 0.)).norm() < EPS);
		const Vec3d upNadir = StelMovementMgr::upVectorFromLonLat(lon, -M_PI_2);
		QVERIFY((upNadir - Vec3d(cos(lon), sin(lon), 0.)).norm() < EPS);
	}
}

// Approaching the pole does not change the up vector suddenly (no jump at the pole).
void TestStelMovementMgr::testUpVectorContinuousNearPole()
{
	for (int i = 0; i < 24; ++i)
	{
		const double lon = i * 15. * M_PI_180;
		const Vec3d upAtPole = StelMovementMgr::upVectorFromLonLat(lon, M_PI_2);
		const Vec3d upNearPole = StelMovementMgr::upVectorFromLonLat(lon, M_PI_2 - 1e-6);
		QVERIFY((upAtPole - upNearPole).norm() < 1e-5);
	}
}
// Only directions exactly into the pole (up to rounding) have no longitude. Length does not matter.
void TestStelMovementMgr::testIsInPole()
{
	Vec3d dir;
	StelUtils::spheToRect(1.234, M_PI_2, dir);
	QVERIFY(StelMovementMgr::isInPole(dir));
	QVERIFY(StelMovementMgr::isInPole(dir*2.));
	StelUtils::spheToRect(1.234, -M_PI_2, dir);
	QVERIFY(StelMovementMgr::isInPole(dir));
	StelUtils::spheToRect(1.234, M_PI_2 - 1e-6, dir);
	QVERIFY(!StelMovementMgr::isInPole(dir));
	StelUtils::spheToRect(1.234, 0., dir);
	QVERIFY(!StelMovementMgr::isInPole(dir));
}

// In the pole, the longitude decoded from the up vector is the one used to build it.
// Without a usable up vector the fallback is returned.
void TestStelMovementMgr::testPoleLongitudeFromUp()
{
	for (int i = -11; i <= 12; ++i)
	{
		const double lon = i * 15. * M_PI_180; // in (-180°, 180°], the range returned by atan2
		const double fromZenith = StelMovementMgr::poleLongitudeFromUp(StelMovementMgr::upVectorFromLonLat(lon, M_PI_2), true, 99.);
		QVERIFY(qAbs(fromZenith - lon) < EPS);
		const double fromNadir = StelMovementMgr::poleLongitudeFromUp(StelMovementMgr::upVectorFromLonLat(lon, -M_PI_2), false, 99.);
		QVERIFY(qAbs(fromNadir - lon) < EPS);
	}
	QCOMPARE(StelMovementMgr::poleLongitudeFromUp(Vec3d(0., 0., 1.), true, 0.5), 0.5);
	QCOMPARE(StelMovementMgr::poleLongitudeFromUp(Vec3d(0., 0., 0.), true, 0.5), 0.5);
}
