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
	, vertexCapacity(0)
	, usingProjectedPositions(false)
{
	// Create a buffer for each vertex attribute.
	for(int attrib = 0; attrib < this->attributes.count; ++attrib)
	{
		const AttributeInterpretation interpretation = 
			this->attributes.attributes[attrib].interpretation;

		AnyAttributeArray* buffer;

		switch(this->attributes.attributes[attrib].type)
		{
			case AttributeType_Vec2f: buffer = new AttributeArray<Vec2f>(interpretation); break;
			case AttributeType_Vec3f: buffer = new AttributeArray<Vec3f>(interpretation); break;
			case AttributeType_Vec4f: buffer = new AttributeArray<Vec4f>(interpretation); break;
			default:  Q_ASSERT(false);
		}

		buffers.append(buffer);
	}
}

StelQGLArrayVertexBufferBackend::~StelQGLArrayVertexBufferBackend()
{
	Q_ASSERT_X(buffers.size() == static_cast<int>(attributes.count),
	           Q_FUNC_INFO, "Attribute buffer count does not match attribute count");

	for(int buffer = 0; buffer < buffers.size(); ++buffer)
	{
		delete buffers[buffer];
	}
}

void StelQGLArrayVertexBufferBackend::addVertex(const void* const vertexInPtr)
{
	//StelVertexBuffer enforces bounds, so we don't need to
	++vertexCount;
	if(vertexCount <= vertexCapacity)
	{
		setVertexNonVirtual(vertexCount - 1, vertexInPtr);
		return;
	}
	++vertexCapacity;
	// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
	const unsigned char* attribPtr = static_cast<const unsigned char*>(vertexInPtr);
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		// Add each attribute to its buffer.
		switch(attributes.attributes[attrib].type)
		{
			case AttributeType_Vec2f: addAttribute<Vec2f>(attrib, attribPtr); break;
			case AttributeType_Vec3f: addAttribute<Vec3f>(attrib, attribPtr); break;
			case AttributeType_Vec4f: addAttribute<Vec4f>(attrib, attribPtr); break;
			default: Q_ASSERT(false);
		}

		// This always works, because the C standard requires that 
		// sizeof(unsigned char) == 1  (that 1 might mean e.g. 16 bits instead of 8
		// on some platforms, but both the size of attribute and of unsigned char is 
		// measured in the same unit, so it's not a problem.
		attribPtr += attributes.sizes[attrib];
	}
}

void StelQGLArrayVertexBufferBackend::getVertex
	(const int index, void* const vertexOutPtr) const
{
	// Points to the current attribute (e.g. color, normal, vertex) within output.
	unsigned char* attribPtr = static_cast<unsigned char*>(vertexOutPtr);
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		// Get each attribute from its buffer and set result's attribute to that.
		switch(attributes.attributes[attrib].type)
		{
			case AttributeType_Vec2f:
				*reinterpret_cast<Vec2f*>(attribPtr) = getAttributeConst<Vec2f>(attrib, index);
				break;
			case AttributeType_Vec3f:
				*reinterpret_cast<Vec3f*>(attribPtr) = getAttributeConst<Vec3f>(attrib, index);
				break;
			case AttributeType_Vec4f:
				*reinterpret_cast<Vec4f*>(attribPtr) = getAttributeConst<Vec4f>(attrib, index);
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

void StelQGLArrayVertexBufferBackend::
	projectVertices(StelProjector* projector, StelQGLIndexBuffer* indexBuffer)
{
	// This is a backend function called during (right before) drawing, so we need 
	// to be locked.
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to project a vertex buffer that is not locked.");

	// Get the position attribute and ensure that we can project it.
	const int posIndex = getAttributeIndex(AttributeInterpretation_Position);
#ifndef NDEBUG
	const StelVertexAttribute& attribute(attributes.attributes[posIndex]);
#endif
	Q_ASSERT_X(posIndex >= 0, Q_FUNC_INFO, "Vertex format without a position attribute");
	Q_ASSERT_X(attributeDimensions(attribute.type) == 3, Q_FUNC_INFO,
	           "Trying to use a custom StelProjector to project non-3D vertex positions");
	Q_ASSERT_X(attribute.type == AttributeType_Vec3f, Q_FUNC_INFO, 
	           "Unknown 3D vertex attribute type");
	AttributeArray<Vec3f>* positions = 
		dynamic_cast<AttributeArray<Vec3f>*>(buffers[posIndex]);
	Q_ASSERT_X(NULL != positions, Q_FUNC_INFO,
	           "Unexpected attribute buffer data type");

	usingProjectedPositions = true;


	// We have two different cases :
	// a) Not using an index array. Size of vertex data is known.
	// b) Using an index array. 
	//    We find the max index and project vertices until that index, 
	//    or if there are not many indices, project vertices separately 
	//    based on indices.
	const int minProjectedSize =
		(NULL == indexBuffer) ? vertexCount : (indexBuffer->maxIndex() + 1);
	if(projectedPositions.size() < minProjectedSize)
	{
		projectedPositions.resize(minProjectedSize);
	}

	// If the index buffer is big, it's likely that it covers most 
	// of the vertex buffer so we can just project everything en masse, 
	// taking advantage of the cache.
	if(NULL == indexBuffer || indexBuffer->length() >= minProjectedSize)
	{
		projector->project(minProjectedSize, positions->data.constData(), 
		                   projectedPositions.data());
	}
	// Project vertices separately based on indices.
	else
	{
		const Vec3f* const pos = positions->data.constData();
		Vec3f* const projected = projectedPositions.data();
		if(indexBuffer->indexType() == IndexType_U16)
		{
			const ushort* const indices = indexBuffer->indices16.constData();
			const int indexCount        = indexBuffer->indices16.size();
			for(int i = 0; i < indexCount; ++i)
			{
				const ushort index = indices[i];
				projector->project(pos[index], projected[index]);
			}
		}
		else if(indexBuffer->indexType() == IndexType_U32)
		{
			const uint* const indices = indexBuffer->indices32.constData();
			const int indexCount      = indexBuffer->indices32.size();
			for(int i = 0; i < indexCount; ++i)
			{
				const uint index = indices[i];
				projector->project(pos[index], projected[index]);
			}
		}
		else
		{
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
		}
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
