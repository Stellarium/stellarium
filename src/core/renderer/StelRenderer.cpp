#include "StelRenderer.hpp"
#include "VecMath.hpp"
#include "StelVertexBuffer.hpp"

void StelRenderer::drawRectInternal
	(const bool textured, const float x, const float y, const float width, 
	 const float height, const float angle)
{
	// Could be improved by keeping the vertex buffer as a data member,
	// or even caching all rectangle draws to the same buffer and drawing them 
	// at once at the end of the frame.
	
	Vec2f ne, nw, se, sw;

	// Faster path for angles that are zero or extremely small.
	if(abs(angle) < 0.1)
	{
		ne = Vec2f(x, y);
		nw = Vec2f(x + width, y);
		se = Vec2f(x, y + height);
		sw = Vec2f(x + width, y + height);
	}
	// Need to rotate the rectangle (around its center).
	else
	{
		const float cosr       = std::cos(angle / 180 * M_PI);
		const float sinr       = std::sin(angle / 180 * M_PI);
		const float halfWidth  = width  * 0.5;
		const float halfHeight = height * 0.5;
		const Vec2f center(x + halfWidth, y + halfHeight);

		const float widthCos  = halfWidth  * cosr;
		const float heightCos = halfHeight * cosr;
		const float widthSin  = halfWidth  * sinr;
		const float heightSin = halfHeight * sinr;

		ne = center + Vec2f(-widthCos + heightSin, -widthSin - heightCos);
		nw = center + Vec2f(widthCos  + heightSin, widthSin  - heightCos);
		se = center + Vec2f(-widthCos - heightSin, -widthSin + heightCos);
		sw = center + Vec2f(widthCos  - heightSin, widthSin  + heightCos);
	}
	
	// Create a vertex buffer for the rectangle and draw it.
	if(textured)
	{
		StelVertexBuffer<TexturedVertex>* buffer = 
			createVertexBuffer<TexturedVertex>(PrimitiveType_TriangleStrip);

		buffer->addVertex(TexturedVertex(ne, Vec2f(0.0f , 0.0f)));
		buffer->addVertex(TexturedVertex(nw, Vec2f(1.0f , 0.0f)));
		buffer->addVertex(TexturedVertex(se, Vec2f(0.0f , 1.0f)));
		buffer->addVertex(TexturedVertex(sw, Vec2f(1.0f , 1.0f)));

		buffer->lock();
		drawVertexBuffer(buffer);
		delete buffer;
	}
	else
	{
		StelVertexBuffer<Vertex>* buffer = 
			createVertexBuffer<Vertex>(PrimitiveType_TriangleStrip);

		buffer->addVertex(Vertex(ne));
		buffer->addVertex(Vertex(nw));
		buffer->addVertex(Vertex(se));
		buffer->addVertex(Vertex(sw));

		buffer->lock();
		drawVertexBuffer(buffer);
		delete buffer;
	}
}

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

void StelRenderer::drawRect
	(const float x, const float y, const float width, const float height, const float angle)
{
	drawRectInternal(false, x, y, width, height, angle);
}

void StelRenderer::drawTexturedRect
	(const float x, const float y, const float width, const float height, const float angle)
{
	drawRectInternal(true, x, y, width, height, angle);
}
