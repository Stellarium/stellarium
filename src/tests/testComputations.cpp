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

#include "tests/testComputations.hpp"

#include <QString>
#include <QDebug>
#include <QtGlobal>
#include <QVariantList>
#include <QMap>

#include "StelUtils.hpp"

#define ERROR_LOW_LIMIT 1e-3
#define ERROR_MID_LIMIT 1e-4
#define ERROR_HIGH_LIMIT 1e-5

QTEST_GUILESS_MAIN(TestComputations)

void TestComputations::testSiderealPeriodComputations()
{
	QVariantList data;

	// According to WolframAlpha
	data << 1.00000011	<< 365.25636;	// Earth
	data << 0.38709893	<< 87.96926;	// Mercury
	data << 0.72333199	<< 224.7008;	// Venus

	while (data.count() >= 2)
	{
		double distance = data.takeFirst().toDouble();
		double exPeriod = data.takeFirst().toDouble();
		double period = StelUtils::calculateSiderealPeriod(distance);

		QVERIFY2(qAbs(period-exPeriod)<=ERROR_LOW_LIMIT, qPrintable(QString("Sidereal period is %1 days for %2 AU (expected %3 days)")
									.arg(QString::number(period, 'f', 6))
									.arg(QString::number(distance, 'f', 5))
									.arg(QString::number(exPeriod, 'f', 6))));
	}
}


void TestComputations::testJDFormBesselianEpoch()
{
	QVariantList data;

	// According to Observational Astrophysics by Pierre Lena, Francois Lebrun, Francois Mignard (ISBN 3662036851, 9783662036853)
	data << 1900.0		<< 2415020.3135;
	data << 1950.0		<< 2433282.4235;
	data << 1995.00048	<< 2449718.5;
	// FIXME: WTF???
	//data << 2000.0		<< 2451544.4334;
	//data << 1950.000210	<< 2433282.5;
	//data << 2000.001278	<< 2451545.0;

	while (data.count() >= 2)
	{
		double besselianEpoch	= data.takeFirst().toDouble();
		double expectedJD	= data.takeFirst().toDouble();
		double JD = StelUtils::getJDFromBesselianEpoch(besselianEpoch);

		QVERIFY2(qAbs(JD-expectedJD)<=ERROR_LOW_LIMIT, qPrintable(QString("Julian date for Besselian epoch %1 is %2 (expected %3)")
									.arg(QString::number(besselianEpoch, 'f', 6))
									.arg(QString::number(JD, 'f', 4))
									.arg(QString::number(expectedJD, 'f', 4))));
	}
}
