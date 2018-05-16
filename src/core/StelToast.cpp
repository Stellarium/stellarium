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

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelToast.hpp"

#include <QTimeLine>

ToastTile::ToastTile(ToastSurvey* survey, int level, int x, int y)
	: survey(survey), level(level), x(x), y(y), empty(false), prepared(false), readyDraw(false), texFader(1000)
{
	Q_ASSERT(level <= getGrid()->getMaxLevel());
	// create the texture
	imagePath = survey->getTilePath(level, x, y);

	if (level==0)
	{
		boundingCap.n=Vec3d(1,0,0);
		boundingCap.d=-1.;
	}
	const QVector<Vec3d>& pts = getGrid()->getPolygon(level, x, y);
	Vec3d n = pts.at(0);
	n+=pts.at(1);
	n+=pts.at(2);
	n+=pts.at(3);
	n.normalize();
	boundingCap.n=n;
	if (level==1)
		boundingCap.d=0;
	else
		boundingCap.d=qMin(qMin(n*pts.at(0), n*pts.at(1)), qMin(n*pts.at(2), n*pts.at(3)));
}

ToastTile::~ToastTile()
{
	//delete all currently owned tiles
	foreach(ToastTile* child, subTiles)
	{
		delete child;
	}
	subTiles.clear();
}

const ToastGrid* ToastTile::getGrid() const
{
	return getSurvey()->getGrid();
}


const ToastSurvey* ToastTile::getSurvey() const
{
	// the survey is stored directly for increased efficiency
	return survey;
}


bool ToastTile::isVisible(const SphericalCap& viewportShape, int maxVisibleLevel) const
{
	if (empty)
		return false;
	if (level == 0)
		return true;
	if (level > maxVisibleLevel)
		return false;
	return viewportShape.intersects(boundingCap);
}

bool ToastTile::isCovered(const SphericalCap& viewportShape) const
{
	// The tile is covered if we have at least one visible child and all the visible children are all ready to be drawn.
	int nbVisibleChildren = 0;
	foreach (const ToastTile* child, subTiles)
	{
		if (!viewportShape.intersects(child->boundingCap))
			continue;
		nbVisibleChildren++;
		if (!child->readyDraw || child->texFader.state()==QTimeLine::Running)
			return false;
	}
	return nbVisibleChildren > 0;
}


void ToastTile::prepareDraw()
{
	Q_ASSERT(!empty);

	if (texture.isNull())
	{
		//qDebug() << "load texture" << imagePath;
		StelTextureMgr& texMgr=StelApp::getInstance().getTextureManager();
		texture = texMgr.createTextureThread(imagePath, StelTexture::StelTextureParams(true));
	}
	if (texture.isNull() || (!texture->isLoading() && !texture->canBind() && !texture->getErrorMessage().isEmpty()))
	{
		if (!texture.isNull())
			qDebug() << "can't get texture" << imagePath << texture->getErrorMessage();
		empty = true;
		return;
	}
	if (!texture->canBind())
		return;
	// Get the opengl arrays
	if (vertexArray.empty())
	{
		int ml = qMin(qMax(3, level+1), getGrid()->getMaxLevel());
		vertexArray = getGrid()->getVertexArray(level, x, y, ml);
		textureArray = getGrid()->getTextureArray(level, x, y, ml);
		indexArray = getGrid()->getTrianglesIndex(level, x, y, ml);
	}

	if (subTiles.isEmpty() && level < getSurvey()->getMaxLevel())
	{
		//qDebug() << "Create children";
		// Create the children
		for (int i = 0; i < 2; ++i)
			for (int j = 0; j < 2; ++j)
			{
				int newLvl = level+1;
				int newX = 2 * this->x + i;
				int newY = 2 * this->y + j;
				ToastTile* tile = survey->getCachedTile(newLvl,newX,newY);
				//if(tile)
				//	qDebug()<<"Cache hit";
				subTiles.append( tile ? tile : new ToastTile(survey, newLvl, newX, newY) );
			}

		Q_ASSERT(subTiles.size() == 4);
	}
	prepared = true;
}


