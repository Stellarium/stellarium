/*
 * Stellarium
 * Copyright (C) 2023 Andy Kirkham
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

/*
 * The standard QDateTime type only stores to the millisecond but 
 * satellite propargation models use to microsecond timimg. 
 * For Stellarium display purposes millisecond accuracy is 
 * probably good enough. However, the Unit Tests source data
 * expectations from common SGP4 models and therefore fail to
 * agree to routines that do not account for microsecond timing.
 * This class therefore is to allow for us timings. 
 * 
 * Epoch times are stored as JD and JDF.
 */

#ifndef SATELLITES_OMMDATETIME_HPP
#define SATELLITES_OMMDATETIME_HPP

#include <QString>
#include <QDateTime>
#include <QSharedPointer>

//! @class OMMDateTime
//! Auxiliary class for the %Satellites plugin.
//! @author Andy Kirkham
//! @ingroup satellites
//!
//! The legacy TLE format held the elements epoch in the
//! format YYDDDdddddddd hereas the newer XML and JSON
//! formats use extended ISO8601. Since the QDateTime 
//! objects can only hold millisecond timing (and legacy 
//! TLE use microsecond) this new class exists to hold
//! the epoch. It's stored internally as a Julian DAY/time
//! record and can decode the TLE format and the extended
//! ISO8601 format.
class OMMDateTime
{
public:

	//! The type this Epoch was decoded from.
	enum Type {
		STR_TLE,
		STR_ISO8601
	};

	OMMDateTime();
	~OMMDateTime();

	OMMDateTime(const OMMDateTime& other) { *this = other; }

	OMMDateTime & operator=(const OMMDateTime &);

	OMMDateTime(const QString& s, Type t = STR_TLE);

	double getJulianDay() { return m_epoch_jd; }

	double getJulianFrac() { return m_epoch_jd_frac; }

	double getJulian() { return m_epoch_jd + m_epoch_jd_frac; }

private:
	void ctorTle(const QString & s);
	void ctorISO(const QString & s);

	double m_epoch_jd{};
	double m_epoch_jd_frac{};
};

#endif
