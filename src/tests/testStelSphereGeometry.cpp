#include <config.h>

#include <QObject>
#include <QtDebug>
#include <QtTest>
#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"

#include "tests/testStelSphereGeometry.hpp"

QTEST_MAIN(TestStelSphericalGeometry);

void TestStelSphericalGeometry::testHalfSpace()
{
	Vec3d p0(1,0,0);
	StelGeom::HalfSpace h0(p0, 0);
	QVERIFY2(contains(h0, p0), "HalfSpace contains point failure");
	StelGeom::HalfSpace h1(p0, 0.8);
	QVERIFY2(contains(h1, p0), "HalfSpace contains point failure");
}

void TestStelSphericalGeometry::testContains()
{
	Vec3d p0(1,0,0);
	Vec3d p1(1,1,1);
	p1.normalize();
	
	Vec3d v0;
	Vec3d v1;
	Vec3d v2;
	Vec3d v3;
	
	// Triangle polygons
	StelGeom::ConvexPolygon triangle1(Vec3d(0,0,1), Vec3d(0,1,0), Vec3d(1,0,0));
	QVERIFY2(contains(triangle1, p1), "Triangle contains point failure");
	QVERIFY2(intersect(triangle1, p1), "Triangle intersect point failure");
	
	// polygons-point intersect
	double deg5 = 5.*M_PI/180.;
	double deg2 = 2.*M_PI/180.;
	StelUtils::spheToRect(-deg5, -deg5, v0);
	StelUtils::spheToRect(+deg5, -deg5, v1);
	StelUtils::spheToRect(+deg5, +deg5, v2);
	StelUtils::spheToRect(-deg5, +deg5, v3);
	StelGeom::ConvexPolygon square1(v3, v2, v1, v0);
	QVERIFY2(contains(square1, p0), "Square contains point failure");
	QVERIFY2(!contains(square1, p1), "Square not contains point failure");
	QVERIFY2(intersect(square1, p0), "Square intersect point failure");
	QVERIFY2(!intersect(square1, p1), "Square not intersect point failure");
	
	// polygons-polygons intersect
	StelUtils::spheToRect(-deg2, -deg2, v0);
	StelUtils::spheToRect(+deg2, -deg2, v1);
	StelUtils::spheToRect(+deg2, +deg2, v2);
	StelUtils::spheToRect(-deg2, +deg2, v3);
	StelGeom::ConvexPolygon square2(v3, v2, v1, v0);
	QVERIFY2(contains(square1, square2), "Square contains square failure");
	QVERIFY2(!contains(square2, square1), "Square not contains square failure");
	QVERIFY2(intersect(square1, square2), "Square intersect square failure");
	QVERIFY2(intersect(square2, square1), "Square intersect square failure");
	
	// Test the tricky case where 2 polygons intersect without having point within each other
	StelUtils::spheToRect(-deg5, -deg2, v0);
	StelUtils::spheToRect(+deg5, -deg2, v1);
	StelUtils::spheToRect(+deg5, +deg2, v2);
	StelUtils::spheToRect(-deg5, +deg2, v3);
	StelGeom::ConvexPolygon squareHoriz(v3, v2, v1, v0);
	StelUtils::spheToRect(-deg2, -deg5, v0);
	StelUtils::spheToRect(+deg2, -deg5, v1);
	StelUtils::spheToRect(+deg2, +deg5, v2);
	StelUtils::spheToRect(-deg2, +deg5, v3);
	StelGeom::ConvexPolygon squareVerti(v3, v2, v1, v0);
	QVERIFY2(!contains(squareHoriz, squareVerti), "Special intersect contains failure");
	QVERIFY2(!contains(squareVerti, squareHoriz), "Special intersect contains failure");
	QVERIFY2(intersect(squareHoriz, squareVerti), "Special intersect failure");
	QVERIFY2(intersect(squareVerti, squareHoriz), "Special intersect failure");
}

void TestStelSphericalGeometry::testPlaneIntersect2()
{
	Vec3d p1,p2;
	Vec3d vx(1,0,0);
	Vec3d vz(0,0,1);
	StelGeom::HalfSpace hx(vx, 0);
	StelGeom::HalfSpace hz(vz, 0);
	QVERIFY2(StelGeom::planeIntersect2(hx, hz, p1, p2)==true, "Plane intersect failed");
	QVERIFY(p1==Vec3d(0,-1,0));
	QVERIFY(p2==Vec3d(0,1,0));
	QVERIFY2(StelGeom::planeIntersect2(hx, hx, p1, p2)==false, "Plane non-intersecting failure");
	
	hx.d = std::sqrt(2.)/2.;
	QVERIFY2(StelGeom::planeIntersect2(hx, hz, p1, p2)==true, "Plane/convex intersect failed");
	Vec3d res(p1-Vec3d(hx.d,-hx.d,0));
	QVERIFY2(res.length()<0.0000001, QString("p1 wrong: %1").arg(p1.toString()).toUtf8());
	res = p2-Vec3d(hx.d,hx.d,0);
	QVERIFY2(res.length()<0.0000001, QString("p2 wrong: %1").arg(p2.toString()).toUtf8());
}
