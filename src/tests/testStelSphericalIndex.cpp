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

#include "tests/testStelSphericalIndex.hpp"

#include <QObject>
#include <QDebug>

#include <stdexcept>

#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"


QTEST_GUILESS_MAIN(TestStelSphericalIndex)

class TestRegionObject : public StelRegionObject
{
	public:
		TestRegionObject(SphericalRegionP reg) : region(reg) {;}
		virtual SphericalRegionP getRegion() const { return region; }
		virtual Vec3d getPointInRegion() const { return Vec3d(0,0,0); }
		SphericalRegionP region;
};

void TestStelSphericalIndex::initTestCase()
{
}

struct CountFuncObject
{
	CountFuncObject() : count(0) {;}
	void operator()(const StelRegionObject* /* obj */)
	{
		count++;
	}
	int count;
};

void TestStelSphericalIndex::testBase()
{
	StelSphericalIndex grid(10);
	grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.9)))));
	grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalCap(Vec3d(-1,0,0), 0.99)))));
	CountFuncObject countFunc;
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.5))), countFunc);
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.95))), countFunc);
	QVERIFY(countFunc.count==2);
	countFunc.count=0;
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalCap(Vec3d(0,1,0), 0.99))), countFunc);
	QVERIFY(countFunc.count==0);
	
	// Process all
	countFunc.count=0;
	grid.processAll(countFunc);
	QVERIFY(countFunc.count==2);
	
	// Clear
	grid.clear();
	countFunc.count=0;
	grid.processAll(countFunc);
	QVERIFY(countFunc.count==0);
	
	QVector<Vec3d> c1(4);
	StelUtils::spheToRect(-0.5, -0.5, c1[3]);
	StelUtils::spheToRect(0.5, -0.5, c1[2]);
	StelUtils::spheToRect(0.5, 0.5, c1[1]);
	StelUtils::spheToRect(-0.5, 0.5, c1[0]);
	SphericalConvexPolygon bigSquareConvex;
	bigSquareConvex.setContour(c1);

	QVector<Vec3d> c2(4);
	StelUtils::spheToRect(-0.5, -0.5, c2[3]);
	StelUtils::spheToRect(0.5, -0.5, c2[2]);
	StelUtils::spheToRect(0.5, 0.5, c2[1]);
	StelUtils::spheToRect(-0.5, 0.5, c2[0]);
	SphericalPolygon bigConvex;
	bigConvex.setContour(c2);
	
	// try with many elements
	for (int i=0;i<10000;++i)
	{
		grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.99)))));
		grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalPoint(Vec3d(1,0,0))))));
		grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalConvexPolygon(c1)))));
		grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalPolygon(c2)))));
	}
	countFunc.count=0;
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.5))), countFunc);
	QVERIFY(countFunc.count==40000);
	countFunc.count=0;
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalConvexPolygon(c1))), countFunc);
	qDebug() << countFunc.count;
	QVERIFY(countFunc.count==40000);
	countFunc.count=0;
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalPolygon(c2))), countFunc);
	qDebug() << countFunc.count;
	QVERIFY(countFunc.count==40000);
}

