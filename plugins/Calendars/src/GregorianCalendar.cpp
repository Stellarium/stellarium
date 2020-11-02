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
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"


GregorianCalendar::GregorianCalendar(double jd): JulianCalendar (jd)
{
}

// Set a calendar date from the Julian day number
void GregorianCalendar::setJD(double JD)
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

		long ta, jalpha, tb, tc, td, te;

//		if (julian >= JD_GREG_CAL)
//		{
			jalpha = (4*(julian - 1867216) - 1) / 146097;
			ta = julian + 1 + jalpha - jalpha / 4;
//		}
//		else
//		if (julian < 0)
//		{
//			ta = julian + 36525 * (1 - julian / 36525);
//		}
//		else
//		{
//			ta = julian;
//		}

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

	int hour, minute, second;
	StelUtils::getTimeFromJulianDay(JD, &hour, &minute, &second, nullptr);
	parts << hour << minute << second;

	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]-Hour-Minute-Second
void GregorianCalendar::setParts(QVector<int> parts)
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
	long laa, lbb, lcc, lee;

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

	// Don't test switchover date. Just always Gregorian...
	//if (d + 31L*(m + 12L * y) >= IGREG2)
	//{
		lcc = jy/100;
		if (jy < 0 && jy % 100)
		{
			--lcc;
		}
		lee = lcc/4;
		if (lcc < 0 && lcc % 4)
		{
			--lee;
		}
		ljul += 2 - lcc + lee;
	//}
	JD = static_cast<double>(ljul);
	JD += deltaTime;

	emit jdChanged(JD);
}

// returns true for leap years
bool GregorianCalendar::isLeap(int year)
{
	if (year % 100 == 0)
		return (year % 400 == 0);
	else
		return (year % 4 == 0);
}
