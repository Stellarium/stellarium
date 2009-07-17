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

#ifndef _STELSPHEREGEOMETRY_HPP_
#define _STELSPHEREGEOMETRY_HPP_

#include <QVector>
#include <QVariant>
#include <QDebug>
#include <QSharedPointer>
#include "VecMath.hpp"

class SphericalPolygon;
class SphericalConvexPolygon;
class SphericalPolygonBase;
class SphericalRegion;
class SphericalCap;
class AllSkySphericalRegion;

//! @file StelSphereGeometry.hpp
//! Define all SphericalGeometry primitives as well as the SphericalRegionP type.

//! @typedef SphericalRegionP
//! Use shared pointer to simplify memory managment.
typedef QSharedPointer<SphericalRegion> SphericalRegionP;

//! @class SphericalRegion
//! Abstract class defining a region of the sphere.
class SphericalRegion
{
public:
	virtual ~SphericalRegion() {;}

	//! Return the area of the region in steradians.
	virtual double getArea() const = 0;

	//! Return true if the region is empty.
	virtual bool isEmpty() const = 0;

	//! Return a point located inside the region.
	virtual Vec3d getPointInside() const = 0;

	//! Returns whether a point is contained into the region.
	virtual bool contains(const Vec3d& p) const = 0;

	//! Equivalent to contains() for a point.
	bool intersects(const Vec3d& p) const {return contains(p);}
	
	//! Returns whether a SphericalRegion is contained into this region.
	bool contains(const SphericalRegionP& region) const;
	
	//! Returns whether a SphericalRegion intersects with this region.
	bool intersects(const SphericalRegionP& region) const;
	
	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const = 0;

	//! Returns whether a SphericalPolygon intersects with the region.
	virtual bool intersects(const SphericalPolygonBase& poly) const = 0;
	
	//! Returns whether a SphericalCap is contained into the region.
	virtual bool contains(const SphericalCap& c) const = 0;

	//! Returns whether a SphericalCap intersects with the region.
	virtual bool intersects(const SphericalCap& c) const = 0;

	//! Returns whether a AllSkySphericalRegion is contained into the region.
	virtual bool contains(const AllSkySphericalRegion& a) const = 0;

	//! Returns whether a AllSkySphericalRegion intersects with the region.
	virtual bool intersects(const AllSkySphericalRegion& a) const = 0;
	
	//! Return the list of SphericalCap bounding the ConvexPolygon.
	virtual QVector<SphericalCap> getBoundingSphericalCaps() const = 0;

	//! Return a bounding SphericalCap. This method is heavily used and therefore needs to be very fast.
	//! The returned SphericalCap doesn't have to be the smallest one, but smaller is better.
	virtual SphericalCap getBoundingCap() const = 0;
	
	//! Return an enlarged version of this SphericalRegion so that any point distant of more 
	//! than the given margin now lays within the region.
	//! The returned region can be larger than the smallest enlarging region, therefore returning
	//! false positive on subsequent intersection tests.
	//! The default implementation always return an enlarged bounding SphericalCap.
	//! @param margin the minimum enlargement margin in radian.
	virtual SphericalRegionP getEnlarged(double margin) const;
	
	//! Create a SphericalRegion from the given input JSON stream.
	//! The type of the region is automatically recognized from the input format.
	//! The currently recognized format are:
	//! <ul>
	//! <li>Polygon vertex list:
	//! @code{
	//!     "worldCoords": [[[ra,dec], [ra,dec], [ra,dec], [ra,dec]], [[ra,dec], [ra,dec], [ra,dec]],[...]]
	//! }@endcode
	//! The format consist of a list of contours, each contours is defined by a list of ra,dec coordinates
	//! composing connected great circles segments with the last point connected to the first one.
	//! ra and dec are expressed in degree in the ICRS reference frame.</li>
	//! <li>Polygon vertex list with associated texture coordinates:
	//! @code{
	//!     "worldCoords": [[[ra,dec],[ra,dec],[ra,dec],[ra,dec]], [[ra,dec],[ra,dec],[ra,dec]],[...]],
	//!     "textureCoords": [[[u,v],[u,v],[u,v],[u,v]], [[u,v],[u,v],[u,v]], [...]]
	//! }@endcode
	//! The format is similar to the one described above, with the addition of a list of texture coordinates
	//! in the u,v texture space (between 0 and 1). There must be one texture coordinate for each vertex.</li>
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
};

