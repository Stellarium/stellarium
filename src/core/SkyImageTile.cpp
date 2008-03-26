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
#include "STexture.hpp"
#include "Projector.hpp"
#include "ToneReproducer.hpp"
#include "StelCore.hpp"
#include "StelTextureMgr.hpp"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QHttp>
#include <QUrl>
#include <QBuffer>
#include <stdexcept>

#ifdef DEBUG_SKYIMAGE_TILE
 #include "SFont.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
 SFont* SkyImageTile::debugFont = NULL;
#endif

// Constructor
SkyImageTile::SkyImageTile(const QString& url, SkyImageTile* parent) : QObject(parent), luminance(-1), noTexture(false), errorOccured(false), http(NULL), downloading(false), downloadId(0)
{
	lastTimeDraw = StelApp::getInstance().getTotalRunTime();
	if (!url.startsWith("http://") && (parent==NULL || !parent->getBaseUrl().startsWith("http://")))
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
				if (parent==NULL)
					throw std::runtime_error("NULL parent");
				fileName = StelApp::getInstance().getFileMgr().findFile(parent->getBaseUrl()+url);
			}
			catch (std::runtime_error e)
			{
				qWarning() << "WARNING : Can't find JSON Image Tile description: " << url << ": " << e.what() << endl;
				errorOccured = true;
				return;
			}
		}
		QFileInfo finf(fileName);
		baseUrl = finf.absolutePath()+'/';
		QFile f(fileName);
		f.open(QIODevice::ReadOnly);
		try
		{
			loadFromJSON(f);
		}
		catch (std::runtime_error e)
		{
			qWarning() << "WARNING : Can't parse JSON Image Tile description: " << fileName << ": " << e.what() << endl;
			errorOccured = true;
			f.close();
			return;
		}
		f.close();
	}
	else
	{
		QUrl qurl;
		if (url.startsWith("http://"))
		{
			qurl.setUrl(url);
		}
		else
		{
			assert(parent->getBaseUrl().startsWith("http://"));
			qurl.setUrl(parent->getBaseUrl()+url);
		}
		http = new QHttp(this);
		connect(http, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadFinished(int, bool)));
		http->setHost(qurl.host(), qurl.port(80));
		downloading = true;
		downloadId = http->get(qurl.toEncoded());
		QString turl = qurl.toString();
		baseUrl = turl.left(turl.lastIndexOf('/')+1);
	}
}

// Constructor from a map used for JSON files with more than 1 level
SkyImageTile::SkyImageTile(const QVariantMap& map, SkyImageTile* parent) : QObject(parent), luminance(-1), noTexture(false), errorOccured(false), http(NULL), downloading(false), downloadId(0)
{
	if (parent!=NULL)
	{
		baseUrl = parent->getBaseUrl();
	}
	loadFromQVariantMap(map);
}
	
// Destructor
SkyImageTile::~SkyImageTile()
{
}

void SkyImageTile::draw(StelCore* core)
{
	Projector* prj = core->getProjection();
	QMultiMap<double, SkyImageTile*> result;
	getTilesToDraw(result, core, prj->getViewportConvexPolygon(0, 0), true);
	
	QMap<double, SkyImageTile*>::Iterator i = result.end();
	while (i!=result.begin())
	{
		--i;
		i.value()->drawTile(core);
	}
	
	deleteUnusedTiles();
}
	
			
// Draw the image on the screen.
void SkyImageTile::getTilesToDraw(QMultiMap<double, SkyImageTile*>& result, StelCore* core, const StelGeom::ConvexPolygon& viewPortPoly, bool recheckIntersect)
{
	lastTimeDraw = StelApp::getInstance().getTotalRunTime();
	
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
			// If no polygon is defined, we assume that the tile cover the whole sky
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
		return;
	
	if (noTexture==false)
	{
		if (!tex)
		{
			// The tile has an associated texture, but it is not yet loaded: load it now
			StelTextureMgr& texMgr=StelApp::getInstance().getTextureManager();
			texMgr.setDefaultParams();
			// static int countG=0;
			// qWarning() << countG++;
			QString fullTexFileName;
			try
			{
				fullTexFileName = StelApp::getInstance().getFileMgr().findFile(baseUrl+imageUrl);
			}
			catch (std::runtime_error er)
			{
				// Maybe the user meant a file in stellarium loical files
				fullTexFileName = imageUrl;
			}
			tex = texMgr.createTextureThread(fullTexFileName);
			if (!tex)
			{
				qWarning() << "WARNING : Can't create tile: " << baseUrl+imageUrl;
				errorOccured = true;
				return;
			}
		}
		
		// Every test passed :) The tile will be displayed
		result.insert(minResolution, this);
		
		// Check that the texture is now fully loaded before trying to load sub tiles
		if (tex->canBind()==false)
			return;
	}
	
	// Check if we reach the resolution limit
	Projector* prj = core->getProjection();
	const double degPerPixel = 1./prj->getPixelPerRadAtCenter()*180./M_PI;
	if (degPerPixel < minResolution)
	{
		if (subTiles.isEmpty())
		{
			if (!subTilesUrls.isEmpty())
			{
				// Load the sub tiles because we reached the maximum resolution
				// and they are not yet loaded
				foreach (QString url, subTilesUrls)
				{
					subTiles.append(new SkyImageTile(url, this));
				}
			}
		}
		else
		{
			// Draw the subtiles
			foreach (SkyImageTile* tile, subTiles)
			{
				tile->getTilesToDraw(result, core, viewPortPoly, !fullInScreen);
			}
		}
	}
}
	
