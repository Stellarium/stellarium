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
#include "VecMath.hpp"
#include <boost/shared_ptr.hpp>

class SphericalPolygon;
class SphericalPolygonBase;
class ConvexRegion;
class SphericalRegion;
class HalfSpace;

//! @file StelSphereGeometry.hpp
//! Define all SphericalGeometry primitives as well as the SphericalRegionP type.

//! @typedef SphericalRegionP
//! Use shared pointer to simplify memory managment
typedef boost::shared_ptr<SphericalRegion> SphericalRegionP;

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
	
	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const = 0;
	
	//! Returns whether a SphericalPolygon intersects with the region.
	virtual bool intersects(const SphericalPolygonBase& poly) const = 0;
	
	//! Equivalent to contains() for a point.
	bool intersects(const Vec3d& p) const {return contains(p);}
	
	//! Return the list of HalfSpace bounding the ConvexPolygon.
	virtual QVector<HalfSpace> getBoundingHalfSpaces() const = 0;
};

//! @class HalfSpace
//! A HalfSpace is defined by a direction and an aperture.
//! It forms a cone from the center of the Coordinate frame with a radius d.
struct HalfSpace : public SphericalRegion
{
	//! Construct a HalfSpace with a 90 deg aperture and an undefined direction.
	HalfSpace() : d(0) {;}
	
	//! Construct a HalfSpace from its direction and assumes a 90 deg aperture.
	//! @param an a unit vector indicating the direction.
	HalfSpace(const Vec3d& an) : n(an), d(0) {;}
	
	//! Construct a HalfSpace from its direction and aperture.
	//! @param an a unit vector indicating the direction.
	//! @param ar cosinus of the aperture.
	HalfSpace(const Vec3d& an, double ar) : n(an), d(ar) {Q_ASSERT(d==0 || std::fabs(n.lengthSquared()-1.)<0.0000001);}
	
	//! Copy constructor.
	HalfSpace(const HalfSpace& other) : SphericalRegion(), n(other.n), d(other.d) {;}
	
	//! Get the area of the intersection of the halfspace on the sphere in steradian.
	virtual double getArea() const {return 2.*M_PI*(1.-d);}
	
	//! Return true if the region is empty.
	virtual bool isEmpty() const {return d>=1.;}
	
	//! Return a point located inside the halfspace.
	virtual Vec3d getPointInside() const {return n;}
	
	//! Find if a point is contained into the halfspace.
	//! @param v a unit vector.
	virtual bool contains(const Vec3d &v) const {Q_ASSERT(d==0 || std::fabs(v.lengthSquared()-1.)<0.0000001);return (v*n>=d);}
	
	//! Comparison operator.
	bool operator==(const HalfSpace& other) const {return (n==other.n && d==other.d);}

	//! Returns whether a SphericalPolygon intersects with the region.
	virtual bool intersects(const SphericalPolygonBase& mpoly) const;
	
	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const;
	
	//! Return the list of HalfSpace bounding the region.
	virtual QVector<HalfSpace> getBoundingHalfSpaces() const {QVector<HalfSpace> res; res << *this; return res;}
	
	//! The direction unit vector. Only if d==0, this vector doesn't need to be unit.
	Vec3d n;
	//! The cos of cone radius
	double d;
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
	
	//! Return the list of HalfSpace bounding the region.
	virtual QVector<HalfSpace> getBoundingHalfSpaces() const {QVector<HalfSpace> res; res << HalfSpace(Vec3d(1,0,0), -2); return res;}
};


