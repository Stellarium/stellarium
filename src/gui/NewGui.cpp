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

#include "NewGui.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "Projector.hpp"
#include "StelModuleMgr.hpp"
#include "StelGLWidget.hpp"

#include "ui_mainGui.h"

#include <QPaintDevice>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItemGroup>

StelWinBarButton::StelWinBarButton(QGraphicsItem* parent, const QPixmap& apixOn, const QPixmap& apixOff) : QGraphicsPixmapItem(apixOff, parent), pixOn(apixOn), pixOff(apixOff), checked(false)
{
	assert(!pixOn.isNull());
	assert(!pixOff.isNull());
	setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
}

void StelWinBarButton::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();
	setChecked(!checked);
	emit(toggled(checked));
}

void StelWinBarButton::setChecked(bool b)
{
	checked=b;
	setPixmap(checked ? pixOn : pixOff);
}

StelBar::StelBar(QGraphicsItem* parent) : QGraphicsPathItem(parent)
{
}

void StelBar::addButton(StelWinBarButton* button)
{
	QRectF rectCh = childrenBoundingRect();
	button->setPos(0, rectCh.height());
	button->setParentItem(this);
	updatePath();
}

void StelBar::updatePath()
{
	QPainterPath newPath;
	if (children().isEmpty())
	{
		setPath(newPath);
		return;
	}
	QRectF rectCh = childrenBoundingRect();
	double roundSize = 5.;
	newPath.moveTo(0, -roundSize);
	newPath.lineTo(rectCh.width()-roundSize,-roundSize);
 	newPath.arcTo(rectCh.width()-2*roundSize, -roundSize, 2*roundSize, 2*roundSize, 90, -90);
	newPath.lineTo(rectCh.width(), rectCh.height());
	setPath(newPath);
}

NewGui::NewGui(Ui_Form* aui) : ui(aui)
{
}

NewGui::~NewGui()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double NewGui::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
		return 100000;
	return 0;
}


void NewGui::init()
{
	QGraphicsScene* scene = StelGLWidget::getInstance().getScene();
	
	winBar = new StelBar(NULL);
	
	QPixmap pxmapOn(":/graphicGui/gui/1-on-time.png");
	QPixmap pxmapOff(":/graphicGui/gui/1-off-time.png");
	StelWinBarButton* b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/2-on-location.png");
	pxmapOff = QPixmap(":/graphicGui/gui/2-off-location.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);	
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/3-on-sky.png");
	pxmapOff = QPixmap(":/graphicGui/gui/3-off-sky.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/4-on-skylore.png");
	pxmapOff = QPixmap(":/graphicGui/gui/4-off-skylore.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/gui/5-off-labels.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/gui/6-off-search.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);
	b->setChecked(ui->actionShow_Search_Window->isChecked());
	QObject::connect(ui->actionShow_Search_Window, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Search_Window, SLOT(setChecked(bool)));
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/7-on-plugins.png");
	pxmapOff = QPixmap(":/graphicGui/gui/7-off-plugins.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/gui/8-off-settings.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/gui/9-off-help.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff);
	b->setChecked(ui->actionShow_Help_Window->isChecked());
	QObject::connect(ui->actionShow_Help_Window, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Help_Window, SLOT(setChecked(bool)));
	winBar->addButton(b);
	
	scene->addItem(winBar);
	winBar->setPos(0, scene->sceneRect().height()-winBar->boundingRect().height()-42);
	
	QPen winBarPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	winBarPen.setWidthF(1.);
	winBar->setPen(winBarPen);
}

double NewGui::draw(StelCore* core)
{
	return 0.;
}

void NewGui::glWindowHasBeenResized(int w, int h)
{
	QGraphicsScene* scene = StelGLWidget::getInstance().getScene();
	winBar->setPos(0, scene->sceneRect().height()-winBar->boundingRect().height()-42);
}

// Update state which is time dependent.
void NewGui::update(double deltaTime)
{
}

void NewGui::updateI18n()
{
}
