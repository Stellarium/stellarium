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

#ifndef EOPUPDATER_HPP
#define EOPUPDATER_HPP

#include <QObject>
#include <QByteArray>
#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

//! (SS) 2026-10-07 Downloads the EOP CSV files of the IERS into the eop folder, without any change to their content.
//! Each file is saved with the download date before the extension: finals.all.csv becomes finals.all_261007.csv.
//! IERS recomputes the series regularly (old entries included), so each file is downloaded whole each time.
//! EOPManager reads the files directly and by default merges the newest of each series.
//! Files are downloaded one after the other. This class does not use StelApp: the caller provides the QNetworkAccessManager.
class EOPUpdater : public QObject
{
	Q_OBJECT
public:
	struct Source
	{
		QString fileName;      //!< name of the file at the IERS (becomes fileName_YYMMDD.csv on disk)
		QUrl url;
		QString description;
		bool isDefault = true; //!< downloaded by defaultSources()
		int refreshDays = 7;   //!< a local copy younger than this many days is considered up to date
		//! (SS) 2026-11-06 File: a CSV file of the IERS downloaded as it is. HorizonsUT1UTC: the UT1-UTC of JPL Horizons, asked to the Horizons
		//! API (POST of a batch file, the answer is a text table) and saved as an overlay file (see horizonsToOverlayCSV()).
		enum class Kind { File, HorizonsUT1UTC };
		Kind kind = Kind::File;
	};

	//! IAU 1980 set (JPL Horizons): finals.all.csv and eopc04_14.62-now.csv, in https://datacenter.iers.org/data/csv/
	static QList<Source> defaultSources();
	//! (SS) 2026-11-05 IAU 2000A set: finals2000A.all.csv and eopc04_20u24.1962-now.csv (isDefault false), for the IAU 2000A model only
	static QList<Source> optionalSources();

	//! (SS) 2026-11-06 UT1-UTC of JPL Horizons (quantity 49), saved as horizons_ut1utc_YYMMDD.csv in the eop folder. Only for the
	//! IAU 1980 model (Horizons uses the IAU 1980 nutation). The predictions of Horizons change with every update of its EOP file
	//! (a few ms in a few days), so a copy is considered up to date for one day only.
	static Source horizonsSource();
	//! Batch file sent to the Horizons API (same request as the curl command of the author): quantity 49 for the Sun seen from the
	//! geocenter, one day step, from 1962-01-01 to @p stop.
	static QString horizonsRequest(const QDate& stop);
	//! Turn the answer of the Horizons API into the text of an overlay file: "n.a." days dropped (UT1-UTC does not exist before 1962-Jan-20),
	//! rows after the last distinct value dropped (Horizons keeps its last prediction constant), comment lines giving the Horizons run,
	//! its EOP file and its coverage, then MJD;Date;JD;UT1-UTC. Same as the script horizons_ut1utc.py.
	//! @param minRows fewer rows with a value than this makes the answer unusable
	//! @return false if the answer is not a Horizons table (error describes what was received)
	static bool horizonsToOverlayCSV(const QString& answer, QString& csv, QString* error = Q_NULLPTR, int* nRows = Q_NULLPTR,
	                                 int minRows = 1000);

	//! @param nam network manager (not owned)
	//! @param dir folder where the files are written (the eop folder)
	EOPUpdater(QNetworkAccessManager* nam, const QString& dir, QObject* parent = Q_NULLPTR);

	//! Start the downloads. Does nothing if some are already running.
	//! @param today date used for the file names, default: current date in UTC
	void start(const QList<Source>& sources, const QDate& today = QDate());
	void abort();
	bool isRunning() const { return reply != Q_NULLPTR; }

	//! "finals.all.csv" and 2026-10-07 give "finals.all_261007.csv"
	static QString fileNameFor(const QString& iersFileName, const QDate& date);
	//! Checks that the data looks like an IERS EOP CSV file (header line and rows).
	static bool validateCSV(const QByteArray& data, QString* error = Q_NULLPTR, int* nRows = Q_NULLPTR);

signals:
	void progress(qint64 received, qint64 total);
	//! One file is done. @param fileName name of the file at the IERS
	void fileFinished(const QString& fileName, bool ok, const QString& message);
	//! All files are done. @param ok true if every file was written. @param written names of the files written in the eop folder
	void finished(bool ok, const QStringList& written, const QString& message);

private slots:
	void onFinished();

private:
	void startNext();

	QNetworkAccessManager* nam;
	QString dirPath;
	QNetworkReply* reply = Q_NULLPTR;
	QList<Source> queue;
	int index = 0;
	bool aborted = false;
	QDate day;
	QStringList written;
	QStringList failures;
};

#endif // EOPUPDATER_HPP
