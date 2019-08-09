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

#include "StelUtils.hpp"
#include "OctahedronPolygon.hpp"
#include "StelSphereGeometry.hpp"
#include "glues.h"

#include <QFile>

const Vec3d OctahedronPolygon::sideDirections[] = {	Vec3d(1,1,1), Vec3d(1,1,-1),Vec3d(-1,1,1),Vec3d(-1,1,-1),
	Vec3d(1,-1,1),Vec3d(1,-1,-1),Vec3d(-1,-1,1),Vec3d(-1,-1,-1)};

inline bool intersectsBoundingCap(const Vec3d& n1, double d1, const Vec3d& n2, double d2)
{
	const double a = d1*d2 - n1*n2;
	return d1+d2<=0. || a<=0. || (a<=1. && a*a <= (1.-d1*d1)*(1.-d2*d2));
}

inline bool containsBoundingCap(const Vec3d& n1, double d1, const Vec3d& n2, double d2)
{
	const double a = n1*n2-d1*d2;
	return d1<=d2 && ( a>=1. || (a>=0. && a*a >= (1.-d1*d1)*(1.-d2*d2)));
}

static int getSide(const Vec3d& v, int onLine)
{
	Q_ASSERT(onLine>=0 && onLine<3);
	return v[onLine]>=0. ? 0 : 1;
}

QDataStream& operator<<(QDataStream& out, const EdgeVertex& v)
{
	out << v.vertex << v.edgeFlag;
	return out;
}

QDataStream& operator>>(QDataStream& in, EdgeVertex& v)
{
	in >> v.vertex >> v.edgeFlag;
	return in;
}

SubContour::SubContour(const QVector<Vec3d>& vertices, bool closed) : QVector<EdgeVertex>(vertices.size(), EdgeVertex(true))
{
	// Create the contour list by adding the matching edge flags
	for (int i=0;i<vertices.size();++i)
		(*this)[i].vertex = vertices.at(i);
	if (closed==false)
	{
		this->first().edgeFlag=false;
		this->last().edgeFlag=false;
	}
}

SubContour SubContour::reversed() const
{
	SubContour res;
	QVectorIterator<EdgeVertex> iter(*this);
	iter.toBack();
	while (iter.hasPrevious())
		res.append(iter.previous());
	return res;
}

QString SubContour::toJSON() const
{
	QString res("[");
	double ra, dec;
	for (const auto& v : *this)
	{
		//res += QString("[") + v.vertex.toString() + "],";
		StelUtils::rectToSphe(&ra, &dec, v.vertex);
		res += QString("[") + QString::number(ra*M_180_PI, 'g', 12) + "," + QString::number(dec*M_180_PI, 'g', 12) + "," + (v.edgeFlag ? QString("true"): QString("false")) + "],";
	}
	res[res.size()-1]=']';
	return res;
}

OctahedronPolygon::OctahedronPolygon(const QVector<Vec3d>& contour) : fillCachedVertexArray(StelVertexArray::Triangles), outlineCachedVertexArray(StelVertexArray::Lines)
{
	sides.resize(8);
	appendSubContour(SubContour(contour));
	tesselate(WindingPositive);
	updateVertexArray();
}

OctahedronPolygon::OctahedronPolygon(const QVector<QVector<Vec3d> >& contours) : fillCachedVertexArray(StelVertexArray::Triangles), outlineCachedVertexArray(StelVertexArray::Lines)
{
	sides.resize(8);
	for (const auto& contour : contours)
		appendSubContour(SubContour(contour));
	tesselate(WindingPositive);
	updateVertexArray();
}

OctahedronPolygon::OctahedronPolygon(const SubContour& initContour)
{
	sides.resize(8);
	appendSubContour(initContour);
	tesselate(WindingPositive);
	updateVertexArray();
}


OctahedronPolygon::OctahedronPolygon(const QList<OctahedronPolygon>& octs) : fillCachedVertexArray(StelVertexArray::Triangles), outlineCachedVertexArray(StelVertexArray::Lines)
{
	sides.resize(8);
	for (const auto& oct : octs)
	{
		for (int i=0;i<8;++i)
		{
			Q_ASSERT(oct.sides.size()==8);
			sides[i] += oct.sides[i];
		}
		tesselate(WindingPositive);
	}
	updateVertexArray();
}

