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

// Utilities for defining the expected value in the test below
double a(double d, double m, double s){
	int sign = d < 0 || m < 0 || s < 0 ? -1 : 1;
	double deg = std::abs(d) + std::abs(m)/60 + std::abs(s)/3600;
	return deg/180*M_PI*sign;
}

double h(double d, double m, double s){
	return a(d,m,s)*15;
}

void TestConversions::testStringCoordinateToRad()
{
	#define LIM (M_PI/(180.*3600000)) // 1 marcsec
	QVariantList data;
	// legacy
	data << "+0d0m0s"	    << 0. << "legacy";
   	data << "+30d0m0s"	    << M_PI/6. << "legacy";
	data << "+45d0m0s"	    << M_PI/4. << "legacy";
	data << "+90d0m0s"	    << M_PI/2. << "legacy";
	data << "+80d25m10s"	<< a(80,25,10) << "legacy";
	data << "+400d0m5s"	    << a(400,0,5) << "legacy";
	data << "-30d0m50s"	    << a(-30,0,50) << "legacy";
	data << "123.567 N"	    << a(123.567,0,0) << "legacy";
	data << "123.567 S"	    << a(-123.567,0,0) << "legacy";
	data << "123.567 W"	    << a(-123.567,0,0) << "legacy";
	data << "+46d6'31\""	<< a(46,6,31) << "legacy";
	data << "12h0m0s"	    << M_PI << "legacy";
	data << "6h0m0s"	    << M_PI/2. << "legacy";
	data << "10h30m0s"	    << h(10,30,0) << "legacy";
	data << "+80°25'10\""	<< a(80,25,10) << "legacy";
	data << "-45d0m0s"	    << -M_PI/4. << "legacy";
	data << "-80°25'10\""	<< a(-80,25,10) << "legacy";
	data << "-80r25m10s"	<< -0.0 << "legacy";
	// fail
	data << "-10.5°1.5'1\"" << -0.0 << "fraction not last";
	data << "1.2.3°4.5'1\"" << -0.0 << "more than one decimal point";
	data << "1h2m3sN" <<       -0.0 << "h-N/S-conflict";
	data << "2x3m4s" <<        -0.0 << "invalid degrees marker";
	data << "2d3n4s" <<        -0.0 << "invalid minutes marker";
	data << "2d3m4m" <<        -0.0 << "invalid seconds marker";
	data << "-" <<             -0.0 << "no digit";
	data << "W" <<             -0.0 << "no digit";
	// pass
	data << "1d1m1s"      << a(1,1,1) << "d-m-s lowercase";
	data << "2D2M2S"      << a(2,2,2) << "d-m-s uppercase";
	data << "1h1m1s"      << h(1,1,1) << "h-m-s lowercase";
	data << "2H2M2S"      << h(2,2,2) << "h-m-s uppercase";
	data << "10m10.5s"    << a(0,10,10.5) << "m-s << omit degrees";
	data << "20d20.5s"    << a(20,0,20.5) << "d-s << omit minutes";
	data << "30d30.5m"    << a(30,30.5,0) << "d-m << omit seconds";
	data << "40.5d"       << a(40.5,0,0)  << "d << omit minutes and seconds";
	data << "50.5m"       << a(0,50.5,0)  << "m << omit degrees-seconds";
	data << "60.5s"       << a(0,0,60.5)  << "s << omit degrees-minutes";
	data << "30d30.5m"    << a(30,30.5,0) << "d-m << omit seconds";
	data << "-4d4m4s"     << a(-4,4,4) <<   "minus sign on degrees";
	data << "-5m5s"       << a(0,-5,5) <<   "minus sign on minutes";
	data << "-6s"         << a(0,0,-6) <<   "minus sign on seconds";
	data << "+4d4m4s"     << a(+4,4,4) <<   "plus sign on degrees";
	data << "+5m5s"       << a(0,+5,5) <<   "plus sign on minutes";
	data << "+6S"         << a(-6,0,0) <<   "S(outh) overrides '+'";
	data << "+6sS"        << a(0,0,-6) <<   "seconds only, S is negative";
	data << "2d3ms"       << a(-2,3,0)   << "d,m with s(outh)";
	data << "10d10mN"     << a(+10,10,0) << "N is positive";
	data << "20d20mE"     << a(+20,20,0) << "E is positive";
	data << "10d10mS"     << a(-10,10,0) << "S is negative";
	data << "20d20mW"     << a(-20,20,0) << "W is negative";
	data << "-10d10mN"    << a(+10,10,0) << "N is positive << overrules negative sign";
	data << "-20d20mE"    << a(+20,20,0) << "E is positive, overrules negative sign";
	data << "+10d10mS"    << a(-10,10,0) << "S is negative, overrules positive sign";
	data << "+20d20mW"    << a(-20,20,0) << "W is negative, overrules positive sign";
	data << "1°1'1\""     << a(1,1,1) << "degree sign-'-\"";
	data << "2º2'2\""     << a(2,2,2) << "masculine ordinal indicator-'-\"";
	data << "  1d1m1s"    << a(1,1,1) << "leading spaces";
	data << "1  h1m1s"    << h(1,1,1) << "embedded spaces";
	data << "1h  1m1s"    << h(1,1,1) << "embedded spaces";
	data << "1d1  m1s"    << a(1,1,1) << "embedded spaces";
	data << "1d1m  1s"    << a(1,1,1) << "embedded spaces";
	data << "1d1m1  s"    << a(1,1,1) << "embedded spaces";
	data << "1d1m1s  "    << a(1,1,1) << "trailing spaces";
	data << "4d 4m 4s"    << a(4,4,4) << "systematic spacing";
	data << "5 d 5 m 5 s" << a(5,5,5) << "maximum spacing";
	data << "123.567dN"   << a( 123.567, 0, 0) << "degrees North";
	data << "123.567dS"   << a(-123.567, 0, 0) << "degrees South";
	data << "123.567°E"   << a( 123.567, 0, 0) << "degrees East";
	data << "123.567°W"   << a(-123.567, 0, 0) << "degrees West";
	data << "+400d0m5s"   << a(400, 0, 5) << ">360°";

	while (data.count()>=3)
	{
		QString coordinate, explain;
		double angle, expectedValue;
		coordinate	= data.takeFirst().toString();
		expectedValue = data.takeFirst().toDouble();
		explain = data.takeFirst().toString();
		angle = StelUtils::getDecAngle(coordinate);

		QVERIFY2(std::abs((angle) - (expectedValue)) < LIM,
				 qPrintable(QString("%1 = %2 radians (expected %3 radians) - %4")
							.arg(coordinate)
							.arg(QString::number(angle))
							.arg(QString::number(expectedValue))
							.arg(explain)) );		
	}
	#undef LIM
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
		unsigned int h, m;
		double expectedHours, s;
		h = data.takeFirst().toUInt();
		m = data.takeFirst().toUInt();
		s = data.takeFirst().toDouble();
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

void TestConversions::testHoursToHMSStrLowPrecision()
{
	QVariantList data;

	data << "0h00m"	<< 0.;
	data << "5h00m"	<< 5.;
	data << "9h00m"	<< 9.;
	data << "12h00m"	<< 12.;
	data << "15h00m"	<< 15.;
	data << "0h15m"	<< 0.25;
	data << "0h00m"	<< 0.004124;
	data << "0h01m"	<< 0.009185;
	data << "4h00m"	<< 3.999722;
	data << "13h00m"	<< 12.9997;
	data << "24h00m"	<< 23.999736;

	while (data.count() >= 2)
	{
		QString expectedHMS = data.takeFirst().toString();
		double hours = data.takeFirst().toDouble();		
		QString hms = StelUtils::hoursToHmsStr(hours, true);
		QString hmsF = StelUtils::hoursToHmsStr(static_cast<float>(hours), true);
		QVERIFY2(expectedHMS==hms, qPrintable(QString("%1h = %2 (expected %3)")
										.arg(QString::number(hours, 'f', 6))
										.arg(hms)
										.arg(expectedHMS)));
		QVERIFY2(expectedHMS==hmsF, qPrintable(QString("%1h = %2 (expected %3)")
										.arg(QString::number(hours, 'f', 6))
										.arg(hmsF)
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

void TestConversions::testVec3iToHtmlColor()
{
	QVariantList data;

	data << "#FFFFFF" << 255 << 255 << 255;
	data << "#FF0000" << 255 << 0 << 0;
	data << "#00FF00" << 0 << 255 << 0;
	data << "#0000FF" << 0 << 0 << 255;
	data << "#999999" << 153 << 153 << 153;
	data << "#666666" << 102 << 102 << 102;
	data << "#000000" << 0 << 0 << 0;
	data << "#000000" << 0 << 0 << 0;

	while (data.count()>=4)
	{
		QString color	= data.takeFirst().toString();
		int v1	= data.takeFirst().toInt();
		int v2	= data.takeFirst().toInt();
		int v3	= data.takeFirst().toInt();
		Vec3i srcColor	= Vec3i(v1, v2, v3);
		QString cColor	= srcColor.toHtmlColor().toUpper();

		QVERIFY2(cColor==color, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(srcColor.toString())
							   .arg(cColor)
							   .arg(color)));
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
		QString cColor	= srcColor.toHtmlColor().toUpper();

		QVERIFY2(cColor==color, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(srcColor.toString())
							   .arg(cColor)
							   .arg(color)));
	}
}
void TestConversions::testVec3dToHtmlColor()
{
	QVariantList data;

	data << "#FFFFFF" << 1. << 1. << 1.;
	data << "#FF0000" << 1. << 0. << 0.;
	data << "#00FF00" << 0. << 1. << 0.;
	data << "#0000FF" << 0. << 0. << 1.;
	data << "#999999" << .6 << .6 << .6;
	data << "#666666" << .4 << .4 << .4;
	data << "#000000" << 0. << 0. << 0.;
	data << "#000000" << 0. << 0. << 0.;

	while (data.count()>=4)
	{
		QString color	= data.takeFirst().toString();
		double v1	= data.takeFirst().toDouble();
		double v2	= data.takeFirst().toDouble();
		double v3	= data.takeFirst().toDouble();
		Vec3d srcColor	= Vec3d(v1, v2, v3);
		QString cColor	= srcColor.toHtmlColor().toUpper();

		QVERIFY2(cColor==color, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(srcColor.toString())
							   .arg(cColor)
							   .arg(color)));
	}
}

void TestConversions::testVec3iSetFromHtmlColor()
{
	QVariantList data;

	data << "#FFFFFF" << 255 << 255 << 255;
	data << "#FF0000" << 255 << 0 << 0;
	data << "#00FF00" << 0 << 255 << 0;
	data << "#0000FF" << 0 << 0 << 255;
	data << "#999999" << 153 << 153 << 153;
	data << "#666666" << 102 << 102 << 102;
	data << "#000"    << 0 << 0 << 0;

	while (data.count()>=4)
	{
		QString color	= data.takeFirst().toString();
		int v1	= data.takeFirst().toInt();
		int v2	= data.takeFirst().toInt();
		int v3	= data.takeFirst().toInt();
		Vec3i expected	= Vec3i(v1, v2, v3);
		Vec3i v3icolor	= Vec3i().setFromHtmlColor(color);

		QVERIFY2(v3icolor==expected, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(color)
							   .arg(v3icolor.toString())
							   .arg(expected.toString())));
	}
}

void TestConversions::testVec3fSetFromHtmlColor()
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
		Vec3f v3fcolor	= Vec3f().setFromHtmlColor(color);

		QVERIFY2(v3fcolor==expected, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(color)
							   .arg(v3fcolor.toString())
							   .arg(expected.toString())));
	}
}

