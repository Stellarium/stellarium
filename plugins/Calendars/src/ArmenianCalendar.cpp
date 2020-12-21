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
#include "ArmenianCalendar.hpp"
#include "JulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

const int ArmenianCalendar::armenianEpoch=JulianCalendar::fixedFromJulian({552, JulianCalendar::july, 11}); //! RD 201443.


ArmenianCalendar::ArmenianCalendar(double jd): EgyptianCalendar(jd)
{
	ArmenianCalendar::retranslate();
}

QMap<int, QString> ArmenianCalendar::monthNames;

void ArmenianCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	monthNames={
		{ 1, qc_("Nawasardi" , "Armenian month name")},
		{ 2, qc_("Hor̄i"      , "Armenian month name")},
		{ 3, qc_("Sahmi"     , "Armenian month name")},
		{ 4, qc_("Trē"       , "Armenian month name")},
		{ 5, qc_("K'aloch"   , "Armenian month name")},
		{ 6, qc_("Arach"     , "Armenian month name")},
		{ 7, qc_("Mehekani"  , "Armenian month name")},
		{ 8, qc_("Areg"      , "Armenian month name")},
		{ 9, qc_("Ahekani"   , "Armenian month name")},
		{10, qc_("Mareri"    , "Armenian month name")},
		{11, qc_("Margach"   , "Armenian month name")},
		{12, qc_("Hrotich"   , "Armenian month name")},
		{13, qc_("(aweleach)", "Armenian month name")}};
}

// Set a calendar date from the Julian day number
void ArmenianCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=armenianFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day
QStringList ArmenianCalendar::getDateStrings() const
{
	QStringList list;
	list << QString::number(parts.at(0));
	list << QString::number(parts.at(1));
	list << monthNames.value(parts.at(1));
	list << QString::number(parts.at(2));

	return list;
}

// get a formatted complete string for a date
QString ArmenianCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1 - %2 (%3) - %4 %5")
			.arg(str.at(3)) // day
			.arg(str.at(1)) // month, numerical
			.arg(str.at(2)) // month, name
			.arg(str.at(0)) // year
			.arg(q_("Armenian Era"));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void ArmenianCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromArmenian(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// actually implementation of alt-fixed-from-egyptian
int ArmenianCalendar::fixedFromArmenian(QVector<int> armenian)
{
	return armenianEpoch + fixedFromEgyptian(armenian)-EgyptianCalendar::egyptianEpoch;
}

QVector<int> ArmenianCalendar::armenianFromFixed(int rd)
{
	return EgyptianCalendar::egyptianFromFixed(rd+EgyptianCalendar::egyptianEpoch-armenianEpoch);
}
