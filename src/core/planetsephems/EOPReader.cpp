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

#include "EOPReader.hpp"

#include <QDate>
#include <QFile>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <erfa.h>	// (SS) 2026-11-05 eraJd2cal() and eraDat() for the TAI-UTC used by interpolate()

namespace
{
// Number field: empty and "-" mean "no value"
bool number(const QString& s, double& v)
{
	const QString t = s.trimmed();
	if (t.isEmpty() || t == QStringLiteral("-"))
		return false;
	bool ok = false;
	v = t.toDouble(&ok);
	return ok;
}

// (SS) 2026-11-05 TAI-UTC (seconds) at a UTC instant given as MJD(UTC), as in the standalone program (TaiMinusUtc()).
// eraDat() returns 1 for a "dubious year" (before 1960 it gives 0, after the last year of its table the last value) and a negative
// value on error. Only an invalid date or an error of eraDat() makes this function fail.
bool taiMinusUtc(double mjdUtc, double& dat)
{
	int iy = 0, im = 0, id = 0;
	double fd = 0.;
	if (eraJd2cal(2400000.5, mjdUtc, &iy, &im, &id, &fd) != 0)
		return false;
	return eraDat(iy, im, id, fd, &dat) >= 0;
}

EOPReader::Status status(const QString& s)
{
	const QString t = s.trimmed().toLower();
	if (t == QStringLiteral("final"))
		return EOPReader::Status::Final;
	if (t == QStringLiteral("prediction"))
		return EOPReader::Status::Prediction;
	if (t.isEmpty() || t == QStringLiteral("-"))
		return EOPReader::Status::Missing;
	return EOPReader::Status::Unknown;
}

// Both CSV layouts of the IERS start with the same 27 columns:
//  0 MJD, 1 Year, 2 Month, 3 Day, 4 Type(pole), 5 x_pole, 6 sigma, 7 y_pole, 8 sigma, 9-12 pole rates and sigmas,
//  13 Type(UT), 14 UT1-UTC, 15 sigma, 16 LOD, 17 sigma, 18 Type(nutation), 19 dPsi, 20 sigma, 21 dEpsilon, 22 sigma,
//  23 dX, 24 sigma, 25 dY, 26 sigma
// The finals files add 10 Bulletin B columns that are not read. Units differ:
//  finals (standard series): pole arcsec, UT1-UTC s, LOD msec, dPsi dEps dX dY marcsec
//  C04 (long term series)  : pole arcsec, UT1-UTC s, LOD s,    dPsi dEps dX dY arcsec, and no Type (all values are final)
enum Col { cMJD = 0, cYear, cMonth, cDay, cPoleType, cX, cSX, cY, cSY, cUTType = 13, cUT, cSUT, cLOD, cSLOD,
           cNutType, cDPsi, cSDPsi, cDEps, cSDEps, cDX, cSDX, cDY, cSDY, cMinFields = 23 };

bool parseCSV(const QStringList& lines, EOPReader::Format fmt, QVector<EOPReader::Record>& out, QString& error)
{
	const bool finals = (fmt == EOPReader::Format::Finals);
	const double lodToSec = finals ? 1e-3 : 1.0;
	const double nutToArcsec = finals ? 1e-3 : 1.0;

	// first line is the header
	for (int i = 1; i < lines.size(); ++i)
	{
		const QString line = lines.at(i).trimmed();
		if (line.isEmpty())
			continue;
		const QStringList f = line.split(';');
		if (f.size() < cMinFields)
			continue; // not a data line

		EOPReader::Record r;
		r.source = fmt;
		bool ok1, ok2, ok3, ok4;
		r.mjd = f.at(cMJD).trimmed().toDouble(&ok1);   // "60000" or "60000.00"
		r.year = f.at(cYear).trimmed().toInt(&ok2);
		r.month = f.at(cMonth).trimmed().toInt(&ok3);
		r.day = f.at(cDay).trimmed().toInt(&ok4);
		if (!(ok1 && ok2 && ok3 && ok4))
		{
			error = QString("bad date/MJD on line %1").arg(i + 1);
			return false;
		}

		double a, b, c;
		if (number(f.at(cX), a) && number(f.at(cY), b))
		{
			r.hasPole = true;
			r.xp = a;
			r.yp = b;
			if (number(f.at(cSX), c)) r.xpErr = c;
			if (number(f.at(cSY), c)) r.ypErr = c;
		}

		if (number(f.at(cUT), a))
		{
			r.hasUT = true;
			r.ut1utc = a;
			if (number(f.at(cSUT), c)) r.ut1utcErr = c;
		}
		// The finals files write 0.0000 for an undefined LOD (first row of 1973), the IERS EOP Reader tool shows "-" there
		if (number(f.at(cLOD), a) && !(finals && a == 0.0))
		{
			r.hasLOD = true;
			r.lod = a * lodToSec;
			if (number(f.at(cSLOD), c)) r.lodErr = c * lodToSec;
		}

		if (number(f.at(cDPsi), a) && number(f.at(cDEps), b))
		{
			r.hasNutation = true;
			r.dpsi = a * nutToArcsec;
			r.deps = b * nutToArcsec;
			if (number(f.at(cSDPsi), c)) r.dpsiErr = c * nutToArcsec;
			if (number(f.at(cSDEps), c)) r.depsErr = c * nutToArcsec;
		}

		// dX, dY (IAU 2006/2000A celestial pole offsets): not used by the IAU 1980 nutation, kept for later.
		// The first 1962 rows of C04 have 0.000000 for them: a defined value.
		if (number(f.value(cDX), a) && number(f.value(cDY), b))
		{
			r.hasDXDY = true;
			r.dx = a * nutToArcsec;
			r.dy = b * nutToArcsec;
			if (number(f.value(cSDX), c)) r.dxErr = c * nutToArcsec;
			if (number(f.value(cSDY), c)) r.dyErr = c * nutToArcsec;
		}

		// rows without any value (end of the finals files) are skipped
		if (!(r.hasPole || r.hasUT || r.hasLOD || r.hasNutation || r.hasDXDY))
			continue;

		if (finals)
		{
			r.poleStatus = status(f.at(cPoleType));
			r.utStatus = status(f.at(cUTType));
			r.nutationStatus = status(f.at(cNutType));
		}
		else
		{
			r.poleStatus = r.hasPole ? EOPReader::Status::Final : EOPReader::Status::Missing;
			r.utStatus = (r.hasUT || r.hasLOD) ? EOPReader::Status::Final : EOPReader::Status::Missing;
			r.nutationStatus = (r.hasNutation || r.hasDXDY) ? EOPReader::Status::Final : EOPReader::Status::Missing;
		}
		r.lodStatus = r.hasLOD ? r.utStatus : EOPReader::Status::Missing;
		r.dxdyStatus = r.hasDXDY ? r.nutationStatus : EOPReader::Status::Missing;
		out.append(r);
	}
	return true;
}

// (SS) 2026-11-06 JPL Horizons UT1-UTC (file written by horizons_ut1utc.py): lines starting with #, the header MJD;Date;JD;UT1-UTC,
// then MJD;1962-Jan-20;2437684.5;0.02340 (UT1-UTC in seconds). The comment
// "# EOP coverage: DATA-BASED 1962-JAN-20 TO 2026-NOV-05. PREDICTS-> 2027-JAN-31" gives the last day of the Horizons data: the values
// up to that day are final, the later ones predictions (all final if the comment is missing).
bool parseHorizons(const QStringList& lines, QVector<EOPReader::Record>& out, QString& error)
{
	static const QRegularExpression coverage(QStringLiteral("DATA-BASED\\s+\\S+\\s+TO\\s+(\\d{4})-([A-Za-z]{3})-(\\d{2})"));
	static const QStringList months = QStringLiteral("JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC").split(' ');
	double dataEnd = 1e30;
	bool header = false;
	for (int i = 0; i < lines.size(); ++i)
	{
		const QString line = lines.at(i).trimmed();
		if (line.isEmpty())
			continue;
		if (line.startsWith('#'))
		{
			const QRegularExpressionMatch m = coverage.match(line);
			if (m.hasMatch())
			{
				const int mon = months.indexOf(m.captured(2).toUpper()) + 1;
				const QDate d(m.captured(1).toInt(), mon, m.captured(3).toInt());
				if (d.isValid())
					dataEnd = double(d.toJulianDay() - 2400001);
			}
			continue;
		}
		if (!header)
		{
			header = true;     // MJD;Date;JD;UT1-UTC
			continue;
		}
		const QStringList f = line.split(';');
		if (f.size() < 4)
			continue;
		bool ok1, ok2;
		const double mjd = f.at(0).trimmed().toDouble(&ok1);
		const double ut = f.at(3).trimmed().toDouble(&ok2);
		if (!(ok1 && ok2))
		{
			error = QString("bad MJD or UT1-UTC on line %1").arg(i + 1);
			return false;
		}
		EOPReader::Record r;
		r.source = EOPReader::Format::HorizonsUT;
		r.mjd = mjd;
		const QDate d = QDate::fromJulianDay(qint64(std::floor(mjd)) + 2400001);
		r.year = d.year();
		r.month = d.month();
		r.day = d.day();
		r.hasUT = true;
		r.ut1utc = ut;
		r.utStatus = (mjd <= dataEnd + 1e-9) ? EOPReader::Status::Final : EOPReader::Status::Prediction;
		out.append(r);
	}
	return true;
}
}

