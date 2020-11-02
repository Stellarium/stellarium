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

JulianCalendar::JulianCalendar(double jd): Calendar(jd)
{
	retranslate();
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
// This was taken from StelUtils but dissected to fit the particular calendar.
void JulianCalendar::setJD(double JD)
{
	this->JD=JD;

	//void getDateFromJulianDay(const double jd, int *yy, int *mm, int *dd)
	//{
		/*
		 * This algorithm is taken from
		 * "Numerical Recipes in C, 2nd Ed." (1992), pp. 14-15
		 * and converted to integer math.
		 * The electronic version of the book is freely available
		 * at http://www.nr.com/ , under "Obsolete Versions - Older
		 * book and code versions".
		 */

		//static const long JD_GREG_CAL = 2299161;
		static const int JB_MAX_WITHOUT_OVERFLOW = 107374182;
		long julian = static_cast<long>(floor(JD + 0.5));

		long ta, /*jalpha,*/ tb, tc, td, te;

//		if (julian >= JD_GREG_CAL)
//		{
//			jalpha = (4*(julian - 1867216) - 1) / 146097;
//			ta = julian + 1 + jalpha - jalpha / 4;
//		}
//		else
		if (julian < 0)
		{
			ta = julian + 36525 * (1 - julian / 36525);
		}
		else
		{
			ta = julian;
		}

		tb = ta + 1524;
		if (tb <= JB_MAX_WITHOUT_OVERFLOW)
		{
			tc = (tb*20 - 2442) / 7305;
		}
		else
		{
			// FIXME: Modifying this cast to modern style breaks a test.
			tc = static_cast<long>((static_cast<unsigned long long>(tb)*20 - 2442) / 7305); //- WTF???
			//tc = (long)(((unsigned long long)tb*20 - 2442) / 7305);
		}
		td = 365 * tc + tc/4;
		te = ((tb - td) * 10000)/306001;

		int dd = tb - td - (306001 * te) / 10000;

		int mm = te - 1;
		if (mm > 12)
		{
			mm -= 12;
		}
		int yy = tc - 4715;
		if (mm > 2)
		{
			--(yy);
		}
		if (julian < 0)
		{
			yy -= 100 * (1 - julian / 36525);
		}
	//}

	parts.clear();
	parts << yy << mm << dd;

	int hour, minute, second, millis;
	StelUtils::getTimeFromJulianDay(JD, &hour, &minute, &second, &millis);
	parts << hour << minute << second+millis/1000.;

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, DayName, Hour, Minute, Second
QStringList JulianCalendar::getPartStrings()
{
	QStringList list;
	list << QString::number(parts.at(0));
	list << QString::number(parts.at(1));
	list << QString::number(parts.at(2));
	list << weekday(JD);
	list << QString::number(parts.at(3));
	list << QString::number(parts.at(4));
	list << QString::number(parts.at(5));

	return list;
}


// get a formatted complete string for a date
QString JulianCalendar::getFormattedDateString()
{
	QStringList str=getPartStrings();
	return QString("%1, %2 - %3 (%4) - %5 %6:%7:%8")
			.arg(str.at(3)) // weekday
			.arg(str.at(0)) // year
			.arg(str.at(1)) // month, numerical
			.arg(monthNames.value(parts.at(1))) // month, name
			.arg(str.at(2)) // day
			.arg(str.at(4)) // hour
			.arg(str.at(5)) // minute
			.arg(str.at(6)); // second
}



// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]-Hour-Minute-Second
// Algorithm from "Numerical Recipes in C, 2nd Ed." (1992), pp. 11-12
// This has been taken from StelUtils, but is always Julian Calendar
void JulianCalendar::setParts(QVector<int> parts)
{
	this->parts=parts;

	const int y=parts.at(0);
	const int m=parts.at(1);
	const int d=parts.at(2);
	const int h=parts.at(3);
	const int min=parts.at(4);
	const int s=parts.at(5);

	//static const long IGREG2 = 15+31L*(10+12L*1582);
	const double deltaTime = (h / 24.0) + (min / (24.0*60.0)) + (static_cast<double>(s) / (24.0 * 60.0 * 60.0)) - 0.5;

	// Algorithm taken from "Numerical Recipes in C, 2nd Ed." (1992), pp. 11-12
	long ljul;
	long jy, jm;
	long laa, lbb; //, lcc, lee;

	jy = y;
	if (m > 2)
	{
		jm = m + 1;
	}
	else
	{
		--jy;
		jm = m + 13;
	}

	laa = 1461 * jy / 4;
	if (jy < 0 && jy % 4)
	{
		--laa;
	}
	lbb = 306001 * jm / 10000;
	ljul = laa + lbb + d + 1720995L;

	//if (d + 31L*(m + 12L * y) >= IGREG2)
	//{
	//	lcc = jy/100;
	//	if (jy < 0 && jy % 100)
	//	{
	//		--lcc;
	//	}
	//	lee = lcc/4;
	//	if (lcc < 0 && lcc % 4)
	//	{
	//		--lee;
	//	}
	//	ljul += 2 - lcc + lee;
	//}
	JD = static_cast<double>(ljul);
	JD += deltaTime;

	emit jdChanged(JD);
}

// returns true for leap years
bool JulianCalendar::isLeap(int year)
{
	return (year % 4 == 0);
}

// return name of week day
QString JulianCalendar::weekday(double jd)
{
	const int dow = StelUtils::getDayOfWeek(jd);
	return weekDayNames.value(dow);
}
