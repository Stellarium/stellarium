#include "StelQGLIndexBuffer.hpp"
#include "StelQGL2Renderer.hpp"
#include "StelTestQGL2VertexBufferBackend.hpp"

StelTestQGL2VertexBufferBackend::
StelTestQGL2VertexBufferBackend(const PrimitiveType type,
                                const QVector<StelVertexAttribute>& attributes)
	: StelVertexBufferBackend(attributes)
	, locked(false)
	, primitiveType(type)
	, vertexCount(0)
	, vertexCapacity(0)
{
	// Create a buffer for each vertex attribute.
	for(int attrib = 0; attrib < attributes.size(); ++attrib)
	{
		const AttributeType type = attributes[attrib].type;
		const AttributeInterpretation interpretation = attributes[attrib].interpretation;

		AnyAttributeBuffer* buffer;

		switch(type)
		{
			case AttributeType_Vec2f: buffer = new AttributeBuffer<Vec2f>(interpretation); break;
			case AttributeType_Vec3f: buffer = new AttributeBuffer<Vec3f>(interpretation); break;
			case AttributeType_Vec4f: buffer = new AttributeBuffer<Vec4f>(interpretation); break;
			default:  Q_ASSERT(false);
		}

		buffers.append(buffer);
	}
}

StelTestQGL2VertexBufferBackend::~StelTestQGL2VertexBufferBackend()
{
	Q_ASSERT_X(buffers.size() == static_cast<int>(attributes.size()),
	           Q_FUNC_INFO, "Attribute buffer count does not match attribute count");

	for(int buffer = 0; buffer < buffers.size(); ++buffer)
	{
		delete buffers[buffer];
	}
}

void StelTestQGL2VertexBufferBackend::addVertex(const quint8* const vertexInPtr)
{
	//StelVertexBuffer enforces bounds, so we don't need to
	++vertexCount;
	if(vertexCount <= vertexCapacity)
	{
		setVertex(vertexCount - 1, vertexInPtr);
		return;
	}
	++vertexCapacity;
	// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
	const quint8* attribPtr = vertexInPtr;
	for(int attrib = 0; attrib < attributes.size(); ++attrib)
	{
		const AttributeType type = attributes[attrib].type;

		// Add each attribute to its buffer.
		switch(type)
		{
			case AttributeType_Vec2f: addAttribute<Vec2f>(attrib, attribPtr); break;
			case AttributeType_Vec3f: addAttribute<Vec3f>(attrib, attribPtr); break;
			case AttributeType_Vec4f: addAttribute<Vec4f>(attrib, attribPtr); break;
			default: Q_ASSERT(false);
		}
		attribPtr += attributeSize(type);
	}
}

void StelTestQGL2VertexBufferBackend::getVertex(const uint index, quint8* const vertexOutPtr)
{
	// Points to the current attribute (e.g. color, normal, vertex) within output.
	quint8* attribPtr = vertexOutPtr;
	for(int attrib = 0; attrib < attributes.size(); ++attrib)
	{
		const AttributeType type = attributes[attrib].type;

		// Get each attribute from its buffer and set result's attribute to that.
		switch(type)
		{
			case AttributeType_Vec2f:
				*reinterpret_cast<Vec2f*>(attribPtr) = getAttribute<Vec2f>(attrib, index);
				break;
			case AttributeType_Vec3f:
				*reinterpret_cast<Vec3f*>(attribPtr) = getAttribute<Vec3f>(attrib, index);
				break;
			case AttributeType_Vec4f:
				*reinterpret_cast<Vec4f*>(attribPtr) = getAttribute<Vec4f>(attrib, index);
				break;
			default:
				Q_ASSERT(false);
		}

		attribPtr += attributeSize(type);
	}
}

void StelTestQGL2VertexBufferBackend::setVertex(const uint index, const quint8* const vertexInPtr)
{
	// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
	const quint8* attribPtr = vertexInPtr;
	for(int attrib = 0; attrib < attributes.size(); ++attrib)
	{
		const AttributeType type = attributes[attrib].type;

		//Set each attribute in its buffer.
		switch(type)
		{
			case AttributeType_Vec2f:
				getAttribute<Vec2f>(attrib, index) = *reinterpret_cast<const Vec2f*>(attribPtr);
				break;
			case AttributeType_Vec3f:
				getAttribute<Vec3f>(attrib, index) = *reinterpret_cast<const Vec3f*>(attribPtr);
				break;
			case AttributeType_Vec4f:
				getAttribute<Vec4f>(attrib, index) = *reinterpret_cast<const Vec4f*>(attribPtr);
				break;
			default:
				Q_ASSERT(false);
		}

		attribPtr += attributeSize(type);
	}
}

void StelTestQGL2VertexBufferBackend::
	draw(StelQGL2Renderer& renderer, const QMatrix4x4& projectionMatrix,
	     StelQGLIndexBuffer* indexBuffer)
{
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to draw a vertex buffer that is not locked.");

	// Get shader for our format from the renderer.
	QGLShaderProgram* program = renderer.getShaderProgram(attributes);
	if(!program->bind())
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Failed to bind shader program");
	}
	
	// Maximum number of vertex attributes is one per interpretation, so this 
	// array is large enough (it is also asserted below that we don't get into
	// trouble if there are over 16 interpretations in future)
	int enabledAttributes [16];
	int attributeCount = 0;

	bool vertexColors = false;
	// Provide all vertex attributes' arrays to GL.
	foreach(const StelVertexAttribute& attribute, attributes)
	{
		if(attribute.interpretation == AttributeInterpretation_Color)
		{
			vertexColors = true;
		}

		const char* const name = glslAttributeName(attribute.interpretation);
		const int handle = program->attributeLocation(name);
		if(handle == -1)
		{
			qDebug() << "Missing vertex attribute: " << name;
			Q_ASSERT_X(false, Q_FUNC_INFO,
			           "Vertex attribute required for current vertex format is not in the GLSL shader");
		}

		program->setAttributeArray(handle, glAttributeType(attribute.type), 
		                           buffers[attributeCount]->constData(), 
		                           attributeDimensions(attribute.type));
		program->enableAttributeArray(handle);

		Q_ASSERT_X(attributeCount < 16, Q_FUNC_INFO,
		           "enabledAttributes array is too small to handle all vertex attributes.");
		enabledAttributes[attributeCount++] = handle;
	}

	program->setUniformValue("projectionMatrix", projectionMatrix);

	// If we don't have a color per vertex, we have a global color
	// (to keep in line with Stellarium behavior before the GL refactor)
	if(!vertexColors)
	{
		const QColor& color = renderer.getGlobalColor();
		program->setUniformValue("globalColor", 
		                         color.redF(), color.greenF(), 
		                         color.blueF(), color.alphaF());
	}

	// Draw the vertex arrays.
	if(NULL != indexBuffer)
	{
		glDrawElements(glPrimitiveType(primitiveType), indexBuffer->length(),
		               glIndexType(indexBuffer->indexType()), indexBuffer->indices());
	}
	else
	{
		glDrawArrays(glPrimitiveType(primitiveType), 0, vertexCount);
	}

	for(int attribute = 0; attribute < attributeCount; attribute++) 
	{
		program->disableAttributeArray(enabledAttributes[attribute]);
	}

	program->release();
}
