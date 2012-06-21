#include "StelRenderer.hpp"
#include "VecMath.hpp"
#include "StelVertexAttribute.hpp"
#include "StelVertexBuffer.hpp"

//! A plain, position-only 2D vertex.
struct Vertex
{
	Vec2f position;
	Vertex(Vec2f position):position(position){}

	VERTEX_ATTRIBUTES(Vec2f Position);
};

//! A simple 2D vertex with a position and texcoord, used for textured rectangles.
struct TexturedVertex
{
	Vec2f position;
	Vec2f texCoord;
	TexturedVertex(Vec2f position, Vec2f texCoord):position(position),texCoord(texCoord){}

	VERTEX_ATTRIBUTES(Vec2f Position, Vec2f TexCoord);
};

void StelRenderer::drawRect
	(const float x, const float y, const float width, const float height, 
	 const bool textured)
{
	//! Create a vertex buffer for the rectangle and draw it.
	if(textured)
	{
		StelVertexBuffer<TexturedVertex>* buffer = 
			createVertexBuffer<TexturedVertex>(PrimitiveType_TriangleStrip);

		buffer->addVertex(TexturedVertex(Vec2f(x,         y),          Vec2f(0.0f, 0.0f)));
		buffer->addVertex(TexturedVertex(Vec2f(x + width, y),          Vec2f(1.0f, 0.0f)));
		buffer->addVertex(TexturedVertex(Vec2f(x,         y + height), Vec2f(0.0f, 1.0f)));
		buffer->addVertex(TexturedVertex(Vec2f(x + width, y + height), Vec2f(1.0f, 1.0f)));

		buffer->lock();
		drawVertexBuffer(buffer);
		delete buffer;
	}
	else
	{
		StelVertexBuffer<Vertex>* buffer = 
			createVertexBuffer<Vertex>(PrimitiveType_TriangleStrip);

		buffer->addVertex(Vertex(Vec2f(0.0f, 0.0f)));
		buffer->addVertex(Vertex(Vec2f(1.0f, 0.0f)));
		buffer->addVertex(Vertex(Vec2f(0.0f, 1.0f)));
		buffer->addVertex(Vertex(Vec2f(1.0f, 1.0f)));

		buffer->lock();
		drawVertexBuffer(buffer);
		delete buffer;
	}
}
