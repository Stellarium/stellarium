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

#include "SkyBackground.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "Projector.hpp"
#include "SkyImageTile.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainGraphicsView.hpp"

#include <stdexcept>
#include <QDebug>
#include <QString>
#include <QProgressBar>

SkyBackground::SkyBackground(void) : flagShow(true)
{
	setObjectName("SkyBackground");
}

SkyBackground::~SkyBackground()
{
	foreach (SkyBackgroundElem* s, allSkyImages)
		delete s;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SkyBackground::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return GETSTELMODULE("MilkyWay")->getCallOrder(actionName)+5;
	return 0;
}

// read from stream
void SkyBackground::init()
{
	addElem("http://voint1.hq.eso.org/fabienDSS2/allDSS.json");
	//addElem("http://stellarium.free.fr/divers/30Dor/x01_00_00.json.qZ");
	try
	{
		addElem(StelApp::getInstance().getFileMgr().findFile("nebulae/default/textures.json"));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR while loading nebula texture set " << "default" << ": " << e.what();
	}
}

void SkyBackground::addElem(const QString& uri)
{
	SkyBackgroundElem* bEl = new SkyBackgroundElem(uri);
	allSkyImages.append(bEl);
	connect(bEl->tile, SIGNAL(loadingStateChanged(bool)), this, SLOT(loadingStateChanged(bool)));
	connect(bEl->tile, SIGNAL(percentLoadedChanged(int)), this, SLOT(percentLoadedChanged(int)));
}

void SkyBackground::insertSkyImage(SkyImageTile* tile, bool ashow)
{
	SkyBackgroundElem* bEl = new SkyBackgroundElem(tile);
	bEl->show = ashow;
	allSkyImages.append(bEl);
	connect(bEl->tile, SIGNAL(loadingStateChanged(bool)), this, SLOT(loadingStateChanged(bool)));
	connect(bEl->tile, SIGNAL(percentLoadedChanged(int)), this, SLOT(percentLoadedChanged(int)));
}

// Remove a sky image tile from the list of background images
void SkyBackground::removeSkyImage(SkyImageTile* img)
{
	SkyBackgroundElem* elem = skyBackgroundElemForTile(img);
	allSkyImages.removeAll(elem);
	delete elem;
}

// Draw all the multi-res images collection
void SkyBackground::draw(StelCore* core)
{
	if (!flagShow)
		return;
	
	Projector* prj = core->getProjection();
	
	prj->setCurrentFrame(Projector::FrameJ2000);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	foreach (SkyBackgroundElem* s, allSkyImages)
	{
		if (s->show)
			s->tile->draw(core);
	}
}

// Called when loading of data started or stopped for one collection
void SkyBackground::loadingStateChanged(bool b)
{
	SkyImageTile* tile = qobject_cast<SkyImageTile*>(QObject::sender());
	Q_ASSERT(tile!=0);
	SkyBackgroundElem* elem = skyBackgroundElemForTile(tile);
	Q_ASSERT(elem!=NULL);
	if (b)
	{
		Q_ASSERT(elem->progressBar==NULL);
		elem->progressBar = StelMainGraphicsView::getInstance().addProgessBar();
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
void SkyBackground::percentLoadedChanged(int percentage)
{
	SkyImageTile* tile = qobject_cast<SkyImageTile*>(QObject::sender());
	Q_ASSERT(tile!=0);
	SkyBackgroundElem* elem = skyBackgroundElemForTile(tile);
	Q_ASSERT(elem!=NULL);
	Q_ASSERT(elem->progressBar!=NULL);
	elem->progressBar->setValue(percentage);
}
	
SkyBackground::SkyBackgroundElem* SkyBackground::skyBackgroundElemForTile(const SkyImageTile* t)
{
	foreach (SkyBackgroundElem* e, allSkyImages)
	{
		if (e->tile==t)
		{
			return e;
		}
	}
	return NULL;
}

SkyBackground::SkyBackgroundElem::SkyBackgroundElem(const QString& uri) : progressBar(NULL)
{
	tile = new SkyImageTile(uri);
}
				 
SkyBackground::SkyBackgroundElem::~SkyBackgroundElem()
{
	if (progressBar)
		progressBar->deleteLater();
	progressBar = NULL;
}
