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

#include <QFile>
#include <QTemporaryDir>
#include <QThread>
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

	// (SS) 2026-11-05 the default selection is the set of the preferred model (IAU 1980, as JPL Horizons): the long term file reaching the
	// latest date, then the newest standard download. The IAU 2000A files are only used when selected (see testDefaultSelectionByModel())
	const QStringList expected = QStringList() << C14 << F_NEW;
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
	// (SS) 2026-11-05 These checks are about the merge order of the files, with the previous rule where the standard files replace the long
	// term values (rule A, the default, is tested in testFinalsDoNotReplaceLongTerm())
	mgr.setFinalsOverrideLongTerm(true);
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

	// (SS) 2026-11-05 dX and dY come from the IAU 2000A files, no longer in the default selection: the four files are selected here
	mgr.setSelectedFiles(QStringList() << C20_2000 << C14 << F_2000 << F_NEW);
	QVERIFY(mgr.reload());
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
	// (SS) 2026-11-05 These checks are about the merge order of the files, with the previous rule where the standard files replace the long
	// term values (rule A, the default, is tested in testFinalsDoNotReplaceLongTerm())
	mgr.setFinalsOverrideLongTerm(true);

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
	// (SS) 2026-11-05 default selection of the IAU 2000A model: its files only
	QCOMPARE(mgr.selectedFiles(), QStringList() << C20_2000 << F_2000);
	// the four files selected explicitly: the IAU 2000A files are merged last
	mgr.setSelectedFiles(QStringList() << C14 << C20_2000 << F_NEW << F_2000);
	QCOMPARE(mgr.mergeOrder(), QStringList() << C14 << C20_2000 << F_NEW << F_2000);
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

	// order of the merge: long term files first, IAU 1980 files last in each group (the four files are selected, the default selection
	// has only those of one model since 2026-11-05)
	mgr.setSelectedFiles(QStringList() << C20_2000 << C14 << F_2000 << F_NEW);
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

