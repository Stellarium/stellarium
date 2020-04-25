/*
 * Stellarium
 * Copyright (C) 2020 Wolfgang Laun
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


#include "tests/testJavaScripting.hpp"

#include <iostream>
#include <QDebug>
#include <QVariantList>

#include "StelScriptMgr.hpp"
#include "VecMath.hpp"

QTEST_GUILESS_MAIN(TestJavaScripting)

QString TestJavaScripting::runScript(QScriptEngine *engine, QString script )
{
//	std::cout << "Script:" << std::endl << script.toStdString() << std::endl;
	QScriptValue result = engine->evaluate(script);
	if (engine->hasUncaughtException()) {
//		int line = engine->uncaughtExceptionLineNumber();
//		std::cout << "uncaught exception at line" << line << ": " <<
//			result.toString().toStdString() << std::endl;
		return "error";
	}
	return result.toString();
}

void TestJavaScripting::initTestCase()
{
	engine = new QScriptEngine(this);
	StelScriptMgr::defVecClasses(engine);
}
	
void TestJavaScripting::testVec3fConstructor()
{
	QVariantList data;
 
	data << "var f1 = Vec3f(0.5, 0.4, 0.3);\n"
		"f1.r.toFixed(1)+','+f1.g.toFixed(1)+','+f1.b.toFixed(1)\n"
		<< "0.5,0.4,0.3"
		<< "var f3 = Vec3f('#00ffff');\n"
		"f3.toString()\n"
		<< "[r:0, g:1, b:1]"
		<< "var f4 = Vec3f('white');\n"
		"f4.toString()"
		<< "[r:1, g:1, b:1]"
		<< "var f5 = Vec3f('lime');\n"
		"f5.toString()\n"
		<< "[r:0, g:1, b:0]"
		<< "var f6 = Vec3f();\n"
		"f6.toString()\n"
		<< "[r:1, g:1, b:1]"
		<< "var f7 = Vec3f('darkorange');\n"
		"f7.toHex()\n"
		<< "#ff8c00"
		<< "var f8 = Color('darkorange');\n"
		"f8.toHex()\n"
		<< "#ff8c00"
		<< "var f9 = Color(f4);\n"  // still the same QScriptEngine
		"f9.toHex()\n"
		<< "#ffffff";

	while (data.count() >= 4)
	{
		QString script = data.takeFirst().toString();
		QString expect = data.takeFirst().toString();
		QString result = TestJavaScripting::runScript(engine, script);
		QVERIFY2( result == expect, qPrintable(QString("%1=%2").arg(script).arg(result)) );
	}
}

void TestJavaScripting::testVec3fConstructorFail()
{
	QVariantList data;

	data << "var f2 = Vec3f(1, 2, 3);\n"
		"f2.toString()\n"
		<< "error"
		<< "var f10 = Color('nosuchcolor');\n"
		"f10.toString()\n"
		<< "error";

	while (data.count() >= 4)
	{
		QString script = data.takeFirst().toString();
		QString expect = data.takeFirst().toString();
		QString result = TestJavaScripting::runScript(engine, script);
		QVERIFY2( result == expect, qPrintable(QString("%1=%2").arg(script).arg(result)) );
	}
}

void TestJavaScripting::testVec3dConstructor()
{
	QVariantList data;

	data << "var v = Vec3d(4,5,6);\n"
		"v.r + ',' + v.g + ',' + v.b\n"
		<< "4,5,6"
		<< "v.toString()\n"
		<< "[4, 5, 6]"
		<< "v.x + '/' + v.y + '/' + v.z\n"
		<< "4/5/6"
		<< "v.x = 40; v.y = 50; v.z = 60;\n"
		"v.toString()\n"
		<< "[40, 50, 60]"
		<< "var w = Vec3d( 90, 90 );\n"
		"w.x.toFixed(1) + ',' + w.y.toFixed(1) + ',' + w.z.toFixed(1)\n"
		<< "0.0,0.0,1.0";   // toString shows coordinates off by epsilon
	/***
         // TODO: find a way to assert the result
		 << "var a = Vec3d( '30d30m', '60d30s' );\n"
		    "a.toString()\n"
		 << "[?, ?, ?]";
	***/

	while (data.count() >= 4)
	{
		QString script = data.takeFirst().toString();
		QString expect = data.takeFirst().toString();
		QString result = TestJavaScripting::runScript(engine, script);
		QVERIFY2( result == expect, qPrintable(QString("%1=%2").arg(script).arg(result)) );
	}
}
