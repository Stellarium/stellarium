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
#include "StelCore.hpp"

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

// Exactly in the pole the result is still defined: the given longitude is at the bottom of the screen
// in the north pole and at the top in the south pole.
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

// The pole of a frame, converted to J2000 and back (as moveTo() does), must still count as the pole,
// and the longitude encoded in the up vector must survive. Supergalactic matrices leave errors of ~7e-9 rad.
void TestStelMovementMgr::testPoleAfterFrameRoundTrip()
{
	const Mat4d toJ2000[]   = { StelCore::matGalacticToJ2000, StelCore::matSupergalacticToJ2000 };
	const Mat4d fromJ2000[] = { StelCore::matJ2000ToGalactic, StelCore::matJ2000ToSupergalactic };
	for (int f = 0; f < 2; ++f)
	{
		for (int i = -11; i <= 12; ++i)
		{
			const double lon = i * 15. * M_PI_180;
			for (const double lat : { M_PI_2, -M_PI_2 })
			{
				Vec3d dir;
				StelUtils::spheToRect(lon, lat, dir);
				const Vec3d dirBack = fromJ2000[f] * (toJ2000[f] * dir);
				QVERIFY(StelMovementMgr::isInPole(dirBack));
				const Vec3d upBack = fromJ2000[f] * (toJ2000[f] * StelMovementMgr::upVectorFromLonLat(lon, lat));
				const double lonBack = StelMovementMgr::poleLongitudeFromUp(upBack, lat > 0., 99.);
				QVERIFY(qAbs(StelUtils::fmodpos(lonBack - lon + M_PI, 2.*M_PI) - M_PI) < 1e-6);
			}
		}
	}
}

// Away from the pole, the result only depends on the direction, not on a stale up vector.
void TestStelMovementMgr::testUprightUpVector()
{
	const Vec3d pole(0., 0., 1.);
	const Vec3d staleUp = StelMovementMgr::upVectorFromLonLat(2.5, 0.7); // belongs to another direction
	for (int i = 0; i < 24; ++i)
	{
		for (int j = -8; j <= 8; ++j)
		{
			Vec3d dir;
			StelUtils::spheToRect(i * 15. * M_PI_180, j * 10. * M_PI_180, dir);
			const Vec3d up = StelMovementMgr::uprightUpVector(dir, staleUp);
			QVERIFY(qAbs(up.norm() - 1.) < EPS);
			QVERIFY(qAbs(up.dot(dir)) < EPS);
			QVERIFY(qAbs((dir ^ pole).dot(up)) < EPS);
			QVERIFY(up.dot(pole) > -EPS);
		}
	}
}

// Exactly in the pole, the rotation given by the current up vector is kept.
void TestStelMovementMgr::testUprightUpVectorInPole()
{
	for (int i = -11; i <= 12; ++i)
	{
		const double lon = i * 15. * M_PI_180;
		for (const double lat : { M_PI_2, -M_PI_2 })
		{
			Vec3d dir;
			StelUtils::spheToRect(1.234, lat, dir); // longitude of the direction itself is meaningless here
			const Vec3d currentUp = StelMovementMgr::upVectorFromLonLat(lon, lat);
			const Vec3d up = StelMovementMgr::uprightUpVector(dir, currentUp);
			QVERIFY((up - currentUp).norm() < EPS);
		}
	}
}

// After switching the mount frame, the old up vector expressed in the new frame tilts the view.
// uprightUpVector() rebuilds an untilted one (this is what setMountMode() does).
void TestStelMovementMgr::testUprightUpVectorAfterMountChange()
{
	const Vec3d pole(0., 0., 1.);
	Vec3d dirEqu;
	StelUtils::spheToRect(1., 0.5, dirEqu);
	double lon, lat;
	StelUtils::rectToSphe(&lon, &lat, dirEqu);
	const Vec3d upEqu = StelMovementMgr::upVectorFromLonLat(lon, lat);

	const Vec3d dirGal = StelCore::matJ2000ToGalactic * dirEqu;
	const Vec3d staleUpGal = StelCore::matJ2000ToGalactic * upEqu;
	QVERIFY(qAbs((dirGal ^ pole).dot(staleUpGal)) > 0.1); // the stale up vector is clearly tilted

	const Vec3d upGal = StelMovementMgr::uprightUpVector(dirGal, staleUpGal);
	QVERIFY(qAbs(upGal.dot(dirGal)) < EPS);
	QVERIFY(qAbs((dirGal ^ pole).dot(upGal)) < EPS);
	QVERIFY(upGal.dot(pole) > -EPS);
}
