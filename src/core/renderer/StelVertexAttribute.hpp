/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELVERTEXATTRIBUTE_HPP_
#define _STELVERTEXATTRIBUTE_HPP_

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVector>

#include "VecMath.hpp"

//! Vertex attribute data types. 
//!
//! Used to specify data type of a vertex attribute in a vertex struct used with VertexBuffer.
enum AttributeType
{
	//! Represents Vec4f (4D float vector).
	AttributeType_Vec4f = 0,
	//! Represents Vec3f (3D float vector).
	AttributeType_Vec3f = 1,
	//! Represents Vec2f (2D float vector).
	AttributeType_Vec2f = 2,
	//! Must be greater than all other AttributeType values.
	AttributeType_Max   = 3
};

//! Return the number of dimensions of a vertex attribute type.
//!
//! E.g. Vec3f is 3-dimensional.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
//!
//! @param type Attribute type to get number of dimensions of.
//! @return Number of dimensions.
inline int attributeDimensions(const AttributeType type)
{
	static const int attributeDimensionsArray [AttributeType_Max] =
		{4, 3, 2};
	return attributeDimensionsArray[type];
}

//! Get size of a vertex attribute of specified type in bytes.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
//!
//! @param type Attribute type to get size of (e.g. Vec2f).
//! @return Size of the vertex attribute in bytes.
inline int attributeSize(const AttributeType type)
{
	static const int attributeSizeArray [AttributeType_Max] = 
		{sizeof(Vec4f), sizeof(Vec3f), sizeof(Vec2f)};
	return attributeSizeArray[type];
}

//! Vertex attribute interpretations. 
//!
//! Used to specify how Renderer should interpret a vertex attribute
//! in a vertex struct used with VertexBuffer.
enum AttributeInterpretation
{
	//! Vertex position.
	AttributeInterpretation_Position = 0,
	//! Color of the vertex.
	AttributeInterpretation_Color    = 1,
	//! Normal of the vertex.
	AttributeInterpretation_Normal   = 2,
	//! Texture coordinate of the vertex.
	AttributeInterpretation_TexCoord = 3,
	//! Never used in an attribute; used to determine maximum number of attributes.
	AttributeInterpretation_MAX
};

//! Describes a single vertex attribute (e.g. 2D vertex, 3D normal and so on).
//!
//! @note This is an internal struct of the Renderer subsystem and should not be used elsewhere.
struct StelVertexAttribute
{
	//! Data type of the attribute.
	AttributeType type;
	//! Specifies how Renderer should interpret this attribute.
	AttributeInterpretation interpretation;
	
	//! Default constructor so vectors can reallocate.
	StelVertexAttribute(){}
	
	//! Construct a StelVertexAttribute from a string.
	//!
	//! The string must be in format "Type Interpretation" where <em>Type</em> matches
	//! a value of AttributeType enum without the "AttributeType_" prefix, and 
	//! <em>Interpretation</em> matches a value of AttributeInterpretation enum without 
	//! the "AttributeInterpretation_" prefix.
	StelVertexAttribute(const QString& attributeString)
	{
		const QString typeStr = 
			attributeString.section(' ', 0, 0, QString::SectionSkipEmpty);
		const QString interpretationStr = 
			attributeString.section(' ', 1, 1, QString::SectionSkipEmpty);

		if(typeStr == "Vec2f")      {type = AttributeType_Vec2f;}
		else if(typeStr == "Vec3f") {type = AttributeType_Vec3f;}
		else if(typeStr == "Vec4f") {type = AttributeType_Vec4f;}
		else 
		{
			qDebug() << "Unknown vertex attribute type: \"" << typeStr 
			         << "\" in attribute \"" << attributeString << "\"";
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute type");
		}

		if(interpretationStr == "Position")      
		{
			interpretation = AttributeInterpretation_Position;
		}
		else if(interpretationStr == "Color") 
		{
			interpretation = AttributeInterpretation_Color;
		}
		else if(interpretationStr == "Normal") 
		{
			interpretation = AttributeInterpretation_Normal;
		}
		else if(interpretationStr == "TexCoord") 
		{
			interpretation = AttributeInterpretation_TexCoord;
		}
		else 
		{
			qDebug() << "Unknown vertex attribute interpretation: \"" << interpretationStr 
			         << "\" in attribute \"" << attributeString << "\"";
			Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute interpretation");
		}
	}

