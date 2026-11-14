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
 * (SS) 2026-10-02
 * Unit tests for BSPReader (NAIF/JPL SPK kernel reader)
 *
 * The tests write small synthetic SPK kernels (type 2, Chebyshev) at run time, in both byte orders,
 * with two summary records, and compare BSPReader::compute() with a closed-form evaluation of the
 * degree-3 Chebyshev series (T0=1, T1=x, T2=2x^2-1, T3=4x^3-3x). No external data files are needed.
 */

#include <QObject>
#include <QtDebug>
#include <QtEndian>
#include <QFile>
#include <QByteArray>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QVector>
#include <cstring>

#include "tests/testBSPReader.hpp"
#include "BSPReader.hpp"

QTEST_GUILESS_MAIN(TestBSPReader)

namespace
{
const int NC = 4;                 // Chebyshev coefficients per component (degree 3)
const double JD_J2000 = 2451545.0;

struct SegDef
{
	int target, center;
	double jd0;      // start of the first record (JD TDB)
	int nRec;        // number of records
	double dpr;      // days per record
	double scale;    // coefficient magnitude (km, or s for TT-TDB)
};

const SegDef segDefs[4] = {
	{399, 3,                                   2451545.0, 6, 16.0, 1.0e7},
	{10,  0,                                   2451545.0, 4, 32.0, 5.0e5},
	{BSPReader::TT_MINUS_TDB_TARGET, 1000000000, 2451545.0, 5, 32.0, 1.0e-3},
	{301, 3,                                   2451600.0, 8,  4.0, 2.0e5}
};

// Deterministic coefficient i (0..NC-1) of component k (0..2) of record rec of segment s
double coef(int s, int rec, int k, int i)
{
	return segDefs[s].scale * (1.0 + 0.25 * rec) * (k + 1) * ((i % 2) ? -1.0 : 1.0) / (i + 1.5);
}

void appendDouble(QByteArray& ba, double d, bool big)
{
	quint64 u;
	std::memcpy(&u, &d, 8);
	u = big ? qToBigEndian<quint64>(u) : qToLittleEndian<quint64>(u);
	ba.append(reinterpret_cast<const char*>(&u), 8);
}

void putInt32(QByteArray& ba, int pos, qint32 v, bool big)
{
	quint32 u = static_cast<quint32>(v);
	u = big ? qToBigEndian<quint32>(u) : qToLittleEndian<quint32>(u);
	std::memcpy(ba.data() + pos, &u, 4);
}

void appendInt32(QByteArray& ba, qint32 v, bool big)
{
	QByteArray tmp(4, '\0');
	putInt32(tmp, 0, v, big);
	ba.append(tmp);
}

struct SummaryInfo
{
	int target, center;
	double jdBegin, jdEnd;
	qint32 firstWord, lastWord;
};

// One DAF summary record holding the given summaries
QByteArray summaryRecord(const QVector<SummaryInfo>& items, int next, int prev, bool big)
{
	QByteArray rec;
	appendDouble(rec, next, big);
	appendDouble(rec, prev, big);
	appendDouble(rec, items.size(), big);
	for (const auto& s : items)
	{
		appendDouble(rec, (s.jdBegin - JD_J2000) * 86400.0, big);
		appendDouble(rec, (s.jdEnd - JD_J2000) * 86400.0, big);
		appendInt32(rec, s.target, big);
		appendInt32(rec, s.center, big);
		appendInt32(rec, 1, big);   // frame J2000
		appendInt32(rec, 2, big);   // SPK type 2
		appendInt32(rec, s.firstWord, big);
		appendInt32(rec, s.lastWord, big);
	}
	rec.append(QByteArray(1024 - rec.size(), '\0'));
	return rec;
}

// Layout: record 1 = file record, 2 = summaries (segments 0,1), 3 = names, 4 = summaries (segments 2,3), 5 = names,
// then the segment data from word 641 on.
QByteArray buildKernel(bool big, bool locfmt)
{
	QByteArray body;
	qint32 w = 5 * 128 + 1; // 1-based DAF word address
	QVector<SummaryInfo> sums;
	for (int s = 0; s < 4; ++s)
	{
		const SegDef& d = segDefs[s];
		const int recSize = 2 + 3 * NC;
		const qint32 first = w;
		for (int r = 0; r < d.nRec; ++r)
		{
			appendDouble(body, (d.jd0 + (r + 0.5) * d.dpr - JD_J2000) * 86400.0, big); // MID
			appendDouble(body, d.dpr * 43200.0, big);                                    // RADIUS
			for (int k = 0; k < 3; ++k)
				for (int i = 0; i < NC; ++i)
					appendDouble(body, coef(s, r, k, i), big);
			w += recSize;
		}
		appendDouble(body, (d.jd0 - JD_J2000) * 86400.0, big); // INIT
		appendDouble(body, d.dpr * 86400.0, big);              // INTLEN
		appendDouble(body, recSize, big);                      // RSIZE
		appendDouble(body, d.nRec, big);                       // N
		w += 4;
		sums.append({d.target, d.center, d.jd0, d.jd0 + d.nRec * d.dpr, first, w - 1});
	}

	QByteArray fileRec(1024, '\0');
	std::memcpy(fileRec.data(), "DAF/SPK ", 8);
	putInt32(fileRec, 8, 2, big);   // ND
	putInt32(fileRec, 12, 6, big);  // NI
	putInt32(fileRec, 76, 2, big);  // FWARD
	putInt32(fileRec, 80, 4, big);  // BWARD
	putInt32(fileRec, 84, w, big);  // FREE
	if (locfmt)
		std::memcpy(fileRec.data() + 88, big ? "BIG-IEEE" : "LTL-IEEE", 8);

	QByteArray out = fileRec;
	out += summaryRecord(sums.mid(0, 2), 4, 0, big);
	out += QByteArray(1024, '\0');
	out += summaryRecord(sums.mid(2, 2), 0, 2, big);
	out += QByteArray(1024, '\0');
	out += body;
	return out;
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

// Closed-form degree-3 Chebyshev series and its derivative, independent of the Clenshaw code in BSPReader
void evalCheb(int s, int rec, int k, double x, double& p, double& dp, double& magP, double& magDp)
{
	const double c0 = coef(s, rec, k, 0), c1 = coef(s, rec, k, 1), c2 = coef(s, rec, k, 2), c3 = coef(s, rec, k, 3);
	p  = c0 + c1 * x + c2 * (2. * x * x - 1.) + c3 * (4. * x * x * x - 3. * x);
	dp = c1 + 4. * c2 * x + c3 * (12. * x * x - 3.);
	magP  = qAbs(c0) + qAbs(c1) + qAbs(c2) + qAbs(c3);
	magDp = qAbs(c1) + 4. * qAbs(c2) + 9. * qAbs(c3);
}

// Expected result of BSPReader::compute() for segment s at jd (units as documented in BSPReader.hpp)
void expected(int s, double jd, double p[3], double v[3], double tolP[3], double tolV[3])
{
	const SegDef& d = segDefs[s];
	int rec = static_cast<int>(std::floor((jd - d.jd0) / d.dpr));
	if (rec >= d.nRec) rec = d.nRec - 1;
	const double x = 2.0 * (jd - (d.jd0 + rec * d.dpr)) / d.dpr - 1.0;
	const bool isTT = (d.target == BSPReader::TT_MINUS_TDB_TARGET);
	for (int k = 0; k < 3; ++k)
	{
		double pp, dp, mP, mD;
		evalCheb(s, rec, k, x, pp, dp, mP, mD);
		const double dpPerDay = dp * 2.0 / d.dpr;
		const double fP = isTT ? 1.0 : 1.0 / BSPReader::KM_PER_AU;
		const double fV = isTT ? 1.0 / 86400.0 : 1.0 / BSPReader::KM_PER_AU;
		p[k] = pp * fP;
		v[k] = dpPerDay * fV;
		tolP[k] = 1e-12 * mP * fP;
		tolV[k] = 1e-12 * mD * 2.0 / d.dpr * fV;
	}
}
}

void TestBSPReader::initTestCase()
{
	QVERIFY2(tmpDir.isValid(), "Cannot create a temporary directory");
	const struct { const char* name; bool big; bool locfmt; } variants[] = {
		{"le.bsp", false, true},
		{"be.bsp", true, true},
		{"be_old.bsp", true, false}
	};
	for (const auto& v : variants)
	{
		const QString path = tmpDir.filePath(QString::fromLatin1(v.name));
		QVERIFY2(writeFile(path, buildKernel(v.big, v.locfmt)), qPrintable(QStringLiteral("Cannot write %1").arg(path)));
		kernelPaths << path;
	}
}

void TestBSPReader::testSegments()
{
	for (const QString& path : std::as_const(kernelPaths))
	{
		BSPReader r(path);
		QVERIFY2(r.open(), qPrintable(QStringLiteral("open() failed for %1").arg(path)));
		QVERIFY(r.isOpen());
		QCOMPARE(r.segments().size(), 4);
		for (int s = 0; s < 4; ++s)
		{
			const BSPReader::Segment& seg = r.segments().at(s);
			const SegDef& d = segDefs[s];
			QVERIFY2(seg.target == d.target && seg.center == d.center, qPrintable(QStringLiteral("Wrong target/center in segment %1 of %2").arg(s).arg(path)));
			QCOMPARE(seg.type, 2);
			QCOMPARE(seg.nCoeff, NC);
			QCOMPARE(seg.nRecords, d.nRec);
			QVERIFY(qAbs(seg.jdBegin - d.jd0) < 1e-9);
			QVERIFY(qAbs(seg.jdEnd - (d.jd0 + d.nRec * d.dpr)) < 1e-9);
			QVERIFY(qAbs(seg.daysPerRecord - d.dpr) < 1e-12);
		}
		r.close();
		QVERIFY(!r.isOpen());
	}
}

void TestBSPReader::testCompute()
{
	QRandomGenerator rng(20261002);
	for (const QString& path : std::as_const(kernelPaths))
	{
		BSPReader r(path);
		QVERIFY(r.open());
		// Alternate between segments (as Stellarium does for Earth, Sun, Moon), exercising the per-segment cache
		for (int n = 0; n < 40; ++n)
		{
			for (int s = 0; s < 4; ++s)
			{
				const SegDef& d = segDefs[s];
				const double jd = d.jd0 + rng.generateDouble() * d.nRec * d.dpr;
				double p[3], v[3], ep[3], ev[3], tp[3], tv[3];
				QVERIFY2(r.compute(jd, d.target, d.center, p, v), qPrintable(QStringLiteral("compute() failed at JD %1").arg(jd, 0, 'f', 6)));
				expected(s, jd, ep, ev, tp, tv);
				for (int k = 0; k < 3; ++k)
				{
					QVERIFY2(qAbs(p[k] - ep[k]) <= tp[k], qPrintable(QStringLiteral("%1 seg %2 JD %3 p[%4]: got %5 expected %6").arg(path).arg(s).arg(jd, 0, 'f', 6).arg(k).arg(p[k], 0, 'g', 17).arg(ep[k], 0, 'g', 17)));
					QVERIFY2(qAbs(v[k] - ev[k]) <= tv[k], qPrintable(QStringLiteral("%1 seg %2 JD %3 v[%4]: got %5 expected %6").arg(path).arg(s).arg(jd, 0, 'f', 6).arg(k).arg(v[k], 0, 'g', 17).arg(ev[k], 0, 'g', 17)));
				}
			}
		}
	}
}

void TestBSPReader::testRecordBoundaryAndEnds()
{
	for (const QString& path : std::as_const(kernelPaths))
	{
		BSPReader r(path);
		QVERIFY(r.open());
		for (int s = 0; s < 4; ++s)
		{
			const SegDef& d = segDefs[s];
			// First instant of record 1 (x = -1) and of record 2, then the very last instant of the segment
			const double jds[3] = {d.jd0 + d.dpr, d.jd0 + 2.0 * d.dpr, d.jd0 + d.nRec * d.dpr};
			for (double jd : jds)
			{
				double p[3], v[3], ep[3], ev[3], tp[3], tv[3];
				QVERIFY2(r.compute(jd, d.target, d.center, p, v), qPrintable(QStringLiteral("compute() failed at JD %1 (segment %2)").arg(jd, 0, 'f', 6).arg(s)));
				expected(s, jd, ep, ev, tp, tv);
				for (int k = 0; k < 3; ++k)
				{
					QVERIFY(qAbs(p[k] - ep[k]) <= tp[k]);
					QVERIFY(qAbs(v[k] - ev[k]) <= tv[k]);
				}
			}
		}
	}
}

void TestBSPReader::testNotCovered()
{
	for (const QString& path : std::as_const(kernelPaths))
	{
		BSPReader r(path);
		QVERIFY(r.open());
		double p[3], v[3];
		const SegDef& d = segDefs[0];
		QVERIFY(r.covers(d.jd0 + 1.0, d.target, d.center));
		QVERIFY(!r.covers(d.jd0 - 1.0, d.target, d.center));                  // before the segment
		QVERIFY(!r.covers(d.jd0 + d.nRec * d.dpr + 1.0, d.target, d.center)); // after the segment
		QVERIFY(!r.covers(d.jd0 + 1.0, d.target, 5));                         // wrong center
		QVERIFY(!r.covers(d.jd0 + 1.0, 499, d.center));                       // wrong target
		QVERIFY(!r.compute(d.jd0 - 1.0, d.target, d.center, p, v));
		QVERIFY(!r.compute(d.jd0 + 1.0, 499, d.center, p, v));
		r.close();
		QVERIFY(!r.compute(d.jd0 + 1.0, d.target, d.center, p, v));          // closed reader
	}
}

void TestBSPReader::testInvalidFiles()
{
	// Missing file
	{
		BSPReader r(tmpDir.filePath(QStringLiteral("does_not_exist.bsp")));
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("BSPReader:.*")));
		QVERIFY(!r.open());
		QVERIFY(!r.isOpen());
	}
	// Not a DAF file
	{
		const QString path = tmpDir.filePath(QStringLiteral("garbage.bsp"));
		QVERIFY(writeFile(path, QByteArray(4096, 'x')));
		BSPReader r(path);
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("BSPReader:.*")));
		QVERIFY(!r.open());
		QVERIFY(!r.isOpen());
	}
	// Truncated file (file record only)
	{
		const QString path = tmpDir.filePath(QStringLiteral("truncated.bsp"));
		QVERIFY(writeFile(path, buildKernel(false, true).left(1024)));
		BSPReader r(path);
		QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("BSPReader:.*")));
		QVERIFY(!r.open());
		QVERIFY(!r.isOpen());
	}
}
