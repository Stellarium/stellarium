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

#ifndef STELSPHEREGEOMETRY_HPP
#define STELSPHEREGEOMETRY_HPP

#include "OctahedronPolygon.hpp"
#include "StelVertexArray.hpp"
#include "VecMath.hpp"

#include <QVector>
#include <QVariant>
#include <QDebug>
#include <QSharedPointer>
#include <QVarLengthArray>
#include <QDataStream>

#include <cstdio>

class SphericalRegion;
class SphericalPolygon;
class SphericalConvexPolygon;
class SphericalCap;
class SphericalPoint;
class AllSkySphericalRegion;
class EmptySphericalRegion;

//! @file StelSphereGeometry.hpp
//! Define all SphericalGeometry primitives as well as the SphericalRegionP type.

//! @class SphericalRegionP
//! A shared pointer on a SphericalRegion.
class SphericalRegionP : public QSharedPointer<SphericalRegion>
{
public:
	// Override the constructors of QSharedPointer
	SphericalRegionP() {;}
	SphericalRegionP(SphericalRegion* ptr) : QSharedPointer<SphericalRegion>(ptr) {;}
	template <class Deleter> SphericalRegionP(SphericalRegion* ptr, Deleter deleter) : QSharedPointer<SphericalRegion>(ptr, deleter) {;}
	SphericalRegionP(const SphericalRegionP& other) : QSharedPointer<SphericalRegion>(other) {;}
	SphericalRegionP(const QWeakPointer<SphericalRegion>& other) : QSharedPointer<SphericalRegion>(other) {;}
	inline SphericalRegionP& operator=(const SphericalRegionP &other){QSharedPointer<SphericalRegion>::operator=(other); return *this;}

	//! Create a SphericalRegion from the given input JSON stream.
	//! The type of the region is automatically recognized from the input format.
	//! The currently recognized format are:
	//! <ul>
	//! <li>Contour list:
	//! @code[[contour1], [contour2],[...]]@endcode
	//! This format consists of a list of contours. Contours defined in counterclockwise direction (CW) are
	//! added to the final region while CCW contours are subtracted (they can be used to form complex polygons even with holes).
	//! Each contour can have different format:
	//! <ul>
	//! <li>
	//! @code[[ra,dec], [ra,dec], [ra,dec], [ra,dec]]@endcode
	//! A list of points connected by great circles with the last point connected to the first one.
	//! Each point is defined by its ra,dec coordinates in degrees.</li>
	//! <li>
	//! @code["CAP", [raCenter,decCenter], ap]@endcode
	//! A spherical cap centered on raCenter/decCenter with aperture radius ap in degrees. For example:
	//! @code["CAP", [0.,-90.], 90]@endcode
	//! define the region covering the entire southern hemisphere.</li>
	//! <li>
	//! @code["PATH", [startRa,startDec], [[prim1],[prim2],[prim3],...]]@endcode
	//! A path is a contour starting a startRa/startDec and defined by a list of primitives. Each primitive can be either
	//! a great circle or a small circle. Great circles are defined by the point to link to: ["greatCircleTo", [ra,dec]]. Small
	//! circles are defined by a rotation axis and a rotation angle: ["smallCircle",[0,90],-88]. A full example is:
	//! @code["PATH",[330,-25],[["smallCircle",[0,90],82.5],["greatCircleTo",[52.5,-35]],["smallCircle",[0,90],-82.5]]]@endcode
	//! This defines a box of 82.5x10 degrees following latitude and longitude lines.</li>
	//! </ul>
	//! <li>Polygon vertex list with associated texture coordinates:
	//! @code{
	//!     "worldCoords": [[[ra,dec],[ra,dec],[ra,dec],[ra,dec]], [[ra,dec],[ra,dec],[ra,dec]],[...]],
	//!     "textureCoords": [[[u,v],[u,v],[u,v],[u,v]], [[u,v],[u,v],[u,v]], [...]]
	//! }@endcode
	//! The format is used to describe textured polygons. The worldCoords part is similar to the one described above.
	//! The textureCoords part is the addition of a list of texture coordinates in the u,v texture space (between 0 and 1).
	//! There must be one texture coordinate for each vertex.</li>
	//! </ul>
	//! @param in an open QIODevice ready for read.
	//! @throws std::runtime_error when there was an error while parsing the file.
	static SphericalRegionP loadFromJson(QIODevice* in);

	//! Create a SphericalRegion from the given QByteArray containing the JSON data.
	//! @see loadFromJson(QIODevice* in) for format info.
	static SphericalRegionP loadFromJson(const QByteArray& a);

	//! Create a SphericalRegion from the given QVariantMap with a format matching the JSON file parsed in loadFromJson().
	//! @param map a valid QVariantMap which can be created e.g. from parsing a JSON file with the StelJsonParser class.
	static SphericalRegionP loadFromQVariant(const QVariantMap& map);
	// It can only be a pure shape definition, without texture coords
	static SphericalRegionP loadFromQVariant(const QVariantList& list);

	//! Method registered to JSON serializer.
	static void serializeToJson(const QVariant& jsonObject, QIODevice* output, int indentLevel=0);

	//! The meta type ID associated to a SphericalRegionP.
	static int metaTypeId;

private:
	//! Initialize stuff to allow SphericalRegionP to be used with Qt meta type system.
	static int initialize();
};

// Allow to use SphericalRegionP with the Qt MetaType system.
Q_DECLARE_METATYPE(SphericalRegionP)

//! Serialize the passed SphericalRegionP into a binary blob.
QDataStream& operator<<(QDataStream& out, const SphericalRegionP& region);
//! Load the SphericalRegionP from a binary blob.
QDataStream& operator>>(QDataStream& in, SphericalRegionP& region);

