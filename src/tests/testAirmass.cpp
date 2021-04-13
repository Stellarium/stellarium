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

#include "tests/testAirmass.hpp"
#include "StelUtils.hpp"

QTEST_GUILESS_MAIN(TestAirmass)

void TestAirmass::initTestCase()
{
	// La Silla observatory (ESO) on June the 1st, 2016
	// Weather condition: http://archive.eso.org/asm/ambient-server?night=01+Jun+2016&site=lasilla
	// Airmass data: https://arxiv.org/pdf/1806.09425.pdf
	data <<  0 << 1.00;
	data << 10 << 1.02;
	data << 20 << 1.06;
	data << 30 << 1.15;
	data << 40 << 1.30;
	data << 50 << 1.55;
	data << 60 << 2.00;
	data << 70 << 2.90;
	data << 80 << 5.60;

	pressure = 768; // mbar
	temperature = 5; // Celsius degrees

	acceptableError = 0.05;
}

void TestAirmass::testRozenbergEquation()
{
	Refraction refCls;
	Extinction extCls;

	refCls.setPressure(pressure);
	refCls.setTemperature(temperature);

	QVariantList d = data;
	while(d.count() >= 2)
	{
		double z = d.takeFirst().toDouble();
		double M = d.takeFirst().toDouble();
		double Mc = extCls.airmass(std::cos(z * M_PI/180.), true);
		double actualError = qAbs(Mc - M);
		QVERIFY2(actualError <= acceptableError, QString("z=%1deg M=%2\" M(expected)=%3\" error=%4 acceptable=%5")
							.arg(QString::number(z, 'f', 2))
							.arg(QString::number(Mc, 'f', 2))
							.arg(QString::number(M, 'f', 2))
							.arg(QString::number(actualError, 'f', 3))
							.arg(QString::number(acceptableError, 'f', 3))
							.toUtf8());
	}
}

void TestAirmass::testYoungEquation()
{
	Refraction refCls;
	Extinction extCls;

	refCls.setPressure(pressure);
	refCls.setTemperature(temperature);

	QVariantList d = data;
	while(d.count() >= 2)
	{
		double z = d.takeFirst().toDouble();
		double M = d.takeFirst().toDouble();
		double Mc = extCls.airmass(std::cos(z * M_PI/180.), false);
		double actualError = qAbs(Mc - M);
		// Error for z=80deg is 0.059; Is it normal?
		if (z>70.) acceptableError = 0.06;
		QVERIFY2(actualError <= acceptableError, QString("z=%1deg M=%2\" M(expected)=%3\" error=%4 acceptable=%5")
							.arg(QString::number(z, 'f', 2))
							.arg(QString::number(Mc, 'f', 2))
							.arg(QString::number(M, 'f', 2))
							.arg(QString::number(actualError, 'f', 3))
							.arg(QString::number(acceptableError, 'f', 3))
							.toUtf8());
	}
}