void OctahedronPolygon::appendSubContour(const SubContour& inContour)
{
	QVarLengthArray<QVector<SubContour>,8 > resultSides;
	resultSides.resize(8);
	QVector<SubContour> splittedContour1[2];
	// Split the contour on the plan Y=0
	splitContourByPlan(1, inContour, splittedContour1);
	
//	for (int c=0;c<2;++c)
//	{
//		for (int i=0;i<splittedContour1[c].size();++i)
//		{
//			SubContour& tmpSubContour = splittedContour1[c][i];
//			// If the contour was not splitted, don't try to connect
//			if (tmpSubContour.last().edgeFlag==true)
//				continue;
//			const Vec3d& v1(tmpSubContour.first().vertex);
//			const Vec3d& v2(tmpSubContour.last().vertex);
//			Q_ASSERT(fabs(v1[1])<1e-50);
//			Q_ASSERT(fabs(v2[1])<1e-50);
//			Vec3d v = tmpSubContour.first().vertex^tmpSubContour.last().vertex;
//			//qDebug() << v.toString();
//			if (v1*v2<0.)
//			{
//				if (v1[0]<0)
//				// A south pole has to be added
//				tmpSubContour << EdgeVertex(Vec3d(0,0,-1), false);
//			}
//			// else the contour ends on the same longitude line as it starts
			
//			else if (v[2]<-0.0000001)
//			{
//				// A north pole has to be added
//				tmpSubContour << EdgeVertex(Vec3d(0,0,1), false);
//			}
//			else
//			{
//				// else the contour ends on the same longitude line as it starts
//				Q_ASSERT(std::fabs(v[0])<0.0000001 || std::fabs(v[1])<0.0000001);
//			}
//			tmpSubContour.closed=true;
//		}
//	}
	
	
	// Re-split the contours on the plan X=0
	QVector<SubContour> splittedVertices2[4];
	for (const auto& subContour : splittedContour1[0])
		splitContourByPlan(0, subContour, splittedVertices2);
	for (const auto& subContour : splittedContour1[1])
		splitContourByPlan(0, subContour, splittedVertices2+2);

	// Now complete the contours which cross the areas from one side to another by adding poles
	for (int c=0;c<4;++c)
	{
		for (int i=0;i<splittedVertices2[c].size();++i)
		{
			SubContour& tmpSubContour = splittedVertices2[c][i];
			// If the contour was not splitted, don't try to connect
			if (tmpSubContour.last().edgeFlag==true)
				continue;
			Vec3d v = tmpSubContour.first().vertex^tmpSubContour.last().vertex;
			//qDebug() << v.toString();
			if (v[2]>0.00000001)
			{
				// A south pole has to be added
				tmpSubContour << EdgeVertex(Vec3d(0,0,-1), false);
			}
			else if (v[2]<-0.0000001)
			{
				// A north pole has to be added
				tmpSubContour << EdgeVertex(Vec3d(0,0,1), false);
			}
			else
			{
				// else the contour ends on the same longitude line as it starts
				Q_ASSERT(std::fabs(v[0])<0.0000001 || std::fabs(v[1])<0.0000001);
			}
		}
		for (const auto& subContour : splittedVertices2[c])
		{
			splitContourByPlan(2, subContour, resultSides.data()+c*2);
		}
	}
	projectOnOctahedron(resultSides);

	// Append the new sides to this
	Q_ASSERT(sides.size()==8 && resultSides.size()==8);
	for (int i=0;i<8;++i)
	{
		sides[i] += resultSides[i];
	}
}

// Return the area in squared degrees.
double OctahedronPolygon::getArea() const
{
	// Use Girard's theorem for each subtriangles
	double area = 0.;
	Vec3d v1, v2, v3;
	const QVector<Vec3d>& trianglesArray = getFillVertexArray().vertex;
	Q_ASSERT(getFillVertexArray().primitiveType==StelVertexArray::Triangles);
	for (int i=0;i<trianglesArray.size()/3;++i)
	{
		area += OctahedronPolygon::sphericalTriangleArea(trianglesArray.at(i*3), trianglesArray.at(i*3+1), trianglesArray.at(i*3+2));
	}
	return area;
}

