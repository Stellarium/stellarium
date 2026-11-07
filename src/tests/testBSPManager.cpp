/*
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
 * 
 * (SS) 2026-10-03
 * Unit tests for BSPManager
 *
 * Small synthetic little-endian SPK kernels (type 2) are written to a temporary folder at run time.
 * TT-TDB values are compared with a closed-form evaluation of the degree-3 Chebyshev series
 * (T0=1, T1=x, T2=2x^2-1, T3=4x^3-3x). Byte-order handling and multi-record summaries are covered by testBSPReader.
 */

#include <QObject>
#include <QtDebug>
#include <QtEndian>
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStringList>
#include <QVector>
#include <cmath>
#include <cstring>

#include "tests/testBSPManager.hpp"
#include "BSPManager.hpp"

QTEST_GUILESS_MAIN(TestBSPManager)

namespace
{
const int NC = 4;
const double JD_J2000 = 2451545.0;

struct SegDef
{
	int target, center;
	double jd0;
	int nRec;
	double dpr;
	double scale;
};

// Coefficient i of component k, record rec; 'seed' makes two TT-TDB kernels differ
double coef(double scale, double seed, int rec, int k, int i)
{
	return scale * seed * (1.0 + 0.25 * rec) * (k + 1) * ((i % 2) ? -1.0 : 1.0) / (i + 1.5);
}

void appendDouble(QByteArray& ba, double d)
{
	quint64 u;
	std::memcpy(&u, &d, 8);
	u = qToLittleEndian<quint64>(u);
	ba.append(reinterpret_cast<const char*>(&u), 8);
}

void appendInt32(QByteArray& ba, qint32 v)
{
	quint32 u = qToLittleEndian<quint32>(static_cast<quint32>(v));
	ba.append(reinterpret_cast<const char*>(&u), 4);
}

// Little-endian kernel with one summary record (record 2), name record (3), data from word 385 (record 4)
QByteArray buildKernel(const QVector<SegDef>& defs, double seed)
{
	QByteArray body;
	qint32 w = 3 * 128 + 1;
	struct Summ { SegDef d; qint32 first, last; };
	QVector<Summ> sums;
	for (const SegDef& d : defs)
	{
		const int recSize = 2 + 3 * NC;
		const qint32 first = w;
		for (int r = 0; r < d.nRec; ++r)
		{
			appendDouble(body, (d.jd0 + (r + 0.5) * d.dpr - JD_J2000) * 86400.0);
			appendDouble(body, d.dpr * 43200.0);
			for (int k = 0; k < 3; ++k)
				for (int i = 0; i < NC; ++i)
					appendDouble(body, coef(d.scale, seed, r, k, i));
			w += recSize;
		}
		appendDouble(body, (d.jd0 - JD_J2000) * 86400.0);
		appendDouble(body, d.dpr * 86400.0);
		appendDouble(body, recSize);
		appendDouble(body, d.nRec);
		w += 4;
		sums.append({d, first, w - 1});
	}

	QByteArray fileRec(1024, '\0');
	std::memcpy(fileRec.data(), "DAF/SPK ", 8);
	const qint32 hdr[5] = {2, 6, 2, 2, w}; // ND, NI at 8,12 ; FWARD, BWARD at 76,80 ; FREE at 84
	quint32 u;
	u = qToLittleEndian<quint32>(hdr[0]); std::memcpy(fileRec.data() + 8,  &u, 4);
	u = qToLittleEndian<quint32>(hdr[1]); std::memcpy(fileRec.data() + 12, &u, 4);
	u = qToLittleEndian<quint32>(hdr[2]); std::memcpy(fileRec.data() + 76, &u, 4);
	u = qToLittleEndian<quint32>(hdr[3]); std::memcpy(fileRec.data() + 80, &u, 4);
	u = qToLittleEndian<quint32>(hdr[4]); std::memcpy(fileRec.data() + 84, &u, 4);
	std::memcpy(fileRec.data() + 88, "LTL-IEEE", 8);

	QByteArray rec;
	appendDouble(rec, 0.0);
	appendDouble(rec, 0.0);
	appendDouble(rec, sums.size());
	for (const Summ& s : sums)
	{
		appendDouble(rec, (s.d.jd0 - JD_J2000) * 86400.0);
		appendDouble(rec, (s.d.jd0 + s.d.nRec * s.d.dpr - JD_J2000) * 86400.0);
		appendInt32(rec, s.d.target);
		appendInt32(rec, s.d.center);
		appendInt32(rec, 1);
		appendInt32(rec, 2);
		appendInt32(rec, s.first);
		appendInt32(rec, s.last);
	}
	rec.append(QByteArray(1024 - rec.size(), '\0'));

	return fileRec + rec + QByteArray(1024, '\0') + body;
}

bool writeFile(const QString& path, const QByteArray& data)
{
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly))
		return false;
	const bool ok = (f.write(data) == data.size());
	f.close();
	return ok;
}