//! @class SphericalPolygonBase
//! Abstract class defining default implementations for some spherical geometry methods.
//! All methods are reentrant.
class SphericalPolygonBase : public SphericalRegion
{
public:
  	//! @enum WindingRule
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
	virtual QVector<Vec2d> getTextureCoordArray() const = 0;
	
	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule the winding rule to use. Default value is WindingPositive, meaning that the 
	//! polygon is the union of the positive contours minus the negative ones.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, PolyWindingRule windingRule=WindingPositive,
		const QVector<QVector<Vec2d> >& textureCoordsContours=QVector<QVector<Vec2d> >()) = 0;
	
	//! Set a single contour defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	virtual void setContour(const QVector<Vec3d>& contour) = 0;

	//! Get the contours defining the SphericalPolygon.
	virtual QVector<QVector<Vec3d> > getContours() const;
	
	//! Return the area in steradians.
	virtual double getArea() const;

	//! Return a point located inside the polygon.
	virtual Vec3d getPointInside() const;
	
	//! Return true if the polygon is an empty polygon.
	virtual bool isEmpty() const {return getVertexArray().isEmpty();}
	
	//! Returns whether a point is contained into the SphericalPolygon.
	virtual bool contains(const Vec3d& p) const;
	
	//! Returns whether another SphericalPolygon intersects with the SphericalPolygon.
	bool intersects(const Vec3d& p) const {return contains(p);}
	
	//! Returns whether another SphericalPolygon intersects with the SphericalPolygon.
	virtual bool intersects(const SphericalPolygonBase& mpoly) const;
	
	//! Returns whether a SphericalPolygon is contained into the region.
	virtual bool contains(const SphericalPolygonBase& poly) const {Q_ASSERT(0); return false;}
	
	//! Return the list of HalfSpace bounding the ConvexPolygon.
	virtual QVector<HalfSpace> getBoundingHalfSpaces() const {Q_ASSERT(0); return QVector<HalfSpace>();}
	
	//! Load the SphericalPolygon information from a QVariant.
	//! The QVariant should contain a list of contours, each contours being a list of ra,dec points
	//! with ra,dec expressed in degree in the ICRS reference frame.
	//! Use QJSONParser to transform a JSON representation into QVariant.
	virtual bool loadFromQVariant(const QVariantMap& contours);

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
	
protected:
	static inline bool sideContains(const Vec3d& v1, const Vec3d& v2, const Vec3d& p)
	{
		return  (v2[1] * v1[2] - v2[2] * v1[1])*p[0] +
				(v2[2] * v1[0] - v2[0] * v1[2])*p[1] +
				(v2[0] * v1[1] - v2[1] * v1[0])*p[2] >= 0.;
	}
};

//! @class SphericalPolygon
//! A SphericalPolygon is a complex shape defined by the union of contours.
//! Each contour is composed of connected great circles segments with the last point connected to the first one.
//! Contours don't need to be convex (they are internally tesselated into triangles).
class SphericalPolygon : public SphericalPolygonBase
{
public:
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
	virtual QVector<Vec2d> getTextureCoordArray() const {return QVector<Vec2d>();}
	
	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule the winding rule to use. Default value is WindingPositive, meaning that the 
	//! polygon is the union of the positive contours minus the negative ones.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, SphericalPolygonBase::PolyWindingRule windingRule=SphericalPolygonBase::WindingPositive,
		const QVector<QVector<Vec2d> >& textureCoordsContours=QVector<QVector<Vec2d> >());
	
	//! Set a single contour defining the SphericalPolygon.
	//! @param contours a contour defining the polygon area.
	virtual void setContour(const QVector<Vec3d>& contour);
	
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

//! @class SphericalPolygonTexture
//! An extension of SphericalPolygon with addition of texture coordinates.
class SphericalPolygonTexture : public SphericalPolygon
{
public:

	//! Constructor from a list of contours.
	SphericalPolygonTexture(const QVector<QVector<Vec3d> >& contours, const QVector<QVector<Vec2d> >& texCoords) {setContours(contours, SphericalPolygonBase::WindingPositive, texCoords);}

	//! Constructor from one contour.
	SphericalPolygonTexture(const QVector<Vec3d>& contour, const QVector<Vec2d>& texCoords) {setContour(contour, texCoords);}
	
	//! Return an openGL compatible array of texture coords to be used using vertex arrays.
	virtual QVector<Vec2d> getTextureCoordArray() const {return textureCoords;}
	
