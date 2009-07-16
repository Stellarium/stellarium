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

#include <config.h>
#include <QObject>
#include <QtDebug>
#include <QtTest>
#include <stdexcept>

#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"
#include "tests/testStelSphericalIndex.hpp"

QTEST_MAIN(TestStelSphericalIndex);

class TestRegionObject : public StelRegionObject
{
	public:
		TestRegionObject(SphericalRegionP reg) : region(reg) {;}
		virtual SphericalRegionP getRegion() const {return region;}
		SphericalRegionP region;
};

void TestStelSphericalIndex::initTestCase()
{
}

struct CountFuncObject
{
	CountFuncObject() : count(0) {;}
	void operator()(const StelRegionObjectP& obj)
	{
		count++;
	}
	int count;
};

void TestStelSphericalIndex::testBase()
{
	StelSphericalIndex grid(10);
	grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.9)))));
	CountFuncObject countFunc;
 	grid.processIntersectingRegions(SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.5)), countFunc);
	grid.processIntersectingRegions(SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.95)), countFunc);
	QVERIFY(countFunc.count==2);
}

