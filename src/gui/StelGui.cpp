/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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

#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "SkyGui.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelProjector.hpp"
#include "StelMovementMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelMainWindow.hpp"
#include "StelObjectMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StarMgr.hpp"
#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "NebulaMgr.hpp"
#include "StelLocaleMgr.hpp"

#include "StelObjectType.hpp"
#include "StelObject.hpp"
#include "StelProjector.hpp"
#include "SolarSystem.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelStyle.hpp"
#include "StelSkyDrawer.hpp"
#include "MeteorMgr.hpp"
#ifdef ENABLE_SCRIPT_CONSOLE
#include "ScriptConsole.hpp"
#endif
#ifndef DISABLE_SCRIPTING
#include "StelScriptMgr.hpp"
#endif
#include "StelAppGraphicsWidget.hpp"

#include <QDebug>
#include <QTimeLine>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QTextBrowser>
#include <QGraphicsWidget>
#include <QGraphicsGridLayout>
#include <QClipboard>

StelGuiBase* StelStandardGuiPluginInterface::getStelGuiBase() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(guiRes);

	return new StelGui();
}
Q_EXPORT_PLUGIN2(StelGui, StelStandardGuiPluginInterface)

StelGui::StelGui() : topLevelGraphicsWidget(NULL), configurationDialog(NULL), initDone(false)
{
	// QPixmapCache::setCacheLimit(30000); ?
	flipHoriz = NULL;
	flipVert = NULL;
	btShowNebulaeBackground = NULL;
}

StelGui::~StelGui()
{
	delete skyGui;
	skyGui = NULL;
}

