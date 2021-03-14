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

#ifndef FRENCHARITHMETICCALENDAR_HPP
#define FRENCHARITHMETICCALENDAR_HPP

#include "Calendar.hpp"

//! The French Revolution also introduced a new calendar which was valid only for few years (1793-1805)
//! and another few weeks in May 1871.
//! In fact, even that calendar saw a reform proposal: The original, astronomically determined begin at autumn equinox
//! should have been replaced by leap-year rules similar to the Gregorian.
//! This class implements the modified, arithmetic calendar proposed in 1795, which however never came into use.

class FrenchArithmeticCalendar : public Calendar
{
	Q_OBJECT

public:
	FrenchArithmeticCalendar(double jd);

	virtual ~FrenchArithmeticCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...13]-Day[1...30]
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! Year, Month, MonthName, Day, DayName
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

	//! get a formatted complete string for a date
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

public:
	//! returns true for leap years
	static bool isLeap(int year);

	//! find RD number for date in the French Revolution calendar
	static int fixedFromFrenchArithmetic(QVector<int> french);
	//! find date in the arithmetic French Revolution calendar from RD number
	static QVector<int> frenchArithmeticFromFixed(int rd);

protected:
	static const int frenchEpoch; //! RD of 1792-sep-22.
	static QMap<int, QString> dayNames;
	static QMap<int, QString> sansculottides;
	static QMap<int, QString> monthNames;
};

#endif
