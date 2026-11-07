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

#include "testEOPManager.hpp"
#include "EOPTestData.hpp"
#include "EOPManager.hpp"

#include <QTemporaryDir>
#include <cmath>

QTEST_GUILESS_MAIN(TestEOPManager)

using namespace EOPTestData;

namespace
{
const double JD_OFFSET = 2400000.5;
const QString C14       = QStringLiteral("eopc04_14.62-now_260126.csv");                  // C04 ITRF2014, IAU 1980 dPsi, dEps, 60000..60004
const QString C20_DPSI  = QStringLiteral("eopc04_20u24.dPsi_dEps.1962-now_261001.csv");   // C04 ITRF2020, dPsi, dEps relative to IAU 2000A, 60000..60006
const QString C20_2000  = QStringLiteral("eopc04_20u24.1962-now_261001.csv");             // C04 ITRF2020, dX, dY, 60000..60006
const QString F_OLD     = QStringLiteral("finals.all_260918.csv");                        // standard IAU 1980: 60005, 60006
const QString F_NEW     = QStringLiteral("finals.all_261001.csv");                        // standard IAU 1980: 60005..60007
const QString F_2000    = QStringLiteral("finals2000A.all_261001.csv");                   // standard IAU 2000: 60005..60007, pole x differs

bool close(double a, double b, double tol = 1e-12) { return std::fabs(a - b) <= tol; }

void makeFiles(const QString& dir)
{
	QVERIFY(writeFile(dir + "/" + C14, c04File(60000, 60004, 0.001)));
	QVERIFY(writeFile(dir + "/" + C20_DPSI, c04File(60000, 60006)));     // its dPsi must not be used
	QVERIFY(writeFile(dir + "/" + C20_2000, c04File2000(60000, 60006)));

	QString s = finalsHeader();
	s += finalsFinalRow(60005, "0.110", "0.210", "0.0510", "-100.0", "-7.0");
	s += finalsFinalRow(60006, "0.111", "0.211", "0.0515", "-100.0", "-7.0");
	QVERIFY(writeFile(dir + "/" + F_OLD, s));

	s = finalsHeader();
	s += finalsFinalRow(60005, "0.110", "0.210", "0.0510", "-110.0", "-7.0");
	s += finalsFinalRow(60006, "0.111", "0.211", "0.0515", "-110.0", "-7.0");
	s += finalsFinalRow(60007, "0.112", "0.212", "0.0520", "-110.0", "-7.0");
	QVERIFY(writeFile(dir + "/" + F_NEW, s));

	s = finalsHeader();
	s += finalsRow(60005, "final", "0.999", "0.210", "final", "0.0510", "1.5", "final", "", "", "0.7", "0.1");
	s += finalsRow(60006, "final", "0.999", "0.211", "final", "0.0515", "1.5", "final", "", "", "0.8", "0.1");
	s += finalsRow(60007, "final", "0.999", "0.212", "final", "0.0520", "1.5", "final", "", "", "0.9", "0.1");
	QVERIFY(writeFile(dir + "/" + F_2000, s));

	QVERIFY(writeFile(dir + "/bad_261001.csv", "this is not an EOP file\n"));
	// the old .txt layout is not looked at
	QVERIFY(writeFile(dir + "/EOP 14 C04 (IAU1980, dPsi, dEps) 0hUTC - one file (1962-now).txt", c04File(60000, 60004)));
}
}

