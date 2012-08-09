#ifndef _STELTESTQGL2VERTEXBUFFERBACKEND_HPP_
#define _STELTESTQGL2VERTEXBUFFERBACKEND_HPP_

#include "StelGLUtilityFunctions.hpp"
#include "StelQGLArrayVertexBufferBackend.hpp"


//! OpenGL 2 vertex array style VertexBuffer backend, used for testing and transition.
//!
//! Should be replaced by a StelQGL2VertexBufferBackend based on QGL and/or using VBOs.
//!
//! @sa StelVertexBuffer, StelRenderer
class StelTestQGL2VertexBufferBackend : public StelQGLArrayVertexBufferBackend
{
//! Only StelQGL2Renderer can construct this backend, and we also need unittesting.
friend class StelQGL2Renderer;
friend class TestStelVertexBuffer;
public:
	//! Draw the vertex buffer, optionally with index buffer specifying which indices to draw.
	//!
	//! Called by StelQGL2Renderer::drawVertexBufferBackend().
	//!
	//! @param renderer         Renderer that owns this buffer.
	//! @param projectionMatrix Projection matrix (in GL format) used for drawing.
	//! @param indexBuffer      If NULL, all vertices in the buffer are drawn 
	//!                         in the order they are stored.
	//!                         If not NULL, specifies indices of vertices to draw.
	void draw(class StelQGL2Renderer& renderer, const QMatrix4x4& projectionMatrix,
	          class StelQGLIndexBuffer* indexBuffer);

private:
	//! Construct a StelTestQGL2VertexBufferBackend. Only StelQGL2Renderer can do this.
	//!
	//! Initializes vertex attribute buffers.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	//! @param attributes Specifications of vertex attributes that will be stored in the buffer.
	StelTestQGL2VertexBufferBackend(const PrimitiveType type,
	                                const QVector<StelVertexAttribute>& attributes);
};

#endif // _STELTESTQGL2VERTEXBUFFERBACKEND_HPP_

