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

#ifndef EOPREADER_HPP
#define EOPREADER_HPP

#include <QString>
#include <QStringList>
#include <QVector>

//! (SS) 2026-10-07 Earth Orientation Parameters (EOP) held in memory as one record per day (0h UTC).
//!
//! Only the CSV files of the IERS are read, exactly as downloaded (the layout is detected from the header line):
//!  - Finals : standard series (finals.all.csv, finals2000A.all.csv), from 1973, final values and predictions.
//!             pole in arcsec, UT1-UTC in s, LOD in msec, dPsi dEps dX dY in marcsec.
//!  - C04    : long term series (eopc04_*.csv, ITRF2014 or ITRF2020), from 1962, final values only.
//!             pole in arcsec, UT1-UTC and LOD in s, dPsi dEps dX dY in arcsec.
//! IAU 1980 files carry dPsi and dEps, IAU 2000 files carry dX and dY: they complement each other when merged.
//! Records are always stored in arcsec (angles) and seconds (times), whatever the file layout.
//! Several files can be merged: values of a later file replace the values of the same MJD (values the later file lacks are kept).
//! Rows without any value are skipped.
//!
//! This class does not use StelApp, StelCore or any other Stellarium singleton (unit testable).
class EOPReader
{
public:
	//! Finals: standard series from datacenter.iers.org (finals.all.csv for IAU 1980 dPsi/dEps, finals2000A.all.csv for IAU 2000 dX/dY),
	//!         from 1973 on, with final values and one year of predictions.
	//! C04:    long term series (eopc04_*.csv), from 1962 on, consolidated final values only.
	enum class Format { Unknown, Finals, C04 };
	enum class Status { Unknown, Final, Prediction, Missing };
	enum class Coverage { Inside, AfterEnd };
	//! Groups of values that are tracked separately. Status (final or prediction) of Pole, UT and Nutation come from the file;
	//! LOD has the status of UT and DXDY the one of Nutation (as in the IERS files).
	enum class Group { Pole, UT, LOD, Nutation, DXDY };

	struct Record
	{
		double mjd = 0.;       //!< MJD (UTC) of the record (0h UTC for file records)
		int year = 0, month = 0, day = 0;

		double xp = 0., yp = 0.;       //!< pole coordinates [arcsec]
		double ut1utc = 0.;            //!< UT1-UTC [s]
		double lod = 0.;               //!< length of day excess [s]
		double dpsi = 0., deps = 0.;   //!< celestial pole offsets w.r.t. IAU 1980 nutation [arcsec]
		double dx = 0., dy = 0.;       //!< celestial pole offsets w.r.t. IAU 2006/2000A precession-nutation [arcsec] (not in C04 files)

		double xpErr = 0., ypErr = 0., ut1utcErr = 0., lodErr = 0., dpsiErr = 0., depsErr = 0., dxErr = 0., dyErr = 0.;  //!< formal errors, same units

		Status poleStatus = Status::Unknown;
		Status utStatus = Status::Unknown;
		Status nutationStatus = Status::Unknown;
		Status lodStatus = Status::Unknown;     //!< status of LOD (the files give one Type for UT1-UTC and LOD)
		Status dxdyStatus = Status::Unknown;    //!< status of dX, dY (the files give one Type for dPsi, dEps and dX, dY)

		bool hasPole = false, hasUT = false, hasLOD = false, hasNutation = false, hasDXDY = false;
		Format source = Format::Unknown;   //!< layout of the file the record comes from

		double jd() const { return mjd + 2400000.5; }
		bool isNutationPrediction() const { return nutationStatus == Status::Prediction; }
	};

	//! Result of interpolate()
	struct Interpolated
	{
		Record rec;                              //!< values at the requested date (only the has* flagged members are valid)
		Coverage coverage = Coverage::Inside;
		//! true when the group has no value at that date and rec holds the last earlier value instead
		bool poleHeld = false, utHeld = false, lodHeld = false, nutationHeld = false, dxdyHeld = false;
	};

	//! Largest change made to a parameter by replaced records (only where old and new records both have it)
	struct Diff
	{
		double max = 0.;   //!< largest absolute difference new - old (arcsec or seconds)
		double mjd = 0.;   //!< where it happens
		int n = 0;         //!< number of records compared
	};

	//! Last record that has values of a group: those values are kept for all later dates
	bool hasLast(Group g) const;
	const Record& lastRecord(Group g) const;

	//! Remove dPsi and dEps from records (files that give them relative to IAU 2000A: dX and dY carry the same information)
	static void dropNutation(QVector<Record>& records);

