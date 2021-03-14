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

#ifndef ISLAMICCALENDAR_HPP
#define ISLAMICCALENDAR_HPP

#include "Calendar.hpp"

//! The Islamic calendar is a strictly Lunar calendar with no month intercalation. It thus does not observe the Solar year,
//! and drifts through the seasons in about 32 solar years.
//! This implementation of the arithmetic Islamic calendar is the easy to compute version, whereas most Muslims follow an
//! observation-based calendar, which by definition cannot be computed.
//! Note that Islamic days begin on the evening before the actual date we compute.
//! Therefore, times lying after sunset will be wrong and should count up one day.

class IslamicCalendar : public Calendar
{
	Q_OBJECT

public:
	IslamicCalendar(double jd);

	virtual ~IslamicCalendar() Q_DECL_OVERRIDE {}

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
	//! Return true if iYear is an Islamic leap year
	static bool isLeap(int iYear);

	//! find RD number for date in the Islamic calendar
	static int fixedFromIslamic(QVector<int> islamic);
	//! find date in the Islamic calendar from RD number
	static QVector<int> islamicFromFixed(int rd);

	static const int islamicEpoch; //! RD of July 16, 622.

protected:
	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> monthNames;
};

#endif
