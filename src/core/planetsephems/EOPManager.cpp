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

#include "EOPManager.hpp"

#include <QDir>
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <QDate>

EOPManager::EOPManager(const QString& dir, QObject* parent)
	: QObject(parent)
	, dirPath(dir)
{
}

QDate EOPManager::generationDateFromName(const QString& fileName)
{
	static const QRegularExpression re("_(\\d{2})(\\d{2})(\\d{2})(?:\\.[^.]*)?$");
	const QRegularExpressionMatch m = re.match(fileName);
	if (!m.hasMatch())
		return QDate();
	return QDate(2000 + m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt()); // invalid if month/day are not
}

QString EOPManager::keyFromName(const QString& fileName)
{
	QString base = QFileInfo(fileName).completeBaseName();
	static const QRegularExpression re("_\\d{6}$");
	base.remove(re);
	return base;
}

bool EOPManager::isOlderThan(const QString& key, const QDate& today, int days) const
{
	QDate newest;
	for (const FileInfo& fi : infos)
	{
		if (!fi.valid || fi.key != key)
			continue;
		QDate d = fi.generated;
		if (!d.isValid())
			d = QFileInfo(QDir(dirPath).filePath(fi.fileName)).lastModified().date();
		if (!newest.isValid() || d > newest)
			newest = d;
	}
	return !newest.isValid() || newest.daysTo(today) >= days;
}

EOPManager::Model EOPManager::modelFromName(const QString& fileName)
{
	const QString key = keyFromName(fileName).toLower();
	if (key.startsWith("finals2000a"))
		return Model::IAU2000A;
	if (key.startsWith("finals"))
		return Model::IAU1980;
	if (key.contains("iau1980"))
		return Model::IAU1980;
	if (key.contains("iau2000"))
		return Model::IAU2000A;
	if (key.startsWith("eopc04_14"))
		return Model::IAU1980;
	if (key.startsWith("eopc04_20"))
		return Model::IAU2000A;
	return Model::Unknown;
}

QString EOPManager::modelName(Model m)
{
	switch (m)
	{
		case Model::IAU1980:  return QStringLiteral("IAU 1980 nutation model");
		case Model::IAU2000A: return QStringLiteral("IAU 2006/2000A, dPsi and dEps not used");
		default:              return QStringLiteral("reference model unknown");
	}
}

QStringList EOPManager::rescan()
{
	infos.clear();
	parsed.clear();

	const QDir d(dirPath);
	const QFileInfoList entries = d.entryInfoList(QStringList() << "*.csv", QDir::Files, QDir::Name);
	QStringList valid;
	for (const QFileInfo& e : entries)
	{
		FileInfo fi;
		fi.fileName = e.fileName();
		fi.generated = generationDateFromName(fi.fileName);
		fi.key = keyFromName(fi.fileName);

		QVector<EOPReader::Record> recs;
		EOPReader::Format fmt = EOPReader::Format::Unknown;
		QString err;
		if (EOPReader::parseFile(e.absoluteFilePath(), recs, fmt, err))
		{
			fi.valid = true;
			fi.format = fmt;
			fi.model = modelFromName(fi.fileName);
			if (fi.model == Model::IAU2000A)
				EOPReader::dropNutation(recs);   // relative to IAU 2000A, not the IAU 1980 dPsi and dEps
			fi.nRecords = recs.size();
			fi.firstMJD = recs.first().mjd;
			fi.lastMJD = recs.last().mjd;
			for (int i = 0; i < recs.size(); ++i)
			{
				fi.hasNutation = fi.hasNutation || recs.at(i).hasNutation;
				fi.hasDXDY = fi.hasDXDY || recs.at(i).hasDXDY;
			}
			parsed.insert(fi.fileName, recs);
			valid << fi.fileName;
		}
		else
		{
			fi.error = err;
		}
		infos.append(fi);
	}

	// keep the explicit selection, minus the files that disappeared
	if (explicitSelection)
	{
		QStringList kept;
		for (int i = 0; i < selection.size(); ++i)
			if (parsed.contains(selection.at(i)))
				kept << selection.at(i);
		selection = kept;
	}
	emit filesRescanned();
	return valid;
}

