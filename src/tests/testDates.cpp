/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#include "tests/testDates.hpp"

#include <QString>
#include <QDateTime>
#include <QDate>
#include <QMap>
#include <QList>
#include <QtGlobal>
#include <QLocale>
#include <QDebug>

#include "StelUtils.hpp"

#define IGREG 2299161

QTEST_GUILESS_MAIN(TestDates)

void TestDates::dateRoundTrip()
{
	QMap<double, QString> map;
	map[0.0] = "-4712-01-01T12:00:00";
	map[-1.0] = "-4713-12-31T12:00:00";
	map[2454466.0] = "2007-12-31T12:00:00";
	map[1721058.0] = "0000-01-01T12:00:00";
	map[2500000.0] = "2132-08-31T12:00:00";
	map[366.0] = "-4711-01-01T12:00:00";
	map[2454534] = "2008-03-08T12:00:00";
	map[2299161.0] = "1582-10-15T12:00:00";
	map[2454466.5] = "2008-01-01T00:00:00";
	map[1720692.0] = "-0002-12-31T12:00:00";
	map[1720693.0] = "-0001-01-01T12:00:00";
	map[2400000.0] = "1858-11-16T12:00:00";
	map[2110516.00000] = "1066-04-12T12:00:00";
	map[1918395.00000] = "0540-04-12T12:00:00";
	map[1794575.00000] = "0201-04-12T12:00:00";
	map[1757319.00000] = "0099-04-12T12:00:00";
	map[1721424.0] = "0001-01-01T12:00:00";
	map[1721789.0] = "0002-01-01T12:00:00";
	map[1721423.0] = "0000-12-31T12:00:00";
	map[1000000.0] = "-1975-11-07T12:00:00";
	map[-31.0] = "-4713-12-01T12:00:00";
	map[-61.0] = "-4713-11-01T12:00:00";
	map[-92.0] = "-4713-10-01T12:00:00";
	map[-122.0] = "-4713-09-01T12:00:00";
	map[-153.0] = "-4713-08-01T12:00:00";
	map[-184.0] = "-4713-07-01T12:00:00";
	map[-214.0] = "-4713-06-01T12:00:00";
	map[-245.0] = "-4713-05-01T12:00:00";
	map[-275.0] = "-4713-04-01T12:00:00";
	map[-306.0] = "-4713-03-01T12:00:00";
	map[-334.0] = "-4713-02-01T12:00:00"; // 28 days
	map[-365.0] = "-4713-01-01T12:00:00";
	map[-699.0] = "-4714-02-01T12:00:00"; // 28 days
	map[-1064.0] = "-4715-02-01T12:00:00"; // 28 days
	map[-1430.0] = "-4716-02-01T12:00:00"; // 29 days
	map[-1795.0] = "-4717-02-01T12:00:00"; // 28 days
	map[-39388.5] = "-4820-02-29T00:00:00"; // 29 days
	map[-1930711.0] ="-9998-01-01T12:00:00";
	map[-1930712.0] ="-9999-12-31T12:00:00";

	bool ok;	
	for (QMap<double, QString>::ConstIterator i=map.constBegin();i!=map.constEnd();++i)
	{
		QCOMPARE(StelUtils::julianDayToISO8601String(i.key()), i.value());
		double tmp = StelUtils::getJulianDayFromISO8601String(i.value(), &ok);
		QVERIFY(ok);
		if (i.key()!=0.0)
			ok = qFuzzyCompare(i.key(), tmp);
		else
			ok = qFuzzyCompare(i.key()+1.0, tmp+1.0);
		QVERIFY(ok);
	}
}