const SegDef ttSeg   = {BSPManager::TTMTDB_TARGET, BSPManager::TTMTDB_CENTER, 2451545.0, 5, 32.0, 1.0e-3};
const SegDef earth   = {399, 3, 2451545.0, 6, 16.0, 1.0e7};
const SegDef moon    = {301, 3, 2451600.0, 8, 4.0, 2.0e5};

// Closed-form TT-TDB (seconds) and rate (s/s) at jd for a kernel written with the given seed
void expectedTT(double seed, double jd, double& secs, double& rate)
{
	int rec = static_cast<int>(std::floor((jd - ttSeg.jd0) / ttSeg.dpr));
	if (rec >= ttSeg.nRec) rec = ttSeg.nRec - 1;
	const double x = 2.0 * (jd - (ttSeg.jd0 + rec * ttSeg.dpr)) / ttSeg.dpr - 1.0;
	const double c0 = coef(ttSeg.scale, seed, rec, 0, 0), c1 = coef(ttSeg.scale, seed, rec, 0, 1);
	const double c2 = coef(ttSeg.scale, seed, rec, 0, 2), c3 = coef(ttSeg.scale, seed, rec, 0, 3);
	secs = c0 + c1 * x + c2 * (2. * x * x - 1.) + c3 * (4. * x * x * x - 3. * x);
	const double dp = c1 + 4. * c2 * x + c3 * (12. * x * x - 3.);
	rate = dp * 2.0 / ttSeg.dpr / 86400.0;
}

const double seedA = 1.0; // de431t.bsp
const double seedB = 3.0; // tt_alt.bsp
}

void TestBSPManager::initTestCase()
{
	QVERIFY2(tmpDir.isValid(), "Cannot create a temporary directory");
	goodDir = tmpDir.filePath(QStringLiteral("good"));
	mixedDir = tmpDir.filePath(QStringLiteral("mixed"));
	QVERIFY(QDir().mkpath(goodDir));
	QVERIFY(QDir().mkpath(mixedDir));

	const QByteArray de431t = buildKernel({ttSeg, earth}, seedA);
	const QByteArray ttAlt  = buildKernel({ttSeg}, seedB);
	const QByteArray planets = buildKernel({earth, moon}, 1.0);
	for (const QString& d : {goodDir, mixedDir})
	{
		QVERIFY(writeFile(d + QStringLiteral("/de431t.bsp"), de431t));
		QVERIFY(writeFile(d + QStringLiteral("/tt_alt.bsp"), ttAlt));
		QVERIFY(writeFile(d + QStringLiteral("/planets.bsp"), planets));
	}
	QVERIFY(writeFile(mixedDir + QStringLiteral("/corrupt.bsp"), QByteArray(4096, 'x')));
	QVERIFY(writeFile(mixedDir + QStringLiteral("/readme.txt"), QByteArray("not a kernel")));
}

void TestBSPManager::testRescan()
{
	BSPManager mgr(mixedDir);
	QCOMPARE(mgr.kernels().size(), 0); // nothing before rescan()
	QSignalSpy spy(&mgr, SIGNAL(kernelsChanged()));

	QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("BSPReader:.*"))); // corrupt.bsp
	QCOMPARE(mgr.rescan(), 4);               // 4 *.bsp files, readme.txt ignored
	QCOMPARE(spy.count(), 1);

	QStringList all;
	for (const BSPManager::KernelInfo& k : mgr.kernels())
		all << k.fileName;
	QCOMPARE(all, QStringList() << QStringLiteral("corrupt.bsp") << QStringLiteral("de431t.bsp")
				    << QStringLiteral("planets.bsp") << QStringLiteral("tt_alt.bsp"));

	// Only readable kernels are offered, and the TT-TDB ones can be singled out
	QCOMPARE(mgr.kernelFileNames(), QStringList() << QStringLiteral("de431t.bsp") << QStringLiteral("planets.bsp") << QStringLiteral("tt_alt.bsp"));
	QCOMPARE(mgr.kernelFileNames(true), QStringList() << QStringLiteral("de431t.bsp") << QStringLiteral("tt_alt.bsp"));

	const BSPManager::KernelInfo* bad = mgr.kernelInfo(QStringLiteral("corrupt.bsp"));
	QVERIFY(bad && !bad->valid);

	const BSPManager::KernelInfo* k = mgr.kernelInfo(QStringLiteral("DE431T.BSP")); // case-insensitive
	QVERIFY(k && k->valid);
	QCOMPARE(k->segmentCount, 2);
	QVERIFY(k->hasTTminusTDB);
	QVERIFY(qAbs(k->ttJdBegin - ttSeg.jd0) < 1e-9);
	QVERIFY(qAbs(k->ttJdEnd - (ttSeg.jd0 + ttSeg.nRec * ttSeg.dpr)) < 1e-9);
	QVERIFY(qAbs(k->jdBegin - 2451545.0) < 1e-9);
	QVERIFY(qAbs(k->jdEnd - (earth.jd0 + earth.nRec * earth.dpr)) < 1e-9 || qAbs(k->jdEnd - (ttSeg.jd0 + ttSeg.nRec * ttSeg.dpr)) < 1e-9);
	QVERIFY(k->sizeBytes > 1024);

	const BSPManager::KernelInfo* p = mgr.kernelInfo(QStringLiteral("planets.bsp"));
	QVERIFY(p && p->valid && !p->hasTTminusTDB);
	QVERIFY(mgr.kernelInfo(QStringLiteral("nothing.bsp")) == nullptr);
}

