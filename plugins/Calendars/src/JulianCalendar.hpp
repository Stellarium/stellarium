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

#ifndef JULIANCALENDAR_HPP
#define JULIANCALENDAR_HPP

#include "Calendar.hpp"

//! Stellarium uses Julian Day numbers internally, and the conventional approach of using the Gregorian calendar for dates after 1582-10-15.
//! For dates before that, the Julian calendar is used, in the form finalized by Augustus and running unchanged since 8AD.
//! Some European countries, especially the Protestant countries, delayed the calendar switch well into the 18th century.
//! @note The implementation does not correctly represent the Roman Julian calendar valid from introduction by Julius Caesar to the reform by Augustus.
//! @note this implementation adheres to Calendrical Calculation's style of omitting a year zero. Negative years represent years B.C.E.
//!       This is very much in contrast to Stellarium's usual behaviour, and also different from a year zero in CC's implementation of the Gregorian calendar.

class JulianCalendar : public Calendar
{
	Q_OBJECT

public:
	typedef enum {january=1, february, march, april, may, june, july, august, september, october, november, december } month;

	JulianCalendar(double jd);

	virtual ~JulianCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-Day[1...31]
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Year, Month, MonthName, Day, DayName
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

	//! get a formatted complete string for a date
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

public:
	//! return name of week day. This is actually independent from any calendar, just a modulo of JD.
	static QString weekday(double jd);

	//! returns true for leap years
	static bool isLeap(int year);

	//! find RD number for date in the Julian calendar (may be used in other calendars!)
	static int fixedFromJulian(QVector<int> julian);
	//! find date in the Julian calendar from RD number (may be used in other calendars!)
	static QVector<int> julianFromFixed(int rd);

	constexpr static const int julianEpoch=-1; //! RD of January 1, AD1.

protected:
	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> monthNames;
};

#endif
