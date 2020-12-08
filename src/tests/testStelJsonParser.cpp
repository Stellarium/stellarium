/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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

#include "tests/testStelJsonParser.hpp"

#include <QObject>
#include <QDebug>
#include <QBuffer>
#include <stdexcept>

#include "StelJsonParser.hpp"


QTEST_GUILESS_MAIN(TestStelJsonParser);

void TestStelJsonParser::initTestCase()
{
	largeJsonBuff = "{\"test1\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test2\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test3\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test4\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test5\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test6\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test7\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test8\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test9\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test10\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test11\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}, \
 \"test12\": {\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}}";

	listJsonBuff = "[{\"project\":\"GOODS\",\"license\":\"ESO Data License : http://www.myLicenseToBeDefinedAtSomePoint.html\",\"copyright\":\"(c) GOODS Sep 10 2007 12:00AM\",\"creator\":\"C. Cesarsky\",\"dataType\":\"image\",\"characterization\":{\"spatialAxis\":{\"footprint\":{\"worldCoords\":[[[53.111991,-27.725812],[53.164780,-27.725812],[53.164780,-27.772234],[53.111991,-27.772234]]]},\"boundingBox\":[[53.111991,-27.725812],[53.164780,-27.725812],[53.164780,-27.772234],[53.111991,-27.772234]],\"centralPos\":[53.138382,-27.749026]},\"temporalAxis\":{\"boundingBox\":[52220.243068,52263.181794],\"integratedCoverage\":0.208333,\"centralPos\":52241.712431,\"coverage\":[52220.243068,52263.181794]}},\"publisher\":\"ESO SAF\",\"collection\":\"168.A-0485(A\",\"targetSource\":{\"names\":[\"GOODS_09\"]},\"ESO\":{\"NGASFileId\":\"GOODS_ISAAC_09_H_V2.0\",\"metadataType\":\"DataProduct\",\"processingType\":\"HighlyProcessed\"},\"acquisitionSetup\":{\"filter\":\"H\",\"instrument\":\"ISAAC\",\"facility\":\"ESO-Paranal\",\"telescope\":\"ESO-VLT-U1\",\"mode\":\"Short Wavelength\"},\"title\":\"GOODS_ISAAC_09_H_v2.0\",\"id\":\"GOODS_ISAAC_09_H_V2.0\"},{\"project\":\"GOODS\",\"license\":\"ESO Data License : http://www.myLicenseToBeDefinedAtSomePoint.html\",\"copyright\":\"(c) GOODS Sep 10 2007 12:00AM\",\"creator\":\"C. Cesarsky\",\"dataType\":\"image\",\"characterization\":{\"spatialAxis\":{\"footprint\":{\"worldCoords\":[[[53.121222,-27.641601],[53.174252,-27.641601],[53.174252,-27.687943],[53.121222,-27.687943]]]},\"boundingBox\":[[53.121222,-27.641601],[53.174252,-27.641601],[53.174252,-27.687943],[53.121222,-27.687943]],\"centralPos\":[53.147732,-27.664775]},\"temporalAxis\":{\"boundingBox\":[53729.079417,53747.174968],\"integratedCoverage\":0.122222,\"centralPos\":53738.127193,\"coverage\":[53729.079417,53747.174968]}},\"publisher\":\"ESO SAF\",\"collection\":\"168.A-0485(G)\",\"targetSource\":{\"names\":[\"GOODS_01\"]},\"ESO\":{\"NGASFileId\":\"GOODS_ISAAC_01_J_V2.0\",\"metadataType\":\"DataProduct\",\"processingType\":\"HighlyProcessed\"},\"acquisitionSetup\":{\"filter\":\"J\",\"instrument\":\"ISAAC\",\"facility\":\"ESO-Paranal\",\"telescope\":\"ESO-VLT-U1\",\"mode\":\"Short Wavelength\"},\"title\":\"GOODS_ISAAC_01_J_v2.0\",\"id\":\"GOODS_ISAAC_01_J_V2.0\"},{\"project\":\"GOODS\",\"license\":\"ESO Data License : http://www.myLicenseToBeDefinedAtSomePoint.html\",\"copyright\":\"(c) GOODS Sep 10 2007 12:00AM\",\"creator\":\"C. Cesarsky\",\"dataType\":\"image\",\"characterization\":{\"spatialAxis\":{\"footprint\":{\"worldCoords\":[[[53.121081,-27.641392],[53.174488,-27.641392],[53.174488,-27.688027],[53.121081,-27.688027]]]},\"boundingBox\":[[53.121081,-27.641392],[53.174488,-27.641392],[53.174488,-27.688027],[53.121081,-27.688027]],\"centralPos\":[53.147779,-27.664712]},\"temporalAxis\":{\"boundingBox\":[53729.179656,53749.175133],\"integratedCoverage\":0.207292,\"centralPos\":53739.177395,\"coverage\":[53729.179656,53749.175133]}},\"publisher\":\"ESO SAF\",\"collection\":\"168.A-0485(G)\",\"targetSource\":{\"names\":[\"GOODS_01\"]},\"ESO\":{\"NGASFileId\":\"GOODS_ISAAC_01_KS_V2.0\",\"metadataType\":\"DataProduct\",\"processingType\":\"HighlyProcessed\"},\"acquisitionSetup\":{\"filter\":\"Ks\",\"instrument\":\"ISAAC\",\"facility\":\"ESO-Paranal\",\"telescope\":\"ESO-VLT-U1\",\"mode\":\"Short Wavelength\"},\"title\":\"GOODS_ISAAC_01_Ks_v2.0\",\"id\":\"GOODS_ISAAC_01_KS_V2.0\"}]";
}

