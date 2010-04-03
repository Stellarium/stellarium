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
#include "TelescopeMgr.hpp"
#include "StarMgr.hpp"
#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "NebulaMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelNavigator.hpp"
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
#include "StelScriptMgr.hpp"
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

StelGuiBase* StelStandardGuiPluginInterface::getStelGuiBase() const
{
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
}

void StelGui::init(QGraphicsWidget* atopLevelGraphicsWidget, StelAppGraphicsWidget* astelAppGraphicsWidget)
{
	qDebug() << "Creating GUI ...";

	StelGuiBase::init(atopLevelGraphicsWidget, astelAppGraphicsWidget);

	skyGui = new SkyGui();
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
	addGuiActions("actionShow_Galactic_Grid", N_("Galactic grid"), "", group, true, false);
	addGuiActions("actionShow_Ecliptic_Line", N_("Ecliptic line"), ",", group, true, false);
	addGuiActions("actionShow_Equator_Line", N_("Equator line"), ".", group, true, false);
	addGuiActions("actionShow_Meridian_Line", N_("Meridian line"), ";", group, true, false);
	addGuiActions("actionShow_Cardinal_Points", N_("Cardinal points"), "Q", group, true, false);

	addGuiActions("actionShow_Ground", N_("Ground"), "G", group, true, false);
	addGuiActions("actionShow_Atmosphere", N_("Atmosphere"), "A", group, true, false);
	addGuiActions("actionShow_Fog", N_("Fog"), "F", group, true, false);

	addGuiActions("actionShow_Nebulas", N_("Nebulas"), "N", group, true, false);
	addGuiActions("actionShow_DSS", N_("Nebulas background images"), "", group, true, false);
	addGuiActions("actionShow_Stars", N_("Stars"), "S", group, true, false);
	addGuiActions("actionShow_Planets_Labels", N_("Planets labels"), "P", group, true, false);
	addGuiActions("actionShow_Planets_Orbits", N_("Planet orbits"), "O", group, true, false);
	addGuiActions("actionShow_Planets_Trails", N_("Planet trails"), "Shift+T", group, true, false);

	addGuiActions("actionShow_Night_Mode", N_("Night mode"), "", group, true, false);
	addGuiActions("actionSet_Full_Screen_Global", N_("Full-screen mode"), "F11", group, true, false);
	addGuiActions("actionHorizontal_Flip", N_("Flip scene horizontally"), "Ctrl+Shift+H", group, true, false);
	addGuiActions("actionVertical_Flip", N_("Flip scene vertically"), "Ctrl+Shift+V", group, true, false);

	group = N_("Windows");
	addGuiActions("actionShow_Help_Window_Global", N_("Help window"), "F1", group, true, false);
	addGuiActions("actionShow_Configuration_Window_Global", N_("Configuration window"), "F2", group, true, false);
	addGuiActions("actionShow_Search_Window_Global", N_("Search window"), "F3, Ctrl+F", group, true, false);
	addGuiActions("actionShow_SkyView_Window_Global", N_("Sky and viewing options window"), "F4", group, true, false);
	addGuiActions("actionShow_DateTime_Window_Global", N_("Date/time window"), "F5", group, true, false);
	addGuiActions("actionShow_Location_Window_Global", N_("Location window"), "F6", group, true, false);
#ifdef ENABLE_SCRIPT_CONSOLE
	addGuiActions("actionShow_ScriptConsole_Window_Global", N_("Script console window"), "F12", group, true, false);
#endif

	group = N_("Date and Time");
	addGuiActions("actionDecrease_Script_Speed", N_("Slow down the script execution rate"), "", group, false, false);
	addGuiActions("actionIncrease_Script_Speed", N_("Speed up the script execution rate"), "", group, false, false);
	addGuiActions("actionSet_Real_Script_Speed", N_("Set the normal script execution rate"), "", group, false, false);
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
	addGuiActions("actionToggle_GuiHidden_Global", N_("Toggle visibility of GUI"), "Ctrl+T", group, true, false);


	///////////////////////////////////////////////////////////////////////
	// Connect all the GUI actions signals with the Core of Stellarium
	connect(getGuiActions("actionQuit_Global"), SIGNAL(triggered()), qApp, SLOT(quit()));

	// Debug
	connect(getGuiActions("action_Reload_Style"), SIGNAL(triggered()), this, SLOT(reloadStyle()));

	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
	connect(getGuiActions("actionShow_Constellation_Lines"), SIGNAL(toggled(bool)), cmgr, SLOT(setFlagLines(bool)));
	getGuiActions("actionShow_Constellation_Lines")->setChecked(cmgr->getFlagLines());
	connect(getGuiActions("actionShow_Constellation_Art"), SIGNAL(toggled(bool)), cmgr, SLOT(setFlagArt(bool)));
	getGuiActions("actionShow_Constellation_Art")->setChecked(cmgr->getFlagArt());
	connect(getGuiActions("actionShow_Constellation_Labels"), SIGNAL(toggled(bool)), cmgr, SLOT(setFlagLabels(bool)));
	getGuiActions("actionShow_Constellation_Labels")->setChecked(cmgr->getFlagLabels());
	connect(getGuiActions("actionShow_Constellation_Boundaries"), SIGNAL(toggled(bool)), cmgr, SLOT(setFlagBoundaries(bool)));
	getGuiActions("actionShow_Constellation_Boundaries")->setChecked(cmgr->getFlagBoundaries());

	GridLinesMgr* gmgr = GETSTELMODULE(GridLinesMgr);
	connect(getGuiActions("actionShow_Equatorial_Grid"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagEquatorGrid(bool)));
	getGuiActions("actionShow_Equatorial_Grid")->setChecked(gmgr->getFlagEquatorGrid());
	connect(getGuiActions("actionShow_Azimuthal_Grid"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagAzimuthalGrid(bool)));
	getGuiActions("actionShow_Azimuthal_Grid")->setChecked(gmgr->getFlagAzimuthalGrid());
	connect(getGuiActions("actionShow_Ecliptic_Line"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagEclipticLine(bool)));
	getGuiActions("actionShow_Ecliptic_Line")->setChecked(gmgr->getFlagEclipticLine());
	connect(getGuiActions("actionShow_Equator_Line"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagEquatorLine(bool)));
	getGuiActions("actionShow_Equator_Line")->setChecked(gmgr->getFlagEquatorLine());

	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	connect(getGuiActions("actionShow_Ground"), SIGNAL(toggled(bool)), lmgr, SLOT(setFlagLandscape(bool)));
	getGuiActions("actionShow_Ground")->setChecked(lmgr->getFlagLandscape());
	connect(getGuiActions("actionShow_Cardinal_Points"), SIGNAL(toggled(bool)), lmgr, SLOT(setFlagCardinalsPoints(bool)));
	getGuiActions("actionShow_Cardinal_Points")->setChecked(lmgr->getFlagCardinalsPoints());
	connect(getGuiActions("actionShow_Atmosphere"), SIGNAL(toggled(bool)), lmgr, SLOT(setFlagAtmosphere(bool)));
	getGuiActions("actionShow_Atmosphere")->setChecked(lmgr->getFlagAtmosphere());
	connect(getGuiActions("actionShow_Fog"), SIGNAL(toggled(bool)), lmgr, SLOT(setFlagFog(bool)));
	getGuiActions("actionShow_Fog")->setChecked(lmgr->getFlagFog());

	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	connect(getGuiActions("actionShow_Nebulas"), SIGNAL(toggled(bool)), nmgr, SLOT(setFlagHints(bool)));
	getGuiActions("actionShow_Nebulas")->setChecked(nmgr->getFlagHints());

	StelSkyLayerMgr* imgr = GETSTELMODULE(StelSkyLayerMgr);
	connect(getGuiActions("actionShow_DSS"), SIGNAL(toggled(bool)), imgr, SLOT(setFlagShow(bool)));
	getGuiActions("actionShow_DSS")->setChecked(imgr->getFlagShow());

	StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
	connect(getGuiActions("actionIncrease_Script_Speed"), SIGNAL(triggered()), this, SLOT(increaseScriptSpeed()));
	connect(getGuiActions("actionDecrease_Script_Speed"), SIGNAL(triggered()), this, SLOT(decreaseScriptSpeed()));
	connect(getGuiActions("actionSet_Real_Script_Speed"), SIGNAL(triggered()), this, SLOT(setRealScriptSpeed()));
	connect(getGuiActions("actionIncrease_Time_Speed"), SIGNAL(triggered()), nav, SLOT(increaseTimeSpeed()));
	connect(getGuiActions("actionDecrease_Time_Speed"), SIGNAL(triggered()), nav, SLOT(decreaseTimeSpeed()));
	connect(getGuiActions("actionIncrease_Time_Speed_Less"), SIGNAL(triggered()), nav, SLOT(increaseTimeSpeedLess()));
	connect(getGuiActions("actionDecrease_Time_Speed_Less"), SIGNAL(triggered()), nav, SLOT(decreaseTimeSpeedLess()));
	connect(getGuiActions("actionSet_Real_Time_Speed"), SIGNAL(triggered()), nav, SLOT(toggleRealTimeSpeed()));
	connect(getGuiActions("actionSet_Time_Rate_Zero"), SIGNAL(triggered()), nav, SLOT(setZeroTimeSpeed()));
	connect(getGuiActions("actionReturn_To_Current_Time"), SIGNAL(triggered()), nav, SLOT(setTimeNow()));
	connect(getGuiActions("actionSwitch_Equatorial_Mount"), SIGNAL(toggled(bool)), mmgr, SLOT(setEquatorialMount(bool)));
	getGuiActions("actionSwitch_Equatorial_Mount")->setChecked(mmgr->getMountMode() != StelMovementMgr::MountAltAzimuthal);
	connect(getGuiActions("actionAdd_Solar_Hour"), SIGNAL(triggered()), nav, SLOT(addHour()));
	connect(getGuiActions("actionAdd_Solar_Day"), SIGNAL(triggered()), nav, SLOT(addDay()));
	connect(getGuiActions("actionAdd_Solar_Week"), SIGNAL(triggered()), nav, SLOT(addWeek()));
	connect(getGuiActions("actionSubtract_Solar_Hour"), SIGNAL(triggered()), nav, SLOT(subtractHour()));
	connect(getGuiActions("actionSubtract_Solar_Day"), SIGNAL(triggered()), nav, SLOT(subtractDay()));
	connect(getGuiActions("actionSubtract_Solar_Week"), SIGNAL(triggered()), nav, SLOT(subtractWeek()));
	connect(getGuiActions("actionAdd_Sidereal_Day"), SIGNAL(triggered()), nav, SLOT(addSiderealDay()));
	connect(getGuiActions("actionAdd_Sidereal_Week"), SIGNAL(triggered()), nav, SLOT(addSiderealWeek()));
	connect(getGuiActions("actionSubtract_Sidereal_Day"), SIGNAL(triggered()), nav, SLOT(subtractSiderealDay()));
	connect(getGuiActions("actionSubtract_Sidereal_Week"), SIGNAL(triggered()), nav, SLOT(subtractSiderealWeek()));
	connect(getGuiActions("actionSet_Home_Planet_To_Selected"), SIGNAL(triggered()), nav, SLOT(moveObserverToSelected()));

	TelescopeMgr* tmgr = GETSTELMODULE(TelescopeMgr);
	connect(getGuiActions("actionMove_Telescope_To_Selection_0"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_1"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_2"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_3"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_4"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_5"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_6"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_7"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_8"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));
	connect(getGuiActions("actionMove_Telescope_To_Selection_9"), SIGNAL(triggered()), tmgr, SLOT(moveTelescopeToSelected()));

	connect(getGuiActions("actionShow_Night_Mode"), SIGNAL(toggled(bool)), &StelApp::getInstance(), SLOT(setVisionModeNight(bool)));
	getGuiActions("actionShow_Night_Mode")->setChecked(StelApp::getInstance().getVisionModeNight());

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

	connect(getGuiActions("actionShow_Meridian_Line"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagMeridianLine(bool)));
	getGuiActions("actionShow_Meridian_Line")->setChecked(gmgr->getFlagMeridianLine());

	connect(getGuiActions("actionShow_Equatorial_J2000_Grid"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagEquatorJ2000Grid(bool)));
	getGuiActions("actionShow_Equatorial_J2000_Grid")->setChecked(gmgr->getFlagEquatorJ2000Grid());

	connect(getGuiActions("actionShow_Galactic_Grid"), SIGNAL(toggled(bool)), gmgr, SLOT(setFlagGalacticGrid(bool)));
	getGuiActions("actionShow_Galactic_Grid")->setChecked(gmgr->getFlagGalacticGrid());

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	setAutoHideHorizontalButtonBar(conf->value("gui/auto_hide_horizontal_toolbar", true).toBool());
	setAutoHideVerticalButtonBar(conf->value("gui/auto_hide_vertical_toolbar", true).toBool());
	connect(getGuiActions("actionAutoHideHorizontalButtonBar"), SIGNAL(toggled(bool)), this, SLOT(setAutoHideHorizontalButtonBar(bool)));
	getGuiActions("actionAutoHideHorizontalButtonBar")->setChecked(getAutoHideHorizontalButtonBar());
	connect(getGuiActions("actionAutoHideVerticalButtonBar"), SIGNAL(toggled(bool)), this, SLOT(setAutoHideVerticalButtonBar(bool)));
	getGuiActions("actionAutoHideVerticalButtonBar")->setChecked(getAutoHideVerticalButtonBar());

	StelScriptMgr& scriptMgr = StelMainGraphicsView::getInstance().getScriptMgr();
	connect(&scriptMgr, SIGNAL(scriptRunning()), this, SLOT(scriptStarted()));
	connect(&scriptMgr, SIGNAL(scriptStopped()), this, SLOT(scriptStopped()));

	///////////////////////////////////////////////////////////////////////////
	//// QGraphicsView based GUI
	///////////////////////////////////////////////////////////////////////////

	// Add everything
	QPixmap pxmapDefault;
	QPixmap pxmapGlow(":/graphicGui/gui/glow.png");
	QPixmap pxmapOn(":/graphicGui/gui/2-on-location.png");
	QPixmap pxmapOff(":/graphicGui/gui/2-off-location.png");
	StelButton*  b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Location_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/gui/1-on-time.png");
	pxmapOff = QPixmap(":/graphicGui/gui/1-off-time.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_DateTime_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/gui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/gui/5-off-labels.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_SkyView_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/gui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/gui/6-off-search.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Search_Window_Global"));
	skyGui->winBar->addButton(b);


	pxmapOn = QPixmap(":/graphicGui/gui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/gui/8-off-settings.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Configuration_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/gui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/gui/9-off-help.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiActions("actionShow_Help_Window_Global"));
	skyGui->winBar->addButton(b);


	QPixmap pxmapGlow32x32(":/graphicGui/gui/glow32x32.png");

	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLines-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLines-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Lines"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationLabels-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationLabels-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Labels"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btConstellationArt-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btConstellationArt-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Constellation_Art"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Equatorial_Grid"));
	skyGui->buttonBar->addButton(b, "020-gridsGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btAzimuthalGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAzimuthalGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Azimuthal_Grid"));
	skyGui->buttonBar->addButton(b, "020-gridsGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btGround-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGround-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Ground"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btCardinalPoints-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btCardinalPoints-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Cardinal_Points"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btAtmosphere-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btAtmosphere-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Atmosphere"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btNebula-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNebula-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Nebulas"));
	skyGui->buttonBar->addButton(b, "040-nebulaeGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btPlanets-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btPlanets-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Planets_Labels"));
	skyGui->buttonBar->addButton(b, "040-nebulaeGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btEquatorialMount-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btEquatorialMount-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSwitch_Equatorial_Mount"));
	b->setChecked(getGuiActions("actionSwitch_Equatorial_Mount")->isChecked());
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btGotoSelectedObject-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btGotoSelectedObject-off.png");
	buttonGotoSelectedObject = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionGoto_Selected_Object"));
	skyGui->buttonBar->addButton(buttonGotoSelectedObject, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btNightView-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btNightView-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionShow_Night_Mode"));
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btFullScreen-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btFullScreen-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionSet_Full_Screen_Global"));
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRewind-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRewind-off.png");
	buttonTimeRewind = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionDecrease_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeRewind, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btTimeRealtime-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeRealtime-off.png");
	pxmapDefault = QPixmap(":/graphicGui/gui/btTimePause-on.png");
	buttonTimeRealTimeSpeed = new StelButton(NULL, pxmapOn, pxmapOff, pxmapDefault, pxmapGlow32x32, getGuiActions("actionSet_Real_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeRealTimeSpeed, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btTimeNow-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeNow-off.png");
	buttonTimeCurrent = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionReturn_To_Current_Time"));
	skyGui->buttonBar->addButton(buttonTimeCurrent, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/gui/btTimeForward-on.png");
	pxmapOff = QPixmap(":/graphicGui/gui/btTimeForward-off.png");
	buttonTimeForward = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiActions("actionIncrease_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeForward, "070-timeGroup");

	skyGui->buttonBar->setGroupMargin("070-timeGroup", 32, 0);

	pxmapOn = QPixmap(":/graphicGui/gui/btQuit.png");
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
	skyGui->setGeometry(stelAppGraphicsWidget->geometry());
	skyGui->updateBarsPos();

	setStelStyle(*StelApp::getInstance().getCurrentStelStyle());

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

	skyGui->setStelStyle(style);
	locationDialog.styleChanged();
	dateTimeDialog.styleChanged();
	configurationDialog->styleChanged();
	searchDialog.styleChanged();
	viewDialog.styleChanged();
	scriptConsole.styleChanged();
}


void StelGui::updateI18n()
{
	StelGuiBase::updateI18n();

	// Update the dialogs
	configurationDialog->languageChanged();
	dateTimeDialog.languageChanged();
	helpDialog.languageChanged();
	locationDialog.languageChanged();
	searchDialog.languageChanged();
	viewDialog.languageChanged();
}

void StelGui::update()
{
	StelNavigator* nav = StelApp::getInstance().getCore()->getNavigator();
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
		if (buttonTimeForward->isChecked()==true)
			buttonTimeForward->setChecked(false);
	}
	if (nav->getTimeRate() == 0) {
		if (buttonTimeRealTimeSpeed->isChecked() != StelButton::ButtonStateNoChange)
			buttonTimeRealTimeSpeed->setChecked(StelButton::ButtonStateNoChange);
	} else if (nav->getRealTimeSpeed()) {
		if (buttonTimeRealTimeSpeed->isChecked() != StelButton::ButtonStateOn)
			buttonTimeRealTimeSpeed->setChecked(StelButton::ButtonStateOn);
	} else if (buttonTimeRealTimeSpeed->isChecked() != StelButton::ButtonStateOff) {
		buttonTimeRealTimeSpeed->setChecked(StelButton::ButtonStateOff);
	}
	const bool isTimeNow=nav->getIsTimeNow();
	if (buttonTimeCurrent->isChecked()!=isTimeNow)
		buttonTimeCurrent->setChecked(isTimeNow);
	StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
	const bool b = mmgr->getFlagTracking();
	if (buttonGotoSelectedObject->isChecked()!=b)
		buttonGotoSelectedObject->setChecked(b);

	StarMgr* smgr = GETSTELMODULE(StarMgr);
	if (getGuiActions("actionShow_Stars")->isChecked()!=smgr->getFlagStars())
		getGuiActions("actionShow_Stars")->setChecked(smgr->getFlagStars());

	skyGui->infoPanel->setTextFromObjects(GETSTELMODULE(StelObjectMgr)->getSelectedObject());

	// Check if the progressbar window changed, if yes update the whole view
	if (savedProgressBarSize!=skyGui->progressBarMgr->boundingRect().size())
	{
		savedProgressBarSize=skyGui->progressBarMgr->boundingRect().size();
		skyGui->updateBarsPos();
	}

	if (dateTimeDialog.visible())
		dateTimeDialog.setDateTime(nav->getJDay());
}

// Add a new progress bar in the lower right corner of the screen.
QProgressBar* StelGui::addProgressBar()
{
	return skyGui->progressBarMgr->addProgressBar();
}

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
		bool bb;
		bb = (getButtonBar()->hideButton("actionShow_DSS")==btShowNebulaeBackground);
		Q_ASSERT(bb);
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

BottomStelBar* StelGui::getButtonBar() {return skyGui->buttonBar;}

LeftStelBar* StelGui::getWindowsButtonBar() {return skyGui->winBar;}

bool StelGui::getAutoHideHorizontalButtonBar() const {return skyGui->autoHideHorizontalButtonBar;}

void StelGui::setAutoHideHorizontalButtonBar(bool b) {skyGui->autoHideHorizontalButtonBar=b;}

bool StelGui::getAutoHideVerticalButtonBar() const {return skyGui->autoHideVerticalButtonBar;}

void StelGui::setAutoHideVerticalButtonBar(bool b) {skyGui->autoHideVerticalButtonBar=b;}

void StelGui::forceRefreshGui()
{
	skyGui->updateBarsPos();
}

void StelGui::scriptStarted()
{
	setScriptKeys(true);
}

void StelGui::scriptStopped()
{
	setScriptKeys(false);
}

void StelGui::setGuiVisible(bool b)
{
	setVisible(b);
}

QAction* StelGui::addGuiActions(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable, bool autoRepeat)
{
	if (!shortCut.isEmpty())
		helpDialog.setKey(helpGroup, "", shortCut, text);
	return StelGuiBase::addGuiActions(actionName, text, shortCut, helpGroup, checkable, autoRepeat);
}
