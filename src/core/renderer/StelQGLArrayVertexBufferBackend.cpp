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
#include "StelQGLArrayVertexBufferBackend.hpp"
#include "StelQGLIndexBuffer.hpp"


StelQGLArrayVertexBufferBackend::
StelQGLArrayVertexBufferBackend(const PrimitiveType type, 
                                const QVector<StelVertexAttribute>& attributes)
	: StelVertexBufferBackend(attributes)
	, locked(false)
	, primitiveType(type)
	, vertexCount(0)
	, vertexCapacity(4)
	, usingProjectedPositions(false)
	, projectedPositions(NULL)
	, projectedPositionsCapacity(0)
{
	// Unused buffers will be NULL.
	memset(attributeBuffers, '\0', AttributeInterpretation_MAX * sizeof(void*));
	// Create a buffer for each vertex attribute.
	for(int attrib = 0; attrib < this->attributes.count; ++attrib)
	{
		const StelVertexAttribute& attribute(this->attributes.attributes[attrib]);

		attributeBuffers[attribute.interpretation] =
			std::malloc(vertexCapacity * attributeSize(attribute.type));
	}
}

StelQGLArrayVertexBufferBackend::~StelQGLArrayVertexBufferBackend()
{
	for(int buffer = 0; buffer < AttributeInterpretation_MAX; ++buffer)
	{
		if(NULL != attributeBuffers[buffer]){std::free(attributeBuffers[buffer]);}
	}
	if(NULL != projectedPositions)
	{
		std::free(projectedPositions);
	}
}

void StelQGLArrayVertexBufferBackend::addVertex(const void* const vertexInPtr)
{
	//StelVertexBuffer enforces bounds, so we don't need to
	Q_ASSERT_X(vertexCount <= vertexCapacity, Q_FUNC_INFO,
	           "Vertex count is greater than vertex capacity");
	// We're out of space; reallocate.
	if(Q_UNLIKELY(vertexCount == vertexCapacity))
	{
		vertexCapacity *= 2;
		for(int attrib = 0; attrib < attributes.count; ++attrib)
		{
			const StelVertexAttribute& attribute(attributes.attributes[attrib]);
			attributeBuffers[attribute.interpretation] = 
				std::realloc(attributeBuffers[attribute.interpretation], 
				             vertexCapacity * attributeSize(attribute.type));
		}
	}

	setVertexNonVirtual(vertexCount, vertexInPtr);
	++vertexCount;
}

void StelQGLArrayVertexBufferBackend::getVertex
	(const int index, void* const vertexOutPtr) const
{
	// Points to the current attribute (e.g. color, normal, vertex) within output.
	unsigned char* attribPtr = static_cast<unsigned char*>(vertexOutPtr);
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		const AttributeInterpretation interpretation = 
			attributes.attributes[attrib].interpretation;
		// Get each attribute from its buffer and set result's attribute to that.
		switch(attributes.attributes[attrib].type)
		{
			case AttributeType_Vec2f:
				*reinterpret_cast<Vec2f*>(attribPtr) 
					= getAttributeConst<Vec2f>(interpretation, index);
				break;
			case AttributeType_Vec3f:
				*reinterpret_cast<Vec3f*>(attribPtr) 
					= getAttributeConst<Vec3f>(interpretation, index);
				break;
			case AttributeType_Vec4f:
				*reinterpret_cast<Vec4f*>(attribPtr) 
					= getAttributeConst<Vec4f>(interpretation, index);
				break;
			default:
				Q_ASSERT(false);
		}

		// This always works, because the C standard requires that 
		// sizeof(unsigned char) == 1  (that 1 might mean e.g. 16 bits instead of 8
		// on some platforms, but both the size of attribute and of unsigned char is 
		// measured in the same unit, so it's not a problem.
		attribPtr += attributes.sizes[attrib];
	}
}

int StelQGLArrayVertexBufferBackend::
	getAttributeIndex(const AttributeInterpretation interpretation) const
{
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		const StelVertexAttribute& attribute(attributes.attributes[attrib]);
		if(attribute.interpretation == interpretation)
		{
			return attrib;
		}
	}
	return -1;
}

void StelQGLArrayVertexBufferBackend::
	projectVertices(StelProjector* projector, StelQGLIndexBuffer* indexBuffer)
{
	// This is a backend function called during (right before) drawing, so we need 
	// to be locked.
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to project a vertex buffer that is not locked.");

	// Get the position attribute and ensure that we can project it.
#ifndef NDEBUG
	const int posIndex = getAttributeIndex(AttributeInterpretation_Position);
	const StelVertexAttribute& attribute(attributes.attributes[posIndex]);
	Q_ASSERT_X(attributeDimensions(attribute.type) == 3, Q_FUNC_INFO,
	           "Trying to use a custom StelProjector to project non-3D vertex positions");
	Q_ASSERT_X(attribute.type == AttributeType_Vec3f, Q_FUNC_INFO, 
	           "Unknown 3D vertex attribute type");
#endif

	usingProjectedPositions = true;

	const Vec3f* const positions = 
		static_cast<Vec3f*>(attributeBuffers[AttributeInterpretation_Position]);
	Q_ASSERT_X(NULL != positions, Q_FUNC_INFO, "Vertex format without a position attribute");


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

	// If the index buffer is big, it's likely that it covers most 
	// of the vertex buffer so we can just project everything en masse, 
	// taking advantage of the cache.
	if(NULL == indexBuffer || indexBuffer->length() >= minProjectedSize)
	{
		projector->project(minProjectedSize, positions, projectedPositions);
	}
	// Project vertices separately based on indices.
	else
	{
		if(indexBuffer->indexType() == IndexType_U16)
		{
			const ushort* const indices = indexBuffer->indices16.constData();
			const int indexCount        = indexBuffer->indices16.size();
			for(int i = 0; i < indexCount; ++i)
			{
				const ushort index = indices[i];
				projector->project(positions[index], projectedPositions[index]);
			}
		}
		else if(indexBuffer->indexType() == IndexType_U32)
		{
			const uint* const indices = indexBuffer->indices32.constData();
			const int indexCount      = indexBuffer->indices32.size();
			for(int i = 0; i < indexCount; ++i)
			{
				const uint index = indices[i];
				projector->project(positions[index], projectedPositions[index]);
			}
		}
		else
		{
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
		}
	}
}
