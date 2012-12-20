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

#include "StelGLCompatibility.hpp"
#include "StelRenderer.hpp"
#include "StelIndexBuffer.hpp"
#include "StelTextureParams.hpp"
#include "StelVertexAttribute.hpp"
#include "StelVertexBuffer.hpp"


//! Get OpenGL data type of a component of an attribute with specified attribute type.
//!
//! E.g. Vec3f is a vector of 3 floats, so GL data type of AttributeType_Vec3f is GL_FLOAT.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
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
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
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
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute interpretation");
	}
	
	// Prevents GCC from complaining about exiting a non-void function:
	return NULL;
}


#ifndef USE_OPENGL_ES2
//! Get the enum value matching specified attribute interpreation used to enable 
//! GL1 vertex array client state.
//!
//! Used with glEnableClientState()/glDisableClientState(),
//! so the value returned can be e.g. GL_VERTEX_ARRAY for positions,
//! GL_COLOR_ARRAY for colors, etc.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
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
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute interpretation");
	}
	
	// Prevents GCC from complaining about exiting a non-void function:
	return GL_VERTEX_ARRAY;
}
#endif //USE_OPENGL_ES2

//! Translate PrimitiveType to OpenGL primitive type.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
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
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline GLenum glIndexType(const IndexType indexType)
{
	if(indexType == IndexType_U16)      {return GL_UNSIGNED_SHORT;}
	else if(indexType == IndexType_U32) {return GL_UNSIGNED_INT;}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

//! Translate TextureWrap to OpenGL texture wrap mode.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
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
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
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

//! Get GL internal (on-GPU) pixel format (internalFormat passed to glTexImage2D()) of an image.
//! 
//! Qt handles image conversion to a compatible in-RAM format before uploading to the GPU.
//! Changes might be needed if we replace Qt texture functions with our own.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline GLint glGetTextureInternalFormat(const QImage& image)
{
	const bool gray  = image.isGrayscale();
	const bool alpha = image.hasAlphaChannel();
#ifndef USE_OPENGL_ES2
	if(gray) {return alpha ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE8;}
	else     {return alpha ? GL_RGBA8 : GL_RGB8;}
#else
    if(gray) {return alpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;}
    else     {return alpha ? GL_RGBA : GL_RGB;}
#endif
}

//! Get GL internal (on-GPU) pixel format (internalFormat passed to glTexImage2D())
//! corresponding to a TextureDataFormat.
//! 
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline GLint glGetTextureInternalFormat(const TextureDataFormat format)
{
	switch(format)
	{
		case TextureDataFormat_RGBA_F32: return GL_RGBA32F; break;
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture data format");
	}
	// Avoid compiler warnings
	return -1;
}

//! Return GL format of pixels to be uploaded to a GPU.
//!
//! This needs to match the value Qt will use when calling glTexImage2D()
//! (Qt might internally convert the image). Changes might be needed if we 
//! replace Qt texture functions with our own.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline GLenum glGetTextureLoadFormat(const QImage& image)
{
	const bool gray  = image.isGrayscale();
	const bool alpha = image.hasAlphaChannel();
	if(gray) {return alpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;}
	else     {return alpha ? GL_RGBA : GL_RGB;}
}

//! Return GL format of pixels to be uploaded to a GPU corresponding to a TextureDataFormat.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline GLint glGetTextureLoadFormat(const TextureDataFormat format)
{
	switch(format)
	{
		case TextureDataFormat_RGBA_F32: return GL_RGBA; break;
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture data format");
	}
	// Avoid compiler warnings
	return -1;
}

//! Return GL data type of data in pixels of an image.
//!
//! This needs to match the value Qt will use when calling glTexImage2D()
//! (Qt might internally convert the image). Changes might be needed if we 
//! replace Qt texture functions with our own.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline GLenum glGetTextureType(const QImage& image)
{
	Q_UNUSED(image);
	return GL_UNSIGNED_BYTE;
}

//! Return GL data type of data in pixels corresponding to a TextureDataFormat.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline GLint glGetTextureType(const TextureDataFormat format)
{
	switch(format)
	{
		case TextureDataFormat_RGBA_F32: return GL_FLOAT; break;
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture data format");
	}
	// Avoid compiler warnings
	return -1;
}

//! Determine if specified texture size is within maximum texture size limits.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline bool glTextureSizeWithinLimits(const QSize size, const TextureDataFormat format)
{
#ifndef USE_OPENGL_ES2
	const GLint internalFormat = glGetTextureInternalFormat(format);
	const GLenum loadFormat    = glGetTextureLoadFormat(format);
	const GLenum type          = glGetTextureType(format);

	glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internalFormat, size.width(), size.height(), 0,
	             loadFormat, type, NULL);

	GLint width  = size.width();
	GLint height = size.height();
	glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &height);

	return width != 0 && height != 0;
#else
    Q_UNUSED(format)
    //ES2 doesn't have proxy textures, nor fetching a texture's height/width; can only rely on GL_MAX_TEXTURE_SIZE
    GLint maxDim;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxDim);
    return size.width() <= maxDim && size.height() <= maxDim;
#endif
}

//! Determine if an image is not too large for the GPU.
//!
//! Maximum texture size depends on image format; this function handles that.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
inline bool glTextureSizeWithinLimits(const QImage& image)
{
#ifndef USE_OPENGL_ES2
	const GLint  internalFormat = glGetTextureInternalFormat(image);
	const GLenum loadFormat     = glGetTextureLoadFormat(image);
	const GLenum type           = glGetTextureType(image);

	glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internalFormat, image.width(), image.height(), 0,
	             loadFormat, type, NULL);

	GLint width  = image.width();
	GLint height = image.height();
	glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &height);

	return width != 0 && height != 0;
#else
    //ES2 doesn't have proxy textures, nor fetching a texture's height/width; can only rely on GL_MAX_TEXTURE_SIZE
    GLint maxDim;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxDim);
    return image.width() <= maxDim && image.height() <= maxDim;
#endif
}

//! Ensure that an image is within maximum texture size limits, shrinking it if needed.
//!
//! The image will be replaced by a resized version if it doesn't fit 
//! into maximum GL texture size on the platform.
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
//!
//! @param image Referernce to the image.
inline void glEnsureTextureSizeWithinLimits(QImage& image)
{
	while(!glTextureSizeWithinLimits(image))
	{
		if(image.width() <= 1)
		{
			Q_ASSERT_X(false, Q_FUNC_INFO, "Even a texture with width <= 1 is \"too large\": "
			           "maybe image format is invalid/not GL supported?");
		}
		image = image.scaledToWidth(image.width() / 2, Qt::FastTransformation);
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
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
//!
//! @param filename File name of the texture.
//! @param pvrSupported Are PVR textures supported?
//!
//! @return Full path of the texture.
QString glFileSystemTexturePath(const QString& filename, const bool pvrSupported);

//! Check for OpenGL errors and print warnings if an error is detected. 
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
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
