/*
 * Stellarium
 * Copyright (C) 2020 Alexander Wolf
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

#include "tests/testComparisons.hpp"

#include <QString>
#include <QDebug>
#include <QtGlobal>
#include <QVariantList>
#include <QMap>

#include "StelUtils.hpp"

QTEST_GUILESS_MAIN(TestComparisons)

void TestComparisons::testVersions()
{
	QVariantList data;
	data << "0.19.1" << "0.19.1" <<  0;
	data << "1.4"    << "1.4"    <<  0;
	data << "0.19.1" << "0.19.2" << -1;
	data << "1.4"    << "1.8"    << -1;
	data << "0.19.3" << "0.19.2" <<  1;
	data << "1.9"    << "1.8"    <<  1;

	while (data.count() >= 3)
	{
		QString v1, v2;
		int r, er;
		v1 = data.takeFirst().toString();
		v2 = data.takeFirst().toString();
		er = data.takeFirst().toInt();
		r = StelUtils::compareVersions(v1, v2);
		QVERIFY2(r==er, qPrintable(QString("%1=%2 (result: %3, expected: %4)").arg(v1).arg(v2).arg(r).arg(er)));
	}
}

void TestComparisons::testOSReports()
{
	QVERIFY2(!StelUtils::getOperatingSystemInfo().isEmpty(), "Oops... No operating system info exist!");
}

void TestComparisons::testUAReports()
{
	QVERIFY2(StelUtils::getUserAgentString().contains("Stellarium", Qt::CaseInsensitive), "Oops... No user agent info exist!");
}

void TestComparisons::testAppName()
{
	QVERIFY2(StelUtils::getApplicationName().contains("Stellarium", Qt::CaseInsensitive), "Oops... No application name exist!");
}