//! @class SphericalCap
//! A SphericalCap is defined by a direction and an aperture.
//! It forms a cone from the center of the Coordinate frame with a radius d.
//! It is a disc on the sphere, a region above a circle on the unit sphere.
struct SphericalCap : public SphericalRegion
{
	//! Construct a SphericalCap with a 90 deg aperture and an undefined direction.
	SphericalCap() : d(0) {;}

	//! Construct a SphericalCap from its direction and assumes a 90 deg aperture.
	SphericalCap(double x, double y, double z) : n(x,y,z), d(0) {;}
	
	//! Construct a SphericalCap from its direction and aperture.
	//! @param an a unit vector indicating the direction.
	//! @param ar cosinus of the aperture.
	SphericalCap(const Vec3d& an, double ar) : n(an), d(ar) {Q_ASSERT(d==0 || std::fabs(n.lengthSquared()-1.)<0.0000001);}

	//! Copy constructor.
	SphericalCap(const SphericalCap& other) : SphericalRegion(), n(other.n), d(other.d) {;}

	//! Get the area of the intersection of the halfspace on the sphere in steradian.
	virtual double getArea() const {return 2.*M_PI*(1.-d);}

	//! Return true if the region is empty.
	virtual bool isEmpty() const {return d>=1.;}

	//! Return a point located inside the SphericalCap.
	virtual Vec3d getPointInside() const {return n;}

	//! Find if a point is contained into the SphericalCap.
	//! @param v a unit vector.
	virtual bool contains(const Vec3d &v) const {Q_ASSERT(d==0 || std::fabs(v.lengthSquared()-1.)<0.0000001);return (v*n>=d);}

	//! Returns whether a SphericalPolygon intersects with the region.
	virtual bool intersects(const SphericalPolygonBase& mpoly) const;

	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const;

	//! Returns whether a SphericalCap is contained into the region.
	virtual bool contains(const SphericalCap& h) const
	{
		const double a = n*h.n-d*h.d;
		return d<=h.d && ( a>=1. || (a>=0. && a*a >= (1.-d*d)*(1.-h.d*h.d)));
	}

	//! Returns whether a SphericalCap intersects with this one.
	//! I managed to make it without sqrt or acos, so it is very fast!
	//! @see http://f4bien.blogspot.com/2009/05/spherical-geometry-optimisations.html for detailed explanations.
	virtual bool intersects(const SphericalCap& h) const
	{
		const double a = d*h.d - n*h.n;
		return d+h.d<=0. || a<=0. || (a<=1. && a*a <= (1.-d*d)*(1.-h.d*h.d));
	}

	//! Returns whether a HalfSpace (like a SphericalCap with d=0) intersects with this SphericalCap.
	//! @param hn0 the x direction of the halfspace.
	//! @param hn1 the y direction of the halfspace.
	//! @param hn2 the z direction of the halfspace.
	inline bool intersectsHalfSpace(double hn0, double hn1, double hn2) const
	{
		const double a = n[0]*hn0+n[1]*hn1+n[2]*hn2;
		return d<=0. || a<=0. || (a<=1. && a*a <= (1.-d*d));
	}
	
	//! Returns whether a AllSkySphericalRegion is contained into the region.
	virtual bool contains(const AllSkySphericalRegion& poly) const {return d<=-1;}

	//! Returns whether a AllSkySphericalRegion intersects with the region.
	virtual bool intersects(const AllSkySphericalRegion& poly) const {return d<=1.;}
	
	//! Return the list of SphericalCap bounding the region.
	virtual QVector<SphericalCap> getBoundingSphericalCaps() const {QVector<SphericalCap> res; res << *this; return res;}

	//! Return itself.
	virtual SphericalCap getBoundingCap() const {return *this;}
	
