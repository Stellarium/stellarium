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
#include "MayaTzolkinCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

const int MayaHaabCalendar::mayanHaabEpoch=MayaLongCountCalendar::mayanEpoch-MayaHaabCalendar::mayanHaabOrdinal({18, 8});

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

	const int rd=fixedFromJD(JD, true);
	const int count=StelUtils::imod(rd-mayanHaabEpoch, 365);
	const int day = count % 20;
	const int month=count/20 + 1;

	parts = { month, day };

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// monthName-dayNumber
QStringList MayaHaabCalendar::getDateStrings() const
{
	QStringList list;
	list << monthNames.value(parts.at(0));
	list << QString::number(parts.at(1));

	return list;
}


// get a formatted complete string for a date
QString MayaHaabCalendar::getFormattedDateString() const
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
	this->parts=parts;

	const int rdOnOrBefore=mayanHaabOnOrBefore(parts, fixedFromJD(JD));

	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rdOnOrBefore+frac, true);

	emit jdChanged(JD);
}

int MayaHaabCalendar::mayanHaabOnOrBefore(QVector<int> haab, int rd)
{
	return modInterval(mayanHaabOrdinal(haab)+mayanHaabEpoch, rd, rd-365);
}

QVector<int> MayaHaabCalendar::mayanHaabFromFixed(int rd)
{
	const int count=StelUtils::imod(rd-mayanHaabEpoch, 365);
	const int day = count % 20;
	const int month=count/20 + 1;
	return {month, day};
}

// get tzolkin name index from Haab date
int MayaHaabCalendar::mayanYearBearerFromFixed(int rd)
{
	QVector<int> haab=mayanHaabFromFixed(rd);
	if (haab.at(0)==19)
			return bogus;
	const int x=mayanHaabOnOrBefore({1, 0}, rd);
	QVector<int> tzolkin=MayaTzolkinCalendar::mayanTzolkinFromFixed(x);
	QVector<int> check({2, 7, 12, 17});
	Q_ASSERT(check.contains(tzolkin.at(1)));
	return tzolkin.at(1);
}

// get RD of a calendar round date
int MayaHaabCalendar::mayanCalendarRoundOnOrBefore(QVector<int>haab, QVector<int>tzolkin, int rd)
{
	const int haabCount=mayanHaabOrdinal(haab)+mayanHaabEpoch;
	const int tzolkinCount=MayaTzolkinCalendar::mayanTzolkinOrdinal(tzolkin)+MayaTzolkinCalendar::mayanTzolkinEpoch;
	const int diff=tzolkinCount-haabCount;
	if (diff % 5 ==0)
		return modInterval(haabCount+365*diff, rd, rd-18980);
	else
		return bogus;
}

