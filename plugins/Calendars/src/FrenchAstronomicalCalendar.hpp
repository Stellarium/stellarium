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

#ifndef FRENCHASTRONOMICALCALENDAR_HPP
#define FRENCHASTRONOMICALCALENDAR_HPP

#include "FrenchArithmeticCalendar.hpp"

//! The French Revolution also introduced a new calendar which was valid only for few years (1793-1805)
//! and another few weeks in May 1871.
//! In fact, even that calendar saw a reform proposal: The original, astronomically determined begin at autumn equinox
//! should have been replaced by leap-year rules similar to the Gregorian.
//! @note This implementation derives from the simpler, algorithmic calendar. This is of course historically wrong.

class FrenchAstronomicalCalendar : public FrenchArithmeticCalendar
{
	Q_OBJECT

public:
	FrenchAstronomicalCalendar(double jd);

	~FrenchAstronomicalCalendar() override {}

public slots:
//	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...13]-Day[1...30]
	void setDate(QVector<int> parts) override;

//	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
//	//! Year, Month, MonthName, Day, DayName
//	QStringList getDateStrings() const override;
//
//	//! get a formatted complete string for a date
//	QString getFormattedDateString() const override;

	//! returns true for leap years. This is french-leap-year?(f-year) [17.7] in the CC:UE book.
	static bool isLeap(int year);

	//! find RD number for date in the French Revolution calendar
	static int fixedFromFrenchAstronomical(QVector<int> french);
	//! find date in the astronomical French Revolution calendar from RD number
	static QVector<int> frenchAstronomicalFromFixed(int rd);

	//! Critical moment for the French Revolutionary calendar (CC:UE 17.2)
	static double midnightInParis(int rd){return midnight(rd+1, paris);}

	//! @return last new year (CC:UE 17.3)
	static int frenchNewYearOnOrBefore(int rd);

protected:
//	static const int frenchEpoch; //! RD of 1792-sep-22.
//	static QMap<int, QString> dayNames;
//	static QMap<int, QString> sansculottides;
//	static QMap<int, QString> monthNames;
};

#endif
