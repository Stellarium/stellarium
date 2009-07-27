/*
 * Stellarium
 * Copyright (C) 2007 Guillaume Chereau
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

#include <QDebug>
#include <QBuffer>
#include <stdexcept>

#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"
#include "StelJsonParser.hpp"

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#else
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif

// Definition of static constants.
const QVariant::Type SphericalRegionP::qVariantType = (QVariant::Type)(QVariant::UserType+1);
int SphericalRegionP::metaTypeId = SphericalRegionP::initialize();

int SphericalRegionP::initialize()
{
	int id = SphericalRegionP::metaTypeId = qRegisterMetaType<SphericalRegionP>();
	qRegisterMetaTypeStreamOperators<SphericalRegionP>("SphericalRegionP");
	return id;
}

QDataStream& operator<<(QDataStream& out, const SphericalRegionP& region)
{
	out << region->toQVariant();
	return out;
}

QDataStream& operator>>(QDataStream& in, SphericalRegionP& region)
{
	QVariantMap v;
	in >> v;
	try
	{
		region=SphericalRegion::loadFromQVariant(v);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << e.what();
		Q_ASSERT(0);
	}
	return in;
}

bool SphericalRegion::contains(const SphericalRegionP& region) const
{
	if (dynamic_cast<const SphericalPoint*>(region.data()))
		return contains(static_cast<const SphericalPoint*>(region.data())->n);
	if (dynamic_cast<const SphericalCap*>(region.data()))
		return contains(*static_cast<const SphericalCap*>(region.data()));
	if (dynamic_cast<const SphericalPolygonBase*>(region.data()))
		return contains(*static_cast<const SphericalPolygonBase*>(region.data()));
	if (dynamic_cast<const AllSkySphericalRegion*>(region.data()))
		return contains(*static_cast<const AllSkySphericalRegion*>(region.data()));
	Q_ASSERT(0);
	return false;
}
	
bool SphericalRegion::intersects(const SphericalRegionP& region) const
{
	if (dynamic_cast<const SphericalPoint*>(region.data()))
		return contains(static_cast<const SphericalPoint*>(region.data())->n);
	if (dynamic_cast<const SphericalCap*>(region.data()))
		return intersects(*static_cast<const SphericalCap*>(region.data()));
	if (dynamic_cast<const SphericalPolygonBase*>(region.data()))
		return intersects(*static_cast<const SphericalPolygonBase*>(region.data()));
	if (dynamic_cast<const AllSkySphericalRegion*>(region.data()))
		return intersects(*static_cast<const AllSkySphericalRegion*>(region.data()));
	Q_ASSERT(0);
	return false;	
}

bool SphericalPoint::intersects(const SphericalPolygonBase& mpoly) const
{
	const SphericalConvexPolygon* cvx = dynamic_cast<const SphericalConvexPolygon*>(&mpoly);
	if (cvx!=NULL)
		return cvx->contains(n);
	else
		return static_cast<const SphericalPolygon*>(&mpoly)->contains(n);
}

SphericalRegionP SphericalRegion::getEnlarged(double margin) const
{
	Q_ASSERT(margin>=0);
	if (margin>=M_PI)
		return SphericalRegionP(new AllSkySphericalRegion());
	const SphericalCap& cap = getBoundingCap();
	double newRadius = std::acos(cap.d)+margin;
	if (newRadius>=M_PI)
		return SphericalRegionP(new AllSkySphericalRegion());
	return SphericalRegionP(new SphericalCap(cap.n, std::cos(newRadius)));
}

// Returns whether a SphericalPolygon is contained into the region.
bool SphericalCap::contains(const SphericalPolygonBase& polyBase) const
{
	const SphericalConvexPolygon* cvx = dynamic_cast<const SphericalConvexPolygon*>(&polyBase);
	if (cvx!=NULL)
	{
		foreach (const Vec3d& v, cvx->getConvexContour())
		{
			if (!contains(v))
				return false;
		}
		return true;
	}
	Q_ASSERT(0); // Not implemented
	return false;
}

bool SphericalCap::intersectsConvexContour(const Vec3d* vertice, int nbVertice) const
{
	for (int i=0;i<nbVertice;++i)
	{
		if (contains(vertice[i]))
			return true;
	}
	// No points of the convex polygon are inside the cap
	if (d<=0)
		return false;

	for (int i=0;i<nbVertice-1;++i)
	{
		if (!sideHalfSpaceIntersects(vertice[i], vertice[i+1], *this))
			return false;
	}
	if (!sideHalfSpaceIntersects(vertice[nbVertice-1], vertice[0], *this))
		return false;

	// Warning!!!! There is a last case which is not managed!
	// When all the points of the polygon are outside the circle but the halfspace of the corner the closest to the
	// circle intersects the circle halfspace..
	return true;
}

// Returns whether a SphericalPolygon intersects the region.
bool SphericalCap::intersects(const SphericalPolygonBase& polyBase) const
{
	// TODO This algo returns sometimes false positives!!
	const SphericalConvexPolygon* cvx = dynamic_cast<const SphericalConvexPolygon*>(&polyBase);
	if (cvx!=NULL)
		return intersectsConvexContour(cvx->getConvexContour().constData(), cvx->getConvexContour().size());
	// Go through the full list of triangle
	const QVector<Vec3d>& vArray = polyBase.getVertexArray();
	for (int i=0;i<vArray.size()/3;++i)
	{
		if (intersectsConvexContour(vArray.constData()+i*3, 3))
			return true;
	}
	return false;
}

// Convert the cap into a SphericalRegionBase instance.
SphericalConvexPolygon SphericalCap::toSphericalConvexPolygon() const
{
	static const int nbStep = 40;
	QVector<Vec3d> contour;
	Vec3d p(n);
	p.transfo4d(Mat4d::xrotation(std::acos(d)));
	const Mat4d& rot = Mat4d::rotation(n, -2.*M_PI/nbStep);
	for (int i=0;i<nbStep;++i)
	{
		contour.append(p);
		p.transfo4d(rot);
	}
	return SphericalConvexPolygon(contour);
}

///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalPolygonBase
///////////////////////////////////////////////////////////////////////////////
#ifndef APIENTRY
#define APIENTRY
#endif

void APIENTRY errorCallback(GLenum errno)
{
	qWarning() << "Tesselator error:" << QString::fromAscii((char*)gluErrorString(errno));
	Q_ASSERT(0);
}

void APIENTRY contourBeginCallback(GLenum type, void* userData)
{
	Q_ASSERT(type==GL_LINE_LOOP);
	QVector<QVector<Vec3d> >* tmpContours = static_cast<QVector<QVector<Vec3d> >*>(userData);
	tmpContours->append(QVector<Vec3d>());
}

void APIENTRY contourVertexCallback(void* vertexData, void* userData)
{
	const double* v = (double*)vertexData;
	QVector<QVector<Vec3d> >* tmpContours = static_cast<QVector<QVector<Vec3d> >*>(userData);
	(*tmpContours)[tmpContours->size()-1].append(Vec3d(v[0], v[1], v[2]));
}

QVector<QVector<Vec3d> > SphericalPolygonBase::getContours() const
{
	// Contour used by the tesselator as temporary objects
	QVector<QVector<Vec3d> > tmpContours;
	// Use GLU tesselation functions to compute the contours from the list of triangles
	GLUtesselator* tess = gluNewTess();
	gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (GLvoid(APIENTRY*)()) &contourBeginCallback);
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLvoid(APIENTRY*)()) &contourVertexCallback);
	gluTessCallback(tess, GLU_TESS_ERROR, (GLvoid (APIENTRY*) ()) &errorCallback);
	gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);
	gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE);
	gluTessBeginPolygon(tess, &tmpContours);
	const QVector<Vec3d>& trianglesArray = getVertexArray();
	for (int c=0;c<trianglesArray.size()/3;++c)
	{
		gluTessBeginContour(tess);
		gluTessVertex(tess, const_cast<GLdouble*>((const double*)trianglesArray[c*3]), const_cast<GLdouble*>((const double*)trianglesArray[c*3]));
		gluTessVertex(tess, const_cast<GLdouble*>((const double*)trianglesArray[c*3+1]), const_cast<GLdouble*>((const double*)trianglesArray[c*3+1]));
		gluTessVertex(tess, const_cast<GLdouble*>((const double*)trianglesArray[c*3+2]), const_cast<GLdouble*>((const double*)trianglesArray[c*3+2]));
		gluTessEndContour(tess);
	}
	gluTessEndPolygon(tess);
	gluDeleteTess(tess);
	return tmpContours;
}

// Returns whether another SphericalPolygon intersects with the SphericalPolygon.
bool SphericalPolygonBase::intersects(const SphericalPolygonBase& mpoly) const
{
	return !getIntersection(mpoly).getVertexArray().isEmpty();
}

// Return a new SphericalPolygon consisting of the intersection of this and the given SphericalPolygon.
SphericalPolygon SphericalPolygonBase::getIntersection(const SphericalPolygonBase& mpoly) const
{
	QVector<QVector<Vec3d> > allContours = getContours();
	allContours += mpoly.getContours();
	SphericalPolygon p;
	p.setContours(allContours, SphericalPolygonBase::WindingAbsGeqTwo);
	return p;
}

// Return a new SphericalPolygon consisting of the union of this and the given SphericalPolygon.
SphericalPolygon SphericalPolygonBase::getUnion(const SphericalPolygonBase& mpoly) const
{
	QVector<QVector<Vec3d> > allContours = getContours();
	allContours += mpoly.getContours();
	return SphericalPolygon(allContours);
}

// Return a new SphericalPolygon consisting of the subtraction of the given SphericalPolygon from this.
SphericalPolygon SphericalPolygonBase::getSubtraction(const SphericalPolygonBase& mpoly) const
{
	QVector<QVector<Vec3d> > allContours = getContours();
	foreach (const QVector<Vec3d>& c, mpoly.getContours())
	{
		QVector<Vec3d> cr;
		cr.reserve(c.size());
		for (int i=c.size()-1;i>=0;--i)
			cr.append(c.at(i));
		allContours.append(cr);
	}
	return SphericalPolygon(allContours);
}

// Return the area in squared degrees.
double SphericalPolygonBase::getArea() const
{
	// Use Girard's theorem for each subtriangles
	double area = 0.;
	Vec3d v1, v2, v3;
	const QVector<Vec3d>& trianglesArray = getVertexArray();
	for (int i=0;i<trianglesArray.size()/3;++i)
	{
		v1 = trianglesArray[i*3+0] ^ trianglesArray[i*3+1];
		v2 = trianglesArray[i*3+1] ^ trianglesArray[i*3+2];
		v3 = trianglesArray[i*3+2] ^ trianglesArray[i*3+0];
		area += 2.*M_PI - v1.angle(v2) - v2.angle(v3) - v3.angle(v1);
	}
	return area;
}

// Return a point located inside the polygon.
Vec3d SphericalPolygonBase::getPointInside() const
{
	const QVector<Vec3d>& trianglesArray = getVertexArray();
	Vec3d res = trianglesArray[0]+trianglesArray[1]+trianglesArray[2];
	res.normalize();
	return res;
}

// Default slow implementation o(n^2).
SphericalCap SphericalPolygonBase::getBoundingCap() const
{
	Vec3d p1(1,0,0), p2(1,0,0);
	double maxDist=1.;
	const QVector<Vec3d>& trianglesArray = getVertexArray();
	foreach (const Vec3d& v1, trianglesArray)
	{
		foreach (const Vec3d& v2, trianglesArray)
		{
			if (v1*v2<maxDist)
			{
				p1 = v1;
				p2 = v2;
				maxDist = v1*v2;	
			}
		}
	}
	Vec3d res = p1+p2;
	res.normalize();
	return SphericalCap(res, res*p1);
}

///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalPolygon
///////////////////////////////////////////////////////////////////////////////

// Store data for the GLU tesselation callbacks
struct GluTessCallbackData
{
	SphericalPolygon* thisPolygon;	//! Reference to the instance of SphericalPolygon being tesselated.
	bool edgeFlag;					//! Used to store temporary edgeFlag found by the tesselator.
	QVector<double*> tempVertices;	//! Use to contain the combined vertices

	~GluTessCallbackData()
	{
		foreach (double* dp, tempVertices)
			delete[] dp;
	}
};

void APIENTRY vertexCallback(void* vertexData, void* userData)
{
	SphericalPolygon* mp = ((GluTessCallbackData*)userData)->thisPolygon;
	const double* v = (double*)vertexData;
	mp->triangleVertices.append(Vec3d(v[0], v[1], v[2]));
	mp->edgeFlags.append(((GluTessCallbackData*)userData)->edgeFlag);
}

void APIENTRY edgeFlagCallback(GLboolean flag, void* userData)
{
	((GluTessCallbackData*)userData)->edgeFlag=flag;
}

void APIENTRY combineCallback(GLdouble coords[3], void* vertex_data[4], GLfloat weight[4], void** outData, void* userData)
{
	QVector<double*>& tempVertices = ((GluTessCallbackData*)userData)->tempVertices;
	double* v = new double[3];
	v[0]=coords[0];
	v[1]=coords[1];
	v[2]=coords[2];
	tempVertices << v;
	*outData = v;
}

void SphericalPolygon::setContours(const QVector<QVector<Vec3d> >& contours, SphericalPolygonBase::PolyWindingRule windingRule)
{
	triangleVertices.clear();
	edgeFlags.clear();

	// Use GLU tesselation functions to transform the polygon into a list of triangles
	GLUtesselator* tess = gluNewTess();
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLvoid(APIENTRY*)()) &vertexCallback);
	gluTessCallback(tess, GLU_TESS_EDGE_FLAG_DATA, (GLvoid(APIENTRY*)()) &edgeFlagCallback);
	gluTessCallback(tess, GLU_TESS_ERROR, (GLvoid (APIENTRY*)()) &errorCallback);
	gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (GLvoid (APIENTRY*)()) &combineCallback);
	const GLdouble windRule = (windingRule==SphericalPolygonBase::WindingPositive) ? GLU_TESS_WINDING_POSITIVE : GLU_TESS_WINDING_ABS_GEQ_TWO;
	gluTessProperty(tess, GLU_TESS_WINDING_RULE, windRule);
	GluTessCallbackData data;
	data.thisPolygon=this;
	gluTessBeginPolygon(tess, &data);
	for (int c=0;c<contours.size();++c)
	{
		gluTessBeginContour(tess);
		for (int i=0;i<contours[c].size();++i)
		{
			gluTessVertex(tess, const_cast<GLdouble*>((const double*)contours[c][i]), const_cast<void*>((const void*)contours[c][i]));
		}
		gluTessEndContour(tess);
	}
	gluTessEndPolygon(tess);
	gluDeleteTess(tess);

	// There should always be an edge flag matching each vertex.
	Q_ASSERT(triangleVertices.size() == edgeFlags.size());
#ifndef NDEBUG
	// Check that the orientation of all the triangles is positive
	for (int i=0;i<triangleVertices.size()/3;++i)
	{
		Q_ASSERT((triangleVertices.at(i*3+1)^triangleVertices.at(i*3))*triangleVertices.at(i*3+2)>=0);
	}
#endif
}

// Set a single contour defining the SphericalPolygon.
void SphericalPolygon::setContour(const QVector<Vec3d>& contour)
{
	QVector<QVector<Vec3d> > contours;
	contours.append(contour);
	setContours(contours);
}



///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalTexturedPolygon
///////////////////////////////////////////////////////////////////////////////
void APIENTRY vertexTextureCallback(void* vertexData, void* userData)
{
	SphericalTexturedPolygon* mp = static_cast<SphericalTexturedPolygon*>(((GluTessCallbackData*)userData)->thisPolygon);
	const SphericalTexturedPolygon::TextureVertex* vData = (SphericalTexturedPolygon::TextureVertex*)vertexData;
	mp->triangleVertices.append(vData->vertex);
	mp->textureCoords.append(vData->texCoord);
	mp->edgeFlags.append(((GluTessCallbackData*)userData)->edgeFlag);
}

void SphericalTexturedPolygon::setContours(const QVector<QVector<TextureVertex> >& contours, SphericalPolygonBase::PolyWindingRule windingRule)
{
	triangleVertices.clear();
	edgeFlags.clear();
	textureCoords.clear();

	// Use GLU tesselation functions to transform the polygon into a list of triangles
	GLUtesselator* tess = gluNewTess();
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLvoid(APIENTRY*)()) &vertexTextureCallback);
	gluTessCallback(tess, GLU_TESS_EDGE_FLAG_DATA, (GLvoid(APIENTRY*)()) &edgeFlagCallback);
	gluTessCallback(tess, GLU_TESS_ERROR, (GLvoid (APIENTRY*) ()) &errorCallback);
	const GLdouble windRule = (windingRule==SphericalPolygonBase::WindingPositive) ? GLU_TESS_WINDING_POSITIVE : GLU_TESS_WINDING_ABS_GEQ_TWO;
	gluTessProperty(tess, GLU_TESS_WINDING_RULE, windRule);
	GluTessCallbackData data;
	data.thisPolygon=this;
	gluTessBeginPolygon(tess, &data);
	for (int c=0;c<contours.size();++c)
	{
		gluTessBeginContour(tess);
		for (int i=0;i<contours[c].size();++i)
		{
			gluTessVertex(tess, const_cast<GLdouble*>((const double*)contours[c][i].vertex), const_cast<void*>((const void*)&(contours[c][i])));
		}
		gluTessEndContour(tess);
	}
	gluTessEndPolygon(tess);
	gluDeleteTess(tess);

	// There should always be a texture coord matching each vertex.
	Q_ASSERT(triangleVertices.size() == edgeFlags.size());
	Q_ASSERT(triangleVertices.size() == textureCoords.size());
}

void SphericalTexturedPolygon::setContour(const QVector<TextureVertex>& contour)
{
	QVector<QVector<TextureVertex> > contours;
	contours.append(contour);
	setContours(contours, SphericalPolygonBase::WindingPositive);
}

// Returns whether a point is contained into the SphericalPolygon.
bool SphericalPolygon::contains(const Vec3d& p) const
{
	const QVector<Vec3d>& trianglesArray = getVertexArray();
	for (int i=0;i<trianglesArray.size()/3;++i)
	{
		if (sideHalfSpaceContains(trianglesArray.at(i*3+1), trianglesArray.at(i*3), p) &&
				  sideHalfSpaceContains(trianglesArray.at(i*3+2), trianglesArray.at(i*3+1), p) &&
				  sideHalfSpaceContains(trianglesArray.at(i*3+0), trianglesArray.at(i*3+2), p))
			return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalConvexPolygon
///////////////////////////////////////////////////////////////////////////////

// Return an openGL compatible array to be displayed using vertex arrays.
QVector<Vec3d> SphericalConvexPolygon::getVertexArray() const
{
	SphericalPolygon p;
	p.setContour(contour);
	return p.getVertexArray();
}

// Return an openGL compatible array of edge flags to be displayed using vertex arrays.
QVector<bool> SphericalConvexPolygon::getEdgeFlagArray() const
{
	SphericalPolygon p;
	p.setContour(contour);
	return p.getEdgeFlagArray();
}

// Check if the polygon is valid, i.e. it has no side >180
bool SphericalConvexPolygon::checkValid() const
{
	return SphericalConvexPolygon::checkValidContour(contour);
}

bool SphericalConvexPolygon::checkValidContour(const QVector<Vec3d>& contour)
{
	if (contour.size()<3)
		return false;
	bool res=true;
	for (int i=0;i<contour.size()-1;++i)
	{
		// Check that all points not on the current convex plane are included in it
		for (int p=0;p<contour.size()-2;++p)
			res &= sideHalfSpaceContains(contour.at(i+1), contour.at(i), contour[(p+i+2)%contour.size()]);
	}
	for (int p=0;p<contour.size()-2;++p)
		res &= sideHalfSpaceContains(contour.first(), contour.last(), contour[(p+contour.size()+1)%contour.size()]);
	return res;
}

// Return the list of halfspace bounding the ConvexPolygon.
QVector<SphericalCap> SphericalConvexPolygon::getBoundingSphericalCaps() const
{
	QVector<SphericalCap> res;
	for (int i=0;i<contour.size()-1;++i)
		res << SphericalCap(contour.at(i+1)^contour.at(i), 0);
	res << SphericalCap(contour.first()^contour.last(), 0);
	return res;
}

// Returns whether a point is contained into the region.
bool SphericalConvexPolygon::contains(const Vec3d& p) const
{
	for (int i=0;i<contour.size()-1;++i)
	{
		if (!sideHalfSpaceContains(contour.at(i+1), contour.at(i), p))
			return false;
	}
	return sideHalfSpaceContains(contour.first(), contour.last(), p);
}

bool SphericalConvexPolygon::contains(const SphericalCap& c) const
{
	for (int i=0;i<contour.size()-1;++i)
	{
		if (!sideHalfSpaceContains(contour.at(i+1), contour.at(i), c))
			return false;
	}
	return sideHalfSpaceContains(contour.first(), contour.last(), c);
}

bool SphericalConvexPolygon::containsConvexContour(const Vec3d* vertice, int nbVertex) const
{
	for (int i=0;i<nbVertex;++i)
	{
		if (!contains(vertice[i]))
			return false;
	}
	return true;
}
		
// Returns whether a SphericalPolygon is contained into the region.
bool SphericalConvexPolygon::contains(const SphericalPolygonBase& polyBase) const
{
	const SphericalConvexPolygon* cvx = dynamic_cast<const SphericalConvexPolygon*>(&polyBase);
	if (cvx!=NULL)
	{
		return containsConvexContour(cvx->getConvexContour().constData(), cvx->getConvexContour().size());
	}
	// For standard polygons, go through the full list of triangles
	const QVector<Vec3d>& vArray = polyBase.getVertexArray();
	for (int i=0;i<vArray.size()/3;++i)
	{
		if (!containsConvexContour(vArray.constData()+i*3, 3))
			return false;
	}
	return true;
}

bool SphericalConvexPolygon::areAllPointsOutsideOneSide(const Vec3d* thisContour, int nbThisContour, const Vec3d* points, int nbPoints)
{
	for (int i=0;i<nbThisContour-1;++i)
	{
		bool allOutside = true;
		for (int j=0;j<nbPoints&& allOutside==true;++j)
		{
			allOutside = allOutside && !sideHalfSpaceContains(thisContour[i+1], thisContour[i], points[j]);
		}
		if (allOutside)
			return true;
	}

		// Last iteration
	bool allOutside = true;
	for (int j=0;j<nbPoints&& allOutside==true;++j)
	{
		allOutside = allOutside && !sideHalfSpaceContains(thisContour[0], thisContour[nbThisContour-1], points[j]);
	}
	if (allOutside)
		return true;

		// Else
	return false;
}

// Returns whether another SphericalPolygon intersects with the SphericalPolygon.
bool SphericalConvexPolygon::intersects(const SphericalPolygonBase& polyBase) const
{
	const SphericalConvexPolygon* cvx = dynamic_cast<const SphericalConvexPolygon*>(&polyBase);
	if (cvx!=NULL)
	{
		return !areAllPointsOutsideOneSide(cvx->contour) && !cvx->areAllPointsOutsideOneSide(contour);
	}
	// For standard polygons, go through the full list of triangles
	const QVector<Vec3d>& vArray = polyBase.getVertexArray();
	for (int i=0;i<vArray.size()/3;++i)
	{
		if (!areAllPointsOutsideOneSide(contour.constData(), contour.size(), vArray.constData()+i*3, 3) && !cvx->areAllPointsOutsideOneSide(vArray.constData()+i*3, 3, contour.constData(), contour.size()))
			return true;
	}
	return false;
}

/*

//! Return the convex polygon area in steradians
// TODO Optimize using add oc formulas from http://en.wikipedia.org/wiki/Solid_angle
double ConvexPolygon::getArea() const
{
	// Use Girard's theorem
	double angleSum=0.;
	const ConvexS& cvx = asConvex();
	const int size = cvx.size();

	if (size==1)
	{
		// Handle special case for > 180 degree polygons
		return cvx[0].getArea();
	}

	// Sum the angles at each corner of the polygon
	// (the non cartesian angle is found from the plan normals)
	for (int i=0;i<size-1;++i)
	{
		angleSum += M_PI-cvx[i].n.angle(cvx[i+1].n);
	}
	// Last angle
	angleSum += M_PI-cvx[size-1].n.angle(cvx[0].n);
	return angleSum - M_PI*(size-2);
}

//! Return the convex polygon barycenter
// TODO this code is quite wrong but good for triangles
Vec3d ConvexPolygon::getBarycenter() const
{
	if (ConvexS::size()==1)
	{
		// Handle special case for > 180 degree polygons
		return asConvex()[0].n;
	}

	Vec3d barycenter(0.);
	for (unsigned int i=0;i<Polygon::size();++i)
	{
		barycenter += Polygon::operator[](i);
	}
	barycenter.normalize();
	return barycenter;
}

//! Special constructor for 4 halfspaces convex
// ConvexS::ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)
// {
// 	reserve(4);
// 	const double d = e3*((e2-e3)^(e1-e3));
// 	if (d > 0)
// 	{
// 		push_back(e1^e0);
// 		push_back(e2^e1);
// 		push_back(e3^e2);
// 		push_back(e0^e3);
//
// 		// Warning: vectors not normalized while they should be
// 		// In this case it works because d==0 for each SphericalCap
// 	}
// 	else
// 	{
// 		push_back((e2-e3)^(e1-e3));
// 		(*begin()).d = d;
// 		(*begin()).n.normalize();
// 	}
// }
*/