// Draw the image on the screen.
void SkyImageTile::drawTile(StelCore* core)
{
	if (!tex->bind())
		return;

	Projector* prj = core->getProjection();
	
	if (luminance>0)
	{
		float ad_lum=core->getToneReproducer()->adaptLuminance(luminance);
		glColor3f(ad_lum,ad_lum,ad_lum);
	}
	
	const float factorX = tex->getCoordinates()[2][0];
	const float factorY = tex->getCoordinates()[2][1];

	// Draw the real texture for this image
	glEnable(GL_TEXTURE_2D);
#if 1
	for (int p=0;p<skyConvexPolygons.size();++p)
	{
		const StelGeom::Polygon& poly = skyConvexPolygons.at(p).asPolygon();
		const QList<Vec2f>& texCoords = textureCoords.at(p);
				
		assert((int)poly.size()==texCoords.size());
						
		Vec3d win;
		const int N=poly.size()-1;
		int idx=N;
		int diff = 0;
				// Using TRIANGLE STRIP requires to use the following vertex order N-0,0,N-1,1,N-2,2 etc..
		glBegin(GL_TRIANGLE_STRIP);
		for (int i=0;i<=N;++i)
		{
			idx = (diff==0 ? N-i/2 : i/2);
			++diff;
			if (diff>1) diff=0;
					
			glTexCoord2d(texCoords[idx][0]*factorX, texCoords[idx][1]*factorY);
			prj->project(poly[idx],win);
			glVertex3dv(win);
		}
		glEnd();
	}
#endif
#if 0
	if (debugFont==NULL)
	{
		debugFont = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), 12);
	}
	foreach (const StelGeom::ConvexPolygon& poly, skyConvexPolygons)
	{
		Vec3d win;
		Vec3d bary = poly.getBarycenter();
		prj->project(bary,win);
		prj->drawText(debugFont, win[0], win[1], getImageUrl());
		
		glDisable(GL_TEXTURE_2D);
		prj->drawPolygon(poly);
		glEnable(GL_TEXTURE_2D);
	}
#endif
}

// Load the tile information from a JSON file
void SkyImageTile::loadFromJSON(QIODevice& input)
{
	QtJsonParser parser;
	QVariantMap map = parser.parse(input).toMap();
	if (map.isEmpty())
		throw std::runtime_error("empty JSON file, cannot load image tile");
	loadFromQVariantMap(map);
}

