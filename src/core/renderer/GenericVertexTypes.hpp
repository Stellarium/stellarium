#ifndef _GENERICVERTEXTYPES_HPP_
#define _GENERICVERTEXTYPES_HPP_

#include "StelVertexAttribute.hpp"
#include "VecMath.hpp"

//! Vertex with a 3D position and a 2D texture coordinate.
struct VertexP3T2
{
	Vec3f position;
	Vec2f texCoord;

	//! Construct from a 3D position and 2D texture coordinate.
	VertexP3T2(const Vec3f& position, const Vec2f texCoord) 
		: position(position), texCoord(texCoord) {}
	
	//! Construct from a 2D position, setting the z coord to 0, and 2D texture coordinate.
	VertexP3T2(const Vec2f pos, const Vec2f texCoord) 
		: position(pos[0], pos[1], 0.0f), texCoord(texCoord) {}

	VERTEX_ATTRIBUTES(Vec3f Position, Vec2f TexCoord);
};

//! Vertex with a 3D position, 2D texture coordinate and an RGBA color.
struct VertexP3T2C4
{
	Vec3f position;
	Vec2f texCoord;
	Vec4f color;

	//! Construct from a 3D position, a texture coordinate and a color,
	VertexP3T2C4(const Vec3f& position, const Vec2f texCoord, const Vec4f& color) 
		: position(position), texCoord(texCoord), color(color) {}
	
	//! Construct from a 2D position, setting the z coord to 0, a texture coordinate and a color.
	VertexP3T2C4(const Vec2f pos, const Vec2f texCoord, const Vec4f& color) 
		: position(pos[0], pos[1], 0.0f), texCoord(texCoord), color(color) {}

	VERTEX_ATTRIBUTES(Vec3f Position, Vec2f TexCoord, Vec4f Color);
};
#endif // _GENERICVERTEXTYPES_HPP_
