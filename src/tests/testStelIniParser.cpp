/*
 * Stellarium
 * Copyright (C) 2019 Alexander Wolf
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

#include "tests/testStelIniParser.hpp"

#include <QObject>
#include <QDebug>
#include <QTest>
#include <QBuffer>
#include <QSettings>

#include "StelIniParser.hpp"
#include "StelFileMgr.hpp"

QTEST_GUILESS_MAIN(TestStelIniParser);

void TestStelIniParser::initTestCase()
{
	QString workingDir = tempDir.path();
	workingDir.replace(QRegExp("/+$"), "");  // sometimes the temp path will have / on the end... zap it.
	if (!QDir::setCurrent(workingDir))
	{
		QFAIL(qPrintable("could not set the working directory to: " + workingDir));
	}

	//qDebug() << "working directory: " << QDir::toNativeSeparators(QDir::currentPath());

	tempIniFile = workingDir + "/test.ini";
	QFile f(tempIniFile);
	if (!f.open(QIODevice::ReadWrite))
	{
		QFAIL(qPrintable("could not create test file " + tempIniFile));
	}
	f.close();
}

void TestStelIniParser::testBase()
{
	if (!QFile::exists(tempIniFile))
	{
		QFAIL(qPrintable("could not open test INI file " + tempIniFile));
	}
	QSettings settings(tempIniFile, StelIniFormat);

	// 1: fill settings
	settings.setValue("some_option", "value");
	settings.setValue("some_flag", true);
	settings.setValue("some_int_number", 10);
	settings.setValue("some_float_number", 10.);
	settings.sync();

	// 2: read settings
	QString someOption = settings.value("some_option", "oops").toString();
	bool someFlag = settings.value("some_flag", false).toBool();
	int someIntNumber = settings.value("some_int_number", 1).toInt();
	double someFloatNumber = settings.value("some_float_number", 1.).toDouble();

	// 3: compare settings
	QVERIFY(someOption=="value");
	QVERIFY(someFlag==true);
	QVERIFY(someIntNumber==10);
	QVERIFY(qAbs(someFloatNumber-10.)<=1e-6);
}