EOPReader::Format EOPReader::detectFormat(const QString& firstLine)
{
	QString s = firstLine;
	if (s.startsWith(QChar(0xFEFF))) // byte order mark added by some editors
		s.remove(0, 1);
	s = s.trimmed();
	if (!s.startsWith(QStringLiteral("MJD;")))
		return Format::Unknown;
	// (SS) 2026-11-06 UT1-UTC of JPL Horizons: MJD;Date;JD;UT1-UTC (the IERS files have x_pole etc. in their header)
	if (s.startsWith(QStringLiteral("MJD;Date;JD;UT1-UTC")))
		return Format::HorizonsUT;
	// the finals files (standard series) have the Bulletin B columns, the C04 files (long term series) do not
	return s.contains(QStringLiteral("bulB/")) ? Format::Finals : Format::C04;
}

QString EOPReader::formatName(Format f)
{
	switch (f)
	{
		case Format::Finals: return QStringLiteral("IERS standard series (finals CSV)");
		case Format::C04:    return QStringLiteral("IERS long term series (EOP C04 CSV)");
		case Format::HorizonsUT: return QStringLiteral("JPL Horizons UT1-UTC");
		default:             return QStringLiteral("unknown");
	}
}

QString EOPReader::statusName(Status s)
{
	switch (s)
	{
		case Status::Final:      return QStringLiteral("final");
		case Status::Prediction: return QStringLiteral("prediction");
		case Status::Missing:    return QStringLiteral("-");
		default:                 return QStringLiteral("?");
	}
}