// Return a point located inside the polygon. Actually, inside the first triangle in this case.
Vec3d OctahedronPolygon::getPointInside() const
{
	const QVector<Vec3d>& trianglesArray = getFillVertexArray().vertex;
	Q_ASSERT(getFillVertexArray().primitiveType==StelVertexArray::Triangles);
	Q_ASSERT(!trianglesArray.isEmpty());
	Vec3d res(trianglesArray[0]);
	res+=trianglesArray[1];
	res+=trianglesArray[2];
	res.normalize();
	return res;
}

void OctahedronPolygon::append(const OctahedronPolygon& other)
{
	Q_ASSERT(sides.size()==8 && other.sides.size()==8);
	for (int i=0;i<8;++i)
	{
		sides[i] += other.sides[i];
	}
}

void OctahedronPolygon::appendReversed(const OctahedronPolygon& other)
{
	Q_ASSERT(sides.size()==8 && other.sides.size()==8);
	for (int i=0;i<8;++i)
	{
		for (const auto& sub : other.sides[i])
		{
			sides[i] += sub.reversed();
		}
	}
}

void OctahedronPolygon::projectOnOctahedron(QVarLengthArray<QVector<SubContour>,8 >& inSides)
{
	Q_ASSERT(inSides.size()==8);
	QVector<SubContour>* subs = inSides.data();

	for (int i=0;i<8;++i)
	{
		for (auto& sub : subs[i])
		{
			for (auto& v : sub)
			{
				// Project on the face with aperture = 90 deg
				v.vertex *= 1./(sideDirections[i]*v.vertex);
				v.vertex[2]=0.;
				// May want to add offsets after that to map TOAST projection
			}
		}
	}
}

bool OctahedronPolygon::isTriangleConvexPositive2D(const Vec3d& a, const Vec3d& b, const Vec3d& c)
{
	return	(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])>=0. &&
			(c[0]-b[0])*(a[1]-b[1])-(c[1]-b[1])*(a[0]-b[0])>=0. &&
			(a[0]-c[0])*(b[1]-c[1])-(a[1]-c[1])*(b[0]-c[0])>=0.;
}

bool OctahedronPolygon::triangleContains2D(const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& p)
{
	return	(b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])>=0. &&
			(c[0]-b[0])*(p[1]-b[1])-(c[1]-b[1])*(p[0]-b[0])>=0. &&
			(a[0]-c[0])*(p[1]-c[1])-(a[1]-c[1])*(p[0]-c[0])>=0.;
}

// Store data for the GLUES tesselation callbacks
struct OctTessTrianglesCallbackData
{
	QVector<Vec3d> result;			//! Contains the resulting tesselated vertices.
	QList<Vec3d> tempVertices;		//! Used to store the temporary combined vertices
};

void errorCallback(GLenum errn)
{
	qWarning() << "Tesselator error:" << QString::fromLatin1(reinterpret_cast<const char*>(gluesErrorString(errn)));
	Q_ASSERT(0);
}

void vertexTrianglesCallback(Vec3d* vertexData, OctTessTrianglesCallbackData* userData)
{
	userData->result.append(*vertexData);
}

void noOpCallback(GLboolean) {;}

void combineTrianglesCallback(double coords[3], Vec3d*[4], GLfloat[4], Vec3d** outData, OctTessTrianglesCallbackData* userData)
{
	// Check that the new coordinate lay on the octahedron plane
	Q_ASSERT(coords[2]<0.000001);
	userData->tempVertices.append(Vec3d(coords[0], coords[1], coords[2]));
	*outData = &(userData->tempVertices.last());
}

#ifndef NDEBUG
void checkBeginTrianglesCallback(GLenum type)
{
	Q_ASSERT(type==GL_TRIANGLES);
}
#endif

QVector<Vec3d> OctahedronPolygon::tesselateOneSideTriangles(GLUEStesselator* tess, int sidenb) const
{
	const QVector<SubContour>& contours = sides[sidenb];
	Q_ASSERT(!contours.isEmpty());
	OctTessTrianglesCallbackData data;
	gluesTessNormal(tess, 0.,0., (sidenb%2==0 ? -1. : 1.));
	gluesTessBeginPolygon(tess, &data);
	for (int c=0;c<contours.size();++c)
	{
		gluesTessBeginContour(tess);
		for (int i=0;i<contours.at(c).size();++i)
		{
			gluesTessVertex(tess, const_cast<double*>((const double*)contours[c][i].vertex.data()), (void*)&(contours[c][i].vertex));
		}
		gluesTessEndContour(tess);
	}
	gluesTessEndPolygon(tess);
	Q_ASSERT(data.result.size()%3==0);	// There should be only positive triangles here
	return data.result;
}

