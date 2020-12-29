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
#include "OldHinduLuniSolarCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"



OldHinduLuniSolarCalendar::OldHinduLuniSolarCalendar(double jd): OldHinduSolarCalendar(jd)
{
	OldHinduLuniSolarCalendar::retranslate();
}

QMap<int, QString> OldHinduLuniSolarCalendar::monthNames;

void OldHinduLuniSolarCalendar::retranslate()
{
	// fill the name lists with translated month names.
	monthNames={
		{  1, qc_("Caitra"     , "old Hindu month name")},
		{  2, qc_("Vaiśākha"   , "old Hindu month name")},
		{  3, qc_("Jyeṣṭha"    , "old Hindu month name")},
		{  4, qc_("Āṣāḑha"     , "old Hindu month name")},
		{  5, qc_("Śrāvaṇa"    , "old Hindu month name")},
		{  6, qc_("Bhādrapada" , "old Hindu month name")},
		{  7, qc_("Āśvina"     , "old Hindu month name")},
		{  8, qc_("Kārtika"    , "old Hindu month name")},
		{  9, qc_("Mārgaśīrṣa" , "old Hindu month name")},
		{ 10, qc_("Pauṣa"      , "old Hindu month name")},
		{ 11, qc_("Māgha"      , "old Hindu month name")},
		{ 12, qc_("Phālguna"   , "old Hindu month name")}};
}

// Set a calendar date from the Julian day number
void OldHinduLuniSolarCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=oldHinduLunarFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// {Year, Month, MonthName, leap[0|1], Day, DayName}
// Again, in this plugin only, note no year zero, and AD/BC counting.
QStringList OldHinduLuniSolarCalendar::getDateStrings() const
{
	const int rd=fixedFromOldHinduLunar(parts);
	const int dow=dayOfWeekFromFixed(rd);

	QStringList list;
	list << QString::number(parts.at(0));        // 0:year
	list << QString::number(parts.at(1));        // 1:Month nr
	list << monthNames.value(parts.at(1));       // 2:Month name
	list << QString::number(parts.at(2));        // 3:leap?
	list << QString::number(parts.at(3));        // 4:Day nr in month
	list << weekDayNames.value(dow, "error");    // 5:weekday
	return list;
}

// get a formatted complete string for a date
QString OldHinduLuniSolarCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TRANSLATORS: Old Hindu Lunar calendar phrase like "[Weekday], [number] - [MonthNumber] ([MonthName]) [leap] - [year]"
	return QString("%1, %2 - %3 (%4) %5- %6 K.Y.")
			.arg(str.at(5)) // weekday
			.arg(str.at(4)) // day
			.arg(str.at(1)) // month, numerical
			.arg(str.at(2)) // month, name
			.arg(str.at(3)=="1" ? qc_("[leap]", "short indicator for leap month") : "") // leap note
			.arg(str.at(0));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void OldHinduLuniSolarCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;
	double rd=fixedFromOldHinduSolar(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}


// parts={ year, month, leap, day}
int OldHinduLuniSolarCalendar::fixedFromOldHinduLunar(QVector<int> parts)
{
	const int year=parts.at(0);
	const int month=parts.at(1);
	const bool leap=parts.at(2);
	const int day=parts.at(3);

	const double mina = (12*year-1)*aryaSolarMonth;
	const double lunarNewYear=aryaLunarMonth*(std::floor(mina/aryaLunarMonth)+1.);

	double rdd=hinduEpoch+lunarNewYear;

	double monthArg=month-1;
	if ((!leap) && (std::ceil((lunarNewYear-mina)/(aryaSolarMonth-aryaLunarMonth)) <= month))
		monthArg=month;
	rdd+= aryaLunarMonth*monthArg;
	rdd+= (day-1)*aryaLunarDay-0.25;

	return std::lround(std::ceil(rdd));
}

// return { year, month, leap, day}
QVector<int> OldHinduLuniSolarCalendar::oldHinduLunarFromFixed(int rd)
{
	const double sun=hinduDayCount(rd)+0.25;
	const double newMoon=sun-StelUtils::fmodpos(sun, aryaLunarMonth);
	bool leapA = aryaSolarMonth-aryaLunarMonth >= StelUtils::fmodpos(newMoon, aryaSolarMonth);
	bool leapB = StelUtils::fmodpos(newMoon, aryaSolarMonth) > 0.;
	int leap = leapA && leapB;

	int month=StelUtils::imod(std::lround(std::ceil(newMoon/aryaSolarMonth)), 12)+1;
	int day  =StelUtils::imod(std::lround(std::floor(sun/aryaLunarDay)), 30)+1;
	int year=std::lround(std::ceil((newMoon+aryaSolarMonth)/aryaSolarYear))-1;

	return {year, month, leap, day};
}

bool OldHinduLuniSolarCalendar::isLeap(int lYear)
{
	return StelUtils::fmodpos(lYear*aryaSolarYear-aryaSolarMonth, aryaLunarMonth) >= 23902504679/1282400064;
}