void TestEOPManager::testNames()
{
	QCOMPARE(EOPManager::generationDateFromName("finals.all_261007.csv"), QDate(2026, 10, 7));
	QCOMPARE(EOPManager::generationDateFromName("eopc04_20u24.dPsi_dEps.1962-now_261001.csv"), QDate(2026, 10, 1));
	QVERIFY(!EOPManager::generationDateFromName("eopc04_14.62-now.csv").isValid());
	QVERIFY(!EOPManager::generationDateFromName("eopc04_20u24.1962-now.csv").isValid());
	QCOMPARE(EOPManager::keyFromName("finals.all_261007.csv"), QString("finals.all"));
	QCOMPARE(EOPManager::keyFromName("eopc04_20u24.1962-now_261007.csv"), QString("eopc04_20u24.1962-now"));
	QCOMPARE(EOPManager::keyFromName("eopc04_14.62-now.csv"), QString("eopc04_14.62-now"));

	// reference model from the name
	QCOMPARE(EOPManager::modelFromName("finals.all_261007.csv"), EOPManager::Model::IAU1980);
	QCOMPARE(EOPManager::modelFromName("finals2000A.all_261007.csv"), EOPManager::Model::IAU2000A);
	QCOMPARE(EOPManager::modelFromName("eopc04_14.62-now_261007.csv"), EOPManager::Model::IAU1980);
	QCOMPARE(EOPManager::modelFromName("eopc04_14_IAU2000.62-now_261007.csv"), EOPManager::Model::IAU2000A);
	QCOMPARE(EOPManager::modelFromName("eopc04_20u24.1962-now_261007.csv"), EOPManager::Model::IAU2000A);
	QCOMPARE(EOPManager::modelFromName("eopc04_20u24.dPsi_dEps.1962-now_261007.csv"), EOPManager::Model::IAU2000A);
	QCOMPARE(EOPManager::modelFromName("eopc04_20u24.dPsi_dEps.IAU1980.1962-now_261007.csv"), EOPManager::Model::IAU1980);
	QCOMPARE(EOPManager::modelFromName("mydata.csv"), EOPManager::Model::Unknown);
}

void TestEOPManager::testScanAndDefaultSelection()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	makeFiles(dir.path());

	EOPManager mgr(dir.path());
	const QStringList valid = mgr.rescan();
	QCOMPARE(int(valid.size()), 6);
	QCOMPARE(int(mgr.files().size()), 7);           // + the invalid csv, the .txt file is ignored
	QVERIFY(!mgr.fileInfo("bad_261001.csv").valid);
	QVERIFY(!mgr.fileInfo("bad_261001.csv").error.isEmpty());

	const EOPManager::FileInfo fi = mgr.fileInfo(F_NEW);
	QVERIFY(fi.valid);
	QCOMPARE(fi.format, EOPReader::Format::Finals);
	QCOMPARE(fi.model, EOPManager::Model::IAU1980);
	QVERIFY(!fi.isLongTerm());
	QCOMPARE(fi.generated, QDate(2026, 10, 1));
	QCOMPARE(fi.key, QString("finals.all"));
	QCOMPARE(fi.nRecords, 3);
	QVERIFY(close(fi.firstMJD, 60005.));
	QVERIFY(close(fi.lastMJD, 60007.));

	QVERIFY(mgr.fileInfo(C14).isLongTerm());
	QVERIFY(mgr.fileInfo(C14).hasNutation && !mgr.fileInfo(C14).hasDXDY);
	QVERIFY(mgr.fileInfo(C20_2000).hasDXDY && !mgr.fileInfo(C20_2000).hasNutation);
	// dPsi and dEps relative to IAU 2000A are not kept
	QVERIFY(!mgr.fileInfo(C20_DPSI).hasNutation && !mgr.fileInfo(C20_DPSI).hasDXDY);

	// long term file of each model, then the newest standard download of each series;
	// the files of the preferred model (IAU 1980) are merged last in each group
	const QStringList expected = QStringList() << C20_2000 << C14 << F_2000 << F_NEW;
	QCOMPARE(mgr.defaultSelection(), expected);
	QCOMPARE(mgr.selectedFiles(), expected);
	QVERIFY(mgr.isUsingDefaultSelection());
}

