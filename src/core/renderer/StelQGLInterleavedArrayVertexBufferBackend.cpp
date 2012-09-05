
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

#include "StelProjector.hpp"
#include "StelQGLInterleavedArrayVertexBufferBackend.hpp"
#include "StelQGLIndexBuffer.hpp"


StelQGLInterleavedArrayVertexBufferBackend::
StelQGLInterleavedArrayVertexBufferBackend
	(const PrimitiveType type, const QVector<StelVertexAttribute>& attributes)
	: StelVertexBufferBackend(attributes)
	, locked(false)
	, primitiveType(type)
	, vertexCount(0)
	, vertexCapacity(4)
	, usingProjectedPositions(false)
	, projectedPositions(NULL)
	, projectedPositionsCapacity(0)
{
	vertexBytes = 0;
	for(int attrib = 0; attrib < this->attributes.count; ++attrib)
	{
		vertexBytes += attributeSize(this->attributes.attributes[attrib].type);
	}
	// Align to VERTEX_ALIGN
	vertexStride = (vertexBytes + VERTEX_ALIGN - 1) & ~(VERTEX_ALIGN - 1);
	vertices = static_cast<char*>(std::malloc(vertexCapacity * vertexStride));
}

StelQGLInterleavedArrayVertexBufferBackend::~StelQGLInterleavedArrayVertexBufferBackend()
{
	delete vertices;
	if(NULL != projectedPositions)
	{
		std::free(projectedPositions);
	}
}

void StelQGLInterleavedArrayVertexBufferBackend::addVertex(const void* const vertexInPtr)
{
	//StelVertexBuffer enforces bounds, so we don't need to
	Q_ASSERT_X(vertexCount <= vertexCapacity, Q_FUNC_INFO,
	           "Vertex count is greater than vertex capacity");
	// We're out of space; reallocate.
	if(Q_UNLIKELY(vertexCount == vertexCapacity))
	{
		vertexCapacity *= 2;
		vertices = static_cast<char*>(std::realloc(vertices, vertexCapacity * vertexStride));
	}

	std::memcpy(vertices + vertexCount * vertexStride, vertexInPtr, vertexBytes);
	++vertexCount;
}

void StelQGLInterleavedArrayVertexBufferBackend::getVertex
	(const int index, void* const vertexOutPtr) const
{
	std::memcpy(vertexOutPtr, vertices + index * vertexStride, vertexBytes);
}

void StelQGLInterleavedArrayVertexBufferBackend::
	projectVertices(StelProjector* projector, StelQGLIndexBuffer* indexBuffer)
{
	// This is a backend function called during (right before) drawing, so we need 
	// to be locked.
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to project a vertex buffer that is not locked.");

	// Index of the position attribute in attributes.
	int posIndex  = 0;
	// Offset of the position attribute relative to start of each vertex in
	// the vertices array.
	int posOffset = 0;
	for(; posIndex < attributes.count; ++posIndex)
	{
		const StelVertexAttribute& attribute(attributes.attributes[posIndex]);
		if(attribute.interpretation == AttributeInterpretation_Position)
		{
			break;
		}
		posOffset += attributeSize(attribute.type);
	}

	// Get the position attribute and ensure that we can project it.
#ifndef NDEBUG
	const StelVertexAttribute& attribute(attributes.attributes[posIndex]);
	Q_ASSERT_X(attributeDimensions(attribute.type) == 3, Q_FUNC_INFO,
	           "Trying to use a custom StelProjector to project non-3D vertex positions");
	Q_ASSERT_X(attribute.type == AttributeType_Vec3f, Q_FUNC_INFO, 
	           "Unknown 3D vertex attribute type");
#endif

	usingProjectedPositions = true;

	// We have two different cases :
	// a) Not using an index array. Size of vertex data is known.
	// b) Using an index array. 
	//    We find the max index and project vertices until that index, 
	//    or if there are not many indices, project vertices separately 
	//    based on indices.
	const int minProjectedSize =
		(NULL == indexBuffer) ? vertexCount : (indexBuffer->maxIndex() + 1);

	if(Q_UNLIKELY(NULL == projectedPositions))
	{
		projectedPositions =
			static_cast<Vec3f*>(std::malloc(minProjectedSize * sizeof(Vec3f)));
	}
	else if(projectedPositionsCapacity < minProjectedSize)
	{
		projectedPositions =
			static_cast<Vec3f*>(std::realloc(projectedPositions, minProjectedSize * sizeof(Vec3f)));
		projectedPositionsCapacity = minProjectedSize;
	}

	// If the index buffer is large, it probably covers most 
	// of the vertex buffer so we can just project everything en masse, 
	// taking advantage of the cache.
	if(NULL == indexBuffer || indexBuffer->length() >= minProjectedSize)
	{
		const char* position = vertices + posOffset;
		for(int i = 0; i < minProjectedSize; ++i, position += vertexStride)
		{
			projector->project(*reinterpret_cast<const Vec3f*>(position), projectedPositions[i]);
		}
	}
	else
	{
		const char* const firstPosition = vertices + posOffset;
		if(indexBuffer->indexType() == IndexType_U16)
		{
			const ushort* const indices = indexBuffer->indices16.constData();
			const int indexCount        = indexBuffer->indices16.size();
			for(int i = 0; i < indexCount; ++i)
			{
				const ushort index = indices[i];
				const Vec3f* const position = 
					(reinterpret_cast<const Vec3f*>(firstPosition + index * vertexStride));
				projector->project(*position, projectedPositions[index]);
			}
		}
		else if(indexBuffer->indexType() == IndexType_U32)
		{
			const uint* const indices = indexBuffer->indices32.constData();
			const int indexCount      = indexBuffer->indices32.size();
			for(int i = 0; i < indexCount; ++i)
			{
				const uint index = indices[i];
				const Vec3f* const position = 
					(reinterpret_cast<const Vec3f*>(firstPosition + index * vertexStride));
				projector->project(*position, projectedPositions[index]);
			}
		}
		else
		{
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
		}
	}
}
