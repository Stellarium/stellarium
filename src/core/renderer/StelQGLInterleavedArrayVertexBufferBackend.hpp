
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

#ifndef _STELQGLINTERLEAVEDARRAYVERTEXBUFFERBACKEND_HPP_
#define _STELQGLINTERLEAVEDARRAYVERTEXBUFFERBACKEND_HPP_

#include <cstring>

#include "VecMath.hpp"
#include "StelProjectorType.hpp"
#include "StelVertexBuffer.hpp"
#include "StelVertexBufferBackend.hpp"
#include "StelVertexAttribute.hpp"


//! Base class for QGL-using vertex buffer backends based on interleaved attribute arrays.
//!
//! Vertex attributes are interleaved in a single array instead of
//! having separate arrays for each attribute. E.g. for a vertex format
//! with a Vec3f position and a Vec2f texcoord, the attributes are stored like this:
//!
//! VVVTTVVVTTVVV...
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelQGLInterleavedArrayVertexBufferBackend : public StelVertexBufferBackend
{
private:
	static const int VERTEX_ALIGN = 16;
public:
	virtual ~StelQGLInterleavedArrayVertexBufferBackend();

	virtual void addVertex(const void* const vertexInPtr);

	virtual void getVertex(const int index, void* const vertexOutPtr) const;

	virtual void setVertex(const int index, const void* const vertexInPtr)
	{
		std::memcpy(vertices + index * vertexStride, vertexInPtr, vertexBytes);
	}

	virtual void lock()
	{
		locked = true;
	}

	virtual void unlock()
	{
		locked = false;
	}

	virtual void clear()
	{
		vertexCount = 0;
	}

	//! Use a StelProjector to project vertex positions.
	//!
	//! Can be called only immediately before drawing. The projected vertex 
	//! positions are only used for one draw call - another one might use a
	//! different/modified projector or index buffer.
	//!
	//! @param projector   Projector to project the vertices.
	//! @param indexBuffer Index buffer specifying which vertices to project.
	//!                    If NULL, all vertices are projected.
	void projectVertices(StelProjector* projector, 
	                     class StelQGLIndexBuffer* indexBuffer);

	//! Get the number of vertices in the buffer.
	int length() const
	{
		return vertexCount;
	}

	//! Return graphics primitive type formed by the vertices of the buffer,
	PrimitiveType getPrimitiveType() const
	{
		return primitiveType;
	}

protected:
	//! Is the vertex buffer locked (i.e. ready to draw?).
	bool locked;

	//! Graphics primitive type formed by the vertices of this buffer.
	PrimitiveType primitiveType;

	//! Number of vertices in the buffer.
	int vertexCount;

	//! Number of vertices we have allocated space for.
	int vertexCapacity;

	//! Size of a single vertex in bytes.
	int vertexBytes;

	//! Size allocated for a single vertex (size of a vertex in the vertices array)
	//!
	//! Used for alignment.
	int vertexStride;

	//! Vertices of the buffer.
	//!
	//! Vertices are stored in an interleaved format; e.g. 
	//! if the vertex format has a Vec3f vertex and Vec2f texcoord, the 
	//! vertices are stored like this (every character represents a single float):
	//!
	//! VVVTTVVVTTVVV...
	//!
	//! Note: On 64bit, this is aligned to 16 bytes by default.
	char* vertices;

	//! Are we using vertex positions projected by a StelProjector?
	//!
	//! (Instead of letting OpenGL handle the projection)
	//!
	//! This is set to true by projectVertices() and back to false by 
	//! the draw call immediately after. The projected positions can only be used 
	//! for one draw call, as for another one the StelProjector might be different/modified.
	bool usingProjectedPositions;

	//! Projected vertex positions to draw when we're projecting vertices with a StelProjector.
	//!
	//! This is a simple array of projected vertex positions (unprojected positions 
	//! are in the vertices array interleaved with other attributes).
	//! usingProjectedPositions is true. The positions are projected by projectVertices().
	Vec3f* projectedPositions;

	//! Allocated space in the projectedPositions array.
	int projectedPositionsCapacity;

	//! Construct a StelQGLInterleavedArrayVertexBufferBackend.
	//!
	//! Initializes vertex attribute buffers.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	//! @param attributes Specifications of vertex attributes that will be stored in the buffer.
	StelQGLInterleavedArrayVertexBufferBackend(const PrimitiveType type,
	                                           const QVector<StelVertexAttribute>& attributes);
};

#endif // _STELQGLINTERLEAVEDARRAYVERTEXBUFFERBACKEND_HPP_
