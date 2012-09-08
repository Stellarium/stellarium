
#include "tests/testStelVertexBuffer.hpp"


QTEST_MAIN(TestStelVertexBuffer)

void TestStelVertexBuffer::initTestCase()
{
}

//! Used to generate random vertices for tests. Not really random, either.
float uniform(float min = 0.0f, float max = 100.0f)
{
	return min + ((max - min) * static_cast<float>(rand()) / RAND_MAX);
}


//! Simple test vertex with only a position.
struct TestVertex1
{
	Vec3f vertex;
	
	TestVertex1() : vertex(uniform(), uniform(), uniform()) {}
	
	bool operator == (const TestVertex1& rhs) const {return vertex == rhs.vertex;}
	
	VERTEX_ATTRIBUTES(Vec3f Position);
};

//! Test vertex with a position and a texcoord.
struct TestVertex2
{
	Vec3f vertex;
	Vec2f texCoord;
	
	TestVertex2() 
		: vertex(uniform(), uniform(), uniform()) 
		, texCoord(uniform(0.0f, 1.0f), uniform(0.0f, 1.0f))
	{
	}
	
	bool operator == (const TestVertex2& rhs) const 
	{
		return vertex == rhs.vertex && texCoord == rhs.texCoord;
	}
	
	VERTEX_ATTRIBUTES(Vec3f Position, Vec2f TexCoord);
};

struct TestVertex3
{
	Vec3f vertex;
	Vec2f texCoord;
	Vec3f normal;
	Vec4f color;
	
	TestVertex3() 
		: vertex(uniform(), uniform(), uniform()) 
		, texCoord(uniform(0.0f, 1.0f), uniform(0.0f, 1.0f))
		, normal(uniform(), uniform(), uniform()) 
		, color(uniform(0.0f, 1.0f), uniform(0.0f, 1.0f), uniform(0.0f, 1.0f), uniform(0.0f, 1.0f))
	{
	}
	
	bool operator == (const TestVertex3& rhs) const 
	{
		return vertex == rhs.vertex && 
		       texCoord == rhs.texCoord && 
		       normal == rhs.normal &&
		       color == rhs.color;
	}
	
	VERTEX_ATTRIBUTES(Vec3f Position, Vec2f TexCoord, Vec3f Normal, Vec4f Color);
};
	
template<class BufferBackend, class Vertex> 
void TestStelVertexBuffer::testVertexBuffer()
{
	StelVertexBuffer<Vertex>* buffer = 
		new StelVertexBuffer<Vertex>(new BufferBackend(PrimitiveType_Triangles, Vertex::attributes()), 
		                             PrimitiveType_Triangles);
	
	// Make sure the buffer was initialized correctly.
	QCOMPARE(buffer->locked(), false);
	QCOMPARE(buffer->length(), 0);
	
	// Vertices we'll be adding (random generated).
	QVector<Vertex> vertexVector;
	for(int v = 0; v < 10042; ++v)
	{
		vertexVector.append(Vertex());
	}
	
	// Add vertices to the buffer.
	foreach(Vertex vertex, vertexVector)
	{
		buffer->addVertex(vertex);
	}
	
	// Verify buffer length.
	QCOMPARE(buffer->length(), vertexVector.size());
	
	// Verify buffer contents (and that getVertex works).
	for(int v = 0; v < buffer->length(); ++v)
	{
		QCOMPARE(buffer->getVertex(v), vertexVector[v]);
	}
	
	// Test setVertex by setting the vertices in reverse order.
	for(int v = 0; v < buffer->length(); ++v)
	{
		buffer->setVertex(buffer->length() - v - 1, vertexVector[v]);
	}
	
	// Verify that the vertices have been set as expected.
	for(int v = 0; v < buffer->length(); ++v)
	{
		QCOMPARE(buffer->getVertex(buffer->length() - v - 1), vertexVector[v]);
	}
	
	delete buffer;
}

void TestStelVertexBuffer::testStelQGL2ArrayVertexBuffer()
{
	testVertexBuffer<StelQGL2ArrayVertexBufferBackend, TestVertex1>();
	testVertexBuffer<StelQGL2ArrayVertexBufferBackend, TestVertex2>();
	testVertexBuffer<StelQGL2ArrayVertexBufferBackend, TestVertex3>();
}
