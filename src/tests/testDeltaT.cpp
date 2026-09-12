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

// (SS) 2025-11-27 Needed to locate ephemeris file for testing JPL Horizons Delta-T
#include "StelFileMgr.hpp"
#include "EphemWrapper.hpp"

#include "StelUtils.hpp"

QTEST_GUILESS_MAIN(TestDeltaT)

void TestDeltaT::initTestCase()
{
	// source: http://astro.ukho.gov.uk/nao/lvm/
	// the data obtainded 13 Jan. 2021

	//              Year         ΔT         ε (seconds)
	// ΔT (s) : −2000 to −800 (convert from hours)
	genericData << -2000.0  << 46080.00 << 1080.00;
	genericData << -1900.0  << 43560.00 << 1080.00;
	genericData << -1800.0  << 41040.00 << 1080.00;
	genericData << -1700.0  << 38880.00 << 1080.00;
	genericData << -1600.0  << 36720.00 <<  720.00;
	genericData << -1500.0  << 34920.00 <<  720.00;
	genericData << -1400.0  << 32760.00 <<  720.00;
	genericData << -1300.0  << 30960.00 <<  720.00;
	genericData << -1200.0  << 29160.00 <<  720.00;
	genericData << -1100.0  << 27360.00 <<  720.00;
	genericData << -1000.0  << 25560.00 <<  720.00;
	genericData <<  -900.0  << 23760.00 <<  360.00;
	genericData <<  -800.0  << 21960.00 <<  360.00;
	// ΔT (s) : −720 to +1600
	genericData <<  -720.0  << 20370.00 <<  180.00;
	genericData <<  -700.0  << 20050.00 <<  170.00;
	genericData <<  -600.0  << 18470.00 <<  160.00;
	genericData <<  -500.0  << 16940.00 <<  150.00;
	genericData <<  -400.0  << 15470.00 <<  130.00;
	genericData <<  -300.0  << 14080.00 <<  120.00;
	genericData <<  -200.0  << 12770.00 <<  110.00;
	genericData <<  -100.0  << 11560.00 <<  100.00;
	genericData <<     0.0  << 10440.00 <<   90.00;
	genericData <<   100.0  <<  9410.00 <<   80.00;
	genericData <<   200.0  <<  8420.00 <<   70.00;
	genericData <<   300.0  <<  7480.00 <<   60.00;
	genericData <<   400.0  <<  6540.00 <<   50.00;
	genericData <<   500.0  <<  5590.00 <<   40.00;
	genericData <<   600.0  <<  4650.00 <<   40.00;
	genericData <<   700.0  <<  3760.00 <<   30.00;
	genericData <<   800.0  <<  2940.00 <<   25.00;
	genericData <<   900.0  <<  2230.00 <<   20.00;
	genericData <<  1000.0  <<  1650.00 <<   15.00;
	genericData <<  1100.0  <<  1220.00 <<   15.00;
	genericData <<  1200.0  <<   910.00 <<   15.00;
	genericData <<  1300.0  <<   680.00 <<   15.00;
	genericData <<  1400.0  <<   480.00 <<   15.00;
	genericData <<  1500.0  <<   290.00 <<   15.00;
	genericData <<  1600.0  <<   110.00 <<   15.00;
	// ΔT (s) : +1600 to +1800
	genericData <<  1600.0  <<   109.00 <<   15.00;
	genericData <<  1610.0  <<    94.00 <<   15.00;
	genericData <<  1620.0  <<    80.00 <<   20.00;
	genericData <<  1630.0  <<    66.00 <<   20.00;
	genericData <<  1640.0  <<    54.00 <<   20.00;
	genericData <<  1650.0  <<    44.00 <<   20.00;
	genericData <<  1660.0  <<    35.00 <<   15.00;
	genericData <<  1670.0  <<    28.00 <<   10.00;
	genericData <<  1680.0  <<    22.00 <<    5.00;
	genericData <<  1690.0  <<    17.00 <<    5.00;
	genericData <<  1700.0  <<    14.00 <<    5.00;
	genericData <<  1710.0  <<    12.00 <<    5.00;
	genericData <<  1720.0  <<    12.00 <<    5.00;
	genericData <<  1730.0  <<    13.00 <<    2.00;
	genericData <<  1740.0  <<    15.00 <<    2.00;
	genericData <<  1750.0  <<    17.00 <<    2.00;
	genericData <<  1760.0  <<    19.00 <<    2.00;
	genericData <<  1770.0  <<    21.00 <<    1.00;
	genericData <<  1780.0  <<    21.00 <<    1.00;
	genericData <<  1790.0  <<    21.00 <<    1.00;
	genericData <<  1800.0  <<    18.00 <<    0.50;
	genericData <<  1810.0  <<    16.00 <<    0.50;
	// ΔT (s) : +1800 to +1950
	genericData <<  1800.0  <<    18.40 <<    0.50;
	genericData <<  1805.0  <<    16.60 <<    0.30;
	genericData <<  1810.0  <<    15.70 <<    0.20;
	genericData <<  1815.0  <<    16.40 <<    0.20;
	genericData <<  1820.0  <<    16.50 <<    0.20;
	genericData <<  1825.0  <<    14.10 <<    0.20;
	genericData <<  1830.0  <<    10.80 <<    0.20;
	genericData <<  1835.0  <<     8.50 <<    0.10;
	genericData <<  1840.0  <<     7.60 <<    0.10;
	genericData <<  1845.0  <<     8.00 <<    0.10;
	genericData <<  1850.0  <<     9.30 <<    0.10;
	genericData <<  1855.0  <<    10.36 <<    0.10;
	genericData <<  1860.0  <<     9.04 <<    0.10;
	genericData <<  1865.0  <<     8.25 <<    0.10;
	genericData <<  1870.0  <<     2.37 <<    0.05;
	genericData <<  1875.0  <<    -1.13 <<    0.05;
	genericData <<  1880.0  <<    -3.21 <<    0.05;
	genericData <<  1885.0  <<    -4.39 <<    0.05;
	genericData <<  1890.0  <<    -3.88 <<    0.05;
	genericData <<  1895.0  <<    -5.02 <<    0.05;
	genericData <<  1900.0  <<    -1.98 <<    0.05;
	genericData <<  1905.0  <<     4.92 <<    0.05;
	genericData <<  1910.0  <<    11.14 <<    0.05;
	genericData <<  1915.0  <<    17.48 <<    0.05;
	genericData <<  1920.0  <<    21.62 <<    0.05;
	genericData <<  1925.0  <<    23.79 <<    0.05;
	genericData <<  1930.0  <<    24.42 <<    0.05;
	genericData <<  1935.0  <<    24.16 <<    0.05;
	genericData <<  1940.0  <<    24.43 <<    0.05;
	genericData <<  1945.0  <<    27.05 <<    0.05;
	genericData <<  1950.0  <<    28.93 <<    0.05;
	// ΔT (s) : +1950 to +2019
	genericData <<  1950.0  <<    28.93 <<    0.05;
	genericData <<  1951.0  <<    29.32 <<    0.05;
	genericData <<  1952.0  <<    29.70 <<    0.05;
	genericData <<  1953.0  <<    30.00 <<    0.05;
	genericData <<  1954.0  <<    30.20 <<    0.05;
	genericData <<  1955.0  <<    30.41 <<    0.05;
	genericData <<  1956.0  <<    30.76 <<    0.05;
	genericData <<  1957.0  <<    31.34 <<    0.05;
	genericData <<  1958.0  <<    32.03 <<    0.05;
	genericData <<  1959.0  <<    32.65 <<    0.05;
	genericData <<  1960.0  <<    33.07 <<    0.05;
	genericData <<  1961.0  <<    33.36 <<    0.05;
	genericData <<  1962.0  <<    33.62 <<    0.05;
	genericData <<  1963.0  <<    33.96 <<    0.05;
	genericData <<  1964.0  <<    34.44 <<    0.05;
	genericData <<  1965.0  <<    35.09 <<    0.05;
	genericData <<  1966.0  <<    35.95 <<    0.05;
	genericData <<  1967.0  <<    36.93 <<    0.05;
	genericData <<  1968.0  <<    37.96 <<    0.05;
	genericData <<  1969.0  <<    38.95 <<    0.05;
	genericData <<  1970.0  <<    39.93 <<    0.05;
	genericData <<  1971.0  <<    40.95 <<    0.05;
	genericData <<  1972.0  <<    42.04 <<    0.05;
	genericData <<  1973.0  <<    43.15 <<    0.05;
	genericData <<  1974.0  <<    44.24 <<    0.05;
	genericData <<  1975.0  <<    45.28 <<    0.05;
	genericData <<  1976.0  <<    46.28 <<    0.05;
	genericData <<  1977.0  <<    47.29 <<    0.05;
	genericData <<  1978.0  <<    48.33 <<    0.05;
	genericData <<  1979.0  <<    49.37 <<    0.05;
	genericData <<  1980.0  <<    50.36 <<    0.05;
	genericData <<  1981.0  <<    51.28 <<    0.05;
	genericData <<  1982.0  <<    52.13 <<    0.05;
	genericData <<  1983.0  <<    52.94 <<    0.05;
	genericData <<  1984.0  <<    53.70 <<    0.05;
	genericData <<  1985.0  <<    54.39 <<    0.05;
	genericData <<  1986.0  <<    54.98 <<    0.05;
	genericData <<  1987.0  <<    55.46 <<    0.05;
	genericData <<  1988.0  <<    55.89 <<    0.05;
	genericData <<  1989.0  <<    56.37 <<    0.05;
	genericData <<  1990.0  <<    56.99 <<    0.05;
	genericData <<  1991.0  <<    57.70 <<    0.05;
	genericData <<  1992.0  <<    58.45 <<    0.05;
	genericData <<  1993.0  <<    59.19 <<    0.05;
	genericData <<  1994.0  <<    59.92 <<    0.05;
	genericData <<  1995.0  <<    60.68 <<    0.05;
	genericData <<  1996.0  <<    61.46 <<    0.05;
	genericData <<  1997.0  <<    62.23 <<    0.05;
	genericData <<  1998.0  <<    62.90 <<    0.05;
	genericData <<  1999.0  <<    63.42 <<    0.05;
	genericData <<  2000.0  <<    63.81 <<    0.05;
	genericData <<  2001.0  <<    64.08 <<    0.05;
	genericData <<  2002.0  <<    64.27 <<    0.05;
	genericData <<  2003.0  <<    64.41 <<    0.05;
	genericData <<  2004.0  <<    64.55 <<    0.05;
	genericData <<  2005.0  <<    64.73 <<    0.05;
	genericData <<  2006.0  <<    64.95 <<    0.05;
	genericData <<  2007.0  <<    65.20 <<    0.05;
	genericData <<  2008.0  <<    65.48 <<    0.05;
	genericData <<  2009.0  <<    65.77 <<    0.05;
	genericData <<  2010.0  <<    66.06 <<    0.05;
	genericData <<  2011.0  <<    66.33 <<    0.05;
	genericData <<  2012.0  <<    66.61 <<    0.05;
	genericData <<  2013.0  <<    66.92 <<    0.05;
	genericData <<  2014.0  <<    67.28 <<    0.05;
	genericData <<  2015.0  <<    67.69 <<    0.05;
	genericData <<  2016.0  <<    68.11 <<    0.05;
	genericData <<  2017.0  <<    68.53 <<    0.05;
	genericData <<  2018.0  <<    68.92 <<    0.05;
	genericData <<  2019.0  <<    69.24 <<    0.05;
	// ΔT (seconds) : +2020 to +2500
	//genericData <<  2020.0  <<       69.50 <<     0.10;
	genericData <<  2030.0  <<    67.00 <<    2.00;
	genericData <<  2040.0  <<    68.00 <<    4.00;
	genericData <<  2050.0  <<    70.00 <<    6.00;
	genericData <<  2100.0  <<    80.00 <<   10.00;
	genericData <<  2200.0  <<   160.00 <<   20.00;
	genericData <<  2300.0  <<   330.00 <<   30.00;
	genericData <<  2400.0  <<   610.00 <<   50.00;
	genericData <<  2500.0  <<  1000.00 <<  100.00;

	// (SS) 2025-11-27 Initialize Stellarium file manager to locate ephemeris files
	StelFileMgr::init();

	//(SS) 2025-11-27: The linux_p1550p2650.440t file must be in the Stellarium user or installation directory
	de440FilePath = StelFileMgr::findFile("ephem/" + QString(DE440_FILENAME), StelFileMgr::File);
	if (!de440FilePath.isEmpty())
	{
		qInfo() << "Use DE440 ephemeris file" << de440FilePath;
		EphemWrapper::init_de440(de440FilePath.toLocal8Bit());
	}
	else
	{
		qWarning() << "DE440 ephemeris file not found, JPL Horizons test will not be performed";
	}
}

