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

#ifndef TIBETANCALENDAR_HPP
#define TIBETANCALENDAR_HPP

#include "Calendar.hpp"

//! @class TibetanCalendar
//! Functions for the Tibetan calendar
//! @author Georg Zotti
//! @ingroup calendars
//! The Tibetan Phuglugs/Phug-pa version of the Kalacakra (Wheel of Time) calendar is similar
//! tot he Hindu Lunisolar calendars, described as between the arithmetic simplicity of the old Hindu and the
//! astronomical complexity of the modern Hindu. Astronomical events are calculated in local time
//! which may lead to regional deviations. Bhutan, Mongolian and Sherpa calendars are very similar.
//! Months are lunar with lengths of 29 or 30 days. Leap months precede their "ordinary" months.
//!
//! Our implementation uses a 5-part QVector<int> {year, month, leap-month, day, leap-day}
//!
//! @todo Also derive the Hindu elements like Nakshatras, when Hindu calendars have been done.
class TibetanCalendar : public Calendar
{
	Q_OBJECT

public:
	TibetanCalendar(double jd);

	virtual ~TibetanCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! {year, month, leap-month, day, leap-day}
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! {Year, Month, MonthName, "leap"|"", Day, "leap"|"", WeekDayName}
	//! The words "leap" (translated) are only given if the respective element before (month or day) are leap. Else an empty string is given.
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

	//! get a formatted complete string for a date
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

	//! find RD number for date in the Tibetan calendar (CC:UE 21.4)
	//! @arg tibetan={year, month, leapMonth, day, leapDay}
	static int fixedFromTibetan(QVector<int> tibetan);
	//! find date in the Tibetan calendar from RD number (CC:UE 21.5)
	static QVector<int> tibetanFromFixed(int rd);

	// Auxiliary functions
	//! @return Tibetan Sun Equation, one of 12 discrete values (CC:UE 21.2)
	static double tibetanSunEquation(double alpha);
	//! @return Tibetan Moon Equation, one of 128 discrete values (CC:UE 21.3)
	static double tibetanMoonEquation(double alpha);

	// Holidays
	//! @return true for a Tibetan leap month (CC:UE 21.6)
	static bool tibetanLeapMonth(const QVector<int> tYM);
	//! @return true for a Tibetan leap day (CC:UE 21.7)
	static bool tibetanLeapDay(const QVector<int> tYMD);
	//! @return RD of Losar (New Year) with year number in Tibetan calendar (CC:UE 21.8)
	static int losar(const int tYear);
	//! @return a QVector<int> of possible Losar (New Year) dates in a Gregorian year (CC:UE 21.9)
	//! @note the QVector usually contains one element, but in 719 had zero, in 718 and 12698 two.
	static QVector<int> tibetanNewYear(const int gYear);

public:
	static const int tibetanEpoch; //! RD of Gregorian {-127, december, 7}. CC:UE 21.1

protected:
	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> monthNames;
	static QMap<int, QString> animals;
	static QMap<int, QString> elements;
	static QMap<int, QString> yogas;
	static QMap<int, QString> naksatras;
	static QMap<int, QString> karanas;
};

#endif
