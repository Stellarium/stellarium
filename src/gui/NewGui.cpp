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
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelAppGraphicsItem.hpp"
#include "StelMainWindow.hpp"
#include "LandscapeMgr.hpp"
#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "NebulaMgr.hpp"
#include "Observer.hpp"
#include "StelLocaleMgr.hpp"
#include "Navigator.hpp"

#include "ui_mainGui.h"

#include <QPaintDevice>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QTimeLine>
#include <QFontDatabase>
#include <QMouseEvent>

StelButton::StelButton(QGraphicsItem* parent, const QPixmap& apixOn, const QPixmap& apixOff,
		const QPixmap& apixHover, QAction* aaction, QGraphicsSimpleTextItem* ahelpLabel) : 
		QGraphicsPixmapItem(apixOff, parent), pixOn(apixOn), pixOff(apixOff), pixHover(apixHover),
		checked(false), action(aaction), helpLabel(ahelpLabel)
{
	assert(!pixOn.isNull());
	assert(!pixOff.isNull());
	setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	setAcceptsHoverEvents(true);
	timeLine = new QTimeLine(250, this);
	timeLine->setCurveShape(QTimeLine::EaseOutCurve);
	connect(timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(animValueChanged(qreal)));
	
	if (action!=NULL)
	{
		QObject::connect(action, SIGNAL(toggled(bool)), this, SLOT(setChecked(bool)));
		if (action->isCheckable())
		{
			setChecked(action->isChecked());
			QObject::connect(this, SIGNAL(toggled(bool)), action, SLOT(setChecked(bool)));
		}
		else
		{
			QObject::connect(this, SIGNAL(triggered()), action, SLOT(trigger()));
		}
	}
}

void StelButton::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	QGraphicsItem::mousePressEvent(event);
	event->accept();
	setChecked(!checked);
	emit(toggled(checked));
	emit(triggered());
}

void StelButton::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	timeLine->setDirection(QTimeLine::Forward);
	if (timeLine->state()!=QTimeLine::Running)
		timeLine->start();
	if (helpLabel && action)
		helpLabel->setText(action->toolTip() + "  [" + action->shortcut().toString() + "]");
}
		
void StelButton::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	timeLine->setDirection(QTimeLine::Backward);
	if (timeLine->state()!=QTimeLine::Running)
		timeLine->start();
	if (helpLabel && action)
		helpLabel->setText("");
}

void StelButton::animValueChanged(qreal value)
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

void StelButton::setChecked(bool b)
{
	checked=b;
	animValueChanged(timeLine->currentValue());
}

LeftStelBar::LeftStelBar(QGraphicsItem* parent) : QGraphicsItem(parent)
{
}

LeftStelBar::~LeftStelBar()
{
}

void LeftStelBar::addButton(StelButton* button)
{
	QRectF rectCh = childrenBoundingRect();
	button->setParentItem(this);
	button->setPos(0, rectCh.bottom()+10);
}

void LeftStelBar::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
}

QRectF LeftStelBar::boundingRect() const
{
	return childrenBoundingRect();
}

BottomStelBar::BottomStelBar(QGraphicsItem* parent) : QGraphicsItem(parent)
{
	QFont font("DejaVuSans", 10);
	QColor color = QColor::fromRgbF(1,1,1,1);
	datetime = new QGraphicsSimpleTextItem("2008-02-06  17:33", this);
	location = new QGraphicsSimpleTextItem("Munich, Earth, 500m", this);
	fov = new QGraphicsSimpleTextItem("FOV 43.45", this);
	fps = new QGraphicsSimpleTextItem("43.2 FPS", this);
	
	datetime->setFont(font);
	QBrush brush(color);
	datetime->setBrush(brush);
	location->setFont(font);
	location->setBrush(brush);
	fov->setFont(font);
	fov->setBrush(brush);
	fps->setFont(font);
	fps->setBrush(brush);
}

void BottomStelBar::addButton(StelButton* button)
{
	double y = datetime->boundingRect().height()+3;
	QRectF rectCh = getButtonsBoundingRect();
	button->setParentItem(this);
	button->setPos(rectCh.right(), y);
	updateText();
}


