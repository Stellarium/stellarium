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

#include "testEOPReader.hpp"
#include "EOPTestData.hpp"
#include "EOPReader.hpp"
#include "EOPUpdater.hpp"

#include <QTemporaryDir>
#include <cmath>

QTEST_GUILESS_MAIN(TestEOPReader)

using namespace EOPTestData;

namespace
{
const double JD_OFFSET = 2400000.5;
bool close(double a, double b, double tol = 1e-12) { return std::fabs(a - b) <= tol; }

// merge the text of a file into a reader
bool mergeText(EOPReader& r, const QString& text, EOPReader::LoadResult* res = nullptr)
{
	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QString err;
	if (!EOPReader::parseText(text, recs, fmt, err))
		return false;
	const EOPReader::LoadResult lr = r.merge(recs, fmt);
	if (res)
		*res = lr;
	return lr.ok;
}

EOPReader loadC04()      // MJD 60000..60004
{
	EOPReader r;
	mergeText(r, c04File(60000, 60004));
	return r;
}

// finals rows 60005 final, 60006 and 60007 predicted without nutation
QString finalsTail()
{
	return finalsHeader()
	     + finalsFinalRow(60005, "0.110", "0.210", "0.0510", "-120.0", "-7.0")
	     + finalsPredictionRow(60006, "0.120", "0.220", "0.0520")
	     + finalsPredictionRow(60007, "0.130", "0.230", "0.0530");
}
}

void TestEOPReader::testDetectFormat()
{
	QCOMPARE(EOPReader::detectFormat(c04Header()), EOPReader::Format::C04);
	QCOMPARE(EOPReader::detectFormat(finalsHeader()), EOPReader::Format::Finals);
	QCOMPARE(EOPReader::detectFormat(QString(QChar(0xFEFF)) + finalsHeader()), EOPReader::Format::Finals);
	QCOMPARE(EOPReader::detectFormat("hello"), EOPReader::Format::Unknown);
	// the .txt layout of the IERS is not read any more
	QCOMPARE(EOPReader::detectFormat("     Date      MJD     xp(\")      yp(\")    UT1-UTC(s)   LOD(s)"), EOPReader::Format::Unknown);
}

void TestEOPReader::testC04()
{
	const EOPReader r = loadC04();
	QCOMPARE(r.size(), 5);
	QVERIFY(close(r.firstMJD(), 60000.));      // "60000.00" in the file
	QVERIFY(close(r.lastMJD(), 60004.));
	QCOMPARE(r.gapCount(), 0);
	QVERIFY(r.hasLastNutation());
	QVERIFY(close(r.record(3).dpsi, 0.03));
	QVERIFY(close(r.record(3).deps, 0.006));
	QVERIFY(r.record(3).hasPole && r.record(3).hasUT && r.record(3).hasLOD && r.record(3).hasNutation);
	QVERIFY(!r.record(3).hasDXDY);
	// no Type in the C04 files: all values are final
	QCOMPARE(r.record(3).poleStatus, EOPReader::Status::Final);
	QCOMPARE(r.record(3).utStatus, EOPReader::Status::Final);
	QCOMPARE(r.record(3).nutationStatus, EOPReader::Status::Final);
	QCOMPARE(r.record(3).source, EOPReader::Format::C04);

	// midpoint of MJD 60001 and 60002
	EOPReader::Interpolated v;
	QVERIFY(r.interpolate(60001.5 + JD_OFFSET, v));
	QCOMPARE(v.coverage, EOPReader::Coverage::Inside);
	QVERIFY(!v.nutationHeld);
	QVERIFY(close(v.rec.dpsi, 0.015));
	QVERIFY(close(v.rec.xp, 0.115));
	QVERIFY(close(v.rec.ut1utc, 0.2 - 0.0015));
	QVERIFY(close(v.rec.lod, 0.001));           // LOD is in seconds in the C04 files
	// exactly on a record
	QVERIFY(r.interpolate(60002. + JD_OFFSET, v));
	QVERIFY(close(v.rec.dpsi, 0.02));
	// before the first record
	QVERIFY(!r.interpolate(59999.5 + JD_OFFSET, v));
}

void TestEOPReader::testLoadFile()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath("eopc04_14.62-now (test, copy).csv");
	QVERIFY(writeFile(path, c04File(60000, 60004)));
	EOPReader r;
	const EOPReader::LoadResult res = r.loadFile(path, true);
	QVERIFY2(res.ok, qPrintable(res.error));
	QCOMPARE(res.format, EOPReader::Format::C04);
	QCOMPARE(res.nAdded, 5);
	QCOMPARE(res.nReplaced, 0);

	EOPReader r2;
	QVERIFY(!r2.loadFile(dir.filePath("missing.csv")).ok);
}

