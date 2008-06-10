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
#include "MovementMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelAppGraphicsItem.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelMainWindow.hpp"
#include "StelObjectMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StarMgr.hpp"
#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "NebulaMgr.hpp"
#include "Observer.hpp"
#include "StelLocaleMgr.hpp"
#include "Navigator.hpp"
#include "StelObjectType.hpp"
#include "StelObject.hpp"
#include "Projector.hpp"
#include "SolarSystem.hpp"
#include "SkyBackground.hpp"

#include <QPaintDevice>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QRectF>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QTimeLine>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QKeySequence>
#include <QRegExp>

#include <vector>

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
	{
		QString tip(action->toolTip());
		QString shortcut(action->shortcut().toString());
		if (!shortcut.isEmpty())
		{
			if (shortcut == "Space")
				shortcut = q_("Space");
			tip += "  [" + shortcut + "]";
		}
		helpLabel->setText(tip);
	}
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
	double posY = 0;
	if (children().size()!=0)
	{
		const QRectF& r = childrenBoundingRect();
		posY += r.bottom()-1;
	}
	button->setParentItem(this);
	button->setPos(0.5, posY+10.5);
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
	
	// The text is dummy just for testing
	datetime = new QGraphicsSimpleTextItem("2008-02-06  17:33", this);
	location = new QGraphicsSimpleTextItem("Munich, Earth, 500m", this);
	fov = new QGraphicsSimpleTextItem("FOV 43.45", this);
	fps = new QGraphicsSimpleTextItem("43.2 FPS", this);
	
	QColor color = QColor::fromRgbF(1,1,1,1);
	setColor(color);
	
	datetime->setFont(font);
	location->setFont(font);
	fov->setFont(font);
	fps->setFont(font);
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
	
	QRectF rectCh = boundingRect();
	
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
	
	datetime->setText(StelApp::getInstance().getLocaleMgr().get_printable_date_local(jd) +"   "
	                  +StelApp::getInstance().getLocaleMgr().get_printable_time_local(jd));
	
	location->setText(core->getObservatory()->getHomePlanetNameI18n() +", "
	                  +core->getObservatory()->getLocationName() + ", "
			  // xgettext:no-c-format
	                  +q_("%1m").arg(core->getObservatory()->getAltitude()));
	
	QString str;
	QTextStream wos(&str);
	wos << "FOV " << qSetRealNumberPrecision(3) << core->getProjection()->getFov() << QChar(0x00B0);
	fov->setText(str);
	
	str="";
	QTextStream wos2(&str);
	wos2 << qSetRealNumberPrecision(3) << StelApp::getInstance().getFps() << " FPS";
	fps->setText(str);
	
	datetime->setPos(10, 0);

	QRectF rectCh = getButtonsBoundingRect();
	
	fov->setPos(rectCh.right()-170, 0);
	fps->setPos(rectCh.right()-60, 0);
	
	double left = datetime->pos().x()+datetime->boundingRect().width();
	double right = fov->pos().x();
	double newPosX = left+(right-left)/2.-location->boundingRect().width()/2.;
	if (std::fabs(newPosX-location->pos().x())>20)
		location->setPos((int)newPosX,0);
}

void BottomStelBar::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	updateText();
}

QRectF BottomStelBar::boundingRect() const
{
	if (children().size()==0)
		return QRectF();
	const QRectF& r = childrenBoundingRect();
	return QRectF(0, 0, r.width()-1, r.height()-1);
}

// Set the pen for all the sub elements
void BottomStelBar::setColor(const QColor& c)
{
	datetime->setBrush(c);
	location->setBrush(c);
	fov->setBrush(c);
	fps->setBrush(c);
}
	
InfoPanel::InfoPanel(QGraphicsItem* parent) : QGraphicsItem(parent)
{
	text = new QGraphicsTextItem("", this);
	object = NULL;
}

void InfoPanel::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	std::vector<StelObjectP> selected = StelApp::getInstance().getStelObjectMgr().getSelectedObject();

	if (selected.size() == 0)
		text->setHtml("");
	else
	{
		// just print details of the first item for now
		StelCore* core = StelApp::getInstance().getCore();
		text->setHtml(selected[0]->getInfoString(core->getNavigation()));
	}
}

QRectF InfoPanel::boundingRect() const
{
	return childrenBoundingRect();
}

