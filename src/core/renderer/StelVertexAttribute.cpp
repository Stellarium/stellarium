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


#include <Qt> 

#include "VecMath.hpp"
#include "StelVertexAttribute.hpp"

int attributeDimensions(const AttributeType type)
{
	switch(type)
	{
		case AttributeType_Vec4f: return 4;
		case AttributeType_Vec3f: return 3;
		case AttributeType_Vec2f: return 2;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute type");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

int attributeSize(const AttributeType type)
{
	switch(type)
	{
		case AttributeType_Vec2f: return sizeof(Vec2f);
		case AttributeType_Vec3f: return sizeof(Vec3f);
		case AttributeType_Vec4f: return sizeof(Vec4f);
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute type");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}