void TestEOPReader::testFinalsMerge()
{
	EOPReader r = loadC04();   // 60000..60004

	QString file = finalsHeader();
	file += finalsFinalRow(60003, "0.100", "0.200", "0.0500", "-100.0", "-5.0");
	file += finalsFinalRow(60004, "0.101", "0.201", "0.0505", "-110.0", "-6.0");
	file += finalsFinalRow(60005, "0.110", "0.210", "0.0510", "-120.0", "-7.0");
	file += finalsPredictionRow(60006, "0.120", "0.220", "0.0520");
	file += finalsPredictionRow(60007, "0.130", "0.230", "0.0530");

	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QString err;
	QVERIFY2(EOPReader::parseText(file, recs, fmt, err), qPrintable(err));
	QCOMPARE(fmt, EOPReader::Format::Finals);
	QCOMPARE(int(recs.size()), 5);
	// units: LOD msec -> sec, dPsi and dEps marcsec -> arcsec
	QVERIFY(close(recs[0].xp, 0.100));
	QVERIFY(close(recs[0].yp, 0.200));
	QVERIFY(close(recs[0].ut1utc, 0.050));
	QVERIFY(close(recs[0].lod, 0.0015));
	QVERIFY(close(recs[0].dpsi, -0.100));
	QVERIFY(close(recs[0].deps, -0.005));
	QVERIFY(close(recs[0].dpsiErr, 0.0005));
	QCOMPARE(recs[0].nutationStatus, EOPReader::Status::Final);
	QCOMPARE(recs[3].poleStatus, EOPReader::Status::Prediction);
	QVERIFY(recs[3].hasPole && recs[3].hasUT);
	QVERIFY(!recs[3].hasNutation && !recs[3].hasLOD);
	QCOMPARE(recs[3].nutationStatus, EOPReader::Status::Missing);

	const EOPReader::LoadResult res = r.merge(recs, fmt);
	QVERIFY(res.ok);
	QCOMPARE(res.nAdded, 3);
	QCOMPARE(res.nReplaced, 2);
	QCOMPARE(res.nFinalAdded, 1);
	QCOMPARE(res.nPredictionAdded, 2);
	// largest changes made to the 2 replaced C04 records (MJD 60003 and 60004)
	QCOMPARE(res.xpDiff.n, 2);
	QVERIFY(close(res.xpDiff.max, 0.039));      // 0.140 -> 0.101
	QVERIFY(close(res.xpDiff.mjd, 60004.));
	QVERIFY(close(res.dpsiDiff.max, 0.15));     // 0.040 -> -0.110
	QCOMPARE(r.size(), 8);
	QCOMPARE(r.gapCount(), 0);
	QVERIFY(close(r.lastMJD(), 60007.));

	// MJD 60003 was replaced by the merged file
	EOPReader::Interpolated v;
	QVERIFY(r.interpolate(60003. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, 0.100));
	QVERIFY(close(v.rec.dpsi, -0.100));
	// untouched record keeps the C04 values
	QVERIFY(r.interpolate(60002. + JD_OFFSET, v));
	QVERIFY(close(v.rec.dpsi, 0.02));
}

void TestEOPReader::testNutationHold()
{
	EOPReader r = loadC04();
	QVERIFY(mergeText(r, finalsTail()));

	QVERIFY(r.hasLastNutation());
	QVERIFY(close(r.lastNutation().mjd, 60005.));

	EOPReader::Interpolated v;
	// between a record with nutation and one without: held value, no interpolation toward zero
	QVERIFY(r.interpolate(60005.5 + JD_OFFSET, v));
	QVERIFY(v.nutationHeld);
	QVERIFY(close(v.rec.dpsi, -0.120));
	QVERIFY(close(v.rec.deps, -0.007));
	// both records without nutation: pole is still interpolated
	QVERIFY(r.interpolate(60006.5 + JD_OFFSET, v));
	QVERIFY(v.nutationHeld);
	QVERIFY(close(v.rec.dpsi, -0.120));
	QVERIFY(v.rec.hasPole);
	QVERIFY(close(v.rec.xp, 0.125));
	QVERIFY(close(v.rec.ut1utc, 0.0525));
	QVERIFY(v.rec.hasLOD && v.lodHeld);              // no LOD after 60005: last value kept
	QVERIFY(close(v.rec.lod, 0.0015));
	QVERIFY(!v.poleHeld && !v.utHeld);
	// last record
	QVERIFY(r.interpolate(60007. + JD_OFFSET, v));
	QCOMPARE(v.coverage, EOPReader::Coverage::Inside);
	QVERIFY(v.nutationHeld);
	QVERIFY(close(v.rec.dpsi, -0.120));
	// after the end: every group keeps its last value, flagged as held
	QVERIFY(r.interpolate(60500.3 + JD_OFFSET, v));
	QCOMPARE(v.coverage, EOPReader::Coverage::AfterEnd);
	QVERIFY(v.nutationHeld);
	QVERIFY(v.rec.hasPole && v.poleHeld);
	QVERIFY(close(v.rec.xp, 0.130));
	QVERIFY(v.rec.hasUT && v.utHeld);
	QVERIFY(close(v.rec.ut1utc, 0.053));
	QVERIFY(v.rec.hasLOD && v.lodHeld);              // LOD ends at 60005 (final): that value is kept, not undefined
	QVERIFY(close(v.rec.lod, 0.0015));
	QCOMPARE(v.rec.lodStatus, EOPReader::Status::Final);
	QCOMPARE(v.rec.utStatus, EOPReader::Status::Prediction);   // UT1-UTC goes on to 60007 (prediction)
	QVERIFY(!v.rec.hasDXDY);
	QVERIFY(close(v.rec.dpsi, -0.120));
	QVERIFY(close(v.rec.deps, -0.007));
}

