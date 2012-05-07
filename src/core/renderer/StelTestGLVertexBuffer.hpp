#ifndef _STELTESTGLVERTEXBUFFER_HPP_
#define _STELTESTGLVERTEXBUFFER_HPP_

#include <QVector>
#include "core/VecMath.hpp"
#include "StelVertexBuffer.hpp"


//! OpenGL vertex array style VertexBuffer backend, used for testing.
//!
//! @sa StelVertexBuffer
template<class V> class StelTestGLVertexBuffer : public StelVertexBuffer<V>
{
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
		
		//! How should the attribute be interpreted (color, normal, etc) ?
		const AttributeInterpretation interpretation;
	};
	
public:
	//! Construct a TestGLVertexBuffer.
	//!
	//! Creates attribute buffers.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	StelTestGLVertexBuffer(const PrimitiveType type)
		: type(type)
	{
		// Create a buffer for each vertex attribute.
		for(uint attrib = 0; attrib < V::attributeCount; ++attrib)
		{
			AttributeType type = V::attributeType[attrib];
			AttributeInterpretation interpretation = V::attributeInterpretation[attrib];
			
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
	
	virtual ~StelTestGLVertexBuffer()
	{
		Q_ASSERT_X(buffers.size() == static_cast<int>(V::attributeCount),
		           "Attribute buffer count does not match attribute count",
		           "StelQGLVertexBuffer::~StelQGLVertexBuffer");
		
		for(uint buffer = 0; buffer < V::attributeCount; ++buffer)
		{
			delete buffers[buffer];
		}
	}
	
	//! Called by Renderer to draw the buffer.
	//!
	//! @note It's the Renderer's responsibility to ensure that the buffer is locked()
	//! before drawing (using the locked() method).
	void draw()
	{
		//TODO first use StelPainter, later we'll set our own color/vertex/texcoord pointers 
		//and draw it ourselves, and finally, use QGeometryData (in a parallel implementation).
	}
	
protected:
	virtual void addVertex_(const V& vertex)
	{
		// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
		const quint8* attribPtr = reinterpret_cast<const quint8*>(&vertex);
		for(uint attrib = 0; attrib < V::attributeCount; ++attrib)
		{
			const AttributeType type = V::attributeType[attrib];
			
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
	
	virtual V getVertex_(const uint index) 
	{
		V result;
		// Points to the current attribute (e.g. color, normal, vertex) within the result.
		quint8* attribPtr = reinterpret_cast<quint8*>(&result);
		for(uint attrib = 0; attrib < V::attributeCount; ++attrib)
		{
			const AttributeType type = V::attributeType[attrib];
			
			// Get each attribute from its buffer and set result attribute to that.
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
		return result;
	}
	
	virtual void setVertex_(const uint index, const V& vertex)
	{
		// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
		const quint8* attribPtr = reinterpret_cast<const quint8*>(&vertex);
		for(uint attrib = 0; attrib < V::attributeCount; ++attrib)
		{
			const AttributeType type = V::attributeType[attrib];
			
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
	
private:
	//! Graphics primitive type formad by the vertices of this buffer.
	PrimitiveType type;
	
	//! Buffers of each vertex attribute.
	QVector<AnyAttributeBuffer*> buffers;
	
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
	
	//! Access particular attribute of a vertex.
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

#endif // _STELTESTGLVERTEXBUFFER_HPP_