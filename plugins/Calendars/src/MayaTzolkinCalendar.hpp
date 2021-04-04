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

#ifndef MAYATZOLKINCALENDAR_HPP
#define MAYATZOLKINCALENDAR_HPP

#include "Calendar.hpp"

//! The Maya Tzolkin was a 260-day calendar consisting of a 13-day count and a 20-name count.
//! The implementation follows CC.
//! The "name" is not a month but also changes every day. The traditional writing is number-name, which is also used in the parts.
class MayaTzolkinCalendar : public Calendar
{
	Q_OBJECT

public:
	MayaTzolkinCalendar(double jd);

	virtual ~MayaTzolkinCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! dayNumber[1..13]-nameIndex[1..20]
	//! We face a problem as the date is not unique. We can only find the date before current JD which matches the parts.
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! dayNumber[1..13]-name
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

public:
	//! usually internal, but used by MayaHaabCalendar.
	static QVector<int> mayanTzolkinFromFixed(int rd);

	//! return Tzolkin name for index [0...19]
	static QString tzolkinName(int i) {return tzolkinNames.value(i);}

	// ordinal of {number, name}
	inline static int mayanTzolkinOrdinal(QVector<int> tzolkin) {return StelUtils::imod((tzolkin.at(0) - 1 + 39*(tzolkin.at(0)-tzolkin.at(1))), 260);}
	static const int mayanTzolkinEpoch;

private:
	static int mayanTzolkinOnOrBefore(QVector<int> tzolkin, int rd);
	static QMap<int, QString> tzolkinNames;
};

#endif
