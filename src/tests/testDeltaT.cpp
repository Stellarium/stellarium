/*
 * Stellarium
 * Copyright (C) 2010 Matthew Gates
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

#include "tests/testDeltaT.hpp"

#include <QDateTime>
#include <QVariantList>
#include <QString>
#include <QDebug>
#include <QtGlobal>

#include "StelUtils.hpp"

QTEST_MAIN(TestDeltaT)

void TestDeltaT::initTestCase()
{
}

void TestDeltaT::testDeltaTByEspenakMeeus()
{
	// test data from http://eclipse.gsfc.nasa.gov/SEcat5/deltat.html#tab1
	QVariantList data;

	data << -500  << 17190 << 430;
	data << -400  << 15530 << 390;
	data << -300  << 14080 << 360;
	data << -200  << 12790 << 330;
	data << -100  << 11640 << 290;
	data << 0     << 10580 << 260;
	data << 100   << 9600  << 240;
	data << 200   << 8640  << 210;
	data << 300   << 7680  << 180;
	data << 400   << 6700  << 160;
	data << 500   << 5710  << 140;
	data << 600   << 4740  << 120;
	data << 700   << 3810  << 100;
	data << 800   << 2960  << 80;
	data << 900   << 2200  << 70;
	data << 1000  << 1570  << 55;
	data << 1100  << 1090  << 40;
	data << 1200  << 740   << 30;
	data << 1300  << 490   << 20;
	data << 1400  << 320   << 20;
	data << 1500  << 200   << 20;
	data << 1600  << 120   << 20;
	data << 1700  << 9     << 5;
	data << 1750  << 13    << 2;
	data << 1800  << 14    << 1;
	data << 1850  << 7     << 1;
	data << 1900  << -3    << 1;
	data << 1950  << 29    << 0.1;
	data << 1955  << 31.1  << 0.1;
	data << 1960  << 33.2  << 0.1;
	data << 1965  << 35.7  << 0.1;
	data << 1970  << 40.2  << 0.1;
	data << 1975  << 45.5  << 0.1;
	data << 1980  << 50.5  << 0.1;
	data << 1985  << 54.3  << 0.1;
	data << 1990  << 56.9  << 0.1;
	data << 1995  << 60.8  << 0.1;
	data << 2000  << 63.8  << 0.1;
	data << 2005  << 64.7  << 0.1;

	while(data.count() >= 3) 
	{
		int year = data.takeFirst().toInt();				
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = data.takeFirst().toDouble();		
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByEspenakMeeus(JD);
		double actualError = qAbs(qAbs(expectedResult) - qAbs(result));
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(actualError <= acceptableError, QString("date=%2 year=%3 result=%4 expected=%5 error=%6 acceptable=%7")
							.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
							.arg(year)
							.arg(result)
							.arg(expectedResult)
							.arg(actualError)
							.arg(acceptableError)
							.toUtf8());
	}
}

void TestDeltaT::testDeltaTByChaprontMeeus()
{
	// test data from Astronomical Algorithms, 2ed., p. 79.
	QVariantList data;
	data << 1620 << 121.0;
	data << 1630 <<  82.0;
	data << 1640 <<  60.0;
	data << 1650 <<  46.0;
	data << 1660 <<  35.0;
	data << 1670 <<  24.0;
	data << 1680 <<  14.0;
	data << 1690 <<   8.0;
	data << 1700 <<   7.0;
	data << 1710 <<   9.0;
	data << 1720 <<  10.0;
	data << 1730 <<  10.0;
	data << 1740 <<  11.0;
	data << 1750 <<  12.0;
	data << 1760 <<  14.0;
	data << 1770 <<  15.0;
	data << 1780 <<  16.0;
	data << 1790 <<  16.0;
	data << 1800 <<  13.1;
	data << 1810 <<  12.0;
	data << 1820 <<  11.6;
	data << 1830 <<   7.1;
	data << 1840 <<   5.4;
	data << 1850 <<   6.8;
	data << 1860 <<   7.7;
	data << 1870 <<   1.4;
	data << 1880 <<  -5.5;
	data << 1890 <<  -6.0;
	data << 1900 <<  -2.8;
	data << 1910 <<  10.4;
	data << 1920 <<  21.1;
	data << 1930 <<  24.4;
	data << 1940 <<  24.3;
	data << 1950 <<  29.1;
	data << 1960 <<  33.1;
	data << 1970 <<  40.2;
	data << 1980 <<  50.5;
	data << 1990 <<  56.9;
	data << 1998 <<  63.0;

	// accuracy 0.9 seconds for years range 1800-1997
	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 0.9; // 0.9 seconds
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByChaprontMeeus(JD);
		double actualError = qAbs(qAbs(expectedResult) - qAbs(result));
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(actualError <= acceptableError, QString("date=%2 year=%3 result=%4 expected=%5 error=%6 acceptable=%7")
							.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
							.arg(year)
							.arg(result)
							.arg(expectedResult)
							.arg(actualError)
							.arg(acceptableError)
							.toUtf8());
	}
}