void TestConversions::testVec3dSetFromHtmlColor()
{
	QVariantList data;

	data << "#FFFFFF" << 1. << 1. << 1.;
	data << "#FF0000" << 1. << 0. << 0.;
	data << "#00FF00" << 0. << 1. << 0.;
	data << "#0000FF" << 0. << 0. << 1.;
	data << "#999999" << .6 << .6 << .6;
	data << "#666666" << .4 << .4 << .4;
	data << "#000"    << 0. << 0. << 0.;

	while (data.count()>=4)
	{
		QString color	= data.takeFirst().toString();
		double v1	= data.takeFirst().toDouble();
		double v2	= data.takeFirst().toDouble();
		double v3	= data.takeFirst().toDouble();
		Vec3d expected	= Vec3d(v1, v2, v3);
		Vec3d v3dcolor	= Vec3d().setFromHtmlColor(color);

		QVERIFY2(v3dcolor==expected, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(color)
							   .arg(v3dcolor.toString())
							   .arg(expected.toString())));
	}
}

void TestConversions::testVec3iQColor()
{
	QVariantList data;

	data << "#FFFFFF" << 255 << 255 << 255;
	data << "#FF0000" << 255 << 0 << 0;
	data << "#00FF00" << 0 << 255 << 0;
	data << "#0000FF" << 0 << 0 << 255;
	data << "#999999" << 153 << 153 << 153;
	data << "#666666" << 102 << 102 << 102;

	while (data.count()>=4)
	{
		QString colorStr= data.takeFirst().toString();
		int v1	= data.takeFirst().toInt();
		int v2	= data.takeFirst().toInt();
		int v3	= data.takeFirst().toInt();
		Vec3i expected	= Vec3i(v1, v2, v3);
		Vec3i v3icolor	= Vec3i().setFromHtmlColor(colorStr);
		QString qcolStr  = v3icolor.toQColor().name(QColor::HexRgb).toUpper();

		QVERIFY2(qcolStr == colorStr,
			 qPrintable(QString("%1 = %2 (expected %3)").arg(colorStr)
								    .arg(qcolStr)
								    .arg(expected.toHtmlColor())));
	}
}
void TestConversions::testVec3fQColor()
{
	QVariantList data;

	data << "#FFFFFF" << 1.f << 1.f << 1.f;
	data << "#FF0000" << 1.f << 0.f << 0.f;
	data << "#00FF00" << 0.f << 1.f << 0.f;
	data << "#0000FF" << 0.f << 0.f << 1.f;
	data << "#999999" << .6f << .6f << .6f;
	data << "#666666" << .4f << .4f << .4f;

	while (data.count()>=4)
	{
		QString colorStr= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		float v3	= data.takeFirst().toFloat();
		Vec3f expected	= Vec3f(v1, v2, v3);
		Vec3f v3fcolor	= Vec3f().setFromHtmlColor(colorStr);
		QString qcolStr  = v3fcolor.toQColor().name(QColor::HexRgb).toUpper();

		QVERIFY2(qcolStr == colorStr,
			 qPrintable(QString("%1 = %2 (expected %3)").arg(colorStr)
								    .arg(qcolStr)
								    .arg(expected.toHtmlColor())));
	}
}
void TestConversions::testVec3dQColor()
{
	QVariantList data;

	data << "#FFFFFF" << 1. << 1. << 1.;
	data << "#FF0000" << 1. << 0. << 0.;
	data << "#00FF00" << 0. << 1. << 0.;
	data << "#0000FF" << 0. << 0. << 1.;
	data << "#999999" << .6 << .6 << .6;
	data << "#666666" << .4 << .4 << .4;

	while (data.count()>=4)
	{
		QString colorStr= data.takeFirst().toString();
		double v1	= data.takeFirst().toDouble();
		double v2	= data.takeFirst().toDouble();
		double v3	= data.takeFirst().toDouble();
		Vec3d expected	= Vec3d(v1, v2, v3);
		Vec3d v3dcolor	= Vec3d().setFromHtmlColor(colorStr);
		QString qcolStr  = v3dcolor.toQColor().name(QColor::HexRgb).toUpper();

		QVERIFY2(qcolStr == colorStr,
			 qPrintable(QString("%1 = %2 (expected %3)").arg(colorStr)
								    .arg(qcolStr)
								    .arg(expected.toHtmlColor())));
	}
}

