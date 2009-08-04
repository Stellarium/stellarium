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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <QObject>
#include <QtDebug>
#include <QtTest>
#include <stdexcept>

#include "testStelJsonParser.hpp"
#include "StelJsonParser.hpp"

QTEST_MAIN(TestStelJsonParser);

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
}

void TestStelJsonParser::testBase()
{
	QBuffer buf;
	buf.setData(largeJsonBuff);
	buf.open(QIODevice::ReadOnly);
	QVariant result = StelJsonParser::parse(buf);
	buf.close();
	QVERIFY(result.canConvert<QVariantMap>());
	QVERIFY(result.toMap().size()==12);
	
	QByteArray doubleStr = "{\"val\": 0.000280}";
	buf.setData(doubleStr);
	buf.open(QIODevice::ReadOnly);
	result = StelJsonParser::parse(buf);
	buf.close();
	bool ok;
	QCOMPARE(result.toMap().value("val").toDouble(&ok), 0.000280);
	QVERIFY(ok==true);
	
	QByteArray boolStr = "{\"valtrue\": true, \"valfalse\": false}";
	buf.setData(boolStr);
	buf.open(QIODevice::ReadOnly);
	result = StelJsonParser::parse(buf);
	buf.close();
	QVERIFY(result.toMap().value("valtrue").canConvert<bool>());
	QVERIFY(result.toMap().value("valtrue").toBool()==true);
	QVERIFY(result.toMap().value("valfalse").canConvert<bool>());
	QVERIFY(result.toMap().value("valfalse").toBool()==false);
	
	QByteArray intStr = "{\"val\": -12356}";
	buf.setData(intStr);
	buf.open(QIODevice::ReadOnly);
	result = StelJsonParser::parse(buf);
	buf.close();
	QVERIFY(result.toMap().value("val").canConvert<int>());
	QVERIFY(result.toMap().value("val").toInt(&ok)==-12356);
	QVERIFY(ok==true);
}

void TestStelJsonParser::benchmarkParse()
{
	QBuffer buf;
	buf.setData(largeJsonBuff);
	buf.open(QIODevice::ReadOnly);
	QVariant result;
	QBENCHMARK {
		result = StelJsonParser::parse(buf);
	}
}