//! @class SphericalRegion
//! Abstract class defining a region of the sphere. It provides default implementation for the general non-convex polygon
//! which can extend on more than 180 deg based on the OctahedronPolygon class.
//! Subclasses provides special faster implementations of many methods.
class SphericalRegion
{
public:
	//! @enum SphericalRegionType define types for all supported regions.
	enum SphericalRegionType
	{
		Point = 0,
		Cap = 1,
		AllSky = 2,
		Polygon = 3,
		ConvexPolygon = 4,
		Empty = 5,
		Invalid = 6
	};

	virtual ~SphericalRegion() {;}

	virtual SphericalRegionType getType() const = 0;

	//! Return the octahedron contour representation of the polygon.
	//! It can be used for safe computation of intersection/union in the general case.
	virtual OctahedronPolygon getOctahedronPolygon() const =0;

	//! Return the area of the region in steradians.
	virtual double getArea() const {return getOctahedronPolygon().getArea();}

	//! Return true if the region is empty.
	virtual bool isEmpty() const {return getOctahedronPolygon().isEmpty();}

	//! Return a point located inside the region.
	virtual Vec3d getPointInside() const {return getOctahedronPolygon().getPointInside();}

	//! Return the list of SphericalCap bounding the ConvexPolygon.
	virtual QVector<SphericalCap> getBoundingSphericalCaps() const;

	//! Return a bounding SphericalCap. This method is heavily used and therefore needs to be very fast.
	//! The returned SphericalCap doesn't have to be the smallest one, but smaller is better.
	virtual SphericalCap getBoundingCap() const;

	//! Return an enlarged version of this SphericalRegion so that any point distant of more
	//! than the given margin now lays within the region.
	//! The returned region can be larger than the smallest enlarging region, therefore returning
	//! false positive on subsequent intersection tests.
	//! The default implementation always return an enlarged bounding SphericalCap.
	//! @param margin the minimum enlargement margin in radian.
	virtual SphericalRegionP getEnlarged(double margin) const;

	//! Return an openGL compatible array to be displayed using vertex arrays.
	virtual StelVertexArray getFillVertexArray() const {return getOctahedronPolygon().getFillVertexArray();}

	//! Get the outline of the contours defining the SphericalPolygon.
	//! @return a list of vertex which taken 2 by 2 define the contours of the polygon.
	virtual StelVertexArray getOutlineVertexArray() const {return getOctahedronPolygon().getOutlineVertexArray();}

	//! Get the contours defining the SphericalPolygon when combined using a positive winding rule.
	//! The default implementation return a list of tesselated triangles derived from the OctahedronPolygon.
	virtual QVector<QVector<Vec3d > > getSimplifiedContours() const;
	
	//! Serialize the region into a QVariant list matching the JSON format.
	virtual QVariantList toQVariant() const = 0;

	//! Serialize the region. This method must allow as fast as possible serialization and work with deserialize().
	virtual void serialize(QDataStream& out) const = 0;

	//! Output a JSON string representing the polygon.
	//! This method is convenient for debugging.
	QByteArray toJSON() const;

	//! Returns whether a SphericalRegion is contained into this region.
	//! A default potentially very slow implementation is provided for each cases.
	bool contains(const SphericalRegion* r) const;
	bool contains(const SphericalRegionP r) const {return contains(r.data());}
	virtual bool contains(const Vec3d& p) const {return getOctahedronPolygon().contains(p);}
	virtual bool contains(const SphericalPolygon& r) const;
	virtual bool contains(const SphericalConvexPolygon& r) const;
	virtual bool contains(const SphericalCap& r) const;
	virtual bool contains(const SphericalPoint& r) const;
	virtual bool contains(const AllSkySphericalRegion& r) const;
	bool contains(const EmptySphericalRegion&) const {return false;}

	//! Returns whether a SphericalRegion intersects with this region.
	//! A default potentially very slow implementation is provided for each cases.
	bool intersects(const SphericalRegion* r) const;
	bool intersects(const SphericalRegionP r) const {return intersects(r.data());}
	bool intersects(const Vec3d& p) const {return contains(p);}
	virtual bool intersects(const SphericalPolygon& r) const;
	virtual bool intersects(const SphericalConvexPolygon& r) const;
	virtual bool intersects(const SphericalCap& r) const;
	virtual bool intersects(const SphericalPoint& r) const;
	virtual bool intersects(const AllSkySphericalRegion& r) const;
	bool intersects(const EmptySphericalRegion&) const {return false;}

	//! Return a new SphericalRegion consisting of the intersection of this and the given region.
	//! A default potentially very slow implementation is provided for each cases.
	SphericalRegionP getIntersection(const SphericalRegion* r) const;
	SphericalRegionP getIntersection(const SphericalRegionP r) const {return getIntersection(r.data());}
	virtual SphericalRegionP getIntersection(const SphericalPolygon& r) const;
	virtual SphericalRegionP getIntersection(const SphericalConvexPolygon& r) const;
	virtual SphericalRegionP getIntersection(const SphericalCap& r) const;
	virtual SphericalRegionP getIntersection(const SphericalPoint& r) const;
	virtual SphericalRegionP getIntersection(const AllSkySphericalRegion& r) const;
	SphericalRegionP getIntersection(const EmptySphericalRegion& r) const;

	//! Return a new SphericalRegion consisting of the union of this and the given region.
	//! A default potentially very slow implementation is provided for each cases.
	SphericalRegionP getUnion(const SphericalRegion* r) const;
	SphericalRegionP getUnion(const SphericalRegionP r) const {return getUnion(r.data());}
	virtual SphericalRegionP getUnion(const SphericalPolygon& r) const;
	virtual SphericalRegionP getUnion(const SphericalConvexPolygon& r) const;
	virtual SphericalRegionP getUnion(const SphericalCap& r) const;
	virtual SphericalRegionP getUnion(const SphericalPoint& r) const;
	SphericalRegionP getUnion(const AllSkySphericalRegion& r) const;
	virtual SphericalRegionP getUnion(const EmptySphericalRegion& r) const;

