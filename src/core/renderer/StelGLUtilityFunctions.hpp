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


//! Get OpenGL data type of an element of an attribute type.
//!
//! E.g. Vec3f is a vector of 3 floats, so it's GL data type is GL_FLOAT.
//!
//! @param type Vertex attribute data type.
//! @return OpenGL attribute element data type.
GLint glAttributeType(const AttributeType type);

//! Get GLSL vertex attribute name corresponding to specified attribute interpretation.
//!
//! Each vertex attribute interpretation uses a specific name in shaders it is used in.
//!
//! @param interpretation Vertex attribute interpretation.
//! @return Name of the attribute as a C string.
const char* glslAttributeName(const AttributeInterpretation interpretation);


//! Get the enum value matching specified attribute interpreation used to enable GL1 client state.
//!
//! Used with glEnableClientState()/glDisableClientState(),
//! so the value returned can be e.g. GL_VERTEX_ARRAY for positions,
//! GL_COLOR_ARRAY for colors, etc.
//!
//! @param interpretation Vertex attribute interpretation.
//! @return Enum value corresponding to the interpretation.
GLenum gl1AttributeEnum(const AttributeInterpretation interpretation);

//! Get OpenGL primitive type corresponding to specified PrimitiveType.
GLint glPrimitiveType(const PrimitiveType type);

//! Get OpenGL index type corresponding to specified IndexType.
GLenum glIndexType(const IndexType type);

//! Get OpenGL texture wrap mode corresponding to specified TextureWrap.
GLint glTextureWrap(const TextureWrap wrap);

//! Convert an OpenGL error code returned by glGetError to string.
QString glErrorToString(const GLenum error);

//! Get OpenGL pixel format of an image.
GLint glGetPixelFormat(const QImage& image);

//! Ensure that an image is within maximum texture size limits, shrinking it if needed.
//!
//! The image will be replaced by a resized version if it doesn't fit 
//! into maximum GL texture size on the platform.
//!
//! @param image Referernce to the image.
void glEnsureTextureSizeWithinLimits(QImage& image);

//! Get filesystem path of an OpenGL texture.
//! 
//! GL-specific to e.g. allow PVR (GLES compression format) support.
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

//! Check for any OpenGL errors and warn if needed. Useful for detecting incorrect GL code.
void checkGLErrors();

#endif // _STELGLUTILITYFUNCTIONS_HPP_
