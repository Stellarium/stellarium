#include "StelRenderer.hpp"
#include "VecMath.hpp"
#include "StelVertexBuffer.hpp"

void StelRenderer::drawLine
	(const float startX, const float startY, const float endX, const float endY)
{
	// Could be improved by keeping the vertex buffer as a data member,
	// or even caching all rectangle draws to the same buffer and drawing them 
	// at once at the end of the frame.
	
	// Create a vertex buffer for the line and draw it.
	StelVertexBuffer<Vertex>* buffer = createVertexBuffer<Vertex>(PrimitiveType_Lines);
	buffer->addVertex(Vertex(Vec2f(startX, startY)));
	buffer->addVertex(Vertex(Vec2f(endX, endY)));
	buffer->lock();
	drawVertexBuffer(buffer);
	delete buffer;
}
