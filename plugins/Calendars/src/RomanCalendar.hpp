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

#ifndef ROMANCALENDAR_HPP
#define ROMANCALENDAR_HPP

#include "JulianCalendar.hpp"

//! The Roman calendar provides the ancient Roman way of expressing dates.
//! It is based on the Julian calendar, but counts years ab urbe condita (AUC)
//!
class RomanCalendar: public JulianCalendar
{
	Q_OBJECT

public:
	typedef enum {kalends=1, nones, ides} events;

	RomanCalendar(double jd);

	~RomanCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-[kalends|nones|ides]-[count]-[leap?]
	void setDate(QVector<int> parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! AUCYear, Month, MonthName(genitive), event, DayName
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date. Years are AUC.
	QString getFormattedDateString() const override;

	//! find RD number for date in the Roman calendar. Years are like in the Julian calendar.
	static int fixedFromRoman(QVector<int> julian);
	//! find date in the Roman calendar from RD number
	static QVector<int> romanFromFixed(int rd);

	//! returns 13 or 15, the date of "idae"
	static int idesOfMonth(int month);
	//! returns the date of "nonae"
	static int nonesOfMonth(int month);

	//! Convert between AUC and Julian year numbers
	static int julianYearFromAUC(int aucYear);
	//! Convert between AUC and Julian year numbers
	static int aucYearFromJulian(int julianYear);

	//! return a Roman number (within int range). Uses multiplicative terms for num>5000.
	static QString romanNumber(const int num);

public:
	constexpr static const int yearRomeFounded=-753;

protected:
	static QMap<int, QString> monthGen;
};

#endif
