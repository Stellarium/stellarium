#ifndef _STELGLUTILITYFUNCTIONS_HPP_
#define _STELGLUTILITYFUNCTIONS_HPP_

#include <QGLContext>
#include <QString>

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

//! Get OpenGL primitive type corresponding to specified PrimitiveType.
GLint glPrimitiveType(const PrimitiveType type);

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

#endif // _STELGLUTILITYFUNCTIONS_HPP_
