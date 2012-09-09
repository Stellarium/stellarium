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

#ifndef _STELSPHEREGEOMETRY_HPP_
#define _STELSPHEREGEOMETRY_HPP_

#include <QDebug>
#include <QSharedPointer>
#include <QVariant>
#include <QVarLengthArray>
#include <QVector>

#include "OctahedronPolygon.hpp"
#include "renderer/StelVertexBuffer.hpp"
#include "Triplet.hpp"
#include "VecMath.hpp"

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
Q_DECLARE_METATYPE(SphericalRegionP);

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

	//! Parameters specifying how to draw a SphericalRegion.
	//!
	//! Passed to e.g. drawFill().
	//!
	//! Also used to remember previously used draw parameters, which enables 
	//! vertex buffer caching.
	//!
	//! This is a builder-style struct. Parameters can be specified like this:
	//!
	//! @code
	//! // Default parameters. Projector is mandatory, so it is specified in the constructor.
	//! SphericalRegion::DrawParams a(someProjectorPointer);
	//! // Do not subdivide the region into smaller triangles.
	//! SphericalRegion::DrawParams b = SphericalRegion::DrawParams(someProjectorPointer)
	//!                                                 .doNotSubdivide();
	//! // Do not subdivide and clip the part of the region outside of specified cap.
	//! SphericalRegion::DrawParams c = SphericalRegion::DrawParams(someProjectorPointer)
	//!                                                 .doNotSubdivide()
	//!                                                 .clippingCap(someSphericalCapPointer);
	//! @endcode
	//!
	//!
	struct DrawParams
	{
		//! Construct DrawParams with default parameters and specified projector.
		//!
		//! @param projector Projector to use for drawing.
		//!
		//! The default parameters are: no clipping cap, subdivide the region to
		//! follow projection distortions, max distortion 5.0.
		DrawParams(class StelProjector* projector)
			: projector_(projector)
			, clippingCap_(NULL)
			, subdivide_(true)
			, maxSqDistortion_(5.0)
		{}

		//! Only used to determine whether to update cached vertex buffers.
		//! 
		//! Compares parameters given in a previous drawXXX() 
		//! call with parameters given in the current call.
		//!
		//! Projector is ignored - outside code needs to figure out if projector 
		//! affects vertex buffer generation. Clipping cap is only assumed to be 
		//! the same if both previos and current is NULL (which is usually the case).
		bool operator != (const DrawParams& rhs) const 
		{
			// Projector is ignored, as even if the pointer points to the same object, 
			// the objects' state might have changed, making comparison useless. 
			// Instead, we only cache in cases when projector is not used used outside 
			// Renderer (i.e. not affecting vertex buffers that get cached) or NULL.

			// Clipping caps are only considered equal when NULL (which is the 
			// most common case, anyway). There is no opportunity for special handling
			// like projectors, since, when not NULL, clipping caps are always used 
			// in vertex buffer generation.

			return (clippingCap_ == NULL && rhs.clippingCap_ == NULL) ||
			       subdivide_ != rhs.subdivide_   ||
			       //Fuzzy compare might be better here
			       maxSqDistortion_ != rhs.maxSqDistortion_;
		}

		//! If not NULL, used to clip the part of the region outside the cap.
		DrawParams& clippingCap(const SphericalCap* clippingCap)
		{
			clippingCap_ = clippingCap;
			return *this;
		}

		//! Do not subdivide the region to follow projection distortions.
		//!
		//! Improves drawing speed.
		//!
		//! You can use this if you think that the region is fully contained in the viewport.
		DrawParams& doNotSubdivide()
		{
			subdivide_ = false;
			return *this;
		}

		//! Specify maximum distortion - we try to subdivide to be withil this limit.
		DrawParams& maxSqDistortion(const double maxSqDistortion)
		{
			maxSqDistortion_ = maxSqDistortion;
			return *this;
		}

		//! Projector to use when drawing.
		class StelProjector* projector_;

		//! If specified, clips the part of the region outside of cap.
		const SphericalCap* clippingCap_;

		//! Subdivide triangles?
		bool subdivide_;

		//! Maximum distortion.
		double maxSqDistortion_;
	};

	//! 3D vertex with only a position.
	struct PlainVertex
	{
		Vec3f position;
		PlainVertex(const Vec3f& position) : position(position){}
		PlainVertex(const Vec3d& pos) : position(pos[0], pos[1], pos[2]) {}
        VERTEX_ATTRIBUTES(Vec3f Position)
	};

	//! 3D vertex with position and a texture coordinate.
	struct TexturedVertex
	{
		Vec3f position;
		Vec2f texCoord;
		TexturedVertex(const Vec3f& position, const Vec2f& texCoord)
			: position(position) , texCoord(texCoord) {}
		TexturedVertex(const Vec3d& pos, const Vec2f& texCoord)
			: position(pos[0], pos[1], pos[2]) , texCoord(texCoord) {}
        VERTEX_ATTRIBUTES(Vec3f Position, Vec2f TexCoord)
	};

	//! Default constructor. Inializes with no vertex buffers.
	SphericalRegion()
		: fillPlainVertexBuffer(NULL)
		, previousFillDrawParams(NULL)
	{
		// Make sure previousFillDrawParams is invalid at start, so it gets replaced 
		// at first drawFill() call.
		previousFillDrawParams.maxSqDistortion_ = -42.0f;
	}

	//! Destructor. Cleans up vertex buffers, if any.
	virtual ~SphericalRegion() 
	{
		if(NULL != fillPlainVertexBuffer)
		{
			delete fillPlainVertexBuffer;
		}
	}

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

	//! Get a vector of vertex positions forming the region.
	virtual const QVector<Vec3d>& getFillVertexPositions() const
	{
		return getOctahedronPolygon().fillVertices();
	}

	//! Get primitive type determining how vertices in vector returned by
	//! getFillVertexPositions() form triangles.
	virtual PrimitiveType getFillPrimitiveType() const 
	{
		return PrimitiveType_Triangles;
	}

	//! Get the outline of the contours defining the SphericalPolygon.
	//! @return a list of vertices which define the contours of the polygon.
	virtual const QVector<Vec3d>& getOutlineVertexPositions() const 
	{
		// This is a workaround around a compiler bug with Clang (as of Clang 3.2).
		// Returning the reference directly results in an uninitialized
		// reference which breaks calling code.
		const QVector<Vec3d>& result(getOctahedronPolygon().outlineVertices());
		return result;
	}

	//! Get primitive type determining how vertices in vector returned by
	//! getOutlinePrimitiveType() form lines.
	virtual PrimitiveType getOutlinePrimitiveType() const
	{
		return PrimitiveType_Lines;
	}

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

	//! Draw the region as triangles (i.e. filling the region).
	//!
	//! @param renderer Renderer to use for drawing.
	//! @param params   Drawing parameters (projector, clipping cap, if any, etc.).
	//!
	//! @see DrawParams
	virtual void drawFill(class StelRenderer* renderer, const DrawParams& params);

	//! Draw the outline of the region.
	//!
	//! @param renderer Renderer to use for drawing.
	//! @param params   Drawing parameters (projector, clipping cap, if any, etc.).
	//!                 Note that maxSqDistortion has no effect here.
	//!
	//! @see DrawParams
	virtual void drawOutline(class StelRenderer* renderer, const DrawParams& params);

