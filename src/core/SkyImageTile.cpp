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
#include "kfilterdev.h"


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
		JsonLoadThread(SkyImageTile* atile, QByteArray content, bool aqZcompressed=false, bool agzCompressed=false) : QThread((QObject*)atile),
			tile(atile), data(content), qZcompressed(aqZcompressed), gzCompressed(agzCompressed){;}
		virtual void run();
	private:
		SkyImageTile* tile;
		QByteArray data;
		const bool qZcompressed;
		const bool gzCompressed;
};

void JsonLoadThread::run()
{
	try
	{
		QBuffer buf(&data);
		buf.open(QIODevice::ReadOnly);
		tile->temporaryResultMap = SkyImageTile::loadFromJSON(buf, qZcompressed, gzCompressed);
	}
	catch (std::runtime_error e)
	{
		qWarning() << "WARNING : Can't parse loaded JSON Image Tile description: " << e.what();
		tile->errorOccured = true;
	}
}

void SkyImageTile::initCtor()
{
	luminance = -1;
	alphaBlend = false;
	noTexture = false;
	errorOccured = false;
	httpReply = NULL;
	downloading = false;
	loadThread = NULL;
	texFader = NULL;
	loadingState = false;
	lastPercent = 0;
	// Avoid tiles to be deleted just after constructed
	timeWhenDeletionScheduled = -1.;
	deletionDelay = 2.;
}

// Constructor
SkyImageTile::SkyImageTile(const QString& url, SkyImageTile* parent) : QObject(parent)
{
	initCtor();
	if (parent!=NULL)
	{
		luminance = parent->luminance;
		alphaBlend = parent->alphaBlend;
		deletionDelay = parent->deletionDelay;
	}
	contructorUrl = url;
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
		const bool gzCompressed = fileName.endsWith(".gz");
		try
		{
			loadFromQVariantMap(loadFromJSON(f, compressed, gzCompressed));
		}
		catch (std::runtime_error e)
		{
			qWarning() << "WARNING : Can't parse JSON Image Tile description: " << fileName << ": " << e.what();
			errorOccured = true;
			f.close();
			return;
		}
		f.close();
	}
	else
	{
		// Use a very short deletion delay to ensure that tile which are outside screen are discared before they are even downloaded
		// This is useful to reduce bandwidth when the user moves rapidely
		deletionDelay = 0.001;
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
		httpReply = netMgr->get(QNetworkRequest(qurl));
		connect(httpReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
		downloading = true;
		QString turl = qurl.toString();
		baseUrl = turl.left(turl.lastIndexOf('/')+1);
	}
}

// Constructor from a map used for JSON files with more than 1 level
SkyImageTile::SkyImageTile(const QVariantMap& map, SkyImageTile* parent) : QObject(parent)
{
	initCtor();
	if (parent!=NULL)
	{
		baseUrl = parent->getBaseUrl();
		luminance = parent->luminance;
		alphaBlend = parent->alphaBlend;
		contructorUrl = parent->contructorUrl + "/?";
		deletionDelay = parent->deletionDelay;
	}
	loadFromQVariantMap(map);
	downloading = false;
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
	
	const float limitLuminance = core->getSkyDrawer()->getLimitLuminance();
	QMultiMap<double, SkyImageTile*> result;
	getTilesToDraw(result, core, prj->getViewportConvexPolygon(0, 0), limitLuminance, true);
	
	int numToBeLoaded=0;
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_ONE, GL_ONE);
	
	QMap<double, SkyImageTile*>::Iterator i = result.end();
	while (i!=result.begin())
	{
		--i;
		if (i.value()->drawTile(core)==false)
			++numToBeLoaded;
	}
	updatePercent(result.size(), numToBeLoaded);
	
	deleteUnusedSubTiles();
}

// Schedule a deletion. It will practically occur after the delay passed as argument to deleteUnusedTiles() has expired
void SkyImageTile::scheduleDeletion()
{
	if (timeWhenDeletionScheduled<0.)
		timeWhenDeletionScheduled = StelApp::getInstance().getTotalRunTime();
}
		