	//! Convert the cap into a SphericalRegionBase instance.
	virtual SphericalConvexPolygon toSphericalConvexPolygon() const;

	//! Comparison operator.
	bool operator==(const SphericalCap& other) const {return (n==other.n && d==other.d);}
	
	//! The direction unit vector. Only if d==0, this vector doesn't need to be unit.
	Vec3d n;
	//! The cos of cone radius
	double d;
	
private:
	bool intersectsConvexContour(const Vec3d* vertice, int nbVertice) const;
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
	
	virtual ~SphericalPoint() {;}

	//! Return the area of the region in steradians.
	virtual double getArea() const {return 0.;}

	//! Return true if the region is empty.
	virtual bool isEmpty() const {return false;}

	//! Return a point located inside the region.
	virtual Vec3d getPointInside() const {return n;}

	//! Returns whether a point is contained into the region.
	virtual bool contains(const Vec3d& p) const {return n==p;}

	//! Returns whether a SphericalPolygon intersects with the region.
	virtual bool intersects(const SphericalPolygonBase& mpoly) const;

	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const {return false;}

	//! Returns whether a SphericalCap is contained into the region.
	virtual bool contains(const SphericalCap& c) const {return false;}

	//! Returns whether a SphericalCap intersects with the region.
	virtual bool intersects(const SphericalCap& c) const {return c.contains(n);}
	
	//! Returns whether a AllSkySphericalRegion is contained into the region.
	virtual bool contains(const AllSkySphericalRegion& poly) const {return false;}

	//! Returns whether a AllSkySphericalRegion intersects with the region.
	virtual bool intersects(const AllSkySphericalRegion& poly) const {return true;}
	
	//! Return the list of SphericalCap bounding the region.
	virtual QVector<SphericalCap> getBoundingSphericalCaps() const {QVector<SphericalCap> res; res << SphericalCap(n, 1); return res;}
	
	//! Return a full sky SphericalCap
	virtual SphericalCap getBoundingCap() const {return SphericalCap(n, 1);}
	
	//! The unit vector of the point direction.
	Vec3d n;
};

//! @class AllSkySphericalRegion
//! Special SphericalRegion for the whole sphere.
class AllSkySphericalRegion : public SphericalRegion
{
public:
	virtual ~AllSkySphericalRegion() {;}

	//! Return the area of the region in steradians.
	virtual double getArea() const {return 4.*M_PI;}

	//! Return true if the region is empty.
	virtual bool isEmpty() const {return false;}

	//! Return a point located inside the region.
	virtual Vec3d getPointInside() const {return Vec3d(1,0,0);}

	//! Returns whether a point is contained into the region.
	virtual bool contains(const Vec3d& p) const {return true;}

	//! Returns whether a SphericalPolygon intersects with the region.
	virtual bool intersects(const SphericalPolygonBase& mpoly) const {return true;}

	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const {return true;}

	//! Returns whether a SphericalCap is contained into the region.
	virtual bool contains(const SphericalCap& c) const {return true;}

	//! Returns whether a SphericalCap intersects with the region.
	virtual bool intersects(const SphericalCap& c) const {return true;}
	
	//! Returns whether a AllSkySphericalRegion is contained into the region.
	virtual bool contains(const AllSkySphericalRegion& poly) const {return true;}

	//! Returns whether a AllSkySphericalRegion intersects with the region.
	virtual bool intersects(const AllSkySphericalRegion& poly) const {return true;}
	
	//! Return the list of SphericalCap bounding the region.
	virtual QVector<SphericalCap> getBoundingSphericalCaps() const {QVector<SphericalCap> res; res << SphericalCap(Vec3d(1,0,0), -2); return res;}
	
	//! Return a full sky SphericalCap
	virtual SphericalCap getBoundingCap() const {return SphericalCap(Vec3d(1,0,0), -2);}
};


//! @class SphericalPolygonBase
//! Abstract class defining default implementations for some spherical geometry methods.
//! All methods are reentrant.
class SphericalPolygonBase : public SphericalRegion
{
public:
	//! @enum PolyWindingRule
	//! Define the possible winding rules to use when setting the contours for a polygon.
	enum PolyWindingRule
	{
		WindingPositive,	//!< Positive winding rule (used for union)
		WindingAbsGeqTwo	//!< Abs greater or equal 2 winding rule (used for intersection)
	};

