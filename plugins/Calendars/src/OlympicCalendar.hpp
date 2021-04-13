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

#ifndef OLYMPICCALENDAR_HPP
#define OLYMPICCALENDAR_HPP

#include "JulianCalendar.hpp"

//! The Olympic calendar only provides the counted olympiad and year in the 4-year cycle. Else it behaves like the Julian calendar.
//! setDate can take either 2 or 4 elements. In the first case, month/day are taken from the current Julian calendar date.
class OlympicCalendar : public JulianCalendar
{
	Q_OBJECT

public:
	OlympicCalendar(double jd);

	virtual ~OlympicCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE {}

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! The elements only change years. The calendar is else based on the Julian, so
	//! unless given explicitly, months and days in the Julian calendar will not be changed.
	//! parts={olympiad, year [, month, day]}
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Olympiad, Year
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

	//! get a formatted complete string for a date ("nth of the monthname, Year X in the YY olympiad")
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

public:
	static int julianYearFromOlympiad(QVector<int>odate);

	static QVector<int> olympiadFromJulianYear(int jYear);

	constexpr static const int olympiadStart=-776;
};

#endif
