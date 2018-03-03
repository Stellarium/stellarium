/*
 * Stellarium
 * Copyright (C) 2010 Guillaume Chereau
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

#ifndef STELTOASTGRID_HPP
#define STELTOASTGRID_HPP

#include <QVector>
#include "VecMath.hpp"

//! Compute 2^x
inline int pow2(int x) {return 1 << x;}

//! @class ToastGrid
//! Convenience class that can be used to compute the TOAST grid points.
//! The ToastGrid class allows to compute the vertex arrays associated
//! with TOAST tiles. Each method refers to a tile by its level and x
//! and y coordinates.
class ToastGrid
{
public:
	ToastGrid(int maxLevel);
	//! Get the vertex array for a given tile.
	//! The position are stored in a grid.
	//! @param level the TOAST level of the tile.
	//! @param x the x coordinate of the tile.
	//! @param y the y coordinate of the tile.
	//! @param resolution the resolution of the returned array. TODO: UNITS?
	QVector<Vec3d> getVertexArray(int level, int x, int y, int resolution) const;
	//! Get the texture array for a given tile.
	//! The position are stored in a grid
	//! @param level the TOAST level of the tile.
	//! @param x the x coordinate of the tile.
	//! @param y the y coordinate of the tile.
	//! @param resolution the resolution of the returned array. TODO: UNITS?
	QVector<Vec2f> getTextureArray(int level, int x, int y, int resolution) const;
	//! Get the index of the vertex from getVertexArray sorted as a list of triangles.
	//! @param level the TOAST level of the tile.
	//! @param x the x coordinate of the tile.
	//! @param y the y coordinate of the tile.
	//! @param resolution the resolution of the returned array.  TODO: UNITS?
	QVector<unsigned short> getTrianglesIndex(int level, int x, int y, int resolution) const;
	//! Returns the polygon contouring a given tile.
	//! @param level the TOAST level of the tile.
	//! @param x the x coordinate of the tile.
	//! @param y the y coordinate of the tile.
	QVector<Vec3d> getPolygon(int level, int x, int y) const;
	//! Return the max TOAST level of this grid.
	int getMaxLevel() const {return maxLevel;}

private:
	//! Get the vector at a given point in the grid
	const Vec3d& at(int x, int y) const {return grid[y * size + x];}
	//! Get the vector at a given point in the grid
	Vec3d& at(int x, int y) {return grid[y * size + x];}
	//! Get the vector at a given point in the grid
	const Vec3d& at(int level, int x, int y) const
		{int scale = pow2(maxLevel - level); return at(scale * x, scale * y);}
	//! Get the vector at a given point in the grid
	Vec3d& at(int level, int x, int y)
		{int scale = pow2(maxLevel - level); return at(scale * x, scale * y);}

	//! initialize the grid
	void init_grid();
	void init_grid(int level, int x, int y, bool side);

	//! The max level of the grid
	int maxLevel;
	//! the size of the grid
	int size;
	//! The actual grid data
	QVector<Vec3d> grid;
};

#endif // STELTOASTGRID_HPP
