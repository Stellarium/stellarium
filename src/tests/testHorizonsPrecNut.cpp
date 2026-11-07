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

#include "tests/testHorizonsPrecNut.hpp"
#include "HorizonsPrecNut.hpp"

#include <erfa.h>
#include <QElapsedTimer>
#include <cmath>

QTEST_GUILESS_MAIN(TestHorizonsPrecNut)

typedef HorizonsPrecNut HPN;
typedef HorizonsZones::Zone Zone;

namespace
{
const double J2000 = 2451545.0;
const double TODAY = 2461322.5;		// 2026-10-09 0h UTC
const double ZONE4_START = TODAY + HorizonsZones::DEFAULT_ZONE4_DAYS;
const double TT_MINUS_UT = 69.2 / 86400.;

// one date per zone (JD(UT)), see HorizonsZones
const double DATES[5] = { 2300000., 2400000., 2451545., 2500000., 3000000. };
const Zone ZONES[5] = { Zone::Zone1, Zone::Zone2, Zone::Zone3, Zone::Zone4, Zone::Zone5 };

HPN::Params params(double jdUT)
{
	HPN::Params p;
	p.jdUT = jdUT;
	p.jdTT = p.jdTDB = jdUT + TT_MINUS_UT;
	p.zone4Start = ZONE4_START;
	return p;
}

double frobenius(const double a[3][3], const double b[3][3])
{
	double s = 0.;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			s += (a[i][j] - b[i][j]) * (a[i][j] - b[i][j]);
	return std::sqrt(s);
}

bool close3(const double a[3], const Vec3d& b, double tol)
{
	return std::fabs(a[0] - b[0]) < tol && std::fabs(a[1] - b[1]) < tol && std::fabs(a[2] - b[2]) < tol;
}
}

void TestHorizonsPrecNut::testMat4dConversion()
{
	// An ERFA matrix and the Mat4d from toMat4d() must give the same vector. This checks the row/column mapping.
	double r[3][3];
	eraIr(r);
	eraRz(0.3, r);
	eraRx(-0.7, r);
	double p[3] = { 1., 2., 3. };
	double q[3];
	eraRxp(r, p, q);
	const Vec3d v = HPN::toMat4d(r) * Vec3d(p[0], p[1], p[2]);
	QVERIFY(close3(q, v, 1e-14));
	// the transposed matrix is the inverse
	const Vec3d w = HPN::toMat4dTransposed(r) * v;
	QVERIFY(close3(p, w, 1e-14));
	// Mat4d::zrotation(a) has the opposite sign of eraRz(a): ERFA matrices must not be rebuilt with Mat4d rotations
	double rz[3][3];
	eraIr(rz);
	eraRz(0.3, rz);
	double b[3];
	eraRxp(rz, p, b);
	QVERIFY(close3(b, Mat4d::zrotation(-0.3) * Vec3d(p[0], p[1], p[2]), 1e-14));
}

void TestHorizonsPrecNut::testOrthonormal()
{
	for (int k = 0; k < 5; ++k)
		for (int nut = 0; nut < 2; ++nut)
		{
			HPN::Params p = params(DATES[k]);
			p.useNutation = (nut == 1);
			HPN::Result res = HPN::compute(p);
			QVERIFY(res.ok);
			QCOMPARE(res.zone, ZONES[k]);
			double rt[3][3], prod[3][3], id[3][3];
			for (int i = 0; i < 3; ++i)
				for (int j = 0; j < 3; ++j)
					rt[i][j] = res.r[j][i];
			eraRxr(res.r, rt, prod);
			eraIr(id);
			QVERIFY2(frobenius(prod, id) < 1e-14, qPrintable(QString("zone %1, nutation %2").arg(k + 1).arg(nut)));
		}
}