protected:
	//! Cached plain vertex buffer for drawing.
	StelVertexBuffer<PlainVertex>* fillPlainVertexBuffer;

	//! Should Renderer draw calls specify a projector?
	//!
	//! This is true unless we've projected the vertices ourselves (which is the
	//! case when we're subdividing the region into smaller triangles.)
	bool useProjector;

	//! Update the vertex buffer used by drawFill().
	//!
	//! Called when drawing parameters have changed, or when we cannot cache 
	//! vertices (e.g. when subdividing and this projecting outside Renderer).
	//!
	//! @param renderer            Renderer to create vertex buffer.
	//! @param params              Parameters used for drawing 
	//!                            (which also affect vertex generation).
	//! @param handleDiscontinuity Do we need to ensure that no triangles cross a 
	//!                            projection discontinuity?
	virtual void updateFillVertexBuffer(class StelRenderer* renderer, const DrawParams& params, bool handleDiscontinuity);

	//! Drawing part of drawFill() - assumes the buffer/s is/are generated.
	//!
	//! @param renderer  Renderer used for drawing.
	//! @param projector Projector to project the vertices 
	//!                  (NULL if subdivision is enabled, as in that case
	//!                  the projection is done during buffer generation)
	virtual void drawFillVertexBuffer(class StelRenderer* renderer, class StelProjector* projector);

	//! Do we need to update vertex buffer/s used by drawFill()?
	//!
	//! Might be true if e.g. the region has changed. Note that this is only
	//! one possible reason to update the buffers, drawFill() contains the full 
	//! logic to determine this (e.g. we always update when subdividing is enabled).
	virtual bool needToUpdateFillVertexBuffers() const
	{
		// Can't determine whether we can cache anything - 
		// we'd need to know if polygon returned by getOctahedronPolygon has changed,
		// but since getOctahedronPolygon returns by value, not reference,
		// it returns a new octahedronPolygon every time.
		//
		// If it was by reference, _and_ guaranteed to always point to the same polygon,
		// we might store a flag in the polygon determining if its vertex array has 
		// changed, and use that for caching.
		return true;
	}

	//! Called after updating vertex buffer/s used by drawFill().
	virtual void fillVertexBuffersUpdated() {}