void TestEOPReader::testLeapSecond()
{
	// (SS) 2026-11-05 UT1-UTC rises by 1 s when a leap second is inserted (TAI-UTC goes from 36 s to 37 s on 2017-01-01, MJD 57754)
	// while UT1-TAI stays continuous: here -0.600 s the day before and 0.399 s on the day of the leap second (UT1-TAI changes
	// by -0.001 s in the day). The first version of this test had the 1 s step with the wrong sign.
	EOPReader r;
	QString s = c04Header();
	s += c04Row(57753, 0.1, -0.600, 0.001, "0.0", "0.0");
	s += c04Row(57754, 0.1, 0.399, 0.001, "0.0", "0.0");
	QVERIFY(mergeText(r, s));
	EOPReader::Interpolated v;
	QVERIFY(r.interpolate(57753.5 + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, -0.6005, 1e-9));

	// 1961-1972: UTC was stepped by 0.1 s (as in EOP 14 C04 at MJD 38334, from -0.1264278 s to -0.0283989 s)
	EOPReader r2;
	QString s2 = c04Header();
	s2 += c04Row(38333, 0.1, -0.1264278, 0.003, "0.0", "0.0");
	s2 += c04Row(38334, 0.1, -0.0283989, 0.003, "0.0", "0.0");
	QVERIFY(mergeText(r2, s2));
	QVERIFY(r2.interpolate(38333.5 + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, (-0.1264278 + (-0.0283989 - 0.1)) / 2.));
	// a normal day to day change is left alone
	EOPReader r3;
	QString s3 = c04Header();
	s3 += c04Row(60000, 0.1, 0.100, 0.001, "0.0", "0.0");
	s3 += c04Row(60001, 0.1, 0.102, 0.001, "0.0", "0.0");
	QVERIFY(mergeText(r3, s3));
	QVERIFY(r3.interpolate(60000.5 + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.101));
}

// (SS) 2026-11-05 UT1-UTC is interpolated as UT1-TAI with TAI-UTC (eraDat()) added back at the requested instant, as in the
// InterpolateEOP() of the standalone program: the steps of UTC fall exactly at 0h UTC and the drift of UTC of 1962-1972 is followed.
void TestEOPReader::testUT1TAIInterpolation()
{
	EOPReader::Interpolated v;

	// 1. Leap second of 2017-01-01 (MJD 57754): just before 0h UTC the value is the one of the next day minus the 1 s step,
	// at 0h UTC it is the value of the record (exact record)
	EOPReader r;
	QString s = c04Header();
	s += c04Row(57753, 0.1, -0.600, 0.001, "0.0", "0.0");
	s += c04Row(57754, 0.1, 0.399, 0.001, "0.0", "0.0");
	QVERIFY(mergeText(r, s));
	QVERIFY(r.interpolate(57754. - 1e-7 + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.399 - 1.0, 1e-6));
	QVERIFY(r.interpolate(57754. + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.399));

	// 2. Step of 0.1 s of 1968-02-01 (MJD 39887): TAI-UTC goes from 4.3131700 + (MJD - 39126) x 0.002592 s to
	// 4.2131700 + (MJD - 39126) x 0.002592 s (rubber second of 1966-1968, as in the ERFA table). UT1-UTC drops by 0.1 s + the
	// drift of a day. Here UT1-TAI changes by -0.0005 s between the two days.
	auto datBefore = [](double mjd) { return 4.3131700 + (mjd - 39126.) * 0.002592; };
	auto datAfter  = [](double mjd) { return 4.2131700 + (mjd - 39126.) * 0.002592; };
	const double u0 = 0.100;
	const double u1 = u0 + (datAfter(39887.) - datBefore(39886.)) - 0.0005;
	EOPReader r2;
	QString s2 = c04Header();
	s2 += c04Row(39886, 0.1, u0, 0.001, "0.0", "0.0");
	s2 += c04Row(39887, 0.1, u1, 0.001, "0.0", "0.0");
	QVERIFY(mergeText(r2, s2));
	// middle of the day before the step: UT1-TAI interpolated, TAI-UTC of that instant added back
	QVERIFY(r2.interpolate(39886.5 + JD_OFFSET, v));
	const double expectedMid = (u0 - datBefore(39886.) + 0.5 * ((u1 - datAfter(39887.)) - (u0 - datBefore(39886.)))) + datBefore(39886.5);
	QVERIFY(close(v.rec.ut1utc, expectedMid, 1e-9));
	// same formula at 1/4 of the day (UT1-TAI interpolated, TAI-UTC of the instant added back)
	QVERIFY(r2.interpolate(39886.25 + JD_OFFSET, v));
	const double expectedQuarter = (u0 - datBefore(39886.) + 0.25 * ((u1 - datAfter(39887.)) - (u0 - datBefore(39886.)))) + datBefore(39886.25);
	QVERIFY(close(v.rec.ut1utc, expectedQuarter, 1e-9));
	// the step falls at 0h UTC: just before, the value is the one of the record plus the 0.1 s step
	QVERIFY(r2.interpolate(39887. - 1e-7 + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, u1 + 0.1, 1e-6));

	// 3. No step, no drift (since 1972 TAI-UTC is constant between leap seconds): plain linear interpolation, also at 1/4 of a day
	EOPReader r3;
	QString s3 = c04Header();
	s3 += c04Row(60000, 0.1, 0.100, 0.001, "0.0", "0.0");
	s3 += c04Row(60001, 0.1, 0.102, 0.001, "0.0", "0.0");
	QVERIFY(mergeText(r3, s3));
	QVERIFY(r3.interpolate(60000.25 + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.1005));
}

