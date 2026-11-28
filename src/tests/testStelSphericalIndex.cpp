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
#include "StelRegionObject.hpp"


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
	StelUtils::spheToRect( 0.5, -0.5, c1[2]);
	StelUtils::spheToRect( 0.5,  0.5, c1[1]);
	StelUtils::spheToRect(-0.5,  0.5, c1[0]);
	SphericalConvexPolygon bigSquareConvex;
	bigSquareConvex.setContour(c1);

	/*
	 * Unit test for spherical polygon was added in 2019
	SphericalPolygon bigConvex;
	bigConvex.setContour(c1);
	*/
	
	// try with many elements
	for (int i=0;i<10000;++i)
	{
		grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.99)))));
		grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalPoint(Vec3d(1,0,0))))));
		grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalConvexPolygon(c1)))));
		//grid.insert(StelRegionObjectP(new TestRegionObject(SphericalRegionP(new SphericalPolygon(c1)))));
	}
	int j = 30000;
	countFunc.count=0;
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalCap(Vec3d(1,0,0), 0.5))), countFunc);
	qDebug() << countFunc.count;
	QVERIFY(countFunc.count==j);
	countFunc.count=0;
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalConvexPolygon(c1))), countFunc);
	qDebug() << countFunc.count;
	QVERIFY(countFunc.count==j);
	/*
	 * Disable unit test for spherical polygon
	 * FIXME: why it works for GCC/MSVC/Clang and not for AppleClang?
	countFunc.count=0;
	grid.processIntersectingRegions(&(*SphericalRegionP(new SphericalPolygon(c1))), countFunc);
	qDebug() << countFunc.count;
	QVERIFY(countFunc.count==j);
	*/
}


class TestPointObject : public StelRegionObject
{
	public:
		TestPointObject(const Vec3d& aPos, float aKey) : pos(aPos), point(new SphericalPoint(aPos)), key(aKey) {;}
		SphericalRegionP getRegion() const override { return point; }
		Vec3d getPointInRegion() const override { return pos; }
		Vec3d pos;
		SphericalRegionP point;
		float key;
};

struct CollectFuncObject
{
	void operator()(const StelRegionObject* obj)
	{
		objects.append(obj);
	}
	QVector<const StelRegionObject*> objects;
};

struct KeyFilterFuncObject
{
	KeyFilterFuncObject(float aMaxKey) : maxKey(aMaxKey) {;}
	void operator()(const StelRegionObject* obj)
	{
		if (static_cast<const TestPointObject*>(obj)->key <= maxKey)
			objects.append(obj);
	}
	float maxKey;
	QVector<const StelRegionObject*> objects;
};

void TestStelSphericalIndex::testKeyLimitedProcessing()
{
	// Many points spread over the sky, with keys from -2 to 20, so that the grid gets split down to several levels.
	StelSphericalIndex grid(20);
	QVector<StelRegionObjectP> objects;
	for (int i=0;i<20000;++i)
	{
		Vec3d pos;
		StelUtils::spheToRect(static_cast<double>(i)*2.39996, std::asin(2.*((i*7919)%20000)/20000.-1.), pos);
		const float key = static_cast<float>((i*104729)%2300)/100.f - 2.f;
		StelRegionObjectP obj(new TestPointObject(pos, key));
		objects.append(obj);
		grid.insert(obj, key);
	}

	Vec3d capCenter(1., 0.3, 0.2);
	capCenter.normalize();
	const SphericalRegionP region(new SphericalCap(capCenter, 0.7));
	for (float maxKey : {-3.f, -2.f, 0.f, 3.5f, 9.99f, 21.f})
	{
		// Reference: all objects of the region, filtered by key afterwards
		KeyFilterFuncObject reference(maxKey);
		grid.processIntersectingPointInRegions(region.data(), reference);
		CollectFuncObject limited;
		grid.processIntersectingPointInRegions(region.data(), maxKey, limited);
		// Same objects in the same order
		QCOMPARE(limited.objects, reference.objects);

		KeyFilterFuncObject referenceAll(maxKey);
		grid.processAll(referenceAll);
		CollectFuncObject limitedAll;
		grid.processIntersectingPointInRegions(AllSkySphericalRegion::staticInstance.data(), maxKey, limitedAll);
		QCOMPARE(limitedAll.objects, referenceAll.objects);
	}

	// Changing the keys keeps the processing order and uses the new keys
	auto negatedKey = [](const StelRegionObject* obj) { return -static_cast<const TestPointObject*>(obj)->key; };
	grid.updateKeys(negatedKey);
	for (auto& obj : objects)
		static_cast<TestPointObject*>(obj.data())->key = -static_cast<TestPointObject*>(obj.data())->key;
	for (float maxKey : {-21.f, -10.f, 0.f, 1.5f, 2.f})
	{
		KeyFilterFuncObject reference(maxKey);
		grid.processIntersectingPointInRegions(region.data(), reference);
		CollectFuncObject limited;
		grid.processIntersectingPointInRegions(region.data(), maxKey, limited);
		QCOMPARE(limited.objects, reference.objects);
	}
}
