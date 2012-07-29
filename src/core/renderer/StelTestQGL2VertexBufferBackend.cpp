#include "StelProjector.hpp"
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
	, usingProjectedPositions(false)
{
	// Create a buffer for each vertex attribute.
	for(int attrib = 0; attrib < this->attributes.count; ++attrib)
	{
		const AttributeInterpretation interpretation = 
			this->attributes.attributes[attrib].interpretation;

		AnyAttributeBuffer* buffer;

		switch(this->attributes.attributes[attrib].type)
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
	Q_ASSERT_X(buffers.size() == static_cast<int>(attributes.count),
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
		setVertexNonVirtual(vertexCount - 1, vertexInPtr);
		return;
	}
	++vertexCapacity;
	// Points to the current attribute (e.g. color, normal, vertex) within the vertex.
	const quint8* attribPtr = vertexInPtr;
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		// Add each attribute to its buffer.
		switch(attributes.attributes[attrib].type)
		{
			case AttributeType_Vec2f: addAttribute<Vec2f>(attrib, attribPtr); break;
			case AttributeType_Vec3f: addAttribute<Vec3f>(attrib, attribPtr); break;
			case AttributeType_Vec4f: addAttribute<Vec4f>(attrib, attribPtr); break;
			default: Q_ASSERT(false);
		}
		attribPtr += attributes.sizes[attrib];
	}
}

void StelTestQGL2VertexBufferBackend::getVertex
	(const int index, quint8* const vertexOutPtr) const
{
	// Points to the current attribute (e.g. color, normal, vertex) within output.
	quint8* attribPtr = vertexOutPtr;
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		// Get each attribute from its buffer and set result's attribute to that.
		switch(attributes.attributes[attrib].type)
		{
			case AttributeType_Vec2f:
				*reinterpret_cast<Vec2f*>(attribPtr) = getAttributeConst<Vec2f>(attrib, index);
				break;
			case AttributeType_Vec3f:
				*reinterpret_cast<Vec3f*>(attribPtr) = getAttributeConst<Vec3f>(attrib, index);
				break;
			case AttributeType_Vec4f:
				*reinterpret_cast<Vec4f*>(attribPtr) = getAttributeConst<Vec4f>(attrib, index);
				break;
			default:
				Q_ASSERT(false);
		}

		attribPtr += attributes.sizes[attrib];
	}
}

//! Helper function that enables a vertex attribute and provides attribute data to GL.
//!
//! @param program   Shader program we're drawing with.
//! @param handleOut GL handle to the attribute (attribute location) will be stored here.
//! @param attribute Defines the attribute to enable.
//! @param data      Attribute data (e.g. positions, texcoords, normals, etc.)
void enableAttribute(QGLShaderProgram* program, int& handleOut,
                     const StelVertexAttribute& attribute, const void* data)
{
	const char* const name = glslAttributeName(attribute.interpretation);
	const int handle = program->attributeLocation(name);
	if(handle == -1)
	{
		qDebug() << "Missing vertex attribute: " << name;
		Q_ASSERT_X(false, Q_FUNC_INFO,
		           "Vertex attribute required for current vertex format is not in the GLSL shader");
	}

	program->setAttributeArray(handle, glAttributeType(attribute.type), 
	                           data, attributeDimensions(attribute.type));
	program->enableAttributeArray(handle);

	handleOut = handle;
}

void StelTestQGL2VertexBufferBackend::
	draw(StelQGL2Renderer& renderer, const QMatrix4x4& projectionMatrix,
	     StelQGLIndexBuffer* indexBuffer)
{
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to draw a vertex buffer that is not locked.");

	// Get shader for our format from the renderer.
	QGLShaderProgram* program = 
		renderer.getShaderProgram(attributes.attributes, attributes.count);
	if(!program->bind())
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Failed to bind shader program");
	}
	
	int enabledAttributes [MAX_VERTEX_ATTRIBUTES];

	bool usingVertexColors = false;
	// Provide all vertex attributes' arrays to GL.
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		Q_ASSERT_X(attrib < MAX_VERTEX_ATTRIBUTES, Q_FUNC_INFO,
		           "enabledAttributes array is too small to handle all vertex attributes.");

		const StelVertexAttribute& attribute(attributes.attributes[attrib]);
		if(attribute.interpretation == AttributeInterpretation_Color)
		{
			usingVertexColors = true;
		}
		else if(attribute.interpretation == AttributeInterpretation_Position &&
		        usingProjectedPositions)
		{
			// Using projected positions, use projectedPositions vertex array.
			enableAttribute(program, enabledAttributes[attrib], 
			                attribute, projectedPositions.constData());
			// Projected positions are used within a single renderer drawVertexBufferBackend
			// call - we set this so any further calls with this buffer won't accidentally 
			// use projected data from before (we don't destroy the buffer so we can 
			// reuse it).
			usingProjectedPositions = false;
			continue;
		}

		// Not a position attribute, or not using projected positions, 
		// so use the normal vertex array.
		enableAttribute(program, enabledAttributes[attrib], attribute, 
		                buffers[attrib]->constData());
	}

	program->setUniformValue("projectionMatrix", projectionMatrix);

	// If we don't have a color per vertex, we have a global color
	// (to keep in line with Stellarium behavior before the GL refactor)
	if(!usingVertexColors)
	{
		const Vec4f& color = renderer.getGlobalColor();
		program->setUniformValue("globalColor", color[0], color[1], color[2], color[3]);
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

	for(int attribute = 0; attribute < attributes.count; attribute++) 
	{
		program->disableAttributeArray(enabledAttributes[attribute]);
	}

	program->release();
}

