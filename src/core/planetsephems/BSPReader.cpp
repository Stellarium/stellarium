/*
Copyright (c) 2026 Sylvain Simard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

BSPReader: reader for NAIF/JPL SPK ephemeris kernels (*.bsp)
Adapted from the standalone JPL Horizons study program.
*/

#include "BSPReader.hpp"

#include <QtGlobal>
#include <QtEndian>
#include <QDebug>
#include <cmath>
#include <cstring>

namespace
{
const qint64 DAF_RECORD_BYTES = 1024;
const double JD_J2000 = 2451545.0;
const double SECS_PER_DAY = 86400.0;

// ET seconds past J2000 -> JD(TDB)
inline double secondsToJD(double secs) { return JD_J2000 + secs / SECS_PER_DAY; }

// Decode little/big endian numbers stored in a byte buffer, independent of the host byte order.
inline double decodeDouble(const char* b, bool big)
{
	quint64 u;
	std::memcpy(&u, b, 8);
	u = big ? qFromBigEndian<quint64>(u) : qFromLittleEndian<quint64>(u);
	double d;
	std::memcpy(&d, &u, 8);
	return d;
}

inline qint32 decodeInt32(const char* b, bool big)
{
	quint32 u;
	std::memcpy(&u, b, 4);
	u = big ? qFromBigEndian<quint32>(u) : qFromLittleEndian<quint32>(u);
	return static_cast<qint32>(u);
}
}

BSPReader::BSPReader(const QString& path)
	: file(path)
{
}

BSPReader::~BSPReader()
{
	close();
}

void BSPReader::close()
{
	if (file.isOpen())
		file.close();
	segs.clear();
	cache.clear();
}

bool BSPReader::readAt(qint64 pos, char* buf, qint64 n)
{
	return file.seek(pos) && file.read(buf, n) == n;
}

bool BSPReader::open()
{
	close();
	if (!file.open(QIODevice::ReadOnly))
	{
		qWarning() << "BSPReader: cannot open" << file.fileName() << ":" << file.errorString();
		return false;
	}
	if (!readFileRecord())
	{
		close();
		return false;
	}
	cache.resize(segs.size());
	return true;
}

// DAF file record (first 1024 bytes):
//   0..7 ID word ("DAF/SPK "), 8..11 ND, 12..15 NI, 16..75 internal name,
//   76..79 FWARD (record number of first summary record), 80..83 BWARD, 84..87 FREE,
//   88..95 LOCFMT ("LTL-IEEE" or "BIG-IEEE"; blank in very old files)
bool BSPReader::readFileRecord()
{
	char rec[DAF_RECORD_BYTES];
	if (!readAt(0, rec, DAF_RECORD_BYTES))
	{
		qWarning() << "BSPReader:" << file.fileName() << ": cannot read the DAF file record";
		return false;
	}
	if (std::strncmp(rec, "DAF/SPK", 7) != 0 && std::strncmp(rec, "NAIF/DAF", 8) != 0)
	{
		qWarning() << "BSPReader:" << file.fileName() << ": not a DAF/SPK file";
		return false;
	}

	if (std::strncmp(rec + 88, "LTL-IEEE", 8) == 0)
		bigEndian = false;
	else if (std::strncmp(rec + 88, "BIG-IEEE", 8) == 0)
		bigEndian = true;
	else // No LOCFMT (old kernels): ND must be 2 for an SPK file, which tells the byte order.
		bigEndian = (decodeInt32(rec + 8, false) != 2 && decodeInt32(rec + 8, true) == 2);

	const int nd = decodeInt32(rec + 8, bigEndian);
	const int ni = decodeInt32(rec + 12, bigEndian);
	const int fward = decodeInt32(rec + 76, bigEndian);
	if (nd != 2 || ni != 6)
	{
		qWarning() << "BSPReader:" << file.fileName() << ": unexpected DAF summary format ND/NI =" << nd << "/" << ni << "(SPK needs 2/6)";
		return false;
	}
	return readSummaries(nd, ni, fward);
}