void TestDates::formatting()
{
	// test formatting of StelUtils::localeDateString, the fall-back if QDateTime cannot do it.
	QLocale::setDefault(QLocale::German);
	QVERIFY2(QString::compare(StelUtils::localeDateString(2008, 03, 10, 0), QString("10.03.08")) == 0,
			 qPrintable("german for 2008-03-10 wrong: " + (StelUtils::localeDateString(2008, 03, 10, 0))));
	QVERIFY2(QString::compare(StelUtils::localeDateString(8, 03, 10, 0), QString("10.03.08")) == 0,
			 qPrintable("german for 8-03-10 wrong: " + (StelUtils::localeDateString(8, 03, 10, 0))));
	QVERIFY2(QString::compare(StelUtils::localeDateString(-8, 03, 10, 0), QString("10.03.-8")) == 0,
			 qPrintable("german for -8-03-10 wrong: " + (StelUtils::localeDateString(-8, 03, 10, 0))));
	QVERIFY2(QString::compare(StelUtils::localeDateString(-1118, 03, 10, 0), QString("10.03.-18")) == 0,
			 qPrintable("german for -1118-03-10 wrong: " + (StelUtils::localeDateString(-1118, 03, 10, 0))));
	QVERIFY2(QString::compare(StelUtils::localeDateString(-5118, 03, 10, 0), QString("10.03.-18")) == 0,
			 qPrintable("german for -5118-03-10 wrong: " + (StelUtils::localeDateString(-5118, 03, 10, 0))));

	QVERIFY2(-18 == (-5118 % 100), qPrintable("modulus arithmetic works diff: " + QString("%1").arg(-5118 % 100)));
}

void TestDates::testRolloverAndValidity()
{
	QVERIFY2(31==StelUtils::numberOfDaysInMonthInYear(1, 2008), "1a");
	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 2008), "1b");
	QVERIFY2(28==StelUtils::numberOfDaysInMonthInYear(2, 2007), "1c");

	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 2000), "1d");
	QVERIFY2(28==StelUtils::numberOfDaysInMonthInYear(2, 1900), "1e");

	QVERIFY2(28==StelUtils::numberOfDaysInMonthInYear(2, 1001), "1f");
	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 1000), "1g");
	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 1200), "1h");

	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 0), "1i");
	QVERIFY2(28==StelUtils::numberOfDaysInMonthInYear(2, 1), "1j");
	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, -4), "1k");

	int dy, dm, dd, dh, dmin,ds;

	QVERIFY2(StelUtils::changeDateTimeForRollover(2008, 1, 32, 12, 0, 0, &dy, &dm, &dd, &dh, &dmin, &ds), "1l");
	QVERIFY(2008==dy);
	QVERIFY(2==dm);
	QVERIFY(1==dd);
	QVERIFY(12==dh);
	QVERIFY(0==dmin);
	QVERIFY(0==ds);

	QVERIFY2(StelUtils::changeDateTimeForRollover(-1, 12, 32, 12, 0, 0, &dy, &dm, &dd, &dh, &dmin, &ds), "1m");
	QVERIFY(0==dy);
	QVERIFY(1==dm);
	QVERIFY(1==dd);
	QVERIFY(12==dh);
	QVERIFY(0==dmin);
	QVERIFY(0==ds);

	QVERIFY2(StelUtils::changeDateTimeForRollover(-1, 11, 45, 12, 0, 0, &dy, &dm, &dd, &dh, &dmin, &ds), "1n");
	QVERIFY(-1==dy);
	QVERIFY(12==dm);
	QVERIFY(15==dd);
	QVERIFY(12==dh);
	QVERIFY(0==dmin);
	QVERIFY(0==ds);
}

/*
 * Original algorithm from "Numerical Recipes in c, 2nd Ed.", pp. 14-15
 */
static void caldat(long julian, int *mm, int *id, int *iyyy)
{
	long ja, jalpha, jb, jc, jd, je;
	if (julian >= IGREG) {
		jalpha = (long)(((double)(julian - 1867216) - 0.25) / 36524.25);
		ja = julian + 1 + jalpha -(long)(0.25 * jalpha);
	} else if (julian < 0) {
		ja = julian + 36525 * (1 - julian / 36525);
	} else {
		ja = julian;
	}
	jb = ja + 1524;
	jc = (long)(6680.0 + ((double)(jb - 2439870) - 122.1) / 365.25);
	jd = (long)(365 * jc + (0.25 * jc));
	je = (long)((jb - jd) / 30.6001);
	*id = jb - jd -(long)(30.6001 * je);

	*mm = je - 1;
	if (*mm > 12) {
		*mm -= 12;
	}
	*iyyy = jc - 4715;
	if (*mm > 2) {
		--(*iyyy);
	}
	if (julian < 0) {
		*iyyy -= 100 * (1 - julian / 36525);
	}
}

