#ifndef _STELQGL1ARRAYVERTEXBUFFERBACKEND_HPP_
#define _STELQGL1ARRAYVERTEXBUFFERBACKEND_HPP_

#include "StelGLUtilityFunctions.hpp"
#include "StelQGLArrayVertexBufferBackend.hpp"


//! OpenGL 1 vertex array style VertexBuffer backend.
//!
//! @sa StelVertexBuffer, StelRenderer
class StelQGL1ArrayVertexBufferBackend : public StelQGLArrayVertexBufferBackend
{
//! Only StelQGL1Renderer can construct this backend.
friend class StelQGL1Renderer;
public:
	//! Draw the vertex buffer, optionally with index buffer specifying which indices to draw.
	//!
	//! Called by StelQGL1Renderer::drawVertexBufferBackend().
	//!
	//! @param renderer         Renderer that owns this buffer.
	//! @param projectionMatrix Projection matrix (in GL format) used for drawing.
	//! @param indexBuffer      If NULL, all vertices in the buffer are drawn 
	//!                         in the order they are stored.
	//!                         If not NULL, specifies indices of vertices to draw.
	void draw(class StelQGL1Renderer& renderer, const Mat4f& projectionMatrix,
	          class StelQGLIndexBuffer* indexBuffer);

private:
	//! Construct a StelQGL1ArrayVertexBufferBackend. Only StelQGL1Renderer can do this.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	//! @param attributes Specifications of vertex attributes that will be stored in the buffer.
	StelQGL1ArrayVertexBufferBackend(const PrimitiveType type,
	                                 const QVector<StelVertexAttribute>& attributes);
};

#endif // _STELQGL1ARRAYVERTEXBUFFERBACKEND_HPP_