	//! Destructor
	virtual ~SphericalPolygonBase() {;}

	//! Return an openGL compatible array to be displayed using vertex arrays.
	virtual QVector<Vec3d> getVertexArray() const = 0;

	//! Return an openGL compatible array of edge flags to be displayed using vertex arrays.
	virtual QVector<bool> getEdgeFlagArray() const = 0;

	//! Return an openGL compatible array of texture coords to be used using vertex arrays.
	//! @return the array or an empty array if the polygon has no texture.
	virtual QVector<Vec2f> getTextureCoordArray() const = 0;

	//! Get the contours defining the SphericalPolygon.
	virtual QVector<QVector<Vec3d> > getContours() const;

	//! Return the area in steradians.
	virtual double getArea() const;

	//! Return a point located inside the polygon.
	virtual Vec3d getPointInside() const;

	//! Return true if the polygon is an empty polygon.
	virtual bool isEmpty() const {return getVertexArray().isEmpty();}

	//! Returns whether another SphericalPolygon intersects with the SphericalPolygon.
	virtual bool intersects(const SphericalPolygonBase& poly) const;
	
	//! Returns whether a SphericalCap intersects with the region.
	virtual bool intersects(const SphericalCap& c) const {return c.intersects(*this);}
	
	//! Returns whether a AllSkySphericalRegion is contained into the region.
	virtual bool contains(const AllSkySphericalRegion& poly) const {return false;}

	//! Returns whether a AllSkySphericalRegion intersects with the region.
	virtual bool intersects(const AllSkySphericalRegion& poly) const {return true;}
	
	//! Return the list of SphericalCap bounding the polygon.
	virtual QVector<SphericalCap> getBoundingSphericalCaps() const {Q_ASSERT(0); return QVector<SphericalCap>();}

	//! Default slow implementation o(n^2).
	virtual SphericalCap getBoundingCap() const;
	
	//! Output the SphericalPolygon information in the form of a QVariant.
	//! The QVariant will contain a list of contours, each contours being a list of ra,dec points
	//! with ra,dec expressed in degree in the ICRS reference frame.
	//! Use QJSONParser to transform a QVariant into its JSON representation.
	virtual QVariantMap toQVariant() const;

	//! Return a new SphericalPolygon consisting of the intersection of this and the given SphericalPolygon.
	SphericalPolygon getIntersection(const SphericalPolygonBase& mpoly) const;
	//! Return a new SphericalPolygon consisting of the union of this and the given SphericalPolygon.
	SphericalPolygon getUnion(const SphericalPolygonBase& mpoly) const;
	//! Return a new SphericalPolygon consisting of the subtraction of the given SphericalPolygon from this.
	SphericalPolygon getSubtraction(const SphericalPolygonBase& mpoly) const;
};

//! @class SphericalPolygon
//! A SphericalPolygon is a complex shape defined by the union of contours.
//! Each contour is composed of connected great circles segments with the last point connected to the first one.
//! Contours don't need to be convex (they are internally tesselated into triangles).
class SphericalPolygon : public SphericalPolygonBase
{
public:
	// Avoid name hiding when overloading the virtual methods.
	using SphericalPolygonBase::intersects;
	using SphericalPolygonBase::contains;
	
	//! Default constructor.
	SphericalPolygon() {;}

	//! Constructor from a list of contours.
	SphericalPolygon(const QVector<QVector<Vec3d> >& contours) {setContours(contours);}

	//! Constructor from one contour.
	SphericalPolygon(const QVector<Vec3d>& contour) {setContour(contour);}

	//! Return an openGL compatible array to be displayed using vertex arrays.
	//! The array was precomputed therefore the method is very fast.
	virtual QVector<Vec3d> getVertexArray() const {return triangleVertices;}

	//! Return an openGL compatible array of edge flags to be displayed using vertex arrays.
	//! The array was precomputed therefore the method is very fast.
	virtual QVector<bool> getEdgeFlagArray() const {return edgeFlags;}

