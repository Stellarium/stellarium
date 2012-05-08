#ifndef _VERTEXATTRIBUTE_HPP_
#define _VERTEXATTRIBUTE_HPP_

//! Vertex attribute types. 
//!
//! Used to specify data type of a vertex attribute in a vertex struct used with VertexBuffer.
enum AttributeType
{
	//! Represents Vec4f (4D float vector).
	AT_Vec4f,
	//! Represents Vec3f (3D float vector).
	AT_Vec3f,
	//! Represents Vec2f (2D float vector).
	AT_Vec2f
};

//! Get size of a vertex attribute of specified type.
//!
//! @param type Attribute type to get size of (e.g. Vec2f).
//! @return Size of the vertex attribute in bytes.
static size_t attributeSize(const AttributeType type)
{
	switch(type)
	{
		case AT_Vec2f: return sizeof(Vec2f);
		case AT_Vec3f: return sizeof(Vec3f);
		case AT_Vec4f: return sizeof(Vec4f);
	}
	Q_ASSERT_X(false, "Unknown vertex attribute type",
				"StelQGLVertexBuffer::attributeSize");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

//! Vertex attribute interpretations. 
//!
//! Used to specify how Renderer should interpret a vertex attribute
//! in a vertex struct used with VertexBuffer.
enum AttributeInterpretation
{
	//! Vertex position.
	Position,
	//! Color of the vertex.
	Color,
	//! Normal of the vertex.
	Normal,
	//! Texture coordinate of the vertex.
	TexCoord
};

//! Describes a single vertex attribute (e.g. 2D vertex, 3D normal and so on).
struct VertexAttribute
{
	//! Data type of the attribute.
	AttributeType type;
	//! Specifies how Renderer should interpret this attribute.
	AttributeInterpretation interpretation;
	
	//! Default ctor so vectors can reallocate.
	VertexAttribute(){}
	
	//! Construct a VertexAttribute.
	//!
	//! @param type Data type of the attribute.
	//! @param interpretation How the Renderer should interpret this attribute.
	VertexAttribute(const AttributeType type, const AttributeInterpretation interpretation)
		:type(type)
		,interpretation(interpretation)
	{
	}
};

#endif // _VERTEXATTRIBUTE_HPP_
