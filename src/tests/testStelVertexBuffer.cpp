
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
	
	static const uint attributeCount = 1;
	static const AttributeType attributeType[attributeCount];
	static const AttributeInterpretation attributeInterpretation[attributeCount];
};
const AttributeType TestVertex1::attributeType[TestVertex1::attributeCount] = 
	{AT_Vec3f};
const AttributeInterpretation TestVertex1::attributeInterpretation[TestVertex1::attributeCount] =
	{Position};


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
	
	static const uint attributeCount = 2;
	static const AttributeType attributeType[attributeCount];
	static const AttributeInterpretation attributeInterpretation[attributeCount];
};
const AttributeType TestVertex2::attributeType[TestVertex2::attributeCount] = 
	{AT_Vec3f, AT_Vec2f};
const AttributeInterpretation TestVertex2::attributeInterpretation[TestVertex2::attributeCount] =
	{Position, TexCoord};

//! Test vertex with a position, texcoord, normal and color.
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
	
	static const uint attributeCount = 4;
	static const AttributeType attributeType[attributeCount];
	static const AttributeInterpretation attributeInterpretation[attributeCount];
};
const AttributeType TestVertex3::attributeType[TestVertex3::attributeCount] = 
	{AT_Vec3f, AT_Vec2f, AT_Vec3f, AT_Vec4f};
const AttributeInterpretation TestVertex3::attributeInterpretation[TestVertex3::attributeCount] =
	{Position, TexCoord, Normal, Color};
	
	
//! Test specified StelVertexBuffer implementation with specified vertex type.
//!
//! @tparam Buffer Vertex buffer type, already parametrized by the vertex type.
//! @tparam Vertex Vertex type stored in the buffer. Must match the vertex type parameter of Buffer.
template<class Buffer, class Vertex> void testVertexBuffer()
{
	StelVertexBuffer<Vertex>* buffer = new Buffer(Triangles);
	
	// Make sure the buffer was initialized correctly.
	QCOMPARE(buffer->locked(), false);
	QCOMPARE(buffer->length(), 0u);
	
	// Vertices we'll be adding (random generated).
	QVector<Vertex> vertexVector;
	for(uint v = 0; v < 10042; ++v)
	{
		vertexVector.append(Vertex());
	}
	
	// Add vertices to the buffer.
	foreach(Vertex vertex, vertexVector)
	{
		buffer->addVertex(vertex);
	}
	
	// Verify buffer length.
	QCOMPARE(buffer->length(), static_cast<uint>(vertexVector.size()));
	
	// Verify buffer contents (and that getVertex works).
	for(uint v = 0; v < buffer->length(); ++v)
	{
		QCOMPARE(buffer->getVertex(v), vertexVector[v]);
	}
	
	// Test setVertex by setting the vertices in reverse order.
	for(uint v = 0; v < buffer->length(); ++v)
	{
		buffer->setVertex(buffer->length() - v - 1, vertexVector[v]);
	}
	
	// Verify that the vertices have been set as expected.
	for(uint v = 0; v < buffer->length(); ++v)
	{
		QCOMPARE(buffer->getVertex(buffer->length() - v - 1), vertexVector[v]);
	}
	
	delete buffer;
}

void TestStelVertexBuffer::testStelTestGLVertexBuffer()
{
	testVertexBuffer<StelTestGLVertexBuffer<TestVertex1>, TestVertex1>();
	testVertexBuffer<StelTestGLVertexBuffer<TestVertex2>, TestVertex2>();
	testVertexBuffer<StelTestGLVertexBuffer<TestVertex3>, TestVertex3>();
}