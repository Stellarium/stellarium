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