int StelTestQGL2VertexBufferBackend::
	getAttributeIndex(const AttributeInterpretation interpretation) const
{
	for(int attrib = 0; attrib < attributes.count; ++attrib)
	{
		const StelVertexAttribute& attribute(attributes.attributes[attrib]);
		if(attribute.interpretation == interpretation)
		{
			return attrib;
		}
	}
	return -1;
}

void StelTestQGL2VertexBufferBackend::
	projectVertices(StelProjectorP projector, StelQGLIndexBuffer* indexBuffer)
{
	// This is a backend function called during (right before) drawing, so we need 
	// to be locked.
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to project a vertex buffer that is not locked.");

	// Get the position attribute and ensure that we can project it.
	const int posIndex = getAttributeIndex(AttributeInterpretation_Position);
#ifndef NDEBUG
	const StelVertexAttribute& attribute(attributes.attributes[posIndex]);
#endif
	Q_ASSERT_X(posIndex >= 0, Q_FUNC_INFO, "Vertex format without a position attribute");
	Q_ASSERT_X(attributeDimensions(attribute.type) == 3, Q_FUNC_INFO,
	           "Trying to use a custom StelProjector to project non-3D vertex positions");
	Q_ASSERT_X(attribute.type == AttributeType_Vec3f, Q_FUNC_INFO, 
	           "Unknown 3D vertex attribute type");
	AttributeBuffer<Vec3f>* positions = 
		dynamic_cast<AttributeBuffer<Vec3f>*>(buffers[posIndex]);
	Q_ASSERT_X(NULL != positions, Q_FUNC_INFO,
	           "Unexpected attribute buffer data type");

	usingProjectedPositions = true;


	// We have two different cases :
	// a) Not using an index array. Size of vertex data is known.
	// b) Using an index array. 
	//    We find the max index and project vertices until that index, 
	//    or if there are not many indices, project vertices separately 
	//    based on indices.
	const int minProjectedSize =
		(NULL == indexBuffer) ? vertexCount : (indexBuffer->maxIndex() + 1);
	if(projectedPositions.size() < minProjectedSize)
	{
		projectedPositions.resize(minProjectedSize);
	}

	// If the index buffer is big, it's likely that it covers most 
	// of the vertex buffer so we can just project everything en masse, 
	// taking advantage of the cache.
	if(NULL == indexBuffer || indexBuffer->length() >= minProjectedSize)
	{
		projector->project(minProjectedSize, positions->data.constData(), 
		                   projectedPositions.data());
	}
	// Project vertices separately based on indices.
	else
	{
		const Vec3f* const pos = positions->data.constData();
		Vec3f* const projected = projectedPositions.data();
		if(indexBuffer->indexType() == IndexType_U16)
		{
			const ushort* const indices = indexBuffer->indices16.constData();
			const int indexCount        = indexBuffer->indices16.size();
			for(int i = 0; i < indexCount; ++i)
			{
				const ushort index = indices[i];
				projector->project(pos[index], projected[index]);
			}
		}
		else if(indexBuffer->indexType() == IndexType_U32)
		{
			const uint* const indices = indexBuffer->indices32.constData();
			const int indexCount      = indexBuffer->indices32.size();
			for(int i = 0; i < indexCount; ++i)
			{
				const uint index = indices[i];
				projector->project(pos[index], projected[index]);
			}
		}
		else
		{
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
		}
	}
}
