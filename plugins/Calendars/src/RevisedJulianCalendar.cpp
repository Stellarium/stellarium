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
#include "RevisedJulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

RevisedJulianCalendar::RevisedJulianCalendar(double jd): JulianCalendar(jd)
{
}

// Set a calendar date from the Julian day number
void RevisedJulianCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=revisedJulianFromFixed(rd);

	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void RevisedJulianCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "RevisedJulianCalendar::setDate:" << parts;
	this->parts=parts;
	// For the Julian calendar, we really have no year 0 in this plugin.
	Q_ASSERT(parts.at(0) != 0);

	double rd=fixedFromRevisedJulian(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// returns true for leap years. The algorithm only works correctly for positive years!
// But we make the switchover in 325.
bool RevisedJulianCalendar::isLeap(int year)
{
	if (year<325)
		return JulianCalendar::isLeap(year);

	bool leap= (StelUtils::imod(year, 4) == 0);
	if (leap && (StelUtils::imod(year, 100) == 0))
	{
		int century=StelUtils::imod(year, 900);
		leap=(century==200) || (century==600);
	}
	return leap;
}

int RevisedJulianCalendar::fixedFromRevisedJulian(QVector<int> revisedJulian)
{
	const int year =revisedJulian.value(0);
	const int month=revisedJulian.value(1);
	const int day  =revisedJulian.value(2);
	// Year BC make no sense here! Don't bother dealing with the leap years...
	// But note that the calendars were parallel in 325 (Nicaea)
	if (year<325)
		return JulianCalendar::fixedFromJulian(revisedJulian);

	const int priorYear=year-1;
	int fixedDay=revisedJulianEpoch+365*priorYear+StelUtils::intFloorDiv(priorYear,4)+StelUtils::intFloorDiv(367*month-362, 12)+day-1;
	// If month is after February then subtract 1 day for a leap year or subtract 2 days for a common year:
	if (month>2)
		fixedDay -= isLeap(year) ? 1 : 2;
	// Finally subtract a day for each prior century year (most of which are non-leap) and then add back in the number of prior century leap years:
	int priorCenturies=StelUtils::intFloorDiv(priorYear, 100);
	fixedDay += -priorCenturies + StelUtils::intFloorDiv(2*priorCenturies+6, 9);
	return fixedDay;
}

QVector<int> RevisedJulianCalendar::revisedJulianFromFixed(int rd)
{
	static const int rd325=fixedFromJulian({325, 6, 30});
	if (rd<rd325)
		return JulianCalendar::julianFromFixed(rd);
	const int days = rd-revisedJulianEpoch+1;
	const int priorCenturies = StelUtils::intFloorDiv(days, 36524);                              // n100 in Gregorian algorithm
	int remainingDays = days-36524*priorCenturies-StelUtils::intFloorDiv(2*priorCenturies+6, 9); //
	const int priorSubcycles = StelUtils::intFloorDiv(remainingDays, 1461);                      // n4 in Gregorian algorithm
	remainingDays = StelUtils::imod(remainingDays,1461);                                         // d3
	const int priorSubcycleYears = StelUtils::intFloorDiv(remainingDays, 365);                   // n1
	int year = 100 * priorCenturies + 4 * priorSubcycles + priorSubcycleYears;
	// Add a correction which was omitted in Wikipedia!
	if ( !((priorSubcycleYears==4) || (priorCenturies==2)))
		year+=1;

	remainingDays = StelUtils::imod(remainingDays, 365);
	if (remainingDays == 0)
	{
	    //This is either the 365th day of a common year, or the 365th or 366th day of a leap year. Either way, we have to decrement the year because we went one year too far:
	    year--;
	    remainingDays = isLeap(year) && (priorSubcycles==0) ? 366 : 365;
	}
	int priorDays = remainingDays - 1;
	int correction = isLeap(year) ? 1 : 0;
	correction = (priorDays < (31+28+correction)) ? 0 : 2 - correction;
	int month = StelUtils::intFloorDiv((12 * (priorDays + correction) + 373) , 367);
	const int day=rd-fixedFromRevisedJulian({year, month, 1})+1;
	return {year, month, day};
}
