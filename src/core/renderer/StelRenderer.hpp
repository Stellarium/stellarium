#ifndef _STELRENDERER_HPP_
#define _STELRENDERER_HPP_

#include <QImage>
#include <QPainter>
#include <QSize>

#include "StelVertexBuffer.hpp"
#include "StelViewportEffect.hpp"


//Development notes:
//
//enable/disablePainting are temporary and will be removed
//    once painting is done through StelRenderer.
//
//FPS balancing, currently in StelAppGraphicsWidget, might be moved into StelRenderer
//    implementation.
//
//VisualEffects now always exist. Renderer implementation decides whether to apply them or not.
//    This is because things such as FBOs are implementation details of Renderer,
//    not seen by outside code.
//    If this is not desired, it a visualEffectsSupported() method could be added to Renderer.

//TODO If Renderer implementation now decides whether or not to use VisualEffects,
//     "framebufferOnly" makes no sense, and should be removed.



//! Handles all graphics related functionality.
//! 
//! @note This is an interface. It should have only functions, no data members,
//!       as it might be used in multiple inheritance.
class StelRenderer
{
public:
	//! Initialize the renderer. 
	//! 
	//! Must be called before any other methods.
	//!
	//! @return true on success, false if there was an error.
	virtual bool init() = 0;
	
	//! Take a screenshot and return it.
	virtual QImage screenshot() = 0;
	
	//! Enable painting.
	//!
	//! Painting must be disabled when calling this.
	//!
	//! This method is temporary and will be removed once painting is handled through Renderer.
	virtual void enablePainting() = 0;
	
	//! Disable painting.
	//!
	//! Painting must be enabled when calling this.
	//!
	//! This method is temporary and will be removed once painting is handled through Renderer.
	virtual void disablePainting() = 0;
	
	//! Set default QPainter to use for Qt-based painting commands.
	virtual void setDefaultPainter(QPainter* painter) = 0;
	
	//! Must be called once at startup and on every GL viewport resize, specifying new size.
	virtual void viewportHasBeenResized(QSize size) = 0;
	
	//! Create an empty vertex buffer and return a pointer to it.
	//!
	//! The vertex buffer must be deleted by the user once it is not used
	//! (and before the Renderer is destroyed).
	//!
	//! @tparam V Vertex type. See the example in StelVertexBuffer documentation
	//!           on how to define a vertex type.
	//! @param primitiveType Graphics primitive type stored in the buffer.
	//!
	//! @return New vertex buffer storing vertices of type V.
	template<class V>
	StelVertexBuffer<V>* createVertexBuffer(const PrimitiveType primitiveType)
	{
		return StelVertexBuffer<V>(createVertexBufferBackend(primitiveType, V::attributes));
	}
	
	//! Draw contents of a vertex buffer.
	//!
	//! @param vertexBuffer Vertex buffer to draw.
	//! @param indexBuffer  Index buffer specifying which vertices from the buffer to draw.
	//!                     If NULL, indexing will not be used and vertices will be drawn
	//!                     directly in order they are in the buffer.
	//! @param projector    Projector to project vertices' positions before drawing.
	//!                     If NULL, no projection will be done and the vertices will be drawn
	//!                     directly.
	//!
	//! @todo This member function is still not implemented.
	template<class V>
	void drawVertexBuffer(StelVertexBuffer<V>* vertexBuffer, 
	                      class StelIndexBuffer* indexBuffer = NULL,
	                      StelProjectorP projector = NULL)
	{
		Q_ASSERT_X(false, "TODO - Implement this method", "StelRenderer::drawVertexBuffer");
	}
	
	//! Start using drawing calls.
	virtual void startDrawing() = 0;
	
	//! Suspend drawing, not showing the result on the screen. Drawing can be continued later.
	virtual void suspendDrawing() = 0;
	
	//! Finish drawing, showing the result on screen.
	virtual void finishDrawing() = 0;
	
	//! Draw the result of drawing commands to the window, applying given effect if possible.
	virtual void drawWindow(StelViewportEffect* effect) = 0;
	
protected:
	//! Create a vertex buffer backend. Used by createVertexBuffer.
	//!
	//! This allows each Renderer backend to create its own vertex buffer backend.
	//!
	//! @param primitiveType Graphics primitive type stored in the buffer.
	//! @param attributes    Descriptions of all attributes of the vertex type stored in the buffer.
	//!
	//! @return Pointer to a vertex buffer backend specific to the Renderer backend.
	virtual StelVertexBufferBackend* createVertexBufferBackend
		(const PrimitiveType primitiveType, const QVector<VertexAttribute>& attributes) = 0;
};

#endif // _STELRENDERER_HPP_