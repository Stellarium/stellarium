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

#ifndef _STELGLUTILITYFUNCTIONS_HPP_
#define _STELGLUTILITYFUNCTIONS_HPP_

#include <QGLContext>
#include <QString>

#include "StelIndexBuffer.hpp"
#include "StelTextureParams.hpp"
#include "StelVertexAttribute.hpp"
#include "StelVertexBuffer.hpp"


//! Get OpenGL data type of a component of an attribute with specified attribute type.
//!
//! E.g. Vec3f is a vector of 3 floats, so GL data type of AttributeType_Vec3f is GL_FLOAT.
//!
//! @param type Vertex attribute data type.
//! @return OpenGL attribute element data type.
inline GLint glAttributeType(const AttributeType type)
{
	switch(type)
	{
		case AttributeType_Vec2f:
		case AttributeType_Vec3f:
		case AttributeType_Vec4f:
			return GL_FLOAT;
		default:
			break;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute type");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}


//! Get GLSL vertex attribute name corresponding to specified attribute interpretation.
//!
//! Each vertex attribute interpretation uses a specific name in shaders it is used in.
//!
//! @param interpretation Vertex attribute interpretation.
//! @return Name of the attribute as a C string.
inline const char* glslAttributeName(const AttributeInterpretation interpretation)
{
	switch(interpretation)
	{
		case AttributeInterpretation_Position: return "vertex";
		case AttributeInterpretation_TexCoord: return "texCoord";
		case AttributeInterpretation_Normal:   return "normal";
		case AttributeInterpretation_Color:    return "color";
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute interpretation");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return NULL;
}


//! Get the enum value matching specified attribute interpreation used to enable 
//! GL1 vertex array client state.
//!
//! Used with glEnableClientState()/glDisableClientState(),
//! so the value returned can be e.g. GL_VERTEX_ARRAY for positions,
//! GL_COLOR_ARRAY for colors, etc.
//!
//! @param interpretation Vertex attribute interpretation.
//! @return Client state corresponding to the interpretation.
inline GLenum gl1AttributeEnum(const AttributeInterpretation interpretation)
{
	switch(interpretation)
	{
		case AttributeInterpretation_Position: return GL_VERTEX_ARRAY;
		case AttributeInterpretation_TexCoord: return GL_TEXTURE_COORD_ARRAY;
		case AttributeInterpretation_Normal:   return GL_NORMAL_ARRAY;
		case AttributeInterpretation_Color:    return GL_COLOR_ARRAY;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute interpretation");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return GL_VERTEX_ARRAY;
}

//! Translate PrimitiveType to OpenGL primitive type.
inline GLint glPrimitiveType(const PrimitiveType type)
{
	switch(type)
	{
		case PrimitiveType_Points:        return GL_POINTS;
		case PrimitiveType_Triangles:     return GL_TRIANGLES;
		case PrimitiveType_TriangleStrip: return GL_TRIANGLE_STRIP;
		case PrimitiveType_TriangleFan:   return GL_TRIANGLE_FAN;
		case PrimitiveType_Lines:         return GL_LINES;
		case PrimitiveType_LineStrip:     return GL_LINE_STRIP;
		case PrimitiveType_LineLoop:      return GL_LINE_LOOP;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown graphics primitive type");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

//! Translate IndexType to OpenGL index type.
inline GLenum glIndexType(const IndexType indexType)
{
	if(indexType == IndexType_U16)      {return GL_UNSIGNED_SHORT;}
	else if(indexType == IndexType_U32) {return GL_UNSIGNED_INT;}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

//! Translate TextureWrap to OpenGL texture wrap mode.
inline GLint glTextureWrap(const TextureWrap wrap)
{
	switch(wrap)
	{
		case TextureWrap_Repeat:      return GL_REPEAT;
		case TextureWrap_ClampToEdge: return GL_CLAMP_TO_EDGE;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture wrap mode");

	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

//! Convert an OpenGL error code returned by glGetError to string.
inline QString glErrorToString(const GLenum error)
{
	switch(error)
	{
		case GL_NO_ERROR:                      return "GL_NO_ERROR";
		case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE"; 
		case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION"; 
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION"; 
		case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY"; 
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown GL error");

	// Prevents GCC from complaining about exiting a non-void function:
	return QString();
}

//! Get OpenGL pixel format of an image.
inline GLint glGetPixelFormat(const QImage& image)
{
	const bool gray  = image.isGrayscale();
	const bool alpha = image.hasAlphaChannel();
	if(gray) {return alpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;}
	else     {return alpha ? GL_RGBA : GL_RGB;}
}

//! Determine if specified texture size is within maximum texture size limits.
inline bool glTextureSizeWithinLimits(const QSize size)
{
	// TODO 
	// GL_MAX_TEXTURE_SIZE is buggy on some implementations,
	// and max texture size also depends on pixel format.
	// Implement a better solution based on texture proxy.
	// (determine if a texture with exactly this width, height, format fits.
	// If not, divide width and height by 2, try again. Ad infinitum, until 0)

	GLint maxSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);

	return size.width() < maxSize && size.height() < maxSize;
}

//! Ensure that an image is within maximum texture size limits, shrinking it if needed.
//!
//! The image will be replaced by a resized version if it doesn't fit 
//! into maximum GL texture size on the platform.
//!
//! @param image Referernce to the image.
inline void glEnsureTextureSizeWithinLimits(QImage& image)
{
	// TODO 
	// GL_MAX_TEXTURE_SIZE is buggy on some implementations,
	// and max texture size also depends on pixel format.
	// Implement a better solution based on texture proxy.
	// (determine if a texture with exactly this width, height, format fits.
	// If not, divide width and height by 2, try again. Ad infinitum, until 0)
	GLint size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);

	if (size < image.width())
	{
		image = image.scaledToWidth(size, Qt::FastTransformation);
	}
	if (size < image.height())
	{
		image = image.scaledToHeight(size, Qt::FastTransformation);
	}
}

//! Get filesystem path of an OpenGL texture.
//! 
//! This is GL-specific to e.g. allow PVR (compression format on some mobile GPUs) support.
//!
//! If PVR is supported, versions of textures with the .pvr extension are preferred. 
//!
//! If the texture is not found, it is searched for in the textures/ directory.
//!
//! @param filename File name of the texture.
//! @param pvrSupported Are PVR textures supported?
//!
//! @return Full path of the texture.
QString glFileSystemTexturePath(const QString& filename, const bool pvrSupported);

//! Check for OpenGL errors and print warnings if an error is detected. 
//!
//! @param context Context in which checkGLErrors is called used for error messages.
//!                (e.g. the function name).
inline void checkGLErrors(const QString& context)
{
	const GLenum glError = glGetError();
	if(glError == GL_NO_ERROR) {return;}

	qWarning() << "OpenGL error detected at " << context <<  " : " 
	           << glErrorToString(glError);
}

#endif // _STELGLUTILITYFUNCTIONS_HPP_
