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

#include "tests/testHorizonsSite.hpp"
#include "HorizonsSite.hpp"

#include <erfa.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

QTEST_GUILESS_MAIN(TestHorizonsSite)

typedef HorizonsSite HS;
typedef HorizonsZones::Zone Zone;

namespace
{
const double TODAY = 2461322.5;		// 2026-10-09 0h UTC
const double ZONE4_START = TODAY + HorizonsZones::DEFAULT_ZONE4_DAYS;
const double TT_MINUS_UT = 69.2 / 86400.;
const double AS = 4.848136811095359935899141e-6;

// one date per zone (JD(UT)), see HorizonsZones
const double DATES[5] = { 2300000., 2400000., 2451545., 2500000., 3000000. };
const Zone ZONES[5] = { Zone::Zone1, Zone::Zone2, Zone::Zone3, Zone::Zone4, Zone::Zone5 };

// Palomar Mountain (Horizons site), WGS84
const double PAL_LON = -(116. + 51. / 60. + 45.01 / 3600.) * M_PI / 180.;
const double PAL_LAT = (33. + 21. / 60. + 14.89 / 3600.) * M_PI / 180.;
const double PAL_H = 1704.;

HS::Params params(double jdUT)
{
	HS::Params p;
	p.jdUT = jdUT;
	p.jdTT = p.jdTDB = jdUT + TT_MINUS_UT;
	p.zone4Start = ZONE4_START;
	p.eopDpsi = -0.05;
	p.eopDeps = -0.005;
	p.ut1utc = -0.034262;
	p.xp = 0.12;
	p.yp = 0.34;
	p.lod = 0.0005;
	p.lonRad = PAL_LON;
	p.latRad = PAL_LAT;
	p.heightM = PAL_H;
	return p;
}

double norm3(const double a[3]) { return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]); }

double dist3(const double a[3], const double b[3])
{
	const double d[3] = { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
	return norm3(d);
}
}

void TestHorizonsSite::testWgs84Site()
{
	// equator, Greenwich, sea level: (a, 0, 0); pole: (0, 0, b)
	HS::Params p = params(DATES[2]);
	p.lonRad = 0.; p.latRad = 0.; p.heightM = 0.;
	HS::Result r = HS::compute(p);
	QVERIFY(r.ok);
	QVERIFY(std::fabs(r.rItrs[0] - 6378137.0) < 1e-6);
	QVERIFY(std::fabs(r.rItrs[1]) < 1e-6);
	QVERIFY(std::fabs(r.rItrs[2]) < 1e-6);
	p.latRad = M_PI / 2.;
	r = HS::compute(p);
	QVERIFY(std::fabs(r.rItrs[2] - 6356752.314245) < 1e-3);
	// height adds along the normal
	p.latRad = 0.; p.heightM = 1000.;
	r = HS::compute(p);
	QVERIFY(std::fabs(r.rItrs[0] - 6379137.0) < 1e-6);
}

void TestHorizonsSite::testRotationsKeepRadius()
{
	// every step ITRS -> J2000 is a rotation: the distance to the geocentre is the same in every zone
	for (int k = 0; k < 5; ++k)
	{
		const HS::Result r = HS::compute(params(DATES[k]));
		QVERIFY2(r.ok, qPrintable(QString("zone %1").arg(k + 1)));
		QCOMPARE(r.zone, ZONES[k]);
		const double R = norm3(r.rItrs);
		QVERIFY2(std::fabs(norm3(r.rTod) - R) < 1e-6, qPrintable(QString("zone %1 rTod").arg(k + 1)));
		QVERIFY2(std::fabs(norm3(r.rJ2000) - R) < 1e-6, qPrintable(QString("zone %1 rJ2000").arg(k + 1)));
		QVERIFY2(std::fabs(norm3(r.vJ2000) - norm3(r.vTod)) < 1e-9, qPrintable(QString("zone %1 v").arg(k + 1)));
	}
}

void TestHorizonsSite::testEarlyZonesIgnoreEop()
{
	// before 1962-01-20 there are no EOP: UT1-UTC, polar motion and LOD of the parameters are not used
	for (int k = 0; k < 2; ++k)
	{
		HS::Params p = params(DATES[k]);
		const HS::Result a = HS::compute(p);
		p.ut1utc = 0.; p.xp = 0.; p.yp = 0.; p.lod = 0.;
		const HS::Result b = HS::compute(p);
		QCOMPARE(a.last, b.last);
		QCOMPARE(a.lastPM, b.lastPM);
		QVERIFY(dist3(a.rJ2000, b.rJ2000) < 1e-9);
		QCOMPARE(a.xp, 0.);
		QCOMPARE(a.ut1utc, 0.);
		// no pole: identity matrix, and the sidereal time with polar motion is the one without
		QVERIFY(std::fabs(a.lastPM - a.last) < 1e-12);
	}
}

