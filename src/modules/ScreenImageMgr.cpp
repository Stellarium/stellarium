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

#include <map>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QString>
#include <QDebug>

///////////////////////
// ScriptImage class //
///////////////////////
ScriptImage::ScriptImage()
{
}

void ScriptImage::update(double deltaTime)
{
	imageFader.update((int)(deltaTime*1000));
	// TODO when we have QT 4.5 - use opacity of QGraphicsItem to set alpha
}

void ScriptImage::setFadeDuration(float duration)
{
	imageFader.setDuration(duration);
}

void ScriptImage::setFlagShow(bool b)
{
	// TODO don't use this value after QT 4.5 and opacity of QGraphicsItem
	imageFader = b;
}

bool ScriptImage::getFlagShow(void)
{
	return imageFader;
}

void ScriptImage::setAlpha(float a)
{
	imageFader.setMaxValue(a);
}

///////////////////////
// ScreenScriptImage //
///////////////////////
ScreenScriptImage::ScreenScriptImage(const QString& filename, float x, float y, bool show)
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
		qWarning() << "Failed to create ScreenScriptImage: " << e.what();
	}
}

ScreenScriptImage::~ScreenScriptImage()
{
	if (tex!=NULL)
	{
		delete tex;
		tex = NULL;
	}
}

bool ScreenScriptImage::draw(const StelCore* core)
{
	tex->setVisible(imageFader);
	return true;
}

void ScreenScriptImage::setXY(float x, float y)
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
	for(std::map<QString, ScriptImage*>::iterator i=allImages.begin(); i!=allImages.end(); i++)
	{
		if ((*i).second != NULL)
			(*i).second->draw(core);
	}
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
	QStringList result;
	for(std::map<QString, ScriptImage*>::iterator i=allImages.begin(); i!=allImages.end(); i++)
	{
		result << (*i).first;
	}
	return result;
}

bool ScreenImageMgr::getImageShow(const QString& id)
{
	std::map<QString, ScriptImage*>::iterator it = allImages.find(id);
	if (it != allImages.end())
		if ((*it).second != NULL)
			return (*it).second->getFlagShow();

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
	for(std::map<QString, ScriptImage*>::iterator i=allImages.begin(); i!=allImages.end(); i++)
	{
		if ((*i).second != NULL)
			(*i).second->update(deltaTime);
	}
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
	std::map<QString, ScriptImage*>::iterator it = allImages.find(id);
	if (it != allImages.end())
		doDeleteImage(id);

	ScriptImage* i = new ScreenScriptImage(filename, x, y, visible);
	if (i==NULL)
		return;

	if (visible)
		i->setFlagShow(true);

	allImages[id] = i;
	return;
}

void ScreenImageMgr::doSetImageShow(const QString& id, bool show)
{
	std::map<QString, ScriptImage*>::iterator it = allImages.find(id);
	if (it != allImages.end())
		if ((*it).second != NULL)
			return (*it).second->setFlagShow(show);
}

void ScreenImageMgr::doSetImageXY(const QString& id, float x, float y)
{
	std::map<QString, ScriptImage*>::iterator it = allImages.find(id);
	if (it != allImages.end())
		if ((*it).second != NULL)
			(*it).second->setXY(x,y);
}

void ScreenImageMgr::doDeleteImage(const QString& id)
{
	std::map<QString, ScriptImage*>::iterator it = allImages.find(id);
	if (it != allImages.end())
	{
		delete (*it).second;
		(*it).second = NULL;
		allImages.erase(it);
	}
}

void ScreenImageMgr::doDeleteAllImages(void)
{
	for(std::map<QString, ScriptImage*>::iterator i=allImages.begin(); i!=allImages.end(); i++)
	{
		if ((*i).second != NULL)
			delete (*i).second;
	}
	allImages.clear();
}

