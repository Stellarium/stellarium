#ifndef _GENERICVERTEXTYPES_HPP_
#define _GENERICVERTEXTYPES_HPP_

#include "StelVertexAttribute.hpp"
#include "VecMath.hpp"

//! Vertex with a 3D position and a 2D texture coordinate.
struct VertexP3T2
{
	Vec3f position;
	Vec2f texCoord;

	VertexP3T2(const Vec3f& position, const Vec2f texCoord) 
		: position(position), texCoord(texCoord) {}

	VERTEX_ATTRIBUTES(Vec3f Position, Vec2f TexCoord);
};

#endif // _GENERICVERTEXTYPES_HPP_