void TestHorizonsPrecNut::testPrecessionOnly()
{
	// Without nutation the result is the precession matrix alone, the true obliquity is the mean one, no EOP and no offset
	HPN::Params p = params(DATES[2]);
	p.useNutation = false;
	p.eopDpsi = 1.;
	p.eopDeps = 1.;
	const HPN::Result res = HPN::compute(p);
	double rp[3][3];
	eraPmat76(J2000, p.jdTT - J2000, rp);
	QVERIFY(frobenius(res.r, rp) < 1e-15);
	QCOMPARE(res.epsTrue, res.epsMean);
	QCOMPARE(res.dpsi, 0.);
	QCOMPARE(res.deps, 0.);
}

void TestHorizonsPrecNut::testZone3FollowsErfa()
{
	// Zone 3 is N * P with the IAU 1976 precession, the IAU 1980 nutation and obliquity, all at JD(TT)
	HPN::Params p = params(DATES[2]);
	p.jdTDB = p.jdTT + 1.;	// not used in zone 3
	const HPN::Result res = HPN::compute(p);
	double rp[3][3], rn[3][3], expected[3][3], dp, de;
	eraPmat76(J2000, p.jdTT - J2000, rp);
	eraNut80(J2000, p.jdTT - J2000, &dp, &de);
	const double eps = eraObl80(J2000, p.jdTT - J2000);
	eraNumat(eps, dp, de, rn);
	eraRxr(rn, rp, expected);
	QVERIFY(frobenius(res.r, expected) < 1e-15);
	QVERIFY(std::fabs(res.epsMean - eps) < 1e-15);
	QVERIFY(std::fabs(res.epsTrue - (eps + de)) < 1e-15);

	// Zone 2 uses JD(TDB) instead
	HPN::Params q = params(DATES[1]);
	q.jdTT = q.jdTDB + 1.;	// not used in zone 2
	const HPN::Result res2 = HPN::compute(q);
	eraPmat76(J2000, q.jdTDB - J2000, rp);
	eraNut80(J2000, q.jdTDB - J2000, &dp, &de);
	dp += HPN::dpsiOffsetMas(Zone::Zone2, false) / 1000. * HPN::ARCSEC2RAD;
	const double eps2 = eraObl80(J2000, q.jdTDB - J2000);
	eraNumat(eps2, dp, de, rn);
	eraRxr(rn, rp, expected);
	QVERIFY(frobenius(res2.r, expected) < 1e-15);
}

void TestHorizonsPrecNut::testOwenZoneSanity()
{
	// Owen (1990) and IAU 1976 are two precession models of the same quality: far from J2000 they must stay within seconds of arc
	for (int k = 0; k < 5; k += 4)	// zones 1 and 5
	{
		HPN::Params p = params(DATES[k]);
		p.useNutation = false;
		const HPN::Result res = HPN::compute(p);
		double rp[3][3];
		eraPmat76(J2000, p.jdTT - J2000, rp);
		const double diff = frobenius(res.r, rp);
		QVERIFY2(diff < 10. * HPN::ARCSEC2RAD, qPrintable(QString("zone %1: %2 arcsec").arg(k + 1).arg(diff / HPN::ARCSEC2RAD)));
		QVERIFY(diff > 0.);
		// the obliquity of the Owen theory is not the IAU 1980 one far from J2000, but is close to it
		QVERIFY(std::fabs(res.epsMean - eraObl80(J2000, p.jdTT - J2000)) < 10. * HPN::ARCSEC2RAD);
	}
}