void TestEOPManager::testReloadNewestWins()
{
	QTemporaryDir dir;
	makeFiles(dir.path());
	EOPManager mgr(dir.path());
	mgr.rescan();
	QVERIFY(mgr.reload());
	QVERIFY(mgr.lastReport().contains("Using EOP file"));
	QVERIFY(mgr.lastReport().contains("Last valid value of each EOP"));

	const EOPReader& r = mgr.reader();
	QCOMPARE(r.size(), 8);                 // 60000..60007
	QVERIFY(close(r.firstMJD(), 60000.));
	QVERIFY(close(r.lastMJD(), 60007.));
	QCOMPARE(r.gapCount(), 0);

	double dpsi = 0., deps = 0.;
	QVERIFY(mgr.getNutationCorrections(60005. + JD_OFFSET, dpsi, deps));
	QVERIFY(close(dpsi, -0.110));          // IAU 1980 standard series replaces the long term one, the newest download wins
	QVERIFY(close(deps, -0.007));
	QVERIFY(mgr.getNutationCorrections(60003. + JD_OFFSET, dpsi, deps));
	QVERIFY(close(dpsi, 0.03 + 0.001));    // EOP 14 C04 (IAU 1980); not the 0.03 of the 20u24 dPsi_dEps file

	// dX and dY of the IAU 2000A files; no IAU 1980 file takes them away
	EOPReader::Interpolated v;
	QVERIFY(mgr.getEOP(60003. + JD_OFFSET, v));
	QVERIFY(v.rec.hasDXDY && !v.dxdyHeld);
	QVERIFY(close(v.rec.dx, 0.0003));
	QVERIFY(mgr.getEOP(60005. + JD_OFFSET, v));
	QVERIFY(close(v.rec.dx, 0.0007));      // finals2000A: marcsec -> arcsec
	// pole: the IAU 1980 files are merged last and win over finals2000A
	QVERIFY(close(v.rec.xp, 0.110));
}

void TestEOPManager::testExplicitSelection()
{
	QTemporaryDir dir;
	makeFiles(dir.path());
	EOPManager mgr(dir.path());
	mgr.rescan();

	// EOP 14 C04 and the older standard download; the unknown name is ignored; the names may come in any order
	mgr.setSelectedFiles(QStringList() << F_OLD << "nope.csv" << C14);
	QVERIFY(!mgr.isUsingDefaultSelection());
	QCOMPARE(int(mgr.selectedFiles().size()), 2);
	QVERIFY(mgr.reload());
	QVERIFY(close(mgr.reader().firstMJD(), 60000.));
	QVERIFY(close(mgr.reader().lastMJD(), 60006.));
	double dpsi = 0., deps = 0.;
	QVERIFY(mgr.getNutationCorrections(60005. + JD_OFFSET, dpsi, deps));
	QVERIFY(close(dpsi, -0.100));
	EOPReader::Interpolated v;
	QVERIFY(mgr.getEOP(60002. + JD_OFFSET, v));
	QVERIFY(!v.rec.hasDXDY);
	QVERIFY(close(v.rec.dpsi, 0.02 + 0.001));   // C14 has an offset of 0.001

	// an empty list means the default selection again
	mgr.setSelectedFiles(QStringList());
	QVERIFY(mgr.isUsingDefaultSelection());
	QVERIFY(mgr.reload());
	QVERIFY(close(mgr.reader().lastMJD(), 60007.));

	// a file removed from the folder disappears from an explicit selection after rescan()
	mgr.setSelectedFiles(QStringList() << C14 << F_OLD);
	QVERIFY(QFile::remove(dir.path() + "/" + F_OLD));
	mgr.rescan();
	QCOMPARE(mgr.selectedFiles(), QStringList() << C14);
}

