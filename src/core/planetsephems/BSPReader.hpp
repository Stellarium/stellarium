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

#ifndef BSPREADER_HPP
#define BSPREADER_HPP

#include <QString>
#include <QVector>
#include <QFile>

//! @class BSPReader
//! Minimal reader for NAIF DAF/SPK kernels (*.bsp) holding Chebyshev segments
//! (SPK type 2 = position only, type 3 = position+velocity; only the position
//! polynomial is used, the velocity being its analytic derivative).
//!
//! Everything needed to decode a file is taken from the file itself (DAF file record:
//! number of doubles/integers per summary, location of the first summary record, byte order),
//! so no hard-coded header offsets or endian flags are needed, and any number of segments
//! and summary records is supported (e.g. DE441, which has two summary records).
//!
//! Time argument: JD in TDB (ephemeris time), as in the BSP file itself.
//! Units returned by compute(): AU and AU/day, except for the NAIF pseudo-body
//! TT_MINUS_TDB_TARGET, for which p[0..2] is in seconds and v[0..2] in s/s (dimensionless).
//!
//! Not thread safe (the last decoded record of each segment is cached).
class BSPReader
{
public:
	//! One SPK segment, i.e. one (target, center, time span) entry of the file summary.
	struct Segment
	{
		double jdBegin = 0.;       //!< start of coverage, JD(TDB)
		double jdEnd = 0.;         //!< end of coverage, JD(TDB)
		int target = 0;            //!< NAIF id of the target body
		int center = 0;            //!< NAIF id of the center body
		int frame = 0;             //!< NAIF frame id (1 = J2000/ICRF)
		int type = 0;              //!< SPK segment type (2 or 3)
		qint64 firstWord = 0;      //!< 1-based DAF address of the first word of the segment data
		qint64 lastWord = 0;       //!< 1-based DAF address of the last word of the segment data
		double jdInit = 0.;        //!< JD(TDB) of the start of the first record
		double daysPerRecord = 0.; //!< length of one Chebyshev record, in days
		int recordSize = 0;        //!< number of doubles per record (incl. MID and RADIUS)
		int nRecords = 0;          //!< number of records in the segment
		int nCoeff = 0;            //!< number of Chebyshev coefficients per component
	};

	//! NAIF pseudo-id used by the TTmTDB kernels (TT - TDB, in seconds)
	static constexpr int TT_MINUS_TDB_TARGET = 1000000001;
	//! IAU 2012 astronomical unit in km, used to convert km, km/day to AU, AU/day.
	//! Deliberately NOT named AU or AU_KM: those are macros in StelUtils.hpp and in JPLStellariumM.hpp.
	static constexpr double KM_PER_AU = 149597870.700;

	explicit BSPReader(const QString& path);
	~BSPReader();

	//! Open the file and decode the DAF file record and all segment summaries.
	//! @return true on success. Problems are reported with qWarning().
	bool open();
	void close();
	bool isOpen() const { return file.isOpen(); }
	QString fileName() const { return file.fileName(); }

	//! All segments found in the file (only types 2 and 3).
	const QVector<Segment>& segments() const { return segs; }

	//! True if a segment (target, center) covers jd (TDB).
	bool covers(double jd, int target, int center) const { return findSegment(jd, target, center) >= 0; }

	//! Compute position and velocity of target relative to center.
	//! @param jd   Julian Date in TDB
	//! @param p    position (AU), or seconds for TT_MINUS_TDB_TARGET
	//! @param v    velocity (AU/day), or s/s for TT_MINUS_TDB_TARGET
	//! @return false if the file is not open or no segment covers the request.
	bool compute(double jd, int target, int center, double p[3], double v[3]);

private:
	struct CachedRecord
	{
		qint64 recno = -1;
		QVector<double> coeffs; //!< 3*nCoeff values: X, Y, Z coefficient blocks
	};

	Q_DISABLE_COPY(BSPReader)

	bool readAt(qint64 pos, char* buf, qint64 n);
	bool readFileRecord();
	bool readSummaries(int nd, int ni, qint64 firstSummaryRecord);
	int findSegment(double jd, int target, int center) const;
	bool loadRecord(int segIndex, qint64 recno);

	QFile file;
	bool bigEndian = false;
	QVector<Segment> segs;
	QVector<CachedRecord> cache; //!< one cached record per segment
	QVector<char> rawBuf;        //!< scratch buffer for reading coefficients
};

#endif // BSPREADER_HPP