	//! Return a new SphericalRegion consisting of the subtraction of the given region from this.
	//! A default potentially very slow implementation is provided for each cases.
	SphericalRegionP getSubtraction(const SphericalRegion* r) const;
	SphericalRegionP getSubtraction(const SphericalRegionP r) const {return getSubtraction(r.data());}
	virtual SphericalRegionP getSubtraction(const SphericalPolygon& r) const;
	virtual SphericalRegionP getSubtraction(const SphericalConvexPolygon& r) const;
	virtual SphericalRegionP getSubtraction(const SphericalCap& r) const;
	virtual SphericalRegionP getSubtraction(const SphericalPoint& r) const;
	SphericalRegionP getSubtraction(const AllSkySphericalRegion& r) const;
	virtual SphericalRegionP getSubtraction(const EmptySphericalRegion& r) const;

private:
	bool containsDefault(const SphericalRegion* r) const;
	bool intersectsDefault(const SphericalRegion* r) const;
	SphericalRegionP getIntersectionDefault(const SphericalRegion* r) const;
	SphericalRegionP getUnionDefault(const SphericalRegion* r) const;
	SphericalRegionP getSubtractionDefault(const SphericalRegion* r) const;
};


//! @class SphericalCap
//! A SphericalCap is defined by a direction and an aperture.
//! It forms a cone from the center of the Coordinate frame with a radius d.
//! It is a disc on the sphere, a region above a circle on the unit sphere.
class SphericalCap : public SphericalRegion
{
	using SphericalRegion::contains;
public:
	//! Construct a SphericalCap with a 90 deg aperture and an undefined direction.
	SphericalCap() : d(0) {;}

	//! Construct a SphericalCap from its direction and assumes a 90 deg aperture.
	SphericalCap(double x, double y, double z) : n(x,y,z), d(0) {;}

	//! Construct a SphericalCap from its direction and aperture.
	//! @param an a unit vector indicating the direction.
	//! @param ar cosinus of the aperture.
	SphericalCap(const Vec3d& an, double ar) : n(an), d(ar) {//n.normalize();
		Q_ASSERT(d==0 || qFuzzyCompare(n.lengthSquared(),1.));}
	// FIXME: GZ reports 2013-03-02: apparently the Q_ASSERT is here because n should be normalized at this point, but
	// for efficiency n.normalize() should not be called at this point.
	// However, when zooming in a bit in Hammer-Aitoff and Mercator projections, this Assertion fires.
	// Atmosphere must be active
	// It may have to do with DSO texture rendering.
	// found at r5863.
	// n.normalize() prevents this for now, but may cost performance.
	// AARGH - activating n.normalize() inhibits mouse-identification/selection of stars!
	// May be compiler dependent (seen on Win/MinGW), AW cannot confirm it on Linux.

	//! Copy constructor.
	SphericalCap(const SphericalCap& other) : SphericalRegion(), n(other.n), d(other.d) {;}
	inline SphericalCap& operator=(const SphericalCap &other){n=other.n; d=other.d; return *this;}

	virtual SphericalRegionType getType() const {return SphericalRegion::Cap;}
	virtual OctahedronPolygon getOctahedronPolygon() const;

	//! Get the area of the intersection of the halfspace on the sphere in steradian.
	virtual double getArea() const {return 2.*M_PI*(1.-d);}

	//! Return true if the region is empty.
	virtual bool isEmpty() const {return d>=1.;}

	//! Return a point located inside the SphericalCap.
	virtual Vec3d getPointInside() const {return n;}

	//! Return itself.
	virtual SphericalCap getBoundingCap() const {return *this;}

	// Contain and intersect	
	virtual bool contains(const Vec3d &v) const {Q_ASSERT(d==0 || std::fabs(v.lengthSquared()-1.)<0.0000002);return (v*n>=d);}
	virtual bool contains(const Vec3f &v) const {Q_ASSERT(d==0 || std::fabs(v.lengthSquared()-1.f)<0.000002f);return (v.toVec3d()*n>=d);}
	virtual bool contains(const SphericalConvexPolygon& r) const;
	virtual bool contains(const SphericalCap& h) const
	{
		const double a = n*h.n-d*h.d;
		return d<=h.d && ( a>=1. || (a>=0. && a*a >= (1.-d*d)*(1.-h.d*h.d)));
	}
	virtual bool contains(const AllSkySphericalRegion&) const {return d<=-1;}
	virtual bool intersects(const SphericalPolygon& r) const;
	virtual bool intersects(const SphericalConvexPolygon& r) const;
	//! Returns whether a SphericalCap intersects with this one.
	//! I managed to make it without sqrt or acos, so it is very fast!
	//! @see http://f4bien.blogspot.com/2009/05/spherical-geometry-optimisations.html for detailed explanations.
	virtual bool intersects(const SphericalCap& h) const
	{
		const double a = d*h.d - n*h.n;
		return d+h.d<=0. || a<=0. || (a<=1. && a*a <= (1.-d*d)*(1.-h.d*h.d));
	}
	virtual bool intersects(const AllSkySphericalRegion&) const {return d<=1.;}

	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is ["CAP", [ra, dec], radius], with ra dec in degree in ICRS frame
	//! and radius in degree (between 0 and 180 deg)
	virtual QVariantList toQVariant() const;

	virtual void serialize(QDataStream& out) const {out << n << d;}

	////////////////////////////////////////////////////////////////////
	// Methods specific to SphericalCap

	//! Return the radiusof the cap  in radian
	double getRadius() const {return std::acos(d);}

	//! Returns whether a HalfSpace (like a SphericalCap with d=0) intersects with this SphericalCap.
	//! @param hn0 the x direction of the halfspace.
	//! @param hn1 the y direction of the halfspace.
	//! @param hn2 the z direction of the halfspace.
	inline bool intersectsHalfSpace(double hn0, double hn1, double hn2) const
	{
		const double a = n[0]*hn0+n[1]*hn1+n[2]*hn2;
		return d<=0. || a<=0. || (a<=1. && a*a <= (1.-d*d));
	}