StelBarsPath::StelBarsPath(QGraphicsItem* parent) : QGraphicsPathItem(parent)
{
	roundSize = 6;
	QPen aPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	aPen.setWidthF(1.);
	setBrush(QBrush(QColor::fromRgbF(0.1, 0.13, 0.23, 0.2)));
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
	newPath.lineTo(p.x()-roundSize, p2.y()+r2.height()+roundSize);
	setPath(newPath);
}


NewGui::NewGui()
{
	setObjectName("NewGui");
	winBar = NULL;
	buttonBar = NULL;
	infoPanel = NULL;
	buttonBarPath = NULL;
	buttonHelpLabel = NULL;
	
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
	delete infoPanel;
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
	qDebug() << "Creating GUI ...";
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
	StelMainGraphicsView::getInstance().setWindowIcon(QIcon(":/mainWindow/icon.bmp"));
	loadStyle("data/gui/normalStyle.css");
	
	///////////////////////////////////////////////////////////////////////
	// Create all the main actions of the program, associated with shortcuts
	QString group = N_("Display Options");
	addGuiActions("actionShow_Constellation_Lines", N_("Constellation lines"), "C", group, true, false);
	addGuiActions("actionShow_Constellation_Art", N_("Constellation art"), "R", group, true, false);
	addGuiActions("actionShow_Constellation_Labels", N_("Constellation labels"), "V", group, true, false);
	addGuiActions("actionShow_Constellation_Boundaries", N_("Constellation boundaries"), "B", group, true, false);
	
	addGuiActions("actionShow_Azimutal_Grid", N_("Azimutal grid"), "Z", group, true, false);
	addGuiActions("actionShow_Equatorial_Grid", N_("Equatorial grid"), "E", group, true, false);
	addGuiActions("actionShow_Equatorial_J2000_Grid", N_("Equatorial J2000 grid"), "", group, true, false);
	addGuiActions("actionShow_Ecliptic_Line", N_("Ecliptic line"), ",", group, true, false);
	addGuiActions("actionShow_Equator_Line", N_("Equator line"), ".", group, true, false);
	addGuiActions("actionShow_Meridian_Line", N_("Meridian line"), "", group, true, false);
	addGuiActions("actionShow_Cardinal_Points", N_("Cardinal points"), "Q", group, true, false);

	addGuiActions("actionShow_Ground", N_("Ground"), "G", group, true, false);
	addGuiActions("actionShow_Atmosphere", N_("Atmosphere"), "A", group, true, false);
	addGuiActions("actionShow_Fog", N_("Fog"), "F", group, true, false);
	
	addGuiActions("actionShow_Nebulas", N_("Nebulas"), "N", group, true, false);
	addGuiActions("actionShow_DSS", N_("DSS"), "", group, true, false);
	addGuiActions("actionShow_Stars", N_("Stars"), "S", group, true, false);
	addGuiActions("actionShow_Planets_Hints", N_("Planets hints"), "P", group, true, false);
	
	addGuiActions("actionShow_Night_Mode", N_("Night mode"), "", group, true, false);
	addGuiActions("actionSet_Full_Screen", N_("Full-screen mode"), "F11", group, true, false);
	addGuiActions("actionHorizontal_Flip", N_("Flip scene horizontally"), "Ctrl+Shift+H", group, true, false);
	addGuiActions("actionVertical_Flip", N_("Flip scene vertically"), "Ctrl+Shift+V", group, true, false);
	
	group = N_("Windows");
	addGuiActions("actionShow_Help_Window", N_("Help window"), "F1", group, true, false);
	addGuiActions("actionShow_Configuration_Window", N_("Configuration window"), "F2", group, true, false);
	addGuiActions("actionShow_Search_Window", N_("Search window"), "F3, Ctrl+F", group, true, false);
	addGuiActions("actionShow_SkyView_Window", N_("Sky and viewing options window"), "F4", group, true, false);
	addGuiActions("actionShow_DateTime_Window", N_("Date/time window"), "F5", group, true, false);
	addGuiActions("actionShow_Location_Window", N_("Location window"), "F6", group, true, false);
	
	group = N_("Date and Time");
	addGuiActions("actionDecrease_Time_Speed", N_("Decrease time speed"), "J", group, false, false);
	addGuiActions("actionIncrease_Time_Speed", N_("Increase time speed"), "L", group, false, false);
	addGuiActions("actionSet_Real_Time_Speed", N_("Set normal time rate"), "K", group, false, false);
	addGuiActions("actionReturn_To_Current_Time", N_("Set time to now"), "8", group, false, false);
	addGuiActions("actionAdd_Solar_Hour", N_("Add 1 solar hour"), "Ctrl+=", group, false, true);
	addGuiActions("actionSubtract_Solar_Hour", N_("Subtract 1 solar hour"), "Ctrl+-", group, false, true);
	addGuiActions("actionAdd_Solar_Day", N_("Add 1 solar day"), "=", group, false, true);
	addGuiActions("actionSubtract_Solar_Day", N_("Subtract 1 solar day"), "-", group, false, true);
	addGuiActions("actionAdd_Solar_Week", N_("Add 1 solar week"), "]", group, false, true);
	addGuiActions("actionSubtract_Solar_Week", N_("Subtract 1 solar week"), "[", group, false, true);
	addGuiActions("actionAdd_Sidereal_Day", N_("Add 1 sidereal day"), "Alt+=", group, false, true);
	addGuiActions("actionSubtract_Sidereal_Day", N_("Subtract 1 sidereal day"), "Alt+-", group, false, true);
	addGuiActions("actionAdd_Sidereal_Week", N_("Add 1 sidereal week"), "Alt+]", group, false, true);
	addGuiActions("actionSubtract_Sidereal_Week", N_("Subtract 1 sidereal week"), "Alt+[", group, false, true);

	group = N_("Movement and Selection");
	addGuiActions("actionGoto_Selected_Object", N_("Center on selected object"), "Space", group, false, false);
	addGuiActions("actionSet_Tracking", N_("Track object"), "T", group, true, false);
	addGuiActions("actionZoom_In_Auto", N_("Zoom in on selected object"), "/", group, false, false);
	addGuiActions("actionZoom_Out_Auto", N_("Zoom out"), "\\", group, false, false);
	addGuiActions("actionSet_Home_Planet_To_Selected", N_("Set home planet to selected planet"), "Ctrl+G", group, false, false);
	
	group = N_("Miscellaneous");
	addGuiActions("actionSwitch_Equatorial_Mount", N_("Switch between equatorial and azimuthal mount"), "Ctrl+M", group, true, false);
	addGuiActions("actionQuit", N_("Quit"), "Ctrl+Q", group, false, false);
	addGuiActions("actionSave_Screenshot", N_("Save screenshot"), "Ctrl+S", group, false, false);
	addGuiActions("action_Reload_Style", "Reload Style", "Ctrl+R", "debug", false, false);
	
	//QMetaObject::connectSlotsByName(Form);
	
	///////////////////////////////////////////////////////////////////////
	// Connect all the GUI actions signals with the Core of Stellarium
	QObject::connect(getGuiActions("actionQuit"), SIGNAL(triggered()), &StelMainWindow::getInstance(), SLOT(close()));
	
	// Debug
	QObject::connect(getGuiActions("action_Reload_Style"), SIGNAL(triggered()), this, SLOT(reloadStyle()));
	
	QObject* module = GETSTELMODULE("ConstellationMgr");
	ConstellationMgr* cmgr = (ConstellationMgr*)module;
	QObject::connect(getGuiActions("actionShow_Constellation_Lines"), SIGNAL(toggled(bool)), module, SLOT(setFlagLines(bool)));
	getGuiActions("actionShow_Constellation_Lines")->setChecked(cmgr->getFlagLines());
	QObject::connect(getGuiActions("actionShow_Constellation_Art"), SIGNAL(toggled(bool)), module, SLOT(setFlagArt(bool)));
	getGuiActions("actionShow_Constellation_Art")->setChecked(cmgr->getFlagArt());
	QObject::connect(getGuiActions("actionShow_Constellation_Labels"), SIGNAL(toggled(bool)), module, SLOT(setFlagNames(bool)));
	getGuiActions("actionShow_Constellation_Labels")->setChecked(cmgr->getFlagNames());
	QObject::connect(getGuiActions("actionShow_Constellation_Boundaries"), SIGNAL(toggled(bool)), module, SLOT(setFlagBoundaries(bool)));
	getGuiActions("actionShow_Constellation_Boundaries")->setChecked(cmgr->getFlagBoundaries());
	
	module = GETSTELMODULE("GridLinesMgr");
	GridLinesMgr* gmgr = (GridLinesMgr*)module;
	QObject::connect(getGuiActions("actionShow_Equatorial_Grid"), SIGNAL(toggled(bool)), module, SLOT(setFlagEquatorGrid(bool)));
	getGuiActions("actionShow_Equatorial_Grid")->setChecked(gmgr->getFlagEquatorGrid());
	QObject::connect(getGuiActions("actionShow_Azimutal_Grid"), SIGNAL(toggled(bool)), module, SLOT(setFlagAzimutalGrid(bool)));
	getGuiActions("actionShow_Azimutal_Grid")->setChecked(gmgr->getFlagAzimutalGrid());
	QObject::connect(getGuiActions("actionShow_Ecliptic_Line"), SIGNAL(toggled(bool)), module, SLOT(setFlagEclipticLine(bool)));
	getGuiActions("actionShow_Ecliptic_Line")->setChecked(gmgr->getFlagEclipticLine());
	QObject::connect(getGuiActions("actionShow_Equator_Line"), SIGNAL(toggled(bool)), module, SLOT(setFlagEquatorLine(bool)));
	getGuiActions("actionShow_Equator_Line")->setChecked(gmgr->getFlagEquatorLine());
	
	module = GETSTELMODULE("LandscapeMgr");
	LandscapeMgr* lmgr = (LandscapeMgr*)module;
	QObject::connect(getGuiActions("actionShow_Ground"), SIGNAL(toggled(bool)), module, SLOT(setFlagLandscape(bool)));
	getGuiActions("actionShow_Ground")->setChecked(lmgr->getFlagLandscape());
	QObject::connect(getGuiActions("actionShow_Cardinal_Points"), SIGNAL(toggled(bool)), module, SLOT(setFlagCardinalsPoints(bool)));
	getGuiActions("actionShow_Cardinal_Points")->setChecked(lmgr->getFlagCardinalsPoints());
	QObject::connect(getGuiActions("actionShow_Atmosphere"), SIGNAL(toggled(bool)), module, SLOT(setFlagAtmosphere(bool)));
	getGuiActions("actionShow_Atmosphere")->setChecked(lmgr->getFlagAtmosphere());
	QObject::connect(getGuiActions("actionShow_Fog"), SIGNAL(toggled(bool)), module, SLOT(setFlagFog(bool)));
	getGuiActions("actionShow_Fog")->setChecked(lmgr->getFlagFog());
	
	module = GETSTELMODULE("NebulaMgr");
	NebulaMgr* nmgr = (NebulaMgr*)module;
	QObject::connect(getGuiActions("actionShow_Nebulas"), SIGNAL(toggled(bool)), module, SLOT(setFlagHints(bool)));
	getGuiActions("actionShow_Nebulas")->setChecked(nmgr->getFlagHints());
	
	module = GETSTELMODULE("SkyBackground");
	SkyBackground* bmgr = (SkyBackground*)module;
	QObject::connect(getGuiActions("actionShow_DSS"), SIGNAL(toggled(bool)), module, SLOT(setFlagShow(bool)));
	getGuiActions("actionShow_DSS")->setChecked(bmgr->getFlagShow());
	
	module = (QObject*)StelApp::getInstance().getCore()->getNavigation();
	QObject::connect(getGuiActions("actionIncrease_Time_Speed"), SIGNAL(triggered()), module, SLOT(increaseTimeSpeed()));
	QObject::connect(getGuiActions("actionDecrease_Time_Speed"), SIGNAL(triggered()), module, SLOT(decreaseTimeSpeed()));
	QObject::connect(getGuiActions("actionSet_Real_Time_Speed"), SIGNAL(triggered()), module, SLOT(setRealTimeSpeed()));
	QObject::connect(getGuiActions("actionReturn_To_Current_Time"), SIGNAL(triggered()), module, SLOT(setTimeNow()));
	QObject::connect(getGuiActions("actionSwitch_Equatorial_Mount"), SIGNAL(toggled(bool)), module, SLOT(setEquatorialMount(bool)));
	QObject::connect(getGuiActions("actionAdd_Solar_Hour"), SIGNAL(triggered()), module, SLOT(addHour()));
	QObject::connect(getGuiActions("actionAdd_Solar_Day"), SIGNAL(triggered()), module, SLOT(addDay()));
	QObject::connect(getGuiActions("actionAdd_Solar_Week"), SIGNAL(triggered()), module, SLOT(addWeek()));
	QObject::connect(getGuiActions("actionSubtract_Solar_Hour"), SIGNAL(triggered()), module, SLOT(subtractHour()));
	QObject::connect(getGuiActions("actionSubtract_Solar_Day"), SIGNAL(triggered()), module, SLOT(subtractDay()));
	QObject::connect(getGuiActions("actionSubtract_Solar_Week"), SIGNAL(triggered()), module, SLOT(subtractWeek()));
	QObject::connect(getGuiActions("actionAdd_Sidereal_Day"), SIGNAL(triggered()), module, SLOT(addSiderealDay()));
	QObject::connect(getGuiActions("actionAdd_Sidereal_Week"), SIGNAL(triggered()), module, SLOT(addSiderealWeek()));
	QObject::connect(getGuiActions("actionSubtract_Sidereal_Day"), SIGNAL(triggered()), module, SLOT(subtractSiderealDay()));
	QObject::connect(getGuiActions("actionSubtract_Sidereal_Week"), SIGNAL(triggered()), module, SLOT(subtractSiderealWeek()));
	QObject::connect(getGuiActions("actionSet_Home_Planet_To_Selected"), SIGNAL(triggered()), module, SLOT(moveObserverToSelected()));

			
	module = &StelApp::getInstance();
	QObject::connect(getGuiActions("actionShow_Night_Mode"), SIGNAL(toggled(bool)), module, SLOT(setVisionModeNight(bool)));
	getGuiActions("actionShow_Night_Mode")->setChecked(StelApp::getInstance().getVisionModeNight());
	
	module = GETSTELMODULE("MovementMgr");
	QObject::connect(getGuiActions("actionGoto_Selected_Object"), SIGNAL(triggered()), module, SLOT(setFlagTracking()));
	QObject::connect(getGuiActions("actionZoom_In_Auto"), SIGNAL(triggered()), module, SLOT(autoZoomIn()));
	QObject::connect(getGuiActions("actionZoom_Out_Auto"), SIGNAL(triggered()), module, SLOT(autoZoomOut()));
	QObject::connect(getGuiActions("actionSet_Tracking"), SIGNAL(toggled(bool)), module, SLOT(setFlagTracking(bool)));
	MovementMgr* mmgr = (MovementMgr*)module;
	getGuiActions("actionSet_Tracking")->setChecked(mmgr->getFlagTracking());
	
	QObject::connect(getGuiActions("actionSet_Full_Screen"), SIGNAL(triggered(bool)), &StelMainWindow::getInstance(), SLOT(setFullScreen(bool)));
	getGuiActions("actionSet_Full_Screen")->setChecked(StelMainWindow::getInstance().isFullScreen());
	
	QObject::connect(getGuiActions("actionShow_Location_Window"), SIGNAL(toggled(bool)), &locationDialog, SLOT(setVisible(bool)));
	QObject::connect(&locationDialog, SIGNAL(closed()), getGuiActions("actionShow_Location_Window"), SLOT(toggle()));

	QObject::connect(getGuiActions("actionShow_Configuration_Window"), SIGNAL(toggled(bool)), &configurationDialog, SLOT(setVisible(bool)));
	QObject::connect(&configurationDialog, SIGNAL(closed()), getGuiActions("actionShow_Configuration_Window"), SLOT(toggle()));
	
	QObject::connect(getGuiActions("actionShow_SkyView_Window"), SIGNAL(toggled(bool)), &viewDialog, SLOT(setVisible(bool)));
	QObject::connect(&viewDialog, SIGNAL(closed()), getGuiActions("actionShow_SkyView_Window"), SLOT(toggle()));
	
	QObject::connect(getGuiActions("actionShow_Help_Window"), SIGNAL(toggled(bool)), &helpDialog, SLOT(setVisible(bool)));
	QObject::connect(&helpDialog, SIGNAL(closed()), getGuiActions("actionShow_Help_Window"), SLOT(toggle()));
	
	QObject::connect(getGuiActions("actionShow_DateTime_Window"), SIGNAL(toggled(bool)), &dateTimeDialog, SLOT(setVisible(bool)));
	QObject::connect(&dateTimeDialog, SIGNAL(closed()), getGuiActions("actionShow_DateTime_Window"), SLOT(toggle()));
	
	QObject::connect(getGuiActions("actionShow_Search_Window"), SIGNAL(toggled(bool)), &searchDialog, SLOT(setVisible(bool)));
	QObject::connect(&searchDialog, SIGNAL(closed()), getGuiActions("actionShow_Search_Window"), SLOT(toggle()));

	getGuiActions("actionSet_Full_Screen")->setChecked(StelMainGraphicsView::getInstance().isFullScreen());
	
	QObject::connect(getGuiActions("actionSave_Screenshot"), SIGNAL(triggered()), &StelMainGraphicsView::getInstance(), SLOT(saveScreenShot()));

	const Projector* proj = StelApp::getInstance().getCore()->getProjection();
	QObject::connect(getGuiActions("actionHorizontal_Flip"), SIGNAL(toggled(bool)), proj, SLOT(setFlipHorz(bool)));
	getGuiActions("actionHorizontal_Flip")->setChecked(proj->getFlipHorz());
	QObject::connect(getGuiActions("actionVertical_Flip"), SIGNAL(toggled(bool)), proj, SLOT(setFlipVert(bool)));
	getGuiActions("actionVertical_Flip")->setChecked(proj->getFlipVert());
	
	StarMgr* smgr = (StarMgr*)GETSTELMODULE("StarMgr");
	QObject::connect(getGuiActions("actionShow_Stars"), SIGNAL(toggled(bool)), smgr, SLOT(setFlagStars(bool)));
	getGuiActions("actionShow_Stars")->setChecked(smgr->getFlagStars());
	
	SolarSystem* ssmgr = (SolarSystem*)GETSTELMODULE("SolarSystem");
	QObject::connect(getGuiActions("actionShow_Planets_Hints"), SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagHints(bool)));
	getGuiActions("actionShow_Planets_Hints")->setChecked(ssmgr->getFlagHints());
	
	QObject::connect(getGuiActions("actionShow_Meridian_Line"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagMeridianLine(bool)));
	getGuiActions("actionShow_Meridian_Line")->setChecked(gmgr->getFlagMeridianLine());
	
	QObject::connect(getGuiActions("actionShow_Equatorial_J2000_Grid"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagEquatorJ2000Grid(bool)));
	getGuiActions("actionShow_Equatorial_J2000_Grid")->setChecked(gmgr->getFlagEquatorJ2000Grid());
	
	///////////////////////////////////////////////////////////////////////////
	//// QGraphicsView based GUI
	///////////////////////////////////////////////////////////////////////////
	// Create the help label
	buttonHelpLabel = new QGraphicsSimpleTextItem("");
	buttonHelpLabel->setFont(QFont("DejaVuSans", 11));
	buttonHelpLabel->setBrush(QBrush(QColor::fromRgbF(1,1,1,1)));
	
	// Construct the Windows buttons bar
	winBar = new LeftStelBar(NULL);
	
	QPixmap pxmapGlow(":/graphicGui/gui/glow.png");
	QPixmap pxmapOn(":/graphicGui/gui/1-on-time.png");
	QPixmap pxmapOff(":/graphicGui/gui/1-off-time.png");
	StelButton* b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_DateTime_Window"), buttonHelpLabel);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/2-on-location.png");
	pxmapOff = QPixmap(":/graphicGui/gui/2-off-location.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Location_Window"), buttonHelpLabel);	
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/gui/5-off-labels.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_SkyView_Window"), buttonHelpLabel);
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/gui/6-off-search.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Search_Window"), buttonHelpLabel);
	winBar->addButton(b);	
	
	
	pxmapOn = QPixmap(":/graphicGui/gui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/gui/8-off-settings.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Configuration_Window"), buttonHelpLabel);
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/gui/9-off-help.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Help_Window"), buttonHelpLabel);
	winBar->addButton(b);
	
	
	// Construct the bottom buttons bar
	buttonBar = new BottomStelBar(NULL);

	infoPanel = new InfoPanel(NULL);
	
	QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
	QPixmap pxmapBlank(":/graphicGui/gui/btBlank.png");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLines-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLines-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Lines"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLabels-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLabels-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Labels"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationArt-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationArt-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Art"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Equatorial_Grid"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btAzimutalGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAzimutalGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Azimutal_Grid"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btGround-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGround-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Ground"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btCardinalPoints-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btCardinalPoints-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Cardinal_Points"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btAtmosphere-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAtmosphere-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Atmosphere"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btNebula-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNebula-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Nebulas"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btDSS-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btDSS-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_DSS"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialMount-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialMount-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSwitch_Equatorial_Mount"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btGotoSelectedObject-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGotoSelectedObject-off.png");
	buttonGotoSelectedObject = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionGoto_Selected_Object"), buttonHelpLabel);
	buttonBar->addButton(buttonGotoSelectedObject);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btNightView-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNightView-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Night_Mode"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btQuit.png");
	b = new StelButton(NULL, pxmapOn, pxmapOn, pxmapGlow32x32, getGuiActions("actionQuit"), buttonHelpLabel);
	buttonBar->addButton(b);
	
	b = new StelButton(NULL, pxmapBlank, pxmapBlank, pxmapBlank);
	buttonBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRewind-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRewind-off.png");
	buttonTimeRewind = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionDecrease_Time_Speed"), buttonHelpLabel);
	buttonBar->addButton(buttonTimeRewind);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRealtime-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRealtime-off.png");
	buttonTimeRealTimeSpeed = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSet_Real_Time_Speed"), buttonHelpLabel);
	buttonBar->addButton(buttonTimeRealTimeSpeed);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeNow-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeNow-off.png");
	buttonTimeCurrent = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionReturn_To_Current_Time"), buttonHelpLabel);
	buttonBar->addButton(buttonTimeCurrent);
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeForward-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeForward-off.png");
	buttonTimeForward = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionIncrease_Time_Speed"), buttonHelpLabel);
	buttonBar->addButton(buttonTimeForward);
	
	// The path drawn around the button bars
	buttonBarPath = new StelBarsPath(NULL);
	buttonBarPath->updatePath(buttonBar, winBar);
	buttonBarPath->setZValue(-0.1);
	
	QGraphicsScene* scene = StelMainGraphicsView::getInstance().scene();
	scene->addItem(buttonHelpLabel);
	scene->addItem(winBar);
	scene->addItem(buttonBar);
	scene->addItem(buttonBarPath);
	scene->addItem(infoPanel);
	
	// Readjust position
	glWindowHasBeenResized((int)scene->sceneRect().width(), (int)scene->sceneRect().height());
}

