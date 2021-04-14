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

QTEST_GUILESS_MAIN(TestDeltaT)

void TestDeltaT::initTestCase()
{
	// source: http://astro.ukho.gov.uk/nao/lvm/

	//                            Year               ΔT              ε (seconds)
	// ΔT (s) : −2000 to −800 (convert from hours)
	genericData << -2000.0  <<  1123200 <<  25920;
	genericData << -1900.0  <<  1062720 <<  25920;
	genericData << -1800.0  <<  1010880 <<  25920;
	genericData << -1700.0  <<    950400 <<  25920;
	genericData << -1600.0  <<    907200 <<  17280;
	genericData << -1500.0  <<    855360 <<  17280;
	genericData << -1400.0  <<    803520 <<  17280;
	genericData << -1300.0  <<    760320 <<  17280;
	genericData << -1200.0  <<    708480 <<  17280;
	genericData << -1100.0  <<    665280 <<  17280;
	genericData << -1000.0  <<    622080 <<  17280;
	genericData <<   -900.0  <<    570240 <<    8640;
	genericData <<   -800.0  <<    527040 <<    8640;
	// ΔT (s) : −720 to +1600
	genericData <<   -720.0  << 20480.00 << 180.00;
	genericData <<   -700.0  << 20120.00 << 170.00;
	genericData <<   -600.0  << 18400.00 << 160.00;
	genericData <<   -500.0  << 16820.00 << 150.00;
	genericData <<   -400.0  << 15370.00 << 130.00;
	genericData <<   -300.0  << 14030.00 << 120.00;
	genericData <<   -200.0  << 12800.00 << 110.00;
	genericData <<   -100.0  << 11640.00 << 100.00;
	genericData <<        0.0  << 10550.00 <<   90.00;
	genericData <<    100.0  <<   9510.00 <<   80.00;
	genericData <<    200.0  <<   8500.00 <<   70.00;
	genericData <<    300.0  <<   7510.00 <<   60.00;
	genericData <<    400.0  <<   6520.00 <<   50.00;
	genericData <<    500.0  <<   5520.00 <<   40.00;
	genericData <<    600.0  <<   4550.00 <<   40.00;
	genericData <<    700.0  <<   3630.00 <<   30.00;
	genericData <<    800.0  <<   2800.00 <<   25.00;
	genericData <<    900.0  <<   2090.00 <<   20.00;
	genericData <<  1000.0  <<   1540.00 <<   15.00;
	genericData <<  1100.0  <<   1170.00 <<   15.00;
	genericData <<  1200.0  <<     900.00 <<   15.00;
	genericData <<  1300.0  <<     660.00 <<   15.00;
	genericData <<  1400.0  <<     400.00 <<   15.00;
	genericData <<  1500.0  <<     200.00 <<   15.00;
	genericData <<  1600.0  <<     110.00 <<   15.00;
	// ΔT (s) : +1600 to +1800
	genericData <<  1600.0  <<     113.00 <<   15.00;
	genericData <<  1610.0  <<     100.00 <<   15.00;
	genericData <<  1620.0  <<       86.00 <<   20.00;
	genericData <<  1630.0  <<       72.00 <<   20.00;
	genericData <<  1640.0  <<       58.00 <<   20.00;
	genericData <<  1650.0  <<       45.00 <<   20.00;
	genericData <<  1660.0  <<       35.00 <<   15.00;
	genericData <<  1670.0  <<       26.00 <<   10.00;
	genericData <<  1680.0  <<       21.00 <<     5.00;
	genericData <<  1690.0  <<       17.00 <<     5.00;
	genericData <<  1700.0  <<       14.00 <<     5.00;
	genericData <<  1710.0  <<       12.00 <<     5.00;
	genericData <<  1720.0  <<       12.00 <<     5.00;
	genericData <<  1730.0  <<       13.00 <<     2.00;
	genericData <<  1740.0  <<       15.00 <<     2.00;
	genericData <<  1750.0  <<       17.00 <<     2.00;
	genericData <<  1760.0  <<       19.00 <<     2.00;
	genericData <<  1770.0  <<       21.00 <<     1.00;
	genericData <<  1780.0  <<       21.00 <<     1.00;
	genericData <<  1790.0  <<       21.00 <<     1.00;
	genericData <<  1800.0  <<       18.00 <<     0.50;
	genericData <<  1810.0  <<       16.00 <<     0.50;
	// ΔT (s) : +1950 to +2016
	genericData <<  1950.0  <<       28.93 <<     0.05;
	genericData <<  1951.0  <<       29.32 <<     0.05;
	genericData <<  1952.0  <<       29.70 <<     0.05;
	genericData <<  1953.0  <<       30.00 <<     0.05;
	genericData <<  1954.0  <<       30.20 <<     0.05;
	genericData <<  1955.0  <<       30.41 <<     0.05;
	genericData <<  1956.0  <<       30.76 <<     0.05;
	genericData <<  1957.0  <<       31.34 <<     0.05;
	genericData <<  1958.0  <<       32.03 <<     0.05;
	genericData <<  1959.0  <<       32.65 <<     0.05;
	genericData <<  1960.0  <<       33.07 <<     0.05;
	genericData <<  1961.0  <<       33.36 <<     0.05;
	genericData <<  1962.0  <<       33.62 <<     0.05;
	genericData <<  1963.0  <<       33.96 <<     0.05;
	genericData <<  1964.0  <<       34.44 <<     0.05;
	genericData <<  1965.0  <<       35.09 <<     0.05;
	genericData <<  1966.0  <<       35.95 <<     0.05;
	genericData <<  1967.0  <<       36.93 <<     0.05;
	genericData <<  1968.0  <<       37.96 <<     0.05;
	genericData <<  1969.0  <<       38.95 <<     0.05;
	genericData <<  1970.0  <<       39.93 <<     0.05;
	genericData <<  1971.0  <<       40.95 <<     0.05;
	genericData <<  1972.0  <<       42.04 <<     0.05;
	genericData <<  1973.0  <<       43.15 <<     0.05;
	genericData <<  1974.0  <<       44.24 <<     0.05;
	genericData <<  1975.0  <<       45.28 <<     0.05;
	genericData <<  1976.0  <<       46.28 <<     0.05;
	genericData <<  1977.0  <<       47.29 <<     0.05;
	genericData <<  1978.0  <<       48.33 <<     0.05;
	genericData <<  1979.0  <<       49.37 <<     0.05;
	genericData <<  1980.0  <<       50.36 <<     0.05;
	genericData <<  1981.0  <<       51.28 <<     0.05;
	genericData <<  1982.0  <<       52.13 <<     0.05;
	genericData <<  1983.0  <<       52.94 <<     0.05;
	genericData <<  1984.0  <<       53.70 <<     0.05;
	genericData <<  1985.0  <<       54.39 <<     0.05;
	genericData <<  1986.0  <<       54.98 <<     0.05;
	genericData <<  1987.0  <<       55.46 <<     0.05;
	genericData <<  1988.0  <<       55.89 <<     0.05;
	genericData <<  1989.0  <<       56.37 <<     0.05;
	genericData <<  1990.0  <<       56.99 <<     0.05;
	genericData <<  1991.0  <<       57.70 <<     0.05;
	genericData <<  1992.0  <<       58.45 <<     0.05;
	genericData <<  1993.0  <<       59.19 <<     0.05;
	genericData <<  1994.0  <<       59.92 <<     0.05;
	genericData <<  1995.0  <<       60.68 <<     0.05;
	genericData <<  1996.0  <<       61.46 <<     0.05;
	genericData <<  1997.0  <<       62.23 <<     0.05;
	genericData <<  1998.0  <<       62.90 <<     0.05;
	genericData <<  1999.0  <<       63.42 <<     0.05;
	genericData <<  2000.0  <<       63.81 <<     0.05;
	genericData <<  2001.0  <<       64.08 <<     0.05;
	genericData <<  2002.0  <<       64.27 <<     0.05;
	genericData <<  2003.0  <<       64.41 <<     0.05;
	genericData <<  2004.0  <<       64.55 <<     0.05;
	genericData <<  2005.0  <<       64.73 <<     0.05;
	genericData <<  2006.0  <<       64.94 <<     0.05;
	genericData <<  2007.0  <<       65.19 <<     0.05;
	genericData <<  2008.0  <<       65.48 <<     0.05;
	genericData <<  2009.0  <<       65.78 <<     0.05;
	genericData <<  2010.0  <<       66.06 <<     0.05;
	genericData <<  2011.0  <<       66.33 <<     0.05;
	genericData <<  2012.0  <<       66.60 <<     0.05;
	genericData <<  2013.0  <<       66.92 <<     0.05;
	genericData <<  2014.0  <<       67.30 <<     0.05;
	genericData <<  2015.0  <<       67.69 <<     0.05;
	genericData <<  2016.0  <<       68.04 <<     0.05;
	// ΔT (seconds) : +2017 to +2500
	genericData <<  2017.0  <<       68.60 <<     0.10;
	genericData <<  2018.0  <<       69.00 <<     0.10;
	genericData <<  2019.0  <<       69.20 <<     0.10;
	genericData <<  2020.0  <<       69.50 <<     0.10;
	genericData <<  2030.0  <<       70.00 <<     2.00;
	genericData <<  2040.0  <<       72.00 <<     4.00;
	genericData <<  2050.0  <<       75.00 <<     6.00;
	genericData <<  2100.0  <<       93.00 <<   10.00;
	genericData <<  2200.0  <<     163.00 <<   20.00;
	genericData <<  2300.0  <<     297.00 <<   30.00;
	genericData <<  2400.0  <<     521.00 <<   50.00;
	genericData <<  2500.0  <<     855.00 << 100.00;
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
	data << 1870  << -0.1  << 1; // https://eclipse.gsfc.nasa.gov/SEsearch/SEsearchmap.php?Ecl=18701222
	data << 1900  << -3    << 1;
	data << 1910  << 10.8  << 1; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot1901/SE1910May09T.GIF
	data << 1925  << 23.6  << 0.5; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot1901/SE1925Jan24T.GIF
	data << 1945  << 26.8  << 0.5; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot1901/SE1945Jan14A.GIF
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
	data << 2010  << 66.6  << 1; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2001/SE2010Jan15A.GIF
	data << 2015  << 67.6  << 2; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2001/SE2015Mar20T.GIF
	data << 2020  << 77.2  << 7; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2001/SE2020Jun21A.GIF
	data << 2030  << 87.9  << 12; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2001/SE2030Jun01A.GIF
	data << 2050  << 111.6 << 20; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2001/SE2050May20H.GIF
	data << 2060  << 124.6 << 15; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2051/SE2060Apr30T.GIF
	data << 2070  << 138.5 << 15; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2051/SE2070Apr11T.GIF
	data << 2090  << 170.3 << 15; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2051/SE2090Sep23T.GIF
	data << 2100  << 187.3 << 20; // https://eclipse.gsfc.nasa.gov/SEplot/SEplot2051/SE2100Sep04T.GIF

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
	data << 1984 <<  53.8; // *
	data << 1988 <<  55.8; // *
	data << 1990 <<  56.9;
	data << 1992 <<  58.3; // *
	data << 1996 <<  61.6; // *
	data << 1998 <<  63.0;
	
	// test data marked as * taken Mathematical Astronomical Morsels III, p. 8. [ISBN 0-943396-81-6]

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

void TestDeltaT::testDeltaTByChaprontMeeusWideDates()
{
	// test data from Mathematical Astronomical Morsels III, p. 8. [ISBN 0-943396-81-6]
	QVariantList data;
	data <<    0 << 165; 
	data <<  100 << 144; 
	data <<  200 << 125;
	data <<  300 << 108;
	data <<  400 <<  92;
	data <<  500 <<  77;
	data <<  600 <<  64;
	data <<  700 <<  53;
	data <<  800 <<  43;
	data <<  900 <<  34;
	data << 1000 <<  27;
	data << 1100 <<  21;
	data << 1200 <<  15;
	data << 1300 <<  10;
	data << 1400 <<   7;
	data << 1500 <<   4;
	data << 1600 <<   2;
	data << 1700 <<   0;
	data << 1800 <<   0;
	data << 1980 <<   1;
	data << 2075 <<   3;
	data << 2200 <<   7;
	data << 2300 <<  11;
	data << 2400 <<  15;
	data << 2500 <<  21;
	data << 2600 <<  27;
	data << 2700 <<  34;
	data << 2800 <<  42;
	data << 2900 <<  51;
	data << 3000 <<  61;

	while(data.count() >= 2)
	{
		// 30 seconds accuracy
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByChaprontMeeus(JD)/60.0;
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(qRound(result) <= expectedResult, QString("date=%2 year=%3 result=%4 expected=%5")
							.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
							.arg(year)
							.arg(result)
							.arg(expectedResult)
							.toUtf8());
	}
}

void TestDeltaT::testDeltaTByMorrisonStephenson1982WideDates()
{
	// test data from Mathematical Astronomical Morsels, p. 8. [ISBN 0-943396-51-4]
	QVariantList data;
	data <<    0 << 177;
	data <<  100 << 158;
	data <<  200 << 140;
	data <<  300 << 123;
	data <<  400 << 107;
	data <<  500 <<  93;
	data <<  600 <<  79;
	data <<  700 <<  66;
	data <<  800 <<  55;
	data <<  900 <<  45;
	data << 1000 <<  35;
	data << 1100 <<  27;
	data << 1200 <<  20;
	data << 1300 <<  14;
	data << 1400 <<   9;
	data << 1500 <<   5;
	data << 1600 <<   2;
	data << 1700 <<   0;
	data << 1800 <<   0;
	data << 1980 <<   1;
	data << 2075 <<   4;
	data << 2200 <<   8;
	data << 2300 <<  13;
	data << 2400 <<  19;
	data << 2500 <<  26;
	data << 2600 <<  34;
	data << 2700 <<  43;
	data << 2800 <<  53;
	data << 2900 <<  64;
	data << 3000 <<  76;

	while(data.count() >= 2)
	{
		// 30 seconds accuracy
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByMorrisonStephenson1982(JD)/60.0;
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(qRound(result) <= expectedResult, QString("date=%2 year=%3 result=%4 expected=%5")
							.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
							.arg(year)
							.arg(result)
							.arg(expectedResult)
							.toUtf8());
	}
}

void TestDeltaT::testDeltaTByStephensonMorrison1984WideDates()
{
	// test data from Mathematical Astronomical Morsels, p. 8. [ISBN 0-943396-51-4]
	QVariantList data;
	data <<    0 << 177;
	data <<  100 << 158;
	data <<  200 << 140;
	data <<  300 << 123;
	data <<  400 << 107;
	data <<  500 <<  93;
	data <<  600 <<  79;
	data <<  700 <<  66;
	data <<  800 <<  55;
	data <<  900 <<  45;
	data << 1000 <<  35;
	data << 1100 <<  27;
	data << 1200 <<  20;
	data << 1300 <<  14;
	data << 1400 <<   9;
	data << 1500 <<   5;
	data << 1600 <<   2;
	/*
	 * WTF?! FIXME: Should be Stephenson & Morrison (1984) give zero value outside of range?
	data << 1700 <<   0;
	data << 1800 <<   0;
	data << 1980 <<   1;
	data << 2075 <<   4;
	data << 2200 <<   8;
	data << 2300 <<  13;
	data << 2400 <<  19;
	data << 2500 <<  26;
	data << 2600 <<  34;
	data << 2700 <<  43;
	data << 2800 <<  53;
	data << 2900 <<  64;
	data << 3000 <<  76;
	*/

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByStephensonMorrison1984(JD)/60.0;
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(result <= expectedResult, QString("date=%2 year=%3 result=%4 expected=%5")
							.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
							.arg(year)
							.arg(result)
							.arg(expectedResult)
							.toUtf8());
	}
}

