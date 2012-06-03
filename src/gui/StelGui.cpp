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
#include "StelIniParser.hpp"
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
	loadShortcuts();
#ifdef ENABLE_SCRIPT_CONSOLE
	addGuiAction("actionShow_ScriptConsole_Window_Global", N_("Script console window"), "F12", N_("Windows"), true, false, true);
#endif
	///////////////////////////////////////////////////////////////////////
	// Connect all the GUI actions signals with the Core of Stellarium
	connect(getGuiAction("actionQuit_Global"), SIGNAL(triggered()), this, SLOT(quit()));

	initConstellationMgr();
	initGrindLineMgr();
	initLandscapeMgr();

	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	connect(getGuiAction("actionShow_Nebulas"), SIGNAL(toggled(bool)), nmgr, SLOT(setFlagHints(bool)));
	getGuiAction("actionShow_Nebulas")->setChecked(nmgr->getFlagHints());

	StelSkyLayerMgr* imgr = GETSTELMODULE(StelSkyLayerMgr);
	connect(getGuiAction("actionShow_DSS"), SIGNAL(toggled(bool)), imgr, SLOT(setFlagShow(bool)));
	getGuiAction("actionShow_DSS")->setChecked(imgr->getFlagShow());


	StelCore* core = StelApp::getInstance().getCore();
	StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
	connect(getGuiAction("actionIncrease_Script_Speed"), SIGNAL(triggered()), this, SLOT(increaseScriptSpeed()));
	connect(getGuiAction("actionDecrease_Script_Speed"), SIGNAL(triggered()), this, SLOT(decreaseScriptSpeed()));
	connect(getGuiAction("actionSet_Real_Script_Speed"), SIGNAL(triggered()), this, SLOT(setRealScriptSpeed()));
	connect(getGuiAction("actionStop_Script"), SIGNAL(triggered()), this, SLOT(stopScript()));
	connect(getGuiAction("actionPause_Script"), SIGNAL(triggered()), this, SLOT(pauseScript()));
	connect(getGuiAction("actionResume_Script"), SIGNAL(triggered()), this, SLOT(resumeScript()));
	connect(getGuiAction("actionIncrease_Time_Speed"), SIGNAL(triggered()), core, SLOT(increaseTimeSpeed()));
	connect(getGuiAction("actionDecrease_Time_Speed"), SIGNAL(triggered()), core, SLOT(decreaseTimeSpeed()));
	connect(getGuiAction("actionIncrease_Time_Speed_Less"), SIGNAL(triggered()), core, SLOT(increaseTimeSpeedLess()));
	connect(getGuiAction("actionDecrease_Time_Speed_Less"), SIGNAL(triggered()), core, SLOT(decreaseTimeSpeedLess()));
	connect(getGuiAction("actionSet_Real_Time_Speed"), SIGNAL(triggered()), core, SLOT(toggleRealTimeSpeed()));
	connect(getGuiAction("actionSet_Time_Rate_Zero"), SIGNAL(triggered()), core, SLOT(setZeroTimeSpeed()));
	connect(getGuiAction("actionReturn_To_Current_Time"), SIGNAL(triggered()), core, SLOT(setTimeNow()));
	connect(getGuiAction("actionSwitch_Equatorial_Mount"), SIGNAL(toggled(bool)), mmgr, SLOT(setEquatorialMount(bool)));
	getGuiAction("actionSwitch_Equatorial_Mount")->setChecked(mmgr->getMountMode() != StelMovementMgr::MountAltAzimuthal);
	connect(getGuiAction("actionAdd_Solar_Hour"), SIGNAL(triggered()), core, SLOT(addHour()));
	connect(getGuiAction("actionAdd_Solar_Day"), SIGNAL(triggered()), core, SLOT(addDay()));
	connect(getGuiAction("actionAdd_Solar_Week"), SIGNAL(triggered()), core, SLOT(addWeek()));
	connect(getGuiAction("actionSubtract_Solar_Hour"), SIGNAL(triggered()), core, SLOT(subtractHour()));
	connect(getGuiAction("actionSubtract_Solar_Day"), SIGNAL(triggered()), core, SLOT(subtractDay()));
	connect(getGuiAction("actionSubtract_Solar_Week"), SIGNAL(triggered()), core, SLOT(subtractWeek()));
	connect(getGuiAction("actionAdd_Sidereal_Day"), SIGNAL(triggered()), core, SLOT(addSiderealDay()));
	connect(getGuiAction("actionAdd_Sidereal_Week"), SIGNAL(triggered()), core, SLOT(addSiderealWeek()));
	connect(getGuiAction("actionAdd_Sidereal_Month"), SIGNAL(triggered()), core, SLOT(addSiderealMonth()));
	connect(getGuiAction("actionAdd_Sidereal_Year"), SIGNAL(triggered()), core, SLOT(addSiderealYear()));
	connect(getGuiAction("actionSubtract_Sidereal_Day"), SIGNAL(triggered()), core, SLOT(subtractSiderealDay()));
	connect(getGuiAction("actionSubtract_Sidereal_Week"), SIGNAL(triggered()), core, SLOT(subtractSiderealWeek()));
	connect(getGuiAction("actionSubtract_Sidereal_Month"), SIGNAL(triggered()), core, SLOT(subtractSiderealMonth()));
	connect(getGuiAction("actionSubtract_Sidereal_Year"), SIGNAL(triggered()), core, SLOT(subtractSiderealYear()));
	connect(getGuiAction("actionSet_Home_Planet_To_Selected"), SIGNAL(triggered()), core, SLOT(moveObserverToSelected()));

	// connect the actor after setting the nightmode.
	// StelApp::init() already set flagNightMode for us, don't do it twice!
	getGuiAction("actionShow_Night_Mode")->setChecked(StelApp::getInstance().getVisionModeNight());
	connect(getGuiAction("actionShow_Night_Mode"), SIGNAL(toggled(bool)), &StelApp::getInstance(), SLOT(setVisionModeNight(bool)));

	connect(getGuiAction("actionGoto_Selected_Object"), SIGNAL(triggered()), mmgr, SLOT(setFlagTracking()));
	connect(getGuiAction("actionZoom_In_Auto"), SIGNAL(triggered()), mmgr, SLOT(autoZoomIn()));
	connect(getGuiAction("actionZoom_Out_Auto"), SIGNAL(triggered()), mmgr, SLOT(autoZoomOut()));
	connect(getGuiAction("actionSet_Tracking"), SIGNAL(toggled(bool)), mmgr, SLOT(setFlagTracking(bool)));
	getGuiAction("actionSet_Tracking")->setChecked(mmgr->getFlagTracking());

	connect(getGuiAction("actionSet_Full_Screen_Global"), SIGNAL(toggled(bool)), &StelMainWindow::getInstance(), SLOT(setFullScreen(bool)));
	getGuiAction("actionSet_Full_Screen_Global")->setChecked(StelMainWindow::getInstance().isFullScreen());

	connect(getGuiAction("actionShow_Location_Window_Global"), SIGNAL(toggled(bool)), &locationDialog, SLOT(setVisible(bool)));
	connect(&locationDialog, SIGNAL(visibleChanged(bool)), getGuiAction("actionShow_Location_Window_Global"), SLOT(setChecked(bool)));

