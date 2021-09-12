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
#include "EthiopicCalendar.hpp"
#include "JulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

// Ethiopian era. This is remarkably when Augustus re-activated leap year counting.
const int EthiopicCalendar::ethiopicEpoch=JulianCalendar::fixedFromJulian({8, JulianCalendar::august, 29}); //! RD 2796.


EthiopicCalendar::EthiopicCalendar(double jd): CopticCalendar(jd)
{
	EthiopicCalendar::retranslate();
}

QMap<int, QString> EthiopicCalendar::monthNames;
QMap<int, QString> EthiopicCalendar::dayNames;

void EthiopicCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	monthNames={
		{ 1, qc_("Maskaram", "Ethiopic month name")},
		{ 2, qc_("Teqemt"  , "Ethiopic month name")},
		{ 3, qc_("Hedār"   , "Ethiopic month name")},
		{ 4, qc_("Takhśāś" , "Ethiopic month name")},
		{ 5, qc_("Ṭer"     , "Ethiopic month name")},
		{ 6, qc_("Yakātit" , "Ethiopic month name")},
		{ 7, qc_("Magābit" , "Ethiopic month name")},
		{ 8, qc_("Miyāzyā" , "Ethiopic month name")},
		{ 9, qc_("Genbot"  , "Ethiopic month name")},
		{10, qc_("Sanē"    , "Ethiopic month name")},
		{11, qc_("Ḥamlē"   , "Ethiopic month name")},
		{12, qc_("Naḥasē"  , "Ethiopic month name")},
		{13, qc_("Pāguemēn", "Ethiopic month name")}};
	dayNames={
		{ 0, qc_("Iḥud"      , "Ethiopic day name")},
		{ 1, qc_("Sanyo"     , "Ethiopic day name")},
		{ 2, qc_("Maksanyo"  , "Ethiopic day name")},
		{ 3, qc_("Rob/Rabu`e", "Ethiopic day name")},
		{ 4, qc_("H̱amus"     , "Ethiopic day name")},
		{ 5, qc_("Arb"       , "Ethiopic day name")},
		{ 6, qc_("Kidāmmē"   , "Ethiopic day name")}};
}

// Set a calendar date from the Julian day number
void EthiopicCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=ethiopicFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, dayName
QStringList EthiopicCalendar::getDateStrings() const
{
	QStringList list;
	list << QString::number(parts.at(0));
	list << QString::number(parts.at(1));
	list << monthNames.value(parts.at(1));
	list << QString::number(parts.at(2));
	list << weekday(JD);

	return list;
}

// get a formatted complete string for a date
QString EthiopicCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1, %2 - %3 (%4) - %5 %6")
			.arg(str.at(4)) // dayname
			.arg(str.at(3)) // day
			.arg(str.at(1)) // month, numerical
			.arg(str.at(2)) // month, name
			.arg(str.at(0)) // year
			.arg(q_("Ethiopic Era of Mercy"));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void EthiopicCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromEthiopic(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

int EthiopicCalendar::fixedFromEthiopic(QVector<int> ethiopic)
{
	return ethiopicEpoch +CopticCalendar::fixedFromCoptic(ethiopic)-CopticCalendar::copticEpoch;
}

QVector<int> EthiopicCalendar::ethiopicFromFixed(int rd)
{
	return CopticCalendar::copticFromFixed(rd+CopticCalendar::copticEpoch-ethiopicEpoch);
}

// return name of week day
QString EthiopicCalendar::weekday(double jd)
{
	const int dow = StelUtils::getDayOfWeek(jd+StelApp::getInstance().getCore()->getUTCOffset(jd)/24.);
	return dayNames.value(dow);
}

// returns true for leap years. NOT IN BOOK???
bool EthiopicCalendar::isLeap(int year)
{
	return StelUtils::imod(year, 4)==3;
}