void TestHorizonsSite::testUt1Rate()
{
	// 1 s of UT1 is 1.00273790935 s of sidereal time: LAST changes by that angle (zone 3)
	HS::Params p = params(DATES[2]);
	p.xp = 0.; p.yp = 0.;
	p.ut1utc = 0.1;
	const HS::Result a = HS::compute(p);
	p.ut1utc = 1.1;
	const HS::Result b = HS::compute(p);
	const double expected = 1.00273790935 * 2. * M_PI / 86400.;
	const double d = eraAnpm(b.last - a.last);
	QVERIFY2(std::fabs(d - expected) < 2e-10, qPrintable(QString("%1 vs %2").arg(d, 0, 'e', 12).arg(expected, 0, 'e', 12)));
	// and so does the position of the site in J2000: it turns about the pole by that angle
	QVERIFY(std::fabs(b.gast - a.gast - expected) < 2e-10);
}

void TestHorizonsSite::testPolarMotion()
{
	// With xp = yp = 0 the pole is the CIP (up to s', below 1 mas): lastPM is the longitude of the site: last
	HS::Params p = params(DATES[2]);
	p.xp = 0.; p.yp = 0.;
	const HS::Result a = HS::compute(p);
	QVERIFY(std::fabs(eraAnpm(a.lastPM - a.last)) < 1e-8);	// 1e-8 rad = 2 mas (s' is a few tenths of mas)
	// With a pole of 0.3" the site moves by at most 0.3" * R (10 m) and at least by 0.1" * R
	p.xp = 0.3; p.yp = 0.1;
	const HS::Result b = HS::compute(p);
	const double R = norm3(a.rItrs);
	const double moved = dist3(a.rTod, b.rTod);
	QVERIFY2(moved < 0.35 * AS * R && moved > 0.1 * AS * R, qPrintable(QString("moved %1 m").arg(moved)));
	// the polar motion matrix is a rotation
	double rm[3][3], rt[3][3], prod[3][3];
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
		{
			rm[i][j] = b.rpom[i][j];
			rt[i][j] = b.rpom[j][i];
		}
	eraRxr(rm, rt, prod);
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			QVERIFY(std::fabs(prod[i][j] - (i == j ? 1. : 0.)) < 1e-14);
	// the sidereal times are not changed by the polar motion, only lastPM
	QCOMPARE(a.gast, b.gast);
	QCOMPARE(a.last, b.last);
}

void TestHorizonsSite::testSiderealTimes()
{
	// GAST - GMST is the equation of the equinoxes: below 1.2 s of time (18") whatever the zone
	for (int k = 0; k < 5; ++k)
	{
		const HS::Result r = HS::compute(params(DATES[k]));
		QVERIFY2(std::fabs(eraAnpm(r.gast - r.gmst)) < 20. * AS, qPrintable(QString("zone %1").arg(k + 1)));
		QVERIFY(std::fabs(eraAnpm(r.last - r.gast - PAL_LON)) < 1e-12);
	}
	// without nutation GAST is GMST
	HS::Params p = params(DATES[2]);
	p.useNutation = false;
	const HS::Result r = HS::compute(p);
	QCOMPARE(r.gast, r.gmst);
	// the equation of the equinoxes is the one of ERFA when the model dPsi is the one given (IAU 1980, no EOP)
	const double jd = DATES[2] + TT_MINUS_UT;
	double dp, de;
	eraNut80(2451545.0, jd - 2451545.0, &dp, &de);
	const double eps = eraObl80(2451545.0, jd - 2451545.0);
	const double mine = HS::equationOfEquinoxes((jd - 2451545.0) / 36525., dp, eps);
	QVERIFY(std::fabs(mine - eraEqeq94(2451545.0, jd - 2451545.0)) < 1e-12);
}

