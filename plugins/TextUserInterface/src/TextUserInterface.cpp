/*
 * Copyright (C) 2009 Matthew Gates
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

#include "TextUserInterface.hpp"
#include "TuiNode.hpp"
#include "TuiNodeActivate.hpp"
#include "TuiNodeBool.hpp"
#include "TuiNodeInt.hpp"
#include "TuiNodeDouble.hpp"
#include "TuiNodeFloat.hpp"
#include "TuiNodeDateTime.hpp"
#include "TuiNodeColor.hpp"
#include "TuiNodeEnum.hpp"

#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StarMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "ConstellationMgr.hpp"
#include "NebulaMgr.hpp"
#include "SolarSystem.hpp"
#include "LandscapeMgr.hpp"
#include "GridLinesMgr.hpp"
#include "MilkyWay.hpp"
#include "StelLocation.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#ifndef DISABLE_SCRIPTING
#include "StelScriptMgr.hpp"
#endif
#include "StelGui.hpp"
#include "StelGuiItems.hpp"// Funny thing to include in a TEXT user interface...
#include "renderer/StelRenderer.hpp"


#include <QKeyEvent>
#include <QDebug>
#include <QLabel>
#include <QTime>
#include <QProcess>
#ifdef DISABLE_SCRIPTING
#include "QSettings" // WTF?
#endif


/*************************************************************************
 Utility functions
*************************************************************************/
QString colToConf(const Vec3f& c)
{
	return QString("%1,%2,%3").arg(c[0],2,'f',2).arg(c[1],2,'f',2).arg(c[2],2,'f',2);
}

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*************************************************************************/
StelModule* TextUserInterfaceStelPluginInterface::getStelModule() const
{
	return new TextUserInterface();
}

StelPluginInfo TextUserInterfaceStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "TextUserInterface";
	info.displayedName = N_("Text User Interface");
	info.authors = "Matthew Gates";
	info.contact = "http://porpoisehead.net/";
	info.description = N_("Plugin implementation of 0.9.x series Text User Interface (TUI), used in planetarium systems");
	return info;
}

Q_EXPORT_PLUGIN2(TextUserInterface, TextUserInterfaceStelPluginInterface)


/*************************************************************************
 Constructor
*************************************************************************/
TextUserInterface::TextUserInterface()
	: dummyDialog(this), tuiActive(false), currentNode(NULL)
{
	setObjectName("TextUserInterface");	
}

/*************************************************************************
 Destructor
*************************************************************************/
TextUserInterface::~TextUserInterface()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double TextUserInterface::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	if (actionName==StelModule::ActionHandleKeys)
		return -1;
	return 0;
}