// (SS) 2026-11-06 UT1-UTC of JPL Horizons (file of horizons_ut1utc.py): comment lines, header MJD;Date;JD;UT1-UTC, one row per day.
// The values up to the end of the Horizons data are final, the later ones predictions. Merged after the IERS files it replaces
// UT1-UTC only; dropAfter() removes the UT1-UTC values after the last day of the file so that the last value is held.
void TestEOPReader::testHorizonsOverlay()
{
	const QString comments = QStringLiteral("# JPL Horizons UT1-UTC (quantity 49), target Sun, geocentric\n"
	                                        "# EOP coverage: DATA-BASED 1962-JAN-20 TO 2023-MAR-01. PREDICTS-> 2023-MAR-03\n");
	const QString body = QStringLiteral("MJD;Date;JD;UT1-UTC\n"
	                                    "60000;2023-Feb-25;2460000.5;0.10000\n"
	                                    "60001;2023-Feb-26;2460001.5;0.10100\n"
	                                    "60002;2023-Feb-27;2460002.5;0.10200\n"
	                                    "60003;2023-Feb-28;2460003.5;0.10300\n"
	                                    "60004;2023-Mar-01;2460004.5;0.10400\n"
	                                    "60005;2023-Mar-02;2460005.5;0.10500\n"
	                                    "60006;2023-Mar-03;2460006.5;0.10600\n");
	QCOMPARE(EOPReader::detectFormat("MJD;Date;JD;UT1-UTC"), EOPReader::Format::HorizonsUT);
	QCOMPARE(EOPReader::detectFormat(c04Header()), EOPReader::Format::C04);

	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QString err;
	QVERIFY2(EOPReader::parseText(comments + body, recs, fmt, err), qPrintable(err));
	QCOMPARE(fmt, EOPReader::Format::HorizonsUT);
	QCOMPARE(int(recs.size()), 7);
	QVERIFY(recs[0].hasUT && !recs[0].hasPole && !recs[0].hasLOD && !recs[0].hasNutation && !recs[0].hasDXDY);
	QCOMPARE(recs[0].year, 2023);
	QCOMPARE(recs[0].month, 2);
	QCOMPARE(recs[0].day, 25);
	QVERIFY(close(recs[2].ut1utc, 0.102));
	QCOMPARE(recs[4].utStatus, EOPReader::Status::Final);        // 2023-03-01, last day of the Horizons data
	QCOMPARE(recs[5].utStatus, EOPReader::Status::Prediction);
	QCOMPARE(recs[6].source, EOPReader::Format::HorizonsUT);

	// without the coverage comment all the values are final
	QVERIFY(EOPReader::parseText(body, recs, fmt, err));
	QCOMPARE(recs[6].utStatus, EOPReader::Status::Final);
	// a row that is not a number
	QVERIFY(!EOPReader::parseText("MJD;Date;JD;UT1-UTC\n60000;2023-Feb-25;2460000.5;abc\n", recs, fmt, err));

	// merged after C04 (60000..60004) and finals (60005 final, 60006 and 60007 predicted)
	EOPReader r = loadC04();
	QVERIFY(mergeText(r, finalsTail()));
	EOPReader::Interpolated before, v;
	QVERIFY(r.interpolate(60002. + JD_OFFSET, before));
	QVERIFY(r.interpolate(60007. + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.0530));                      // UT1-UTC of the finals file, predicted
	EOPReader::LoadResult res;
	QVERIFY(mergeText(r, comments + body, &res));
	QCOMPARE(res.format, EOPReader::Format::HorizonsUT);
	QCOMPARE(res.nReplaced, 7);
	QCOMPARE(res.nAdded, 0);
	QVERIFY(r.interpolate(60002. + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.102));                       // replaced, although C04 has a final value
	QVERIFY(close(v.rec.xp, before.rec.xp) && close(v.rec.yp, before.rec.yp));   // the other groups are not touched
	QVERIFY(close(v.rec.lod, before.rec.lod));
	QVERIFY(close(v.rec.dpsi, before.rec.dpsi) && close(v.rec.deps, before.rec.deps));
	QVERIFY(r.interpolate(60005. + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.105));
	QCOMPARE(v.rec.utStatus, EOPReader::Status::Prediction);

	// the UT1-UTC value of the finals file after the last day of the overlay is removed: the last Horizons value is held
	QCOMPARE(r.dropAfter(EOPReader::Group::UT, 60006.), 1);
	QCOMPARE(r.dropAfter(EOPReader::Group::UT, 60006.), 0);
	QVERIFY(r.interpolate(60007. + JD_OFFSET, v));
	QVERIFY(v.utHeld && v.rec.hasUT);
	QVERIFY(close(v.rec.ut1utc, 0.106));
	QVERIFY(!v.poleHeld && close(v.rec.xp, 0.130));            // pole of the finals file is still there
	QVERIFY(r.interpolate(60020.5 + JD_OFFSET, v));
	QVERIFY(v.utHeld && close(v.rec.ut1utc, 0.106));

	const EOPReader::Span sp = r.span(EOPReader::Group::UT);
	QVERIFY(sp.valid && sp.hasFinal && sp.hasPrediction());
	QVERIFY(close(sp.firstMJD, 60000.));
	QVERIFY(close(sp.lastFinalMJD, 60004.));
	QVERIFY(close(sp.lastMJD, 60006.));

	// (SS) 2026-11-08 the sigma of UT1-UTC of the IERS is kept where the overlay replaces the value (the overlay has no sigma); a day that is
	// only in the overlay has none
	EOPReader r2;
	QVector<EOPReader::Record> iers;
	for (int d = 0; d < 2; ++d)
	{
		EOPReader::Record a;
		a.mjd = 60000. + d;
		a.hasUT = true;
		a.ut1utc = 0.2;
		a.ut1utcErr = 0.00003 + d * 1e-5;
		a.utStatus = EOPReader::Status::Final;
		a.source = EOPReader::Format::C04;
		iers.append(a);
	}
	r2.merge(iers, EOPReader::Format::C04);
	QVector<EOPReader::Record> hz;
	EOPReader::Format fmtHz;
	QString errHz;
	QVERIFY(EOPReader::parseText("MJD;Date;JD;UT1-UTC\n60000;a;b;0.10000\n60001;a;b;0.10100\n60002;a;b;0.10200\n", hz, fmtHz, errHz));
	r2.merge(hz, fmtHz);
	EOPReader::Interpolated w;
	QVERIFY(r2.interpolate(60000. + JD_OFFSET, w));
	QVERIFY(close(w.rec.ut1utc, 0.10000) && close(w.rec.ut1utcErr, 0.00003));
	QVERIFY(r2.interpolate(60001. + JD_OFFSET, w));
	QVERIFY(close(w.rec.ut1utc, 0.10100) && close(w.rec.ut1utcErr, 0.00004));
	QVERIFY(r2.interpolate(60002. + JD_OFFSET, w));
	QVERIFY(close(w.rec.ut1utc, 0.10200) && close(w.rec.ut1utcErr, 0.));
}

