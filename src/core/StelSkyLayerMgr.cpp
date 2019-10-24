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
#include "StelPainter.hpp"
#include "MilkyWay.hpp"
#include "StelGuiBase.hpp"
#include "StelSkyDrawer.hpp"
#include "StelTranslator.hpp"
#include "StelProgressController.hpp"

#include <QNetworkAccessManager>
#include <stdexcept>
#include <QDebug>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QDir>
#include <QSettings>

StelSkyLayerMgr::StelSkyLayerMgr(void) : flagShow(true)
{
	setObjectName("StelSkyLayerMgr");
}

StelSkyLayerMgr::~StelSkyLayerMgr()
{
	for (auto* s : allSkyLayers)
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
	loadCollection();

	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("skylayers");
	for (const auto& key : conf->childKeys())
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

	setFlagShow(!conf->value("astro/flag_nebula_display_no_texture", false).toBool());
	addAction("actionShow_DSO_Textures", N_("Display Options"), N_("Deep-sky objects background images"), "flagShow", "I");
	addAction("actionShow_DSO_Textures_Reload", N_("Display Options"), N_("Reload the deep-sky objects background images"), "loadCollection()", "Ctrl+I");
}

void StelSkyLayerMgr::loadCollection()
{
	if (!allSkyLayers.isEmpty())
		allSkyLayers.clear();

	QString path = StelFileMgr::findFile("nebulae/default/textures.json");
	if (path.isEmpty())
		qWarning() << "ERROR while loading nebula texture set default";
	else
		insertSkyImage(path);
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
void StelSkyLayerMgr::draw(StelCore* core)
{
	if (!flagShow)
		return;

	StelPainter sPainter(core->getProjection(StelCore::FrameJ2000));
	sPainter.setBlending(true, GL_ONE, GL_ONE); //additive blending
	for (auto* s : allSkyLayers)
	{
		if (s->show) 
		{
			sPainter.setProjector(core->getProjection(s->layer->getFrameType()));
			s->layer->draw(core, sPainter, 1.);
		}
	}
}

void noDelete(StelSkyLayer*) {;}

// Called when loading of data started or stopped for one collection
void StelSkyLayerMgr::loadingStateChanged(bool b)
{
	StelSkyLayer* tile = qobject_cast<StelSkyLayer*>(QObject::sender());
	Q_ASSERT(tile!=Q_NULLPTR);
	SkyLayerElem* elem = skyLayerElemForLayer(tile);
	Q_ASSERT(elem!=Q_NULLPTR);
	if (b)
	{
		Q_ASSERT(elem->progressBar==Q_NULLPTR);
		elem->progressBar = StelApp::getInstance().addProgressBar();
		QString serverStr = elem->layer->getShortServerCredits();
		if (!serverStr.isEmpty())
			serverStr = " from "+serverStr;
		elem->progressBar->setFormat("Loading "+elem->layer->getShortName()+serverStr);
		elem->progressBar->setRange(0,100);
	}
	else
	{
		Q_ASSERT(elem->progressBar!=Q_NULLPTR);
		StelApp::getInstance().removeProgressBar(elem->progressBar);
		elem->progressBar = Q_NULLPTR;
	}
}

// Called when the percentage of loading tiles/tiles to be displayed changed for one collection
void StelSkyLayerMgr::percentLoadedChanged(int percentage)
{
	StelSkyLayer* tile = qobject_cast<StelSkyLayer*>(QObject::sender());
	Q_ASSERT(tile!=Q_NULLPTR);
	SkyLayerElem* elem = skyLayerElemForLayer(tile);
	Q_ASSERT(elem!=Q_NULLPTR);
	Q_ASSERT(elem->progressBar!=Q_NULLPTR);
	elem->progressBar->setValue(percentage);
}

StelSkyLayerMgr::SkyLayerElem* StelSkyLayerMgr::skyLayerElemForLayer(const StelSkyLayer* t)
{
	for (auto* e : allSkyLayers)
	{
		if (e->layer==t)
		{
			return e;
		}
	}
	return Q_NULLPTR;
}

QString StelSkyLayerMgr::keyForLayer(const StelSkyLayer* t)
{
	return allSkyLayers.key(skyLayerElemForLayer(t));
}

StelSkyLayerMgr::SkyLayerElem::SkyLayerElem(StelSkyLayerP t, bool ashow) : layer(t), progressBar(Q_NULLPTR), show(ashow)
{;}

StelSkyLayerMgr::SkyLayerElem::~SkyLayerElem()
{
	if (progressBar)
		StelApp::getInstance().removeProgressBar(progressBar);
	progressBar = Q_NULLPTR;
}

bool StelSkyLayerMgr::loadSkyImage(const QString& id, const QString& filename,
								   double long0, double lat0,
								   double long1, double lat1,
								   double long2, double lat2,
								   double long3, double lat3,
								   double minRes, double maxBright, bool visible, StelCore::FrameType frameType)
{
	if (allSkyLayers.contains(id))
	{
		qWarning() << "Image ID" << id << "already exists, removing old image before loading";
		removeSkyLayer(id);
	}

	QString path = StelFileMgr::findFile(filename);
	if (path.isEmpty())
	{
		qWarning() << "Could not find image" << QDir::toNativeSeparators(filename);
		return false;
	}
	QVariantMap vm;
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
	c.clear(); c.append(long0); c.append(lat0); cl.append(QVariant(c));
	c.clear(); c.append(long1); c.append(lat1); cl.append(QVariant(c));
	c.clear(); c.append(long2); c.append(lat2); cl.append(QVariant(c));
	c.clear(); c.append(long3); c.append(lat3); cl.append(QVariant(c));
	ol.append(QVariant(cl));
	vm["worldCoords"] = ol;

	vm["alphaBlend"] = true; // new 2017-3: Make black correctly see-through.

	StelSkyLayerP tile = StelSkyLayerP(new StelSkyImageTile(vm, Q_NULLPTR));
	tile->setFrameType(frameType);
	
	try
	{
		QString key = insertSkyLayer(tile, filename, visible);
		if (key == id)
			return true;
		else
			return false;
	}
	catch (std::runtime_error& e)
	{
		qWarning() << e.what();
		return false;
	}
}

void StelSkyLayerMgr::showLayer(const QString& id, bool b)
{
	if (allSkyLayers.contains(id))
	{
		if (allSkyLayers[id]!=Q_NULLPTR)
			allSkyLayers[id]->show = b;
	}
}

bool StelSkyLayerMgr::getShowLayer(const QString& id) const
{
	if (allSkyLayers.contains(id))
	{
		if (allSkyLayers[id]!=Q_NULLPTR)
			return allSkyLayers[id]->show;
	}
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