void TestEOPManager::testModels()
{
	QTemporaryDir dir;
	makeFiles(dir.path());
	EOPManager mgr(dir.path());
	mgr.rescan();

	// the dPsi_dEps file of the 20u24 series alone: no IAU 1980 nutation at all
	mgr.setSelectedFiles(QStringList() << C20_DPSI);
	QVERIFY(mgr.reload());
	double dpsi = 0., deps = 0.;
	QVERIFY(!mgr.getNutationCorrections(60003. + JD_OFFSET, dpsi, deps));
	EOPReader::Interpolated v;
	QVERIFY(mgr.getEOP(60003. + JD_OFFSET, v));
	QVERIFY(v.rec.hasPole && v.rec.hasUT);          // pole and UT1-UTC are still there

	// preferred model IAU 2000A: finals2000A is merged after finals.all, its pole wins
	mgr.setSelectedFiles(QStringList());
	QCOMPARE(mgr.preferredModel(), EOPManager::Model::IAU1980);
	mgr.setPreferredModel(EOPManager::Model::IAU2000A);
	const QStringList expected = QStringList() << C14 << C20_2000 << F_NEW << F_2000;
	QCOMPARE(mgr.selectedFiles(), expected);
	QVERIFY(mgr.reload());
	QVERIFY(mgr.getEOP(60005. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, 0.999));
	QVERIFY(close(v.rec.dpsi, -0.110));             // dPsi still comes from the IAU 1980 files
}

void TestEOPManager::testKeysAndOrder()
{
	QCOMPARE(EOPManager::modelKey(EOPManager::Model::IAU1980), QString("IAU1980"));
	QCOMPARE(EOPManager::modelKey(EOPManager::Model::IAU2000A), QString("IAU2000A"));
	QCOMPARE(EOPManager::modelFromKey("IAU1980"), EOPManager::Model::IAU1980);
	QCOMPARE(EOPManager::modelFromKey("iau2000a"), EOPManager::Model::IAU2000A);
	QCOMPARE(EOPManager::modelFromKey("nonsense"), EOPManager::Model::Unknown);

	QTemporaryDir dir;
	makeFiles(dir.path());
	EOPManager mgr(dir.path());
	mgr.rescan();
	QVERIFY(mgr.reload());

	// order of the merge: long term files first, IAU 1980 files last in each group
	QCOMPARE(mgr.mergeOrder(), QStringList() << C20_2000 << C14 << F_2000 << F_NEW);
	mgr.setPreferredModel(EOPManager::Model::IAU2000A);
	QCOMPARE(mgr.mergeOrder(), QStringList() << C14 << C20_2000 << F_NEW << F_2000);
	// explicit selection
	mgr.setSelectedFiles(QStringList() << F_NEW << C14);
	QCOMPARE(mgr.mergeOrder(), QStringList() << C14 << F_NEW);

	// records for the GUI
	mgr.setSelectedFiles(QStringList());
	mgr.setPreferredModel(EOPManager::Model::IAU1980);
	QVERIFY(mgr.reload());
	QCOMPARE(int(mgr.records(60001., 60003.).size()), 3);
	QVERIFY(!mgr.coverageText().isEmpty());
	QVERIFY(mgr.lastValuesText().contains("Last valid value"));
}

void TestEOPManager::testUpdateAge()
{
	QTemporaryDir dir;
	makeFiles(dir.path());
	EOPManager mgr(dir.path());
	mgr.rescan();

	// newest download of finals.all is 2026-10-01
	QVERIFY(!mgr.isOlderThan("finals.all", QDate(2026, 10, 1), 7));
	QVERIFY(!mgr.isOlderThan("finals.all", QDate(2026, 10, 7), 7));
	QVERIFY(mgr.isOlderThan("finals.all", QDate(2026, 10, 8), 7));
	QVERIFY(!mgr.isOlderThan("finals.all", QDate(2026, 10, 8), 8));
	// unknown series, and a series whose only file is not valid: nothing usable, so update
	QVERIFY(mgr.isOlderThan("finals2000A.csv", QDate(2026, 10, 1), 7));
	QVERIFY(mgr.isOlderThan("bad", QDate(2026, 10, 1), 7));
	// no date in the name: the modification date of the file (just created) is used
	QVERIFY(writeFile(dir.path() + "/eopc04_14.62-now.csv", c04File(60000, 60002)));
	mgr.rescan();
	QVERIFY(!mgr.isOlderThan("eopc04_14.62-now", QDate::currentDate(), 7));
	QVERIFY(mgr.isOlderThan("eopc04_14.62-now", QDate::currentDate().addDays(30), 7));
}