//! Compute the intersection of the planes defined by the 2 halfspaces on the sphere (usually on 2 points) and return it in p1 and p2.
//! If the 2 SphericalCaps don't interesect or intersect only at 1 point, false is returned and p1 and p2 are undefined
bool planeIntersect2(const SphericalCap& h1, const SphericalCap& h2, Vec3d& p1, Vec3d& p2)
{
	if (!h1.intersects(h2))
		return false;
	const Vec3d& n1 = h1.n;
	const Vec3d& n2 = h2.n;
	const double& d1 = -h1.d;
	const double& d2 = -h2.d;
	const double& a1 = n1[0];
	const double& b1 = n1[1];
	const double& c1 = n1[2];
	const double& a2 = n2[0];
	const double& b2 = n2[1];
	const double& c2 = n2[2];

	Q_ASSERT(fabs(n1.lengthSquared()-1.)<0.000001);
	Q_ASSERT(fabs(n2.lengthSquared()-1.)<0.000001);

	// Compute the parametric equation of the line at the intersection of the 2 planes
	Vec3d u = n1^n2;
	if (u[0]==0. && u[1]==0. && u[2]==0.)
	{
		// The planes are parallel
		return false;
	}
	u.normalize();

	// u gives the direction of the line, still need to find a suitable start point p0
	// Find the axis on which the line varies the fastest, and solve the system for value == 0 on this axis
	int maxI = (fabs(u[0])>=fabs(u[1])) ? (fabs(u[0])>=fabs(u[2]) ? 0 : 2) : (fabs(u[2])>fabs(u[1]) ? 2 : 1);
	Vec3d p0(0);
	switch (maxI)
	{
		case 0:
		{
			// Intersection of the line with the plane x=0
			const double denom = b1*c2-b2*c1;
			p0[1] = (d2*c1-d1*c2)/denom;
			p0[2] = (d1*b2-d2*b1)/denom;
			break;
		}
		case 1:
		{
			// Intersection of the line with the plane y=0
			const double denom = a1*c2-a2*c1;
			p0[0]=(c1*d2-c2*d1)/denom;
			p0[2]=(a2*d1-d2*a1)/denom;
			break;
		}
		case 2:
		{
			// Intersection of the line with the plane z=0
			const double denom = a1*b2-a2*b1;
			p0[0]=(b1*d2-b2*d1)/denom;
			p0[1]=(a2*d1-a1*d2)/denom;
			break;
		}
	}

	// The intersection line is now fully defined by the parametric equation p = p0 + u*t

	// The points are on the unit sphere x^2+y^2+z^2=1, replace x, y and z by the parametric equation to get something of the form at^2+b*t+c=0
	// const double a = 1.;
	const double b = p0*u*2.;
	const double c = p0.lengthSquared()-1.;

	// If discriminant <=0, zero or 1 real solution
	const double D = b*b-4.*c;
	if (D<=0.)
		return false;

	const double sqrtD = std::sqrt(D);
	const double t1 = (-b+sqrtD)/2.;
	const double t2 = (-b-sqrtD)/2.;
	p1 = p0+u*t1;
	p2 = p0+u*t2;

	Q_ASSERT(fabs(p1.lengthSquared()-1.)<0.000001);
	Q_ASSERT(fabs(p2.lengthSquared()-1.)<0.000001);

	return true;
}


