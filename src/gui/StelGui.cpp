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

#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "Projector.hpp"
#include "MovementMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelMainWindow.hpp"
#include "StelObjectMgr.hpp"
#include "LandscapeMgr.hpp"
#include "TelescopeMgr.hpp"
#include "StarMgr.hpp"
#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "NebulaMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "Navigator.hpp"
#include "StelObjectType.hpp"
#include "StelObject.hpp"
#include "Projector.hpp"
#include "SolarSystem.hpp"
#include "SkyImageMgr.hpp"
#include "StelStyle.hpp"
#include "SkyDrawer.hpp"
#include "MeteorMgr.hpp"
#include "DownloadPopup.hpp"
#include "DownloadMgr.hpp"

#include <QDebug>
#include <QTimeLine>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QTextDocument>
#include <QTextBrowser>
#include <QGraphicsWidget>

InfoPanel::InfoPanel(QGraphicsItem* parent) : QGraphicsTextItem("", parent)
{
	QSettings* conf = StelApp::getInstance().getSettings();
        Q_ASSERT(conf);
	QString objectInfo = conf->value("gui/selected_object_info", "all").toString();
	if (objectInfo == "all")
		infoTextFilters = StelObject::InfoStringGroup(StelObject::AllInfo);
	else if (objectInfo == "short")
		infoTextFilters = StelObject::InfoStringGroup(StelObject::ShortInfo);
	else if (objectInfo == "none")
		infoTextFilters = StelObject::InfoStringGroup(0);
	else
	{
		qWarning() << "config.ini option gui/selected_object_info is invalid, using \"all\"";
		infoTextFilters = StelObject::InfoStringGroup(StelObject::AllInfo);
	}

	QFont font("DejaVuSans");
	font.setPixelSize(13);
	setFont(font);
}

void InfoPanel::setTextFromObjects(const QList<StelObjectP>& selected)
{
	if (selected.size() == 0)
	{
		if (!document()->isEmpty())
			document()->clear();
	}
	else
	{
		// just print details of the first item for now
		QString s = selected[0]->getInfoString(StelApp::getInstance().getCore(), infoTextFilters);
		setHtml(s);
	}
}

StelGui::StelGui() : initDone(false)
{
	// QPixmapCache::setCacheLimit(30000); ?
	
	setObjectName("StelGui");
	winBar = NULL;
	buttonBar = NULL;
	infoPanel = NULL;
	buttonBarPath = NULL;
	lastButtonbarWidth = 0;
	autoHidebts = NULL;

	autoHideHorizontalButtonBar = true;
	autoHideVerticalButtonBar = true;
			
	animLeftBarTimeLine = new QTimeLine(200, this);
	animLeftBarTimeLine->setCurveShape(QTimeLine::EaseInOutCurve);
	connect(animLeftBarTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateBarsPos()));
	
	animBottomBarTimeLine = new QTimeLine(200, this);
	animBottomBarTimeLine->setCurveShape(QTimeLine::EaseInOutCurve);
	connect(animBottomBarTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateBarsPos()));

	flipHoriz = NULL;
	flipVert = NULL;
	btShowNebulaeBackground = NULL;
}

StelGui::~StelGui()
{
	infoPanel->deleteLater();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double StelGui::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return 100000;
	if (actionName==StelModule::ActionHandleMouseMoves)
		return -1;
	return 0;
}


