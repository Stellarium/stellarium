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
#include "IslamicCalendar.hpp"
#include "JulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

IslamicCalendar::IslamicCalendar(double jd): Calendar(jd)
{
	IslamicCalendar::retranslate();
}

const int IslamicCalendar::islamicEpoch=JulianCalendar::fixedFromJulian({622, JulianCalendar::july, 16});
QMap<int, QString> IslamicCalendar::weekDayNames;
QMap<int, QString> IslamicCalendar::monthNames;

void IslamicCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	weekDayNames={
		{0, qc_("yaum al-aḥad (first day)"       , "Islamic day name")},
		{1, qc_("yaum al-ithnaya (second day)"   , "Islamic day name")},
		{2, qc_("yaum ath-thalāthāʾ (third day)" , "Islamic day name")},
		{3, qc_("yaum al-arbaʿāʾ (fourth day)"   , "Islamic day name")},
		{4, qc_("yaum al-ẖamis (fifth day)"      , "Islamic day name")},
		{5, qc_("yaum al-jumʿa (day of assembly)", "Islamic day name")},
		{6, qc_("yaum as-sabt (sabbath day)"     , "Islamic day name")}};
	monthNames={
		{ 1, qc_("Muḥarram"            , "Islamic month name")},
		{ 2, qc_("Ṣafar"               , "Islamic month name")},
		{ 3, qc_("Rabīʿ I (al-Awwal)"  , "Islamic month name")},
		{ 4, qc_("Rabīʿ II (al-Āḥir)"  , "Islamic month name")},
		{ 5, qc_("Jumādā I (al-Ūlā)"   , "Islamic month name")},
		{ 6, qc_("Jumādā II (al-Āḥira)", "Islamic month name")},
		{ 7, qc_("Rajab"               , "Islamic month name")},
		{ 8, qc_("Shaʿbān"             , "Islamic month name")},
		{ 9, qc_("Ramaḍān"             , "Islamic month name")},
		{10, qc_("Shawwāl"             , "Islamic month name")},
		{11, qc_("Dhu al-Qaʿda"        , "Islamic month name")},
		{12, qc_("Dhu al-Ḥijja"        , "Islamic month name")}};
}

// Set a calendar date from the Julian day number
void IslamicCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=islamicFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, WeekDayName
QStringList IslamicCalendar::getDateStrings() const
{
	const int rd=fixedFromIslamic(parts);
	const int dow=dayOfWeekFromFixed(rd);

	QStringList list;
	list << QString::number(parts.at(0));            // 0:year
	list << QString::number(parts.at(1));            // 1:month
	list << monthNames.value(parts.at(1), "error");  // 2:monthName
	list << QString::number(parts.at(2));            // 3:day
	list << weekDayNames.value(dow);                 // 4:weekday

	return list;
}

// get a formatted complete string for a date
QString IslamicCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1, %2 - %3 (%4) - %5 A.H.")
			.arg(str.at(4)) // weekday
			.arg(str.at(3)) // day
			.arg(str.at(1)) // month, numerical
			.arg(str.at(2)) // month, name
			.arg(str.at(0));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...30]
// Time is not changed!
void IslamicCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromIslamic(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

int IslamicCalendar::fixedFromIslamic(QVector<int> islamic)
{
	const int year  = islamic.at(0);
	const int month = islamic.at(1);
	const int day   = islamic.at(2);
	int rd=islamicEpoch-1+(year-1)*354;
	rd += StelUtils::intFloorDiv(3+11*year, 30);
	rd += 29*(month-1);
	rd += StelUtils::intFloorDiv(month, 2) + day;
	return rd;
}

QVector<int> IslamicCalendar::islamicFromFixed(int rd)
{
	const int year = StelUtils::intFloorDiv(30*(rd-islamicEpoch)+10646, 10631);
	const int priorDays = rd-fixedFromIslamic({year, 1, 1});
	const int month = StelUtils::intFloorDiv(11*priorDays + 330, 325);
	const int day = rd - fixedFromIslamic({year, month, 1}) + 1;

	return {year, month, day};
}

//! Return true if iYear is an Islamic leap year
bool IslamicCalendar::isLeap(int iYear)
{
	return StelUtils::imod(14+11*iYear, 30) < 11;
}
