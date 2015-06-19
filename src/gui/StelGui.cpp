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
#include "StelMainView.hpp"
#include "StelObjectMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StarMgr.hpp"
#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "NebulaMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelActionMgr.hpp"

#include "SporadicMeteorMgr.hpp"
#include "StelObjectType.hpp"
#include "StelObject.hpp"
#include "SolarSystem.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelStyle.hpp"
#include "StelSkyDrawer.hpp"
#ifdef ENABLE_SCRIPT_CONSOLE
#include "ScriptConsole.hpp"
#endif
#ifndef DISABLE_SCRIPTING
#include "StelScriptMgr.hpp"
#endif

#include "ConfigurationDialog.hpp"
#include "DateTimeDialog.hpp"
#include "HelpDialog.hpp"
#include "LocationDialog.hpp"
#include "SearchDialog.hpp"
#include "ViewDialog.hpp"
#include "ShortcutsDialog.hpp"

#include <QDebug>
#include <QTimeLine>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QFile>
#include <QTextBrowser>
#include <QGraphicsWidget>
#include <QGraphicsGridLayout>
#include <QClipboard>
#include <QPalette>
#include <QColor>
#include <QAction>

StelGui::StelGui()
	: topLevelGraphicsWidget(NULL)
	, skyGui(NULL)
	, buttonTimeRewind(NULL)
	, buttonTimeRealTimeSpeed(NULL)
	, buttonTimeCurrent(NULL)
	, buttonTimeForward(NULL)
	, buttonGotoSelectedObject(NULL)
	, locationDialog(0)
	, helpDialog(0)
	, dateTimeDialog(0)
	, searchDialog(0)
	, viewDialog(0)
	, shortcutsDialog(0)
	, configurationDialog(0)
#ifdef ENABLE_SCRIPT_CONSOLE
	, scriptConsole(0)
#endif
	, flagShowFlipButtons(false)
	, flipVert(NULL)
	, flipHoriz(NULL)
	, flagShowNebulaBackgroundButton(false)
	, btShowNebulaeBackground(NULL)
	, initDone(false)
{
	// QPixmapCache::setCacheLimit(30000); ?
}

StelGui::~StelGui()
{
	delete skyGui;
	skyGui = NULL;

	if (locationDialog)
	{
		delete locationDialog;
		locationDialog = 0;
	}
	if (helpDialog)
	{
		delete helpDialog;
		helpDialog = 0;
	}
	if (dateTimeDialog)
	{
		delete dateTimeDialog;
		dateTimeDialog = 0;
	}
	if (searchDialog)
	{
		delete searchDialog;
		searchDialog = 0;
	}
	if (viewDialog)
	{
		delete viewDialog;
		viewDialog = 0;
	}
	if (shortcutsDialog)
	{
		delete shortcutsDialog;
		shortcutsDialog = NULL;
	}
	// configurationDialog is automatically deleted with its parent widget.
#ifdef ENABLE_SCRIPT_CONSOLE
	if (scriptConsole)
	{
		delete scriptConsole;
		scriptConsole = 0;
	}
#endif
}

