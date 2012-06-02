#include "StelTestQGL2VertexBufferBackend.hpp"
#include "StelQGL2Renderer.hpp"

StelTestQGL2VertexBufferBackend::
StelTestQGL2VertexBufferBackend(const PrimitiveType type,
                                const QVector<StelVertexAttribute>& attributes)
	: StelVertexBufferBackend(attributes)
	, locked(false)
	, primitiveType(type)
	, vertexCount(0)
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

StelTestQGL2VertexBufferBackend::~StelTestQGL2VertexBufferBackend()
{
	Q_ASSERT_X(buffers.size() == static_cast<int>(attributes.size()),
				  "Attribute buffer count does not match attribute count",
				  "StelTestQGL2VertexBufferBackend::~StelTestQGL2VertexBufferBackend");

	for(int buffer = 0; buffer < buffers.size(); ++buffer)
	{
		delete buffers[buffer];
	}
}

void StelTestQGL2VertexBufferBackend::addVertex(const quint8* const vertexInPtr)
{
	++vertexCount;
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

void StelTestQGL2VertexBufferBackend::
     draw(StelQGL2Renderer& renderer, const QMatrix4x4& projectionMatrix)
{
	Q_ASSERT_X(locked, "Trying to draw a vertex buffer that is not locked.",
	           "StelTestQGxBufferBackend::draw");

	// Get shader for our format from the renderer.
	QGLShaderProgram* program = renderer.getShaderProgram(attributes);
	if(!program->bind())
	{
		Q_ASSERT_X(false, "Failed to bind shader program", 
		           "StelTestQGL2VertexBufferBackend::bind");
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
		if(attribute.interpretation == Color){vertexColors = true;}

		const char* const name = glslAttributeName(attribute.interpretation);
		const int handle = program->attributeLocation(name);
		if(handle == -1)
		{
			qDebug() << "Missing vertex attribute: " << name;
			Q_ASSERT_X(false, "Vertex attribute required for current vertex format "
			           "is not in the GLSL shader", "StelTestQGL2VertexBufferBackend::draw");
		}

		program->setAttributeArray(handle, glAttributeType(attribute.type), 
		                           buffers[attributeCount]->constData(), 
		                           attributeDimensions(attribute.type));
		program->enableAttributeArray(handle);

		Q_ASSERT_X(attributeCount < 16, "enabledAttributes array is too small to "
					  "handle all vertex attributes.", 
					  "StelTestQGL2VertexBufferBackend::draw");
		enabledAttributes[attributeCount++] = handle;
	}

	program->setUniformValue("projectionMatrix", projectionMatrix);

	// If we don't have a color per vertex, we have a global color
	// (to keep in line with Stellarium behavior before the GL refactor)
	if(!vertexColors)
	{
		// TODO Placeholder, until Renderer can set colors
		program->setUniformValue("globalColor", 1.0f, 1.0f, 1.0f, 1.0f);
	}

	// Draw the vertex arrays.
	glDrawArrays(glPrimitiveType(primitiveType), 0, vertexCount);

	for(int attribute = 0; attribute < attributeCount; attribute++) 
	{
		program->disableAttributeArray(enabledAttributes[attribute]);
	}

	program->release();
}



