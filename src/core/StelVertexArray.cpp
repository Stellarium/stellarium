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

#include "StelVertexArray.hpp"
#include "StelProjector.hpp"
#include "StelPainter.hpp"

#include <QtOpenGL>

void StelVertexArray::draw(StelPainter* painter) const
{
	painter->setVertexPointer(3, GL_DOUBLE, vertex.constData());
	if (isTextured())
	{
		painter->setTexCoordPointer(2, GL_FLOAT, texCoords.constData());
		painter->enableClientStates(true, true);
	}
	else
	{
		painter->enableClientStates(true, false);
	}
	if (isIndexed())
		painter->drawFromArray((StelPainter::DrawingMode)primitiveType, indices.size(), 0, true, indices.constData());
	else
		painter->drawFromArray((StelPainter::DrawingMode)primitiveType, vertex.size());

	painter->enableClientStates(false);
}


StelVertexArray StelVertexArray::removeDiscontinuousTriangles(const StelProjector* prj) const
{
	// We only support TriangleStrip mode for the moment.
	Q_ASSERT(primitiveType == TriangleStrip);
	// Also only support not indexed arrays.
	Q_ASSERT(!isIndexed());

	StelVertexArray ret = *this;
	ret.indices.reserve(vertex.size() * 3);

	// Create a 'Triangles' vertex array from this 'TriangleStrip' array.
	for (int i = 2; i < vertex.size(); ++i)
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
				ret.indices << i-1 << i-2 << i;
		}
	}
	ret.primitiveType = Triangles;

	return ret;
}