void TestDeltaT::testDeltaTByEspenakMeeus()
{
	// test data from https://eclipse.gsfc.nasa.gov/SEcat5/deltat.html#tab1
	// and https://eclipsewise.com/solar/solar.html (2015-)
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
	data << 2015  << 67.7  << 2; // https://eclipsewise.com/solar/SEprime/2001-2100/SE2015Mar20Tprime.html
	data << 2020  << 70.0  << 7; // https://eclipsewise.com/solar/SEprime/2001-2100/SE2020Jun21Aprime.html
	data << 2030  << 74.0  << 12; // https://eclipsewise.com/solar/SEprime/2001-2100/SE2030Jun01Aprime.html
	data << 2050  << 84.3 << 20; // https://eclipsewise.com/solar/SEprime/2001-2100/SE2050May20Hprime.html
	data << 2060  << 90.6 << 15; // https://eclipsewise.com/solar/SEprime/2001-2100/SE2060Apr30Tprime.html
	data << 2070  << 97.7 << 15; // https://eclipsewise.com/solar/SEprime/2001-2100/SE2070Apr11Tprime.html
	data << 2090  << 114.8 << 15; // https://eclipsewise.com/solar/SEprime/2001-2100/SE2090Sep23Tprime.html
	data << 2100  << 124.3 << 20; // https://eclipsewise.com/solar/SEprime/2001-2100/SE2100Sep04Tprime.html

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

void TestDeltaT::testDeltaTByStephenson1997()
{
	// test data taken from http://ebooks.cambridge.org/ebook.jsf?bid=CBO9780511525186

	QVariantList data;
	//data << -500 << 16800;
	//data << -400 << 15300;
	//data << -300 << 14000;
	//data << -200 << 12800;
	//data << -100 << 11600;
	data <<    0 << 10600;
	data <<  100 <<  9600;
	data <<  200 <<  8600;
	data <<  300 <<  7700;
	data <<  400 <<  6700;
	data <<  500 <<  5700;
	data <<  600 <<  4700;
	data <<  700 <<  3800;
	//data <<  800 <<  3000;
	//data <<  900 <<  2200;
	//data << 1000 <<  1600;
	//data << 1100 <<  1100;
	//data << 1200 <<   750;
	//data << 1300 <<   470;
	//data << 1400 <<   300;
	//data << 1500 <<   180;
	//data << 1600 <<   110;

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
	// Valid range: 720 BC to AD 2019

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

		if (year>=-720. && year<2019.)
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

void TestDeltaT::testDeltaTByStephensonMorrisonHohenkerk2016SpecialDates()
{
	QVariantList data;
	// From United States Naval Observatory table
	// see details: https://github.com/Stellarium/stellarium-web-engine/blob/53906077226a735473023d4c5beb4adce6855903/src/algos/deltat.c#L108
	data << 1980 <<  50.5387;
	data << 2000 <<  63.8285;
	data << 2010 <<  66.0699;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 0.2; // TODO: Increase accuracy to 0.1 seconds
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByStephensonMorrisonHohenkerk2016(JD);
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
	data << 1800 <<  0.000160;
	data << 1850 <<  0.000080;
	data << 1890 << -0.000070;
	data << 1900 << -0.000020;
	data << 1987 <<  0.000640;
	data << 2000 <<  0.000739;
	data << 2005 <<  0.000749;
	data << 2006 <<  0.000752;
	data << 2010 <<  0.000772;
	data << 2050 <<  0.001076;
	data << 2060 <<  0.002490;
	data << 2100 <<  0.003000;
	data << 2150 <<  0.003802;
	data << 2200 <<  0.005117;

	// TODO: switch to use seconds
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
			 .arg(QString("%1-%2-%3 00:00:00").arg(yout).arg(mout).arg(dout)).arg(year)
			 .arg(QString::number(result, 'f', 5), QString::number(expectedResult, 'f', 5))
			 .toUtf8());
	}
}

