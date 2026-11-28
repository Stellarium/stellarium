/*
 * Stellarium
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
 */

#ifndef TESTEOPMANAGER_HPP
#define TESTEOPMANAGER_HPP

#include <QObject>
#include <QtTest>

class TestEOPManager : public QObject
{
	Q_OBJECT

private slots:
	void testNames();
	void testScanAndDefaultSelection();
	void testReloadNewestWins();
	void testExplicitSelection();
	void testModels();
	void testKeysAndOrder();
	void testFinalsDoNotReplaceLongTerm();	// (SS) 2026-11-05 rule A and its option
	void testDefaultSelectionByModel();	// (SS) 2026-11-05 default selection: files of one nutation model
	void testHorizonsOverlay();	// (SS) 2026-11-06 UT1-UTC of JPL Horizons
	void testRemoveOldVersions();	// (SS) 2026-11-07 older versions of the files just downloaded
	void testUpdateAge();
	void testLastValidReport();
	void testQueries();
};

#endif // TESTEOPMANAGER_HPP