void TestEOPReader::testFinals2000A()
{
	// rows as in finals2000A.all.csv: dX and dY (marcsec), no dPsi/dEps
	QString s = finalsHeader();
	s += finalsRow(41684, "final", "0.120733", "0.136966", "final", "0.8084178", "0.0000", "prediction", "", "", "-0.766", "-0.720");
	s += finalsRow(41685, "final", "0.118980", "0.135656", "final", "0.8056163", "3.5563", "prediction", "", "", "-0.751", "-0.701");
	s += finalsRow(41686, "", "", "", "", "", "", "", "", "");            // row without any value: skipped
	s += finalsRow(41687, "final", "0.115473", "0.133044", "final", "0.7998729", "0.0000", "prediction", "44.969", "2.839");

	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QString err;
	QVERIFY2(EOPReader::parseText(s, recs, fmt, err), qPrintable(err));
	QCOMPARE(fmt, EOPReader::Format::Finals);
	QCOMPARE(int(recs.size()), 3);

	QVERIFY(close(recs[0].mjd, 41684.));
	QCOMPARE(recs[0].year, 1973);
	QCOMPARE(recs[0].month, 1);
	QCOMPARE(recs[0].day, 2);
	QVERIFY(close(recs[0].xp, 0.120733));
	QVERIFY(close(recs[0].ut1utc, 0.8084178));
	QVERIFY(!recs[0].hasLOD);                       // 0.0000 means undefined
	QVERIFY(recs[1].hasLOD);
	QVERIFY(close(recs[1].lod, 0.0035563));
	QVERIFY(recs[0].hasDXDY && !recs[0].hasNutation);
	QVERIFY(close(recs[0].dx, -0.000766));          // marcsec -> arcsec
	QCOMPARE(recs[0].dxdyStatus, EOPReader::Status::Prediction);
	QCOMPARE(recs[1].lodStatus, EOPReader::Status::Final);
	QVERIFY(close(recs[0].dy, -0.000720));
	QCOMPARE(recs[0].nutationStatus, EOPReader::Status::Prediction);
	QCOMPARE(recs[0].poleStatus, EOPReader::Status::Final);
	QVERIFY(close(recs[2].mjd, 41687.));            // the empty row is not there
	QVERIFY(recs[2].hasNutation && !recs[2].hasDXDY);
	QVERIFY(close(recs[2].dpsi, 0.044969));
	QVERIFY(close(recs[2].deps, 0.002839));
}

void TestEOPReader::testC04IAU2000()
{
	// IAU 2000 C04 file: dX, dY (0.000000 is a value), no dPsi/dEps
	EOPReader r;
	QVERIFY(mergeText(r, c04File(60000, 60004)));          // IAU 1980 file
	EOPReader::LoadResult res;
	QVERIFY(mergeText(r, c04File2000(60000, 60004), &res)); // IAU 2000 file: complements it
	QCOMPARE(res.nReplaced, 5);
	QCOMPARE(res.nAdded, 0);
	QCOMPARE(r.size(), 5);

	EOPReader::Interpolated v;
	QVERIFY(r.interpolate(60003. + JD_OFFSET, v));
	QVERIFY(v.rec.hasNutation && v.rec.hasDXDY);
	QVERIFY(close(v.rec.dpsi, 0.03));                      // kept
	QVERIFY(close(v.rec.dx, 0.0003));                      // new
	QVERIFY(close(v.rec.dy, 0.));
	QVERIFY(!v.nutationHeld && !v.dxdyHeld);

	// alone
	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QString err;
	QVERIFY(EOPReader::parseText(c04File2000(60000, 60001), recs, fmt, err));
	QVERIFY(recs[0].hasDXDY && !recs[0].hasNutation);
	QCOMPARE(recs[0].nutationStatus, EOPReader::Status::Final);
}