/*************************************************************************
 Init our module
*************************************************************************/
void TextUserInterface::init()
{
	//Reusing translations: The translators will have to do less work if the
	//strings used here match strings used elsewhere. Do not change strings
	//unless you have a good reason. --BM
	
	StelCore* core = StelApp::getInstance().getCore();
	// Main config.
	loadConfiguration();
	//Reusing strings from the location dialog
	TuiNode* m1 = new TuiNode(N_("Location"));
	TuiNode* m1_1 = new TuiNodeDouble(N_("Latitude:"),
	                                  this, SLOT(setLatitude(double)),
	                                  getLatitude(), -180, 180, 0.5, m1);
	TuiNode* m1_2 = new TuiNodeDouble(N_("Longitude:"),
	                                  this, SLOT(setLongitude(double)),
	                                  getLongitude(), -180, 180, 0.5, m1, m1_1);
	TuiNode* m1_3 = new TuiNodeInt(N_("Altitude:"),
	                               this, SLOT(setAltitude(int)),
	                               core->getCurrentLocation().altitude,
	                               -200, 200000, 100, m1, m1_2);
	
	// TODO: Now new Solar System bodies can be added at runtime, so the list
	// needs to be populated every time this happens. --BM
	SolarSystem* solarSystem = GETSTELMODULE(SolarSystem);
	TuiNode* m1_4 = new TuiNodeEnum(N_("Solar System body"),
	                                this, SLOT(setHomePlanet(QString)),
	                                solarSystem->getAllPlanetEnglishNames(),
	                                core->getCurrentLocation().planetName,
	                                m1, m1_3);
	m1_1->setNextNode(m1_2);
	m1_2->setNextNode(m1_3);
	m1_3->setNextNode(m1_4);
	m1_4->setNextNode(m1_1);
	m1_1->loopToTheLast();
	m1->setChildNode(m1_1);

	TuiNode* m2 = new TuiNode(N_("Date and Time"), NULL, m1);
	m1->setNextNode(m2);
	TuiNode* m2_1 = new TuiNodeDateTime(N_("Current date/time"),
	                                    core,
	                                    SLOT(setJDay(double)),  
	                                    core->getJDay(),
	                                    m2);
	TuiNode* m2_2 = new TuiNode(N_("Set time zone"), m2, m2_1);
	TuiNode* m2_3 = new TuiNode(N_("Day keys"), m2, m2_2);
	TuiNode* m2_4 = new TuiNodeDateTime(N_("Startup date/time preset"),
										core,
	                                    SLOT(setPresetSkyTime(double)), 
										core->getPresetSkyTime(),
	                                    m2, m2_3);
	QStringList startupModes;
	// TRANSLATORS: The current system time is used at startup
	startupModes << N_("system");
	// TRANSLATORS: A pre-set time is used at startup
	startupModes << N_("preset");
	TuiNode* m2_5 = new TuiNodeEnum(N_("Startup date and time"),
	                                this, SLOT(setStartupDateMode(QString)),
	                                startupModes,
									core->getStartupTimeMode(),
	                                m2, m2_4);
	StelLocaleMgr& localeMgr = StelApp::getInstance().getLocaleMgr();
	QStringList dateFormats;
	dateFormats << "system_default" << N_("mmddyyyy") << N_("ddmmyyyy") << N_("yyyymmdd");
	TuiNode* m2_6 = new TuiNodeEnum(N_("Date display format"), //Used in Time Zone plugin
	                                this, SLOT(setDateFormat(QString)),
	                                dateFormats,
	                                localeMgr.getDateFormatStr(),
	                                m2, m2_5);
	QStringList timeFormats;
	timeFormats << "system_default";
	// TRANSLATORS: 12-hour time format
	timeFormats << N_("12h");
	// TRANSLATORS: 24-hour time format
	timeFormats << N_("24h");
	TuiNode* m2_7 = new TuiNodeEnum(N_("Time display format"), //Used in Time Zone plugin
	                                this, SLOT(setTimeFormat(QString)),
	                                timeFormats,
	                                localeMgr.getTimeFormatStr(),
	                                m2, m2_6);
	m2_1->setNextNode(m2_2);
	m2_2->setNextNode(m2_3);
	m2_3->setNextNode(m2_4);
	m2_4->setNextNode(m2_5);
	m2_5->setNextNode(m2_6);
	m2_6->setNextNode(m2_7);
	m2_7->setNextNode(m2_1);
	m2_1->loopToTheLast();
	m2->setChildNode(m2_1);

	TuiNode* m3 = new TuiNode(N_("General"), NULL, m2);
	m2->setNextNode(m3);
	StelSkyCultureMgr& skyCultureMgr = StelApp::getInstance().getSkyCultureMgr();
	TuiNode* m3_1 = new TuiNodeEnum(N_("Starlore"),
	                                this, 
	                                SLOT(setSkyCulture(QString)), 
	                                skyCultureMgr.getSkyCultureListI18(),
	                                skyCultureMgr.getCurrentSkyCultureNameI18(),
	                                m3);
	TuiNode* m3_2 = new TuiNodeEnum(N_("Language"),
	                                this, 
	                                SLOT(setAppLanguage(QString)), 
									StelTranslator::globalTranslator.getAvailableLanguagesNamesNative(StelFileMgr::getLocaleDir()),
					StelTranslator::iso639_1CodeToNativeName(localeMgr.getAppLanguage()),
	                                m3, m3_1);
	m3_1->setNextNode(m3_2);
	m3_2->setNextNode(m3_1);
	m3_1->loopToTheLast();
	m3->setChildNode(m3_1);

	TuiNode* m4 = new TuiNode(N_("Stars"), NULL, m3);
	m3->setNextNode(m4);
	StarMgr* starMgr = GETSTELMODULE(StarMgr);
	TuiNode* m4_1 = new TuiNodeBool(N_("Show stars"),
	                                starMgr, SLOT(setFlagStars(bool)), 
	                                starMgr->getFlagStars(), m4);
	StelSkyDrawer* skyDrawer = core->getSkyDrawer();
	TuiNode* m4_2 = new TuiNodeDouble(N_("Relative scale:"),
	                                  skyDrawer,
	                                  SLOT(setRelativeStarScale(double)),
	                                  skyDrawer->getRelativeStarScale(),
	                                  0.0, 5., 0.15,
	                                  m4, m4_1);
	TuiNode* m4_3 = new TuiNodeDouble(N_("Absolute scale:"),
	                                  skyDrawer,
	                                  SLOT(setAbsoluteStarScale(double)),
	                                  skyDrawer->getAbsoluteStarScale(),
	                                  0.0, 9., 0.15,
	                                  m4, m4_2);
	TuiNode* m4_4 = new TuiNodeDouble(N_("Twinkle:"),
	                                  skyDrawer, SLOT(setTwinkleAmount(double)),
	                                  skyDrawer->getTwinkleAmount(),
	                                  0.0, 1.5, 0.1,
	                                  m4, m4_3);
	m4_1->setNextNode(m4_2);
	m4_2->setNextNode(m4_3);
	m4_3->setNextNode(m4_4);
	m4_4->setNextNode(m4_1);
	m4_1->loopToTheLast();
	m4->setChildNode(m4_1);

	TuiNode* m5 = new TuiNode(N_("Colors"), NULL, m4);
	m4->setNextNode(m5);
	ConstellationMgr* constellationMgr = GETSTELMODULE(ConstellationMgr);
	TuiNode* m5_1 = new TuiNodeColor(N_("Constellation lines"),
	                                 constellationMgr,
	                                 SLOT(setLinesColor(Vec3f)),
	                                 constellationMgr->getLinesColor(), 
	                                 m5);
	TuiNode* m5_2 = new TuiNodeColor(N_("Constellation labels"),
	                                 constellationMgr,
					 SLOT(setLabelsColor(Vec3f)),
	                                 constellationMgr->getLabelsColor(), 
	                                 m5, m5_1);
	TuiNode* m5_3 = new TuiNode(N_("Constellation art"), m5, m5_2);
	TuiNode* m5_4 = new TuiNodeColor(N_("Constellation boundaries"),
	                                 constellationMgr,
	                                 SLOT(setBoundariesColor(Vec3f)),
	                                 constellationMgr->getBoundariesColor(), 
                                         m5, m5_3);
	// TRANSLATORS: Refers to constellation art
	TuiNode* m5_5 = new TuiNodeDouble(N_("Art brightness:"),
	                                  constellationMgr,
	                                  SLOT(setArtIntensity(double)),
	                                  constellationMgr->getArtIntensity(),
	                                  0.0, 1.0, 0.05,
	                                  m5, m5_4);
	LandscapeMgr* landscapeMgr = GETSTELMODULE(LandscapeMgr);
	TuiNode* m5_6 = new TuiNodeColor(N_("Cardinal points"),
	                                 landscapeMgr,
	                                 SLOT(setColorCardinalPoints(Vec3f)),
	                                 landscapeMgr->getColorCardinalPoints(), 
	                                 m5, m5_5);
	TuiNode* m5_7 = new TuiNodeColor(N_("Planet labels"),
	                                 solarSystem, SLOT(setLabelsColor(Vec3f)),
	                                 solarSystem->getLabelsColor(), 
	                                 m5, m5_6);
	TuiNode* m5_8 = new TuiNodeColor(N_("Planet orbits"),
	                                 solarSystem, SLOT(setOrbitsColor(Vec3f)),
	                                 solarSystem->getOrbitsColor(), 
	                                 m5, m5_7);
	TuiNode* m5_9 = new TuiNodeColor(N_("Planet trails"),
	                                 solarSystem, SLOT(setTrailsColor(Vec3f)),
	                                 solarSystem->getTrailsColor(), 
	                                 m5, m5_8);
	GridLinesMgr* gridLinesMgr = GETSTELMODULE(GridLinesMgr);
	TuiNode* m5_10 = new TuiNodeColor(N_("Meridian line"),
	                                 gridLinesMgr,
	                                 SLOT(setColorMeridianLine(Vec3f)),
	                                 gridLinesMgr->getColorMeridianLine(), 
	                                 m5, m5_9);
	TuiNode* m5_11 = new TuiNodeColor(N_("Azimuthal grid"),
	                                 gridLinesMgr,
	                                 SLOT(setColorAzimuthalGrid(Vec3f)),
	                                 gridLinesMgr->getColorAzimuthalGrid(), 
	                                 m5, m5_10);
	TuiNode* m5_12 = new TuiNodeColor(N_("Equatorial grid"),
	                                 gridLinesMgr,
	                                 SLOT(setColorEquatorGrid(Vec3f)),
	                                 gridLinesMgr->getColorEquatorGrid(), 
	                                 m5, m5_11);
	TuiNode* m5_13 = new TuiNodeColor(N_("Equatorial J2000 grid"),
	                                 gridLinesMgr,
	                                 SLOT(setColorEquatorJ2000Grid(Vec3f)),
	                                 gridLinesMgr->getColorEquatorJ2000Grid(), 
	                                 m5, m5_12);
	TuiNode* m5_14 = new TuiNodeColor(N_("Equator line"),
	                                 gridLinesMgr,
	                                 SLOT(setColorEquatorLine(Vec3f)),
	                                 gridLinesMgr->getColorEquatorLine(), 
	                                 m5, m5_13);
	TuiNode* m5_15 = new TuiNodeColor(N_("Ecliptic line"),
	                                 gridLinesMgr,
	                                 SLOT(setColorEclipticLine(Vec3f)),
	                                 gridLinesMgr->getColorEclipticLine(), 
					 m5, m5_14);
	NebulaMgr* nebulaMgr = GETSTELMODULE(NebulaMgr);
	TuiNode* m5_16 = new TuiNodeColor(N_("Nebula names"),
	                                 nebulaMgr, SLOT(setLabelsColor(Vec3f)),
	                                 nebulaMgr->getLabelsColor(), 
					 m5, m5_15);
	TuiNode* m5_17 = new TuiNodeColor(N_("Nebula hints"),
	                                  nebulaMgr, SLOT(setCirclesColor(Vec3f)),
	                                  nebulaMgr->getCirclesColor(), 
					  m5, m5_16);
	TuiNode* m5_18 = new TuiNodeColor(N_("Horizon line"),
					 gridLinesMgr,
					 SLOT(setColorHorizonLine(Vec3f)),
					 gridLinesMgr->getColorHorizonLine(),
					 m5, m5_17);
	TuiNode* m5_19 = new TuiNodeColor(N_("Galactic grid"),
					 gridLinesMgr,
					 SLOT(setColorGalacticGrid(Vec3f)),
					 gridLinesMgr->getColorGalacticGrid(),
					 m5, m5_18);
	TuiNode* m5_20 = new TuiNodeColor(N_("Galactic plane line"),
					 gridLinesMgr,
					 SLOT(setColorGalacticPlaneLine(Vec3f)),
					 gridLinesMgr->getColorGalacticPlaneLine(),
					 m5, m5_19);

	m5_1->setNextNode(m5_2);
	m5_2->setNextNode(m5_3);
	m5_3->setNextNode(m5_4);
	m5_4->setNextNode(m5_5);
	m5_5->setNextNode(m5_6);
	m5_6->setNextNode(m5_7);
	m5_7->setNextNode(m5_8);
	m5_8->setNextNode(m5_9);
	m5_9->setNextNode(m5_10);
	m5_10->setNextNode(m5_11);
	m5_11->setNextNode(m5_12);
	m5_12->setNextNode(m5_13);
	m5_13->setNextNode(m5_14);
	m5_14->setNextNode(m5_15);
	m5_15->setNextNode(m5_16);
	m5_16->setNextNode(m5_17);
	m5_17->setNextNode(m5_18);
	m5_18->setNextNode(m5_19);
	m5_19->setNextNode(m5_20);
	m5_20->setNextNode(m5_1);
	m5_1->loopToTheLast();
	m5->setChildNode(m5_1);

	TuiNode* m6 = new TuiNode(N_("Effects"), NULL, m5);
	m5->setNextNode(m6);
	TuiNode* m6_1 = new TuiNodeInt(N_("Light pollution:"),
				       this,
				       SLOT(setBortleScale(int)),
	                               3, 1, 9, 1,
	                               m6);
	TuiNode* m6_2 = new TuiNodeEnum(N_("Landscape"),
	                                landscapeMgr,
	                                SLOT(setCurrentLandscapeName(QString)),
	                                landscapeMgr->getAllLandscapeNames(),
	                                landscapeMgr->getCurrentLandscapeName(),
	                                m6, m6_1);
	StelMovementMgr* movementMgr = GETSTELMODULE(StelMovementMgr);
	TuiNode* m6_3 = new TuiNodeBool(N_("Manual zoom"),
	                                movementMgr,
	                                SLOT(setFlagAutoZoomOutResetsDirection(bool)), 
	                                movementMgr->getFlagAutoZoomOutResetsDirection(), 
	                                m6, m6_2);
	TuiNode* m6_4 = new TuiNode(N_("Magnitude scaling multiplier"),
	                                    m6, m6_3);
	TuiNode* m6_5 = new TuiNodeDouble(N_("Milky Way intensity:"),
	                                 GETSTELMODULE(MilkyWay),
					 SLOT(setIntensity(double)),
	                                 GETSTELMODULE(MilkyWay)->getIntensity(),
	                                 0, 10.0, 0.1, 
	                                 m6, m6_4);
	TuiNode* m6_6 = new TuiNode(N_("Nebula label frequency:"), m6, m6_5);
	TuiNode* m6_7 = new TuiNodeFloat(N_("Zoom duration:"),
	                                 movementMgr,
	                                 SLOT(setAutoMoveDuration(float)), 
	                                 movementMgr->getAutoMoveDuration(),
	                                 0, 20.0, 0.1,
	                                 m6, m6_6);
	TuiNode* m6_8 = new TuiNode(N_("Cursor timeout:"), m6, m6_7);
	TuiNode* m6_9 = new TuiNodeBool(N_("Setting landscape sets location"),
	                                landscapeMgr,
	                                SLOT(setFlagLandscapeSetsLocation(bool)), 
	                                landscapeMgr->getFlagLandscapeSetsLocation(), 
	                                m6, m6_8);
	m6_1->setNextNode(m6_2);
	m6_2->setNextNode(m6_3);
	m6_3->setNextNode(m6_4);
	m6_4->setNextNode(m6_5);
	m6_5->setNextNode(m6_6);
	m6_6->setNextNode(m6_7);
	m6_7->setNextNode(m6_8);
	m6_8->setNextNode(m6_9);
	m6_9->setNextNode(m6_1);
	m6_1->loopToTheLast();
	m6->setChildNode(m6_1);

	#ifndef DISABLE_SCRIPTING
	TuiNode* m7 = new TuiNode(N_("Scripts"), NULL, m6);
	m6->setNextNode(m7);	
	StelScriptMgr& scriptMgr = StelMainGraphicsView::getInstance().getScriptMgr();
	TuiNode* m7_1 = new TuiNodeEnum(N_("Run local script"),
	                                &scriptMgr,
	                                SLOT(runScript(QString)),
	                                scriptMgr.getScriptList(),
	                                "",
	                                m7);
	TuiNode* m7_2 = new TuiNodeActivate(N_("Stop running script"),
	                                    &scriptMgr, SLOT(stopScript()),
	                                    m7, m7_1);
	TuiNode* m7_3 = new TuiNode(N_("CD/DVD script"), m7, m7_2);
	m7_1->setNextNode(m7_2);
	m7_2->setNextNode(m7_3);
	m7_3->setNextNode(m7_1);
	m7_1->loopToTheLast();
	m7->setChildNode(m7_1);


	TuiNode* m8 = new TuiNode(N_("Administration"), NULL, m7);
	m7->setNextNode(m8);
	#endif
	#ifdef DISABLE_SCRIPTING
	TuiNode* m8 = new TuiNode(N_("Administration"), NULL, m6);
	m6->setNextNode(m8);
	#endif
	m8->setNextNode(m1);
	m1->loopToTheLast();
	TuiNode* m8_1 = new TuiNode(N_("Load default configuration"), m8);
	TuiNode* m8_2 = new TuiNodeActivate(N_("Save current configuration"),
	                                    this, SLOT(saveDefaultSettings()),
	                                    m8, m8_1);
	TuiNode* m8_3 = new TuiNodeActivate(N_("Shut down"), this, SLOT(shutDown()), 
					    m8, m8_2);
	m8_1->setNextNode(m8_2);
	m8_2->setNextNode(m8_3);
	m8_3->setNextNode(m8_1);
	m8_1->loopToTheLast();
	m8->setChildNode(m8_1);


	currentNode = m1;
}

