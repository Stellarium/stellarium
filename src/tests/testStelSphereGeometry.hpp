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

#ifndef TESTSTELSPHERICALGEOMETRY_HPP
#define TESTSTELSPHERICALGEOMETRY_HPP

#include <QObject>
#include <QtTest>
#include "StelSphereGeometry.hpp"

//! This includes test for the SphericalPolygons and OcrtahedronPolygons.
//! Developer note, 07/2022: The classes were developed around 2009 and worked in Qt5. Changes in Qt6 causes some methods to deliver wrong results.
//! These now-buggy methods had been developed for a now-defunct plugin and are not used in versions 0.20 and later,
//! although they seem interesting and may be useful in later versions, if somebody cares to debug them.
//! The problem seems to lie in Qt's changes in the QVector/QList classes.
//! For now, some test have been disabled in testSphericalPolygon() and testOctahedronPolygon() when run on Qt6.
//!
class TestStelSphericalGeometry : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	void testOctahedronPolygon();
	void testSphericalCap();
	void testContains();
	void testPlaneIntersect2();
	void testGreatCircleIntersection();
	void testSphericalPolygon();
	void testConsistency();
	void testLoading();
	void testEnlarge();
	void benchmarkContains();
	void benchmarkCheckValid();
	void benchmarkSphericalCap();
	void benchmarkGetIntersection();
	void testSerialize();
	void benchmarkCreatePolygon();
private:
	// define a few polygonal shapes on the unit sphere. These should be Counterclockwise as seen from the inside.
	SphericalPolygon holySquare; // A counterclockwise (CCW) square with a clockwise square hole ;-)
	SphericalPolygon bigSquare;  // The same outer square
	SphericalPolygon smallSquare; // The same inner square, but reversed (to be CCW)
	SphericalPolygon opositeSquare;
	SphericalConvexPolygon bigSquareConvex;   // The same outer square
	SphericalConvexPolygon smallSquareConvex; // The same inner square, reversed
	SphericalConvexPolygon triangle;          // A triangle which looks CCW from the origin
	SphericalPolygon northPoleSquare;
	SphericalPolygon southPoleSquare;
};

#endif // _TESTSTELSPHERICALGEOMETRY_HPP
