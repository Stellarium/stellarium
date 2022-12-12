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

#include "BahaiAstronomicalCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

BahaiAstronomicalCalendar::BahaiAstronomicalCalendar(double jd): BahaiArithmeticCalendar (jd)
{
	BahaiAstronomicalCalendar::retranslate();
}

// Set a calendar date from the Julian day number
void BahaiAstronomicalCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=bahaiAstronomicalFromFixed(rd);
	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Major-Cycle-Year-Month[0...19]-Day[1...19]
void BahaiAstronomicalCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "BahaiAstronomicalCalendar::setDate:" << parts;
	this->parts=parts;

	double rd=fixedFromBahaiAstronomical(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Major, Cycle, Year, YearNameInCycle, Month, MonthName, Day, DayName, WeekdayName
QStringList BahaiAstronomicalCalendar::getDateStrings() const
{
//	const int dow=(int)floor(JD+2.5) % 7;

	const int rd=fixedFromBahaiAstronomical(parts);
	const int dow=dayOfWeekFromFixed(rd+1);

	QStringList list;
	list << QString::number(parts.at(0));    // 0 major
	list << QString::number(parts.at(1));    // 1 cycle
	list << QString::number(parts.at(2));    // 2 year
	list << yearNames.value(parts.at(2));    // 3 yearName
	list << QString::number(parts.at(3));    // 4 month
	list << cycleNames.value(parts.at(3));   // 5 monthName
	list << QString::number(parts.at(4));    // 6 day
	list << cycleNames.value(parts.at(4));   // 7 dayName
	list << weekDayNames.value(dow);         // 8 weekdayName

	return list;
}

/* ====================================================
 * Functions from CC.UE ch16.3
 */

const StelLocation BahaiAstronomicalCalendar::bahaiLocation( "Tehran",  "ir", "Southern Asia", 51.423056f, 35.696111f, 0, 7153, "Asia/Tehran", 9, 'C');

// Return moment of sunset in Tehran as defined by Bahai astronomical calendar rules. (CC:UE 16.6)
double BahaiAstronomicalCalendar::bahaiSunset(int rd)
{
	return universalFromStandard(sunset(rd, bahaiLocation), bahaiLocation);
}

// Return RD of New year  (CC:UE 16.7)
int BahaiAstronomicalCalendar::astroBahaiNewYearOnOrBefore(int rd)
{
	double approx=estimatePriorSolarLongitude(static_cast<double>(Calendar::spring), bahaiSunset(rd));

	int day=qRound(floor(approx))-2;
	double lng;
	do {
		day++;
		lng=modInterval(solarLongitude(bahaiSunset(day)), -180., 180.);
	} while (lng<=static_cast<double>(Calendar::spring));
	return day;
}

// Return R.D. of date given in the Bahai Astronomical calendar. (CC:UE 16.8)
int BahaiAstronomicalCalendar::fixedFromBahaiAstronomical(QVector<int> bahai5)
{
	const int major=bahai5.value(0);
	const int cycle=bahai5.value(1);
	const int year =bahai5.value(2);
	const int month=bahai5.value(3);
	const int day  =bahai5.value(4);

	const int years=361*(major-1)+19*(cycle-1)+year;
	if (month==19)
		return astroBahaiNewYearOnOrBefore(bahaiEpoch+floor(meanTropicalYear*(years+0.5)))-20+day;
	else if (month==ayyam_i_Ha)
		return astroBahaiNewYearOnOrBefore(bahaiEpoch+floor(meanTropicalYear*(years-0.5)))+341+day;
	else
		return astroBahaiNewYearOnOrBefore(bahaiEpoch+floor(meanTropicalYear*(years-0.5)))+(month-1)*19+day-1;
}

QVector<int> BahaiAstronomicalCalendar::bahaiAstronomicalFromFixed(int rd)
{
	const int newYear=astroBahaiNewYearOnOrBefore(rd);
	const int years = round((newYear-bahaiEpoch)/meanTropicalYear);

	const int major = StelUtils::intFloorDiv(years, 361)+1;
	const int cycle = StelUtils::intFloorDiv(StelUtils::imod(years, 361), 19)+1;
	const int year = StelUtils::imod(years, 19) + 1;
	const int days = rd-newYear;
	int month = StelUtils::intFloorDiv(days, 19)+1;
	if (rd>=fixedFromBahaiAstronomical({major, cycle, year, 19,1}))
		month = 19;
	else if (rd>=fixedFromBahaiAstronomical({major, cycle, year, ayyam_i_Ha,1}))
		month = ayyam_i_Ha;
	const int day = rd+1-fixedFromBahaiAstronomical({major, cycle, year, month, 1});
	return {major, cycle, year, month, day};
}

int BahaiAstronomicalCalendar::nawRuz(int gYear)
{
	return astroBahaiNewYearOnOrBefore(GregorianCalendar::gregorianNewYear(gYear+1));
}

int BahaiAstronomicalCalendar::feastOfRidvan(int gYear)
{
	return nawRuz(gYear)+31;
}

int BahaiAstronomicalCalendar::birthOfTheBab(int gYear)
{
	const int ny=nawRuz(gYear);
	const double set1=bahaiSunset(ny);
	const double m1 = newMoonAtOrAfter(set1);
	const double m8 = newMoonAtOrAfter(m1+190);
	const int day=fixedFromMoment(m8);
	const double set8=bahaiSunset(day);
	if (m8<set8)
		return day+1;
	else
		return day+2;
}
