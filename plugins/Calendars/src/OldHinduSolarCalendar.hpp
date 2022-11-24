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

#ifndef OLDHINDUSOLARCALENDAR_HPP
#define OLDHINDUSOLARCALENDAR_HPP

#include "Calendar.hpp"

//! @class OldHinduSolarCalendar
//! Functions for the Old Hindu Solar calendar described in CC:UE chapter 10.
//! @author Georg Zotti
//! @ingroup calendars
//! The old hindu Solar calendar as given in CC.UE describes the one given in the (First) Arya Siddhanta of Aryabhata (499 C.E.),
//! as amended by Lalla (ca. 720-790 C.E.).
//! There are many variations which are not described in CC.UE and therefore not handled in this implementation.
//!

class OldHinduSolarCalendar : public Calendar
{
	Q_OBJECT

public:
	OldHinduSolarCalendar(double jd);

	~OldHinduSolarCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-Day[1...30]
	void setDate(QVector<int> parts) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! {Year, JovianCycleNr, JovianCycleName, Month, MonthName, Day, DayName}
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! Return Hindu day count from RD
	static int hinduDayCount(int rd) {return rd-hinduEpoch;}

	static int jovianYear(int rd); //! year index in Jovian cycle [1..60]

	static int fixedFromOldHinduSolar(QVector<int> parts);
	static QVector<int> oldHinduSolarFromFixed(int rd);

	//! configure details for future output. This could be used in a startup script.
	//! valid styles: 0|1 (real difference not documented in CC.UE!)
	void setWeekdayStyle(int style);
	//! configure details for future output. This could be used in a startup script.
	//! valid styles: 0=Vedic or 1=Sanskrit or 2=Zodiacal
	void setMonthStyle(int style);

protected:
	static const int hinduEpoch; // RD -1132959.
	constexpr static const double aryaSolarYear    = 1577917500./4320000.0;
	constexpr static const double aryaSolarMonth   = aryaSolarYear/12.0;
	constexpr static const double aryaJovianPeriod = 1577917500./364224.0;

	static QMap<int, QString> weekDayNames;
	static QMap<int, QString> monthNames;	
	static QMap<int, QString> jovianNames;
private:
	int weekdayStyle; // 0 or 1
	int monthStyle; // 0=Vedic or 1=Sanskrit or 2=Zodiacal
};

#endif