/*************************************************************************
 Load settings from configuration file.
*************************************************************************/
void TextUserInterface::loadConfiguration(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	font.setPixelSize(conf->value("tui/tui_font_size", 15).toInt());
	tuiDateTime = conf->value("tui/flag_show_tui_datetime", false).toBool();
	tuiObjInfo = conf->value("tui/flag_show_tui_short_obj_info", false).toBool();
	tuiGravityUi = conf->value("tui/flag_show_gravity_ui", false).toBool();
}

/*************************************************************************
 Draw our module.
*************************************************************************/
void TextUserInterface::draw(StelCore* core, StelRenderer* renderer)
{
	if (!tuiActive && !tuiDateTime && !tuiObjInfo)
		return;

	int x = 0, y = 0;
	int xVc = 0, yVc = 0;
	int pixOffset = 15;
	int fovOffsetX = 0, fovOffsetY=0;
	bool fovMaskDisk = false;

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui->getVisible())
	{
		QGraphicsItem* bottomBar = dynamic_cast<QGraphicsItem*>(gui->getButtonBar());
		LeftStelBar* sideBar = gui->getWindowsButtonBar();			
		x = (sideBar) ? sideBar->boundingRectNoHelpLabel().right() : 50;
		y = (bottomBar) ? bottomBar->boundingRect().height() : 50;
	}

	// Alternate x,y for Disk viewport
	if (core->getProjection(StelCore::FrameJ2000)->getMaskType() == StelProjector::MaskDisk)
	{
		fovMaskDisk = true;
		StelProjector::StelProjectorParams projParams = core->getCurrentStelProjectorParams();
		xVc = projParams.viewportCenter[0];
		yVc = projParams.viewportCenter[1];
		fovOffsetX = projParams.viewportFovDiameter*std::sin(20)/2;
		fovOffsetY = projParams.viewportFovDiameter*std::cos(20)/2;
	}
	else 
	{
		xVc = core->getProjection(StelCore::FrameJ2000)->getViewportWidth()/2;
	}

	if (tuiActive)
	{
		int text_x = x + pixOffset, text_y = y + pixOffset;
		if (fovMaskDisk) {
			text_x = xVc - fovOffsetX + pixOffset;
			text_y = yVc - fovOffsetY + pixOffset;
		}
			
		QString tuiText = q_("[no TUI node]");
		if (currentNode!=NULL) {
			tuiText = currentNode->getDisplayText();
		}

		renderer->setFont(font);
		renderer->setGlobalColor(0.3f, 1.0f, 0.3f);
		TextParams params = TextParams(text_x, text_y, tuiText)
		                    .projector(core->getProjection(StelCore::FrameJ2000));
		if(tuiGravityUi){params.useGravity();}
		renderer->drawText(params);
	}

	if (tuiDateTime) 
	{
		double jd = core->getJDay();
		int text_x = x + xVc*2/3, text_y = y + pixOffset;

		QString newDate = StelApp::getInstance().getLocaleMgr().getPrintableDateLocal(jd) + "   "
                       +StelApp::getInstance().getLocaleMgr().getPrintableTimeLocal(jd);
		 
		if (fovMaskDisk) {
			text_x = xVc + fovOffsetY - pixOffset;
			text_y = yVc - fovOffsetX + pixOffset;
		}

		renderer->setFont(font);
		renderer->setGlobalColor(0.3f, 1.0f, 0.3f);
		TextParams params = TextParams(text_x, text_y, newDate)
		                    .projector(core->getProjection(StelCore::FrameAltAz));
		if(tuiGravityUi){params.useGravity();}
		renderer->drawText(params);
	}

	if (tuiObjInfo) 
	{
		QString objInfo = ""; 
		StelObject::InfoStringGroup tuiInfo(StelObject::Name|StelObject::CatalogNumber
				|StelObject::Distance|StelObject::PlainText);
		int text_x = x + xVc*4/3, text_y = y + pixOffset; 

		QList<StelObjectP> selectedObj = GETSTELMODULE(StelObjectMgr)->getSelectedObject();
		if (selectedObj.isEmpty()) {
			objInfo = "";	
		} else {
			objInfo = selectedObj[0]->getInfoString(core, tuiInfo);
			objInfo.replace("\n"," ");
			objInfo.replace("Distance:"," ");
			objInfo.replace("Light Years","ly");
		}

		if (fovMaskDisk) {
			text_x = xVc + fovOffsetX - pixOffset;
			text_y = yVc + fovOffsetY - pixOffset;
		}

		renderer->setFont(font);
		renderer->setGlobalColor(0.3f, 1.0f, 0.3f);
		TextParams params = TextParams(text_x, text_y, objInfo)
		                    .projector(core->getProjection(StelCore::FrameJ2000));
		if(tuiGravityUi){params.useGravity();}
		renderer->drawText(params);
	}
}

