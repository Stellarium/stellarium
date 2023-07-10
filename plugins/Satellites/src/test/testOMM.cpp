/*
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

#include <QDir>
#include <QDebug>
#include <QVector>
#include <QDateTime>
#include "testOMM.hpp"

QTEST_GUILESS_MAIN(TestOMM)

void TestOMM::testLegacyTle()
{
	QString l0("ISS (ZARYA)");
	QString l1("1 25544U 98067A   23187.34555919  .00007611  00000+0  14335-3 0  9995");
	QString l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	PluginSatellites::OMM::ShPtr dut(new PluginSatellites::OMM(l0, l1, l2));
	QVERIFY(dut->getSourceType() == PluginSatellites::OMM::SourceType::LegacyTle);
	QVERIFY(dut->hasValidLegacyTleData() == true);
	QVERIFY(dut->getLine0() == l0);
	QVERIFY(dut->getLine1() == l1);
	QVERIFY(dut->getLine2() == l2);
}

void TestOMM::testProcessTleLegacy()
{
	QString l0("ISS (ZARYA)");
	QString l1("1 25544U 98067A   23187.34555919  .00007611  00000+0  14335-3 0  9995");
	QString l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	PluginSatellites::OMM::ShPtr dut(new PluginSatellites::OMM(l0, l1, l2));
	QVERIFY(dut->getNoradcatId() == 25544);
	QVERIFY(dut->getClassification() == 'U');
	QCOMPARE(dut->getObjectId(), QString("98067A"));
	// ToDo, Epoch
	QCOMPARE(dut->getMeanMotionDot(), 0.00007611);
	QCOMPARE(dut->getMeanMotionDDot(), 0.0);
}

void TestOMM::testXMLread()
{
	QVector<QString> expectOjectId = { 
		QString("1998-067A"), 
		QString("2018-046D"), 
		QString("1998-067RZ")
	};
	QVector<int> expectNorad = { 
		25544, 43557, 47853
	};
	QVector<QString> expectEpoch = {
		QString("2023-07-06T08:17:36.314016"),
		QString("2023-07-06T01:58:30.910944"),
		QString("2023-07-04T18:34:04.881504")
	};
	int idx = 0;
	bool testContinue = true;
	bool chkTestDataFileOpened = false;
	QFile file("test_data.xml");
	chkTestDataFileOpened = file.open(QFile::ReadOnly | QFile::Text);
	QVERIFY(true == chkTestDataFileOpened);
	if (!chkTestDataFileOpened) return;

	QXmlStreamReader r(&file);

	while (testContinue  && !r.atEnd()) {
		QString tag = r.name().toString();
		if (r.isStartElement() && tag.toLower() == "omm") {
			PluginSatellites::OMM::ShPtr dut(new PluginSatellites::OMM(r));
			QVERIFY(dut->getObjectId() == expectOjectId[idx]);
			QVERIFY(dut->getNoradcatId() == expectNorad[idx]);
			QVERIFY(dut->getEpochStr() == expectEpoch[idx]);
			QDateTime ep = QDateTime::fromString(expectEpoch[idx]);
			QVERIFY(dut->getEpoch() == ep);
			idx++;
		}
		r.readNext();
	}
	file.close();
}