static void oldGetDateFromJulianDay(double jd, int *year, int *month, int *day)
{
	int y, m, d;

	// put us in the right calendar day for the time of day.
	double fraction = jd - floor(jd);
	if (fraction >= .5)
	{
		jd += 1.0;
	}

	if (jd >= 2299161)
	{
		// Gregorian calendar starting from October 15, 1582
		// This algorithm is from Henry F. Fliegel and Thomas C. Van Flandern
		qulonglong ell, n, i, j;
		ell = qulonglong(floor(jd)) + 68569;
		n = (4 * ell) / 146097;
		ell = ell - (146097 * n + 3) / 4;
		i = (4000 * (ell + 1)) / 1461001;
		ell = ell - (1461 * i) / 4 + 31;
		j = (80 * ell) / 2447;
		d = ell - (2447 * j) / 80;
		ell = j / 11;
		m = j + 2 - (12 * ell);
		y = 100 * (n - 49) + i + ell;
	}
	else
	{
		// Julian calendar until October 4, 1582
		// Algorithm from Frequently Asked Questions about Calendars by Claus Toendering
		int julianDay = static_cast<int>(floor(jd));
		julianDay += 32082;
		int dd = (4 * julianDay + 3) / 1461;
		int ee = julianDay - (1461 * dd) / 4;
		int mm = ((5 * ee) + 2) / 153;
		d = ee - (153 * mm + 2) / 5 + 1;
		m = mm + 3 - 12 * (mm / 10);
		y = dd - 4800 + (mm / 10);
	}
	*year = y;
	*month = m;
	*day = d;
}

static bool oldGetJDFromDate(double* newjd, int y, int m, int d, int h, int min, int s)
{
	double deltaTime = (h / 24.0) + (min / (24.0*60.0)) + (s / (24.0 * 60.0 * 60.0)) - 0.5;
	QDate test((y <= 0 ? y-1 : y), m, d);
	// if QDate will oblige, do so.
	if ( test.isValid() )
	{
		double qdjd = (double)test.toJulianDay();
		qdjd += deltaTime;
		*newjd = qdjd;
		return true;
	} else
	{
		double jd = (double)((1461 * (y + 4800 + (m - 14) / 12)) / 4 + (367 * (m - 2 - 12 * ((m - 14) / 12))) / 12 - (3 * ((y + 4900 + (m - 14) / 12) / 100)) / 4 + d - 32075) - 38;
		jd += deltaTime;
		*newjd = jd;
		return true;
	}
	return false;
}