inline void unprojectOctahedron(Vec3d& v, const Vec3d& sideDirection)
{
	Q_ASSERT(v[2]<0.0000001);
	v[2]=(1.-sideDirection*v)/sideDirection[2];
	v.normalize();
}

void OctahedronPolygon::updateVertexArray()
{
	Q_ASSERT(sides.size()==8);
	fillCachedVertexArray.vertex.clear();
	outlineCachedVertexArray.vertex.clear();

	Q_ASSERT(sides.size()==8);
	// Use GLUES tesselation functions to transform the polygon into a list of triangles
	GLUEStesselator* tess = gluesNewTess();
#ifndef NDEBUG
	gluesTessCallback(tess, GLUES_TESS_BEGIN, (GLvoid(*)()) &checkBeginTrianglesCallback);
#endif
	gluesTessCallback(tess, GLUES_TESS_VERTEX_DATA, (GLvoid(*)()) &vertexTrianglesCallback);
	gluesTessCallback(tess, GLUES_TESS_EDGE_FLAG, (GLvoid(*)()) &noOpCallback);
	gluesTessCallback(tess, GLUES_TESS_ERROR, (GLvoid(*)()) &errorCallback);
	gluesTessCallback(tess, GLUES_TESS_COMBINE_DATA, (GLvoid(*)()) &combineTrianglesCallback);
	gluesTessProperty(tess, GLUES_TESS_WINDING_RULE, GLUES_TESS_WINDING_POSITIVE);

	// Call the tesselator on each side
	for (int sidenb=0;sidenb<8;++sidenb)
	{
		if (sides[sidenb].isEmpty())
			continue;
		const Vec3d& sideDirection = sideDirections[sidenb];
		QVector<Vec3d> res = tesselateOneSideTriangles(tess, sidenb);
		Q_ASSERT(res.size()%3==0);	// There should be only triangles here
		for (int j=0;j<=res.size()-3;j+=3)
		{
			// Post processing, GLU seems to sometimes output triangles oriented in the wrong direction..
			// Get rid of them in an ugly way. TODO Need to find the real cause.
			if (((sidenb&1)==0 ?
			isTriangleConvexPositive2D(res.at(j+2), res.at(j+1), res.at(j)) :
			isTriangleConvexPositive2D(res.at(j), res.at(j+1), res.at(j+2))))
			{
				fillCachedVertexArray.vertex+=res.at(j);
				unprojectOctahedron(fillCachedVertexArray.vertex.last(), sideDirection);
				fillCachedVertexArray.vertex+=res.at(j+1);
				unprojectOctahedron(fillCachedVertexArray.vertex.last(), sideDirection);
				fillCachedVertexArray.vertex+=res.at(j+2);
				unprojectOctahedron(fillCachedVertexArray.vertex.last(), sideDirection);
			}
			else
			{
				//  Discard vertex..
				//qDebug() << "Found a fucking CW triangle";
			}
		}

		// Now compute the outline contours, getting rid of non edge segments
		EdgeVertex previous;
		for (const auto& c : sides[sidenb])
		{
			Q_ASSERT(!c.isEmpty());
			previous = c.first();
			unprojectOctahedron(previous.vertex, sideDirection);
			for (int j=0;j<c.size()-1;++j)
			{
				if (previous.edgeFlag || c.at(j+1).edgeFlag)
				{
					outlineCachedVertexArray.vertex.append(previous.vertex);
					previous=c.at(j+1);
					unprojectOctahedron(previous.vertex, sideDirection);
					outlineCachedVertexArray.vertex.append(previous.vertex);
				}
				else
				{
					previous=c.at(j+1);
					unprojectOctahedron(previous.vertex, sideDirection);
				}
			}
			// Last point connects with first point
			if (previous.edgeFlag || c.first().edgeFlag)
			{
				outlineCachedVertexArray.vertex.append(previous.vertex);
				outlineCachedVertexArray.vertex.append(c.first().vertex);
				unprojectOctahedron(outlineCachedVertexArray.vertex.last(), sideDirection);
			}
		}
	}
	gluesDeleteTess(tess);
	computeBoundingCap();

#ifndef NDEBUG
	// Check that all triangles are properly oriented
	QVector<Vec3d> c;
	c.resize(3);
	for (int j=0;j<fillCachedVertexArray.vertex.size()/3;++j)
	{
		c[0]=fillCachedVertexArray.vertex.at(j*3);
		c[1]=fillCachedVertexArray.vertex.at(j*3+1);
		c[2]=fillCachedVertexArray.vertex.at(j*3+2);
		Q_ASSERT(SphericalConvexPolygon::checkValidContour(c));
	}
#else
	// If I don't let this like that, the bahaviour will fail in Release mode!!!!
	// It is either a bug in GCC either a memory problem which appears only when optimizations are activated.
	QVector<Vec3d> c;
	c.resize(3);
#endif
}

