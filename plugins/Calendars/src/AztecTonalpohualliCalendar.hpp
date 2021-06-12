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

#ifndef AZTECTONALPOHUALLICALENDAR_HPP
#define AZTECTONALPOHUALLICALENDAR_HPP

#include "MayaTzolkinCalendar.hpp"

//! The Aztec Tonalpohualli is just like the Maya Tzolkin, a 260-day calendar consisting of a 13-day count and a 20-name count.
//! The implementation follows CC.
//! The "name" is not a month but also changes every day. The traditional writing is number-name, which is also used in the parts.
class AztecTonalpohualliCalendar: public MayaTzolkinCalendar
{
	Q_OBJECT

public:
	AztecTonalpohualliCalendar(double jd);

	virtual ~AztecTonalpohualliCalendar() Q_DECL_OVERRIDE {}

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
	//! Return ordinal in Tonalpohualli cycle.
	//! @arg tonalpohualli is a QVector<int> of {number, name} (typo in book...)
	inline static int aztecTonalpohualliOrdinal(QVector<int> tonalpohualli) {return StelUtils::imod((tonalpohualli.at(0) - 1 + 39*(tonalpohualli.at(0)-tonalpohualli.at(1))), 260);}

	//! get 2-part vector of Tonalpohualli date from RD
	static QVector<int> aztecTonalpohualliFromFixed(int rd);

	//! A constant to correlate calendars
	static const int aztecTonalpohualliCorrelation;

private:
	static int aztecTonalpohualliOnOrBefore(QVector<int> tonalpohualli, int rd);
	static QMap<int, QString> tonalpohualliNames;
};

#endif
