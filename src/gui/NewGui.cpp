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
#include "StelAppGraphicsItem.hpp"
#include "StelMainWindow.hpp"

#include "ui_mainGui.h"

#include <QPaintDevice>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItemGroup>
#include <QTimeLine>

StelWinBarButton::StelWinBarButton(QGraphicsItem* parent, const QPixmap& apixOn, const QPixmap& apixOff, const QPixmap& apixHover) : QGraphicsPixmapItem(apixOff, parent), pixOn(apixOn), pixOff(apixOff), pixHover(apixHover), checked(false)
{
	assert(!pixOn.isNull());
	assert(!pixOff.isNull());
	setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	setAcceptsHoverEvents(true);
	timeLine = new QTimeLine(300, this);
	timeLine->setCurveShape(QTimeLine::EaseInOutCurve);
	connect(timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(animValueChanged(qreal)));
}

void StelWinBarButton::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	QGraphicsItem::mousePressEvent(event);
	event->accept();
	setChecked(!checked);
	emit(toggled(checked));
	emit(triggered());
}

void StelWinBarButton::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	timeLine->setDirection(QTimeLine::Forward);
	if (timeLine->state()!=QTimeLine::Running)
		timeLine->start();
}
		
void StelWinBarButton::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	timeLine->setDirection(QTimeLine::Backward);
	if (timeLine->state()!=QTimeLine::Running)
		timeLine->start();
}

void StelWinBarButton::animValueChanged(qreal value)
{
	QPixmap pix = checked ? pixOn : pixOff;
	if (value>0)
	{
		QPainter painter(&pix);
		painter.setOpacity(value);
		painter.drawPixmap(0,0, pixHover);
	}
	setPixmap(pix);
}

void StelWinBarButton::setChecked(bool b)
{
	checked=b;
	animValueChanged(timeLine->currentValue());
}

LeftStelBar::LeftStelBar(QGraphicsItem* parent) : QGraphicsPathItem(parent)
{
	roundSize = 5;
}

void LeftStelBar::addButton(StelWinBarButton* button)
{
	QRectF rectCh = childrenBoundingRect();
	button->setParentItem(this);
	button->setPos(roundSize, rectCh.bottom()+10);
	updatePath();
}

void LeftStelBar::updatePath()
{
	QPainterPath newPath;
	if (children().isEmpty())
	{
		setPath(newPath);
		return;
	}
	QRectF rectCh = childrenBoundingRect();
	newPath.moveTo(0, 0);
	newPath.lineTo(rectCh.width()+roundSize,0);
 	newPath.arcTo(rectCh.width(), 0, 2.*roundSize, 2.*roundSize, 90, -90);
	newPath.lineTo(rectCh.width()+2.*roundSize, rectCh.height());
	setPath(newPath);
}

BottomStelBar::BottomStelBar(QGraphicsItem* parent) : QGraphicsPathItem(parent)
{
	roundSize = 5;
}

void BottomStelBar::addButton(StelWinBarButton* button)
{
	QRectF rectCh = childrenBoundingRect();
	button->setParentItem(this);
	button->setPos(rectCh.right(), roundSize);
	updatePath();
}

void BottomStelBar::updatePath()
{
	QPainterPath newPath;
	if (children().isEmpty())
	{
		setPath(newPath);
		return;
	}
	QRectF rectCh = childrenBoundingRect();
	newPath.moveTo(0, 0);
	newPath.lineTo(rectCh.width()-roundSize,0);
	newPath.arcTo(rectCh.width()-2.*roundSize, 0, 2.*roundSize, 2.*roundSize, 90, -90);
	newPath.lineTo(rectCh.width(), rectCh.height()+roundSize);
	setPath(newPath);
}

