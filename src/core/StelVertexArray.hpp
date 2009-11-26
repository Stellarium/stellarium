/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __STELVERTEXARRAY_HPP__
#define __STELVERTEXARRAY_HPP__

#include <QVector>
#include <QDebug>
#include "VecMath.hpp"

struct StelVertexArray
{
	// TODO maybe merge this with StelPainter DrawingMode
	enum StelPrimitiveType
	{
		Points                      = 0x0000, // GL_POINTS
		Lines                       = 0x0001, // GL_LINES
		LineLoop                    = 0x0002, // GL_LINE_LOOP
		LineStrip                   = 0x0003, // GL_LINE_STRIP
		Triangles                   = 0x0004, // GL_TRIANGLES
		TriangleStrip               = 0x0005, // GL_TRIANGLE_STRIP
		TriangleFan                 = 0x0006  // GL_TRIANGLE_FAN
	};

	StelVertexArray(StelPrimitiveType pType=StelVertexArray::Triangles) : useIndice(false), primitiveType(pType) {;}
	StelVertexArray(const QVector<Vec3d>& v, StelPrimitiveType pType=StelVertexArray::Triangles,const QVector<Vec2f>& t=QVector<Vec2f>(), const QVector<unsigned int> i=QVector<unsigned int>()) :
		vertex(v), texCoords(t), indices(i), useIndice(!i.empty()), primitiveType(pType) {;}

	//! OpenGL compatible array of 3D vertex to be displayed using vertex arrays.
	//! TODO, move to float? Most of the vectors are normalized, thus the precision is around 1E-45 using float
	//! which is enough (=2E-37 milli arcsec).
	QVector<Vec3d> vertex;
	//! OpenGL compatible array of edge flags to be displayed using vertex arrays.
	QVector<Vec2f> texCoords;

	QVector<unsigned int> indices;
	bool useIndice;

	StelPrimitiveType primitiveType;

	const Vec3d& vertexAt(int i) const {
		return useIndice ? vertex.constData()[indices.at(i)] : vertex.constData()[i];
	}
	const Vec2f& texCoordAt(int i) const {
		return useIndice ? texCoords.constData()[indices.at(i)] : texCoords.constData()[i];
	}

	//! call a function for each triangle of the array.
	//! func should define the following method :
	//!     void operator() (const Vec3d* vertex[3], const Vec2f* tex[3], unsigned int indices[3])
	//! The method takes arrays of *pointers* as arguments because we can't assume the values are contiguous
	template<class Func>
	inline void foreachTriangle(Func& func) const;
};


template<class Func>
void StelVertexArray::foreachTriangle(Func& func) const
{
	const Vec3d* ar[3];
	const Vec2f* te[3];
	unsigned int indices[3];

	Q_ASSERT(texCoords.size() == vertex.size());

	switch (primitiveType)
	{
		case StelVertexArray::Triangles:
			Q_ASSERT(vertex.size() % 3 == 0);
			for (int i = 0; i < vertex.size() / 3; ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					ar[j] = &vertexAt(i * 3 + j);
					te[j] = &texCoordAt(i * 3 + j);
					indices[j] = i * 3 + j;
				}
				func(ar, te, indices);
			}
			break;
		case StelVertexArray::TriangleFan:
		{
			ar[0]=&vertexAt(0);
			te[0]=&texCoordAt(0);
			indices[0] = 0;
			for (int i = 1; i < vertex.size() - 1; ++i)
			{
				ar[1] = &vertexAt(i);
				ar[2] = &vertexAt(i+1);
				te[1] = &texCoordAt(i);
				te[2] = &texCoordAt(i+1);
				indices[1] = i;
				indices[2] = i+1;
				func(ar, te, indices);
			}
			break;
		}
		default:
			Q_ASSERT_X(0, Q_FUNC_INFO, "unsuported primitive type");
	}
}

#endif // __STELVERTEXARRAY_HPP__
