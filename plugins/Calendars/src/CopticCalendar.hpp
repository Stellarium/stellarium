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

#ifndef COPTICCALENDAR_HPP
#define COPTICCALENDAR_HPP

#include "JulianCalendar.hpp"

//! Based on the Egyptian calendar (esp. the month names!), but with leap years like the Julian. CC.UE chapter 4.
//! Epoch is the era of Diocletian (Era Martyrum)

class CopticCalendar : public JulianCalendar
{
	Q_OBJECT

public:
	CopticCalendar(double jd);

	~CopticCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...13]-Day[1...30]
	void setDate(const QVector<int> &parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Year, Month, MonthName, Day, DayName
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! find RD number for date in the Coptic calendar (may be used in other calendars!)
	static int fixedFromCoptic(const QVector<int> &coptic);
	//! find date in the Coptic calendar from RD number (may be used in other calendars!)
	static QVector<int> copticFromFixed(int rd);

	//! returns true for leap years
	static bool isLeap(int year);

	//! returns name of weekday for a JD
	static QString weekday(double jd);
public:
	static const int copticEpoch; //! RD of 284-aug-29 (Jul).

protected:
	static QMap<int, QString> monthNames;
	static QMap<int, QString> dayNames;
};

#endif
