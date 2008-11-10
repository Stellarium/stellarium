/*
 * Stellarium
 * This file Copyright (C) 2008 Matthew Gates
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


#include "ScreenImageMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "Navigator.hpp"
#include "Projector.hpp"
#include "StelModule.hpp"
#include "StelUtils.hpp"
#include "vecmath.h"

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QString>
#include <QDebug>
#include <QMap>

///////////////////////
// ScreenImage class //
///////////////////////
ScreenImage::ScreenImage()
{
}

void ScreenImage::update(double deltaTime)
{
	imageFader.update((int)(deltaTime*1000));
	// TODO when we have QT 4.5 - use opacity of QGraphicsItem to set alpha
}

void ScreenImage::setFadeDuration(float duration)
{
	imageFader.setDuration(duration);
}

void ScreenImage::setFlagShow(bool b)
{
	// TODO don't use this value after QT 4.5 and opacity of QGraphicsItem
	imageFader = b;
}

bool ScreenImage::getFlagShow(void)
{
	return imageFader;
}

void ScreenImage::setAlpha(float a)
{
	imageFader.setMaxValue(a);
}

///////////////////////
// ScreenScreenImage //
///////////////////////
ScreenScreenImage::ScreenScreenImage(const QString& filename, float x, float y, bool show)
	: tex(NULL)
{
	try
	{
		QString path = StelApp::getInstance().getFileMgr().findFile("scripts/" + filename);
		tex = StelMainGraphicsView::getInstance().scene()->addPixmap(QPixmap(path));
		tex->setOffset(x, y);
		tex->setVisible(show);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Failed to create ScreenScreenImage: " << e.what();
	}
}

ScreenScreenImage::~ScreenScreenImage()
{
	if (tex!=NULL)
	{
		delete tex;
		tex = NULL;
	}
}

bool ScreenScreenImage::draw(const StelCore* core)
{
	tex->setVisible(imageFader);
	return true;
}

void ScreenScreenImage::setXY(float x, float y)
{
	tex->setOffset(x, y);
}

//////////////////////////
// ScreenImageMgr class //
//////////////////////////
ScreenImageMgr::ScreenImageMgr()
{
	setObjectName("ScreenImageMgr");
	connect(this, SIGNAL(requestCreateScreenImage(const QString&, const QString&, float, float, bool, float)),
	        this, SLOT(doCreateScreenImage(const QString&, const QString&, float, float, bool, float)));

	connect(this, SIGNAL(requestSetImageShow(const QString&, bool)),
	        this, SLOT(doSetImageShow(const QString&, bool)));

	connect(this, SIGNAL(requestSetImageXY(const QString&, float, float)),
	        this, SLOT(doSetImageXY(const QString&, float, float)));

	connect(this, SIGNAL(requestDeleteImage(const QString&)),
	        this, SLOT(doDeleteImage(const QString&)));

	connect(this, SIGNAL(requestDeleteAllImages()),
	        this, SLOT(doDeleteAllImages()));
}
 
ScreenImageMgr::~ScreenImageMgr()
{
}

void ScreenImageMgr::init()
{
}

void ScreenImageMgr::draw(StelCore* core)
{
	foreach(ScreenImage* m, allScreenImages)
		if (m!=NULL)
			m->draw(core);
}

void ScreenImageMgr::createScreenImage(const QString& id,
                                       const QString& filename,
	                               float x,
	                               float y,
	                               bool visible,
	                               float alpha)
{
	emit(requestCreateScreenImage(id, filename, x, y, visible, alpha));
}

void ScreenImageMgr::deleteImage(const QString& id)
{
	emit(requestDeleteImage(id));
}
	
void ScreenImageMgr::deleteAllImages()
{
	emit(requestDeleteAllImages());
}

QStringList ScreenImageMgr::getAllImageIDs(void)
{
	return allScreenImages.keys();
}

bool ScreenImageMgr::getImageShow(const QString& id)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			return allScreenImages[id]->getFlagShow();

	return false;
}
	
void ScreenImageMgr::setImageShow(const QString& id, bool show)
{
	emit(requestSetImageShow(id, show));
}

void ScreenImageMgr::setImageXY(const QString& id, float x, float y)
{
	emit(requestSetImageXY(id, x, y));
}


void ScreenImageMgr::update(double deltaTime)
{
	foreach(ScreenImage* m, allScreenImages)
		if (m!=NULL)
			m->update(deltaTime);
}
	
double ScreenImageMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+11;
        return 0;
}

void ScreenImageMgr::doCreateScreenImage(const QString& id,
                                         const QString& filename,
                                         float x,
                                         float y,
                                         bool visible,
                                         float alpha)
{
	// First check to see if there is already an image loaded with the
	// specified ID, and drop it if necessary
	if (allScreenImages.contains(id))
		doDeleteImage(id);

	ScreenImage* i = new ScreenScreenImage(filename, x, y, visible);
	if (i==NULL)
		return;

	if (visible)
		i->setFlagShow(true);

	allScreenImages[id] = i;
	return;
}

void ScreenImageMgr::doSetImageShow(const QString& id, bool show)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			allScreenImages[id]->setFlagShow(show);
}

void ScreenImageMgr::doSetImageXY(const QString& id, float x, float y)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			allScreenImages[id]->setXY(x,y);
}

void ScreenImageMgr::doDeleteImage(const QString& id)
{
	if (allScreenImages.contains(id))
	{
		if (allScreenImages[id]!=NULL)
		{
			delete allScreenImages[id];
			allScreenImages[id] = NULL;
		}

		allScreenImages.remove(id);
	}
}

void ScreenImageMgr::doDeleteAllImages(void)
{
	foreach(ScreenImage* m, allScreenImages)
	{
		if (m!=NULL)
		{
			delete m;
			m = NULL;
		}
	}
	allScreenImages.clear();
}

