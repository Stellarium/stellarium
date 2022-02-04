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
#include "RomanCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

RomanCalendar::RomanCalendar(double jd): JulianCalendar(jd)
{
	RomanCalendar::retranslate();
}

QMap<int, QString> RomanCalendar::monthGen;

void RomanCalendar::retranslate()
{
	// fill the name lists with translated month names
	monthGen={
		{ 1, qc_("Ianuarii"   , "Roman month name (genitive)")},
		{ 2, qc_("Februarii"  , "Roman month name (genitive)")},
		{ 3, qc_("Martii"     , "Roman month name (genitive)")},
		{ 4, qc_("Aprilis"    , "Roman month name (genitive)")},
		{ 5, qc_("Maii"       , "Roman month name (genitive)")},
		{ 6, qc_("Iunii"      , "Roman month name (genitive)")},
		{ 7, qc_("Iulii"      , "Roman month name (genitive)")},
		{ 8, qc_("Augusti"    , "Roman month name (genitive)")},
		{ 9, qc_("Septembris" , "Roman month name (genitive)")},
		{10, qc_("Octobris"   , "Roman month name (genitive)")},
		{11, qc_("Novembris"  , "Roman month name (genitive)")},
		{12, qc_("Decembris"  , "Roman month name (genitive)")}};
}

// Set a calendar date from the Julian day number
void RomanCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=romanFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// AUCYear, Month, MonthName(genitive), event, DayName
QStringList RomanCalendar::getDateStrings() const
{
	QStringList events={qc_("Kalendae", "Roman calendar term"),
			    qc_("Nones",    "Roman calendar term"),
			    qc_("Ides",     "Roman calendar term")};
	QStringList eventsShort={qc_("Kal.", "Roman calendar term"),
				 qc_("Non.", "Roman calendar term"),
				 qc_("Id." , "Roman calendar term")};

	QStringList list;
	list << QString::number(aucYearFromJulian(parts.at(0))); // 0:AUC year
	list << QString::number(parts.at(1)); // 1:Month (numeric)
	list << monthGen.value(parts.at(1));  // 2:Month (genitive form)
	// 3:event:
	if (parts.at(3)==1)
		list << events.at(parts.at(2)-1);
	else
		list << eventsShort.at(parts.at(2)-1);

	// 4:day
	if (parts.at(3)==1)
		list << "";
	else if (parts.at(3)==2)
		list << qc_("pridie", "Roman calendar term");
	else {
		const QString bisTerm=QString(" %1").arg(parts.at(4)? qc_("bis", "Roman calendar term") : "");
		list << QString("%1%2 %3").arg(qc_("ante diem", "Roman calendar term"), bisTerm, romanNumber(parts.at(3)));
	}

	return list;
}

// get a formatted complete string for a date
QString RomanCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1 %2 %3 %4 %5").arg(
			str.at(4),
			str.at(3),
			str.at(2),
			str.at(0),
			qc_("A.U.C.", "ab urbe condita"));// year AUC
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// JulianYear-Month[1...12]-event-count-leap
// Time is not changed!
void RomanCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromRoman(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// returns 13 or 15, the day of "idae"
int RomanCalendar::idesOfMonth(int month)
{
	static const QVector<int> cand={JulianCalendar::march, JulianCalendar::may, JulianCalendar::july, JulianCalendar::october};
	if (cand.contains(month))
		return 15;
	else
		return 13;
}

// returns the day of "nones"
int RomanCalendar::nonesOfMonth(int month)
{
	return idesOfMonth(month)-8;
}

// Convert between AUC and Julian year numbers
int RomanCalendar::julianYearFromAUC(int aucYear)
{
	if ((1<=aucYear) && (aucYear<=-yearRomeFounded))
		return aucYear + yearRomeFounded - 1;
	else
		return aucYear + yearRomeFounded;
}

int RomanCalendar::fixedFromRoman(QVector<int> roman)
{
	const int year =roman.value(0);
	const int month=roman.value(1);
	const events event=static_cast<events>(roman.value(2));
	const int count=roman.value(3);
	const int leap =roman.value(4);

	int rd = 0; // Suppress warning
	switch (event)
	{
	case kalends:
		rd=fixedFromJulian({year, month, 1});
		break;
	case nones:
		rd=fixedFromJulian({year, month, nonesOfMonth(month)});
		break;
	case ides:
		rd=fixedFromJulian({year, month, idesOfMonth(month)});
		break;
	}
	rd-=count;
	if (!(JulianCalendar::isLeap(year) && (month==JulianCalendar::march) && (event==kalends) && (16>=count) && (count>=6)))
		rd+=1;
	rd+=leap;
	return rd;
}

QVector<int> RomanCalendar::romanFromFixed(int rd)
{
	const QVector<int>jdate=julianFromFixed(rd);
	const int year=jdate.at(0);
	const int month=jdate.at(1);
	const int day=jdate.at(2);
	const int monthP=StelUtils::amod(month+1, 12);
	const int yearP=(monthP!=1 ? year : (year!=-1 ? year+1 : 1 ));
	const int kalends1=fixedFromRoman({yearP, monthP, kalends, 1, false});

	if (day==1)
		return {year, month, kalends, 1, false};
	else if (day<=nonesOfMonth(month))
		return {year, month, nones, nonesOfMonth(month)-day+1, false};
	else if (day<=idesOfMonth(month))
		return {year, month, ides, idesOfMonth(month)-day+1, false};
	else if (month!=february || !isLeap(year))
		return {yearP, monthP, kalends, kalends1-rd+1, false};
	else if (day<25)
		return {year, march, kalends, 30-day, false};
	else return {year, march, kalends, 31-day, day==25};
}

// Convert between AUC and Julian year numbers
int RomanCalendar::aucYearFromJulian(int julianYear)
{
	if ((yearRomeFounded<=julianYear) && (julianYear<=-1))
		return julianYear - yearRomeFounded + 1;
	else
		return julianYear - yearRomeFounded;
}

// return a Roman number (within int range)
QString RomanCalendar::romanNumber(const int num)
{
	if (num==0)
		return ("N");
	QString res;
	if (num < 0 )
		res.append("-");
	int n=abs(num);
	if ( n>=5000 )
	{
		int mils=n/1000;
		res.append(romanNumber(mils));
		res.append("&middot;M ");
		n-=mils*1000;
	}
	while ( n>=1000 )
	{
		res.append("M");
		n -= 1000;
	}
	if ( n>=900 )
	{
		res.append("CM");
		n -= 900;
	}
	if ( n>=500 )
	{
		res.append("D");
		n -= 500;
	}
	if ( n>=400 )
	{
		res.append("CD");
		n -= 400;
	}
	while ( n>=100 )
	{
		res.append("C");
		n -= 100;
	}
	if ( n>=90 )
	{
		res.append("XC");
		n -= 90;
	}
	if ( n>=50 )
	{
		res.append("L");
		n -= 50;
	}
	if ( n>=40 )
	{
		res.append("XL");
		n -= 40;
	}
	while (n >= 10)
	{
		res.append("X");
		n -= 10;
	}
	if ( n==9 )
	{
		res.append("IX");
		n -= 9;
	}
	if ( n>=5 )
	{
		res.append("V");
		n -= 5;
	}
	if ( n==4 )
	{
		res.append("IV");
		n -= 4;
	}
	while ( n>0 )
	{
		res.append("I");
		n -= 1;
	}
	return res;
}