QRectF BottomStelBar::getButtonsBoundingRect()
{
	location->setParentItem(NULL);
	datetime->setParentItem(NULL);
	fov->setParentItem(NULL);
	fps->setParentItem(NULL);
	
	QRectF rectCh = childrenBoundingRect();
	
	location->setParentItem(this);
	datetime->setParentItem(this);
	fov->setParentItem(this);
	fps->setParentItem(this);
	
	return rectCh;
}

void BottomStelBar::updateText()
{
	StelCore* core = StelApp::getInstance().getCore();
	double jd = core->getNavigation()->getJDay();
	
	datetime->setText(QString::fromStdWString(StelApp::getInstance().getLocaleMgr().get_printable_date_local(jd))+
			"   "+QString::fromStdWString(StelApp::getInstance().getLocaleMgr().get_printable_time_local(jd)));
	
	location->setText(core->getObservatory()->getHomePlanetNameI18n() +
			", " + core->getObservatory()->getLocationName() +
			QString(", %1m").arg(core->getObservatory()->getAltitude()));
	
	QString str;
	QTextStream wos(&str);
	wos << "FOV " << qSetRealNumberPrecision(3) << core->getProjection()->getFov() << QString::fromWCharArray(L"\u00B0");
	fov->setText(str);
	
	str="";
	QTextStream wos2(&str);
	wos2 << qSetRealNumberPrecision(3) << StelApp::getInstance().getFps() << " FPS";
	fps->setText(str);
	
	datetime->setPos(10, 0);

	QRectF rectCh = getButtonsBoundingRect();
	
	fov->setPos(rectCh.right()-150, 0);
	fps->setPos(rectCh.right()-70, 0);
	
	double left = datetime->pos().x()+datetime->boundingRect().width();
	double right = fov->pos().x();
	double newPosX = left+(right-left)/2.-location->boundingRect().width()/2.;
	if (std::fabs(newPosX-location->pos().x())>20)
		location->setPos(newPosX,0);
}

void BottomStelBar::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	updateText();
}

QRectF BottomStelBar::boundingRect() const
{
	return childrenBoundingRect();
}

StelBarsPath::StelBarsPath(QGraphicsItem* parent) : QGraphicsPathItem(parent)
{
	roundSize = 6;
	QPen aPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	aPen.setWidthF(1.);
	setPen(aPen);
}

void StelBarsPath::updatePath(BottomStelBar* bot, LeftStelBar* lef)
{
	QPainterPath newPath;
	QPointF p = lef->pos();
	QRectF r = lef->boundingRect();
	QPointF p2 = bot->pos();
	QRectF r2 = bot->boundingRect();
	
	newPath.moveTo(p.x()-roundSize, p.y()-roundSize);
	newPath.lineTo(p.x()+r.width(),p.y()-roundSize);
	newPath.arcTo(p.x()+r.width()-roundSize, p.y()-roundSize, 2.*roundSize, 2.*roundSize, 90, -90);
	newPath.lineTo(p.x()+r.width()+roundSize, p2.y()-roundSize);
	newPath.lineTo(p2.x()+r2.width(),p2.y()-roundSize);
	newPath.arcTo(p2.x()+r2.width()-roundSize, p2.y()-roundSize, 2.*roundSize, 2.*roundSize, 90, -90);
	newPath.lineTo(p2.x()+r2.width()+roundSize, p2.y()+r2.height()+roundSize);
	setPath(newPath);
}


NewGui::NewGui()
{
	setObjectName("NewGui");
	winBar = NULL;
	buttonBar = NULL;
	buttonBarPath = NULL;
	buttonHelpLabel = NULL;
	ui = new Ui_Form();

	// These ugly minimum and maximum sizes are to prevent the setupUi
	// from setting the window size (which we have already set from
	// the values in config.ini)
	QSize windowSize = StelMainWindow::getInstance().size();
	QSize oldMinSize = StelMainWindow::getInstance().minimumSize();
	QSize oldMaxSize = StelMainWindow::getInstance().maximumSize();
	StelMainWindow::getInstance().setMinimumSize(windowSize);
	StelMainWindow::getInstance().setMaximumSize(windowSize);
	ui->setupUi(&StelMainWindow::getInstance());
	
	// OK, un-fix the window size again
	StelMainWindow::getInstance().setMinimumSize(oldMinSize);
	StelMainWindow::getInstance().setMaximumSize(oldMaxSize);
	
	animLeftBarTimeLine = new QTimeLine(200, this);
	animLeftBarTimeLine->setCurveShape(QTimeLine::EaseInOutCurve);
	connect(animLeftBarTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateBarsPos(qreal)));
	
	animBottomBarTimeLine = new QTimeLine(200, this);
	animBottomBarTimeLine->setCurveShape(QTimeLine::EaseInOutCurve);
	connect(animBottomBarTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateBarsPos(qreal)));
}