void TestDeltaT::testDeltaTByStephensonMorrison1995WideDates()
{
	// test data taken from (a spline):
	// Long-Term Fluctuations in the Earth's Rotation: 700 BC to AD 1990.
	// Stephenson, F. R.; Morrison, L. V.
	// Philosophical Transactions: Physical Sciences and Engineering, Volume 351, Issue 1695, pp. 165-202
	// 1995RSPTA.351..165S [http://adsabs.harvard.edu/abs/1995RSPTA.351..165S]
	QVariantList data;

	data << -500 << 16800;
	data << -450 << 16000;
	data << -400 << 15300;
	data << -350 << 14600;
	data << -300 << 14000;
	data << -250 << 13400;
	data << -200 << 12800;
	data << -150 << 12200;
	data << -100 << 11600;
	data <<  -50 << 11100;
	data <<    0 << 10600;
	data <<   50 << 10100;
	data <<  100 <<  9600;
	data <<  150 <<  9100;
	data <<  200 <<  8600;
	data <<  250 <<  8200;
	data <<  300 <<  7700;
	data <<  350 <<  7200;
	data <<  400 <<  6700;
	data <<  450 <<  6200;
	data <<  500 <<  5700;
	data <<  550 <<  5200;
	data <<  600 <<  4700;
	data <<  650 <<  4300;
	/*
	// FIXME: WTF? http://www.staff.science.uu.nl/~gent0113/deltat/deltat_old.htm
	data <<  700 <<  3800;
	data <<  750 <<  3400;
	data <<  800 <<  3000;
	data <<  850 <<  2600;
	data <<  900 <<  2200;
	data <<  950 <<  1900;
	data << 1000 <<  1600;
	data << 1050 <<  1350;
	data << 1100 <<  1100;
	data << 1150 <<   900;
	data << 1200 <<   750;
	data << 1250 <<   600;
	data << 1300 <<   460;
	data << 1350 <<   360;
	data << 1400 <<   280; // 300
	data << 1450 <<   200; // 230
	data << 1500 <<   150; // 180
	data << 1550 <<   110; // 140
	data << 1600 <<    80;  // 110
	*/

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByStephensonMorrison1995(JD);
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(result <= expectedResult, QString("date=%2 year=%3 result=%4 expected=%5")
							.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
							.arg(year)
							.arg(result)
							.arg(expectedResult)
							.toUtf8());
	}
}

