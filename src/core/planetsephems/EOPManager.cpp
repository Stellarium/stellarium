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
bool mergeBefore(const EOPManager::FileInfo& a, const EOPManager::FileInfo& b, EOPManager::Model preferred)
{
	if (a.isLongTerm() != b.isLongTerm())
		return a.isLongTerm();
	const bool pa = (a.model == preferred), pb = (b.model == preferred);
	if (pa != pb)
		return pb;
	return a.generated != b.generated ? a.generated < b.generated : a.fileName < b.fileName;
}
}

QStringList EOPManager::defaultSelection() const
{
	QList<FileInfo> chosen;

	// long term files: for each model the file that reaches the latest date and has the offsets of that model
	// (dPsi, dEps for IAU 1980, dX, dY for IAU 2000A). A newer series thus replaces an older one as soon as it goes further.
	const Model models[] = {Model::IAU1980, Model::IAU2000A};
	for (Model m : models)
	{
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
	}

	// standard files: the newest download of each series (finals.all, finals2000A.all)
	QHash<QString, FileInfo> newest;
	for (const FileInfo& fi : infos)
	{
		if (!fi.valid || fi.isLongTerm() || fi.model == Model::Unknown)
			continue;
		if (!newest.contains(fi.key) || mergeBefore(newest.value(fi.key), fi, preferred))
			newest.insert(fi.key, fi);
	}
	chosen << newest.values();

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
QString EOPManager::describeMerge(const FileInfo& fi, const EOPReader::LoadResult& r) const
{
	QString s = QString("Using EOP file: %1 [%2, %3]\n  %4 EOP records read\n")
	            .arg(fi.fileName, EOPReader::formatName(fi.format), modelName(fi.model)).arg(r.nRecords);
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

bool EOPManager::reload()
{
	eop.clear();
	report.clear();

	// merge order: long term files first, then by download date (oldest first), so the newest data replace the older ones
	QList<FileInfo> todo;
	for (const QString& n : selectedFiles())
	{
		const FileInfo fi = fileInfo(n);
		if (fi.valid)
			todo << fi;
	}
	std::stable_sort(todo.begin(), todo.end(), [this](const FileInfo& a, const FileInfo& b) { return mergeBefore(a, b, preferred); });

	for (const FileInfo& fi : todo)
	{
		const EOPReader::LoadResult r = eop.merge(parsed.value(fi.fileName), fi.format, false);
		report += describeMerge(fi, r);
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
