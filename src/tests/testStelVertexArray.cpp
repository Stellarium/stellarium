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

#include "tests/testStelVertexArray.hpp"


QTEST_GUILESS_MAIN(TestStelVertexArray)


void TestStelVertexArray::initTestCase()
{
	QVector<Vec3d> vertices;
	QVector<Vec2f> textureCoords;

	for (int i = 0; i < 1000; ++i)
	{
		Vec3d v(i+1, i+1, i+1);
		v.normalize();
		vertices.append(v);

		Vec2f t(i, i);
		textureCoords.append(t);
	}

	array = StelVertexArray(vertices, StelVertexArray::TriangleStrip, textureCoords);
}

struct EmptyVisitor
{
	inline void operator()(const Vec3d* , const Vec3d* , const Vec3d* ,
			       const Vec2f* , const Vec2f* , const Vec2f* ,
			       const Vec3f* , const Vec3f* , const Vec3f* ,
			       unsigned int, unsigned int , unsigned int )
	{
	}
};

void TestStelVertexArray::benchmarkForeachTriangleNoOp()
{
	QBENCHMARK {
		array.foreachTriangle(EmptyVisitor());
	}
}

struct VerticesVisitor
{
	VerticesVisitor(const VerticesVisitor& rst) : sum(rst.sum) {}

	VerticesVisitor() : sum(0, 0, 0) {}
	inline void operator()(const Vec3d* , const Vec3d* v1, const Vec3d* v2,
			       const Vec2f* , const Vec2f* , const Vec2f* ,
			       const Vec3f* , const Vec3f* , const Vec3f* ,
			       unsigned int , unsigned int , unsigned int )
	{
		sum += *v1 + *v2;
	}

	Vec3d sum;
};

void TestStelVertexArray::benchmarkForeachTriangle()
{
	Vec3d sum(0, 0, 0);
	QBENCHMARK {
		VerticesVisitor result = array.foreachTriangle(VerticesVisitor());
		sum = result.sum;
	}
	qDebug() << sum.toString();
}

void TestStelVertexArray::benchmarkForeachTriangleDirect()
{
	// Now we do the same thing "manually"
	Vec3d sum(0, 0, 0);
	QBENCHMARK {
		sum = Vec3d(0, 0, 0);
		for (int i = 2; i < array.vertex.size(); ++i)
		{
			if ((i % 2) == 0)
			{
				sum += array.vertex.at(i-1) + array.vertex.at(i);
			}
			else
			{
				sum += array.vertex.at(i-2) + array.vertex.at(i);
			}
		}
	}
	qDebug() << sum.toString();
}


