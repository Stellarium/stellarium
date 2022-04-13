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
#include "FrenchAstronomicalCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "RomanCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

FrenchAstronomicalCalendar::FrenchAstronomicalCalendar(double jd): FrenchArithmeticCalendar(jd)
{
}

// Set a calendar date from the Julian day number
void FrenchAstronomicalCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=frenchAstronomicalFromFixed(rd);

	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...13]-Day[1...30]
// Time is not changed!
void FrenchAstronomicalCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "FrenchAstronomicalCalendar::setDate:" << parts;
	this->parts=parts;

	double rd=fixedFromFrenchAstronomical(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// returns true for leap years. This is french-leap-year?(f-year) [17.7] in the CC:UE book.
bool FrenchAstronomicalCalendar::isLeap(int fYear)
{
	return fixedFromFrenchAstronomical({fYear+1, 1, 1})-fixedFromFrenchAstronomical({fYear, 1, 1}) > 365;
}

int FrenchAstronomicalCalendar::fixedFromFrenchAstronomical(QVector<int> french)
{
	const int year =french.value(0);
	const int month=french.value(1);
	const int day  =french.value(2);

	const int newYear=frenchNewYearOnOrBefore(qRound(floor(frenchEpoch+180+meanTropicalYear*(year-1))));
	return newYear-1+30*(month-1)+day;
}

QVector<int> FrenchAstronomicalCalendar::frenchAstronomicalFromFixed(int rd)
{
	const int newYear=frenchNewYearOnOrBefore(rd);
	const int year=qRound((newYear-frenchEpoch)/meanTropicalYear)+1;
	const int month=StelUtils::intFloorDiv(rd-newYear,30)+1;
	const int day=StelUtils::imod(rd-newYear,30)+1;

	return {year, month, day};
}


//! @return last new year (CC:UE 17.3)
int FrenchAstronomicalCalendar::frenchNewYearOnOrBefore(int rd)
{
	const double approx=estimatePriorSolarLongitude(static_cast<double>(Calendar::autumn), midnightInParis(rd));

	int day=qRound(floor(approx))-2;
	double lng;
	do {
		day++;
		lng=solarLongitude(midnightInParis(day));
	} while (lng<=static_cast<double>(Calendar::autumn));
	return day;
}
