/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#include <QObject>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QString>
#include <QStringList>

#include <QtDebug>
#include <QtTest>
#include <QRegExp>

#include "StelFileMgr.hpp"
#include "tests/testStelFileMgr.hpp"

QTEST_MAIN(TestStelFileMgr);

void TestStelFileMgr::initTestCase()
{
	partialPath1 = "testfilemgr/path1";
	partialPath2 = "testfilemgr/path2";
	workingDir = QDir::tempPath();
	workingDir.replace(QRegExp("/+$"), "");  // sometimes the temp path will have / on the end... zap it.
	if (!QDir::setCurrent(workingDir))
	{
		QFAIL(qPrintable("could not set the working directory to: "+workingDir));
	}

	qDebug() << "working directory: " << QDir::currentPath();

	StelFileMgr::init();

	// set up a directory hierarchy to test on...
	testDirs << "testfilemgr"
			 << partialPath1
			 << partialPath1+"/landscapes"
			 << partialPath1+"/landscapes/ls1"
			 << partialPath1+"/landscapes/ls2"
			 << partialPath1+"/landscapes/emptydir"
			 << partialPath2
			 << partialPath2+"/landscapes"
			 << partialPath2+"/landscapes/ls1"
			 << partialPath2+"/landscapes/ls3";

	testFiles << partialPath1+"/landscapes/ls1/landscape.ini"
			  << partialPath1+"/landscapes/ls2/landscape.ini"
			  << partialPath1+"/config.ini"
			  << partialPath1+"/inboth.txt"
			  << partialPath2+"/landscapes/dummy.txt"
			  << partialPath2+"/landscapes/ls1/landscape.ini"
			  << partialPath2+"/landscapes/ls3/landscape.ini"
			  << partialPath2+"/inboth.txt";

	foreach(QString path, testDirs)
	{
		if (!QDir().mkdir(path))
		{
			QFAIL(qPrintable("could not create test path " + path));
		}
	}

	// create test files as empty files...
	foreach(QString path, testFiles)
	{
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly))
		{
			QFAIL(qPrintable("could not create test file " + path));
		}
		f.close();
	}

	QStringList path;
	path << "./"+partialPath1;
	path << workingDir+"/"+partialPath2;
	StelFileMgr::setSearchPaths(path);
	qDebug() << "search paths are:  " << StelFileMgr::getSearchPaths();
}

void TestStelFileMgr::cleanupTestCase()
{
	// delete test files
	foreach(QString p, testFiles)
	{
		QFile f(p);
		if (!f.remove())
		{
			qWarning() << "could not clean up file:" << workingDir + "/" + p;
		}
	}

	// remove test directories
	// go over dirs in reverse
	for(int i=testDirs.size()-1; i>=0; i--)
	{
		if (!QDir().rmdir(testDirs.at(i)))
		{
			qWarning() << "could not clean up directory:" << workingDir + "/" + testDirs.at(i);
		}
	}
}

void TestStelFileMgr::testFindFileVanilla()
{
	QString exists, notExists;
	try { exists = StelFileMgr::findFile("landscapes"); } catch (std::runtime_error&) {}
	try { notExists = StelFileMgr::findFile("notexists"); } catch (std::runtime_error&) {}

	QVERIFY(!exists.isEmpty());
	QVERIFY(QFileInfo(exists).fileName()=="landscapes");
	QVERIFY(notExists.isEmpty());
}

void TestStelFileMgr::testFindFileVanillaAbs()
{
	QString exists, notExists;
	try { exists = StelFileMgr::findFile(workingDir + "/" + partialPath1 + "/config.ini"); } catch (std::runtime_error&) {}
	try { notExists = StelFileMgr::findFile(workingDir + "/" + partialPath2 + "/config.ini"); } catch (std::runtime_error&) {}

	QVERIFY(!exists.isEmpty());
	QVERIFY(QFileInfo(exists).fileName()=="config.ini");
	QVERIFY(notExists.isEmpty());
}

void TestStelFileMgr::testFindFileFile()
{
	QString exists, notExists, existsWrongType;
	try { exists = StelFileMgr::findFile("config.ini", StelFileMgr::File); } catch (std::runtime_error&) {}
	try { notExists = StelFileMgr::findFile("notexists", StelFileMgr::File); } catch (std::runtime_error&) {}
	try { existsWrongType = StelFileMgr::findFile("landscapes", StelFileMgr::File); } catch (std::runtime_error&) {}

	QVERIFY(!exists.isEmpty());
	QVERIFY(QFileInfo(exists).fileName()=="config.ini");
	QVERIFY(notExists.isEmpty());
	QVERIFY(existsWrongType.isEmpty());
}

