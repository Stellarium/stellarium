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
#include "EgyptianCalendar.hpp"
//#include "JulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

// Nabonasssar era
const int EgyptianCalendar::egyptianEpoch=fixedFromJD(1448638, false); //! RD of JD1448638.


EgyptianCalendar::EgyptianCalendar(double jd): Calendar(jd)
{
	EgyptianCalendar::retranslate();
}

QMap<int, QString> EgyptianCalendar::monthNames;

void EgyptianCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	monthNames={
		{ 1, qc_("Thoth"        , "Egyptian month name")},
		{ 2, qc_("Phaophi"      , "Egyptian month name")},
		{ 3, qc_("Athyr"        , "Egyptian month name")},
		{ 4, qc_("Choiak"       , "Egyptian month name")},
		{ 5, qc_("Tybi"         , "Egyptian month name")},
		{ 6, qc_("Mechir"       , "Egyptian month name")},
		{ 7, qc_("Phamenoth"    , "Egyptian month name")},
		{ 8, qc_("Pharmuthi"    , "Egyptian month name")},
		{ 9, qc_("Pachon"       , "Egyptian month name")},
		{10, qc_("Payni"        , "Egyptian month name")},
		{11, qc_("Epiphi"       , "Egyptian month name")},
		{12, qc_("Mesori"       , "Egyptian month name")},
		{13, qc_("(Epagomenae)" , "Egyptian month name")}};
}

// Set a calendar date from the Julian day number
void EgyptianCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=egyptianFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day
QStringList EgyptianCalendar::getDateStrings() const
{
	QStringList list;
	list << QString::number(parts.at(0));
	list << QString::number(parts.at(1));
	list << monthNames.value(parts.at(1));
	list << QString::number(parts.at(2));

	return list;
}

// get a formatted complete string for a date
QString EgyptianCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1 - %2 (%3) - %4 %5")
			.arg(str.at(3)) // day
			.arg(str.at(1)) // month, numerical
			.arg(str.at(2)) // month, name
			.arg(str.at(0)) // year
			.arg(q_("Nabonassar Era"));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void EgyptianCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromEgyptian(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// actually implementation of alt-fixed-from-egyptian
int EgyptianCalendar::fixedFromEgyptian(QVector<int> egyptian)
{
	return egyptianEpoch + rdCorrSum(egyptian, {365, 30,1}, -1);
}

QVector<int> EgyptianCalendar::egyptianFromFixed(int rd)
{
	const int days=rd-egyptianEpoch;
	const int year = StelUtils::intFloorDiv(days, 365)+1;
	const int month=StelUtils::intFloorDiv(StelUtils::imod(days, 365), 30)+1;
	const int day=days-365*(year-1)-30*(month-1)+1;

	return {year, month, day};
}
