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

#ifndef ISOCALENDAR_HPP
#define ISOCALENDAR_HPP

#include "GregorianCalendar.hpp"

//! The ISO calendar counts weeks [1...53] in Gregorian years. Some dates in the border weeks 1 and 53 may lie outside of the respective year. Week 1 contains the first Thursday.

class ISOCalendar : public GregorianCalendar
{
	Q_OBJECT

public:
	ISOCalendar(double jd);

	virtual ~ISOCalendar() Q_DECL_OVERRIDE {}

public slots:
	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Week[1...53]-Day[1...7]
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

//	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
//	//! The order depends on the actual calendar
//	virtual QStringList getDateStrings() Q_DECL_OVERRIDE;

	//! get a formatted complete string for a date
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

public:
	static QVector<int> isoFromFixed(int rd);
	static int fixedFromISO(QVector<int> iso);
};

#endif
