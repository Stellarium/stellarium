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
#include "SkyDrawer.hpp"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QHttp>
#include <QUrl>
#include <QBuffer>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <stdexcept>

#ifdef DEBUG_SKYIMAGE_TILE
 #include "SFont.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
 SFont* SkyImageTile::debugFont = NULL;
#endif

/*************************************************************************
  Class used to load a JSON file in a thread
 *************************************************************************/
class JsonLoadThread : public QThread
{
	public:
		JsonLoadThread(SkyImageTile* atile, QByteArray content, bool acompressed=false) : QThread((QObject*)atile),
			tile(atile), data(content), compressed(acompressed) {;}
		virtual void run();
	private:
		SkyImageTile* tile;
		QByteArray data;
		const bool compressed;
};

void JsonLoadThread::run()
{
	try
	{
		QBuffer buf(&data);
		buf.open(QIODevice::ReadOnly);
		tile->temporaryResultMap = SkyImageTile::loadFromJSON(buf, compressed);
	}
	catch (std::runtime_error e)
	{
		qWarning() << "WARNING : Can't parse loaded JSON Image Tile description: " << e.what();
		tile->errorOccured = true;
	}
}

// Constructor
SkyImageTile::SkyImageTile(const QString& url, SkyImageTile* parent) : QObject(parent), luminance(-1), alphaBlend(false), noTexture(false), errorOccured(false), httpReply(NULL), downloading(false), loadThread(NULL), texFader(NULL)
{
	if (parent!=NULL)
	{
		luminance = parent->luminance;
		alphaBlend = parent->alphaBlend;
	}
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
				qWarning() << "WARNING : Can't find JSON Image Tile description: " << url << ": " << e.what();
				errorOccured = true;
				return;
			}
		}
		QFileInfo finf(fileName);
		baseUrl = finf.absolutePath()+'/';
		QFile f(fileName);
		f.open(QIODevice::ReadOnly);
		const bool compressed = fileName.endsWith(".qZ");
		try
		{
			loadFromQVariantMap(loadFromJSON(f, compressed));
		}
		catch (std::runtime_error e)
		{
			qWarning() << "WARNING : Can't parse JSON Image Tile description: " << fileName << ": " << e.what();
			errorOccured = true;
			f.close();
			return;
		}
		f.close();
		lastTimeDraw = StelApp::getInstance().getTotalRunTime();
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
		assert(httpReply==NULL);
		httpReply = StelApp::getInstance().getNetworkAccessManager()->get(QNetworkRequest(qurl));
		connect(httpReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		downloading = true;
		QString turl = qurl.toString();
		baseUrl = turl.left(turl.lastIndexOf('/')+1);
	}
}

// Constructor from a map used for JSON files with more than 1 level
SkyImageTile::SkyImageTile(const QVariantMap& map, SkyImageTile* parent) : QObject(parent), luminance(-1), noTexture(false), errorOccured(false), httpReply(NULL), downloading(false), loadThread(NULL), texFader(NULL)
{
	if (parent!=NULL)
	{
		baseUrl = parent->getBaseUrl();
		luminance = parent->luminance;
		alphaBlend = parent->alphaBlend;
	}
	loadFromQVariantMap(map);
	lastTimeDraw = StelApp::getInstance().getTotalRunTime();
}
	
