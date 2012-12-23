/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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

#ifndef _STELGEOMETRYBUILDER_HPP_
#define _STELGEOMETRYBUILDER_HPP_


#include "GenericVertexTypes.hpp"
#include "StelIndexBuffer.hpp"
#include "StelLight.hpp"
#include "StelProjector.hpp"
#include "StelVertexBuffer.hpp"
#include "VecMath.hpp"


//! Parameters specifying how to generate a sphere.
//!
//! These are passed to StelGeometryBuilder to build a sphere.
//!
//! This is a builder-style struct. Parameters can be specified like this:
//!
//! @code
//! // Radius (required parameter) set to 5, optional parameters have default values
//! // (completely spherical, 20 stacks, 20 slices, oriented outside, texture not flipped)
//! SphereParams a = SphereParams(5.0f);
//! // Higher detail sphere
//! SphereParams b = SphereParams(5.0f).resolution(40, 40);
//! // Oblate "sphere" with faces oriented inside
//! SphereParams c = SphereParams(5.0f).oneMinusOblateness(0.5f).orientInside();
//! @endcode
//!
//! @see StelGeometrySphere, StelGeometryBuilder::buildSphereUnlit,
//!      StelGeometryBuilder::buildSphereLit, StelGeometryBuilder::buildSphereFisheye
struct SphereParams
{
	//! Construct SphereParams specifying the required radius parameter with other parameters at
	//! default values.
	//!
	//! Default values are no oblateness, 20 stacks, 20 slices, faces oriented outside
	//! and texture not flipped.
	//!
	//! @param radius Radius of the sphere. Must be greater than zero.
	SphereParams(const float radius)
		: radius_(radius)
		, oneMinusOblateness_(1.0f)
		, slices_(20)
		, stacks_(20)
		, orientInside_(false)
		, flipTexture_(false)
	{
	}

	//! Set the oblateness of the sphere.
	//!
	//! 1.0f is a sphere, 0.0f a flat ring.
	//!
	//! Must be at least zero and at most one.
	SphereParams& oneMinusOblateness(const float rhs)
	{
		oneMinusOblateness_ = rhs;
		return *this;
	}

	//! Set resolution (detail) of the sphere grid.
	//!
	//! Higher values result in more detail, but also higher resource usage.
	//!
	//! @param slices Number of slices/columns, i.e. horizontal resolution of the sphere.
	//!               Must be at least 3 and at most 4096.
	//! @param stacks Number of stacks/rows/rings, i.e. vertical resolution of the sphere.
	//!               Must be at least 3 and at most 4096.
	SphereParams& resolution(const int slices, const int stacks)
	{
		slices_ = slices;
		stacks_ = stacks;
		return *this;
	}

	//! If specified, the faces of the sphere will point inside instead of outside.
	SphereParams& orientInside()
	{
		orientInside_ = true;
		return *this;
	}

	//! If specified, texture coords will be flipped on the (texture, not global) X axis.
	SphereParams& flipTexture()
	{
		flipTexture_ = true;
		return *this;
	}

	//! Radius of the sphere.
	float radius_;
	//! Determines how oblate the "sphere" is.
	float oneMinusOblateness_;
	//! Number of slices/columns in the sphere grid.
	int slices_;
	//! Number of stacks/rows in the sphere grid.
	int stacks_;
	//! Are the faces of the sphere oriented inside?
	bool orientInside_;
	//! Are the the texture coordinates flipped on the X axis?
	bool flipTexture_;
};


