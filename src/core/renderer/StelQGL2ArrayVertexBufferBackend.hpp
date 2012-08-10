#ifndef _STELQGL2ARRAYVERTEXBUFFERBACKEND_
#define _STELQGL2ARRAYVERTEXBUFFERBACKEND_

#include "StelGLUtilityFunctions.hpp"
#include "StelQGLArrayVertexBufferBackend.hpp"


//! OpenGL 2 vertex array style VertexBuffer backend, used for testing and transition.
//!
//! Should be replaced by a StelQGL2VBOVertexBufferBackend based on QGL and/or using VBOs.
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
	//! @param renderer         Renderer that owns this buffer.
	//! @param projectionMatrix Projection matrix (in Qt format) used for drawing.
	//! @param indexBuffer      If NULL, all vertices in the buffer are drawn 
	//!                         in the order they are stored.
	//!                         If not NULL, specifies indices of vertices to draw.
	void draw(class StelQGL2Renderer& renderer, const QMatrix4x4& projectionMatrix,
	          class StelQGLIndexBuffer* indexBuffer);

private:
	//! Construct a StelQGL2ArrayVertexBufferBackend. Only StelQGL2Renderer can do this.
	//!
	//! @param type Graphics primitive type stored in the buffer.
	//! @param attributes Specifications of vertex attributes that will be stored in the buffer.
	StelQGL2ArrayVertexBufferBackend(const PrimitiveType type,
	                                const QVector<StelVertexAttribute>& attributes);
};

#endif // _STELQGL2ARRAYVERTEXBUFFERBACKEND_

