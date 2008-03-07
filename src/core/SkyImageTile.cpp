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

#include "SkyImageTile.hpp"
#include "QtJsonParser.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <stdexcept>

// Constructor
SkyImageTile::SkyImageTile(const QString& url, SkyImageTile* parent) : errorOccured(false)
{
	if (!url.startsWith("http://"))
	{
		// Assume a local file
		QString fileName;
		try
		{
			fileName = StelApp::getInstance().getFileMgr().findFile(url);
		}
		catch (std::exception e)
		{
			try
			{
				fileName = StelApp::getInstance().getFileMgr().findFile(parent->getBaseUrl()+url);
			}
			catch (std::exception e)
			{
				qWarning() << "WARNING : Can't find JSON Image Tile description: " << url << ": " << e.what() << endl;
				errorOccured = true;
				return;
			}
		}
		QFile f(fileName);
		f.open(QIODevice::ReadOnly);
		try
		{
			loadFromJSON(f);
		}
		catch (std::exception e)
		{
			qWarning() << "WARNING : Can't parse JSON Image Tile description: " << fileName << ": " << e.what() << endl;
			errorOccured = true;
			f.close();
			return;
		}
		f.close();
		QFileInfo finf(fileName);
		baseUrl = finf.absolutePath();
	}
	else
	{
		assert(0); // TODO, manage remote loading
	}
}

// Destructor
SkyImageTile::~SkyImageTile()
{
}
	
// Draw the image on the screen.
void SkyImageTile::draw(StelCore* core)
{
}
	
// Load the tile information from a JSON file
void SkyImageTile::loadFromJSON(QIODevice& input)
{
	QtJsonParser parser;
	QVariantMap map = parser.parse(input).toMap();
	if (map.isEmpty())
		throw std::runtime_error("WARNING: empty JSON file, cannot load image tile");
	credits = map.value("credits").toString();
	infoUrl = map.value("infoUrl").toString();
	imageUrl = map.value("imageUrl").toString();
	bool ok=false;
	minResolution = map.value("minResolution").toDouble(&ok);
	if (!ok)
		throw std::runtime_error("WARNING: minResolution expect a double value");
	
	// Load the convex polygons
	QVariantList polyList = map.value("skyConvexPolygons").toList();
	foreach (const QVariant& polyRaDec, polyList)
	{
		QList<Vec3d> vertices;
		foreach (QVariant vRaDec, polyRaDec.toList())
		{
			const QVariantList vl = vRaDec.toList();
			Vec3d v;
			StelUtils::sphe_to_rect(vl.at(0).toDouble(&ok), vl.at(1).toDouble(&ok), v);
			if (!ok)
				throw std::runtime_error("WARNING: wrong Ra and Dec, expect a double value");
			vertices.append(v);
		}
		assert(vertices.size()==4);
		skyConvexPolygons.append(StelGeom::ConvexPolygon(vertices[0], vertices[1], vertices[2], vertices[3]));
	}
	
	// Load the matching textures positions
	polyList = map.value("textureCoords").toList();
	foreach (const QVariant& polyXY, polyList)
	{
		QList<Vec2f> vertices;
		foreach (QVariant vXY, polyXY.toList())
		{
			const QVariantList vl = vXY.toList();
			vertices.append(Vec2f(vl.at(0).toDouble(&ok), vl.at(1).toDouble(&ok)));
			if (!ok)
				throw std::runtime_error("WARNING: wrong X and Y, expect a double value");
		}
		assert(vertices.size()==4);
		textureCoords.append(vertices);
	}
	
	if (skyConvexPolygons.size()!=textureCoords.size())
		throw std::runtime_error("WARNING: the number of convex polygons does not match the number of texture space polygon");
	
	QVariantList subTiles = map.value("subTiles").toList();
	foreach (QVariant subTile, subTiles)
	{
		subTilesUrls.append(subTile.toString());
	}
}
