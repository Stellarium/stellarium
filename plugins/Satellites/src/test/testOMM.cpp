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

#include <QDir>
#include <QDebug>
#include <QVector>
#include <QDateTime>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>

#include "testOMM.hpp"

QTEST_GUILESS_MAIN(TestOMM)

void TestOMM::testLegacyTle()
{
	QString l0("ISS (ZARYA)");
	//                    1         2         3         4         5         6         7
	//          01234567890123456789012345678901234567890123456789012345678901234567890
	QString l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM dut(l0, l1, l2);
	QVERIFY(dut.getSourceType() == OMM::SourceType::LegacyTle);
	QVERIFY(dut.hasValidLegacyTleData() == true);
	QVERIFY(dut.getLine0() == l0);
	QVERIFY(dut.getLine1() == l1);
	QVERIFY(dut.getLine2() == l2);
}

void TestOMM::testProcessTleLegacyLine0()
{
	QString l0("ISS (ZARYA)");
	//                    1         2         3         4         5         6         7
	//          01234567890123456789012345678901234567890123456789012345678901234567890
	QString l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM dut(l0, l1, l2);
	QCOMPARE(dut.getObjectName(), "ISS (ZARYA)");
}

void TestOMM::testProcessTleLegacyLine1()
{
	QString l0("ISS (ZARYA)");
	//                    1         2         3         4         5         6         7
	//          01234567890123456789012345678901234567890123456789012345678901234567890
	QString l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM dut(l0, l1, l2);
	QVERIFY(dut.getNoradcatId() == 25544);
	QVERIFY(dut.getClassification() == 'U');
	QCOMPARE(dut.getObjectId(), QString("98067A"));
	QCOMPARE(dut.getMeanMotionDot(), 0.00007611);
	QCOMPARE(dut.getMeanMotionDDot(), 0.0);
	
	auto jd_of_epoch = dut.getEpoch().getJulian();
	QCOMPARE(jd_of_epoch, 2460135.906404059846);
	QCOMPARE(dut.getBstar(), 0.00014334999999999998785);
	QVERIFY(dut.getEphermisType() == 0);
	QVERIFY(dut.getElementNumber() == 999);
}

void TestOMM::testProcessTleLegacyLine2()
{
	QString    l0("ISS (ZARYA)");
	//                       1         2         3         4         5         6         7
	//             01234567890123456789012345678901234567890123456789012345678901234567890
	QString    l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString    l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM dut(l0, l1, l2);
	QCOMPARE(dut.getInclination(),  51.6398);
	QCOMPARE(dut.getAscendingNode(), 233.5611);
	QCOMPARE(dut.getArgumentOfPerigee(), 12.3897);
	QCOMPARE(dut.getEccentricity(), 0.0000373);
	QCOMPARE(dut.getMeanAnomoly(), 91.4664);
	QCOMPARE(dut.getMeanMotion(), 15.49560249);
	QCOMPARE(dut.getRevAtEpoch(), 40476);
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
	QVector<double> expectEpoch = { 
		2460135.906404059846, 
		2460135.906404059846, 
		2460135.906404059846
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
			OMM dut(r);
			QVERIFY(dut.getObjectId() == expectOjectId[idx]);
			QVERIFY(dut.getNoradcatId() == expectNorad[idx]);
			auto jd_of_epoch = dut.getEpoch().getJulian();
			QCOMPARE(jd_of_epoch, expectEpoch[idx]);
			idx++;
		}
		r.readNext();
	}
	file.close();
}

void TestOMM::testLegacyTleVsXML()
{
	OMM dut_xml;
	bool  flag = false;
	QFile file("test_data.xml");
	flag = file.open(QFile::ReadOnly | QFile::Text);
	QVERIFY(true == flag);
	if (!flag)
		return;
	QXmlStreamReader r(&file);
	flag = true;
	while (flag && !r.atEnd()) {
		QString tag = r.name().toString();
		if (r.isStartElement() && tag.toLower() == "omm") {
			dut_xml = OMM(r);
			QVERIFY(dut_xml.getObjectId() == "1998-067A");
			flag = false;
		}
		r.readNext();
	}
	file.close();

	// Load the TLE version of the OMM and compare with the XML version.
	QString    l0("ISS (ZARYA)");
	//                       1         2         3         4         5         6         7
	//             01234567890123456789012345678901234567890123456789012345678901234567890
	QString    l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString    l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM dut_tle(l0, l1, l2);
	QVERIFY(dut_tle.getObjectName() == "ISS (ZARYA)");
	
	QCOMPARE(dut_xml.getInclination(), dut_tle.getInclination());
	QCOMPARE(dut_xml.getAscendingNode(), dut_tle.getAscendingNode());
	QCOMPARE(dut_xml.getArgumentOfPerigee(), dut_tle.getArgumentOfPerigee());
	QCOMPARE(dut_xml.getEccentricity(), dut_tle.getEccentricity());
	QCOMPARE(dut_xml.getMeanAnomoly(), dut_tle.getMeanAnomoly());
	QCOMPARE(dut_xml.getMeanMotion(), dut_tle.getMeanMotion());
	QCOMPARE(dut_xml.getRevAtEpoch(), dut_tle.getRevAtEpoch());
	QCOMPARE(dut_xml.getEpoch().getJulian(), dut_tle.getEpoch().getJulian());
}