void TestEOPManager::testLastValidReport()
{
	// each group has its own last valid record: here LOD and nutation end at 60005, pole and UT1-UTC at 60007
	QTemporaryDir dir;
	QVERIFY(writeFile(dir.path() + "/eopc04_14.62-now_260126.csv", c04File(60000, 60004)));
	QString s = finalsHeader();
	s += finalsFinalRow(60005, "0.110", "0.210", "0.0510", "-120.0", "-7.0");     // LOD 1.5 msec
	s += finalsPredictionRow(60006, "0.120", "0.220", "0.0520");
	s += finalsPredictionRow(60007, "0.130", "0.230", "0.0530");
	QVERIFY(writeFile(dir.path() + "/finals.all_261001.csv", s));

	EOPManager mgr(dir.path());
	mgr.rescan();
	QVERIFY(mgr.reload());

	const QStringList lines = mgr.lastReport().split('\n');
	auto find = [&lines](const QString& marker) -> QString
	{
		for (const QString& l : lines)
			if (l.contains(marker))
				return l;
		return QString();
	};

	const QString pole = find("xp = ");
	QVERIFY(pole.contains("MJD 60007") && pole.contains("prediction"));
	QVERIFY(pole.contains("0.130000") && pole.contains("0.230000"));
	const QString ut = find("UT1 - UTC = ");
	QVERIFY(ut.contains("MJD 60007") && ut.contains("0.0530000"));
	const QString lod = find("LOD = ");
	QVERIFY(lod.contains("MJD 60005") && lod.contains("final") && lod.contains("0.0015000"));
	const QString nut = find("dPsi = ");
	QVERIFY(nut.contains("MJD 60005") && nut.contains("-0.120000") && nut.contains("-0.007000"));
}

void TestEOPManager::testQueries()
{
	QTemporaryDir dir;
	makeFiles(dir.path());
	EOPManager mgr(dir.path());
	mgr.rescan();
	QVERIFY(mgr.reload());

	double dpsi = 0., deps = 0.;
	// before the first record: nothing
	QVERIFY(!mgr.getNutationCorrections(50000. + JD_OFFSET, dpsi, deps));
	// after the last record: last values kept
	QVERIFY(mgr.getNutationCorrections(70000. + JD_OFFSET, dpsi, deps));
	QVERIFY(close(dpsi, -0.110));

	EOPReader::Interpolated v;
	QVERIFY(mgr.getEOP(60005.5 + JD_OFFSET, v));
	QVERIFY(v.rec.hasPole && v.rec.hasUT);
	QVERIFY(close(v.rec.xp, 0.1105));
	QVERIFY(close(v.rec.yp, 0.2105));

	// table of daily records for the GUI: 1 before, the one at the date, 1 after
	const QVector<EOPReader::Record> t = mgr.table(60002.5 + JD_OFFSET, 1, 1);
	QCOMPARE(int(t.size()), 3);
	QVERIFY(close(t[0].mjd, 60001.));
	QVERIFY(close(t[1].mjd, 60002.));
	QVERIFY(close(t[2].mjd, 60003.));

	// manager without any file
	QTemporaryDir empty;
	EOPManager none(empty.path());
	QCOMPARE(int(none.rescan().size()), 0);
	QVERIFY(!none.reload());
	QVERIFY(none.isEmpty());
	QVERIFY(!none.getNutationCorrections(2460000.5, dpsi, deps));
}
