/*
 * Stellarium
 * This file Copyright (C) 2008 Matthew Gates
 * Parts copyright (C) 2016 Georg Zotti (added size transitions)
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
#include "StelMainView.hpp"
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
ScreenImage::ScreenImage(const QString& filename, qreal x, qreal y, bool show, qreal scale, qreal alpha, float fadeDuration)
	: tex(Q_NULLPTR), maxAlpha(alpha)
{
	QPixmap pixmap(filename);
	tex = StelMainView::getInstance().scene()->addPixmap(pixmap.scaled(pixmap.size()*scale));
	tex->setPos(x, y);

	anim = new QGraphicsItemAnimation();
	moveTimer = new QTimeLine();
	moveTimer->setCurveShape(QTimeLine::LinearCurve);
	anim->setTimeLine(moveTimer);
	anim->setItem(tex);

	scaleAnim = new QGraphicsItemAnimation();
	scaleTimer = new QTimeLine();
	scaleTimer->setCurveShape(QTimeLine::LinearCurve);
	scaleAnim->setTimeLine(scaleTimer);
	scaleAnim->setItem(tex);
	// we must configure the end for proper rescaling
	scaleAnim->setScaleAt(0., 1., 1.);
	scaleAnim->setScaleAt(1., 1., 1.);

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

ScreenImage::~ScreenImage()
{
	moveTimer->stop();
	scaleTimer->stop();
	fadeTimer->stop();
	delete anim; anim = Q_NULLPTR;
	delete scaleAnim; scaleAnim = Q_NULLPTR;
	delete moveTimer; moveTimer = Q_NULLPTR;
	delete scaleTimer; scaleTimer = Q_NULLPTR;
	delete fadeTimer; fadeTimer = Q_NULLPTR;
	if (tex!=Q_NULLPTR)
	{
		delete tex;
		tex = Q_NULLPTR;
	}
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
	fadeTimer->setDuration(qMax(1, static_cast<int>(duration*1000.f)));
}

void ScreenImage::setFlagShow(bool b)
{
	if (b)
	{
		fadeTimer->setFrameRange(static_cast<int>(tex->opacity()*fadeTimer->duration()),fadeTimer->duration());
		fadeTimer->setDirection(QTimeLine::Forward);
	}
	else
	{
		fadeTimer->setFrameRange(static_cast<int>(tex->opacity()*fadeTimer->duration()),0);
		fadeTimer->setDirection(QTimeLine::Backward);
	}
	fadeTimer->start();
}

bool ScreenImage::getFlagShow(void) const
{
	return (tex->opacity() > 0.);
}

void ScreenImage::setAlpha(qreal a)
{
	maxAlpha = a;
	if (getFlagShow())
	{
		fadeTimer->stop();
		tex->setOpacity(a);
	}
}

void ScreenImage::setXY(qreal x, qreal y, float duration)
{
	if (duration<=0.f)
	{
		moveTimer->stop();
		tex->setPos(x, y);
	}
	else
	{
		moveTimer->stop();
		moveTimer->setDuration(static_cast<int>(duration*1000.f));
		moveTimer->setFrameRange(0,100);
		QPointF p(tex->pos());
		if (p == QPointF(x,y)) return;
		qreal sX = p.x();
		qreal sY = p.y();
		qreal dX = (x-sX) / 200.;
		qreal dY = (y-sY) / 200.;
		for(int i=0; i<200; i++)
			anim->setPosAt(static_cast<qreal>(i/200.f), QPointF(sX+(dX*i), (sY+(dY*i))));
		anim->setPosAt(1.0, QPointF(x, y));
		moveTimer->start();
	}
}

void ScreenImage::addXY(qreal x, qreal y, float duration)
{
	QPointF currentPos = tex->pos();
	setXY(currentPos.x() + x, currentPos.y() + y, duration);
}

int ScreenImage::imageWidth(void) const
{
	return tex->pixmap().size().width();
}

int ScreenImage::imageHeight(void) const
{
	return tex->pixmap().size().height();
}

void ScreenImage::setOpacity(qreal alpha)
{
	tex->setOpacity(alpha*maxAlpha);
}

void ScreenImage::setScale(qreal scale)
{
	tex->setScale(scale);
}

void ScreenImage::setScale(qreal scaleX, qreal scaleY, float duration)
{
	scaleTimer->stop();

	// Set a least a tiny duration to allow running the "animation"
	scaleTimer->setDuration(static_cast<int>(qMax(0.001f, duration)*1000));
	scaleTimer->setFrameRange(0,100);

	tex->setTransformationMode(Qt::SmoothTransformation);

	// reconfigure the animation which may have halted at 1 after a previous run, so the current scale is at position 1.
	scaleAnim->setScaleAt(0., scaleAnim->horizontalScaleAt(1.), scaleAnim->verticalScaleAt(1.));
	scaleAnim->setScaleAt(1., scaleX, scaleY);
	scaleTimer->start();
}

qreal ScreenImage::imageScaleX() const
{
	return scaleAnim->horizontalScaleAt(1.);
}

qreal ScreenImage::imageScaleY() const
{
	return scaleAnim->verticalScaleAt(1.);
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

void ScreenImageMgr::draw(StelCore* core)
{
	for (auto* m : allScreenImages)
		if (m!=Q_NULLPTR)
			m->draw(core);
}

void ScreenImageMgr::createScreenImage(const QString& id, const QString& filename, qreal x, qreal y,
    qreal scale, bool visible, qreal alpha, float fadeDuration)
{
	// First check to see if there is already an image loaded with the
	// specified ID, and drop it if necessary
	if (allScreenImages.contains(id))
		deleteImage(id);

	QString path = StelFileMgr::findFile("scripts/" + filename);
	if (!path.isEmpty())
	{
		ScreenImage* i = new ScreenImage(path, x, y, visible, scale, alpha, fadeDuration);
		if (i==Q_NULLPTR)
			return;

		allScreenImages[id] = i;
	}
	else
	{
		qWarning() << "Failed to create ScreenImage" << id << ": file not found: " << filename;
	}
}

void ScreenImageMgr::deleteImage(const QString& id)
{
	if (allScreenImages.contains(id))
	{
		if (allScreenImages[id]!=Q_NULLPTR)
		{
			delete allScreenImages[id];
			allScreenImages[id] = Q_NULLPTR;
		}
		allScreenImages.remove(id);
	}
}
	
void ScreenImageMgr::deleteAllImages()
{
	for (auto* m : allScreenImages)
	{
		if (m!=Q_NULLPTR)
		{
			delete m;
			m = Q_NULLPTR;
		}
	}
	allScreenImages.clear();
}

QStringList ScreenImageMgr::getAllImageIDs(void) const
{
	return allScreenImages.keys();
}

bool ScreenImageMgr::getShowImage(const QString& id) const
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			return allScreenImages[id]->getFlagShow();
	return false;
}

int ScreenImageMgr::getImageWidth(const QString& id) const
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			return allScreenImages[id]->imageWidth();
	return 0;
}
	
int ScreenImageMgr::getImageHeight(const QString& id) const
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			return allScreenImages[id]->imageHeight();
	return 0;
}

qreal ScreenImageMgr::getImageScaleX(const QString& id) const
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			return allScreenImages[id]->imageScaleX();
	return 0;
}

qreal ScreenImageMgr::getImageScaleY(const QString& id) const
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			return allScreenImages[id]->imageScaleY();
	return 0;
}

void ScreenImageMgr::showImage(const QString& id, bool show)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			allScreenImages[id]->setFlagShow(show);
}

void ScreenImageMgr::setImageAlpha(const QString& id, qreal alpha)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			allScreenImages[id]->setAlpha(alpha);
}

void ScreenImageMgr::setImageXY(const QString& id, qreal x, qreal y, float duration)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			allScreenImages[id]->setXY(x, y, duration);
}

void ScreenImageMgr::addImageXY(const QString& id, qreal x, qreal y, float duration)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			allScreenImages[id]->addXY(x, y, duration);
}

void ScreenImageMgr::setImageScale(const QString& id, qreal scaleX, qreal scaleY, float duration)
{
	if (allScreenImages.contains(id))
		if (allScreenImages[id]!=Q_NULLPTR)
			allScreenImages[id]->setScale(scaleX, scaleY, duration);
}


void ScreenImageMgr::update(double deltaTime)
{
	for (auto* m : allScreenImages)
		if (m!=Q_NULLPTR)
			m->update(deltaTime);
}
	
double ScreenImageMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+11;
        return 0;
}
