#ifndef _STELVERTEXBUFFER_HPP_
#define _STELVERTEXBUFFER_HPP_

#include "core/VecMath.hpp"
#include "StelVertexBufferBackend.hpp"
#include "StelVertexAttribute.hpp"


//! Graphics primitive types. 
//!
//! All graphics is composed from these primitives, which in turn are composed of vertices.
enum PrimitiveType
{
	//! Each vertex is a separate point.
	Points,
	//! Every 3 vertices form a triangle.
	Triangles,
	//! The first 3 vertices form a triangle and each vertex after that forms a triangle with its
	//! previous 2 vertices.
	TriangleStrip
};

//! Vertex buffer interface.
//!
//! Each StelRenderer backend might have its own vertex buffer backend.
//! StelVertexBuffer is created through StelRenderer's createVertexBuffer function
//! and wraps this backend.
//! 
//! StelVertexBuffer supports basic operations such as adding, reading and modifying vertices.
//! 
//! It can also be locked to allow uploading the data to the GPU, or unlocked
//! to allow manipulation of the data.
//!
//! In order to be drawn (by a Renderer), the vertex buffer has to be locked. 
//! To access/modify vertices, it must be unlocked.
//! A newly created vertex buffer is always unlocked.
//!
//! @tparam V Vertex type. Each vertex access function works with this type.
//!           This allows the API to be minimal and safe at the same time
//!           (no way to mess up vertex format).
//! 
//! The vertex type has to be defined by user.
//!
//! Example:
//!
//! @code
//! struct MyVertex
//! {
//! 	// Vertex data members (attributes). 
//! 	Vec3f position;
//! 	Vec2f texCoord;
//! 	
//! 	// Specifies data type and interpretation of each attribute.
//! 	// Data type can be AT_Vec2f, AT_Vec3f or AT_Vec4f.
//! 	// Interpretation can be Position, TexCoord, Normal or Color.
//! 	// Two attributes must never have the same interpretation
//! 	// (this is asserted by the vertex buffer backend at run time).
//! 	//
//! 	// The vector contents are defined below the class 
//! 	// (C++ doesn't allow non-integer static const data members).
//! 	static const QVector<StelVertexAttribute> attributes;
//! };
//!
//! //This might need to be defined in a .cpp file to prevent multiple definition errors
//! const QVector<StelVertexAttribute> MyVertex::attributes = 
//! 	(QVector<StelVertexAttribute>() << StelVertexAttribute(AT_Vec3f, Position)
//! 	                                << StelVertexAttribute(AT_Vec2f, TexCoord));
//! @endcode
//!
//! @see AttributeType, AttributeInterpretation, StelVertexAttribute, StelRenderer
//!
//! @section apinotes API design notes
//!
//! Currently, vertices must be accessed individually through functions that might
//! have considerable overhead (at least due to indirect call through the vtable).
//!
//! If vertex processing turns out to be too slow, the solution is not to allow direct
//! access to data through the pointer, as that ties us to a particular implementation
//! (array in memory) and might be completely unusable with e.g. QGL based backends.
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
//! and backend, which doesn't know vertex type directly and works on raw 
//! data described by attributes static data member of the vertex struct.
//! 
//! This is because a virtual method can't be templated at the same time.
//! There might be a possible workaround around this, but I'm not aware of that
//! at the moment. C++11 might make it possible for the backend to at least
//! re-create Vertex struct as a tuple in its functions using variadic recursive 
//! templates, but that might be too complex/hacky to be worth the benefit.
template<class V> class StelVertexBuffer
{
//! Only Renderer can construct a vertex buffer, and we also need unittesting.
friend class StelRenderer;
friend class TestStelVertexBuffer;

public:
	//! Destroy the vertex buffer. StelVertexBuffer must be destroyed by the user, not Renderer.
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
		Q_ASSERT_X(!locked_, "Trying to add a vertex to a locked vertex buffer",
		           "StelVertexBuffer::addVertex");
		backend->addVertex(reinterpret_cast<const quint8*>(&vertex));
		++vertexCount;
	}
	
	//! Return a vertex at specified index the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param index Index of the vertex to get.
	//! @return Vertex at specified index.
	V getVertex(const uint index)
	{
		Q_ASSERT_X(!locked_, "Trying to get a vertex in a locked vertex buffer",
		           "StelVertexBuffer::getVertex");
		Q_ASSERT_X(index < vertexCount, "Vertex index out of bounds",
		           "StelVertexBuffer::getVertex");
		V result;
		backend->getVertex(index, reinterpret_cast<quint8*>(&result));
		return result;
	}
	
	//! Set a vertex at specified index in the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param index Index of the vertex to set.
	//! @param vertex Value to set the vertex to.
	void setVertex(const uint index, const V& vertex)
	{
		Q_ASSERT_X(!locked_, "Trying to set a vertex in a locked vertex buffer",
		           "StelVertexBuffer::setVertex");
		Q_ASSERT_X(index < vertexCount, "Vertex index out of bounds",
		           "StelVertexBuffer::setVertex");
		backend->setVertex(index, reinterpret_cast<const quint8*>(&vertex));
	}
	
	//! Lock the buffer. Must be called before drawing.
	void lock() 
	{
		locked_ = true;
		backend->lock();
	}
	
	//! Unlock the buffer. Must be called to modify the buffer after drawing.
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
	uint length() const
	{
		return vertexCount;
	}
	
private:
	//! Construct a StelVertexBuffer wrapping specified backend. Only Renderer can do that.
	StelVertexBuffer(StelVertexBufferBackend* backend)
		: locked_(false)
		, vertexCount(0)
		, backend(backend)
	{
		backend->validateVertexType(sizeof(V));
	}
	
	//! Is the buffer locked (i.e. ready to draw, possibly in GPU memory) ?
	bool locked_;
	
	//! Number of vertices in the buffer.
	uint vertexCount;
	
	//! Vertex buffer backend.
	StelVertexBufferBackend* backend;
};

#endif // _STELVERTEXBUFFER_HPP_
