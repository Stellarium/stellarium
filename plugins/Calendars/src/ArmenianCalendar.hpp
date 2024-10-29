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

#ifndef ARMENIANCALENDAR_HPP
#define ARMENIANCALENDAR_HPP

#include "EgyptianCalendar.hpp"

//! The Armenian calendar has the same structure as the Egyptian calendar
//! Epoch is 552 C.E.

class ArmenianCalendar: public EgyptianCalendar
{
	Q_OBJECT

public:
	ArmenianCalendar(double jd);

	~ArmenianCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-Day[1...31]
	void setDate(const QVector<int> &parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Year, Month, MonthName, Day, DayName
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! find RD number for date in the Armenian calendar (may be used in other calendars!)
	static int fixedFromArmenian(const QVector<int> &armenian);
	//! find date in the Armenian calendar from RD number (may be used in other calendars!)
	static QVector<int> armenianFromFixed(int rd);

public:
	static const int armenianEpoch;

protected:
	static QMap<int, QString> monthNames;
};

#endif
