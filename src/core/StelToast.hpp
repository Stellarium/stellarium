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
#include "StelToastGrid.hpp"

class StelPainter;
class ToastSurvey;

//! @class ToastTile
//! Represents a tile in a Toast image.
//! The tiles are stored in a tree structure, using the QObject
//! children/parent relationships.
class ToastTile : public QObject
{
	Q_OBJECT

public:
	ToastTile(QObject* parent, int level, int x, int y);	
	void draw(StelPainter* painter, const SphericalCap& viewportShape, int maxVisibleLevel);
	bool isTransparent();

protected:
	void drawTile(StelPainter* painter);
	//! Return the survey the tile belongs to.
	const ToastSurvey* getSurvey() const;
	//! Return the toast grid used by the tile.
	const ToastGrid* getGrid() const;
	//! Return whether the tile should be drawn
	bool isVisible(const SphericalCap& viewportShape, int maxVisibleLevel) const;
	//! return whether the tile is covered by its children tiles
	//! This is used to avoid drawing tiles that will be covered anyway
	bool isCovered(const SphericalCap& viewportShape) const;
	void prepareDraw();

private:
	//! The level of the tile
	int level;
	// x coordinate of the tile
	int x;
	// y coordinate of the tile
	int y;
	//! Path to the tile image
	QString imagePath;
	// Set to true if the tile has no texture
	bool empty;
	//! Set to true if the tile is ready to draw
	bool ready;
	//! The texture associated with the tile
	StelTextureSP texture;
	//! The bounding cap used to check if the tile is visible
	SphericalCap boundingCap;

	QList<ToastTile*> subTiles;

	// QList<SphericalRegionP> skyConvexPolygons;
	//! OpenGl arrays
	QVector<Vec3d> vertexArray;
	QVector<Vec2f> textureArray;
	QVector<unsigned short> indexArray;

	// Used for smooth fade in
	class QTimeLine* texFader;
};


//! @class ToastSurvey
//! Represents a full Toast survey.
class ToastSurvey : public QObject
{
	Q_OBJECT

public:
	ToastSurvey(const QString& path, int maxLevel);
	QString getTilePath(int level, int x, int y) const;
	void draw(StelPainter* sPainter);
	const ToastGrid* getGrid() const {return &grid;}
	int getMaxLevel() const {return maxLevel;}
	int getTilesSize() const {return 256;}

private:
	ToastGrid grid;
	QString path;
	ToastTile* rootTile;
	int maxLevel;
};

#endif // _STELTOAST_HPP_
