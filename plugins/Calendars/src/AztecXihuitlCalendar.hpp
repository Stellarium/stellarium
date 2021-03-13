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

#ifndef AZTECXIHUITLCALENDAR_HPP
#define AZTECXIHUITLCALENDAR_HPP

#include "Calendar.hpp"

//! The Aztec Xihuitl was similar to the Maya Haab, a 365-day Solar calendar without intercalation.
//! The major difference is that days are counted 1...20, not 0...19. 
//! Similar to the Egyptian calendar, after 18 months of 20 days there was a short "month" of 5 extra days (Nemontemi). 
//! Some communities place this month of "worthless" days elsewhere in the sequence, this cannot be mapped here. 
//! The implementation follows CC.
class AztecXihuitlCalendar : public Calendar
{
	Q_OBJECT

public:
	AztecXihuitlCalendar(double jd);

	virtual ~AztecXihuitlCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! month[1..19]-day[1..20]
	//! We face a problem as the year is not counted. We can only find the date before current JD which matches the parts.
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! monthName-day[1..20]
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

	//! get a formatted complete string for a date
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

public:
	//! find number in sequence from a xihuitl date of {month[1...19], day[1...20]}
	inline static int aztecXihuitlOrdinal(QVector<int> xihuitl) {return (xihuitl.at(0)-1)*20+xihuitl.at(1)-1;}

	//! get 2-part vector of xihuitl date from RD
	static QVector<int> aztecXihuitlFromFixed(int rd);

	//! find RD number of a Xihuitl date on or before rd.
	static int aztecXihuitlOnOrBefore(QVector<int> xihuitl, int rd);

	//! get RD of a combined date on or before rd. They repeat every 18980 days.
	static int aztecXihuitlTonalpohualliOnOrBefore(QVector<int>xihuitl, QVector<int>tonalpohualli, int rd);

	//! Aztec date of fall of Tenochtitlan
	static const int aztecCorrelation;
	static const int aztecXihuitlCorrelation;


private:
	static QMap<int, QString> monthNames;
};

#endif