void TestHorizonsSite::testVelocity()
{
	// equator, sea level, no LOD: speed omega * a = 465.1 m/s, perpendicular to the radius, in the equator plane of date
	HS::Params p = params(DATES[2]);
	p.latRad = 0.; p.heightM = 0.; p.lod = 0.;
	const HS::Result r = HS::compute(p);
	QVERIFY(std::fabs(norm3(r.vTod) - 7.2921151467064e-5 * 6378137.0) < 1e-6);
	QVERIFY(std::fabs(r.vTod[0] * r.rTod[0] + r.vTod[1] * r.rTod[1] + r.vTod[2] * r.rTod[2]) < 1e-6);
	QVERIFY(std::fabs(r.vTod[2]) < 1e-12);
	// LOD of 1 ms slows the rotation by 1 ms/86400 s
	p.lod = 0.001;
	const HS::Result s = HS::compute(p);
	QVERIFY(std::fabs(norm3(s.vTod) / norm3(r.vTod) - (1. - 0.001 / 86400.)) < 1e-12);
	// at the pole there is no velocity
	p.latRad = M_PI / 2.; p.lod = 0.; p.xp = 0.; p.yp = 0.;	// (with a pole offset the site is a few metres off the axis)
	QVERIFY(norm3(HS::compute(p).vTod) < 1e-6);
}

void TestHorizonsSite::testHourAngleAzEl()
{
	const double lat = PAL_LAT;
	// a direction on the local meridian at the latitude of the site: zenith
	const double last = 1.234;
	double p3[3];
	const double dec = lat, ra = last;
	p3[0] = std::cos(dec) * std::cos(ra);
	p3[1] = std::cos(dec) * std::sin(ra);
	p3[2] = std::sin(dec);
	double ha, az, el;
	HS::hourAngleAzEl(last, p3, lat, ha, az, el);
	QVERIFY(std::fabs(ha) < 1e-12);
	QVERIFY(std::fabs(el - M_PI / 2.) < 1e-9);
	// a direction whose RA is 15 degrees east of the local sidereal time is east of the meridian: hour angle -15 degrees
	const double ra2 = last + 15. * M_PI / 180.;
	p3[0] = std::cos(ra2); p3[1] = std::sin(ra2); p3[2] = 0.;
	HS::hourAngleAzEl(last, p3, lat, ha, az, el);
	QVERIFY(std::fabs(ha - (-15. * M_PI / 180.)) < 1e-12);
	// on the meridian (ha = 0), north site, dec 0: azimuth South (180 degrees), elevation 90 - lat
	p3[0] = std::cos(last); p3[1] = std::sin(last); p3[2] = 0.;
	HS::hourAngleAzEl(last, p3, lat, ha, az, el);
	QVERIFY(std::fabs(az - M_PI) < 1e-9);
	QVERIFY(std::fabs(el - (M_PI / 2. - lat)) < 1e-9);
}

void TestHorizonsSite::testOutOfRange()
{
	HS::Params p = params(-3000000.);
	QVERIFY(!HS::compute(p).ok);
	p = params(6000000.);
	const HS::Result r = HS::compute(p);
	QVERIFY(!r.ok);
	QCOMPARE(r.zone, Zone::None);
}