//! Drawable 3D sphere.
//!
//! Encapsulates vertex and index buffers needed to draw the sphere.
//! These are generated as needed any time sphere parameters change.
//!
//! The sphere is a spherical grid with multiple stacks each composed of multiple slices.
//!
//! Spheres are constructed by StelGeometryBuilder functions buildSphereFisheye(),
//! buildSphereUnlit() and buildSphereLit().
//!
//! @see StelGeometryBuilder::buildSphereUnlit, StelGeometryBuilder::buildSphereLit,
//!      StelGeometryBuilder::buildSphereFisheye
class StelGeometrySphere
{
friend class StelGeometryBuilder;
private:
	//! Types of spheres. Affects how the sphere is generated and drawn.
	enum SphereType
	{
		//! Sphere with fisheye texture coordinates.
		SphereType_Fisheye,
		//! Sphere with regular grid texture coordinates.
		SphereType_Unlit,
		//! Like Unlit, but lighting is baked into vertex colors.
		SphereType_Lit
	};

public:
	//! Destroy the sphere, freeing vertex and index buffers.
	~StelGeometrySphere()
	{
		if(NULL != unlitVertices)
		{
			Q_ASSERT_X(NULL == litVertices, Q_FUNC_INFO,
			           "Both lit and unlit vertex buffers are used");
			delete unlitVertices;
			unlitVertices = NULL;
		}
 		if(NULL != litVertices)
		{
			Q_ASSERT_X(NULL == unlitVertices, Q_FUNC_INFO,
			           "Both lit and unlit vertex buffers are used");
			delete litVertices;
			litVertices = NULL;
		}   
		for(int row = 0; row < rowIndices.size(); ++row)
		{
			delete rowIndices[row];
		}
		rowIndices.clear();
	}

	//! Draw the sphere.
	//!
	//! @param renderer  Renderer to draw the sphere.
	//! @param projector Projector to project the vertices.
	void draw(class StelRenderer* renderer, StelProjectorP projector);

	//! Set radius of the sphere.
	//!
	//! @param radius Radius to set. Must be greater than zero.
	void setRadius(const float radius)
	{
		Q_ASSERT_X(radius > 0.0f, Q_FUNC_INFO, "Sphere radius must be greater than zero");
		this->radius = radius;
		updated = true;
	}

	//! Set the oblateness of the sphere.
	//!
	//! 1.0 is a perfect sphere, 0.0f a flat ring.
	//!
	//! Must be at least zero and at most one.
	void setOneMinusOblateness(const float oneMinusOblateness)
	{
		Q_ASSERT_X(oneMinusOblateness >= 0.0f && oneMinusOblateness <= 1.0f, Q_FUNC_INFO,
		           "Sphere oneMinusOblateness parameter must be at least zero and at most one");
		this->oneMinusOblateness = oneMinusOblateness;
		updated = true;
	}

	//! Set resolution (detail) of the sphere grid.
	//!
	//! Higher values result in more detail, but also higher resource usage.
	//!
	//! @param slices Number of slices/columns, i.e. horizontal resolution of the sphere.
	//!               Must be at least 3 and at most 4096.
	//! @param stacks Number of stacks/rows/rings, i.e. vertical resolution of the sphere.
	//!               Must be at least 3 and at most 4096.
	void setResolution(const int slices, const int stacks)
	{
		// The maximums here must match MAX_STACKS and MAX_SLICES in the cpp file.
		Q_ASSERT_X(stacks >= 3 && stacks < 4096, Q_FUNC_INFO,
		           "There must be at least 3 stacks in a sphere, and at most 4096.");
		Q_ASSERT_X(slices >= 3 && slices < 4096, Q_FUNC_INFO,
		           "There must be at least 3 slices in a sphere, and at most 4096.");
		this->stacks = stacks;
		this->slices = slices;
		updated = true;
	}

	//! Should sphere faces be oriented inside? (Useful e.g. for skyspheres)
	void setOrientInside(const bool orientInside)
	{
		this->orientInside = orientInside;
		updated = true;
	}

	//! Should texture coordinates be flipped on the x (texture coord, not world) axis?
	void setFlipTexture(const bool flipTexture)
	{
		this->flipTexture = flipTexture;
		updated = true;
	}

	//! Update the light. Can only be called if this is a lit sphere (constructed by buildSphereLit). 
	void setLight(const StelLight& light)
	{
		Q_ASSERT_X(type == SphereType_Lit, Q_FUNC_INFO,
		           "Trying to set light for an unlit sphere");
		if(this->light == light){return;}
		this->light = light;
		updated     = true;
	}

private:
	//! Sphere type. Affects how the sphere is generated and drawn.
	const SphereType type;

	//! Have the sphere parameters been updated since the last call to regenerate() ?
	//!
	//! If true, the sphere will be regenerated at the next draw() call.
	bool updated;

