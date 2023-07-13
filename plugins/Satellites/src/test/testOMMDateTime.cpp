/*
 * Stellarium
 * Copyright (C) 2023 Andy Kirkham
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

#include <ios>

#include "testOMMDateTime.hpp"

QTEST_GUILESS_MAIN(TestOMMDateTime)

void TestOMMDateTime::testEpoch_DEFAULT()
{
	QString     epoch("23191.40640406"); // 2023-07-10T09:45:13.310784 :: JD 2460135.90640406
	OMMDateTime dut(epoch); // Assume TLE formatted string for ctor.
#ifdef _DEBUG
	qDebug() << "2023-07-10T09:45:13.310784";
	qDebug() << std::fixed << qSetRealNumberPrecision(20) << dut.getJulian();
#endif	
	QCOMPARE(dut.getJulian(), 2460135.906404059846);
}

void TestOMMDateTime::testEpoch_TLE()
{
	QString epoch("23191.40640406"); // 2023-07-10T09:45:13.310784 :: JD 2460135.90640406
	OMMDateTime dut(epoch, OMMDateTime::STR_TLE);
#ifdef _DEBUG
	qDebug() << "2023-07-10T09:45:13.310784";
	qDebug() << std::fixed << qSetRealNumberPrecision(20) << dut.getJulian();
#endif
	QCOMPARE(dut.getJulian(), 2460135.906404059846);
}

void TestOMMDateTime::testEpoch_ISO()
{
	QString epoch("2023-07-10T09:45:13.310784"); // JD 2460135.90640406
	OMMDateTime dut(epoch, OMMDateTime::STR_ISO8601);
#ifdef _DEBUG
	qDebug() << "2023-07-10T09:45:13.310784";
	qDebug() << std::fixed << qSetRealNumberPrecision(20) << dut.getJulian();
#endif
	QCOMPARE(dut.getJulian(), 2460135.906404059846);
}

void TestOMMDateTime::testOperatorEquals()
{
	QString epoch("23191.40640406"); // 2023-07-10T09:45:13.310784 :: JD 2460135.90640406
	OMMDateTime dut1(epoch, OMMDateTime::STR_TLE);
	QCOMPARE(dut1.getJulian(), 2460135.906404059846);
	
	OMMDateTime dut2;
	dut2 = dut1;
	QCOMPARE(dut1.getJulian(), dut2.getJulian());
}

void TestOMMDateTime::testCopyCTOR()
{
	QString     epoch("23191.40640406"); // 2023-07-10T09:45:13.310784 :: JD 2460135.90640406
	OMMDateTime dut1(epoch, OMMDateTime::STR_TLE);
	QCOMPARE(dut1.getJulian(), 2460135.906404059846);

	OMMDateTime dut2(dut1);
	QCOMPARE(dut1.getJulian(), dut2.getJulian());
}

void TestOMMDateTime::testToISO8601()
{
	QString epoch("23194.57613635"); // 2023-07-13T13:49:38.180640
	OMMDateTime dut(epoch, OMMDateTime::STR_TLE);
    QString s = dut.toISO8601String();

	// TLE dates are only accurate to 1/10 of a millisecond.
	QCOMPARE(s, "2023-07-13T13:49:38.1806");
}