#ifdef ENABLE_SCRIPT_CONSOLE
	connect(getGuiAction("actionShow_ScriptConsole_Window_Global"), SIGNAL(toggled(bool)), &scriptConsole, SLOT(setVisible(bool)));
	connect(&scriptConsole, SIGNAL(visibleChanged(bool)), getGuiAction("actionShow_ScriptConsole_Window_Global"), SLOT(setChecked(bool)));
#endif

	connect(getGuiAction("actionShow_Configuration_Window_Global"), SIGNAL(toggled(bool)), configurationDialog, SLOT(setVisible(bool)));
	connect(configurationDialog, SIGNAL(visibleChanged(bool)), getGuiAction("actionShow_Configuration_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiAction("actionShow_SkyView_Window_Global"), SIGNAL(toggled(bool)), &viewDialog, SLOT(setVisible(bool)));
	connect(&viewDialog, SIGNAL(visibleChanged(bool)), getGuiAction("actionShow_SkyView_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiAction("actionShow_Help_Window_Global"), SIGNAL(toggled(bool)), &helpDialog, SLOT(setVisible(bool)));
	connect(&helpDialog, SIGNAL(visibleChanged(bool)), getGuiAction("actionShow_Help_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiAction("actionShow_DateTime_Window_Global"), SIGNAL(toggled(bool)), &dateTimeDialog, SLOT(setVisible(bool)));
	connect(&dateTimeDialog, SIGNAL(visibleChanged(bool)), getGuiAction("actionShow_DateTime_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiAction("actionShow_Search_Window_Global"), SIGNAL(toggled(bool)), &searchDialog, SLOT(setVisible(bool)));
	connect(&searchDialog, SIGNAL(visibleChanged(bool)), getGuiAction("actionShow_Search_Window_Global"), SLOT(setChecked(bool)));

	connect(getGuiAction("actionSave_Screenshot_Global"), SIGNAL(triggered()), &StelMainGraphicsView::getInstance(), SLOT(saveScreenShot()));
	connect(getGuiAction("actionSave_Copy_Object_Information_Global"), SIGNAL(triggered()), this, SLOT(copySelectedObjectInfo()));

	getGuiAction("actionToggle_GuiHidden_Global")->setChecked(true);
	connect(getGuiAction("actionToggle_GuiHidden_Global"), SIGNAL(toggled(bool)), this, SLOT(setGuiVisible(bool)));

	connect(getGuiAction("actionHorizontal_Flip"), SIGNAL(toggled(bool)), StelApp::getInstance().getCore(), SLOT(setFlipHorz(bool)));
	getGuiAction("actionHorizontal_Flip")->setChecked(StelApp::getInstance().getCore()->getFlipHorz());
	connect(getGuiAction("actionVertical_Flip"), SIGNAL(toggled(bool)), StelApp::getInstance().getCore(), SLOT(setFlipVert(bool)));
	getGuiAction("actionVertical_Flip")->setChecked(StelApp::getInstance().getCore()->getFlipVert());

	StarMgr* smgr = GETSTELMODULE(StarMgr);
	connect(getGuiAction("actionShow_Stars"), SIGNAL(toggled(bool)), smgr, SLOT(setFlagStars(bool)));
	getGuiAction("actionShow_Stars")->setChecked(smgr->getFlagStars());

	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	connect(getGuiAction("actionShow_Planets_Labels"), SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagLabels(bool)));
	getGuiAction("actionShow_Planets_Labels")->setChecked(ssmgr->getFlagLabels());

	connect(getGuiAction("actionShow_Planets_Orbits"), SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagOrbits(bool)));
	getGuiAction("actionShow_Planets_Orbits")->setChecked(ssmgr->getFlagOrbits());

	connect(getGuiAction("actionShow_Planets_Trails"), SIGNAL(toggled(bool)), ssmgr, SLOT(setFlagTrails(bool)));
	getGuiAction("actionShow_Planets_Trails")->setChecked(ssmgr->getFlagTrails());

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	setAutoHideHorizontalButtonBar(conf->value("gui/auto_hide_horizontal_toolbar", true).toBool());
	setAutoHideVerticalButtonBar(conf->value("gui/auto_hide_vertical_toolbar", true).toBool());
	connect(getGuiAction("actionAutoHideHorizontalButtonBar"), SIGNAL(toggled(bool)), this, SLOT(setAutoHideHorizontalButtonBar(bool)));
	getGuiAction("actionAutoHideHorizontalButtonBar")->setChecked(getAutoHideHorizontalButtonBar());
	connect(getGuiAction("actionAutoHideVerticalButtonBar"), SIGNAL(toggled(bool)), this, SLOT(setAutoHideVerticalButtonBar(bool)));
	getGuiAction("actionAutoHideVerticalButtonBar")->setChecked(getAutoHideVerticalButtonBar());

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
	StelButton*  b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiAction("actionShow_Location_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/1-on-time.png");
	pxmapOff = QPixmap(":/graphicGui/1-off-time.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiAction("actionShow_DateTime_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/5-off-labels.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiAction("actionShow_SkyView_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/6-off-search.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiAction("actionShow_Search_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/8-off-settings.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiAction("actionShow_Configuration_Window_Global"));
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/9-off-help.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, getGuiAction("actionShow_Help_Window_Global"));
	skyGui->winBar->addButton(b);

	QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");

	pxmapOn = QPixmap(":/graphicGui/btConstellationLines-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationLines-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Constellation_Lines"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btConstellationLabels-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationLabels-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Constellation_Labels"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btConstellationArt-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationArt-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Constellation_Art"));
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btEquatorialGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/btEquatorialGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Equatorial_Grid"));
	skyGui->buttonBar->addButton(b, "020-gridsGroup");

	pxmapOn = QPixmap(":/graphicGui/btAzimuthalGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/btAzimuthalGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Azimuthal_Grid"));
	skyGui->buttonBar->addButton(b, "020-gridsGroup");

	pxmapOn = QPixmap(":/graphicGui/btGround-on.png");
	pxmapOff = QPixmap(":/graphicGui/btGround-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Ground"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btCardinalPoints-on.png");
	pxmapOff = QPixmap(":/graphicGui/btCardinalPoints-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Cardinal_Points"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btAtmosphere-on.png");
	pxmapOff = QPixmap(":/graphicGui/btAtmosphere-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Atmosphere"));
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btNebula-on.png");
	pxmapOff = QPixmap(":/graphicGui/btNebula-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Nebulas"));
	skyGui->buttonBar->addButton(b, "040-nebulaeGroup");

	pxmapOn = QPixmap(":/graphicGui/btPlanets-on.png");
	pxmapOff = QPixmap(":/graphicGui/btPlanets-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Planets_Labels"));
	skyGui->buttonBar->addButton(b, "040-nebulaeGroup");

	pxmapOn = QPixmap(":/graphicGui/btEquatorialMount-on.png");
	pxmapOff = QPixmap(":/graphicGui/btEquatorialMount-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionSwitch_Equatorial_Mount"));
	b->setChecked(getGuiAction("actionSwitch_Equatorial_Mount")->isChecked());
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btGotoSelectedObject-on.png");
	pxmapOff = QPixmap(":/graphicGui/btGotoSelectedObject-off.png");
	buttonGotoSelectedObject = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionGoto_Selected_Object"));
	skyGui->buttonBar->addButton(buttonGotoSelectedObject, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btNightView-on.png");
	pxmapOff = QPixmap(":/graphicGui/btNightView-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionShow_Night_Mode"));
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btFullScreen-on.png");
	pxmapOff = QPixmap(":/graphicGui/btFullScreen-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionSet_Full_Screen_Global"));
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeRewind-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeRewind-off.png");
	buttonTimeRewind = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionDecrease_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeRewind, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeRealtime-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeRealtime-off.png");
	pxmapDefault = QPixmap(":/graphicGui/btTimePause-on.png");
	buttonTimeRealTimeSpeed = new StelButton(NULL, pxmapOn, pxmapOff, pxmapDefault, pxmapGlow32x32, getGuiAction("actionSet_Real_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeRealTimeSpeed, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeNow-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeNow-off.png");
	buttonTimeCurrent = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionReturn_To_Current_Time"));
	skyGui->buttonBar->addButton(buttonTimeCurrent, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeForward-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeForward-off.png");
	buttonTimeForward = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, getGuiAction("actionIncrease_Time_Speed"));
	skyGui->buttonBar->addButton(buttonTimeForward, "070-timeGroup");

	skyGui->buttonBar->setGroupMargin("070-timeGroup", 32, 0);

	pxmapOn = QPixmap(":/graphicGui/btQuit.png");
	b = new StelButton(NULL, pxmapOn, pxmapOn, pxmapGlow32x32, getGuiAction("actionQuit_Global"));
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
	getGuiAction("actionShow_Constellation_Lines")->setChecked(constellationMgr->getFlagLines());
	connect(getGuiAction("actionShow_Constellation_Lines"),
					SIGNAL(toggled(bool)),
					constellationMgr,
					SLOT(setFlagLines(bool)));
	connect(constellationMgr,
					SIGNAL(linesDisplayedChanged(const bool)),
					this,
					SLOT(linesDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Constellation_Art")->setChecked(constellationMgr->getFlagArt());
	connect(getGuiAction("actionShow_Constellation_Art"),
					SIGNAL(toggled(bool)),
					constellationMgr,
					SLOT(setFlagArt(bool)));
	connect(constellationMgr,
					SIGNAL(artDisplayedChanged(const bool)),
					this,
					SLOT(artDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Constellation_Labels")->setChecked(constellationMgr->getFlagLabels());
	connect(getGuiAction("actionShow_Constellation_Labels"),
					SIGNAL(toggled(bool)),
					constellationMgr,
					SLOT(setFlagLabels(bool)));
	connect(constellationMgr,
					SIGNAL(namesDisplayedChanged(const bool)),
					this,
					SLOT(namesDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Constellation_Boundaries")->setChecked(constellationMgr->getFlagBoundaries());
	connect(getGuiAction("actionShow_Constellation_Boundaries"),
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
	getGuiAction("actionShow_Equatorial_Grid")->setChecked(gridLineManager->getFlagEquatorGrid());
	connect(getGuiAction("actionShow_Equatorial_Grid"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagEquatorGrid(bool)));
	connect(gridLineManager,
					SIGNAL(equatorGridDisplayedChanged(const bool)),
					this,
					SLOT(equatorGridDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Azimuthal_Grid")->setChecked(gridLineManager->getFlagAzimuthalGrid());
	connect(getGuiAction("actionShow_Azimuthal_Grid"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagAzimuthalGrid(bool)));
	connect(gridLineManager,
					SIGNAL(azimuthalGridDisplayedChanged(const bool)),
					this,
					SLOT(azimuthalGridDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Ecliptic_Line")->setChecked(gridLineManager->getFlagEclipticLine());
	connect(getGuiAction("actionShow_Ecliptic_Line"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagEclipticLine(bool)));
	connect(gridLineManager,
					SIGNAL(eclipticLineDisplayedChanged(const bool)),
					this,
					SLOT(eclipticLineDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Equator_Line")->setChecked(gridLineManager->getFlagEquatorLine());
	connect(getGuiAction("actionShow_Equator_Line"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagEquatorLine(bool)));
	connect(gridLineManager,
					SIGNAL(equatorLineDisplayedChanged(const bool)),
					this,
					SLOT(equatorLineDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Meridian_Line")->setChecked(gridLineManager->getFlagMeridianLine());
	connect(getGuiAction("actionShow_Meridian_Line"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagMeridianLine(bool)));
	connect(gridLineManager,
					SIGNAL(meridianLineDisplayedChanged(const bool)),
					this,
					SLOT(meridianLineDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Horizon_Line")->setChecked(gridLineManager->getFlagHorizonLine());
	connect(getGuiAction("actionShow_Horizon_Line"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagHorizonLine(bool)));
	connect(gridLineManager,
					SIGNAL(horizonLineDisplayedChanged(const bool)),
					this,
					SLOT(horizonLineDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Equatorial_J2000_Grid")->setChecked(gridLineManager->getFlagEquatorJ2000Grid());
	connect(getGuiAction("actionShow_Equatorial_J2000_Grid"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagEquatorJ2000Grid(bool)));
	connect(gridLineManager,
					SIGNAL(equatorJ2000GridDisplayedChanged(const bool)),
					this,
					SLOT(equatorJ2000GridDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Ecliptic_J2000_Grid")->setChecked(gridLineManager->getFlagEclipticJ2000Grid());
	connect(getGuiAction("actionShow_Ecliptic_J2000_Grid"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagEclipticJ2000Grid(bool)));
	connect(gridLineManager,
					SIGNAL(eclipticJ2000GridDisplayedChanged(const bool)),
					this,
					SLOT(eclipticJ2000GridDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Galactic_Grid")->setChecked(gridLineManager->getFlagGalacticGrid());
	connect(getGuiAction("actionShow_Galactic_Grid"),
					SIGNAL(toggled(bool)),
					gridLineManager,
					SLOT(setFlagGalacticGrid(bool)));
	connect(gridLineManager,
					SIGNAL(galacticGridDisplayedChanged(const bool)),
					this,
					SLOT(galacticGridDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Galactic_Plane_Line")->setChecked(gridLineManager->getFlagGalacticPlaneLine());
	connect(getGuiAction("actionShow_Galactic_Plane_Line"),
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
	getGuiAction("actionShow_Ground")->setChecked(landscapeMgr->getFlagLandscape());
	connect(getGuiAction("actionShow_Ground"),
					SIGNAL(toggled(bool)),
					landscapeMgr,
					SLOT(setFlagLandscape(bool)));
	connect(landscapeMgr,
					SIGNAL(landscapeDisplayedChanged(const bool)),
					this,
					SLOT(landscapeDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Cardinal_Points")->setChecked(landscapeMgr->getFlagCardinalsPoints());
	connect(getGuiAction("actionShow_Cardinal_Points"),
					SIGNAL(toggled(bool)),
					landscapeMgr,
					SLOT(setFlagCardinalsPoints(bool)));
	connect(landscapeMgr,
					SIGNAL(cardinalsPointsDisplayedChanged(const bool)),
					this,
					SLOT(cardinalsPointsDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Atmosphere")->setChecked(landscapeMgr->getFlagAtmosphere());
	connect(getGuiAction("actionShow_Atmosphere"),
					SIGNAL(toggled(bool)),
					landscapeMgr,
					SLOT(setFlagAtmosphere(bool)));
	connect(landscapeMgr,
					SIGNAL(atmosphereDisplayedChanged(const bool)),
					this,
					SLOT(atmosphereDisplayedUpdated(const bool)));

	getGuiAction("actionShow_Fog")->setChecked(landscapeMgr->getFlagFog());
	connect(getGuiAction("actionShow_Fog"),
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
	if (getGuiAction("actionShow_Stars")->isChecked() != flag) {
		getGuiAction("actionShow_Stars")->setChecked(flag);
	}

	flag = GETSTELMODULE(NebulaMgr)->getFlagHints();
	if (getGuiAction("actionShow_Nebulas")->isChecked() != flag)
		getGuiAction("actionShow_Nebulas")->setChecked(flag);

	flag = GETSTELMODULE(StelSkyLayerMgr)->getFlagShow();
	if (getGuiAction("actionShow_DSS")->isChecked() != flag)
		getGuiAction("actionShow_DSS")->setChecked(flag);

	flag = mmgr->getMountMode() != StelMovementMgr::MountAltAzimuthal;
	if (getGuiAction("actionSwitch_Equatorial_Mount")->isChecked() != flag)
		getGuiAction("actionSwitch_Equatorial_Mount")->setChecked(flag);

	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	flag = ssmgr->getFlagLabels();
	if (getGuiAction("actionShow_Planets_Labels")->isChecked() != flag)
		getGuiAction("actionShow_Planets_Labels")->setChecked(flag);
	flag = ssmgr->getFlagOrbits();
	if (getGuiAction("actionShow_Planets_Orbits")->isChecked() != flag)
		getGuiAction("actionShow_Planets_Orbits")->setChecked(flag);
	flag = ssmgr->getFlagTrails();
	if (getGuiAction("actionShow_Planets_Trails")->isChecked() != flag)
		getGuiAction("actionShow_Planets_Trails")->setChecked(flag);
	flag = StelApp::getInstance().getVisionModeNight();
	if (getGuiAction("actionShow_Night_Mode")->isChecked() != flag)
		getGuiAction("actionShow_Night_Mode")->setChecked(flag);
	flag = StelMainWindow::getInstance().isFullScreen();
	if (getGuiAction("actionSet_Full_Screen_Global")->isChecked() != flag)
		getGuiAction("actionSet_Full_Screen_Global")->setChecked(flag);

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
		getGuiAction("actionDecrease_Time_Speed")->setShortcut(QKeySequence());
		getGuiAction("actionIncrease_Time_Speed")->setShortcut(QKeySequence());
		getGuiAction("actionSet_Real_Time_Speed")->setShortcut(QKeySequence());
		getGuiAction("actionDecrease_Script_Speed")->setShortcut(QKeySequence("J"));
		getGuiAction("actionIncrease_Script_Speed")->setShortcut(QKeySequence("L"));
		getGuiAction("actionSet_Real_Script_Speed")->setShortcut(QKeySequence("K"));
	}
	else
	{
		getGuiAction("actionDecrease_Script_Speed")->setShortcut(QKeySequence());
		getGuiAction("actionIncrease_Script_Speed")->setShortcut(QKeySequence());
		getGuiAction("actionSet_Real_Script_Speed")->setShortcut(QKeySequence());
		getGuiAction("actionDecrease_Time_Speed")->setShortcut(QKeySequence("J"));
		getGuiAction("actionIncrease_Time_Speed")->setShortcut(QKeySequence("L"));
		getGuiAction("actionSet_Real_Time_Speed")->setShortcut(QKeySequence("K"));
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
																getGuiAction("actionVertical_Flip"));
		}
		if (flipHoriz==NULL) {
			QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");
			flipHoriz = new StelButton(NULL,
																 QPixmap(":/graphicGui/btFlipHorizontal-on.png"),
																 QPixmap(":/graphicGui/btFlipHorizontal-off.png"),
																 pxmapGlow32x32,
																 getGuiAction("actionHorizontal_Flip"));
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
			btShowNebulaeBackground = new StelButton(NULL, QPixmap(":/graphicGui/btDSS-on.png"), QPixmap(":/graphicGui/btDSS-off.png"), pxmapGlow32x32, getGuiAction("actionShow_DSS"));
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

QAction* StelGui::addGuiAction(const QString& actionName, const QString& text, const QString& shortCut, const QString& helpGroup, bool checkable, bool autoRepeat, bool global)
{
	if (!shortCut.isEmpty())
		helpDialog.setKey(helpGroup, "", shortCut, text);
	return StelGuiBase::addGuiAction(actionName, text, shortCut, helpGroup, checkable, autoRepeat, global);
}

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Process changes from the ConstellationMgr
#endif
/* ****************************************************************************************************************** */
void StelGui::artDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Constellation_Art")->isChecked() != displayed) {
		getGuiAction("actionShow_Constellation_Art")->setChecked(displayed);
	}
}
void StelGui::boundariesDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Constellation_Boundaries")->isChecked() != displayed) {
		getGuiAction("actionShow_Constellation_Boundaries")->setChecked(displayed);
	}
}
void StelGui::linesDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Constellation_Lines")->isChecked() != displayed) {
		getGuiAction("actionShow_Constellation_Lines")->setChecked(displayed);
	}
}
void StelGui::namesDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Constellation_Labels")->isChecked() != displayed) {
		getGuiAction("actionShow_Constellation_Labels")->setChecked(displayed);
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
	if (getGuiAction("actionShow_Azimuthal_Grid")->isChecked() != displayed) {
		getGuiAction("actionShow_Azimuthal_Grid")->setChecked(displayed);
	}
}

void StelGui::equatorGridDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Equatorial_Grid")->isChecked() != displayed) {
		getGuiAction("actionShow_Equatorial_Grid")->setChecked(displayed);
	}
}

void StelGui::equatorJ2000GridDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Equatorial_J2000_Grid")->isChecked() != displayed) {
		getGuiAction("actionShow_Equatorial_J2000_Grid")->setChecked(displayed);
	}
}

void StelGui::eclipticJ2000GridDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Ecliptic_J2000_Grid")->isChecked() != displayed) {
		getGuiAction("actionShow_Ecliptic_J2000_Grid")->setChecked(displayed);
	}
}


void StelGui::galacticGridDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Galactic_Grid")->isChecked() != displayed) {
		getGuiAction("actionShow_Galactic_Grid")->setChecked(displayed);
	}
}

void StelGui::equatorLineDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Equator_Line")->isChecked() != displayed) {
		getGuiAction("actionShow_Equator_Line")->setChecked(displayed);
	}
}

void StelGui::eclipticLineDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Ecliptic_Line")->isChecked() != displayed) {
		getGuiAction("actionShow_Ecliptic_Line")->setChecked(displayed);
	}
}

void StelGui::meridianLineDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Meridian_Line")->isChecked() != displayed) {
		getGuiAction("actionShow_Meridian_Line")->setChecked(displayed);
	}
}

void StelGui::horizonLineDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Horizon_Line")->isChecked() != displayed) {
		getGuiAction("actionShow_Horizon_Line")->setChecked(displayed);
	}
}

void StelGui::galacticPlaneLineDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Galactic_Plane_Line")->isChecked() != displayed) {
		getGuiAction("actionShow_Galactic_Plane_Line")->setChecked(displayed);
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
	if (getGuiAction("actionShow_Atmosphere")->isChecked() != displayed) {
		getGuiAction("actionShow_Atmosphere")->setChecked(displayed);
	}
}

void StelGui::cardinalsPointsDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Cardinal_Points")->isChecked() != displayed) {
		getGuiAction("actionShow_Cardinal_Points")->setChecked(displayed);
	}
}

void StelGui::fogDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Fog")->isChecked() != displayed) {
		getGuiAction("actionShow_Fog")->setChecked(displayed);
	}
}

void StelGui::landscapeDisplayedUpdated(const bool displayed)
{
	if (getGuiAction("actionShow_Ground")->isChecked() != displayed) {
		getGuiAction("actionShow_Ground")->setChecked(displayed);
	}
}

void StelGui::copySelectedObjectInfo(void)
{
	QApplication::clipboard()->setText(skyGui->infoPanel->getSelectedText());
}

bool StelGui::loadShortcuts(const QString &filePath)
{
	QSettings scs(filePath, StelIniFormat);
	if (scs.status() != QSettings::NoError)
	{
		qWarning() << "ERROR while parsing" << filePath;
		return false;
	}
	// Shortcuts format example:
	// [search]
	// name = actionShow_Search_Window_Global
	// text = Search window
	// shortcuts = F3; Ctrl+F
	// group = Display Options
	// checkable = 1
	// autorepeat = 0
	// global = 1
	QStringList sections = scs.childGroups();
	foreach (QString section, sections)
	{
		QString name = section;
		QString text = scs.value(section + "/text").toString();
		QString shortcuts = scs.value(section + "/shortcuts").toString();
		QString group = scs.value(section + "/group").toString();
		// TODO: add true/false, on/off, yes/no, y/n cases (not only 1/0) for bool values
		bool checkable = scs.value(section + "/checkable", QVariant("1")).toString().toInt();
		bool autorepeat = scs.value(section + "/autorepeat", QVariant("0")).toString().toInt();
		bool global = scs.value(section + "/global", QVariant("0")).toString().toInt();
		if (name == "")
		{
			qWarning() << "Name missing in " << section << " shortcut, parsing wasn't done";
			continue;
		}
		addGuiAction(name, text, shortcuts, group, checkable, autorepeat, global);
	}
	return true;
}

void StelGui::loadShortcuts()
{
	qDebug() << "Loading shortcuts ...";
	QStringList shortcutFiles;
	try
	{
		shortcutFiles = StelFileMgr::findFileInAllPaths("data/shortcuts.ini");
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "ERROR while loading shortcuts.ini (unable to find data/shortcuts.ini): " << e.what() << endl;
		return;
	}
	foreach (QString shortcutFile, shortcutFiles)
	{
		if (loadShortcuts(shortcutFile))
			break;
		else
		{
			if (shortcutFile.contains(StelFileMgr::getUserDir()))
			{
				QString newName = QString("%1/data/shortcuts-%2.ini").arg(StelFileMgr::getUserDir()).arg(QDateTime::currentDateTime().toString("yyyyMMddThhmmss"));
				if (QFile::rename(shortcutFile, newName))
					qWarning() << "Invalid shortcuts file" << shortcutFile << "has been renamed to" << newName;
				else
				{
					qWarning() << "Invalid shortcuts file" << shortcutFile << "cannot be removed!";
					qWarning() << "Please either delete it, rename it or move it elsewhere.";
				}
			}
		}
	}
}