	//! Radius of the sphere.
	float radius;
	//! Determines how oblate the "sphere" is.
	float oneMinusOblateness;
	//! Number of stacks/rows in the sphere grid.
	int stacks;
	//! Number of slices/columns in the sphere grid.
	int slices;
	//! Are the faces of the sphere oriented inside?
	bool orientInside;
	//! Is the the texture coordinates flipped on the X axis?
	bool flipTexture;

	//! FOV of fisheye texture coordinates used when type is SphereType_Fisheye.
	const float fisheyeTextureFov;

	//! Light used to bake lighting when type is SphereType_Lit.
	StelLight light;

	//! Vertex buffer used when sphere type is SphereType_Fisheye or SphereType_Unlit
	StelVertexBuffer<VertexP3T2>* unlitVertices;
	//! Vertex buffer used when sphere type is SphereType_Lit.
	StelVertexBuffer<VertexP3T2C4>* litVertices;

	//! Each index buffer is a triangle strip forming one row of the sphere pointing to the used
	//! vertex buffer.
	QVector<StelIndexBuffer*> rowIndices;

	//! Construct a StelGeometrySphere.
	//!
	//! This can only be used by StelGeometryBuilder functions such as buildSphereUnlit().
	//!
	//! Sphere parameters are set to invalid values and have to be set by the builder function.
	//!
	//! @param type       Type of sphere (fisheye, unlit or lit).
	//! @param textureFov FOV of fisheye texture coordinates. Used only with SphereType_Fisheye.
	//! @param light      Light. Used only when type is SphereType_Lit.
	StelGeometrySphere(const SphereType type, const float textureFov = -100.0f, 
	                   const StelLight& light = StelLight())
		: type(type)
		, updated(true)
		, radius(0.0f)
		, oneMinusOblateness(0.0f)
		, stacks(0)
		, slices(0)
		, orientInside(false)
		, flipTexture(false)
		, fisheyeTextureFov(textureFov)
		, light(light)
		, unlitVertices(NULL)
		, litVertices(NULL)
	{
		if(type == SphereType_Fisheye)
		{
			Q_ASSERT_X(textureFov > 0.0f, Q_FUNC_INFO,
			           "Fisheye sphere texture FOV must be greater than zero");
		}
	}

	//! Regenerate the sphere.
	//!
	//! Called at first draw and when sphere parameters change.
	//!
	//! @param renderer  Renderer to create vertex/index buffers.
	//! @param projector Projector used in drawing (used by lighting code).
	void regenerate(class StelRenderer* renderer, StelProjectorP projector);
};


//! Parameters specifying how to generate a ring.
//!
//! These are passed to StelGeometryBuilder to build a sphere.
//!
//! This is a builder-style struct. Parameters can be specified like this:
//! 
//! @code
//! // Required parameters, inner and outer radius, set to 5 and 10 respectively,
//! // optional parameters have default values
//! // (20 loops, 20 slices, faces not flipped)
//! RingParams a = RingParams(5.0f, 10.0f);
//! // Higher detail ring
//! RingParams b = RingParams(5.0f, 10.0f).resolution(40, 40);
//! // Ring facing the opposite side
//! RingParams c = RingParams(5.0f, 10.0f).flipFaces();
//! @endcode
//!
//! @see StelGeometryRing, StelGeometryBuilder::buildRingTextured, StelGeometryBuilder::buildRing2D
struct RingParams
{
	//! Construct RingParams specifying the required inner and outer radius parameter 
	//! and other parameters at default values.
	//!
	//! Default values are 20 loops, 20 slices and faces not flipped.
	//!
	//! @param innerRadius Inner radius. Must be greater than zero.
	//! @param outerRadius Outer radius. Must be greater than the inner radius.
	RingParams(const float innerRadius, const float outerRadius)
		: innerRadius_(innerRadius)
		, outerRadius_(outerRadius)
		, loops_(20)
		, slices_(20)
		, flipFaces_(false)
	{
	}

