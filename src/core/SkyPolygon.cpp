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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "SkyPolygon.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "Projector.hpp"
#include "StelCore.hpp"

#include <stdexcept>
#include <QDebug>

void SkyPolygon::initCtor()
{
	minResolution = -1;
	texFader = NULL;
}

// Constructor
SkyPolygon::SkyPolygon(const QString& url, SkyPolygon* parent) : MultiLevelJsonBase(parent)
{
	initCtor();
	initFromUrl(url);
}

// Constructor from a map used for JSON files with more than 1 level
SkyPolygon::SkyPolygon(const QVariantMap& map, SkyPolygon* parent) : MultiLevelJsonBase(parent)
{
	initCtor();
	initFromQVariantMap(map);
}
	
// Destructor
SkyPolygon::~SkyPolygon()
{
}
	
void SkyPolygon::draw(StelCore* core)
{
	Projector* prj = core->getProjection();
	
	QMultiMap<double, SkyPolygon*> result;
	getTilesToDraw(result, core, prj->getViewportConvexPolygon(0, 0), true);
	
	// Draw in the good order
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_ONE, GL_ONE);
	QMap<double, SkyPolygon*>::Iterator i = result.end();
	while (i!=result.begin())
	{
		--i;
		i.value()->drawTile(core);
	}
	
	deleteUnusedSubTiles();
}
	
// Return the list of tiles which should be drawn.
void SkyPolygon::getTilesToDraw(QMultiMap<double, SkyPolygon*>& result, StelCore* core, const StelGeom::ConvexPolygon& viewPortPoly, bool recheckIntersect)
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
			foreach (const StelGeom::ConvexPolygon poly, skyConvexPolygons)
			{
				if (contains(viewPortPoly, poly))
				{
					intersectScreen = true;
				}
				else
				{
					fullInScreen = false;
					if (intersect(viewPortPoly, poly))
						intersectScreen = true;
				}
			}
		}
	}
	// The tile is outside screen
	if (fullInScreen==false && intersectScreen==false)
	{
		// Schedule a deletion
		scheduleDeletion();
		return;
	}
	
	// The tile is in screen, make sure that it's not going to be deleted
	cancelDeletion();
		
	// The tile is in screen and has a texture: every test passed :) The tile will be displayed
	result.insert(minResolution, this);
	
	// Check if we reach the resolution limit
	const double degPerPixel = 1./core->getProjection()->getPixelPerRadAtCenter()*180./M_PI;
	if (degPerPixel < minResolution)
	{
		if (subTiles.isEmpty() && !subTilesUrls.isEmpty())
		{
			// Load the sub tiles because we reached the maximum resolution and they are not yet loaded
			foreach (QVariant s, subTilesUrls)
			{
				SkyPolygon* nt;
				if (s.type()==QVariant::Map)
					nt = new SkyPolygon(s.toMap(), this);
				else
				{
					Q_ASSERT(s.type()==QVariant::String);
					nt = new SkyPolygon(s.toString(), this);
				}
				subTiles.append(nt);
			}
		}
		// Try to add the subtiles
		foreach (MultiLevelJsonBase* tile, subTiles)
		{
			qobject_cast<SkyPolygon*>(tile)->getTilesToDraw(result, core, viewPortPoly, !fullInScreen);
		}
	}
	else
	{
		foreach (MultiLevelJsonBase* tile, subTiles)
		{
			tile->scheduleDeletion();
		}
	}
}
	
// Draw the image on the screen.
// Assume GL_TEXTURE_2D is enabled
bool SkyPolygon::drawTile(StelCore* core)
{	
	if (!texFader)
	{
		texFader = new QTimeLine(1000, this);
		texFader->start();
	}
	
	const Projector* prj = core->getProjection();
	
	foreach (const StelGeom::ConvexPolygon& poly, skyConvexPolygons)
		prj->drawPolygon(poly);
	
	return true;
}

// Load the tile from a valid QVariantMap
void SkyPolygon::loadFromQVariantMap(const QVariantMap& map)
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
	minResolution = map.value("minResolution").toDouble(&ok);
	if (!ok)
		throw std::runtime_error("minResolution expect a double value");
	
	// Load the convex polygons (if any)
	QVariantList polyList = map.value("worldCoords").toList();
	foreach (const QVariant& polyRaDec, polyList)
	{
		QList<Vec3d> vertices;
		foreach (QVariant vRaDec, polyRaDec.toList())
		{
			const QVariantList vl = vRaDec.toList();
			Vec3d v;
			StelUtils::spheToRect(vl.at(0).toDouble(&ok)*M_PI/180., vl.at(1).toDouble(&ok)*M_PI/180., v);
			if (!ok)
				throw std::runtime_error("wrong Ra and Dec, expect a double value");
			vertices.append(v);
		}
		Q_ASSERT(vertices.size()==4);
		skyConvexPolygons.append(StelGeom::ConvexPolygon(vertices[0], vertices[1], vertices[2], vertices[3]));
	}
	
	// This is a list of URLs to the child tiles or a list of already loaded map containing child information
	// (in this later case, the SkyPolygon objects will be created later)
	subTilesUrls = map.value("subTiles").toList();
}

// Convert the image informations to a map following the JSON structure.
QVariantMap SkyPolygon::toQVariantMap() const
{
	Q_ASSERT(0);
	return QVariantMap();
}
