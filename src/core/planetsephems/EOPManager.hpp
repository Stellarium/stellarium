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

#ifndef EOPMANAGER_HPP
#define EOPMANAGER_HPP

#include "EOPReader.hpp"

#include <QObject>
#include <QDate>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

//! (SS) 2026-10-07 Manager of the EOP files (*.csv, as downloaded from the IERS) found in the "eop" folder of the Stellarium user directory.
//!
//! Files are never selected by hard-coded names: each one is parsed, its layout, coverage and (when the name ends with
//! _YYMMDD before the extension, e.g. finals.all_261007.csv) its download date are recorded. The selected files are merged in this order:
//! long term (C04) files first, then the standard (finals) files, each group from the oldest to the newest date, so that the newest data win.
//! Like BSPManager, this class does not use StelApp: StelCore owns it and stores the selection in the configuration.
class EOPManager : public QObject
{
	Q_OBJECT
public:
	//! Reference model of the celestial pole offsets of a file. IAU 1980 files (finals.all, EOP 14 C04) give dPsi and dEps
	//! for the IAU 1980 nutation model, as used by JPL Horizons. IAU 2000A files (finals2000A.all, EOP 20 C04) give dX and dY
	//! (and, in the dPsi_dEps files, the same offsets as dPsi and dEps relative to IAU 2006/2000A, which are not used).
	enum class Model { Unknown, IAU1980, IAU2000A };

	struct FileInfo
	{
		QString fileName;                //!< name inside the eop folder
		bool valid = false;              //!< could be parsed
		QString error;                   //!< reason when not valid
		EOPReader::Format format = EOPReader::Format::Unknown;
		Model model = Model::Unknown;    //!< from the name of the file
		int nRecords = 0;
		double firstMJD = 0., lastMJD = 0.;
		QDate generated;                 //!< from the _YYMMDD suffix of the name, invalid when absent
		QString key;                     //!< name without the _YYMMDD suffix and extension: files with the same key are successive versions
		bool hasNutation = false;        //!< some records have dPsi, dEps (IAU 1980 file)
		bool hasDXDY = false;            //!< some records have dX, dY (IAU 2000 file)
		//! long term series (EOP C04): merged first, the standard series (finals) then replace its values
		bool isLongTerm() const { return format == EOPReader::Format::C04; }
	};

	//! @param dir folder holding the EOP files (created by the caller)
	explicit EOPManager(const QString& dir, QObject* parent = Q_NULLPTR);

	QString directory() const { return dirPath; }

	//! Parse again the files of the folder. The current selection is kept (missing files are dropped), nothing is merged.
	//! @return names of the valid files
	QStringList rescan();
	QList<FileInfo> files() const { return infos; }
	//! @return the file info or an invalid one when the name is unknown
	FileInfo fileInfo(const QString& fileName) const;

	//! Default files: of the long term (C04) files, the one reaching the latest date for IAU 1980 (dPsi, dEps) and the one for
	//! IAU 2000 (dX, dY); of the standard (finals) files, the newest download of each series. Empty if no valid file.
	QStringList defaultSelection() const;
	//! Names of the files to merge. An empty list means defaultSelection(). Unknown names are ignored.
	void setSelectedFiles(const QStringList& names);
	//! Files that reload() merges: the explicit selection or, if none was set, the default one.
	QStringList selectedFiles() const;
	bool isUsingDefaultSelection() const { return !explicitSelection; }

	//! Model whose files are merged last, so that their pole, UT1-UTC and LOD values win. Default IAU 1980 (JPL Horizons).
	Model preferredModel() const { return preferred; }
	void setPreferredModel(Model m) { preferred = m; }

	//! Merge the selected files into the in-memory EOP table.
	//! @return true if at least one record is available afterwards
	bool reload();
	//! Text for the log or the GUI: for each merged file the records read, replaced and added, the largest change made to
	//! already loaded values, then the last nutation record whose values are held for later dates
	QString lastReport() const { return report; }

	const EOPReader& reader() const { return eop; }
	//! Selected files in the order in which they are merged (the last one wins)
	QStringList mergeOrder() const;
	//! Daily records from MJD @p mjd0 to @p mjd1 for the GUI (see EOPReader::recordsBetween())
	QVector<EOPReader::Record> records(double mjd0, double mjd1, int maxRecords = 0) const { return eop.recordsBetween(mjd0, mjd1, maxRecords); }
	//! First date, last final and last predicted date of each group of values, for the GUI
	QString coverageText() const { return describeCoverage(); }
	//! Last valid value of each group of values, with its own date, for the GUI
	QString lastValuesText() const { return describeLastValues(); }
	bool isEmpty() const { return eop.isEmpty(); }

	//! Interpolated EOP at JD(UTC). See EOPReader::interpolate().
	bool getEOP(double jdUTC, EOPReader::Interpolated& out) const { return eop.interpolate(jdUTC, out); }
	//! IAU 1980 nutation corrections at JD(UTC) in arcsec, to be added to dpsi and deps of the nutation model.
	//! Constant after the last known value. @return false if no value is available (dates before the data, or no data)
	bool getNutationCorrections(double jdUTC, double& dpsiArcsec, double& depsArcsec) const;
	//! Daily records around JD(UTC) for the GUI table
	QVector<EOPReader::Record> table(double jdUTC, int before, int after) const;

	//! True if no valid file of the series @p key (see keyFromName()) is younger than @p days days at date @p today.
	//! The age is the download date of the name or, without one, the modification date of the file.
	bool isOlderThan(const QString& key, const QDate& today, int days) const;

	//! Download date written at the end of a file name: finals.all_261001.csv gives 2026-10-01. Invalid date if none.
	static QDate generationDateFromName(const QString& fileName);
	//! Name without extension and without the _YYMMDD suffix: finals.all_261001.csv gives "finals.all".
	static QString keyFromName(const QString& fileName);
	//! Reference model from the name of the file (the header of the CSV files does not tell):
	//! finals.all and eopc04_14.62-now are IAU 1980; finals2000A.all, eopc04_20u24.* and names with IAU2000 are IAU 2000A.
	static Model modelFromName(const QString& fileName);
	static QString modelName(Model m);
	//! Key of a model in the configuration file and for scripts: "IAU1980" or "IAU2000A" ("Unknown" otherwise)
	static QString modelKey(Model m);
	static Model modelFromKey(const QString& key);

signals:
	void filesRescanned();
	void reloaded();

private:
	QString describeMerge(const FileInfo& fi, const EOPReader::LoadResult& r) const;
	QString describeLastValues() const;
	QString describeCoverage() const;

	QString dirPath;
	QList<FileInfo> infos;
	QHash<QString, QVector<EOPReader::Record>> parsed;  //!< records of each valid file, so that reload() does not parse again
	QStringList selection;
	bool explicitSelection = false;
	Model preferred = Model::IAU1980;
	EOPReader eop;
	QString report;
};

#endif // EOPMANAGER_HPP