//! Load color scheme from the given ini file and section name
void NewGui::setColorScheme(const QSettings* conf, const QString& section)
{
	if (section=="night_color")
	{
		loadStyle("data/gui/nightStyle.css");
		buttonBarPath->setPen(QColor::fromRgbF(0.7,0.2,0.2,0.5));
		buttonBarPath->setBrush(QColor::fromRgbF(0.23, 0.13, 0.03, 0.2));
		buttonHelpLabel->setBrush(QColor::fromRgbF(0.9, 0.33, 0.33, 0.9));
		buttonBar->setColor(QColor::fromRgbF(0.9, 0.33, 0.33, 0.9));
	}
	else
	{
		loadStyle("data/gui/normalStyle.css");
		buttonBarPath->setPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
		buttonBarPath->setBrush(QColor::fromRgbF(0.1, 0.13, 0.23, 0.2));
		buttonHelpLabel->setBrush(QColor::fromRgbF(0.9, 0.9, 1.0, 0.9));
		buttonBar->setColor(QColor::fromRgbF(0.9, 0.9, 1.0, 0.9));
	}
}
	
//! Load a Qt style sheet to define the widgets style
void NewGui::loadStyle(const QString& fileName)
{
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	QString styleFilePath;
	try
	{
		styleFilePath = fileMan.findFile(fileName);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: can't find Qt style sheet";
	}
	QFile styleFile(styleFilePath);
	styleFile.open(QIODevice::ReadOnly);
	StelMainGraphicsView::getInstance().setStyleSheet(styleFile.readAll());
}

