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
#include "IcelandicCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"


const int IcelandicCalendar::icelandicEpoch=GregorianCalendar::fixedFromGregorian({1, GregorianCalendar::april, 19});  //! RD of April 19, AD1 (Gregorian).


int IcelandicCalendar::icelandicSummer(int iyear)
{
	static const QVector<int>a({97, 24, 1,0});
	QVector<int>y=toRadix(iyear, {4, 25, 4}); // split year into mixed-radix notation

	int apr19=icelandicEpoch + 365*(iyear-1)+rdCorrSum(y, a, 0);
	return kdayOnOrAfter(thursday, apr19);
}

IcelandicCalendar::IcelandicCalendar(double jd): Calendar (jd)
{
	retranslate();
}

// Set a calendar date from the Julian day number
void IcelandicCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=icelandicFromFixed(rd);
	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Season[summer=90 | winter=270]-Week[1...12]-WeekDay[0=Sunday...6]
void IcelandicCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromIcelandic(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Season, Month, MonthName, Week, Day, DayName
QStringList IcelandicCalendar::getDateStrings() const
{
	QStringList list;
	// 0: year
	list << QString::number(parts.at(0));
	// 1: season
	if (parts.at(1)==Calendar::summer) list << qc_("summer", "Icelandic calendar");
	else if (parts.at(1)==Calendar::winter) list << qc_("winter", "Icelandic calendar");
	else list << "errorSeason";

	// 2: month number in season
	QPair<int, int> month=icelandicMonth(parts);
	list << QString::number(month.first);
	// 3: month name
	list << icelandicMonthName(month.second);
	// 4: week
	list << QString::number(parts.at(2));
	// 5: weekday number (0=sun...6)
	list << QString::number(parts.at(3));
	// 6: weekday name
	list << weekDayNames.value(parts.at(3), "error");

	return list;
}

// get a formatted complete string for a date
QString IcelandicCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TRANSLATORS: Icelandic calendar phrase like "[Weekday] of week [number] of [summer|winter] (Month [number]: [name]) - [year]"
	return QString(q_("%1 of week %2 of %3 (Month %4: %5) - %6"))
			.arg(str.at(6)) // weekday
			.arg(str.at(4)) // week
			.arg(str.at(1)) // season
			.arg(str.at(2)) // month
			.arg(str.at(3)) // monthName
			.arg(str.at(0));// year
}

/* ====================================================
 * Functions from CC.UE ch6
 */

// RD from {year, season, week[1...27], weekday[0...6]}
int IcelandicCalendar::fixedFromIcelandic(QVector<int> icelandic)
{
	const int year=icelandic.at(0);
	const Calendar::Season season=static_cast<Calendar::Season>(icelandic.at(1));
	const int week=icelandic.at(2);
	const Calendar::Day weekday=static_cast<Calendar::Day>(icelandic.at(3));

	int start = (season==summer ? icelandicSummer(year) : icelandicWinter(year));
	int shift = (season==summer ? thursday : saturday );

	return start+7*(week-1)+StelUtils::imod((weekday-shift), 7);
}

QVector<int> IcelandicCalendar::icelandicFromFixed(int rd)
{
	const int approx=StelUtils::intFloorDiv(400*(rd-icelandicEpoch+369), 146097);
	const int year=(rd>=icelandicSummer(approx) ? approx : approx-1 );
	const Season season=(rd<icelandicWinter(year) ? summer : winter);
	const int start = (season==summer ? icelandicSummer(year) : icelandicWinter(year));
	const int week = StelUtils::intFloorDiv(rd-start, 7) + 1;
	const int weekday=dayOfWeekFromFixed(rd);

	return {year, season, week, weekday};
}

// returns true for leap years
bool IcelandicCalendar::isLeap(int iyear)
{
	return (icelandicSummer(iyear+1)-icelandicSummer(iyear)) != 364;
}

QPair<int,int> IcelandicCalendar::icelandicMonth(QVector<int>iDate)
{
	const int rd=fixedFromIcelandic(iDate);
	const int year=iDate.at(0);
	const Season season=static_cast<Season>(iDate.at(1));
	const int midsummer=icelandicWinter(year)-90;
	int start;
	if (season==winter)
		start=icelandicWinter(year);
	else if (rd>=midsummer)
		start=midsummer-90;
	else if (rd<icelandicSummer(year)+90)
		start=icelandicSummer(year);
	else
		start=midsummer;

	int month=StelUtils::intFloorDiv(rd-start, 30)+1;
	return QPair<int,int>(month, month+(season==winter ? 6 : 0));
}

QMap<int, QString> IcelandicCalendar::weekDayNames;
QMap<int, QString> IcelandicCalendar::monthNames;

void IcelandicCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	weekDayNames={
		{0, qc_("Sunnudagur"  , "Icelandic day name")},
		{1, qc_("Mánudagur"   , "Icelandic day name")},
		{2, qc_("Þriðjudagur" , "Icelandic day name")},
		{3, qc_("Miðvikudagur", "Icelandic day name")},
		{4, qc_("Fimmtudagur" , "Icelandic day name")},
		{5, qc_("Föstudagur"  , "Icelandic day name")},
		{6, qc_("Laugardagur" , "Icelandic day name")}};
	monthNames={
		{ 0, qc_("unnamed"     , "Icelandic month name")},
		{ 1, qc_("Harpa"       , "Icelandic month name")},
		{ 2, qc_("Skerpla"     , "Icelandic month name")},
		{ 3, qc_("Sólmánuður"  , "Icelandic month name")},
		{ 4, qc_("Heyannir"    , "Icelandic month name")},
		{ 5, qc_("Tvímánuður"  , "Icelandic month name")},
		{ 6, qc_("Haustmánuður", "Icelandic month name")},
		{ 7, qc_("Gormánuður"  , "Icelandic month name")},
		{ 8, qc_("Ýlir"        , "Icelandic month name")},
		{ 9, qc_("Mörsugur"    , "Icelandic month name")},
		{10, qc_("Þorri"       , "Icelandic month name")},
		{11, qc_("Góa"         , "Icelandic month name")},
		{12, qc_("Einmánuður"  , "Icelandic month name")}};
}