void TestStelFileMgr::testFindFileFileAbs()
{
	QString exists, notExists, existsWrongType;
	try { exists = StelFileMgr::findFile(workingDir + "/" + partialPath1 + "/config.ini", StelFileMgr::File); } catch (std::runtime_error&) {}
	try { notExists = StelFileMgr::findFile(workingDir + "/" + partialPath2 + "/config.ini", StelFileMgr::File); } catch (std::runtime_error&) {}
	try { existsWrongType = StelFileMgr::findFile(workingDir + "/" + partialPath1 + "/landscapes", StelFileMgr::File); } catch (std::runtime_error&) {}

	QVERIFY(!exists.isEmpty());
	QVERIFY(QFileInfo(exists).fileName()=="config.ini");
	QVERIFY(notExists.isEmpty());
	QVERIFY(existsWrongType.isEmpty());
}

void TestStelFileMgr::testFindFileDir()
{
	QString exists, notExists, existsWrongType;
	try { exists = StelFileMgr::findFile("landscapes", StelFileMgr::Directory); } catch (std::runtime_error&) {}
	try { notExists = StelFileMgr::findFile("notexists", StelFileMgr::Directory); } catch (std::runtime_error&) {}
	try { existsWrongType = StelFileMgr::findFile("config.ini", StelFileMgr::Directory); } catch (std::runtime_error&) {}

	QVERIFY(!exists.isEmpty());
	QVERIFY(QFileInfo(exists).fileName()=="landscapes");
	QVERIFY(notExists.isEmpty());
	QVERIFY(existsWrongType.isEmpty());
}

void TestStelFileMgr::testFindFileDirAbs()
{
	QString exists, notExists, existsWrongType;
	try { exists = StelFileMgr::findFile(workingDir + "/" + partialPath1 + "/landscapes", StelFileMgr::Directory); } catch (std::runtime_error&) {}
	try { notExists = StelFileMgr::findFile(workingDir + "/" + partialPath1 + "/notexists", StelFileMgr::Directory); } catch (std::runtime_error&) {}
	try { existsWrongType = StelFileMgr::findFile(workingDir + "/" + partialPath1 + "/config.ini", StelFileMgr::Directory); } catch (std::runtime_error&) {}

	QVERIFY(!exists.isEmpty());
	QVERIFY(QFileInfo(exists).fileName()=="landscapes");
	QVERIFY(notExists.isEmpty());
	QVERIFY(existsWrongType.isEmpty());
}

void TestStelFileMgr::testFindFileNew()
{
	QString existsInBoth, notExistsInOne, notExistsInBoth;
	try { existsInBoth = StelFileMgr::findFile("inboth.txt", StelFileMgr::New); } catch (std::runtime_error&) {}
	try { notExistsInOne = StelFileMgr::findFile("config.ini", StelFileMgr::New); } catch (std::runtime_error&) {}
	try { notExistsInBoth = StelFileMgr::findFile("notexists", StelFileMgr::New); } catch (std::runtime_error&) {}

	QVERIFY(existsInBoth.isEmpty());

	QVERIFY(!notExistsInOne.isEmpty());
	QVERIFY(!QFileInfo(notExistsInOne).exists());
	QVERIFY(QFileInfo(notExistsInOne).fileName()=="config.ini");

	QVERIFY(!notExistsInBoth.isEmpty());
	QVERIFY(!QFileInfo(notExistsInBoth).exists());
	QVERIFY(QFileInfo(notExistsInBoth).fileName()=="notexists");
}

void TestStelFileMgr::testFindFileNewAbs()
{
	QString existsInBoth, notExistsInOne;
	try { existsInBoth = StelFileMgr::findFile(workingDir + "/" + partialPath1 + "/inboth.txt", StelFileMgr::New); } catch (std::runtime_error&) {}
	try { notExistsInOne = StelFileMgr::findFile(workingDir + "/" + partialPath2 + "/config.ini", StelFileMgr::New); } catch (std::runtime_error&) {}

	QVERIFY(existsInBoth.isEmpty());

	QVERIFY(!notExistsInOne.isEmpty());
	QVERIFY(!QFileInfo(notExistsInOne).exists());
	QVERIFY(QFileInfo(notExistsInOne).fileName()=="config.ini");
}

