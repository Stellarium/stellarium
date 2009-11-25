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

#ifndef _OCTAHEDRON_REGION_HPP_
#define _OCTAHEDRON_REGION_HPP_

#include <QVector>
#include <QDebug>
#include <QVarLengthArray>
#include "VecMath.hpp"

struct StelVertexArray
{
	// TODO maybe merge this with StelPainter DrawingMode
	enum StelPrimitiveType
	{
		Points                      = 0x0000, // GL_POINTS
		Lines                       = 0x0001, // GL_LINES
		LineLoop                    = 0x0002, // GL_LINE_LOOP
		LineStrip                   = 0x0003, // GL_LINE_STRIP
		Triangles                   = 0x0004, // GL_TRIANGLES
		TriangleStrip               = 0x0005, // GL_TRIANGLE_STRIP
		TriangleFan                 = 0x0006  // GL_TRIANGLE_FAN
	};

	StelVertexArray(StelPrimitiveType pType=StelVertexArray::Triangles) : useIndice(false), primitiveType(pType) {;}
	StelVertexArray(const QVector<Vec3d>& v, StelPrimitiveType pType=StelVertexArray::Triangles,const QVector<Vec2f>& t=QVector<Vec2f>(), const QVector<unsigned int> i=QVector<unsigned int>()) :
		vertex(v), texCoords(t), indices(i), useIndice(!i.empty()), primitiveType(pType) {;}

	//! OpenGL compatible array of 3D vertex to be displayed using vertex arrays.
	//! TODO, move to float? Most of the vectors are normalized, thus the precision is around 1E-45 using float
	//! which is enough (=2E-37 milli arcsec).
	QVector<Vec3d> vertex;
	//! OpenGL compatible array of edge flags to be displayed using vertex arrays.
	QVector<Vec2f> texCoords;

	QVector<unsigned int> indices;
	bool useIndice;

	StelPrimitiveType primitiveType;

	const Vec3d& vertexAt(int i) const {
		return useIndice ? vertex.constData()[indices.at(i)] : vertex.constData()[i];
	}
	const Vec2f& texCoordAt(int i) const {
		return useIndice ? texCoords.constData()[indices.at(i)] : texCoords.constData()[i];
	}

	//! call a function for each triangle of the array.
	//! func should define the following method :
	//!     void operator() (const Vec3d* vertex[3], const Vec2f* tex[3], unsigned int indices[3])
	//! The method takes arrays of *pointers* as arguments because we can't assume the values are contiguous
	template<class Func>
	void foreachTriangle(Func& func) const;
};

struct EdgeVertex
{
	EdgeVertex() : edgeFlag(false) {;}
	EdgeVertex(bool b) : edgeFlag(b) {;}
	EdgeVertex(Vec3d v, bool b) : vertex(v), edgeFlag(b) {;}
	Vec3d vertex;
	bool edgeFlag;
};

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

//! @class OctahedronContour
//! Manage a non-convex polygon which can extends on more than 180 deg.
//! The contours defining the polygon are splitted and projected on the 8 sides of an Octahedron to enable 2D geometry
//! algorithms to be used.
class OctahedronPolygon
{
public:
	OctahedronPolygon() : fillCachedVertexArray(StelVertexArray::Triangles), outlineCachedVertexArray(StelVertexArray::Lines), capN(1,0,0), capD(-2.)
	{sides.resize(8);}

	//! Create the OctahedronContour by splitting the passed SubContour on the 8 sides of the octahedron.
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

	//! Set this OctahedronContourOctahedronPolygonion of itself with the given OctahedronContour.
	void inPlaceIntersection(const OctahedronPolygon& mpoly);
	//! Set this OctahedronContour as the union of itself with the given OctahedronContour.
	void inPlaceUnion(const OctahedronPolygon& mpoly);
	//! Set this OctahedronContour as the subtraction of itself with the given OctahedronContour.
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

	//! Tesselate the contours per side, producing a list of triangles subcontours according to the given rule.
	void tesselate(TessWindingRule rule);

	QVector<SubContour> tesselateOneSideLineLoop(class GLUEStesselator* tess, int sidenb) const;
	QVector<Vec3d> tesselateOneSideTriangles(class GLUEStesselator* tess, int sidenb) const;
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

//! call a function for each triangle of the array.
//! func should define the following method :
//!     void operator() (const Vec3d* v[3], const Vec2f* t[3], unsigned int indices[3])
template<class Func>
void StelVertexArray::foreachTriangle(Func& func) const
{
	const Vec3d* ar[3];
	const Vec2f* te[3];
	unsigned int indices[3];

	Q_ASSERT(texCoords.size() == vertex.size());

	switch (primitiveType)
	{
		case StelVertexArray::Triangles:
			Q_ASSERT(vertex.size() % 3 == 0);
			for (int i = 0; i < vertex.size() / 3; ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					ar[j] = &vertexAt(i * 3 + j);
					te[j] = &texCoordAt(i * 3 + j);
					indices[j] = i * 3 + j;
				}
				func(ar, te, indices);
			}
			break;
		case StelVertexArray::TriangleFan:
		{
			ar[0]=&vertexAt(0);
			te[0]=&texCoordAt(0);
			indices[0] = 0;
			for (int i = 1; i < vertex.size() - 1; ++i)
			{
				ar[1] = &vertexAt(i);
				ar[2] = &vertexAt(i+1);
				te[1] = &texCoordAt(i);
				te[2] = &texCoordAt(i+1);
				indices[1] = i;
				indices[2] = i+1;
				func(ar, te, indices);
			}
			break;
		}
		default:
			Q_ASSERT_X(0, Q_FUNC_INFO, "unsuported primitive type");
	}
}

#endif // _OCTAHEDRON_REGION_HPP_

