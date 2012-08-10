/*
 * Copyright (C) 2008 Fabien Chereau
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


#include "renderer/StelRenderer.hpp"
#include "StelSkyPolygon.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelProjector.hpp"
#include "StelCore.hpp"

#include <stdexcept>
#include <QDebug>

void StelSkyPolygon::initCtor()
{
	minResolution = -1;
	texFader = NULL;
}

// Constructor
StelSkyPolygon::StelSkyPolygon(const QString& url, StelSkyPolygon* parent) : MultiLevelJsonBase(parent)
{
	initCtor();
	initFromUrl(url);
}

// Constructor from a map used for JSON files with more than 1 level
StelSkyPolygon::StelSkyPolygon(const QVariantMap& map, StelSkyPolygon* parent) : MultiLevelJsonBase(parent)
{
	initCtor();
	initFromQVariantMap(map);
}

// Destructor
StelSkyPolygon::~StelSkyPolygon()
{
}

void StelSkyPolygon::draw(StelCore* core, StelRenderer* renderer, StelProjectorP projector, float)
{
	QMultiMap<double, StelSkyPolygon*> result;
	getTilesToDraw(result, core, projector->getViewportConvexPolygon(0, 0), true);

	// Draw in the right order
	renderer->setBlendMode(BlendMode_Add);
	QMap<double, StelSkyPolygon*>::Iterator i = result.end();
	while (i!=result.begin())
	{
		--i;
		i.value()->drawTile(renderer, projector);
	}

	deleteUnusedSubTiles();
}

// Return the list of tiles which should be drawn.
void StelSkyPolygon::getTilesToDraw(QMultiMap<double, StelSkyPolygon*>& result, StelCore* core, const SphericalRegionP& viewPortPoly, bool recheckIntersect)
{
	// An error occured during loading
	if (errorOccured)
		return;

	// The JSON file is currently being downloaded
	if (downloading)
		return;

	// Check that we are in the screen
	bool fullInScreen = true;
	bool intersectScreen = false;
	if (recheckIntersect)
	{
		if (skyConvexPolygons.isEmpty())
		{
			// If no polygon is defined, we assume that the tile covers the whole sky
			fullInScreen=false;
			intersectScreen=true;
		}
		else
		{
			foreach (const SphericalConvexPolygon poly, skyConvexPolygons)
			{
				if (viewPortPoly->contains(poly))
				{
					intersectScreen = true;
				}
				else
				{
					fullInScreen = false;
					if (viewPortPoly->intersects(poly))
						intersectScreen = true;
				}
			}
		}
	}
	// The tile is outside screen
	if (fullInScreen==false && intersectScreen==false)
	{
		// Schedule a deletion
		scheduleChildsDeletion();
		return;
	}

	// The tile is in screen, make sure that it's not going to be deleted
	cancelDeletion();

	// The tile is in screen and has a texture: every test passed :) The tile will be displayed
	result.insert(minResolution, this);

	// Check if we reach the resolution limit
	const double degPerPixel = 1./core->getProjection(StelCore::FrameJ2000)->getPixelPerRadAtCenter()*180./M_PI;
	if (degPerPixel < minResolution)
	{
		if (subTiles.isEmpty() && !subTilesUrls.isEmpty())
		{
			// Load the sub tiles because we reached the maximum resolution and they are not yet loaded
			foreach (QVariant s, subTilesUrls)
			{
				StelSkyPolygon* nt;
				if (s.type()==QVariant::Map)
					nt = new StelSkyPolygon(s.toMap(), this);
				else
				{
					Q_ASSERT(s.type()==QVariant::String);
					nt = new StelSkyPolygon(s.toString(), this);
				}
				subTiles.append(nt);
			}
		}
		// Try to add the subtiles
		foreach (MultiLevelJsonBase* tile, subTiles)
		{
			qobject_cast<StelSkyPolygon*>(tile)->getTilesToDraw(result, core, viewPortPoly, !fullInScreen);
		}
	}
	else
	{
		scheduleChildsDeletion();
	}
}

// Draw the image on the screen.
// Assume GL_TEXTURE_2D is enabled
bool StelSkyPolygon::drawTile(StelRenderer* renderer, StelProjectorP projector)
{
	if (!texFader)
	{
		texFader = new QTimeLine(1000, this);
		texFader->start();
	}

	for(int poly = 0; poly < skyConvexPolygons.size(); ++poly)
	{
		skyConvexPolygons[poly].drawFill(renderer, SphericalRegion::DrawParams(&(*projector)));
	}

	return true;
}

// Load the tile from a valid QVariantMap
void StelSkyPolygon::loadFromQVariantMap(const QVariantMap& map)
{
	if (map.contains("imageCredits"))
	{
		QVariantMap dsCredits = map.value("imageCredits").toMap();
		dataSetCredits.shortCredits = dsCredits.value("short").toString();
		dataSetCredits.fullCredits = dsCredits.value("full").toString();
		dataSetCredits.infoURL = dsCredits.value("infoUrl").toString();
	}
	if (map.contains("serverCredits"))
	{
		QVariantMap sCredits = map.value("serverCredits").toMap();
		serverCredits.shortCredits = sCredits.value("short").toString();
		serverCredits.fullCredits = sCredits.value("full").toString();
		serverCredits.infoURL = sCredits.value("infoUrl").toString();
	}

	shortName = map.value("shortName").toString();
	bool ok=false;
	minResolution = map.value("minResolution").toFloat(&ok);
	if (!ok)
		throw std::runtime_error(qPrintable(QString("minResolution expect a double value, found: %1").arg(map.value("minResolution").toString())));

	// Load the convex polygons (if any)
	QVariantList polyList = map.value("worldCoords").toList();
	foreach (const QVariant& polyRaDec, polyList)
	{
		QList<Vec3d> vertices;
		foreach (QVariant vRaDec, polyRaDec.toList())
		{
			const QVariantList vl = vRaDec.toList();
			Vec3d v;
			StelUtils::spheToRect(vl.at(0).toFloat(&ok)*M_PI/180.f, vl.at(1).toFloat(&ok)*M_PI/180.f, v);
			if (!ok)
				throw std::runtime_error("wrong Ra and Dec, expect a double value");
			vertices.append(v);
		}
		Q_ASSERT(vertices.size()==4);
		skyConvexPolygons.append(SphericalConvexPolygon(vertices[0], vertices[1], vertices[2], vertices[3]));
	}

	// This is a list of URLs to the child tiles or a list of already loaded map containing child information
	// (in this later case, the StelSkyPolygon objects will be created later)
	subTilesUrls = map.value("subTiles").toList();
}

// Convert the image informations to a map following the JSON structure.
QVariantMap StelSkyPolygon::toQVariantMap() const
{
	Q_ASSERT(0);
	return QVariantMap();
}