struct OctTessLineLoopCallbackData
{
	SubContour result;				//! Contains the resulting tesselated vertices.
	QVector<SubContour> resultList;
	QList<EdgeVertex> tempVertices;	//! Used to store the temporary combined vertices
};

QVector<SubContour> OctahedronPolygon::tesselateOneSideLineLoop(GLUEStesselator* tess, int sidenb) const
{
	const QVector<SubContour>& contours = sides[sidenb];
	Q_ASSERT(!contours.isEmpty());
	OctTessLineLoopCallbackData data;
	gluesTessNormal(tess, 0.,0., (sidenb%2==0 ? -1. : 1.));
	gluesTessBeginPolygon(tess, &data);
	for (int c=0;c<contours.size();++c)
	{
		gluesTessBeginContour(tess);
		for (int i=0;i<contours.at(c).size();++i)
		{
			Q_ASSERT(contours[c][i].vertex[2]<0.000001);
			gluesTessVertex(tess, const_cast<double*>((const double*)contours[c][i].vertex.data()), const_cast<void*>((const void*)&(contours[c][i])));
		}
		gluesTessEndContour(tess);
	}
	gluesTessEndPolygon(tess);
	return data.resultList;
}

// Define the square of the angular distance from which we merge 2 points.
inline bool tooClose(const Vec3d& e1, const Vec3d& e2)
{
	return (e1[0]-e2[0])*(e1[0]-e2[0])+(e1[1]-e2[1])*(e1[1]-e2[1])<0.000000002;
}

void vertexLineLoopCallback(EdgeVertex* vertexData, OctTessLineLoopCallbackData* userData)
{
	Q_ASSERT(vertexData->vertex[2]<0.0000001);
	if (userData->result.isEmpty() || !tooClose(userData->result.last().vertex, vertexData->vertex))
		userData->result.append(*vertexData);
	else
		userData->result.last().edgeFlag = userData->result.last().edgeFlag && vertexData->edgeFlag;
}

void combineLineLoopCallback(double coords[3], EdgeVertex* vertex_data[4], GLfloat[4], EdgeVertex** outData, OctTessLineLoopCallbackData* userData)
{
	bool newFlag=false;
	for (int i=0;i<4;++i)
	{
		if (vertex_data[i]==Q_NULLPTR)
			break;
		newFlag = newFlag || vertex_data[i]->edgeFlag;
	}
	// Check that the new coordinate lay on the octahedron plane
//	Q_ASSERT(fabs(fabs(coords[0])+fabs(coords[1])+fabs(coords[2])-1.)<0.000001);
	Q_ASSERT(coords[2]<0.000001);
	userData->tempVertices.append(EdgeVertex(Vec3d(coords[0], coords[1], coords[2]),newFlag));
	*outData = &(userData->tempVertices.last());
}

#ifndef NDEBUG
void checkBeginLineLoopCallback(GLenum type)
{
	Q_ASSERT(type==GL_LINE_LOOP);
}
#endif

void endLineLoopCallback(OctTessLineLoopCallbackData* data)
{
	// Store the finished contour and prepare for the next one
	if (data->result.size()>2)
		data->resultList.append(data->result);
	data->result.clear();
}