	//! Where a group of values exists in the merged table
	struct Span
	{
		bool valid = false;          //!< the group has at least one value
		double firstMJD = 0.;        //!< first MJD with a value
		bool hasFinal = false;       //!< at least one value is final
		double lastFinalMJD = 0.;    //!< last MJD with a final value
		double lastMJD = 0.;         //!< last MJD with a value (final or prediction): this value is kept for later dates
		bool hasPrediction() const { return valid && lastMJD > lastFinalMJD + 1e-9; }
	};

	struct LoadResult
	{
		bool ok = false;
		QString error;
		Format format = Format::Unknown;
		int nRecords = 0;      //!< records in the file
		int nAdded = 0;        //!< records with a new MJD
		int nReplaced = 0;     //!< records replacing an existing MJD
		int nFinalAdded = 0;       //!< added records without any predicted pole, UT1-UTC or nutation value
		int nPredictionAdded = 0;  //!< added records with a predicted pole, UT1-UTC or nutation value
		Diff xpDiff, ypDiff, ut1utcDiff, lodDiff, dpsiDiff, depsDiff;  //!< what replaced records changed
		double firstMJD = 0., lastMJD = 0.;
	};

	EOPReader() = default;

	//! Parse a file without touching any state. Records are returned sorted by MJD, duplicated MJDs removed (last wins).
	static bool parseFile(const QString& path, QVector<Record>& records, Format& format, QString& error);
	//! Same for text already in memory
	static bool parseText(const QString& text, QVector<Record>& records, Format& format, QString& error);
	static Format detectFormat(const QString& firstLine);
	static QString formatName(Format f);
	static QString statusName(Status s);

	//! Parse a file and merge it (records of this file replace records of the same MJD). With replaceAll the content is cleared first.
	LoadResult loadFile(const QString& path, bool replaceAll = false);
	//! Merge already parsed records (see parseFile()). A record replaces the one with the same MJD value by value:
	//! pole, UT1-UTC, LOD and nutation of the new record win when it has them, otherwise the old ones are kept.
	LoadResult merge(const QVector<Record>& records, Format format, bool replaceAll = false);
	void clear();

	bool isEmpty() const { return recs.isEmpty(); }
	int size() const { return recs.size(); }
	double firstMJD() const { return recs.isEmpty() ? 0. : recs.first().mjd; }
	double lastMJD() const { return recs.isEmpty() ? 0. : recs.last().mjd; }
	const Record& record(int i) const { return recs.at(i); }
	//! Number of places where consecutive records are more than one day apart
	int gapCount() const { return nGaps; }

	//! Last record that has nutation corrections. As written in the JPL Horizons manual, these values are
	//! kept constant for any later date.
	bool hasLastNutation() const { return lastNutIndex >= 0; }
	const Record& lastNutation() const { return recs.at(lastNutIndex); }

	//! Values at JD(UTC), linear interpolation between the daily records.
	//!  - Each group (pole, UT1-UTC, LOD, nutation, dX/dY) is handled separately.
	//!  - A group with values at both neighbouring days is interpolated; UT1-UTC across a leap second (the 1 s step is removed first).
	//!  - Otherwise the last earlier value of the group is returned and its xxxHeld flag is set. This is the rule used after the
	//!    last predicted value (JPL Horizons keeps the last dPsi and dEps). It is up to the caller to decide whether a held
	//!    value is meaningful (a held UT1-UTC drifts away from the real one, for example).
	//!  - A group with no earlier value has its has* flag false.
	//! @return false before the first record
	bool interpolate(double jdUTC, Interpolated& out) const;

	//! First MJD, last final and last predicted MJD of a group of values in the merged table
	Span span(Group g) const;
	static QString groupName(Group g);

	//! Records with a MJD from @p mjd0 to @p mjd1 (both included), at most @p maxRecords of them (0: no limit)
	QVector<Record> recordsBetween(double mjd0, double mjd1, int maxRecords = 0) const;

	//! Daily records around a date, for tables: up to @p before records before the one at or just before mjd, and @p after after it.
	QVector<Record> recordsAround(double mjd, int before, int after) const;

	static constexpr double ARCSEC2RAD = 4.848136811095359935899141e-6;

private:
	void updateDerived();
	static bool has(const Record& r, Group g);
	static Status statusOf(const Record& r, Group g);

	QVector<Record> recs;   //!< sorted by MJD, unique MJDs
	QVector<int> prevValid[5];  //!< per group: index of the last record at or before i that has a value, -1 if none
	int lastNutIndex = -1;
	int nGaps = 0;
};

#endif // EOPREADER_HPP