void TestEOPReader::testMergeKeepsOldValues()
{
	EOPReader r = loadC04();   // 60000..60004, everything present

	// a row for MJD 60003 with a pole but no UT1-UTC, LOD or nutation: the pole is replaced, the other values are kept
	QString file = finalsHeader();
	file += finalsRow(60003, "final", "0.100", "0.200", "", "", "", "", "", "");
	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QString err;
	QVERIFY(EOPReader::parseText(file, recs, fmt, err));
	QVERIFY(!recs[0].hasUT && !recs[0].hasLOD && !recs[0].hasNutation);
	const EOPReader::LoadResult res = r.merge(recs, fmt);
	QVERIFY(res.ok);
	QCOMPARE(res.nReplaced, 1);
	QCOMPARE(r.size(), 5);

	EOPReader::Interpolated v;
	QVERIFY(r.interpolate(60003. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, 0.100));                 // new
	QVERIFY(close(v.rec.yp, 0.200));
	QVERIFY(v.rec.hasUT);
	QVERIFY(close(v.rec.ut1utc, 0.2 - 0.003));       // kept from the C04 file
	QVERIFY(v.rec.hasLOD);
	QVERIFY(close(v.rec.lod, 0.001));
	QVERIFY(v.rec.hasNutation);
	QVERIFY(close(v.rec.dpsi, 0.03));
	QVERIFY(close(v.rec.deps, 0.006));
	QCOMPARE(v.rec.nutationStatus, EOPReader::Status::Final);
	QVERIFY(!v.nutationHeld);
}

void TestEOPReader::testCoverage()
{
	EOPReader r = loadC04();   // 60000..60004, all final
	QVERIFY(mergeText(r, finalsTail()));

	EOPReader::Span s = r.span(EOPReader::Group::Pole);
	QVERIFY(s.valid && s.hasFinal && s.hasPrediction());
	QVERIFY(close(s.firstMJD, 60000.));
	QVERIFY(close(s.lastFinalMJD, 60005.));
	QVERIFY(close(s.lastMJD, 60007.));

	s = r.span(EOPReader::Group::UT);
	QVERIFY(close(s.lastFinalMJD, 60005.) && close(s.lastMJD, 60007.));

	s = r.span(EOPReader::Group::LOD);            // no LOD in the prediction rows
	QVERIFY(close(s.lastMJD, 60005.));
	QVERIFY(!s.hasPrediction());

	s = r.span(EOPReader::Group::Nutation);       // no nutation in the prediction rows
	QVERIFY(close(s.lastFinalMJD, 60005.) && close(s.lastMJD, 60005.));
	QVERIFY(!s.hasPrediction());

	QVERIFY(!r.span(EOPReader::Group::DXDY).valid);
}

void TestEOPReader::testUpdaterHelpers()
{
	QCOMPARE(EOPUpdater::fileNameFor("finals.all.csv", QDate(2026, 10, 7)), QString("finals.all_261007.csv"));
	QCOMPARE(EOPUpdater::fileNameFor("eopc04_20u24.1962-now.csv", QDate(2026, 10, 7)), QString("eopc04_20u24.1962-now_261007.csv"));
	QCOMPARE(EOPUpdater::fileNameFor("eopc04_20u24.dPsi_dEps.1962-now.csv", QDate(2027, 1, 5)), QString("eopc04_20u24.dPsi_dEps.1962-now_270105.csv"));

	QString err;
	int n = 0;
	const QString good = finalsHeader() + finalsFinalRow(60005, "0.110", "0.210", "0.0510", "-120.0", "-7.0")
	                   + finalsPredictionRow(60006, "0.120", "0.220", "0.0520");
	QVERIFY2(EOPUpdater::validateCSV(good.toUtf8(), &err, &n), qPrintable(err));
	QCOMPARE(n, 2);
	QVERIFY(EOPUpdater::validateCSV(c04File(60000, 60002).toUtf8(), &err, &n));
	QCOMPARE(n, 3);
	QVERIFY(!EOPUpdater::validateCSV("<html>Service unavailable</html>", &err));
	QVERIFY(!err.isEmpty());
	QVERIFY(!EOPUpdater::validateCSV(finalsHeader().toUtf8(), &err));   // header only

	QVERIFY(!EOPUpdater::defaultSources().isEmpty());
	QCOMPARE(EOPUpdater::defaultSources().first().fileName, QString("finals.all.csv"));
	QCOMPARE(EOPUpdater::defaultSources().first().refreshDays, 7);
	// (SS) 2026-11-05 only the IAU 1980 set is downloaded by default (finals.all, EOP 14 C04); the IAU 2000A files are optional
	QCOMPARE(int(EOPUpdater::defaultSources().size()), 2);
	QCOMPARE(EOPUpdater::defaultSources().at(1).fileName, QString("eopc04_14.62-now.csv"));
	QCOMPARE(EOPUpdater::defaultSources().at(1).refreshDays, 365);
	QCOMPARE(int(EOPUpdater::optionalSources().size()), 2);
	for (const EOPUpdater::Source& src : EOPUpdater::optionalSources())
		QVERIFY(!src.isDefault);
}

