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

#ifndef GREGORIANCALENDAR_HPP
#define GREGORIANCALENDAR_HPP

#include "JulianCalendar.hpp"

//! Stellarium uses Julian Day numbers internally, and the conventional approach of using the Gregorian calendar for dates after 1582-10-15.
//! For dates before that, the Julian calendar is used, in the form finalized by Augustus and running unchanged since 8AD.
//! Some European countries, especially the Protestant countries, delayed the calendar switch well into the 18th century.
//! This implementation strictly follows CC. It provides the "Proleptic Gregorian Calendar" for dates before 1582-10-15.
//! This may be helpful for a better estimate of seasons' beginnings in prehistory. However, also the Gregorian calendar is not perfect, and
//! Neolithic and even earlier dates will still show deviations from the dates well-known from today.

class GregorianCalendar : public JulianCalendar
{
	Q_OBJECT

public:
	GregorianCalendar(double jd);

	virtual ~GregorianCalendar() Q_DECL_OVERRIDE {}

public slots:
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
	static bool isLeap(int year);

	constexpr static const int gregorianEpoch=1;  //! RD of January 1, AD1.
	//! auxiliary functions from CC.UE ch2.5
	//! Return R.D. of date in the Gregorian calendar.
	static int fixedFromGregorian(QVector<int> gregorian);

	//! Orthodox Easter sunday (RD) from chapter 9.1
	static int orthodoxEaster(int gYear);

	//! Gregorian Easter sunday (RD) from chapter 9.2
	static int easter(int gYear);

	//! Return RD of Pentecost in Gregorian calendar.
	static int pentecost(int gYear) { return easter(gYear)+49; }


protected:
	static int gregorianNewYear(int year) {return fixedFromGregorian({year, january, 1});}
	static int gregorianYearFromFixed(int rd);
	//! return year-month-day for RD date
	static QVector<int> gregorianFromFixed(int rd);

	//! @return RD date of the n-th k-day
	static int nthKday(const int n, const Calendar::Day k, const int gYear, const int gMonth, const int gDay);
};

#endif
