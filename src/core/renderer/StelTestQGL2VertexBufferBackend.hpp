#ifndef _STELTESTQGL2VERTEXBUFFERBACKEND_HPP_
#define _STELTESTQGL2VERTEXBUFFERBACKEND_HPP_

#include <QVector>

#include "core/VecMath.hpp"
#include "StelGLUtilityFunctions.hpp"
#include "StelVertexBufferBackend.hpp"
#include "StelVertexAttribute.hpp"


//! OpenGL vertex array style VertexBuffer backend, used for testing and transition.
//!
//! Should be replaced by a StelQGL2VertexBufferBackend based on QGL.
//!
//! @sa StelVertexBuffer, StelRenderer
class StelTestQGL2VertexBufferBackend : public StelVertexBufferBackend
{
//! Only GLRenderer can construct this backend, and we also need unittesting.
friend class StelQGL2Renderer;
friend class TestStelVertexBuffer;
	//! Buffer containing a single attribute (e.g. color, normal, vertex).
	//!
	//! Must be downcasted to AttributeBuffer to access the stored data.
	struct AnyAttributeBuffer
	{
		//! Required to ensure that derived classes get properly deallocated.
		virtual ~AnyAttributeBuffer(){};

		//! Get a read only pointer to the data stored in buffer.
		virtual const void* constData() = 0;
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

		virtual const void* constData()
		{
			return static_cast<const void*>(data.constData());
		}

		//! Stores the attribute data (in GL terms, a vertex array).
		QVector<A> data;

		//! Specifies how should the attribute be interpreted (color, normal, etc.).
		const AttributeInterpretation interpretation;
	};

public:
	virtual ~StelTestQGL2VertexBufferBackend();

	virtual void addVertex(const quint8* const vertexInPtr);

	virtual void getVertex(const uint index, quint8* const vertexOutPtr);

	virtual void setVertex(const uint index, const quint8* const vertexInPtr);

	virtual void lock()
	{
		locked = true;
	}

	virtual void unlock()
	{
		locked = false;
	}

	virtual void clear()
	{
		vertexCount = 0;
	}

	void draw(class StelQGL2Renderer& renderer, const QMatrix4x4& projectionMatrix);

private:
	//! Is the vertex buffer locked (i.e. ready to draw?).
	bool locked;

	//! Graphics primitive type formad by the vertices of this buffer.
	PrimitiveType primitiveType;

	//! Number of used vertices in the buffer.
	int vertexCount;

	//! Actual number of vertices in the buffer.
	int vertexCapacity;

	//! Buffers of each vertex attribute.
	QVector<AnyAttributeBuffer*> buffers;

	//! Construct a StelTestQGL2VertexBufferBackend. Only GLRenderer can do this.
	//!
	//! Initializes vertex attribute buffers.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	//! @param attributes Specifications of vertex attributes that will be stored in the buffer.
	StelTestQGL2VertexBufferBackend(const PrimitiveType type,
	                                const QVector<StelVertexAttribute>& attributes);

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

#endif // _STELTESTQGL2VERTEXBUFFERBACKEND_HPP_