void TestConversions::testStrToVec2f()
{
	QVariantList data;

	data << "1,1" << 1.f << 1.f;
	data << "1,0" << 1.f << 0.f;
	data << "0,1" << 0.f << 1.f;
	data << "0,0" << 0.f << 0.f;
	data << "0"   << 0.f << 0.f; // may cause warning

	while (data.count()>=3)
	{
		QString vec	= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		Vec2f srcVec	= Vec2f(v1, v2);
		Vec2f dstVec	= Vec2f(vec);

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
		QString dstVec	= srcVec.toStr();

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
	data << "0,0"   << 0.f << 0.f << 0.f; // may cause warning
	data << "0"     << 0.f << 0.f << 0.f; // may cause warning

	while (data.count()>=4)
	{
		QString vec	= data.takeFirst().toString();
		float v1	= data.takeFirst().toFloat();
		float v2	= data.takeFirst().toFloat();
		float v3	= data.takeFirst().toFloat();
		Vec3f srcVec	= Vec3f(v1, v2, v3);
		Vec3f dstVec	= Vec3f(vec);

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
		//QString dstVec	= StelUtils::vec3fToStr(srcVec);
		QString dstVec	= srcVec.toStr();

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
	data << "0,0,0"   << 0. << 0. << 0. << 0.; // may cause warning
	data << "0,0"     << 0. << 0. << 0. << 0.; // may cause warning
	data << "0"       << 0. << 0. << 0. << 0.; // may cause warning

	while (data.count()>=5)
	{
		QString vec	= data.takeFirst().toString();
		double v1	= data.takeFirst().toDouble();
		double v2	= data.takeFirst().toDouble();
		double v3	= data.takeFirst().toDouble();
		double v4	= data.takeFirst().toDouble();
		Vec4d srcVec	= Vec4d(v1, v2, v3, v4);
		Vec4d dstVec	= Vec4d(vec);

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
		QString dstVec	= srcVec.toStr();

		QVERIFY2(vec==dstVec, qPrintable(QString("%1 = %2 (expected %3)")
							   .arg(srcVec.toString())
							   .arg(dstVec)
							   .arg(vec)));
	}
}

void TestConversions::testQDateTimeToJD()
{
	 QMap<double, QString> map;
	 map[2454466.0] = "2007-12-31 12:00:00";
	 map[2500000.0] = "2132-08-31 12:00:00";
	 map[2454534.0] = "2008-03-08 12:00:00";
	 map[2299161.0] = "1582-10-15 12:00:00";
	 map[2454466.5] = "2008-01-01 00:00:00";
	 map[2400000.0] = "1858-11-16 12:00:00";
	 //map[2110516.0] = "1066-04-12 12:00:00";
	 //map[1918395.0] = "0540-04-12 12:00:00";
	 //map[1794575.0] = "0201-04-12 12:00:00";
	 //map[1757319.0] = "0099-04-12 12:00:00";
	 //map[1721424.0] = "0001-01-01 12:00:00";
	 //map[1721789.0] = "0002-01-01 12:00:00";

	 // See https://doc.qt.io/qt-5/qdate.html#details for restrictions and converion issues (qint64 -> double)
	 QString format = "yyyy-MM-dd HH:mm:ss";
	 for (QMap<double, QString>::ConstIterator i=map.constBegin();i!=map.constEnd();++i)
	 {
		 //QDateTime d = QDateTime::fromString(i.value(), format);
		 //qWarning() << i.value() << d.toString(format);
		 double JD = StelUtils::qDateTimeToJd(QDateTime::fromString(i.value(), format));
		 QVERIFY2(qAbs(i.key() - JD)<=ERROR_LIMIT, qPrintable(QString("JD: %1 Date: %2 Expected JD: %3")
					 .arg(QString::number(JD, 'f', 5))
					 .arg(i.value())
					 .arg(QString::number(i.key(), 'f', 5))
					 ));
	 }
}

void TestConversions::testTrunc()
{
	QMap<double, double> mapd;
	mapd[0.0] = 0.0;
	mapd[-1.0] = -1.0;
	mapd[2454466.0] = 2454466.0;
	mapd[24.4] = 24.0;
	mapd[34.5] = 34.0;
	mapd[-4.9] = -4.0;

	for (QMap<double, double>::ConstIterator i=mapd.constBegin();i!=mapd.constEnd();++i)
	{
		double res = StelUtils::trunc(i.key());
		QVERIFY2(qAbs(i.value() - res)<=ERROR_LIMIT, qPrintable(QString("Result: %1 Expected: %2")
					.arg(QString::number(res, 'f', 2))
					.arg(QString::number(i.value(), 'f', 2))
					));
		float resf = StelUtils::trunc(static_cast<float>(i.key()));
		QVERIFY2(qAbs(static_cast<float>(i.value()) - resf)<=ERROR_LIMIT, qPrintable(QString("Result: %1 Expected: %2")
					.arg(QString::number(resf, 'f', 2))
					.arg(QString::number(static_cast<float>(i.value()), 'f', 2))
					));
	}
}
