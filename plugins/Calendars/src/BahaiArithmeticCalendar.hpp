/*
 * Copyright (C) 2022 Georg Zotti
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

#ifndef BAHAIARITHMETICCALENDAR_HPP
#define BAHAIARITHMETICCALENDAR_HPP

#include "Calendar.hpp"

//! @class BahaiArithmeticCalendar
//! Functions for the Bahá´á Arithmetic calendar
//! @author Georg Zotti
//! @ingroup calendars
//! The Bahá´í faith, founded in 1844, uses its own calendar, based on the number 19.
//! Until 2015 the calendar was based on the Gregorian calendar. This is the version implemented here.

class BahaiArithmeticCalendar : public Calendar
{
	Q_OBJECT

public:
	BahaiArithmeticCalendar(double jd);

	~BahaiArithmeticCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-Day[1...31]
	void setDate(QVector<int> parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Year, Month, MonthName, Day, DayName
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! Return R.D. of date given in the Bahai Arithmetic calendar. (CC:UE 16.3)
	static int fixedFromBahaiArithmetic(QVector<int> bahai5);

	//! Return R.D. of date given in the Bahai Arithmetic calendar. (CC:UE 16.4)
	//! return major-cycle-year-month-day for RD date
	static QVector<int> bahaiArithmeticFromFixed(int rd);

	//! Return R.D. of new year date in the Bahai Arithmetic for the given Gregorian year. (CC:UE 16.10)
	//! return RD date
	static int bahaiNewYear(int gYear);

public:
	static const int bahaiEpoch; // =fixedFromGregorian({1844, GregorianCalendar::march, 21});  //! RD of March 21, AD1844 (greg).
	constexpr static const int ayyam_i_Ha = 0;

protected:
	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> cycleNames;
	static QMap<int, QString> yearNames;
};

#endif