void TestDeltaT::testDeltaTByStephenson1997WideDates()
{
	// test data taken from http://www.staff.science.uu.nl/~gent0113/deltat/deltat_old.htm

	QVariantList data;

	/*
	// FIXME: WTF?
	data << -500 << 16800;
	data << -450 << 16000;
	data << -400 << 15300;
	data << -350 << 14600;
	data << -300 << 14000;
	data << -250 << 13400;
	data << -200 << 12800;
	data << -150 << 12200;
	data << -100 << 11600;
	data <<  -50 << 11100;
	*/
	data <<    0 << 10600;
	data <<   50 << 10100;
	data <<  100 <<  9600;
	data <<  150 <<  9100;
	data <<  200 <<  8600;
	data <<  250 <<  8200;
	data <<  300 <<  7700;
	data <<  350 <<  7200;
	data <<  400 <<  6700;
	data <<  450 <<  6200;
	data <<  500 <<  5700;
	data <<  550 <<  5200;
	data <<  600 <<  4700;
	data <<  650 <<  4300;
	data <<  700 <<  3800;
	data <<  750 <<  3400;
	/*
	// FIXME: WTF?
	data <<  800 <<  3000;
	data <<  850 <<  2600;
	data <<  900 <<  2200;
	data <<  950 <<  1900;
	data << 1000 <<  1600;
	data << 1050 <<  1350;
	data << 1100 <<  1100;
	data << 1150 <<   900;
	data << 1200 <<   750;
	data << 1250 <<   600;
	data << 1300 <<   460;
	data << 1350 <<   360;
	data << 1400 <<   280; // 300
	data << 1450 <<   200; // 230
	data << 1500 <<   150; // 180
	data << 1550 <<   110; // 140
	data << 1600 <<    80;  // 110
	*/

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByStephenson1997(JD);
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(result <= expectedResult, QString("date=%2 year=%3 result=%4 expected=%5")
							.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
							.arg(year)
							.arg(result)
							.arg(expectedResult)
							.toUtf8());
	}
}

