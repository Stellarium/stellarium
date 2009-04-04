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
#include "StelJsonParser.hpp"
#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"

#include "tests/testStelSphereGeometry.hpp"

QTEST_MAIN(TestStelSphericalGeometry);

void TestStelSphericalGeometry::initTestCase()
{
	// Testing code for new polygon code
	QVector<QVector<Vec3d> > contours;
	QVector<Vec3d> c1(4);
	StelUtils::spheToRect(-0.5, -0.5, c1[0]);
	StelUtils::spheToRect(0.5, -0.5, c1[1]);
	StelUtils::spheToRect(0.5, 0.5, c1[2]);
	StelUtils::spheToRect(-0.5, 0.5, c1[3]);
	contours.append(c1);
	QVector<Vec3d> c2(4);
	StelUtils::spheToRect(-0.2, 0.2, c2[0]);
	StelUtils::spheToRect(0.2, 0.2, c2[1]);
	StelUtils::spheToRect(0.2, -0.2, c2[2]);
	StelUtils::spheToRect(-0.2, -0.2, c2[3]);
	contours.append(c2);
	
	holySquare.setContours(contours);
	bigSquare.setContour(c1);
	QVector<Vec3d> c2inv(4);
	c2inv[0]=c2[3]; c2inv[1]=c2[2]; c2inv[2]=c2[1]; c2inv[3]=c2[0];
	smallSquare.setContour(c2inv);	
}

void TestStelSphericalGeometry::testHalfSpace()
{
	Vec3d p0(1,0,0);
	Vec3d p1(-1,0,0);
	Vec3d p2(1,1,1);
	p2.normalize();
	
	HalfSpace h0(p0, 0);
	HalfSpace h1(p0, 0.8);
	HalfSpace h2(p0, -0.5);
	HalfSpace h3(p1, 0.5);
	HalfSpace h4(p2, 0.8);
	HalfSpace h5(p2, 1.);
	
	QVERIFY2(h0.contains(p0), "HalfSpace contains point failure");
	QVERIFY2(h1.contains(p0), "HalfSpace contains point failure");
	
	QVERIFY(h0.intersects(h1));
	QVERIFY(h0.intersects(h2));
	QVERIFY(h1.intersects(h2));
	QVERIFY(h4.intersects(h1));
	QVERIFY(!h0.intersects(h3));
	QVERIFY(!h1.intersects(h3));
	QVERIFY(h2.intersects(h3));
	QVERIFY(h0.intersects(h5));
	
}

void TestStelSphericalGeometry::benchmarkHalfspace()
{
	Vec3d p0(1,0,0);
	Vec3d p2(1,1,1);
	p2.normalize();
	HalfSpace h0(p0, 0);
	HalfSpace h4(p2, 0.8);
	QBENCHMARK {
		h0.intersects(h4);
	}
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
	SphericalConvexPolygon triangle1(Vec3d(0,0,1), Vec3d(0,1,0), Vec3d(1,0,0));
	QVERIFY2(triangle1.contains(p1), "Triangle contains point failure");
	QVERIFY2(!triangle1.contains(Vec3d(-1, -1, -1)), "Triangle not contains point failure");
	
	// polygons-point intersect
	double deg5 = 5.*M_PI/180.;
	double deg2 = 2.*M_PI/180.;
	StelUtils::spheToRect(-deg5, -deg5, v0);
	StelUtils::spheToRect(+deg5, -deg5, v1);
	StelUtils::spheToRect(+deg5, +deg5, v2);
	StelUtils::spheToRect(-deg5, +deg5, v3);
	SphericalConvexPolygon square1(v3, v2, v1, v0);
	QVERIFY2(square1.contains(p0), "Square contains point failure");
	QVERIFY2(!square1.contains(p1), "Square not contains point failure");
	
	// polygons-polygons intersect
	StelUtils::spheToRect(-deg2, -deg2, v0);
	StelUtils::spheToRect(+deg2, -deg2, v1);
	StelUtils::spheToRect(+deg2, +deg2, v2);
	StelUtils::spheToRect(-deg2, +deg2, v3);
	SphericalConvexPolygon square2(v3, v2, v1, v0);
	QVERIFY2(square1.contains(square2), "Square contains square failure");
	QVERIFY2(!square2.contains(square1), "Square not contains square failure");
	QVERIFY2(square1.intersects(square2), "Square intersect square failure");
	QVERIFY2(square2.intersects(square1), "Square intersect square failure");
	
	// Test the tricky case where 2 polygons intersect without having point within each other
	StelUtils::spheToRect(-deg5, -deg2, v0);
	StelUtils::spheToRect(+deg5, -deg2, v1);
	StelUtils::spheToRect(+deg5, +deg2, v2);
	StelUtils::spheToRect(-deg5, +deg2, v3);
	SphericalConvexPolygon squareHoriz(v3, v2, v1, v0);
	StelUtils::spheToRect(-deg2, -deg5, v0);
	StelUtils::spheToRect(+deg2, -deg5, v1);
	StelUtils::spheToRect(+deg2, +deg5, v2);
	StelUtils::spheToRect(-deg2, +deg5, v3);
	SphericalConvexPolygon squareVerti(v3, v2, v1, v0);
	QVERIFY2(!squareHoriz.contains(squareVerti), "Special intersect contains failure");
	QVERIFY2(!squareVerti.contains(squareHoriz), "Special intersect contains failure");
	QVERIFY2(squareHoriz.intersects(squareVerti), "Special intersect failure");
	QVERIFY2(squareVerti.intersects(squareHoriz), "Special intersect failure");
}