// (SS) 2026-11-05 Rule A: the final values of the long term (C04) files are not replaced by the standard (finals) files, group by group;
// the finals files give the days after the last final value of the long term files, and their predictions. Option: finals replace C04.
void TestEOPManager::testFinalsDoNotReplaceLongTerm()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	QVERIFY(writeFile(dir.path() + "/" + C14, c04File(60000, 60004, 0.001)));
	QString s = finalsHeader();
	s += finalsFinalRow(60003, "0.153", "0.253", "0.0553", "-153.0", "-9.0");     // overlap with the C04 file
	s += finalsFinalRow(60004, "0.154", "0.254", "0.0554", "-154.0", "-9.0");     // overlap with the C04 file
	s += finalsFinalRow(60005, "0.155", "0.255", "0.0555", "-155.0", "-9.0");     // after the C04 file
	s += finalsPredictionRow(60006, "0.156", "0.256", "0.0556");
	QVERIFY(writeFile(dir.path() + "/finals.all_261001.csv", s));

	// reference: what the long term file alone gives at 60003 and 60004
	EOPManager ref(dir.path());
	ref.rescan();
	ref.setSelectedFiles(QStringList() << C14);
	QVERIFY(ref.reload());
	EOPReader::Interpolated c[2];
	QVERIFY(ref.getEOP(60003. + JD_OFFSET, c[0]));
	QVERIFY(ref.getEOP(60004. + JD_OFFSET, c[1]));

	EOPManager mgr(dir.path());
	mgr.rescan();
	QVERIFY(!mgr.finalsOverrideLongTerm());          // default: rule A
	QVERIFY(mgr.reload());
	EOPReader::Interpolated v;
	for (int k = 0; k < 2; ++k)                      // days of the overlap: the values of the long term file, for every group
	{
		QVERIFY(mgr.getEOP(60003. + k + JD_OFFSET, v));
		QVERIFY2(close(v.rec.xp, c[k].rec.xp) && close(v.rec.yp, c[k].rec.yp),
		         qPrintable(QString("MJD %1: xp = %2 (long term file: %3), yp = %4 (%5), the finals values replaced the long term ones if they are 0.15x\n%6")
		                    .arg(60003 + k).arg(v.rec.xp, 0, 'f', 6).arg(c[k].rec.xp, 0, 'f', 6).arg(v.rec.yp, 0, 'f', 6).arg(c[k].rec.yp, 0, 'f', 6)
		                    .arg(mgr.lastReport())));
		QVERIFY(close(v.rec.ut1utc, c[k].rec.ut1utc));
		QVERIFY(close(v.rec.lod, c[k].rec.lod));
		QVERIFY(close(v.rec.dpsi, c[k].rec.dpsi) && close(v.rec.deps, c[k].rec.deps));
		QCOMPARE(v.rec.poleStatus, EOPReader::Status::Final);
	}
	QVERIFY(mgr.lastReport().contains("Values not used"));
	// after the long term file: the finals values, then the prediction
	QVERIFY(mgr.getEOP(60005. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, 0.155) && close(v.rec.yp, 0.255) && close(v.rec.ut1utc, 0.0555));
	QVERIFY(close(v.rec.dpsi, -0.155) && close(v.rec.deps, -0.009));
	QVERIFY(mgr.getEOP(60006. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, 0.156));
	QCOMPARE(v.rec.utStatus, EOPReader::Status::Prediction);
	QCOMPARE(int(mgr.reader().size()), 7);            // 60000..60006

	// option on: the finals values replace the long term ones where they have a value
	mgr.setFinalsOverrideLongTerm(true);
	QVERIFY(mgr.finalsOverrideLongTerm());
	QVERIFY(mgr.reload());
	QVERIFY(!mgr.lastReport().contains("Values not used"));
	QVERIFY(mgr.getEOP(60003. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, 0.153) && close(v.rec.ut1utc, 0.0553));
	QVERIFY(close(v.rec.dpsi, -0.153));
	QVERIFY(mgr.getEOP(60004. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, 0.154) && close(v.rec.dpsi, -0.154));
	// days before the finals file are still the long term ones
	EOPReader::Interpolated c2;
	QVERIFY(ref.getEOP(60002. + JD_OFFSET, c2));
	QVERIFY(mgr.getEOP(60002. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, c2.rec.xp) && close(v.rec.ut1utc, c2.rec.ut1utc));

	// back to the default
	mgr.setFinalsOverrideLongTerm(false);
	QVERIFY(mgr.reload());
	QVERIFY(mgr.getEOP(60003. + JD_OFFSET, v));
	QVERIFY(close(v.rec.xp, c[0].rec.xp));
}

// (SS) 2026-11-05 The default selection has the files of one nutation model: the preferred one, IAU 1980 as JPL Horizons.
void TestEOPManager::testDefaultSelectionByModel()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	makeFiles(dir.path());
	EOPManager mgr(dir.path());
	mgr.rescan();

	QCOMPARE(mgr.preferredModel(), EOPManager::Model::IAU1980);
	QCOMPARE(mgr.defaultSelection(), QStringList() << C14 << F_NEW);
	mgr.setPreferredModel(EOPManager::Model::IAU2000A);
	QCOMPARE(mgr.defaultSelection(), QStringList() << C20_2000 << F_2000);
	mgr.setPreferredModel(EOPManager::Model::IAU1980);

	// no IAU 1980 file in the folder: the IAU 2000A files are used, so that some EOP are available
	QTemporaryDir dir2000;
	QVERIFY(dir2000.isValid());
	QVERIFY(writeFile(dir2000.path() + "/" + C20_2000, c04File2000(60000, 60006)));
	QString s = finalsHeader();
	s += finalsRow(60005, "final", "0.999", "0.210", "final", "0.0510", "1.5", "final", "", "", "0.7", "0.1");
	QVERIFY(writeFile(dir2000.path() + "/" + F_2000, s));
	EOPManager only2000(dir2000.path());
	only2000.rescan();
	QCOMPARE(only2000.defaultSelection(), QStringList() << C20_2000 << F_2000);
	QVERIFY(only2000.reload());
	QVERIFY(!only2000.isEmpty());

	// an empty folder: nothing
	QTemporaryDir empty;
	EOPManager none(empty.path());
	none.rescan();
	QVERIFY(none.defaultSelection().isEmpty());
}