void TestDates::testJulianDaysRange(int jd_a, int jd_b)
{
	int jd;
	double djd;
	int yy, mm, dd, yy0, mm0, dd0, dim;
	int jdFirst, jdLast;

	if (jd_a > jd_b)
	{
		jdFirst = jd_a;
		jdLast = jd_b;
	} else {
		jdFirst = jd_b;
		jdLast = jd_a;
	}

	qDebug() << "testing range " << jdLast << "..." << jdFirst;

	jd = jdFirst;

	StelUtils::getDateFromJulianDay(jd, &yy0, &mm0, &dd0);

	jd--;

	for ( ; jd >= jdLast; jd--)
	{
		StelUtils::getDateFromJulianDay(jd, &yy, &mm, &dd);
		StelUtils::getJDFromDate(&djd, yy, mm, dd, 12, 0, 0);

		QVERIFY2((long)djd == jd,
		         qPrintable("wrong jd from day " + QString("%1/%2/%3").arg(yy).arg(mm).arg(dd) +
		                    ": expected " + QString("%1").arg(jd) + ", found " + QString("%1").arg((long)djd)
		         )
		);

		QVERIFY2(mm >= 1 && mm <= 12,
		         qPrintable("wrong month " + QString("%1").arg(mm) + " for jd " + QString("%1").arg(jd)));
		dim = StelUtils::numberOfDaysInMonthInYear(mm, yy);

		QVERIFY2(dd >= 1 && dd <= dim,
		         qPrintable("bad date " + QString("%1/%2/%3").arg(yy).arg(mm).arg(dd) +
		                    " for jd " + QString("%1").arg(jd) + "; previous date was " +
		                    QString("%1/%2/%3").arg(yy0).arg(mm0).arg(dd0) + " for jd " + QString("%1").arg(jd + 1)
		         )
		);
		if (dd0 > 1) {
			// day changed, month and year are the same
			QVERIFY2(dd == (dd0 - 1),
			         qPrintable("wrong day: expected " + QString("%1").arg(dd0 - 1) +
			                    ", found " + QString("%1").arg(dd) + ", jd " + QString("%1").arg(jd)
			         )
			);
			QCOMPARE(mm, mm0);
			QCOMPARE(yy, yy0);
		} else {
			// day changed to the last day of previous month;
			QVERIFY(dd > dd0);
			if (mm0 > 1) {
				// month changed; year is the same
				QVERIFY2((mm0 - 1) == mm,
				         qPrintable("bad month change, month not adjacent; date is " +
				                    QString("%1/%2/%3").arg(yy).arg(mm).arg(dd) + " for jd " + QString("%1").arg(jd) +
				                    "; previous date was " +QString("%1/%2/%3").arg(yy0).arg(mm0).arg(dd0) +
				                    " for jd " + QString("%1").arg(jd + 1)
				         )
				);
				QVERIFY2(yy0 == yy,
				         qPrintable("bad month change, year changed; date is " +
				                    QString("%1/%2/%3").arg(yy).arg(mm).arg(dd) + " for jd " + QString("%1").arg(jd) +
				                    "; previous date was " +QString("%1/%2/%3").arg(yy0).arg(mm0).arg(dd0) +
				                    " for jd " + QString("%1").arg(jd + 1)
				         )
				);
			} else {
				// month changed to december of the previous year
				QCOMPARE(mm, 12);
				QCOMPARE(yy, yy0 - 1);
			}
		}
		dd0 = dd;
		mm0 = mm;
		yy0 = yy;
	}
}

void TestDates::testJulianDays()
{
	testJulianDaysRange( 400000000,  400001000);
	testJulianDaysRange( 200000000,  200001000);
	testJulianDaysRange(   2299200,    2299161);	
	testJulianDaysRange(   2299160,    2299000);	
	testJulianDaysRange(   2211000,    2210000);
	testJulianDaysRange(   1721789,    1721788);
	testJulianDaysRange(   1721424,    1721423);
	testJulianDaysRange(   1501000,    1500000);
	testJulianDaysRange(   1001000,    1000000);
	testJulianDaysRange(    501000,     500000);
	testJulianDaysRange(      1000,      -1000);
	testJulianDaysRange(    -32000,     -33000);
	testJulianDaysRange(   -320000,    -319000);
	testJulianDaysRange(  -1905086,   -1905084);
	testJulianDaysRange(  -1001000,   -1000000);
	testJulianDaysRange(  -2001000,   -2000000);
	testJulianDaysRange(  -2301000,   -2300000);
	testJulianDaysRange( -99001000,  -99000000);
	testJulianDaysRange(-200001000, -200000000);
	testJulianDaysRange(-400001000, -400000000);	
}

void TestDates::testLeapYears()
{
	QVariantList data;

	data << 1500 << true;
	data << 1600 << true;
	data << 2000 << true;
	data << 2100 << false;
	data << 2200 << false;
	data << 2300 << false;
	data << 2016 << true;
	data << 2017 << false;
	data << 2018 << false;
	data << 2019 << false;
	data << 2020 << true;
	data << 1852 << true;
	data << 1851 << false;

	while (data.count()>=2)
	{
		int year	= data.takeFirst().toInt();
		bool expected	= data.takeFirst().toBool();
		bool status	= StelUtils::isLeapYear(year);

		QVERIFY2(status==expected, qPrintable(QString("Year %1 is %2 (expected %3)")
							   .arg(year)
							   .arg(status ? "leap" : "not leap")
							   .arg(expected ? "leap" : "not leap")));
	}
}

