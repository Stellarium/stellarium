#include "StelGLUtilityFunctions.hpp"

int attributeGLType(const AttributeType type)
{
	switch(type)
	{
		case AT_Vec2f:
		case AT_Vec3f:
		case AT_Vec4f:
			return GL_FLOAT;
	}
	Q_ASSERT_X(false, "Unknown vertex attribute type", "attributeGLType");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

const char* attributeGLSLName(const AttributeInterpretation interpretation)
{
	switch(interpretation)
	{
		case Position: return "vertex";
		case TexCoord: return "texCoord";
		case Normal:   return "normal";
		case Color:    return "color";
	}
	Q_ASSERT_X(false, "Unknown vertex attribute interpretation", "attributeGLSLName");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return NULL;
}

int primitiveGLType(const PrimitiveType type)
{
	switch(type)
	{
		case Points:        return GL_POINTS;
		case Triangles:     return GL_TRIANGLES;
		case TriangleStrip: return GL_TRIANGLE_STRIP;
	}
	Q_ASSERT_X(false, "Unknown graphics primitive type", "primitiveGLType");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}
