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
#include "CopticCalendar.hpp"
#include "JulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

// Diocletian era
const int CopticCalendar::copticEpoch=JulianCalendar::fixedFromJulian({284, JulianCalendar::august, 29}); //! RD 103605.


CopticCalendar::CopticCalendar(double jd): JulianCalendar(jd)
{
	CopticCalendar::retranslate();
}

QMap<int, QString> CopticCalendar::monthNames;
QMap<int, QString> CopticCalendar::dayNames;

void CopticCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	monthNames={
		{ 1, qc_("Thoouth"  , "Coptic month name")},
		{ 2, qc_("Paope"    , "Coptic month name")},
		{ 3, qc_("Athōr"    , "Coptic month name")},
		{ 4, qc_("Koiak"    , "Coptic month name")},
		{ 5, qc_("Tōbe"     , "Coptic month name")},
		{ 6, qc_("Meshir"   , "Coptic month name")},
		{ 7, qc_("Paremotep", "Coptic month name")},
		{ 8, qc_("Parmoute" , "Coptic month name")},
		{ 9, qc_("Pashons"  , "Coptic month name")},
		{10, qc_("Paōne"    , "Coptic month name")},
		{11, qc_("Epēp"     , "Coptic month name")},
		{12, qc_("Mesorē"   , "Coptic month name")},
		{13, qc_("Epagomenē", "Coptic month name")}};
	dayNames={
		{ 0, qc_("Tkyriake" , "Coptic day name")},
		{ 1, qc_("Pesnau"   , "Coptic day name")},
		{ 2, qc_("Pshoment" , "Coptic day name")},
		{ 3, qc_("Peftoou"  , "Coptic day name")},
		{ 4, qc_("Ptiou"    , "Coptic day name")},
		{ 5, qc_("Psoou"    , "Coptic day name")},
		{ 6, qc_("Psabbaton", "Coptic day name")}};
}

// Set a calendar date from the Julian day number
void CopticCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=copticFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, dayName
QStringList CopticCalendar::getDateStrings() const
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
QString CopticCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1, %2 - %3 (%4) - %5 %6")
			.arg(str.at(4)) // dayname
			.arg(str.at(3)) // day
			.arg(str.at(1)) // month, numerical
			.arg(str.at(2)) // month, name
			.arg(str.at(0)) // year
			.arg(q_("Era Martyrum"));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...30]
// Time is not changed!
void CopticCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromCoptic(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

int CopticCalendar::fixedFromCoptic(QVector<int> coptic)
{
	const int year=coptic.at(0);
	const int month=coptic.at(1);
	const int day=coptic.at(2);

	return copticEpoch - 1 + 365*(year-1) + StelUtils::intFloorDiv(year, 4) + 30*(month-1) + day;
}

QVector<int> CopticCalendar::copticFromFixed(int rd)
{
	const int year  = StelUtils::intFloorDiv(4 * (rd-copticEpoch) + 1463, 1461);
	const int month = StelUtils::intFloorDiv(rd-fixedFromCoptic({year, 1, 1}), 30)+1;
	const int day   = rd + 1 - fixedFromCoptic({year, month, 1});

	return {year, month, day};
}

// return name of week day
QString CopticCalendar::weekday(double jd)
{
	const int dow = StelUtils::getDayOfWeek(jd+StelApp::getInstance().getCore()->getUTCOffset(jd)/24.);
	return dayNames.value(dow);
}

// returns true for leap years
bool CopticCalendar::isLeap(int year)
{
	return StelUtils::imod(year, 4)==3;
}

