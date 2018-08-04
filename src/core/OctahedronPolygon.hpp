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

#ifndef OCTAHEDRONREGION_HPP
#define OCTAHEDRONREGION_HPP

#include "StelVertexArray.hpp"
#include "VecMath.hpp"

#include <QVector>
#include <QDebug>
#include <QVarLengthArray>

//! @struct EdgeVertex
//! Describe a vertex composing polygon contours, and whether it belong to an edge or not.
struct EdgeVertex
{
	EdgeVertex() : edgeFlag(false) {;}
	EdgeVertex(bool b) : edgeFlag(b) {;}
	EdgeVertex(const Vec3d& v, bool b) : vertex(v), edgeFlag(b) {;}
	//! The normalized vertex direction.
	Vec3d vertex;
	//! Set to true if the vertex is part of at least one edge segment, i.e. it lays on the boundary of the polygon.
	bool edgeFlag;
};

Q_DECLARE_TYPEINFO(EdgeVertex, Q_PRIMITIVE_TYPE);

QDataStream& operator<<(QDataStream& out, const EdgeVertex&);
QDataStream& operator>>(QDataStream& in, EdgeVertex&);

class SubContour : public QVector<EdgeVertex>
{
public:
	// Create a SubContour from a list of vertices
	SubContour(const QVector<Vec3d>& vertices, bool closed=true);
	SubContour() {;}
	SubContour(int size, const EdgeVertex& v) : QVector<EdgeVertex>(size, v) {;}
	SubContour reversed() const;
	QString toJSON() const;
};

//! @class OctahedronPolygon
//! Manage a non-convex polygon which can extends on more than 180 deg.
//! The contours defining the polygon are splitted and projected on the 8 sides of an Octahedron to enable 2D geometry
//! algorithms to be used.
class OctahedronPolygon
{
public:
	OctahedronPolygon() : fillCachedVertexArray(StelVertexArray::Triangles), outlineCachedVertexArray(StelVertexArray::Lines), capN(1,0,0), capD(-2.)
	{sides.resize(8);}

	//! Create the OctahedronPolygon by splitting the passed SubContour on the 8 sides of the octahedron.
	OctahedronPolygon(const SubContour& subContour);
	OctahedronPolygon(const QVector<QVector<Vec3d> >& contours);
	OctahedronPolygon(const QVector<Vec3d>& contour);
	OctahedronPolygon(const QList<OctahedronPolygon>& octContours);

	double getArea() const;

	Vec3d getPointInside() const;

	//! Returns the list of triangles resulting from tesselating the contours.
	StelVertexArray getFillVertexArray() const {return fillCachedVertexArray;}
	StelVertexArray getOutlineVertexArray() const {return outlineCachedVertexArray;}

	void getBoundingCap(Vec3d& v, double& d) const {v=capN; d=capD;}

	//! Set this OctahedronPolygon as the intersection of itself with the given OctahedronPolygon.
	void inPlaceIntersection(const OctahedronPolygon& mpoly);
	//! Set this OctahedronPolygon as the union of itself with the given OctahedronPolygon.
	void inPlaceUnion(const OctahedronPolygon& mpoly);
	//! Set this OctahedronPolygon as the subtraction of itself with the given OctahedronPolygon.
	void inPlaceSubtraction(const OctahedronPolygon& mpoly);

	bool intersects(const OctahedronPolygon& mpoly) const;
	bool contains(const OctahedronPolygon& mpoly) const;

	bool contains(const Vec3d& p) const;
	bool isEmpty() const;

	static const OctahedronPolygon& getAllSkyOctahedronPolygon();
	static const OctahedronPolygon& getEmptyOctahedronPolygon() {static OctahedronPolygon poly; return poly;}

	static double sphericalTriangleArea(const Vec3d& v0, const Vec3d& v1, const Vec3d& v2)
	{
		const Vec3d& p1 = v0 ^ v1;
		const Vec3d& p2 = v1 ^ v2;
		const Vec3d& p3 = v2 ^ v0;
		return 2.*M_PI - p1.angle(p2) - p2.angle(p3) - p3.angle(p1);
	}

	QString toJson() const;

private:
	// For unit tests
	friend class TestStelSphericalGeometry;

	friend QDataStream& operator<<(QDataStream&, const OctahedronPolygon&);
	friend QDataStream& operator>>(QDataStream&, OctahedronPolygon&);
	friend OctahedronPolygon createAllSkyOctahedronPolygon();

	//! Creates a full Octahedron.
	static OctahedronPolygon createAllSkyOctahedronPolygon();

	//! Append all theOctahedronPolygonach octahedron sides. No tesselation occurs at this point,
	//! and a call to tesselatOctahedronPolygon each appended SubContours per side.
	void append(const OctahedronPolygon& other);
	void appendReversed(const OctahedronPolygon& other);
	void appendSubContour(const SubContour& contour);

	enum TessWindingRule
	{
		WindingPositive=0,	//!< Positive winding rule (used for union)
		WindingAbsGeqTwo=1	//!< Abs greater or equal 2 winding rule (used for intersection)
	};

	bool sideContains2D(const Vec3d& p, int sideNb) const;

	//! Tesselate the contours per side, producing (in @var sides) a list of triangles subcontours according to the given rule.
	void tesselate(TessWindingRule rule);

	QVector<SubContour> tesselateOneSideLineLoop(struct GLUEStesselator* tess, int sidenb) const;
	QVector<Vec3d> tesselateOneSideTriangles(struct GLUEStesselator* tess, int sidenb) const;
	QVarLengthArray<QVector<SubContour>,8 > sides;

	//! Update the content of both cached vertex arrays.
	void updateVertexArray();
	StelVertexArray fillCachedVertexArray;
	StelVertexArray outlineCachedVertexArray;
	void computeBoundingCap();
	Vec3d capN;
	double capD;

	static const Vec3d sideDirections[];
	static int getSideNumber(const Vec3d& v) {return v[0]>=0. ?  (v[1]>=0. ? (v[2]>=0.?0:1) : (v[2]>=0.?4:5))   :   (v[1]>=0. ? (v[2]>=0.?2:3) : (v[2]>=0.?6:7));}
	static bool isTriangleConvexPositive2D(const Vec3d& a, const Vec3d& b, const Vec3d& c);
	static bool triangleContains2D(const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& p);

	static void projectOnOctahedron(QVarLengthArray<QVector<SubContour>,8 >& inSides);
	static void splitContourByPlan(int onLine, const SubContour& contour, QVector<SubContour> result[2]);
};

// Serialization routines
QDataStream& operator<<(QDataStream& out, const OctahedronPolygon&);
QDataStream& operator>>(QDataStream& in, OctahedronPolygon&);

#endif // OCTAHEDRONREGION_HPP

