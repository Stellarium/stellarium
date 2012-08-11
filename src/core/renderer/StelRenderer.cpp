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

#include "StelRenderer.hpp"
#include "VecMath.hpp"
#include "StelVertexBuffer.hpp"

void StelRenderer::drawLine
	(const float startX, const float startY, const float endX, const float endY)
{
	// Could be improved by keeping the vertex buffer as a data member,
	// or even caching all rectangle draws to the same buffer and drawing them 
	// at once at the end of the frame.
	
	// Create a vertex buffer for the line and draw it.
	StelVertexBuffer<Vertex>* buffer = createVertexBuffer<Vertex>(PrimitiveType_Lines);
	buffer->addVertex(Vertex(Vec2f(startX, startY)));
	buffer->addVertex(Vertex(Vec2f(endX, endY)));
	buffer->lock();
	drawVertexBuffer(buffer);
	delete buffer;
}