void StelGui::init()
{
	qDebug() << "Creating GUI ...";
	
	///////////////////////////////////////////////////////////////////////
	// Create all the main actions of the program, associated with shortcuts
	QString group = N_("Display Options");
	addGuiActions("actionShow_Constellation_Lines", N_("Constellation lines"), "C", group, true, false, "viewing/flag_constellation_drawing");
	addGuiActions("actionShow_Constellation_Art", N_("Constellation art"), "R", group, true, false, "viewing/flag_constellation_art");
	addGuiActions("actionShow_Constellation_Labels", N_("Constellation labels"), "V", group, true, false, "viewing/flag_constellation_name");
	addGuiActions("actionShow_Constellation_Boundaries", N_("Constellation boundaries"), "B", group, true, false, "viewing/flag_constellation_boundaries");
	
	addGuiActions("actionShow_Azimuthal_Grid", N_("Azimuthal grid"), "Z", group, true, false, "viewing/flag_azimuthal_grid");
	addGuiActions("actionShow_Equatorial_Grid", N_("Equatorial grid"), "E", group, true, false, "viewing/flag_equatorial_grid");
	addGuiActions("actionShow_Equatorial_J2000_Grid", N_("Equatorial J2000 grid"), "", group, true, false, "viewing/flag_equatorial_J2000_grid");
	addGuiActions("actionShow_Ecliptic_Line", N_("Ecliptic line"), ",", group, true, false, "viewing/flag_ecliptic_line");
	addGuiActions("actionShow_Equator_Line", N_("Equator line"), ".", group, true, false, "viewing/flag_equator_line");
	addGuiActions("actionShow_Meridian_Line", N_("Meridian line"), "", group, true, false, "viewing/flag_meridian_line");
	addGuiActions("actionShow_Cardinal_Points", N_("Cardinal points"), "Q", group, true, false, "viewing/flag_cardinal_points");

	addGuiActions("actionShow_Ground", N_("Ground"), "G", group, true, false, "landscape/flag_landscape");
	addGuiActions("actionShow_Atmosphere", N_("Atmosphere"), "A", group, true, false, "landscape/flag_atmosphere");
	addGuiActions("actionShow_Fog", N_("Fog"), "F", group, true, false, "landscape/flag_fog");
	
	addGuiActions("actionShow_Nebulas", N_("Nebulas"), "N", group, true, false, "astro/flag_nebula_name");
	addGuiActions("actionShow_DSS", N_("Nebulas background images"), "", group, true, false);
	addGuiActions("actionShow_Stars", N_("Stars"), "S", group, true, false, "astro/flag_stars");
	addGuiActions("actionShow_Planets_Hints", N_("Planets hints"), "P", group, true, false, "astro/flag_planets_hints");
	
	addGuiActions("actionShow_Night_Mode", N_("Night mode"), "", group, true, false, "viewing/flag_night");
	addGuiActions("actionSet_Full_Screen_Global", N_("Full-screen mode"), "F11", group, true, false); // TODO: move persistence here? (currently elsewhere)
	addGuiActions("actionHorizontal_Flip", N_("Flip scene horizontally"), "Ctrl+Shift+H", group, true, false);
	addGuiActions("actionVertical_Flip", N_("Flip scene vertically"), "Ctrl+Shift+V", group, true, false);
	
	group = N_("Windows");
	addGuiActions("actionShow_Help_Window_Global", N_("Help window"), "F1", group, true, false);
	addGuiActions("actionShow_Configuration_Window_Global", N_("Configuration window"), "F2", group, true, false);
	addGuiActions("actionShow_Search_Window_Global", N_("Search window"), "F3, Ctrl+F", group, true, false);
	addGuiActions("actionShow_SkyView_Window_Global", N_("Sky and viewing options window"), "F4", group, true, false);
	addGuiActions("actionShow_DateTime_Window_Global", N_("Date/time window"), "F5", group, true, false);
	addGuiActions("actionShow_Location_Window_Global", N_("Location window"), "F6", group, true, false);
	
	group = N_("Date and Time");
	addGuiActions("actionDecrease_Time_Speed", N_("Decrease time speed"), "J", group, false, false);
	addGuiActions("actionIncrease_Time_Speed", N_("Increase time speed"), "L", group, false, false);
	addGuiActions("actionSet_Real_Time_Speed", N_("Set normal time rate"), "K", group, false, false);
	addGuiActions("actionSet_Time_Rate_Zero", N_("Set time rate to zero"), "7", group, false, false);
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
	
	group = N_("Telescope Control");
	addGuiActions("actionMove_Telescope_To_Selection_0", N_("Move telescope #0 to selected object"), "Ctrl+0", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_1", N_("Move telescope #1 to selected object"), "Ctrl+1", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_2", N_("Move telescope #2 to selected object"), "Ctrl+2", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_3", N_("Move telescope #3 to selected object"), "Ctrl+3", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_4", N_("Move telescope #4 to selected object"), "Ctrl+4", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_5", N_("Move telescope #5 to selected object"), "Ctrl+5", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_6", N_("Move telescope #6 to selected object"), "Ctrl+6", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_7", N_("Move telescope #7 to selected object"), "Ctrl+7", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_8", N_("Move telescope #8 to selected object"), "Ctrl+8", group, false, false);
	addGuiActions("actionMove_Telescope_To_Selection_9", N_("Move telescope #9 to selected object"), "Ctrl+9", group, false, false);

	group = N_("Miscellaneous");
	addGuiActions("actionSwitch_Equatorial_Mount", N_("Switch between equatorial and azimuthal mount"), "Ctrl+M", group, true, false);
	addGuiActions("actionQuit_Global", N_("Quit"), "Ctrl+Q", group, false, false);
	addGuiActions("actionSave_Screenshot_Global", N_("Save screenshot"), "Ctrl+S", group, false, false);
	addGuiActions("action_Reload_Style", "Reload style", "Ctrl+R", "Debug", false, false);
	
	addGuiActions("actionAutoHideHorizontalButtonBar", N_("Auto hide horizontal button bar"), "", group, true, false);
	addGuiActions("actionAutoHideVerticalButtonBar", N_("Auto hide vertical button bar"), "", group, true, false);
	addGuiActions("actionToggle_GuiHidden_Global", N_("Toggle visibility of toolbars"), "Ctrl+T", group, true, false);

	
	//QMetaObject::connectSlotsByName(Form);
	
	///////////////////////////////////////////////////////////////////////
	// Connect all the GUI actions signals with the Core of Stellarium
	QObject::connect(getGuiActions("actionQuit_Global"), SIGNAL(triggered()), this, SLOT(quitStellarium()));
	
	// Debug
	QObject::connect(getGuiActions("action_Reload_Style"), SIGNAL(triggered()), this, SLOT(reloadStyle()));

	QObject* module = GETSTELMODULE("ConstellationMgr");
	ConstellationMgr* cmgr = (ConstellationMgr*)module;
	QObject::connect(getGuiActions("actionShow_Constellation_Lines"), SIGNAL(toggled(bool)), module, SLOT(setFlagLines(bool)));
	getGuiActions("actionShow_Constellation_Lines")->setChecked(cmgr->getFlagLines());
	QObject::connect(getGuiActions("actionShow_Constellation_Art"), SIGNAL(toggled(bool)), module, SLOT(setFlagArt(bool)));
	getGuiActions("actionShow_Constellation_Art")->setChecked(cmgr->getFlagArt());
	QObject::connect(getGuiActions("actionShow_Constellation_Labels"), SIGNAL(toggled(bool)), module, SLOT(setFlagLabels(bool)));
	getGuiActions("actionShow_Constellation_Labels")->setChecked(cmgr->getFlagLabels());
	QObject::connect(getGuiActions("actionShow_Constellation_Boundaries"), SIGNAL(toggled(bool)), module, SLOT(setFlagBoundaries(bool)));
	getGuiActions("actionShow_Constellation_Boundaries")->setChecked(cmgr->getFlagBoundaries());
	
	module = GETSTELMODULE("GridLinesMgr");
	GridLinesMgr* gmgr = (GridLinesMgr*)module;
	QObject::connect(getGuiActions("actionShow_Equatorial_Grid"), SIGNAL(toggled(bool)), module, SLOT(setFlagEquatorGrid(bool)));
	getGuiActions("actionShow_Equatorial_Grid")->setChecked(gmgr->getFlagEquatorGrid());
	QObject::connect(getGuiActions("actionShow_Azimuthal_Grid"), SIGNAL(toggled(bool)), module, SLOT(setFlagAzimuthalGrid(bool)));
	getGuiActions("actionShow_Azimuthal_Grid")->setChecked(gmgr->getFlagAzimuthalGrid());
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
	
	module = GETSTELMODULE("SkyImageMgr");
	SkyImageMgr* bmgr = (SkyImageMgr*)module;
	QObject::connect(getGuiActions("actionShow_DSS"), SIGNAL(toggled(bool)), module, SLOT(setFlagShow(bool)));
	getGuiActions("actionShow_DSS")->setChecked(bmgr->getFlagShow());
	
	module = (QObject*)StelApp::getInstance().getCore()->getNavigation();
	QObject::connect(getGuiActions("actionIncrease_Time_Speed"), SIGNAL(triggered()), module, SLOT(increaseTimeSpeed()));
	QObject::connect(getGuiActions("actionDecrease_Time_Speed"), SIGNAL(triggered()), module, SLOT(decreaseTimeSpeed()));
	QObject::connect(getGuiActions("actionSet_Real_Time_Speed"), SIGNAL(triggered()), module, SLOT(setRealTimeSpeed()));
	QObject::connect(getGuiActions("actionSet_Time_Rate_Zero"), SIGNAL(triggered()), module, SLOT(setZeroTimeSpeed()));
	QObject::connect(getGuiActions("actionReturn_To_Current_Time"), SIGNAL(triggered()), module, SLOT(setTimeNow()));
	QObject::connect(getGuiActions("actionSwitch_Equatorial_Mount"), SIGNAL(toggled(bool)), module, SLOT(setEquatorialMount(bool)));
	getGuiActions("actionSwitch_Equatorial_Mount")->setChecked(StelApp::getInstance().getCore()->getNavigation()->getMountMode() != Navigator::MountAltAzimuthal);
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

	module = GETSTELMODULE("TelescopeMgr");
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_0"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_1"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_2"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_3"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_4"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_5"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_6"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_7"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_8"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));
	QObject::connect(getGuiActions("actionMove_Telescope_To_Selection_9"), SIGNAL(triggered()), module, SLOT(moveTelescopeToSelected()));

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
	
	QObject::connect(getGuiActions("actionSet_Full_Screen_Global"), SIGNAL(toggled(bool)), &StelMainWindow::getInstance(), SLOT(setFullScreen(bool)));
	getGuiActions("actionSet_Full_Screen_Global")->setChecked(StelMainWindow::getInstance().isFullScreen());
	
	QObject::connect(getGuiActions("actionShow_Location_Window_Global"), SIGNAL(toggled(bool)), &locationDialog, SLOT(setVisible(bool)));
	QObject::connect(&locationDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_Location_Window_Global"), SLOT(setChecked(bool)));

	QObject::connect(getGuiActions("actionShow_Configuration_Window_Global"), SIGNAL(toggled(bool)), &configurationDialog, SLOT(setVisible(bool)));
	QObject::connect(&configurationDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_Configuration_Window_Global"), SLOT(setChecked(bool)));
	
	QObject::connect(getGuiActions("actionShow_SkyView_Window_Global"), SIGNAL(toggled(bool)), &viewDialog, SLOT(setVisible(bool)));
	QObject::connect(&viewDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_SkyView_Window_Global"), SLOT(setChecked(bool)));
	
	QObject::connect(getGuiActions("actionShow_Help_Window_Global"), SIGNAL(toggled(bool)), &helpDialog, SLOT(setVisible(bool)));
	QObject::connect(&helpDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_Help_Window_Global"), SLOT(setChecked(bool)));
	
	QObject::connect(getGuiActions("actionShow_DateTime_Window_Global"), SIGNAL(toggled(bool)), &dateTimeDialog, SLOT(setVisible(bool)));
	QObject::connect(&dateTimeDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_DateTime_Window_Global"), SLOT(setChecked(bool)));
	
	QObject::connect(getGuiActions("actionShow_Search_Window_Global"), SIGNAL(toggled(bool)), &searchDialog, SLOT(setVisible(bool)));
	QObject::connect(&searchDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_Search_Window_Global"), SLOT(setChecked(bool)));
	
	QObject::connect(getGuiActions("actionSave_Screenshot_Global"), SIGNAL(triggered()), &StelMainGraphicsView::getInstance(), SLOT(saveScreenShot()));
	getGuiActions("actionToggle_GuiHidden_Global")->setChecked(false);
	QObject::connect(getGuiActions("actionToggle_GuiHidden_Global"), SIGNAL(triggered()), this, SLOT(toggleHideGui()));

	QObject::connect(getGuiActions("actionHorizontal_Flip"), SIGNAL(toggled(bool)), StelApp::getInstance().getCore(), SLOT(setFlipHorz(bool)));
	getGuiActions("actionHorizontal_Flip")->setChecked(StelApp::getInstance().getCore()->getFlipHorz());
	QObject::connect(getGuiActions("actionVertical_Flip"), SIGNAL(toggled(bool)), StelApp::getInstance().getCore(), SLOT(setFlipVert(bool)));
	getGuiActions("actionVertical_Flip")->setChecked(StelApp::getInstance().getCore()->getFlipVert());
	
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
	
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	setAutoHideHorizontalButtonBar(conf->value("gui/auto_hide_horizontal_toolbar", true).toBool());
	setAutoHideVerticalButtonBar(conf->value("gui/auto_hide_vertical_toolbar", true).toBool());
	QObject::connect(getGuiActions("actionAutoHideHorizontalButtonBar"), SIGNAL(toggled(bool)), this, SLOT(setAutoHideHorizontalButtonBar(bool)));
	getGuiActions("actionAutoHideHorizontalButtonBar")->setChecked(getAutoHideHorizontalButtonBar());
	QObject::connect(getGuiActions("actionAutoHideVerticalButtonBar"), SIGNAL(toggled(bool)), this, SLOT(setAutoHideVerticalButtonBar(bool)));
	getGuiActions("actionAutoHideVerticalButtonBar")->setChecked(getAutoHideVerticalButtonBar());
	
	///////////////////////////////////////////////////////////////////////////
	//// QGraphicsView based GUI
	///////////////////////////////////////////////////////////////////////////
	
	// Construct the Windows buttons bar
	winBar = new LeftStelBar(NULL);
	
	QPixmap pxmapGlow(":/graphicGui/gui/glow.png");
	QPixmap pxmapOn(":/graphicGui/gui/2-on-location.png");
	QPixmap pxmapOff(":/graphicGui/gui/2-off-location.png");
	StelButton*  b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Location_Window_Global"));	
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/1-on-time.png");
	pxmapOff = QPixmap(":/graphicGui/gui/1-off-time.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_DateTime_Window_Global"));
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/gui/5-off-labels.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_SkyView_Window_Global"));
	winBar->addButton(b);	
	
	pxmapOn = QPixmap(":/graphicGui/gui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/gui/6-off-search.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Search_Window_Global"));
	winBar->addButton(b);	
	
	
	pxmapOn = QPixmap(":/graphicGui/gui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/gui/8-off-settings.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Configuration_Window_Global"));
	winBar->addButton(b);
	
	pxmapOn = QPixmap(":/graphicGui/gui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/gui/9-off-help.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Help_Window_Global"));
	winBar->addButton(b);
	
	
	// Construct the bottom buttons bar
	buttonBar = new BottomStelBar(NULL, QPixmap(":/graphicGui/gui/btbg-left.png"), QPixmap(":/graphicGui/gui/btbg-right.png"), 
			QPixmap(":/graphicGui/gui/btbg-middle.png"), QPixmap(":/graphicGui/gui/btbg-single.png"));

	infoPanel = new InfoPanel(NULL);
	
	QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLines-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLines-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Lines"));
	buttonBar->addButton(b, "010-constellationsGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLabels-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLabels-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Labels"));
	buttonBar->addButton(b, "010-constellationsGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationArt-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationArt-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Art"));
	buttonBar->addButton(b, "010-constellationsGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Equatorial_Grid"));
	buttonBar->addButton(b, "020-gridsGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btAzimuthalGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAzimuthalGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Azimuthal_Grid"));
	buttonBar->addButton(b, "020-gridsGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btGround-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGround-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Ground"));
	buttonBar->addButton(b, "030-landscapeGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btCardinalPoints-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btCardinalPoints-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Cardinal_Points"));
	buttonBar->addButton(b, "030-landscapeGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btAtmosphere-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAtmosphere-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Atmosphere"));
	buttonBar->addButton(b, "030-landscapeGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btNebula-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNebula-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Nebulas"));
	buttonBar->addButton(b, "040-nebulaeGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btPlanets-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btPlanets-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Planets_Hints"));
	buttonBar->addButton(b, "040-nebulaeGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialMount-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialMount-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSwitch_Equatorial_Mount"));
	b->setChecked(getGuiActions("actionSwitch_Equatorial_Mount")->isChecked());
	buttonBar->addButton(b, "060-othersGroup");
	
	
	pxmapOn = QPixmap(":/graphicGui/gui/btGotoSelectedObject-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGotoSelectedObject-off.png");
	buttonGotoSelectedObject = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionGoto_Selected_Object"));
	buttonBar->addButton(buttonGotoSelectedObject, "060-othersGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btNightView-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNightView-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Night_Mode"));
	buttonBar->addButton(b, "060-othersGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btFullScreen-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btFullScreen-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSet_Full_Screen_Global"));
	buttonBar->addButton(b, "060-othersGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btQuit.png");
	b = new StelButton(NULL, pxmapOn, pxmapOn, pxmapGlow32x32, getGuiActions("actionQuit_Global"));
	buttonBar->addButton(b, "060-othersGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRewind-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRewind-off.png");
	buttonTimeRewind = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionDecrease_Time_Speed"));
	buttonBar->addButton(buttonTimeRewind, "070-timeGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRealtime-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRealtime-off.png");
	buttonTimeRealTimeSpeed = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSet_Real_Time_Speed"));
	buttonBar->addButton(buttonTimeRealTimeSpeed, "070-timeGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeNow-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeNow-off.png");
	buttonTimeCurrent = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionReturn_To_Current_Time"));
	buttonBar->addButton(buttonTimeCurrent, "070-timeGroup");
	
	pxmapOn = QPixmap(":/graphicGui/gui/btTimeForward-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeForward-off.png");
	buttonTimeForward = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionIncrease_Time_Speed"));
	buttonBar->addButton(buttonTimeForward, "070-timeGroup");

	buttonBar->setGroupMargin("070-timeGroup", 32, 0);
	
	// The path drawn around the button bars
	buttonBarPath = new StelBarsPath(NULL);
	buttonBarPath->updatePath(buttonBar, winBar);
	buttonBarPath->setZValue(-0.1);
	
	// Used to display some progress bar in the lower right corner, e.g. when loading a file
	progressBarMgr = new StelProgressBarMgr(NULL);
									
	// Add everything in the scene
	QGraphicsScene* scene = StelMainGraphicsView::getInstance().scene();
	scene->addItem(winBar);
	scene->addItem(buttonBar);
	scene->addItem(buttonBarPath);
	scene->addItem(infoPanel);
	scene->addItem(progressBarMgr);
	
	// Create the 2 auto hide buttons in the bottom left corner
	autoHidebts = new CornerButtons();
	pxmapOn = QPixmap(":/graphicGui/gui/HorizontalAutoHideOn.png");
	pxmapOff = QPixmap(":/graphicGui/gui/HorizontalAutoHideOff.png");
	btHorizAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), getGuiActions("actionAutoHideHorizontalButtonBar"), true);
	btHorizAutoHide->setChecked(getAutoHideHorizontalButtonBar());
	
	pxmapOn = QPixmap(":/graphicGui/gui/VerticalAutoHideOn.png");
	pxmapOff = QPixmap(":/graphicGui/gui/VerticalAutoHideOff.png");
	btVertAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), getGuiActions("actionAutoHideVerticalButtonBar"), true);
	btVertAutoHide->setChecked(getAutoHideVerticalButtonBar());
	
	btHorizAutoHide->setPos(1,btVertAutoHide->pixmap().height()-btHorizAutoHide->pixmap().height()+1);
	btVertAutoHide->setPos(0,0);
	btVertAutoHide->setZValue(1000);
	scene->addItem(autoHidebts);

	// Re-adjust position
	setStelStyle(*StelApp::getInstance().getCurrentStelStyle());
	updateBarsPos();
	infoPanel->setPos(8,8);

	// If auto hide is off, show the relevant toolbars
	if (!getAutoHideHorizontalButtonBar())
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}

	if (!getAutoHideVerticalButtonBar())
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}

	// add the flip buttons if requested in the config
	setFlagShowFlipButtons(conf->value("gui/flag_show_flip_buttons", false).toBool());
	setFlagShowNebulaBackgroundButton(conf->value("gui/flag_show_nebulae_background_button", false).toBool());

	initDone = true;
}

