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
#include "StelCore.hpp"
#include "StelTextureMgr.hpp"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QHttp>
#include <QUrl>
#include <QBuffer>
#include <stdexcept>

// Constructor
SkyImageTile::SkyImageTile(const QString& url, SkyImageTile* parent) : errorOccured(false), http(NULL), downloading(false), downloadId(0)
{
	lastTimeDraw = StelApp::getInstance().getTotalRunTime();
	if (!url.startsWith("http://") && !parent->getBaseUrl().startsWith("http://"))
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
			catch (std::runtime_error e)
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
		catch (std::runtime_error e)
		{
			qWarning() << "WARNING : Can't parse JSON Image Tile description: " << fileName << ": " << e.what() << endl;
			errorOccured = true;
			f.close();
			return;
		}
		f.close();
		QFileInfo finf(fileName);
		baseUrl = finf.absolutePath()+'/';
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

// Destructor
SkyImageTile::~SkyImageTile()
{
	
}
	
// Draw the image on the screen.
void SkyImageTile::draw(StelCore* core, const StelGeom::ConvexPolygon& viewPortPoly, bool recheckIntersect)
{
	lastTimeDraw = StelApp::getInstance().getTotalRunTime();
	
	if (errorOccured)
		return;
	
	if (downloading)
		return;
	
	if (!tex)
	{
		StelTextureMgr& texMgr=StelApp::getInstance().getTextureManager();
		texMgr.setDefaultParams();
		
		tex = texMgr.createTextureThread(baseUrl+imageUrl);
		if (!tex)
		{
			errorOccured = true;
			return;
		}
	}
	
	if (!tex->bind())
		return;
	
	const float factorX = tex->getCoordinates()[2][0];
	const float factorY = tex->getCoordinates()[2][1];
	
	// Check that we are in the screen
	bool fullInScreen = true;
	bool intersectScreen = false;
	if (recheckIntersect)
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

	if (fullInScreen==false && intersectScreen==false)
		return;
	
	// Draw the real texture for this image
	Projector* prj = core->getProjection();
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(1.0,1.0,1.0,1.0);
	
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
			
			glTexCoord2f(texCoords[idx][0]*factorX, texCoords[idx][1]*factorY);
			prj->project(poly[idx],win);
			glVertex3dv(win);
		}
		glEnd();
	}
	
	// Check if we reach the resolution limit
	const double degPerPixel = 1./prj->getPixelPerRadAtCenter()*180./M_PI;
	if (degPerPixel < minResolution && !subTilesUrls.isEmpty())
	{
		if (subTiles.isEmpty())
		{
			// Load the sub tiles because we reached the maximum resolution
			// and they are not yet loaded
			foreach (QString url, subTilesUrls)
			{
				subTiles.append(new SkyImageTile(url, this));
			}
		}
		else
		{
			// Draw the subtiles
			foreach (SkyImageTile* tile, subTiles)
			{
				tile->draw(core, viewPortPoly, !fullInScreen);
			}
		}
	}
}
	
// Load the tile information from a JSON file
void SkyImageTile::loadFromJSON(QIODevice& input)
{
	QtJsonParser parser;
	QVariantMap map = parser.parse(input).toMap();
	if (map.isEmpty())
		throw std::runtime_error("empty JSON file, cannot load image tile");
	credits = map.value("credits").toString();
	infoUrl = map.value("infoUrl").toString();
	imageUrl = map.value("imageUrl").toString();
	bool ok=false;
	minResolution = map.value("minResolution").toDouble(&ok);
	if (!ok)
		throw std::runtime_error("minResolution expect a double value");
	
	// Load the convex polygons
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
				throw std::runtime_error("wrong X and Y, expect a double value");
		}
		assert(vertices.size()==4);
		textureCoords.append(vertices);
	}
	
	if (skyConvexPolygons.size()!=textureCoords.size())
		throw std::runtime_error("the number of convex polygons does not match the number of texture space polygon");
	
	QVariantList subTilesl = map.value("subTiles").toList();
	foreach (QVariant subTile, subTilesl)
	{
		subTilesUrls.append(subTile.toString());
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
}

// Delete all the subtiles which were not displayed since more than lastDrawTrigger seconds
void SkyImageTile::deleteUnusedTiles(double lastDrawTrigger)
{
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
		// None of the subtiles are displayed: delete all
		foreach (SkyImageTile* tile, subTiles)
		{
			//qWarning() << "Delete " << tile->getImageUrl();
			tile->deleteLater();
		}
		subTiles.clear();
		return;
	}
	
	// Propagate
	foreach (SkyImageTile* tile, subTiles)
	{
		tile->deleteUnusedTiles(lastDrawTrigger);
	}
}
