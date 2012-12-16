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

#ifndef _TRIANGLEITERATOR_HPP_
#define _TRIANGLEITERATOR_HPP_

#include <QVector>

// For PrimitiveType
#include "renderer/StelVertexBuffer.hpp"


//! Allows iteration over triangles in QVectors of vertices.
//!
//! How the vertices are iterated depends on specified primitive type.
//! Triangles iterated will match triangles that would be drawn 
//! if the vertices were drawn with specified primitive type.
//!
//! Used by StelSphereGeometry classes.
template<class V>
class TriangleIterator
{
public:
	//! Construct a TriangleIterator iterating over specified vertex array.
	//!
	//! @param vertices      Vertices to iterate over.
	//! @param primitiveType Primitive type to determine how the vertices form triangles.
	//!                      Must be PrimitiveType_Triangles, PrimitiveType_TriangleStrip or 
	//!                      PrimitiveType_TriangleFan.
	//!
	//! Vertex count can be 0, but can't be 1 or 2. If primitiveType is PrimitiveType_Triangles,
	//! vertex count must be divisible by 3.
	TriangleIterator(const QVector<V>& vertices, const PrimitiveType primitiveType)
		: vertices(vertices)
		, primitiveType(primitiveType)
		, currentVertex(0)
	{
		Q_ASSERT_X(vertices.size() != 1 && vertices.size() != 2, Q_FUNC_INFO,
		           "A vertex array containing triangles of any primitive type must not "
		           "have 1 or 2 vertices");
		switch(primitiveType)
		{
			case PrimitiveType_Triangles:
				Q_ASSERT_X(vertices.size() % 3 == 0, Q_FUNC_INFO,
				           "Size of a vertex array of triangles must be divisible by 3");
				currentVertex = 0;
				break;
			case PrimitiveType_TriangleStrip:
				currentVertex = 2;
				break;
			case PrimitiveType_TriangleFan:
				currentVertex = 1;
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Not a triangle primitive type");
		}
	}

	//! Get the next triangle, if any.
	//!
	//! @param a The first vertex of the triangle will be written here.
	//! @param b The second vertex of the triangle will be written here.
	//! @param c The third vertex of the triangle will be written here.
	//!
	//! @return true if we've got the triangle, false if there are no more triangles
	//!         (in this case a, b and c are unchanged);
	bool next(V& a, V& b, V& c)
	{
		Q_ASSERT_X(currentVertex > 0, Q_FUNC_INFO, "Current vertex is negative");
		Q_ASSERT_X(currentVertex <= vertices.size(), Q_FUNC_INFO,
		           "Iterating outside the vertex array");
		if(currentVertex == vertices.size()){return false;}

		switch(primitiveType)
		{
			case PrimitiveType_Triangles:
				a = vertices[currentVertex++];
				b = vertices[currentVertex++];
				c = vertices[currentVertex++];
				break;
			case PrimitiveType_TriangleStrip:
				if(currentVertex % 2 == 0)
				{
					a = vertices[currentVertex - 2];
					b = vertices[currentVertex - 1];
				}
				else
				{
					a = vertices[currentVertex - 1];
					b = vertices[currentVertex - 2];
				}
				c = vertices[currentVertex];
				++currentVertex;
				break;
			case PrimitiveType_TriangleFan:
				a = vertices[0];
				b = vertices[currentVertex];
				c = vertices[currentVertex + 1];
				++currentVertex;
				break;
			default:
				Q_ASSERT_X(false, Q_FUNC_INFO, "Not a triangle primitive type");
		}

		return true;
	}

private:
	//! Vertices we're iterating over;
	const QVector<V>& vertices;

	//! Primitive type of the vertex array.
	const PrimitiveType primitiveType;

	//! First vertex of the next triangle to iterate over.
	int currentVertex;
};

#endif // _TRIANGLEITERATOR_HPP_
