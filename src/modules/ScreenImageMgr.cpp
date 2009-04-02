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
#include "StelNavigator.hpp"
#include "StelProjector.hpp"
#include "StelModule.hpp"
#include "StelUtils.hpp"
#include "VecMath.hpp"

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QString>
#include <QDebug>
#include <QMap>
#include <QTimeLine>
#include <QGraphicsItemAnimation>

///////////////////////
// ScreenImage class //
///////////////////////
ScreenImage::ScreenImage(const QString& filename, float x, float y, bool show, float scale, float fadeDuration)
	: imageFader(1000*fadeDuration), tex(NULL)
{
	try
	{
		QString path = StelApp::getInstance().getFileMgr().findFile("scripts/" + filename);
		tex = StelMainGraphicsView::getInstance().scene()->addPixmap(QPixmap(path));
		if (scale != 1.)
			tex->scale(scale,scale);
		tex->setOffset(x, y);
		tex->setVisible(show);
		anim = new QGraphicsItemAnimation();
		moveTimer = new QTimeLine();
		moveTimer->setCurveShape(QTimeLine::LinearCurve);
		anim->setTimeLine(moveTimer);
		anim->setItem(tex);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "Failed to create ScreenImage: " << e.what();
	}
}

ScreenImage::~ScreenImage()
{
	if (tex!=NULL)
	{
		delete tex;
		tex = NULL;
	}
	moveTimer->stop();
	delete anim; anim = NULL;
	delete moveTimer; moveTimer = NULL;
}

bool ScreenImage::draw(const StelCore* core)
{
	tex->setVisible(imageFader.getInterstate() > 0. || imageFader);
	tex->setOpacity(imageFader.getInterstate());
	return true;
}

void ScreenImage::update(double deltaTime)
{
	imageFader.update((int)(deltaTime*1000));
}

void ScreenImage::setFadeDuration(float duration)
{
	imageFader.setDuration(duration);
}

void ScreenImage::setFlagShow(bool b)
{
	imageFader = b;
}

bool ScreenImage::getFlagShow(void)
{
	return imageFader;
}

void ScreenImage::setAlpha(float a)
{
	imageFader.setMaxValue(a);
	imageFader.setMaxValue(a);
}

void ScreenImage::setXY(float x, float y, float duration)
{
	if (duration<=0.)
	{
		moveTimer->stop();
		tex->setOffset(x, y);
	}
	else
	{
		moveTimer->stop();
		moveTimer->setDuration(duration*1000);
		QPointF p(tex->offset());
		float sX = p.x();
		float sY = p.y();
		float eX = x;
		float eY = y;
		float dX = (x-sX) / 200.;
		float dY = (y-sY) / 200.;
		for(int i=0; i<200; i++)
			anim->setPosAt(i/200., QPointF(sX+(dX*i), sY+(dY*i)));
		anim->setPosAt(200., QPointF(eX, eY));
		moveTimer->start();
	}
}

//////////////////////////
// ScreenImageMgr class //
//////////////////////////
ScreenImageMgr::ScreenImageMgr()
{
	setObjectName("ScreenImageMgr");
	connect(this, SIGNAL(requestCreateScreenImage(const QString&, const QString&, float, float, float, bool, float, float)),
	        this, SLOT(doCreateScreenImage(const QString&, const QString&, float, float, float, bool, float, float)));

	connect(this, SIGNAL(requestSetImageShow(const QString&, bool)),
	        this, SLOT(doSetImageShow(const QString&, bool)));

	connect(this, SIGNAL(requestSetImageXY(const QString&, float, float, float)),
	        this, SLOT(doSetImageXY(const QString&, float, float, float)));

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
                                       float scale,
	                               bool visible,
	                               float alpha,
                                       float fadeDuration)
{
	emit(requestCreateScreenImage(id, filename, x, y, scale, visible, alpha, fadeDuration));
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

bool ScreenImageMgr::getShowImage(const QString& id)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			return allScreenImages[id]->getFlagShow();

	return false;
}
	
void ScreenImageMgr::showImage(const QString& id, bool show)
{
	emit(requestSetImageShow(id, show));
}

void ScreenImageMgr::setImageXY(const QString& id, float x, float y, float duration)
{
	emit(requestSetImageXY(id, x, y, duration));
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
                                         float scale,
                                         bool visible,
                                         float alpha,
                                         float fadeDuration)
{
	// First check to see if there is already an image loaded with the
	// specified ID, and drop it if necessary
	if (allScreenImages.contains(id))
		doDeleteImage(id);

	ScreenImage* i = new ScreenImage(filename, x, y, visible, scale, fadeDuration);
	if (i==NULL)
		return;

	i->setAlpha(alpha);

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

void ScreenImageMgr::doSetImageXY(const QString& id, float x, float y, float duration)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			allScreenImages[id]->setXY(x,y, duration);
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