void TextUserInterface::handleKeys(QKeyEvent* event)
{
	if (currentNode == NULL)
	{
		qWarning() << "WARNING: no current node in TextUserInterface plugin";
		event->setAccepted(false);
		return;
	}

	if (event->type()==QEvent::KeyPress && event->key()==Qt::Key_M)
	{
		tuiActive = ! tuiActive;
		dummyDialog.close();
		event->setAccepted(true);
		return;
	}

	if (!tuiActive)
	{
		event->setAccepted(false);
		return;
	}

	if (event->type()==QEvent::KeyPress)
	{
		TuiNodeResponse response = currentNode->handleKey(event->key());
		if (response.accepted)
		{
			currentNode = response.newNode;
			event->setAccepted(true);
		}
		else
		{
			event->setAccepted(false);
		}
		return;
	}
}

void TextUserInterface::setHomePlanet(QString planetName)
{
	StelCore* core = StelApp::getInstance().getCore();
	if (core->getCurrentLocation().planetName != planetName)
	{
		StelLocation newLocation = core->getCurrentLocation();
		newLocation.planetName = planetName;
		core->moveObserverTo(newLocation);
	}
}

void TextUserInterface::setAltitude(int altitude)
{
	StelCore* core = StelApp::getInstance().getCore();
	if (core->getCurrentLocation().altitude != altitude)
	{
		StelLocation newLocation = core->getCurrentLocation();
		newLocation.altitude = altitude;
		core->moveObserverTo(newLocation, 0.0, 0.0);
	}
}