SphericalRegionP SphericalRegion::loadFromJson(QIODevice* in)
{
	StelJsonParser parser;
	return loadFromQVariant(parser.parse(*in).toMap());
}

SphericalRegionP SphericalRegion::loadFromJson(const QByteArray& a)
{
	QBuffer buf;
	buf.setData(a);
	buf.open(QIODevice::ReadOnly);
	return loadFromJson(&buf);
}

inline void parseRaDec(const QVariant& vRaDec, Vec3d& v)
{
	const QVariantList& vl = vRaDec.toList();
	bool ok;
	if (vl.size()!=2)
		throw std::runtime_error(qPrintable(QString("invalid Ra,Dec pair: \"%1\" (expect 2 double values in degree, got %2)").arg(vRaDec.toString()).arg(vl.size())));
	StelUtils::spheToRect(vl.at(0).toDouble(&ok)*M_PI/180., vl.at(1).toDouble(&ok)*M_PI/180., v);
	if (!ok)
		throw std::runtime_error(qPrintable(QString("invalid Ra,Dec pair: \"%1\" (expect 2 double values in degree)").arg(vRaDec.toString())));
}

SphericalRegionP SphericalRegion::loadFromQVariant(const QVariantMap& map)
{
	QVariantList contoursList = map.value("skyConvexPolygons").toList();
	if (contoursList.empty())
		contoursList = map.value("worldCoords").toList();
	else
		qWarning() << "skyConvexPolygons in preview JSON files is deprecated. Replace with worldCoords.";

	if (contoursList.empty())
		throw std::runtime_error("missing sky contours description required for Spherical Geometry elements.");

	// Load the matching textures positions (if any)
	QVariantList texCoordList = map.value("textureCoords").toList();
	if (!texCoordList.isEmpty() && contoursList.size()!=texCoordList.size())
		throw std::runtime_error(qPrintable(QString("the number of sky contours (%1) does not match the number of texture space contours (%2)").arg( contoursList.size()).arg(texCoordList.size())));

	bool ok;
	if (texCoordList.isEmpty())
	{
		// No texture coordinates
		QVector<QVector<Vec3d> > contours;
		QVector<Vec3d> vertices;
		for (int i=0;i<contoursList.size();++i)
		{
			const QVariantList& contourToList = contoursList.at(i).toList();
			if (contourToList.size()<1)
				throw std::runtime_error(qPrintable(QString("invalid contour definition: %1").arg(contoursList.at(i).toString())));
			if (contourToList.at(0).toString()=="CAP")
			{
				// We now parse a cap, the format is "CAP",[ra, dec],aperture
				if (contourToList.size()!=3)
					throw std::runtime_error(qPrintable(QString("invalid CAP description: %1 (expect \"CAP\",[ra, dec],aperture)").arg(contoursList.at(i).toString())));
				Vec3d v;
				parseRaDec(contourToList.at(1), v);
				double d = contourToList.at(2).toDouble(&ok)*M_PI/180.;
				if (!ok)
					throw std::runtime_error(qPrintable(QString("invalid aperture angle: \"%1\" (expect a double value in degree)").arg(contourToList.at(2).toString())));
				SphericalCap cap(v,std::cos(d));
				contours.append(cap.toSphericalConvexPolygon().getConvexContour());
				vertices.clear();
				continue;
			}
			// If no type is provided, assume a polygon
			if (contourToList.size()<3)
				throw std::runtime_error("a polygon contour must have at least 3 vertices");
			Vec3d v;
			foreach (const QVariant& vRaDec, contourToList)
			{
				parseRaDec(vRaDec, v);
				vertices.append(v);
			}
			Q_ASSERT(vertices.size()>2);
			contours.append(vertices);
			vertices.clear();
		}
		return SphericalRegionP(new SphericalPolygon(contours));
	}
	else
	{
		// With texture coordinates
		QVector<QVector<SphericalTexturedPolygon::TextureVertex> > contours;
		QVector<SphericalTexturedPolygon::TextureVertex> vertices;
		for (int i=0;i<contoursList.size();++i)
		{
			// Load vertices
			const QVariantList& polyRaDecToList = contoursList.at(i).toList();
			if (polyRaDecToList.size()<3)
				throw std::runtime_error("a polygon contour must have at least 3 vertices");
			SphericalTexturedPolygon::TextureVertex v;
			foreach (const QVariant& vRaDec, polyRaDecToList)
			{
				parseRaDec(vRaDec, v.vertex);
				vertices.append(v);
			}
			Q_ASSERT(vertices.size()>2);

			// Add the texture coordinates
			const QVariantList& polyXYToList = texCoordList.at(i).toList();
			if (polyXYToList.size()!=vertices.size())
				throw std::runtime_error("texture coordinate and vertices number mismatch for contour");
			for (int n=0;n<polyXYToList.size();++n)
			{
				const QVariantList& vl = polyXYToList.at(n).toList();
				if (vl.size()!=2)
					throw std::runtime_error("invalid texture coordinate pair (expect 2 double values in degree)");
				vertices[n].texCoord.set(vl.at(0).toDouble(&ok), vl.at(1).toDouble(&ok));
				if (!ok)
					throw std::runtime_error("invalid texture coordinate pair (expect 2 double values in degree)");
			}
			contours.append(vertices);
			vertices.clear();
		}
		return SphericalRegionP(new SphericalTexturedPolygon(contours));
	}
	Q_ASSERT(0);
	return SphericalRegionP(new SphericalCap());
}

