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
	// UT1-UTC drops by about 1 s when a leap second is inserted
	EOPReader r;
	QString s = c04Header();
	s += c04Row(57753, 0.1, 0.30, 0.001, "0.0", "0.0");
	s += c04Row(57754, 0.1, -0.69, 0.001, "0.0", "0.0");
	QVERIFY(mergeText(r, s));
	EOPReader::Interpolated v;
	QVERIFY(r.interpolate(57753.5 + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.305));

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
	QCOMPARE(int(EOPUpdater::defaultSources().size()), 4);
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