void TestDeltaT::testDeltaTByStephensonMorrison2004WideDates()
{
	// test data from
	// * Morrison, L. V.; Stephenson, F. R., "Historical values of the Earth's clock error ΔT and the calculation of eclipses",
	//    Journal for the History of Astronomy, 35(2004), 327-336 - http://adsabs.harvard.edu/abs/2004JHA....35..327M
	// * addendum in ibid., 36 (2005), 339 - http://adsabs.harvard.edu/abs/2005JHA....36..339M
	QVariantList data;
	data << -1000 << 25400 << 640;
	data <<  -900 << 23700 << 590;
	data <<  -800 << 22000 << 550;
	data <<  -700 << 20400 << 500;
	data <<  -600 << 18800 << 460;
	data <<  -500 << 17190 << 430;
	data <<  -400 << 15530 << 390;
	data <<  -300 << 14080 << 360;
	data <<  -200 << 12790 << 330;
	data <<  -100 << 11640 << 290;
	data <<     0 << 10580 << 260;
	data <<   100 <<  9600 << 240;
	/*
	data <<   200 <<  8640 << 210;	
	data <<   300 <<  7680 << 180;
	data <<   400 <<  6700 << 160;
	data <<   500 <<  5710 << 140;
	data <<   600 <<  4740 << 120;
	data <<   700 <<  3810 << 100;
	data <<   800 <<  2960 <<  80;
	data <<   900 <<  2200 <<  70;
	data <<  1000 <<  1570 <<  55;
	data <<  1100 <<  1090 <<  40;
	data <<  1200 <<   740 <<  30;
	data <<  1300 <<   490 <<  20;
	data <<  1400 <<   320 <<  20;
	data <<  1500 <<   200 <<  20;
	data <<  1600 <<   120 <<   9;
	data <<  1700 <<     9 <<   5;
	data <<  1710 <<    10 <<   3;
	data <<  1720 <<    11 <<   3;
	data <<  1730 <<    11 <<   3;
	data <<  1740 <<    12 <<   2;
	data <<  1750 <<    13 <<   2;
	data <<  1760 <<    15 <<   2;
	data <<  1770 <<    16 <<   2;
	data <<  1780 <<    17 <<   1;
	data <<  1790 <<    17 <<   1;
	data <<  1800 <<    14 <<   1;
	data <<  1810 <<    13 <<   1;
	data <<  1820 <<    12 <<   1;
	data <<  1830 <<     8 <<   1; // *  - <1
	data <<  1840 <<     6 <<   1; // ** - undefined error
	data <<  1850 <<     7 <<   1; // **
	data <<  1860 <<     8 <<   1; // **
	data <<  1870 <<     2 <<   1; // **
	data <<  1880 <<    -5 <<   1; // **
	data <<  1890 <<    -6 <<   1; // **
	data <<  1900 <<    -3 <<   1; // **
	data <<  1910 <<    10 <<   1; // **
	data <<  1920 <<    21 <<   1; // **
	data <<  1930 <<    24 <<   1; // **
	data <<  1940 <<    24 <<   1; // **
	data <<  1950 <<    29 <<   1; // **
	data <<  1960 <<    33 <<   1; // **
	data <<  1970 <<    40 <<   1; // **
	data <<  1980 <<    51 <<   1; // **
	data <<  1990 <<    57 <<   1; // **
	data <<  2000 <<    65 <<   1; // **
	*/

	while(data.count() >= 3)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByMorrisonStephenson2004(JD);
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

void TestDeltaT::testDeltaTByStephensonMorrisonHohenkerk2016GenericDates()
{
	// Valid range: 720 BC to AD 2015

	double year, expectedResult, acceptableError, JD;
	int yout, mout, dout;
	QVariantList d = genericData;
	while(d.count() >= 3)
	{
		year = d.takeFirst().toDouble();
		expectedResult = d.takeFirst().toDouble();
		acceptableError = d.takeFirst().toDouble();

		if (year<0)  // https://github.com/Stellarium/stellarium/wiki/FAQ#There_is_no_year_0_or_BC_dates_are_a_year_out
			year += 1;

		// Unit tests failures ranges: 300-600, 900-1610, 1780, 1800-1810
		// TODO: Check algorithm
		if ((year>=-720. && year<300.) || (year>600. && year<900.) || (year>=1620. && year<=1770.) || (year>=1820. && year<=2015.))
		{
			StelUtils::getJDFromDate(&JD, static_cast<int>(year), 1, 1, 0, 0, 0);
			double result = StelUtils::getDeltaTByStephensonMorrisonHohenkerk2016(JD);
			double actualError = qAbs(qAbs(expectedResult) - qAbs(result));
			StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
			QVERIFY2(actualError <= acceptableError, QString("[%8] date=%2 year=%3 result=%4 expected=%5 error=%6 acceptable=%7")
								.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
								.arg(year)
								.arg(result)
								.arg(expectedResult)
								.arg(actualError)
								.arg(acceptableError)
								.arg("valid range dates")
								.toUtf8());
		}
	}

	// Check dates prior 720 BC
	/*
	d.clear();
	d = genericData;
	while(d.count() >= 3)
	{
		year = d.takeFirst().toDouble();
		expectedResult = d.takeFirst().toDouble();
		acceptableError = d.takeFirst().toDouble();

		if (year<0)  // https://github.com/Stellarium/stellarium/wiki/FAQ#There_is_no_year_0_or_BC_dates_are_a_year_out
			year += 1;

		// Since year 300 the unit tests failures
		if (year<=-720)
		{
			StelUtils::getJDFromDate(&JD, static_cast<int>(year), 1, 1, 0, 0, 0);
			double result = StelUtils::getDeltaTByStephensonMorrisonHohenkerk2016(JD);
			double actualError = qAbs(qAbs(expectedResult) - qAbs(result));
			StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
			QVERIFY2(actualError <= acceptableError, QString("[%8] date=%2 year=%3 result=%4 expected=%5 error=%6 acceptable=%7")
								.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
								.arg(year)
								.arg(QString::number(result, 'f', 2))
								.arg(QString::number(expectedResult, 'f', 2))
								.arg(QString::number(actualError, 'f', 2))
								.arg(QString::number(acceptableError, 'f', 2))
								.arg("prior 720 BC")
								.toUtf8());
		}
	}

	// Check dates after AD 2015
	d.clear();
	d = genericData;
	while(d.count() >= 3)
	{
		year = d.takeFirst().toDouble();
		expectedResult = d.takeFirst().toDouble();
		acceptableError = d.takeFirst().toDouble();

		if (year>=2015)
		{
			StelUtils::getJDFromDate(&JD, static_cast<int>(year), 1, 1, 0, 0, 0);
			double result = StelUtils::getDeltaTByStephensonMorrisonHohenkerk2016(JD);
			double actualError = qAbs(qAbs(expectedResult) - qAbs(result));
			StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
			QVERIFY2(actualError <= acceptableError, QString("[%8] date=%2 year=%3 result=%4 expected=%5 error=%6 acceptable=%7")
								.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
								.arg(year)
								.arg(QString::number(result, 'f', 2))
								.arg(QString::number(expectedResult, 'f', 2))
								.arg(QString::number(actualError, 'f', 2))
								.arg(QString::number(acceptableError, 'f', 2))
								.arg("after AD 2015")
								.toUtf8());
		}
	}
	*/
}

void TestDeltaT::testDeltaTByMeeusSimons()
{
	// test data from Meeus, Jean, "The Effect of Delta T on Astronomical Calculations",
	// The Journal of the British Astronomical Association, 108 (1998), 154-156
	// http://adsabs.harvard.edu/abs/1998JBAA..108..154M

	QVariantList data;
	data << 1619 <<  0.00; // zero outside the valid range
	data << 1974 << 44.49;
	data << 1975 << 45.48;
	data << 1976 << 46.46;
	data << 1977 << 47.52;
	data << 1978 << 48.53;
	data << 1979 << 49.59;
	data << 1980 << 50.54;
	data << 1981 << 51.38;
	data << 1982 << 52.17;
	data << 1983 << 52.96;
	data << 1984 << 53.79;
	data << 1985 << 54.34;
	data << 1986 << 54.87;
	data << 1987 << 55.32;
	data << 1988 << 55.82;
	data << 1989 << 56.30;
	data << 1990 << 56.86;
	data << 1991 << 57.57;
	data << 1992 << 58.31;
	data << 1993 << 59.12;
	data << 1994 << 59.99;
	data << 1995 << 60.79;
	data << 1996 << 61.63;
	data << 1997 << 62.30;
	data << 1999 << 64.00;
	data << 2001 <<  0.00; // zero outside the valid range

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByMeeusSimons(JD);
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(qAbs(result-expectedResult)<=1, QString("date=%2 year=%3 result=%4 expected=%5")
							.arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
							.arg(year)
							.arg(result)
							.arg(expectedResult)
							.toUtf8());
	}
}