void StelGui::init(QGraphicsWidget* atopLevelGraphicsWidget, StelAppGraphicsWidget* astelAppGraphicsWidget)
{
	qDebug() << "Creating GUI ...";

	StelGuiBase::init(atopLevelGraphicsWidget, astelAppGraphicsWidget);

	skyGui = new SkyGui(atopLevelGraphicsWidget);
	configurationDialog = new ConfigurationDialog(this);

	///////////////////////////////////////////////////////////////////////
	// Create all the main actions of the program, associated with shortcuts
	QString group = N_("Display Options");
	addGuiActions("actionShow_Constellation_Lines", N_("Constellation lines"), "C", group, true, false);
	addGuiActions("actionShow_Constellation_Art", N_("Constellation art"), "R", group, true, false);
	addGuiActions("actionShow_Constellation_Labels", N_("Constellation labels"), "V", group, true, false);
	addGuiActions("actionShow_Constellation_Boundaries", N_("Constellation boundaries"), "B", group, true, false);

	addGuiActions("actionShow_Azimuthal_Grid", N_("Azimuthal grid"), "Z", group, true, false);
	addGuiActions("actionShow_Equatorial_Grid", N_("Equatorial grid"), "E", group, true, false);
	addGuiActions("actionShow_Equatorial_J2000_Grid", N_("Equatorial J2000 grid"), "", group, true, false);
	addGuiActions("actionShow_Ecliptic_J2000_Grid", N_("Ecliptic J2000 grid"), "", group, true, false);
	addGuiActions("actionShow_Galactic_Grid", N_("Galactic grid"), "", group, true, false);
	addGuiActions("actionShow_Galactic_Plane_Line", N_("Galactic plane"), "", group, true, false);
	addGuiActions("actionShow_Ecliptic_Line", N_("Ecliptic line"), ",", group, true, false);
	addGuiActions("actionShow_Equator_Line", N_("Equator line"), ".", group, true, false);
	addGuiActions("actionShow_Meridian_Line", N_("Meridian line"), ";", group, true, false);
	addGuiActions("actionShow_Horizon_Line", N_("Horizon line"), "", group, true, false);
	addGuiActions("actionShow_Cardinal_Points", N_("Cardinal points"), "Q", group, true, false);

	addGuiActions("actionShow_Ground", N_("Ground"), "G", group, true, false);
	addGuiActions("actionShow_Atmosphere", N_("Atmosphere"), "A", group, true, false);
	addGuiActions("actionShow_Fog", N_("Fog"), "F", group, true, false);

	addGuiActions("actionShow_Nebulas", N_("Nebulas"), "N", group, true, false);
	addGuiActions("actionShow_DSS", N_("Nebulas background images"), "", group, true, false);
	addGuiActions("actionShow_Stars", N_("Stars"), "S", group, true, false);
	addGuiActions("actionShow_Planets_Labels", N_("Planet labels"), "P", group, true, false);
	addGuiActions("actionShow_Planets_Orbits", N_("Planet orbits"), "O", group, true, false);
	addGuiActions("actionShow_Planets_Trails", N_("Planet trails"), "Shift+T", group, true, false);

	addGuiActions("actionShow_Night_Mode", N_("Night mode"), "", group, true, false);
	addGuiActions("actionSet_Full_Screen_Global", N_("Full-screen mode"), "F11", group, true, false, true);
	addGuiActions("actionHorizontal_Flip", N_("Flip scene horizontally"), "Ctrl+Shift+H", group, true, false);
	addGuiActions("actionVertical_Flip", N_("Flip scene vertically"), "Ctrl+Shift+V", group, true, false);

	group = N_("Windows");
	addGuiActions("actionShow_Help_Window_Global", N_("Help window"), "F1", group, true, false, true);
	addGuiActions("actionShow_Configuration_Window_Global", N_("Configuration window"), "F2", group, true, false, true);
	addGuiActions("actionShow_Search_Window_Global", N_("Search window"), "F3, Ctrl+F", group, true, false, true);
	addGuiActions("actionShow_SkyView_Window_Global", N_("Sky and viewing options window"), "F4", group, true, false, true);
	addGuiActions("actionShow_DateTime_Window_Global", N_("Date/time window"), "F5", group, true, false, true);
	addGuiActions("actionShow_Location_Window_Global", N_("Location window"), "F6", group, true, false, true);
#ifdef ENABLE_SCRIPT_CONSOLE
	addGuiActions("actionShow_ScriptConsole_Window_Global", N_("Script console window"), "F12", group, true, false, true);
#endif

	group = N_("Date and Time");
	addGuiActions("actionDecrease_Script_Speed", N_("Slow down the script execution rate"), "", group, false, false);
	addGuiActions("actionIncrease_Script_Speed", N_("Speed up the script execution rate"), "", group, false, false);
	addGuiActions("actionSet_Real_Script_Speed", N_("Set the normal script execution rate"), "", group, false, false);
	addGuiActions("actionStop_Script", N_("Pause script execution"), "", group, false, false);
	addGuiActions("actionPause_Script", N_("Pause script execution"), "", group, false, false);
	addGuiActions("actionResume_Script", N_("Resume script execution"), "", group, false, false);
	addGuiActions("actionDecrease_Time_Speed", N_("Decrease time speed"), "J", group, false, false);
	addGuiActions("actionIncrease_Time_Speed", N_("Increase time speed"), "L", group, false, false);
	addGuiActions("actionSet_Real_Time_Speed", N_("Set normal time rate"), "K", group, false, false);
	addGuiActions("actionDecrease_Time_Speed_Less", N_("Decrease time speed (a little)"), "Shift+J", group, false, false);
	addGuiActions("actionIncrease_Time_Speed_Less", N_("Increase time speed (a little)"), "Shift+L", group, false, false);
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
	addGuiActions("actionAdd_Sidereal_Month", N_("Add 1 sidereal month"), "Alt+Shift+]", group, false, true);
	addGuiActions("actionSubtract_Sidereal_Month", N_("Subtract 1 sidereal month"), "Alt+Shift+[", group, false, true);
	addGuiActions("actionAdd_Sidereal_Year", N_("Add 1 sidereal year"), "Ctrl+Alt+Shift+]", group, false, true);
	addGuiActions("actionSubtract_Sidereal_Year", N_("Subtract 1 sidereal year"), "Ctrl+Alt+Shift+[", group, false, true);

	group = N_("Movement and Selection");
	addGuiActions("actionGoto_Selected_Object", N_("Center on selected object"), "Space", group, false, false);
	addGuiActions("actionSet_Tracking", N_("Track object"), "T", group, true, false);
	addGuiActions("actionZoom_In_Auto", N_("Zoom in on selected object"), "/", group, false, false);
	addGuiActions("actionZoom_Out_Auto", N_("Zoom out"), "\\", group, false, false);
	addGuiActions("actionSet_Home_Planet_To_Selected", N_("Set home planet to selected planet"), "Ctrl+G", group, false, false);

	group = N_("Miscellaneous");
	addGuiActions("actionSwitch_Equatorial_Mount", N_("Switch between equatorial and azimuthal mount"), "Ctrl+M", group, true, false);
	addGuiActions("actionQuit_Global", N_("Quit"), "Ctrl+Q", group, false, false, true);
	addGuiActions("actionSave_Screenshot_Global", N_("Save screenshot"), "Ctrl+S", group, false, false, true);
	addGuiActions("actionSave_Copy_Object_Information_Global", N_("Copy selected object information to clipboard"), "Ctrl+C", group, false, false, true);
	
	addGuiActions("actionAutoHideHorizontalButtonBar", N_("Auto hide horizontal button bar"), "", group, true, false);
	addGuiActions("actionAutoHideVerticalButtonBar", N_("Auto hide vertical button bar"), "", group, true, false);
	addGuiActions("actionToggle_GuiHidden_Global", N_("Toggle visibility of GUI"), "Ctrl+T", group, true, false, true);


	///////////////////////////////////////////////////////////////////////
	// Connect all the GUI actions signals with the Core of Stellarium
	connect(getGuiActions("actionQuit_Global"), SIGNAL(triggered()), this, SLOT(quit()));

	initConstellationMgr();
	initGrindLineMgr();
	initLandscapeMgr();

	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	connect(getGuiActions("actionShow_Nebulas"), SIGNAL(toggled(bool)), nmgr, SLOT(setFlagHints(bool)));
	getGuiActions("actionShow_Nebulas")->setChecked(nmgr->getFlagHints());

	StelSkyLayerMgr* imgr = GETSTELMODULE(StelSkyLayerMgr);
	connect(getGuiActions("actionShow_DSS"), SIGNAL(toggled(bool)), imgr, SLOT(setFlagShow(bool)));
	getGuiActions("actionShow_DSS")->setChecked(imgr->getFlagShow());


	StelCore* core = StelApp::getInstance().getCore();
	StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
	connect(getGuiActions("actionIncrease_Script_Speed"), SIGNAL(triggered()), this, SLOT(increaseScriptSpeed()));
	connect(getGuiActions("actionDecrease_Script_Speed"), SIGNAL(triggered()), this, SLOT(decreaseScriptSpeed()));
	connect(getGuiActions("actionSet_Real_Script_Speed"), SIGNAL(triggered()), this, SLOT(setRealScriptSpeed()));
	connect(getGuiActions("actionStop_Script"), SIGNAL(triggered()), this, SLOT(stopScript()));
	connect(getGuiActions("actionPause_Script"), SIGNAL(triggered()), this, SLOT(pauseScript()));
	connect(getGuiActions("actionResume_Script"), SIGNAL(triggered()), this, SLOT(resumeScript()));
	connect(getGuiActions("actionIncrease_Time_Speed"), SIGNAL(triggered()), core, SLOT(increaseTimeSpeed()));
	connect(getGuiActions("actionDecrease_Time_Speed"), SIGNAL(triggered()), core, SLOT(decreaseTimeSpeed()));
	connect(getGuiActions("actionIncrease_Time_Speed_Less"), SIGNAL(triggered()), core, SLOT(increaseTimeSpeedLess()));
	connect(getGuiActions("actionDecrease_Time_Speed_Less"), SIGNAL(triggered()), core, SLOT(decreaseTimeSpeedLess()));
	connect(getGuiActions("actionSet_Real_Time_Speed"), SIGNAL(triggered()), core, SLOT(toggleRealTimeSpeed()));
	connect(getGuiActions("actionSet_Time_Rate_Zero"), SIGNAL(triggered()), core, SLOT(setZeroTimeSpeed()));
	connect(getGuiActions("actionReturn_To_Current_Time"), SIGNAL(triggered()), core, SLOT(setTimeNow()));
	connect(getGuiActions("actionSwitch_Equatorial_Mount"), SIGNAL(toggled(bool)), mmgr, SLOT(setEquatorialMount(bool)));
	getGuiActions("actionSwitch_Equatorial_Mount")->setChecked(mmgr->getMountMode() != StelMovementMgr::MountAltAzimuthal);
	connect(getGuiActions("actionAdd_Solar_Hour"), SIGNAL(triggered()), core, SLOT(addHour()));
	connect(getGuiActions("actionAdd_Solar_Day"), SIGNAL(triggered()), core, SLOT(addDay()));
	connect(getGuiActions("actionAdd_Solar_Week"), SIGNAL(triggered()), core, SLOT(addWeek()));
	connect(getGuiActions("actionSubtract_Solar_Hour"), SIGNAL(triggered()), core, SLOT(subtractHour()));
	connect(getGuiActions("actionSubtract_Solar_Day"), SIGNAL(triggered()), core, SLOT(subtractDay()));
	connect(getGuiActions("actionSubtract_Solar_Week"), SIGNAL(triggered()), core, SLOT(subtractWeek()));
	connect(getGuiActions("actionAdd_Sidereal_Day"), SIGNAL(triggered()), core, SLOT(addSiderealDay()));
	connect(getGuiActions("actionAdd_Sidereal_Week"), SIGNAL(triggered()), core, SLOT(addSiderealWeek()));
	connect(getGuiActions("actionAdd_Sidereal_Month"), SIGNAL(triggered()), core, SLOT(addSiderealMonth()));
	connect(getGuiActions("actionAdd_Sidereal_Year"), SIGNAL(triggered()), core, SLOT(addSiderealYear()));
	connect(getGuiActions("actionSubtract_Sidereal_Day"), SIGNAL(triggered()), core, SLOT(subtractSiderealDay()));
	connect(getGuiActions("actionSubtract_Sidereal_Week"), SIGNAL(triggered()), core, SLOT(subtractSiderealWeek()));
	connect(getGuiActions("actionSubtract_Sidereal_Month"), SIGNAL(triggered()), core, SLOT(subtractSiderealMonth()));
	connect(getGuiActions("actionSubtract_Sidereal_Year"), SIGNAL(triggered()), core, SLOT(subtractSiderealYear()));
	connect(getGuiActions("actionSet_Home_Planet_To_Selected"), SIGNAL(triggered()), core, SLOT(moveObserverToSelected()));

	// connect the actor after setting the nightmode.
	// StelApp::init() already set flagNightMode for us, don't do it twice!
	getGuiActions("actionShow_Night_Mode")->setChecked(StelApp::getInstance().getVisionModeNight());
	connect(getGuiActions("actionShow_Night_Mode"), SIGNAL(toggled(bool)), &StelApp::getInstance(), SLOT(setVisionModeNight(bool)));

	connect(getGuiActions("actionGoto_Selected_Object"), SIGNAL(triggered()), mmgr, SLOT(setFlagTracking()));
	connect(getGuiActions("actionZoom_In_Auto"), SIGNAL(triggered()), mmgr, SLOT(autoZoomIn()));
	connect(getGuiActions("actionZoom_Out_Auto"), SIGNAL(triggered()), mmgr, SLOT(autoZoomOut()));
	connect(getGuiActions("actionSet_Tracking"), SIGNAL(toggled(bool)), mmgr, SLOT(setFlagTracking(bool)));
	getGuiActions("actionSet_Tracking")->setChecked(mmgr->getFlagTracking());

	connect(getGuiActions("actionSet_Full_Screen_Global"), SIGNAL(toggled(bool)), &StelMainWindow::getInstance(), SLOT(setFullScreen(bool)));
	getGuiActions("actionSet_Full_Screen_Global")->setChecked(StelMainWindow::getInstance().isFullScreen());

	connect(getGuiActions("actionShow_Location_Window_Global"), SIGNAL(toggled(bool)), &locationDialog, SLOT(setVisible(bool)));
	connect(&locationDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_Location_Window_Global"), SLOT(setChecked(bool)));

#ifdef ENABLE_SCRIPT_CONSOLE
	connect(getGuiActions("actionShow_ScriptConsole_Window_Global"), SIGNAL(toggled(bool)), &scriptConsole, SLOT(setVisible(bool)));
	connect(&scriptConsole, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_ScriptConsole_Window_Global"), SLOT(setChecked(bool)));
#endif

	connect(getGuiActions("actionShow_Configuration_Window_Global"), SIGNAL(toggled(bool)), configurationDialog, SLOT(setVisible(bool)));
	connect(configurationDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_Configuration_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiActions("actionShow_SkyView_Window_Global"), SIGNAL(toggled(bool)), &viewDialog, SLOT(setVisible(bool)));
	connect(&viewDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_SkyView_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiActions("actionShow_Help_Window_Global"), SIGNAL(toggled(bool)), &helpDialog, SLOT(setVisible(bool)));
	connect(&helpDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_Help_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiActions("actionShow_DateTime_Window_Global"), SIGNAL(toggled(bool)), &dateTimeDialog, SLOT(setVisible(bool)));
	connect(&dateTimeDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_DateTime_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiActions("actionShow_Search_Window_Global"), SIGNAL(toggled(bool)), &searchDialog, SLOT(setVisible(bool)));
	connect(&searchDialog, SIGNAL(visibleChanged(bool)), getGuiActions("actionShow_Search_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiActions("actionSave_Screenshot_Global"), SIGNAL(triggered()), &StelMainGraphicsView::getInstance(), SLOT(saveScreenShot()));
	connect(getGuiActions("actionSave_Copy_Object_Information_Global"), SIGNAL(triggered()), this, SLOT(copySelectedObjectInfo()));

	getGuiActions("actionToggle_GuiHidden_Global")->setChecked(true);
	connect(getGuiActions("actionToggle_GuiHidden_Global"), SIGNAL(toggled(bool)), this, SLOT(setGuiVisible(bool)));

	connect(getGuiActions("actionHorizontal_Flip"), SIGNAL(toggled(bool)), StelApp::getInstance().getCore(), SLOT(setFlipHorz(bool)));
	getGuiActions("actionHorizontal_Flip")->setChecked(StelApp::getInstance().getCore()->getFlipHorz());
	connect(getGuiActions("actionVertical_Flip"), SIGNAL(toggled(bool)), StelApp::getInstance().getCore(), SLOT(setFlipVert(bool)));
	getGuiActions("actionVertical_Flip")->setChecked(StelApp::getInstance().getCore()->getFlipVert());

	StarMgr* smgr = GETSTELMODULE(StarMgr);
	connect(getGuiActions("actionShow_Stars"), SIGNAL(toggled(bool)), smgr, SLOT(setFlagStars(bool)));
	getGuiActions("actionShow_Stars")->setChecked(smgr->getFlagStars());

	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	connect(getGuiActions("actionShow_Planets_Labels"), SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagLabels(bool)));
	getGuiActions("actionShow_Planets_Labels")->setChecked(ssmgr->getFlagLabels());

	connect(getGuiActions("actionShow_Planets_Orbits"), SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagOrbits(bool)));
	getGuiActions("actionShow_Planets_Orbits")->setChecked(ssmgr->getFlagOrbits());

	connect(getGuiActions("actionShow_Planets_Trails"), SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagTrails(bool)));
	getGuiActions("actionShow_Planets_Trails")->setChecked(ssmgr->getFlagTrails());

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	setAutoHideHorizontalButtonBar(conf->value("gui/auto_hide_horizontal_toolbar", true).toBool());
	setAutoHideVerticalButtonBar(conf->value("gui/auto_hide_vertical_toolbar", true).toBool());
	connect(getGuiActions("actionAutoHideHorizontalButtonBar"), SIGNAL(toggled(bool)), this, SLOT(setAutoHideHorizontalButtonBar(bool)));
	getGuiActions("actionAutoHideHorizontalButtonBar")->setChecked(getAutoHideHorizontalButtonBar());
	connect(getGuiActions("actionAutoHideVerticalButtonBar"), SIGNAL(toggled(bool)), this, SLOT(setAutoHideVerticalButtonBar(bool)));
	getGuiActions("actionAutoHideVerticalButtonBar")->setChecked(getAutoHideVerticalButtonBar());

#ifndef DISABLE_SCRIPTING
	StelScriptMgr& scriptMgr = StelMainGraphicsView::getInstance().getScriptMgr();
	connect(&scriptMgr, SIGNAL(scriptRunning()), this, SLOT(scriptStarted()));
	connect(&scriptMgr, SIGNAL(scriptStopped()), this, SLOT(scriptStopped()));
#endif

	///////////////////////////////////////////////////////////////////////////
	//// QGraphicsView based GUI
	///////////////////////////////////////////////////////////////////////////

	// Add everything
	QPixmap pxmapDefault;
	QPixmap pxmapGlow(":/graphicGui/glow.png");
	QPixmap pxmapOn(":/graphicGui/2-on-location.png");
	QPixmap pxmapOff(":/graphicGui/2-off-location.png");
	StelButton*  b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Location_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/1-on-time.png");
	pxmapOff = QPixmap(":/graphicGui/1-off-time.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_DateTime_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/5-off-labels.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_SkyView_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/6-off-search.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Search_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/8-off-settings.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Configuration_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/9-off-help.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Help_Window_Global"));
	skyGui->winBar->addButton(b);

	QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");

	pxmapOn = QPixmap(":/graphicGui/btConstellationLines-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationLines-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Lines"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btConstellationLabels-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationLabels-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Labels"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btConstellationArt-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationArt-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Art"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btEquatorialGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/btEquatorialGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Equatorial_Grid"));
	skyGui->buttonBar->addButton(b, "020-gridsGroup");

	pxmapOn = QPixmap(":/graphicGui/btAzimuthalGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/btAzimuthalGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Azimuthal_Grid"));
	skyGui->buttonBar->addButton(b, "020-gridsGroup");

	pxmapOn = QPixmap(":/graphicGui/btGround-on.png");
	pxmapOff = QPixmap(":/graphicGui/btGround-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Ground"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btCardinalPoints-on.png");
	pxmapOff = QPixmap(":/graphicGui/btCardinalPoints-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Cardinal_Points"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btAtmosphere-on.png");
	pxmapOff = QPixmap(":/graphicGui/btAtmosphere-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Atmosphere"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btNebula-on.png");
	pxmapOff = QPixmap(":/graphicGui/btNebula-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Nebulas"));
	skyGui->buttonBar->addButton(b, "040-nebulaeGroup");

	pxmapOn = QPixmap(":/graphicGui/btPlanets-on.png");
	pxmapOff = QPixmap(":/graphicGui/btPlanets-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Planets_Labels"));
	skyGui->buttonBar->addButton(b, "040-nebulaeGroup");

	pxmapOn = QPixmap(":/graphicGui/btEquatorialMount-on.png");
	pxmapOff = QPixmap(":/graphicGui/btEquatorialMount-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSwitch_Equatorial_Mount"));
	b->setChecked(getGuiActions("actionSwitch_Equatorial_Mount")->isChecked());
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btGotoSelectedObject-on.png");
	pxmapOff = QPixmap(":/graphicGui/btGotoSelectedObject-off.png");
	buttonGotoSelectedObject = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionGoto_Selected_Object"));
	skyGui->buttonBar->addButton(buttonGotoSelectedObject, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btNightView-on.png");
	pxmapOff = QPixmap(":/graphicGui/btNightView-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Night_Mode"));
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btFullScreen-on.png");
	pxmapOff = QPixmap(":/graphicGui/btFullScreen-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSet_Full_Screen_Global"));
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeRewind-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeRewind-off.png");
	buttonTimeRewind = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionDecrease_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeRewind, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeRealtime-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeRealtime-off.png");
	pxmapDefault = QPixmap(":/graphicGui/btTimePause-on.png");
	buttonTimeRealTimeSpeed = new StelButton(NULL, pxmapOn, pxmapOff, pxmapDefault, pxmapGlow32x32, getGuiActions("actionSet_Real_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeRealTimeSpeed, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeNow-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeNow-off.png");
	buttonTimeCurrent = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionReturn_To_Current_Time"));
	skyGui->buttonBar->addButton(buttonTimeCurrent, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeForward-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeForward-off.png");
	buttonTimeForward = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionIncrease_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeForward, "070-timeGroup");

	skyGui->buttonBar->setGroupMargin("070-timeGroup", 32, 0);

	pxmapOn = QPixmap(":/graphicGui/btQuit.png");
	b = new StelButton(NULL, pxmapOn, pxmapOn, pxmapGlow32x32, getGuiActions("actionQuit_Global"));
	skyGui->buttonBar->addButton(b, "080-quitGroup");

	// add the flip buttons if requested in the config
	setFlagShowFlipButtons(conf->value("gui/flag_show_flip_buttons", false).toBool());
	setFlagShowNebulaBackgroundButton(conf->value("gui/flag_show_nebulae_background_button", false).toBool());

	///////////////////////////////////////////////////////////////////////
	// Create the main base widget
	skyGui->init(this);
	QGraphicsGridLayout* l = new QGraphicsGridLayout();
	l->setContentsMargins(0,0,0,0);
	l->setSpacing(0);
	l->addItem(skyGui, 0, 0);
	stelAppGraphicsWidget->setLayout(l);

	setStelStyle(StelApp::getInstance().getCurrentStelStyle());

	skyGui->setGeometry(stelAppGraphicsWidget->geometry());
	skyGui->updateBarsPos();
	
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	initDone = true;
}

