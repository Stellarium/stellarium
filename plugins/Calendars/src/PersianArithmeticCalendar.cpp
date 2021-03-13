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
#include "PersianArithmeticCalendar.hpp"
#include "JulianCalendar.hpp"
#include "RomanCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

PersianArithmeticCalendar::PersianArithmeticCalendar(double jd): Calendar(jd)
{
	PersianArithmeticCalendar::retranslate();
}

const int PersianArithmeticCalendar::persianEpoch=JulianCalendar::fixedFromJulian({622, JulianCalendar::march, 19});
QMap<int, QString> PersianArithmeticCalendar::weekDayNames;
QMap<int, QString> PersianArithmeticCalendar::monthNames;

void PersianArithmeticCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	weekDayNames={
		{0, qc_("Yek-shanbēh" , "Persian weekday name")}, // Sun
		{1, qc_("Do-shanbēh"  , "Persian weekday name")}, // Mon
		{2, qc_("Se-shanbēh"  , "Persian weekday name")}, // Tue
		{3, qc_("Chār-shanbēh", "Persian weekday name")}, // Wed
		{4, qc_("Panj-shanbēh", "Persian weekday name")}, // Thu
		{5, qc_("Jom`ēh"      , "Persian weekday name")}, // Fri
		{6, qc_("Shanbēh"     , "Persian weekday name")}};// Sat
	monthNames={
		{ 1, qc_("Farvardīn"     , "Persian month name")},
		{ 2, qc_("Ordībehesht"   , "Persian month name")},
		{ 3, qc_("Xordād"        , "Persian month name")},
		{ 4, qc_("Tīr"           , "Persian month name")},
		{ 5, qc_("Mordād"        , "Persian month name")},
		{ 6, qc_("Shahrīvar"     , "Persian month name")},
		{ 7, qc_("Mehr"          , "Persian month name")},
		{ 8, qc_("Ābān"          , "Persian month name")},
		{ 9, qc_("Āzar"          , "Persian month name")},
		{10, qc_("Dey"           , "Persian month name")},
		{11, qc_("Bahman"        , "Persian month name")},
		{12, qc_("Esfand"        , "Persian month name")}};
}

// Set a calendar date from the Julian day number
void PersianArithmeticCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=persianArithmeticFromFixed(rd);

	emit partsChanged(parts);
}

// get a 6-part stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, WeekdayName
QStringList PersianArithmeticCalendar::getDateStrings() const
{
	const int rd=fixedFromPersianArithmetic(parts);
	const int dow=dayOfWeekFromFixed(rd);

	QStringList list;
	list << QString::number(parts.at(0));            // 0
	list << QString::number(parts.at(1));            // 1
	list << monthNames.value(parts.at(1), "ERROR");  // 2
	list << QString::number(parts.at(2));            // 3
	list << weekDayNames.value(dow);                 // 4:weekday

	return list;
}

// get a formatted complete string for a date
QString PersianArithmeticCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();

	return QString("%1, %2 - %3 (%4) - %5 A.P.")
			.arg(str.at(4)) // weekday
			.arg(str.at(3)) // day
			.arg(str.at(1)) // month, numerical
			.arg(str.at(2)) // month, name
			.arg(str.at(0));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...13]-Day[1...30]
// Time is not changed!
void PersianArithmeticCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "FrenchArithmeticCalendar::setDate:" << parts;
	this->parts=parts;

	double rd=fixedFromPersianArithmetic(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// returns true for leap years. This is arithmetic-persian-leap-year?(p-year) [15.7] in the CC.UE book.
bool PersianArithmeticCalendar::isLeap(int pYear)
{
	int y = pYear - (0<pYear ? 474 : 473);
	int year = StelUtils::imod(y, 2820) + 474;

	return StelUtils::imod((year+38)*31, 128) < 31;
}

int PersianArithmeticCalendar::fixedFromPersianArithmetic(QVector<int> persian)
{
	const int pYear=persian.at(0);
	const int month=persian.at(1);
	const int day=persian.at(2);

	int y = pYear - (0<pYear ? 474 : 473);
	int year = StelUtils::imod(y, 2820) + 474;


	int rd=persianEpoch-1
			+1029983*StelUtils::intFloorDiv(y, 2820)+365*(year-1)
			+StelUtils::intFloorDiv(31*year-5, 128);
	if (month <=7)
		rd+=31*(month-1);
	else
		rd+=30*(month-1)+6;
	rd+=day;
	return rd;
}

QVector<int> PersianArithmeticCalendar::persianArithmeticFromFixed(int rd)
{
	int d0=rd-fixedFromPersianArithmetic({475, 1, 1});
	int n2820=StelUtils::intFloorDivLL(d0, 1029983);
	int d1= StelUtils::imod(d0, 1029983);
	int y2820=(d1==1029982 ? 2820 : StelUtils::intFloorDivLL(128*d1+46878, 46751));
	int year=474+2820*n2820+y2820;
	if (year<=0) year--;

	int dayOfYear=1+rd-fixedFromPersianArithmetic({year, 1, 1});
	int month=(dayOfYear<=186 ? lround(ceil(dayOfYear/31.)) : lround(ceil((dayOfYear-6)/30.)));
	int day=rd - fixedFromPersianArithmetic({year, month, 1}) + 1;

	return {year, month, day};
}