void TestDeltaT::testDeltaTByKhalidSultanaZaidiWideDates()
{
	// test data and max. error taken from:
	// Delta T: Polynomial Approximation of Time Period 1620-2013
	// M. Khalid, Mariam Sultana and Faheem Zaidi
	// Journal of Astrophysics, Vol. 2014, Article ID 480964
	// https://doi.org/10.1155/2014/480964

	QVariantList data;
	//      year        deltaT   max.err.
	data << 1620 << 124.201597 << 0.5709;
	data << 1672 <<   23.455938 << 0.5709;
	data << 1673 <<   23.518597 << 0.5989;
	data << 1729 <<   10.794455 << 0.5989;
	data << 1730 <<   10.964421 << 0.5953;
	data << 1797 <<   15.096315 << 0.5953;
	data << 1798 <<   14.433142 << 0.4643;
	data << 1843 <<     6.564327 << 0.4643;
	data << 1844 <<     6.668453 << 0.5894;
	data << 1877 <<    -4.648388 << 0.5894;
	data << 1878 <<    -5.058000 << 0.5410;
	data << 1904 <<     3.181019 << 0.5410;
	data << 1905 <<     3.559383 << 0.5495;
	data << 1945 <<   27.258421 << 0.5495;
	data << 1946 <<   27.234794 << 0.4279;
	data << 1989 <<   55.872143 << 0.4279;
	data << 1990 <<   56.659321 << 0.2477;
	data << 2013 <<   67.135703 << 0.2477;

	while(data.count() >= 3)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double maxError = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByKhalidSultanaZaidi(JD);
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(qAbs(result-expectedResult)<=maxError, QString("date=%2 year=%3 result=%4 expected=%5 exp. error=%6")
			 .arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
			 .arg(year)
			 .arg(QString::number(result, 'f', 5))
			 .arg(QString::number(expectedResult, 'f', 5))
			 .arg(QString::number(maxError, 'f', 4))
			 .toUtf8());
	}
}

