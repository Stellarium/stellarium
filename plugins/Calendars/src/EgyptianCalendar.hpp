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

#ifndef EGYPTIANCALENDAR_HPP
#define EGYPTIANCALENDAR_HPP

#include "Calendar.hpp"

//! This is the "Simple Calendar" exemplified in CC.UE chapter 1.11.
//! Epoch is the era of Nabonassar (Nebukadnezar)

class EgyptianCalendar : public Calendar
{
	Q_OBJECT

public:
	EgyptianCalendar(double jd);

	virtual ~EgyptianCalendar() Q_DECL_OVERRIDE {}

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
	//! find RD number for date in the Egyptian calendar (may be used in other calendars!)
	static int fixedFromEgyptian(QVector<int> julian);
	//! find date in the Egyptian calendar from RD number (may be used in other calendars!)
	static QVector<int> egyptianFromFixed(int rd);

	static const int egyptianEpoch; //! RD of JD1448638.

protected:
	static QMap<int, QString> monthNames;
};

#endif