bool EOPReader::parseText(const QString& text, QVector<Record>& records, Format& format, QString& error)
{
	records.clear();
	error.clear();
	QStringList lines = text.split('\n');
	if (!lines.isEmpty() && lines.first().startsWith(QChar(0xFEFF)))
		lines[0].remove(0, 1);
	// (SS) 2026-11-06 the first lines may be comments starting with # (file of the Horizons UT1-UTC)
	int hdr = 0;
	while (hdr < lines.size() && lines.at(hdr).trimmed().startsWith('#'))
		++hdr;
	format = detectFormat(lines.value(hdr));

	bool ok = false;
	switch (format)
	{
		case Format::Finals:
		case Format::C04:        ok = parseCSV(hdr > 0 ? lines.mid(hdr) : lines, format, records, error); break;
		case Format::HorizonsUT: ok = parseHorizons(lines, records, error); break;
		default:
			error = QStringLiteral("unrecognized EOP file layout");
			return false;
	}
	if (!ok)
	{
		records.clear();
		return false;
	}
	if (records.isEmpty())
	{
		error = QStringLiteral("no EOP record found");
		return false;
	}

	// sort by MJD; for duplicated MJDs the later line wins
	std::stable_sort(records.begin(), records.end(), [](const Record& a, const Record& b) { return a.mjd < b.mjd; });
	QVector<Record> unique;
	unique.reserve(records.size());
	for (int i = 0; i < records.size(); ++i)
	{
		const Record& r = records.at(i);
		if (!unique.isEmpty() && std::fabs(unique.last().mjd - r.mjd) < 1e-9)
			unique.last() = r;
		else
			unique.append(r);
	}
	records = unique;
	return true;
}

