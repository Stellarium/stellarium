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
	data << 1 << 0 << 0 << M_PI/12.;
	data << 1 << 12 << 0 << M_PI/10.;
	data << 6 << 0 << 0 << M_PI_2;
	data << 12 << 0 << 0 << M_PI;
	data << 15 << 0 << 0 << 5.*M_PI_4;
	data << 0 << 15 << 0 << M_PI/48.;
	data << 0 << 0 << 15 << M_PI/2880.;
	data << 2 << 15 << 45 << 181.*M_PI/960.;
	data << 20 << 0 << 0 << 5.*M_PI/3.;
	data << 24 << 0 << 0 << 2.*M_PI;
	data << 0 << 59 << 0 << 59.*M_PI/720.;
	data << 0 << 0 << 59 << 59.*M_PI/43200.;
	data << 0 << 59 << 59 << 3599.*M_PI/43200.;
	data << 3 << 59 << 59 << 14399.*M_PI/43200.;

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
	data << 45 << 0 << 0 << M_PI_4;
	data << 60 << 0 << 0 << M_PI/3;
	data << 90 << 0 << 0 << M_PI_2;
	data << 120 << 0 << 0 << 2*M_PI/3;
	data << 180 << 0 << 0 << M_PI;
	data << 270 << 0 << 0 << 3*M_PI_2;
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