//! Reload the current Qt Style Sheet (Debug only)
void StelGui::reloadStyle()
{
	setStelStyle(*StelApp::getInstance().getCurrentStelStyle());
}

//! Load color scheme from the given ini file and section name
void StelGui::setStelStyle(const StelStyle& style)
{
	qApp->setStyleSheet(style.qtStyleSheet);
	
	if (style.confSectionName=="night_color")
	{
		buttonBarPath->setPen(QColor::fromRgbF(0.7,0.2,0.2,0.5));
		buttonBarPath->setBrush(QColor::fromRgbF(0.23, 0.13, 0.03, 0.2));
		buttonBar->setColor(QColor::fromRgbF(0.9, 0.33, 0.33, 0.9));
		winBar->setColor(QColor::fromRgbF(0.9, 0.33, 0.33, 0.9));
		winBar->setRedMode(true);
		buttonBar->setRedMode(true);
		btHorizAutoHide->setRedMode(true);
		btVertAutoHide->setRedMode(true);
	}
	else
	{
		buttonBarPath->setPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
		buttonBarPath->setBrush(QColor::fromRgbF(0.15, 0.16, 0.19, 0.2));
		buttonBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
		winBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
		buttonBar->setRedMode(false);
		winBar->setRedMode(false);
		btHorizAutoHide->setRedMode(false);
		btVertAutoHide->setRedMode(false);
	}
	
	locationDialog.styleChanged();
	dateTimeDialog.styleChanged();
	configurationDialog.styleChanged();
	searchDialog.styleChanged();
	viewDialog.styleChanged();
}