void TestDeltaT::testDeltaTByMontenbruckPfleger()
{
	// test data taken from book  "Astronomy on the Personal Computer" by O. Montenbruck & T. Pfleger (4th ed., 2000), p. 42

	QVariantList data;
	data << 1820 <<  0.00; // zero outside the valid range
	data << 1900 << -2.72;
	data << 1905 <<  3.86;
	data << 1910 << 10.46;
	data << 1915 << 17.20;
	data << 1920 << 21.16;
	data << 1925 << 23.62;
	data << 1930 << 24.02;
	data << 1935 << 23.93;
	data << 1940 << 24.33;
	data << 1945 << 26.77;
	data << 1950 << 29.15;
	data << 1955 << 31.07;
	data << 1960 << 33.15;
	data << 1965 << 35.73;
	data << 1970 << 40.18;
	data << 1975 << 45.48;
	data << 1980 << 50.54;
	data << 1985 << 54.34;
	data << 1990 << 56.86;
	data << 1995 << 60.82;
	//data << 2000 << 63.83; // NOTE: ???
	data << 2010 <<  0.00; // zero outside the valid range

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByMontenbruckPfleger(JD);
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		QVERIFY2(qAbs(result-expectedResult)<=1.0, QString("date=%2 year=%3 result=%4 expected=%5")
			 .arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
			 .arg(year)
			 .arg(QString::number(result, 'f', 5))
			 .arg(QString::number(expectedResult, 'f', 5))
			 .toUtf8());
	}
}

