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

	for (int i = 0; i < 3000; ++i) // Use i%3 for enable testing all types of triangles
	{
		Vec3d v(i+1, i+1, i+1);
		v.normalize();
		vertices.append(v);

		Vec2f t(i, i);
		textureCoords.append(t);
	}

	arrayTriangleStrip = StelVertexArray(vertices, StelVertexArray::TriangleStrip, textureCoords);
	arrayTriangleFan = StelVertexArray(vertices, StelVertexArray::TriangleFan, textureCoords);
	arrayTriangles = StelVertexArray(vertices, StelVertexArray::Triangles, textureCoords);
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
		arrayTriangleStrip.foreachTriangle(EmptyVisitor());
	}
	QBENCHMARK {
		arrayTriangleFan.foreachTriangle(EmptyVisitor());
	}
	QBENCHMARK {
		arrayTriangles.foreachTriangle(EmptyVisitor());
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
		VerticesVisitor result = arrayTriangleStrip.foreachTriangle(VerticesVisitor());
		sum = result.sum;
	}
	qDebug() << sum.toString();

	sum.set(0, 0, 0);
	QBENCHMARK {
		VerticesVisitor result = arrayTriangleFan.foreachTriangle(VerticesVisitor());
		sum = result.sum;
	}
	qDebug() << sum.toString();

	sum.set(0, 0, 0);
	QBENCHMARK {
		VerticesVisitor result = arrayTriangles.foreachTriangle(VerticesVisitor());
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
		for (int i = 2; i < arrayTriangleStrip.vertex.size(); ++i)
		{
			if ((i % 2) == 0)
			{
				sum += arrayTriangleStrip.vertex.at(i-1) + arrayTriangleStrip.vertex.at(i);
			}
			else
			{
				sum += arrayTriangleStrip.vertex.at(i-2) + arrayTriangleStrip.vertex.at(i);
			}
		}
	}
	qDebug() << sum.toString();

	sum.set(0, 0, 0);
	QBENCHMARK {
		sum = Vec3d(0, 0, 0);
		for (int i = 2; i < arrayTriangleFan.vertex.size(); ++i)
		{
			if ((i % 2) == 0)
			{
				sum += arrayTriangleFan.vertex.at(i-1) + arrayTriangleFan.vertex.at(i);
			}
			else
			{
				sum += arrayTriangleFan.vertex.at(i-2) + arrayTriangleFan.vertex.at(i);
			}
		}
	}
	qDebug() << sum.toString();

	sum.set(0, 0, 0);
	QBENCHMARK {
		sum = Vec3d(0, 0, 0);
		for (int i = 3; i < arrayTriangles.vertex.size(); ++i)
		{
			if ((i % 3) == 0)
			{
				sum += arrayTriangles.vertex.at(i-2) + arrayTriangles.vertex.at(i-1) + arrayTriangles.vertex.at(i);
			}
		}
	}
	qDebug() << sum.toString();
}