	//! Clip the passed great circle connecting points v1 and v2.
	//! @return true if the great circle intersects with the cap, false otherwise.
	bool clipGreatCircle(Vec3d& v1, Vec3d& v2) const;

	//! Comparison operator.
	bool operator==(const SphericalCap& other) const {return (n==other.n && d==other.d);}

	//! Return the list of closed contours defining the polygon boundaries.
	QVector<Vec3d> getClosedOutlineContour() const;

	//! Return whether the cap intersect with a convex contour defined by nbVertice.
	bool intersectsConvexContour(const Vec3d* vertice, int nbVertice) const;

	//! Return whether the cap contains the passed triangle.
	bool containsTriangle(const Vec3d* vertice) const;

	//! Return whether the cap intersect with the passed triangle.
	bool intersectsTriangle(const Vec3d* vertice) const;

	//! Deserialize the region. This method must allow as fast as possible deserialization.
	static SphericalRegionP deserialize(QDataStream& in);

	//! Return the relative overlap between the areas of the 2 caps, i.e:
	//! min(intersectionArea/c1.area, intersectionArea/c2.area)
	static double relativeAreaOverlap(const SphericalCap& c1, const SphericalCap& c2);

	//! Return the relative overlap between the diameter of the 2 caps, i.e:
	//! min(intersectionDistance/c1.diameter, intersectionDistance/c2.diameter)
	static double relativeDiameterOverlap(const SphericalCap& c1, const SphericalCap& c2);

	//! Compute the intersection of 2 halfspaces on the sphere (usually on 2 points) and return it in p1 and p2.
	//! If the 2 SphericalCap don't intersect or intersect only at 1 point, false is returned and p1 and p2 are undefined.
	static bool intersectionPoints(const SphericalCap& h1, const SphericalCap& h2, Vec3d& p1, Vec3d& p2);

	//! The direction unit vector. Only if d==0, this vector doesn't need to be unit.
	Vec3d n;
	//! The cos of cone radius
	double d;
};

//! Return whether the halfspace defined by the vectors v1^v2 and with aperture 90 deg contains the point p.
//! The comparison is made with a double number slightly smaller than zero to avoid floating point precision
//! errors problems (one test fails if it is set to zero).
inline bool sideHalfSpaceContains(const Vec3d& v1, const Vec3d& v2, const Vec3d& p)
{
	return (v1[1] * v2[2] - v1[2] * v2[1])*p[0] +
			(v1[2] * v2[0] - v1[0] * v2[2])*p[1] +
			(v1[0] * v2[1] - v1[1] * v2[0])*p[2]>=-1e-17;
}

//! Return whether the halfspace defined by the vectors v1 and v2 contains the SphericalCap h.
inline bool sideHalfSpaceContains(const Vec3d& v1, const Vec3d& v2, const SphericalCap& h)
{
	Vec3d n(v1[1]*v2[2]-v1[2]*v2[1], v2[0]*v1[2]-v2[2]*v1[0], v2[1]*v1[0]-v2[0]*v1[1]);
	n.normalize();
	const double a = n*h.n;
	return 0<=h.d && ( a>=1. || (a>=0. && a*a >= 1.-h.d*h.d));
}

//! Return whether the halfspace defined by the vectors v1 and v2 intersects the SphericalCap h.
inline bool sideHalfSpaceIntersects(const Vec3d& v1, const Vec3d& v2, const SphericalCap& h)
{
	Vec3d n(v1[1]*v2[2]-v1[2]*v2[1], v2[0]*v1[2]-v2[2]*v1[0], v2[1]*v1[0]-v2[0]*v1[1]);
	n.normalize();
	return  h.intersectsHalfSpace(n[0], n[1], n[2]);
}

//! @class SphericalPoint
//! Special SphericalRegion for a point on the sphere.
class SphericalPoint : public SphericalRegion
{
public:
	SphericalPoint(const Vec3d& an) : n(an) {Q_ASSERT(std::fabs(1.-n.length())<0.0000001);}
	SphericalPoint(const SphericalPoint &other): n(other.n){}
	inline SphericalPoint& operator=(const SphericalPoint &other){n=other.n; return *this;}
	virtual ~SphericalPoint() {;}

	virtual SphericalRegionType getType() const {return SphericalRegion::Point;}
	virtual OctahedronPolygon getOctahedronPolygon() const;
	virtual double getArea() const {return 0.;}
	virtual bool isEmpty() const {return false;}
	virtual Vec3d getPointInside() const {return n;}
	virtual SphericalCap getBoundingCap() const {return SphericalCap(n, 1);}
	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is ["POINT", [ra, dec]], with ra dec in degree in ICRS frame.
	virtual QVariantList toQVariant() const;
	virtual void serialize(QDataStream& out) const {out << n;}

	// Contain and intersect
	virtual bool contains(const Vec3d& p) const {return n==p;}
	virtual bool contains(const SphericalPolygon&) const {return false;}
	virtual bool contains(const SphericalConvexPolygon&) const {return false;}
	virtual bool contains(const SphericalCap&) const {return false;}
	virtual bool contains(const SphericalPoint& r) const {return n==r.n;}
	virtual bool contains(const AllSkySphericalRegion&) const {return false;}
	virtual bool intersects(const SphericalPolygon&) const;
	virtual bool intersects(const SphericalConvexPolygon&) const;
	virtual bool intersects(const SphericalCap& r) const {return r.contains(n);}
	virtual bool intersects(const SphericalPoint& r) const {return n==r.n;}
	virtual bool intersects(const AllSkySphericalRegion&) const {return true;}

	//! Deserialize the region. This method must allow as fast as possible deserialization.
	static SphericalRegionP deserialize(QDataStream& in);