void TextUserInterface::setLatitude(double latitude)
{
	StelCore* core = StelApp::getInstance().getCore();
	if (core->getCurrentLocation().latitude != latitude)
	{
		StelLocation newLocation = core->getCurrentLocation();
		newLocation.latitude = latitude;
		core->moveObserverTo(newLocation, 0.0, 0.0);
	}
}

void TextUserInterface::setLongitude(double longitude)
{
	StelCore* core = StelApp::getInstance().getCore();
	if (core->getCurrentLocation().longitude != longitude)
	{
		StelLocation newLocation = core->getCurrentLocation();
		newLocation.longitude = longitude;
		core->moveObserverTo(newLocation, 0.0, 0.0);
	}
}

double TextUserInterface::getLatitude(void)
{
	return StelApp::getInstance().getCore()->getCurrentLocation().latitude;
}

double TextUserInterface::getLongitude(void)
{
	return StelApp::getInstance().getCore()->getCurrentLocation().longitude;
}

void TextUserInterface::setStartupDateMode(QString mode)
{
	StelApp::getInstance().getCore()->setStartupTimeMode(mode);
}

void TextUserInterface::setDateFormat(QString format)
{
	StelApp::getInstance().getLocaleMgr().setDateFormatStr(format);
}

void TextUserInterface::setTimeFormat(QString format)
{
	StelApp::getInstance().getLocaleMgr().setTimeFormatStr(format);
}

