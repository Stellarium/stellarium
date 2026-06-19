/*
 * Stellarium
 * Copyright (C) 2026 Stellarium Developers
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

#ifndef TESTDYNAMICLOADING_HPP
#define TESTDYNAMICLOADING_HPP

#include <QTest>

class DynamicZoneArray;

template<class Star>
class DynamicZoneArray;

class TestDynamicLoading : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void cleanupTestCase();

	void testSearchPhase1HealpixZone();
	void testSearchGaiaIDLoadsZone();
	void testCacheReuse();

private:
	DynamicZoneArray<Star2>* stars9  = nullptr;
	DynamicZoneArray<Star2>* stars10 = nullptr;
};

#endif // TESTDYNAMICLOADING_HPP