// (SS) 2025-11-27 Revised test case for JPL Horizons Delta-T validation:
void TestDeltaT::testDeltaTByJPLHorizons()
{
	// NOTE: Test data were obtained from JPL Horizons application which are based on DE441 Ephemerides
	// source: https://ssd.jpl.nasa.gov/horizons/app.html#/
	// Settings used with JPL Horizons app:
	// Ephemeris Type: Observer Table
	// Target Body: Mercury
	// Observer Location: Geocentric [code: 500]
	// Time Specification: Time-Form: JD (Julian Day), Time-Type: (UT)
	// Table Settings (partial list):
	//  - checkbox 2, 30
	//  - Reference frame: ICRF
	//  - Date/time format: calendar and Julian Day Number
	//  - Calendar type: Mixed
	//  - Time digits: HH:MM
	//  - Rferaction model: no refraction (airless)
	//  - Range units: astronomical units (au)
	//  - RTS flag: disabled
	//  - CSV format: checked
	//  - Object summary: checked

	// Data table structure: JD_UT, deltaT (seconds)
	QVariantList data;

	// Random JD values covering time frame: 9999BC-03-20 to 721BC-01-01

	data << -1930633.50000 << 439804.45257; // b9999-Mar-20 00:00:00.000
	data << -1688582.04875 << 391890.47426; // b9337-Nov-30 10:49:48.000
	data << -1446531.73978 << 346739.76079; // b8674-Aug-12 18:14:43.008
	data << -1204481.28541 << 304352.05489; // b8011-Apr-24 05:09:00.576
	data << -962439.97134 << 264728.82866;  // b7349-Dec-25 12:41:16.224
	data << -720379.37761 << 227865.60324;  // b7349-Dec-25 12:41:16.224
	data << -478328.56448 << 193766.96684;  // b6023-May-29 22:27:08.928
	data << -236278.18537 << 162431.42787;  // b5360-Feb-08 07:33:04.032
	data << 5772.12345    << 133858.93061;  // b4698-Oct-21 14:57:46.080
	data << 247823.42395  << 108049.36547;  // b4035-Jul-03 22:10:29.280
	data << 489874.66318  << 85002.860653;  // b3372-Mar-16 03:54:58.752
	data << 731925.79532  << 64719.417649;  // b2710-Nov-27 07:05:15.648
	data << 973975.36328  << 47199.129939;  // b2047-Aug-07 20:43:07.392
	data << 1216026.92947 << 32441.747096;  // b1384-Apr-20 10:18:26.208
	data << 1458077.49999 << 20447.468820;  // b0722-Dec-31 23:59:59.136
	
	// Random JD values covering time frame: 721BC-01-01 to 1962-02-20

	data << 1523384.48723 << 17628.307203;  // b0543-Oct-19 23:41:36.672
	data << 1588691.18456 << 14997.831155;  // b0364-Aug-07 16:25:45.984
	data << 1653998.78296 << 12614.877477;  // b0185-May-27 06:47:27.744
	data << 1719305.87213 << 10533.423426;  // b0006-Mar-16 08:55:52.032
	data << 1784612.23865 << 8709.051459;   // 0174-Jan-01 17:43:39.360
	data << 1849919.98126 << 7006.929127;   // 0352-Oct-21 11:33:00.864
	data << 1915226.03956 << 5309.637916;   // 0531-Aug-09 12:56:57.984
	data << 1980533.33681 << 3687.778516;   // 0710-May-28 20:05:00.384
	data << 2045840.75298 << 2312.725605;   // 0889-Mar-17 06:04:17.472
	data << 2111147.89432 << 1352.842646;   // 1068-Jan-04 09:27:49.248
	data << 2176454.45721 << 803.995322;    // 1246-Oct-22 22:58:22.944
	data << 2241761.00974 << 436.642375;    // 1425-Aug-10 12:14:01.536
	data << 2307068.25431 << 103.529811;    // 1604-Jun-08 18:06:12.384
	data << 2372375.98746 << 21.699491;     // 1783-Mar-30 11:41:56.544
	data << 2437683.49857 << 33.635365;     // 1962-Jan-18 23:57:56.448

	// Random JD values covering time frame: 1962-02-20 to 2025-01-01

	data << 2437684.50000 << 34.051667;     // 1962-Jan-20 00:00:00.000
	data << 2439326.68324 << 37.015633;     // 1966-Jul-20 04:23:51.936
	data << 2440968.12765 << 41.171056;     // 1971-Jan-16 15:03:48.960
	data << 2442610.98457 << 46.183683;     // 1975-Jul-17 11:37:46.848
	data << 2444252.42398 << 51.184311;     // 1980-Jan-13 22:10:31.872
	data << 2445894.00347 << 54.183764;     // 1984-Jul-12 12:04:59.808
	data << 2447536.92384 << 56.184204;     // 1989-Jan-10 10:10:19.776
	data << 2449178.34297 << 60.183836;     // 1993-Jul-09 20:13:52.608
	data << 2450820.76576 << 63.184083;     // 1998-Jan-07 06:22:41.664
	data << 2454104.95603 << 65.184030;     // 2007-Jan-04 10:56:40.992
	data << 2455746.04392 << 66.184080;     // 2011-Jul-03 13:03:14.688
	data << 2457388.52387 << 68.183931;     // 2016-Jan-01 00:34:22.368
	data << 2459030.98735 << 69.184123;     // 2020-Jun-30 11:41:47.040
	data << 2460676.50000 << 69.183919;     // 2025-Jan-01 00:00:00.000

	// Random JD values covering time frame: 2025-01-01 to 2650-12-31

	data << 2461766.38765 << 69.183771;     // 2027-Dec-26 21:18:12.960
	data << 2486040.37875 << 69.184679;     // 2094-Jun-11 21:05:24.000
	data << 2511404.12977 << 69.182782;     // 2163-Nov-21 15:06:52.128
	data << 2536768.98567 << 69.185498;     // 2233-May-03 11:39:21.888
	data << 2562132.53472 << 69.182346;     // 2302-Oct-13 00:49:59.808
	data << 2587496.00345 << 69.185559;     // 2372-Mar-22 12:04:58.080
	data << 2612860.98542 << 69.182768;     // 2441-Sep-01 11:39:00.288
	data << 2638224.27398 << 69.184789;     // 2511-Feb-10 18:34:31.872
	data << 2663588.74930 << 69.183805;     // 2580-Jul-22 05:58:59.520

	// Random JD values covering time frame: 2650-01-25 to 9999-12-31

	data << 2688952.50000 << 69.183588;     // 2650-Jan-01 00:00:00.000
	data << 2876738.13756 << 69.184765;     // 3164-Feb-21 15:18:05.184
	data << 3084769.85693 << 69.182935;     // 3733-Sep-17 08:33:58.752
	data << 3292800.64289 << 69.185331;     // 4303-Apr-14 03:25:45.696
	data << 3500831.04572 << 69.182564;     // 4872-Nov-05 13:05:50.208
	data << 3708862.59450 << 69.185484;     // 5442-Jun-03 02:16:04.800
	data << 3916893.23856 << 69.182605;     // 6011-Dec-27 17:43:31.584
	data << 4124924.87623 << 69.185213;     // 6581-Jul-23 09:01:46.272
	data << 4332955.99532 << 69.183032;     // 7151-Feb-17 11:53:15.648
	data << 4540986.00234 << 69.184656;     // 7720-Sep-11 12:03:22.176
	data << 4749017.59456 << 69.183699;     // 8290-Apr-08 02:16:09.984
	data << 4957048.05673 << 69.183972;     // 8859-Nov-01 13:21:41.472
	data << 5165079.12845 << 69.184375;     // 9429-May-28 15:04:58.080
	data << 5373119.49877 << 69.183192;     // 9998-Dec-31 23:58:13.728

	// Moon secular acceleration parameters
	const double deltaTnDot = -25.82;
	const bool useDE43x     = false;
	const bool useDE44x     = true;

	// Pass/Fail criteria +/- 0.1 seconds from 9999BC to 1962-01-20
	double acceptableError_before_1962 = 0.1;
	// Pass/Fail criteria +/- 10 microseconds from 1962-02-20 to 2650-01-25
	double acceptableError_1962_to_2650 = 0.00001;
	// Pass/Fail criteria +/- 2 millisecond from 2650-01-25 to 9999AD
	double acceptableError_after_2650 = 0.002;

	if (de440FilePath.isEmpty())
		qWarning() << "JPL Horizons test has been marked as 'passed' (It cannot be passed, because DE440 file has not been found)!";
	else
	{	
		while (data.count() >= 2)
		{
			double JD = data.takeFirst().toDouble();

			if (JD < 2437684.5) // les than 1962-01-20
			{
				double expectedResult = data.takeFirst().toDouble();
				double result         = StelUtils::getDeltaTByJPLHorizons(JD);
				result += StelUtils::getMoonSecularAcceleration(JD, deltaTnDot, useDE43x, useDE44x); // Correction is done only up to 1955.5
				double actualError = qAbs(qAbs(expectedResult) - qAbs(result));

				QString dateTime = StelUtils::julianDayToISO8601String(JD, true);

				QVERIFY2(actualError <= acceptableError_before_1962,
						QString("date=%1 JD=%2 result=%3 expected=%4 error=%5 acceptable=%6")
								.arg(dateTime)
								.arg(JD, 0, 'f', 5)
								.arg(result, 0, 'f', 5)
								.arg(expectedResult, 0, 'f', 5)
								.arg(actualError, 0, 'f', 5)
								.arg(acceptableError_before_1962, 0, 'f', 5)
								.toUtf8());
			}
			else if (JD >= 2437684.5 && JD < 2688976.5) // between 1962-01-20 and 2650-01-25
			{
				double expectedResult = data.takeFirst().toDouble();
				double result         = StelUtils::getDeltaTByJPLHorizons(JD);
				result += StelUtils::getMoonSecularAcceleration(JD, deltaTnDot, useDE43x, useDE44x);
				double actualError = qAbs(qAbs(expectedResult) - qAbs(result));

				QString dateTime = StelUtils::julianDayToISO8601String(JD, true);

				QVERIFY2(actualError <= acceptableError_1962_to_2650,
				         QString("date=%1 JD=%2 result=%3 expected=%4 error=%5 acceptable=%6")
				                 .arg(dateTime)
				                 .arg(JD, 0, 'f', 5)
				                 .arg(result, 0, 'f', 5)
				                 .arg(expectedResult, 0, 'f', 5)
				                 .arg(actualError, 0, 'f', 5)
				                 .arg(acceptableError_1962_to_2650, 0, 'f', 5)
				                 .toUtf8());
			}
			else // after 2650-01-25
			{
				double expectedResult = data.takeFirst().toDouble();
				double result         = StelUtils::getDeltaTByJPLHorizons(JD);
				result += StelUtils::getMoonSecularAcceleration(JD, deltaTnDot, useDE43x, useDE44x);
				double actualError = qAbs(qAbs(expectedResult) - qAbs(result));
				QString dateTime   = StelUtils::julianDayToISO8601String(JD, true);
				QVERIFY2(actualError <= acceptableError_after_2650,
				         QString("date=%1 JD=%2 result=%3 expected=%4 error=%5 acceptable=%6")
				                 .arg(dateTime)
				                 .arg(JD, 0, 'f', 5)
				                 .arg(result, 0, 'f', 5)
				                 .arg(expectedResult, 0, 'f', 5)
				                 .arg(actualError, 0, 'f', 5)
				                 .arg(acceptableError_after_2650, 0, 'f', 5)
				                 .toUtf8());
			}
		}				
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

void TestDeltaT::testDeltaTByChaprontTouze()
{
	QVariantList data;
	// check outside valid range
	data << -395 <<  0.0;
	data << 1610 <<  0.0;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 1.0; // TODO: Increase accuracy to 0.1 seconds
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByChaprontTouze(JD);
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

void TestDeltaT::testDeltaTByIAUWideDates()
{
	// Source of test data: https://web.archive.org/web/20150226203358/http://user.online.be/felixverbelen/dt.htm
	QVariantList data;
	data << -2000 << 42757.0;
	data << -1900 << 40524.0;
	data << -1800 << 38350.0;
	data << -1700 << 36236.0;
	data << -1600 << 34181.0;
	data << -1500 << 32187.0;
	data << -1400 << 30253.0;
	data << -1300 << 28378.0;
	data << -1200 << 26564.0;
	data << -1100 << 24809.0;
	data << -1000 << 23115.0;
	data <<  -900 << 21480.0;
	data <<  -800 << 19905.0;
	data <<  -700 << 18390.0;
	data <<  -600 << 16935.0;
	data <<  -500 << 15539.0;
	data <<  -400 << 14204.0;
	data <<  -300 << 12929.0;
	data <<  -200 << 11713.0;
	data <<  -100 << 10557.0;
	data <<     0 <<  9462.0;
	data <<   100 <<  8426.0;
	data <<   200 <<  7450.0;
	data <<   300 <<  6534.0;
	data <<   400 <<  5678.0;
	data <<   500 <<  4882.0;
	data <<   600 <<  4145.0;
	data <<   700 <<  3469.0;
	data <<   800 <<  2852.0;
	data <<   900 <<  2296.0;
	data <<  1000 <<  1799.0;
	data <<  1100 <<  1362.0;
	data <<  1200 <<   985.0;
	data <<  1300 <<   668.0;
	data <<  1400 <<   411.0;
	data <<  1500 <<   214.0;
	data <<  1600 <<    76.0;
	data <<  1700 <<    -1.0;
	//data <<  1800 <<   -19.0;
	//data <<  1900 <<    24.0;
	data <<  2000 <<   126.0;

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

void TestDeltaT::testDeltaTByStephensonHoulden()
{
	// Source of test data: https://web.archive.org/web/20150226203358/http://user.online.be/felixverbelen/dt.htm
	QVariantList data;
	data << -2000 << 54181.0;
	data << -1900 << 51081.0;
	data << -1800 << 48073.0;
	data << -1700 << 45159.0;
	data << -1600 << 42338.0;
	data << -1500 << 39610.0; // begin of valid range
	data << -1400 << 36975.0;
	data << -1300 << 34433.0;
	data << -1200 << 31984.0;
	data << -1100 << 29627.0;
	data << -1000 << 27364.0;
	data <<  -900 << 25194.0;
	data <<  -800 << 23117.0;
	data <<  -700 << 21133.0;
	data <<  -600 << 19242.0;
	data <<  -500 << 17444.0;
	data <<  -400 << 15738.0;
	data <<  -300 << 14126.0;
	data <<  -200 << 12607.0;
	data <<  -100 << 11181.0;
	data <<     0 <<  9848.0;
	data <<   100 <<  8608.0;
	data <<   200 <<  7461.0;
	data <<   300 <<  6406.0;
	data <<   400 <<  5445.0;
	data <<   500 <<  4577.0;
	data <<   600 <<  3802.0;
	data <<   700 <<  3120.0;
	data <<   800 <<  2531.0;
	data <<   900 <<  2035.0;
	data <<  1000 <<  1625.0;
	data <<  1100 <<  1265.0;
	data <<  1200 <<   950.0;
	data <<  1300 <<   680.0;
	data <<  1400 <<   455.0;
	data <<  1500 <<   275.0;
	data <<  1600 <<   140.0;

	while(data.count() >= 2)
	{
		int year = data.takeFirst().toInt();
		int yout, mout, dout;
		double JD;
		double expectedResult = data.takeFirst().toDouble();
		double acceptableError = 1.0; // 1 sec
		StelUtils::getJDFromDate(&JD, year, 1, 1, 0, 0, 0);
		double result = StelUtils::getDeltaTByStephensonHoulden(JD);
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