NewGui::~NewGui()
{
	// When the QGraphicsItems are deleted, they are automatically removed from the scene
	delete winBar;
	delete buttonBar;
	delete buttonBarPath;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double NewGui::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
		return 100000;
	if (actionName==StelModule::ACTION_HANDLEMOUSEMOVES)
		return -1;
	return 0;
}


void NewGui::init()
{	
	std::cout << "Creating GUI" << std::endl;
	QString fName;
	try
	{
		fName = StelApp::getInstance().getFileMgr().findFile("data/DejaVuSans.ttf");
	}
	catch (exception& e)
	{
		qWarning() << "ERROR while loading font DejaVuSans : " << e.what();
	}
	if (!fName.isEmpty())
		QFontDatabase::addApplicationFont(fName);
	
	///////////////////////////////////////////////////////////////////////////
	// Set up the new GUI
	// The actions need to be added to the main form to be effective
	foreach (QObject* obj, StelMainWindow::getInstance().children())
	{
		QAction* a = qobject_cast<QAction *>(obj);
		if (a)
		{
			if (!a->shortcut().isEmpty())
			{
				ui->helpBrowser->setKey(
					a->property("helpGroup").toString(), "",
					a->shortcut().toString(), a->text());
			}
			StelMainWindow::getInstance().addAction(a);
		}
	}
	
	// Connect all the GUI actions signals with the Core of Stellarium
	QObject* module = GETSTELMODULE("ConstellationMgr");
	ConstellationMgr* cmgr = (ConstellationMgr*)module;
	QObject::connect(ui->actionShow_Constellation_Lines, SIGNAL(toggled(bool)), module, SLOT(setFlagLines(bool)));
	ui->actionShow_Constellation_Lines->setChecked(cmgr->getFlagLines());
	QObject::connect(ui->actionShow_Constellation_Art, SIGNAL(toggled(bool)), module, SLOT(setFlagArt(bool)));
	ui->actionShow_Constellation_Art->setChecked(cmgr->getFlagArt());
	QObject::connect(ui->actionShow_Constellation_Labels, SIGNAL(toggled(bool)), module, SLOT(setFlagNames(bool)));
	ui->actionShow_Constellation_Labels->setChecked(cmgr->getFlagNames());
	
	module = GETSTELMODULE("GridLinesMgr");
	GridLinesMgr* gmgr = (GridLinesMgr*)module;
	QObject::connect(ui->actionShow_Equatorial_Grid, SIGNAL(toggled(bool)), module, SLOT(setFlagEquatorGrid(bool)));
	ui->actionShow_Equatorial_Grid->setChecked(gmgr->getFlagEquatorGrid());
	QObject::connect(ui->actionShow_Azimutal_Grid, SIGNAL(toggled(bool)), module, SLOT(setFlagAzimutalGrid(bool)));
	ui->actionShow_Azimutal_Grid->setChecked(gmgr->getFlagAzimutalGrid());
	
	module = GETSTELMODULE("LandscapeMgr");
	LandscapeMgr* lmgr = (LandscapeMgr*)module;
	QObject::connect(ui->actionShow_Ground, SIGNAL(toggled(bool)), module, SLOT(setFlagLandscape(bool)));
	ui->actionShow_Ground->setChecked(lmgr->getFlagLandscape());
	QObject::connect(ui->actionShow_Cardinal_Points, SIGNAL(toggled(bool)), module, SLOT(setFlagCardinalsPoints(bool)));
	ui->actionShow_Cardinal_Points->setChecked(lmgr->getFlagCardinalsPoints());
	QObject::connect(ui->actionShow_Atmosphere, SIGNAL(toggled(bool)), module, SLOT(setFlagAtmosphere(bool)));
	ui->actionShow_Atmosphere->setChecked(lmgr->getFlagAtmosphere());
	
	module = GETSTELMODULE("NebulaMgr");
	NebulaMgr* nmgr = (NebulaMgr*)module;
	QObject::connect(ui->actionShow_Nebulas, SIGNAL(toggled(bool)), module, SLOT(setFlagHints(bool)));
	ui->actionShow_Nebulas->setChecked(nmgr->getFlagHints());
	
	module = (QObject*)StelApp::getInstance().getCore()->getNavigation();
	QObject::connect(ui->actionIncrease_Time_Speed, SIGNAL(triggered()), module, SLOT(increaseTimeSpeed()));
	QObject::connect(ui->actionDecrease_Time_Speed, SIGNAL(triggered()), module, SLOT(decreaseTimeSpeed()));
	QObject::connect(ui->actionSet_Real_Time_Speed, SIGNAL(triggered()), module, SLOT(setRealTimeSpeed()));
	QObject::connect(ui->actionReturn_To_Current_Time, SIGNAL(triggered()), module, SLOT(setTimeNow()));
	QObject::connect(ui->actionSwitch_Equatorial_Mount, SIGNAL(toggled(bool)), module, SLOT(setEquatorialMount(bool)));
			
	module = &StelApp::getInstance();
	QObject::connect(ui->actionShow_Night_Mode, SIGNAL(toggled(bool)), module, SLOT(setVisionModeNight(bool)));
	ui->actionShow_Night_Mode->setChecked(StelApp::getInstance().getVisionModeNight());
	
	module = GETSTELMODULE("MovementMgr");
	QObject::connect(ui->actionGoto_Selected_Object, SIGNAL(triggered()), module, SLOT(setFlagTracking()));
	
	QObject::connect(ui->actionSet_Full_Screen, SIGNAL(toggled(bool)), &StelMainWindow::getInstance(), SLOT(setFullScreen(bool)));
	ui->actionSet_Full_Screen->setChecked(StelMainWindow::getInstance().isFullScreen());
	
	
	///////////////////////////////////////////////////////////////////////////
	//// QGraphicsView based GUI
	///////////////////////////////////////////////////////////////////////////
	// Create the help label
	buttonHelpLabel = new QGraphicsSimpleTextItem("");
	buttonHelpLabel->setFont(QFont("DejaVuSans", 12));
	buttonHelpLabel->setBrush(QBrush(QColor::fromRgbF(1,1,1,1)));
	
	// Construct the Windows buttons bar
	winBar = new LeftStelBar(NULL);
	
	QPixmap pxmapGlow(":/graphicGui/gui/glow.png");
	QPixmap pxmapOn(":/graphicGui/gui/1-on-time.png");
	QPixmap pxmapOff(":/graphicGui/gui/1-off-time.png");
	StelButton* b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, NULL, buttonHelpLabel);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/2-on-location.png");
	pxmapOff = QPixmap(":/graphicGui/gui/2-off-location.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, ui->actionShow_Location_Window, buttonHelpLabel);	
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/3-on-sky.png");
	pxmapOff = QPixmap(":/graphicGui/gui/3-off-sky.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, NULL, buttonHelpLabel);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/4-on-skylore.png");
	pxmapOff = QPixmap(":/graphicGui/gui/4-off-skylore.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, NULL, buttonHelpLabel);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/gui/5-off-labels.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, NULL, buttonHelpLabel);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/gui/6-off-search.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, ui->actionShow_Search_Window, buttonHelpLabel);
	winBar->addButton(b);	
	
	
	pxmapOn = QPixmap(":/graphicGui/gui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/gui/8-off-settings.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, NULL, buttonHelpLabel);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/gui/9-off-help.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, ui->actionShow_Help_Window, buttonHelpLabel);
	winBar->addButton(b);
	
	
	// Construct the bottom buttons bar
	buttonBar = new BottomStelBar(NULL);
	
	QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
	QPixmap pxmapBlank(":/graphicGui/gui/btBlank.png");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLines-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLines-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Constellation_Lines, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLabels-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLabels-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Constellation_Labels, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationArt-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationArt-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Constellation_Art, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Equatorial_Grid, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btAzimutalGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAzimutalGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Azimutal_Grid, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btGround-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGround-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Ground, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btCardinalPoints-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btCardinalPoints-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Cardinal_Points, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btAtmosphere-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAtmosphere-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Atmosphere, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btNebula-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNebula-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Nebulas, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialMount-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialMount-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionSwitch_Equatorial_Mount, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btGotoSelectedObject-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGotoSelectedObject-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionGoto_Selected_Object, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btNightView-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNightView-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionShow_Night_Mode, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btQuit.png");
	b = new StelButton(NULL, pxmapOn, pxmapOn, pxmapGlow32x32, ui->actionQuit, buttonHelpLabel);
	buttonBar->addButton(b);
	
	b = new StelButton(NULL, pxmapBlank, pxmapBlank, pxmapBlank);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRewind-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRewind-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionDecrease_Time_Speed, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRealtime-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRealtime-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionSet_Real_Time_Speed, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeNow-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeNow-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionReturn_To_Current_Time, buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeForward-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeForward-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, ui->actionIncrease_Time_Speed, buttonHelpLabel);
	buttonBar->addButton(b);
	
	// The path drawn around the button bars
	buttonBarPath = new StelBarsPath(NULL);
	buttonBarPath->updatePath(buttonBar, winBar);
	
	QGraphicsScene* scene = StelMainWindow::getInstance().getGraphicsView()->scene();
	scene->addItem(buttonHelpLabel);
	scene->addItem(winBar);
	scene->addItem(buttonBar);
	scene->addItem(buttonBarPath);
	
	// Readjust position
	glWindowHasBeenResized((int)scene->sceneRect().width(), (int)scene->sceneRect().height());
}

void NewGui::glWindowHasBeenResized(int ww, int hh)
{
	double h=hh;
	if (!winBar || !buttonBar || !buttonHelpLabel)
		return;
	
	double rangeX = winBar->boundingRect().width()+2.*buttonBarPath->getRoundSize();
	winBar->setPos(buttonBarPath->getRoundSize()-(1.-animLeftBarTimeLine->currentValue())*rangeX-0.5, h-winBar->boundingRect().height()-buttonBar->boundingRect().height()-20);
	
	double rangeY = buttonBar->boundingRect().height()-7.-buttonBarPath->getRoundSize();
	buttonBar->setPos(buttonBarPath->getRoundSize(), h-buttonBar->boundingRect().height()-buttonBarPath->getRoundSize()+0.5+(1.-animBottomBarTimeLine->currentValue())*rangeY);
	buttonHelpLabel->setPos(winBar->pos().x()+winBar->boundingRect().right()+buttonBarPath->getRoundSize()+10, buttonBar->pos().y()-buttonBarPath->getRoundSize()-25);
	buttonBarPath->updatePath(buttonBar, winBar);
}


void NewGui::updateI18n()
{
}


bool NewGui::handleMouseMoves(int x, int y)
{
	double maxX = winBar->boundingRect().width()+2.*buttonBarPath->getRoundSize();
	double minX = 0;
	if (x<maxX && animLeftBarTimeLine->state()==QTimeLine::NotRunning && winBar->pos().x()<minX)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}
	if (x>maxX && animLeftBarTimeLine->state()==QTimeLine::NotRunning && winBar->pos().x()>=minX)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Backward);
		animLeftBarTimeLine->start();
	}
	
	double maxY = buttonBar->boundingRect().height()+2.*buttonBarPath->getRoundSize();
	if (y<maxY && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()<1.)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}
	if (y>maxY && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()>=0.9999999)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Backward);
		animBottomBarTimeLine->start();
	}
	
	return false;
}

void NewGui::updateBarsPos(qreal value)
{
	glWindowHasBeenResized(StelMainWindow::getInstance().getGraphicsView()->size().width(), StelMainWindow::getInstance().getGraphicsView()->size().height());
}
