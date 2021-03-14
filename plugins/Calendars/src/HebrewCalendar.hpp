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

#ifndef HEBREWCALENDAR_HPP
#define HEBREWCALENDAR_HPP

#include "Calendar.hpp"

//! The Hebrew calendar, introduced in the 4th century C.E., is a Lunisolar calendar with Lunar months and leap months in a fixed scheme.
//! The year can have 6 different lengths.

class HebrewCalendar : public Calendar
{
	Q_OBJECT

public:
	typedef enum {nisan=1, iyyar, sivan, tammuz, av, elul, tishri, marheshvan, kislev, tevet, shevat, adar, adarii} month;

	HebrewCalendar(double jd);

	virtual ~HebrewCalendar() Q_DECL_OVERRIDE {}

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
	//! returns true for leap years
	static bool isLeap(int hYear);

	//! return number of months in a hebrew year
	static month lastMonthOfHebrewYear(int hYear);

	//! return if hYear is sabbatical
	static bool hebrewSabbaticalYear(int hYear){ return (StelUtils::imod(hYear, 7) == 0);}

	//! return molad of a hebrew month
	static double molad(int hYear, int hMonth);

	//! return RD of new year
	static int hebrewNewYear(int hYear);

	//! return number of days of a Hebrew year
	static int daysInHebrewYear(int hYear);

	//! return number of days
	static int lastDayOfHebrewMonth(int hYear, int hMonth);

	//! find RD number for date in the Hebrewcalendar
	static int fixedFromHebrew(QVector<int> hebrew);
	//! find date in the Hebrew calendar from RD number
	static QVector<int> hebrewFromFixed(int rd);

	static const int hebrewEpoch; //! RD of January 1, AD1.

protected:
	// auxiliary functions
	static int hebrewCalendarElapsedDays(int hYear);
	static int hebrewYearLengthCorrection(int hYear);
	static bool longMarheshvan(int hYear);
	static bool shortKislev(int hYear);

	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> monthNames;
};

#endif