EOPManager::FileInfo EOPManager::fileInfo(const QString& fileName) const
{
	for (const FileInfo& fi : infos)
		if (fi.fileName == fileName)
			return fi;
	return FileInfo();
}

namespace
{
// merge order: long term (C04) files first, then the standard ones; in each group the files of the preferred model last
// (their values win), then from the oldest to the newest download, ties by name
// (SS) 2026-11-06 the Horizons UT1-UTC is merged after all the IERS files
int mergeRank(const EOPManager::FileInfo& f) { return f.isLongTerm() ? 0 : (f.isOverlay() ? 2 : 1); }

bool mergeBefore(const EOPManager::FileInfo& a, const EOPManager::FileInfo& b, EOPManager::Model preferred)
{
	if (mergeRank(a) != mergeRank(b))
		return mergeRank(a) < mergeRank(b);
	const bool pa = (a.model == preferred), pb = (b.model == preferred);
	if (pa != pb)
		return pb;
	return a.generated != b.generated ? a.generated < b.generated : a.fileName < b.fileName;
}
}

namespace
{
//! (SS) 2026-11-06 UT1-UTC of the last record of a file (the last value of the Horizons UT1-UTC, held afterwards)
double recsLastUT(const QVector<EOPReader::Record>& recs)
{
	return recs.isEmpty() ? 0. : recs.last().ut1utc;
}

//! (SS) 2026-11-05 Number of values of each group (EOPReader::Group order) taken out of a standard file by filterLongTerm()
struct FilterStats
{
	int n[5] = {0, 0, 0, 0, 0};
	int droppedRecords = 0;     //!< records left with no value at all
	int total() const { return n[0] + n[1] + n[2] + n[3] + n[4]; }
};

//! (SS) 2026-11-05 Rule A: take out of the records of a standard file the values of days up to the last final value of the same group
//! in the long term files (cutoff[group], in MJD). A group has its own cutoff (pole and UT1-UTC can go further than the others).
FilterStats filterLongTerm(QVector<EOPReader::Record>& recs, const double cutoff[5])
{
	FilterStats st;
	QVector<EOPReader::Record> kept;
	kept.reserve(recs.size());
	for (EOPReader::Record r : recs)
	{
		const double mjd = r.mjd - 1e-9;   // tolerance: a record on the cutoff day itself is protected too
		if (r.hasPole && mjd <= cutoff[int(EOPReader::Group::Pole)])      { r.hasPole = false;      ++st.n[int(EOPReader::Group::Pole)]; }
		if (r.hasUT && mjd <= cutoff[int(EOPReader::Group::UT)])          { r.hasUT = false;        ++st.n[int(EOPReader::Group::UT)]; }
		if (r.hasLOD && mjd <= cutoff[int(EOPReader::Group::LOD)])        { r.hasLOD = false;       ++st.n[int(EOPReader::Group::LOD)]; }
		if (r.hasNutation && mjd <= cutoff[int(EOPReader::Group::Nutation)]) { r.hasNutation = false; ++st.n[int(EOPReader::Group::Nutation)]; }
		if (r.hasDXDY && mjd <= cutoff[int(EOPReader::Group::DXDY)])      { r.hasDXDY = false;      ++st.n[int(EOPReader::Group::DXDY)]; }
		if (r.hasPole || r.hasUT || r.hasLOD || r.hasNutation || r.hasDXDY)
			kept.append(r);
		else
			++st.droppedRecords;
	}
	recs = kept;
	return st;
}

QString describeFilter(const FilterStats& st)
{
	const EOPReader::Group groups[] = {EOPReader::Group::Pole, EOPReader::Group::UT, EOPReader::Group::LOD,
	                                   EOPReader::Group::Nutation, EOPReader::Group::DXDY};
	QStringList parts;
	for (EOPReader::Group g : groups)
		if (st.n[int(g)] > 0)
			parts << QString("%1 %2").arg(EOPReader::groupName(g)).arg(st.n[int(g)]);
	return QString("  Values not used, the final values of the long term series are kept: %1 (%2)\n")
	        .arg(st.total()).arg(parts.join(", "));
}
}

