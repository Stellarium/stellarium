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

#ifndef _TESTSTELFILEMGR_HPP_
#define _TESTSTELFILEMGR_HPP_

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtTest>

class StelFileMgr;

class TestStelFileMgr : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	void cleanupTestCase();

	void testFindFileVanilla();
	void testFindFileVanillaAbs();
	void testFindFileFile();
	void testFindFileFileAbs();
	void testFindFileDir();
	void testFindFileDirAbs();
	void testFindFileNew();
	void testFindFileNewAbs();
	void testListContentsVanilla();
	void testListContentsVanillaAbs();
	void testListContentsFile();
	void testListContentsFileAbs();
	void testListContentsDir();
	void testListContentsDirAbs();

private:
	QString workingDir;
	QString partialPath1;
	QString partialPath2;
	QStringList testDirs;
	QStringList testFiles;
};

#endif // _TESTSTELFILEMGR_HPP_

