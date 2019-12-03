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
#include "StelSkyDrawer.hpp"
#include "StelModuleMgr.hpp"
#include "RefractionExtinction.hpp"
#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelToast.hpp"
#include "LandscapeMgr.hpp"

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
	for (auto* child : subTiles)
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
	for (const auto* child : subTiles)
	{
		if (!viewportShape.intersects(child->boundingCap))
			continue;
		nbVisibleChildren++;
		if (!child->readyDraw || child->texFader.state()==QTimeLine::Running)
			return false;
	}
	return nbVisibleChildren > 0;
}


void ToastTile::prepareDraw(Vec3f color)
{
	Q_ASSERT(!empty);

	StelSkyDrawer *drawer=StelApp::getInstance().getCore()->getSkyDrawer();
	const bool withExtinction=(drawer->getFlagHasAtmosphere() && drawer->getExtinction().getExtinctionCoefficient()>=0.01f);

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
		colorArray.clear();
		for (int i=0; i<vertexArray.size(); ++i)
			colorArray.append(color);
	}
	// Recreate the color array in any case. Assume we must compute extinction on every frame.
	if (withExtinction)
	{
		StelCore *core=StelApp::getInstance().getCore();
		// We must process the vertices to find geometric altitudes in order to compute vertex colors.
		const Extinction& extinction=drawer->getExtinction();
		colorArray.clear();

		for (int i=0; i<vertexArray.size(); ++i)
		{
			Vec3d vertAltAz=core->j2000ToAltAz(vertexArray.at(i), StelCore::RefractionOn);
			Q_ASSERT(fabs(vertAltAz.lengthSquared()-1.0) < 0.001);

			float oneMag=0.0f;
			extinction.forward(vertAltAz, &oneMag);
			// drop of one magnitude: should be factor 2.5 or 40%. We take 70% to keep it more visible.
			// Also, for Toast, we do not observe Bortle as for the default MilkyWay.
			float extinctionFactor=std::pow(0.7f , oneMag);
			Vec3f thisColor=Vec3f(color[0]*extinctionFactor, color[1]*extinctionFactor, color[2]*extinctionFactor);
			colorArray.append(thisColor);
		}
	}
	else
	{
		colorArray.fill(Vec3f(1.0f));
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


void ToastTile::drawTile(StelPainter* sPainter, Vec3f color)
{
	prepareDraw(color);

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
	}
	else
	{
		sPainter->setBlending(false);
	}

	Q_ASSERT(vertexArray.size() == textureArray.size());

	sPainter->setCullFace(true);
	sPainter->setArrays(vertexArray.constData(), textureArray.constData(), colorArray.constData());
	sPainter->drawFromArray(StelPainter::Triangles, indexArray.size(), 0, true, indexArray.constData());

//	SphericalConvexPolygon poly(getGrid()->getPolygon(level, x, y));
//	sPainter->enableTexture2d(false);
//	sPainter->drawSphericalRegion(&poly, StelPainter::SphericalPolygonDrawModeBoundary);
}


void ToastTile::draw(StelPainter* sPainter, const SphericalCap& viewportShape, int maxVisibleLevel, Vec3f color)
{
	if (!isVisible(viewportShape, maxVisibleLevel))
	{
		// Clean up to save memory.
		for (auto* child : subTiles)
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
		drawTile(sPainter, color);

	// Draw all the children
	for (auto* child : subTiles)
	{
		child->draw(sPainter, viewportShape, maxVisibleLevel, color);
	}
}

/////// ToastSurvey methods ////////////
ToastSurvey::ToastSurvey(const QString& path, int amaxLevel)
	: grid(Q_NULLPTR), path(path), rootTile(Q_NULLPTR), maxLevel(amaxLevel), toastCache(200)
{
}

ToastSurvey::~ToastSurvey()
{
	delete rootTile;
	rootTile = Q_NULLPTR;
	delete grid;
	grid = Q_NULLPTR;
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
	const double anglePerPixel = 1./static_cast<double>(sPainter->getProjector()->getPixelPerRadAtCenter())*M_180_PI;
	const double maxAngle = anglePerPixel * getTilesSize();
	int maxVisibleLevel = static_cast<int>(log2(360. / maxAngle));

	// Lazily creation of the grid and root tile.
	if (!grid) grid = new ToastGrid(maxLevel);
	if (!rootTile) rootTile = new ToastTile(this, 0, 0, 0);

	// Compute global brightness depending on sky/atmosphere. (taken from MilkyWay, but without extra Bortle stuff)
	StelCore *core=StelApp::getInstance().getCore();
	StelSkyDrawer *drawer=core->getSkyDrawer();
	const bool withExtinction=(drawer->getFlagHasAtmosphere() && drawer->getExtinction().getExtinctionCoefficient()>=0.01f);
	Vec3f color(1.0f);

	if (withExtinction)
	{
		const float lum = drawer->surfaceBrightnessToLuminance(12.f); // How to calibrate the DSS texture?

		// Get the luminance scaled between 0 and 1
		StelToneReproducer* eye = core->getToneReproducer();
		float aLum =eye->adaptLuminanceScaled(lum);

		// Bound a maximum luminance. GZ: Is there any reference/reason, or just trial and error?
		aLum = qMin(1.0f, aLum*2.f); // Was 0.38 for MilkyWay. TOAST is allowed to look a bit artificial though...
		color.set(aLum, aLum, aLum);

		// adapt brightness by atmospheric brightness. This block developed for ZodiacalLight, hopefully similarly applicable...
		const float atmLum = GETSTELMODULE(LandscapeMgr)->getAtmosphereAverageLuminance();
		// 10cd/m^2 at sunset, 3.3 at civil twilight (sun at -6deg). 0.0145 sun at -12, 0.0004 sun at -18,  0.01 at Full Moon!?
		//qDebug() << "AtmLum: " << atmLum;
		float atmFactor=qMax(0.35f, 50.0f*(0.02f-atmLum)); // keep visible in twilight, but this is enough for some effect with the moon.
		color*=atmFactor*atmFactor;

		if (color[0]<0) color[0]=0;
		if (color[1]<0) color[1]=0;
		if (color[2]<0) color[2]=0;
	}

	// We also get the viewport shape to discard invisible tiles.
	const SphericalCap& viewportRegion = sPainter->getProjector()->getBoundingCap();
	rootTile->draw(sPainter, viewportRegion, maxVisibleLevel, color);
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