bool EOPReader::parseFile(const QString& path, QVector<Record>& records, Format& format, QString& error)
{
	records.clear();
	format = Format::Unknown;
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
	{
		error = QString("cannot open %1: %2").arg(path, f.errorString());
		return false;
	}
	const QString text = QString::fromUtf8(f.readAll());
	return parseText(text, records, format, error);
}

void EOPReader::clear()
{
	recs.clear();
	for (int g = 0; g < 5; ++g)
		prevValid[g].clear();
	lastNutIndex = -1;
	nGaps = 0;
}

bool EOPReader::has(const Record& r, Group g)
{
	switch (g)
	{
		case Group::Pole:      return r.hasPole;
		case Group::UT:        return r.hasUT;
		case Group::LOD:       return r.hasLOD;
		case Group::Nutation:  return r.hasNutation;
		case Group::DXDY:      return r.hasDXDY;
	}
	return false;
}

EOPReader::Status EOPReader::statusOf(const Record& r, Group g)
{
	switch (g)
	{
		case Group::Pole:      return r.poleStatus;
		case Group::UT:        return r.utStatus;
		case Group::LOD:       return r.lodStatus;
		case Group::Nutation:  return r.nutationStatus;
		case Group::DXDY:      return r.dxdyStatus;
	}
	return Status::Unknown;
}

QString EOPReader::groupName(Group g)
{
	switch (g)
	{
		case Group::Pole:      return QStringLiteral("x_pole, y_pole");
		case Group::UT:        return QStringLiteral("UT1-UTC");
		case Group::LOD:       return QStringLiteral("LOD");
		case Group::Nutation:  return QStringLiteral("dPsi, dEps");
		case Group::DXDY:      return QStringLiteral("dX, dY");
	}
	return QString();
}

EOPReader::Span EOPReader::span(Group g) const
{
	Span s;
	for (int i = 0; i < recs.size(); ++i)
	{
		const Record& r = recs.at(i);
		if (!has(r, g))
			continue;
		if (!s.valid)
		{
			s.valid = true;
			s.firstMJD = r.mjd;
		}
		s.lastMJD = r.mjd;
		if (statusOf(r, g) == Status::Final)
		{
			s.hasFinal = true;
			s.lastFinalMJD = r.mjd;
		}
	}
	return s;
}

bool EOPReader::hasLast(Group g) const
{
	const QVector<int>& pv = prevValid[static_cast<int>(g)];
	return !pv.isEmpty() && pv.last() >= 0;
}

const EOPReader::Record& EOPReader::lastRecord(Group g) const
{
	return recs.at(prevValid[static_cast<int>(g)].last());
}

int EOPReader::dropAfter(Group g, double mjd)
{
	int n = 0;
	for (int i = 0; i < recs.size(); ++i)
	{
		Record& r = recs[i];
		if (r.mjd <= mjd + 1e-9 || !has(r, g))
			continue;
		switch (g)
		{
			case Group::Pole:     r.hasPole = false; r.xp = r.yp = r.xpErr = r.ypErr = 0.; r.poleStatus = Status::Missing; break;
			case Group::UT:       r.hasUT = false; r.ut1utc = r.ut1utcErr = 0.; r.utStatus = Status::Missing; break;
			case Group::LOD:      r.hasLOD = false; r.lod = r.lodErr = 0.; r.lodStatus = Status::Missing; break;
			case Group::Nutation: r.hasNutation = false; r.dpsi = r.deps = r.dpsiErr = r.depsErr = 0.; r.nutationStatus = Status::Missing; break;
			case Group::DXDY:     r.hasDXDY = false; r.dx = r.dy = r.dxErr = r.dyErr = 0.; r.dxdyStatus = Status::Missing; break;
		}
		++n;
	}
	if (n > 0)
		updateDerived();
	return n;
}

void EOPReader::dropNutation(QVector<Record>& records)
{
	for (int i = 0; i < records.size(); ++i)
	{
		Record& r = records[i];
		r.hasNutation = false;
		r.dpsi = r.deps = r.dpsiErr = r.depsErr = 0.;
	}
}