	//! Set resolution (detail) of the ring.
	//!
	//! Higher values result in more detail, but also higher resource usage.
	//!
	//! @param slices Number of slices/subdivisions. E.g. 3 is a triangle, 5 a pentagon, etc.
	//!               Must be at least 3 and at most 4096.
	//! @param loops  Number of concentric loops in the ring.
	//!               Must be at least 1.
	RingParams& resolution(const int slices, const int loops)
	{
		slices_ = slices;
		loops_ = loops;
		return *this;
	}

	//! If specified, the ring will be flipped to face the opposite side.
	RingParams& flipFaces()
	{
		flipFaces_ = true;
		return *this;
	}

	//! Inner radius of the ring.
	float innerRadius_; 
	//! Outer radius of the ring.
	float outerRadius_;
	//! Number of loops in the ring.
	int loops_;
	//! Number of slices/subdivisions in the ring.
	int slices_;
	//! Should the faces in the ring be flipped to face the opposite side?
	bool flipFaces_;
};


//! Drawable 2D or 3D ring.
//!
//! Encapsulates vertex and index buffers needed to draw a ring.
//! These are generated as needed any time ring parameters change.
//!
//! The ring is a flat circle with a hole in center formed by a 
//! circular grid with multiple loops each composed of multiple slices.
//!
//! Rings are constructed by StelGeometryBuilder functions buildRingTextured(),
//! and buildRing2D().
//!
//! @see StelGeometryBuilder::buildRingTextured, StelGeometryBuilder::buildRing2D
class StelGeometryRing
{
friend class StelGeometryBuilder;
private:
	//! Types of rings. Affects how the ring is generated and drawn.
	enum RingType
	{
		RingType_Textured,
		RingType_Plain2D
	};

public:
	//! Destroy the ring, freeing vertex and index buffers.
	~StelGeometryRing()
	{
		if(NULL != texturedVertices)
		{
			Q_ASSERT_X(NULL == plain2DVertices, Q_FUNC_INFO,
			           "Both textured and 2D vertex buffers are used");
			delete texturedVertices;
			texturedVertices = NULL;
		}
		if(NULL != plain2DVertices)
		{
			Q_ASSERT_X(NULL == texturedVertices, Q_FUNC_INFO,
			           "Both textured and 2D vertex buffers are used");
			delete plain2DVertices;
			plain2DVertices = NULL;
		}
		for(int loop = 0; loop < loopIndices.size(); ++loop)
		{
			delete loopIndices[loop];
		}
		loopIndices.clear();
	}

	//! Draw the ring.
	//!
	//! @param renderer  Renderer to draw the ring.
	//! @param projector Projector to project the vertices, if any.
	//!                  Not used for 2D rings.
	void draw(class StelRenderer* renderer, StelProjectorP projector = StelProjectorP());
	
	//! Set inner and outer radius of the ring.
	//!
	//! @param inner Inner radius. Must be greater than zero.
	//! @param outer Outer radius. Must be greater than the inner radius.
	void setInnerOuterRadius(const float inner, const float outer)
	{
		Q_ASSERT_X(inner > 0.0f, Q_FUNC_INFO, "Inner ring radius must be greater than zero");

		// No need to regenerate
		if(inner == this->innerRadius && outer == this->outerRadius) {return;}

		this->innerRadius = inner;
		this->outerRadius = outer;
		updated = true;
	}

	//! Set resolution (detail) of the ring.
	//!
	//! Higher values result in more detail, but also higher resource usage.
	//!
	//! @param slices Number of slices/subdivisions. E.g. 3 is a triangle, 5 a pentagon, etc.
	//!               Must be at least 3 and at most 4096.
	//! @param loops  Number of concentric loops in the ring.
	//!               Must be at least 1.
	void setResolution(const int slices, const int loops)
	{
		Q_ASSERT_X(loops >= 1, Q_FUNC_INFO, "There must be at least 1 loop in a ring");
		// Must correspond with MAX_SLICES in the .cpp file.
		Q_ASSERT_X(slices >= 3 && slices <= 4096, Q_FUNC_INFO,
		           "There must be at least 3 and at most 4096 slices in a ring.");

		// No need to regenerate
		if(slices == this->slices && loops == this->loops) {return;}

		this->loops = loops;
		this->slices = slices;
		updated = true;
	}

