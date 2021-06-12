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
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

GregorianCalendar::GregorianCalendar(double jd): JulianCalendar (jd)
{
}

// Set a calendar date from the Julian day number
void GregorianCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=gregorianFromFixed(rd);
	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
void GregorianCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "GregorianCalendar::setDate:" << parts;
	this->parts=parts;

	double rd=fixedFromGregorian(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, DayName
// Gregorian with year zero, negative years and no AD/BC counting.
QStringList GregorianCalendar::getDateStrings() const
{
	QStringList list;
	list << QString::number(parts.at(0));
	list << QString::number(parts.at(1));
	list << QString::number(parts.at(2));
	list << weekday(JD);

	return list;
}

// get a formatted complete string for a date
QString GregorianCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TODO: Maybe use QDate with user's localisation here? Weekday has to be taken from our results, though.
	return QString("%1, %2 - %3 (%4) - %5")
			.arg(str.at(3)) // weekday
			.arg(str.at(2)) // day
			.arg(str.at(1)) // month, numerical
			.arg(monthNames.value(parts.at(1))) // month, name
			.arg(str.at(0));// year
}

// returns true for leap years
bool GregorianCalendar::isLeap(int year)
{
	if (year % 100 == 0)
		return (year % 400 == 0);
	else
		return (year % 4 == 0);
}

/* ====================================================
 * Functions from CC.UE ch2
 */

int GregorianCalendar::fixedFromGregorian(QVector<int> gregorian)
{
	const int year=gregorian.at(0);
	const int month=gregorian.at(1);
	const int day=gregorian.at(2);

	int rd=gregorianEpoch-1+365*(year-1)+StelUtils::intFloorDiv((year-1), 4)-StelUtils::intFloorDiv((year-1), 100)
			+StelUtils::intFloorDiv((year-1), 400)+(367*month-362)/12+day;
	if (month>2)
		rd+=(isLeap(year) ? -1 : -2);
	return rd;
}


// @return RD date of the n-th k-day in a date on the Gregorian calendar
int GregorianCalendar::nthKday(const int n, const Calendar::Day k, const int gYear, const int gMonth, const int gDay)
{
//	if (n==0)
//	{
//		qWarning() << "GregorianCalendar::nthKday called with n==0";
//		return bogus;
//		Q_ASSERT(n!=0);
//	}
//	else
		if (n>=0)
		return 7*n+kdayBefore(k, fixedFromGregorian({gYear, gMonth, gDay}));
	else
		return 7*n+kdayAfter(k, fixedFromGregorian({gYear, gMonth, gDay}));
}

int GregorianCalendar::gregorianYearFromFixed(int rd)
{
	const int d0=rd-gregorianEpoch;
	const int n400=StelUtils::intFloorDiv(d0, 146097);
	const int d1=StelUtils::imod(d0, 146097);
	const int n100=StelUtils::intFloorDiv(d1, 36524);
	const int d2=StelUtils::imod(d1, 36524);
	const int n4=StelUtils::intFloorDiv(d2, 1461);
	const int d3=StelUtils::imod(d2, 1461);
	const int n1=StelUtils::intFloorDiv(d3, 365);
	const int year=400*n400+100*n100+4*n4+n1;
	if ((n100==4) || (n1==4))
		return year;
	else
		return year+1;
}

QVector<int> GregorianCalendar::gregorianFromFixed(int rd)
{
	int year=gregorianYearFromFixed(rd);
	int priorDays=rd-gregorianNewYear(year);
	int correction=2;
	if (rd<fixedFromGregorian({year, march, 1}))
		correction=0;
	else if (isLeap(year))
		correction=1;
	int month=StelUtils::intFloorDiv(12*(priorDays+correction)+373, 367);
	int day = rd-fixedFromGregorian({year, month, 1})+1;
	return {year, month, day};
}

//! Gregorian Easter sunday (RD) from chapter 9.2
int GregorianCalendar::easter(int gYear)
{
	const int century=StelUtils::intFloorDiv(gYear, 100) + 1;
	const int shiftedEpact=StelUtils::imod(14+11*StelUtils::imod(gYear, 19)
					       - StelUtils::intFloorDiv(3*century, 4)
					       + StelUtils::intFloorDiv(5+8*century, 25), 30);
	int adjustedEpact=shiftedEpact;
	if (shiftedEpact==0) adjustedEpact++;
	else if ((shiftedEpact==1) && (10< StelUtils::imod(gYear, 19))) adjustedEpact++;

	const int paschalMoon=fixedFromGregorian({gYear, april, 19}) - adjustedEpact;
	return kdayAfter(sunday, paschalMoon);
}

//! Orthodox Easter sunday (RD) from chapter 9.1
int GregorianCalendar::orthodoxEaster(int gYear)
{
	const int shiftedEpact=StelUtils::imod(14+11*StelUtils::imod(gYear, 19), 30);
	const int jYear=(gYear>0 ? gYear : gYear-1);

	const int paschalMoon=fixedFromJulian({jYear, april, 19}) - shiftedEpact;
	return kdayAfter(sunday, paschalMoon);
}
