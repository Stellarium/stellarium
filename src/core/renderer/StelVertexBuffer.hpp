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

	//! The first 3 vertices form a triangle and each vertex after that forms a triangle with its
	//! previous 2 vertices.
	PrimitiveType_TriangleStrip,

	//! The first vertex forms a triangle with the second and third, another with 
	//! the third and fourth, and so on.
	PrimitiveType_TriangleFan,

	//! Every 2 vertices form a line.
	PrimitiveType_Lines, 

	//! Vertices form lines from the first to the second, second to third, and so on.
	//! The last vertex forms a line with the first, closing the loop.
	PrimitiveType_LineLoop,
};

//! Base class for all vertex buffers.
//!
//! Used to store a pointer that might point to any vertex buffer.
class StelAbstractVertexBuffer
{
public:
	virtual ~StelAbstractVertexBuffer(){};
};

//! Vertex buffer interface.
//!
//! Each StelRenderer backend might have its own vertex buffer backend.
//! StelVertexBuffer is created through StelRenderer's createVertexBuffer function
//! and wraps this backend.
//! 
//! StelVertexBuffer supports basic operations such as adding, reading and modifying vertices.
//! 
//! It can be locked to allow uploading the data to the GPU, or unlocked
//! to manipulate the data.
//!
//! To be drawn (by a StelRenderer), the vertex buffer must be locked. 
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
//! 	// This macro specifies metadata (data type and interpretation)
//! 	// of each attribute.  Data type can be Vec2f, Vec3f or Vec4f.
//! 	// Interpretation can be Position, TexCoord, Normal or Color.
//! 	// Two attributes must never have the same interpretation
//! 	// (this is asserted by the vertex buffer backend at run time).
//!   //
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
//! Note that there are some requirements for a vertex type. 
//! These are verified at run time.
//! The requirements are:
//!
//! @li A position attribute is required.
//! @li Any texture coordinates must be 2D. There is no 1D or 3D texture support right now.
//! @li Any normals must be 3D.
//!
//! @note When drawing with a custom StelProjector (Renderer::drawVertexBuffer), 
//! only 3D vertex positions are supported.
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
template<class V> class StelVertexBuffer : public StelAbstractVertexBuffer
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
		Q_ASSERT_X(!locked_, Q_FUNC_INFO,
		           "Trying to add a vertex to a locked vertex buffer");
		backend->addVertex(reinterpret_cast<const quint8*>(&vertex));
		++vertexCount;
	}
	
	//! Return vertex at specified index the buffer.
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
		// Using a quint array instead of a V directly avoids calling the 
		// default constructor of V (which might not be defined, and the 
		// default-constructed data would be overwritten anyway)
		quint8 result[sizeof(V)];
		backend->getVertex(index, result);
		return *(reinterpret_cast<V*>(result));
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
	int length() const
	{
		return vertexCount;
	}
	
	//! Clear the buffer, removing all vertices.
	//!
	//! Can be only called when unlocked.
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
	int vertexCount;
	
	//! Vertex buffer backend.
	StelVertexBufferBackend* backend;
};

#endif // _STELVERTEXBUFFER_HPP_