QStringList EOPManager::defaultSelection() const
{
	// (SS) 2026-11-05 Only the files of one nutation model are merged by default: the preferred one (IAU 1980, as JPL Horizons, unless
	// the user chose IAU 2000A in the EOP dialog). Files of the other model are only used when selected explicitly, or when the folder
	// has no valid file of the preferred model (so that some EOP are available anyway).
	auto pick = [this](Model m)
	{
		QList<FileInfo> chosen;

		// long term file of that model: the one that reaches the latest date and has the offsets of that model
		// (dPsi, dEps for IAU 1980, dX, dY for IAU 2000A). A newer series thus replaces an older one as soon as it goes further.
		const FileInfo* best = nullptr;
		for (const FileInfo& fi : infos)
		{
			if (!fi.valid || !fi.isLongTerm() || fi.model != m || !(m == Model::IAU1980 ? fi.hasNutation : fi.hasDXDY))
				continue;
			if (!best || fi.lastMJD > best->lastMJD
			    || (fi.lastMJD == best->lastMJD && mergeBefore(*best, fi, preferred)))
				best = &fi;
		}
		if (best)
			chosen << *best;

		// standard files of that model: the newest download of each series
		QHash<QString, FileInfo> newest;
		for (const FileInfo& fi : infos)
		{
			if (!fi.valid || fi.isLongTerm() || fi.model != m)
				continue;
			if (!newest.contains(fi.key) || mergeBefore(newest.value(fi.key), fi, preferred))
				newest.insert(fi.key, fi);
		}
		chosen << newest.values();
		return chosen;
	};

	Model used = (preferred == Model::IAU2000A) ? Model::IAU2000A : Model::IAU1980;
	QList<FileInfo> chosen = pick(used);
	if (chosen.isEmpty())
	{
		used = (used == Model::IAU2000A) ? Model::IAU1980 : Model::IAU2000A;
		chosen = pick(used);
	}
	// (SS) 2026-11-06 The UT1-UTC of JPL Horizons goes with the IAU 1980 files (Horizons uses the IAU 1980 nutation): the newest
	// file is added. It is not in the default selection of the IAU 2000A model; it can be selected in the Files table.
	if (used == Model::IAU1980)
	{
		const FileInfo* overlay = nullptr;
		for (const FileInfo& fi : infos)
			if (fi.valid && fi.isOverlay() && (!overlay || mergeBefore(*overlay, fi, preferred)))
				overlay = &fi;
		if (overlay)
			chosen << *overlay;
	}

	std::sort(chosen.begin(), chosen.end(), [this](const FileInfo& a, const FileInfo& b) { return mergeBefore(a, b, preferred); });
	QStringList res;
	for (const FileInfo& fi : chosen)
		res << fi.fileName;
	return res;
}

void EOPManager::setSelectedFiles(const QStringList& names)
{
	if (names.isEmpty())
	{
		explicitSelection = false;
		selection.clear();
		return;
	}
	explicitSelection = true;
	selection.clear();
	for (const QString& n : names)
		if (!selection.contains(n))
			selection << n;
}

QStringList EOPManager::selectedFiles() const
{
	if (!explicitSelection)
		return defaultSelection();
	QStringList res;
	for (const QString& n : selection)
		if (parsed.contains(n))
			res << n;
	return res;
}

namespace
{
QString diffLine(const QString& name, const QString& unit, const EOPReader::Diff& d)
{
	if (d.n == 0)
		return QString();
	return QString("    %1: %2 %3 (MJD %4, %5 records compared)\n")
	        .arg(name).arg(d.max, 0, 'g', 4).arg(unit).arg(d.mjd, 0, 'f', 0).arg(d.n);
}
}

