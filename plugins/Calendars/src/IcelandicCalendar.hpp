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

#ifndef ICELANDICCALENDAR_HPP
#define ICELANDICCALENDAR_HPP

#include "Calendar.hpp"
#include <QPair>

//! The Icelandic calendar includes years, seasons (summer/winter), months (1..6 per season plus unaccounted days), weeks (1..52/53) and days (7-day week)
//! This implementation follows CC. It provides proper dates after the Gregorian switchover of A.D. 1700.

class IcelandicCalendar : public Calendar
{
	Q_OBJECT

public:	
	IcelandicCalendar(double jd);

	~IcelandicCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Season[90=summer/270=winter]-week[1..27]-Day[1...31]
	void setDate(const QVector<int> &parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Year, Season, Month, MonthName, Week, Day, DayName
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! returns true for leap years (those with 53 weeks)
	static bool isLeap(int iyear);

	//! RD from {year, season, week[1...27], weekday[0...6]}
	static int fixedFromIcelandic(const QVector<int> &icelandic);

	//! return year-season-week-day for RD date
	//! N.B. This adds month over the definition given by CC
	static QVector<int> icelandicFromFixed(int rd);

	//! return month number in the season.
	//! The first number of the pair is the month number,
	//! the second is an index to retrieve the name with icelandicMonthName(int)
	static QPair<int,int> icelandicMonth(const QVector<int> &iDate);

	//! Retrieve Icelandic month name. Numbers 1..6 for summer months, 7..12 for winter months.
	static QString icelandicMonthName(int i) {return monthNames.value(i);}

	//! find RD of begin of summer for Icelandic (Gregorian) year.
	static int icelandicSummer(int iyear);
	//! find RD of begin of winter for Icelandic (Gregorian) year.
	static int icelandicWinter(int iyear){ return icelandicSummer(iyear+1)-180; }

public:
	static const int icelandicEpoch;  //! RD of April 19, AD1 (Gregorian).

protected:
	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> monthNames;
};

#endif
