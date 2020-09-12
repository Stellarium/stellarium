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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelSphereGeometry.hpp"
#include "StelUtils.hpp"
#include "StelJsonParser.hpp"

#include <QDebug>
#include <QBuffer>
#include <stdexcept>

// Definition of static constants.
int SphericalRegionP::metaTypeId = SphericalRegionP::initialize();
int SphericalRegionP::initialize()
{
	int id = qRegisterMetaType<SphericalRegionP>("SphericalRegionP");
	qRegisterMetaTypeStreamOperators<SphericalRegionP>("SphericalRegionP");
	return id;
}

QDataStream& operator<<(QDataStream& out, const SphericalRegionP& region)
{
	out << static_cast<quint8>(region->getType());
	region->serialize(out);
	return out;
}

QDataStream& operator>>(QDataStream& in, SphericalRegionP& region)
{
	quint8 regType = SphericalRegion::Invalid;
	in >> regType;
	switch (regType)
	{
		case SphericalRegion::Empty:
			region = EmptySphericalRegion::staticInstance;
			return in;
		case SphericalRegion::AllSky:
			region = AllSkySphericalRegion::staticInstance;
			return in;
		case SphericalRegion::Cap:
			region = SphericalCap::deserialize(in);
			return in;
		case SphericalRegion::ConvexPolygon:
			region = SphericalConvexPolygon::deserialize(in);
			return in;
		case SphericalRegion::Polygon:
			region = SphericalPolygon::deserialize(in);
			return in;
		case SphericalRegion::Point:
			region = SphericalPoint::deserialize(in);
			return in;
		default:
			Q_ASSERT(0);	// Unknown region type
	}
	return in;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Default implementations of methods for SphericalRegion
///////////////////////////////////////////////////////////////////////////////////////////////
QByteArray SphericalRegion::toJSON() const
{
	QByteArray res;
	QBuffer buf1(&res);
	buf1.open(QIODevice::WriteOnly);
	StelJsonParser::write(toQVariant(), &buf1);
	buf1.close();
	return res;
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

QVector<SphericalCap> SphericalRegion::getBoundingSphericalCaps() const
{
	QVector<SphericalCap> res;
	res << getBoundingCap();
	return res;
}

// Default slow implementation o(n^2).
SphericalCap SphericalRegion::getBoundingCap() const
{
	SphericalCap res;
	getOctahedronPolygon().getBoundingCap(res.n, res.d);
	return res;
}

bool SphericalRegion::contains(const SphericalPolygon& r) const {return containsDefault(&r);}
bool SphericalRegion::contains(const SphericalConvexPolygon& r) const {return containsDefault(&r);}
bool SphericalRegion::contains(const SphericalCap& r) const {return containsDefault(&r);}
bool SphericalRegion::contains(const SphericalPoint& r) const {return contains(r.n);}
bool SphericalRegion::contains(const AllSkySphericalRegion& r) const {return containsDefault(&r);}
bool SphericalRegion::contains(const SphericalRegion* r) const
{
	switch (r->getType())
	{
		case SphericalRegion::Point:
			return contains(static_cast<const SphericalPoint*>(r)->n);
		case SphericalRegion::Cap:
			return contains(*static_cast<const SphericalCap*>(r));
		case SphericalRegion::Polygon:
			return contains(*static_cast<const SphericalPolygon*>(r));
		case SphericalRegion::ConvexPolygon:
			return contains(*static_cast<const SphericalConvexPolygon*>(r));
		case SphericalRegion::AllSky:
			return contains(*static_cast<const AllSkySphericalRegion*>(r));
		case SphericalRegion::Empty:
			return false;
		default:
			return containsDefault(r);
	}
	//Q_ASSERT(0);
	//return false;
}

bool SphericalRegion::intersects(const SphericalPolygon& r) const {return intersectsDefault(&r);}
bool SphericalRegion::intersects(const SphericalConvexPolygon& r) const {return intersectsDefault(&r);}
bool SphericalRegion::intersects(const SphericalCap& r) const {return intersectsDefault(&r);}
bool SphericalRegion::intersects(const SphericalPoint& r) const {return contains(r.n);}
bool SphericalRegion::intersects(const AllSkySphericalRegion&) const {return getType()==SphericalRegion::Empty ? false : true;}
bool SphericalRegion::intersects(const SphericalRegion* r) const
{
	switch (r->getType())
	{
		case SphericalRegion::Point:
			return intersects(static_cast<const SphericalPoint*>(r)->n);
		case SphericalRegion::Cap:
			return intersects(*static_cast<const SphericalCap*>(r));
		case SphericalRegion::Polygon:
			return intersects(*static_cast<const SphericalPolygon*>(r));
		case SphericalRegion::ConvexPolygon:
			return intersects(*static_cast<const SphericalConvexPolygon*>(r));
		case SphericalRegion::AllSky:
			return intersects(*static_cast<const AllSkySphericalRegion*>(r));
		case SphericalRegion::Empty:
			return false;
		default:
			return intersectsDefault(r);
	}
	//Q_ASSERT(0);
	//return false;
}

SphericalRegionP SphericalRegion::getIntersection(const SphericalRegion* r) const
{
	switch (r->getType())
	{
		case SphericalRegion::Point:
			return getIntersection(static_cast<const SphericalPoint*>(r)->n);
		case SphericalRegion::Cap:
			return getIntersection(*static_cast<const SphericalCap*>(r));
		case SphericalRegion::Polygon:
			return getIntersection(*static_cast<const SphericalPolygon*>(r));
		case SphericalRegion::ConvexPolygon:
			return getIntersection(*static_cast<const SphericalConvexPolygon*>(r));
		case SphericalRegion::AllSky:
			return getIntersection(*static_cast<const AllSkySphericalRegion*>(r));
		case SphericalRegion::Empty:
			return EmptySphericalRegion::staticInstance;
		default:
			return getIntersectionDefault(r);
	}
	// Q_ASSERT(0);
	// return SphericalRegionP();
}
SphericalRegionP SphericalRegion::getIntersection(const SphericalPolygon& r) const {return getIntersectionDefault(&r);}
SphericalRegionP SphericalRegion::getIntersection(const SphericalConvexPolygon& r) const {return getIntersectionDefault(&r);}
SphericalRegionP SphericalRegion::getIntersection(const SphericalCap& r) const {return getIntersectionDefault(&r);}
SphericalRegionP SphericalRegion::getIntersection(const SphericalPoint& r) const {return getIntersectionDefault(&r);}
SphericalRegionP SphericalRegion::getIntersection(const AllSkySphericalRegion& r) const {return getIntersectionDefault(&r);}
SphericalRegionP SphericalRegion::getIntersection(const EmptySphericalRegion&) const {return SphericalRegionP(new EmptySphericalRegion());}

SphericalRegionP SphericalRegion::getUnion(const SphericalRegion* r) const
{
	switch (r->getType())
	{
		case SphericalRegion::Point:
			return getUnion(static_cast<const SphericalPoint*>(r)->n);
		case SphericalRegion::Cap:
			return getUnion(*static_cast<const SphericalCap*>(r));
		case SphericalRegion::Polygon:
			return getUnion(*static_cast<const SphericalPolygon*>(r));
		case SphericalRegion::ConvexPolygon:
			return getUnion(*static_cast<const SphericalConvexPolygon*>(r));
		case SphericalRegion::AllSky:
			return getUnion(*static_cast<const AllSkySphericalRegion*>(r));
		case SphericalRegion::Empty:
			return getUnion(*static_cast<const EmptySphericalRegion*>(r));
		default:
			return getUnionDefault(r);
	}
	// Q_ASSERT(0);
	// return SphericalRegionP();
}
SphericalRegionP SphericalRegion::getUnion(const SphericalPolygon& r) const {return getUnionDefault(&r);}
SphericalRegionP SphericalRegion::getUnion(const SphericalConvexPolygon& r) const {return getUnionDefault(&r);}
SphericalRegionP SphericalRegion::getUnion(const SphericalCap& r) const {return getUnionDefault(&r);}
SphericalRegionP SphericalRegion::getUnion(const SphericalPoint& r) const {return getUnionDefault(&r);}
SphericalRegionP SphericalRegion::getUnion(const AllSkySphericalRegion&) const {return SphericalRegionP(new AllSkySphericalRegion());}
SphericalRegionP SphericalRegion::getUnion(const EmptySphericalRegion& r) const {return getUnionDefault(&r);}


SphericalRegionP SphericalRegion::getSubtraction(const SphericalRegion* r) const
{
	switch (r->getType())
	{
		case SphericalRegion::Point:
			return getSubtraction(static_cast<const SphericalPoint*>(r)->n);
		case SphericalRegion::Cap:
			return getSubtraction(*static_cast<const SphericalCap*>(r));
		case SphericalRegion::Polygon:
			return getSubtraction(*static_cast<const SphericalPolygon*>(r));
		case SphericalRegion::ConvexPolygon:
			return getSubtraction(*static_cast<const SphericalConvexPolygon*>(r));
		case SphericalRegion::AllSky:
			return getSubtraction(*static_cast<const AllSkySphericalRegion*>(r));
		case SphericalRegion::Empty:
			return getSubtraction(*static_cast<const EmptySphericalRegion*>(r));
		default:
			return getSubtractionDefault(r);
	}
	// Q_ASSERT(0);
	// return SphericalRegionP();
}
SphericalRegionP SphericalRegion::getSubtraction(const SphericalPolygon& r) const {return getSubtractionDefault(&r);}
SphericalRegionP SphericalRegion::getSubtraction(const SphericalConvexPolygon& r) const {return getSubtractionDefault(&r);}
SphericalRegionP SphericalRegion::getSubtraction(const SphericalCap& r) const {return getSubtractionDefault(&r);}
SphericalRegionP SphericalRegion::getSubtraction(const SphericalPoint& r) const {return getSubtractionDefault(&r);}
SphericalRegionP SphericalRegion::getSubtraction(const AllSkySphericalRegion&) const {return SphericalRegionP(new EmptySphericalRegion());}
SphericalRegionP SphericalRegion::getSubtraction(const EmptySphericalRegion& r) const {return getSubtractionDefault(&r);}

// Returns whether another SphericalPolygon intersects with the SphericalPolygon.
bool SphericalRegion::containsDefault(const SphericalRegion* r) const
{
	if (!getBoundingCap().contains(r->getBoundingCap()))
		return false;
	return getOctahedronPolygon().contains(r->getOctahedronPolygon());
}

// Returns whether another SphericalPolygon intersects with the SphericalPolygon.
bool SphericalRegion::intersectsDefault(const SphericalRegion* r) const
{
	if (!getBoundingCap().intersects(r->getBoundingCap()))
		return false;
	return getOctahedronPolygon().intersects(r->getOctahedronPolygon());
}

// Return a new SphericalPolygon consisting of the intersection of this and the given SphericalPolygon.
SphericalRegionP SphericalRegion::getIntersectionDefault(const SphericalRegion* r) const
{
	if (!getBoundingCap().intersects(r->getBoundingCap()))
		return SphericalRegionP(new EmptySphericalRegion());
	OctahedronPolygon resOct(getOctahedronPolygon());
	resOct.inPlaceIntersection(r->getOctahedronPolygon());
	return SphericalRegionP(new SphericalPolygon(resOct));
}

// Return a new SphericalPolygon consisting of the union of this and the given SphericalPolygon.
SphericalRegionP SphericalRegion::getUnionDefault(const SphericalRegion* r) const
{
	OctahedronPolygon resOct(getOctahedronPolygon());
	resOct.inPlaceUnion(r->getOctahedronPolygon());
	return SphericalRegionP(new SphericalPolygon(resOct));
}

// Return a new SphericalPolygon consisting of the subtraction of the given SphericalPolygon from this.
SphericalRegionP SphericalRegion::getSubtractionDefault(const SphericalRegion* r) const
{
	OctahedronPolygon resOct(getOctahedronPolygon());
	resOct.inPlaceSubtraction(r->getOctahedronPolygon());
	return SphericalRegionP(new SphericalPolygon(resOct));
}




////////////////////////////////////////////////////////////////////////////
// Methods for SphericalCap
////////////////////////////////////////////////////////////////////////////

// Returns whether a SphericalPolygon is contained into the region.
bool SphericalCap::contains(const SphericalConvexPolygon& cvx) const
{
	for (const auto& v : cvx.getConvexContour())
	{
		if (!contains(v))
			return false;
	}
	return true;
}

bool SphericalCap::containsTriangle(const Vec3d* v) const
{
	return contains(*(v++)) && contains(*(v++)) && contains(*v);
}

bool SphericalCap::intersectsTriangle(const Vec3d* v) const
{
	if (contains(*v) || contains(*(v+1)) || contains(*(v+2)))
		return true;
	// No points of the triangle are inside the cap
	if (d<=0)
		return false;

	if (!sideHalfSpaceIntersects(*v, *(v+1), *this) || !sideHalfSpaceIntersects(*(v+1), *(v+2), *this) || !sideHalfSpaceIntersects(*(v+2), *v, *this))
		return false;

	// Warning!!!! There is a last case which is not managed!
	// When all the points of the polygon are outside the circle but the halfspace of the corner the closest to the
	// circle intersects the circle halfspace..
	return true;
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
bool SphericalCap::intersects(const SphericalConvexPolygon& cvx) const
{
	// TODO This algo returns sometimes false positives!!
	return intersectsConvexContour(cvx.getConvexContour().constData(), cvx.getConvexContour().size());
}

bool SphericalCap::intersects(const SphericalPolygon& polyBase) const
{
	// Go through the full list of triangle
	const QVector<Vec3d>& vArray = polyBase.getFillVertexArray().vertex;
	for (int i=0;i<vArray.size()/3;++i)
	{
		if (intersectsConvexContour(vArray.constData()+i*3, 3))
			return true;
	}
	return false;
}


bool SphericalCap::clipGreatCircle(Vec3d& v1, Vec3d& v2) const
{
	if (v1==v2)
	{
		return contains(v1);
	}
	const bool b1 = contains(v1);
	const bool b2 = contains(v2);
	if (b1)
	{
		if (b2)
		{
			// Both points inside, nothing to do
			return true;
		}
		else
		{
			// v1 inside, v2 outside. Return segment v1 -- intersection
			Vec3d v = v1^v2;
			v.normalize();
			Vec3d vv;
			if (!SphericalCap::intersectionPoints(*this, SphericalCap(v, 0), v, vv))
				return false;
			const double cosDist = v1*v2;
			v2 = (v1*v >= cosDist && v2*v >= cosDist) ? v : vv;
			return true;
		}
	}
	else
	{
		if (b2)
		{
			// v2 inside, v1 outside. Return segment v2 -- intersection
			Vec3d v = v1^v2;
			v.normalize();
			Vec3d vv;
			if (!SphericalCap::intersectionPoints(*this, SphericalCap(v, 0), v, vv))
				return false;
			const double cosDist = v1*v2;
			v1 = (v1*v >= cosDist && v2*v >= cosDist) ? v : vv;
			return true;
		}
		else
		{
			// Both points outside
			Vec3d v = v1^v2;
			v.normalize();
			Vec3d vv;
			if (!SphericalCap::intersectionPoints(*this, SphericalCap(v, 0), v, vv))
				return false;
			const double cosDist = v1*v2;
			if (v1*v >= cosDist && v2*v >= cosDist && v1*vv >= cosDist && v2*vv >= cosDist)
			{
				v1 = v;
				v2 = vv;
				return true;
			}
			return false;
		}
	}
	//Q_ASSERT(0);
	//return false;
}

//! Compute the intersection of the circles defined by the 2 caps on the sphere (usually on 2 points) and return it in p1 and p2.
//! If the 2 SphericalCaps don't interesect or intersect only at 1 point, false is returned and p1 and p2 are undefined.
bool SphericalCap::intersectionPoints(const SphericalCap& h1, const SphericalCap& h2, Vec3d& p1, Vec3d& p2)
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
	Vec3d p0(0.);
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


double SphericalCap::relativeAreaOverlap(const SphericalCap& c1, const SphericalCap& c2)
{
	// TODO, this is wrong for many reasons
	Q_ASSERT(0);
	if (!c1.intersects(c2))
		return 0.;
	if (c1.contains(c2))
		return c2.getArea()/c1.getArea();
	if (c2.contains(c1))
		return c1.getArea()/c2.getArea();
	Vec3d p1, p2;
	double area1=c1.getArea();
	double area2=c2.getArea();
	bool ok = SphericalCap::intersectionPoints(c1, c2, p1, p2);
#ifndef NDEBUG
	Q_ASSERT(ok);
#else
	Q_UNUSED(ok)
#endif
	Vec3d c(c1.n);
	c*=c1.d;
	const double a1 = std::acos((p1-c)*(p2-c)/(1.-fabs(c1.d)))/(2.*M_PI)*area1 - OctahedronPolygon::sphericalTriangleArea(p1,p2,c1.n);
	c=c2.n;
	c*=c2.d;
	const double a2 = std::acos((p1-c)*(p2-c)/(1.-fabs(c2.d)))/(2.*M_PI)*area2 - OctahedronPolygon::sphericalTriangleArea(p2,p1,c2.n);
	const double overlapArea = a1+a2;
	return qMin(overlapArea/area1, overlapArea/area2);
}

double SphericalCap::relativeDiameterOverlap(const SphericalCap& c1, const SphericalCap& c2)
{
	if (!c1.intersects(c2))
		return 0.;
	if (c1.contains(c2))
		return c2.getRadius()/c1.getRadius();
	if (c2.contains(c1))
		return c1.getRadius()/c2.getRadius();
	const double r1 = c1.getRadius();
	const double r2 = c2.getRadius();
	const double a = c1.n.angleNormalized(c2.n);
	const double overlapDist = (a-r1-r2)/(std::fabs(r1-r2)-r1-r2);
	Q_ASSERT(overlapDist>=0);
	return overlapDist*qMin(r1/r2, r2/r1);
}

QVector<Vec3d> SphericalCap::getClosedOutlineContour() const
{
	static const int nbStep = 40;
	QVector<Vec3d> contour;
	Vec3d p(n);
	Vec3d axis = n^Vec3d(1,0,0);
	if (axis.lengthSquared()<0.1)
		axis = n^Vec3d(0,1,0);	// Improve precision
	p.transfo4d(Mat4d::rotation(axis, std::acos(d)));
	const Mat4d& rot = Mat4d::rotation(n, -2.*M_PI/nbStep);
	for (int i=0;i<nbStep;++i)
	{
		contour.append(p);
		p.transfo4d(rot);
	}
	return contour;
}

OctahedronPolygon SphericalCap::getOctahedronPolygon() const
{
	if (d>=0)
		return OctahedronPolygon(getClosedOutlineContour());
	else
	{
		SphericalCap cap(-n, -d);
		AllSkySphericalRegion allSky;
		OctahedronPolygon poly = allSky.getOctahedronPolygon();
		poly.inPlaceSubtraction(cap.getOctahedronPolygon());
		return poly;
	}
}

QVariantList SphericalCap::toQVariant() const
{
	QVariantList res;
	res << "CAP";
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, n);
	QVariantList l;
	l << ra*M_180_PI << dec*M_180_PI;
	res << QVariant(l);
	res << std::acos(d)*M_180_PI;
	return res;
}

SphericalRegionP SphericalCap::deserialize(QDataStream& in)
{
	Vec3d nn;
	double dd;
	in >> nn >> dd;
	return SphericalRegionP(new SphericalCap(nn, dd));
}

////////////////////////////////////////////////////////////////////////////
// Methods for SphericalPoint
////////////////////////////////////////////////////////////////////////////
bool SphericalPoint::intersects(const SphericalPolygon& r) const
{
	return r.contains(n);
}

bool SphericalPoint::intersects(const SphericalConvexPolygon& r) const
{
	return r.contains(n);
}

OctahedronPolygon SphericalPoint::getOctahedronPolygon() const
{
	QVector<Vec3d> contour;
	contour << n << n << n;
	return OctahedronPolygon(contour);
}

QVariantList SphericalPoint::toQVariant() const
{
	QVariantList res;
	res << "POINT";
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, n);
	QVariantList l;
	l << ra*M_180_PI << dec*M_180_PI;
	res << l;
	return res;
}

SphericalRegionP SphericalPoint::deserialize(QDataStream& in)
{
	Vec3d nn;
	in >> nn;
	return SphericalRegionP(new SphericalPoint(nn));
}

////////////////////////////////////////////////////////////////////////////
// Methods for AllSkySphericalRegion
////////////////////////////////////////////////////////////////////////////
const SphericalRegionP AllSkySphericalRegion::staticInstance = SphericalRegionP(new AllSkySphericalRegion());

QVariantList AllSkySphericalRegion::toQVariant() const
{
	QVariantList res;
	res << "ALLSKY";
	return res;
}


////////////////////////////////////////////////////////////////////////////
// Methods for EmptySphericalRegion
////////////////////////////////////////////////////////////////////////////
const SphericalRegionP EmptySphericalRegion::staticInstance = SphericalRegionP(new EmptySphericalRegion());

QVariantList EmptySphericalRegion::toQVariant() const
{
	QVariantList res;
	res << "EMPTY";
	return res;
}

///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalPolygon
///////////////////////////////////////////////////////////////////////////////
SphericalCap SphericalPolygon::getBoundingCap() const
{
	SphericalCap res;
	octahedronPolygon.getBoundingCap(res.n, res.d);
	return res;
}

struct TriangleSerializer
{
	TriangleSerializer(const TriangleSerializer& ts) : triangleList(ts.triangleList) {}

	TriangleSerializer() {}
	inline void operator=(const TriangleSerializer &other){triangleList=other.triangleList;}
	inline void operator()(const Vec3d* v1, const Vec3d* v2, const Vec3d* v3,
			       const Vec2f* , const Vec2f* , const Vec2f* ,
			       const Vec3f* , const Vec3f* , const Vec3f* ,
						   unsigned int , unsigned int , unsigned int )
	{
		QVariantList triangle;
		double ra, dec;
		QVariantList l;
		StelUtils::rectToSphe(&ra, &dec, *v1);
		l << ra*M_180_PI << dec*M_180_PI;
		triangle << QVariant(l);
		l.clear();
		StelUtils::rectToSphe(&ra, &dec, *v2);
		l << ra*M_180_PI << dec*M_180_PI;
		triangle << QVariant(l);
		l.clear();
		StelUtils::rectToSphe(&ra, &dec, *v3);
		l << ra*M_180_PI << dec*M_180_PI;
		triangle << QVariant(l);
		Q_ASSERT(triangle.size()==3);
		triangleList << QVariant(triangle);
	}

	QVariantList triangleList;
};

QVariantList SphericalPolygon::toQVariant() const
{
	TriangleSerializer result = getFillVertexArray().foreachTriangle(TriangleSerializer());
	return result.triangleList;
}

void SphericalPolygon::serialize(QDataStream& out) const
{
	out << octahedronPolygon;
}

SphericalRegionP SphericalPolygon::deserialize(QDataStream& in)
{
	OctahedronPolygon p;
	in >> p;
	return SphericalRegionP(new SphericalPolygon(p));
}

bool SphericalPolygon::contains(const SphericalConvexPolygon& r) const {return octahedronPolygon.contains(r.getOctahedronPolygon());}
bool SphericalPolygon::intersects(const SphericalConvexPolygon& r) const {return r.intersects(*this);}

SphericalRegionP SphericalPolygon::multiUnion(const QList<SphericalRegionP>& regions, bool optimizeByPreGrouping)
{
	if (regions.size()==1)
		return regions.at(0);
	if (optimizeByPreGrouping)
	{
		static const double minOverlap = 0.2;
		// Try to first split the set of regions into groups of intersecting regions
		QList<QList<SphericalRegionP> > res;
		QList<SphericalCap> groupReferenceCap;
		for (const auto& newReg : regions)
		{
			bool createNewGroup = true;
			const SphericalCap& newRegBoundingCap = newReg->getBoundingCap();
			for (int i=0;i<res.size();++i)
			{
				// Make sure not to group full sky regions because it is usually not what we want.
				if (SphericalCap::relativeDiameterOverlap(newRegBoundingCap, groupReferenceCap.at(i))>minOverlap && newRegBoundingCap.d>-0.9)
				{
					// It intersects with the reference element of the group
					res[i].append(newReg);
					createNewGroup = false;
					break;
				}
			}
			if (createNewGroup)
			{
				QList<SphericalRegionP> newGroup;
				newGroup.append(newReg);
				res.append(newGroup);
				// The reference element of the group is defined as the first element
				groupReferenceCap.append(newRegBoundingCap);
			}
		}
		// res now contains n list of regions to union together		
		QList<SphericalRegionP> mappedRegions;
		for (const auto& l : res)
		{
			mappedRegions.append(SphericalPolygon::multiUnion(l));
		}
		return SphericalPolygon::multiUnion(mappedRegions);
	}
	else
	{
		// Just add all contours to one polygon
		QList<OctahedronPolygon> l;
		for (const auto& r : regions)
			l.append(r->getOctahedronPolygon());
		return SphericalRegionP(new SphericalPolygon(l));
	}
}

SphericalRegionP SphericalPolygon::multiIntersection(const QList<SphericalRegionP>& regions)
{
	if (regions.isEmpty())
		return EmptySphericalRegion::staticInstance;
	SphericalRegionP reg = regions.at(0);
	for (int i=1;i<regions.size();++i)
		reg = reg->getIntersection(regions.at(i));
	return reg;
}

///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalTexturedPolygon
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalConvexPolygon
///////////////////////////////////////////////////////////////////////////////

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

//! Return the area of the region in steradians.
double SphericalConvexPolygon::getArea() const
{
	double area = 0.;
	Vec3d ar[3];
	Vec3d v1, v2, v3;
	ar[0]=contour.at(0);
	for (int i=1;i<contour.size()-1;++i)
	{
		// Use Girard's theorem for each subtriangles
		ar[1]=contour.at(i);
		ar[2]=contour.at(i+1);
		v1 = ar[0] ^ ar[1];
		v2 = ar[1] ^ ar[2];
		v3 = ar[2] ^ ar[0];
		area += 2.*M_PI - v1.angle(v2) - v2.angle(v3) - v3.angle(v1);
	}
	return area;
}

//! Return a point located inside the region.
Vec3d SphericalConvexPolygon::getPointInside() const
{
	Q_ASSERT(!isEmpty());
	Vec3d p(contour.at(0));
	p+=contour.at(1);
	p+=contour.at(2);
	p.normalize();
	return p;
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
	if (!cachedBoundingCap.contains(p))
		return false;
	for (int i=0;i<contour.size()-1;++i)
	{
		if (!sideHalfSpaceContains(contour.at(i+1), contour.at(i), p))
			return false;
	}
	return sideHalfSpaceContains(contour.first(), contour.last(), p);
}

bool SphericalConvexPolygon::contains(const SphericalCap& c) const
{
	if (!cachedBoundingCap.contains(c))
		return false;
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

bool SphericalConvexPolygon::contains(const SphericalConvexPolygon& cvx) const
{
	if (!cachedBoundingCap.contains(cvx.cachedBoundingCap))
		return false;
	return containsConvexContour(cvx.getConvexContour().constData(), cvx.getConvexContour().size());
}

bool SphericalConvexPolygon::contains(const SphericalPolygon& poly) const
{
	if (!cachedBoundingCap.contains(poly.getBoundingCap()))
		return false;
	// For standard polygons, go through the full list of triangles
	const QVector<Vec3d>& vArray = poly.getFillVertexArray().vertex;
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

bool SphericalConvexPolygon::intersects(const SphericalConvexPolygon& cvx) const
{
	if (!cachedBoundingCap.intersects(cvx.cachedBoundingCap))
		return false;
	return !areAllPointsOutsideOneSide(cvx.contour) && !cvx.areAllPointsOutsideOneSide(contour);
}

bool SphericalConvexPolygon::intersects(const SphericalPolygon& poly) const
{
	if (!cachedBoundingCap.intersects(poly.getBoundingCap()))
		return false;
	// For standard polygons, go through the full list of triangles
	const QVector<Vec3d>& vArray = poly.getFillVertexArray().vertex;
	for (int i=0;i<vArray.size()/3;++i)
	{
		if (!areAllPointsOutsideOneSide(contour.constData(), contour.size(), vArray.constData()+i*3, 3) && !areAllPointsOutsideOneSide(vArray.constData()+i*3, 3, contour.constData(), contour.size()))
			return true;
	}
	return false;
}

// This algo is wrong
void SphericalConvexPolygon::updateBoundingCap()
{
	Q_ASSERT(contour.size()>2);
	// Use this crapy algorithm instead
	cachedBoundingCap.n.set(0,0,0);
	for (const auto& v : contour)
		cachedBoundingCap.n+=v;
	cachedBoundingCap.n.normalize();
	cachedBoundingCap.d = 1.;
	for (const auto& v : contour)
	{
		if (cachedBoundingCap.n*v<cachedBoundingCap.d)
			cachedBoundingCap.d = cachedBoundingCap.n*v;
	}
	cachedBoundingCap.d*=cachedBoundingCap.d>0 ? 0.9999999 : 1.0000001;
#ifndef NDEBUG
	for (const auto& v : contour)
		Q_ASSERT(cachedBoundingCap.contains(v));
#endif
}

QVariantList SphericalConvexPolygon::toQVariant() const
{
	QVariantList res;
	res << "CONVEX_POLYGON";
	QVariantList cv;
	double ra, dec;
	for (const auto& v : contour)
	{
		StelUtils::rectToSphe(&ra, &dec, v);
		QVariantList vv;
		vv << ra*M_180_PI << dec*M_180_PI;
		cv.append(static_cast<QVariant>(vv));
	}
	res << cv;
	return res;
}

SphericalRegionP SphericalConvexPolygon::deserialize(QDataStream& in)
{
	QVector<Vec3d> contour;
	in >> contour;
	return SphericalRegionP(new SphericalConvexPolygon(contour));
}

///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalTexturedConvexPolygon
///////////////////////////////////////////////////////////////////////////////
QVariantList SphericalTexturedConvexPolygon::toQVariant() const
{
	QVariantList res = SphericalConvexPolygon::toQVariant();
	QVariantList cv;
	for (const auto& v : textureCoords)
	{
		QVariantList vv;
		vv << v[0] << v[1];
		cv.append(static_cast<QVariant>(vv));
	}
	res << cv;
	return res;
}


///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalTexturedPolygon
///////////////////////////////////////////////////////////////////////////////
QVariantList SphericalTexturedPolygon::toQVariant() const
{
	Q_ASSERT(0);
	// TODO store a tesselated polygon?, including edge flags?
	return QVariantList();
}



///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////


Vec3d greatCircleIntersection(const Vec3d& p1, const Vec3d& p2, const Vec3d& p3, const Vec3d& p4, bool& ok)
{
	if (p3*p4>1.-1E-10)
	{
		// p3 and p4 are too close to avoid floating point problems
		ok=false;
		return p3;
	}
	Vec3d n2 = p3^p4;
	n2.normalize();
	return greatCircleIntersection(p1, p2, n2, ok);
}

Vec3d greatCircleIntersection(const Vec3d& p1, const Vec3d& p2, const Vec3d& n2, bool& ok)
{
	if (p1*p2>1.-1E-10)
	{
		// p1 and p2 are too close to avoid floating point problems
		ok=false;
		return p1;
	}
	Vec3d n1 = p1^p2;
	Q_ASSERT(std::fabs(n2.lengthSquared()-1.)<0.00000001);
	n1.normalize();
	// Compute the parametric equation of the line at the intersection of the 2 planes
	Vec3d u = n1^n2;
	if (u.length()<1e-7)
	{
		// The planes are parallel
		ok = false;
		return u;
	}
	u.normalize();

	// The 2 candidates are u and -u. Now need to find which point is the correct one.
	ok = true;

	n1 = p1; n1+=p2;
	n1.normalize();
	if (n1*u>0.)
		return u;
	else
		return -u;
}

///////////////////////////////////////////////////////////////////////////////
// Methods for SphericalRegionP
///////////////////////////////////////////////////////////////////////////////
SphericalRegionP SphericalRegionP::loadFromJson(QIODevice* in)
{
    return loadFromQVariant(StelJsonParser::parse(in).toMap());
}

SphericalRegionP SphericalRegionP::loadFromJson(const QByteArray& a)
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

struct TriangleDumper
{
	TriangleDumper(const TriangleDumper& ts) : triangleList(ts.triangleList) {}

	TriangleDumper() {}
	inline void operator=(const TriangleDumper &other){triangleList=other.triangleList;}
	inline void operator()(const Vec3d* v1, const Vec3d* v2, const Vec3d* v3,
			       const Vec2f* , const Vec2f* , const Vec2f* ,
			       const Vec3f* , const Vec3f* , const Vec3f* , // GZ NEW
						   unsigned int , unsigned int , unsigned int )
	{
		QVector<Vec3d> triangle;
		triangle << *v1 << *v2 << *v3;
		triangleList.append(triangle);
	}

	QVector<QVector<Vec3d > > triangleList;
};

QVector<QVector<Vec3d > > SphericalRegion::getSimplifiedContours() const
{
	TriangleDumper result = getFillVertexArray().foreachTriangle(TriangleDumper());
	return result.triangleList;
}

SphericalRegionP capFromQVariantList(const QVariantList& l)
{
	Q_ASSERT(l.at(0).toString()=="CAP");
	// We now parse a cap, the format is "CAP",[ra, dec],aperture
	if (l.size()!=3)
		throw std::runtime_error(qPrintable(QString("invalid CAP description: %1 (expect \"CAP\",[ra, dec],aperture)").arg(QString::fromUtf8(StelJsonParser::write(l)))));
	Vec3d v;
	parseRaDec(l.at(1), v);
	bool ok;
	double d = l.at(2).toDouble(&ok)*M_PI/180.;
	if (!ok)
		throw std::runtime_error(qPrintable(QString("invalid aperture angle: \"%1\" (expect a double value in degree)").arg(l.at(2).toString())));
	return SphericalRegionP(new SphericalCap(v,std::cos(d)));
}

QVector<Vec3d> pathFromQVariantList(const QVariantList& l)
{
	Q_ASSERT(l.at(0).toString()=="PATH");
	// We now parse a path single contour, the format is:
	// "PATH",[ra, dec],["greatCircleTo", [ra, dec]], ["smallCircle", [raAxis, decAxis], angle], [etc..]
	QVector<Vec3d> vertices;
	Vec3d v;
	parseRaDec(l.at(1), v);
	vertices.append(v);	// Starting point
	for (int k=2;k<l.size();++k)
	{
		const QVariantList& elemList = l.at(k).toList();
		if (elemList.size()<1)
			throw std::runtime_error(qPrintable(QString("invalid PATH description: \"%1\" (expect a list of greatCircleTo or smallCircle").arg(l.at(2).toString())));
		if (elemList.at(0)=="greatCircleTo")
		{
			parseRaDec(elemList.at(1), v);
			vertices.append(v);
			continue;
		}
		if (elemList.at(0)=="smallCircle")
		{
			Vec3d axis;
			parseRaDec(elemList.at(1), axis);
			bool ok;
			double angle = elemList.at(2).toDouble(&ok)*M_PI/180.;
			if (!ok || std::fabs(angle)>2.*M_PI)
				throw std::runtime_error(qPrintable(QString("invalid small circle rotation angle: \"%1\" (expect a double value in degree betwwen -2pi and 2pi)").arg(elemList.at(2).toString())));
			int nbStep = 1+static_cast<int>(std::fabs(angle)/(2.*M_PI)*75);
			Q_ASSERT(nbStep>0);
			v = vertices.last();
			const Mat4d& rotMat = Mat4d::rotation(axis, angle/nbStep);
			for (int step=0; step<nbStep;++step)
			{
				v.transfo4d(rotMat);
				vertices.append(v);
			}
			continue;
		}
		throw std::runtime_error(qPrintable(QString("invalid PATH description: \"%1\" (expect a list of greatCircleTo or smallCircle").arg(l.at(2).toString())));
	}
	Q_ASSERT(vertices.size()>2);
	return vertices;
}

QVector<Vec3d> singleContourFromQVariantList(const QVariantList& l)
{
	if (l.size()<3)
		throw std::runtime_error("a polygon contour must have at least 3 vertices");
	QVector<Vec3d> vertices;
	Vec3d v;
	for (const auto& vRaDec : l)
	{
		parseRaDec(vRaDec, v);
		vertices.append(v);
	}
	Q_ASSERT(vertices.size()>2);
	return vertices;
}

SphericalRegionP SphericalRegionP::loadFromQVariant(const QVariantList& l)
{
	if (l.isEmpty())
		return EmptySphericalRegion::staticInstance;
	if (l.at(0).type()==QVariant::List)
	{
		// The region is composed of either:
		// - a list of regions, which are assumed to be combined using the positive winding rule.
		// - or a single contour
		QVector<QVector<Vec3d> > contours;
		try {
			Vec3d v;
			parseRaDec(l.at(0), v);
			// No exception was thrown, we are parsing a single contour
			contours.append(singleContourFromQVariantList(l));
		}
		catch (std::runtime_error&)
		{
			// We are parsing a list of regions.
			for (int i=0;i<l.size();++i)
			{
				const QVariantList& subL = l.at(i).toList();
				if (subL.isEmpty())
					throw std::runtime_error(qPrintable(QString("invalid region definition: %1").arg(l.at(i).toString())));
				if (subL.at(0).type()==QVariant::List)
				{
					// Special optimization for basic contours (if no type is provided, assume a polygon)
					contours.append(singleContourFromQVariantList(subL));
					continue;
				}
				Q_ASSERT(subL.at(0).type()==QVariant::String || subL.at(0).type()==QVariant::ByteArray);
				const SphericalRegionP& reg = loadFromQVariant(subL);
				if (!reg->isEmpty())
					contours << reg->getSimplifiedContours();
			}
		}
		return SphericalRegionP(new SphericalPolygon(contours));
	}
	
	Q_ASSERT(l.at(0).type()==QVariant::String || l.at(0).type()==QVariant::ByteArray);
	const QString& code=l.at(0).toString();
	if (code=="CAP")
		return capFromQVariantList(l);
	else if (code=="INTERSECTION")
	{
		Q_ASSERT(l.size()>2);
		SphericalRegionP reg1 = loadFromQVariant(l.at(1).toList());
		for (int n=2;n<l.size();++n)
		{
			SphericalRegionP reg2 = loadFromQVariant(l.at(n).toList());
			reg1 = reg1->getIntersection(reg2.data());
		}
		return reg1;
	}
	else if (code=="SUBTRACTION")
	{
		Q_ASSERT(l.size()==3);
		SphericalRegionP reg1 = loadFromQVariant(l.at(1).toList());
		SphericalRegionP reg2 = loadFromQVariant(l.at(2).toList());
		SphericalRegionP regIntersection = reg1->getIntersection(reg2.data());
		reg1 = reg1->getSubtraction(regIntersection.data());
		return reg1;
	}
	else if (code=="PATH")
	{
		return SphericalRegionP(new SphericalPolygon(pathFromQVariantList(l)));
	}
	else if (code=="CONVEX_POLYGON")
	{
		return SphericalRegionP(new SphericalConvexPolygon(singleContourFromQVariantList(l.at(1).toList())));
	}

	Q_ASSERT(0);
	return EmptySphericalRegion::staticInstance;
}

SphericalRegionP SphericalRegionP::loadFromQVariant(const QVariantMap& map)
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
		return loadFromQVariant(contoursList);
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
			for (const auto& vRaDec : polyRaDecToList)
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
				vertices[n].texCoord.set(vl.at(0).toFloat(&ok), vl.at(1).toFloat(&ok));
				if (!ok)
					throw std::runtime_error("invalid texture coordinate pair (expect 2 double values in degree)");
			}
			contours.append(vertices);
			vertices.clear();
		}
		return SphericalRegionP(new SphericalTexturedPolygon(contours));
	}
	// Q_ASSERT(0);
	// return SphericalRegionP(new SphericalCap());
}

void SphericalRegionP::serializeToJson(const QVariant& jsonObject, QIODevice* output, int indentLevel)
{
	const SphericalRegionP& reg = jsonObject.value<SphericalRegionP>();
	StelJsonParser::write(reg->toQVariant(), output, indentLevel);
}
