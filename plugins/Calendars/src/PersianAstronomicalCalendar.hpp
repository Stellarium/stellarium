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

#ifndef PERSIANASTRONOMICALCALENDAR_HPP
#define PERSIANASTRONOMICALCALENDAR_HPP

#include "PersianArithmeticCalendar.hpp"

//! This class implements an astronomically exact version of the Persian calendar of 1925.

class PersianAstronomicalCalendar : public PersianArithmeticCalendar
{
	Q_OBJECT

public:
	PersianAstronomicalCalendar(double jd);

	virtual ~PersianAstronomicalCalendar() Q_DECL_OVERRIDE {}

public slots:
//	virtual void retranslate() Q_DECL_OVERRIDE;

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...13]-Day[1...30]
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

//	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
//	//! Year, Month, MonthName, Day, DayName
//	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;
//
//	//! get a formatted complete string for a date
//	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

	//! find RD number for date in the Persian astronomical calendar (CC:UE 15.5)
	//! @input {year, month, date}
	static int fixedFromPersianAstronomical(QVector<int> persian);
	//! alias for fixedFromPersianAstronomical();
	static int fixedFromPersian(QVector<int> persian){return fixedFromPersianAstronomical(persian);}
	//! find date in the Persian calendar from RD number
	static QVector<int> persianAstronomicalFromFixed(int rd);
	//! alias for persianAstronomicalFromFixed();
	static QVector<int> persianFromFixed(int rd){return persianAstronomicalFromFixed(rd);}

	//! find midday in Tehran (CC:UE 15.3)
	static double middayInTehran(int rd){return midday(rd, tehran);}
	//! find midday in Isfahan (CC:UE 15.3b)
	static double middayInIsfahan(int rd){return midday(rd, isfahan);}

	//! find vernal equinox on or before a given fixed (RD) date (CC:UE 15.4)
	static int persianNewYearOnOrBefore(int rd);

	//! find RD number of Persian New Year (Nowruz)
	static int nowruz(const int gYear);

protected:
	//! location where solar observation is linked to
	//static const StelLocation tehran; // CC:UE 15.2
	static const StelLocation isfahan; // CC:UE 15.2 alternative
};

#endif