	//! Should the ring be flipped to face the opposite side?
	void setFlipFaces(const bool flipFaces)
	{
		// No need to regenerate
		if(flipFaces == this->flipFaces) {return;}
		this->flipFaces = flipFaces;
		updated = true;
	}

	//! Set offset (position) of the ring (for 2D rings, the Z coordinate is ignored).
	void setOffset(const Vec3f offset)
	{
		// No need to regenerate
		if(offset == this->offset) {return;}
		this->offset = offset;
		updated = true;
	}

private:
	//! Type of ring. Affects how the ring is generated and drawn.
	const RingType type; 

	//! Have the ring parameters been updated since the last call to regenerate() ?
	//!
	//! If true, the ring will be regenerated at the next draw() call.
	bool updated;

	//! Inner radius of the ring.
	float innerRadius; 
	//! Outer radius of the ring.
	float outerRadius;
	//! Number of loops in the ring.
	int loops;
	//! Number of slices/subdivisions in the ring.
	int slices;
	//! Should the faces in the ring be flipped to face the opposite side?
	bool flipFaces;
	//! Offset (position) of the ring.
	Vec3f offset;

	//! Vertex buffer used when ring type is RingType_Textured.
	StelVertexBuffer<VertexP3T2>* texturedVertices;
	//! Vertex buffer used when ring type is RingType_Textured.
	StelVertexBuffer<VertexP2>* plain2DVertices;

	//! Each index buffer is a triangle strip forming one loop of the ring pointing to the used
	//! vertex buffer.
	QVector<StelIndexBuffer*> loopIndices;

	//! Construct a StelGeometryRing.
	//!
	//! This can only be used by StelGeometryRing functions such as buildRingTextured().
	//!
	//! Ring parameters are set to invalid value and must be set by the builder function.
	//!
	//! @param type Type of ring (textured or plain 2D)
	StelGeometryRing(const RingType type)
		: type(type)
		, updated(true)
		, innerRadius(0.0f)
		, outerRadius(0.0f)
		, loops(0)
		, slices(0)
		, flipFaces(0)
		, offset(0.0f, 0.0f, 0.0f)
		, texturedVertices(NULL)
		, plain2DVertices(NULL)
	{
	}

	//! Regenerate the ring.
	//!
	//! Called at first draw and when ring parameters change.
	//!
	//! @param renderer  Renderer to create vertex/index buffers.
	void regenerate(class StelRenderer* renderer);
};


//! Builds various geometry primitives, storing them in vertex buffers.
class StelGeometryBuilder
{
public:

	//! Build a 2D circle.
	//!
	//! @param vertexBuffer Vertex buffer to store the circle. Must be empty,
	//!                     and have the line strip primitive type.
	//! @param x            X position of the center of the ring.
	//! @param y            Y position of the center of the ring.
	//! @param radius       Radius of the ring. Must be greater than zero.
	//! @param segments     Number of segments to subdivide the ring into.
	void buildCircle(StelVertexBuffer<VertexP2>* vertexBuffer, 
	                 const float x, const float y, const float radius, 
	                 const int segments = 128)
	{
		Q_ASSERT_X(radius > 0.0f, Q_FUNC_INFO, "Circle must have a radius greater than zero");
		Q_ASSERT_X(segments > 3, Q_FUNC_INFO, "Circle must have at least 3 segments");
		Q_ASSERT_X(vertexBuffer->length() == 0, Q_FUNC_INFO,
		           "Need an empty vertex buffer to build a circle");
		Q_ASSERT_X(vertexBuffer->primitiveType() == PrimitiveType_LineStrip, Q_FUNC_INFO,
		           "Need a line loop vertex buffer to build a circle");

		const Vec2f center(x,y);
		const float phi = 2.0f * M_PI / segments;
		const float cp = std::cos(phi);
		const float sp = std::sin(phi);
		float dx = radius;
		float dy = 0;

		vertexBuffer->unlock();

		for (int i = 0; i < segments; i++)
		{
			vertexBuffer->addVertex(VertexP2(x + dx, y + dy));
			const float dxNew = dx * cp - dy * sp;
			dy = dx * sp + dy * cp;
			dx = dxNew;
		}
		vertexBuffer->addVertex(VertexP2(x + radius, y));
		vertexBuffer->lock();
	}

