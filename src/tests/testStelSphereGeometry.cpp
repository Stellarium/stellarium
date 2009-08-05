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

#include "StelJsonParser.hpp"
#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"

#include "tests/testStelSphereGeometry.hpp"

QTEST_MAIN(TestStelSphericalGeometry)

void TestStelSphericalGeometry::initTestCase()
{
	// Testing code for new polygon code
	QVector<QVector<Vec3d> > contours;
	QVector<Vec3d> c1(4);
	StelUtils::spheToRect(-0.5, -0.5, c1[3]);
	StelUtils::spheToRect(0.5, -0.5, c1[2]);
	StelUtils::spheToRect(0.5, 0.5, c1[1]);
	StelUtils::spheToRect(-0.5, 0.5, c1[0]);
	contours.append(c1);
	QVector<Vec3d> c2(4);
	StelUtils::spheToRect(-0.2, 0.2, c2[3]);
	StelUtils::spheToRect(0.2, 0.2, c2[2]);
	StelUtils::spheToRect(0.2, -0.2, c2[1]);
	StelUtils::spheToRect(-0.2, -0.2, c2[0]);
	contours.append(c2);

	holySquare.setContours(contours);
	bigSquare.setContour(c1);
	bigSquareConvex.setContour(c1);
	QVector<Vec3d> c2inv(4);
	c2inv[0]=c2[3]; c2inv[1]=c2[2]; c2inv[2]=c2[1]; c2inv[3]=c2[0];
	smallSquare.setContour(c2inv);
	smallSquareConvex.setContour(c2inv);

	QVector<Vec3d> triCont;
	triCont << Vec3d(1,0,0) << Vec3d(0,0,1) << Vec3d(0,1,0);
	triangle.setContour(triCont);
	
	
	QVector<Vec3d> c4(4);
	StelUtils::spheToRect(M_PI-0.5, -0.5, c4[3]);
	StelUtils::spheToRect(M_PI+0.5, -0.5, c4[2]);
	StelUtils::spheToRect(M_PI+0.5, 0.5, c4[1]);
	StelUtils::spheToRect(M_PI-0.5, 0.5, c4[0]);
	opositeSquare.setContour(c4);
}

void TestStelSphericalGeometry::testSphericalCap()
{
	Vec3d p0(1,0,0);
	Vec3d p1(-1,0,0);
	Vec3d p2(1,1,1);
	p2.normalize();
	Vec3d p3(0,1,0);
	
	SphericalCap h0(p0, 0);
	SphericalCap h1(p0, 0.8);
	SphericalCap h2(p0, -0.5);
	SphericalCap h3(p1, 0.5);
	SphericalCap h4(p2, 0.8);
	SphericalCap h5(p2, 1.);
	SphericalCap h6(p1, 0);

	QVERIFY2(h0.contains(p0), "SphericalCap contains point failure");
	QVERIFY2(h1.contains(p0), "SphericalCap contains point failure");
	QVERIFY2(h0.contains(p3), "SphericalCap contains point on the edge failure");
	QVERIFY2(h6.contains(p3), "SphericalCap contains point on the edge failure");
	
	QVERIFY(h0.intersects(h1));
	QVERIFY(h0.intersects(h2));
	QVERIFY(h1.intersects(h2));
	QVERIFY(h4.intersects(h1));
	QVERIFY(!h0.intersects(h3));
	QVERIFY(!h1.intersects(h3));
	QVERIFY(h2.intersects(h3));
	QVERIFY(h0.intersects(h5));
	
	QVERIFY(h0.intersects(h0));
	QVERIFY(h1.intersects(h1));
	QVERIFY(h2.intersects(h2));
	QVERIFY(h3.intersects(h3));
	QVERIFY(h4.intersects(h4));
	QVERIFY(h5.intersects(h5));
	QVERIFY(h6.intersects(h0));
	QVERIFY(h0.intersects(h6));
	
	QVERIFY(h0.contains(h1));
	QVERIFY(!h1.contains(h0));
	QVERIFY(h2.contains(h0));
	QVERIFY(!h0.contains(h2));
	QVERIFY(!h6.contains(h0));
	QVERIFY(!h0.contains(h6));
	QVERIFY(h2.contains(h1));
	QVERIFY(!h1.contains(h2));
	QVERIFY(!h0.contains(h3));
	QVERIFY(!h1.contains(h3));
	QVERIFY(h0.contains(h5));
	QVERIFY(h2.contains(h5));
	QVERIFY(!h5.contains(h0));
	QVERIFY(!h5.contains(h1));
	QVERIFY(!h5.contains(h2));
	QVERIFY(!h5.contains(h3));
	QVERIFY(!h5.contains(h4));
	QVERIFY(h0.contains(h0));
	QVERIFY(h1.contains(h1));
	QVERIFY(h2.contains(h2));
	QVERIFY(h3.contains(h3));
	QVERIFY(h4.contains(h4));
	QVERIFY(h5.contains(h5));
}

