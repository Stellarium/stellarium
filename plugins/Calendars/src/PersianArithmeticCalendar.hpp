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

#ifndef PERSIANARITHMETICCALENDAR_HPP
#define PERSIANARITHMETICCALENDAR_HPP

#include "Calendar.hpp"

//! This class implements an algorithmical intercalation scheme for the Persian calendar of 1925.
//! The leap year cycle is 2820 years long.

class PersianArithmeticCalendar : public Calendar
{
	Q_OBJECT

public:
	PersianArithmeticCalendar(double jd);

	virtual ~PersianArithmeticCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...13]-Day[1...30]
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Year, Month, MonthName, Day, DayName
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

	//! get a formatted complete string for a date
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

public:
	//! returns true for leap years
	static bool isLeap(int pYear);

	//! find RD number for date in the Persian arithmetic calendar
	static int fixedFromPersianArithmetic(QVector<int> persian);
	//! find date in the Persian calendar from RD number
	static QVector<int> persianArithmeticFromFixed(int rd);

protected:
	static const int persianEpoch; //! RD of .
	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> monthNames;
};

#endif