// (SS) 2026-11-10 Golden values: output of mercury_r6.cpp with TOPOCENTRIC true, Palomar Mountain (lon -116.8625, lat 33.354136, h 1704.89 m), EOP files of
// 2026-11-08, JD(UT) 2300000.5, 2400000.5, 2461325.5, 2488069.5, 3000000.5 (zones 1 to 5), with r_itrs, r_top, r_j2000, v_j2000 and rnpSite printed.
// UT1-UTC is printed to 1e-6 s only: 0.5e-6 s of rotation moves the site by 0.2 mm, so the tolerance is 1 mm on positions, 1e-6 m/s on velocities and 2e-10 rad
// (0.04 mas) on the sidereal times. rnpSite is checked to 1e-12. Only zone 3 has a JPL Horizons reference (HA, Az, El of Horizons agree with the standalone
// program to 15 mas); the other zones check that the port gives what the standalone program gives (zones 1 and 5: rnpSite is not the rnp of the object).
namespace
{
double hmsRad(int h, int m, double s) { return (h * 3600. + m * 60. + s) / 86400. * 2. * M_PI; }

struct Golden
{
	double jdUT, jdTT, jdTDB, ut1utc, xp, yp, lod, dpsi, deps;	// inputs (EOP in arcsec and s; unused in zones 1 and 2)
	double gmst, gast, last, lastPM;
	double rTod[3], rJ2000[3], vJ2000[3];
	double rnpSite[3][3];
	double p3[3];	// topocentric p3 of the standalone program (true equator of date)
	double ha, azDeg, elDeg;
};

const double R_ITRS[3] = { -2410356.211509282, -4758788.463240026, 3487769.634639088 };

HS::Params goldenParams(const Golden& g)
{
	HS::Params p;
	p.jdUT = g.jdUT; p.jdTT = g.jdTT; p.jdTDB = g.jdTDB;
	p.zone4Start = 2461437.5;	// "JD(UT) ZONE 3 END" of the standalone program
	p.eopDpsi = g.dpsi; p.eopDeps = g.deps;
	p.ut1utc = g.ut1utc; p.xp = g.xp; p.yp = g.yp; p.lod = g.lod;
	p.lonRad = -116.8625 * M_PI / 180.;
	p.latRad = 33.354136 * M_PI / 180.;
	p.heightM = 1704.89;
	return p;
}

const Golden GOLD[5] = {
	// zone 1 (no EOP)
	{ 2300000.5, 2300000.501561753, 2300000.501561764, 0., 0., 0., 0., 0., 0.,
	  hmsRad(8, 44, 7.19956889), hmsRad(8, 44, 8.10979670), hmsRad(0, 56, 41.10979670), hmsRad(0, 56, 41.10979670),
	  { 5172069.883812455, 1305977.726145420, 3487769.634639088 }, { 4884595.569255932, 1771879.422652202, 3690743.985286061 }, { -129.612627496, 366.734357510, -4.525861379 },
	  { { 0.994904183981147, 0.092440034321229, 0.040255493432550 }, { -0.092441244963131, 0.995716449527798, -0.001835311805096 }, { -0.040252713280902, -0.001895308535710, 0.999187733551148 } },
	  { 7.369959863876953E-01, -6.060237982255023E-01, -2.992859369111153E-01 }, hmsRad(3, 34, 24.331), 233.839243130, 17.962183979 },
	// zone 2 (no EOP)
	{ 2400000.5, 2400000.500108447, 2400000.500108433, 0., 0., 0., 0., 0., 0.,
	  hmsRad(3, 43, 2.57267469), hmsRad(3, 43, 2.92352065), hmsRad(19, 55, 35.92352065), hmsRad(19, 55, 35.92352065),
	  { 2577998.350240306, -4670097.344851176, 3487769.634639088 }, { 2675837.674051829, -4587156.106911051, 3523962.729867013 }, { 334.423978585, 198.628490193, 4.618553611 },
	  { { 0.999409445898367, 0.031511611078229, 0.013703204602315 }, { -0.031511067424032, 0.999503370319300, -0.000255636770041 }, { -0.013704454710663, -0.000176316801450, 0.999906074005688 } },
	  { -4.289301295436010E-01, -8.178946407268967E-01, -3.834935470642634E-01 }, hmsRad(3, 46, 17.673), 232.099809554, 12.362818005 },
	// zone 3 (EOP interpolated)
	{ 2461325.5, 2461325.500800740, 2461325.500800722, -0.034520, 0.163782, 0.323584, 0.0011430, -0.122082, -0.010398,
	  hmsRad(1, 22, 20.29630251), hmsRad(1, 22, 20.77875771), hmsRad(17, 34, 53.77875771), hmsRad(17, 34, 53.76601507),
	  { -583143.595934285, -5302432.415930985, 3487775.186210167 }, { -605940.867141998, -5298713.076075097, 3489541.106147946 }, { 386.394999621, -44.851756519, -1.009894286 },
	  { { 0.999978434244705, -0.006023455289813, -0.002617065509257 }, { 0.006023351662326, 0.999981858325790, -0.000047476859518 }, { 0.002617304006048, 0.000031712329758, 0.999996574351167 } },
	  { -7.221962643470267E-01, -6.130290526916005E-01, -3.203559525266206E-01 }, hmsRad(2, 53, 35.543), 225.213201149, 23.502959219 },
	// zone 4 (last EOP)
	{ 2488069.5, 2488069.500800740, 2488069.500800740, -0.140070, 0.072033, 0.397038, 0., -0.115158, -0.008501,
	  hmsRad(6, 42, 57.03623380), hmsRad(6, 42, 57.23008189), hmsRad(22, 55, 30.23008189), hmsRad(22, 55, 30.21945789),
	  { 5124558.305282254, -1481461.729033583, 3487777.953039689 }, { 5123782.861010545, -1596013.487251004, 3438024.413855391 }, { 116.360761899, 371.177378685, -1.106209413 },
	  { { 0.999702281507601, -0.022380072109656, -0.009720119385283 }, { 0.022379669044089, 0.999749532552554, -0.000150247883037 }, { 0.009721047370248, -0.000067329903448, 0.999952747235943 } },
	  { 3.088807524427212E-01, -8.573663087381234E-01, -4.117228356688314E-01 }, hmsRad(3, 36, 15.240), 229.152631519, 12.730912251 },
	// zone 5 (last EOP)
	{ 3000000.5, 3000000.500800741, 3000000.500800736, -0.140070, 0.072033, 0.397038, 0., -0.115158, -0.008501,
	  hmsRad(21, 37, 3.96384110), hmsRad(21, 37, 4.97634105), hmsRad(13, 49, 37.97634105), hmsRad(13, 49, 37.96567314),
	  { -4735611.697196009, -2455566.845396158, 3487777.953039689 }, { -4733425.957850993, -839827.885743701, 4184470.044391274 }, { 53.209034716, -384.955913647, -17.071620584 },
	  { { 0.933259859656722, -0.329848154467997, -0.142218948623492 }, { 0.329841854490105, 0.943723173931830, -0.024308887480943 }, { 0.142233559260969, -0.024223252838730, 0.989536683828279 } },
	  { -8.381145913101913E-01, 4.884644790422723E-01, 2.428299498556812E-01 }, hmsRad(3, 50, 34.176), 264.242617124, 34.553676276 }
};
}

