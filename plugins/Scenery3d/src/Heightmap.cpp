/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2016 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#include <limits>

#include "Heightmap.hpp"
#include "VecMath.hpp"
#include <cassert>

#define INF (std::numeric_limits<float>::max())
#define NO_HEIGHT (-INF)

Heightmap::Heightmap() : grid(Q_NULLPTR), nullHeight(0.0)
{
}

Heightmap::~Heightmap()
{
	delete[] grid;
}

void Heightmap::setMeshData(const IdxList &indexList, const PosList &posList)
{
	this->indexList = indexList;
	this->posList = posList;

	//re-calc min/max
	xMin = yMin =  std::numeric_limits<float>::max();
	xMax = yMax = -std::numeric_limits<float>::max();
	for(int i = 0;i<posList.size();++i)
	{
		xMin = std::min(xMin, posList.at(i)[0]);
		yMin = std::min(yMin, posList.at(i)[1]);
		xMax = std::max(xMax, posList.at(i)[0]);
		yMax = std::max(yMax, posList.at(i)[1]);
	}

	this->initGrid();
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
		float h = space->getHeight(posList, x, y);
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
float Heightmap::GridSpace::getHeight(const PosList& posList, const float x, const float y) const
{
	float h = NO_HEIGHT;

	// this was actually broken for a very long time
	// and checked ALL faces instead of only the ones in the grid cell...
	for(unsigned int i=0; i<faces.size(); ++i)
	{
		//points into the index list
		const unsigned int* pTriangle = faces[i];

		float face_h = face_height_at(posList, pTriangle, x, y);
		if(face_h > h)
		{
			h = face_h;
		}
	}

	return h;
}

/**
 * Fills every grid space with
 * pointers to the faces in that space.
 */
void Heightmap::initGrid()
{
	delete[] grid;
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

			for(int i=0; i<indexList.size(); i+=3)
			{
				const unsigned int* pTriangle = &(indexList.at(i));
				//TODO this can be optimized massively by only checking the grid spaces which the triangle actually covers...
				// or at least check only the bounding rectangle...

				if(face_in_area(pTriangle, xmin, ymin, xmax, ymax))
				{
					faces->push_back(pTriangle);
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
float Heightmap::GridSpace::face_height_at(const PosList& obj, const unsigned int* pTriangle, const float x, const float y)
{
	//Vertices in triangle
	const Vec3f& pVertex0 = obj.at(pTriangle[0]);
	const Vec3f& pVertex1 = obj.at(pTriangle[1]);
	const Vec3f& pVertex2 = obj.at(pTriangle[2]);

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
bool Heightmap::face_in_area(const unsigned int* pTriangle, const float xmin, const float ymin, const float xmax, const float ymax) const
{
	// current implementation: use face's bounding box
	float f_xmin = xmax;
	float f_ymin = ymax;
	float f_xmax = xmin;
	float f_ymax = ymin;

	for(int i=0; i<3; i++)
	{
		const Vec3f& pos = posList.at(pTriangle[i]);
		f_xmin = std::min(f_xmin, pos[0]);
		f_ymin = std::min(f_ymin, pos[1]);
		f_xmax = std::max(f_xmax, pos[0]);
		f_ymax = std::max(f_ymax, pos[1]);
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
