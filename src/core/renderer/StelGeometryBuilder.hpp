#ifndef _STELGEOMETRYBUILDER_HPP_
#define _STELGEOMETRYBUILDER_HPP_

#include "StelIndexBuffer.hpp"
#include "StelVertexBuffer.hpp"
#include "GenericVertexTypes.hpp"

//! Builds various geometry primitives, storing them in vertex buffers.
class StelGeometryBuilder
{
public:
	//! Build a cylinder vertex buffer without top and bottom caps.
	//!
	//! @param vertexBuffer Vertex buffer (of 3D-position/2D-texcoord vertices)
	//!                     to store the cylinder. Must be empty, and have the
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
	//! The disk is made up of concentric circles with increasing refinement.
	//! The number of slices of the outmost circle is (innerFanSlices << level).
	//!
	//! Index buffer is used to decrease vertex count.
	//!
	//! @param vertexBuffer   Vertex buffer (of 3D-position/2D-texcoord vertices)
	//!                       to store the fan disk. Must be empty, and have the
	//!                       triangles primitive type.
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
	//! The sphere is a spherical grid formed by rowIndexBuffers.size()
	//! rows (each a single triangle strip), and slices columns.
	//!
	//! That is, empty index buffers must be provided for each row,
	//! and the row count depends on how many index buffers are provided.
	//! as well as an empty vertex buffer.
	//!
	//! @param vertices        Vertex buffer to store vertices of the sphere.
	//!                        Indices in row index buffers will point to 
	//!                        vertices in this buffer.
	//!                        Must be empty.
	//! @param rowIndexBuffers Index buffers to store rows of the sphere.
	//!                        The number of rows generated is 
	//!                        rowIndexBuffers.size(). At least 3 index buffers
	//!                        must be provided, and all of them must be empty.
	//! @param radius          Radius of the sphere.
	//! @param slices          Number of columns in each row of the sphere.
	//!                        Must be at least 3.
	//! @param textureFov      Field of view of the texture coordinates.
	//! @param orientInside    Should the front faces of the sphere point inside?
	void buildSphereFisheye
		(StelVertexBuffer<VertexP3T2>* const vertices, 
		 QVector<StelIndexBuffer* >& rowIndexBuffers, const float radius,
		 const int slices, const float textureFov, const bool orientInside = false);

	// The parameter list here is horrible (7 parameters!)
	// A solution might be a Sphere class wrapping up details and drawing of
	// various spheres.
	//! Build a regularly texture mapped sphere.
	//!
	//! Texture coordinates: x goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y 
	//! sides of the sphere, y goes from -1.0/+1.0 at z = -radius/+radius 
	//! (linear along longitudes)
	//!
	//! The sphere is a spherical grid formed by rowIndexBuffers.size()
	//! rows (each a single triangle strip), and slices columns.
	//!
	//! That is, empty index buffers must be provided for each row,
	//! and the row count depends on how many index buffers are provided.
	//! as well as an empty vertex buffer.
	//!
	//! @param vertices           Vertex buffer to store vertices of the sphere.
	//!                           Indices in row index buffers will point to 
	//!                           vertices in this buffer.
	//!                           Must be empty.
	//! @param rowIndexBuffers    Index buffers to store rows of the sphere.
	//!                           The number of rows generated is 
	//!                           rowIndexBuffers.size(). At least 3 index buffers
	//!                           must be provided, and all of them must be empty.
	//! @param radius             Radius of the sphere.
	//! @param oneMinusOblateness Defines how "squished" the sphere is -
	//!                           for planets, this is the polar radius divided 
	//!                           by the equatorial radius.
	//! @param slices             Number of columns in each row of the sphere.
	//!                           Must be at least 3.
	//! @param orientInside       Should the front faces of the sphere point inside?
	//! @param flipTexture        Should the texture coordinates be horizontally flipped?
	void buildSphere 
		(StelVertexBuffer<VertexP3T2>* const vertices, 
		 QVector<StelIndexBuffer*>& rowIndexBuffers, const float radius,
		 const float oneMinusOblateness, const int slices, 
		 const bool orientInside = false, const bool flipTexture = false);
};
#endif // _STELGEOMETRYBUILDER_HPP_