///////////////////////////////////////////////////////////////////////////////
// Serialization into QVariant

QVariantMap SphericalPoint::toQVariant() const
{
	QVariantMap res;
	res.insert("type", "POINT");
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, n);
	QVariantList l;
	l << ra*180./M_PI << dec*180./M_PI;
	res.insert("pos", l);
	return res;
}

QVariantMap SphericalCap::toQVariant() const
{
	QVariantMap res;
	res.insert("type", "CAP");
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, n);
	QVariantList l;
	l << ra*180./M_PI << dec*180./M_PI;
	res.insert("center", l);
	res.insert("radius", std::acos(d)*180./M_PI);
	return res;
}

QVariantMap AllSkySphericalRegion::toQVariant() const
{
	QVariantMap res;
	res.insert("type", "ALLSKY");
	return res;
}

QVariantMap SphericalPolygon::toQVariant() const
{
	QVariantMap res;
	QVariantList worldCoordinates;
	double ra, dec;
	foreach (const QVector<Vec3d>& contour, getContours())
	{
		QVariantList cv;
		foreach (const Vec3d& v, contour)
		{
			StelUtils::rectToSphe(&ra, &dec, v);
			QVariantList vv;
			vv << ra*180./M_PI << dec*180./M_PI;
			cv.append((QVariant)vv);
		}
		worldCoordinates.append((QVariant)cv);
	}
	res.insert("worldCoords", worldCoordinates);
	return res;
}

QVariantMap SphericalTexturedPolygon::toQVariant() const
{
	Q_ASSERT(0);
	// TODO store a tesselated polygon?, including edge flags?
	return QVariantMap();
}

QVariantMap SphericalConvexPolygon::toQVariant() const
{
	QVariantMap res;
	res.insert("type", "CVXPOLYGON");
	QVariantList cv;
	double ra, dec;
	foreach (const Vec3d& v, contour)
	{
		StelUtils::rectToSphe(&ra, &dec, v);
		QVariantList vv;
		vv << ra*180./M_PI << dec*180./M_PI;
		cv.append((QVariant)vv);
	}
	res.insert("worldCoords", cv);
	return res;
}


QVariantMap SphericalTexturedConvexPolygon::toQVariant() const
{
	QVariantMap res = SphericalConvexPolygon::toQVariant();
	QVariantList cv;
	foreach (const Vec2f& v, textureCoords)
	{
		QVariantList vv;
		vv << v[0] << v[1];
		cv.append((QVariant)vv);
	}
	res.insert("textureCoords", cv);
	return res;
}