void TextUserInterface::setSkyCulture(QString i18)
{
	StelApp::getInstance().getSkyCultureMgr().setCurrentSkyCultureNameI18(i18);
}

void TextUserInterface::setAppLanguage(QString lang)
{
	QString code = StelTranslator::nativeNameToIso639_1Code(lang);
	StelApp::getInstance().getLocaleMgr().setAppLanguage(code);
	StelApp::getInstance().getLocaleMgr().setSkyLanguage(code);
}

void TextUserInterface::saveDefaultSettings(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	Q_ASSERT(lmgr);
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	Q_ASSERT(ssmgr);
	StelSkyDrawer* skyd = StelApp::getInstance().getCore()->getSkyDrawer();
	Q_ASSERT(skyd);
	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
	Q_ASSERT(cmgr);
	StarMgr* smgr = GETSTELMODULE(StarMgr);
	Q_ASSERT(smgr);
	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	Q_ASSERT(nmgr);
	GridLinesMgr* glmgr = GETSTELMODULE(GridLinesMgr);
	Q_ASSERT(glmgr);
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);
	StelCore* core = StelApp::getInstance().getCore();
	Q_ASSERT(core);
	MilkyWay* milk = GETSTELMODULE(MilkyWay);
	Q_ASSERT(milk);
	const StelProjectorP proj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000);
	Q_ASSERT(proj);
	StelLocaleMgr& lomgr = StelApp::getInstance().getLocaleMgr();

	// MENU ITEMS
	// sub-menu 1: location
	// TODO
	

	// sub-menu 2: date and time
	conf->setValue("navigation/preset_sky_time", core->getPresetSkyTime());
	conf->setValue("navigation/startup_time_mode", core->getStartupTimeMode());
	conf->setValue("navigation/startup_time_mode", core->getStartupTimeMode());
	conf->setValue("localization/time_display_format", lomgr.getTimeFormatStr());
	conf->setValue("localization/date_display_format", lomgr.getDateFormatStr());


	// sub-menu 3: general
	StelApp::getInstance().getSkyCultureMgr().setDefaultSkyCultureID(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID());
	QString langName = lomgr.getAppLanguage();
	conf->setValue("localization/app_locale", StelTranslator::nativeNameToIso639_1Code(langName));
	langName = StelApp::getInstance().getLocaleMgr().getSkyLanguage();
	conf->setValue("localization/sky_locale", StelTranslator::nativeNameToIso639_1Code(langName));

	// sub-menu 4: stars
	conf->setValue("astro/flag_stars", smgr->getFlagStars());
	conf->setValue("stars/absolute_scale", skyd->getAbsoluteStarScale());
	conf->setValue("stars/relative_scale", skyd->getRelativeStarScale());
	conf->setValue("stars/flag_star_twinkle", skyd->getFlagTwinkle());

	// sub-menu 5: colors
	conf->setValue("color/const_lines_color", colToConf(cmgr->getLinesColor()));
 	conf->setValue("color/const_names_color", colToConf(cmgr->getLabelsColor()));
	conf->setValue("color/const_boundary_color", colToConf(cmgr->getBoundariesColor()));
	conf->setValue("viewing/constellation_art_intensity", cmgr->getArtIntensity());
	conf->setValue("color/cardinal_color", colToConf(lmgr->getColorCardinalPoints()) );
	conf->setValue("color/planet_names_color", colToConf(ssmgr->getLabelsColor()));
	conf->setValue("color/planet_orbits_color", colToConf(ssmgr->getOrbitsColor()));
	conf->setValue("color/object_trails_color", colToConf(ssmgr->getTrailsColor()));
	conf->setValue("color/meridian_color", colToConf(glmgr->getColorMeridianLine()));
	conf->setValue("color/azimuthal_color", colToConf(glmgr->getColorAzimuthalGrid()));
	conf->setValue("color/equator_color", colToConf(glmgr->getColorEquatorGrid()));
	conf->setValue("color/equatorial_J2000_color", colToConf(glmgr->getColorEquatorJ2000Grid()));
	conf->setValue("color/equator_color", colToConf(glmgr->getColorEquatorLine()));
	conf->setValue("color/ecliptic_color", colToConf(glmgr->getColorEclipticLine()));
	conf->setValue("color/nebula_label_color", colToConf(nmgr->getLabelsColor()));
	conf->setValue("color/nebula_circle_color", colToConf(nmgr->getCirclesColor()));

	// sub-menu 6: effects
	conf->setValue("stars/init_bortle_scale", lmgr->getAtmosphereBortleLightPollution());
	lmgr->setDefaultLandscapeID(lmgr->getCurrentLandscapeID());
	conf->setValue("navigation/auto_zoom_out_resets_direction", mvmgr->getFlagAutoZoomOutResetsDirection());
	conf->setValue("astro/milky_way_intensity", milk->getIntensity());
	conf->setValue("navigation/auto_move_duration", mvmgr->getAutoMoveDuration());
	conf->setValue("landscape/flag_landscape_sets_location", lmgr->getFlagLandscapeSetsLocation());

	// GLOBAL DISPLAY SETTINGS
	// TODO 
	
	qDebug() << "TextUserInterface::saveDefaultSettings done";
}

void TextUserInterface::shutDown()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	QString shutdownCmd = conf->value("tui/admin_shutdown_cmd", "").toString();
	int err; 
	if (!(err = QProcess::execute(shutdownCmd))) {
		qDebug() << "[TextUserInterface] shutdown error, QProcess::execute():" << err;
	}
}

void TextUserInterface::setBortleScale(int bortle)
{
	LandscapeMgr* landscapeMgr = GETSTELMODULE(LandscapeMgr);
	StelSkyDrawer* skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();
	landscapeMgr->setAtmosphereBortleLightPollution(bortle);
	skyDrawer->setBortleScale(bortle);
}