void OctahedronPolygon::tesselate(TessWindingRule windingRule)
{
	Q_ASSERT(sides.size()==8);
	// Use GLUES tesselation functions to transform the polygon into a list of triangles
	GLUEStesselator* tess = gluesNewTess();
#ifndef NDEBUG
	gluesTessCallback(tess, GLUES_TESS_BEGIN, (GLvoid(*)()) &checkBeginLineLoopCallback);
#endif
	gluesTessCallback(tess, GLUES_TESS_END_DATA, (GLvoid(*)()) &endLineLoopCallback);
	gluesTessCallback(tess, GLUES_TESS_VERTEX_DATA, (GLvoid(*)()) &vertexLineLoopCallback);
	gluesTessCallback(tess, GLUES_TESS_ERROR, (GLvoid(*)()) &errorCallback);
	gluesTessCallback(tess, GLUES_TESS_COMBINE_DATA, (GLvoid(*)()) &combineLineLoopCallback);
	const double windRule = (windingRule==OctahedronPolygon::WindingPositive) ? GLUES_TESS_WINDING_POSITIVE : GLUES_TESS_WINDING_ABS_GEQ_TWO;
	gluesTessProperty(tess, GLUES_TESS_WINDING_RULE, windRule);
	gluesTessProperty(tess, GLUES_TESS_BOUNDARY_ONLY, GL_TRUE);
	// Call the tesselator on each side
	for (int i=0;i<8;++i)
	{
		if (sides[i].isEmpty())
			continue;
		sides[i] = tesselateOneSideLineLoop(tess, i);
	}
	gluesDeleteTess(tess);
}


QString OctahedronPolygon::toJson() const
{
	QString res = "[";
	for (int sidenb=0;sidenb<8;++sidenb)
	{
		res += "[";
		for (const auto& c : sides[sidenb])
		{
			res += c.toJSON();
		}
		res += "]";
	}
	res += "]";
	return res;
}

void OctahedronPolygon::inPlaceIntersection(const OctahedronPolygon& mpoly)
{
	if (!intersectsBoundingCap(capN, capD, mpoly.capN, mpoly.capD))
		return;
	append(mpoly);
	tesselate(WindingAbsGeqTwo);
	//tesselate(WindingPositive);
	updateVertexArray();
}

void OctahedronPolygon::inPlaceUnion(const OctahedronPolygon& mpoly)
{
	const bool intersect = intersectsBoundingCap(capN, capD, mpoly.capN, mpoly.capD);
	append(mpoly);
	if (intersect)
		tesselate(WindingPositive);
	updateVertexArray();
}

void OctahedronPolygon::inPlaceSubtraction(const OctahedronPolygon& mpoly)
{
	if (!intersectsBoundingCap(capN, capD, mpoly.capN, mpoly.capD))
		return;
	appendReversed(mpoly);
	tesselate(WindingPositive);
	updateVertexArray();
}

bool OctahedronPolygon::intersects(const OctahedronPolygon& mpoly) const
{
	if (!intersectsBoundingCap(capN, capD, mpoly.capN, mpoly.capD))
		return false;
	OctahedronPolygon resOct(*this);
	resOct.inPlaceIntersection(mpoly);
	return !resOct.isEmpty();
}

bool OctahedronPolygon::contains(const OctahedronPolygon& mpoly) const
{
	if (!containsBoundingCap(capN, capD, mpoly.capN, mpoly.capD))
		return false;
	OctahedronPolygon resOct(*this);
	resOct.inPlaceUnion(mpoly);
	return resOct.getArea()-getArea()<0.00000000001;
}

bool OctahedronPolygon::contains(const Vec3d& p) const
{
	if (sides[getSideNumber(p)].isEmpty())
		return false;
	for (int i=0;i<fillCachedVertexArray.vertex.size()/3;++i)
	{
		if (sideHalfSpaceContains(fillCachedVertexArray.vertex.at(i*3+1), fillCachedVertexArray.vertex.at(i*3), p) &&
			sideHalfSpaceContains(fillCachedVertexArray.vertex.at(i*3+2), fillCachedVertexArray.vertex.at(i*3+1), p) &&
			sideHalfSpaceContains(fillCachedVertexArray.vertex.at(i*3), fillCachedVertexArray.vertex.at(i*3+2), p))
			return true;
	}
	return false;
}

bool OctahedronPolygon::isEmpty() const
{
	return sides[0].isEmpty() && sides[1].isEmpty() && sides[2].isEmpty() && sides[3].isEmpty() &&
			sides[4].isEmpty() && sides[5].isEmpty() && sides[6].isEmpty() && sides[7].isEmpty();
}


