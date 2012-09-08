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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */


#include "ScreenImageMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"

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
ScreenImage::ScreenImage(const QString& filename, float x, float y, bool show, float scale, float alpha, float fadeDuration)
	: tex(NULL), maxAlpha(alpha)
{
	try
	{
		QString path = StelFileMgr::findFile("scripts/" + filename);
		QPixmap pm(path);
		tex = StelMainGraphicsView::getInstance().scene()->addPixmap(pm.scaled(pm.size()*scale));
		tex->setPos(x, y);

		anim = new QGraphicsItemAnimation();
		moveTimer = new QTimeLine();
		moveTimer->setCurveShape(QTimeLine::LinearCurve);
		anim->setTimeLine(moveTimer);
		anim->setItem(tex);

		fadeTimer = new QTimeLine();
		fadeTimer->setCurveShape(QTimeLine::LinearCurve);
		setFadeDuration(fadeDuration);
		connect(fadeTimer, SIGNAL(valueChanged(qreal)), this, SLOT(setOpacity(qreal)));


		// set inital displayed state
		if (show)
			tex->setOpacity(maxAlpha);
		else
			tex->setOpacity(0.);
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

bool ScreenImage::draw(const StelCore*)
{
	return true;
}

void ScreenImage::update(double)
{
}

void ScreenImage::setFadeDuration(float duration)
{
	int fadeMs = duration * 1000;
	if (fadeMs<=0) fadeMs=1;
	fadeTimer->setDuration(fadeMs);
}

void ScreenImage::setFlagShow(bool b)
{
	if (b)
	{
		fadeTimer->setFrameRange(tex->opacity()*fadeTimer->duration(),fadeTimer->duration());
		fadeTimer->setDirection(QTimeLine::Forward);
	}
	else
	{
		fadeTimer->setFrameRange(tex->opacity()*fadeTimer->duration(),0);
		fadeTimer->setDirection(QTimeLine::Backward);
	}
	fadeTimer->start();
}

bool ScreenImage::getFlagShow(void)
{
	return (tex->opacity() > 0.);
}

void ScreenImage::setAlpha(float a)
{
	maxAlpha = a;
	if (getFlagShow())
	{
		fadeTimer->stop();
		tex->setOpacity(a);
	}
}

void ScreenImage::setXY(float x, float y, float duration)
{
	if (duration<=0.)
	{
		moveTimer->stop();
		tex->setPos(x, y);
	}
	else
	{
		moveTimer->stop();
		moveTimer->setDuration(duration*1000);
		moveTimer->setFrameRange(0,100);
		QPointF p(tex->pos());
		if (p == QPointF(x,y)) return;
		float sX = p.x();
		float sY = p.y();
		float dX = (x-sX) / 200.;
		float dY = (y-sY) / 200.;
		for(int i=0; i<200; i++)
			anim->setPosAt(i/200., QPointF(sX+(dX*i), (sY+(dY*i))));
		anim->setPosAt(1.0, QPointF(x, y));
		moveTimer->start();
	}
}

void ScreenImage::addXY(float x, float y, float duration)
{
	QPointF currentPos = tex->pos();
	setXY(currentPos.x() + x, currentPos.y() + y, duration);
}

int ScreenImage::imageWidth(void)
{
	return tex->pixmap().size().width();
}

int ScreenImage::imageHeight(void)
{
	return tex->pixmap().size().height();
}

void ScreenImage::setOpacity(qreal alpha)
{
	tex->setOpacity(alpha*maxAlpha);
}

//////////////////////////
// ScreenImageMgr class //
//////////////////////////
ScreenImageMgr::ScreenImageMgr()
{
	setObjectName("ScreenImageMgr");
}
 
ScreenImageMgr::~ScreenImageMgr()
{
}

void ScreenImageMgr::init()
{
}

void ScreenImageMgr::draw(StelCore* core, class StelRenderer* renderer)
{
	Q_UNUSED(renderer);
	foreach(ScreenImage* m, allScreenImages)
		if (m!=NULL)
			m->draw(core);
}

void ScreenImageMgr::createScreenImage(const QString& id, const QString& filename, float x, float y,
	float scale, bool visible, float alpha, float fadeDuration)
{
	// First check to see if there is already an image loaded with the
	// specified ID, and drop it if necessary
	if (allScreenImages.contains(id))
		deleteImage(id);

	ScreenImage* i = new ScreenImage(filename, x, y, visible, scale, alpha, fadeDuration);
	if (i==NULL)
		return;

	allScreenImages[id] = i;
}

void ScreenImageMgr::deleteImage(const QString& id)
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
	
void ScreenImageMgr::deleteAllImages()
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

int ScreenImageMgr::getImageWidth(const QString& id)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			return allScreenImages[id]->imageWidth();
	return 0;
}
	
int ScreenImageMgr::getImageHeight(const QString& id)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			return allScreenImages[id]->imageHeight();
	return 0;
}
	
void ScreenImageMgr::showImage(const QString& id, bool show)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			allScreenImages[id]->setFlagShow(show);
}

void ScreenImageMgr::setImageAlpha(const QString& id, float alpha)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			allScreenImages[id]->setAlpha(alpha);
}

void ScreenImageMgr::setImageXY(const QString& id, float x, float y, float duration)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=NULL)
			allScreenImages[id]->setXY(x,y, duration);
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