void ToastTile::drawTile(StelPainter* sPainter)
{
	if (!prepared)
		prepareDraw();

	// Still not ready
	if (texture.isNull() || !texture->bind())
		return;

	if(!readyDraw)
	{
		// Just finished loading all resources, start the fader
		texFader.start();
		readyDraw = true;
	}

	if (texFader.state()==QTimeLine::Running)
	{
		sPainter->setBlending(true);
		sPainter->setColor(1,1,1, texFader.currentValue());
	}
	else
	{
		sPainter->setBlending(false);
		sPainter->setColor(1, 1, 1, 1);
	}

	Q_ASSERT(vertexArray.size() == textureArray.size());

	sPainter->setCullFace(true);
	// sPainter.drawArrays(GL_TRIANGLES, vertexArray.size(), vertexArray.data(), textureArray.data(), Q_NULLPTR, Q_NULLPTR, indexArray.size(), indexArray.constData());
	sPainter->setArrays(vertexArray.constData(), textureArray.constData());
	sPainter->drawFromArray(StelPainter::Triangles, indexArray.size(), 0, true, indexArray.constData());

//	SphericalConvexPolygon poly(getGrid()->getPolygon(level, x, y));
//	sPainter->enableTexture2d(false);
//	sPainter->drawSphericalRegion(&poly, StelPainter::SphericalPolygonDrawModeBoundary);
}


void ToastTile::draw(StelPainter* sPainter, const SphericalCap& viewportShape, int maxVisibleLevel)
{
	if (!isVisible(viewportShape, maxVisibleLevel))
	{
		// Clean up to save memory.
		foreach (ToastTile* child, subTiles)
		{
			//put into cache instead of delete
			//the subtiles of the child remain owned by it
			survey->putIntoCache(child);
		}
		subTiles.clear();
		prepared = false;
		//dont reset the fader
		//readyDraw = false;
		return;
	}
	if (level==maxVisibleLevel || !isCovered(viewportShape))
		drawTile(sPainter);

	// Draw all the children
	foreach (ToastTile* child, subTiles)
	{
		child->draw(sPainter, viewportShape, maxVisibleLevel);
	}
}

/////// ToastSurvey methods ////////////
ToastSurvey::ToastSurvey(const QString& path, int amaxLevel)
	: path(path), maxLevel(amaxLevel), toastCache(200)
{
}

ToastSurvey::~ToastSurvey()
{
	delete rootTile;
	delete grid;
	rootTile = Q_NULLPTR;
}


QString ToastSurvey::getTilePath(int level, int x, int y) const
{
	QString ret = path;
	ret.replace("{level}", QString::number(level));
	ret.replace("{x}", QString::number(x));
	ret.replace("{y}", QString::number(y));
	return ret;
}


void ToastSurvey::draw(StelPainter* sPainter)
{
	// Compute the maximum visible level for the tiles according to the view resolution.
	// We know that each tile at level L represents an angle of 360 / 2**L
	// The maximum angle we want to see is the size of a tile in pixels time the angle for one visible pixel.
	const double anglePerPixel = 1./sPainter->getProjector()->getPixelPerRadAtCenter()*180./M_PI;
	const double maxAngle = anglePerPixel * getTilesSize();
	int maxVisibleLevel = (int)(log2(360. / maxAngle));

	// Lazily creation of the grid and root tile.
	if (!grid) grid = new ToastGrid(maxLevel);
	if (!rootTile) rootTile = new ToastTile(this, 0, 0, 0);

	// We also get the viewport shape to discard invisibly tiles.
	const SphericalCap& viewportRegion = sPainter->getProjector()->getBoundingCap();
	rootTile->draw(sPainter, viewportRegion, maxVisibleLevel);
}


ToastTile* ToastSurvey::getCachedTile(int level, int x, int y)
{
	ToastTile::Coord c = {level, x, y};
	return toastCache.take(c);
}


void ToastSurvey::putIntoCache(ToastTile *tile)
{
	//TODO: calculate varying cost depending on tile level and/or loaded subtiles?
	toastCache.insert(tile->getCoord(),tile);
}
