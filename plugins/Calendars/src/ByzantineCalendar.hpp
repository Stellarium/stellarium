/*
 * Copyright (C) 2026 Georg Zotti
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

#ifndef BYZANTINECALENDAR_HPP
#define BYZANTINECALENDAR_HPP

#include "JulianCalendar.hpp"

//! @class ByzantineCalendar
//! @brief Functions for the Byzantine calendar
//! @author Georg Zotti
//! @ingroup calendars
//! Stellarium uses Julian Day numbers internally, and the conventional approach of using the Gregorian calendar for dates after 1582-10-15.
//! For dates before that, the Julian calendar is used, in the form finalized by Augustus and running unchanged since 8AD.
//! Some European countries, especially the Protestant countries, delayed the calendar switch well into the 18th century.
//! @note The implementation does not correctly represent the Roman Julian calendar valid from introduction by Julius Caesar to the reform by Augustus.
//! @note this implementation adheres to Calendrical Calculation's style of omitting a year zero. Negative years represent years B.C.E.
//!       This is very much in contrast to Stellarium's usual behaviour, and also different from a year zero in CC's implementation of the Gregorian calendar.
//!
//! The year count (Annus Mundi) was understood to originate with the creation of the world, and so that Christ was born around 5500 of the Byzantine era (B.E.).
//! This implementation of the Byzantine calendar is based on years starting on September 1st. This is the form observed in Russia from 1492 (7000AM) to 1700 when Tsar Peter introduced the Julian Calendar, moving New Year to January 1st.
//! An earlier tradition had years start with March 1.
//! More accurate reckoning allows saying a Julian date between January and August of Y is in Y+5509, from September to December it is Y+5508.
//! Byzantine date 1-1-1 was September 1, 5509 BC (Julian)
//! The implementation then uses year zero and negative years, which do not make any sense in a calendar that assumes to start on the day of Creation.
//!
//! Further reading:
//! https://en.wikipedia.org/wiki/Byzantine_calendar
//! https://sites.google.com/site/seesscm/eastern-slavic-and-russian-calendar-systems
//!

class ByzantineCalendar : public JulianCalendar
{
	Q_OBJECT

public:
	//typedef enum {january=1, february, march, april, may, june, july, august, september, october, november, december } month;

	ByzantineCalendar(double jd);

	~ByzantineCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-Day[1...31]
	void setDate(const QVector<int> &parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Year, Month, MonthName, Day, DayName
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! return name of week day. This is actually independent from any calendar, just a modulo of JD.
	static QString weekday(double jd);

	//! returns true for leap years
	static bool isLeap(int year);

	//! find RD number for date in the Julian calendar (may be used in other calendars!)
	static int fixedFromByzantine(const QVector<int> &byzantine);
	//! find date in the Julian calendar from RD number (may be used in other calendars!)
	static QVector<int> byzantineFromFixed(int rd);

protected:
	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> monthNames;
};

#endif
