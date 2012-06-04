#ifndef _STELVERTEXATTRIBUTE_HPP_
#define _STELVERTEXATTRIBUTE_HPP_

//! Vertex attribute types. 
//!
//! Used to specify data type of a vertex attribute in a vertex struct used with VertexBuffer.
enum AttributeType
{
	//! Represents Vec4f (4D float vector).
	AttributeType_Vec4f,
	//! Represents Vec3f (3D float vector).
	AttributeType_Vec3f,
	//! Represents Vec2f (2D float vector).
	AttributeType_Vec2f
};

//! Return the number of dimensions of a vertex attribute type.
//!
//! E.g. Vec3f is 3-dimensional.
//!
//! @param type Attribute type to get number of dimensions of.
//! @return Number of dimensions.
int attributeDimensions(const AttributeType type);

//! Get size of a vertex attribute of specified type.
//!
//! @param type Attribute type to get size of (e.g. Vec2f).
//! @return Size of the vertex attribute in bytes.
int attributeSize(const AttributeType type);

//! Vertex attribute interpretations. 
//!
//! Used to specify how Renderer should interpret a vertex attribute
//! in a vertex struct used with VertexBuffer.
enum AttributeInterpretation
{
	//! Vertex position.
	AttributeInterpretation_Position,
	//! Color of the vertex.
	AttributeInterpretation_Color,
	//! Normal of the vertex.
	AttributeInterpretation_Normal,
	//! Texture coordinate of the vertex.
	AttributeInterpretation_TexCoord
};

//! Describes a single vertex attribute (e.g. 2D vertex, 3D normal and so on).
struct StelVertexAttribute
{
	//! Data type of the attribute.
	AttributeType type;
	//! Specifies how Renderer should interpret this attribute.
	AttributeInterpretation interpretation;
	
	//! Default ctor so vectors can reallocate.
	StelVertexAttribute(){}
	
	//! Construct a StelVertexAttribute.
	//!
	//! @param type Data type of the attribute.
	//! @param interpretation How the Renderer should interpret this attribute.
	StelVertexAttribute(const AttributeType type, const AttributeInterpretation interpretation)
		:type(type)
		,interpretation(interpretation)
	{
	}
};

#endif // _STELVERTEXATTRIBUTE_HPP_