void TestConversions::testDMSStrToRad()
{
	QVariantList data;

	data << "+0d0'0\""	<< 0.;
	data << "+30d0'0\""	<< M_PI/6;
	data << "+45d0'0\""	<< M_PI_4;
	data << "+60d0'0\""	<< M_PI/3;
	data << "+90d0'0\""	<< M_PI_2;
	data << "+120d0'0\""	<< 2*M_PI/3;
	data << "+180d0'0\""	<< M_PI;
	data << "+270d0'0\""	<< 3*M_PI_2;
	data << "+360d0'0\""	<< 2*M_PI;
	data << "+0d30'0\""	<< M_PI/360;
	data << "+0d45'0\""	<< M_PI/240;
	data << "+30d30'0\""	<< 61*M_PI/360;
	data << "+0d0'1\""	<< M_PI/648000;
	data << "+90d58'30\""	<< 1213*M_PI/2400;
	data << "+10d59'59\""	<< 39599*M_PI/648000;
	data << "-120d0'0\""	<< -2*M_PI/3;
	data << "-180d0'0\""	<< -M_PI;
	data << "-270d0'0\""	<< -3*M_PI_2;
	data << "-0d30'0\""	<< -M_PI/360;
	data << "-0d45'0\""	<< -M_PI/240;

	while (data.count() >= 2)
	{
		QString DMSStr = data.takeFirst().toString();
		double radians = data.takeFirst().toDouble();
		double dms = StelUtils::dmsStrToRad(DMSStr);

		QVERIFY2(qAbs(dms-radians)<=ERROR_LIMIT, qPrintable(QString("%1 = %2 radians (expected %3 radians)")
								    .arg(DMSStr)
								    .arg(QString::number(dms, 'f', 5))
								    .arg(QString::number(radians, 'f', 5))));
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

void TestConversions::testRadToHMSStrAdapt()
{
	QVariantList data;

	data << 0.		<< "0h";
	data << M_PI/36		<< "0h20m";
	data << 7*M_PI/8	<< "10h30m";
	data << 2*M_PI/5	<< "4h48m";

	while (data.count()>=2)
	{
		double rad	= data.takeFirst().toDouble();
		QString ehms	= data.takeFirst().toString();
		QString rhms	= StelUtils::radToHmsStrAdapt(rad);
		QVERIFY2(rhms==ehms, qPrintable(QString("%1 radians = %2 (expected %3)")
					    .arg(QString::number(rad, 'f', 5))
					    .arg(rhms)
					    .arg(ehms)));
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

void TestConversions::testRadToDMSStrAdapt()
{
	QVariantList data;

	data << 0.			<< "+0°"		<< false;
	data << M_PI/6			<< "+30°"		<< false;
	data << M_PI/4			<< "+45°"		<< false;
	data << M_PI/3			<< "+60°"		<< false;
	data << M_PI/2			<< "+90°"		<< false;
	data << 2*M_PI/3		<< "+120°"		<< false;
	data << M_PI			<< "+180°"		<< false;
	data << 3*M_PI/2		<< "+270°"		<< false;
	data << M_PI/360		<< "+0°30'"		<< false;
	data << M_PI/240		<< "+0°45'"		<< false;
	data << 61*M_PI/360		<< "+30°30'"		<< false;
	data << M_PI/648000		<< "+0°0'1\""		<< false;
	data << 1213*M_PI/2400		<< "+90°58'30\""	<< false;
	data << 39599*M_PI/648000	<< "+10°59'59\""	<< false;
	data << -M_PI/36		<< "-5°"		<< false;
	data << -7*M_PI/8		<< "-157°30'"		<< false;
	data << -2*M_PI/5		<< "-72°"		<< false;
	data << -M_PI			<< "-180°"		<< false;
	data << -10*M_PI/648		<< "-2°46'40\""		<< false;

	data << 0.			<< "+0d"		<< true;
	data << M_PI/6			<< "+30d"		<< true;
	data << M_PI/4			<< "+45d"		<< true;
	data << M_PI/3			<< "+60d"		<< true;
	data << M_PI/2			<< "+90d"		<< true;
	data << 2*M_PI/3		<< "+120d"		<< true;
	data << M_PI			<< "+180d"		<< true;
	data << 3*M_PI/2		<< "+270d"		<< true;
	data << M_PI/360		<< "+0d30'"		<< true;
	data << M_PI/240		<< "+0d45'"		<< true;
	data << 61*M_PI/360		<< "+30d30'"		<< true;
	data << M_PI/648000		<< "+0d0'1\""		<< true;
	data << 1213*M_PI/2400		<< "+90d58'30\""	<< true;
	data << 39599*M_PI/648000	<< "+10d59'59\""	<< true;
	data << -M_PI/36		<< "-5d"		<< true;
	data << -7*M_PI/8		<< "-157d30'"		<< true;
	data << -2*M_PI/5		<< "-72d"		<< true;
	data << -M_PI			<< "-180d"		<< true;
	data << -10*M_PI/648		<< "-2d46'40\""		<< true;

	while (data.count()>=3)
	{
		double rad	= data.takeFirst().toDouble();
		QString edms	= data.takeFirst().toString();
		bool flag	= data.takeFirst().toBool();
		QString rdms	= StelUtils::radToDmsStrAdapt(rad, flag);
		QVERIFY2(rdms==edms, qPrintable(QString("%1 radians = %2 (expected %3) [flag: %4]")
						.arg(QString::number(rad, 'f', 5))
						.arg(rdms)
						.arg(edms)
						.arg(flag)));
	}
}

void TestConversions::testRadToDMSStr()
{
	QVariantList data;

	data << 0.			<<   "+0°00'00.0\""	<< true		<< false;
	data << M_PI/6			<<  "+30°00'00.0\""	<< true		<< false;
	data << M_PI/4			<<  "+45°00'00.0\""	<< true		<< false;
	data << M_PI/3			<<  "+60°00'00.0\""	<< true		<< false;
	data << M_PI/2			<<  "+90°00'00.0\""	<< true		<< false;
	data << 2*M_PI/3		<< "+120°00'00.0\""	<< true		<< false;
	data << M_PI			<< "+180°00'00.0\""	<< true		<< false;
	data << 3*M_PI/2		<< "+270°00'00.0\""	<< true		<< false;
	data << M_PI/360		<<   "+0°30'00.0\""	<< true		<< false;
	data << M_PI/240		<<   "+0°45'00.0\""	<< true		<< false;
	data << 61*M_PI/360		<<  "+30°30'00.0\""	<< true		<< false;
	data << M_PI/648000		<<   "+0°00'01.0\""	<< true		<< false;
	data << 1213*M_PI/2400		<<  "+90°58'30.0\""	<< true		<< false;
	data << 39599*M_PI/648000	<<  "+10°59'59.0\""	<< true		<< false;
	data << -M_PI/36		<<   "-5°00'00.0\""	<< true		<< false;
	data << -7*M_PI/8		<< "-157°30'00.0\""	<< true		<< false;
	data << -2*M_PI/5		<<  "-72°00'00.0\""	<< true		<< false;
	data << -M_PI			<< "-180°00'00.0\""	<< true		<< false;
	data << -10*M_PI/648		<<   "-2°46'40.0\""	<< true		<< false;

	data << 0.			<<   "+0d00'00.0\""	<< true		<< true;
	data << M_PI/6			<<  "+30d00'00.0\""	<< true		<< true;
	data << M_PI/4			<<  "+45d00'00.0\""	<< true		<< true;
	data << M_PI/3			<<  "+60d00'00.0\""	<< true		<< true;
	data << M_PI/2			<<  "+90d00'00.0\""	<< true		<< true;
	data << 2*M_PI/3		<< "+120d00'00.0\""	<< true		<< true;
	data << M_PI			<< "+180d00'00.0\""	<< true		<< true;
	data << 3*M_PI/2		<< "+270d00'00.0\""	<< true		<< true;
	data << M_PI/360		<<   "+0d30'00.0\""	<< true		<< true;
	data << M_PI/240		<<   "+0d45'00.0\""	<< true		<< true;
	data << 61*M_PI/360		<<  "+30d30'00.0\""	<< true		<< true;
	data << M_PI/648000		<<   "+0d00'01.0\""	<< true		<< true;
	data << 1213*M_PI/2400		<<  "+90d58'30.0\""	<< true		<< true;
	data << 39599*M_PI/648000	<<  "+10d59'59.0\""	<< true		<< true;
	data << -M_PI/36		<<   "-5d00'00.0\""	<< true		<< true;
	data << -7*M_PI/8		<< "-157d30'00.0\""	<< true		<< true;
	data << -2*M_PI/5		<<  "-72d00'00.0\""	<< true		<< true;
	data << -M_PI			<< "-180d00'00.0\""	<< true		<< true;
	data << -10*M_PI/648		<<   "-2d46'40.0\""	<< true		<< true;

	data << 0.			<<   "+0°00'00\""	<< false	<< false;
	data << M_PI/6			<<  "+30°00'00\""	<< false	<< false;
	data << M_PI/4			<<  "+45°00'00\""	<< false	<< false;
	data << M_PI/3			<<  "+60°00'00\""	<< false	<< false;
	data << M_PI/2			<<  "+90°00'00\""	<< false	<< false;
	data << 2*M_PI/3		<< "+120°00'00\""	<< false	<< false;
	data << M_PI			<< "+180°00'00\""	<< false	<< false;
	data << 3*M_PI/2		<< "+270°00'00\""	<< false	<< false;
	data << M_PI/360		<<   "+0°30'00\""	<< false	<< false;
	data << M_PI/240		<<   "+0°45'00\""	<< false	<< false;
	data << 61*M_PI/360		<<  "+30°30'00\""	<< false	<< false;
	data << M_PI/648000		<<   "+0°00'01\""	<< false	<< false;
	data << 1213*M_PI/2400		<<  "+90°58'30\""	<< false	<< false;
	data << 39599*M_PI/648000	<<  "+10°59'59\""	<< false	<< false;
	data << -M_PI/36		<<   "-5°00'00\""	<< false	<< false;
	data << -7*M_PI/8		<< "-157°30'00\""	<< false	<< false;
	data << -2*M_PI/5		<<  "-72°00'00\""	<< false	<< false;
	data << -M_PI			<< "-180°00'00\""	<< false	<< false;
	data << -10*M_PI/648		<<   "-2°46'40\""	<< false	<< false;

	data << 0.			<<   "+0d00'00\""	<< false	<< true;
	data << M_PI/6			<<  "+30d00'00\""	<< false	<< true;
	data << M_PI/4			<<  "+45d00'00\""	<< false	<< true;
	data << M_PI/3			<<  "+60d00'00\""	<< false	<< true;
	data << M_PI/2			<<  "+90d00'00\""	<< false	<< true;
	data << 2*M_PI/3		<< "+120d00'00\""	<< false	<< true;
	data << M_PI			<< "+180d00'00\""	<< false	<< true;
	data << 3*M_PI/2		<< "+270d00'00\""	<< false	<< true;
	data << M_PI/360		<<   "+0d30'00\""	<< false	<< true;
	data << M_PI/240		<<   "+0d45'00\""	<< false	<< true;
	data << 61*M_PI/360		<<  "+30d30'00\""	<< false	<< true;
	data << M_PI/648000		<<   "+0d00'01\""	<< false	<< true;
	data << 1213*M_PI/2400		<<  "+90d58'30\""	<< false	<< true;
	data << 39599*M_PI/648000	<<  "+10d59'59\""	<< false	<< true;
	data << -M_PI/36		<<   "-5d00'00\""	<< false	<< true;
	data << -7*M_PI/8		<< "-157d30'00\""	<< false	<< true;
	data << -2*M_PI/5		<<  "-72d00'00\""	<< false	<< true;
	data << -M_PI			<< "-180d00'00\""	<< false	<< true;
	data << -10*M_PI/648		<<   "-2d46'40\""	<< false	<< true;

	while (data.count()>=4)
	{
		double rad	= data.takeFirst().toDouble();
		QString edms	= data.takeFirst().toString();
		bool decimalF	= data.takeFirst().toBool();
		bool useDF	= data.takeFirst().toBool();
		QString rdms	= StelUtils::radToDmsStr(rad, decimalF, useDF);
		QVERIFY2(rdms==edms, qPrintable(QString("%1 radians = %2 (expected %3) [flags: %4, %5]")
						.arg(QString::number(rad, 'f', 5))
						.arg(rdms)
						.arg(edms)
						.arg(decimalF)
						.arg(useDF)));
	}
}

void TestConversions::testRadToDMSPStr()
{
	QVariantList data;

	data << 0.			<< "+0°00'00.00\""	<< 2	<< false;
	data << M_PI/6			<< "+30°00'00.00\""	<< 2	<< false;
	data << M_PI/4			<< "+45°00'00.00\""	<< 2	<< false;
	data << M_PI/3			<< "+60°00'00.00\""	<< 2	<< false;
	data << M_PI/2			<< "+90°00'00.00\""	<< 2	<< false;
	data << 2*M_PI/3		<< "+120°00'00.00\""	<< 2	<< false;
	data << M_PI			<< "+180°00'00.00\""	<< 2	<< false;
	data << 3*M_PI/2		<< "+270°00'00.00\""	<< 2	<< false;
	data << M_PI/360		<< "+0°30'00.00\""	<< 2	<< false;
	data << M_PI/240		<< "+0°45'00.00\""	<< 2	<< false;
	data << 61*M_PI/360		<< "+30°30'00.00\""	<< 2	<< false;
	data << M_PI/648000		<< "+0°00'01.00\""	<< 2	<< false;
	data << 1213*M_PI/2400		<< "+90°58'30.00\""	<< 2	<< false;
	data << 39599*M_PI/648000	<< "+10°59'59.00\""	<< 2	<< false;
	data << -M_PI/36		<< "-5°00'00.00\""	<< 2	<< false;
	data << -7*M_PI/8		<< "-157°30'00.00\""	<< 2	<< false;
	data << -2*M_PI/5		<< "-72°00'00.00\""	<< 2	<< false;
	data << -M_PI			<< "-180°00'00.00\""	<< 2	<< false;
	data << -10*M_PI/648		<< "-2°46'40.00\""	<< 2	<< false;

	data << 0.			<< "+0d00'00.00\""	<< 2	<< true;
	data << M_PI/6			<< "+30d00'00.00\""	<< 2	<< true;
	data << M_PI/4			<< "+45d00'00.00\""	<< 2	<< true;
	data << M_PI/3			<< "+60d00'00.00\""	<< 2	<< true;
	data << M_PI/2			<< "+90d00'00.00\""	<< 2	<< true;
	data << 2*M_PI/3		<< "+120d00'00.00\""	<< 2	<< true;
	data << M_PI			<< "+180d00'00.00\""	<< 2	<< true;
	data << 3*M_PI/2		<< "+270d00'00.00\""	<< 2	<< true;
	data << M_PI/360		<< "+0d30'00.00\""	<< 2	<< true;
	data << M_PI/240		<< "+0d45'00.00\""	<< 2	<< true;
	data << 61*M_PI/360		<< "+30d30'00.00\""	<< 2	<< true;
	data << M_PI/648000		<< "+0d00'01.00\""	<< 2	<< true;
	data << 1213*M_PI/2400		<< "+90d58'30.00\""	<< 2	<< true;
	data << 39599*M_PI/648000	<< "+10d59'59.00\""	<< 2	<< true;
	data << -M_PI/36		<< "-5d00'00.00\""	<< 2	<< true;
	data << -7*M_PI/8		<< "-157d30'00.00\""	<< 2	<< true;
	data << -2*M_PI/5		<< "-72d00'00.00\""	<< 2	<< true;
	data << -M_PI			<< "-180d00'00.00\""	<< 2	<< true;
	data << -10*M_PI/648		<< "-2d46'40.00\""	<< 2	<< true;

	data << 0.			<< "+0°00'00.000\""	<< 3	<< false;
	data << M_PI/6			<< "+30°00'00.000\""	<< 3	<< false;
	data << M_PI/4			<< "+45°00'00.000\""	<< 3	<< false;
	data << M_PI/3			<< "+60°00'00.000\""	<< 3	<< false;
	data << M_PI/2			<< "+90°00'00.000\""	<< 3	<< false;
	data << 2*M_PI/3		<< "+120°00'00.000\""	<< 3	<< false;
	data << M_PI			<< "+180°00'00.000\""	<< 3	<< false;
	data << 3*M_PI/2		<< "+270°00'00.000\""	<< 3	<< false;
	data << M_PI/360		<< "+0°30'00.000\""	<< 3	<< false;
	data << M_PI/240		<< "+0°45'00.000\""	<< 3	<< false;
	data << 61*M_PI/360		<< "+30°30'00.000\""	<< 3	<< false;
	data << M_PI/648000		<< "+0°00'01.000\""	<< 3	<< false;
	data << 1213*M_PI/2400		<< "+90°58'30.000\""	<< 3	<< false;
	data << 39599*M_PI/648000	<< "+10°59'59.000\""	<< 3	<< false;
	data << -M_PI/36		<< "-5°00'00.000\""	<< 3	<< false;
	data << -7*M_PI/8		<< "-157°30'00.000\""	<< 3	<< false;
	data << -2*M_PI/5		<< "-72°00'00.000\""	<< 3	<< false;
	data << -M_PI			<< "-180°00'00.000\""	<< 3	<< false;
	data << -10*M_PI/648		<< "-2°46'40.000\""	<< 3	<< false;

	data << 0.			<< "+0d00'00.000\""	<< 3	<< true;
	data << M_PI/6			<< "+30d00'00.000\""	<< 3	<< true;
	data << M_PI/4			<< "+45d00'00.000\""	<< 3	<< true;
	data << M_PI/3			<< "+60d00'00.000\""	<< 3	<< true;
	data << M_PI/2			<< "+90d00'00.000\""	<< 3	<< true;
	data << 2*M_PI/3		<< "+120d00'00.000\""	<< 3	<< true;
	data << M_PI			<< "+180d00'00.000\""	<< 3	<< true;
	data << 3*M_PI/2		<< "+270d00'00.000\""	<< 3	<< true;
	data << M_PI/360		<< "+0d30'00.000\""	<< 3	<< true;
	data << M_PI/240		<< "+0d45'00.000\""	<< 3	<< true;
	data << 61*M_PI/360		<< "+30d30'00.000\""	<< 3	<< true;
	data << M_PI/648000		<< "+0d00'01.000\""	<< 3	<< true;
	data << 1213*M_PI/2400		<< "+90d58'30.000\""	<< 3	<< true;
	data << 39599*M_PI/648000	<< "+10d59'59.000\""	<< 3	<< true;
	data << -M_PI/36		<< "-5d00'00.000\""	<< 3	<< true;
	data << -7*M_PI/8		<< "-157d30'00.000\""	<< 3	<< true;
	data << -2*M_PI/5		<< "-72d00'00.000\""	<< 3	<< true;
	data << -M_PI			<< "-180d00'00.000\""	<< 3	<< true;
	data << -10*M_PI/648		<< "-2d46'40.000\""	<< 3	<< true;

	while (data.count()>=4)
	{
		double rad	= data.takeFirst().toDouble();
		QString edms	= data.takeFirst().toString();
		int precission	= data.takeFirst().toInt();
		bool flag	= data.takeFirst().toBool();
		QString rdms	= StelUtils::radToDmsPStr(rad, precission, flag);
		QVERIFY2(rdms==edms, qPrintable(QString("%1 radians = %2 (expected %3) [precission: %4; flag: %5]")
						.arg(QString::number(rad, 'f', 5))
						.arg(rdms)
						.arg(edms)
						.arg(precission)
						.arg(flag)));
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

void TestConversions::testDDToDMSStr()
{
	QVariantList data;

	data << 0.	<< "+0°00'00\"";
	data << 10.	<< "+10°00'00\"";
	data << -13.5	<< "-13°30'00\"";
	data << -90.1	<< "-90°06'00\"";
	data << 360.6	<< "+360°36'00\"";
	data << -0.01	<< "-0°00'36\"";

	while (data.count()>=2)
	{
		double angle	= data.takeFirst().toDouble();
		QString expDMS	= data.takeFirst().toString();
		QString DMS	= StelUtils::decDegToDmsStr(angle);

		QVERIFY2(DMS==expDMS, qPrintable(QString("%1 degrees = %2 (expected %3)")
						 .arg(QString::number(angle, 'f', 5))
						 .arg(DMS)
						 .arg(expDMS)));
	}
}

void TestConversions::testRadToDD()
{
	QVariantList data;

	data << 0.		<< 0.;
	data << M_PI/6		<< 30.;
	data << M_PI/4		<< 45.;
	data << M_PI/3		<< 60.;
	data << M_PI/2		<< 90.;
	data << 2*M_PI/3	<< 120.;
	data << M_PI		<< 180.;
	data << 3*M_PI/2	<< 270.;
	data << M_PI/360	<< 0.5;
	data << 61*M_PI/360	<< 30.5;
	data << -M_PI/36	<< -5.;
	data << -7*M_PI/8	<< -157.5;
	data << -2*M_PI/5	<< -72.;

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
	data << "12h0m0s"	        << M_PI;
	data << "6h0m0s"	        << M_PI/2.;
	data << "10h30m0s"	<< 2.749;
	data << "+80°25'10\""	<< 1.404;
	data << "-45d0m0s"	<< -M_PI/4.;
	data << "-80°25'10\""	<< -1.404;
	data << "-80r25m10s"	<< -0.0;

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

	data << "0h0m0s"	<< 0.;
	data << "0h"		<< 0.;
	data << "1h0m0s"	<< 1.;
	data << "6h0m0s"	<< 6.;
	data << "12h0m0s"	<< 12.;
	data << "15h0m0s"	<< 15.;
	data << "0h15m0s"	<< 0.25;
	data << "0h0m15s"	<< 0.004167;
	data << "2h15m45s"	<< 2.2625;
	data << "20h0m0s"	<< 20.;
	data << "24h0m0s"	<< 24.;
	data << "0h59m0s"	<< 0.983333;
	data << "0h0m59s"	<< 0.016389;
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
	data << M_PI/36		<< "0h20m00.0s";
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

void TestConversions::testRadToDecDegStr()
{
	QVariantList data;

	data << 0.		<< "+0.00d";
	data << M_PI/6		<< "+30.00d";
	data << M_PI/4		<< "+45.00d";
	data << M_PI/3		<< "+60.00d";
	data << M_PI/2		<< "+90.00d";
	data << 2*M_PI/3	<< "+120.00d";
	data << M_PI		<< "+180.00d";
	data << 3*M_PI/2	<< "+270.00d";
	data << M_PI/360	<< "+0.50d";
	data << 61*M_PI/360	<< "+30.50d";
	data << -M_PI/36	<< "+355.00d";
	data << -7*M_PI/8	<< "+202.50d";
	data << -2*M_PI/5	<< "+288.00d";

	while (data.count()>=2)
	{
		double angle		= data.takeFirst().toDouble();
		QString expectedValue	= data.takeFirst().toString();
		QString degree = StelUtils::radToDecDegStr(angle, 2, true, true);

		QVERIFY2(degree==expectedValue, qPrintable(QString("%1 radians = %2 (expected %3)")
							   .arg(QString::number(angle, 'f', 5))
							   .arg(degree)
							   .arg(expectedValue)));
	}
}

void TestConversions::testVec3fToHtmlColor()
{
	QVariantList data;

	data << "#FFFFFF" << 1.f << 1.f << 1.f;
	data << "#FF0000" << 1.f << 0.f << 0.f;
	data << "#00FF00" << 0.f << 1.f << 0.f;
	data << "#0000FF" << 0.f << 0.f << 1.f;
	data << "#999999" << .6f << .6f << .6f;
	data << "#666666" << .4f << .4f << .4f;
	data << "#000000" << 0.f << 0.f << 0.f;
	data << "#000000" << 0.f << 0.f << 0.f;

	while (data.count()>=4)
	{
		QString color	= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		float v3	= data.takeFirst().toFloat();
		Vec3f srcColor	= Vec3f(v1, v2, v3);
		QString cColor	= StelUtils::vec3fToHtmlColor(srcColor).toUpper();

		QVERIFY2(cColor==color, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(srcColor.toString())
							   .arg(cColor)
							   .arg(color)));
	}
}

void TestConversions::testHtmlColorToVec3f()
{
	QVariantList data;

	data << "#FFFFFF" << 1.f << 1.f << 1.f;
	data << "#FF0000" << 1.f << 0.f << 0.f;
	data << "#00FF00" << 0.f << 1.f << 0.f;
	data << "#0000FF" << 0.f << 0.f << 1.f;
	data << "#999999" << .6f << .6f << .6f;
	data << "#666666" << .4f << .4f << .4f;
	data << "#000"    << 0.f << 0.f << 0.f;

	while (data.count()>=4)
	{
		QString color	= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		float v3	= data.takeFirst().toFloat();
		Vec3f expected	= Vec3f(v1, v2, v3);
		Vec3f v3fcolor	= StelUtils::htmlColorToVec3f(color);

		QVERIFY2(v3fcolor==expected, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(color)
							   .arg(v3fcolor.toString())
							   .arg(expected.toString())));
	}
}

void TestConversions::testStrToVec2f()
{
	QVariantList data;

	data << "1,1" << 1.f << 1.f;
	data << "1,0" << 1.f << 0.f;
	data << "0,1" << 0.f << 1.f;
	data << "0,0" << 0.f << 0.f;
	data << "0"    << 0.f << 0.f;

	while (data.count()>=3)
	{
		QString vec	= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		Vec2f srcVec	= Vec2f(v1, v2);
		Vec2f dstVec	= StelUtils::strToVec2f(vec);

		QVERIFY2(srcVec==dstVec, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(vec)
							   .arg(dstVec.toString())
							   .arg(srcVec.toString())));
	}
}

void TestConversions::testVec2fToStr()
{
	QVariantList data;

	data << "1.000000,1.000000" << 1.f << 1.f;
	data << "1.000000,0.000000" << 1.f << 0.f;
	data << "0.000000,1.000000" << 0.f << 1.f;
	data << "0.000000,0.000000" << 0.f << 0.f;

	while (data.count()>=3)
	{
		QString vec	= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		Vec2f srcVec	= Vec2f(v1, v2);
		QString dstVec	= StelUtils::vec2fToStr(srcVec);

		QVERIFY2(vec==dstVec, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(srcVec.toString())
							   .arg(dstVec)
							   .arg(vec)));
	}
}

void TestConversions::testStrToVec3f()
{
	QVariantList data;

	data << "1,1,1" << 1.f << 1.f << 1.f;
	data << "1,0,1" << 1.f << 0.f << 1.f;
	data << "0,1,0" << 0.f << 1.f << 0.f;
	data << "0,0,0" << 0.f << 0.f << 0.f;
	data << "0,0"    << 0.f << 0.f << 0.f;
	data << "0"       << 0.f << 0.f << 0.f;

	while (data.count()>=4)
	{
		QString vec	= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		float v3	= data.takeFirst().toFloat();
		Vec3f srcVec	= Vec3f(v1, v2, v3);
		Vec3f dstVec	= StelUtils::strToVec3f(vec);

		QVERIFY2(srcVec==dstVec, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(vec)
							   .arg(dstVec.toString())
							   .arg(srcVec.toString())));
	}
}

void TestConversions::testVec3fToStr()
{
	QVariantList data;

	data << "1.000000,1.000000,1.000000" << 1.f << 1.f << 1.f;
	data << "1.000000,0.000000,1.000000" << 1.f << 0.f << 1.f;
	data << "0.000000,1.000000,0.000000" << 0.f << 1.f << 0.f;
	data << "0.000000,0.000000,0.000000" << 0.f << 0.f << 0.f;

	while (data.count()>=4)
	{
		QString vec	= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		float v3	= data.takeFirst().toFloat();
		Vec3f srcVec	= Vec3f(v1, v2, v3);
		QString dstVec	= StelUtils::vec3fToStr(srcVec);

		QVERIFY2(vec==dstVec, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(srcVec.toString())
							   .arg(dstVec)
							   .arg(vec)));
	}
}

void TestConversions::testStrToVec4d()
{
	QVariantList data;

	data << "1,1,1,1" << 1. << 1. << 1. << 1.;
	data << "1,0,1,0" << 1. << 0. << 1. << 0.;
	data << "0,1,0,1" << 0. << 1. << 0. << 1.;
	data << "0,0,0,0" << 0. << 0. << 0. << 0.;
	data << "0,0,0"    << 0. << 0. << 0. << 0.;
	data << "0,0"       << 0. << 0. << 0. << 0.;
	data << "0"          << 0. << 0. << 0. << 0.;

	while (data.count()>=5)
	{
		QString vec	= data.takeFirst().toString();
		double v1	= data.takeFirst().toDouble();
		double v2	= data.takeFirst().toDouble();
		double v3	= data.takeFirst().toDouble();
		double v4	= data.takeFirst().toDouble();
		Vec4d srcVec	= Vec4d(v1, v2, v3, v4);
		Vec4d dstVec	= StelUtils::strToVec4d(vec);

		QVERIFY2(srcVec==dstVec, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(vec)
							   .arg(dstVec.toString())
							   .arg(srcVec.toString())));
	}
}

