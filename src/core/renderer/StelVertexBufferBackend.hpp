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

#ifndef _STELVERTEXBUFFERBACKEND_HPP_
#define _STELVERTEXBUFFERBACKEND_HPP_

#include <QVector>

#include "StelVertexAttribute.hpp"


//! Maximum number of vertex attributes.
//!
//! Must be greater than the number of enum values in AttributeInterpretation.
static const int MAX_VERTEX_ATTRIBUTES = 8;

//! Base class of all vertex buffer backends.
//!
//! This is where the vertex buffer logic is implemented.
//!
//! The backend doesn't directly know the vertex type (since templating doesn't mix
//! with virtual inheritance), so it works with raw (void) pointers.
//!
//! Vertex format is described by the attributes data member.
//! Each item in attributes specifies a vertex attribute (such as a position or normal),
//! and these correspond to the data members of the user-specified vertex struct which is
//! the template parameter of the StelVertexBuffer class.
//!
//! StelVertexBufferBackend is always owned by a StelVertexBuffer and can't exist separately.
//! Only the StelRenderer backend has direct access to StelVertexBufferBackend's implementation.
//!
//! When the user asks StelRenderer to create a vertex buffer, the renderer backend first creates
//! its own implementation of the vertex buffer backend, and then wraps it up in a
//! StelVertexBuffer.
//!
//! @sa StelVertexBuffer, StelRenderer
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
class StelVertexBufferBackend
{
	//! Stores vertex attribute metadata of a buffer.
	//!
	//! QVector<StelVertexAttribute> was used previously. This is an optimization
	//! to speed up vertex manipulation.
	struct Attributes
	{
		//! Attribute definitions.
		StelVertexAttribute attributes[MAX_VERTEX_ATTRIBUTES];
		//! Size of each attribute in bytes.
		int sizes[MAX_VERTEX_ATTRIBUTES];
		//! Number of attributes.
		int count;

		//! Construct from a vertex attribute vector.
		Attributes(const QVector<StelVertexAttribute>& attributeVector)
			: count(attributeVector.size())
		{
			for(int a = 0; a < count; ++a)
			{
				Q_ASSERT_X(count < MAX_VERTEX_ATTRIBUTES, Q_FUNC_INFO,
				           "Too many vertex attributes (increase MAX_VERTEX_ATTRIBUTES"
				           "in StelVertexBufferBackend.hpp)");
				attributes[a] = attributeVector[a];
				sizes[a]      = attributeSize(attributes[a].type);
			}
		}
	};

public:
	//! Construct a StelVertexBufferBackend, specifying attributes of the vertex type.
	StelVertexBufferBackend(const QVector<StelVertexAttribute>& attributes)
		: attributes(attributes)
	{
	}

	//! Required to ensure that derived classes get properly deallocated.
	virtual ~StelVertexBufferBackend(){};
	
	//! Add a new vertex to the buffer.
	//!
	//! StelVertexBuffer guarantees that when this is called,
	//! buffer is not locked and index is in range.
	//!
	//! @param vertexInPtr Pointer to the beginning of the vertex.
	//!                    Data members of the vertex must match vertex attributes of the buffer.
	virtual void addVertex(const void* const vertexInPtr) = 0;
	
	//! Get a vertex from the buffer.
	//!
	//! StelVertexBuffer guarantees that when this is called,
	//! buffer is not locked and index is in range.
	//!
	//! @param index        Index of the vertex to read.
	//! @param vertexOutPtr Pointer to the beginning of the output vertex.
	//!                     Data members of the vertex must match vertex attributes of the buffer.
	virtual void getVertex(const int index, void* const vertexOutPtr) const = 0;
	
	//! Rewrite a vertex in the buffer.
	//!
	//! StelVertexBuffer guarantees that when this is called,
	//! buffer is not locked and index is in range.
	//!
	//! @param index       Index of the vertex to set.
	//! @param vertexInPtr Pointer to the beginning of the vertex that will rewrite the vertex in
	//!                    the buffer. Data members of the vertex must match vertex attributes of
	//!                    the buffer.
	virtual void setVertex(const int index, const void* const vertexInPtr) = 0;
	
	//! Lock the buffer. Must be called before drawing.
	virtual void lock() = 0;
	
	//! Unlock the buffer. Must be called to modify the buffer after drawing.
	virtual void unlock() = 0;
	
	//! Clear the buffer, removing all vertices.
	//!
	//! The backend implementation might reuse previously allocated storage after 
	//! clearing, so calling clear() might be more efficient than destroying 
	//! a buffer and then constructing a new one.
	virtual void clear() = 0;

	//! Assert that the user-specified (in StelVertexBuffer) vertex type is valid.
	void validateVertexType(const int vertexSize)
	{
#ifdef NDEBUG
		Q_UNUSED(vertexSize);
#endif
		// We have no way of looking at each data member of the vertex type, 
		// but we can at least enforce that their total length matches.
		int bytes = 0;
		
		for(int attrib = 0; attrib < attributes.count; ++attrib)
		{
			bytes += attributes.sizes[attrib];
		}
		
		Q_ASSERT_X(bytes == vertexSize, Q_FUNC_INFO,
		           "Size of the vertex type in bytes doesn't match the sum of sizes of "
		           "all vertex attributes as reported by \"attributes\" data member.");
	}
	
protected:
	//! Specifies layout of vertex attributes in the vertex type.
	const Attributes attributes;
};


#endif // _STELVERTEXBUFFERBACKEND_HPP_