void TestBSPManager::testTTminusTDB()
{
	BSPManager mgr(goodDir);
	QCOMPARE(mgr.rescan(), 3);

	// Default kernel
	QCOMPARE(mgr.ttMinusTDBKernel(), QStringLiteral("de431t.bsp"));
	QVERIFY(mgr.ttMinusTDBAvailable(2451600.0));
	for (double jd : {2451545.0, 2451560.3, 2451577.0, 2451600.9, 2451545.0 + 5 * 32.0})
	{
		double secs, rate, es, er;
		QVERIFY2(mgr.ttMinusTDB(jd, secs, &rate), qPrintable(QStringLiteral("TT-TDB failed at JD %1").arg(jd, 0, 'f', 3)));
		expectedTT(seedA, jd, es, er);
		QVERIFY2(qAbs(secs - es) <= 1e-12 * 1e-3 * 10, qPrintable(QStringLiteral("JD %1: got %2 expected %3").arg(jd, 0, 'f', 3).arg(secs, 0, 'g', 17).arg(es, 0, 'g', 17)));
		QVERIFY(qAbs(rate - er) <= 1e-12 * 1e-3 * 10);

		double jdTT;
		QVERIFY(mgr.jdTTfromTDB(jd, jdTT));
		QVERIFY(qAbs(jdTT - (jd + es / 86400.0)) < 1e-12);
	}

	// Another kernel with different TT-TDB data
	mgr.setTTminusTDBKernel(QStringLiteral("tt_alt.bsp"));
	double secs, es, er;
	QVERIFY(mgr.ttMinusTDB(2451560.3, secs));
	expectedTT(seedB, 2451560.3, es, er);
	QVERIFY(qAbs(secs - es) <= 1e-12 * 1e-3 * 30);
	double esA;
	expectedTT(seedA, 2451560.3, esA, er);
	QVERIFY(qAbs(es - esA) > 1e-6); // the two kernels really differ

	// Outside the coverage of the segment
	QVERIFY(!mgr.ttMinusTDBAvailable(2451000.0));
	QVERIFY(!mgr.ttMinusTDB(2451000.0, secs));

	// A kernel without TT-TDB, an unknown kernel
	mgr.setTTminusTDBKernel(QStringLiteral("planets.bsp"));
	QVERIFY(!mgr.ttMinusTDBAvailable(2451560.3));
	QVERIFY(!mgr.ttMinusTDB(2451560.3, secs));
	mgr.setTTminusTDBKernel(QStringLiteral("missing.bsp"));
	QVERIFY(!mgr.ttMinusTDBAvailable(2451560.3));
	QVERIFY(!mgr.ttMinusTDB(2451560.3, secs));

	// Selecting a kernel before it exists is remembered once it appears after a rescan
	mgr.setTTminusTDBKernel(QStringLiteral("de431t.bsp"));
	QVERIFY(mgr.ttMinusTDBAvailable(2451560.3));
}

