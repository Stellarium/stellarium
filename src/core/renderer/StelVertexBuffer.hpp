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

#ifndef _STELVERTEXBUFFER_HPP_
#define _STELVERTEXBUFFER_HPP_

#include "core/VecMath.hpp"
#include "StelVertexAttribute.hpp"
#include "StelVertexBufferBackend.hpp"


//! Graphics primitive types. 
//!
//! All graphics is composed from these primitives, which in turn are composed of vertices.
enum PrimitiveType
{
	//! Each vertex is a separate point.
	PrimitiveType_Points,

	//! Every 3 vertices form a triangle.
	PrimitiveType_Triangles,

	//! The first 3 vertices form a triangle and each vertex after that forms a triangle with
	//! previous 2 vertices.
	PrimitiveType_TriangleStrip,

	//! The first vertex forms a triangle with the second and third, another with 
	//! the third and fourth, and so on.
	PrimitiveType_TriangleFan,

	//! Every 2 vertices form a line.
	PrimitiveType_Lines, 

	//! Vertices form a strip of lines from the first to the last.
	PrimitiveType_LineStrip,

	//! Like LineStrip, but the last vertex also forms a line with the first.
	PrimitiveType_LineLoop,
};

//! Base class for all vertex buffers.
//!
//! Allows to store a pointer that might point to any vertex buffer.
class StelAbstractVertexBuffer
{
public:
	virtual ~StelAbstractVertexBuffer(){};
};

