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

#ifndef STELTOAST_HPP
#define STELTOAST_HPP

#include <QCache>
#include <QObject>
#include <QString>
#include <QTimeLine>
#include <QVector>

#include "StelSphereGeometry.hpp"
#include "StelTexture.hpp"
#include "StelTextureTypes.hpp"
#include "VecMath.hpp"
#include "StelToastGrid.hpp"
#include "StelVertexArray.hpp"
#include "StelFader.hpp"

class StelPainter;
class ToastSurvey;

//! @class ToastTile
//! Represents a tile in a TOAST image.
//! The tiles are stored in a tree structure
class ToastTile
{
public:
	//! Triple struct for a coordinate of a ToastTile
	struct Coord
	{
		int level;
		int x;
		int y;

		bool operator==(const Coord& b) const
		{
			return level == b.level && x == b.x && y == b.y;
		}
		bool operator!=(const Coord& b) const
		{
			return !(*this == b);
		}
	};

	ToastTile(ToastSurvey *survey, int level, int x, int y);
	virtual ~ToastTile();
	Coord getCoord() const { Coord c = { level, x, y }; return c; }
	// color is a global sky color (set to 1/1/1 for full brightness) which may be modulated by atmospheric brightness.
	void draw(StelPainter* painter, const SphericalCap& viewportShape, int maxVisibleLevel, Vec3f color);
	bool isTransparent();

protected:
	void drawTile(StelPainter* painter, Vec3f color);
	//! Return the survey the tile belongs to.
	const ToastSurvey* getSurvey() const;
	//! Return the toast grid used by the tile.
	const ToastGrid* getGrid() const;
	//! Return whether the tile should be drawn
	bool isVisible(const SphericalCap& viewportShape, int maxVisibleLevel) const;
	//! return whether the tile is covered by its children tiles
	//! This is used to avoid drawing tiles that will be covered anyway
	bool isCovered(const SphericalCap& viewportShape) const;
	//! prepare arrays. color is set for a global brightness scaling. With atmosphere on, this will also set extinction effects.
	void prepareDraw(Vec3f color);

private:
	//! The ToastSurvey object this tile belongs to
	ToastSurvey* survey;
	//! The TOAST level of the tile
	int level;
	//! x coordinate of the tile
	int x;
	//! y coordinate of the tile
	int y;
	//! Path to the tile image
	QString imagePath;
	//! Set to true if the tile has no texture
	bool empty;
	//! Set to true if the tile is prepared for drawing (prepareDraw() has been called).
	//! This does not necessarily meen the tile will be drawn
	//! (because texture loading, etc. might not have completed yet), see readyDraw.
	bool prepared;
	//! If true, the tile is fully ready for drawing (all resources are loaded)
	bool readyDraw;
	//! The texture associated with the tile
	StelTextureSP texture;
	//! The bounding cap used to check if the tile is visible
	SphericalCap boundingCap;

	QList<ToastTile*> subTiles;

	// QList<SphericalRegionP> skyConvexPolygons;
	//! OpenGL arrays
	QVector<Vec3d> vertexArray;
	QVector<Vec2f> textureArray;
	QVector<unsigned short> indexArray;
	QVector<Vec3f> colorArray; // for extinction
	// Used for smooth fade in
	QTimeLine texFader;
};

//! Needed for QHash/QCache compatibility
inline uint qHash(const ToastTile::Coord& key, uint seed = 0)
{
	//with a maximum level of 11, the x/y coords may be up to 4^11
	return qHash(key.level << 28 |
		     key.x << 14 |
		     key.y,
		     seed);
}


//! @class ToastSurvey
//! Represents a full Toast survey.
class ToastSurvey : public QObject
{
	Q_OBJECT

public:
	ToastSurvey(const QString& path, int maxLevel);
	virtual ~ToastSurvey();
	QString getTilePath(int level, int x, int y) const;
	void draw(StelPainter* sPainter);
	const ToastGrid* getGrid() const {Q_ASSERT(grid); return grid;}
	int getMaxLevel() const {return maxLevel;}
	int getTilesSize() const {return 256;}

	//! Returns a cached, non-active but recently used tile with the specified coordinates
	//! or Q_NULLPTR if not currently cached. The ownership of the tile transfers to the caller.
	ToastTile* getCachedTile(int level, int x, int y);
	//! Puts the given tile into the tile cache. The ownership of the tile will be taken.
	void putIntoCache(ToastTile* tile);

private:
	ToastGrid* grid;
	QString path;
	ToastTile* rootTile;
	int maxLevel;

	typedef QCache<ToastTile::Coord, ToastTile> ToastCache;
	ToastCache toastCache;
};

#endif // STELTOAST_HPP