void TestDates::testNumberOfDaysInMonthInYear()
{
	QVariantList data;

	data << 2019 << 1		<< 31;
	data << 2019 << 2		<< 28;
	data << 2019 << 3		<< 31;
	data << 2019 << 4		<< 30;
	data << 2019 << 5		<< 31;
	data << 2019 << 6		<< 30;
	data << 2019 << 7		<< 31;
	data << 2019 << 8		<< 31;
	data << 2019 << 9		<< 30;
	data << 2019 << 10	<< 31;
	data << 2019 << 11	<< 30;
	data << 2019 << 12	<< 31;
	data << 2020 << 1		<< 31;
	data << 2020 << 2		<< 29;
	data << 2020 << 3		<< 31;
	data << 2020 << 4		<< 30;
	data << 2020 << 5		<< 31;
	data << 2020 << 6		<< 30;
	data << 2020 << 7		<< 31;
	data << 2020 << 8		<< 31;
	data << 2020 << 9		<< 30;
	data << 2020 << 10	<< 31;
	data << 2020 << 11	<< 30;
	data << 2020 << 12	<< 31;
	data << 2020 << 0		<< 31;
	data << 2020 << 13	<< 31;
	data << 1852 << 1		<< 31;
	data << 1852 << 2		<< 29;
	data << 1851 << 2		<< 28;

	while (data.count()>=3)
	{
		int year	= data.takeFirst().toInt();
		int month	= data.takeFirst().toInt();
		int expected	= data.takeFirst().toInt();
		int days	= StelUtils::numberOfDaysInMonthInYear(month, year);

		QVERIFY2(days==expected, qPrintable(QString("Month %1 in year %2 has %3 days (expected %4 days)")
						    .arg(month)
						    .arg(year)
						    .arg(days)
						    .arg(expected)));
	}
}

void TestDates::testFixedFromGregorian()
{
	QVariantList data;
	//            year	  month	day	        days
	data <<       1 <<   1 <<   1 <<           1;
	data <<       8 <<   8 << 27 <<     2796;
	data <<   632 <<   6 << 19 << 230638;
	data << 1792 <<   9 << 22 << 654415;
	data << 1858 << 11 << 17 << 678576;
	data << 1970 <<   1 <<   1 << 719163;
	data << 1945 << 11 << 12 << 710347;

	while (data.count()>=4)
	{
		int year	= data.takeFirst().toInt();
		int month	= data.takeFirst().toInt();
		int day	= data.takeFirst().toInt();
		int expected	= data.takeFirst().toInt();
		int days	= StelUtils::getFixedFromGregorian(year, month, day);

		QVERIFY2(days==expected, qPrintable(QString("Date %1-%2-%3 has %4 days from R.D. (expected %5 days)")
						    .arg(year)
						    .arg(month)
						    .arg(day)
						    .arg(days)
						    .arg(expected)));
	}
}

void TestDates::testWeekdays()
{
	QVariantList data;
	data << 2019 <<  9 << 25 << 3; // Wednesday
	data << 1582 << 10 << 15 << 5; // Friday (1st day in Greg. Calendar)
	data << 1582 << 10 <<  4 << 4; // Thursday (Day before that, given in Jul. Calendar)
	data << 1961 <<  4 << 12 << 3; // Wednesday (Wostok-1)
	data << 1981 <<  4 << 12 << 0; // Sunday (STS-1)
	data << 1972 <<  5 << 14 << 0; // Sunday
	while (data.count() >=4)
	{
		int year     = data.takeFirst().toInt();
		int month    = data.takeFirst().toInt();
		int day	     = data.takeFirst().toInt();
		int expected = data.takeFirst().toInt();
		int wd=StelUtils::getDayOfWeek(year, month, day);

		QVERIFY2(wd==expected, qPrintable(QString("Date %1-%2-%3 wrongly has weekday index %4 (expected %5)")
						  .arg(year)
						  .arg(month)
						  .arg(day)
						  .arg(day)
						  .arg(expected)));
	}
}