// Load the tile from a valid QVariantMap
void SkyImageTile::loadFromQVariantMap(const QVariantMap& map)
{
	credits = map.value("credits").toString();
	infoUrl = map.value("infoUrl").toString();
	bool ok=false;
	minResolution = map.value("minResolution").toDouble(&ok);
	if (!ok)
		throw std::runtime_error("minResolution expect a double value");
	
	if (map.contains("luminance"))
	{
		luminance = map.value("luminance").toDouble(&ok);
		if (!ok)
			throw std::runtime_error("luminance expect a float value");
	}
	
	// Load the convex polygons (if any)
	QVariantList polyList = map.value("skyConvexPolygons").toList();
	foreach (const QVariant& polyRaDec, polyList)
	{
		QList<Vec3d> vertices;
		foreach (QVariant vRaDec, polyRaDec.toList())
		{
			const QVariantList vl = vRaDec.toList();
			Vec3d v;
			StelUtils::sphe_to_rect(vl.at(0).toDouble(&ok)*M_PI/180., vl.at(1).toDouble(&ok)*M_PI/180., v);
			if (!ok)
				throw std::runtime_error("wrong Ra and Dec, expect a double value");
			vertices.append(v);
		}
		assert(vertices.size()==4);
		skyConvexPolygons.append(StelGeom::ConvexPolygon(vertices[0], vertices[1], vertices[2], vertices[3]));
	}
	
	// Load the matching textures positions (if any)
	polyList = map.value("textureCoords").toList();
	foreach (const QVariant& polyXY, polyList)
	{
		QList<Vec2f> vertices;
		foreach (QVariant vXY, polyXY.toList())
		{
			const QVariantList vl = vXY.toList();
			vertices.append(Vec2f(vl.at(0).toDouble(&ok), vl.at(1).toDouble(&ok)));
			if (!ok)
				throw std::runtime_error("wrong X and Y, expect a double value");
		}
		assert(vertices.size()==4);
		textureCoords.append(vertices);
	}
	
	if (map.contains("imageUrl"))
	{
		imageUrl = map.value("imageUrl").toString();
	}
	else
	{
		if (skyConvexPolygons.size()!=textureCoords.size())
			throw std::runtime_error("the number of convex polygons does not match the number of texture space polygon");
		noTexture = true;
	}
	
	QVariantList subTilesl = map.value("subTiles").toList();
	foreach (QVariant subTile, subTilesl)
	{
		// The JSON file contains a nested tile structure
		if (subTile.type()==QVariant::Map)
		{
			subTiles.append(new SkyImageTile(subTile.toMap(), this));
		}
		else
		{
			// This is an URL to the child tile
			subTilesUrls.append(subTile.toString());
		}
	}
}
	
// Called when the download for the JSON file terminated
void SkyImageTile::downloadFinished(int id, bool error)
{
	if (id!=downloadId)
		return;
	//qWarning() << "Downloaded JSON " << http->currentRequest().path();
	downloading = false;
	disconnect(http, SIGNAL(requestFinished(int, bool)), this, SLOT(downloadFinished(int, bool)));
	if (error)
	{
		qWarning() << "WARNING : Problem while downloading JSON Image Tile description: " << http->errorString();
		errorOccured = true;
		return;
	}
	
	try
	{
		QByteArray content = http->readAll();
		QBuffer buf(&content);
		buf.open(QIODevice::ReadOnly);
		loadFromJSON(buf);
		buf.close();
	}
	catch (std::runtime_error e)
	{
		qWarning() << "WARNING : Can't parse loaded JSON Image Tile description: " << e.what();
		errorOccured = true;
	}
	http->close();
}

// Delete all the subtiles which were not displayed since more than lastDrawTrigger seconds
void SkyImageTile::deleteUnusedTiles(double lastDrawTrigger)
{
	if (subTiles.isEmpty())
		return;
	
	double now = StelApp::getInstance().getTotalRunTime();
	bool deleteAll = true;
	foreach (SkyImageTile* tile, subTiles)
	{
		// At least one of the subtiles is displayed
		if (now-tile->getLastTimeDraw()<lastDrawTrigger)
		{
			deleteAll = false;
			break;
		}
	}
	
	if (deleteAll==true)
	{
		// If there is no subTilesUrls stored it means that the tile description was
		// embeded into the same JSON file as the parent. Therefore it cannot be deleted
		// without deleting also the parent because it couldn't be reloaded alone.
		const bool removeOnlyTextures = subTilesUrls.isEmpty();
		
		if (removeOnlyTextures)
		{
			foreach (SkyImageTile* tile, subTiles)
			{
				tile->deleteTexture();
				tile->deleteUnusedTiles(lastDrawTrigger);
			}
		}
		else
		{
			// None of the subtiles are displayed: delete all
			foreach (SkyImageTile* tile, subTiles)
			{
				//qWarning() << "Delete " << tile->getImageUrl();
				tile->deleteLater();
			}
			subTiles.clear();
		}
		return;
	}
	
	// Propagate
	foreach (SkyImageTile* tile, subTiles)
	{
		tile->deleteUnusedTiles(lastDrawTrigger);
	}
}
