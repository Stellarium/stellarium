/*
 * Stellarium 
 * Copyright (C) 2019 Alexander Wolf
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

#include <QObject>
#include <QtDebug>
#include <QVariantList>

#include "tests/testSiderealTime.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"
#include "Orbit.hpp"

QTEST_GUILESS_MAIN(TestSiderealTime)

void TestSiderealTime::initTestCase()
{
}

void TestSiderealTime::testGreenwichMeanSiderealTime()
{
	QVariantList data;
	// JD << h << m << s
	// year 1942;
	// source: Astronomical Yearbook of USSR for the Year 1942 (Original: Астрономический ежегодник СССР на 1942 г)
	data << 2430360.5 <<  6 << 40 <<  3.137;
	data << 2430391.5 <<  8 << 42 << 16.391;
	data << 2430419.5 << 10 << 32 << 39.890;
	data << 2430450.5 << 12 << 34 << 52.986;
	data << 2430480.5 << 14 << 33 <<  9.561;
	data << 2430511.5 << 16 << 35 << 22.787;
	data << 2430541.5 << 18 << 33 << 39.501;
	data << 2430572.5 << 20 << 35 << 52.728;
	data << 2430603.5 << 22 << 38 <<  5.879;
	data << 2430633.5 <<  0 << 36 << 22.443;
	data << 2430664.5 <<  2 << 38 << 35.586;
	data << 2430694.5 <<  4 << 36 << 52.244;
	// year 1993;
	// source: Astronomical Yearbook of USSR for the Year 1993 (Original: Астрономический ежегодник СССР на 1993 г)
	data << 2448988.5 <<  6 << 42 << 36.7508;
	data << 2449019.5 <<  8 << 44 << 49.9672;
	data << 2449047.5 << 10 << 35 << 13.5175;
	data << 2449078.5 << 12 << 37 << 26.7339;
	data << 2449108.5 << 14 << 35 << 43.3949;
	data << 2449139.5 << 16 << 37 << 56.6113;
	data << 2449169.5 << 18 << 36 << 13.2723;
	data << 2449200.5 << 20 << 38 << 26.4887;
	data << 2449231.5 << 22 << 40 << 39.7051;
	data << 2449261.5 <<  0 << 38 << 56.4662;
	data << 2449292.5 <<  2 << 41 <<  9.5825;
	data << 2449322.5 <<  4 << 39 << 26.2436;
	// year 2017;
	// source: Astronomical Almanac for the Year 2017
	data << 2457844.5 << 12 << 38 << 11.0891;
	data << 2457874.5 << 14 << 36 << 27.7502;
	data << 2457905.5 << 16 << 38 << 40.9666;
	data << 2457935.5 << 18 << 36 << 57.6276;
	data << 2457966.5 << 20 << 39 << 10.8440;

	double JD, JDE, s, est, ast, actualError, acceptableError = 2e-4;
	int h, m;

	while(data.count() >= 4)
	{
		JD = data.takeFirst().toDouble();
		JDE = JD + StelUtils::getDeltaTByEspenakMeeus(JD)/86400.;
		h = data.takeFirst().toInt();
		m = data.takeFirst().toInt();
		s = data.takeFirst().toDouble();
		est = h + m/60. + s/3600.;
		ast = get_mean_sidereal_time(JD, JDE)/15.;
		ast=fmod(ast, 24.);
		if (ast < 0.) ast+=24.;
		actualError = qAbs(qAbs(est) - qAbs(ast));
		QVERIFY2(actualError <= acceptableError, QString("JD=%1 expected=%2 result=%3 error=%4")
							.arg(QString::number(JD, 'f', 1))
							.arg(StelUtils::hoursToHmsStr(est))
							.arg(StelUtils::hoursToHmsStr(ast))
							.arg(actualError)
							.toUtf8());
	}
}

void TestSiderealTime::testGreenwichApparentSiderealTime()
{
	QVariantList data;
	// JD << h << m << s
	// year 2017;
	// source: Astronomical Almanac for the Year 2017
	data << 2457844.5 << 12 << 38 << 10.5428;
	data << 2457874.5 << 14 << 36 << 27.1459;
	data << 2457905.5 << 16 << 38 << 40.3712;
	data << 2457935.5 << 18 << 36 << 57.0663;
	data << 2457966.5 << 20 << 39 << 10.2937;

	double JD, JDE, s, est, ast, actualError, acceptableError = 1e-4;
	int h, m;

	while(data.count() >= 4)
	{
		JD = data.takeFirst().toDouble();
		JDE = JD + StelUtils::getDeltaTByEspenakMeeus(JD)/86400.;
		h = data.takeFirst().toInt();
		m = data.takeFirst().toInt();
		s = data.takeFirst().toDouble();
		est = h + m/60. + s/3600.;
		ast = get_apparent_sidereal_time(JD, JDE)/15.;
		ast=fmod(ast, 24.);
		if (ast < 0.) ast+=24.;
		actualError = qAbs(qAbs(est) - qAbs(ast));
		QVERIFY2(actualError <= acceptableError, QString("JD=%1 expected=%2 result=%3 error=%4")
							.arg(QString::number(JD, 'f', 1))
							.arg(StelUtils::hoursToHmsStr(est))
							.arg(StelUtils::hoursToHmsStr(ast))
							.arg(actualError)
							.toUtf8());
	}
}

void TestSiderealTime::testSiderealPeriodComputations()
{
	QVariantList data;
	// According to WolframAlpha
	data << 1.00000011	<< 365.25636;	// Earth
	data << 0.38709893	<< 87.96926;	// Mercury
	data << 0.72333199	<< 224.7008;	// Venus

	double acceptableError = 1e-3;
	while (data.count() >= 2)
	{
		double distance = data.takeFirst().toDouble();
		double exPeriod = data.takeFirst().toDouble();
		double period = KeplerOrbit::calculateSiderealPeriod(distance, 1.);

		QVERIFY2(qAbs(period-exPeriod)<=acceptableError, qPrintable(QString("Sidereal period is %1 days for %2 AU (expected %3 days)")
									.arg(QString::number(period, 'f', 6))
									.arg(QString::number(distance, 'f', 5))
									.arg(QString::number(exPeriod, 'f', 6))));
	}
}
