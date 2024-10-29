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

#ifndef NEWHINDULUNARCALENDAR_HPP
#define NEWHINDULUNARCALENDAR_HPP

#include "NewHinduCalendar.hpp"

//! @class NewHinduLunarCalendar
//! Functions for the New Hindu Lunisolar calendar described in CC:UE chapter 20.
//! @author Georg Zotti
//! @ingroup calendars
//!
//! @note This class contains only the Calendar interfacing methods for the New Hindu "Lunar" dates from CC:UE chapter 20.
//! For scripting, you may use the methods found in class NewHinduCalendar directly.


class NewHinduLunarCalendar : public NewHinduCalendar
{
	Q_OBJECT

public:
	NewHinduLunarCalendar(double jd);

	~NewHinduLunarCalendar() override {}

public slots:
	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set RD date from a vector of calendar date elements sorted from the largest to the smallest.
	//! parts = {Year, Month[1...12], leapMonth[0|1], Day[1...30], leapDay[0|1] }
	void setDate(const QVector<int> &parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! @return {Year, Month, MonthName, leap[0|1], Day, DayName}
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! get a formatted string for displaying the "panchang", a set of
	//! tithi (lunar day), day of week, nakshatra, yoga, karana.
	virtual QString getFormattedPanchangString();
};

#endif
