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

#ifndef ASTROHINDUSOLARCALENDAR_HPP
#define ASTROHINDUSOLARCALENDAR_HPP

#include "NewHinduCalendar.hpp"

//! @class AstroHinduSolarCalendar
//! Functions for the Hindu Solar calendar based on hindu-astronomical functions described in CC:UE chapter 20.
//! @author Georg Zotti
//! @ingroup calendars
//!
//! @note This class contains only the Calendar interfacing methods for the Hindu "Astro-Solar" dates from CC:UE chapter 20.
//! For scripting, you may use the methods found in class NewHinduCalendar directly.


class AstroHinduSolarCalendar : public NewHinduCalendar
{
	Q_OBJECT

public:
	AstroHinduSolarCalendar(double jd);

	virtual ~AstroHinduSolarCalendar() Q_DECL_OVERRIDE {}

public slots:
	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set RD date from a vector of calendar date elements sorted from the largest to the smallest.
	//! parts = {Year, Month[1...12], leapMonth[0|1], Day[1...30], leapDay[0|1] }
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements for the Hindu Astro Solar calendar sorted from the largest to the smallest.
	//! {Year, Month, MonthName, Day, WeekDayName}
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;
};

#endif