// Summary of one merged file, in the style of the standalone program log
QString EOPManager::describeMerge(const FileInfo& fi, const EOPReader::LoadResult& r, int nInFile) const
{
	// (SS) 2026-11-06 records of the file, and how many were used when values were taken out (rule A)
	const QString model = fi.isOverlay() ? QStringLiteral("UT1-UTC only") : modelName(fi.model);
	QString s = QString("Using EOP file: %1 [%2, %3]\n  %4 EOP records read").arg(fi.fileName, EOPReader::formatName(fi.format), model).arg(nInFile);
	if (r.nRecords != nInFile)
		s += QString(", %1 used").arg(r.nRecords);
	s += "\n";
	s += QString("  Existing records replaced : %1\n  Final records added       : %2\n  Prediction records added  : %3\n")
	     .arg(r.nReplaced).arg(r.nFinalAdded).arg(r.nPredictionAdded);
	if (r.nReplaced > 0)
	{
		// how much the file changed the values already in memory: a large change in a final region deserves a look
		QString d = diffLine(QStringLiteral("x_pole"), QStringLiteral("arcsec"), r.xpDiff) + diffLine(QStringLiteral("y_pole"), QStringLiteral("arcsec"), r.ypDiff)
		          + diffLine(QStringLiteral("UT1-UTC"), QStringLiteral("s"), r.ut1utcDiff) + diffLine(QStringLiteral("LOD"), QStringLiteral("s"), r.lodDiff)
		          + diffLine(QStringLiteral("dPsi"), QStringLiteral("arcsec"), r.dpsiDiff) + diffLine(QStringLiteral("dEps"), QStringLiteral("arcsec"), r.depsDiff);
		if (!d.isEmpty())
			s += "  Largest change made to the replaced records:\n" + d;
	}
	return s + "\n";
}

// Where each group of values starts, stops being final and stops being predicted
QString EOPManager::describeCoverage() const
{
	auto day = [](double mjd)
	{
		return QString("%1 (MJD %2)").arg(QDate::fromJulianDay(qint64(std::floor(mjd)) + 2400001).toString("yyyy-MM-dd")).arg(mjd, 0, 'f', 0);
	};
	QString s = QStringLiteral("EOP coverage of the merged table (the last value is kept for all later dates):\n");
	const EOPReader::Group groups[] = {EOPReader::Group::Pole, EOPReader::Group::UT, EOPReader::Group::LOD,
	                                   EOPReader::Group::Nutation, EOPReader::Group::DXDY};
	for (EOPReader::Group g : groups)
	{
		const EOPReader::Span sp = eop.span(g);
		QString line = EOPReader::groupName(g).leftJustified(15) + ": ";
		if (!sp.valid)
			line += QStringLiteral("no value");
		else
		{
			line += "from " + day(sp.firstMJD);
			line += sp.hasFinal ? ", final to " + day(sp.lastFinalMJD) : QStringLiteral(", no final value");
			line += sp.hasPrediction() ? ", prediction to " + day(sp.lastMJD) : QStringLiteral(", no prediction");
		}
		s += "  " + line + "\n";
	}
	return s;
}