void StelGui::glWindowHasBeenResized(int ww, int hh)
{
	if (!winBar || !buttonBar)
		return;
	
	updateBarsPos();
}


void StelGui::updateI18n()
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

void StelGui::update(double deltaTime)
{
	Navigator* nav = StelApp::getInstance().getCore()->getNavigation();
	if (nav->getTimeRate()<-0.99*JD_SECOND)
	{
		if (buttonTimeRewind->isChecked()==false)
			buttonTimeRewind->setChecked(true);
	}
	else
	{
		if (buttonTimeRewind->isChecked()==true)
			buttonTimeRewind->setChecked(false);
	}
	if (nav->getTimeRate()>1.01*JD_SECOND)
	{
		if (buttonTimeForward->isChecked()==false)
			buttonTimeForward->setChecked(true);
	}
	else
	{
		if (buttonTimeRewind->isChecked()==true)
			buttonTimeRewind->setChecked(false);
	}
	if (buttonTimeRealTimeSpeed->isChecked()!=nav->getRealTimeSpeed())
	{
		if (buttonTimeRealTimeSpeed->isChecked()!=nav->getRealTimeSpeed())
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
	
	infoPanel->setTextFromObjects(StelApp::getInstance().getStelObjectMgr().getSelectedObject());
}

bool StelGui::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
	double maxX = winBar->boundingRect().width()+2.*buttonBarPath->getRoundSize();
	double maxY = winBar->boundingRect().height()+buttonBar->boundingRect().height()+2.*buttonBarPath->getRoundSize();
	double minX = 0;
	if (x<=maxX && y<=maxY && b==Qt::NoButton && animLeftBarTimeLine->state()==QTimeLine::NotRunning && winBar->pos().x()<minX)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}
	if (autoHideVerticalButtonBar && (x>maxX || y>maxY) && animLeftBarTimeLine->state()==QTimeLine::NotRunning && winBar->pos().x()>=minX)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Backward);
		animLeftBarTimeLine->start();
	}
	
	maxX = winBar->boundingRect().width()+buttonBar->boundingRect().width()+2.*buttonBarPath->getRoundSize();
	maxY = buttonBar->boundingRect().height()+2.*buttonBarPath->getRoundSize();
	if (x<=maxX && y<=maxY && b==Qt::NoButton && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()<1.)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}
	if (autoHideHorizontalButtonBar && (x>maxX || y>maxY) && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()>=0.9999999)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Backward);
		animBottomBarTimeLine->start();
	}
	
	return false;
}