	//! The unit vector of the point direction.
	Vec3d n;
};

//! @class AllSkySphericalRegion
//! Special SphericalRegion for the whole sphere.
class AllSkySphericalRegion : public SphericalRegion
{
public:
	virtual ~AllSkySphericalRegion() {;}

	virtual SphericalRegionType getType() const {return SphericalRegion::AllSky;}
	virtual OctahedronPolygon getOctahedronPolygon() const {return OctahedronPolygon::getAllSkyOctahedronPolygon();}
	virtual double getArea() const {return 4.*M_PI;}
	virtual bool isEmpty() const {return false;}
	virtual Vec3d getPointInside() const {return Vec3d(1,0,0);}
	virtual SphericalCap getBoundingCap() const {return SphericalCap(Vec3d(1,0,0), -2);}
	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is ["ALLSKY"]
	virtual QVariantList toQVariant() const;
	virtual void serialize(QDataStream&) const {;}

	// Contain and intersect
	virtual bool contains(const Vec3d&) const {return true;}
	virtual bool contains(const SphericalPolygon&) const {return true;}
	virtual bool contains(const SphericalConvexPolygon&) const {return true;}
	virtual bool contains(const SphericalCap&) const {return true;}
	virtual bool contains(const SphericalPoint&) const {return true;}
	virtual bool contains(const AllSkySphericalRegion&) const {return true;}
	virtual bool intersects(const SphericalPolygon&) const {return true;}
	virtual bool intersects(const SphericalConvexPolygon&) const {return true;}
	virtual bool intersects(const SphericalCap&) const {return true;}
	virtual bool intersects(const SphericalPoint&) const {return true;}
	virtual bool intersects(const AllSkySphericalRegion&) const {return true;}

	static const SphericalRegionP staticInstance;
};

//! @class EmptySphericalRegion
//! Special SphericalRegion for --- UMM, WHAT EXACTLY?
class EmptySphericalRegion : public SphericalRegion
{
public:
	// Avoid name hiding when overloading the virtual methods.
	using SphericalRegion::intersects;
	using SphericalRegion::contains;
	using SphericalRegion::getIntersection;
	using SphericalRegion::getUnion;
	using SphericalRegion::getSubtraction;

	EmptySphericalRegion() {;}
	virtual ~EmptySphericalRegion() {;}

	virtual SphericalRegionType getType() const {return SphericalRegion::Empty;}
	virtual OctahedronPolygon getOctahedronPolygon() const {return OctahedronPolygon::getEmptyOctahedronPolygon();}
	virtual double getArea() const {return 0.;}
	virtual bool isEmpty() const {return true;}
	virtual Vec3d getPointInside() const {return Vec3d(1,0,0);}
	virtual SphericalCap getBoundingCap() const {return SphericalCap(Vec3d(1,0,0), 2);}
	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is ["EMPTY"]
	virtual QVariantList toQVariant() const;
	virtual void serialize(QDataStream&) const {;}

	// Contain and intersect
	virtual bool contains(const Vec3d&) const {return false;}
	virtual bool contains(const SphericalPolygon&) const {return false;}
	virtual bool contains(const SphericalConvexPolygon&) const {return false;}
	virtual bool contains(const SphericalCap&) const {return false;}
	virtual bool contains(const SphericalPoint&) const {return false;}
	virtual bool contains(const AllSkySphericalRegion&) const {return false;}
	virtual bool intersects(const SphericalPolygon&) const {return false;}
	virtual bool intersects(const SphericalConvexPolygon&) const {return false;}
	virtual bool intersects(const SphericalCap&) const {return false;}
	virtual bool intersects(const SphericalPoint&) const {return false;}
	virtual bool intersects(const AllSkySphericalRegion&) const {return false;}

	static const SphericalRegionP staticInstance;
};


//! @class SphericalPolygon
//! Class defining default implementations for some spherical geometry methods.
//! All methods are reentrant.
class SphericalPolygon : public SphericalRegion
{
public:
	// Avoid name hiding when overloading the virtual methods.
	using SphericalRegion::intersects;
	using SphericalRegion::contains;
	using SphericalRegion::getIntersection;
	using SphericalRegion::getUnion;
	using SphericalRegion::getSubtraction;

	SphericalPolygon() {;}
	//! Constructor from a list of contours.
	SphericalPolygon(const QVector<QVector<Vec3d> >& contours) : octahedronPolygon(contours) {;}
	//! Constructor from one contour.
	SphericalPolygon(const QVector<Vec3d>& contour) : octahedronPolygon(contour) {;}
	SphericalPolygon(const OctahedronPolygon& octContour) : octahedronPolygon(octContour) {;}
	SphericalPolygon(const QList<OctahedronPolygon>& octContours) : octahedronPolygon(octContours) {;}

	virtual SphericalRegionType getType() const {return SphericalRegion::Polygon;}
	virtual OctahedronPolygon getOctahedronPolygon() const {return octahedronPolygon;}

	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is:
	//! @code[[[ra,dec], [ra,dec], [ra,dec], [ra,dec]], [[ra,dec], [ra,dec], [ra,dec]],[...]]@endcode
	//! it is a list of closed contours, with each points defined by ra dec in degree in the ICRS frame.
	virtual QVariantList toQVariant() const;
	virtual void serialize(QDataStream& out) const;

	virtual SphericalCap getBoundingCap() const;

	virtual bool contains(const Vec3d& p) const {return octahedronPolygon.contains(p);}
	virtual bool contains(const SphericalPolygon& r) const {return octahedronPolygon.contains(r.octahedronPolygon);}
	virtual bool contains(const SphericalConvexPolygon& r) const;
	virtual bool contains(const SphericalCap& r) const {return octahedronPolygon.contains(r.getOctahedronPolygon());}
	virtual bool contains(const SphericalPoint& r) const {return octahedronPolygon.contains(r.n);}
	virtual bool contains(const AllSkySphericalRegion& r) const {return octahedronPolygon.contains(r.getOctahedronPolygon());}

