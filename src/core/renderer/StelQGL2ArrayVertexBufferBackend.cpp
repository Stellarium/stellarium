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

#include "StelQGL2Renderer.hpp"
#include "StelQGL2ArrayVertexBufferBackend.hpp"


StelQGL2ArrayVertexBufferBackend::
StelQGL2ArrayVertexBufferBackend(const PrimitiveType type,
                                 const QVector<StelVertexAttribute>& attributes)
	: StelQGLArrayVertexBufferBackend(type, attributes)
{
}

//! Helper function that enables a vertex attribute and provides attribute data to GL.
//!
//! @param program   Shader program we're drawing with.
//! @param handleOut GL handle to the attribute (attribute location) will be stored here.
//! @param attribute Defines the attribute to enable.
//! @param data      Attribute data (e.g. positions, texcoords, normals, etc.)
void enableAttribute(QGLShaderProgram& program, int& handleOut,
                     const StelVertexAttribute& attribute, const void* data)
{
	const char* const name = glslAttributeName(attribute.interpretation);
	const int handle = program.attributeLocation(name);
	if(handle == -1)
	{
		qDebug() << "Missing vertex attribute: " << name;
		foreach(QGLShader* shader, program.shaders())
		{
			qDebug() << "Source of a shader linked in the program: ";
			qDebug() << shader->sourceCode();
		}
		Q_ASSERT_X(false, Q_FUNC_INFO,
		           "Vertex attribute required for current vertex format is not in the GLSL shader");
	}

	program.setAttributeArray(handle, glAttributeType(attribute.type), 
	                          data, attributeDimensions(attribute.type));
	program.enableAttributeArray(handle);

	handleOut = handle;
}

void StelQGL2ArrayVertexBufferBackend::
	draw(StelQGL2Renderer& renderer, const Mat4f& projectionMatrix,
	     StelQGLIndexBuffer* indexBuffer, StelQGLGLSLShader* shader)
{
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to draw a vertex buffer that is not locked.");

	// Get shader for our format from the renderer.
	QGLShaderProgram& program = shader->getProgram();
	
	int enabledAttributes [MAX_VERTEX_ATTRIBUTES];

	bool usingVertexColors = false;
	// Number of all attributes, including any special ones like unprojected position.
	int totalAttributes = 0;
	// Provide all vertex attributes' arrays to GL.
	for(int attrib = 0; attrib < attributes.count; ++attrib, ++totalAttributes)
	{
		Q_ASSERT_X(attrib < MAX_VERTEX_ATTRIBUTES, Q_FUNC_INFO,
		           "enabledAttributes array is too small to handle all vertex attributes.");

		const StelVertexAttribute& attribute(attributes.attributes[attrib]);
		if(attribute.interpretation == AttributeInterpretation_Color)
		{
			usingVertexColors = true;
		}

		const bool position = (attribute.interpretation == AttributeInterpretation_Position);
		const void* attributeData = attributeBuffers[attribute.interpretation];
	
		// Some shaders (e.g. lighting) need unprojected vertex positions. Vertex 
		// positions can be projected on the CPU or GPU. If they are projected on the
		// GPU, the shader already has the unprojected vertex (the default position 
		// attribute), but we'd need two versions of each shader: One for CPU 
		// projection (need "unprojectedVertex"), and one for GPU ("vertex" is enough).
		// So we just pass unprojectedVertex in both cases.
		if(position && shader->useUnprojectedPosition())
		{
			const int handle = program.attributeLocation("unprojectedVertex");
			if(handle == -1)
			{
				qDebug() << "Missing vertex attribute: unprojectedVertex";
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unprojected position vertex attribute is "
				                               "enabled, but not present in the GLSL shader");
			}
			program.setAttributeArray(handle, glAttributeType(attribute.type), 
			                          attributeData, attributeDimensions(attribute.type));
			program.enableAttributeArray(handle);
			enabledAttributes[totalAttributes] = handle;
			++totalAttributes;
		}

		// If we're projecting vertices on the CPU, we pass them here.
		// (CPU-projected vertices are stored in a separate array - projectedPositions)
		if(position && usingProjectedPositions)
		{
			// Projected positions are valid for a single renderer drawVertexBufferBackend
			// call - we set this so further draws won't accidentally use projected data 
			// from before (we don't destroy the buffer so we can reuse it).
			usingProjectedPositions = false;

			// Using projected positions, use projectedPositions vertex array.
			attributeData = projectedPositions;
		}
		enableAttribute(program, enabledAttributes[totalAttributes], attribute, attributeData);
	}

	const RAW_GL_MATRIX& glProjectionMatrix
		(*reinterpret_cast<const RAW_GL_MATRIX* const>(&projectionMatrix));
	program.setUniformValue("projectionMatrix", glProjectionMatrix);

	// If we don't have a color per vertex, we have a global color
	// (to keep in line with Stellarium behavior before the GL refactor)
	if(!usingVertexColors)
	{
		const Vec4f& color = renderer.getGlobalColor();
		program.setUniformValue("globalColor", color[0], color[1], color[2], color[3]);
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

	for(int attribute = 0; attribute < totalAttributes; attribute++) 
	{
		program.disableAttributeArray(enabledAttributes[attribute]);
	}
}
