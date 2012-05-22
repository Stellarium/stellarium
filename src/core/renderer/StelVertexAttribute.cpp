
#include <Qt> 

#include "VecMath.hpp"
#include "StelVertexAttribute.hpp"

int attributeDimensions(const AttributeType type)
{
	switch(type)
	{
		case AT_Vec4f: return 4;
		case AT_Vec3f: return 3;
		case AT_Vec2f: return 2;
	}
	Q_ASSERT_X(false, "Unknown vertex attribute type", "attributeDimensions");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

int attributeSize(const AttributeType type)
{
	switch(type)
	{
		case AT_Vec2f: return sizeof(Vec2f);
		case AT_Vec3f: return sizeof(Vec3f);
		case AT_Vec4f: return sizeof(Vec4f);
	}
	Q_ASSERT_X(false, "Unknown vertex attribute type", "attributeSize");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}