void OctahedronPolygon::splitContourByPlan(int onLine, const SubContour& inputContour, QVector<SubContour> result[2])
{
 	SubContour currentSubContour;
	SubContour unfinishedSubContour;
	int previousQuadrant=getSide(inputContour.first().vertex, onLine);
	int currentQuadrant=0;
	Vec3d tmpVertex;
	EdgeVertex previousVertex=inputContour.first();
	EdgeVertex currentVertex;
	int i=0;
	bool ok=true;
	const Vec3d plan(onLine==0?1:0, onLine==1?1:0, onLine==2?1:0);

	// Take care first of the unfinished contour
	for (i=0;i<inputContour.size();++i)
	{
		currentVertex = inputContour.at(i);
		if (currentVertex.vertex[onLine]==0)
			currentVertex.vertex[onLine]=1e-98;
		currentQuadrant = getSide(currentVertex.vertex, onLine);
		if (currentQuadrant==previousQuadrant)
		{
			unfinishedSubContour << currentVertex;
		}
		else
		{
			Q_ASSERT(currentSubContour.isEmpty());
			// We crossed the line
			tmpVertex = greatCircleIntersection(previousVertex.vertex, currentVertex.vertex, plan, ok);
			if (!ok)
			{
				// There was a problem, probably the 2 vertices are too close, just keep them like that
				// since they are each at a different side of the plan.
				currentSubContour << currentVertex;
			}
			else
			{
				Vec3d tmpVertexSides[2];
				tmpVertexSides[0]=tmpVertex;
				tmpVertexSides[1]=tmpVertex;
				tmpVertexSides[0][onLine]=1e-99;
				tmpVertexSides[1][onLine]=-1e-99;
				Q_ASSERT(getSide(tmpVertexSides[0], onLine)==0);
				Q_ASSERT(getSide(tmpVertexSides[1], onLine)==1);
				
				unfinishedSubContour << EdgeVertex(tmpVertexSides[previousQuadrant], false); // Last point of the contour, it's not an edge
				currentSubContour << EdgeVertex(tmpVertexSides[currentQuadrant], false);
			}
			previousQuadrant = currentQuadrant;
			previousVertex=currentVertex;
			break;
		}
		previousVertex=currentVertex;
	}
	
	// Now handle the other ones
	for (;i<inputContour.size();++i)
	{
		currentVertex = inputContour.at(i);
		if (currentVertex.vertex[onLine]==0)
			currentVertex.vertex[onLine]=1e-98;
		currentQuadrant = getSide(currentVertex.vertex, onLine);
		if (currentQuadrant==previousQuadrant)
		{
			currentSubContour << currentVertex;
		}
		else
		{
			// We crossed the line
			tmpVertex = greatCircleIntersection(previousVertex.vertex, currentVertex.vertex, plan, ok);
			if (!ok)
			{
				// There was a problem, probably the 2 vertices are too close, just keep them like that
				// since they are each at a different side of the plan.
				currentSubContour.last().edgeFlag = true;
				result[previousQuadrant] << currentSubContour;
				currentSubContour.clear();
				currentSubContour << currentVertex;
				currentSubContour.last().edgeFlag = true;
			}
			else
			{
				Vec3d tmpVertexSides[2];
				tmpVertexSides[0]=tmpVertex;
				tmpVertexSides[1]=tmpVertex;
				tmpVertexSides[0][onLine]=1e-99;
				tmpVertexSides[1][onLine]=-1e-99;
				Q_ASSERT(getSide(tmpVertexSides[0], onLine)==0);
				Q_ASSERT(getSide(tmpVertexSides[1], onLine)==1);
				
				currentSubContour << EdgeVertex(tmpVertexSides[previousQuadrant], false); 
				result[previousQuadrant] << currentSubContour;
				currentSubContour.clear();
				currentSubContour << EdgeVertex(tmpVertexSides[currentQuadrant], false);
				currentSubContour << currentVertex;
			}
			previousQuadrant = currentQuadrant;
		}
		previousVertex=currentVertex;
	}

	
	// Handle the last line between the last and first point
	previousQuadrant = currentQuadrant;
	currentQuadrant = getSide(inputContour.first().vertex, onLine);
	if (currentQuadrant!=previousQuadrant)
	{
		// We crossed the line
		tmpVertex = greatCircleIntersection(previousVertex.vertex, inputContour.first().vertex, plan, ok);
		if (!ok)
		{
			// There was a problem, probably the 2 vertices are too close, just keep them like that
			// since they are each at a different side of the plan.
			currentSubContour.last().edgeFlag = true;
			result[previousQuadrant] << currentSubContour;
			currentSubContour.clear();
		}
		else
		{
			Vec3d tmpVertexSides[2];
			tmpVertexSides[0]=tmpVertex;
			tmpVertexSides[1]=tmpVertex;
			tmpVertexSides[0][onLine]=1e-99;
			tmpVertexSides[1][onLine]=-1e-99;
			Q_ASSERT(getSide(tmpVertexSides[0], onLine)==0);
			Q_ASSERT(getSide(tmpVertexSides[1], onLine)==1);
			
			currentSubContour << EdgeVertex(tmpVertexSides[previousQuadrant], false);	// Last point of the contour, it's not an edge
			result[previousQuadrant] << currentSubContour;
			currentSubContour.clear();
			currentSubContour << EdgeVertex(tmpVertexSides[currentQuadrant], false);
		}
	}

	// Append the last contour made from the last vertices + the previous unfinished ones
	currentSubContour << unfinishedSubContour;
	
	result[currentQuadrant] << currentSubContour;
	
//	qDebug() << onLine << inputContour.toJSON();
//	if (!result[0].isEmpty())
//		qDebug() << result[0].at(0).toJSON();
//	if (!result[1].isEmpty())
//		qDebug() << result[1].at(0).toJSON();
}