// (SS) 2026-10-07 Last valid value of each group of EOP. Each group has its own last date: pole and UT1-UTC go further than
// LOD, dPsi and dEps. These are the values that are kept for all later dates (see EOPReader::interpolate()).
QString EOPManager::describeLastValues() const
{
	auto day = [](double mjd)
	{
		return QString("%1 (MJD %2)").arg(QDate::fromJulianDay(qint64(std::floor(mjd)) + 2400001).toString("yyyy-MM-dd")).arg(mjd, 0, 'f', 0);
	};
	QString s = QStringLiteral("Last valid value of each EOP (kept constant for all later dates):\n");
	const EOPReader::Group groups[] = {EOPReader::Group::Pole, EOPReader::Group::UT, EOPReader::Group::LOD,
	                                   EOPReader::Group::Nutation, EOPReader::Group::DXDY};
	for (EOPReader::Group g : groups)
	{
		QString line = EOPReader::groupName(g).leftJustified(15) + ": ";
		if (!eop.hasLast(g))
		{
			s += "  " + line + "no value\n";
			continue;
		}
		const EOPReader::Record& r = eop.lastRecord(g);
		switch (g)
		{
			case EOPReader::Group::Pole:
				line += day(r.mjd) + ", " + EOPReader::statusName(r.poleStatus) + ": "
				        + QString("xp = %1 arcsec, yp = %2 arcsec").arg(r.xp, 0, 'f', 6).arg(r.yp, 0, 'f', 6);
				break;
			case EOPReader::Group::UT:
				line += day(r.mjd) + ", " + EOPReader::statusName(r.utStatus) + ": "
				        + QString("UT1 - UTC = %1 sec").arg(r.ut1utc, 0, 'f', 7);
				break;
			case EOPReader::Group::LOD:
				line += day(r.mjd) + ", " + EOPReader::statusName(r.lodStatus) + ": "
				        + QString("LOD = %1 sec").arg(r.lod, 0, 'f', 7);
				break;
			case EOPReader::Group::Nutation:
				line += day(r.mjd) + ", " + EOPReader::statusName(r.nutationStatus) + ": "
				        + QString("dPsi = %1 arcsec, dEps = %2 arcsec").arg(r.dpsi, 0, 'f', 6).arg(r.deps, 0, 'f', 6);
				break;
			case EOPReader::Group::DXDY:
				line += day(r.mjd) + ", " + EOPReader::statusName(r.dxdyStatus) + ": "
				        + QString("dX = %1 arcsec, dY = %2 arcsec").arg(r.dx, 0, 'f', 6).arg(r.dy, 0, 'f', 6);
				break;
		}
		s += "  " + line + "\n";
	}
	return s;
}

QStringList EOPManager::removeOldVersions(const QStringList& justWritten, int keep)
{
	QStringList removed;
	if (keep < 1)
		return removed;

	// series (names without the date) of the files just written
	QSet<QString> keys;
	for (const QString& n : justWritten)
		if (generationDateFromName(n).isValid())
			keys.insert(keyFromName(n));

	struct Item { QString name; QDate date; };
	QHash<QString, QList<Item>> bySeries;
	const QDir d(dirPath);
	const QFileInfoList entries = d.entryInfoList(QStringList() << "*.csv", QDir::Files);
	for (const QFileInfo& e : entries)
	{
		const QDate g = generationDateFromName(e.fileName());
		if (!g.isValid())
			continue;                       // no date in the name: not ours
		const QString key = keyFromName(e.fileName());
		if (keys.contains(key))
			bySeries[key].append(Item{e.fileName(), g});
	}

	for (auto it = bySeries.begin(); it != bySeries.end(); ++it)
	{
		QList<Item>& items = it.value();
		std::sort(items.begin(), items.end(), [](const Item& a, const Item& b)
		{
			return a.date != b.date ? a.date > b.date : a.name > b.name;      // newest first
		});
		for (int i = keep; i < items.size(); ++i)
		{
			const QString& name = items.at(i).name;
			if (justWritten.contains(name) || (explicitSelection && selection.contains(name)))
				continue;
			if (QFile::remove(d.filePath(name)))
				removed << name;
		}
	}
	removed.sort();
	return removed;
}