// (SS) 2026-11-06 UT1-UTC of JPL Horizons (horizons_ut1utc_YYMMDD.csv): merged last, replaces UT1-UTC only, the UT1-UTC values after its last
// day are removed (the last value is held); part of the default selection of the IAU 1980 model, not of the IAU 2000A model.
void TestEOPManager::testHorizonsOverlay()
{
	const QString OV = QStringLiteral("horizons_ut1utc_261106.csv");
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	makeFiles(dir.path());
	QVERIFY(writeFile(dir.path() + "/" + OV,
	                  "# JPL Horizons UT1-UTC (quantity 49), target Sun, geocentric\n"
	                  "# EOP coverage: DATA-BASED 1962-JAN-20 TO 2023-MAR-01. PREDICTS-> 2023-MAR-03\n"
	                  "MJD;Date;JD;UT1-UTC\n"
	                  "60000;2023-Feb-25;2460000.5;0.10000\n"
	                  "60001;2023-Feb-26;2460001.5;0.10100\n"
	                  "60002;2023-Feb-27;2460002.5;0.10200\n"
	                  "60003;2023-Feb-28;2460003.5;0.10300\n"
	                  "60004;2023-Mar-01;2460004.5;0.10400\n"
	                  "60005;2023-Mar-02;2460005.5;0.10500\n"
	                  "60006;2023-Mar-03;2460006.5;0.10600\n"));

	EOPManager mgr(dir.path());
	QCOMPARE(int(mgr.rescan().size()), 7);          // the 6 IERS files and the overlay
	const EOPManager::FileInfo fi = mgr.fileInfo(OV);
	QVERIFY(fi.valid && fi.isOverlay() && !fi.isLongTerm());
	QCOMPARE(fi.format, EOPReader::Format::HorizonsUT);
	QCOMPARE(fi.key, QString("horizons_ut1utc"));
	QCOMPARE(fi.generated, QDate(2026, 11, 6));
	QCOMPARE(fi.nRecords, 7);
	QVERIFY(close(fi.firstMJD, 60000.) && close(fi.lastMJD, 60006.));

	// IAU 1980 model: the overlay is added to the default files and merged last; not for the IAU 2000A model
	QCOMPARE(mgr.defaultSelection(), QStringList() << C14 << F_NEW << OV);
	QCOMPARE(mgr.mergeOrder(), QStringList() << C14 << F_NEW << OV);
	mgr.setPreferredModel(EOPManager::Model::IAU2000A);
	QCOMPARE(mgr.defaultSelection(), QStringList() << C20_2000 << F_2000);
	// but it can be selected, and is still merged last
	mgr.setSelectedFiles(QStringList() << OV << C20_2000 << F_2000);
	QCOMPARE(mgr.mergeOrder().last(), OV);
	mgr.setSelectedFiles(QStringList());
	mgr.setPreferredModel(EOPManager::Model::IAU1980);

	// reference without the overlay
	EOPManager ref(dir.path());
	ref.rescan();
	ref.setSelectedFiles(QStringList() << C14 << F_NEW);
	QVERIFY(ref.reload());
	EOPReader::Interpolated r2, r7, v;
	QVERIFY(ref.getEOP(60002. + JD_OFFSET, r2));
	QVERIFY(ref.getEOP(60007. + JD_OFFSET, r7));
	QVERIFY(!close(r2.rec.ut1utc, 0.102));
	QVERIFY(close(r7.rec.ut1utc, 0.0520));

	QVERIFY(mgr.reload());
	// (SS) 2026-11-07 the last day of the Horizons UT1-UTC is where the constant EOP regime (TIME ZONE 4) starts
	QVERIFY(mgr.hasHorizonsUT());
	QVERIFY(close(mgr.horizonsUTLastMJD(), 60006.));
	// UT1-UTC replaced where the overlay has a value (also where C04 has a final value); the other groups are not touched
	QVERIFY(mgr.getEOP(60002. + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.102));
	QVERIFY(close(v.rec.xp, r2.rec.xp) && close(v.rec.yp, r2.rec.yp) && close(v.rec.lod, r2.rec.lod) && close(v.rec.dpsi, r2.rec.dpsi));
	QVERIFY(mgr.getEOP(60005. + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.105));
	QCOMPARE(v.rec.utStatus, EOPReader::Status::Prediction);
	// after its last day the last value is held, the UT1-UTC of finals.all (0.0520) is not used; its pole is
	QVERIFY(mgr.getEOP(60007. + JD_OFFSET, v));
	QVERIFY(v.utHeld && close(v.rec.ut1utc, 0.106));
	QVERIFY(close(v.rec.xp, r7.rec.xp));
	QVERIFY(mgr.getEOP(60100. + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, 0.106));

	const EOPReader::Span sp = mgr.reader().span(EOPReader::Group::UT);
	QVERIFY(close(sp.lastFinalMJD, 60004.) && close(sp.lastMJD, 60006.));
	const QString rep = mgr.lastReport();
	QVERIFY(rep.contains("Using EOP file: " + OV));
	QVERIFY(rep.contains("UT1-UTC only"));
	QVERIFY(rep.contains("from 2023-02-25 to 2023-03-03"));
	QVERIFY(rep.contains("1 UT1-UTC value(s) of other files after 2023-03-03 removed"));
	QVERIFY(rep.contains("0.10600 s"));

	// without the overlay in the selection nothing changes
	mgr.setSelectedFiles(QStringList() << C14 << F_NEW);
	QVERIFY(mgr.reload());
	QVERIFY(!mgr.hasHorizonsUT());
	QVERIFY(mgr.getEOP(60002. + JD_OFFSET, v));
	QVERIFY(close(v.rec.ut1utc, r2.rec.ut1utc));
	QVERIFY(mgr.getEOP(60007. + JD_OFFSET, v));
	QVERIFY(!v.utHeld && close(v.rec.ut1utc, 0.0520));
}