	//! Parse a string of comma separated vertex attributes to a vector of StelVertexAttribute.
	static QVector<StelVertexAttribute> parseAttributes(const char* const attribsCString)
	{
		QString attributes(attribsCString);
		QVector<StelVertexAttribute> result;
		const QStringList parts = attributes.split(",");
		for(int part = 0; part < parts.size(); ++part)
		{
			result << StelVertexAttribute(parts.at(part));
		}

#ifndef NDEBUG
		validateAttributes(result);
#endif
		return result;
	}

private:
#ifndef NDEBUG
	//! Assert that vertex attributes form a usable vertex format.
	//!
	//! For instance, assert that there is no more than 1 attribute with
	//! a particular interpretation, that we have a position attribute, and so on.
	static void validateAttributes(const QVector<StelVertexAttribute>& attributes)
	{
		bool vertex, texCoord, normal, color;
		vertex = texCoord = normal = color = false;
		
		// Ensure that every kind of vertex attribute is present at most once.
		foreach(const StelVertexAttribute& attribute, attributes)
		{
			switch(attribute.interpretation)
			{
				case AttributeInterpretation_Position:
					Q_ASSERT_X(!vertex, Q_FUNC_INFO,
					           "Vertex type has more than one vertex position attribute");
					vertex = true;
					break;
				case AttributeInterpretation_TexCoord:
					Q_ASSERT_X(attributeDimensions(attribute.type) == 2, Q_FUNC_INFO,
					           "Only 2D texture coordinates are supported at the moment");
					Q_ASSERT_X(!texCoord, 
					           Q_FUNC_INFO,
					           "Vertex type has more than one texture coordinate attribute");
					texCoord = true;
					break;
				case AttributeInterpretation_Normal:
					Q_ASSERT_X(attributeDimensions(attribute.type) == 3, Q_FUNC_INFO,
					           "Only 3D vertex normals are supported");
					Q_ASSERT_X(!normal, Q_FUNC_INFO,
					           "Vertex type has more than one normal attribute");
					normal = true;
					break;
				case AttributeInterpretation_Color:
					Q_ASSERT_X(!color, Q_FUNC_INFO,
					           "Vertex type has more than one color attribute");
					color = true;
					break;
				default:
					Q_ASSERT_X(false, Q_FUNC_INFO,
					           "Unknown vertex attribute interpretation");
			}
		}

		Q_ASSERT_X(vertex, Q_FUNC_INFO, 
		           "Vertex formats without a position attribute are not supported");
	}
#endif
};

//! Generate vertex attribute information for a vertex type.
//!
//! NOTE: This depends on variadic macros - a C99 feature.
//! Also supported in MSVC (which is not C99 compliant) and C++11.
//!
//! This macro specifies data type and interpretation of each attribute.
//! Data type can be <em>Vec2f</em>, <em>Vec3f</em> or <em>Vec4f</em>.
//! Interpretation can be <em>Position</em>, <em>TexCoord</em>, <em>Normal</em> 
//! or <em>Color</em>.
//! Two attributes must never have the same interpretation
//! (this is asserted by the vertex buffer backend at run time).
//! 
//! Note:
//! Attribute interpretations are processed at runtime, so if there 
//! is an error here, Stellarium will crash with an assertion failure and an 
//! error message.
//! 
//! (C++11 TODO) This might be changed with constexpr
#define VERTEX_ATTRIBUTES(...)\
	static const QVector<StelVertexAttribute>& attributes()\
	{\
		static QVector<StelVertexAttribute> attribs =\
			StelVertexAttribute::parseAttributes(#__VA_ARGS__);\
		return attribs;\
	}

#endif // _STELVERTEXATTRIBUTE_HPP_