void TestOMM::testLegacyTleVsJSON()
{
	OMM dut_json;
	bool flag = false;
	QFile file("test_data.json");
	flag = file.open(QFile::ReadOnly | QFile::Text);
	QVERIFY(true == flag);
	if (!flag)
		return;

	QByteArray data = file.readAll();
	file.close();
	QJsonDocument doc(QJsonDocument::fromJson(data));
	QJsonArray arr = doc.array();
	for (const auto & item : arr) {
		QJsonObject obj = item.toObject();
		dut_json = OMM(obj);
		QVERIFY(dut_json.getObjectId() == "1998-067A");
		if (dut_json.getObjectId() == "1998-067A")
			break;
	}

	// Load the TLE version of the OMM and compare with the JSON version.
	QString    l0("ISS (ZARYA)");
	//                       1         2         3         4         5         6         7
	//             01234567890123456789012345678901234567890123456789012345678901234567890
	QString    l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString    l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM dut_tle(l0, l1, l2);
	QVERIFY(dut_tle.getObjectName() == "ISS (ZARYA)");
	QVERIFY(dut_json.getObjectName() == "ISS (ZARYA)");

	QCOMPARE(dut_json.getInclination(), dut_tle.getInclination());
	QCOMPARE(dut_json.getAscendingNode(), dut_tle.getAscendingNode());
	QCOMPARE(dut_json.getArgumentOfPerigee(), dut_tle.getArgumentOfPerigee());
	QCOMPARE(dut_json.getEccentricity(), dut_tle.getEccentricity());
	QCOMPARE(dut_json.getMeanAnomoly(), dut_tle.getMeanAnomoly());
	QCOMPARE(dut_json.getMeanMotion(), dut_tle.getMeanMotion());
	QCOMPARE(dut_json.getRevAtEpoch(), dut_tle.getRevAtEpoch());
	QCOMPARE(dut_json.getEpoch().getJulian(), dut_tle.getEpoch().getJulian());
}

void TestOMM::testCopyCTOR()
{
	QString    l0("ISS (ZARYA)");
	//                    1         2         3         4         5         6         7
	//          01234567890123456789012345678901234567890123456789012345678901234567890
	QString    l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString    l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM dut(l0, l1, l2);
	QVERIFY(dut.getSourceType() == OMM::SourceType::LegacyTle);
	QVERIFY(dut.hasValidLegacyTleData() == true);
	QVERIFY(dut.getLine0() == l0);
	QVERIFY(dut.getLine1() == l1);
	QVERIFY(dut.getLine2() == l2);

	OMM dut2(dut);
	QVERIFY(dut2.getSourceType() == OMM::SourceType::LegacyTle);
	QVERIFY(dut2.hasValidLegacyTleData() == true);
	QVERIFY(dut2.getLine0() == l0);
	QVERIFY(dut2.getLine1() == l1);
	QVERIFY(dut2.getLine2() == l2);
}

void TestOMM::testOperatorEquals()
{
	QString l0("ISS (ZARYA)");
	//                    1         2         3         4         5         6         7
	//          01234567890123456789012345678901234567890123456789012345678901234567890
	QString l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM     dut(l0, l1, l2);
	QVERIFY(dut.getSourceType() == OMM::SourceType::LegacyTle);
	QVERIFY(dut.hasValidLegacyTleData() == true);
	QVERIFY(dut.getLine0() == l0);
	QVERIFY(dut.getLine1() == l1);
	QVERIFY(dut.getLine2() == l2);

	OMM dut2;
	dut2 = dut;
	QVERIFY(dut2.getSourceType() == OMM::SourceType::LegacyTle);
	QVERIFY(dut2.hasValidLegacyTleData() == true);
	QVERIFY(dut2.getLine0() == l0);
	QVERIFY(dut2.getLine1() == l1);
	QVERIFY(dut2.getLine2() == l2);
}

void TestOMM::testFetchJSONObj() 
{
	QString l0("ISS (ZARYA)");
	//                    1         2         3         4         5         6         7
	//          01234567890123456789012345678901234567890123456789012345678901234567890
	QString l1("1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995");
	QString l2("2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764");
	OMM     src(l0, l1, l2);

	QJsonObject dut;
	src.toJsonObj(dut);

	//qDebug() << "Count: " << dut.count();
	//qDebug() << dut;

	QJsonValue value;

	value = dut.take("OBJECT_NAME");
	QVERIFY(value != QJsonValue::Undefined);
	QCOMPARE(value.toString(), "ISS (ZARYA)");

	value = dut.take("EPOCH");
	QVERIFY(value != QJsonValue::Undefined);
	QCOMPARE(value.toString(), "2023-07-10T09:45:13.3108");

	value = dut.take("ARG_OF_PERICENTER");
	QVERIFY(value != QJsonValue::Undefined);
	QCOMPARE(value.toDouble(), 12.3897);

	value = dut.take("BSTAR");
	QVERIFY(value != QJsonValue::Undefined);
	QCOMPARE(value.toDouble(), 0.00014335);

	value = dut.take("CLASSIFICATION_TYPE");
	QVERIFY(value != QJsonValue::Undefined);
	QCOMPARE(value.toString(), "U");

	value = dut.take("RA_OF_ASC_NODE");
	QVERIFY(value != QJsonValue::Undefined);
	QCOMPARE(value.toDouble(), 233.5611);
}


