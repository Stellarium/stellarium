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

#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"
#include <QDebug>

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#else
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif

using namespace StelGeom;

//! Special constructor for 3 halfspaces convex
ConvexS::ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2)
{
	reserve(3);
	push_back(e1^e0);
	push_back(e2^e1);
	push_back(e0^e2);

	// Warning: vectors not normalized while they should be
	// In this case it works because d==0 for each HalfSpace
}

//! Special constructor for 4 halfspaces convex
ConvexS::ConvexS(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)
{
	reserve(4);
	const double d = e3*((e2-e3)^(e1-e3));
	if (d > 0)
	{
		push_back(e1^e0);
		push_back(e2^e1);
		push_back(e3^e2);
		push_back(e0^e3);

		// Warning: vectors not normalized while they should be
		// In this case it works because d==0 for each HalfSpace
	}
	else
	{
		push_back((e2-e3)^(e1-e3));
		(*begin()).d = d;
		(*begin()).n.normalize();
	}
}



//! Special constructor for 3 points polygon
Polygon::Polygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2)
{
	reserve(3);
	push_back(e0);
	push_back(e1);
	push_back(e2);
}

//! Special constructor for 4 points polygon
Polygon::Polygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)
{
	reserve(4);
	push_back(e0);
	push_back(e1);
	push_back(e2);
	push_back(e3);
}

bool SphericalPolygonBase::loadFromQVariant(const QVariantMap& qv)
{
	QVector<QVector<Vec3d> > contours;
	Q_ASSERT(0);
	setContours(contours);
	return true;
}

QVariantMap SphericalPolygonBase::toQVariant() const
{
	QVariantMap res;
	Q_ASSERT(0);
	return res;
}


// Store data for the GLU tesselation callbacks
struct GluTessCallbackData
{
	SphericalPolygon* thisPolygon;	//! Reference to the instance of SphericalPolygon being tesselated.
	bool edgeFlag;					//! Used to store temporary edgeFlag found by the tesselator.
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

void APIENTRY errorCallback(GLenum errno)
{
	qWarning() << "Tesselator error:" << errno;
}

void SphericalPolygon::setContours(const QVector<QVector<Vec3d> >& contours, SphericalPolygonBase::PolyWindingRule windingRule)
{
	triangleVertices.clear();
	edgeFlags.clear();
	
	// Use GLU tesselation functions to transform the polygon into a list of triangles
	GLUtesselator* tess = gluNewTess();
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLvoid(APIENTRY*)()) &vertexCallback);
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
			gluTessVertex(tess, const_cast<GLdouble*>((const double*)contours[c][i]), const_cast<void*>((const void*)contours[c][i]));
		}
		gluTessEndContour(tess);
	}
	gluTessEndPolygon(tess);
	gluDeleteTess(tess);
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

// Return a new SphericalPolygon consisting of the intersection of this and the given SphericalPolygon.
SphericalPolygon SphericalPolygonBase::getIntersection(const SphericalPolygonBase& mpoly)
{
	QVector<QVector<Vec3d> > allContours = getContours();
	allContours += mpoly.getContours();
	SphericalPolygon p;
	p.setContours(allContours, SphericalPolygonBase::WindingAbsGeqTwo);
	return p;
}

// Return a new SphericalPolygon consisting of the union of this and the given SphericalPolygon.
SphericalPolygon SphericalPolygonBase::getUnion(const SphericalPolygonBase& mpoly)
{
	QVector<QVector<Vec3d> > allContours = getContours();
	allContours += mpoly.getContours();
	return SphericalPolygon(allContours);
}

// Return a new SphericalPolygon consisting of the subtraction of the given SphericalPolygon from this.
SphericalPolygon SphericalPolygonBase::getSubtraction(const SphericalPolygonBase& mpoly)
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
	
ConvexPolygon ConvexPolygon::fullSky()
{
	ConvexPolygon poly;
	poly.asConvex().push_back(HalfSpace(Vec3d(1.,0.,0.), -1.));
	return poly;
}

//! Check if the polygon is valid, i.e. it has no side >180
bool ConvexPolygon::checkValid() const
{
	const ConvexS& cvx = asConvex();
	const Polygon& poly = asPolygon();
	if (cvx.size()<3)
		return false;
	bool res=true;
	for (size_t i=0;i<cvx.size();++i)
	{
		// Check that all points not on the current convex plane are included in it
		for (size_t p=0;p<cvx.size()-2;++p)
			res &= cvx[i].contains(poly[(p+i+2)%poly.size()]);
	}
	return res;
}

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

namespace StelGeom
{

//! Compute the intersection of the planes defined by the 2 halfspaces on the sphere (usually on 2 points) and return it in p1 and p2.
//! If the 2 HalfSpaces don't interesect or intersect only at 1 point, false is returned and p1 and p2 are undefined
bool planeIntersect2(const HalfSpace& h1, const HalfSpace& h2, Vec3d& p1, Vec3d& p2)
{
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

}