void TestDeltaT::testDeltaTByReingoldDershowitzWideDates()
{
	// the test data was calculated for polynome from 4th ed. of Calendrical Calculations

	QVariantList data;
	//      year   frac. of day
	data << -600 <<  0.216672;
	data << -500 <<  0.199117;
	data << -400 <<  0.179756;
	data <<    0 <<  0.122495;
	data <<  400 <<  0.077537;
	data <<  500 <<  0.066089;
	data << 1000 <<  0.018220;
	data << 1500 <<  0.002295;
	data << 1590 <<  0.001448;
	data << 1600 <<  0.001389;
	data << 1690 <<  0.000115;
	data << 1700 <<  0.000094;
	data << 1790 <<  0.000177;
	// TODO: Compute and fill the test data for range [1800..1986]
	//data << 1800 << 128.824; // pass: 1e-3 ; ??? seems equation has wrong signs for terms
	data << 1850 <<  1.755490;
	//data << 1890 <<  0.002340; // pass: 1e-4
	//data << 1900 <<  0.000591; // pass: 1e-3
	data << 1987 <<  0.000640;
	data << 2000 <<  0.000739;
	data << 2005 <<  0.000749;
	data << 2006 <<  0.000752;
	data << 2010 <<  0.000772;
	data << 2050 <<  0.001076;
	data << 2060 <<  0.002488;
	data << 2100 <<  0.002998;
	data << 2150 <<  0.003802;
	data << 2200 <<  0.005117;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByReingoldDershowitz(JD)/86400.;
		StelUtils::getDateFromJulianDay(JD, &yout, &mout, &dout);
		double actualError = qAbs(qAbs(expectedResult) - qAbs(result));
		QVERIFY2(actualError<=1e-5, QString("date=%2 year=%3 result=%4 expected=%5")
			 .arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout))
			 .arg(year)
			 .arg(QString::number(result, 'f', 5))
			 .arg(QString::number(expectedResult, 'f', 5))
			 .toUtf8());
	}
}