// Summary record: 3 doubles (NEXT, PREV, NSUM), then NSUM summaries of (ND + (NI+1)/2) doubles each.
// SPK summary: JDb, JDe (ET seconds), then ints: target, center, frame, type, first word, last word.
bool BSPReader::readSummaries(int nd, int ni, qint64 recNum)
{
	const int summaryBytes = (nd + (ni + 1) / 2) * 8;
	const int maxPerRecord = static_cast<int>((DAF_RECORD_BYTES - 24) / summaryBytes);
	char buf[DAF_RECORD_BYTES];
	int guard = 0;

	segs.clear();
	while (recNum > 0)
	{
		if (++guard > 100000 || !readAt((recNum - 1) * DAF_RECORD_BYTES, buf, DAF_RECORD_BYTES))
		{
			qWarning() << "BSPReader:" << file.fileName() << ": cannot read summary record" << recNum;
			return false;
		}
		const double next = decodeDouble(buf, bigEndian);
		const int nsum = static_cast<int>(decodeDouble(buf + 16, bigEndian));
		if (nsum < 0 || nsum > maxPerRecord)
		{
			qWarning() << "BSPReader:" << file.fileName() << ": corrupt summary record" << recNum;
			return false;
		}

		for (int i = 0; i < nsum; ++i)
		{
			const char* s = buf + 24 + i * summaryBytes;
			Segment seg;
			seg.jdBegin   = secondsToJD(decodeDouble(s, bigEndian));
			seg.jdEnd     = secondsToJD(decodeDouble(s + 8, bigEndian));
			seg.target    = decodeInt32(s + 16, bigEndian);
			seg.center    = decodeInt32(s + 20, bigEndian);
			seg.frame     = decodeInt32(s + 24, bigEndian);
			seg.type      = decodeInt32(s + 28, bigEndian);
			seg.firstWord = decodeInt32(s + 32, bigEndian);
			seg.lastWord  = decodeInt32(s + 36, bigEndian);

			if (seg.type != 2 && seg.type != 3)
			{
				qWarning() << "BSPReader: skipping segment" << seg.target << "/" << seg.center << "of unsupported SPK type" << seg.type;
				continue;
			}

			// Segment trailer: the last 4 words are INIT (ET s), INTLEN (s), RSIZE, N
			char t[32];
			if (!readAt((seg.lastWord - 4) * 8, t, 32))
			{
				qWarning() << "BSPReader: cannot read the trailer of segment" << seg.target << "/" << seg.center;
				return false;
			}
			const double init = decodeDouble(t, bigEndian);
			const double intlen = decodeDouble(t + 8, bigEndian);
			const double rsize = decodeDouble(t + 16, bigEndian);
			const double n = decodeDouble(t + 24, bigEndian);
			const int components = (seg.type == 2) ? 3 : 6;
			seg.recordSize = static_cast<int>(rsize);
			seg.nRecords = static_cast<int>(n);
			seg.nCoeff = (seg.recordSize - 2) / components;
			if (intlen <= 0. || seg.nRecords < 1 || seg.nCoeff < 2 || (seg.recordSize - 2) % components != 0)
			{
				qWarning() << "BSPReader: invalid trailer for segment" << seg.target << "/" << seg.center;
				return false;
			}
			seg.jdInit = secondsToJD(init);
			seg.daysPerRecord = intlen / SECS_PER_DAY;
			segs.append(seg);
		}
		recNum = static_cast<qint64>(next); // 0 = last summary record
	}
	return true;
}

int BSPReader::findSegment(double jd, int target, int center) const
{
	for (int i = 0; i < segs.size(); ++i)
	{
		const Segment& s = segs.at(i);
		if (s.target == target && s.center == center && jd >= s.jdBegin && jd <= s.jdEnd)
			return i;
	}
	return -1;
}

// Record layout: MID, RADIUS, then X, Y, Z (and VX, VY, VZ for type 3) Chebyshev blocks of nCoeff each.
bool BSPReader::loadRecord(int segIndex, qint64 recno)
{
	const Segment& s = segs.at(segIndex);
	CachedRecord& c = cache[segIndex];
	const int nc = s.nCoeff;
	const qint64 offset = (s.firstWord - 1) * 8 + recno * s.recordSize * 8 + 16;

	rawBuf.resize(3 * nc * 8);
	if (!readAt(offset, rawBuf.data(), 3 * nc * 8))
	{
		c.recno = -1;
		qWarning() << "BSPReader: read error in" << file.fileName() << "(segment" << s.target << "/" << s.center << ", record" << recno << ")";
		return false;
	}
	c.coeffs.resize(3 * nc);
	for (int i = 0; i < 3 * nc; ++i)
		c.coeffs[i] = decodeDouble(rawBuf.constData() + 8 * i, bigEndian);
	c.recno = recno;
	return true;
}

bool BSPReader::compute(double jd, int target, int center, double p[3], double v[3])
{
	if (!file.isOpen())
		return false;
	const int index = findSegment(jd, target, center);
	if (index < 0)
		return false;

	const Segment& s = segs.at(index);
	qint64 recno = static_cast<qint64>(std::floor((jd - s.jdInit) / s.daysPerRecord));
	if (recno < 0) recno = 0;
	if (recno >= s.nRecords) recno = s.nRecords - 1; // jd == end of coverage

	if (cache.at(index).recno != recno && !loadRecord(index, recno))
		return false;
	const CachedRecord& c = cache.at(index);

	// Time within the record, split in integer and fractional day to keep full precision
	const double jdStart = s.jdInit + static_cast<double>(recno) * s.daysPerRecord;
	double jdInt, jdFrac, j0Int, j0Frac;
	jdFrac = std::modf(jd, &jdInt);
	j0Frac = std::modf(jdStart, &j0Int);
	const double deltaDays = (jdInt - j0Int) + (jdFrac - j0Frac);
	const double x = 2.0 * deltaDays / s.daysPerRecord - 1.0;

	const int nc = s.nCoeff;
	for (int k = 0; k < 3; ++k)
	{
		const double* ck = c.coeffs.constData() + k * nc;

		// Position: Clenshaw recurrence
		double d1 = 0.0, d2 = 0.0;
		for (int i = nc - 1; i >= 1; --i)
		{
			const double temp = d1;
			d1 = 2.0 * x * d1 - d2 + ck[i];
			d2 = temp;
		}
		p[k] = x * d1 - d2 + ck[0];

		// Velocity: derivative of the Chebyshev series, per day
		double v1 = 0.0, v2 = 0.0;
		for (int i = nc - 1; i >= 2; --i)
		{
			const double temp = v1;
			v1 = 2.0 * x * v1 - v2 + i * ck[i];
			v2 = temp;
		}
		v[k] = (2.0 * x * v1 - v2 + ck[1]) * (2.0 / s.daysPerRecord);

		if (target != TT_MINUS_TDB_TARGET)
		{
			p[k] /= KM_PER_AU;
			v[k] /= KM_PER_AU;
		}
		else
		{
			v[k] /= SECS_PER_DAY; // s/day -> s/s
		}
	}
	return true;
}