// Return the list of tiles which should be drawn.
void SkyImageTile::getTilesToDraw(QMultiMap<double, SkyImageTile*>& result, StelCore* core, const StelGeom::ConvexPolygon& viewPortPoly, float limitLuminance, bool recheckIntersect)
{
	// An error occured during loading
	if (errorOccured)
		return;
	
	// The JSON file is currently being downloaded
	if (downloading)
		return;
	
	if (luminance>0 && luminance<limitLuminance)
	{
		// Schedule a deletion
		scheduleDeletion();
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
	{
		// Schedule a deletion
		scheduleDeletion();
		return;
	}
	
	// The tile is in screen, make sure that it's not going to be deleted
	cancelDeletion();
	
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
			tex = texMgr.createTextureThread(absoluteImageURI);
			if (!tex)
			{
				qWarning() << "WARNING : Can't create tile: " << absoluteImageURI;
				errorOccured = true;
				return;
			}
		}
		
		// The tile is in screen and has a texture: every test passed :) The tile will be displayed
		result.insert(minResolution, this);
		
		// Check that the texture is now fully loaded before trying to load sub tiles
// 		if (tex->canBind()==false)
// 			return;
	}
	
	// Check if we reach the resolution limit
	const double degPerPixel = 1./core->getProjection()->getPixelPerRadAtCenter()*180./M_PI;
	if (degPerPixel < minResolution)
	{
		if (subTiles.isEmpty() && !subTilesUrls.isEmpty())
		{
			//qDebug() << "Load subtiles for " << contructorUrl;
			// Load the sub tiles because we reached the maximum resolution and they are not yet loaded
			foreach (QVariant s, subTilesUrls)
			{
				SkyImageTile* nt;
				if (s.type()==QVariant::Map)
					nt = new SkyImageTile(s.toMap(), this);
				else
				{
					Q_ASSERT(s.type()==QVariant::String);
					nt = new SkyImageTile(s.toString(), this);
				}
				subTiles.append(nt);
			}
		}
		// Try to add the subtiles
		foreach (SkyImageTile* tile, subTiles)
		{
			tile->getTilesToDraw(result, core, viewPortPoly, limitLuminance, !fullInScreen);
		}
	}
	else
	{
		foreach (SkyImageTile* tile, subTiles)
		{
			tile->scheduleDeletion();
		}
	}
}
	
// Draw the image on the screen.
// Assume GL_TEXTURE_2D is enabled
bool SkyImageTile::drawTile(StelCore* core)
{	
	if (!tex->bind())
	{
		return false;
	}
	
	if (!texFader)
	{
		texFader = new QTimeLine(1000, this);
		texFader->start();
	}
	
	const Projector* prj = core->getProjection();
	const float factorX = tex->getCoordinates()[2][0];
	const float factorY = tex->getCoordinates()[2][1];

	// Draw the real texture for this image
	const float ad_lum = (luminance>0) ? core->getToneReproducer()->adaptLuminanceScaled(luminance) : 1.f;
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
#ifdef DEBUG_SKYIMAGE_TILE
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
		prj->drawText(debugFont, win[0], win[1], getAbsoluteImageURI());
		
		glDisable(GL_TEXTURE_2D);
		prj->drawPolygon(poly);
		glEnable(GL_TEXTURE_2D);
	}
#endif
	if (!alphaBlend)
		glBlendFunc(GL_ONE, GL_ONE); // Revert
	
	return true;
}

// Return true if the tile is fully loaded and can be displayed
bool SkyImageTile::isReadyToDisplay() const
{
	return tex && tex->canBind();
}
	
