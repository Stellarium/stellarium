#ifndef _STELTESTGLVERTEXBUFFERBACKEND_HPP_
#define _STELTESTGLVERTEXBUFFERBACKEND_HPP_

#include <QVector>

#include "core/VecMath.hpp"
#include "StelVertexBufferBackend.hpp"
#include "VertexAttribute.hpp"


//! OpenGL vertex array style VertexBuffer backend, used for testing and transition.
//!
//! Should be replaced by a StelQGLVertexBufferBackend based on QGL.
//!
//! @sa StelVertexBuffer, StelRenderer
class StelTestGLVertexBufferBackend : public StelVertexBufferBackend
{
//! Only GLRenderer can construct this backend, and we also need unittesting.
friend class StelGLRenderer;
friend class TestStelVertexBuffer;
	//! Buffer containing a single attribute (e.g. color, normal, vertex).
	//!
	//! Must be downcasted to AttributeBuffer to access the stored data.
	struct AnyAttributeBuffer
	{
		//! Required to ensure that derived classes get properly deallocated.
		virtual ~AnyAttributeBuffer(){};
	};

	//! Concrete attribute buffer storing attributes of a particular type.
	//!
	//! Handles the GL logic required to provide the buffer to the GPU.
	template<class A> struct AttributeBuffer : public AnyAttributeBuffer
	{
		//! Construct an AttributeBuffer.
		//!
		//! @param interpretation How should GL interpret this attribute? (e.g. color, normal, etc).
		AttributeBuffer(const AttributeInterpretation interpretation)
			:interpretation(interpretation)
		{
		}
		
		//! Stores the attribute data (in GL terms, a vertex array).
		QVector<A> data;
		
		//! Specifies how should the attribute be interpreted (color, normal, etc.).
		const AttributeInterpretation interpretation;
	};
	
public:
	virtual ~StelTestGLVertexBufferBackend()
	{
		Q_ASSERT_X(buffers.size() == static_cast<int>(attributes.size()),
		           "Attribute buffer count does not match attribute count",
		           "StelTestGLVertexBufferBackend::~StelTestGLVertexBufferBackend");
		
		for(int buffer = 0; buffer < buffers.size(); ++buffer)
		{
			delete buffers[buffer];
		}
	}
	
	virtual void addVertex(const quint8* const vertexInPtr)
	{
		// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
		const quint8* attribPtr = vertexInPtr;
		for(int attrib = 0; attrib < attributes.size(); ++attrib)
		{
			const AttributeType type = attributes[attrib].type;
			
			// Add each attribute to its buffer.
			switch(type)
			{
				case AT_Vec2f: addAttribute<Vec2f>(attrib, attribPtr); break;
				case AT_Vec3f: addAttribute<Vec3f>(attrib, attribPtr); break;
				case AT_Vec4f: addAttribute<Vec4f>(attrib, attribPtr); break;
				default:  Q_ASSERT(false);
			}
			attribPtr += attributeSize(type);
		}
	}
	
	virtual void getVertex(const uint index, quint8* const vertexOutPtr) 
	{
		// Points to the current attribute (e.g. color, normal, vertex) within output.
		quint8* attribPtr = vertexOutPtr;
		for(int attrib = 0; attrib < attributes.size(); ++attrib)
		{
			const AttributeType type = attributes[attrib].type;
			
			// Get each attribute from its buffer and set result's attribute to that.
			switch(type)
			{
				case AT_Vec2f:
					*reinterpret_cast<Vec2f*>(attribPtr) = getAttribute<Vec2f>(attrib, index);
					break;
				case AT_Vec3f:
					*reinterpret_cast<Vec3f*>(attribPtr) = getAttribute<Vec3f>(attrib, index);
					break;
				case AT_Vec4f:
					*reinterpret_cast<Vec4f*>(attribPtr) = getAttribute<Vec4f>(attrib, index);
					break;
				default:  
					Q_ASSERT(false);
			}
			
			attribPtr += attributeSize(type);
		}
	}
	
