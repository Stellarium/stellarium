#ifndef _STELGLUTILITYFUNCTIONS_HPP_
#define _STELGLUTILITYFUNCTIONS_HPP_

#include <QGLContext>
#include "StelVertexAttribute.hpp"


//! Get OpenGL data type of an element of an attribute type.
//!
//! E.g. Vec3f is a vector of 3 floats, so it's GL data type is GL_FLOAT.
//!
//! @param type Vertex attribute data type.
//! @return OpenGL attribute element data type.
int attributeGLType(const AttributeType type);

#endif // _STELGLUTILITYFUNCTIONS_HPP_