void StelGui::init(QGraphicsWidget *atopLevelGraphicsWidget)
{
	qDebug() << "Creating GUI ...";

	StelGuiBase::init(atopLevelGraphicsWidget);
	skyGui = new SkyGui(atopLevelGraphicsWidget);
	locationDialog = new LocationDialog(atopLevelGraphicsWidget);
	helpDialog = new HelpDialog(atopLevelGraphicsWidget);
	dateTimeDialog = new DateTimeDialog(atopLevelGraphicsWidget);
	searchDialog = new SearchDialog(atopLevelGraphicsWidget);
	viewDialog = new ViewDialog(atopLevelGraphicsWidget);
	shortcutsDialog = new ShortcutsDialog(atopLevelGraphicsWidget);
	configurationDialog = new ConfigurationDialog(this, atopLevelGraphicsWidget);
#ifdef ENABLE_SCRIPT_CONSOLE
	scriptConsole = new ScriptConsole();
#endif

	///////////////////////////////////////////////////////////////////////
	// Create all the main actions of the program, associated with shortcuts

	///////////////////////////////////////////////////////////////////////
	// Connect all the GUI actions signals with the Core of Stellarium
	StelActionMgr* actionsMgr = StelApp::getInstance().getStelActionManager();

	// XXX: this should probably go into the script manager.
	QString datetimeGroup = N_("Date and Time");
	QString windowsGroup = N_("Windows");
	QString miscGroup = N_("Miscellaneous");
	actionsMgr->addAction("actionQuit_Global", miscGroup, N_("Quit"), this, "quit()", "Ctrl+Q", "Ctrl+X");
	actionsMgr->addAction("actionIncrease_Script_Speed", datetimeGroup, N_("Speed up the script execution rate"), this, "increaseScriptSpeed()");
	actionsMgr->addAction("actionDecrease_Script_Speed", datetimeGroup, N_("Slow down the script execution rate"), this, "decreaseScriptSpeed()");
	actionsMgr->addAction("actionSet_Real_Script_Speed", datetimeGroup, N_("Set the normal script execution rate"), this, "setRealScriptSpeed()");
	actionsMgr->addAction("actionStop_Script", datetimeGroup, N_("Stop script execution"), this, "stopScript()", "Ctrl+D, S");
	actionsMgr->addAction("actionPause_Script", datetimeGroup, N_("Pause script execution"), this, "pauseScript()", "Ctrl+D, P");
	actionsMgr->addAction("actionResume_Script", datetimeGroup, N_("Resume script execution"), this, "resumeScript()", "Ctrl+D, R");

#ifdef ENABLE_SCRIPT_CONSOLE
	actionsMgr->addAction("actionShow_ScriptConsole_Window_Global", windowsGroup, N_("Script console window"), scriptConsole, "visible", "F12", "", true);
#endif

	actionsMgr->addAction("actionShow_Help_Window_Global", windowsGroup, N_("Help window"), helpDialog, "visible", "F1", "", true);
	actionsMgr->addAction("actionShow_Configuration_Window_Global", windowsGroup, N_("Configuration window"), configurationDialog, "visible", "F2", "", true);
	actionsMgr->addAction("actionShow_Search_Window_Global", windowsGroup, N_("Search window"), searchDialog, "visible", "F3", "Ctrl+F", true);
	actionsMgr->addAction("actionShow_SkyView_Window_Global", windowsGroup, N_("Sky and viewing options window"), viewDialog, "visible", "F4", "", true);
	actionsMgr->addAction("actionShow_DateTime_Window_Global", windowsGroup, N_("Date/time window"), dateTimeDialog, "visible", "F5", "", true);
	actionsMgr->addAction("actionShow_Location_Window_Global", windowsGroup, N_("Location window"), locationDialog, "visible", "F6", "", true);
	actionsMgr->addAction("actionShow_Shortcuts_Window_Global", windowsGroup, N_("Shortcuts window"), shortcutsDialog, "visible", "F7", "", true);
	actionsMgr->addAction("actionSave_Copy_Object_Information_Global", miscGroup, N_("Copy selected object information to clipboard"), this, "copySelectedObjectInfo()", "Ctrl+C", "", true);	

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	setAutoHideHorizontalButtonBar(conf->value("gui/auto_hide_horizontal_toolbar", true).toBool());
	setAutoHideVerticalButtonBar(conf->value("gui/auto_hide_vertical_toolbar", true).toBool());
	actionsMgr->addAction("actionAutoHideHorizontalButtonBar", miscGroup, N_("Auto hide horizontal button bar"), this, "autoHideHorizontalButtonBar");
	actionsMgr->addAction("actionAutoHideVerticalButtonBar", miscGroup, N_("Auto hide vertical button bar"), this, "autoHideVerticalButtonBar");

	setGuiVisible(conf->value("gui/flag_show_gui", true).toBool());
	actionsMgr->addAction("actionToggle_GuiHidden_Global", miscGroup, N_("Toggle visibility of GUI"), this, "visible", "Ctrl+T", "", true);

#ifndef DISABLE_SCRIPTING
	StelScriptMgr* scriptMgr = &StelApp::getInstance().getScriptMgr();
	connect(scriptMgr, SIGNAL(scriptRunning()), this, SLOT(scriptStarted()));
	connect(scriptMgr, SIGNAL(scriptStopped()), this, SLOT(scriptStopped()));
#endif

	///////////////////////////////////////////////////////////////////////////
	//// QGraphicsView based GUI
	///////////////////////////////////////////////////////////////////////////

	// Add everything
	QPixmap pxmapDefault;
	QPixmap pxmapGlow(":/graphicGui/glow.png");
	QPixmap pxmapOn(":/graphicGui/2-on-location.png");
	QPixmap pxmapOff(":/graphicGui/2-off-location.png");
	StelButton*  b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, "actionShow_Location_Window_Global");
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/1-on-time.png");
	pxmapOff = QPixmap(":/graphicGui/1-off-time.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, "actionShow_DateTime_Window_Global");
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/5-on-labels.png");
	pxmapOff = QPixmap(":/graphicGui/5-off-labels.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, "actionShow_SkyView_Window_Global");
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/6-on-search.png");
	pxmapOff = QPixmap(":/graphicGui/6-off-search.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, "actionShow_Search_Window_Global");
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/8-on-settings.png");
	pxmapOff = QPixmap(":/graphicGui/8-off-settings.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, "actionShow_Configuration_Window_Global");
	skyGui->winBar->addButton(b);

	pxmapOn = QPixmap(":/graphicGui/9-on-help.png");
	pxmapOff = QPixmap(":/graphicGui/9-off-help.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow, "actionShow_Help_Window_Global");
	skyGui->winBar->addButton(b);

	QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");

	pxmapOn = QPixmap(":/graphicGui/btConstellationLines-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationLines-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Constellation_Lines");
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btConstellationLabels-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationLabels-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Constellation_Labels");
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btConstellationArt-on.png");
	pxmapOff = QPixmap(":/graphicGui/btConstellationArt-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Constellation_Art");
	skyGui->buttonBar->addButton(b, "010-constellationsGroup");

	pxmapOn = QPixmap(":/graphicGui/btEquatorialGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/btEquatorialGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Equatorial_Grid");
	skyGui->buttonBar->addButton(b, "020-gridsGroup");

	pxmapOn = QPixmap(":/graphicGui/btAzimuthalGrid-on.png");
	pxmapOff = QPixmap(":/graphicGui/btAzimuthalGrid-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Azimuthal_Grid");
	skyGui->buttonBar->addButton(b, "020-gridsGroup");

	pxmapOn = QPixmap(":/graphicGui/btGround-on.png");
	pxmapOff = QPixmap(":/graphicGui/btGround-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Ground");
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btCardinalPoints-on.png");
	pxmapOff = QPixmap(":/graphicGui/btCardinalPoints-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Cardinal_Points");
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btAtmosphere-on.png");
	pxmapOff = QPixmap(":/graphicGui/btAtmosphere-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Atmosphere");
	skyGui->buttonBar->addButton(b, "030-landscapeGroup");

	pxmapOn = QPixmap(":/graphicGui/btNebula-on.png");
	pxmapOff = QPixmap(":/graphicGui/btNebula-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Nebulas");
	skyGui->buttonBar->addButton(b, "040-nebulaeGroup");

	pxmapOn = QPixmap(":/graphicGui/btPlanets-on.png");
	pxmapOff = QPixmap(":/graphicGui/btPlanets-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Planets_Labels");
	skyGui->buttonBar->addButton(b, "040-nebulaeGroup");

	pxmapOn = QPixmap(":/graphicGui/btEquatorialMount-on.png");
	pxmapOff = QPixmap(":/graphicGui/btEquatorialMount-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionSwitch_Equatorial_Mount");
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btGotoSelectedObject-on.png");
	pxmapOff = QPixmap(":/graphicGui/btGotoSelectedObject-off.png");
	buttonGotoSelectedObject = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionGoto_Selected_Object");
	skyGui->buttonBar->addButton(buttonGotoSelectedObject, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btNightView-on.png");
	pxmapOff = QPixmap(":/graphicGui/btNightView-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_Night_Mode");
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btFullScreen-on.png");
	pxmapOff = QPixmap(":/graphicGui/btFullScreen-off.png");
	b = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionSet_Full_Screen_Global");
	skyGui->buttonBar->addButton(b, "060-othersGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeRewind-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeRewind-off.png");
	buttonTimeRewind = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionDecrease_Time_Speed");
	skyGui->buttonBar->addButton(buttonTimeRewind, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeRealtime-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeRealtime-off.png");
	pxmapDefault = QPixmap(":/graphicGui/btTimePause-on.png");
	buttonTimeRealTimeSpeed = new StelButton(NULL, pxmapOn, pxmapOff, pxmapDefault, pxmapGlow32x32, "actionSet_Real_Time_Speed");
	skyGui->buttonBar->addButton(buttonTimeRealTimeSpeed, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeNow-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeNow-off.png");
	buttonTimeCurrent = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionReturn_To_Current_Time");
	skyGui->buttonBar->addButton(buttonTimeCurrent, "070-timeGroup");

	pxmapOn = QPixmap(":/graphicGui/btTimeForward-on.png");
	pxmapOff = QPixmap(":/graphicGui/btTimeForward-off.png");
	buttonTimeForward = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionIncrease_Time_Speed");
	skyGui->buttonBar->addButton(buttonTimeForward, "070-timeGroup");

	skyGui->buttonBar->setGroupMargin("070-timeGroup", 32, 0);

	pxmapOn = QPixmap(":/graphicGui/btQuit.png");
	b = new StelButton(NULL, pxmapOn, pxmapOn, pxmapGlow32x32, "actionQuit_Global");
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
	atopLevelGraphicsWidget->setLayout(l);

	setStelStyle(StelApp::getInstance().getCurrentStelStyle());

	skyGui->setGeometry(atopLevelGraphicsWidget->geometry());
	skyGui->updateBarsPos();

	// The disabled text for checkboxes is embossed with the QPalette::Light setting for the ColorGroup Disabled.
	// It doesn't appear to be possible to set this from the stylesheet.  Instead we'll make it 100% transparent
	// and set the text color for disabled in the stylesheets.
	QPalette p = QGuiApplication::palette();
	p.setColor(QPalette::Disabled, QPalette::Light, QColor(0,0,0,0));

	// And this is for the focus...  apparently the focus indicator is the inverted value for Active/Button.
	p.setColor(QPalette::Active, QPalette::Button, QColor(255,255,255));
	QGuiApplication::setPalette(p);
	
	// FIXME: Workaround for set UI language when app is started --AW
	updateI18n();

	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	initDone = true;
}

void StelGui::quit()
{
	StelApp::getInstance().quit();
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
		if(styleFile.open(QIODevice::ReadOnly))
		{
			currentStelStyle.qtStyleSheet = styleFile.readAll();
			styleFile.close();
		}

		QFile htmlStyleFile(htmlStyleFileName);
		if(htmlStyleFile.open(QIODevice::ReadOnly))
		{
			currentStelStyle.htmlStyleSheet = htmlStyleFile.readAll();
			htmlStyleFile.close();
		}
	}
	
	locationDialog->styleChanged();
	dateTimeDialog->styleChanged();
	configurationDialog->styleChanged();
	searchDialog->styleChanged();
	viewDialog->styleChanged();
#ifdef ENABLE_SCRIPT_CONSOLE
	scriptConsole->styleChanged();
#endif // ENABLE_SCRIPT_CONSOLE
}


void StelGui::updateI18n()
{
	// Translate all action texts
	foreach (QObject* obj, StelMainView::getInstance().children())
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

	bool flag;

	// XXX: this should probably be removed, we can use property NOTIFY instead.
	flag = GETSTELMODULE(StelSkyLayerMgr)->getFlagShow();
	if (getAction("actionShow_DSS")->isChecked() != flag)
		getAction("actionShow_DSS")->setChecked(flag);

	flag = StelApp::getInstance().getVisionModeNight();
	if (getAction("actionShow_Night_Mode")->isChecked() != flag)
		getAction("actionShow_Night_Mode")->setChecked(flag);

	flag = StelMainView::getInstance().isFullScreen();
	if (getAction("actionSet_Full_Screen_Global")->isChecked() != flag)
		getAction("actionSet_Full_Screen_Global")->setChecked(flag);

	skyGui->infoPanel->setTextFromObjects(GETSTELMODULE(StelObjectMgr)->getSelectedObject());

	// Check if the progressbar window changed, if yes update the whole view
	if (savedProgressBarSize!=skyGui->progressBarMgr->boundingRect().size())
	{
		savedProgressBarSize=skyGui->progressBarMgr->boundingRect().size();
		forceRefreshGui();
	}

	dateTimeDialog->setDateTime(core->getJDay());
}

#ifndef DISABLE_SCRIPTING
void StelGui::setScriptKeys(bool b)
{
	if (b)
	{
		getAction("actionDecrease_Time_Speed")->setShortcut("");
		getAction("actionIncrease_Time_Speed")->setShortcut("");
		getAction("actionSet_Real_Time_Speed")->setShortcut("");
		getAction("actionDecrease_Script_Speed")->setShortcut("J");
		getAction("actionIncrease_Script_Speed")->setShortcut("L");
		getAction("actionSet_Real_Script_Speed")->setShortcut("K");
	}
	else
	{
		getAction("actionDecrease_Script_Speed")->setShortcut("");
		getAction("actionIncrease_Script_Speed")->setShortcut("");
		getAction("actionSet_Real_Script_Speed")->setShortcut("");
		getAction("actionDecrease_Time_Speed")->setShortcut("J");
		getAction("actionIncrease_Time_Speed")->setShortcut("L");
		getAction("actionSet_Real_Time_Speed")->setShortcut("K");
	}
}

void StelGui::increaseScriptSpeed()
{
	StelApp::getInstance().getScriptMgr().setScriptRate(StelApp::getInstance().getScriptMgr().getScriptRate()*2);
}

void StelGui::decreaseScriptSpeed()
{	
	StelApp::getInstance().getScriptMgr().setScriptRate(StelApp::getInstance().getScriptMgr().getScriptRate()/2);
}

void StelGui::setRealScriptSpeed()
{	
	StelApp::getInstance().getScriptMgr().setScriptRate(1);
}

void StelGui::stopScript()
{	
	StelApp::getInstance().getScriptMgr().stopScript();
}

void StelGui::pauseScript()
{	
	StelApp::getInstance().getScriptMgr().pauseScript();
}

void StelGui::resumeScript()
{	
	StelApp::getInstance().getScriptMgr().resumeScript();
}
#endif

void StelGui::setFlagShowFlipButtons(bool b)
{
	if (b==true) {
		if (flipVert==NULL) {
			// Create the vertical flip button
			QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");
			QPixmap pxmapOn(":/graphicGui/btFlipVertical-on.png");
			QPixmap pxmapOff(":/graphicGui/btFlipVertical-off.png");
			flipVert = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionVertical_Flip");
		}
		if (flipHoriz==NULL) {
			QPixmap pxmapGlow32x32(":/graphicGui/glow32x32.png");
			QPixmap pxmapOn(":/graphicGui/btFlipHorizontal-on.png");
			QPixmap pxmapOff(":/graphicGui/btFlipHorizontal-off.png");
			flipHoriz = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionHorizontal_Flip");
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
			QPixmap pxmapOn(":/graphicGui/btDSS-on.png");
			QPixmap pxmapOff(":/graphicGui/btDSS-off.png");
			btShowNebulaeBackground = new StelButton(NULL, pxmapOn, pxmapOff, pxmapGlow32x32, "actionShow_DSS");
		}
		getButtonBar()->addButton(btShowNebulaeBackground, "040-nebulaeGroup");
	} else {
		getButtonBar()->hideButton("actionShow_DSS");
	}
	flagShowNebulaBackgroundButton = b;
}

void StelGui::setFlagShowDecimalDegrees(bool b)
{
	StelApp::getInstance().setFlagShowDecimalDegrees(b);
	if (searchDialog->visible())
	{
		// Update format of input fields if Search Dialog is open
		searchDialog->populateCoordinateAxis();
	}
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

StelAction* StelGui::getAction(const QString& actionName)
{
	return StelApp::getInstance().getStelActionManager()->findAction(actionName);
}

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Process changes from the ConstellationMgr
#endif
/* ****************************************************************************************************************** */

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Process changes from the GridLinesMgr
#endif
/* ****************************************************************************************************************** */

/* ****************************************************************************************************************** */
#if 0
#pragma mark -
#pragma mark Process changes from the GridLinesMgr
#endif
/* ****************************************************************************************************************** */


void StelGui::copySelectedObjectInfo(void)
{
	QGuiApplication::clipboard()->setText(skyGui->infoPanel->getSelectedText());
}
