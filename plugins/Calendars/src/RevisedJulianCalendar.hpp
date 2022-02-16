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

#ifndef REVISEDJULIANCALENDAR_HPP
#define REVISEDJULIANCALENDAR_HPP

#include "JulianCalendar.hpp"

//! The Orthodox Church worked out a Revised Julian Calendar in 1923 to overcome the 13-day gap between their traditional Julian and the rest of the world which is using the Gregorian.
//! In this calendar, only centuries where division by 900 yields 200 or 600 are leap years.
//! Dates from March 1st, 1600 to February 28th, 2800, go in sync with the Gregorian calendar.
//! The algorithm implemented here was taken from https://en.wikipedia.org/wiki/Revised_Julian_calendar in January 2022 (with a necessary fix discovered during implementation)
//! @note Behaviour of this calendar for dates BC is not documented.
//! In the first century, dates are 2 days away from the Julian.
//! Around the Nicaean concile (AD325) this calendar provides the same dates as the Julian, therefore we make the switchover in AD325, and forward all earlier dates to the Julian calendar.

class RevisedJulianCalendar : public JulianCalendar
{
	Q_OBJECT

public:
	RevisedJulianCalendar(double jd);
	virtual ~RevisedJulianCalendar() Q_DECL_OVERRIDE {}

public slots:
	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-Day[1...31]
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! returns true for leap years. We handle years prior to 325 like in the regular Julian calendar.
	static bool isLeap(int year);

	//! find RD number for date in the Revised Julian calendar. Dates before AD325 are handled as dates in the regular Julian calendar.
	static int fixedFromRevisedJulian(QVector<int> revisedJulian);
	//! find date in the Revised Julian calendar from RD number. If date is earlier than AD325,
	//! the returned date is in the standard Julian Calendar.
	static QVector<int> revisedJulianFromFixed(int rd);

public:
	constexpr static const int revisedJulianEpoch=1; //! RD of January 1, AD1.
};

#endif
