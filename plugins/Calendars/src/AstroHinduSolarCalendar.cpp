/*
 * Copyright (C) 2022 Georg Zotti
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
#include "AstroHinduSolarCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"


AstroHinduSolarCalendar::AstroHinduSolarCalendar(double jd): NewHinduCalendar(jd){}

// Set a calendar date from the Julian day number
void AstroHinduSolarCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=astroHinduSolarFromFixed(rd);

	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// {Year, Month[1...12], leapMonth[0|1], Day[1...30], leapDay}
// Time is not changed!
void AstroHinduSolarCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;
	double rd=fixedFromAstroHinduSolar(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Input {year, month, day}
// {Year, MonthNr, MonthName, DayNr, DayName}
QStringList AstroHinduSolarCalendar::getDateStrings() const
{
	const int rd=fixedFromAstroHinduSolar(parts);
	const int dow=dayOfWeekFromFixed(rd);

	QStringList list;
	list << QString::number(parts.at(0));        // 0:year
	list << QString::number(parts.at(1));        // 1:Month nr
	list << monthNames.value(parts.at(1));       // 2:Month name
	list << QString::number(parts.at(2));        // 3:Day nr in month
	list << weekDayNames.value(dow, "error");    // 4:weekday
	return list;
}