void TestDates::testDatesFromJD()
{
	QVariantList data;
	data << 2500000.0 <<  2132 <<  8 << 31;
	data << 2454466.5 <<  2008 <<  1 <<  1;
	data << 2454466.0 <<  2007 << 12 << 31;
	data << 2451545.0 <<  2000 <<  1 <<  1;
	data << 2442413.5 <<  1975 <<  1 <<  1;
	data << 2433282.5 <<  1950 <<  1 <<  1;
	data << 2415020.5 <<  1900 <<  1 <<  1;
	data << 2405889.5 <<  1875 <<  1 <<  1;
	data << 2400000.0 <<  1858 << 11 << 16;
	data << 2396758.5 <<  1850 <<  1 <<  1;
	data << 2385800.5 <<  1820 <<  1 <<  1;
	data << 2378496.5 <<  1800 <<  1 <<  1;
	data << 2110516.0 <<  1066 <<  4 << 12;
	data << 1720693.0 <<    -1 <<  1 <<  1;
	data <<     366.0 << -4711 <<  1 <<  1;
	data <<      -1.0 << -4713 << 12 << 31;

	while (data.count() >=4)
	{
		int yout, mout, dout;
		double JD = data.takeFirst().toDouble();
		int year  = data.takeFirst().toInt();
		int month = data.takeFirst().toInt();
		int day	  = data.takeFirst().toInt();
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);

		QVERIFY2((year==yout && month==mout && day == dout), qPrintable(QString("JD %1 = %2-%3-%4 (expected %5-%6-%7)")
						  .arg(QString::number(JD, 'f', 1))
						  .arg(yout).arg(mout).arg(dout)
						  .arg(year).arg(month).arg(day)));
	}
}

#define TJ1 (2450000)

void TestDates::benchmarkOldGetDateFromJulianDay()
{
	long jd = TJ1;
	int mm, dd, yy;

	QBENCHMARK {
		oldGetDateFromJulianDay(jd++, &yy, &mm, &dd);
	}
}

void TestDates::benchmarkGetDateFromJulianDayFloatingPoint()
{
	long jd = TJ1;
	int mm, dd, yy;

	QBENCHMARK {
		caldat(jd++, &mm, &dd, &yy);
	}
}

void TestDates::benchmarkGetDateFromJulianDay()
{
	long jd = TJ1;
	int mm, dd, yy;

	QBENCHMARK {
		StelUtils::getDateFromJulianDay(jd++, &yy, &mm, &dd);
	}
}

static QList<QDate*> *newDatesTestList(void)
{
	QList<QDate*> *testDates = new QList<QDate*>();
	QDate *qd;
	double djd;
	long jd, jd1, jd2;
	int yy, mm, dd;

	StelUtils::getJDFromDate(&djd, 2000, 1, 1, 12, 0, 0);
	jd1 = (long)djd;
	StelUtils::getJDFromDate(&djd, 2012, 1, 1, 12, 0, 0);
	jd2 = (long)djd;

	for (jd = jd1; jd < jd2; jd++)
	{
		StelUtils::getDateFromJulianDay(jd, &yy, &mm, &dd);
		qd = new QDate(yy, mm, dd);
		testDates->append(qd);
	}
	return testDates;
}

void TestDates::benchmarkOldGetJDFromDate()
{
//		oldGetJDFromDate(&djd, qd->year(), qd->month(), qd->day(), 12, 0, 0);
	QList<QDate*> *testDates;
	int ii, nn;
	const QDate *qd;
	double djd;

	testDates = newDatesTestList();

	nn = testDates->size();
	ii = 0;
	QBENCHMARK {
		qd = testDates->at(ii);
		oldGetJDFromDate(&djd, qd->year(), qd->month(), qd->day(), 12, 0, 0);
		if (++ii >= nn)
		{
			ii = 0;
		}
	}

	while (!testDates->isEmpty())
	{
		delete testDates->takeFirst();
	}

	delete testDates;
}

void TestDates::benchmarkGetJDFromDate()
{
	QList<QDate*> *testDates;
	int ii, nn;
	const QDate *qd;
	double djd;

	testDates = newDatesTestList();

	nn = testDates->size();
	ii = 0;
	QBENCHMARK {
		qd = testDates->at(ii);
		StelUtils::getJDFromDate(&djd, qd->year(), qd->month(), qd->day(), 12, 0, 0);
		if (++ii >= nn)
		{
			ii = 0;
		}
	}

	while (!testDates->isEmpty())
	{
		delete testDates->takeFirst();
	}

	delete testDates;
}
