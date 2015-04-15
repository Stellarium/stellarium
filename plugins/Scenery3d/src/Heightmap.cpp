/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#include <limits>

#include "Heightmap.hpp"
#include "VecMath.hpp"
#include <cassert>

#define INF (std::numeric_limits<float>::max())
#define NO_HEIGHT (-INF)

Heightmap::Heightmap(OBJ* obj) : obj(obj), xMin(INF), yMin(INF), xMax(-INF), yMax(-INF), nullHeight(0)
{
	xMin = std::min(obj->pBoundingBox.min[0], xMin);
	yMin = std::min(obj->pBoundingBox.min[1], yMin);
	xMax = std::max(obj->pBoundingBox.max[0], xMax);
	yMax = std::max(obj->pBoundingBox.max[1], yMax);

	this->initGrid();
}

Heightmap::~Heightmap()
{
	delete[] grid;
}

/**
 * Returns the height of the ground model for any observer x/y coords.
 * The height is the highest z value of the ground model at these
 * coordinates.
 */
float Heightmap::getHeight(const float x, const float y) const
{
	Heightmap::GridSpace* space = getSpace(x, y);
	if (space == NULL)
	{
		return nullHeight;
	}
	else
	{
		float h = space->getHeight(*obj, x, y);
		if (h == NO_HEIGHT)
		{
			return nullHeight;
		}
		else
		{
			return h;
		}
	}
}

/**
 * Height query within a single grid space. The list of faces to check
 * for intersection with the observer coords is limited to faces
 * intersecting this grid space.
 */
float Heightmap::GridSpace::getHeight(const OBJ& obj, const float x, const float y) const
{
	float h = NO_HEIGHT;

	for(unsigned int i=0; i<obj.m_numberOfTriangles; ++i)
	{
		const unsigned int* pTriangle = &obj.m_indexArray[i*3];

		float face_h = face_height_at(obj, pTriangle, x, y);
		if(face_h > h)
		{
			h = face_h;
		}
	}

	return h;
}

/**
 * Allocates memory for the grid and fills every grid space with
 * pointers to the faces in that space.
 */
void Heightmap::initGrid()
{
	grid = new GridSpace[GRID_LENGTH*GRID_LENGTH];

	for (int y = 0; y < GRID_LENGTH; y++)
	{
		for (int x = 0; x < GRID_LENGTH; x++)
		{
			float xmin = this->xMin + (x * (this->xMax - this->xMin)) / GRID_LENGTH;
			float ymin = this->yMin + (y * (this->yMax - this->yMin)) / GRID_LENGTH;
			float xmax = this->xMin + ((x+1) * (this->xMax - this->xMin)) / GRID_LENGTH;
			float ymax = this->yMin + ((y+1) * (this->yMax - this->yMin)) / GRID_LENGTH;

			FaceVector* faces = &grid[y*GRID_LENGTH + x].faces;

			for(unsigned int i=0; i<obj->m_numberOfTriangles; ++i)
			{
				const unsigned int* pTriangle = &(obj->m_indexArray.at(i*3));
				if(face_in_area(*obj, pTriangle, xmin, ymin, xmax, ymax))
				{
					faces->push_back(*pTriangle);
				}
			}
		}
	}
}

/**
 * Returns the GridSpace which covers the area around x/y.
 */
Heightmap::GridSpace* Heightmap::getSpace(const float x, const float y) const
{
	int ix = (x - xMin) / (xMax - xMin) * GRID_LENGTH;
	int iy = (y - yMin) / (yMax - yMin) * GRID_LENGTH;

	if ((ix < 0) || (ix >= GRID_LENGTH) || (iy < 0) || (iy >= GRID_LENGTH))
	{
		return NULL;
	}
	else
	{
		return &grid[iy*GRID_LENGTH + ix];
	}
}

/**
 * Returns the height of the face at the given point or -inf if
 * the coordinates are outside the bounds of the face.
 */
float Heightmap::GridSpace::face_height_at(const OBJ& obj,const unsigned int* pTriangle, const float x, const float y)
{
	//Vertices in triangle
	const OBJ::Vertex& pV0 = obj.m_vertexArray.at(pTriangle[0]);
	const OBJ::Vertex& pV1 = obj.m_vertexArray.at(pTriangle[1]);
	const OBJ::Vertex& pV2 = obj.m_vertexArray.at(pTriangle[2]);

	const float* pVertex0 = pV0.position;
	const float* pVertex1 = pV1.position;
	const float* pVertex2 = pV2.position;

	// Weight of those vertices is used to calculate exact height at (x,y), using barycentric coordinates, see also
	// http://en.wikipedia.org/wiki/Barycentric_coordinate_system_(mathematics)#Converting_to_barycentric_coordinates
	float det_T = (pVertex1[1]-pVertex2[1]) *
		      (pVertex0[0]-pVertex2[0]) +
		      (pVertex2[0]-pVertex1[0]) *
		      (pVertex0[1]-pVertex2[1]);

	float l1 = ((pVertex1[1]-pVertex2[1]) *
		    (x-pVertex2[0]) +
		    (pVertex2[0]-pVertex1[0]) *
		    (y-pVertex2[1]))/det_T;

	float l2 = ((pVertex2[1]-pVertex0[1]) *
		    (x-pVertex2[0]) +
		    (pVertex0[0]-pVertex2[0]) *
		    (y-pVertex2[1]))/det_T;

	float l3 = 1.0f - l1 - l2;

	if ((l1 < 0) || (l2 < 0) || (l3 < 0))
	{
		return NO_HEIGHT; // (x,y) out of face bounds
	}
	else
	{
		return l1*pVertex0[2] + l2*pVertex1[2] + l3*pVertex2[2];
	}
}

/**
 * Returns true if the given face intersects the given area.
 */
bool Heightmap::face_in_area(const OBJ& obj, const unsigned int* pTriangle, const float xmin, const float ymin, const float xmax, const float ymax) const
{
	// current implementation: use face's bounding box
	float f_xmin = xmax;
	float f_ymin = ymax;
	float f_xmax = xmin;
	float f_ymax = ymin;

	for(int i=0; i<3; i++)
	{
		const OBJ::Vertex& pVertex = obj.m_vertexArray.at(pTriangle[i]);
		if(pVertex.position[0] < f_xmin) f_xmin = pVertex.position[0];
		if(pVertex.position[1] < f_ymin) f_ymin = pVertex.position[1];
		if(pVertex.position[0] > f_xmax) f_xmax = pVertex.position[0];
		if(pVertex.position[1] > f_ymax) f_ymax = pVertex.position[1];
	}

	if ((f_xmin < xmax) && (f_ymin < ymax) && (f_xmax > xmin) && (f_ymax > ymin))
	{
		return true;
	}
	else
	{
		return false;
	}
}