	virtual void setVertex(const uint index, const quint8* const vertexInPtr)
	{
		// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
		const quint8* attribPtr = vertexInPtr;
		for(int attrib = 0; attrib < attributes.size(); ++attrib)
		{
			const AttributeType type = attributes[attrib].type;
			
			//Set each attribute in its buffer.
			switch(type)
			{
				case AT_Vec2f:
					getAttribute<Vec2f>(attrib, index) = *reinterpret_cast<const Vec2f*>(attribPtr);
					break;
				case AT_Vec3f:
					getAttribute<Vec3f>(attrib, index) = *reinterpret_cast<const Vec3f*>(attribPtr);
					break;
				case AT_Vec4f:
					getAttribute<Vec4f>(attrib, index) = *reinterpret_cast<const Vec4f*>(attribPtr);
					break;
				default:  
					Q_ASSERT(false);
			}
			
			attribPtr += attributeSize(type);
		}
	}
	
	virtual void lock()
	{
		locked = true;
	}
	
	virtual void unlock()
	{
		locked = false;
	}
	
	//! Called by Renderer to draw the buffer.
	//!
	void draw()
	{
		Q_ASSERT_X(locked, "Trying to draw a vertex buffer that is not locked.",
		           "StelTestGLVertexBufferBackend::draw");
		Q_ASSERT_X(false, "TODO - implement this function", "StelTestGLVertexBufferBackend::draw");
	}
	
private:
	//! Is the vertex buffer locked (i.e. ready to draw?).
	bool locked;
	
	//! Graphics primitive type formad by the vertices of this buffer.
	PrimitiveType primitiveType;
	
	//! Buffers of each vertex attribute.
	QVector<AnyAttributeBuffer*> buffers;
	
	//! Construct a StelTestGLVertexBufferBackend. Only GLRenderer can do this.
	//!
	//! Initializes vertex attribute buffers.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	//! @param attributes Specifications of vertex attributes that will be stored in the buffer.
	StelTestGLVertexBufferBackend(const PrimitiveType type, 
	                              const QVector<VertexAttribute>& attributes)
		: StelVertexBufferBackend(attributes)
		, locked(false)
		, primitiveType(type)
	{
		// Create a buffer for each vertex attribute.
		for(int attrib = 0; attrib < attributes.size(); ++attrib)
		{
			const AttributeType type = attributes[attrib].type;
			const AttributeInterpretation interpretation = attributes[attrib].interpretation;
			
			AnyAttributeBuffer* buffer;
			
			switch(type)
			{
				case AT_Vec2f: buffer = new AttributeBuffer<Vec2f>(interpretation); break;
				case AT_Vec3f: buffer = new AttributeBuffer<Vec3f>(interpretation); break;
				case AT_Vec4f: buffer = new AttributeBuffer<Vec4f>(interpretation); break;
				default:  Q_ASSERT(false);
			}
			
			buffers.append(buffer);
		}
	}
	
	//! Add an attribute to specified attribute buffer.
	//!
	//! @param attributeIndex Specifies which attribute (e.g. normal, texcoord, vertex) we're
	//!                       adding.
	//! @param attributePtr   Raw pointer to attribute data. Data format must match
	//!                       the attribute at specified index.
	template<class A> 
	void addAttribute(const uint attributeIndex, const quint8* attributePtr)
	{
		const A* attrib = reinterpret_cast<const A*>(attributePtr); 
		getBuffer<A>(attributeIndex).data.append(*attrib);
	}
	
	//! Access specified attribute of a vertex.
	//!
	//! @tparam A             Attribute type. Must match the type of attribute at
	//!                       attributeIndex.
	//! @param attributeIndex Specifies which attribute (e.g. normal, texcoord, vertex) we're
	//!                       accessing.
	//! @param vertexIndex    Specifies which vertex we're accessing.
	//! @return               Non-const reference to the attribute.
	template<class A> 
	A& getAttribute(const uint attributeIndex, const uint vertexIndex)
	{
		return getBuffer<A>(attributeIndex).data[vertexIndex];
	}
	
	//! Access buffer of the specified vertex attribute.
	//!
	//! @tparam A    Attribute type. Must match the type of attribute at specified index.
	//! @param index Attribute index. Specifies which attribute (e.g. normal, texcoord, vertex)
	//!              we're working with.
	//! @return      Non-const reference to the attribute buffer.
	template<class A> 
	AttributeBuffer<A>& getBuffer(const uint attributeIndex)
	{
		return *static_cast<AttributeBuffer<A>*>(buffers[attributeIndex]);
	}
};

#endif // _STELTESTGLVERTEXBUFFERBACKEND_HPP_