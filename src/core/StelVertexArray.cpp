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

#include "StelVertexArray.hpp"
#include "StelProjector.hpp"

StelVertexArray StelVertexArray::removeDiscontinuousTriangles(const StelProjector* prj) const
{
	StelVertexArray ret = *this;

	if (isIndexed())
	{
		switch (primitiveType)
		{
			case Triangles:
			{
				QVector<unsigned short> indicesOrig = ret.indices;
				ret.indices.resize(0);
				for (int i = 0; i < indicesOrig.size(); i += 3)
				{
					if (prj->intersectViewportDiscontinuity(vertex.at(indicesOrig.at(i)), vertex.at(indicesOrig.at(i+1))) ||
							prj->intersectViewportDiscontinuity(vertex.at(indicesOrig.at(i+1)), vertex.at(indicesOrig.at(i+2))) ||
							prj->intersectViewportDiscontinuity(vertex.at(indicesOrig.at(i+2)), vertex.at(indicesOrig.at(i))))
					{
						// We have a discontinuity.
					}
					else
					{
						ret.indices << indicesOrig.at(i) << indicesOrig.at(i+1) << indicesOrig.at(i+2);
					}
				}
				break;
			}
			default:
				// Unsupported
				Q_ASSERT(false);
		}
	}
	else
	{
		ret.indices.clear();
		unsigned short int limit;
		// Create a 'Triangles' vertex array from this array.
		// We have different algorithms for different original mode
		switch (primitiveType)
		{
			case TriangleStrip:
				ret.indices.reserve(vertex.size() * 3);
				limit = static_cast<unsigned short int>(vertex.size());
				for (unsigned short int i = 2; i < limit; ++i)
				{
					if (prj->intersectViewportDiscontinuity(vertex[i], vertex[i-1]) ||
							prj->intersectViewportDiscontinuity(vertex[i-1], vertex[i-2]) ||
							prj->intersectViewportDiscontinuity(vertex[i-2], vertex[i]))
					{
						// We have a discontinuity.
					}
					else
					{
						if (i % 2 == 0)
							ret.indices << i-2 << i-1 << i;
						else
							ret.indices << i-2 << i << i-1;
					}
				}
				break;

			case Triangles:
				ret.indices.reserve(vertex.size());
				limit = static_cast<unsigned short int>(vertex.size());
				for (unsigned short int i = 0; i < limit; i += 3)
				{
					if (prj->intersectViewportDiscontinuity(vertex.at(i), vertex.at(i+1)) ||
							prj->intersectViewportDiscontinuity(vertex.at(i+1), vertex.at(i+2)) ||
							prj->intersectViewportDiscontinuity(vertex.at(i+2), vertex.at(i)))
					{
						// We have a discontinuity.
					}
					else
					{
						ret.indices << i << i+1 << i+2;
					}
				}
				break;

			default:
				Q_ASSERT(false);
		}
	}
	// Just in case we don't have any triangles, we also remove all the vertices.
	// This is because we can't specify an empty indexed VertexArray.
	// FIXME: we should use an attribute for indexed array.
	if (ret.indices.isEmpty())
		ret.vertex.clear();
	ret.primitiveType = Triangles;

	return ret;
}

QDataStream& operator<<(QDataStream& out, const StelVertexArray& p)
{
	out << p.vertex;
	out << p.texCoords;
	out << p.colors;
	out << p.indices;
	out << static_cast<unsigned int>(p.primitiveType);
	return out;
}

QDataStream& operator>>(QDataStream& in, StelVertexArray& p)
{
	in >> p.vertex;
	in >> p.texCoords;
	in >> p.colors;
	in >> p.indices;
	unsigned int t;
	in >> t;
	p.primitiveType=static_cast<StelVertexArray::StelPrimitiveType>(t);
	return in;
}
