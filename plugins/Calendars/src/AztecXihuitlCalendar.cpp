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
#include "JulianCalendar.hpp"
#include "AztecXihuitlCalendar.hpp"
#include "AztecTonalpohualliCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"


const int AztecXihuitlCalendar::aztecCorrelation = JulianCalendar::fixedFromJulian({1521, JulianCalendar::august, 13});
const int AztecXihuitlCalendar::aztecXihuitlCorrelation = aztecCorrelation - aztecXihuitlOrdinal({11, 2});


AztecXihuitlCalendar::AztecXihuitlCalendar(double jd): Calendar(jd)
{
	retranslate();
}

QMap<int, QString> AztecXihuitlCalendar::monthNames;

void AztecXihuitlCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	monthNames={
		{ 1, qc_("Izcalli (Sprout)"                 , "Aztec Xihuitl month name")},
		{ 2, qc_("Atlcahualo (Water left)"          , "Aztec Xihuitl month name")},
		{ 3, qc_("Tlacaxipehualiztli (Man flaying)" , "Aztec Xihuitl month name")},
		{ 4, qc_("Tozoztontli (1-Vigil)"            , "Aztec Xihuitl month name")},
		{ 5, qc_("Huei Tozoztli (2-Vigil)"          , "Aztec Xihuitl month name")},
		{ 6, qc_("Toxcatl (Drought)"                , "Aztec Xihuitl month name")},
		{ 7, qc_("Etzalcualiztli (Eating bean soup)", "Aztec Xihuitl month name")},
		{ 8, qc_("Tecuilhuitontli (1-Lord’s feast)" , "Aztec Xihuitl month name")},
		{ 9, qc_("Huei Tecuilhuitl (2-Lord’s feast)", "Aztec Xihuitl month name")},
		{10, qc_("Tlaxochimaco (Give flowers)"      , "Aztec Xihuitl month name")},
		{11, qc_("Xocotlhuetzi (Fruit falls)"       , "Aztec Xihuitl month name")},
		{12, qc_("Ochpaniztli (Road sweeping)"      , "Aztec Xihuitl month name")},
		{13, qc_("Teotleco (God arrives)"           , "Aztec Xihuitl month name")},
		{14, qc_("Tepeilhuitl (Mountain feast)"     , "Aztec Xihuitl month name")},
		{15, qc_("Quecholli (Macaw)"                , "Aztec Xihuitl month name")},
		{16, qc_("Panquetzaliztli (Flag raising)"   , "Aztec Xihuitl month name")},
		{17, qc_("Atemoztli (Falling water)"        , "Aztec Xihuitl month name")},
		{18, qc_("Tititl (Storm)"                   , "Aztec Xihuitl month name")},
		{19, qc_("Nemontemi (Full in vain)"         , "Aztec Xihuitl month name")}};
}

// Set a calendar date from the Julian day number
void AztecXihuitlCalendar::setJD(double JD)
{
	this->JD=JD;

	const int rd=fixedFromJD(JD, true);
	const int count=StelUtils::imod(rd-aztecXihuitlCorrelation, 365);
	const int day = count % 20 + 1;
	const int month=count/20 + 1;

	parts = { month, day };

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// monthName-dayNumber
QStringList AztecXihuitlCalendar::getDateStrings() const
{
	QStringList list;
	list << monthNames.value(parts.at(0));
	list << QString::number(parts.at(1));

	return list;
}


// get a formatted complete string for a date
QString AztecXihuitlCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1 %2")
			.arg(str.at(1))
			.arg(str.at(0));
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// month-day
// We face a problem as the year is not unique. We can only find the date before current JD which matches the parts.
void AztecXihuitlCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	const int rdOnOrBefore=aztecXihuitlOnOrBefore(parts, fixedFromJD(JD));

	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rdOnOrBefore+frac, true);

	emit jdChanged(JD);
}

QVector<int> AztecXihuitlCalendar::aztecXihuitlFromFixed(int rd)
{
	const int count = StelUtils::imod(rd-aztecXihuitlCorrelation, 365);
	const int day = count % 20 + 1;
	const int month=count/20 + 1;
	return {month, day};
}

int AztecXihuitlCalendar::aztecXihuitlOnOrBefore(QVector<int> xihuitl, int rd)
{
	return modInterval(aztecXihuitlCorrelation+aztecXihuitlOrdinal(xihuitl), rd, rd-365);
}

// get RD of a combined date
int AztecXihuitlCalendar::aztecXihuitlTonalpohualliOnOrBefore(QVector<int>xihuitl, QVector<int>tonalpohualli, int rd)
{
	const int xihuitlCount=aztecXihuitlOrdinal(xihuitl)+aztecXihuitlCorrelation;
	const int tonalpohualliCount=AztecTonalpohualliCalendar::aztecTonalpohualliOrdinal(tonalpohualli)+AztecTonalpohualliCalendar::aztecTonalpohualliCorrelation;
	const int diff=tonalpohualliCount-xihuitlCount;
	if (diff % 5 ==0)
		return modInterval(xihuitlCount+365*diff, rd, rd-18980);
	else
		return bogus;
}