// Destructor
SkyImageTile::~SkyImageTile()
{
	if (httpReply)
	{
		httpReply->abort();
		delete httpReply;
		httpReply = NULL;
	}
	if (loadThread && loadThread->isRunning())
	{
		disconnect(loadThread, SIGNAL(finished()), this, SLOT(JsonLoadFinished()));
		// The thread is currently running, it needs to be properly stopped
		if (loadThread->wait(500)==false)
		{
			loadThread->terminate();
			//loadThread->wait(2000);
		}
	}
	foreach (SkyImageTile* tile, subTiles)
	{
		delete tile;
	}
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
	
// Return the list of tiles which should be drawn.
void SkyImageTile::getTilesToDraw(QMultiMap<double, SkyImageTile*>& result, StelCore* core, const StelGeom::ConvexPolygon& viewPortPoly, bool recheckIntersect)
{
	// An error occured during loading
	if (errorOccured)
		return;
	
	// The JSON file is currently being downloaded
	if (downloading)
	{
		// Avoid the tile to be deleted by telling that it was drawn
		lastTimeDraw = StelApp::getInstance().getTotalRunTime();
		return;
	}
	
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
		return;
	
	lastTimeDraw = StelApp::getInstance().getTotalRunTime();
	
	if (noTexture==false)
	{
		if (!tex)
		{
			// The tile has an associated texture, but it is not yet loaded: load it now
			StelTextureMgr& texMgr=StelApp::getInstance().getTextureManager();
			texMgr.setDefaultParams();
 			texMgr.setMipmapsMode(true);
			texMgr.setMinFilter(GL_LINEAR);
 			texMgr.setMagFilter(GL_LINEAR);
			// static int countG=0;
			// qWarning() << countG++;
			QString fullTexFileName;
			if (baseUrl.startsWith("http://"))
			{
				fullTexFileName = baseUrl+imageUrl;
			}
			else
			{
				try
				{
					fullTexFileName = StelApp::getInstance().getFileMgr().findFile(baseUrl+imageUrl);
				}
				catch (std::runtime_error er)
				{
					// Maybe the user meant a file in stellarium local files
					fullTexFileName = imageUrl;
				}
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
	if (degPerPixel < minResolution*0.8)
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
			// Try to add the subtiles
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
	float ad_lum=1.f;
	if (luminance>0)
	{
		ad_lum=core->getToneReproducer()->adaptLuminanceScaled(luminance);
		if (ad_lum<0.01)
		{
			return;
		}
	}
	
	if (!tex->bind())
	{
		return;
	}
	
	if (!texFader)
	{
		texFader = new QTimeLine(1000, this);
		texFader->start();
	}
	
	Projector* prj = core->getProjection();
	
	const float factorX = tex->getCoordinates()[2][0];
	const float factorY = tex->getCoordinates()[2][1];

	// Draw the real texture for this image
	glEnable(GL_TEXTURE_2D);
	if (alphaBlend==true || texFader->state()==QTimeLine::Running)
	{
		if (!alphaBlend)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		glEnable(GL_BLEND);
		glColor4f(ad_lum,ad_lum,ad_lum, texFader->currentValue());
	}
	else
	{
		glDisable(GL_BLEND);
		glColor3f(ad_lum,ad_lum,ad_lum);
	}
	
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
#if 0
	if (debugFont==NULL)
	{
		debugFont = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), 12);
	}
	glColor3f(1.0,0.5,0.5);
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
	if (!alphaBlend)
		glBlendFunc(GL_ONE, GL_ONE); // Revert
}

// Load the tile information from a JSON file
QVariantMap SkyImageTile::loadFromJSON(QIODevice& input, bool compressed)
{
	QtJsonParser parser;
	QVariantMap map;
	if (compressed && input.size()>0)
	{
		QByteArray ar = qUncompress(input.readAll());
		input.close();
		QBuffer buf(&ar);
		buf.open(QIODevice::ReadOnly);
		map = parser.parse(buf).toMap();
		buf.close();
	}
	else
	{
		map = parser.parse(input).toMap();
	}
	
	if (map.isEmpty())
		throw std::runtime_error("empty JSON file, cannot load image tile");
	return map;
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
	
	if (map.contains("maxBrightness"))
	{
		luminance = map.value("maxBrightness").toDouble(&ok);
		if (!ok)
			throw std::runtime_error("maxBrightness expect a float value");
		luminance = StelApp::getInstance().getCore()->getSkyDrawer()->surfacebrightnessToLuminance(luminance);
	}
	
	if (map.contains("alphaBlend"))
	{
		alphaBlend = map.value("alphaBlend").toBool();
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
		if (skyConvexPolygons.size()!=textureCoords.size())
			throw std::runtime_error("the number of convex polygons does not match the number of texture space polygon");
	}
	else
		noTexture = true;
	
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
void SkyImageTile::downloadFinished()
{
	//qDebug() << "Download finished for " << httpReply->request().url().path() << ((httpReply->error()!=QNetworkReply::NoError) ? httpReply->errorString() : QString(""));
	
	if (httpReply->error()!=QNetworkReply::NoError)
	{
		if (httpReply->error()!=QNetworkReply::OperationCanceledError)
			qWarning() << "WARNING : Problem while downloading JSON Image Tile description for " << httpReply->request().url().path() << ": "<< httpReply->errorString();
		errorOccured = true;
		httpReply->deleteLater();
		httpReply=NULL;
		downloading=false;
		return;
	}	
	
	QByteArray content = httpReply->readAll();
	if (content.isEmpty())
	{
		errorOccured = true;
		httpReply->deleteLater();
		httpReply=NULL;
		downloading=false;
		return;
	}
	
	const bool compressed = httpReply->request().url().path().endsWith(".qZ");
	httpReply->deleteLater();
	httpReply=NULL;
	
	assert(loadThread==NULL);
	loadThread = new JsonLoadThread(this, content, compressed);
	connect(loadThread, SIGNAL(finished()), this, SLOT(JsonLoadFinished()));
	loadThread->start(QThread::LowestPriority);
}

// Called when the tile is fully loaded from the JSON file
void SkyImageTile::JsonLoadFinished()
{
	loadThread->wait();
	delete loadThread;
	loadThread = NULL;
	downloading = false;
	if (errorOccured)
		return;
	loadFromQVariantMap(temporaryResultMap);
	lastTimeDraw = StelApp::getInstance().getTotalRunTime();
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
		if (now-tile->getLastTimeDraw()<lastDrawTrigger || tile->downloading) // || (tile->tex && tile->tex->isLoading())
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
				if (tile->texFader)
				{
					tile->texFader->deleteLater();
					tile->texFader = NULL;
				}
				tile->deleteUnusedTiles(lastDrawTrigger);
			}
		}
		else
		{
			// None of the subtiles are displayed: delete all
			foreach (SkyImageTile* tile, subTiles)
			{
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
