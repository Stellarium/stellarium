/*
 * Copyright (C) 2026 Georg Zotti
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
#include "ByzantineCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

ByzantineCalendar::ByzantineCalendar(double jd): JulianCalendar(jd)
{
	ByzantineCalendar::retranslate();
}

QMap<int, QString> ByzantineCalendar::weekDayNames;
QMap<int, QString> ByzantineCalendar::monthNames;

void ByzantineCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	weekDayNames={
		{0, qc_("Day of the Lord"   , "long day name")},
		{1, qc_("Second day"        , "long day name")},
		{2, qc_("Third day"         , "long day name")},
		{3, qc_("Fourth day"        , "long day name")},
		{4, qc_("Fifth day"         , "long day name")},
		{5, qc_("Day of Preparation", "long day name")},
		{6, qc_("Sabbaton"          , "long day name")}};
	monthNames={
		{ 1, qc_("September", "long month name")},
		{ 2, qc_("October"  , "long month name")},
		{ 3, qc_("November" , "long month name")},
		{ 4, qc_("December" , "long month name")},
		{ 5, qc_("January"  , "long month name")},
		{ 6, qc_("February" , "long month name")},
		{ 7, qc_("March"    , "long month name")},
		{ 8, qc_("April"    , "long month name")},
		{ 9, qc_("May"      , "long month name")},
		{10, qc_("June"     , "long month name")},
		{11, qc_("July"     , "long month name")},
		{12, qc_("August"   , "long month name")}
	};
}

// Set a calendar date from the Julian day number
void ByzantineCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=byzantineFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, DayName
// In this plugin we use year zero and negative for the (unthinkable) years before creation.
QStringList ByzantineCalendar::getDateStrings() const
{
	QStringList list;
	list << QString("%1 %2").arg(abs(parts.at(0))).arg(qc_("A.M.", "Annus Mundi"));
	list << QString::number(parts.at(1));
	list << QString::number(parts.at(2));
	list << weekday(JD);

	return list;
}

// get a formatted complete string for a date
QString ByzantineCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TODO: Maybe use QDate with user's localisation here? Weekday has to be taken from our results, though.
	return QString("%1, %2 - %3 (%4) - %5").arg(
			str.at(3), // weekday
			str.at(2), // day
			str.at(1), // month, numerical
			monthNames.value(parts.at(1)), // month, name
			str.at(0));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void ByzantineCalendar::setDate(const QVector<int> &parts)
{
	//qDebug() << "ByzantineCalendar::setDate:" << parts;
	this->parts=parts;

	double rd=fixedFromByzantine(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// returns true for leap years. Coincidentally, same rule as Julian (but without the zero exception).
bool ByzantineCalendar::isLeap(int year)
{
	return (StelUtils::imod(year, 4) == 0 );
}

// return name of week day
QString ByzantineCalendar::weekday(double jd)
{
	const int dow = StelUtils::getDayOfWeek(jd+StelApp::getInstance().getCore()->getUTCOffset(jd)/24.);
	return weekDayNames.value(dow);
}

int ByzantineCalendar::fixedFromByzantine(const QVector<int> &byzantine)
{
	int year =byzantine.value(0);
	int month=byzantine.value(1);
	const int day  =byzantine.value(2);
	//int y=(year<5510 ? year+1 : year);
	int y=year-5509;

	// Convert this to Julian calendar date

	month += 8;
	if (month > 12)
	{
		month -= 12;
		y++;
	}

	// From here, it's just Julian
	int ret=julianEpoch-1+365*(y-1)+StelUtils::intFloorDiv(y-1, 4)
			+StelUtils::intFloorDiv(367*month-362, 12)+day;
	if (month>2)
		ret+= (isLeap(y) ? -1 : -2);
	return ret;
}

QVector<int> ByzantineCalendar::byzantineFromFixed(int rd)
{
	const int approx=StelUtils::intFloorDiv(4*(rd-julianEpoch)+1464, 1461);
	int year=(approx<=0 ? approx-1 : approx);
	const int priorDays=rd-fixedFromJulian({year, january, 1});
	int correction=0;
	if (rd>=fixedFromJulian({year, march, 1}))
		correction=(isLeap(year) ? 1 : 2);
	int month=StelUtils::intFloorDiv(12*(priorDays+correction)+373, 367);
	int day=rd-fixedFromJulian({year, month, 1})+1;

	//From here, convert this to Byzantine calendar date
	const int yCorr=(year < 0 ? 1 : 0); // enable year 0
	year += (month<9 ? 5508: 5509) + yCorr;
	month -= 8;
	if (month<1)
		month +=12;

	// In some cases this now has day==0. Adjust to last day of previous month.
	if (day==0)
	{
		month--;
		if (month<=0)
		{
			month=12;
			year--;
		}
		Q_ASSERT(month<13);
		static const int mLength[] = {30, 31, 30, 31, 31, 28, 31, 30, 31, 30, 31, 31};
		day = mLength[month-1];
		if (isLeap(year) && month==6)
			day=29;
	}

	return {year, month, day};
}