//! Reload the current Qt Style Sheet (Debug only)
void NewGui::reloadStyle()
{
	loadStyle("data/gui/normalStyle.css");
}

void NewGui::glWindowHasBeenResized(int ww, int hh)
{
	double h=hh;
	if (!winBar || !buttonBar || !buttonHelpLabel)
		return;
	
	double rangeX = winBar->boundingRect().width()+2.*buttonBarPath->getRoundSize();
	winBar->setPos(buttonBarPath->getRoundSize()-(1.-animLeftBarTimeLine->currentValue())*rangeX-0.5, h-winBar->boundingRect().height()-buttonBar->boundingRect().height()-20);
	
	double rangeY = buttonBar->boundingRect().height()+0.5-7.-buttonBarPath->getRoundSize();
	buttonBar->setPos(winBar->boundingRect().right()+buttonBarPath->getRoundSize(), h-buttonBar->boundingRect().height()-buttonBarPath->getRoundSize()+0.5+(1.-animBottomBarTimeLine->currentValue())*rangeY);
	
	buttonHelpLabel->setPos((int)(winBar->pos().x()+winBar->boundingRect().right()+buttonBarPath->getRoundSize()+8), (int)(buttonBar->pos().y()-buttonBarPath->getRoundSize()-20));
	
	buttonBarPath->updatePath(buttonBar, winBar);

	infoPanel->setPos(8,8);
}


