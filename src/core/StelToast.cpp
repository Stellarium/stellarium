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

#include <QtOpenGL>

#include <limits>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelToast.hpp"


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
	grid.fill(Vec3d(0, 0, 0), size * size);
	// Set up the level 0.
	at(0, 0, 0) = at(0, 1, 0) = at(0, 1, 1) = at(0, 0, 1) = Vec3d(0, 0, 1);
	// And the level 1
	at(1, 1, 1) = Vec3d(0, 0, -1);
	at(1, 1, 0) = Vec3d(1, 0, 0);
	at(1, 1, 2) = Vec3d(-1, 0, 0);
	at(1, 0, 1) = Vec3d(0, -1, 0);
	at(1, 2, 1) = Vec3d(0, 1, 0);
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


QVector<unsigned int> ToastGrid::getTrianglesIndex(int level, int x, int y, int resolution) const
{
	Q_UNUSED(x);
	Q_UNUSED(y);
	Q_ASSERT(resolution >= level);
	Q_ASSERT(resolution <= maxLevel);
	int size = pow2(resolution - level) + 1;
	int nbTiles = (size - 1) * (size - 1);
	QVector<unsigned int> ret;
	ret.reserve(nbTiles * 6);
	for (int i = 0; i < size - 1; ++i)
	{
		for (int j = 0; j < size - 1; ++j)
		{
			Q_ASSERT(i * size + j <= std::numeric_limits<int>::max());
			unsigned int a = i * size + j;
			unsigned int b = (i + 1) * size + j;
			unsigned int c = (i + 1) * size + j + 1;
			unsigned int d = i * size + j + 1;
			ret << a << c << b << a << d << c;
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
	ret << array[0] << array[1] << array[3] << array[2];
	return ret;
}



////////////////// ToastTile methods /////////////////

ToastTile::ToastTile(QObject* parent, int level, int x, int y)
	: QObject(parent), level(level), x(x), y(y), empty(false), ready(false), texture(NULL)
{
	Q_ASSERT(level <= getGrid()->getMaxLevel());
	const ToastSurvey* survey = getSurvey();
	// create the texture
	imagePath = survey->getTilePath(level, x, y);
	resolution = 1.5 / pow2(level+1);  // maybe we should find a better value for this ?
	shape = SphericalConvexPolygon(getGrid()->getPolygon(level, x, y));
	Q_ASSERT(shape.checkValid());
}


const ToastGrid* ToastTile::getGrid() const
{
	return getSurvey()->getGrid();
}


const ToastSurvey* ToastTile::getSurvey() const
{
	// the parent can either be a ToastSurvey either be a ToastTile
	ToastSurvey* ret = qobject_cast<ToastSurvey*>(parent());
	if (ret)
		return ret;
	ToastTile* tile = qobject_cast<ToastTile*>(parent());
	Q_ASSERT(tile);
	return tile->getSurvey();
}


bool ToastTile::isVisible(SphericalRegionP viewportShape, float resolution) const
{
	if (empty)
		return false;
	if (level == 0)
		return true;
	if (this->resolution < resolution)
		return false;
	return viewportShape->intersects(shape);
}

bool ToastTile::isCovered(SphericalRegionP viewportShape, float resolution) const
{
	// The tile is covered if we have at least one visible child and all the visible children are all ready to be drawn.
	int nbVisibleChildren = 0;
	foreach (ToastTile* child, getSubTiles())
	{
		if (child->isVisible(viewportShape, resolution))
			continue;
		nbVisibleChildren++;
		if (!child->ready)
			return false;
	}
	return nbVisibleChildren > 0;
}


void ToastTile::prepareDraw()
{
	if (texture.isNull())
	{
		qDebug() << "load texture" << imagePath;
		StelTextureMgr& texMgr=StelApp::getInstance().getTextureManager();
		texture = texMgr.createTextureThread(imagePath);
	}
	if (!texture->isLoading() && !texture->canBind() && !texture->getErrorMessage().isEmpty())
	{
		qDebug() << "can't get texture" << imagePath << texture->getErrorMessage();
		empty = true;
		return;
	}
	// Get the opengl arrays
	if (vertexArray.empty() && level < getGrid()->getMaxLevel())
	{
		vertexArray = getGrid()->getVertexArray(level, x, y, 6);
		textureArray = getGrid()->getTextureArray(level, x, y, 6);
		indexArray = getGrid()->getTrianglesIndex(level, x, y, 6);
	}

	if (getSubTiles().isEmpty() && level < getSurvey()->getMaxLevel())
	{
		qDebug() << "Create children";
		// Create the children
		for (int i = 0; i < 2; ++i)
			for (int j = 0; j < 2; ++j)
				new ToastTile(this, level + 1, 2 * this->x + i, 2 * this->y + j);
		Q_ASSERT(children().size() == 4);
	}
	ready = true;
}


void ToastTile::drawTile(StelCore* core)
{
	if (!ready)
		prepareDraw();

	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	sPainter.setColor(1, 1, 1, 1);

	if (!texture->bind())
		return;

	sPainter.enableTexture2d(true);

	//	// We need to make a copy of the vertex arrays because they are modified by the painter
	//	QVector<Vec3d> vertexArray(this->vertexArray);
	Q_ASSERT(vertexArray.size() == textureArray.size());

	glEnable(GL_CULL_FACE);
	// sPainter.drawArrays(GL_TRIANGLES, vertexArray.size(), vertexArray.data(), textureArray.data(), NULL, NULL, indexArray.size(), indexArray.constData());
	sPainter.setArrays(vertexArray.constData(), textureArray.constData());
	sPainter.drawFromArray(StelPainter::Triangles, indexArray.size(), 0, true, indexArray.constData());
	glDisable(GL_CULL_FACE);
}


void ToastTile::draw(StelCore* core, SphericalRegionP viewportShape, float resolution)
{
	if (!isVisible(viewportShape, resolution))
	{
		free();
		return;
	}
	if (!isCovered(viewportShape, resolution))
		drawTile(core);
	// Draw all the children
	foreach(ToastTile* child, getSubTiles())
	{
		child->draw(core, viewportShape, resolution);
	}
}


void ToastTile::free()
{
	texture.clear();
	Q_ASSERT(texture.isNull());
	foreach(ToastTile* child, getSubTiles())
	{
		child->free();
		child->deleteLater();
	}
	ready = false;
}

/////// ToastSurvey methods ////////////
ToastSurvey::ToastSurvey(const QString& path)
	: grid(6), path(path), maxLevel(6)
{
	rootTile = new ToastTile(this, 0, 0, 0);
}


QString ToastSurvey::getTilePath(int level, int x, int y) const
{
	QString ret = path;
	ret.replace("{level}", QString::number(level));
	ret.replace("{x}", QString::number(x));
	ret.replace("{y}", QString::number(y));
	return ret;
}


void ToastSurvey::draw(StelCore* core)
{
	// Get the viewport resolution in degree per pixel.  We can then
	// use it to discard tiles that are in a higher resolution.
	const double degPerPixel = 1./core->getProjection(StelCore::FrameJ2000)->getPixelPerRadAtCenter()*180./M_PI;
	// We also get the viewport shape to discard invisibly tiles.
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	SphericalRegionP viewportRegion = prj->getViewportConvexPolygon(0, 0);
	rootTile->draw(core, viewportRegion, degPerPixel);
}
