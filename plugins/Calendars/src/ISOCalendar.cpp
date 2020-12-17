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

#include "StelTranslator.hpp"
#include "ISOCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

ISOCalendar::ISOCalendar(double jd): GregorianCalendar (jd)
{
}

// Set a calendar date from the Julian day number
void ISOCalendar::setJD(double JD)
{
	this->JD=JD;

	const int rd=fixedFromJD(JD);

	parts=isoFromFixed(rd);

	int hour, minute, second;
	StelUtils::getTimeFromJulianDay(JD, &hour, &minute, &second, nullptr);
	parts << hour << minute << second;

	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Week[1...53]-Day[1...7]
void ISOCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	int rd=fixedFromISO(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

//// get a stringlist of calendar date elements sorted from the largest to the smallest.
//// The order depends on the actual calendar
//QStringList ISOCalendar::getDateStrings()
//{
//	// If we don't change this, just delete this inherited version
//	QStringList list;
//	list << QString::number(parts.at(0));
//	list << QString::number(parts.at(1));
//	list << QString::number(parts.at(2));
//	list << weekday(JD);
//
//	return list;
//}

//! get a formatted complete string for a date
QString ISOCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1, %2 %3, %4")
			.arg(str.at(3)) // weekday
			.arg(q_("Week"))
			.arg(str.at(1)) // week, numerical
			.arg(str.at(0));// year
}

int ISOCalendar::fixedFromISO(QVector<int> iso)
{
	const int year=iso.at(0);
	const int week=iso.at(1);
	const int day=iso.at(2);

	return nthKday(week, sunday, year-1, december, 28)+day;
}

QVector<int> ISOCalendar::isoFromFixed(int rd)
{
	int approx = gregorianYearFromFixed(rd-3);
	int year   = rd>=fixedFromISO({approx+1, 1, 1}) ? approx+1 : approx;
	int week   = StelUtils::intFloorDiv(rd-fixedFromISO({year, 1, 1}), 7)+1;
	int day    = StelUtils::amod(rd, 7);
	return {year, week, day};
}