void TestStelJsonParser::testBase()
{
	QVariant result = StelJsonParser::parse(largeJsonBuff);
	QVERIFY(result.canConvert<QVariantMap>());
	QVERIFY(result.toMap().size()==12);

	result = StelJsonParser::parse(listJsonBuff);
	QVERIFY(result.canConvert<QVariantList>());
	QVERIFY(result.value<QVariantList>().size()==3);

	result = StelJsonParser::parse("{\"val\": 0.000280}");
	bool ok;
	QCOMPARE(result.toMap().value("val").toDouble(&ok), 0.000280);
	QVERIFY(ok==true);

	result = StelJsonParser::parse("{\"valtrue\": true, \"valfalse\": false}");
	QVERIFY(result.toMap().value("valtrue").canConvert<bool>());
	QVERIFY(result.toMap().value("valtrue").toBool()==true);
	QVERIFY(result.toMap().value("valfalse").canConvert<bool>());
	QVERIFY(result.toMap().value("valfalse").toBool()==false);

	result = StelJsonParser::parse("{\"val\": -12356}");
	QVERIFY(result.toMap().value("val").canConvert<int>());
	QVERIFY(result.toMap().value("val").toInt(&ok)==-12356);
	QVERIFY(ok==true);

	result = StelJsonParser::parse("{\"val\": -12356\n}");
	QVERIFY(result.toMap().value("val").canConvert<int>());
	QVERIFY(result.toMap().value("val").toInt(&ok)==-12356);
	QVERIFY(ok==true);

	// Test windows line ending
	result = StelJsonParser::parse("{\"val\": -12356\r\n}");
	QVERIFY(result.toMap().value("val").canConvert<int>());
	QVERIFY(result.toMap().value("val").toInt(&ok)==-12356);
	QVERIFY(ok==true);
}

void TestStelJsonParser::testErrors()
{
	bool wasCatched = false;
	QVariant result;
	QString erMsg;
	try
	{
		result = StelJsonParser::parse("{val: -12356}");
	}
	catch (std::runtime_error& e)
	{
		wasCatched = true;
		erMsg=e.what();
	}
	// qDebug() << erMsg;
	QVERIFY(wasCatched);
	QVERIFY(result.isNull());
}

void TestStelJsonParser::benchmarkParse()
{
	QBuffer buf;
	buf.setData(largeJsonBuff);
	buf.open(QIODevice::ReadOnly);
	QVariant result;
	QBENCHMARK {
		buf.seek(0);
		result = StelJsonParser::parse(&buf);
	}
}

void TestStelJsonParser::testWriteParse()
{
	bool ok;
	QVariantMap json;
	json.insert("val", 280113);

	QByteArray res;
	QBuffer buf1(&res);
	buf1.open(QIODevice::WriteOnly);
	StelJsonParser::write(json, &buf1);
	buf1.close();

	QVariant result = StelJsonParser::parse(res);
	QVERIFY(result.toMap().value("val").canConvert<int>());
	QVERIFY(result.toMap().value("val").toInt(&ok)==280113);
	QVERIFY(ok==true);
}