// Note: "text" and "helpGroup" must be in English -- this method and the help
// dialog take care of translating them. Of course, they still have to be
// marked for translation using the N_() macro.
QAction* StelGui::addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable, bool autoRepeat, const QString& persistenceName)
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
	a->setProperty("persistenceName", QVariant(persistenceName));
	a->setShortcutContext(Qt::WidgetShortcut);
	if (!shortCut.isEmpty())
		helpDialog.setKey(helpGroup, "", shortCut, text);
	StelMainGraphicsView::getInstance().addAction(a);
	
	// connect(a, SIGNAL(toggled(bool)), this, SLOT(guiActionTriggered(bool)));
	return a;
}

QAction* StelGui::getGuiActions(const QString& actionName)
{
	QAction* a = StelMainGraphicsView::getInstance().findChild<QAction*>(actionName);
	if (!a)
	{
		qWarning() << "Can't find action " << actionName;
		return NULL;
	}
	return a;
}

// Called each time a GUI action is triggered
void StelGui::guiActionTriggered(bool b)
{
	// can get the action name from: QObject::sender()->objectName()
	// to get the config.ini key:
	// and QObject::sender()->property("persistenceName").toString();
}
	
void StelGui::updateBarsPos()
{
	const int ww = StelMainGraphicsView::getInstance().width();
	const int hh = StelMainGraphicsView::getInstance().height();
			
	bool updatePath = false;
	
	// Use a position cache to avoid useless redraw triggered by the position set if the bars don't move
	double rangeX = winBar->boundingRectNoHelpLabel().width()+2.*buttonBarPath->getRoundSize()+1.;
	const qreal newWinBarX = buttonBarPath->getRoundSize()-(1.-animLeftBarTimeLine->currentValue())*rangeX-0.5;
	const qreal newWinBarY = hh-winBar->boundingRectNoHelpLabel().height()-buttonBar->boundingRectNoHelpLabel().height()-20;
	if (winBar->pos().x()!=newWinBarX || winBar->pos().y()!=newWinBarY)
	{
		winBar->setPos(newWinBarX, newWinBarY);
		updatePath = true;
	}
	
	double rangeY = buttonBar->boundingRectNoHelpLabel().height()+0.5-7.-buttonBarPath->getRoundSize();
	const qreal newButtonBarX = winBar->boundingRectNoHelpLabel().right()+buttonBarPath->getRoundSize();
	const qreal newButtonBarY = hh-buttonBar->boundingRectNoHelpLabel().height()-buttonBarPath->getRoundSize()+0.5+(1.-animBottomBarTimeLine->currentValue())*rangeY;
	if (buttonBar->pos().x()!=newButtonBarX || buttonBar->pos().y()!=newButtonBarY)
	{
		buttonBar->setPos(newButtonBarX, newButtonBarY);
		updatePath = true;
	}
	
	if (lastButtonbarWidth != buttonBar->boundingRectNoHelpLabel().width())
	{
		updatePath = true;
		lastButtonbarWidth = (int)(buttonBar->boundingRectNoHelpLabel().width());
	}
			
	if (updatePath)
		buttonBarPath->updatePath(buttonBar, winBar);

	const qreal newProgressBarX = ww-progressBarMgr->boundingRect().width()-5;
	const qreal newProgressBarY = hh-progressBarMgr->boundingRect().height()-5;
	if (progressBarMgr->pos().x()!=newProgressBarX || progressBarMgr->pos().y()!=newProgressBarY)
	{
		progressBarMgr->setPos(newProgressBarX, newProgressBarY);
		updatePath = true;
	}
	
	// Update position of the auto-hide buttons
	autoHidebts->setPos(0, hh-autoHidebts->childrenBoundingRect().height()+1);
	double opacity = qMax(animLeftBarTimeLine->currentValue(), animBottomBarTimeLine->currentValue());
	autoHidebts->setOpacity(opacity);
}

