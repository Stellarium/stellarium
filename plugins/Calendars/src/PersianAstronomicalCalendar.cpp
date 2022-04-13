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
#include "PersianAstronomicalCalendar.hpp"
#include "JulianCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "RomanCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocation.hpp"

PersianAstronomicalCalendar::PersianAstronomicalCalendar(double jd): PersianArithmeticCalendar(jd)
{
	PersianAstronomicalCalendar::retranslate();
}

//const StelLocation PersianAstronomicalCalendar::tehran( "Tehran",  "ir", "Southern Asia", 51.42f,    35.68f,    1100, 7153, "Asia/Tehran", 9, 'C');
const StelLocation PersianAstronomicalCalendar::isfahan("Isfahan", "ir", "Southern Asia", 51.67462f, 32.65246f, 1578, 1547, "Asia/Tehran", 9, 'R');

// Set a calendar date from the Julian day number
void PersianAstronomicalCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=persianAstronomicalFromFixed(rd);

	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...13]-Day[1...30]
// Time is not changed!
void PersianAstronomicalCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "PersianAstronomicalCalendar::setDate:" << parts;
	this->parts=parts;

	double rd=fixedFromPersianAstronomical(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

int PersianAstronomicalCalendar::fixedFromPersianAstronomical(QVector<int> persian)
{
	const int year =persian.value(0);
	const int month=persian.value(1);
	const int day  =persian.value(2);

	int newYear=persianNewYearOnOrBefore(persianEpoch+180+std::lround(std::floor(meanTropicalYear*(0<year? year-1 : year))));

	return newYear-1+(month<=7 ? 31*(month-1) : 30*(month-1)+6)+day;
}

QVector<int> PersianAstronomicalCalendar::persianAstronomicalFromFixed(int rd)
{
	const int newYear=persianNewYearOnOrBefore(rd);
	const int y=qRound((newYear-persianEpoch)/meanTropicalYear)+1;
	const int year= 0<y ? y : y-1; // no year zero
	const int dayOfYear=rd-fixedFromPersianAstronomical({year, 1, 1})+1;
	const int month= dayOfYear<=186 ? qRound(ceil(dayOfYear/31.)) : qRound(ceil((dayOfYear-6)/30.));
	const int day=rd-fixedFromPersianAstronomical({year, month, 1})+1;
	return {year, month, day};
}

int PersianAstronomicalCalendar::persianNewYearOnOrBefore(int rd)
{
	double approx=estimatePriorSolarLongitude(static_cast<double>(Calendar::spring), middayInTehran(rd));

	int day=qRound(floor(approx))-2;
	double lng;
	do {
		day++;
		lng=modInterval(solarLongitude(middayInTehran(day)), -180., 180.);
	} while (lng<=static_cast<double>(Calendar::spring)); // +2. // It seems the +2 is erroneous!
	return day;
}

// find RD number of Persian New Year (Nowruz)
int PersianAstronomicalCalendar::nowruz(const int gYear)
{
	const int pYear=gYear-GregorianCalendar::gregorianYearFromFixed(persianEpoch)+1;
	const int y = pYear<=0 ? pYear-1 : pYear;
	return fixedFromPersianAstronomical({y, 1, 1});
}