void TestStelFileMgr::testListContentsVanilla()
{
	QSet<QString> resultSetEmptyQuery;
	QSet<QString> resultSetDotQuery;
	QSet<QString> resultSetEmptyQueryExpected;
	try { resultSetEmptyQuery = StelFileMgr::listContents(""); } catch (std::runtime_error&) {}
	try { resultSetDotQuery = StelFileMgr::listContents("."); } catch (std::runtime_error&) {}
	resultSetEmptyQueryExpected << "config.ini" << "landscapes" << "inboth.txt";
	QVERIFY(resultSetEmptyQuery==resultSetEmptyQueryExpected);
	QVERIFY(resultSetDotQuery==resultSetEmptyQueryExpected);

	// now for some path within the hierarchy
	QSet<QString> resultSetQuery;
	QSet<QString> resultSetQueryExpected;
	try { resultSetQuery = StelFileMgr::listContents("landscapes"); } catch (std::runtime_error&) {}
	resultSetQueryExpected << "ls1" << "ls2" << "ls3" << "emptydir" << "dummy.txt";
	QVERIFY(resultSetQuery==resultSetQueryExpected);
}

void TestStelFileMgr::testListContentsVanillaAbs()
{
	QSet<QString> resultSetQuery;
	QSet<QString> resultSetQueryExpected;
	try { resultSetQuery = StelFileMgr::listContents(workingDir + "/" + partialPath2 + "/landscapes"); } catch (std::runtime_error&) {}
	resultSetQueryExpected << "ls1" << "ls3" << "dummy.txt";
	QVERIFY(resultSetQuery==resultSetQueryExpected);
}

void TestStelFileMgr::testListContentsFile()
{
	QSet<QString> resultSetEmptyQuery;
	QSet<QString> resultSetDotQuery;
	QSet<QString> resultSetEmptyQueryExpected;
	try { resultSetEmptyQuery = StelFileMgr::listContents("", StelFileMgr::File); } catch (std::runtime_error&) {}
	try { resultSetDotQuery = StelFileMgr::listContents(".", StelFileMgr::File); } catch (std::runtime_error&) {}
	resultSetEmptyQueryExpected << "config.ini" << "inboth.txt";
	QVERIFY(resultSetEmptyQuery==resultSetEmptyQueryExpected);
	QVERIFY(resultSetDotQuery==resultSetEmptyQueryExpected);

	// now for some path within the hierarchy
	QSet<QString> resultSetQuery;
	QSet<QString> resultSetQueryExpected;
	try { resultSetQuery = StelFileMgr::listContents("landscapes/ls1", StelFileMgr::File); } catch (std::runtime_error&) {}
	resultSetQueryExpected << "landscape.ini";
	QVERIFY(resultSetQuery==resultSetQueryExpected);
}

void TestStelFileMgr::testListContentsFileAbs()
{
	QSet<QString> resultSetQuery;
	QSet<QString> resultSetQueryExpected;
	try { resultSetQuery = StelFileMgr::listContents(workingDir + "/" + partialPath2 + "/landscapes", StelFileMgr::File); } catch (std::runtime_error&) {}
	resultSetQueryExpected << "dummy.txt";
	QVERIFY(resultSetQuery==resultSetQueryExpected);
}

void TestStelFileMgr::testListContentsDir()
{
	QSet<QString> resultSetEmptyQuery;
	QSet<QString> resultSetDotQuery;
	QSet<QString> resultSetEmptyQueryExpected;
	try { resultSetEmptyQuery = StelFileMgr::listContents("", StelFileMgr::Directory); } catch (std::runtime_error&) {}
	try { resultSetDotQuery = StelFileMgr::listContents(".", StelFileMgr::Directory); } catch (std::runtime_error&) {}
	resultSetEmptyQueryExpected << "landscapes";
	QVERIFY(resultSetEmptyQuery==resultSetEmptyQueryExpected);
	QVERIFY(resultSetDotQuery==resultSetEmptyQueryExpected);

	// now for some path within the hierarchy
	QSet<QString> resultSetQuery;
	QSet<QString> resultSetQueryExpected;
	try { resultSetQuery = StelFileMgr::listContents("landscapes", StelFileMgr::Directory); } catch (std::runtime_error&) {}
	resultSetQueryExpected << "ls1" << "ls2" << "ls3" << "emptydir";
	QVERIFY(resultSetQuery==resultSetQueryExpected);
}

void TestStelFileMgr::testListContentsDirAbs()
{
	QSet<QString> resultSetQuery;
	QSet<QString> resultSetQueryExpected;
	try { resultSetQuery = StelFileMgr::listContents(workingDir + "/" + partialPath2 + "/landscapes", StelFileMgr::Directory); } catch (std::runtime_error&) {}
	resultSetQueryExpected << "ls1" << "ls3";
	QVERIFY(resultSetQuery==resultSetQueryExpected);
}