void TestConversions::testVec4dToStr()
{
	QVariantList data;

	data << "1.0000000000,1.0000000000,1.0000000000,1.0000000000" << 1. << 1. << 1. << 1.;
	data << "1.0000000000,0.0000000000,1.0000000000,0.0000000000" << 1. << 0. << 1. << 0.;
	data << "0.0000000000,1.0000000000,0.0000000000,1.0000000000" << 0. << 1. << 0. << 1.;
	data << "0.0000000000,0.0000000000,0.0000000000,0.0000000000" << 0. << 0. << 0. << 0.;

	while (data.count()>=5)
	{
		QString vec	= data.takeFirst().toString();
		double v1	= data.takeFirst().toDouble();
		double v2	= data.takeFirst().toDouble();
		double v3	= data.takeFirst().toDouble();
		double v4	= data.takeFirst().toDouble();
		Vec4d srcVec	= Vec4d(v1, v2, v3, v4);
		QString dstVec	= StelUtils::vec4dToStr(srcVec);

		QVERIFY2(vec==dstVec, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(srcVec.toString())
							   .arg(dstVec)
							   .arg(vec)));
	}
}

void TestConversions::testQDateTimeToJD()
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

	 for (QMap<double, QString>::ConstIterator i=map.constBegin();i!=map.constEnd();++i)
	 {
		 qFuzzyCompare(i.key(), StelUtils::qDateTimeToJd(QDateTime::fromString(i.value(), Qt::ISODate)));
	 }
}