	virtual bool intersects(const SphericalPolygon& r) const {return octahedronPolygon.intersects(r.octahedronPolygon);}
	virtual bool intersects(const SphericalConvexPolygon& r) const;
	virtual bool intersects(const SphericalCap& r) const {return r.intersects(*this);}
	virtual bool intersects(const SphericalPoint& r) const {return octahedronPolygon.contains(r.n);}
	virtual bool intersects(const AllSkySphericalRegion&) const {return !isEmpty();}

	virtual SphericalRegionP getIntersection(const SphericalPoint& r) const {return contains(r.n) ? SphericalRegionP(new SphericalPoint(r)) : EmptySphericalRegion::staticInstance;}
	virtual SphericalRegionP getIntersection(const AllSkySphericalRegion& ) const {return SphericalRegionP(new SphericalPolygon(octahedronPolygon));}

	virtual SphericalRegionP getUnion(const SphericalPoint&) const {return SphericalRegionP(new SphericalPolygon(octahedronPolygon));}
	virtual SphericalRegionP getUnion(const EmptySphericalRegion&) const {return SphericalRegionP(new SphericalPolygon(octahedronPolygon));}

	virtual SphericalRegionP getSubtraction(const SphericalPoint&) const {return SphericalRegionP(new SphericalPolygon(octahedronPolygon));}
	virtual SphericalRegionP getSubtraction(const EmptySphericalRegion&) const {return SphericalRegionP(new SphericalPolygon(octahedronPolygon));}

	////////////////////////////////////////////////////////////////////
	// Methods specific to SphericalPolygon
	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area. The contours are combined using
	//! the positive winding rule, meaning that the polygon is the union of the positive contours minus the negative ones.
	void setContours(const QVector<QVector<Vec3d> >& contours) {octahedronPolygon = OctahedronPolygon(contours);}

	//! Set a single contour defining the SphericalPolygon.
	//! @param contour a contour defining the polygon area.
	void setContour(const QVector<Vec3d>& contour) {octahedronPolygon = OctahedronPolygon(contour);}

	//! Return the list of closed contours defining the polygon boundaries.
	QVector<QVector<Vec3d> > getClosedOutlineContours() const {Q_ASSERT(0); return QVector<QVector<Vec3d> >();}

	//! Deserialize the region. This method must allow as fast as possible deserialization.
	static SphericalRegionP deserialize(QDataStream& in);

	//! Create a new SphericalRegionP which is the union of all the passed ones.
	static SphericalRegionP multiUnion(const QList<SphericalRegionP>& regions, bool optimizeByPreGrouping=false);
	
	//! Create a new SphericalRegionP which is the intersection of all the passed ones.
	static SphericalRegionP multiIntersection(const QList<SphericalRegionP>& regions);

private:
	OctahedronPolygon octahedronPolygon;
};


//! @class SphericalConvexPolygon
//! A special case of SphericalPolygon for which the polygon is convex.
class SphericalConvexPolygon : public SphericalRegion
{
public:
	// Avoid name hiding when overloading the virtual methods.
	using SphericalRegion::intersects;
	using SphericalRegion::contains;

	//! Default constructor.
	SphericalConvexPolygon() {;}

	//! Constructor from a list of contours.
	SphericalConvexPolygon(const QVector<QVector<Vec3d> >& contours) {Q_ASSERT(contours.size()==1); setContour(contours.at(0));}
	//! Constructor from one contour.
	SphericalConvexPolygon(const QVector<Vec3d>& contour) {setContour(contour);}
	//! Special constructor for triangle.
	SphericalConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2) {contour << e0 << e1 << e2; updateBoundingCap();}
	//! Special constructor for quads.
	SphericalConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)  {contour << e0 << e1 << e2 << e3; updateBoundingCap();}

	virtual SphericalRegionType getType() const {return SphericalRegion::ConvexPolygon;}
	virtual OctahedronPolygon getOctahedronPolygon() const {return OctahedronPolygon(contour);}
	virtual StelVertexArray getFillVertexArray() const {return StelVertexArray(contour, StelVertexArray::TriangleFan);}
	virtual StelVertexArray getOutlineVertexArray() const {return StelVertexArray(contour, StelVertexArray::LineLoop);}
	virtual double getArea() const;
	virtual bool isEmpty() const {return contour.isEmpty();}
	virtual Vec3d getPointInside() const;
	virtual SphericalCap getBoundingCap() const {return cachedBoundingCap;}
	QVector<SphericalCap> getBoundingSphericalCaps() const;
	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is
	//! @code["CONVEX_POLYGON", [[ra,dec], [ra,dec], [ra,dec], [ra,dec]]]@endcode
	//! where the coords are a closed convex contour, with each points defined by ra dec in degree in the ICRS frame.
	virtual QVariantList toQVariant() const;
	virtual void serialize(QDataStream& out) const {out << contour;}

	// Contain and intersect
	virtual bool contains(const Vec3d& p) const;
	virtual bool contains(const SphericalPolygon& r) const;
	virtual bool contains(const SphericalConvexPolygon& r) const;
	virtual bool contains(const SphericalCap& r) const;
	virtual bool contains(const SphericalPoint& r) const {return contains(r.n);}
	virtual bool contains(const AllSkySphericalRegion&) const {return false;}
	virtual bool intersects(const SphericalCap& r) const {if (!cachedBoundingCap.intersects(r)) return false; return r.intersects(*this);}
	virtual bool intersects(const SphericalPolygon& r) const;
	virtual bool intersects(const SphericalConvexPolygon& r) const;
	virtual bool intersects(const SphericalPoint& r) const {return contains(r.n);}
	virtual bool intersects(const AllSkySphericalRegion&) const {return true;}

	////////////////////////// TODO