void TestHorizonsSite::testGoldenStandalone()
{
	for (int k = 0; k < 5; ++k)
	{
		const Golden& g = GOLD[k];
		const HS::Params p = goldenParams(g);
		const HS::Result r = HS::compute(p);
		const QString z = QString("zone %1").arg(k + 1);
		QVERIFY2(r.ok, qPrintable(z));
		QCOMPARE(r.zone, ZONES[k]);

		// site in ITRS (WGS84)
		QVERIFY2(dist3(r.rItrs, R_ITRS) < 1e-6, qPrintable(QString("%1 r_itrs").arg(z)));

		// sidereal times
		QVERIFY2(std::fabs(eraAnpm(r.gmst - g.gmst)) < 2e-10, qPrintable(QString("%1 GMST").arg(z)));
		QVERIFY2(std::fabs(eraAnpm(r.gast - g.gast)) < 2e-10, qPrintable(QString("%1 GAST").arg(z)));
		QVERIFY2(std::fabs(eraAnpm(r.last - g.last)) < 2e-10, qPrintable(QString("%1 LAST").arg(z)));
		QVERIFY2(std::fabs(eraAnpm(r.lastPM - g.lastPM)) < 2e-10, qPrintable(QString("%1 LAST with polar motion").arg(z)));

		// rnpSite: zones 1 and 5 are the plain IAU 1976/80 matrix, the others are the matrix of the object
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				QVERIFY2(std::fabs(r.rSite[i][j] - g.rnpSite[i][j]) < 1e-12, qPrintable(QString("%1 rnpSite[%2][%3]").arg(z).arg(i).arg(j)));

		// site and velocity
		QVERIFY2(dist3(r.rTod, g.rTod) < 1e-3, qPrintable(QString("%1 r_top").arg(z)));
		QVERIFY2(dist3(r.rJ2000, g.rJ2000) < 1e-3, qPrintable(QString("%1 r_j2000").arg(z)));
		QVERIFY2(dist3(r.vJ2000, g.vJ2000) < 1e-6, qPrintable(QString("%1 v_j2000").arg(z)));

		// hour angle, azimuth and elevation of the topocentric p3 of the standalone program, from our LAST with polar motion
		double ha, az, el;
		HS::hourAngleAzEl(r.lastPM, g.p3, p.latRad, ha, az, el);
		QVERIFY2(std::fabs(eraAnpm(ha - g.ha)) < 4e-8, qPrintable(QString("%1 hour angle").arg(z)));
		QVERIFY2(std::fabs(az * 180. / M_PI - g.azDeg) < 2e-8, qPrintable(QString("%1 azimuth %2 vs %3").arg(z).arg(az * 180. / M_PI, 0, 'f', 9).arg(g.azDeg, 0, 'f', 9)));
		QVERIFY2(std::fabs(el * 180. / M_PI - g.elDeg) < 2e-8, qPrintable(QString("%1 elevation %2 vs %3").arg(z).arg(el * 180. / M_PI, 0, 'f', 9).arg(g.elDeg, 0, 'f', 9)));
	}
}