//! Vertex buffer interface.
//!
//! Each StelRenderer backend might have a different vertex buffer buffer implementation.
//! StelVertexBuffer, created by the StelRenderer's <em>createVertexBuffer()</em> function
//! wraps this backend.
//! 
//! StelVertexBuffer supports basic operations such as adding, getting and modifying vertices.
//! It can be locked to allow uploading the data to the GPU, and unlocked
//! to modify the buffer.
//!
//! For drawing (by a StelRenderer), the vertex buffer must be locked. 
//! To access/modify vertices, it must be unlocked.
//! A newly created vertex buffer is always unlocked.
//!
//! @tparam V Vertex type. The addVertex(), getVertex() and setVertex() functions work with this type.
//!           This allows the API to be minimal and safe at the same time
//!           (no way to mess up vertex format).
//! 
//! The vertex type is defined by the user. 
//! Some commonly used vertex types are in the file <em>GenericVertexTypes.hpp</em>.
//!
//! Example vertex type:
//!
//! @code
//! struct MyVertex
//! {
//! 	// Vertex data members (attributes). 
//! 	Vec3f position;
//! 	Vec2f texCoord;
//! 	
//! 	// This macro specifies metadata (data type and interpretation)
//! 	// of each attribute.  Data type can be Vec2f, Vec3f or Vec4f.
//! 	// Interpretation can be Position, TexCoord, Normal or Color.
//! 	// Two attributes must never have the same interpretation
//! 	// (this is asserted by the vertex buffer backend at run time).
//! 	//
//! 	// Note:
//! 	// Attribute interpretations are processed at runtime, so if there 
//! 	// is an error here, Stellarium will crash with an assertion failure and an 
//! 	// error message.
//! 	//
//! 	// (C++11 TODO) This might be changed with constexpr
//! 	VERTEX_ATTRIBUTES(Vec3f Position, Vec2f TexCoord);
//! };
//! @endcode
//!
//! @note There are some requirements for a vertex type. 
//! These are verified at run time.
//! The requirements are:
//!
//! @li A position attribute is required.
//! @li Any texture coordinates must be 2D. There is no 1D or 3D texture support right now.
//! @li Any normals must be 3D.
//!
//! @note When drawing with a custom StelProjector (using StelRenderer::drawVertexBuffer), 
//! only 3D vertex positions are supported.
//!
//! @note Currently, the GL2 backend only supports some combinations of attribute
//!       interpretations (e.g. just position, position-color, etc.). This is because
//!       every such combination needs a custom shader. If you get an assertion error 
//!       about an unimplemented shader for vertex format, feel free to add it
//!       (in StelQGL2Renderer). All vertex formats currently used in Stellarium 
//!       are already covered.
//!
//! @see AttributeType, AttributeInterpretation, StelVertexAttribute, StelRenderer
//!
//! @section apinotes API design notes
//!
//! Currently, vertices must be accessed individually through functions that might
//! have considerable overhead (at least due to indirect call through the virtual
//! function table).
//!
//! If vertex processing turns out to be too slow, the solution is not to allow direct
//! access to data through the pointer, as that ties us to a particular implementation
//! (array in memory) and might be completely unusable with e.g. VBO based backends.
//!
//! Rather, considerable speedup could be achieved by adding member functions to 
//! add, get and set ranges of vertices.
//!
//! E.g.: 
//!
//! @li void addVertexRange(const QVector<V>& vertices)
//! @li void getVertexRange(QVector<V>& outVertices, uint start, uint end)
//! @li void setVertexRange(const QVector<V> vertices, uint start, uint end)
//!
//! Note that end might be unnecessary in setVertexRange.
//!
//! This is still not the same speedup as direct access (due to copying in get/set), but
//! should allow for considerably faster implementations than accessing vertices individually,
//! without forcing backends to store vertices in a particular way.
//!
//! An alternative, possibly better (especially with C++11) option for modifying vertices
//! would be to use a function pointer / function object to process each vertex.
//!
//! @subsection apinotes_backend Vertex buffer backend
//!
//! VertexBuffer is currently separated into frontend (StelVertexBuffer), 
//! which allows type-safe vertex buffer construction thanks to templates,
//! and backend, which doesn't know the vertex type and works on raw 
//! data described by metadata generated by the VERTEX_ATTRIBUTES macro in 
//! the vertex type.
//! 
//! This is because virtual methods can't be templated.
//! There might be a workaround for this, but I'm not aware of any
//! at the moment. 
template<class V> class StelVertexBuffer : public StelAbstractVertexBuffer
{
//! Only StelRenderer can construct a vertex buffer, and we also need unittesting.
friend class StelRenderer;
friend class TestStelVertexBuffer;

public:
	//! Destroy the vertex buffer. StelVertexBuffer is deleted by the user, not StelRenderer.
	~StelVertexBuffer()
	{
		delete backend;
	}
	
	//! Add a new vertex to the end of the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param vertex Vertex to add.
	void addVertex(const V& vertex)
	{
		Q_ASSERT_X(!locked_, Q_FUNC_INFO,
		           "Trying to add a vertex to a locked vertex buffer");
		backend->addVertex(static_cast<const void*>(&vertex));
		++vertexCount;
	}
	
	//! Return vertex at specified index in the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param index Index of the vertex to get.
	//! @return Vertex at specified index.
	V getVertex(const int index) const
	{
		Q_ASSERT_X(!locked_, Q_FUNC_INFO,
		           "Trying to get a vertex in a locked vertex buffer");
		Q_ASSERT_X(index < vertexCount, Q_FUNC_INFO, "Vertex index out of bounds");
		// Using a char array instead of a V directly avoids calling the 
		// default constructor of V (which might not be defined, and the 
		// default-constructed data would be overwritten anyway)
		
		unsigned char storage[sizeof(V)];
		void* result = &storage[0];
		backend->getVertex(index, result);
		return *static_cast<V*>(result);
	}
	
	//! Set vertex at specified index in the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param index Index of the vertex to set.
	//! @param vertex Value to set the vertex to.
	void setVertex(const int index, const V& vertex)
	{
		Q_ASSERT_X(!locked_, Q_FUNC_INFO,
		           "Trying to set a vertex in a locked vertex buffer");
		Q_ASSERT_X(index < vertexCount, Q_FUNC_INFO, "Vertex index out of bounds");
		backend->setVertex(index, static_cast<const void*>(&vertex));
	}
	
	//! Lock the buffer. Must be called before drawing.
	void lock() 
	{
		locked_ = true;
		backend->lock();
	}
	
	//! Unlock the buffer. This is needed to modify the buffer after drawing.
	void unlock()
	{
		backend->unlock();
		locked_ = false;
	}
	
	//! Is this buffer locked?
	bool locked() const
	{
		return locked_;
	}
	
	//! Returns the number of vertices in the buffer.
	int length() const
	{
		return vertexCount;
	}

	//! Return the type of graphics primitives drawn with this vertex buffer.
	PrimitiveType primitiveType() const 
	{
		return primitiveType_;
	}
	
	//! Clear the buffer, removing all vertices.
	//!
	//! The buffer must not be locked.
	//!
	//! The backend implementation might reuse previously allocated storage after 
	//! clearing, so calling clear() might be more efficient than destroying 
	//! a buffer and then constructing a new one.
	virtual void clear()
	{
		Q_ASSERT_X(!locked_, Q_FUNC_INFO, "Trying to clear a locked vertex buffer");
		vertexCount = 0;
		backend->clear();
	}

private:
	//! Construct a StelVertexBuffer wrapping specified backend with specified primitive type.
	//!
	//! Only Renderer can call this.
	StelVertexBuffer(StelVertexBufferBackend* backend, const PrimitiveType primitiveType)
		: locked_(false)
		, vertexCount(0)
		, primitiveType_(primitiveType)
		, backend(backend)
	{
		backend->validateVertexType(sizeof(V));
	}
	
	//! Is the buffer locked (i.e. ready to draw, possibly in GPU memory) ?
	bool locked_;
	
	//! Number of vertices in the buffer.
	int vertexCount;

	//! Type of graphics primitives drawn with this vertex buffer.
	PrimitiveType primitiveType_;
	
	//! Vertex buffer backend.
	StelVertexBufferBackend* backend;
};

#endif // _STELVERTEXBUFFER_HPP_
