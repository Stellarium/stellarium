#ifndef _STELGEOMETRYBUILDER_HPP_
#define _STELGEOMETRYBUILDER_HPP_

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
	void buildCylinder(StelVertexBuffer<VertexP3T2>* vertexBuffer, 
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
};
#endif // _STELGEOMETRYBUILDER_HPP_