void StelGui::retranslateUi(QWidget *Form)
{
	Form->setWindowTitle(QApplication::translate("Form", "Stellarium", 0, QApplication::UnicodeUTF8));
} // retranslateUi

// Add a new progress bar in the lower right corner of the screen.
QProgressBar* StelGui::addProgressBar()
{
	return progressBarMgr->addProgressBar();
}

void StelGui::quitStellarium()
{
	if(StelApp::getInstance().getDownloadMgr().isDownloading())
	{
		downloadPopup.setVisible(true);
		downloadPopup.setText(QString("Stellarium is still downloading the star "
			"catalog %1. Would you like to cancel the download and quit "
			"Stellarium? (You can always restart the download at a later time.)")
			.arg(StelApp::getInstance().getDownloadMgr().name()));
		
		connect(&downloadPopup, SIGNAL(cancelClicked()), this, SLOT(cancelDownloadAndQuit()));
		connect(&downloadPopup, SIGNAL(continueClicked()), this, SLOT(dontQuit()));
	}
	else
		QCoreApplication::exit();
}

void StelGui::cancelDownloadAndQuit()
{
	StelApp::getInstance().getDownloadMgr().abort();
	QCoreApplication::exit();
}

void StelGui::dontQuit()
{
	downloadPopup.setVisible(false);
}