	//! Return an openGL compatible array of texture coords to be used using vertex arrays.
	virtual QVector<Vec2f> getTextureCoordArray() const {return QVector<Vec2f>();}

	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule the winding rule to use. Default value is WindingPositive, meaning that the
	//! polygon is the union of the positive contours minus the negative ones.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, SphericalPolygonBase::PolyWindingRule windingRule=SphericalPolygonBase::WindingPositive);

	//! Set a single contour defining the SphericalPolygon.
	//! @param contours a contour defining the polygon area.
	virtual void setContour(const QVector<Vec3d>& contour);
	
	//! Returns whether a point is contained into the SphericalPolygon.
	virtual bool contains(const Vec3d& p) const;
	
	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const {Q_ASSERT(0); return false;}

	//! Returns whether a SphericalCap is contained into the region.
	virtual bool contains(const SphericalCap& c) const {Q_ASSERT(0); return false;}
	
protected:
	friend void vertexCallback(void* vertexData, void* userData);
	friend void vertexTextureCallback(void* vertexData, void* userData);

	//! A list of vertices describing the tesselated polygon. The vertices have to be
	//! used 3 by 3, forming triangles.
	QVector<Vec3d> triangleVertices;

	//! A list of booleans: one per vertex of the triangleVertices array.
	//! Value is true if the vertex belongs to an edge, false otherwise.
	QVector<bool> edgeFlags;
};

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

	//! Default constructor.
	SphericalTexturedPolygon() {;}

	//! Constructor from a list of contours.
	SphericalTexturedPolygon(const QVector<QVector<TextureVertex> >& contours) {setContours(contours, SphericalPolygonBase::WindingPositive);}

	//! Constructor from one contour.
	SphericalTexturedPolygon(const QVector<TextureVertex>& contour) {setContour(contour);}

	//! Return an openGL compatible array of texture coords to be used using vertex arrays.
	virtual QVector<Vec2f> getTextureCoordArray() const {return textureCoords;}

	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule the winding rule to use. Default value is WindingPositive, meaning that the
	//! polygon is the union of the positive contours minus the negative ones.
	virtual void setContours(const QVector<QVector<TextureVertex> >& contours, SphericalPolygonBase::PolyWindingRule windingRule=SphericalPolygonBase::WindingPositive);

	//! Set a single contour defining the SphericalPolygon.
	//! @param contour a contour defining the polygon area.
	virtual void setContour(const QVector<TextureVertex>& contour);

private:
	friend void vertexTextureCallback(void* vertexData, void* userData);

	//! A list of uv textures coordinates corresponding to the triangle vertices.
	//! There should be 1 uv position per vertex.
	QVector<Vec2f> textureCoords;
};

//! @class SphericalConvexPolygon
//! A special case of SphericalPolygon for which the polygon is convex.
class SphericalConvexPolygon : public SphericalPolygonBase
{
public:
	
	// Avoid name hiding when overloading the virtual methods.
	using SphericalPolygonBase::intersects;
	using SphericalPolygonBase::contains;
	
	//! Default constructor.
	SphericalConvexPolygon() {;}

	//! Constructor from a list of contours.
	SphericalConvexPolygon(const QVector<QVector<Vec3d> >& contours) {setContours(contours);}

	//! Constructor from one contour.
	SphericalConvexPolygon(const QVector<Vec3d>& contour) {setContour(contour);}

