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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "tests/testConversions.hpp"
#include "StelUtils.hpp"
#include <QString>
#include <QDebug>
#include <QtGlobal>
#include <QVariantList>
#include <QMap>

#define ERROR_LIMIT 1e-6

QTEST_MAIN(TestConversions)

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
		QVERIFY2(std::abs(StelUtils::hmsToRad(h, m, s)-rad)<=ERROR_LIMIT, qPrintable(QString("%1h%2m%3s=%4").arg(h).arg(m).arg(s).arg(rad)));
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
		QVERIFY2(std::abs(StelUtils::dmsToRad(deg, min, sec)-rad)<=ERROR_LIMIT, qPrintable(QString("%1d%2m%3s=%4").arg(deg).arg(min).arg(sec).arg(rad)));
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
		QVERIFY2(std::abs(t1-t2)<=0.1, qPrintable(QString("%1rad=%2h%3m%4s").arg(rad).arg(ho).arg(mo).arg(so)));
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
			angle1 = -1*(sec+min*60+std::abs(deg)*3600);
		StelUtils::radToDms(rad, sign, dego, mino, seco);
		angle2 = seco+mino*60+dego*3600;
		if (!sign)
		{
			angle2 = -1*angle2;
			s = "-";
		}
		else
			s = "+";
		QVERIFY2(std::abs(angle1-angle2)<=ERROR_LIMIT, qPrintable(QString("%1rad=%2%3d%4m%5s").arg(rad).arg(s).arg(dego).arg(mino).arg(seco)));
	}
}