//	virtual SphericalRegionP getIntersection(const SphericalPolygon& r) const;
//	virtual SphericalRegionP getIntersection(const SphericalConvexPolygon& r) const;
//	virtual SphericalRegionP getIntersection(const SphericalCap& r) const;
//	virtual SphericalRegionP getIntersection(const SphericalPoint& r) const;
//	virtual SphericalRegionP getIntersection(const AllSkySphericalRegion& r) const;
//	virtual SphericalRegionP getUnion(const SphericalPolygon& r) const;
//	virtual SphericalRegionP getUnion(const SphericalConvexPolygon& r) const;
//	virtual SphericalRegionP getUnion(const SphericalCap& r) const;
//	virtual SphericalRegionP getUnion(const SphericalPoint& r) const;
//	virtual SphericalRegionP getUnion(const EmptySphericalRegion& r) const;
//	virtual SphericalRegionP getSubtraction(const SphericalPolygon& r) const;
//	virtual SphericalRegionP getSubtraction(const SphericalConvexPolygon& r) const;
//	virtual SphericalRegionP getSubtraction(const SphericalCap& r) const;
//	virtual SphericalRegionP getSubtraction(const SphericalPoint& r) const;
//	virtual SphericalRegionP getSubtraction(const EmptySphericalRegion& r) const;

	////////////////////////////////////////////////////////////////////
	// Methods specific to SphericalConvexPolygon
	////////////////////////////////////////////////////////////////////
	//! Set a single contour defining the SphericalPolygon.
	//! @param acontour a contour defining the polygon area.
	void setContour(const QVector<Vec3d>& acontour) {contour=acontour; updateBoundingCap();}

	//! Get the single contour defining the SphericalConvexPolygon.
	const QVector<Vec3d>& getConvexContour() const {return contour;}

	//! Check if the polygon is valid, i.e. it has no side >180.
	bool checkValid() const;

	//! Check if the passed contour is convex and valid, i.e. it has no side >180.
	static bool checkValidContour(const QVector<Vec3d>& contour);

	//! Deserialize the region. This method must allow as fast as possible deserialization.
	static SphericalRegionP deserialize(QDataStream& in);

protected:
	//! A list of vertices of the convex contour.
	QVector<Vec3d> contour;

	//! Cache the bounding cap.
	SphericalCap cachedBoundingCap;

	//! Update the bounding cap from the vertex list.
	void updateBoundingCap();

	//! Computes whether the passed points are all outside of at least one SphericalCap defining the polygon boundary.
	//! @param thisContour the vertices defining the contour.
	//! @param nbThisContour nb of vertice of the contour.
	//! @param points the points to test.
	//! @param nbPoints the number of points to test.
	static bool areAllPointsOutsideOneSide(const Vec3d* thisContour, int nbThisContour, const Vec3d* points, int nbPoints);

	//! Computes whether the passed points are all outside of at least one SphericalCap defining the polygon boundary.
	bool areAllPointsOutsideOneSide(const QVector<Vec3d>& points) const
	{
		return areAllPointsOutsideOneSide(contour.constData(), contour.size(), points.constData(), points.size());
	}

	bool containsConvexContour(const Vec3d* vertice, int nbVertex) const;
};


// ! @class SphericalConvexPolygonSet
// ! A special case of SphericalPolygon for which the polygon is composed of disjoint convex polygons.
//class SphericalConvexPolygonSet : public SphericalRegion
//{
//public:
//	// Avoid name hiding when overloading the virtual methods.
//	using SphericalRegion::intersects;
//	using SphericalRegion::contains;
//
//	//! Default constructor.
//	SphericalConvexPolygonSet() {;}
//
//	//! Constructor from a list of contours.
//	SphericalConvexPolygonSet(const QVector<QVector<Vec3d> >& contours);
//
//	virtual SphericalRegionType getType() const {return SphericalRegion::ConvexPolygonSet;}
//	virtual OctahedronPolygon getOctahedronPolygon() const;
//	virtual StelVertexArray getFillVertexArray() const;
//	virtual StelVertexArray getOutlineVertexArray() const;
//	virtual double getArea() const;
//	virtual bool isEmpty() const;
//	virtual Vec3d getPointInside() const;
//	virtual SphericalCap getBoundingCap() const {return cachedBoundingCap;}
//	QVector<SphericalCap> getBoundingSphericalCaps() const;
//	//! Serialize the region into a QVariant map matching the JSON format.
//	//! The format is
//	//! @code["CONVEX_POLYGON_SET", [[ra,dec], [ra,dec], [ra,dec], [ra,dec]], [[ra,dec], [ra,dec], [ra,dec], [ra,dec]]]@endcode
//	//! where the coords from a list of closed convex contour, with each points defined by ra dec in degree in the ICRS frame.
//	virtual QVariantList toQVariant() const;
//	virtual void serialize(QDataStream& out) const;
//
//	// Contain and intersect
//	virtual bool contains(const Vec3d& p) const;
//	virtual bool contains(const SphericalPolygon& r) const;
//	virtual bool contains(const SphericalConvexPolygon& r) const;
//	virtual bool contains(const SphericalCap& r) const;
//	virtual bool contains(const SphericalPoint& r) const {return contains(r.n);}
//	virtual bool contains(const AllSkySphericalRegion&) const {return false;}
//	virtual bool intersects(const SphericalCap& r) const {if (!cachedBoundingCap.intersects(r)) return false; return r.intersects(*this);}
//	virtual bool intersects(const SphericalPolygon& r) const;
//	virtual bool intersects(const SphericalConvexPolygon& r) const;
//	virtual bool intersects(const SphericalPoint& r) const {return contains(r.n);}
//	virtual bool intersects(const AllSkySphericalRegion&) const {return true;}
//
//	////////////////////////////////////////////////////////////////////
//	// Methods specific to SphericalConvexPolygonSet
//	////////////////////////////////////////////////////////////////////
//	//! Deserialize the region. This method must allow as fast as possible deserialization.
//	static SphericalRegionP deserialize(QDataStream& in);
//
//protected:
//	QVector<SphericalConvexPolygon> contours;
//
//	//! Cache the bounding cap.
//	SphericalCap cachedBoundingCap;
//
//	//! Update the bounding cap from the vertex list.
//	void updateBoundingCap();
//};

