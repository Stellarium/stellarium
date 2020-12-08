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

#include "tests/testStelSphereGeometry.hpp"

#include <QObject>
#include <QtDebug>
#include <QBuffer>

#include <stdexcept>

#include "StelJsonParser.hpp"
#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"

QTEST_GUILESS_MAIN(TestStelSphericalGeometry)

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

	QVector<Vec3d> cpole(4);
	StelUtils::spheToRect(0.1,M_PI/2.-0.1, cpole[3]);
	StelUtils::spheToRect(0.1+M_PI/2., M_PI/2.-0.1, cpole[2]);
	StelUtils::spheToRect(0.1+M_PI, M_PI/2.-0.1, cpole[1]);
	StelUtils::spheToRect(0.1+M_PI+M_PI/2.,M_PI/2.-0.1, cpole[0]);
	northPoleSquare.setContour(cpole);

	StelUtils::spheToRect(0.1,-M_PI/2.+0.1, cpole[0]);
	StelUtils::spheToRect(0.1+M_PI/2., -M_PI/2.+0.1, cpole[1]);
	StelUtils::spheToRect(0.1+M_PI, -M_PI/2.+0.1, cpole[2]);
	StelUtils::spheToRect(0.1+M_PI+M_PI/2.,-M_PI/2.+0.1, cpole[3]);
	southPoleSquare.setContour(cpole);
}

void TestStelSphericalGeometry::testSphericalCap()
{
	Vec3d p0(1.,0.,0.);
	Vec3d p1(-1.,0.,0.);
	Vec3d p2(1.,1.,1.);
	p2.normalize();
	Vec3d p3(0.,1.,0.);

	SphericalCap h0(p0, 0.);
	SphericalCap h1(p0, 0.8);
	SphericalCap h2(p0, -0.5);
	SphericalCap h3(p1, 0.5);
	SphericalCap h4(p2, 0.8);
	SphericalCap h5(p2, 1.);
	SphericalCap h6(p1, 0.);

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
	#ifndef __MINGW32__
	// NOTE: It fails on Windows/MinGW GCC
	QVERIFY(h5.intersects(h5));
	#endif
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
	#ifndef __MINGW32__
	// NOTE: It fails on Windows/MinGW GCC
	QVERIFY(h4.contains(h4));	
	QVERIFY(h5.contains(h5));
	#endif
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
	Vec3d vt(-1, -1, -1);
	vt.normalize();
	QVERIFY2(!triangle.contains(vt), "Triangle not contains point failure");

	for (const auto& h : triangle.getBoundingSphericalCaps())
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
	QVERIFY2(SphericalCap::intersectionPoints(hx, hz, p1, p2)==true, "Plane intersect failed");
	QVERIFY(p1==Vec3d(0,-1,0));
	QVERIFY(p2==Vec3d(0,1,0));
	QVERIFY2(SphericalCap::intersectionPoints(hx, hx, p1, p2)==false, "Plane non-intersecting failure");

	hx.d = std::sqrt(2.)/2.;
	QVERIFY2(SphericalCap::intersectionPoints(hx, hz, p1, p2)==true, "Plane/convex intersect failed");
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
	Vec3d v(0.0);
	QBENCHMARK {
		v = greatCircleIntersection(v3, v1, v0, v2, ok);
	}
	QVERIFY(v.angle(Vec3d(1.,0.,0.))<0.00001);
}


