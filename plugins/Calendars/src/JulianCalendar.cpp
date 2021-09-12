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
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

JulianCalendar::JulianCalendar(double jd): Calendar(jd)
{
	JulianCalendar::retranslate();
}

QMap<int, QString> JulianCalendar::weekDayNames;
QMap<int, QString> JulianCalendar::monthNames;

void JulianCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	weekDayNames={
		{0, qc_("Sunday"   , "long day name")},
		{1, qc_("Monday"   , "long day name")},
		{2, qc_("Tuesday"  , "long day name")},
		{3, qc_("Wednesday", "long day name")},
		{4, qc_("Thursday" , "long day name")},
		{5, qc_("Friday"   , "long day name")},
		{6, qc_("Saturday" , "long day name")}};
	monthNames={
		{ 1, qc_("January"  , "long month name")},
		{ 2, qc_("February" , "long month name")},
		{ 3, qc_("March"    , "long month name")},
		{ 4, qc_("April"    , "long month name")},
		{ 5, qc_("May"      , "long month name")},
		{ 6, qc_("June"     , "long month name")},
		{ 7, qc_("July"     , "long month name")},
		{ 8, qc_("August"   , "long month name")},
		{ 9, qc_("September", "long month name")},
		{10, qc_("October"  , "long month name")},
		{11, qc_("November" , "long month name")},
		{12, qc_("December" , "long month name")}};
}

// Set a calendar date from the Julian day number
void JulianCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=julianFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, DayName
// Again, in this plugin only, note no year zero, and AD/BC counting.
QStringList JulianCalendar::getDateStrings() const
{
	QStringList list;
	list << QString("%1 %2").arg(abs(parts.at(0))).arg(parts.at(0)>0 ? q_("A.D.") : q_("B.C."));
	list << QString::number(parts.at(1));
	list << QString::number(parts.at(2));
	list << weekday(JD);

	return list;
}

// get a formatted complete string for a date
QString JulianCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TODO: Maybe use QDate with user's localisation here? Weekday has to be taken from our results, though.
	return QString("%1, %2 - %3 (%4) - %5")
			.arg(str.at(3)) // weekday
			.arg(str.at(2)) // day
			.arg(str.at(1)) // month, numerical
			.arg(monthNames.value(parts.at(1))) // month, name
			.arg(str.at(0));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void JulianCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "JulianCalendar::setDate:" << parts;
	this->parts=parts;
	// For the Julian calendar, we really have no year 0 in this plugin.
	Q_ASSERT(parts.at(0) != 0);

	double rd=fixedFromJulian(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// returns true for leap years. Note that for a calendar without year 0, we must do it more complicated.
bool JulianCalendar::isLeap(int year)
{
	return (StelUtils::imod(year, 4) == (year > 0 ? 0 : 3) );
}

// return name of week day
QString JulianCalendar::weekday(double jd)
{
	const int dow = StelUtils::getDayOfWeek(jd+StelApp::getInstance().getCore()->getUTCOffset(jd)/24.);
	return weekDayNames.value(dow);
}

int JulianCalendar::fixedFromJulian(QVector<int> julian)
{
	const int year=julian.at(0);
	const int month=julian.at(1);
	const int day=julian.at(2);
	const int y=(year<0 ? year+1 : year);

	int ret=julianEpoch-1+365*(y-1)+StelUtils::intFloorDiv(y-1, 4)
			+StelUtils::intFloorDiv(367*month-362, 12)+day;
	if (month>2)
		ret+= (isLeap(year) ? -1 : -2);
	return ret;
}

QVector<int> JulianCalendar::julianFromFixed(int rd)
{
	const int approx=StelUtils::intFloorDiv(4*(rd-julianEpoch)+1464, 1461);
	const int year=(approx<=0 ? approx-1 : approx);
	const int priorDays=rd-fixedFromJulian({year, january, 1});
	int correction=0;
	if (rd>=fixedFromJulian({year, march, 1}))
		correction=(isLeap(year) ? 1 : 2);
	const int month=StelUtils::intFloorDiv(12*(priorDays+correction)+373, 367);
	const int day=rd-fixedFromJulian({year, month, 1})+1;
	return {year, month, day};
}