private:
	//! Parameters used for the previous drawFill() call.
	DrawParams previousFillDrawParams;

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
public:
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
	bool contains(const Vec3d &v) const 
	{
		Q_ASSERT(d==0 || std::fabs(v.lengthSquared()-1.)<0.00000021);
		return (v*n>=d);
	}
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

	//! Does the cap contain the passed triangle?
	bool containsTriangle(const Triplet<Vec3d> triangle) const;

	//! Does the cap intersect with the passed triangle?
	bool intersectsTriangle(const Triplet<Vec3d>& triangle) const;

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
	virtual OctahedronPolygon getOctahedronPolygon() const 
	{
		return OctahedronPolygon::getAllSkyOctahedronPolygon();
	}
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

//! @class AllSkySphericalRegion
//! Special SphericalRegion for the whole sphere.
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
	virtual OctahedronPolygon getOctahedronPolygon() const 
	{
		return OctahedronPolygon::getEmptyOctahedronPolygon();
	}
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


//! @class SphericalPolygonBase
//! Abstract class defining default implementations for some spherical geometry methods.
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
	virtual OctahedronPolygon getOctahedronPolygon() const 
	{
		return octahedronPolygon;
	}

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
	void setContours(const QVector<QVector<Vec3d> >& contours) 
	{
		octahedronPolygon = OctahedronPolygon(contours);
	}

	//! Set a single contour defining the SphericalPolygon.
	//! @param contour a contour defining the polygon area.
	void setContour(const QVector<Vec3d>& contour) 
	{
		octahedronPolygon = OctahedronPolygon(contour);
	}

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
	SphericalConvexPolygon(const QVector<QVector<Vec3d> >& contours)
		: fillVertexBufferNeedsUpdate(true)
	{
		Q_ASSERT(contours.size()==1); 
		setContour(contours.at(0));
	}

	//! Constructor from one contour.
	SphericalConvexPolygon(const QVector<Vec3d>& contour) 
		: fillVertexBufferNeedsUpdate(true)
	{
		setContour(contour);
	}

	//! Special constructor for triangle.
	SphericalConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2) 
		: fillVertexBufferNeedsUpdate(true)
	{
		contour << e0 << e1 << e2; updateBoundingCap();
	}

	//! Special constructor for quads.
	SphericalConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3)  
		: fillVertexBufferNeedsUpdate(true)
	{
		contour << e0 << e1 << e2 << e3; updateBoundingCap();
	}

	virtual SphericalRegionType getType() const {return SphericalRegion::ConvexPolygon;}

	virtual OctahedronPolygon getOctahedronPolygon() const 
	{
		return OctahedronPolygon(contour);
	}

	virtual const QVector<Vec3d>& getFillVertexPositions() const
	{
		return contour;
	}

	virtual PrimitiveType getFillPrimitiveType() const 
	{
		return PrimitiveType_TriangleFan;
	}

	virtual const QVector<Vec3d>& getOutlineVertexPositions() const 
	{
		return contour;
	}

	virtual PrimitiveType getOutlinePrimitiveType() const
	{
		return PrimitiveType_LineLoop;
	}

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
	void setContour(const QVector<Vec3d>& acontour) 
	{
		contour = acontour;
		fillVertexBufferNeedsUpdate = true;
		updateBoundingCap();
	}

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

	//! Does the drawFill() vertex buffer need an update?
	bool fillVertexBufferNeedsUpdate;

	//! Update the bounding cap from the vertex list.
	void updateBoundingCap();

	virtual void updateFillVertexBuffer(class StelRenderer* renderer, const DrawParams& params, bool handleDiscontinuity);

	virtual void drawFillVertexBuffer(class StelRenderer* renderer, class StelProjector* projector);

	virtual bool needToUpdateFillVertexBuffers() const
	{
		return fillVertexBufferNeedsUpdate;
	}

	virtual void fillVertexBuffersUpdated()
	{
		fillVertexBufferNeedsUpdate = false;
	}

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