void TestStelSphericalGeometry::benchmarkGetIntersection()
{
	SphericalRegionP bug1 = SphericalRegionP::loadFromJson("{\"worldCoords\": [[[123.023842, -49.177087], [122.167613, -49.177087], [122.167613, -48.631248], [123.023842, -48.631248]]]}");
	SphericalRegionP bug2 = SphericalRegionP::loadFromJson("{\"worldCoords\": [[[123.028902, -49.677124], [122.163995, -49.677124], [122.163995, -49.131382], [123.028902, -49.131382]]]}");
	QVERIFY(bug1->intersects(bug2));
	SphericalRegionP res;
	QBENCHMARK {
		res = bug1->getIntersection(bug2);
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
	SphericalRegionP holySquare2 = bigSquare.getSubtraction(smallSquare);

	QCOMPARE(holySquare2->getArea(), holySquare.getArea());

	//Booleans methods
	QCOMPARE(holySquare.getArea(), bigSquare.getArea()-smallSquare.getArea());
	QCOMPARE(bigSquare.getUnion(holySquare)->getArea(), bigSquare.getArea());
	QCOMPARE(bigSquare.getSubtraction(smallSquare)->getArea(), bigSquare.getArea()-smallSquare.getArea());
	QCOMPARE(bigSquare.getIntersection(smallSquare)->getArea(), smallSquare.getArea());

	// Point contain methods
	Vec3d v0, v1;
	StelUtils::spheToRect(0.00000, 0.00000, v0);
	StelUtils::spheToRect(0.3, 0.3, v1);
	QVERIFY(smallSquareConvex.contains(v0));
	QVERIFY(smallSquare.contains(v0));
	QVERIFY(bigSquareConvex.contains(v0));
	QVERIFY(bigSquare.contains(v0));
	//FIXME: sometimes give errors on 32-bit systems when unit tests call by CLI. WTF?!?
	//QVERIFY(!holySquare.contains(v0));

	QVERIFY(!smallSquare.contains(v1));
	QVERIFY(bigSquare.contains(v1));
	QVERIFY(holySquare.contains(v1));

	QVERIFY(holySquare.intersects(bigSquare));
	QVERIFY(bigSquare.intersects(smallSquare));
	QVERIFY(!holySquare.intersects(smallSquare));

	SphericalCap cap(Vec3d(1,0,0), 0.99);
	QVERIFY(bigSquareConvex.intersects(cap));

	// A case which caused a problem
	SphericalRegionP bug1 = SphericalRegionP::loadFromJson("{\"worldCoords\": [[[123.023842, -49.177087], [122.167613, -49.177087], [122.167613, -48.631248], [123.023842, -48.631248]]]}");
	SphericalRegionP bug2 = SphericalRegionP::loadFromJson("{\"worldCoords\": [[[123.028902, -49.677124], [122.163995, -49.677124], [122.163995, -49.131382], [123.028902, -49.131382]]]}");
	QVERIFY(bug1->intersects(bug2));

	// Another one
	bug1 = SphericalRegionP::loadFromJson("{\"worldCoords\": [[[52.99403, -27.683551], [53.047302, -27.683551], [53.047302, -27.729923], [52.99403, -27.729923]]]}");
	bug2 = SphericalRegionP::loadFromJson("{\"worldCoords\": [[[52.993701, -27.683092], [53.047302, -27.683092], [53.047302, -27.729839], [52.993701, -27.729839]]]}");
	SphericalRegionP bugIntersect = bug1->getIntersection(bug2);
}

void TestStelSphericalGeometry::testLoading()
{
	QByteArray ar = "{\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]], [[-0.2,-0.2],[0.2,-0.2],[0.2,0.2],[-0.2,0.2]]]}";
	//QByteArray arTex = "{\"worldCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]]], \"textureCoords\": [[[-0.5,0.5],[0.5,0.5],[0.5,-0.5],[-0.5,-0.5]]]}";
	SphericalRegionP reg;
	//SphericalRegionP regTex;
	try
	{
		reg = SphericalRegionP::loadFromJson(ar);
//		regTex = SphericalRegionP::loadFromJson(arTex);
	}
	catch (std::runtime_error& e)
	{
		QString msg("Exception while loading: ");
		msg+=e.what();
		QFAIL(qPrintable(msg));
	}

	QVERIFY(reg->getType()==SphericalRegion::Polygon);
	qDebug() << reg->getArea()*M_180_PI*M_180_PI;

	// FIXME: WTF?!?
	//StelVertexArray vertexAr = reg->getOutlineVertexArray();
	//QVERIFY(vertexAr.primitiveType==StelVertexArray::Lines && vertexAr.vertex.size()%2==0);
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

void TestStelSphericalGeometry::testOctahedronPolygon()
{
	QVERIFY(OctahedronPolygon::triangleContains2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0), Vec3d(0.8,0.1,0)));
	QVERIFY(OctahedronPolygon::triangleContains2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0), Vec3d(1,0.1,0)));
	QVERIFY(OctahedronPolygon::triangleContains2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0), Vec3d(0.5,0.,0)));

	// Check points outside triangle
	QVERIFY(!OctahedronPolygon::triangleContains2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0), Vec3d(0.,0.1,0)));
	QVERIFY(!OctahedronPolygon::triangleContains2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0), Vec3d(-1.,-1.,0)));

	// Check that the corners are included into the triangle
	QVERIFY(OctahedronPolygon::triangleContains2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0), Vec3d(0,0,0)));
	QVERIFY(OctahedronPolygon::triangleContains2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0), Vec3d(1,0,0)));
	QVERIFY(OctahedronPolygon::triangleContains2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0), Vec3d(1,1,0)));

	QVERIFY(OctahedronPolygon::isTriangleConvexPositive2D(Vec3d(0,0,0), Vec3d(1,0,0), Vec3d(1,1,0)));

	SubContour contour(smallSquareConvex.getConvexContour());
	OctahedronPolygon splittedSub(contour);
	QCOMPARE(splittedSub.getArea(), smallSquareConvex.getArea());

	QVector<Vec3d> va = northPoleSquare.getOutlineVertexArray().vertex;
	QCOMPARE(va.size(),16);
	va = southPoleSquare.getOutlineVertexArray().vertex;
	QCOMPARE(va.size(),16);

	// Copy
	OctahedronPolygon splittedSubCopy;
	splittedSubCopy = splittedSub;

	QCOMPARE(splittedSub.getArea(), splittedSubCopy.getArea());
	double oldArea = splittedSubCopy.getArea();
	splittedSub = OctahedronPolygon();
	QCOMPARE(splittedSub.getArea(), 0.);
	QCOMPARE(splittedSubCopy.getArea(), oldArea);
	splittedSubCopy.inPlaceIntersection(splittedSub);
	QCOMPARE(splittedSubCopy.getArea(), 0.);

	QCOMPARE(southPoleSquare.getArea(), northPoleSquare.getArea());
	QCOMPARE(southPoleSquare.getIntersection(northPoleSquare)->getArea(), 0.);
	QCOMPARE(southPoleSquare.getUnion(northPoleSquare)->getArea(), 2.*southPoleSquare.getArea());
	QCOMPARE(southPoleSquare.getSubtraction(northPoleSquare)->getArea(), southPoleSquare.getArea());

	QCOMPARE(northPoleSquare.getIntersection(northPoleSquare)->getArea(), northPoleSquare.getArea());
	QCOMPARE(northPoleSquare.getUnion(northPoleSquare)->getArea(), northPoleSquare.getArea());
	QCOMPARE(northPoleSquare.getSubtraction(northPoleSquare)->getArea(), 0.);

	// Test binary IO
	QByteArray ar;
	QBuffer buf(&ar);
	buf.open(QIODevice::WriteOnly);
	QDataStream out(&buf);
	out << northPoleSquare.getOctahedronPolygon();
	buf.close();
	QVERIFY(!ar.isEmpty());

	// Re-read it
	OctahedronPolygon northPoleSquareRead;
	buf.open(QIODevice::ReadOnly);
	QDataStream in(&buf);
	in >> northPoleSquareRead;
	buf.close();
	QVERIFY(!northPoleSquareRead.isEmpty());
	QCOMPARE(northPoleSquareRead.getArea(), northPoleSquare.getArea());
	QVERIFY(northPoleSquareRead.intersects(northPoleSquare.getOctahedronPolygon()));

	// Spherical cap with aperture > 90 deg
	SphericalCap cap1(Vec3d(0,0,1), std::cos(95.*M_PI/180.));
	qDebug() << "---------------------";
	OctahedronPolygon northCapPoly = cap1.getOctahedronPolygon();
	qDebug() << "---------------------";
	qDebug() << northCapPoly.getArea() << OctahedronPolygon::getAllSkyOctahedronPolygon().getArea()/2;
	QVERIFY(northCapPoly.getArea()>OctahedronPolygon::getAllSkyOctahedronPolygon().getArea()/2);
}


