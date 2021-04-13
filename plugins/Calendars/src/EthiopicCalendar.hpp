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

#ifndef ETHIOPICCALENDAR_HPP
#define ETHIOPICCALENDAR_HPP

#include "CopticCalendar.hpp"

//! Equal in structure to the Coptic calendar (with different month names!). CC.UE chapter 4.
//! Epoch is the Ethiopian era of Mercy (8 AD, the year where Augustus re-introduced leap years)

class EthiopicCalendar : public CopticCalendar
{
	Q_OBJECT

public:
	EthiopicCalendar(double jd);

	virtual ~EthiopicCalendar() Q_DECL_OVERRIDE {}

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
	//! find RD number for date in the Ethiopic calendar (may be used in other calendars!)
	static int fixedFromEthiopic(QVector<int> ethiopic);
	//! find date in the Ethiopic calendar from RD number (may be used in other calendars!)
	static QVector<int> ethiopicFromFixed(int rd);

	//! returns true for leap years
	static bool isLeap(int year);

	static const int ethiopicEpoch; //! RD of 8-aug-29 (Jul). (E.E.)

protected:
	static QMap<int, QString> monthNames;
	static QMap<int, QString> dayNames;

	static QString weekday(double jd);
};

#endif
