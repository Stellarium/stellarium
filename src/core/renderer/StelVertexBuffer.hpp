#ifndef _STELVERTEXBUFFER_HPP_
#define _STELVERTEXBUFFER_HPP_

#include "core/VecMath.hpp"


//! Vertex attribute types. 
//!
//! Used to specify data type of a vertex attribute in a vertex struct used with VertexBuffer.
enum AttributeType
{
	//! Represents Vec4f (4D float vector).
	AT_Vec4f,
	//! Represents Vec3f (3D float vector).
	AT_Vec3f,
	//! Represents Vec2f (2D float vector).
	AT_Vec2f
};

//! Get size of a vertex attribute of specified type.
//!
//! @param type Attribute type to get size of (e.g. Vec2f).
//! @return Size of the vertex attribute in bytes.
static size_t attributeSize(const AttributeType type)
{
	switch(type)
	{
		case AT_Vec2f: return sizeof(Vec2f);
		case AT_Vec3f: return sizeof(Vec3f);
		case AT_Vec4f: return sizeof(Vec4f);
	}
	Q_ASSERT_X(false, "Unknown vertex attribute type",
				"StelQGLVertexBuffer::attributeSize");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

//! Vertex attribute interpretations. 
//!
//! Used to specify how Renderer should interpret a vertex attribute
//! in a vertex struct used with VertexBuffer.
enum AttributeInterpretation
{
	//! Vertex position.
	Position,
	//! Color of the vertex.
	Color,
	//! Normal of the vertex.
	Normal,
	//! Texture coordinate of the vertex.
	TexCoord
};

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
//! Each StelRenderer backend might have its own vertex buffer, created by its
//! createVertexBuffer method.
//! 
//! StelVertexBuffer supports basic operations such as adding, reading and modifying vertices.
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
//! 	Vec2f vertex;
//! 	Vec2f texCoord;
//! 	Vec4f color;
//! 	
//! 	// Number of vertex attributes. Must match the number of vertex data members.
//! 	static const uint attributeCount = 3;
//! 	
//! 	// Specifies type of each attribute. Length of this array must be attributeCount.
//! 	// Attribute type specifies if it's e.g. a Vec2f or Vec4f.
//! 	// Array contents are specified below the class .
//! 	// (C++ doesn't allow static array definitions inside the class)
//! 	static const AttributeType attributeType[attributeCount];
//! 	
//! 	// Specifies interpretation of each attribute. Length of this array must be attributeCount.
//! 	// Interpretation specifies how the Renderer should interpret this attribute
//! 	// (normal, texcoord, vertex, etc.)
//! 	// 2 attributes must never have the same interpretation.
//! 	// Array contents are specified below the class .
//! 	static const AttributeInterpretation attributeInterpretation[attributeCount];
//! };
//!
//! // Contents of attributeType and attributeInterpretation
//! const AttributeType MyVertex::attributeType[MyVertex::attributeCount] = {V2f, V2f, V4f};
//! const AttributeInterpretation MyVertex::attributeInterpretation[MyVertex::attributeCount] =
//! 	{Vertex, TexCoord, Color};
//! @endcode
//!
//! @see AttributeType, AttributeInterpretation, StelRenderer
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
template<class V> class StelVertexBuffer
{
public:
	StelVertexBuffer()
		: locked_(false)
		, vertexCount(0)
	{
		validateVertexType();
	}
	
	//! Required to ensure that derived classes get properly deallocated.
	virtual ~StelVertexBuffer(){};
	
	//! Add a new vertex to the end of the buffer.
	//!
	//! The buffer must not be locked.
	//!
	//! @param vertex Vertex to add.
	void addVertex(const V& vertex)
	{
		Q_ASSERT_X(!locked_, "Trying to add a vertex to a locked vertex buffer",
		           "StelVertexBuffer::addVertex");
		addVertex_(vertex);
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
		return getVertex_(index);
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
		setVertex_(index, vertex);
	}
	
	//! Lock the buffer. Must be called before drawing.
	void lock() 
	{
		locked_ = true;
		lock_();
	}
	
	//! Unlock the buffer. Must be called to modify the buffer after drawing.
	void unlock()
	{
		unlock_();
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
	
protected:
	//! Called when the buffer is locked. This can be used e.g. to upload vertex data to the GPU.
	virtual void lock_(){}
	
	//! Called when the buffer is unlocked. This can be used e.g. to map a GPU vertex buffer to RAM.
	virtual void unlock_(){}
	
	//! Add a vertex to the buffer. The buffer is guaranteed to be unlocked at call.
	virtual void addVertex_(const V& vertex) = 0;
	
	//! Return a vertex from the buffer. The buffer is guaranteed to be unlocked at call.
	virtual V getVertex_(const uint index) = 0;
	
	//! Set a vertex in the buffer. The buffer is guaranteed to be unlocked at call.
	virtual void setVertex_(const uint index, const V& vertex) = 0;
	
private:
	//! Is the buffer locked (i.e. ready to draw, possibly in GPU memory) ?
	bool locked_;
	
	//! Number of vertices in the buffer.
	uint vertexCount;
	
	//! Assert that the user-specified vertex type is valid.
	//!
	//! For instance, assert that there is no more than 1 attribute with
	//! a particular interpretation (e.g. color, normal),
	//! and that total size of attributes in bytes matches size
	//! of the vertex.
	void validateVertexType()
	{
		// We have no way of looking at each data member of the vertex type, 
		// but we can at least enforce that their total length matches.
		uint bytes = 0;
		
		for(uint attrib = 0; attrib < V::attributeCount; ++attrib)
		{
			bytes += attributeSize(V::attributeType[attrib]);
		}
		
		Q_ASSERT_X(bytes == sizeof(V), 
		           "Size of the vertex type in bytes doesn't match the sum of sizes of "
		           "all vertex attributes as reported by attributeType() method.",
		           "StelVertexBuffer::validateVertexType");
		
		
		bool vertex, texCoord, normal, color;
		vertex = texCoord = normal = color = false;
		
		// Ensure that every kind of vertex attribute is present at most once.
		for(uint attrib = 0; attrib < V::attributeCount; ++attrib)
		{
			switch(V::attributeInterpretation[attrib])
			{
				case Position:
					Q_ASSERT_X(!vertex, 
					           "Vertex type has more than one vertex position attribute",
					           "StelVertexBuffer::validateVertexType");
					vertex = true;
					break;
				case TexCoord:
					Q_ASSERT_X(!texCoord, 
					           "Vertex type has more than one texture coordinate attribute",
					           "StelVertexBuffer::validateVertexType");
					texCoord = true;
					break;
				case Normal:
					Q_ASSERT_X(!normal, 
					           "Vertex type has more than one normal attribute",
					           "StelVertexBuffer::validateVertexType");
					normal = true;
					break;
				case Color:
					Q_ASSERT_X(!color, 
					           "Vertex type has more than one color attribute",
					           "StelVertexBuffer::validateVertexType");
					color = true;
					break;
				default:
					Q_ASSERT_X(false, "Unknown vertex attribute interpretation",
					           "StelVertexBuffer::validateVertexType");
			}
		}
	}
};


#endif // _STELVERTEXBUFFER_HPP_