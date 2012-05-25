#ifndef _STELGLUTILITYFUNCTIONS_HPP_
#define _STELGLUTILITYFUNCTIONS_HPP_

#include <QGLContext>
#include "StelVertexAttribute.hpp"
#include "StelTextureParams.hpp"
#include "StelVertexBuffer.hpp"


//! Get OpenGL data type of an element of an attribute type.
//!
//! E.g. Vec3f is a vector of 3 floats, so it's GL data type is GL_FLOAT.
//!
//! @param type Vertex attribute data type.
//! @return OpenGL attribute element data type.
GLint attributeGLType(const AttributeType type);

//! Get GLSL vertex attribute name corresponding to specified attribute interpretation.
//!
//! Each vertex attribute interpretation uses a specific name in shaders it is used in.
//!
//! @param interpretation Vertex attribute interpretation.
//! @return Name of the attribute as a C string.
const char* attributeGLSLName(const AttributeInterpretation interpretation);

//! Get OpenGL primitive type corresponding to specified PrimitiveType.
GLint primitiveGLType(const PrimitiveType type);

//! Get OpenGL texture wrap mode corresponding to specified TextureWrap.
GLint textureWrapGL(const TextureWrap wrap);

#endif // _STELGLUTILITYFUNCTIONS_HPP_
