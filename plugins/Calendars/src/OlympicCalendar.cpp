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
#include "OlympicCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

OlympicCalendar::OlympicCalendar(double jd): JulianCalendar(jd)
{
	OlympicCalendar::retranslate();
}

// Set a calendar date from the Julian day number
void OlympicCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=julianFromFixed(rd);
	QVector<int>oDate=olympiadFromJulianYear(parts.at(0));
	parts[0]=oDate.at(1);
	parts.prepend(oDate.at(0));

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Olympiad, Year, Month, MonthName, Day, DayName
QStringList OlympicCalendar::getDateStrings() const
{
	QStringList list;
	list << QString::number(parts.at(0));
	list << QString::number(parts.at(1));
	list << QString::number(parts.at(2));
	list << QString::number(parts.at(3));

	return list;
}

// get a formatted complete string for a date
QString OlympicCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString(q_("Day %1, month %2 in the year %3 of the %4th Olympiad"))
			.arg(str.at(3)) // day
			.arg(str.at(2)) // month, numerical
			.arg(str.at(1)) // year
			.arg(str.at(0));// olympiad
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// The elements only change years. The calendar is else based on the Julian, so
// unless given explicitly, months and days in the Julian calendar will not be changed.
// parts={olympiad, year [, month, day]}

void OlympicCalendar::setDate(QVector<int> parts)
{
	if (parts.length()<4)
	{
		this->parts[0]=parts.at(0);
		this->parts[1]=parts.at(1);
	}
	else
		this->parts=parts;

	int jYear=julianYearFromOlympiad({this->parts.at(0), this->parts.at(1)});
	double rd=fixedFromJulian({jYear, this->parts.at(2), this->parts.at(3)});
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}


int OlympicCalendar::julianYearFromOlympiad(QVector<int>odate)
{
	int cycle=odate.at(0);
	int year=odate.at(1);
	int years=olympiadStart+4*(cycle-1)+year-1;
	return (years < 0 ? years : years+1);
}

QVector<int> OlympicCalendar::olympiadFromJulianYear(int jYear)
{
	int years=jYear-olympiadStart-(jYear<0?0:1);
	return {static_cast<int>(StelUtils::intFloorDiv(years, 4))+1, StelUtils::imod(years, 4)+1};
}


