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

#ifndef _STELQGL2ARRAYVERTEXBUFFERBACKEND_
#define _STELQGL2ARRAYVERTEXBUFFERBACKEND_

#include "StelGLUtilityFunctions.hpp"
#include "StelQGLArrayVertexBufferBackend.hpp"


//! OpenGL 2 vertex array style VertexBuffer backend, used for testing and transition.
//!
//! @note This is an internal class of the Renderer subsystem and should not be used elsewhere.
//!
//! @sa StelVertexBuffer, StelRenderer
class StelQGL2ArrayVertexBufferBackend : public StelQGLArrayVertexBufferBackend
{
//! Only StelQGL2Renderer can construct this backend, and we also need unittesting.
friend class StelQGL2Renderer;
friend class TestStelVertexBuffer;
public:
	//! Draw the vertex buffer, optionally with index buffer specifying which indices to draw.
	//!
	//! Called by StelQGL2Renderer::drawVertexBufferBackend().
	//!
	//! @param renderer         Renderer that created this buffer.
	//! @param projectionMatrix Projection matrix (column major) used for drawing.
	//! @param indexBuffer      If NULL, all vertices in the buffer are drawn 
	//!                         in the order they are stored.
	//!                         If not NULL, specifies indices of vertices to draw.
	//! @param shader           Shader used for drawing, with any needed projection/transform 
	//!                         shaders enabled.
	void draw(class StelQGL2Renderer& renderer, const Mat4f& projectionMatrix,
	          class StelQGLIndexBuffer* indexBuffer, class StelQGLGLSLShader* shader);

private:
	//! Construct a StelQGL2ArrayVertexBufferBackend. Only StelQGL2Renderer can do this.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	//! @param attributes Specifications of vertex attributes that will be stored in the buffer.
	StelQGL2ArrayVertexBufferBackend(const PrimitiveType type,
	                                 const QVector<StelVertexAttribute>& attributes);
};

#endif // _STELQGL2ARRAYVERTEXBUFFERBACKEND_