void NewGui::updateI18n()
{
	// Translate all action texts
	foreach (QObject* obj, StelMainGraphicsView::getInstance().children())
	{
		QAction* a = qobject_cast<QAction*>(obj);
		if (a)
		{
			const QString& englishText = a->property("englishText").toString();
			if (!englishText.isEmpty())
			{
				a->setText(q_(englishText));
			}
		}
	}

	// Update the dialogs
	configurationDialog.languageChanged();
	dateTimeDialog.languageChanged();
	helpDialog.languageChanged();
	locationDialog.languageChanged();
	searchDialog.languageChanged();
	viewDialog.languageChanged();
}

void NewGui::update(double deltaTime)
{
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	if (nav->getTimeSpeed()<-0.99*JD_SECOND)
	{
		buttonTimeRewind->setChecked(true);
	}
	else
	{
		buttonTimeRewind->setChecked(false);
	}
	if (nav->getTimeSpeed()>1.01*JD_SECOND)
	{
		buttonTimeForward->setChecked(true);
	}
	else
	{
		buttonTimeForward->setChecked(false);
	}
	if (buttonTimeRealTimeSpeed->isChecked()!=nav->getRealTimeSpeed())
	{
		buttonTimeRealTimeSpeed->setChecked(nav->getRealTimeSpeed());
	}
	const bool isTimeNow=nav->getIsTimeNow();
	if (buttonTimeCurrent->isChecked()!=isTimeNow)
		buttonTimeCurrent->setChecked(isTimeNow);
	MovementMgr* mmgr = (MovementMgr*)GETSTELMODULE("MovementMgr");
	const bool b = mmgr->getFlagTracking();
	if (buttonGotoSelectedObject->isChecked()!=b)
		buttonGotoSelectedObject->setChecked(b);
	
	StarMgr* smgr = (StarMgr*)GETSTELMODULE("StarMgr");
	if (getGuiActions("actionShow_Stars")->isChecked()!=smgr->getFlagStars())
		getGuiActions("actionShow_Stars")->setChecked(smgr->getFlagStars());
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
	glWindowHasBeenResized(StelMainGraphicsView::getInstance().size().width(), StelMainGraphicsView::getInstance().size().height());
}

