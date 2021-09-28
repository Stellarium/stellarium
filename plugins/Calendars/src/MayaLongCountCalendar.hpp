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

#ifndef MAYALONGCOUNTCALENDAR_HPP
#define MAYALONGCOUNTCALENDAR_HPP

#include "Calendar.hpp"

//! The Maya Long Count is a 5-part integer day count number in base 20.
//! The implementation follows CC.UE
class MayaLongCountCalendar : public Calendar
{
	Q_OBJECT

public:
	MayaLongCountCalendar(double jd);

	virtual ~MayaLongCountCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE {}

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! baktun[0..19]-katun[0..19]-tun{0..19]-uinal[0..17]-kin[0..19]
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! baktun[0..19]-katun[0..19]-tun[0..19]-uinal[0..17]-kin[0..19]
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

	//! get a formatted complete string for a date
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

public:
	//! get RD date from Long Count date
	static int fixedFromMayanLongCount(QVector<int> longCount);
	//! get Long Count date from RD date
	static QVector<int>mayaLongCountFromFixed(int rd);

	static const long int mayanEpoch;
};

#endif