QPixmap StelGui::makeRed(const QPixmap& p)
{
	QImage im = p.toImage().convertToFormat(QImage::Format_ARGB32);
	Q_ASSERT(im.format()==QImage::Format_ARGB32);
	QRgb* bits = (QRgb*)im.bits();
	const QRgb* stop = bits+im.width()*im.height();
	do
	{
		*bits = qRgba(qRed(*bits), (int)(0.2*qGreen(*bits)), (int)(0.2*qBlue(*bits)), qAlpha(*bits));
		++bits;
	}
	while (bits!=stop);
		
	return QPixmap::fromImage(im);
}

void StelGui::setFlagShowFlipButtons(bool b)
{
	if (b==true)
	{
		if (flipVert==NULL)
		{
			// Create the vertical flip button
			QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
			flipVert = new StelButton(NULL, 
			                          QPixmap(":/graphicGui/gui/btFlipVertical-on.png"), 
			                          QPixmap(":/graphicGui/gui/btFlipVertical-off.png"), 
			                          pxmapGlow32x32, 
			                          getGuiActions("actionVertical_Flip"));
		}
		if (flipHoriz==NULL)
		{
			QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
			flipHoriz = new StelButton(NULL, 
			                           QPixmap(":/graphicGui/gui/btFlipHorizontal-on.png"), 
			                           QPixmap(":/graphicGui/gui/btFlipHorizontal-off.png"), 
			                           pxmapGlow32x32, 
			                           getGuiActions("actionHorizontal_Flip"));
		}
		getButtonBar()->addButton(flipVert, "060-othersGroup", "actionQuit_Global");
		getButtonBar()->addButton(flipHoriz, "060-othersGroup", "actionVertical_Flip");
	}
	else
	{
		bool b = getButtonBar()->hideButton("actionVertical_Flip")==flipVert;
		Q_ASSERT(b);
		b = getButtonBar()->hideButton("actionHorizontal_Flip")==flipHoriz;
		Q_ASSERT(b);
	}
	flagShowFlipButtons = b;
}


// Define whether the button toggling nebulae backround images should be visible
void StelGui::setFlagShowNebulaBackgroundButton(bool b)
{
	if (b==true)
	{
		if (btShowNebulaeBackground==NULL)
		{
			// Create the nebulae background button
			QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");
			btShowNebulaeBackground = new StelButton(NULL, QPixmap(":/graphicGui/gui/btDSS-on.png"), QPixmap(":/graphicGui/gui/btDSS-off.png"), pxmapGlow32x32, getGuiActions("actionShow_DSS"));
		}
		getButtonBar()->addButton(btShowNebulaeBackground, "040-nebulaeGroup");
	}
	else
	{
		bool b;
		b = getButtonBar()->hideButton("actionShow_DSS")==btShowNebulaeBackground;
		Q_ASSERT(b);
	}
	flagShowNebulaBackgroundButton = b;
}

void StelGui::setHideGui(bool b)
{
	buttonBar->setVisible(!b);
	winBar->setVisible(!b);
	autoHidebts->setVisible(!b);
	buttonBarPath->setVisible(!b);
}

bool StelGui::getHideGui(void)
{
	return getGuiActions("actionToggle_GuiHidden_Global")->isChecked();
}