void TestBSPManager::testNoKernel()
{
	// "No kernel" is a valid choice: TT-TDB is then simply unavailable (the caller falls back on something else)
	BSPManager mgr(goodDir);
	QCOMPARE(mgr.rescan(), 3);
	double secs;
	QVERIFY(mgr.ttMinusTDB(2451560.3, secs)); // default kernel de431t.bsp
	QVERIFY(!mgr.ttMinusTDBKernelIsNone());

	QSignalSpy spy(&mgr, SIGNAL(errorOccurred(QString)));
	for (const QString& none : {BSPManager::noTTminusTDBKernel(), QStringLiteral(""), QStringLiteral("  "), QStringLiteral("NONE")})
	{
		mgr.setTTminusTDBKernel(none);
		QVERIFY(mgr.ttMinusTDBKernelIsNone());
		QCOMPARE(mgr.ttMinusTDBKernel(), BSPManager::noTTminusTDBKernel()); // always stored in the same form
		QVERIFY(!mgr.ttMinusTDBAvailable(2451560.3));
		QVERIFY(!mgr.ttMinusTDB(2451560.3, secs));
		double jdTT;
		QVERIFY(!mgr.jdTTfromTDB(2451560.3, jdTT));
	}
	QCOMPARE(spy.count(), 0); // choosing no kernel on purpose is not an error worth reporting

	// and back to a real kernel
	mgr.setTTminusTDBKernel(QStringLiteral("de431t.bsp"));
	QVERIFY(!mgr.ttMinusTDBKernelIsNone());
	QVERIFY(mgr.ttMinusTDB(2451560.3, secs));
}

void TestBSPManager::testReaderReuse()
{
	BSPManager mgr(goodDir);
	mgr.rescan();
	BSPReader* r1 = mgr.reader(QStringLiteral("planets.bsp"));
	QVERIFY(r1 != nullptr);
	QVERIFY(r1->isOpen());
	QVERIFY(mgr.reader(QStringLiteral("PLANETS.bsp")) == r1);     // same reader, found case-insensitively
	QVERIFY(mgr.reader(QStringLiteral("unknown.bsp")) == nullptr);

	double p[3], v[3];
	QVERIFY(mgr.compute(QStringLiteral("planets.bsp"), 2451560.0, 399, 3, p, v));
	QVERIFY(!mgr.compute(QStringLiteral("planets.bsp"), 2451560.0, 499, 4, p, v)); // not in the file
	QVERIFY(!mgr.compute(QStringLiteral("unknown.bsp"), 2451560.0, 399, 3, p, v));

	mgr.closeAll();
	BSPReader* r2 = mgr.reader(QStringLiteral("planets.bsp")); // reopened on demand
	QVERIFY(r2 != nullptr && r2->isOpen());
}

void TestBSPManager::testRefresh()
{
	const QString dir = tmpDir.filePath(QStringLiteral("refresh"));
	QVERIFY(QDir().mkpath(dir));
	QVERIFY(writeFile(dir + QStringLiteral("/de431t.bsp"), buildKernel({ttSeg}, seedA)));

	BSPManager mgr(dir);
	QCOMPARE(mgr.rescan(), 1);

	// A kernel added later is not seen until the next rescan
	QVERIFY(writeFile(dir + QStringLiteral("/tt_alt.bsp"), buildKernel({ttSeg}, seedB)));
	QCOMPARE(mgr.kernels().size(), 1);
	QSignalSpy spy(&mgr, SIGNAL(kernelsChanged()));
	QCOMPARE(mgr.rescan(), 2);
	QCOMPARE(spy.count(), 1);
	QCOMPARE(mgr.kernelFileNames(true), QStringList() << QStringLiteral("de431t.bsp") << QStringLiteral("tt_alt.bsp"));

	// The TT-TDB reader still works after a rescan (readers were closed and are reopened)
	double secs, es, er;
	QVERIFY(mgr.ttMinusTDB(2451560.3, secs));
	expectedTT(seedA, 2451560.3, es, er);
	QVERIFY(qAbs(secs - es) <= 1e-12 * 1e-3 * 10);

	// setDirectory() switches folders and rescans
	mgr.setDirectory(goodDir);
	QCOMPARE(mgr.kernels().size(), 3);
}

void TestBSPManager::testMissingDirectory()
{
	BSPManager mgr(tmpDir.filePath(QStringLiteral("does_not_exist")));
	QCOMPARE(mgr.rescan(), 0);
	QVERIFY(mgr.kernels().isEmpty());
	QVERIFY(mgr.kernelFileNames().isEmpty());
	double secs;
	QVERIFY(!mgr.ttMinusTDBAvailable(2451560.3));
	QVERIFY(!mgr.ttMinusTDB(2451560.3, secs));
	double jdTT;
	QVERIFY(!mgr.jdTTfromTDB(2451560.3, jdTT));
}

void TestBSPManager::testDumpSegments()
{
	BSPManager mgr(goodDir);
	mgr.rescan();
	const QStringList lines = mgr.dumpSegments(QStringLiteral("planets.bsp"));
	QCOMPARE(lines.size(), 2);
	QVERIFY(lines.at(0).startsWith(QStringLiteral("399 / 3")));
	QVERIFY(lines.at(1).startsWith(QStringLiteral("301 / 3")));
	QVERIFY(mgr.dumpSegments(QStringLiteral("unknown.bsp")).isEmpty());
}