	//! Build a cylinder without top and bottom caps.
	//!
	//! @param vertexBuffer Vertex buffer to store the cylinder. Must be empty, and have the
	//!                     triangle strip primitive type.
	//! @param radius       Radius of the cylinder.
	//! @param height       Height of the cylinder.
	//! @param slices       Number of slices (sides) of the cylinder.
	//!                     Must be at least 3.
	//! @param orientInside Should the cylinder's faces point inside?
	void buildCylinder(StelVertexBuffer<VertexP3T2>* const vertexBuffer, 
	                   const float radius, const float height, const int slices,
	                   const bool orientInside = false)
	{
		// No need for indices - w/o top/bottom a cylinder is just a single tristrip
		// with no duplicate vertices except start/end

		Q_ASSERT_X(vertexBuffer->length() == 0, Q_FUNC_INFO, 
		           "Need an empty vertex buffer to start building a cylinder");
		Q_ASSERT_X(vertexBuffer->primitiveType() == PrimitiveType_TriangleStrip, 
		            Q_FUNC_INFO, "Need a triangle strip vertex buffer to build a cylinder");
		Q_ASSERT_X(slices >= 3, Q_FUNC_INFO, 
		           "can't build a cylinder with less than 3 slices");

		vertexBuffer->unlock();
		float texCoord = orientInside ? 1.0f : 0.0f;
		float angle    = 0.0f;

		// Needed to generate vertices with opposite winding if orientInside == true.
		const float sign          = orientInside ? -1.0f : 1.0f;
		const float deltaTexCoord = 1.0f / slices;
		const float deltaAngle    = 2.0f * M_PI / slices;

		for (int i = 0; i <= slices; ++i)
		{
			const float x = std::sin(angle) * radius;
			const float y = std::cos(angle) * radius;

			vertexBuffer->addVertex(VertexP3T2(Vec3f(x, y, 0.0f),   Vec2f(texCoord, 0.0f)));
			vertexBuffer->addVertex(VertexP3T2(Vec3f(x, y, height), Vec2f(texCoord, 1.0f)));

			// If oriented outside, we go from 0 at 0deg to 1 at 360deg
			// If oriented inside, we go from 1 at 360 deg to 0 at 0deg
			texCoord += sign * deltaTexCoord;
			angle    += sign * deltaAngle;
		}
		vertexBuffer->lock();
	}

	//! Build a disk having texture center at center of disk.
	//! The disk is composed from concentric circles with increasing refinement.
	//! The number of slices of the outmost circle is (innerFanSlices << level).
	//!
	//! Index buffer is used to decrease vertex count.
	//!
	//! @param vertexBuffer   Vertex buffer to store the fan disk. 
	//!                       Must be empty, and have the triangles primitive type.
	//! @param indexBuffer    Index buffer to store indices specifying the triangles
	//!                       to draw.
	//! @param radius         Radius of the disk.
	//! @param innerFanSlices Number of slices. Must be at least 3.
	//! @param level          Number of concentric circles. Must be at most 31,
	//!                       but much lower (e.g; 8) is recommended.
	void buildFanDisk(StelVertexBuffer<VertexP3T2>* const vertexBuffer,
	                  StelIndexBuffer* const indexBuffer, const float radius,
	                  const int innerFanSlices, const int level);

	//! Build a fisheye-textured sphere.
	//!
	//! The sphere must be deleted to free graphics resources.
	//!
	//! @param params     Sphere parameters.
	//! @param textureFov Field of view of the texture coordinates.
	//!
	//! @see SphereParams
	StelGeometrySphere* buildSphereFisheye(const SphereParams& params, const float textureFov)
	{
		StelGeometrySphere* result = 
			new StelGeometrySphere(StelGeometrySphere::SphereType_Fisheye, textureFov);
		result->setRadius(params.radius_);
		result->setOneMinusOblateness(params.oneMinusOblateness_);
		result->setResolution(params.slices_, params.stacks_);
		result->setOrientInside(params.orientInside_);
		result->setFlipTexture(params.flipTexture_);
		return result;
	}