NewGui::NewGui(Ui_Form* aui) : ui(aui)
{
	winBar = NULL;
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
	// Connect all the GUI actions signals with the Core of Stellarium
	QObject* module = GETSTELMODULE("ConstellationMgr");
	QObject::connect(ui->actionShow_Constellation_Lines, SIGNAL(toggled(bool)), module, SLOT(setFlagLines(bool)));
	QObject::connect(ui->actionShow_Constellation_Art, SIGNAL(toggled(bool)), module, SLOT(setFlagArt(bool)));
	QObject::connect(ui->actionShow_Constellation_Labels, SIGNAL(toggled(bool)), module, SLOT(setFlagNames(bool)));
	
	module = GETSTELMODULE("GridLinesMgr");
	QObject::connect(ui->actionShow_Equatorial_Grid, SIGNAL(toggled(bool)), module, SLOT(setFlagEquatorGrid(bool)));
	QObject::connect(ui->actionShow_Azimutal_Grid, SIGNAL(toggled(bool)), module, SLOT(setFlagAzimutalGrid(bool)));
	
	module = GETSTELMODULE("LandscapeMgr");
	QObject::connect(ui->actionShow_Ground, SIGNAL(toggled(bool)), module, SLOT(setFlagLandscape(bool)));
	QObject::connect(ui->actionShow_Cardinal_Points, SIGNAL(toggled(bool)), module, SLOT(setFlagCardinalsPoints(bool)));
	QObject::connect(ui->actionShow_Atmosphere, SIGNAL(toggled(bool)), module, SLOT(setFlagAtmosphere(bool)));
	
	module = GETSTELMODULE("NebulaMgr");
	QObject::connect(ui->actionShow_Nebulas, SIGNAL(toggled(bool)), module, SLOT(setFlagHints(bool)));
	
	module = (QObject*)StelApp::getInstance().getCore()->getNavigation();
	QObject::connect(ui->actionIncrease_Time_Speed, SIGNAL(triggered()), module, SLOT(increaseTimeSpeed()));
	QObject::connect(ui->actionDecrease_Time_Speed, SIGNAL(triggered()), module, SLOT(decreaseTimeSpeed()));
	QObject::connect(ui->actionSet_Real_Time_Speed, SIGNAL(triggered()), module, SLOT(setRealTimeSpeed()));
	QObject::connect(ui->actionReturn_To_Current_Time, SIGNAL(triggered()), module, SLOT(setTimeNow()));
	QObject::connect(ui->actionSwitch_Equatorial_Mount, SIGNAL(toggled(bool)), module, SLOT(setEquatorialMount(bool)));
			
	module = &StelApp::getInstance();
	QObject::connect(ui->actionShow_Night_Mode, SIGNAL(toggled(bool)), module, SLOT(setVisionModeNight(bool)));
	
	module = GETSTELMODULE("MovementMgr");
	QObject::connect(ui->actionGoto_Selected_Object, SIGNAL(triggered()), module, SLOT(setFlagTracking()));
	
	ui->actionSet_Full_Screen->setChecked(StelMainWindow::getInstance().isFullScreen());
	QObject::connect(ui->actionSet_Full_Screen, SIGNAL(toggled(bool)), &StelMainWindow::getInstance(), SLOT(setFullScreen(bool)));
	
	
	// Construct the Windows buttons bar
	winBar = new LeftStelBar(NULL);
	
	QPixmap pxmapGlow(":/graphicGui/gui/glow.png");
	QPixmap pxmapOn(":/graphicGui/gui/1-on-time.png");
	QPixmap pxmapOff(":/graphicGui/gui/1-off-time.png");
	StelWinBarButton* b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/2-on-location.png");
	pxmapOff = QPixmap(":/graphicGui/gui/2-off-location.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);	
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/3-on-sky.png");
	pxmapOff = QPixmap(":/graphicGui/gui/3-off-sky.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/4-on-skylore.png");
	pxmapOff = QPixmap(":/graphicGui/gui/4-off-skylore.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/gui/5-off-labels.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/gui/6-off-search.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);
	b->setChecked(ui->actionShow_Search_Window->isChecked());
	QObject::connect(ui->actionShow_Search_Window, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Search_Window, SLOT(setChecked(bool)));
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/7-on-plugins.png");
	pxmapOff = QPixmap(":/graphicGui/gui/7-off-plugins.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/gui/8-off-settings.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/gui/9-off-help.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow);
	b->setChecked(ui->actionShow_Help_Window->isChecked());
	QObject::connect(ui->actionShow_Help_Window, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Help_Window, SLOT(setChecked(bool)));
	winBar->addButton(b);
	
	
	// Construct the bottom buttons bar
	buttonBar = new BottomStelBar(NULL);
	
	QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
	QPixmap pxmapBlank(":/graphicGui/gui/btBlank.png");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLines-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLines-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Constellation_Lines->isChecked());
	QObject::connect(ui->actionShow_Constellation_Lines, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Constellation_Lines, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLabels-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLabels-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Constellation_Labels->isChecked());
	QObject::connect(ui->actionShow_Constellation_Labels, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Constellation_Labels, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationArt-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationArt-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Constellation_Art->isChecked());
	QObject::connect(ui->actionShow_Constellation_Art, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Constellation_Art, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialGrid-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Equatorial_Grid->isChecked());
	QObject::connect(ui->actionShow_Equatorial_Grid, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Equatorial_Grid, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btAzimutalGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAzimutalGrid-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Azimutal_Grid->isChecked());
	QObject::connect(ui->actionShow_Azimutal_Grid, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Azimutal_Grid, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btGround-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGround-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Ground->isChecked());
	QObject::connect(ui->actionShow_Ground, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Ground, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btCardinalPoints-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btCardinalPoints-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Cardinal_Points->isChecked());
	QObject::connect(ui->actionShow_Cardinal_Points, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Cardinal_Points, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btAtmosphere-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAtmosphere-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Atmosphere->isChecked());
	QObject::connect(ui->actionShow_Atmosphere, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Atmosphere, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btNebula-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNebula-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Nebulas->isChecked());
	QObject::connect(ui->actionShow_Nebulas, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Nebulas, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialMount-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialMount-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionSwitch_Equatorial_Mount->isChecked());
	QObject::connect(ui->actionSwitch_Equatorial_Mount, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionSwitch_Equatorial_Mount, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btGotoSelectedObject-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGotoSelectedObject-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionGoto_Selected_Object->isChecked());
	QObject::connect(ui->actionGoto_Selected_Object, SIGNAL(triggered(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(triggered()), ui->actionGoto_Selected_Object, SLOT(trigger()));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btNightView-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNightView-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionShow_Night_Mode->isChecked());
	QObject::connect(ui->actionShow_Night_Mode, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(toggled(bool)), ui->actionShow_Night_Mode, SLOT(setChecked(bool)));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btQuit.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOn, pxmapGlow32x32);
	b->setChecked(ui->actionQuit->isChecked());
	QObject::connect(ui->actionQuit, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(triggered()), ui->actionQuit, SLOT(trigger()));
	buttonBar->addButton(b);
	
	b = new StelWinBarButton(NULL, pxmapBlank, pxmapBlank, pxmapBlank);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRewind-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRewind-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionDecrease_Time_Speed->isChecked());
	QObject::connect(ui->actionDecrease_Time_Speed, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(triggered()), ui->actionDecrease_Time_Speed, SLOT(trigger()));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRealtime-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRealtime-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionSet_Real_Time_Speed->isChecked());
	QObject::connect(ui->actionSet_Real_Time_Speed, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(triggered()), ui->actionSet_Real_Time_Speed, SLOT(trigger()));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeNow-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeNow-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionReturn_To_Current_Time->isChecked());
	QObject::connect(ui->actionReturn_To_Current_Time, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(triggered()), ui->actionReturn_To_Current_Time, SLOT(trigger()));
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeForward-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeForward-off.png");
	b = new StelWinBarButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32);
	b->setChecked(ui->actionIncrease_Time_Speed->isChecked());
	QObject::connect(ui->actionIncrease_Time_Speed, SIGNAL(toggled(bool)), b, SLOT(setChecked(bool)));
	QObject::connect(b, SIGNAL(triggered()), ui->actionIncrease_Time_Speed, SLOT(trigger()));
	buttonBar->addButton(b);
	
	
	QGraphicsScene* scene = StelMainWindow::getInstance().getGraphicsView()->scene();
	scene->addItem(winBar);
	scene->addItem(buttonBar);
	
	QPen winBarPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	winBarPen.setWidthF(1.);
	winBar->setPen(winBarPen);
	buttonBar->setPen(winBarPen);
	
	// Readjust position
	glWindowHasBeenResized(scene->sceneRect().width(),scene->sceneRect().height());
}

double NewGui::draw(StelCore* core)
{
	return 0.;
}

void NewGui::glWindowHasBeenResized(int w, int h)
{
	if (!winBar || !buttonBar)
		return;
	winBar->setPos(0, h-winBar->boundingRect().height()-buttonBar->boundingRect().height());
	buttonBar->setPos(winBar->boundingRect().right(), h-buttonBar->boundingRect().height());
}

// Update state which is time dependent.
void NewGui::update(double deltaTime)
{
}

void NewGui::updateI18n()
{
}
