/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#ifndef _TESTDATES_HPP_
#define _TESTDATES_HPP_

#include <QObject>
#include <QtTest>

class TestDates : public QObject
{
Q_OBJECT
private slots:
	void dateRoundTrip();
	void formatting();
	void testRolloverAndValidity();
	void testJulianDays();
	void benchmarkOldGetDateFromJulianDay();
	void benchmarkGetDateFromJulianDayFloatingPoint();
	void benchmarkGetDateFromJulianDay();
	void benchmarkOldGetJDFromDate();
	void benchmarkGetJDFromDate();

private:
	void testJulianDaysRange(int jd_first, int jd_last);
};

#endif // _TESTDATES_HPP_