bool EOPManager::reload()
{
	eop.clear();
	report.clear();
	horizonsLastMJD = -1.;

	// merge order: long term files first, then by download date (oldest first), so the newest data replace the older ones
	QList<FileInfo> todo;
	for (const QString& n : selectedFiles())
	{
		const FileInfo fi = fileInfo(n);
		if (fi.valid)
			todo << fi;
	}
	std::stable_sort(todo.begin(), todo.end(), [this](const FileInfo& a, const FileInfo& b) { return mergeBefore(a, b, preferred); });

	// (SS) 2026-11-05 Rule A (see finalsOverrideLongTerm()): the long term files are merged first (see mergeBefore()); after each of them the
	// last final MJD of every group is the cutoff up to which the standard files do not replace values.
	double cutoff[5];
	for (double& c : cutoff)
		c = -1e30;
	for (const FileInfo& fi : todo)
	{
		QVector<EOPReader::Record> recs = parsed.value(fi.fileName);
		const int nInFile = recs.size();
		FilterStats fs;
		if (!finalsOverride && !fi.isLongTerm() && !fi.isOverlay())
			fs = filterLongTerm(recs, cutoff);
		if (recs.isEmpty())
		{
			report += QString("Using EOP file: %1 [%2, %3]\n  No value used: all its values are in the period of the final values of the long term series\n\n")
			          .arg(fi.fileName, EOPReader::formatName(fi.format), modelName(fi.model));
			continue;
		}
		const EOPReader::LoadResult r = eop.merge(recs, fi.format, false);
		report += describeMerge(fi, r, nInFile);
		if (fs.total() > 0)
			report.insert(report.size() - 1, describeFilter(fs));   // before the empty line that ends the block of the file
		if (fi.isOverlay())
		{
			// (SS) 2026-11-06 Horizons keeps its last UT1-UTC for all later dates: the UT1-UTC predictions of the other files after the
			// last day of the overlay are removed, so that the last value of the overlay is the one held
			const int removed = eop.dropAfter(EOPReader::Group::UT, fi.lastMJD);
			horizonsLastMJD = fi.lastMJD;
			auto day = [](double mjd) { return QDate::fromJulianDay(qint64(std::floor(mjd)) + 2400001).toString("yyyy-MM-dd"); };
			report.insert(report.size() - 1,
			              QString("  UT1-UTC of every day from %1 to %2 comes from this file; %3 UT1-UTC value(s) of other files after %2 removed,\n"
			                      "  the last value (%4 s) is held for all later dates\n")
			              .arg(day(fi.firstMJD), day(fi.lastMJD)).arg(removed).arg(recsLastUT(parsed.value(fi.fileName)), 0, 'f', 5));
		}
		if (fi.isLongTerm())
		{
			const EOPReader::Group groups[] = {EOPReader::Group::Pole, EOPReader::Group::UT, EOPReader::Group::LOD,
			                                   EOPReader::Group::Nutation, EOPReader::Group::DXDY};
			for (EOPReader::Group g : groups)
			{
				const EOPReader::Span sp = eop.span(g);
				if (sp.hasFinal && sp.lastFinalMJD > cutoff[int(g)])
					cutoff[int(g)] = sp.lastFinalMJD;
			}
		}
	}
	if (todo.isEmpty())
		report = QStringLiteral("No EOP file selected\n");
	else
	{
		if (eop.gapCount() > 0)
			report += QString("\nWarning: %1 gap(s) of more than one day in the merged EOP table\n").arg(eop.gapCount());
		report += describeCoverage() + "\n";
		report += describeLastValues();
	}

	emit reloaded();
	return !eop.isEmpty();
}

bool EOPManager::getNutationCorrections(double jdUTC, double& dpsiArcsec, double& depsArcsec) const
{
	EOPReader::Interpolated v;
	if (!eop.interpolate(jdUTC, v) || !v.rec.hasNutation)
		return false;
	dpsiArcsec = v.rec.dpsi;
	depsArcsec = v.rec.deps;
	return true;
}

QVector<EOPReader::Record> EOPManager::table(double jdUTC, int before, int after) const
{
	return eop.recordsAround(jdUTC - 2400000.5, before, after);
}

QString EOPManager::modelKey(Model m)
{
	switch (m)
	{
		case Model::IAU1980:  return QStringLiteral("IAU1980");
		case Model::IAU2000A: return QStringLiteral("IAU2000A");
		default:              return QStringLiteral("Unknown");
	}
}

EOPManager::Model EOPManager::modelFromKey(const QString& key)
{
	if (key.compare("IAU1980", Qt::CaseInsensitive) == 0)
		return Model::IAU1980;
	if (key.compare("IAU2000A", Qt::CaseInsensitive) == 0)
		return Model::IAU2000A;
	return Model::Unknown;
}

QStringList EOPManager::mergeOrder() const
{
	QList<FileInfo> todo;
	const QStringList names = selectedFiles();
	for (const QString& n : names)
	{
		const FileInfo fi = fileInfo(n);
		if (fi.valid)
			todo << fi;
	}
	std::stable_sort(todo.begin(), todo.end(), [this](const FileInfo& a, const FileInfo& b) { return mergeBefore(a, b, preferred); });
	QStringList res;
	for (const FileInfo& fi : todo)
		res << fi.fileName;
	return res;
}