void TestStelSphericalGeometry::testSerialize()
{
	// Store a SphericalPolygon as QVariant
	SphericalRegionP holyReg(new SphericalPolygon(holySquare));
	QVariant vHolyReg = QVariant::fromValue(holyReg);
	QVERIFY(QString(vHolyReg.typeName())=="SphericalRegionP");
	QVERIFY(vHolyReg.canConvert<SphericalRegionP>());
	// and reconvert it
	SphericalRegionP reg2 = vHolyReg.value<SphericalRegionP>();
	QCOMPARE(holyReg->getArea(), reg2->getArea());
	QVERIFY(holyReg->getType()==reg2->getType());

	// Store a SphericalCap as QVariant
	SphericalRegionP capReg(new SphericalCap(Vec3d(1,0,0), 0.12));
	QVariant vCapReg = QVariant::fromValue(capReg);
	QVERIFY(QString(vCapReg.typeName())=="SphericalRegionP");
	QVERIFY(vCapReg.canConvert<SphericalRegionP>());
	// and reconvert it
	reg2 = vCapReg.value<SphericalRegionP>();
	QCOMPARE(capReg->getArea(), reg2->getArea());
	QVERIFY(capReg->getType()==reg2->getType());

	// Test serialize the QVariants as binary
	QByteArray ar;
	QBuffer buf(&ar);
	buf.open(QIODevice::WriteOnly);
	QDataStream out(&buf);
	out << vHolyReg << vCapReg;
	buf.close();
	QVERIFY(!ar.isEmpty());

	// Re-read it
	QVariant readVCapReg, readVHolyReg;
	buf.open(QIODevice::ReadOnly);
	QDataStream in(&buf);
	in >> readVHolyReg >> readVCapReg;
	buf.close();
	reg2 = readVHolyReg.value<SphericalRegionP>();
	QCOMPARE(holyReg->getArea(), reg2->getArea());
	QVERIFY(holyReg->getType()==reg2->getType());
	reg2 = readVCapReg.value<SphericalRegionP>();
	QCOMPARE(capReg->getArea(), reg2->getArea());
	QVERIFY(capReg->getType()==reg2->getType());
}

void TestStelSphericalGeometry::benchmarkCreatePolygon()
{
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
	QBENCHMARK
	{
		SphericalPolygon holySquare(contours);
	}
}