void TestHorizonsPrecNut::testEopAndOffsets()
{
	const double as = HPN::ARCSEC2RAD;
	for (int k = 0; k < 5; ++k)
	{
		HPN::Params p = params(DATES[k]);
		const double jd = (k < 2) ? p.jdTDB : p.jdTT;
		double dp80, de80;
		eraNut80(J2000, jd - J2000, &dp80, &de80);

		const HPN::Result noEop = HPN::compute(p);
		p.eopDpsi = 0.1;
		p.eopDeps = 0.05;
		const HPN::Result withEop = HPN::compute(p);
		const double offset = HPN::dpsiOffsetMas(ZONES[k], false) / 1000.;
		QVERIFY2(std::fabs(noEop.dpsi - (dp80 + offset * as)) < 1e-15, qPrintable(QString("zone %1 without EOP").arg(k + 1)));
		QVERIFY(std::fabs(noEop.deps - de80) < 1e-15);
		if (k < 2)
		{
			// zones 1 and 2: the EOP are not used
			QVERIFY(std::fabs(withEop.dpsi - noEop.dpsi) < 1e-18);
			QVERIFY(std::fabs(withEop.deps - noEop.deps) < 1e-18);
		}
		else
		{
			QVERIFY2(std::fabs(withEop.dpsi - (noEop.dpsi + 0.1 * as)) < 1e-15, qPrintable(QString("zone %1 with EOP").arg(k + 1)));
			QVERIFY(std::fabs(withEop.deps - (noEop.deps + 0.05 * as)) < 1e-15);
		}
	}
	QCOMPARE(HPN::dpsiOffsetMas(Zone::Zone1, false), 65.80);
	QCOMPARE(HPN::dpsiOffsetMas(Zone::Zone2, false), 65.49);
	QCOMPARE(HPN::dpsiOffsetMas(Zone::Zone3, false), 0.);
	QCOMPARE(HPN::dpsiOffsetMas(Zone::Zone4, false), 1.79);
	QCOMPARE(HPN::dpsiOffsetMas(Zone::Zone5, false), 2.42);
	QCOMPARE(HPN::dpsiOffsetMas(Zone::Zone4, true), 0.5);
	QCOMPARE(HPN::dpsiOffsetMas(Zone::Zone5, true), 0.5);
	QCOMPARE(HPN::dpsiOffsetMas(Zone::Zone3, true), 0.);
}

void TestHorizonsPrecNut::testMoonOffset()
{
	// The matrices of the Moon and of the other objects differ by a rotation of (offset - offset Moon) about the ecliptic pole.
	// For a rotation by theta, |R - I|_F = 2 sqrt(2) sin(theta/2) = sqrt(2) theta.
	const double expectedMas[2] = { 1.79 - 0.5, 2.42 - 0.5 };
	const double dates[2] = { DATES[3], DATES[4] };
	for (int k = 0; k < 2; ++k)
	{
		HPN::Params p = params(dates[k]);
		const HPN::Result gen = HPN::compute(p);
		p.forMoon = true;
		const HPN::Result moon = HPN::compute(p);
		const double expected = std::sqrt(2.) * expectedMas[k] / 1000. * HPN::ARCSEC2RAD;
		const double diff = frobenius(gen.r, moon.r);
		QVERIFY2(diff > 0.8 * expected && diff < 1.2 * expected, qPrintable(QString("zone %1: %2 vs %3").arg(k + 4).arg(diff).arg(expected)));
	}
	// no difference in the other zones
	for (int k = 0; k < 3; ++k)
	{
		HPN::Params p = params(DATES[k]);
		const HPN::Result gen = HPN::compute(p);
		p.forMoon = true;
		QCOMPARE(frobenius(gen.r, HPN::compute(p).r), 0.);
	}
}

void TestHorizonsPrecNut::testOutOfRange()
{
	HPN::Params p = params(-3000000.);
	QVERIFY(!HPN::compute(p).ok);
	p = params(6000000.);
	const HPN::Result res = HPN::compute(p);
	QVERIFY(!res.ok);
	QCOMPARE(res.zone, Zone::None);
}

void TestHorizonsPrecNut::testTiming()
{
	// No assertion on the speed but a generous limit: this is done for each change of date, not for each object
	const int n = 20000;
	for (int k = 2; k < 5; k += 2)	// zone 3 (IAU 1976/1980) and zone 5 (Owen)
	{
		QElapsedTimer t;
		t.start();
		double sum = 0.;
		for (int i = 0; i < n; ++i)
		{
			const HPN::Result res = HPN::compute(params(DATES[k] + i * 0.01));
			sum += res.r[0][0];
		}
		const double us = t.nsecsElapsed() / 1000. / n;
		qInfo() << "HorizonsPrecNut::compute, zone" << (k + 1) << ":" << us << "microseconds per call" << sum;
		QVERIFY(us < 1000.);
	}
}