//! @class SphericalTexturedPolygon
//! An extension of SphericalPolygon with addition of texture coordinates.
class SphericalTexturedPolygon : public SphericalPolygon
{
public:
	//! @struct TextureVertex
	//! A container for 3D vertex + associated texture coordinates
	struct TextureVertex
	{
		Vec3d vertex;
		Vec2f texCoord;
	};

	SphericalTexturedPolygon() {;}
	//! Constructor from a list of contours.
	SphericalTexturedPolygon(const QVector<QVector<TextureVertex> >& contours) {Q_UNUSED(contours); Q_ASSERT(0);}
	//! Constructor from one contour.
	SphericalTexturedPolygon(const QVector<TextureVertex>& contour) {Q_UNUSED(contour); Q_ASSERT(0);}

	//! Return an openGL compatible array of texture coords to be used using vertex arrays.
	virtual StelVertexArray getFillVertexArray() const {Q_ASSERT(0); return StelVertexArray();}
	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is:
	//! @code["TEXTURED_POLYGON", [[[ra,dec], [ra,dec], [ra,dec], [ra,dec]], [[ra,dec], [ra,dec], [ra,dec]],[...]],
	//! [[[u,v],[u,v],[u,v],[u,v]], [[u,v],[u,v],[u,v]], [...]]]@endcode
	//! where the two lists are a list of closed contours, with each points defined by ra dec in degree in the ICRS frame 
	//! followed by a list of texture coordinates in the u,v texture space (between 0 and 1).
	//! There must be one texture coordinate for each vertex.
	virtual QVariantList toQVariant() const;
	virtual void serialize(QDataStream& out) const {Q_UNUSED(out); Q_ASSERT(0);}

	////////////////////////////////////////////////////////////////////
	// Methods specific to SphericalTexturedPolygon
	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area using the WindingPositive winding rule.
	void setContours(const QVector<QVector<TextureVertex> >& contours) {Q_UNUSED(contours); Q_ASSERT(0);}

	//! Set a single contour defining the SphericalPolygon.
	//! @param contour a contour defining the polygon area.
	void setContour(const QVector<TextureVertex>& contour) {Q_UNUSED(contour); Q_ASSERT(0);}

private:
	//! A list of uv textures coordinates corresponding to the triangle vertices.
	//! There should be 1 uv position per vertex.
	QVector<Vec2f> textureCoords;
};


Q_DECLARE_TYPEINFO(SphericalTexturedPolygon::TextureVertex, Q_PRIMITIVE_TYPE);

//! @class SphericalTexturedConvexPolygon
//! Extension of SphericalConvexPolygon for textured polygon.
class SphericalTexturedConvexPolygon : public SphericalConvexPolygon
{
public:
	//! Default constructor.
	SphericalTexturedConvexPolygon() {;}

	//! Constructor from one contour.
	SphericalTexturedConvexPolygon(const QVector<Vec3d>& contour, const QVector<Vec2f>& texCoord) {setContour(contour, texCoord);}

	//! Special constructor for quads.
	//! Use the 4 textures corners for the 4 vertices.
	SphericalTexturedConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3) : SphericalConvexPolygon(e0,e1,e2,e3)
	{
		textureCoords << Vec2f(0.f, 0.f) << Vec2f(1.f, 0.f) << Vec2f(1.f, 1.f) << Vec2f(0.f, 1.f);
	}

	//! Return an openGL compatible array to be displayed using vertex arrays.
	//! This method is not optimized for SphericalConvexPolygon instances.
	virtual StelVertexArray getFillVertexArray() const {return StelVertexArray(contour, StelVertexArray::TriangleFan, textureCoords);}

	//! Set a single contour defining the SphericalPolygon.
	//! @param acontour a contour defining the polygon area.
	//! @param texCoord a list of texture coordinates matching the vertices of the contour.
	virtual void setContour(const QVector<Vec3d>& acontour, const QVector<Vec2f>& texCoord) {SphericalConvexPolygon::setContour(acontour); textureCoords=texCoord;}

	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is:
	//! @code["TEXTURED_CONVEX_POLYGON", [[ra,dec], [ra,dec], [ra,dec], [ra,dec]], [[u,v],[u,v],[u,v],[u,v]]]@endcode
	//! where the two lists are a closed convex contours, with each points defined by ra dec in degree in the ICRS frame 
	//! followed by a list of texture coordinates in the u,v texture space (between 0 and 1).
	//! There must be one texture coordinate for each vertex.
	virtual QVariantList toQVariant() const;

	virtual void serialize(QDataStream& out) const {out << contour << textureCoords;}

protected:
	//! A list of uv textures coordinates corresponding to the triangle vertices.
	//! There should be 1 uv position per vertex.
	QVector<Vec2f> textureCoords;
};


//! Compute the intersection of 2 great circles segments.
//! @param ok is set to false if no intersection was found.
//! @return the intersection point on the sphere (normalized) if ok is true, or undefined of ok is false.
Vec3d greatCircleIntersection(const Vec3d& p1, const Vec3d& p2, const Vec3d& p3, const Vec3d& p4, bool& ok);

//! Compute the intersection of a great circles segment with another great circle.
//! @param ok is set to false if no intersection was found.
//! @return the intersection point on the sphere (normalized) if ok is true, or undefined of ok is false.
Vec3d greatCircleIntersection(const Vec3d& p1, const Vec3d& p2, const Vec3d& nHalfSpace, bool& ok);

#endif // STELSPHEREGEOMETRY_HPP

