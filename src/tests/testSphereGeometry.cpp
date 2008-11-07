#include <config.h>
#include <QtDebug>

#include "SphereGeometry.hpp"
#include "StelUtils.hpp"

void _assert(bool b, QString msg = "")
{
	if (!b)
	{
		qFatal("FAILED: %s",  qPrintable(msg));
	}
}

void _assertEquals(int a, int b, QString msg="")
{
	_assert(a == b, msg);
}

void testHalfSpace()
{
	Vec3d p0(1,0,0);
	
	// Half Space
	StelGeom::HalfSpace h0(p0, 0);
	_assert(contains(h0, p0), "HalfSpace contains point failure");
	StelGeom::HalfSpace h1(p0, 0.8);
	_assert(contains(h1, p0), "HalfSpace contains point failure");
	qDebug() << "Passed HalfSpace tests";
}

void testContains()
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
	_assert(contains(triangle1, p1), "Triangle contains point failure");
	_assert(intersect(triangle1, p1), "Triangle intersect point failure");
	
	// polygons-point intersect
	double deg5 = 5.*M_PI/180.;
	double deg2 = 2.*M_PI/180.;
	StelUtils::spheToRect(-deg5, -deg5, v0);
	StelUtils::spheToRect(+deg5, -deg5, v1);
	StelUtils::spheToRect(+deg5, +deg5, v2);
	StelUtils::spheToRect(-deg5, +deg5, v3);
	StelGeom::ConvexPolygon square1(v3, v2, v1, v0);
	_assert(contains(square1, p0), "Square contains point failure");
	_assert(!contains(square1, p1), "Square not contains point failure");
	_assert(intersect(square1, p0), "Square intersect point failure");
	_assert(!intersect(square1, p1), "Square not intersect point failure");
	
	// polygons-polygons intersect
	StelUtils::spheToRect(-deg2, -deg2, v0);
	StelUtils::spheToRect(+deg2, -deg2, v1);
	StelUtils::spheToRect(+deg2, +deg2, v2);
	StelUtils::spheToRect(-deg2, +deg2, v3);
	StelGeom::ConvexPolygon square2(v3, v2, v1, v0);
	_assert(contains(square1, square2), "Square contains square failure");
	_assert(!contains(square2, square1), "Square not contains square failure");
	_assert(intersect(square1, square2), "Square intersect square failure");
	_assert(intersect(square2, square1), "Square intersect square failure");
	
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
	_assert(!contains(squareHoriz, squareVerti), "Special intersect contains failure");
	_assert(!contains(squareVerti, squareHoriz), "Special intersect contains failure");
	_assert(intersect(squareHoriz, squareVerti), "Special intersect failure");
	_assert(intersect(squareVerti, squareHoriz), "Special intersect failure");
	
	
	qDebug() << "Passed intersection tests";
}

void testPlaneIntersect2()
{
	Vec3d p1,p2;
	Vec3d vx(1,0,0);
	Vec3d vz(0,0,1);
	StelGeom::HalfSpace hx(vx, 0);
	StelGeom::HalfSpace hz(vz, 0);
	_assert(StelGeom::planeIntersect2(hx, hz, p1, p2)==true, "Plane intersect failed");
	_assert(p1==Vec3d(0,-1,0));
	_assert(p2==Vec3d(0,1,0));
	_assert(StelGeom::planeIntersect2(hx, hx, p1, p2)==false, "Plane non-intersecting failure");
	
	hx.d = std::sqrt(2.)/2.;
	_assert(StelGeom::planeIntersect2(hx, hz, p1, p2)==true, "Plane/convex intersect failed");
	Vec3d res(p1-Vec3d(hx.d,-hx.d,0));
	_assert(res.length()<0.0000001, QString("p1 wrong: %1").arg(p1.toString()));
	res = p2-Vec3d(hx.d,hx.d,0);
	_assert(res.length()<0.0000001, QString("p2 wrong: %1").arg(p2.toString()));
}

/************************************************************************
 Run several of the time-related functions through paces.
************************************************************************/
int main(int argc, char* argv[])
{
	testHalfSpace();
	testContains();
	testPlaneIntersect2();
}

