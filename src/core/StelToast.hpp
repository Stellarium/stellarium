/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef _STELTOAST_HPP_
#define _STELTOAST_HPP_

#include <QObject>
#include <QString>
#include <QVector>

#include "StelSphereGeometry.hpp"
#include "StelTexture.hpp"
#include "StelTextureTypes.hpp"
#include "VecMath.hpp"

//! compute 2**x
inline int pow2(int x) {return 1 << x;}

//! @class ToasGrid
//
//! convenience class that can be used to compute the toast grid
//! points.
//
//! The ToastGrid class allow to compute the vertex arrays associated
//! with Toast tiles.  Each method refers to a tile by its level and x
//! and y coordinates.
class ToastGrid
{
public:
	ToastGrid(int level);
	//! Get the vertice array for a given tile.
	//! The position are stored in a grid.
	//! @param level the level of the tile.
	//! @param x the x coordinate of the tile.
	//! @param y the y coordinate of the tile.
	//! @param resolution the resolution of the returned array.
	QVector<Vec3d> getVertexArray(int level, int x, int y, int resolution) const;
	//! Get the texture array for a given tile.
	//! The position are stored in a grid
	//! @param level the level of the tile.
	//! @param x the x coordinate of the tile.
	//! @param y the y coordinate of the tile.
	//! @param resolution the resolution of the returned array.
	QVector<Vec2f> getTextureArray(int level, int x, int y, int resolution) const;
	//! Get the index of the vertex from getVertexArray sorted as a list of triangles.
	//! @param level the level of the tile.
	//! @param x the x coordinate of the tile.
	//! @param y the y coordinate of the tile.
	//! @param resolution the resolution of the returned array.
	QVector<unsigned int> getTrianglesIndex(int level, int x, int y, int resolution) const;
	//! Returns the polygon contouring a given tile.
	//! @param level the level of the tile.
	//! @param x the x coordinate of the tile.
	//! @param y the y coordinate of the tile.
	QVector<Vec3d> getPolygon(int level, int x, int y) const;
	//! Return the max level of this grid.
	int getLevel() const {return level;}

private:
	//! Get the vector at a given point in the grid
	const Vec3d& at(int x, int y) const {return grid[y * size + x];}
	//! Get the vector at a given point in the grid
	Vec3d& at(int x, int y) {return grid[y * size + x];}
	//! Get the vector at a given point in the grid
	const Vec3d& at(int level, int x, int y) const
		{int scale = pow2(this->level - level); return at(scale * x, scale * y);}
	//! Get the vector at a given point in the grid
	Vec3d& at(int level, int x, int y)
		{int scale = pow2(this->level - level); return at(scale * x, scale * y);}

	//! initialize the grid
	void init_grid();
	void init_grid(int level, int x, int y, bool side);

	//! The max level of the grid
	int level;
	//! the size of the grid
	int size;
	//! The actual grid data
	QVector<Vec3d> grid;
};


class ToastSurvey;


//!@class ToastTile
//
//! Represents a tile in a Toast image.
//
//! The tiles are stored in a tree structure, using the QObject
//! children/parent relationships.
class ToastTile : public QObject
{
	Q_OBJECT

public:
	ToastTile(QObject* parent, int level, int x, int y);
	void draw(class StelCore* core, SphericalRegionP viewportShape, float resolution);

protected:
	void drawTile(StelCore* core);
	//! Return the survey the tile belongs to.
	const ToastSurvey* getSurvey() const;
	//! Return the toast grid used by the tile.
	const ToastGrid* getGrid() const;
	//! Return all the subtiles
	QList<ToastTile*> getSubTiles() const {return findChildren<ToastTile*>();}
	//! Return whether the tile should be drawn
	bool isVisible(SphericalRegionP viewportShape, float resolution) const;
	//! return whether the tile is covered by its children tiles
	//! This is used to avoid drawing tiles that will be covered anyway
	bool isCovered(SphericalRegionP viewportShape, float resolution) const;
	void prepareDraw();
	void free();

private:
	//! The level of the tile
	int level;
	// x coordinate of the tile
	int x;
	// y coordinate of the tile
	int y;
	//! resolution of the data of the texture in degree/pixel
	float resolution;
	//! Path to the tile image
	QString imagePath;
	// Set to true if the tile has no texture
	bool empty;
	//! Set to true if the tile is ready to draw
	bool ready;
	//! The texture associated with the tile
	StelTextureSP texture;
	//! The shape used to check if the tile is visible
	SphericalConvexPolygon shape;

	// QList<SphericalRegionP> skyConvexPolygons;
	//! OpenGl arrays
	QVector<Vec3d> vertexArray;
	QVector<Vec2f> textureArray;
	QVector<unsigned int> indexArray;
};


//!@class ToastSurvey
//
//! Represents a full Toast survey.
class ToastSurvey : public QObject
{
	Q_OBJECT

public:
	ToastSurvey(const QString& path);
	QString getTilePath(int level, int x, int y) const;
	void draw(class StelCore* core);
	const ToastGrid* getGrid() const {return &grid;}
	int getMaxLevel() const {return maxLevel;}

private:
	ToastGrid grid;
	QString path;
	ToastTile* rootTile;
	int maxLevel;
};

#endif // _STELTOAST_HPP_
