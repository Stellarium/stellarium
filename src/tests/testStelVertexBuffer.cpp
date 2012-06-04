
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
	
	static const QVector<StelVertexAttribute> attributes;
};
const QVector<StelVertexAttribute> TestVertex1::attributes = 
	(QVector<StelVertexAttribute>() << StelVertexAttribute(AttributeType_Vec3f, 
	                                                       AttributeInterpretation_Position));

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
	
	static const QVector<StelVertexAttribute> attributes;
};
const QVector<StelVertexAttribute> TestVertex2::attributes = 
	(QVector<StelVertexAttribute>() << StelVertexAttribute(AttributeType_Vec3f, 
	                                                       AttributeInterpretation_Position)
	                                << StelVertexAttribute(AttributeType_Vec2f, 
	                                                       AttributeInterpretation_TexCoord));

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
	
	static const QVector<StelVertexAttribute> attributes;
};
const QVector<StelVertexAttribute> TestVertex3::attributes = 
	(QVector<StelVertexAttribute>() << StelVertexAttribute(AttributeType_Vec3f, 
	                                                       AttributeInterpretation_Position)
	                                << StelVertexAttribute(AttributeType_Vec2f, 
	                                                       AttributeInterpretation_TexCoord)
	                                << StelVertexAttribute(AttributeType_Vec3f,
	                                                       AttributeInterpretation_Normal)
	                                << StelVertexAttribute(AttributeType_Vec4f,
	                                                       AttributeInterpretation_Color));
	
template<class BufferBackend, class Vertex> 
void TestStelVertexBuffer::testVertexBuffer()
{
	StelVertexBuffer<Vertex>* buffer = 
		new StelVertexBuffer<Vertex>(new BufferBackend(PrimitiveType_Triangles, 
		                                               Vertex::attributes));
	
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

void TestStelVertexBuffer::testStelTestQGL2VertexBuffer()
{
	testVertexBuffer<StelTestQGL2VertexBufferBackend, TestVertex1>();
	testVertexBuffer<StelTestQGL2VertexBufferBackend, TestVertex2>();
	testVertexBuffer<StelTestQGL2VertexBufferBackend, TestVertex3>();
}