// Load the tile information from a JSON file
QVariantMap SkyImageTile::loadFromJSON(QIODevice& input, bool qZcompressed, bool gzCompressed)
{
	QtJsonParser parser;
	QVariantMap map;
	if (qZcompressed && input.size()>0)
	{
		QByteArray ar = qUncompress(input.readAll());
		input.close();
		QBuffer buf(&ar);
		buf.open(QIODevice::ReadOnly);
		map = parser.parse(buf).toMap();
		buf.close();
	}
	else if (gzCompressed)
	{
		QIODevice* d = KFilterDev::device(&input, "application/x-gzip", false);
		d->open(QIODevice::ReadOnly);
		map = parser.parse(*d).toMap();
		d->close();
		delete d;
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
	
	if (map.contains("luminance"))
	{
		luminance = map.value("luminance").toDouble(&ok);
		if (!ok)
			throw std::runtime_error("luminance expect a float value");
		qWarning() << "luminance in preview JSON files is deprecated. Replace with maxBrightness.";
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
	if (polyList.empty())
		polyList = map.value("worldCoords").toList();
	else
		qWarning() << "skyConvexPolygons in preview JSON files is deprecated. Replace with worldCoords.";
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
		QString imageUrl = map.value("imageUrl").toString();
		if (baseUrl.startsWith("http://"))
		{
			absoluteImageURI = baseUrl+imageUrl;
		}
		else
		{
			try
			{
				absoluteImageURI = StelApp::getInstance().getFileMgr().findFile(baseUrl+imageUrl);
			}
			catch (std::runtime_error& er)
			{
				// Maybe the user meant a file in stellarium local files
				absoluteImageURI = imageUrl;
			}
		}
		if (skyConvexPolygons.size()!=textureCoords.size())
			throw std::runtime_error("the number of convex polygons does not match the number of texture space polygon");
	}
	else
		noTexture = true;
	
	// This is a list of URLs to the child tiles or a list of already loaded map containing child information
	// (in this later case, the SkyImageTile objects will be created later)
	subTilesUrls = map.value("subTiles").toList();
// 	if (subTilesUrls.size()>10)
// 	{
// 		qWarning() << "Large tiles number for " << shortName << ": " << subTilesUrls.size();
// 	}
}
	
// Called when the download for the JSON file terminated
void SkyImageTile::downloadFinished()
{
	//qDebug() << "Download finished for " << httpReply->request().url().path() << ((httpReply->error()!=QNetworkReply::NoError) ? httpReply->errorString() : QString(""));
	Q_ASSERT(downloading);
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
		qWarning() << "WARNING : empty JSON Image Tile description for " << httpReply->request().url().path();
		errorOccured = true;
		httpReply->deleteLater();
		httpReply=NULL;
		downloading=false;
		return;
	}
	
	const bool qZcompressed = httpReply->request().url().path().endsWith(".qZ");
	const bool gzCompressed = httpReply->request().url().path().endsWith(".gz");
	httpReply->deleteLater();
	httpReply=NULL;
	
	assert(loadThread==NULL);
	loadThread = new JsonLoadThread(this, content, qZcompressed, gzCompressed);
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
}



// Delete all the subtiles which were not displayed since more than lastDrawTrigger seconds
void SkyImageTile::deleteUnusedSubTiles()
{
	if (subTiles.isEmpty())
		return;
	
	const double now = StelApp::getInstance().getTotalRunTime();
	bool deleteAll = true;
	foreach (SkyImageTile* tile, subTiles)
	{
		if (tile->timeWhenDeletionScheduled<0 || now-tile->timeWhenDeletionScheduled<deletionDelay)
		{
			deleteAll = false;
			break;
		}
	}
	if (deleteAll==true)
	{
		//qDebug() << "Delete all tiles for " << contructorUrl;
		foreach (SkyImageTile* tile, subTiles)
			tile->deleteLater();
		subTiles.clear();
	}
	else
	{
		// Nothing to delete at this level, propagate
		foreach (SkyImageTile* tile, subTiles)
			tile->deleteUnusedSubTiles();
	}
}
	
void SkyImageTile::updatePercent(int tot, int toBeLoaded)
{
	if (tot+toBeLoaded==0)
	{
		if (loadingState==true)
		{
			loadingState=false;
			emit(loadingStateChanged(false));
		}
		return;
	}

	int p = (int)(100.f*tot/(tot+toBeLoaded));
	if (p>100)
		p=100;
	if (p<0)
		p=0;
	if (p==100 || p==0)
	{
		if (loadingState==true)
		{
			loadingState=false;
			emit(loadingStateChanged(false));
		}
		return;
	}
	else
	{
		if (loadingState==false)
		{
			loadingState=true;
			emit(loadingStateChanged(true));
		}
	}
	if (p==lastPercent)
		return;
	lastPercent=p;
	emit(percentLoadedChanged(p));
}