// (SS) 2026-11-06 UT1-UTC of JPL Horizons asked to the Horizons API by EOPUpdater: the request, and the transformation of the text table
// of the answer (layout of a real answer of 2026-11-06) into an overlay file that EOPReader reads.
void TestEOPReader::testHorizonsDownload()
{
	const EOPUpdater::Source src = EOPUpdater::horizonsSource();
	QCOMPARE(src.fileName, QString("horizons_ut1utc.csv"));
	QCOMPARE(src.url.host(), QString("ssd.jpl.nasa.gov"));
	QCOMPARE(src.refreshDays, 1);
	QVERIFY(src.kind == EOPUpdater::Source::Kind::HorizonsUT1UTC);
	QCOMPARE(EOPUpdater::fileNameFor(src.fileName, QDate(2026, 11, 6)), QString("horizons_ut1utc_261106.csv"));
	// the IERS sources are files
	for (const EOPUpdater::Source& s : EOPUpdater::defaultSources())
		QVERIFY(s.kind == EOPUpdater::Source::Kind::File);

	const QString request = EOPUpdater::horizonsRequest(QDate(2027, 4, 5));
	QVERIFY(request.startsWith("!$$SOF\n"));
	QVERIFY(request.contains("START_TIME='1962-01-01'\n"));
	QVERIFY(request.contains("STOP_TIME='2027-04-05'\n"));
	QVERIFY(request.contains("QUANTITIES='49'\n"));
	QVERIFY(request.contains("CENTER='@399'\n"));

	const QString answer = QStringLiteral(
	    "API VERSION: 1.0\n"
	    "API SOURCE: NASA/JPL Horizons API\n"
	    "\n"
	    "*******************************************************************************\n"
	    " Revised: July 31, 2013                  Sun                                 10\n"
	    "\n"
	    "*******************************************************************************\n"
	    "Ephemeris / WWW_USER Fri Nov  6 04:52:09 2026 Pasadena, USA      / Horizons    \n"
	    "*******************************************************************************\n"
	    "Target body name: Sun (10)                        {source: DE441}\n"
	    "Center body name: Earth (399)                     {source: DE441}\n"
	    "EOP file        : eop.261105.p270201                                           \n"
	    "EOP coverage    : DATA-BASED 1962-JAN-20 TO 2026-NOV-05. PREDICTS-> 2027-JAN-31\n"
	    "Table format    : Comma Separated Values (spreadsheet)\n"
	    "*******************************************************************************\n"
	    " Date__(UT)__HR:MN:SC.fff, Date_________JDUT, , ,  UT1-UTC,\n"
	    "***********************************************************\n"
	    "$$SOE\n"
	    " 1962-Jan-18 00:00:00.000, 2437682.500000000, , ,     n.a.,\n"
	    " 1962-Jan-19 00:00:00.000, 2437683.500000000, , ,     n.a.,\n"
	    " 1962-Jan-20 00:00:00.000, 2437684.500000000, , ,  0.02340,\n"
	    " 1962-Jan-21 00:00:00.000, 2437685.500000000, , ,  0.02377,\n"
	    " 2027-Jan-28 00:00:00.000, 2461433.500000000, , , -0.13870,\n"
	    " 2027-Jan-29 00:00:00.000, 2461434.500000000, , , -0.13971,\n"
	    " 2027-Jan-30 00:00:00.000, 2461435.500000000, , , -0.14049,\n"
	    " 2027-Jan-31 00:00:00.000, 2461436.500000000, , , -0.14107,\n"
	    " 2027-Feb-01 00:00:00.000, 2461437.500000000, , , -0.14107,\n"
	    " 2027-Feb-02 00:00:00.000, 2461438.500000000, , , -0.14107,\n"
	    " 2027-Feb-03 00:00:00.000, 2461439.500000000, , , -0.14107,\n"
	    "$$EOE\n"
	    "*******************************************************************************\n"
	    "  API            https://ssd-api.jpl.nasa.gov/doc/horizons.html\n");

	QString csv, err;
	int n = 0;
	QVERIFY2(EOPUpdater::horizonsToOverlayCSV(answer, csv, &err, &n, 3), qPrintable(err));
	QCOMPARE(n, 6);        // 2 n.a. days dropped, 3 days after the last distinct value dropped
	QVERIFY(csv.contains("# Horizons run: 2026-11-06\n"));
	QVERIFY(csv.contains("# EOP file used by Horizons: eop.261105.p270201\n"));
	QVERIFY(csv.contains("# EOP coverage: DATA-BASED 1962-JAN-20 TO 2026-NOV-05. PREDICTS-> 2027-JAN-31\n"));
	QVERIFY(csv.contains("MJD;Date;JD;UT1-UTC\n37684;1962-Jan-20;2437684.500000000;0.02340\n"));
	QVERIFY(csv.endsWith("61436;2027-Jan-31;2461436.500000000;-0.14107\n"));
	// the data rows only (the comment lines say that the first days are n.a.)
	const QString dataRows = csv.mid(csv.indexOf("MJD;Date;JD;UT1-UTC\n"));
	QVERIFY(!dataRows.contains("2027-Feb") && !dataRows.contains("n.a."));
	// the same answer with Windows line ends
	QString csv2;
	QVERIFY(EOPUpdater::horizonsToOverlayCSV(QString(answer).replace("\n", "\r\n"), csv2, &err, &n, 3));
	QCOMPARE(csv2, csv);

	// EOPReader reads the file: final up to the end of the Horizons data, predictions after
	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QVERIFY2(EOPReader::parseText(csv, recs, fmt, err), qPrintable(err));
	QCOMPARE(fmt, EOPReader::Format::HorizonsUT);
	QCOMPARE(int(recs.size()), 6);
	QVERIFY(close(recs[0].mjd, 37684.) && close(recs[0].ut1utc, 0.02340));
	QCOMPARE(recs[1].utStatus, EOPReader::Status::Final);
	QCOMPARE(recs[2].utStatus, EOPReader::Status::Prediction);
	QVERIFY(close(recs[5].mjd, 61436.) && close(recs[5].ut1utc, -0.14107));

	// answers that are not usable: nothing is written by the updater
	QVERIFY(!EOPUpdater::horizonsToOverlayCSV("ERROR: Cannot interpret date. Type: 'Date' in the batch file", csv, &err, &n, 3));
	QVERIFY(err.contains("no table of UT1-UTC") && err.contains("Cannot interpret date"));
	QVERIFY(!EOPUpdater::horizonsToOverlayCSV(answer, csv, &err, &n, 10));      // 9 days with a value only
	QVERIFY(err.contains("only 9 days"));
	QVERIFY(!EOPUpdater::horizonsToOverlayCSV(QString(answer).replace(" 2027-Jan-29 00:00:00.000, 2461434.500000000, , , -0.13971,",
	                                                                  " 2027-Jan-29 something went wrong"), csv, &err, &n, 3));
	QVERIFY(err.contains("row not understood"));
	QVERIFY(csv.isEmpty());
}

