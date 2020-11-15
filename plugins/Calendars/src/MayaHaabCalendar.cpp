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
#include "MayaHaabCalendar.hpp"
#include "MayaLongCountCalendar.hpp"
#include "StelUtils.hpp"

const int MayaHaabCalendar::mayanHaabEpoch=MayaLongCountCalendar::mayanEpoch-MayaHaabCalendar::mayanHaabOrdinal(18,8);

MayaHaabCalendar::MayaHaabCalendar(double jd): Calendar(jd)
{
	retranslate();
}

QMap<int, QString> MayaHaabCalendar::monthNames;

void MayaHaabCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	monthNames={
		{ 1, qc_("Pop"   , "Maya Haab month name")},
		{ 2, qc_("Uo"    , "Maya Haab month name")},
		{ 3, qc_("Zip"   , "Maya Haab month name")},
		{ 4, qc_("Zotz"  , "Maya Haab month name")},
		{ 5, qc_("Tzec"  , "Maya Haab month name")},
		{ 6, qc_("Xul"   , "Maya Haab month name")},
		{ 7, qc_("Yaxkin", "Maya Haab month name")},
		{ 8, qc_("Mol"   , "Maya Haab month name")},
		{ 9, qc_("Chen"  , "Maya Haab month name")},
		{10, qc_("Yax"   , "Maya Haab month name")},
		{11, qc_("Zac"   , "Maya Haab month name")},
		{12, qc_("Ceh"   , "Maya Haab month name")},
		{13, qc_("Mac"   , "Maya Haab month name")},
		{14, qc_("Kankin", "Maya Haab month name")},
		{15, qc_("Muan"  , "Maya Haab month name")},
		{16, qc_("Pax"   , "Maya Haab month name")},
		{17, qc_("Kayab" , "Maya Haab month name")},
		{18, qc_("Cumku" , "Maya Haab month name")},
		{19, qc_("Uayeb" , "Maya Haab month name")}};
}

// Set a calendar date from the Julian day number
void MayaHaabCalendar::setJD(double JD)
{
	this->JD=JD;

	const int rd=fixedFromJD(JD);
	const int count=(rd-mayanHaabEpoch) % 365;
	const int day = count % 20;
	const int month=count/20 + 1;

	parts.clear();
	parts << month << day;

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// baktun-katun-tun-uinal-kin
QStringList MayaHaabCalendar::getDateStrings()
{
	QStringList list;
	list << monthNames.value(std::lround(parts.at(0)));
	list << QString::number(std::lround(parts.at(1)));

	return list;
}


// get a formatted complete string for a date
QString MayaHaabCalendar::getFormattedDateString()
{
	QStringList str=getDateStrings();
	return QString("%1 %2")
			.arg(str.at(1))
			.arg(str.at(0));
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// month-day
// We face a problem as the year is not unique. We can only find the date before current JD which matches the parts.
void MayaHaabCalendar::setDate(QVector<int> parts)
{
	// Problem: This sets time to midnight. We need to keep and reset the fractional day.
	const double dayFraction=JD-std::floor(JD-.5);

	this->parts=parts;

	const int rdOnOrBefore=mayanHaabOnOrBefore(parts, fixedFromJD(JD));

	JD=jdFromFixed(rdOnOrBefore)+dayFraction;
	emit jdChanged(JD);
}

int MayaHaabCalendar::mayanHaabOnOrBefore(QVector<int> haab, int rd)
{
	return rd-((rd-mayanHaabEpoch-mayanHaabOrdinal(std::lround(haab.at(0)), std::lround(haab.at(1))))  % 365 );
}