	//! Set the contours defining the SphericalPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule the winding rule to use. Default value is WindingPositive, meaning that the 
	//! polygon is the union of the positive contours minus the negative ones.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, SphericalPolygonBase::PolyWindingRule windingRule=SphericalPolygonBase::WindingPositive,
		const QVector<QVector<Vec2d> >& textureCoordsContours=QVector<QVector<Vec2d> >());
	
	//! Set a single contour defining the SphericalPolygon.
	//! @param contours a contour defining the polygon area.
	virtual void setContour(const QVector<Vec3d>& contour, const QVector<Vec2d>& textureCoordsContours=QVector<Vec2d>());

private:
	friend void vertexTextureCallback(void* vertexData, void* userData);
	
	//! A list of uv textures coordinates corresponding to the triangle vertices.
	//! There should be 1 uv position per vertex.
	QVector<Vec2d> textureCoords;
};

//! @class SphericalConvexPolygon
//! A special case of SphericalPolygon for which the polygon is convex.
class SphericalConvexPolygon : public SphericalPolygonBase
{
public:
	//! Default constructor.
	SphericalConvexPolygon() {;}

	//! Constructor from a list of contours.
	SphericalConvexPolygon(QVector<QVector<Vec3d> >& contours) {setContours(contours);}

	//! Constructor from one contour.
	SphericalConvexPolygon(QVector<Vec3d>& contour) {setContour(contour);}
	
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
	virtual QVector<Vec2d> getTextureCoordArray() const {return QVector<Vec2d>();}
	
	//! Return true if the polygon is an empty polygon.
	virtual bool isEmpty() const {return contour.isEmpty();}
	
	//! Set the contours defining the SphericalConvexPolygon.
	//! @param contours the list of contours defining the polygon area.
	//! @param windingRule unused for SphericalConvexPolygon.
	virtual void setContours(const QVector<QVector<Vec3d> >& contours, PolyWindingRule windingRule=WindingPositive,
		const QVector<QVector<Vec2d> >& textureCoordsContours=QVector<QVector<Vec2d> >())
	{
		Q_ASSERT(contours.size()==1);
		Q_ASSERT(textureCoordsContours.isEmpty());
		contour=contours.at(0);
	}
	
	//! Set a single contour defining the SphericalPolygon.
	//! @param contours a contour defining the polygon area.
	virtual void setContour(const QVector<Vec3d>& acontour) {contour=acontour;}
	
	//! Get the contours defining the SphericalConvexPolygon.
	virtual QVector<QVector<Vec3d> > getContours() const {QVector<QVector<Vec3d> > contours; contours.append(contour); return contours;}
	
	//! Returns whether a point is contained into the region.
	virtual bool contains(const Vec3d& p) const;
	
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
	
	//! Return the list of halfspace bounding the ConvexPolygon.
	QVector<HalfSpace> getBoundingHalfSpaces() const;
	
private:
	//! A list of vertices of the convex contour.
	QVector<Vec3d> contour;
	
	//! Tell whether the points of the passed Polygon are all outside of at least one HalfSpace
	bool areAllPointsOutsideOneSide(const QVector<Vec3d>& points) const
	{
		for (int i=0;i<contour.size()-1;++i)
		{
			bool allOutside = true;
			for (QVector<Vec3d>::const_iterator v=points.begin();v!=points.end()&& allOutside==true;++v)
			{
				allOutside = allOutside && !sideContains(contour.at(i), contour.at(i+1), *v);
			}
			if (allOutside)
				return true;
		}
		
		// Last iteration
		bool allOutside = true;
		for (QVector<Vec3d>::const_iterator v=points.begin();v!=points.end()&& allOutside==true;++v)
		{
			allOutside = allOutside && !sideContains(contour.last(), contour.at(0), *v);
		}
		if (allOutside)
			return true;
		
		// Else
		return false;
	}
};


//! Compute the intersection of 2 halfspaces on the sphere (usually on 2 points) and return it in p1 and p2.
//! If the 2 HalfSpace don't interesect or intersect only at 1 point, false is returned and p1 and p2 are undefined
bool planeIntersect2(const HalfSpace& h1, const HalfSpace& h2, Vec3d& p1, Vec3d& p2);


#endif // _STELSPHEREGEOMETRY_HPP_

