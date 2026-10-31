/*
 * Copyright (C) 2026 Sylvain Simard
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
 * 
 * (SS) 2026-10-03
 * Unit tests for BSPManager
 */

#ifndef TESTBSPMANAGER_HPP
#define TESTBSPMANAGER_HPP

#include <QObject>
#include <QtTest>
#include <QTemporaryDir>

class TestBSPManager : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase();
	void testRescan();
	void testTTminusTDB();
	void testNoKernel();
	void testReaderReuse();
	void testRefresh();
	void testMissingDirectory();
	void testDumpSegments();

private:
	QTemporaryDir tmpDir;
	QString goodDir;  //!< de431t.bsp, tt_alt.bsp, planets.bsp
	QString mixedDir; //!< the same plus a corrupt .bsp and a non-.bsp file
};

#endif // TESTBSPMANAGER_HPP