// Note: "text" and "helpGroup" must be in English -- this method and the help
// dialog take care of translating them. Of course, they still have to be
// marked for translation using the N_() macro.
void NewGui::addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable, bool autoRepeat)
{
	QAction* a;
	a = new QAction(&StelMainGraphicsView::getInstance());
	a->setObjectName(actionName);
	a->setText(q_(text));
	QList<QKeySequence> shortcuts;
	QStringList shortcutStrings = shortCut.split(QRegExp(",(?!,|$)"));
	for (int i = 0; i < shortcutStrings.size(); ++i)
		shortcuts << QKeySequence(shortcutStrings.at(i).trimmed());
	
	a->setShortcuts(shortcuts);
	a->setCheckable(checkable);
	a->setAutoRepeat(autoRepeat);
	a->setProperty("englishText", QVariant(text));
	a->setShortcutContext(Qt::WidgetShortcut);
	if (!shortCut.isEmpty())
		helpDialog.setKey(helpGroup, "", shortCut, text);
	StelMainGraphicsView::getInstance().addAction(a);
}

QAction* NewGui::getGuiActions(const QString& actionName)
{
	QAction* a = StelMainGraphicsView::getInstance().findChild<QAction*>(actionName);
	if (!a)
		qWarning() << "Can't find action " << actionName;
	Q_ASSERT(a);
	return a;
}


void NewGui::retranslateUi(QWidget *Form)
{
	Form->setWindowTitle(QApplication::translate("Form", "Stellarium", 0, QApplication::UnicodeUTF8));
} // retranslateUi