	//! Build a regularly texture mapped sphere.
	//!
	//! Texture coordinates: x goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y 
	//! sides of the sphere, y goes from -1.0/+1.0 at z = -radius/+radius 
	//! (linear along longitudes)
	//!
	//! The sphere must be deleted to free graphics resources.
	//!
	//! @param params Sphere parameters.
	//!
	//! @see SphereParams
	StelGeometrySphere* buildSphereUnlit(const SphereParams& params)
	{
		StelGeometrySphere* result = 
			new StelGeometrySphere(StelGeometrySphere::SphereType_Unlit);
		result->setRadius(params.radius_);
		result->setOneMinusOblateness(params.oneMinusOblateness_);
		result->setResolution(params.slices_, params.stacks_);
		result->setOrientInside(params.orientInside_);
		result->setFlipTexture(params.flipTexture_);
		return result;
	}

	//! Build a regularly texture mapped sphere with lighting baked into vertex colors.
	//!
	//! Texture coordinates: x goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y 
	//! sides of the sphere, y goes from -1.0/+1.0 at z = -radius/+radius 
	//! (linear along longitudes)
	//!
	//! The sphere must be deleted to free graphics resources.
	//!
	//! @param params Sphere parameters.
	//! @param light  Light to use for lighting.
	//!
	//! @see SphereParams
	StelGeometrySphere* buildSphereLit(const SphereParams& params, const StelLight& light)
	{
		StelGeometrySphere* result = 
			new StelGeometrySphere(StelGeometrySphere::SphereType_Lit, -100.0f, light);
		result->setRadius(params.radius_);
		result->setOneMinusOblateness(params.oneMinusOblateness_);
		result->setResolution(params.slices_, params.stacks_);
		result->setOrientInside(params.orientInside_);
		result->setFlipTexture(params.flipTexture_);
		return result;
	}

	//! Build a flat texture mapped ring with 3D coordinates (for example for planet's rings)
	//!
	//! (Note that the parameters refer to RingParams parameters)
	//!
	//! The ring is a disk with a radius specified by the outerRadius parameter 
	//! , with a circular hole in center that has radius set by
	//! the innerRadius parameter. The ring is subdivided into multiple concentric loops,
	//! and radially into multiple slices.
	//!
	//! Orientation of the ring's faces can be flipped by the flipFaces parameter.
	//! The offset parameter is an offset that is added to positions of all vertices 
	//! in the ring.
	//! 
	//! The ring must be deleted to free graphics resources.
	//!
	//! @see RingParams
	StelGeometryRing* buildRingTextured
		(const RingParams& params, const Vec3f offset = Vec3f(0.0f, 0.0f, 0.0f))
	{
		StelGeometryRing* result = new StelGeometryRing(StelGeometryRing::RingType_Textured);
		result->setInnerOuterRadius(params.innerRadius_, params.outerRadius_);
		result->setResolution(params.slices_, params.loops_);
		result->setFlipFaces(params.flipFaces_);
		result->setOffset(offset);
		return result;
	}

	//! Build a 2D ring without texture mapping.
	//!
	//! (Note that the parameters refer to RingParams parameters)
	//!
	//! The ring is a disk with a radius specified by the outerRadius parameter 
	//! , with a circular hole in center that has radius set by
	//! the innerRadius parameter. The ring is subdivided into multiple concentric loops,
	//! and radially into multiple slices.
	//!
	//! Orientation of the ring's faces can be flipped by the flipFaces parameter.
	//! The offset parameter is an offset that is added to positions of all vertices 
	//! in the ring.
	//!
	//! The ring must be deleted to free graphics resources.
	//! 
	//! @see RingParams
	StelGeometryRing* buildRing2D
		(const RingParams& params, const Vec2f offset = Vec2f(0.0f, 0.0f))
	{
		StelGeometryRing* result = new StelGeometryRing(StelGeometryRing::RingType_Plain2D);
		result->setInnerOuterRadius(params.innerRadius_, params.outerRadius_);
		result->setResolution(params.slices_, params.loops_);
		result->setFlipFaces(params.flipFaces_);
		result->setOffset(Vec3f(offset[0], offset[1], 0.0f));
		return result;
	}
};
#endif // _STELGEOMETRYBUILDER_HPP_