void TestEOPReader::testBadInput()
{
	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QString err;
	QVERIFY(!EOPReader::parseText("", recs, fmt, err));
	QVERIFY(!EOPReader::parseText("just some text\n1 2 3\n", recs, fmt, err));
	// header only
	QVERIFY(!EOPReader::parseText(c04Header(), recs, fmt, err));
	// bad MJD
	QVERIFY(!EOPReader::parseText(c04Header() + "abc;1973;01;02" + QString(23, ';') + "\r\n", recs, fmt, err));
	// a file whose rows have no value at all
	QVERIFY(!EOPReader::parseText(finalsHeader() + finalsRow(41686, "", "", "", "", "", "", "", "", ""), recs, fmt, err));

	EOPReader empty;
	EOPReader::Interpolated v;
	QVERIFY(!empty.interpolate(2460000.5, v));
	QVERIFY(empty.recordsAround(60000., 1, 1).isEmpty());
}

void TestEOPReader::testDropNutationAndLast()
{
	// files that give dPsi, dEps relative to IAU 2000A: they are removed, the rest of the records stays
	QVector<EOPReader::Record> recs;
	EOPReader::Format fmt;
	QString err;
	QVERIFY(EOPReader::parseText(c04File(60000, 60002), recs, fmt, err));
	QVERIFY(recs[1].hasNutation);
	EOPReader::dropNutation(recs);
	QVERIFY(!recs[1].hasNutation);
	QVERIFY(close(recs[1].dpsi, 0.));
	QVERIFY(recs[1].hasPole && recs[1].hasUT && recs[1].hasLOD);

	// last record of a group
	EOPReader r;
	QVERIFY(mergeText(r, c04File(60000, 60004)));
	QVERIFY(!r.hasLast(EOPReader::Group::DXDY));
	QVERIFY(r.hasLast(EOPReader::Group::Nutation));
	QVERIFY(mergeText(r, c04File2000(60003, 60006)));
	QVERIFY(r.hasLast(EOPReader::Group::DXDY));
	QVERIFY(close(r.lastRecord(EOPReader::Group::DXDY).mjd, 60006.));
	QVERIFY(close(r.lastRecord(EOPReader::Group::DXDY).dx, 0.0006));
	QVERIFY(close(r.lastRecord(EOPReader::Group::Nutation).mjd, 60004.));
}

void TestEOPReader::testRecordsBetween()
{
	const EOPReader r = loadC04();   // 60000..60004
	QCOMPARE(int(r.recordsBetween(60001., 60003.).size()), 3);
	QVERIFY(close(r.recordsBetween(60001., 60003.).first().mjd, 60001.));
	QVERIFY(close(r.recordsBetween(60001., 60003.).last().mjd, 60003.));
	QCOMPARE(int(r.recordsBetween(60001., 60003., 2).size()), 2);       // limit
	QCOMPARE(int(r.recordsBetween(50000., 70000.).size()), 5);          // whole table
	QCOMPARE(int(r.recordsBetween(70000., 70010.).size()), 0);
	QCOMPARE(int(r.recordsBetween(59990., 60000.).size()), 1);
}