void StelGui::initConstellationMgr()
{
	ConstellationMgr* constellationMgr = GETSTELMODULE(ConstellationMgr);
	getGuiActions("actionShow_Constellation_Lines")->setChecked(constellationMgr->getFlagLines());
	connect(getGuiActions("actionShow_Constellation_Lines"),
			SIGNAL(toggled(bool)),
			constellationMgr,
			SLOT(setFlagLines(bool)));
	connect(constellationMgr,
			SIGNAL(linesDisplayedChanged(const bool)),
			this,
			SLOT(linesDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Constellation_Art")->setChecked(constellationMgr->getFlagArt());
	connect(getGuiActions("actionShow_Constellation_Art"),
			SIGNAL(toggled(bool)),
			constellationMgr,
			SLOT(setFlagArt(bool)));
	connect(constellationMgr,
			SIGNAL(artDisplayedChanged(const bool)),
			this,
			SLOT(artDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Constellation_Labels")->setChecked(constellationMgr->getFlagLabels());
	connect(getGuiActions("actionShow_Constellation_Labels"),
			SIGNAL(toggled(bool)),
			constellationMgr,
			SLOT(setFlagLabels(bool)));
	connect(constellationMgr,
			SIGNAL(namesDisplayedChanged(const bool)),
			this,
			SLOT(namesDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Constellation_Boundaries")->setChecked(constellationMgr->getFlagBoundaries());
	connect(getGuiActions("actionShow_Constellation_Boundaries"),
			SIGNAL(toggled(bool)),
			constellationMgr,
			SLOT(setFlagBoundaries(bool)));
	connect(constellationMgr,
			SIGNAL(boundariesDisplayedChanged(const bool)),
			this,
			SLOT(boundariesDisplayedUpdated(const bool)));
}

void StelGui::initGrindLineMgr()
{
	GridLinesMgr* gridLineManager = GETSTELMODULE(GridLinesMgr);
	getGuiActions("actionShow_Equatorial_Grid")->setChecked(gridLineManager->getFlagEquatorGrid());
	connect(getGuiActions("actionShow_Equatorial_Grid"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagEquatorGrid(bool)));
	connect(gridLineManager,
			SIGNAL(equatorGridDisplayedChanged(const bool)),
			this,
			SLOT(equatorGridDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Azimuthal_Grid")->setChecked(gridLineManager->getFlagAzimuthalGrid());
	connect(getGuiActions("actionShow_Azimuthal_Grid"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagAzimuthalGrid(bool)));
	connect(gridLineManager,
			SIGNAL(azimuthalGridDisplayedChanged(const bool)),
			this,
			SLOT(azimuthalGridDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Ecliptic_Line")->setChecked(gridLineManager->getFlagEclipticLine());
	connect(getGuiActions("actionShow_Ecliptic_Line"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagEclipticLine(bool)));
	connect(gridLineManager,
			SIGNAL(eclipticLineDisplayedChanged(const bool)),
			this,
			SLOT(eclipticLineDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Equator_Line")->setChecked(gridLineManager->getFlagEquatorLine());
	connect(getGuiActions("actionShow_Equator_Line"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagEquatorLine(bool)));
	connect(gridLineManager,
			SIGNAL(equatorLineDisplayedChanged(const bool)),
			this,
			SLOT(equatorLineDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Meridian_Line")->setChecked(gridLineManager->getFlagMeridianLine());
	connect(getGuiActions("actionShow_Meridian_Line"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagMeridianLine(bool)));
	connect(gridLineManager,
			SIGNAL(meridianLineDisplayedChanged(const bool)),
			this,
			SLOT(meridianLineDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Horizon_Line")->setChecked(gridLineManager->getFlagHorizonLine());
	connect(getGuiActions("actionShow_Horizon_Line"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagHorizonLine(bool)));
	connect(gridLineManager,
			SIGNAL(horizonLineDisplayedChanged(const bool)),
			this,
			SLOT(horizonLineDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Equatorial_J2000_Grid")->setChecked(gridLineManager->getFlagEquatorJ2000Grid());
	connect(getGuiActions("actionShow_Equatorial_J2000_Grid"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagEquatorJ2000Grid(bool)));
	connect(gridLineManager,
			SIGNAL(equatorJ2000GridDisplayedChanged(const bool)),
			this,
			SLOT(equatorJ2000GridDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Ecliptic_J2000_Grid")->setChecked(gridLineManager->getFlagEclipticJ2000Grid());
	connect(getGuiActions("actionShow_Ecliptic_J2000_Grid"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagEclipticJ2000Grid(bool)));
	connect(gridLineManager,
			SIGNAL(eclipticJ2000GridDisplayedChanged(const bool)),
			this,
			SLOT(eclipticJ2000GridDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Galactic_Grid")->setChecked(gridLineManager->getFlagGalacticGrid());
	connect(getGuiActions("actionShow_Galactic_Grid"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagGalacticGrid(bool)));
	connect(gridLineManager,
			SIGNAL(galacticGridDisplayedChanged(const bool)),
			this,
			SLOT(galacticGridDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Galactic_Plane_Line")->setChecked(gridLineManager->getFlagGalacticPlaneLine());
	connect(getGuiActions("actionShow_Galactic_Plane_Line"),
			SIGNAL(toggled(bool)),
			gridLineManager,
			SLOT(setFlagGalacticPlaneLine(bool)));
	connect(gridLineManager,
			SIGNAL(galacticPlaneLineDisplayedChanged(const bool)),
			this,
			SLOT(galacticPlaneLineDisplayedUpdated(const bool)));

}

void StelGui::initLandscapeMgr()
{
	LandscapeMgr* landscapeMgr = GETSTELMODULE(LandscapeMgr);
	getGuiActions("actionShow_Ground")->setChecked(landscapeMgr->getFlagLandscape());
	connect(getGuiActions("actionShow_Ground"),
			SIGNAL(toggled(bool)),
			landscapeMgr,
			SLOT(setFlagLandscape(bool)));
	connect(landscapeMgr,
			SIGNAL(landscapeDisplayedChanged(const bool)),
			this,
			SLOT(landscapeDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Cardinal_Points")->setChecked(landscapeMgr->getFlagCardinalsPoints());
	connect(getGuiActions("actionShow_Cardinal_Points"),
			SIGNAL(toggled(bool)),
			landscapeMgr,
			SLOT(setFlagCardinalsPoints(bool)));
	connect(landscapeMgr,
			SIGNAL(cardinalsPointsDisplayedChanged(const bool)),
			this,
			SLOT(cardinalsPointsDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Atmosphere")->setChecked(landscapeMgr->getFlagAtmosphere());
	connect(getGuiActions("actionShow_Atmosphere"),
			SIGNAL(toggled(bool)),
			landscapeMgr,
			SLOT(setFlagAtmosphere(bool)));
	connect(landscapeMgr,
			SIGNAL(atmosphereDisplayedChanged(const bool)),
			this,
			SLOT(atmosphereDisplayedUpdated(const bool)));

	getGuiActions("actionShow_Fog")->setChecked(landscapeMgr->getFlagFog());
	connect(getGuiActions("actionShow_Fog"),
			SIGNAL(toggled(bool)),
			landscapeMgr,
			SLOT(setFlagFog(bool)));
	connect(landscapeMgr,
			SIGNAL(fogDisplayedChanged(const bool)),
			this,
			SLOT(fogDisplayedUpdated(const bool)));
}

void StelGui::quit()
{
	#ifndef DISABLE_SCRIPTING
	StelMainGraphicsView::getInstance().getScriptMgr().stopScript();
	#endif
	QCoreApplication::exit();
}

//! Reload the current Qt Style Sheet (Debug only)
void StelGui::reloadStyle()
{
	setStelStyle(StelApp::getInstance().getCurrentStelStyle());
}

//! Load color scheme from the given ini file and section name
void StelGui::setStelStyle(const QString& section)
{
	if (currentStelStyle.confSectionName!=section)
	{
		// Load the style sheets
		currentStelStyle.confSectionName = section;

		QString qtStyleFileName;
		QString htmlStyleFileName;

		if (section=="night_color")
		{
			qtStyleFileName = ":/graphicGui/nightStyle.css";
			htmlStyleFileName = ":/graphicGui/nightHtml.css";
		}
		else if (section=="color")
		{
			qtStyleFileName = ":/graphicGui/normalStyle.css";
			htmlStyleFileName = ":/graphicGui/normalHtml.css";
		}

		// Load Qt style sheet
		QFile styleFile(qtStyleFileName);
		styleFile.open(QIODevice::ReadOnly);
		currentStelStyle.qtStyleSheet = styleFile.readAll();

		QFile htmlStyleFile(htmlStyleFileName);
		htmlStyleFile.open(QIODevice::ReadOnly);
		currentStelStyle.htmlStyleSheet = htmlStyleFile.readAll();
	}
	qApp->setStyleSheet(currentStelStyle.qtStyleSheet);

	locationDialog.styleChanged();
	dateTimeDialog.styleChanged();
	configurationDialog->styleChanged();
	searchDialog.styleChanged();
	viewDialog.styleChanged();
#ifdef ENABLE_SCRIPT_CONSOLE
	scriptConsole.styleChanged();
#endif // ENABLE_SCRIPT_CONSOLE
}


void StelGui::updateI18n()
{
	StelGuiBase::updateI18n();
}

void StelGui::update()
{
	StelCore* core = StelApp::getInstance().getCore();
	if (core->getTimeRate()<-0.99*StelCore::JD_SECOND) {
		if (buttonTimeRewind->isChecked()==false)
			buttonTimeRewind->setChecked(true);
	} else {
		if (buttonTimeRewind->isChecked()==true)
			buttonTimeRewind->setChecked(false);
	}
	if (core->getTimeRate()>1.01*StelCore::JD_SECOND) {
		if (buttonTimeForward->isChecked()==false) {
			buttonTimeForward->setChecked(true);
		}
	} else {
		if (buttonTimeForward->isChecked()==true)
			buttonTimeForward->setChecked(false);
	}
	if (core->getTimeRate() == 0) {
		if (buttonTimeRealTimeSpeed->isChecked() != StelButton::ButtonStateNoChange)
			buttonTimeRealTimeSpeed->setChecked(StelButton::ButtonStateNoChange);
	} else if (core->getRealTimeSpeed()) {
		if (buttonTimeRealTimeSpeed->isChecked() != StelButton::ButtonStateOn)
			buttonTimeRealTimeSpeed->setChecked(StelButton::ButtonStateOn);
	} else if (buttonTimeRealTimeSpeed->isChecked() != StelButton::ButtonStateOff) {
		buttonTimeRealTimeSpeed->setChecked(StelButton::ButtonStateOff);
	}
	const bool isTimeNow=core->getIsTimeNow();
	if (buttonTimeCurrent->isChecked()!=isTimeNow) {
		buttonTimeCurrent->setChecked(isTimeNow);
	}
	StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
	const bool b = mmgr->getFlagTracking();
	if (buttonGotoSelectedObject->isChecked()!=b) {
		buttonGotoSelectedObject->setChecked(b);
	}

	bool flag = GETSTELMODULE(StarMgr)->getFlagStars();
	if (getGuiActions("actionShow_Stars")->isChecked() != flag) {
		getGuiActions("actionShow_Stars")->setChecked(flag);
	}

	flag = GETSTELMODULE(NebulaMgr)->getFlagHints();
	if (getGuiActions("actionShow_Nebulas")->isChecked() != flag)
		getGuiActions("actionShow_Nebulas")->setChecked(flag);

	flag = GETSTELMODULE(StelSkyLayerMgr)->getFlagShow();
	if (getGuiActions("actionShow_DSS")->isChecked() != flag)
		getGuiActions("actionShow_DSS")->setChecked(flag);

	flag = mmgr->getMountMode() != StelMovementMgr::MountAltAzimuthal;
	if (getGuiActions("actionSwitch_Equatorial_Mount")->isChecked() != flag)
		getGuiActions("actionSwitch_Equatorial_Mount")->setChecked(flag);

	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	flag = ssmgr->getFlagLabels();
	if (getGuiActions("actionShow_Planets_Labels")->isChecked() != flag)
		getGuiActions("actionShow_Planets_Labels")->setChecked(flag);
	flag = ssmgr->getFlagOrbits();
	if (getGuiActions("actionShow_Planets_Orbits")->isChecked() != flag)
		getGuiActions("actionShow_Planets_Orbits")->setChecked(flag);
	flag = ssmgr->getFlagTrails();
	if (getGuiActions("actionShow_Planets_Trails")->isChecked() != flag)
		getGuiActions("actionShow_Planets_Trails")->setChecked(flag);
	flag = StelApp::getInstance().getVisionModeNight();
	if (getGuiActions("actionShow_Night_Mode")->isChecked() != flag)
		getGuiActions("actionShow_Night_Mode")->setChecked(flag);
	flag = StelMainWindow::getInstance().isFullScreen();
	if (getGuiActions("actionSet_Full_Screen_Global")->isChecked() != flag)
		getGuiActions("actionSet_Full_Screen_Global")->setChecked(flag);

	skyGui->infoPanel->setTextFromObjects(GETSTELMODULE(StelObjectMgr)->getSelectedObject());

	// Check if the progressbar window changed, if yes update the whole view
	if (savedProgressBarSize!=skyGui->progressBarMgr->boundingRect().size())
	{
		savedProgressBarSize=skyGui->progressBarMgr->boundingRect().size();
		skyGui->updateBarsPos();
	}

	dateTimeDialog.setDateTime(core->getJDay());
}

// Add a new progress bar in the lower right corner of the screen.
QProgressBar* StelGui::addProgressBar()
{
	return skyGui->progressBarMgr->addProgressBar();
}

#ifndef DISABLE_SCRIPTING
void StelGui::setScriptKeys(bool b)
{
	if (b)
	{
		getGuiActions("actionDecrease_Time_Speed")->setShortcut(QKeySequence());
		getGuiActions("actionIncrease_Time_Speed")->setShortcut(QKeySequence());
		getGuiActions("actionSet_Real_Time_Speed")->setShortcut(QKeySequence());
		getGuiActions("actionDecrease_Script_Speed")->setShortcut(QKeySequence("J"));
		getGuiActions("actionIncrease_Script_Speed")->setShortcut(QKeySequence("L"));
		getGuiActions("actionSet_Real_Script_Speed")->setShortcut(QKeySequence("K"));

		getGuiActions("actionStop_Script")->setShortcut(QKeySequence("4"));
		getGuiActions("actionPause_Script")->setShortcut(QKeySequence("5"));
		getGuiActions("actionResume_Script")->setShortcut(QKeySequence("6"));
	}
	else
	{
		getGuiActions("actionDecrease_Script_Speed")->setShortcut(QKeySequence());
		getGuiActions("actionIncrease_Script_Speed")->setShortcut(QKeySequence());
		getGuiActions("actionSet_Real_Script_Speed")->setShortcut(QKeySequence());
		getGuiActions("actionDecrease_Time_Speed")->setShortcut(QKeySequence("J"));
		getGuiActions("actionIncrease_Time_Speed")->setShortcut(QKeySequence("L"));
		getGuiActions("actionSet_Real_Time_Speed")->setShortcut(QKeySequence("K"));
	}
}

void StelGui::increaseScriptSpeed()
{
	StelMainGraphicsView::getInstance().getScriptMgr().setScriptRate(StelMainGraphicsView::getInstance().getScriptMgr().getScriptRate()*2);
}

void StelGui::decreaseScriptSpeed()
{	
	StelMainGraphicsView::getInstance().getScriptMgr().setScriptRate(StelMainGraphicsView::getInstance().getScriptMgr().getScriptRate()/2);
}

void StelGui::setRealScriptSpeed()
{	
	StelMainGraphicsView::getInstance().getScriptMgr().setScriptRate(1);	
}

void StelGui::stopScript()
{	
	StelMainGraphicsView::getInstance().getScriptMgr().stopScript();	
}

void StelGui::pauseScript()
{	
	StelMainGraphicsView::getInstance().getScriptMgr().pauseScript();	
}

void StelGui::resumeScript()
{	
	StelMainGraphicsView::getInstance().getScriptMgr().resumeScript();	
}
#endif

void StelGui::setFlagShowFlipButtons(bool b)
{
	if (b==true) {
		if (flipVert==NULL) {
			// Create the vertical flip button
			QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");
			flipVert = new StelButton(NULL,
									  QPixmap(":/graphicGui/btFlipVertical-on.png"),
									  QPixmap(":/graphicGui/btFlipVertical-off.png"),
									  pxmapGlow32x32,
									  getGuiActions("actionVertical_Flip"));
		}
		if (flipHoriz==NULL) {
			QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");
			flipHoriz = new StelButton(NULL,
									   QPixmap(":/graphicGui/btFlipHorizontal-on.png"),
									   QPixmap(":/graphicGui/btFlipHorizontal-off.png"),
									   pxmapGlow32x32,
									   getGuiActions("actionHorizontal_Flip"));
		}
		getButtonBar()->addButton(flipVert, "060-othersGroup", "actionQuit_Global");
		getButtonBar()->addButton(flipHoriz, "060-othersGroup", "actionVertical_Flip");
	} else {
		getButtonBar()->hideButton("actionVertical_Flip");
		getButtonBar()->hideButton("actionHorizontal_Flip");
	}
	flagShowFlipButtons = b;
	if (initDone) {
		skyGui->updateBarsPos();
	}
}


// Define whether the button toggling nebulae backround images should be visible
void StelGui::setFlagShowNebulaBackgroundButton(bool b)
{
	if (b==true) {
		if (btShowNebulaeBackground==NULL) {
			// Create the nebulae background button
			QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");
			btShowNebulaeBackground = new StelButton(NULL, QPixmap(":/graphicGui/btDSS-on.png"), QPixmap(":/graphicGui/btDSS-off.png"), pxmapGlow32x32, getGuiActions("actionShow_DSS"));
		}
		getButtonBar()->addButton(btShowNebulaeBackground, "040-nebulaeGroup");
	} else {
		getButtonBar()->hideButton("actionShow_DSS");
	}
	flagShowNebulaBackgroundButton = b;
}

void StelGui::setVisible(bool b)
{
	skyGui->setVisible(b);
}

bool StelGui::getVisible() const
{
	return skyGui->isVisible();
}

bool StelGui::isCurrentlyUsed() const
{
	return skyGui->buttonBar->isUnderMouse() || skyGui->winBar->isUnderMouse();
}

void setScriptKeys()
{
}

void setNormalKeys()
{
}

void StelGui::setInfoTextFilters(const StelObject::InfoStringGroup& aflags)
{
	skyGui->infoPanel->setInfoTextFilters(aflags);
}

const StelObject::InfoStringGroup& StelGui::getInfoTextFilters() const
{
	return skyGui->infoPanel->getInfoTextFilters();
}

BottomStelBar* StelGui::getButtonBar() const
{
	return skyGui->buttonBar;
}

LeftStelBar* StelGui::getWindowsButtonBar() const
{
	return skyGui->winBar;
}

SkyGui* StelGui::getSkyGui() const
{
	return skyGui;
}

bool StelGui::getAutoHideHorizontalButtonBar() const
{
	return skyGui->autoHideHorizontalButtonBar;
}

void StelGui::setAutoHideHorizontalButtonBar(bool b)
{
	skyGui->autoHideHorizontalButtonBar=b;
}

bool StelGui::getAutoHideVerticalButtonBar() const
{
	return skyGui->autoHideVerticalButtonBar;
}

void StelGui::setAutoHideVerticalButtonBar(bool b)
{
	skyGui->autoHideVerticalButtonBar=b;
}

bool StelGui::getFlagShowFlipButtons() const
{
	return flagShowFlipButtons;
}

bool StelGui::getFlagShowNebulaBackgroundButton() const
{
	return flagShowNebulaBackgroundButton;
}

bool StelGui::initComplete(void) const
{
	return initDone;
}

void StelGui::forceRefreshGui()
{
  skyGui->updateBarsPos();
}

#ifndef DISABLE_SCRIPTING
void StelGui::scriptStarted()
{
	setScriptKeys(true);
}

void StelGui::scriptStopped()
{
	setScriptKeys(false);
}
#endif

void StelGui::setGuiVisible(bool b)
{
	setVisible(b);
}

QAction* StelGui::addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable, bool autoRepeat, bool global)
{
	if (!shortCut.isEmpty())
		helpDialog.setKey(helpGroup, "", shortCut, text);
	return StelGuiBase::addGuiActions(actionName, text, shortCut, helpGroup, checkable, autoRepeat, global);
}

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Process changes from the ConstellationMgr
#endif
/* ****************************************************************************************************************** */
void StelGui::artDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Constellation_Art")->isChecked() != displayed) {
		getGuiActions("actionShow_Constellation_Art")->setChecked(displayed);
	}
}
void StelGui::boundariesDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Constellation_Boundaries")->isChecked() != displayed) {
		getGuiActions("actionShow_Constellation_Boundaries")->setChecked(displayed);
	}
}
void StelGui::linesDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Constellation_Lines")->isChecked() != displayed) {
		getGuiActions("actionShow_Constellation_Lines")->setChecked(displayed);
	}
}
void StelGui::namesDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Constellation_Labels")->isChecked() != displayed) {
		getGuiActions("actionShow_Constellation_Labels")->setChecked(displayed);
	}
}
/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Process changes from the GridLinesMgr
#endif
/* ****************************************************************************************************************** */
void StelGui::azimuthalGridDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Azimuthal_Grid")->isChecked() != displayed) {
		getGuiActions("actionShow_Azimuthal_Grid")->setChecked(displayed);
	}
}

void StelGui::equatorGridDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Equatorial_Grid")->isChecked() != displayed) {
		getGuiActions("actionShow_Equatorial_Grid")->setChecked(displayed);
	}
}

void StelGui::equatorJ2000GridDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Equatorial_J2000_Grid")->isChecked() != displayed) {
		getGuiActions("actionShow_Equatorial_J2000_Grid")->setChecked(displayed);
	}
}

void StelGui::eclipticJ2000GridDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Ecliptic_J2000_Grid")->isChecked() != displayed) {
		getGuiActions("actionShow_Ecliptic_J2000_Grid")->setChecked(displayed);
	}
}


void StelGui::galacticGridDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Galactic_Grid")->isChecked() != displayed) {
		getGuiActions("actionShow_Galactic_Grid")->setChecked(displayed);
	}
}

void StelGui::equatorLineDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Equator_Line")->isChecked() != displayed) {
		getGuiActions("actionShow_Equator_Line")->setChecked(displayed);
	}
}

void StelGui::eclipticLineDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Ecliptic_Line")->isChecked() != displayed) {
		getGuiActions("actionShow_Ecliptic_Line")->setChecked(displayed);
	}
}

void StelGui::meridianLineDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Meridian_Line")->isChecked() != displayed) {
		getGuiActions("actionShow_Meridian_Line")->setChecked(displayed);
	}
}

void StelGui::horizonLineDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Horizon_Line")->isChecked() != displayed) {
		getGuiActions("actionShow_Horizon_Line")->setChecked(displayed);
	}
}

void StelGui::galacticPlaneLineDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Galactic_Plane_Line")->isChecked() != displayed) {
		getGuiActions("actionShow_Galactic_Plane_Line")->setChecked(displayed);
	}

}

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Process changes from the GridLinesMgr
#endif
/* ****************************************************************************************************************** */
void StelGui::atmosphereDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Atmosphere")->isChecked() != displayed) {
		getGuiActions("actionShow_Atmosphere")->setChecked(displayed);
	}
}

void StelGui::cardinalsPointsDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Cardinal_Points")->isChecked() != displayed) {
		getGuiActions("actionShow_Cardinal_Points")->setChecked(displayed);
	}
}

void StelGui::fogDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Fog")->isChecked() != displayed) {
		getGuiActions("actionShow_Fog")->setChecked(displayed);
	}
}

void StelGui::landscapeDisplayedUpdated(const bool displayed)
{
	if (getGuiActions("actionShow_Ground")->isChecked() != displayed) {
		getGuiActions("actionShow_Ground")->setChecked(displayed);
	}
}

void StelGui::copySelectedObjectInfo(void)
{
	QApplication::clipboard()->setText(skyGui->infoPanel->getSelectedText());
}
