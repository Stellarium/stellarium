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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelSkyLayerMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelProjector.hpp"
#include "StelSkyImageTile.hpp"
#include "StelModuleMgr.hpp"
#include "MilkyWay.hpp"
#include "StelGuiBase.hpp"
#include "StelSkyDrawer.hpp"
#include "renderer/StelRenderer.hpp"

#include <QNetworkAccessManager>
#include <stdexcept>
#include <QDebug>
#include <QString>
#include <QProgressBar>
#include <QVariantMap>
#include <QVariantList>
#include <QSettings>

StelSkyLayerMgr::StelSkyLayerMgr() 
	: flagShow(true)
{
	setObjectName("StelSkyLayerMgr");
}

StelSkyLayerMgr::~StelSkyLayerMgr()
{
	foreach (SkyLayerElem* s, allSkyLayers)
		delete s;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double StelSkyLayerMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return GETSTELMODULE(MilkyWay)->getCallOrder(actionName)+5;
	return 0;
}

// read from stream
void StelSkyLayerMgr::init()
{
	try
	{
		insertSkyImage(StelFileMgr::findFile("nebulae/default/textures.json"));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading nebula texture set " << "default" << ": " << e.what();
	}
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("skylayers");
	foreach (const QString& key, conf->childKeys())
	{
		QString uri = conf->value(key, "").toString();
		if (!uri.isEmpty())
		{
			if (key=="clilayer")
				insertSkyImage(uri, "Command-line layer");
			else
				insertSkyImage(uri);
		}
	}
	conf->endGroup();
}

QString StelSkyLayerMgr::insertSkyLayer(StelSkyLayerP tile, const QString& keyHint, bool ashow)
{
	SkyLayerElem* bEl = new SkyLayerElem(tile, ashow);
	QString key = tile->getKeyHint();
	if (key.isEmpty() || key=="no name")
	{
		if (!keyHint.isEmpty())
		{
			key = keyHint;
		}
		else
		{
			key = "no name";
		}
	}
	if (allSkyLayers.contains(key))
	{
		QString suffix = "_01";
		int i=1;
		while (allSkyLayers.contains(key+suffix))
		{
			suffix=QString("_%1").arg(i);
			++i;
		}
		key+=suffix;
	}
	allSkyLayers.insert(key,bEl);
	connect(bEl->layer.data(), SIGNAL(loadingStateChanged(bool)), this, SLOT(loadingStateChanged(bool)));
	connect(bEl->layer.data(), SIGNAL(percentLoadedChanged(int)), this, SLOT(percentLoadedChanged(int)));
	return key;
}

// Add a new image from its URI (URL or local file name)
QString StelSkyLayerMgr::insertSkyImage(const QString& uri, const QString& keyHint, bool ashow)
{
	return insertSkyLayer(StelSkyLayerP(new StelSkyImageTile(uri)), keyHint, ashow);
}

// Remove a sky image tile from the list of background images
void StelSkyLayerMgr::removeSkyLayer(const QString& key)
{
	//qDebug() << "StelSkyLayerMgr::removeSkyImage removing image:" << key;
	if (allSkyLayers.contains(key))
	{
		SkyLayerElem* bEl = allSkyLayers[key];
		disconnect(bEl->layer.data(), SIGNAL(loadingStateChanged(bool)), this, SLOT(loadingStateChanged(bool)));
		disconnect(bEl->layer.data(), SIGNAL(percentLoadedChanged(int)), this, SLOT(percentLoadedChanged(int)));
		delete bEl;
		allSkyLayers.remove(key);
	}
	else
	{
		qDebug() << "StelSkyLayerMgr::removeSkyLayer there is no such key" << key << "nothing is removed";
	}
}

// Remove a sky image tile from the list of background images
void StelSkyLayerMgr::removeSkyLayer(StelSkyLayerP l)
{
	const QString k = keyForLayer(l.data());
	if (!k.isEmpty())
		removeSkyLayer(k);
}