void TestDeltaT::testDeltaTByJPLHorizons()
{
	// NOTE: the test data just an calculated!
	// http://www.staff.science.uu.nl/~gent0113/deltat/deltat.htm with -25.7376 L.A.P. - WTF?!?
	QVariantList data;
	data <<   500 << 5401.2;
	data <<   600 << 4614.0;
	data <<   700 << 3888.6;
	data <<   800 << 3225.0;
	data <<   900 << 2623.8;
	data << 1000 << 1625.4;
	data << 1100 << 1265.4;
	data << 1200 <<   950.4;
	data << 1300 <<   680.4;
	data << 1400 <<   455.4;
	data << 1500 <<   275.4;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 1.0; // TODO: Increase accuracy to 0.1 seconds
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByJPLHorizons(JD);
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

void TestDeltaT::testDeltaTByBorkowski()
{
	// NOTE: the test data just an calculated!
	// http://www.staff.science.uu.nl/~gent0113/deltat/deltat.htm with -23.8946 L.A.P. - WTF?!?
	QVariantList data;
	data <<   500 << 4469.4;
	data <<   600 << 3717.0;
	data <<   700 << 3034.8;
	data <<   800 << 2422.2;
	data <<   900 << 1879.8;
	data << 1000 << 1407.0;
	data << 1100 << 1004.4;
	data << 1200 <<   672.0;
	data << 1300 <<   409.8;
	data << 1400 <<   217.2;
	data << 1500 <<     94.8;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 1.0; // TODO: Increase accuracy to 0.1 seconds
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByBorkowski(JD);
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

void TestDeltaT::testDeltaTByAstronomicalEphemeris()
{
	// NOTE: the test data just an calculated!
	// http://www.staff.science.uu.nl/~gent0113/deltat/deltat.htm with -22.44 L.A.P. - WTF?!?
	QVariantList data;
	data <<   500 <<  4882.2;
	data <<   600 <<  4145.4;
	data <<   700 <<  3469.2;
	data <<   800 <<  2852.4;
	data <<   900 <<  2296.2;
	data << 1000 <<  1799.4;
	data << 1100 <<  1362.6;
	data << 1200 <<    985.8;
	data << 1300 <<    668.4;
	data << 1400 <<    411.6;
	data << 1500 <<    214.2;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 1.0; // TODO: Increase accuracy to 0.1 seconds
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByAstronomicalEphemeris(JD);
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

void TestDeltaT::testDeltaTByIAU()
{
	// NOTE: the test data just an calculated!
	// http://www.staff.science.uu.nl/~gent0113/deltat/deltat.htm with -22.44 L.A.P. - WTF?!?
	QVariantList data;
	data <<   500 <<  4882.2;
	data <<   600 <<  4145.4;
	data <<   700 <<  3469.2;
	data <<   800 <<  2852.4;
	data <<   900 <<  2296.2;
	data << 1000 <<  1799.4;
	data << 1100 <<  1362.6;
	data << 1200 <<    985.8;
	data << 1300 <<    668.4;
	data << 1400 <<    411.6;
	data << 1500 <<    214.2;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 1.0; // TODO: Increase accuracy to 0.1 seconds
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByIAU(JD);
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

void TestDeltaT::testDeltaTByTuckermanGoldstine()
{
	// NOTE: the test data just an calculated!
	// http://www.staff.science.uu.nl/~gent0113/deltat/deltat.htm with 0 L.A.P.
	QVariantList data;
	data <<   500 <<  6724.8;
	data <<   600 <<  5766.6;
	data <<   700 <<  4882.2;
	data <<   800 <<  4071.0;
	data <<   900 <<  3333.0;
	data << 1000 <<  2669.4;
	data << 1100 <<  2079.4;
	data << 1200 <<  1562.4;
	data << 1300 <<  1119.0;
	data << 1400 <<    749.4;
	data << 1500 <<    453.0;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 1.0; // TODO: Increase accuracy to 0.1 seconds
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByTuckermanGoldstine(JD);
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

void TestDeltaT::testDeltaTStandardError()
{
	// the test data was obtained from https://eclipse.gsfc.nasa.gov/SEhelp/uncertainty2004.html
	QVariantList data;
	data << -1000 << 636;
	data <<  -500 << 431;
	data <<     0 << 265;
	data <<   500 << 139;
	data <<  1000 <<  54;
	data <<  1200 <<  31;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toInt();
		double acceptableError = 1.0;
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTStandardError(JD);
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