void TestStelSphericalGeometry::benchmarkSphericalCap()
{
	Vec3d p0(1,0,0);
	Vec3d p2(1,1,1);
	p2.normalize();
	SphericalCap h0(p0, 0);
	SphericalCap h4(p2, 0.8);
	QBENCHMARK {
		h0.intersects(h4);
	}
}

void TestStelSphericalGeometry::testConsistency()
{
	QCOMPARE(bigSquare.getArea(), bigSquareConvex.getArea());
	QCOMPARE(smallSquare.getArea(), smallSquareConvex.getArea());
	QVERIFY(smallSquare.getSimplifiedContours().size()==1);
	QVERIFY(smallSquare.getSimplifiedContours()[0].size()==4);
	QVERIFY(smallSquareConvex.checkValid());
	QVERIFY(bigSquareConvex.checkValid());
	QVERIFY(triangle.checkValid());
	QVERIFY(triangle.getConvexContour().size()==3);
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
	QVERIFY2(triangle.contains(p1), "Triangle contains point failure");
	QVERIFY2(!triangle.contains(Vec3d(-1, -1, -1)), "Triangle not contains point failure");

	foreach(const SphericalCap& h, triangle.getBoundingSphericalCaps())
	{
		QVERIFY(h.contains(p1));
	}

	// polygons-point intersect
	double deg5 = 5.*M_PI/180.;
	double deg2 = 2.*M_PI/180.;
	StelUtils::spheToRect(-deg5, -deg5, v3);
	StelUtils::spheToRect(+deg5, -deg5, v2);
	StelUtils::spheToRect(+deg5, +deg5, v1);
	StelUtils::spheToRect(-deg5, +deg5, v0);
	//qDebug() << v0.toString() << v1.toString() << v2.toString() << v3.toString();
	SphericalConvexPolygon square1(v0, v1, v2, v3);
	QVERIFY(square1.checkValid());
	QVERIFY2(square1.contains(p0), "Square contains point failure");
	QVERIFY2(!square1.contains(p1), "Square not contains point failure");

	// polygons-polygons intersect
	StelUtils::spheToRect(-deg2, -deg2, v3);
	StelUtils::spheToRect(+deg2, -deg2, v2);
	StelUtils::spheToRect(+deg2, +deg2, v1);
	StelUtils::spheToRect(-deg2, +deg2, v0);
	SphericalConvexPolygon square2(v0, v1, v2, v3);
	QVERIFY(square2.checkValid());
	QVERIFY2(square1.contains(square2), "Square contains square failure");
	QVERIFY2(!square2.contains(square1), "Square not contains square failure");
	QVERIFY2(square1.intersects(square2), "Square intersect square failure");
	QVERIFY2(square2.intersects(square1), "Square intersect square failure");

	// Check when the polygons are far appart
	QVERIFY(!square1.intersects(opositeSquare));
	QVERIFY(!square2.intersects(opositeSquare));
	QVERIFY(!holySquare.intersects(opositeSquare));
	QVERIFY(!bigSquare.intersects(opositeSquare));
	QVERIFY(opositeSquare.intersects(opositeSquare));
			
	// Test the tricky case where 2 polygons intersect without having point within each other
	StelUtils::spheToRect(-deg5, -deg2, v3);
	StelUtils::spheToRect(+deg5, -deg2, v2);
	StelUtils::spheToRect(+deg5, +deg2, v1);
	StelUtils::spheToRect(-deg5, +deg2, v0);
	SphericalConvexPolygon squareHoriz(v0, v1, v2, v3);
	QVERIFY(squareHoriz.checkValid());

	StelUtils::spheToRect(-deg2, -deg5, v3);
	StelUtils::spheToRect(+deg2, -deg5, v2);
	StelUtils::spheToRect(+deg2, +deg5, v1);
	StelUtils::spheToRect(-deg2, +deg5, v0);
	SphericalConvexPolygon squareVerti(v0, v1, v2, v3);
	QVERIFY(squareVerti.checkValid());
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
	SphericalCap hx(vx, 0);
	SphericalCap hz(vz, 0);
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

void TestStelSphericalGeometry::testGreatCircleIntersection()
{
	Vec3d v0,v1,v2,v3;
	double deg5 = 5.*M_PI/180.;
	StelUtils::spheToRect(-deg5, -deg5, v3);
	StelUtils::spheToRect(+deg5, -deg5, v2);
	StelUtils::spheToRect(+deg5, +deg5, v1);
	StelUtils::spheToRect(-deg5, +deg5, v0);
	
	bool ok;
	Vec3d v(0);
	QBENCHMARK {
		v = greatCircleIntersection(v3, v1, v0, v2, ok);
	}
	QVERIFY(v.angle(Vec3d(1.,0.,0.))<0.00001);
}


void TestStelSphericalGeometry::benchmarkGetIntersection()
{
	SphericalRegionP bug1 = SphericalRegion::loadFromJson("{\"worldCoords\": [[[123.023842, -49.177087], [122.167613, -49.177087], [122.167613, -48.631248], [123.023842, -48.631248]]]}");
	SphericalRegionP bug2 = SphericalRegion::loadFromJson("{\"worldCoords\": [[[123.028902, -49.677124], [122.163995, -49.677124], [122.163995, -49.131382], [123.028902, -49.131382]]]}");
	QVERIFY(bug1->intersects(bug2));
	SphericalRegionP res;
	QBENCHMARK {
		res = SphericalRegionP::getIntersection(bug1, bug2);
	}
}

void TestStelSphericalGeometry::testEnlarge()
{
	Vec3d vx(1,0,0);
	SphericalRegionP reg(new SphericalCap(vx, 0.9));
	for (double margin=0.00000001;margin<15.;margin+=0.1)
	{
		QVERIFY(reg->getEnlarged(margin)->contains(reg));
	}
}

void TestStelSphericalGeometry::testSphericalPolygon()
{
	// Testing code for new polygon code
	QVector<QVector<Vec3d> > contours = holySquare.getSimplifiedContours();
	QVERIFY(contours.size()==2);
	QVERIFY(contours[0].size()==4);
	QVERIFY(contours[1].size()==4);

	SphericalPolygon holySquare2 = bigSquare.getSubtraction(smallSquare);
// 	QVector<QVector<Vec3d> > contours2 = holySquare2.getSimplifiedContours();
// 	QVERIFY(contours2.size()==2);
// 	QVERIFY(contours2[0].size()==4);
// 	qDebug() << holySquare.toJSON();
// 	qDebug() << holySquare2.toJSON();
// 	QVERIFY(contours2[1].size()==4);
// 	QVERIFY(contours==contours2);

// 	QCOMPARE(holySquare2.getArea(), holySquare.getArea());
// 	
// 	//Booleans methods
// 	QCOMPARE(holySquare.getArea(), bigSquare.getArea()-smallSquare.getArea());
// 	QCOMPARE(bigSquare.getUnion(holySquare).getArea(), bigSquare.getArea());
// 	QCOMPARE(bigSquare.getSubtraction(smallSquare).getArea(), bigSquare.getArea()-smallSquare.getArea());
// 	QCOMPARE(bigSquare.getIntersection(smallSquare).getArea(), smallSquare.getArea());

	// Point contain methods
	Vec3d v0, v1, v2;
	StelUtils::spheToRect(0, 0, v0);
	StelUtils::spheToRect(0.3, 0.3, v1);
	QVERIFY(smallSquareConvex.contains(v0));
	QVERIFY(smallSquare.contains(v0));
	QVERIFY(bigSquareConvex.contains(v0));
	QVERIFY(bigSquare.contains(v0));
	QVERIFY(!holySquare.contains(v0));

	QVERIFY(!smallSquare.contains(v1));
	QVERIFY(bigSquare.contains(v1));
	QVERIFY(holySquare.contains(v1));

	QVERIFY(holySquare.intersects(bigSquare));
	QVERIFY(bigSquare.intersects(smallSquare));
	QVERIFY(!holySquare.intersects(smallSquare));
	
	SphericalCap cap(Vec3d(1,0,0), 0.99);
	QVERIFY(bigSquareConvex.intersects(cap));
	
	// A case which caused a problem
	SphericalRegionP bug1 = SphericalRegion::loadFromJson("{\"worldCoords\": [[[123.023842, -49.177087], [122.167613, -49.177087], [122.167613, -48.631248], [123.023842, -48.631248]]]}");
	SphericalRegionP bug2 = SphericalRegion::loadFromJson("{\"worldCoords\": [[[123.028902, -49.677124], [122.163995, -49.677124], [122.163995, -49.131382], [123.028902, -49.131382]]]}");
	QVERIFY(bug1->intersects(bug2));
	
	// Another one
	bug1 = SphericalRegion::loadFromJson("{\"worldCoords\": [[[52.99403, -27.683551], [53.047302, -27.683551], [53.047302, -27.729923], [52.99403, -27.729923]]]}");
	bug2 = SphericalRegion::loadFromJson("{\"worldCoords\": [[[52.993701, -27.683092], [53.047302, -27.683092], [53.047302, -27.729839], [52.993701, -27.729839]]]}");
	SphericalRegionP bugIntersect = SphericalRegionP::getIntersection(bug1, bug2);

}

void TestStelSphericalGeometry::testLoading()
{
	QByteArray ar = "{\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}";
	QByteArray arTex = "{\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]]], \"textureCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]]]}";
	SphericalRegionP reg;
	SphericalRegionP regTex;
	try
	{
		reg = SphericalRegion::loadFromJson(ar);
		regTex = SphericalRegion::loadFromJson(arTex);
	}
	catch (std::runtime_error& e)
	{
		QString msg("Exception while loading: ");
		msg+=e.what();
		QFAIL(qPrintable(msg));
	}

	SphericalPolygon* poly = dynamic_cast<SphericalPolygon*>(reg.data());
	QVERIFY(poly!=NULL);

	SphericalTexturedPolygon* polyTex = dynamic_cast<SphericalTexturedPolygon*>(regTex.data());
	QVERIFY(polyTex!=NULL);

	QVector<QVector<Vec3d> > contours = poly->getSimplifiedContours();
	QVERIFY(contours.size()==2);
	QVERIFY(contours[0].size()==4);
	QVERIFY(contours[1].size()==4);
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