void TestStelSphericalGeometry::testPlaneIntersect2()
{
	Vec3d p1,p2;
	Vec3d vx(1,0,0);
	Vec3d vz(0,0,1);
	HalfSpace hx(vx, 0);
	HalfSpace hz(vz, 0);
	QVERIFY2(planeIntersect2(hx, hz, p1, p2)==true, "Plane intersect failed");
	QVERIFY(p1==Vec3d(0,-1,0));
	QVERIFY(p2==Vec3d(0,1,0));
	QVERIFY2(planeIntersect2(hx, hx, p1, p2)==false, "Plane non-intersecting failure");
	
	hx.d = std::sqrt(2.)/2.;
	QVERIFY2(planeIntersect2(hx, hz, p1, p2)==true, "Plane/convex intersect failed");
	Vec3d res(p1-Vec3d(hx.d,-hx.d,0));
	QVERIFY2(res.length()<0.0000001, QString("p1 wrong: %1").arg(p1.toString()).toUtf8());
	res = p2-Vec3d(hx.d,hx.d,0);
	QVERIFY2(res.length()<0.0000001, QString("p2 wrong: %1").arg(p2.toString()).toUtf8());
}

void TestStelSphericalGeometry::testSphericalPolygon()
{
	// Testing code for new polygon code
	QVector<QVector<Vec3d> > contours = holySquare.getContours();
	QVERIFY(contours.size()==2);
	QVERIFY(contours[0].size()==4);
	QVERIFY(contours[1].size()==4);
	
	// Booleans methods
	QCOMPARE(bigSquare.getUnion(holySquare).getArea(), bigSquare.getArea());
	QCOMPARE(bigSquare.getSubtraction(smallSquare).getArea(), holySquare.getArea());
	QCOMPARE(bigSquare.getIntersection(smallSquare).getArea(), smallSquare.getArea());
	
	// Point contain methods
	Vec3d v0, v1, v2;
	StelUtils::spheToRect(0., 0., v0);
	StelUtils::spheToRect(0.3, 0.3, v1);
	QVERIFY(smallSquare.contains(v0));
	QVERIFY(bigSquare.contains(v0));
	QVERIFY(!holySquare.contains(v0));
	
// 	StelJsonParser parser;
// 	QBuffer buf;
// 	buf.open(QBuffer::ReadWrite);
// 	parser.write(p.toQVariant(), buf);
// 	buf.seek(0);
// 	qDebug() << buf.buffer();
// 	buf.close();
	QVERIFY(!smallSquare.contains(v1));
	QVERIFY(bigSquare.contains(v1));
	QVERIFY(holySquare.contains(v1));
	
	QVERIFY(holySquare.intersects(bigSquare));
	QVERIFY(bigSquare.intersects(smallSquare));
	QVERIFY(!holySquare.intersects(smallSquare));

	StelUtils::spheToRect(-0.5, -0.5, v0);
	StelUtils::spheToRect(0.5, -0.5, v1);
	StelUtils::spheToRect(0.5, 0.5, v2);
	SphericalConvexPolygon cvx(v2, v1, v0);
	QVERIFY(cvx.checkValid());
}

void TestStelSphericalGeometry::benchmarkContains()
{
	Vec3d v0, v1;
	StelUtils::spheToRect(0., 0., v0);
	StelUtils::spheToRect(0.3, 0.3, v1);
	
	QBENCHMARK {
		holySquare.contains(v1);
		holySquare.contains(v0);
	}
}

void TestStelSphericalGeometry::benchmarkCheckValid()
{
	Vec3d v0, v1, v2;
	StelUtils::spheToRect(-0.5, -0.5, v0);
	StelUtils::spheToRect(0.5, -0.5, v1);
	StelUtils::spheToRect(0.5, 0.5, v2);
	SphericalConvexPolygon cvx(v0, v1, v2);
	QBENCHMARK {
		cvx.checkValid();
	}
}
