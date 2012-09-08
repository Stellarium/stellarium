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

#ifndef _GENERICVERTEXTYPES_HPP_
#define _GENERICVERTEXTYPES_HPP_

#include "StelVertexAttribute.hpp"
#include "VecMath.hpp"

//! Vertex with only a 2D position.
struct VertexP2
{
	Vec2f position;

	//! Construct from a 2D position.
	VertexP2(const Vec2f position) : position(position) {}

	//! Construct from X and Y coordinates.
	VertexP2(const float x, const float y) : position(x, y) {}
	VERTEX_ATTRIBUTES(Vec2f Position);
};

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