	//! Special constructor for triangle.
	SphericalConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2) {contour << e0 << e1 << e2;}

	//! Special constructor for quads.
	SphericalConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)  {contour << e0 << e1 << e2 << e3;}

	//! Return an openGL compatible array to be displayed using vertex arrays.
	//! This method is not optimized for SphericalConvexPolygon instances.
	virtual QVector<Vec3d> getVertexArray() const;

	//! Return an openGL compatible array of edge flags to be displayed using vertex arrays.
	//! This method is not optimized for SphericalConvexPolygon instances.
	virtual QVector<bool> getEdgeFlagArray() const;

	//! Return an openGL compatible array of texture coords to be used using vertex arrays.
	virtual QVector<Vec2f> getTextureCoordArray() const {return QVector<Vec2f>();}

	//! Return true if the polygon is an empty polygon.
	virtual bool isEmpty() const {return contour.isEmpty();}

	//! Set the contours defining the SphericalConvexPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule unused for SphericalConvexPolygon.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, PolyWindingRule windingRule=WindingPositive)
	{
		Q_ASSERT(contours.size()==1);
		contour=contours.at(0);
	}

	//! Set a single contour defining the SphericalPolygon.
	//! @param acontour a contour defining the polygon area.
	virtual void setContour(const QVector<Vec3d>& acontour) {contour=acontour;}

	//! Get the contours defining the SphericalConvexPolygon.
	virtual QVector<QVector<Vec3d> > getContours() const {QVector<QVector<Vec3d> > contours; contours.append(contour); return contours;}

	//! Returns whether a point is contained into the region.
	virtual bool contains(const Vec3d& p) const;

	//! Returns whether a SphericalCap is contained into the region.
	virtual bool contains(const SphericalCap& c) const;
	
	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const;

	//! Returns whether another SphericalPolygon intersects with the SphericalPolygon.
	virtual bool intersects(const SphericalPolygonBase& polyBase) const;
	
	///////////////////////////////////////
	// Methods specific to convex polygons
	//! Get the single contour defining the SphericalConvexPolygon.
	const QVector<Vec3d>& getConvexContour() const {return contour;}

	//! Check if the polygon is valid, i.e. it has no side >180.
	bool checkValid() const;

	//! Check if the passed contour is convex and valid, i.e. it has no side >180.
	static bool checkValidContour(const QVector<Vec3d>& contour);
	
	//! Return the list of halfspace bounding the ConvexPolygon.
	QVector<SphericalCap> getBoundingSphericalCaps() const;

protected:
	//! A list of vertices of the convex contour.
	QVector<Vec3d> contour;

	//! Computes whether the passed points are all outside of at least one SphericalCap defining the polygon boundary.
	//! @param thisContour the vertices defining the contour.
	//! @param nbThisContour nb of vertice of the contour.
	//! @param points the points to test.
	//! @param points the number of points to test.
	static bool areAllPointsOutsideOneSide(const Vec3d* thisContour, int nbThisContour, const Vec3d* points, int nbPoints);
	
	//! Computes whether the passed points are all outside of at least one SphericalCap defining the polygon boundary.
	bool areAllPointsOutsideOneSide(const QVector<Vec3d>& points) const
	{
		return areAllPointsOutsideOneSide(contour.constData(), contour.size(), points.constData(), points.size());
	}
	
	bool containsConvexContour(const Vec3d* vertice, int nbVertex) const;
};

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
	SphericalTexturedConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)
	{
		contour << e0 << e1 << e2 << e3;
		textureCoords << Vec2f(0.f, 0.f) << Vec2f(1.f, 0.f) << Vec2f(1.f, 1.f) << Vec2f(0.f, 1.f);
	}

	//! Return an openGL compatible array of texture coords to be used using vertex arrays.
	virtual QVector<Vec2f> getTextureCoordArray() const {return textureCoords;}

	//! Set a single contour defining the SphericalPolygon.
	//! @param acontour a contour defining the polygon area.
	//! @param texCoord a list of texture coordinates matching the vertices of the contour.
	virtual void setContour(const QVector<Vec3d>& acontour, const QVector<Vec2f>& texCoord) {contour=acontour; textureCoords=texCoord;}

protected:
	//! A list of uv textures coordinates corresponding to the triangle vertices.
	//! There should be 1 uv position per vertex.
	QVector<Vec2f> textureCoords;
};

//! Compute the intersection of 2 halfspaces on the sphere (usually on 2 points) and return it in p1 and p2.
//! If the 2 SphericalCap don't interesect or intersect only at 1 point, false is returned and p1 and p2 are undefined
bool planeIntersect2(const SphericalCap& h1, const SphericalCap& h2, Vec3d& p1, Vec3d& p2);

#endif // _STELSPHEREGEOMETRY_HPP_