// (SS) 2026-11-07 After a download the older versions of the files just written are removed (the newest @p keep are kept).
void TestEOPManager::testRemoveOldVersions()
{
	// (SS) 2026-11-08 On Windows a file that has just been created can be locked for a moment (virus scanner): write with a few retries and,
	// if it still fails, say why. Returns an empty text when the file is written.
	auto put = [](const QString& path) -> QString
	{
		QString why;
		for (int attempt = 0; attempt < 10; ++attempt)
		{
			QFile f(path);
			if (f.open(QIODevice::WriteOnly) && f.write("x\n") == 2 && f.flush())
			{
				f.close();
				return QString();
			}
			why = f.errorString();
			QThread::msleep(100);
		}
		return why;
	};
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString p = dir.path() + "/";
	const QStringList all = QStringList()
	    << "finals.all_261001.csv" << "finals.all_261020.csv" << "finals.all_261105.csv" << "finals.all_261106.csv"
	    << "horizons_ut1utc_261105.csv" << "horizons_ut1utc_261106.csv"
	    << "eopc04_14.62-now_260126.csv" << "eopc04_14.62-now_260301.csv"       // another series, not downloaded this time
	    << "finals2000A.all_261001.csv" << "finals2000A.all_261002.csv"          // another series, not downloaded this time
	    << "my_finals.all_261104_mod.csv" << "my_eopc04_14.62-now_261104.csv"   // made by hand
	    << "eopc04_14.62-now.csv" << "notes.txt";                                // no date in the name, not a csv file
	for (const QString& n : all)
		{ const QString why = put(p + n); QVERIFY2(why.isEmpty(), qPrintable(p + n + ": " + why)); }

	EOPManager mgr(dir.path());
	const QStringList written = QStringList() << "finals.all_261106.csv" << "horizons_ut1utc_261106.csv";
	QCOMPARE(mgr.removeOldVersions(written, 0), QStringList());               // 0: keep everything
	// the two newest versions of the series just written are kept
	QCOMPARE(mgr.removeOldVersions(written, 2), QStringList() << "finals.all_261001.csv" << "finals.all_261020.csv");
	for (const QString& n : all)
		QCOMPARE(QFile::exists(p + n), n != "finals.all_261001.csv" && n != "finals.all_261020.csv");
	// only the new one
	QCOMPARE(mgr.removeOldVersions(written, 1), QStringList() << "finals.all_261105.csv" << "horizons_ut1utc_261105.csv");
	QVERIFY(QFile::exists(p + "finals.all_261106.csv") && QFile::exists(p + "horizons_ut1utc_261106.csv"));
	// nothing of the other series, of the files made by hand and of the files without a date was touched
	// (SS) 2026-11-08 the list is a named variable: a range-based for over "QStringList() << a << b" uses a temporary that is already destroyed
	// when the loop starts (operator<< returns a reference to it), which showed up as empty names in Debug builds only
	const QStringList untouched = QStringList() << "eopc04_14.62-now_260126.csv" << "eopc04_14.62-now_260301.csv" << "finals2000A.all_261001.csv"
	                                            << "finals2000A.all_261002.csv" << "my_finals.all_261104_mod.csv" << "my_eopc04_14.62-now_261104.csv"
	                                            << "eopc04_14.62-now.csv" << "notes.txt";
	for (const QString& n : untouched)
		QVERIFY(QFile::exists(p + n));
	QCOMPARE(mgr.removeOldVersions(written, 1), QStringList());               // nothing more to remove

	// a file of an explicit selection is never removed
	QTemporaryDir dir2;
	QVERIFY(dir2.isValid());
	const QString p2 = dir2.path() + "/";
	const QStringList files2 = QStringList() << "finals.all_261001.csv" << "finals.all_261105.csv" << "finals.all_261106.csv";
	for (const QString& n : files2)
		{ const QString why = put(p2 + n); QVERIFY2(why.isEmpty(), qPrintable(p2 + n + ": " + why)); }
	EOPManager mgr2(dir2.path());
	mgr2.setSelectedFiles(QStringList() << "finals.all_261001.csv");
	QCOMPARE(mgr2.removeOldVersions(QStringList() << "finals.all_261106.csv", 1), QStringList() << "finals.all_261105.csv");
	QVERIFY(QFile::exists(p2 + "finals.all_261001.csv"));
	// a file just written is never removed, even when other files have a later date
	QTemporaryDir dir3;
	const QStringList files3 = QStringList() << "finals.all_261106.csv" << "finals.all_261107.csv" << "finals.all_261108.csv";
	for (const QString& n : files3)
		{ const QString why = put(dir3.path() + "/" + n); QVERIFY2(why.isEmpty(), qPrintable(dir3.path() + "/" + n + ": " + why)); }
	EOPManager mgr3(dir3.path());
	QCOMPARE(mgr3.removeOldVersions(QStringList() << "finals.all_261106.csv", 1), QStringList() << "finals.all_261107.csv");
	QVERIFY(QFile::exists(dir3.path() + "/finals.all_261106.csv"));
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
	// (SS) 2026-11-05 These checks are about the merge order of the files, with the previous rule where the standard files replace the long
	// term values (rule A, the default, is tested in testFinalsDoNotReplaceLongTerm())
	mgr.setFinalsOverrideLongTerm(true);
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