void EOPReader::updateDerived()
{
	for (int g = 0; g < 5; ++g)
	{
		QVector<int>& pv = prevValid[g];
		pv.resize(recs.size());
		int last = -1;
		for (int i = 0; i < recs.size(); ++i)
		{
			if (has(recs.at(i), static_cast<Group>(g)))
				last = i;
			pv[i] = last;
		}
	}
	lastNutIndex = recs.isEmpty() ? -1 : prevValid[static_cast<int>(Group::Nutation)].last();

	nGaps = 0;
	for (int i = 1; i < recs.size(); ++i)
		if (recs.at(i).mjd - recs.at(i - 1).mjd > 1.0 + 1e-6)
			++nGaps;
}

namespace
{
// New record over old record of the same MJD: the new values win, but a group of values (pole, UT1-UTC, LOD, nutation)
// that the new record does not have is kept from the old one (e.g. C04 values are never lost to an empty field)
EOPReader::Record combine(const EOPReader::Record& old, const EOPReader::Record& nw)
{
	EOPReader::Record r = nw;
	if (!nw.hasPole && old.hasPole)
	{
		r.hasPole = true; r.xp = old.xp; r.yp = old.yp; r.xpErr = old.xpErr; r.ypErr = old.ypErr; r.poleStatus = old.poleStatus;
	}
	if (!nw.hasUT && old.hasUT)
	{
		r.hasUT = true; r.ut1utc = old.ut1utc; r.ut1utcErr = old.ut1utcErr; r.utStatus = old.utStatus;
	}
	if (!nw.hasLOD && old.hasLOD)
	{
		r.hasLOD = true; r.lod = old.lod; r.lodErr = old.lodErr; r.lodStatus = old.lodStatus;
	}
	if (!nw.hasNutation && old.hasNutation)
	{
		r.hasNutation = true; r.dpsi = old.dpsi; r.deps = old.deps; r.dpsiErr = old.dpsiErr; r.depsErr = old.depsErr;
		r.nutationStatus = old.nutationStatus;
	}
	if (!nw.hasDXDY && old.hasDXDY)
	{
		r.hasDXDY = true; r.dx = old.dx; r.dy = old.dy; r.dxErr = old.dxErr; r.dyErr = old.dyErr; r.dxdyStatus = old.dxdyStatus;
	}
	// (SS) 2026-11-08 The UT1-UTC of JPL Horizons has no sigma: where it replaces a value of the IERS, the sigma of the IERS is kept (it is not used
	// by any computation; it stays as an example of the uncertainty of the IERS value, which is not that of the value of Horizons)
	if (nw.source == EOPReader::Format::HorizonsUT && nw.hasUT && old.hasUT)
		r.ut1utcErr = old.ut1utcErr;
	return r;
}

// keep the largest change
void track(EOPReader::Diff& d, double oldValue, double newValue, double mjd)
{
	const double diff = std::fabs(newValue - oldValue);
	++d.n;
	if (diff > d.max)
	{
		d.max = diff;
		d.mjd = mjd;
	}
}
}

EOPReader::LoadResult EOPReader::merge(const QVector<Record>& records, Format format, bool replaceAll)
{
	LoadResult res;
	res.format = format;
	res.nRecords = records.size();
	if (records.isEmpty())
	{
		res.error = QStringLiteral("no EOP record");
		return res;
	}
	res.firstMJD = records.first().mjd;
	res.lastMJD = records.last().mjd;

	if (replaceAll)
		recs.clear();

	for (const Record& r : records)
	{
		auto it = std::lower_bound(recs.begin(), recs.end(), r.mjd - 1e-9,
		                           [](const Record& rec, double mjd) { return rec.mjd < mjd; });
		if (it != recs.end() && std::fabs(it->mjd - r.mjd) < 1e-9)
		{
			if (it->hasPole && r.hasPole)
			{
				track(res.xpDiff, it->xp, r.xp, r.mjd);
				track(res.ypDiff, it->yp, r.yp, r.mjd);
			}
			if (it->hasUT && r.hasUT) track(res.ut1utcDiff, it->ut1utc, r.ut1utc, r.mjd);
			if (it->hasLOD && r.hasLOD) track(res.lodDiff, it->lod, r.lod, r.mjd);
			if (it->hasNutation && r.hasNutation)
			{
				track(res.dpsiDiff, it->dpsi, r.dpsi, r.mjd);
				track(res.depsDiff, it->deps, r.deps, r.mjd);
			}
			*it = combine(*it, r);  // the file merged last is the newer or more authoritative one, but missing values do not erase old ones
			++res.nReplaced;
		}
		else
		{
			recs.insert(it, r);
			++res.nAdded;
			if (r.poleStatus == Status::Prediction || r.utStatus == Status::Prediction || r.nutationStatus == Status::Prediction)
				++res.nPredictionAdded;
			else
				++res.nFinalAdded;
		}
	}
	updateDerived();
	res.ok = true;
	return res;
}

