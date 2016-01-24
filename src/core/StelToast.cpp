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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelToast.hpp"

ToastTile::ToastTile(QObject* parent, int level, int x, int y)
	: QObject(parent), level(level), x(x), y(y), empty(false), ready(false), texture(NULL), texFader(NULL)
{
	Q_ASSERT(level <= getGrid()->getMaxLevel());
	const ToastSurvey* survey = getSurvey();
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
		if (!child->ready || child->texFader->state()==QTimeLine::Running)
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
				subTiles.append(new ToastTile(this, level + 1, 2 * this->x + i, 2 * this->y + j));
		Q_ASSERT(subTiles.size() == 4);
	}
	ready = true;
}


void ToastTile::drawTile(StelPainter* sPainter)
{
	if (!ready)
		prepareDraw();

	// Still not ready
	if (texture.isNull() || !texture->bind())
		return;

	if (!texFader)
	{
		texFader = new QTimeLine(1000, this);
		texFader->start();
	}

	if (texFader->state()==QTimeLine::Running)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		glEnable(GL_BLEND);
		sPainter->setColor(1,1,1, texFader->currentValue());
	}
	else
	{
		glDisable(GL_BLEND);
		sPainter->setColor(1, 1, 1, 1);
	}

	sPainter->enableTexture2d(true);
	Q_ASSERT(vertexArray.size() == textureArray.size());

	glEnable(GL_CULL_FACE);
	// sPainter.drawArrays(GL_TRIANGLES, vertexArray.size(), vertexArray.data(), textureArray.data(), NULL, NULL, indexArray.size(), indexArray.constData());
	sPainter->setArrays(vertexArray.constData(), textureArray.constData());
	sPainter->drawFromArray(StelPainter::Triangles, indexArray.size(), 0, true, indexArray.constData());
	glDisable(GL_CULL_FACE);

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
			child->deleteLater();
		}
		subTiles.clear();
		ready = false;
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
	: grid(amaxLevel), path(path), maxLevel(amaxLevel)
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


void ToastSurvey::draw(StelPainter* sPainter)
{
	// Compute the maximum visible level for the tiles according to the view resolution.
	// We know that each tile at level L represents an angle of 360 / 2**L
	// The maximum angle we want to see is the size of a tile in pixels time the angle for one visible pixel.
	const double anglePerPixel = 1./sPainter->getProjector()->getPixelPerRadAtCenter()*180./M_PI;
	const double maxAngle = anglePerPixel * getTilesSize();
	int maxVisibleLevel = (int)(log2(360. / maxAngle));

	// We also get the viewport shape to discard invisibly tiles.
	const SphericalCap& viewportRegion = sPainter->getProjector()->getBoundingCap();
	rootTile->draw(sPainter, viewportRegion, maxVisibleLevel);
}
