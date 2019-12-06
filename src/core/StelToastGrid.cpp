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

#include <limits>
#include "StelToastGrid.hpp"

//! compute the middle of two points on the sphere
static inline Vec3d middle(const Vec3d& a, const Vec3d b)
{
	Vec3d ret = a;
	ret += b;
	ret.normalize();
	return ret;
}


ToastGrid::ToastGrid(int amaxLevel)
	: maxLevel(amaxLevel), size(pow2(amaxLevel) + 1)
{
	// We assume that initialization of the grid is fast enough to be
	// done in the constructor.
	init_grid();
}


void ToastGrid::init_grid()
{
	// Allocate the grid memory.
	grid.resize(size * size);
	// Set up the level 0.
	at(0, 0, 0) = at(0, 1, 0) = at(0, 1, 1) = at(0, 0, 1) = Vec3d(0, 0, -1);
	// And the level 1
	// Need mirror
	at(1, 1, 1) = Vec3d(0, 0, 1);
	at(1, 1, 0) = Vec3d(0, -1, 0);
	at(1, 2, 1) = Vec3d(1, 0, 0);
	at(1, 1, 2) = Vec3d(0, 1, 0);
	at(1, 0, 1) = Vec3d(-1, 0, 0);

	// Then we can compute the other levels by recursion
	init_grid(1, 0, 0, false);
	init_grid(1, 0, 1, true);
	init_grid(1, 1, 1, false);
	init_grid(1, 1, 0, true);
}


void ToastGrid::init_grid(int level, int x, int y, bool side)
{
	Q_ASSERT(level >= 1); // This method won't work for level 0 !
	int clevel = level + 1;
	int cx = 2*x;
	int cy = 2*y;
	// first we compute all the mid points
	at(clevel, cx, cy+1) = middle(at(level, x, y), at(level, x, y+1));
	at(clevel, cx+1, cy+2) = middle(at(level, x, y+1), at(level, x+1, y+1));
	at(clevel, cx+2, cy+1) = middle(at(level, x+1, y+1), at(level, x+1, y));
	at(clevel, cx+1, cy) = middle(at(level, x+1, y), at(level, x, y));
	if (side)
		at(clevel, cx+1, cy+1) = middle(at(level, x, y), at(level, x+1, y+1));
	else
		at(clevel, cx+1, cy+1) = middle(at(level, x, y+1), at(level, x+1, y));
	// now we can compute the higher levels
	if (clevel < maxLevel)
	{
		init_grid(clevel, cx, cy, side);
		init_grid(clevel, cx+1, cy, side);
		init_grid(clevel, cx+1, cy+1, side);
		init_grid(clevel, cx, cy+1, side);
	}
}


QVector<Vec3d> ToastGrid::getVertexArray(int level, int x, int y, int resolution) const
{
	Q_ASSERT(resolution >= level);
	Q_ASSERT(resolution <= maxLevel);
	// The size of the returned array
	int size = pow2(resolution - level) + 1;
	QVector<Vec3d> ret;
	ret.reserve(size * size);
	// Compute the real position in the grid
	int scale = pow2(maxLevel - level);
	x *= scale;
	y *= scale;
	// Fill the array
	int step = pow2(maxLevel - resolution);
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			ret.append(at(x + j * step, y + i * step));
		}
	}
	Q_ASSERT(ret.size() == size * size);
	return ret;
}


QVector<Vec2f> ToastGrid::getTextureArray(int level, int x, int y, int resolution) const
{
	Q_UNUSED(x);
	Q_UNUSED(y);
	Q_ASSERT(resolution >= level);
	Q_ASSERT(resolution <= maxLevel);
	// The size of the returned array
	int size = pow2(resolution - level) + 1;
	QVector<Vec2f> ret;
	ret.reserve(size * size);
	for (int i = size-1; i >= 0; i--)
	{
		for (int j = 0; j < size; j++)
		{
			ret.append(Vec2f(j,i) / (size-1));
		}
	}
	Q_ASSERT(ret.size() == size * size);
	return ret;
}


QVector<unsigned short> ToastGrid::getTrianglesIndex(int level, int x, int y, int resolution) const
{
	Q_ASSERT(resolution >= level);
	Q_ASSERT(resolution <= maxLevel);
	int size = pow2(resolution - level) + 1;
	int nbTiles = (size - 1) * (size - 1);
	// If we are in the top right or the bottom left quadrant we invert the diagonal of the triangles.
	int middleIndex = pow2(level) / 2;
	bool invert = (x >= middleIndex) == (y >= middleIndex);
	QVector<unsigned short> ret;
	ret.reserve(nbTiles * 6);
	for (int i = 0; i < size - 1; ++i)
	{
		for (int j = 0; j < size - 1; ++j)
		{
			Q_ASSERT(i * size + j <= std::numeric_limits<short>::max());
			unsigned short int a = static_cast<unsigned short int>(i * size + j);
			unsigned short int b = static_cast<unsigned short int>((i + 1) * size + j);
			unsigned short int c = static_cast<unsigned short int>((i + 1) * size + j) + 1;
			unsigned short int d = static_cast<unsigned short int>(i * size + j + 1);
			if (!invert)
				ret << b << c << a << c << d << a;
			else
				ret << b << d << a << d << b << c;
		}
	}

	Q_ASSERT(ret.size() == nbTiles * 6);
	return ret;
}


QVector<Vec3d> ToastGrid::getPolygon(int level, int x, int y) const
{
	QVector<Vec3d> array = getVertexArray(level, x, y, level);
	QVector<Vec3d> ret;
	ret.reserve(4);
	ret << array[2] << array[3] << array[1] << array[0];
	return ret;
}