EOPReader::LoadResult EOPReader::loadFile(const QString& path, bool replaceAll)
{
	QVector<Record> records;
	Format fmt = Format::Unknown;
	QString err;
	if (!parseFile(path, records, fmt, err))
	{
		LoadResult res;
		res.format = fmt;
		res.error = err;
		return res;
	}
	return merge(records, fmt, replaceAll);
}

bool EOPReader::interpolate(double jdUTC, Interpolated& out) const
{
	out = Interpolated();
	if (recs.isEmpty())
		return false;

	const double mjd = jdUTC - 2400000.5;
	if (mjd < recs.first().mjd - 1e-9)
		return false;

	// r0: last record at or before the date, r1: the next one (none after the end)
	auto it = std::upper_bound(recs.begin(), recs.end(), mjd + 1e-9,
	                           [](double m, const Record& r) { return m < r.mjd; });
	const int i0 = int(it - recs.begin()) - 1;
	const Record& r0 = recs.at(i0);
	const Record* r1 = (i0 + 1 < recs.size()) ? &recs.at(i0 + 1) : nullptr;
	const bool onRecord = std::fabs(mjd - r0.mjd) < 1e-9;
	const double t = r1 ? (mjd - r0.mjd) / (r1->mjd - r0.mjd) : 0.;
	auto lerp = [t](double a, double b) { return a + t * (b - a); };

	Record& o = out.rec;
	o.mjd = mjd;
	o.year = r0.year; o.month = r0.month; o.day = r0.day;
	o.source = r0.source;
	out.coverage = (mjd > recs.last().mjd + 1e-9) ? Coverage::AfterEnd : Coverage::Inside;

	// How a group gets its value: exact record, interpolation, last earlier value (held), or nothing
	enum class Mode { None, Exact, Interp, Held };
	auto modeFor = [&](Group g, const Record*& src) -> Mode
	{
		if (has(r0, g) && onRecord)
		{
			src = &r0;
			return Mode::Exact;
		}
		if (has(r0, g) && r1 && has(*r1, g))
			return Mode::Interp;
		const int a = prevValid[static_cast<int>(g)].at(i0);
		if (a >= 0)
		{
			src = &recs.at(a);
			return Mode::Held;
		}
		return Mode::None;
	};

	const Record* s = nullptr;
	Mode m = modeFor(Group::Pole, s);
	if (m != Mode::None)
	{
		o.hasPole = true;
		out.poleHeld = (m == Mode::Held);
		if (m == Mode::Interp)
		{
			o.xp = lerp(r0.xp, r1->xp); o.yp = lerp(r0.yp, r1->yp);
			o.xpErr = lerp(r0.xpErr, r1->xpErr); o.ypErr = lerp(r0.ypErr, r1->ypErr);
			o.poleStatus = r0.poleStatus;
		}
		else
		{
			o.xp = s->xp; o.yp = s->yp; o.xpErr = s->xpErr; o.ypErr = s->ypErr; o.poleStatus = s->poleStatus;
		}
	}

	m = modeFor(Group::UT, s);
	if (m != Mode::None)
	{
		o.hasUT = true;
		out.utHeld = (m == Mode::Held);
		if (m == Mode::Interp)
		{
			// (SS) 2026-11-05 Same logic as InterpolateEOP() of the standalone program. UT1-UTC is discontinuous wherever UTC itself
			// is stepped (1 s leap seconds since 1972, steps of 0.1 s and 0.05 s in 1962-1972), so interpolating it linearly across
			// such a day is wrong for the whole day before the step. UT1-TAI is continuous: it is interpolated and TAI-UTC is added back
			// at the requested UTC instant, so the step falls exactly at 0h UTC, as in JPL Horizons. The drift of UTC of 1962-1972 is
			// included in eraDat().
			double dat0 = 0., dat1 = 0., datT = 0.;
			if (taiMinusUtc(r0.mjd, dat0) && taiMinusUtc(r1->mjd, dat1) && taiMinusUtc(mjd, datT))
			{
				o.ut1utc = lerp(r0.ut1utc - dat0, r1->ut1utc - dat1) + datT;
			}
			else
			{
				// TAI-UTC not available (invalid date, error of eraDat()): previous behaviour. A change larger than 0.025 s between two days is a step,
				// a multiple of 0.05 s, to remove first.
				double u1 = r1->ut1utc;
				const double d = u1 - r0.ut1utc;
				if (std::fabs(d) > 0.025)
					u1 -= 0.05 * std::round(d / 0.05);
				o.ut1utc = lerp(r0.ut1utc, u1);
			}
			o.ut1utcErr = lerp(r0.ut1utcErr, r1->ut1utcErr);
			o.utStatus = r0.utStatus;
		}
		else
		{
			o.ut1utc = s->ut1utc; o.ut1utcErr = s->ut1utcErr; o.utStatus = s->utStatus;
		}
	}

	m = modeFor(Group::LOD, s);
	if (m != Mode::None)
	{
		o.hasLOD = true;
		out.lodHeld = (m == Mode::Held);
		if (m == Mode::Interp)
		{
			o.lod = lerp(r0.lod, r1->lod);
			o.lodErr = lerp(r0.lodErr, r1->lodErr);
			o.lodStatus = r0.lodStatus;
		}
		else
		{
			o.lod = s->lod; o.lodErr = s->lodErr; o.lodStatus = s->lodStatus;
		}
	}

	m = modeFor(Group::Nutation, s);
	if (m != Mode::None)
	{
		o.hasNutation = true;
		out.nutationHeld = (m == Mode::Held);
		if (m == Mode::Interp)
		{
			o.dpsi = lerp(r0.dpsi, r1->dpsi); o.deps = lerp(r0.deps, r1->deps);
			o.dpsiErr = lerp(r0.dpsiErr, r1->dpsiErr); o.depsErr = lerp(r0.depsErr, r1->depsErr);
			o.nutationStatus = r0.nutationStatus;
		}
		else
		{
			o.dpsi = s->dpsi; o.deps = s->deps; o.dpsiErr = s->dpsiErr; o.depsErr = s->depsErr; o.nutationStatus = s->nutationStatus;
		}
	}

	m = modeFor(Group::DXDY, s);
	if (m != Mode::None)
	{
		o.hasDXDY = true;
		out.dxdyHeld = (m == Mode::Held);
		if (m == Mode::Interp)
		{
			o.dx = lerp(r0.dx, r1->dx); o.dy = lerp(r0.dy, r1->dy);
			o.dxErr = lerp(r0.dxErr, r1->dxErr); o.dyErr = lerp(r0.dyErr, r1->dyErr);
			o.dxdyStatus = r0.dxdyStatus;
		}
		else
		{
			o.dx = s->dx; o.dy = s->dy; o.dxErr = s->dxErr; o.dyErr = s->dyErr; o.dxdyStatus = s->dxdyStatus;
		}
	}
	return true;
}

QVector<EOPReader::Record> EOPReader::recordsAround(double mjd, int before, int after) const
{
	QVector<Record> res;
	if (recs.isEmpty() || mjd < recs.first().mjd - 1e-9)
		return res;
	auto it = std::upper_bound(recs.begin(), recs.end(), mjd + 1e-9,
	                           [](double m, const Record& r) { return m < r.mjd; });
	const int i0 = int(it - recs.begin()) - 1; // last record at or before mjd
	const int from = std::max(0, i0 - std::max(0, before));
	const int to = std::min(int(recs.size()) - 1, i0 + std::max(0, after));
	for (int i = from; i <= to; ++i)
		res.append(recs.at(i));
	return res;
}

QVector<EOPReader::Record> EOPReader::recordsBetween(double mjd0, double mjd1, int maxRecords) const
{
	QVector<Record> res;
	auto it = std::lower_bound(recs.begin(), recs.end(), mjd0 - 1e-9,
	                           [](const Record& r, double m) { return r.mjd < m; });
	for (; it != recs.end() && it->mjd <= mjd1 + 1e-9; ++it)
	{
		if (maxRecords > 0 && res.size() >= maxRecords)
			break;
		res.append(*it);
	}
	return res;
}
