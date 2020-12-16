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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef STELVERTEXARRAY_HPP
#define STELVERTEXARRAY_HPP

#include "VecMath.hpp"

#include <QVector>
#include <QDebug>

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

	StelVertexArray(StelPrimitiveType pType=StelVertexArray::Triangles) : primitiveType(pType) {;}
	StelVertexArray(const QVector<Vec3d>& v, StelPrimitiveType pType=StelVertexArray::Triangles,const QVector<Vec2f>& t=QVector<Vec2f>(), const QVector<unsigned short> i=QVector<unsigned short>(),const QVector<Vec3f>& col=QVector<Vec3f>()) :
		vertex(v), texCoords(t), colors(col), indices(i), primitiveType(pType) {;}

	//! OpenGL compatible array of 3D vertex to be displayed using vertex arrays.
	//! TODO, move to float? Most of the vectors are normalized, thus the precision is around 1E-45 using float
	//! which is enough (=2E-37 milli arcsec).
	QVector<Vec3d> vertex;
	//! OpenGL compatible array of edge flags to be displayed using vertex arrays.
	QVector<Vec2f> texCoords;
	//! OpenGL compatible array of vertex colors to be displayed using arrays.
	//! The color (if exists) shall be multiplied with texture to modulate e.g. for extinction of Milky Way or other large items.
	QVector<Vec3f> colors;
	//! OpenGL compatible array of indices for the vertex and the textures
	QVector<unsigned short> indices;

	StelPrimitiveType primitiveType;

	bool isIndexed() const {return !indices.isEmpty();}

	bool isTextured() const {return !texCoords.isEmpty();}

	bool isColored() const {return !colors.isEmpty();}


	//! call a function for each triangle of the array.
	//! func should define the following method :
	//!     void operator() (const Vec3d* vertex[3], const Vec2f* tex[3], const Vec3f* colors[3], unsigned int indices[3])
	//! The method takes arrays of *pointers* as arguments because we can't assume the values are contiguous
	template<class Func>
	inline Func foreachTriangle(Func func) const;

	//! Create a copy of the array with all the triangles intersecting the projector discontinuity removed.
	StelVertexArray removeDiscontinuousTriangles(const class StelProjector* prj) const;

private:
	// Below we define a few methods that are templated to be optimized according to different types of VertexArray :
	// The template parameter <bool T> defines whether the array has a texture.
	// The template parameter <bool I> defines whether the array is indexed.
	// The template parameter <bool C> defines whether the array is colored.
	template <bool I>
	const Vec3d* specVertexAt(int i) const {
		return &vertex.at(specIndiceAt<I>(i));
	}

	template <bool T, bool I>
	const Vec2f* specTexCoordAt(int i) const {
		return T ? &texCoords.at(specIndiceAt<I>(i)) : Q_NULLPTR;
	}

	template <bool C, bool I>
	const Vec3f* specColorAt(int i) const {
		return C ? &colors.at(specIndiceAt<I>(i)) : Q_NULLPTR;
	}

	template<bool I>
	unsigned int specIndiceAt(unsigned int i) const {
		return I ? indices.at(i) : i;
	}

	template<bool T, bool I, bool C, class Func>
	inline Func specForeachTriangle(Func func) const;
};

// Serialization routines
QDataStream& operator<<(QDataStream& out, const StelVertexArray&);
QDataStream& operator>>(QDataStream& in, StelVertexArray&);

template<class Func>
Func StelVertexArray::foreachTriangle(Func func) const
{
	// Here we just dispatch the method into one of the 8 possible cases
	bool textured = isTextured();
	bool colored  = isColored();
	bool useIndice = isIndexed();

	if (textured)
	{
		if (useIndice)
		{
			if (colored)
				return specForeachTriangle<true, true, true, Func>(func);
			else
				return specForeachTriangle<true, true, false, Func>(func);
		}
		else // not indiced
		{
			if (colored)
				return specForeachTriangle<true, false, true, Func>(func);
			else
				return specForeachTriangle<true, false, false, Func>(func);
		}
	}
	else // not textured
	{
		if (useIndice)
		{
			if (colored)
				return specForeachTriangle<false, true, true, Func>(func);
			else
				return specForeachTriangle<false, true, false, Func>(func);
		}
		else // not indiced
		{
			if (colored)
				return specForeachTriangle<false, false, true, Func>(func);
			else
				return specForeachTriangle<false, false, false, Func>(func);
		}
	}
}

template<bool T, bool I, bool C, class Func>
Func StelVertexArray::specForeachTriangle(Func func) const
{
	switch (primitiveType)
	{
		case StelVertexArray::Triangles:
			Q_ASSERT(vertex.size() % 3 == 0);
			for (int i = 0; i < vertex.size(); i += 3)
			{
				func(specVertexAt<I>(i), specVertexAt<I>(i+1), specVertexAt<I>(i+2),
					 specTexCoordAt<T, I>(i), specTexCoordAt<T, I>(i+1), specTexCoordAt<T, I>(i+2),
					 specColorAt<C, I>(i), specColorAt<C, I>(i+1), specColorAt<C, I>(i+2),
					 specIndiceAt<I>(i), specIndiceAt<I>(i+1), specIndiceAt<I>(i+2));
			}
			break;
		case StelVertexArray::TriangleFan:
		{
			const Vec3d* v0 = specVertexAt<I>(0);
			const Vec2f* t0 = specTexCoordAt<T, I>(0);
			const Vec3f* c0 = specColorAt<C, I>(0);
			unsigned int i0 = specIndiceAt<I>(0);
			for (int i = 1; i < vertex.size() - 1; ++i)
			{
				func(v0, specVertexAt<I>(i), specVertexAt<I>(i+1),
					 t0, specTexCoordAt<T, I>(i), specTexCoordAt<T, I>(i+1),
					 c0, specColorAt<C, I>(i),    specColorAt<C, I>(i+1),
					 i0, specIndiceAt<I>(i),      specIndiceAt<I>(i+1));
			}
			break;
		}
		case StelVertexArray::TriangleStrip:
		{
			for (int i = 2; i < vertex.size(); ++i)
			{
				if (i % 2 == 0)
					func(specVertexAt<I>(i-2), specVertexAt<I>(i-1), specVertexAt<I>(i),
						 specTexCoordAt<T, I>(i-2), specTexCoordAt<T, I>(i-1), specTexCoordAt<T, I>(i),
						 specColorAt<C, I>(i-2), specColorAt<C, I>(i-1), specColorAt<C, I>(i),
						 specIndiceAt<I>(i-2), specIndiceAt<I>(i-1), specIndiceAt<I>(i));
				else
					func(specVertexAt<I>(i-1), specVertexAt<I>(i-2), specVertexAt<I>(i),
						 specTexCoordAt<T, I>(i-1), specTexCoordAt<T, I>(i-2), specTexCoordAt<T, I>(i),
						 specColorAt<C, I>(i-1), specColorAt<C, I>(i-2), specColorAt<C, I>(i),
						 specIndiceAt<I>(i-1), specIndiceAt<I>(i-2), specIndiceAt<I>(i));
			}
			break;
		}
		default:
			Q_ASSERT_X(0, Q_FUNC_INFO, "unsupported primitive type");
	}
	return func;
}


#endif // STELVERTEXARRAY_HPP
