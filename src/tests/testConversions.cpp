/*
 * Stellarium
 * Copyright (C) 2013 Alexander Wolf
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

#include "tests/testConversions.hpp"

#include <QString>
#include <QDebug>
#include <QtGlobal>
#include <QVariantList>
#include <QMap>

#include "StelUtils.hpp"

#define ERROR_LIMIT 1e-6

QTEST_GUILESS_MAIN(TestConversions)

void TestConversions::testHMSToRad()
{
	QVariantList data;

	data << 0 << 0 << 0 << 0.;
	data << 1 << 0 << 0 << M_PI/12;
	data << 6 << 0 << 0 << M_PI/2;
	data << 12 << 0 << 0 << M_PI;
	data << 15 << 0 << 0 << 5*M_PI/4;
	data << 0 << 15 << 0 << M_PI/720;
	data << 0 << 0 << 15 << M_PI/43200;
	data << 2 << 15 << 45 << 269*M_PI/1600;
	data << 20 << 0 << 0 << 5*M_PI/3;
	data << 24 << 0 << 0 << 2*M_PI;
	data << 0 << 59 << 0 << 59*M_PI/10800;
	data << 0 << 0 << 59 << 59*M_PI/648000;
	data << 0 << 59 << 59 << 3599*M_PI/648000;
	data << 3 << 59 << 59 << 165599*M_PI/648000;

	while (data.count() >= 4)
	{
		int h, m, s;
		double rad;
		h = data.takeFirst().toInt();
		m = data.takeFirst().toInt();
		s = data.takeFirst().toInt();
		rad = data.takeFirst().toDouble();		
		QVERIFY2(qAbs(StelUtils::hmsToRad(h, m, s)-rad)<=ERROR_LIMIT, qPrintable(QString("%1h%2m%3s=%4").arg(h).arg(m).arg(s).arg(rad)));
	}
}

void TestConversions::testDMSToRad()
{
	QVariantList data;

	data << 0 << 0 << 0 << 0.;
	data << 30 << 0 << 0 << M_PI/6;
	data << 45 << 0 << 0 << M_PI/4;
	data << 60 << 0 << 0 << M_PI/3;
	data << 90 << 0 << 0 << M_PI/2;
	data << 120 << 0 << 0 << 2*M_PI/3;
	data << 180 << 0 << 0 << M_PI;
	data << 270 << 0 << 0 << 3*M_PI/2;
	data << 360 << 0 << 0 << 2*M_PI;
	data << 0 << 30 << 0 << M_PI/360;
	data << 0 << 45 << 0 << M_PI/240;
	data << 30 << 30 << 0 << 61*M_PI/360;
	data << 0 << 0 << 1 << M_PI/648000;
	data << 90 << 58 << 30 << 1213*M_PI/2400;
	data << 10 << 59 << 59 << 39599*M_PI/648000;

	while (data.count() >= 4)
	{
		int deg, min, sec;
		double rad;
		deg = data.takeFirst().toInt();
		min = data.takeFirst().toInt();
		sec = data.takeFirst().toInt();
		rad = data.takeFirst().toDouble();
		QVERIFY2(qAbs(StelUtils::dmsToRad(deg, min, sec)-rad)<=ERROR_LIMIT, qPrintable(QString("%1d%2m%3s=%4").arg(deg).arg(min).arg(sec).arg(rad)));
	}
}

void TestConversions::testRadToHMS()
{
	QVariantList data;

	data << 0. << 0 << 0 << 0.;
	data << M_PI/36 << 0 << 19 << 59.9;
	data << 7*M_PI/8 << 10 << 30 << 0.;
	data << 2*M_PI/5 << 4 << 48 << 0.;

	while (data.count()>=4)
	{
		double rad, s, so, t1, t2;
		int h, m;
		unsigned int ho, mo;
		rad = data.takeFirst().toDouble();
		h = data.takeFirst().toInt();
		m = data.takeFirst().toInt();
		s = data.takeFirst().toDouble();
		t1 = s+m*60+h*3600;
		StelUtils::radToHms(rad, ho, mo, so);
		t2 = so+mo*60+ho*3600;
		QVERIFY2(qAbs(t1-t2)<=0.1, qPrintable(QString("%1rad=%2h%3m%4s").arg(rad).arg(ho).arg(mo).arg(so)));
	}
}

void TestConversions::testRadToDMS()
{
	QVariantList data;

	data << 0. << 0 << 0 << 0.;
	data << M_PI/6 << 30 << 0 << 0.;
	data << M_PI/4 << 45 << 0 << 0.;
	data << M_PI/3 << 60 << 0 << 0.;
	data << M_PI/2 << 90 << 0 << 0.;
	data << 2*M_PI/3 << 120 << 0 << 0.;
	data << M_PI << 180 << 0 << 0.;
	data << 3*M_PI/2 << 270 << 0 << 0.;
	data << M_PI/360 << 0 << 30 << 0.;
	data << M_PI/240 << 0 << 45 << 0.;
	data << 61*M_PI/360 << 30 << 30 << 0.;
	data << M_PI/648000 << 0 << 0 << 1.;
	data << 1213*M_PI/2400 << 90 << 58 << 30.;
	data << 39599*M_PI/648000 << 10 << 59 << 59.;
	data << -M_PI/36 << -5 << 0 << 0.;
	data << -7*M_PI/8 << -157 << 30 << 0.;
	data << -2*M_PI/5 << -72 << 0 << 0.;
	data << -M_PI << -180 << 0 << 0.;
	data << -10*M_PI/648 << -2 << 46 << 40.0;

	while (data.count()>=4)
	{
		double rad, sec, seco, angle1, angle2;
		int deg, min;
		unsigned int dego, mino;
		bool sign;
		QString s;
		rad = data.takeFirst().toDouble();
		deg = data.takeFirst().toInt();
		min = data.takeFirst().toInt();
		sec = data.takeFirst().toDouble();
		if (deg>=0)
			angle1 = sec+min*60+deg*3600;
		else
			angle1 = -1*(sec+min*60+qAbs((double)deg)*3600);
		StelUtils::radToDms(rad, sign, dego, mino, seco);
		angle2 = seco+mino*60+dego*3600;
		if (!sign)
		{
			angle2 = -1*angle2;
			s = "-";
		}
		else
			s = "+";
		QVERIFY2(qAbs(angle1-angle2)<=ERROR_LIMIT, qPrintable(QString("%1rad=%2%3d%4m%5s").arg(rad).arg(s).arg(dego).arg(mino).arg(seco)));
	}
}

void TestConversions::testDDToDMS()
{
	QVariantList data;

	data << 0. << 0 << 0 << 0.;
	data << 10. << 10 << 0 << 0.;
	data << -13.5 << -13 << 30 << 0.;
	data << 30.263888889 << 30 << 15 << 50.;
	data << -90.1 << -90 << 6 << 0.;
	data << 128.9999 << 128 << 59 << 59.64;
	data << 360.6 << 360 << 36 << 0.;
	data << -180.786 << -180 << 47 << 9.6;
	data << -0.01 << -0 << 0 << 36.;
	data << -0.039 << -0 << 2 << 20.4;

	while (data.count()>=4)
	{
		double angle, sec, seco, angle1, angle2;
		int deg, min;
		unsigned int dego, mino;
		bool sign;
		QString s = "+";
		angle	= data.takeFirst().toDouble();
		deg	= data.takeFirst().toInt();
		min	= data.takeFirst().toInt();
		sec	= data.takeFirst().toDouble();
		if (angle>=0)
			angle1 = sec+min*60+deg*3600;
		else
			angle1 = -1*(sec+min*60+qAbs((double)deg)*3600);
		StelUtils::decDegToDms(angle, sign, dego, mino, seco);
		angle2 = seco+mino*60+dego*3600;
		if (!sign)
		{
			angle2 *= -1;
			s = "-";
		}
		QVERIFY2(qAbs(angle1-angle2)<=ERROR_LIMIT, qPrintable(QString("%1degrees=%2%3d%4m%5s").arg(angle).arg(s).arg(dego).arg(mino).arg(seco)));
	}
}

void TestConversions::testRadToDD()
{
	QVariantList data;

	data << 0.			<< 0.;
	data << M_PI/6			<< 30.;
	data << M_PI/4			<< 45.;
	data << M_PI/3			<< 60.;
	data << M_PI/2			<< 90.;
	data << 2*M_PI/3		<< 120.;
	data << M_PI			<< 180.;
	data << 3*M_PI/2		<< 270.;
	data << M_PI/360		<< 0.5;
	data << 61*M_PI/360	<< 30.5;
	data << -M_PI/36		<< -5.;
	data << -7*M_PI/8		<< -157.5;
	data << -2*M_PI/5		<< -72.;

	while (data.count()>=2)
	{
		double angle, decimalValue, degree;
		bool sign;
		angle		= data.takeFirst().toDouble();
		decimalValue	= data.takeFirst().toDouble();
		StelUtils::radToDecDeg(angle, sign, degree);
		if (!sign)
			decimalValue *= -1;

		QVERIFY2(qAbs(degree-decimalValue)<=ERROR_LIMIT, qPrintable(QString("%1 radians = %2 degrees (expected %3 degrees)")
									    .arg(QString::number(angle, 'f', 5))
									    .arg(QString::number(degree, 'f', 2))
									    .arg(QString::number(decimalValue, 'f', 2))));
	}
}

void TestConversions::testStringCoordinateToRad()
{
	QVariantList data;

	data << "+0d0m0s"	<< 0.;
	data << "+30d0m0s"	<< M_PI/6.;
	data << "+45d0m0s"	<< M_PI/4.;
	data << "+90d0m0s"	<< M_PI/2.;
	data << "+80d25m10s"	<< 1.404;
	data << "+400d0m5s"	<< 6.981;
	data << "-30d0m50s"	<< -0.5235;
	data << "123.567 N"	<< 2.1567;
	data << "123.567 S"	<< -2.1567;
	data << "123.567 W"	<< -2.1567;
	data << "+46d6'31\""	<< 0.8047;
	data << "12h0m0s"		<< M_PI;
	data << "6h0m0s"		<< M_PI/2.;
	data << "10h30m0s"	<< 2.749;

	while (data.count()>=2)
	{
		QString coordinate;
		double angle, expectedValue;

		coordinate	= data.takeFirst().toString();
		expectedValue	= data.takeFirst().toDouble();
		angle = StelUtils::getDecAngle(coordinate);

		QVERIFY2(qAbs(angle-expectedValue)<=1e-3, qPrintable(QString("%1 = %2 radians (expected %3 radians)")
									    .arg(coordinate)
									    .arg(QString::number(angle, 'f', 5))
									    .arg(QString::number(expectedValue, 'f', 5))));
	}
}

void TestConversions::testHMSToHours()
{
	QVariantList data;

	data << 0	<< 0	<< 0	<< 0.;
	data << 1	<< 0	<< 0	<< 1.;
	data << 6	<< 0	<< 0	<< 6.;
	data << 12	<< 0	<< 0	<< 12.;
	data << 15	<< 0	<< 0	<< 15.;
	data << 0	<< 15	<< 0	<< 0.25;
	data << 0	<< 0	<< 15	<< 0.004167;
	data << 2	<< 15	<< 45	<< 2.2625;
	data << 20	<< 0	<< 0	<< 20.;
	data << 24	<< 0	<< 0	<< 24.;
	data << 0	<< 59	<< 0	<< 0.983333;
	data << 0	<< 0	<< 59	<< 0.016389;
	data << 0	<< 59	<< 59	<< 0.999722;
	data << 3	<< 59	<< 59	<< 3.999722;

	while (data.count() >= 4)
	{
		int h, m, s;
		double expectedHours;
		h = data.takeFirst().toInt();
		m = data.takeFirst().toInt();
		s = data.takeFirst().toInt();
		expectedHours = data.takeFirst().toDouble();
		double hours = StelUtils::hmsToHours(h, m, s);
		QVERIFY2(qAbs(hours-expectedHours)<=ERROR_LIMIT, qPrintable(QString("%1h%2m%3s = %4h (expected %5h)")
									    .arg(h)
									    .arg(m)
									    .arg(s)
									    .arg(QString::number(hours, 'f', 6))
									    .arg(QString::number(expectedHours, 'f', 6))));
	}
}

void TestConversions::testHMSStringToHours()
{
	QVariantList data;

	data << "0h0m0s"		<< 0.;
	data << "1h0m0s"		<< 1.;
	data << "6h0m0s"		<< 6.;
	data << "12h0m0s"		<< 12.;
	data << "15h0m0s"		<< 15.;
	data << "0h15m0s"		<< 0.25;
	data << "0h0m15s"		<< 0.004167;
	data << "2h15m45s"	<< 2.2625;
	data << "20h0m0s"		<< 20.;
	data << "24h0m0s"		<< 24.;
	data << "0h59m0s"		<< 0.983333;
	data << "0h0m59s"		<< 0.016389;
	data << "0h59m59s"	<< 0.999722;
	data << "3h59m59s"	<< 3.999722;

	while (data.count() >= 2)
	{
		QString hms;
		double expectedHours;
		hms = data.takeFirst().toString();
		expectedHours = data.takeFirst().toDouble();
		double hours = StelUtils::hmsStrToHours(hms);
		QVERIFY2(qAbs(hours-expectedHours)<=ERROR_LIMIT, qPrintable(QString("%1 = %2h (expected %3h)")
									    .arg(hms)
									    .arg(QString::number(hours, 'f', 6))
									    .arg(QString::number(expectedHours, 'f', 6))));
	}
}

void TestConversions::testHoursToHMSStr()
{
	QVariantList data;

	data << "0h00m00.0s"	<< 0.;
	data << "1h00m00.0s"	<< 1.;
	data << "6h00m00.0s"	<< 6.;
	data << "12h00m00.0s"	<< 12.;
	data << "15h00m00.0s"	<< 15.;
	data << "0h15m00.0s"	<< 0.25;
	data << "0h00m15.0s"	<< 0.004167;
	data << "2h15m45.0s"	<< 2.2625;
	data << "20h00m00.0s"	<< 20.;
	data << "24h00m00.0s"	<< 24.;
	data << "0h59m00.0s"	<< 0.983333;
	data << "0h00m59.0s"	<< 0.016389;
	data << "0h59m59.0s"	<< 0.999722;
	data << "3h59m59.0s"	<< 3.999722;

	while (data.count() >= 2)
	{
		QString expectedHMS = data.takeFirst().toString();
		double hours = data.takeFirst().toDouble();
		QString hms = StelUtils::hoursToHmsStr(hours);
		QVERIFY2(expectedHMS==hms, qPrintable(QString("%1h = %2 (expected %3)")
									    .arg(QString::number(hours, 'f', 6))
									    .arg(hms)
									    .arg(expectedHMS)));
	}
}

void TestConversions::testRadToHMSStr()
{
	QVariantList data;

	data << 0.		<< "0h00m00.0s";
	data << M_PI/36	<< "0h20m00.0s";
	data << 7*M_PI/8	<< "10h30m00.0s";
	data << 2*M_PI/5	<< "4h48m00.0s";

	while (data.count()>=2)
	{
		double rad = data.takeFirst().toDouble();
		QString expectedHMS = data.takeFirst().toString();
		QString hms = StelUtils::radToHmsStr(rad).trimmed();
		QVERIFY2(expectedHMS==hms, qPrintable(QString("%1 radians = %2 (expected %3)")
						      .arg(rad)
						      .arg(hms)
						      .arg(expectedHMS)));
	}
}
