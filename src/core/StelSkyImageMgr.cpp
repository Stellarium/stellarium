/*
 * Stellarium
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

#include "StelSkyImageMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelProjector.hpp"
#include "StelSkyImageTile.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelPainter.hpp"

#include <QNetworkAccessManager>
#include <stdexcept>
#include <QDebug>
#include <QString>
#include <QProgressBar>
#include <QVariantMap>
#include <QVariantList>

StelSkyImageMgr::StelSkyImageMgr(void) : flagShow(true)
{
	setObjectName("StelSkyImageMgr");
}

StelSkyImageMgr::~StelSkyImageMgr()
{
	foreach (StelSkyImageMgrElem* s, allSkyImages)
		delete s;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double StelSkyImageMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return GETSTELMODULE("MilkyWay")->getCallOrder(actionName)+5;
	return 0;
}

// read from stream
void StelSkyImageMgr::init()
{
	try
	{
		insertSkyImage(StelApp::getInstance().getFileMgr().findFile("nebulae/default/textures.json"));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading nebula texture set " << "default" << ": " << e.what();
	}
	// loadSkyImage("dobbs", "./scripts/dobbs.png", 11.5, 41, 11.5, 41.17, 11.75, 41.17, 11.75, 41, 2.5, 14, true);
}

QString StelSkyImageMgr::insertSkyImage(StelSkyImageTile* tile, bool ashow, bool aexternallyOwned)
{
	StelSkyImageMgrElem* bEl = new StelSkyImageMgrElem(tile, ashow, aexternallyOwned);
	QString key = tile->getShortName();
	if (key.isEmpty())
		key = tile->getAbsoluteImageURI();
	if (allSkyImages.contains(key))
	{
		QString suffix = "_01";
		int i=1;
		while (allSkyImages.contains(key+suffix))
		{
			suffix=QString("_%1").arg(i);
			++i;
		}
		key+=suffix;
	}
	allSkyImages.insert(key,bEl);
	connect(bEl->tile, SIGNAL(loadingStateChanged(bool)), this, SLOT(loadingStateChanged(bool)));
	connect(bEl->tile, SIGNAL(percentLoadedChanged(int)), this, SLOT(percentLoadedChanged(int)));
	return key;
}

// Add a new image from its URI (URL or local file name)
QString StelSkyImageMgr::insertSkyImage(const QString& uri, bool ashow)
{
	return insertSkyImage(new StelSkyImageTile(uri), ashow, false);
}

// Remove a sky image tile from the list of background images
void StelSkyImageMgr::removeSkyImage(const QString& key)
{
	//qDebug() << "StelSkyImageMgr::removeSkyImage removing image:" << key;
	if (allSkyImages.contains(key))
	{
		StelSkyImageMgrElem* bEl = allSkyImages[key];
		disconnect(bEl->tile, SIGNAL(loadingStateChanged(bool)), this, SLOT(loadingStateChanged(bool)));
		disconnect(bEl->tile, SIGNAL(percentLoadedChanged(int)), this, SLOT(percentLoadedChanged(int)));
		delete bEl;
		allSkyImages.remove(key);
	}
	else
	{
		qDebug() << "StelSkyImageMgr::removeSkyImage there is no such key" << key << "nothing is removed";
	}
}

// Remove a sky image tile from the list of background images
void StelSkyImageMgr::removeSkyImage(StelSkyImageTile* img)
{
	const QString k = keyForTile(img);
	if (!k.isEmpty())
		removeSkyImage(k);
}

// Draw all the multi-res images collection
void StelSkyImageMgr::draw(StelCore* core)
{
	if (!flagShow)
		return;

	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	foreach (StelSkyImageMgrElem* s, allSkyImages)
	{
		if (s->show)
			s->tile->draw(core, sPainter);
	}
}

// Called when loading of data started or stopped for one collection
void StelSkyImageMgr::loadingStateChanged(bool b)
{
	StelSkyImageTile* tile = qobject_cast<StelSkyImageTile*>(QObject::sender());
	Q_ASSERT(tile!=0);
	StelSkyImageMgrElem* elem = skyBackgroundElemForTile(tile);
	Q_ASSERT(elem!=NULL);
	if (b)
	{
		Q_ASSERT(elem->progressBar==NULL);
		elem->progressBar = StelMainGraphicsView::getInstance().addProgressBar();
		QString serverStr = elem->tile->getServerCredits().shortCredits;
		if (!serverStr.isEmpty())
			serverStr = " from "+serverStr;
		elem->progressBar->setFormat("Loading "+elem->tile->getShortName()+serverStr);
		elem->progressBar->setRange(0,100);
	}
	else
	{
		Q_ASSERT(elem->progressBar!=NULL);
		elem->progressBar->deleteLater();
		elem->progressBar = NULL;
	}
}
	
// Called when the percentage of loading tiles/tiles to be displayed changed for one collection
void StelSkyImageMgr::percentLoadedChanged(int percentage)
{
	StelSkyImageTile* tile = qobject_cast<StelSkyImageTile*>(QObject::sender());
	Q_ASSERT(tile!=0);
	StelSkyImageMgrElem* elem = skyBackgroundElemForTile(tile);
	Q_ASSERT(elem!=NULL);
	Q_ASSERT(elem->progressBar!=NULL);
	elem->progressBar->setValue(percentage);
}
	
StelSkyImageMgr::StelSkyImageMgrElem* StelSkyImageMgr::skyBackgroundElemForTile(const StelSkyImageTile* t)
{
	foreach (StelSkyImageMgrElem* e, allSkyImages)
	{
		if (e->tile==t)
		{
			return e;
		}
	}
	return NULL;
}

QString StelSkyImageMgr::keyForTile(const StelSkyImageTile* t)
{
	return allSkyImages.key(skyBackgroundElemForTile(t));
}

StelSkyImageMgr::StelSkyImageMgrElem::StelSkyImageMgrElem(StelSkyImageTile* t, bool ashow, bool aexternallyOwned) : 
		tile(t), progressBar(NULL), show(ashow), externallyOwned(aexternallyOwned)
{;}
				 
StelSkyImageMgr::StelSkyImageMgrElem::~StelSkyImageMgrElem()
{
	if (progressBar)
		progressBar->deleteLater();
	progressBar = NULL;
	if (!externallyOwned)
		delete tile;
	tile = NULL;
}

bool StelSkyImageMgr::loadSkyImage(const QString& id, const QString& filename, 
                               double ra0, double dec0, 
                               double ra1, double dec1, 
                               double ra2, double dec2, 
                               double ra3, double dec3, 
                               double minRes, double maxBright, bool visible)
{
	if (allSkyImages.contains("id"))
	{
		qWarning() << "Image ID" << id << "already exists, removing old image before loading";
		removeSkyImage(id);
	}

	QString path;
	// Possible exception sources: 
	// - StelFileMgr file not found
	// - list index out of range in insertSkyImage
	try
	{
		path = StelApp::getInstance().getFileMgr().findFile(filename);

		QVariantMap vm;
		QVariantMap st;
		QVariantList l;
		vm["shortName"] = QVariant(id);
		vm["minResolution"] = QVariant(0.05);
		vm["alphaBlend"] = true;

		st["imageUrl"] = QVariant(path);
		st["minResolution"] = QVariant(minRes);
		st["maxBrightness"] = QVariant(maxBright);
		QVariantList cl; // coordinates list for adding worldCoords and textureCoords
		QVariantList c;  // a map for a pair of coordinates
		QVariantList ol; // outer list - we want a structure 3 levels deep...
		c.append(dec0); c.append(ra0); cl.append(c); c.clear();
		c.append(dec1); c.append(ra1); cl.append(c); c.clear();
		c.append(dec2); c.append(ra2); cl.append(c); c.clear();
		c.append(dec3); c.append(ra3); cl.append(c); c.clear();
		ol.append(cl);
		st["worldCoords"] = ol;

		// textureCoords (define the ordering of worldCoords)
		cl.clear();
		ol.clear();
		c.append(0); c.append(0); cl.append(c); c.clear();
		c.append(1); c.append(0); cl.append(c); c.clear();
		c.append(1); c.append(1); cl.append(c); c.clear();
		c.append(0); c.append(1); cl.append(c);
		ol.append(cl);
		st["textureCoords"] = ol;
		l.append(st);
		vm["subTiles"] = l;

		StelSkyImageTile* tile = new StelSkyImageTile(vm, 0);
		QString key = insertSkyImage(tile, visible, false);
		if (key == id)
			return true;
		else
			return false;
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Could not find image" << filename << ":" << e.what();
		return false;
	}
}

void StelSkyImageMgr::showImage(const QString& id, bool b)
{
	if (allSkyImages.contains(id))
		if (allSkyImages[id]!=NULL)
			allSkyImages[id]->show = b;
}

bool StelSkyImageMgr::getShowImage(const QString& id)
{
	if (allSkyImages.contains(id))
		if (allSkyImages[id]!=NULL)
			return allSkyImages[id]->show;
	return false;
}