// Draw all the multi-res images collection
void StelSkyLayerMgr::draw(StelCore* core, StelRenderer* renderer)
{
	if (!flagShow)
		return;

	renderer->setBlendMode(BlendMode_Add);
	foreach (SkyLayerElem* s, allSkyLayers)
	{
		if (s->show) 
		{
			if (s->layer->getFrameType() == StelCore::FrameAltAz) 
			{
				s->layer->draw(core, renderer, core->getProjection(StelCore::FrameAltAz), 1.);
			} 
			else
			{
				// TODO : Use the respective reference frames, once every SkyLayer
				// object sets their frame type. Defaulting to Equatorial frame now.
				s->layer->draw(core, renderer, core->getProjection(StelCore::FrameJ2000), 1.);
			}
		}
	}
}

void noDelete(StelSkyLayer*) {;}

// Called when loading of data started or stopped for one collection
void StelSkyLayerMgr::loadingStateChanged(bool b)
{
	StelSkyLayer* tile = qobject_cast<StelSkyLayer*>(QObject::sender());
	Q_ASSERT(tile!=0);
	SkyLayerElem* elem = skyLayerElemForLayer(tile);
	Q_ASSERT(elem!=NULL);
	if (b)
	{
		Q_ASSERT(elem->progressBar==NULL);
		elem->progressBar = StelApp::getInstance().getGui()->addProgressBar();
		QString serverStr = elem->layer->getShortServerCredits();
		if (!serverStr.isEmpty())
			serverStr = " from "+serverStr;
		elem->progressBar->setFormat("Loading "+elem->layer->getShortName()+serverStr);
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
void StelSkyLayerMgr::percentLoadedChanged(int percentage)
{
	StelSkyLayer* tile = qobject_cast<StelSkyLayer*>(QObject::sender());
	Q_ASSERT(tile!=0);
	SkyLayerElem* elem = skyLayerElemForLayer(tile);
	Q_ASSERT(elem!=NULL);
	Q_ASSERT(elem->progressBar!=NULL);
	elem->progressBar->setValue(percentage);
}

StelSkyLayerMgr::SkyLayerElem* StelSkyLayerMgr::skyLayerElemForLayer(const StelSkyLayer* t)
{
	foreach (SkyLayerElem* e, allSkyLayers)
	{
		if (e->layer==t)
		{
			return e;
		}
	}
	return NULL;
}

QString StelSkyLayerMgr::keyForLayer(const StelSkyLayer* t)
{
	return allSkyLayers.key(skyLayerElemForLayer(t));
}

StelSkyLayerMgr::SkyLayerElem::SkyLayerElem(StelSkyLayerP t, bool ashow) : layer(t), progressBar(NULL), show(ashow)
{;}

StelSkyLayerMgr::SkyLayerElem::~SkyLayerElem()
{
	if (progressBar)
		progressBar->deleteLater();
	progressBar = NULL;
}

bool StelSkyLayerMgr::loadSkyImage(const QString& id, const QString& filename,
								   double ra0, double dec0,
								   double ra1, double dec1,
								   double ra2, double dec2,
								   double ra3, double dec3,
								   double minRes, double maxBright, bool visible)
{
	if (allSkyLayers.contains(id))
	{
		qWarning() << "Image ID" << id << "already exists, removing old image before loading";
		removeSkyLayer(id);
	}

	QString path;
	// Possible exception sources:
	// - StelFileMgr file not found
	// - list index out of range in insertSkyImage
	try
	{
		path = StelFileMgr::findFile(filename);

		QVariantMap vm;
		QVariantList l;
		QVariantList cl; // coordinates list for adding worldCoords and textureCoords
		QVariantList c;  // a list for a pair of coordinates
		QVariantList ol; // outer list - we want a structure 3 levels deep...
		vm["imageUrl"] = QVariant(path);
		vm["maxBrightness"] = QVariant(maxBright);
		vm["minResolution"] = QVariant(minRes);
		vm["shortName"] = QVariant(id);

		// textureCoords (define the ordering of worldCoords)
		cl.clear();
		ol.clear();
		c.clear(); c.append(0); c.append(0); cl.append(QVariant(c));
		c.clear(); c.append(1); c.append(0); cl.append(QVariant(c));
		c.clear(); c.append(1); c.append(1); cl.append(QVariant(c));
		c.clear(); c.append(0); c.append(1); cl.append(QVariant(c));
		ol.append(QVariant(cl));
		vm["textureCoords"] = ol;

		// world coordinates
		cl.clear();
		ol.clear();
		c.clear(); c.append(ra0); c.append(dec0); cl.append(QVariant(c));
		c.clear(); c.append(ra1); c.append(dec1); cl.append(QVariant(c));
		c.clear(); c.append(ra2); c.append(dec2); cl.append(QVariant(c));
		c.clear(); c.append(ra3); c.append(dec3); cl.append(QVariant(c)); 
		ol.append(QVariant(cl));
		vm["worldCoords"] = ol;

		StelSkyLayerP tile = StelSkyLayerP(new StelSkyImageTile(vm, 0));
		tile->setFrameType(StelCore::FrameJ2000);
		QString key = insertSkyLayer(tile, filename, visible);
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

bool StelSkyLayerMgr::loadSkyImageAltAz(const QString& id, const QString& filename,
								   double alt0, double azi0,
								   double alt1, double azi1,
								   double alt2, double azi2,
								   double alt3, double azi3,
								   double minRes, double maxBright, bool visible)
{
	if (allSkyLayers.contains(id))
	{
		qWarning() << "Image ID" << id << "already exists, removing old image before loading";
		removeSkyLayer(id);
	}

	QString path;
	// Possible exception sources:
	// - StelFileMgr file not found
	// - list index out of range in insertSkyImage
	try
	{
		path = StelFileMgr::findFile(filename);

		QVariantMap vm;
		QVariantList l;
		QVariantList cl; // coordinates list for adding worldCoords and textureCoords
		QVariantList c;  // a list for a pair of coordinates
		QVariantList ol; // outer list - we want a structure 3 levels deep...
		vm["imageUrl"] = QVariant(path);
		vm["maxBrightness"] = QVariant(maxBright);
		vm["minResolution"] = QVariant(minRes);
		vm["shortName"] = QVariant(id);

		// textureCoords (define the ordering of worldCoords)
		cl.clear();
		ol.clear();
		c.clear(); c.append(0); c.append(0); cl.append(QVariant(c));
		c.clear(); c.append(1); c.append(0); cl.append(QVariant(c));
		c.clear(); c.append(1); c.append(1); cl.append(QVariant(c));
		c.clear(); c.append(0); c.append(1); cl.append(QVariant(c));
		ol.append(QVariant(cl));
		vm["textureCoords"] = ol;

		// world coordinates
		cl.clear();
		ol.clear();
		c.clear(); c.append(alt0); c.append(azi0); cl.append(QVariant(c));
		c.clear(); c.append(alt1); c.append(azi1); cl.append(QVariant(c));
		c.clear(); c.append(alt2); c.append(azi2); cl.append(QVariant(c));
		c.clear(); c.append(alt3); c.append(azi3); cl.append(QVariant(c)); 
		ol.append(QVariant(cl));
		vm["worldCoords"] = ol;

		StelSkyLayerP tile = StelSkyLayerP(new StelSkyImageTile(vm, 0));
		tile->setFrameType(StelCore::FrameAltAz);
		QString key = insertSkyLayer(tile, filename, visible);
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

void StelSkyLayerMgr::showLayer(const QString& id, bool b)
{
	if (allSkyLayers.contains(id))
		if (allSkyLayers.value(id)!=NULL)
			allSkyLayers[id]->show = b;
}

bool StelSkyLayerMgr::getShowLayer(const QString& id) const
{
	if (allSkyLayers.contains(id))
		if (allSkyLayers.value(id)!=NULL)
			return allSkyLayers.value(id)->show;
	return false;
}


//! Get the list of all the currently loaded layers.
QMap<QString, StelSkyLayerP> StelSkyLayerMgr::getAllSkyLayers() const
{
	QMap<QString, StelSkyLayerP> res;
	for (QMap<QString, StelSkyLayerMgr::SkyLayerElem*>::ConstIterator iter=allSkyLayers.constBegin();iter!=allSkyLayers.constEnd();++iter)
	{
		//qDebug() << iter.key() << iter.value()->layer->getShortName();
		res.insert(iter.key(), iter.value()->layer);
	}
	return res;
}

StelSkyLayerP StelSkyLayerMgr::getSkyLayer(const QString& key) const
{
	if (allSkyLayers.contains(key))
		return allSkyLayers[key]->layer;
	return StelSkyLayerP();
}
