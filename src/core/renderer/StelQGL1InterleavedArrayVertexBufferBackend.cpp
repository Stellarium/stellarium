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


#include "StelQGL1Renderer.hpp"
#include "StelQGL1InterleavedArrayVertexBufferBackend.hpp"


StelQGL1InterleavedArrayVertexBufferBackend::
StelQGL1InterleavedArrayVertexBufferBackend
	(const PrimitiveType type, const QVector<StelVertexAttribute>& attributes)
	: StelQGLInterleavedArrayVertexBufferBackend(type, attributes)
{
}

//! Helper function that enables a vertex attribute in an interleaved array and
//! provides attribute data to GL.
//!
//! @param attribEnum GL enumeration corresponding to the attribute interpretation will be
//!                   written here.
//! @param attribute  Defines the attribute to enable.
//! @param data       Attribute data (e.g. positions, texcoords, normals, etc.)
//! @param stride     Offset betweeen consecutive attributes in the array.
//!                   If 0, size of the attribute is assumed
//!                   (i.e. attributes are tightly packed).
void enableAttribute(GLenum& attribEnum, const StelVertexAttribute& attribute, 
                     const void* data, const int stride)
{
	attribEnum = gl1AttributeEnum(attribute.interpretation);
	glEnableClientState(attribEnum);
	switch(attribute.interpretation)
	{
		case AttributeInterpretation_Position:
			glVertexPointer(attributeDimensions(attribute.type),
			                glAttributeType(attribute.type), stride, data);
			break;
		case AttributeInterpretation_TexCoord:
			glTexCoordPointer(attributeDimensions(attribute.type),
			                  glAttributeType(attribute.type), stride, data);
			break;
		case AttributeInterpretation_Color:
			glColorPointer(attributeDimensions(attribute.type),
			               glAttributeType(attribute.type), stride, data);
			break;
		case AttributeInterpretation_Normal:
			glNormalPointer(glAttributeType(attribute.type), stride, data);
			break;
		default:
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown attribute interpretation");
	}
}

void StelQGL1InterleavedArrayVertexBufferBackend::
	draw(StelQGL1Renderer& renderer, const Mat4f& projectionMatrix,
	     StelQGLIndexBuffer* indexBuffer)
{
	Q_ASSERT_X(locked, Q_FUNC_INFO,
	           "Trying to draw a vertex buffer that is not locked.");

	GLenum enabledAttributes [MAX_VERTEX_ATTRIBUTES];

	bool usingVertexColors = false;
	bool usingTexturing = false;
	// Offset of the current attribute relative to start of a vertex in the array.
	int attributeOffset = 0;
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
		const void* attributeData = vertices + attributeOffset;
		int stride = vertexStride;

		if(attribute.interpretation == AttributeInterpretation_TexCoord)
		{
			glEnable(GL_TEXTURE_2D);
			usingTexturing = true;
		}
		if(attribute.interpretation == AttributeInterpretation_Color)
		{
			usingVertexColors = true;
		}
		if(attribute.interpretation == AttributeInterpretation_Position &&
		   usingProjectedPositions)
		{
			// Projected positions are used within a single renderer drawVertexBufferBackend
			// call - we set this so any further calls with this buffer won't accidentally 
			// use projected data from before (we don't destroy the buffer so we can 
			// reuse it).
			usingProjectedPositions = false;

			// Using projected positions from projectedPositions vertex array.
			attributeData = projectedPositions;
			stride = 0;
		}

		// Not a position attribute, or not using projected positions, 
		// so use the normal vertex array.
		enableAttribute(enabledAttributes[attrib], attribute, 
		                attributeData, stride);

		attributeOffset += attributeSize(attribute.type);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Set the real openGL projection and modelview matrix to 2d orthographic projection
	// thus we never need to change to 2dMode from now on before drawing
	glMultMatrixf(projectionMatrix);
	glMatrixMode(GL_MODELVIEW);
	// If we don't have a color per vertex, we have a global color
	// (to keep in line with Stellarium behavior before the GL refactor)
	if(!usingVertexColors)
	{
		const Vec4f& color = renderer.getGlobalColor();
		glColor4f(color[0], color[1], color[2], color[3]);
	}

	// Draw the vertices.
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
		glDisableClientState(enabledAttributes[attribute]);
	}

	if(usingTexturing)
	{
		glDisable(GL_TEXTURE_2D);
	}
}

