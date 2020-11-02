/*
 * Copyright (C) 2020 Georg Zotti
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

#ifndef CALENDAR_HPP
#define CALENDAR_HPP

#include <QString>
#include <QObject>

//! Abstract superclass for all calendars.
//! Stellarium uses Julian Day numbers internally, and the conventional approach of using the Gregorian calendar for dates after 1582-10-15.
//! For dates before that, the Julian calendar is used, in the form finalized by Augustus and running unchanged since 8AD.
//! Some European countries, especially the Protestant countries, delayed the calendar switch well into the 18th century.
//! Other cultures have various other calendar schemes.
//! All calendars have a numerical vector of time elements and may have individual string lists for names of elements like "week days" and "months".
//! If the pattern of weeks and months does not apply, the names may refer to other structural elements.
//!
//! One important source used is
//! Edward M. Reingold, Nachum Dershowitz: Calendrical Calculations
//! in various editions (1997-2018), referred to as CC. It does not use JD directly but a number called Rata Die (RD),
//! with functions including "fixed" in their names. Our calendar subclasses can use Calendar::fixedFromJD(jd) / Calendar::jdFromFixed(rd).
class Calendar : public QObject
{
	Q_OBJECT

public:
	Calendar(double jd):JD(jd) {}

	virtual ~Calendar() {}

	//! Translate e.g. stringlists of part names
	virtual void retranslate() = 0;

	//! Set a calendar date from the Julian day number
	//! Subclasses set JD and compute the parts and possibly other data
	//! This triggers the partsChanged() signal
	virtual void setJD(double JD) = 0;

	//! Get Julian day number from a calendar date
	virtual double getJD() { return JD;}

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! This triggers the jdChanged() signal
	virtual void setParts(QVector<int> parts) = 0;

	//! get a vector of calendar date elements sorted from the largest to the smallest.
	//! The order depends on the actual calendar
	virtual QVector<int> getParts() { return parts; }


	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! The order depends on the actual calendar
	virtual QStringList getPartStrings() = 0;

	//! get a formatted complete string for a date
	virtual QString getFormattedDateString() = 0;

	constexpr static const double J2000=2451545.0;

signals:
	void partsChanged(QVector<int> parts);
	void jdChanged(double jd);

protected:
	constexpr static const double jdEpoch=-1721424.5;

	//! interfacing function from Reingold/Dershowitz: Calendrical Calculations
	inline static double momentFromJD(double jd) { return jd+jdEpoch;}
	inline static long fixedFromJD(double jd) { return std::lround(std::floor(momentFromJD(jd)));}

	//! interfacing function from Reingold/Dershowitz: Calendrical Calculations
	inline static double jdFromMoment(double rd) { return rd-jdEpoch;}
	inline static double jdFromFixed(double rd) { return jdFromMoment(rd);}

	double JD;		//! date expressed as JD
	QVector<int> parts;	//! date expressed in the numerical parts of the calendar
};

#endif