void OctahedronPolygon::computeBoundingCap()
{
	const QVector<Vec3d>& trianglesArray = outlineCachedVertexArray.vertex;
	if (trianglesArray.isEmpty())
	{
		capN.set(1,0,0);
		capD = 2.;
		return;
	}
	// This is a quite crapy algorithm
	capN.set(0,0,0);
	for (const auto& v : trianglesArray)
		capN+=v;
	capN.normalize();
	capD = 1.;
	for (const auto& v : trianglesArray)
	{
		if (capN*v<capD)
			capD = capN*v;
	}
	capD*=capD>0 ? 0.9999999 : 1.0000001;
#ifndef NDEBUG
	for (const auto& v : trianglesArray)
	{
		Q_ASSERT(capN*v>=capD);
	}
#endif
}

OctahedronPolygon OctahedronPolygon::createAllSkyOctahedronPolygon()
{
	OctahedronPolygon poly;
	poly.sides.resize(8);
	static const Vec3d vertice[6] =
	{
		Vec3d(0,0,1), Vec3d(1,0,0), Vec3d(0,1,0), Vec3d(-1,0,0), Vec3d(0,-1,0), Vec3d(0,0,-1)
	};
	static const int verticeIndice[8][3] =
	{
		{0,2,1}, {5,1,2}, {0,3,2}, {5,2,3}, {0,1,4}, {5,4,1}, {0,4,3}, {5,3,4}
	};

	// Create the 8 base triangles
	QVector<SubContour> side;
	SubContour sc;
	for (int i=0;i<8;++i)
	{
		sc.clear();
		side.clear();
		sc << EdgeVertex(vertice[verticeIndice[i][0]], false)
				<< EdgeVertex(vertice[verticeIndice[i][1]], false)
				<< EdgeVertex(vertice[verticeIndice[i][2]], false);
		side.append(sc);
		poly.sides[i]=side;
	}
	projectOnOctahedron(poly.sides);
	poly.updateVertexArray();
	poly.capD = -2;
	Q_ASSERT(std::fabs(poly.getArea()-4.*M_PI)<0.0000001);
	return poly;
}

const OctahedronPolygon& OctahedronPolygon::getAllSkyOctahedronPolygon()
{
	static const OctahedronPolygon poly = createAllSkyOctahedronPolygon();
	return poly;
}

QDataStream& operator<<(QDataStream& out, const OctahedronPolygon& p)
{
	for (int i=0;i<8;++i)
	{
		out << p.sides[i];
	}
	out << p.fillCachedVertexArray;
	out << p.outlineCachedVertexArray;
	out << p.capN;
	out << p.capD;

	return out;
}

QDataStream& operator>>(QDataStream& in, OctahedronPolygon& p)
{
	for (int i=0;i<8;++i)
	{
		in >> p.sides[i];
	}
//	p.updateVertexArray();
	in >> p.fillCachedVertexArray;
	in >> p.outlineCachedVertexArray;
	in >> p.capN;
	in >> p.capD;
	return in;
}