//! @class SphericalConvexPolygonSet
//! A special case of SphericalPolygon for which the polygon is composed of disjoint convex polygons.
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


//! @class SphericalTexturedConvexPolygon
//! Extension of SphericalConvexPolygon for textured polygon.
class SphericalTexturedConvexPolygon : public SphericalConvexPolygon
{
public:
	//! Default constructor.
	SphericalTexturedConvexPolygon() : fillTexturedVertexBuffer(NULL) 
	{
		fillVertexBufferNeedsUpdate = true;
	}

	//! Constructor from one contour.
	SphericalTexturedConvexPolygon(const QVector<Vec3d>& contour, const QVector<Vec2f>& texCoord) 
		: fillTexturedVertexBuffer(NULL)
	{
		setContour(contour, texCoord);
		fillVertexBufferNeedsUpdate = true;
	}

	//! Special constructor for quads.
	//! Use the 4 textures corners for the 4 vertices.
	SphericalTexturedConvexPolygon(const Vec3d &e0,const Vec3d &e1,const Vec3d &e2, const Vec3d &e3) 
		: SphericalConvexPolygon(e0,e1,e2,e3)
		, fillTexturedVertexBuffer(NULL)
	{
		textureCoords << Vec2f(0.f, 0.f) << Vec2f(1.f, 0.f) << Vec2f(1.f, 1.f) << Vec2f(0.f, 1.f);
		fillVertexBufferNeedsUpdate = true;
	}

	virtual ~SphericalTexturedConvexPolygon()
	{
		if(NULL != fillTexturedVertexBuffer)
		{
			delete fillTexturedVertexBuffer;
		}
	}

	//! Set a single contour defining the SphericalPolygon.
	//! @param acontour a contour defining the polygon area.
	//! @param texCoord a list of texture coordinates matching the vertices of the contour.
	virtual void setContour(const QVector<Vec3d>& acontour, const QVector<Vec2f>& texCoord) 
	{
		SphericalConvexPolygon::setContour(acontour); 
		textureCoords=texCoord;
		fillVertexBufferNeedsUpdate = true;
	}

	//! Serialize the region into a QVariant map matching the JSON format.
	//! The format is:
	//! @code["TEXTURED_CONVEX_POLYGON", [[ra,dec], [ra,dec], [ra,dec], [ra,dec]], [[u,v],[u,v],[u,v],[u,v]]]@endcode
	//! where the two lists are a closed convex contours, with each points defined by ra dec in degree in the ICRS frame 
	//! followed by a list of texture coordinates in the u,v texture space (between 0 and 1).
	//! There must be one texture coordinate for each vertex.
	virtual QVariantList toQVariant() const;

	virtual void serialize(QDataStream& out) const {out << contour << textureCoords;}

protected:
	//! A list of uv texture coordinates corresponding to the triangle vertices.
	//! There should be 1 uv position per vertex.
	QVector<Vec2f> textureCoords;

	//! Vertex buffer used in our override of drawFill() (using vertices with texcoords).
	StelVertexBuffer<TexturedVertex>* fillTexturedVertexBuffer;

	virtual void updateFillVertexBuffer(class StelRenderer* renderer, const DrawParams& params, bool handleDiscontinuity);

	virtual void drawFillVertexBuffer(class StelRenderer* renderer, class StelProjector* projector);
};


//! Compute the intersection of 2 great circles segments.
//! @param p1 Start of the first great circle segment.
//! @param p2 End of the first great circle segment.
//! @param p3 Start of the second great circle segment.
//! @param p4 End of the second great circle segment.
//! @param ok is set to false if no intersection was found.
//! @return the intersection point on the sphere (normalized) if ok is true, or undefined of ok is false.
Vec3d greatCircleIntersection(const Vec3d& p1, const Vec3d& p2, const Vec3d& p3, const Vec3d& p4, bool& ok);

//! Compute the intersection of a great circles segment with another great circle.
//! @param p1 Start of the great circle segment.
//! @param p2 End of the great circle segment.
//! @param nHalfSpace Normal of the plane separating half space to intersect with.
//! @param ok is set to false if no intersection was found.
//! @return the intersection point on the sphere (normalized) if ok is true, or undefined of ok is false.
Vec3d greatCircleIntersection(const Vec3d& p1, const Vec3d& p2, const Vec3d& nHalfSpace, bool& ok);

#endif // _STELSPHEREGEOMETRY_HPP_

