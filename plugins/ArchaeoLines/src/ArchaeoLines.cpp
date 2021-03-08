/*
 * Copyright (C) 2014 Georg Zotti
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

#include "StelUtils.hpp"
#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelVertexArray.hpp"
#include "ArchaeoLines.hpp"
#include "ArchaeoLinesDialog.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"

#include <QDebug>
#include <QTimer>
#include <QPixmap>
#include <QColor>
#include <QSettings>
#include <QMouseEvent>
#include <cmath>
#include <stdexcept>

//! This method is the one called automatically by the StelModuleMgr just
//! after loading the dynamic library
StelModule* ArchaeoLinesStelPluginInterface::getStelModule() const
{
	return new ArchaeoLines();
}

StelPluginInfo ArchaeoLinesStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(ArchaeoLines);

	StelPluginInfo info;
	info.id = "ArchaeoLines";
	info.displayedName = N_("ArchaeoLines");
	info.authors = "Georg Zotti";
	info.contact = "https://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("A tool for archaeo-/ethnoastronomical alignment studies");
	info.version = ARCHAEOLINES_PLUGIN_VERSION;
	info.license = ARCHAEOLINES_PLUGIN_LICENSE;
	return info;
}

ArchaeoLines::ArchaeoLines()
	: flagShowArchaeoLines(false)
	, lineWidth(1)
	, flagShowEquinox(false)
	, flagShowSolstices(false)
	, flagShowCrossquarters(false)
	, flagShowMajorStandstills(false)
	, flagShowMinorStandstills(false)
	, flagShowZenithPassage(false)
	, flagShowNadirPassage(false)
	, flagShowSelectedObject(false)
	, flagShowSelectedObjectAzimuth(false)
	, flagShowSelectedObjectHourAngle(false)
	, flagShowCurrentSun(false)
	, flagShowCurrentMoon(false)
	, enumShowCurrentPlanet(ArchaeoLine::CurrentPlanetNone)
	, flagShowGeographicLocation1(false)
	, geographicLocation1Longitude(39.8) // approx. Mecca
	, geographicLocation1Latitude(21.4)
	, flagShowGeographicLocation2(false)
	, geographicLocation2Longitude(35.2) // approx. Jerusalem
	, geographicLocation2Latitude(31.8)
	, flagShowCustomAzimuth1(false)
	, flagShowCustomAzimuth2(false)
	//, customAzimuth1(0.0)
	//, customAzimuth2(0.0)
	, flagShowCustomDeclination1(false)
	, flagShowCustomDeclination2(false)
	, lastJDE(0.0)
	, toolbarButton(Q_NULLPTR)
{
	setObjectName("ArchaeoLines");
	font.setPixelSize(16);
	core=StelApp::getInstance().getCore();
	Q_ASSERT(core);
	objMgr=GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(objMgr);

	// optimize readability so that each upper line of the lunistice doubles is labeled.
	equinoxLine = new ArchaeoLine(ArchaeoLine::Equinox, 0.0);
	northernSolsticeLine = new ArchaeoLine(ArchaeoLine::Solstices, 23.50);
	southernSolsticeLine = new ArchaeoLine(ArchaeoLine::Solstices, -23.50);
	northernCrossquarterLine = new ArchaeoLine(ArchaeoLine::Crossquarters, 16.50);
	southernCrossquarterLine = new ArchaeoLine(ArchaeoLine::Crossquarters, -16.50);
	northernMajorStandstillLine0 = new ArchaeoLine(ArchaeoLine::MajorStandstill, 23.5+5.1);
	northernMajorStandstillLine1 = new ArchaeoLine(ArchaeoLine::MajorStandstill, 23.5+5.1);
	northernMajorStandstillLine0->setLabelVisible(false);
	northernMinorStandstillLine2 = new ArchaeoLine(ArchaeoLine::MinorStandstill, 23.5-5.1);
	northernMinorStandstillLine3 = new ArchaeoLine(ArchaeoLine::MinorStandstill, 23.5-5.1);
	northernMinorStandstillLine2->setLabelVisible(false);
	southernMinorStandstillLine4 = new ArchaeoLine(ArchaeoLine::MinorStandstill, -23.5+5.1);
	southernMinorStandstillLine5 = new ArchaeoLine(ArchaeoLine::MinorStandstill, -23.5+5.1);
	southernMinorStandstillLine4->setLabelVisible(false);
	southernMajorStandstillLine6 = new ArchaeoLine(ArchaeoLine::MajorStandstill, -23.5-5.1);
	southernMajorStandstillLine7 = new ArchaeoLine(ArchaeoLine::MajorStandstill, -23.5-5.1);
	southernMajorStandstillLine6->setLabelVisible(false);
	zenithPassageLine  = new ArchaeoLine(ArchaeoLine::ZenithPassage, 48.0);
	nadirPassageLine   = new ArchaeoLine(ArchaeoLine::NadirPassage, 42.0);
	selectedObjectLine = new ArchaeoLine(ArchaeoLine::SelectedObject, 0.0);
	selectedObjectAzimuthLine = new ArchaeoLine(ArchaeoLine::SelectedObjectAzimuth, 0.0);
	selectedObjectHourAngleLine = new ArchaeoLine(ArchaeoLine::SelectedObjectHourAngle, 0.0);
	currentSunLine     = new ArchaeoLine(ArchaeoLine::CurrentSun, 0.0);
	currentMoonLine    = new ArchaeoLine(ArchaeoLine::CurrentMoon, 0.0);
	currentPlanetLine  = new ArchaeoLine(ArchaeoLine::CurrentPlanetNone, 0.0);
	geographicLocation1Line = new ArchaeoLine(ArchaeoLine::GeographicLocation1, 0.0);
	geographicLocation2Line = new ArchaeoLine(ArchaeoLine::GeographicLocation2, 0.0);
	customAzimuth1Line = new ArchaeoLine(ArchaeoLine::CustomAzimuth1, 0.0);
	customAzimuth2Line = new ArchaeoLine(ArchaeoLine::CustomAzimuth2, 0.0);
	customDeclination1Line = new ArchaeoLine(ArchaeoLine::CustomDeclination1, 0.0);
	customDeclination2Line = new ArchaeoLine(ArchaeoLine::CustomDeclination2, 0.0);

	configDialog = new ArchaeoLinesDialog();
	conf = StelApp::getInstance().getSettings();

	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(updateObserverLocation(StelLocation)));
}

ArchaeoLines::~ArchaeoLines()
{
	delete equinoxLine; equinoxLine=Q_NULLPTR;
	delete northernSolsticeLine; northernSolsticeLine=Q_NULLPTR;
	delete southernSolsticeLine; southernSolsticeLine=Q_NULLPTR;
	delete northernCrossquarterLine; northernCrossquarterLine=Q_NULLPTR;
	delete southernCrossquarterLine; southernCrossquarterLine=Q_NULLPTR;
	delete northernMajorStandstillLine0; northernMajorStandstillLine0=Q_NULLPTR;
	delete northernMajorStandstillLine1; northernMajorStandstillLine1=Q_NULLPTR;
	delete northernMinorStandstillLine2; northernMinorStandstillLine2=Q_NULLPTR;
	delete northernMinorStandstillLine3; northernMinorStandstillLine3=Q_NULLPTR;
	delete southernMinorStandstillLine4; southernMinorStandstillLine4=Q_NULLPTR;
	delete southernMinorStandstillLine5; southernMinorStandstillLine5=Q_NULLPTR;
	delete southernMajorStandstillLine6; southernMajorStandstillLine6=Q_NULLPTR;
	delete southernMajorStandstillLine7; southernMajorStandstillLine7=Q_NULLPTR;
	delete zenithPassageLine;  zenithPassageLine=Q_NULLPTR;
	delete nadirPassageLine;   nadirPassageLine=Q_NULLPTR;
	delete selectedObjectLine; selectedObjectLine=Q_NULLPTR;
	delete selectedObjectAzimuthLine; selectedObjectAzimuthLine=Q_NULLPTR;
	delete selectedObjectHourAngleLine; selectedObjectHourAngleLine=Q_NULLPTR;
	delete currentSunLine;     currentSunLine=Q_NULLPTR;
	delete currentMoonLine;    currentMoonLine=Q_NULLPTR;
	delete currentPlanetLine;  currentPlanetLine=Q_NULLPTR;
	delete geographicLocation1Line; geographicLocation1Line=Q_NULLPTR;
	delete geographicLocation2Line; geographicLocation2Line=Q_NULLPTR;
	delete customAzimuth1Line; customAzimuth1Line=Q_NULLPTR;
	delete customAzimuth2Line; customAzimuth2Line=Q_NULLPTR;
	delete customDeclination1Line; customDeclination1Line=Q_NULLPTR;
	delete customDeclination2Line; customDeclination2Line=Q_NULLPTR;

	delete configDialog; configDialog=Q_NULLPTR;
}

bool ArchaeoLines::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

//! Determine which "layer" the plugin's drawing will happen on.
double ArchaeoLines::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
	  return StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr")->getCallOrder(actionName)+1.; // one after GridlineMgr: else its equator covers our equinox line!
	return 0;
}

void ArchaeoLines::init()
{
	Q_ASSERT(equinoxLine);
	Q_ASSERT(northernSolsticeLine);
	Q_ASSERT(southernSolsticeLine);
	Q_ASSERT(northernCrossquarterLine);
	Q_ASSERT(southernCrossquarterLine);
	Q_ASSERT(northernMajorStandstillLine0);
	Q_ASSERT(northernMajorStandstillLine1);
	Q_ASSERT(northernMinorStandstillLine2);
	Q_ASSERT(northernMinorStandstillLine3);
	Q_ASSERT(southernMinorStandstillLine4);
	Q_ASSERT(southernMinorStandstillLine5);
	Q_ASSERT(southernMajorStandstillLine6);
	Q_ASSERT(southernMajorStandstillLine7);
	Q_ASSERT(zenithPassageLine);
	Q_ASSERT(nadirPassageLine);
	Q_ASSERT(selectedObjectLine);
	Q_ASSERT(selectedObjectAzimuthLine);
	Q_ASSERT(selectedObjectHourAngleLine);
	Q_ASSERT(currentSunLine);
	Q_ASSERT(currentMoonLine);
	Q_ASSERT(currentPlanetLine);
	Q_ASSERT(geographicLocation1Line);
	Q_ASSERT(geographicLocation2Line);
	Q_ASSERT(customAzimuth1Line);
	Q_ASSERT(customAzimuth2Line);
	Q_ASSERT(customDeclination1Line);
	Q_ASSERT(customDeclination2Line);

	connect(this, SIGNAL(equinoxColorChanged(Vec3f)),                equinoxLine                 , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(solsticesColorChanged(Vec3f)),              northernSolsticeLine        , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(solsticesColorChanged(Vec3f)),              southernSolsticeLine        , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(crossquartersColorChanged(Vec3f)),          northernCrossquarterLine    , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(crossquartersColorChanged(Vec3f)),          southernCrossquarterLine    , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(majorStandstillColorChanged(Vec3f)),        northernMajorStandstillLine0, SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(majorStandstillColorChanged(Vec3f)),        northernMajorStandstillLine1, SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(majorStandstillColorChanged(Vec3f)),        southernMajorStandstillLine6, SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(majorStandstillColorChanged(Vec3f)),        southernMajorStandstillLine7, SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(minorStandstillColorChanged(Vec3f)),        northernMinorStandstillLine2, SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(minorStandstillColorChanged(Vec3f)),        northernMinorStandstillLine3, SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(minorStandstillColorChanged(Vec3f)),        southernMinorStandstillLine4, SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(minorStandstillColorChanged(Vec3f)),        southernMinorStandstillLine5, SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(zenithPassageColorChanged(Vec3f)),          zenithPassageLine           , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(nadirPassageColorChanged(Vec3f)),           nadirPassageLine            , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(selectedObjectColorChanged(Vec3f)),         selectedObjectLine          , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(selectedObjectAzimuthColorChanged(Vec3f)),  selectedObjectAzimuthLine   , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(selectedObjectHourAngleColorChanged(Vec3f)),selectedObjectHourAngleLine , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(currentSunColorChanged(Vec3f)),             currentSunLine              , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(currentMoonColorChanged(Vec3f)),            currentMoonLine             , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(currentPlanetColorChanged(Vec3f)),          currentPlanetLine           , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(geographicLocation1ColorChanged(Vec3f)),    geographicLocation1Line     , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(geographicLocation2ColorChanged(Vec3f)),    geographicLocation2Line     , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(customAzimuth1ColorChanged(Vec3f)),         customAzimuth1Line          , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(customAzimuth2ColorChanged(Vec3f)),         customAzimuth2Line          , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(customDeclination1ColorChanged(Vec3f)),     customDeclination1Line      , SLOT(setColor(Vec3f)));
	connect(this, SIGNAL(customDeclination2ColorChanged(Vec3f)),     customDeclination2Line      , SLOT(setColor(Vec3f)));

	loadSettings();

	// Create action for enable/disable & hook up signals
	QString section=N_("ArchaeoLines");
	addAction("actionShow_ArchaeoLines",         section, N_("ArchaeoLines"), "enabled", "Ctrl+U");
	addAction("actionShow_ArchaeoLines_dialog",  section, N_("Show settings dialog"),  configDialog,  "visible",           "Ctrl+Shift+U");

	// Add a toolbar button
	StelApp& app=StelApp::getInstance();
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
		if (gui!=Q_NULLPTR)
		{
			toolbarButton = new StelButton(Q_NULLPTR,
						       QPixmap(":/archaeoLines/bt_archaeolines_on.png"),
						       QPixmap(":/archaeoLines/bt_archaeolines_off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_ArchaeoLines");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar button for ArchaeoLines plugin: " << e.what();
	}
	addAction("actionAL_showEquinoxLine",          section, N_("Show Line for Equinox"),            "flagShowEquinox"         ); // No Shortcuts configured.
	addAction("actionAL_showSolsticeLines",        section, N_("Show Line for Solstices"),          "flagShowSolstices"       ); // No Shortcuts configured.
	addAction("actionAL_showCrossquarterLines",    section, N_("Show Line for Crossquarter"),       "flagShowCrossquarters"   ); // No Shortcuts configured.
	addAction("actionAL_showMajorStandstillLines", section, N_("Show Line for Major Standstill"),   "flagShowMajorStandstills"); // No Shortcuts configured.
	addAction("actionAL_showMinorStandstillLines", section, N_("Show Line for Minor Standstill"),   "flagShowMinorStandstills"); // No Shortcuts configured.
	addAction("actionAL_showZenithPassageLine",    section, N_("Show Line for Zenith Passage"),     "flagShowZenithPassage"   ); // No Shortcuts configured.
	addAction("actionAL_showNadirPassageLine",     section, N_("Show Line for Nadir Passage"),      "flagShowNadirPassage"    ); // No Shortcuts configured.
	addAction("actionAL_showSelectedObjectLine",   section, N_("Show Line for Selected Object"),    "flagShowSelectedObject"  ); // No Shortcuts configured.
	addAction("actionAL_showSelectedObjectAzimuthLine",   section, N_("Show Line for Selected Object's Azimuth"),    "flagShowSelectedObjectAzimuth"   ); // No Shortcuts configured.
	addAction("actionAL_showSelectedObjectHourAngleLine", section, N_("Show Line for Selected Object's Hour Angle"), "flagShowSelectedObjectHourAngle" ); // No Shortcuts configured.
	addAction("actionAL_showCurrentSunLine",       section, N_("Show Line for Current Sun"),        "flagShowCurrentSun"      ); // No Shortcuts configured.
	addAction("actionAL_showCurrentMoonLine",      section, N_("Show Line for Current Moon"),       "flagShowCurrentMoon"     ); // No Shortcuts configured.
	addAction("actionAL_showGeographicLocation1Line",section, N_("Show Vertical for Geographic Location 1"),   "flagShowGeographicLocation1"  ); // No Shortcuts configured.
	addAction("actionAL_showGeographicLocation2Line",section, N_("Show Vertical for Geographic Location 2"),   "flagShowGeographicLocation2"  ); // No Shortcuts configured.
	addAction("actionAL_showCustomAzimuth1Line",     section, N_("Show Vertical for Custom Azimuth 1"),   "flagShowCustomAzimuth1"  ); // No Shortcuts configured.
	addAction("actionAL_showCustomAzimuth2Line",     section, N_("Show Vertical for Custom Azimuth 2"),   "flagShowCustomAzimuth2"  ); // No Shortcuts configured.
	addAction("actionAL_showCustomDeclination1Line", section, N_("Show Line for Custom Declination 1"),   "flagShowCustomDeclination1"  ); // No Shortcuts configured.
	addAction("actionAL_showCustomDeclination2Line", section, N_("Show Line for Custom Declination 2"),   "flagShowCustomDeclination2"  ); // No Shortcuts configured.
}

void ArchaeoLines::update(double deltaTime)
{
	if (core->getCurrentPlanet()->getEnglishName()!="Earth")
		return;

	static SolarSystem *ssystem=GETSTELMODULE(SolarSystem);
	static const double lunarI=5.145396; // inclination of lunar orbit
	// compute min and max distance values for horizontal parallax.
	// Meeus, AstrAlg 98, p342.
	static const double meanDist=385000.56; // km earth-moon.
	static const double addedValues=20905.355+3699.111+2955.968+569.925+48.888+3.149+246.158+152.138+170.733+
			204.586+129.620+108.743+104.755+10.321+79.661+34.782+23.210+21.636+24.208+30.824+8.379+
			16.675+12.831+10.445+11.650+14.403+7.003+10.056+6.322+9.884;
	static const double minDist=meanDist-addedValues;
	static const double maxDist=meanDist+addedValues;
	static const double sinPiMin=6378.14/maxDist;
	static const double sinPiMax=6378.14/minDist; // maximal parallax at min. distance!
	static double eps;

	PlanetP planet=ssystem->getSun();
	double dec_equ, ra_equ, az, alt;
	StelUtils::rectToSphe(&ra_equ,&dec_equ,planet->getEquinoxEquatorialPos(core));
	currentSunLine->setDefiningAngle(dec_equ * 180.0/M_PI);
	planet=ssystem->getMoon();
	StelUtils::rectToSphe(&ra_equ,&dec_equ,planet->getEquinoxEquatorialPos(core));
	currentMoonLine->setDefiningAngle(dec_equ * 180.0/M_PI);

	if (enumShowCurrentPlanet>ArchaeoLine::CurrentPlanetNone)
	{
		const char *planetStrings[]={"", "Mercury", "Venus", "Mars", "Jupiter", "Saturn"};
		QString currentPlanet(planetStrings[enumShowCurrentPlanet - ArchaeoLine::CurrentPlanetNone]);
		planet=ssystem->searchByEnglishName(currentPlanet);
		Q_ASSERT(planet);
		StelUtils::rectToSphe(&ra_equ,&dec_equ,planet->getEquinoxEquatorialPos(core));
		currentPlanetLine->setDefiningAngle(dec_equ * 180.0/M_PI);
	}

	double newJDE=core->getJDE();
	if (fabs(newJDE-lastJDE) > 10.0) // enough to compute this every 10 days?
	{
		static const double invSqrt2=1.0/std::sqrt(2.0);
		double epsRad=ssystem->getEarth()->getRotObliquity(newJDE);
		double xqDec=asin(sin(epsRad)*invSqrt2)*180.0/M_PI;
		eps= epsRad*180.0/M_PI;
		northernSolsticeLine->setDefiningAngle(eps);
		southernSolsticeLine->setDefiningAngle(-eps);
		northernCrossquarterLine->setDefiningAngle( xqDec);
		southernCrossquarterLine->setDefiningAngle(-xqDec);
		lastJDE=newJDE;
	}
	StelLocation loc=core->getCurrentLocation();

	// compute parallax correction with Meeus 40.6. First, find H from h=0, then add corrections.

	static const double b_over_a=0.99664719;
	const bool useGeocentric = !core->getUseTopocentricCoordinates();
	const double latRad=useGeocentric ? 0.0 : static_cast<double>(loc.latitude)*M_PI_180;
	const double u=std::atan(b_over_a*std::tan(latRad));
	const double rhoSinPhiP=useGeocentric ? 0. : b_over_a*std::sin(u)+loc.altitude/6378140.0*std::sin(latRad);
	const double rhoCosPhiP=useGeocentric ? 1. :          std::cos(u)+loc.altitude/6378140.0*std::cos(latRad);

	QVector<double> lunarDE(8), sinPi(8);
	lunarDE[0]=(eps+lunarI)*M_PI/180.0; // min_distance=max_parallax
	lunarDE[1]=(eps+lunarI)*M_PI/180.0;
	lunarDE[2]=(eps-lunarI)*M_PI/180.0;
	lunarDE[3]=(eps-lunarI)*M_PI/180.0;
	lunarDE[4]=(-eps+lunarI)*M_PI/180.0;
	lunarDE[5]=(-eps+lunarI)*M_PI/180.0;
	lunarDE[6]=(-eps-lunarI)*M_PI/180.0;
	lunarDE[7]=(-eps-lunarI)*M_PI/180.0;
	for (int i=0; i<8; i+=2){
		sinPi[i]=sinPiMax;
		sinPi[i+1]=sinPiMin;
	}

	// In the following we compute parallax-corrected declinations of the setting moon for max and min distances.
	// odd indices for max_distance=min_parallax, even indices for min_distance=max_parallax. References are for Meeus AstrAlg 1998.
	QVector<double> cosHo(8), sinHo(8); // setting hour angles.
	for (int i=0; i<8; i++){
		cosHo[i]=qMax(-1.0, qMin(1.0, -std::tan(latRad)*std::tan(lunarDE[i])));
		sinHo[i]=std::sin(std::acos(cosHo[i]));
	}

	// 40.6
	QVector<double> A(8), B(8), C(8), q(8), lunarDEtopo(8);
	for (int i=0; i<8; i++){
		A[i]=std::cos(lunarDE[i])*sinHo[i];
		B[i]=std::cos(lunarDE[i])*cosHo[i]-rhoCosPhiP*sinPi[i];
		C[i]=std::sin(lunarDE[i])-rhoSinPhiP*sinPi[i];
		q[i]=std::sqrt(A[i]*A[i]+B[i]*B[i]+C[i]*C[i]);
		lunarDEtopo[i]=std::asin(C[i]/q[i]);
	}
	northernMajorStandstillLine0->setDefiningAngle(lunarDEtopo[0] *180.0/M_PI);
	northernMajorStandstillLine1->setDefiningAngle(lunarDEtopo[1] *180.0/M_PI);
	northernMinorStandstillLine2->setDefiningAngle(lunarDEtopo[2] *180.0/M_PI);
	northernMinorStandstillLine3->setDefiningAngle(lunarDEtopo[3] *180.0/M_PI);
	southernMinorStandstillLine4->setDefiningAngle(lunarDEtopo[4] *180.0/M_PI);
	southernMinorStandstillLine5->setDefiningAngle(lunarDEtopo[5] *180.0/M_PI);
	southernMajorStandstillLine6->setDefiningAngle(lunarDEtopo[6] *180.0/M_PI);
	southernMajorStandstillLine7->setDefiningAngle(lunarDEtopo[7] *180.0/M_PI);

	zenithPassageLine->setDefiningAngle(static_cast<double>(loc.latitude));
	nadirPassageLine->setDefiningAngle(static_cast<double>(-loc.latitude));

	// Selected object?
	if (objMgr->getWasSelected())
	{
		StelObjectP obj=objMgr->getSelectedObject().first();
		StelUtils::rectToSphe(&ra_equ,&dec_equ,obj->getEquinoxEquatorialPos(core));
		selectedObjectLine->setDefiningAngle(dec_equ * 180.0/M_PI);
		selectedObjectLine->setLabel(obj->getNameI18n());
		selectedObjectHourAngleLine->setDefiningAngle((M_PI-ra_equ) * 180.0/M_PI);
		selectedObjectHourAngleLine->setLabel(obj->getNameI18n());
		StelUtils::rectToSphe(&az,&alt,obj->getAltAzPosAuto(core));
		selectedObjectAzimuthLine->setDefiningAngle((M_PI-az) * 180.0/M_PI);
		selectedObjectAzimuthLine->setLabel(obj->getNameI18n());
	}

	// Updates for line brightness
	lineFader.update(static_cast<int>(deltaTime*1000));
	equinoxLine->update(deltaTime);
	northernSolsticeLine->update(deltaTime);
	southernSolsticeLine->update(deltaTime);
	northernCrossquarterLine->update(deltaTime);
	southernCrossquarterLine->update(deltaTime);
	northernMajorStandstillLine0->update(deltaTime);
	northernMajorStandstillLine1->update(deltaTime);
	northernMinorStandstillLine2->update(deltaTime);
	northernMinorStandstillLine3->update(deltaTime);
	southernMinorStandstillLine4->update(deltaTime);
	southernMinorStandstillLine5->update(deltaTime);
	southernMajorStandstillLine6->update(deltaTime);
	southernMajorStandstillLine7->update(deltaTime);
	zenithPassageLine->update(deltaTime);
	nadirPassageLine->update(deltaTime);
	selectedObjectLine->update(deltaTime);
	selectedObjectAzimuthLine->update(deltaTime);
	selectedObjectHourAngleLine->update(deltaTime);
	currentSunLine->update(deltaTime);
	currentMoonLine->update(deltaTime);
	currentPlanetLine->update(deltaTime);
	geographicLocation1Line->update(deltaTime);
	geographicLocation2Line->update(deltaTime);
	customAzimuth1Line->update(deltaTime);
	customAzimuth2Line->update(deltaTime);
	customDeclination1Line->update(deltaTime);
	customDeclination2Line->update(deltaTime);

	//withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
}


//! Draw any parts on the screen which are for our module
void ArchaeoLines::draw(StelCore* core)
{
	if (core->getCurrentPlanet()->getEnglishName()!="Earth")
		return;

	equinoxLine->draw(core, lineFader.getInterstate());
	northernSolsticeLine->draw(core, lineFader.getInterstate());
	southernSolsticeLine->draw(core, lineFader.getInterstate());
	northernCrossquarterLine->draw(core, lineFader.getInterstate());
	southernCrossquarterLine->draw(core, lineFader.getInterstate());
	northernMajorStandstillLine0->draw(core, lineFader.getInterstate());
	northernMajorStandstillLine1->draw(core, lineFader.getInterstate());
	northernMinorStandstillLine2->draw(core, lineFader.getInterstate());
	northernMinorStandstillLine3->draw(core, lineFader.getInterstate());
	southernMinorStandstillLine4->draw(core, lineFader.getInterstate());
	southernMinorStandstillLine5->draw(core, lineFader.getInterstate());
	southernMajorStandstillLine6->draw(core, lineFader.getInterstate());
	southernMajorStandstillLine7->draw(core, lineFader.getInterstate());
	zenithPassageLine->draw(core, lineFader.getInterstate());
	nadirPassageLine->draw(core, lineFader.getInterstate());
	if (objMgr->getWasSelected())
	{
		selectedObjectLine->draw(core, lineFader.getInterstate());
		selectedObjectAzimuthLine->draw(core, lineFader.getInterstate());
		selectedObjectHourAngleLine->draw(core, lineFader.getInterstate());
	}
	currentSunLine->draw(core, lineFader.getInterstate());
	currentMoonLine->draw(core, lineFader.getInterstate());
	if (enumShowCurrentPlanet>ArchaeoLine::CurrentPlanetNone)
		currentPlanetLine->draw(core, lineFader.getInterstate());
	geographicLocation1Line->draw(core, lineFader.getInterstate());
	geographicLocation2Line->draw(core, lineFader.getInterstate());
	customAzimuth1Line->draw(core, lineFader.getInterstate());
	customAzimuth2Line->draw(core, lineFader.getInterstate());
	customDeclination1Line->draw(core, lineFader.getInterstate());
	customDeclination2Line->draw(core, lineFader.getInterstate());
}


void ArchaeoLines::enableArchaeoLines(bool b)
{
	if (b!=flagShowArchaeoLines)
	{
		flagShowArchaeoLines = b;
		lineFader = b;
		conf->setValue("ArchaeoLines/enable_at_startup", flagShowArchaeoLines);
		emit archaeoLinesEnabledChanged(b);
	}
}

void ArchaeoLines::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	// Remove the old values...
	conf->remove("ArchaeoLines");
	// ...load the default values...
	loadSettings();
}

void ArchaeoLines::loadSettings()
{
	const bool azFromSouth=StelApp::getInstance().getFlagSouthAzimuthUsage();
	setLineWidth(conf->value("ArchaeoLines/line_thickness", 1).toInt());
	setEquinoxColor(                Vec3f(conf->value("ArchaeoLines/color_equinox",                    "1.00,1.00,0.50").toString()));
	setSolsticesColor(              Vec3f(conf->value("ArchaeoLines/color_solstices",                  "1.00,0.15,0.15").toString()));
	setCrossquartersColor(          Vec3f(conf->value("ArchaeoLines/color_crossquarters",              "1.00,0.75,0.25").toString()));
	setMajorStandstillColor(        Vec3f(conf->value("ArchaeoLines/color_major_standstill",           "0.25,1.00,0.25").toString()));
	setMinorStandstillColor(        Vec3f(conf->value("ArchaeoLines/color_minor_standstill",           "0.20,0.75,0.20").toString()));
	setZenithPassageColor(          Vec3f(conf->value("ArchaeoLines/color_zenith_passage",             "0.75,0.75,0.75").toString()));
	setNadirPassageColor(           Vec3f(conf->value("ArchaeoLines/color_nadir_passage",              "0.25,0.25,0.25").toString()));
	setSelectedObjectColor(         Vec3f(conf->value("ArchaeoLines/color_selected_object",            "1.00,1.00,1.00").toString()));
	setSelectedObjectAzimuthColor(  Vec3f(conf->value("ArchaeoLines/color_selected_object_azimuth",    "1.00,1.00,1.00").toString()));
	setSelectedObjectHourAngleColor(Vec3f(conf->value("ArchaeoLines/color_selected_object_hour_angle", "1.00,1.00,1.00").toString()));
	setCurrentSunColor(             Vec3f(conf->value("ArchaeoLines/color_current_sun",                "1.00,1.00,0.75").toString()));
	setCurrentMoonColor(            Vec3f(conf->value("ArchaeoLines/color_current_moon",               "0.50,1.00,0.50").toString()));
	setCurrentPlanetColor(          Vec3f(conf->value("ArchaeoLines/color_current_planet",             "0.25,0.80,1.00").toString()));
	setGeographicLocation1Color(    Vec3f(conf->value("ArchaeoLines/color_geographic_location_1",      "0.25,1.00,0.25").toString()));
	setGeographicLocation2Color(    Vec3f(conf->value("ArchaeoLines/color_geographic_location_2",      "0.25,0.25,1.00").toString()));
	setCustomAzimuth1Color(         Vec3f(conf->value("ArchaeoLines/color_custom_azimuth_1",           "0.25,1.00,0.25").toString()));
	setCustomAzimuth2Color(         Vec3f(conf->value("ArchaeoLines/color_custom_azimuth_2",           "0.25,0.50,0.75").toString()));
	setCustomDeclination1Color(     Vec3f(conf->value("ArchaeoLines/color_custom_declination_1",       "0.45,1.00,0.15").toString()));
	setCustomDeclination2Color(     Vec3f(conf->value("ArchaeoLines/color_custom_declination_2",       "0.45,0.50,0.65").toString()));

	setGeographicLocation1Longitude(conf->value("ArchaeoLines/geographic_location_1_longitude",  39.826175).toDouble());
	setGeographicLocation1Latitude( conf->value("ArchaeoLines/geographic_location_1_latitude",   21.422476).toDouble());
	setGeographicLocation2Longitude(conf->value("ArchaeoLines/geographic_location_2_longitude",  35.235774).toDouble());
	setGeographicLocation2Latitude( conf->value("ArchaeoLines/geographic_location_2_latitude",   31.778087).toDouble());
	StelLocation loc=core->getCurrentLocation();
	double azi=loc.getAzimuthForLocation(geographicLocation1Longitude, geographicLocation1Latitude);
	if (azFromSouth) azi+=180.0;
	geographicLocation1Line->setDefiningAngle(azi);
	azi = loc.getAzimuthForLocation(geographicLocation2Longitude, geographicLocation2Latitude);
	if (azFromSouth) azi+=180.0;
	geographicLocation2Line->setDefiningAngle(azi);
	geographicLocation1Line->setLabel(conf->value("ArchaeoLines/geographic_location_1_label", "Mecca (Qibla)").toString());
	geographicLocation2Line->setLabel(conf->value("ArchaeoLines/geographic_location_2_label", "Jerusalem").toString());

	customAzimuth1Line->setDefiningAngle(conf->value("ArchaeoLines/custom_azimuth_1_angle", 0.0).toDouble());
	customAzimuth2Line->setDefiningAngle(conf->value("ArchaeoLines/custom_azimuth_2_angle", 0.0).toDouble());
	customAzimuth1Line->setLabel(conf->value("ArchaeoLines/custom_azimuth_1_label", "custAzi1").toString());
	customAzimuth2Line->setLabel(conf->value("ArchaeoLines/custom_azimuth_2_label", "custAzi2").toString());
	customDeclination1Line->setDefiningAngle(conf->value("ArchaeoLines/custom_declination_1_angle", 0.0).toDouble());
	customDeclination2Line->setDefiningAngle(conf->value("ArchaeoLines/custom_declination_2_angle", 0.0).toDouble());
	customDeclination1Line->setLabel(conf->value("ArchaeoLines/custom_declination_1_label", "custDec1").toString());
	customDeclination2Line->setLabel(conf->value("ArchaeoLines/custom_declination_2_label", "custDec2").toString());

	// Now activate line display if needed.
	// 5 solar limits
	showEquinox(conf->value("ArchaeoLines/show_equinox", true).toBool());
	showSolstices(conf->value("ArchaeoLines/show_solstices", true).toBool());
	showCrossquarters(conf->value("ArchaeoLines/show_crossquarters", true).toBool());
	// 4 lunar limits
	showMajorStandstills(conf->value("ArchaeoLines/show_major_standstills", true).toBool());
	showMinorStandstills(conf->value("ArchaeoLines/show_minor_standstills", true).toBool());
	// esp. Mesoamerica
	showZenithPassage(conf->value("ArchaeoLines/show_zenith_passage", true).toBool());
	showNadirPassage(conf->value("ArchaeoLines/show_nadir_passage",  false).toBool());
	// indicators for line representing currently selected object's declination, azimuth and hour angle (or right ascension)
	showSelectedObject(conf->value("ArchaeoLines/show_selected_object", false).toBool());
	showSelectedObjectAzimuth(conf->value("ArchaeoLines/show_selected_object_azimuth", false).toBool());
	showSelectedObjectHourAngle(conf->value("ArchaeoLines/show_selected_object_hour_angle", false).toBool());
	// indicators for current declinations (those move fast over days...)
	showCurrentSun(conf->value("ArchaeoLines/show_current_sun", true).toBool());
	showCurrentMoon(conf->value("ArchaeoLines/show_current_moon", true).toBool());
	showCurrentPlanetNamed(conf->value("ArchaeoLines/show_current_planet", "none").toString());
	// azimuths to geographic targets, and custom azimuths.
	showGeographicLocation1(conf->value("ArchaeoLines/show_geographic_location_1", false).toBool());
	showGeographicLocation2(conf->value("ArchaeoLines/show_geographic_location_2", false).toBool());
	showCustomAzimuth1(conf->value("ArchaeoLines/show_custom_azimuth_1", false).toBool());
	showCustomAzimuth2(conf->value("ArchaeoLines/show_custom_azimuth_2", false).toBool());
	showCustomDeclination1(conf->value("ArchaeoLines/show_custom_declination_1", false).toBool());
	showCustomDeclination2(conf->value("ArchaeoLines/show_custom_declination_2", false).toBool());

	enableArchaeoLines(conf->value("ArchaeoLines/enable_at_startup", false).toBool());
}

void ArchaeoLines::setLineWidth(int width)
{
	if (width!=lineWidth)
	{
		lineWidth=qBound(1, width, 8); // Force some sensible limit
		conf->setValue("ArchaeoLines/line_thickness", lineWidth);
		emit lineWidthChanged(lineWidth);
	}
}

void ArchaeoLines::showEquinox(bool b)
{
	if (b!=flagShowEquinox)
	{
		flagShowEquinox=b;
		conf->setValue("ArchaeoLines/show_equinox",         isEquinoxDisplayed());
		equinoxLine->setDisplayed(b);
		emit showEquinoxChanged(b);
	}
}
void ArchaeoLines::showSolstices(bool b)
{
	if (b!=flagShowSolstices)
	{
		flagShowSolstices=b;
		conf->setValue("ArchaeoLines/show_solstices",         isSolsticesDisplayed());
		northernSolsticeLine->setDisplayed(b);
		southernSolsticeLine->setDisplayed(b);
		emit showSolsticesChanged(b);
	}
}
void ArchaeoLines::showCrossquarters(bool b)
{
	if (b!=flagShowCrossquarters)
	{
		flagShowCrossquarters=b;
		conf->setValue("ArchaeoLines/show_crossquarters",     isCrossquartersDisplayed());
		northernCrossquarterLine->setDisplayed(b);
		southernCrossquarterLine->setDisplayed(b);
		emit showCrossquartersChanged(b);
	}
}
void ArchaeoLines::showMajorStandstills(bool b)
{
	if (b!=flagShowMajorStandstills)
	{
		flagShowMajorStandstills=b;
		conf->setValue("ArchaeoLines/show_major_standstills", isMajorStandstillsDisplayed());
		northernMajorStandstillLine0->setDisplayed(b);
		northernMajorStandstillLine1->setDisplayed(b);
		southernMajorStandstillLine6->setDisplayed(b);
		southernMajorStandstillLine7->setDisplayed(b);
		emit showMajorStandstillsChanged(b);
	}
}
void ArchaeoLines::showMinorStandstills(bool b)
{
	if (b!=flagShowMinorStandstills)
	{
		flagShowMinorStandstills=b;
		conf->setValue("ArchaeoLines/show_minor_standstills", isMinorStandstillsDisplayed());
		northernMinorStandstillLine2->setDisplayed(b);
		northernMinorStandstillLine3->setDisplayed(b);
		southernMinorStandstillLine4->setDisplayed(b);
		southernMinorStandstillLine5->setDisplayed(b);
		emit showMinorStandstillsChanged(b);
	}
}
void ArchaeoLines::showZenithPassage(bool b)
{
	if (b!=flagShowZenithPassage)
	{
		flagShowZenithPassage=b;
		conf->setValue("ArchaeoLines/show_zenith_passage",      isZenithPassageDisplayed());
		zenithPassageLine->setDisplayed(b);
		emit showZenithPassageChanged(b);
	}
}
void ArchaeoLines::showNadirPassage(bool b)
{
	if (b!=flagShowNadirPassage)
	{
		flagShowNadirPassage=b;
		conf->setValue("ArchaeoLines/show_nadir_passage",       isNadirPassageDisplayed());
		nadirPassageLine->setDisplayed(b);
		emit showNadirPassageChanged(b);
	}
}
void ArchaeoLines::showSelectedObject(bool b)
{
	if (b!=flagShowSelectedObject)
	{
		flagShowSelectedObject=b;
		conf->setValue("ArchaeoLines/show_selected_object",       isSelectedObjectDisplayed());
		selectedObjectLine->setDisplayed(b);
		emit showSelectedObjectChanged(b);
	}
}
void ArchaeoLines::showSelectedObjectAzimuth(bool b)
{
	if (b!=flagShowSelectedObjectAzimuth)
	{
		flagShowSelectedObjectAzimuth=b;
		conf->setValue("ArchaeoLines/show_selected_object_azimuth", isSelectedObjectAzimuthDisplayed());
		selectedObjectAzimuthLine->setDisplayed(b);
		emit showSelectedObjectAzimuthChanged(b);
	}
}
void ArchaeoLines::showSelectedObjectHourAngle(bool b)
{
	if (b!=flagShowSelectedObjectHourAngle)
	{
		flagShowSelectedObjectHourAngle=b;
		conf->setValue("ArchaeoLines/show_selected_object_hour_angle", isSelectedObjectHourAngleDisplayed());
		selectedObjectHourAngleLine->setDisplayed(b);
		emit showSelectedObjectHourAngleChanged(b);
	}
}
void ArchaeoLines::showCurrentSun(bool b)
{
	if (b!=flagShowCurrentSun)
	{
		flagShowCurrentSun=b;
		conf->setValue("ArchaeoLines/show_current_sun",       isCurrentSunDisplayed());
		currentSunLine->setDisplayed(b);
		emit showCurrentSunChanged(b);
	}
}
void ArchaeoLines::showCurrentMoon(bool b)
{
	if (b!=flagShowCurrentMoon)
	{
		flagShowCurrentMoon=b;
		conf->setValue("ArchaeoLines/show_current_moon",       isCurrentMoonDisplayed());
		currentMoonLine->setDisplayed(b);
		emit showCurrentMoonChanged(b);
	}
}
void ArchaeoLines::showCurrentPlanet(ArchaeoLine::Line l)
{
	// Avoid a crash but give warning.
	if ((l<ArchaeoLine::CurrentPlanetNone) || (l>ArchaeoLine::CurrentPlanetSaturn))
	{
		qWarning() << "ArchaeoLines::showCurrentPlanet: Invalid planet called:" << l << "Setting to none.";
		l=ArchaeoLine::CurrentPlanetNone;
	}
	if(l!=enumShowCurrentPlanet)
	{
		enumShowCurrentPlanet=l;
		const char *planetStrings[]={"none", "Mercury", "Venus", "Mars", "Jupiter", "Saturn"};

		conf->setValue("ArchaeoLines/show_current_planet", planetStrings[l-ArchaeoLine::CurrentPlanetNone]);
		currentPlanetLine->setLineType(enumShowCurrentPlanet);
		currentPlanetLine->setDisplayed(enumShowCurrentPlanet != ArchaeoLine::CurrentPlanetNone);

		emit currentPlanetChanged(l);
	}
}
void ArchaeoLines::showCurrentPlanetNamed(QString planet)
{
	if (planet=="none")
		enumShowCurrentPlanet=ArchaeoLine::CurrentPlanetNone;
	else if (planet=="Mercury")
		enumShowCurrentPlanet=ArchaeoLine::CurrentPlanetMercury;
	else if (planet=="Venus")
		enumShowCurrentPlanet=ArchaeoLine::CurrentPlanetVenus;
	else if (planet=="Mars")
		enumShowCurrentPlanet=ArchaeoLine::CurrentPlanetMars;
	else if (planet=="Jupiter")
		enumShowCurrentPlanet=ArchaeoLine::CurrentPlanetJupiter;
	else if (planet=="Saturn")
		enumShowCurrentPlanet=ArchaeoLine::CurrentPlanetSaturn;
	else {
		qWarning() << "ArchaeoLines: showCurrentPlanet: Invalid planet requested: " << planet;
		enumShowCurrentPlanet=ArchaeoLine::CurrentPlanetNone;
	}

	conf->setValue("ArchaeoLines/show_current_planet", planet);
	currentPlanetLine->setLineType(enumShowCurrentPlanet);
	currentPlanetLine->setDisplayed(enumShowCurrentPlanet != ArchaeoLine::CurrentPlanetNone);
	emit currentPlanetChanged(enumShowCurrentPlanet);
}
void ArchaeoLines::showGeographicLocation1(bool b)
{
	if (b!=flagShowGeographicLocation1)
	{
		flagShowGeographicLocation1=b;
		conf->setValue("ArchaeoLines/show_geographic_location_1",       isGeographicLocation1Displayed());
		geographicLocation1Line->setDisplayed(b);
		emit showGeographicLocation1Changed(b);
	}
}
void ArchaeoLines::showGeographicLocation2(bool b)
{
	if (b!=flagShowGeographicLocation2)
	{
		flagShowGeographicLocation2=b;
		conf->setValue("ArchaeoLines/show_geographic_location_2",       isGeographicLocation2Displayed());
		geographicLocation2Line->setDisplayed(b);
		emit showGeographicLocation2Changed(b);
	}
}
void ArchaeoLines::showCustomAzimuth1(bool b)
{
	if (b!=flagShowCustomAzimuth1)
	{
		flagShowCustomAzimuth1=b;
		conf->setValue("ArchaeoLines/show_custom_azimuth_1",       isCustomAzimuth1Displayed());
		customAzimuth1Line->setDisplayed(b);
		emit showCustomAzimuth1Changed(b);
	}
}
void ArchaeoLines::showCustomAzimuth2(bool b)
{
	if (b!=flagShowCustomAzimuth2)
	{
		flagShowCustomAzimuth2=b;
		conf->setValue("ArchaeoLines/show_custom_azimuth_2",       isCustomAzimuth2Displayed());
		customAzimuth2Line->setDisplayed(b);
		emit showCustomAzimuth2Changed(b);
	}
}
void ArchaeoLines::showCustomDeclination1(bool b)
{
	if (b!=flagShowCustomDeclination1)
	{
		flagShowCustomDeclination1=b;
		conf->setValue("ArchaeoLines/show_custom_declination_1",       isCustomDeclination1Displayed());
		customDeclination1Line->setDisplayed(b);
		emit showCustomDeclination1Changed(b);
	}
}
void ArchaeoLines::showCustomDeclination2(bool b)
{
	if (b!=flagShowCustomDeclination2)
	{
		flagShowCustomDeclination2=b;
		conf->setValue("ArchaeoLines/show_custom_declination_2",       isCustomDeclination2Displayed());
		customDeclination2Line->setDisplayed(b);
		emit showCustomDeclination2Changed(b);
	}
}

void ArchaeoLines::setGeographicLocation1Longitude(double lng)
{
	conf->setValue("ArchaeoLines/geographic_location_1_longitude", lng);
	geographicLocation1Longitude=lng;
	StelLocation loc=core->getCurrentLocation();
	double az=loc.getAzimuthForLocation(geographicLocation1Longitude, geographicLocation1Latitude);
	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		az+=180.0;
	geographicLocation1Line->setDefiningAngle(az);
	emit geographicLocation1Changed();
}
void ArchaeoLines::setGeographicLocation1Latitude(double lat)
{
	conf->setValue("ArchaeoLines/geographic_location_1_latitude", lat);
	geographicLocation1Latitude=lat;
	StelLocation loc=core->getCurrentLocation();
	double az=loc.getAzimuthForLocation(geographicLocation1Longitude, geographicLocation1Latitude);
	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		az+=180.0;
	geographicLocation1Line->setDefiningAngle(az);
	emit geographicLocation1Changed();
}
void ArchaeoLines::setGeographicLocation1Label(QString label)
{
	geographicLocation1Line->setLabel(label);
	conf->setValue("ArchaeoLines/geographic_location_1_label", label);
	emit geographicLocation1LabelChanged(label);
}
void ArchaeoLines::setGeographicLocation2Longitude(double lng)
{
	conf->setValue("ArchaeoLines/geographic_location_2_longitude", lng);
	geographicLocation2Longitude=lng;
	StelLocation loc=core->getCurrentLocation();
	double az=loc.getAzimuthForLocation(geographicLocation2Longitude, geographicLocation2Latitude);
	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		az+=180.0;
	geographicLocation2Line->setDefiningAngle(az);
	emit geographicLocation2Changed();
}
void ArchaeoLines::setGeographicLocation2Latitude(double lat)
{
	conf->setValue("ArchaeoLines/geographic_location_2_latitude", lat);
	geographicLocation2Latitude=lat;
	StelLocation loc=core->getCurrentLocation();
	double az=loc.getAzimuthForLocation(geographicLocation2Longitude, geographicLocation2Latitude);
	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		az+=180.0;
	geographicLocation2Line->setDefiningAngle(az);
	emit geographicLocation2Changed();
}
void ArchaeoLines::setGeographicLocation2Label(QString label)
{
	geographicLocation2Line->setLabel(label);
	conf->setValue("ArchaeoLines/geographic_location_2_label", label);
	emit geographicLocation2LabelChanged(label);
}

void ArchaeoLines::updateObserverLocation(const StelLocation &loc)
{
	geographicLocation1Line->setDefiningAngle(loc.getAzimuthForLocation(geographicLocation1Longitude, geographicLocation1Latitude));
	geographicLocation2Line->setDefiningAngle(loc.getAzimuthForLocation(geographicLocation2Longitude, geographicLocation2Latitude));
}


void ArchaeoLines::setCustomAzimuth1(double az)
{
	if (!qFuzzyCompare(az, customAzimuth1Line->getDefiningAngle()))
	{
		customAzimuth1Line->setDefiningAngle(az);
		conf->setValue("ArchaeoLines/custom_azimuth_1_angle", az);
		emit customAzimuth1Changed(az);
	}
}
void ArchaeoLines::setCustomAzimuth2(double az)
{
	if (!qFuzzyCompare(az, customAzimuth1Line->getDefiningAngle()))
	{
		customAzimuth2Line->setDefiningAngle(az);
		conf->setValue("ArchaeoLines/custom_azimuth_2_angle", az);
		emit customAzimuth2Changed(az);
	}
}
void ArchaeoLines::setCustomAzimuth1Label(QString label)
{
	customAzimuth1Line->setLabel(label);
	conf->setValue("ArchaeoLines/custom_azimuth_1_label", label);
	emit customAzimuth1LabelChanged(label);
}
void ArchaeoLines::setCustomAzimuth2Label(QString label)
{
	customAzimuth2Line->setLabel(label);
	conf->setValue("ArchaeoLines/custom_azimuth_2_label", label);
	emit customAzimuth2LabelChanged(label);
}
void ArchaeoLines::setCustomDeclination1(double dec)
{
	if (!qFuzzyCompare(dec, customDeclination1Line->getDefiningAngle()))
	{
		customDeclination1Line->setDefiningAngle(dec);
		conf->setValue("ArchaeoLines/custom_declination_1_angle", dec);
		emit customDeclination1Changed(dec);
	}
}
void ArchaeoLines::setCustomDeclination2(double dec)
{
	if (!qFuzzyCompare(dec, customDeclination1Line->getDefiningAngle()))
	{
		customDeclination2Line->setDefiningAngle(dec);
		conf->setValue("ArchaeoLines/custom_declination_2_angle", dec);
		emit customDeclination2Changed(dec);
	}
}
void ArchaeoLines::setCustomDeclination1Label(QString label)
{
	customDeclination1Line->setLabel(label);
	conf->setValue("ArchaeoLines/custom_declination_1_label", label);
	emit customDeclination1LabelChanged(label);
}
void ArchaeoLines::setCustomDeclination2Label(QString label)
{
	customDeclination2Line->setLabel(label);
	conf->setValue("ArchaeoLines/custom_declination_2_label", label);
	emit customDeclination2LabelChanged(label);
}

void ArchaeoLines::setEquinoxColor(Vec3f color)
{
	if (color!=getEquinoxColor())
	{
		equinoxColor=color;
		emit equinoxColorChanged(color);
	}
}
void ArchaeoLines::setSolsticesColor(Vec3f color)
{
	if (color!=getSolsticesColor())
	{
		solsticesColor=color;
		emit solsticesColorChanged(color);
	}
}
void ArchaeoLines::setCrossquartersColor(Vec3f color)
{
	if (color!=getCrossquartersColor())
	{
		crossquartersColor=color;
		emit crossquartersColorChanged(color);
	}
}
void ArchaeoLines::setMajorStandstillColor(Vec3f color)
{
	if (color!=getMajorStandstillColor())
	{
		majorStandstillColor=color;
		emit majorStandstillColorChanged(color);
	}
}
void ArchaeoLines::setMinorStandstillColor(Vec3f color)
{
	if (color!=getMinorStandstillColor())
	{
		minorStandstillColor=color;
		emit minorStandstillColorChanged(color);
	}
}
void ArchaeoLines::setZenithPassageColor(Vec3f color)
{
	if (color!=getZenithPassageColor())
	{
		zenithPassageColor=color;
		emit zenithPassageColorChanged(color);
	}
}
void ArchaeoLines::setNadirPassageColor(Vec3f color)
{
	if (color!=getNadirPassageColor())
	{
		nadirPassageColor=color;
		emit nadirPassageColorChanged(color);
	}
}
void ArchaeoLines::setSelectedObjectColor(Vec3f color)
{
	if (color!=getSelectedObjectColor())
	{
		selectedObjectColor=color;
		emit selectedObjectColorChanged(color);
	}
}
void ArchaeoLines::setSelectedObjectAzimuthColor(Vec3f color)
{
	if (color!=getSelectedObjectAzimuthColor())
	{
		selectedObjectAzimuthColor=color;
		emit selectedObjectAzimuthColorChanged(color);
	}
}
void ArchaeoLines::setSelectedObjectHourAngleColor(Vec3f color)
{
	if (color!=getSelectedObjectHourAngleColor())
	{
		selectedObjectHourAngleColor=color;
		emit selectedObjectHourAngleColorChanged(color);
	}
}
void ArchaeoLines::setCurrentSunColor(Vec3f color)
{
	if (color!=getCurrentSunColor())
	{
		currentSunColor=color;
		emit currentSunColorChanged(color);
	}
}
void ArchaeoLines::setCurrentMoonColor(Vec3f color)
{
	if (color!=getCurrentMoonColor())
	{
		currentMoonColor=color;
		emit currentMoonColorChanged(color);
	}
}
void ArchaeoLines::setCurrentPlanetColor(Vec3f color)
{
	if (color!=getCurrentPlanetColor())
	{
		currentPlanetColor=color;
		emit currentPlanetColorChanged(color);
	}
}
void ArchaeoLines::setGeographicLocation1Color(Vec3f color)
{
	if (color!=getGeographicLocation1Color())
	{
		geographicLocation1Color=color;
		emit geographicLocation1ColorChanged(color);
	}
}
void ArchaeoLines::setGeographicLocation2Color(Vec3f color)
{
	if (color!=getGeographicLocation2Color())
	{
		geographicLocation2Color=color;
		emit geographicLocation2ColorChanged(color);
	}
}
void ArchaeoLines::setCustomAzimuth1Color(Vec3f color)
{
	if (color!=getCustomAzimuth1Color())
	{
		customAzimuth1Color=color;
		emit customAzimuth1ColorChanged(color);
	}
}
void ArchaeoLines::setCustomAzimuth2Color(Vec3f color)
{
	if (color!=getCustomAzimuth2Color())
	{
		customAzimuth2Color=color;
		emit customAzimuth2ColorChanged(color);
	}
}
void ArchaeoLines::setCustomDeclination1Color(Vec3f color)
{
	if (color!=getCustomDeclination1Color())
	{
		customDeclination1Color=color;
		emit customDeclination1ColorChanged(color);
	}
}
void ArchaeoLines::setCustomDeclination2Color(Vec3f color)
{
	if (color!=getCustomDeclination2Color())
	{
		customDeclination2Color=color;
		emit customDeclination2ColorChanged(color);
	}
}



double ArchaeoLines::getLineAngle(ArchaeoLine::Line whichLine) const
{
	switch (whichLine){
		case ArchaeoLine::Equinox:
			return equinoxLine->getDefiningAngle();
		case ArchaeoLine::Solstices:
			return northernSolsticeLine->getDefiningAngle();
		case ArchaeoLine::Crossquarters:
			return northernCrossquarterLine->getDefiningAngle();
		case ArchaeoLine::MajorStandstill:
			return northernMajorStandstillLine0->getDefiningAngle();
		case ArchaeoLine::MinorStandstill:
			return northernMinorStandstillLine2->getDefiningAngle();
		case ArchaeoLine::ZenithPassage:
			return zenithPassageLine->getDefiningAngle();
		case ArchaeoLine::NadirPassage:
			return nadirPassageLine->getDefiningAngle();
		case ArchaeoLine::SelectedObject:
			return selectedObjectLine->getDefiningAngle();
		case ArchaeoLine::SelectedObjectAzimuth:
			return selectedObjectAzimuthLine->getDefiningAngle();
		case ArchaeoLine::SelectedObjectHourAngle:
			return selectedObjectHourAngleLine->getDefiningAngle();
		case ArchaeoLine::CurrentSun:
			return currentSunLine->getDefiningAngle();
		case ArchaeoLine::CurrentMoon:
			return currentMoonLine->getDefiningAngle();
		case ArchaeoLine::CurrentPlanetNone:
			return 0.0;
		case ArchaeoLine::CurrentPlanetMercury:
		case ArchaeoLine::CurrentPlanetVenus:
		case ArchaeoLine::CurrentPlanetMars:
		case ArchaeoLine::CurrentPlanetJupiter:
		case ArchaeoLine::CurrentPlanetSaturn:
			return currentPlanetLine->getDefiningAngle();
		case ArchaeoLine::GeographicLocation1:
			return geographicLocation1Line->getDefiningAngle();
		case ArchaeoLine::GeographicLocation2:
			return geographicLocation2Line->getDefiningAngle();
		case ArchaeoLine::CustomAzimuth1:
			return customAzimuth1Line->getDefiningAngle();
		case ArchaeoLine::CustomAzimuth2:
			return customAzimuth2Line->getDefiningAngle();
		case ArchaeoLine::CustomDeclination1:
			return customDeclination1Line->getDefiningAngle();
		case ArchaeoLine::CustomDeclination2:
			return customDeclination2Line->getDefiningAngle();
		default:
			Q_ASSERT(0);
	}
	return -100.0;
}

QString ArchaeoLines::getLineLabel(ArchaeoLine::Line whichLine) const
{
	switch (whichLine){
		case ArchaeoLine::Equinox:
			return equinoxLine->getLabel();
		case ArchaeoLine::Solstices:
			return northernSolsticeLine->getLabel();
		case ArchaeoLine::Crossquarters:
			return northernCrossquarterLine->getLabel();
		case ArchaeoLine::MajorStandstill:
			return northernMajorStandstillLine0->getLabel();
		case ArchaeoLine::MinorStandstill:
			return northernMinorStandstillLine2->getLabel();
		case ArchaeoLine::ZenithPassage:
			return zenithPassageLine->getLabel();
		case ArchaeoLine::NadirPassage:
			return nadirPassageLine->getLabel();
		case ArchaeoLine::SelectedObject:
			return selectedObjectLine->getLabel();
		case ArchaeoLine::SelectedObjectAzimuth:
			return selectedObjectAzimuthLine->getLabel();
		case ArchaeoLine::SelectedObjectHourAngle:
			return selectedObjectHourAngleLine->getLabel();
		case ArchaeoLine::CurrentSun:
			return currentSunLine->getLabel();
		case ArchaeoLine::CurrentMoon:
			return currentMoonLine->getLabel();
		case ArchaeoLine::CurrentPlanetNone:
		case ArchaeoLine::CurrentPlanetMercury:
		case ArchaeoLine::CurrentPlanetVenus:
		case ArchaeoLine::CurrentPlanetMars:
		case ArchaeoLine::CurrentPlanetJupiter:
		case ArchaeoLine::CurrentPlanetSaturn:
			return currentPlanetLine->getLabel();
		case ArchaeoLine::GeographicLocation1:
			return geographicLocation1Line->getLabel();
		case ArchaeoLine::GeographicLocation2:
			return geographicLocation2Line->getLabel();
		case ArchaeoLine::CustomAzimuth1:
			return customAzimuth1Line->getLabel();
		case ArchaeoLine::CustomAzimuth2:
			return customAzimuth2Line->getLabel();
		case ArchaeoLine::CustomDeclination1:
			return customDeclination1Line->getLabel();
		case ArchaeoLine::CustomDeclination2:
			return customDeclination2Line->getLabel();
		default:
			Q_ASSERT(0);
			return "ArchaeoLines::getLineLabel(): Error!";
	}
}

// callback stuff shamelessly taken from GridLinesMgr. Changes: Text MUST be filled, can also be empty for no label!
struct ALViewportEdgeIntersectCallbackData
{
	ALViewportEdgeIntersectCallbackData(StelPainter* p)
		: sPainter(p)
	{;}
	StelPainter* sPainter;
	//Vec4f textColor;
	QString text;		// Label to display at the intersection of the lines and screen side
};

// Callback which draws the label of the grid
void alViewportEdgeIntersectCallback(const Vec3d& screenPos, const Vec3d& direction, void* userData)
{
	// TODO: decide whether to use different color for labels.
	// Currently label color is equal to line color, and provisions for changing that are commented away.

	ALViewportEdgeIntersectCallbackData* d = static_cast<ALViewportEdgeIntersectCallbackData*>(userData);
	Vec3d direc(direction);
	direc.normalize();
	//const Vec4f tmpColor = d->sPainter->getColor();
	//d->sPainter->setColor(d->textColor[0], d->textColor[1], d->textColor[2], d->textColor[3]);

	QString text = d->text; // original text-from-coordinates taken out!
	if (text.isEmpty())
		return;

	float angleDeg = static_cast<float>(std::atan2(-direc[1], -direc[0])*M_180_PI);
	float xshift=6.f;
	if (angleDeg>90.f || angleDeg<-90.f)
	{
		angleDeg+=180.f;
		xshift=-d->sPainter->getFontMetrics().boundingRect(text).width()-6.f;
	}

	d->sPainter->drawText(static_cast<float>(screenPos[0]), static_cast<float>(screenPos[1]), text, angleDeg, xshift, 3);
	//d->sPainter->setColor(tmpColor[0], tmpColor[1], tmpColor[2], tmpColor[3]); // RESTORE
	d->sPainter->setBlending(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ArchaeoLine::ArchaeoLine(ArchaeoLine::Line lineType, double definingAngle) :
	lineType(lineType), definingAngle(definingAngle), color(0.f, 0.f, 1.f), frameType(StelCore::FrameEquinoxEqu), flagLabel(true)
{
	if (lineType>=SelectedObjectAzimuth)
		frameType=StelCore::FrameAltAz;
	// Font size is 14
	setFontSizeFromApp(StelApp::getInstance().getScreenFontSize());
	updateLabel();
	fader.setDuration(1000);
	// Initialize the message strings and make sure they are translated when the language changes.
	StelApp& app = StelApp::getInstance();
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateLabel()));
	connect(&app, SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSizeFromApp(const int)));
}

void ArchaeoLine::updateLabel()
{
	//qDebug() << "ArchaeoLine::updateLabel(): lineType is " << lineType;
	// TODO: decide whether showing declinations in addition.
	switch (lineType)
	{
		case ArchaeoLine::Equinox:
			label = q_("Equinox");
			break;
		case ArchaeoLine::Solstices:
			label = q_("Solstice");
			break;
		case ArchaeoLine::Crossquarters:
			label = q_("Crossquarter");
			break;
		case ArchaeoLine::MajorStandstill:
			label = q_("Major Lunar Standstill");
			break;
		case ArchaeoLine::MinorStandstill:
			label = q_("Minor Lunar Standstill");
			break;
		case ArchaeoLine::ZenithPassage:
			label = q_("Zenith Passage");
			break;
		case ArchaeoLine::NadirPassage:
			label = q_("Nadir Passage");
			break;
		case ArchaeoLine::SelectedObject:
		case ArchaeoLine::SelectedObjectAzimuth:
		case ArchaeoLine::SelectedObjectHourAngle:
			label = q_("Selected Object");
			break;
		case ArchaeoLine::CurrentSun:
			label = q_("Sun");
			break;
		case ArchaeoLine::CurrentMoon:
			label = q_("Moon");
			break;
		case ArchaeoLine::CurrentPlanetNone:
			label = q_("error if you can read this");
			break;
		case ArchaeoLine::CurrentPlanetMercury:
			label = q_("Mercury");
			break;
		case ArchaeoLine::CurrentPlanetVenus:
			label = q_("Venus");
			break;
		case ArchaeoLine::CurrentPlanetMars:
			label = q_("Mars");
			break;
		case ArchaeoLine::CurrentPlanetJupiter:
			label = q_("Jupiter");
			break;
		case ArchaeoLine::CurrentPlanetSaturn:
			label = q_("Saturn");
			break;
		case ArchaeoLine::GeographicLocation1: // label was set in setLabel(). DO NOT update.
		case ArchaeoLine::GeographicLocation2:
		case ArchaeoLine::CustomAzimuth1:
		case ArchaeoLine::CustomAzimuth2:
		case ArchaeoLine::CustomDeclination1:
		case ArchaeoLine::CustomDeclination2:
			break;
		default:
			Q_ASSERT(0);
	}
}


void ArchaeoLine::draw(StelCore *core, float intensity) const
{
	// borrowed largely from GridLinesMgr.
	if (intensity*fader.getInterstate() < 0.000001f)
		return;


	StelProjectorP prj = core->getProjection(frameType, StelCore::RefractionAuto);

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = prj->getBoundingCap();

	// Initialize a painter and set OpenGL state
	StelPainter sPainter(prj);
	sPainter.setBlending(true);
	sPainter.setLineSmooth(true);
	const float oldLineWidth=sPainter.getLineWidth();
	sPainter.setLineWidth(GETSTELMODULE(ArchaeoLines)->getLineWidth());
	sPainter.setColor(color[0], color[1], color[2], intensity*fader.getInterstate());
	//Vec4f textColor(color[0], color[1], color[2], intensity*fader.getInterstate());

	ALViewportEdgeIntersectCallbackData userData(&sPainter);
	sPainter.setFont(font);
	//userData.textColor = textColor;
	userData.text = (isLabelVisible() ? label : "");
	/////////////////////////////////////////////////
	// Azimuth lines are Great Semicircles. TODO: Make sure the code commented away below is OK in all cases, then cleanup if full circles are never required.
	if (lineType>=ArchaeoLine::SelectedObjectHourAngle)
	{
		SphericalCap meridianSphericalCap(Vec3d(0,1,0), 0);
		Vec3d fpt(-1,0,0);
		meridianSphericalCap.n.transfo4d(Mat4d::rotation(Vec3d(0, 0, 1), -definingAngle*M_PI/180.));
		fpt.transfo4d(Mat4d::rotation(Vec3d(0, 0, 1), -definingAngle*M_PI/180.));

//		Vec3d p1, p2;
//		//if (!SphericalCap::intersectionPoints(viewPortSphericalCap, meridianSphericalCap, p1, p2))
//		{
//			//if ((viewPortSphericalCap.d<meridianSphericalCap.d && viewPortSphericalCap.contains(meridianSphericalCap.n))
//			//	|| (viewPortSphericalCap.d<-meridianSphericalCap.d && viewPortSphericalCap.contains(-meridianSphericalCap.n)))
//			{ // N.B. we had 3x120degrees here. Look into GridLineMgr to restore if necessary.
//				// The meridian is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
				const Mat4d& rotLonP90 = Mat4d::rotation(meridianSphericalCap.n, 90.*M_PI/180.);
				const Mat4d& rotLonM90 = Mat4d::rotation(meridianSphericalCap.n, -90.*M_PI/180.);
				Vec3d rotFpt=fpt;
				rotFpt.transfo4d(rotLonP90);
				Vec3d rotFpt2=fpt;
				rotFpt2.transfo4d(rotLonM90);
				sPainter.drawGreatCircleArc(fpt, rotFpt, Q_NULLPTR, alViewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt2, fpt, Q_NULLPTR, alViewportEdgeIntersectCallback, &userData);
				//sPainter.drawGreatCircleArc(rotFpt2, fpt, Q_NULLPTR, alViewportEdgeIntersectCallback, &userData);
				//return;
//			}
//			//else
//			//	return;
//		}

//		Vec3d middlePoint = p1+p2;
//		middlePoint.normalize();
//		if (!viewPortSphericalCap.contains(middlePoint))
//			middlePoint*=-1.;

//		// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
//		sPainter.drawGreatCircleArc(p1, middlePoint, Q_NULLPTR, alViewportEdgeIntersectCallback, &userData);
//		sPainter.drawGreatCircleArc(p2, middlePoint, Q_NULLPTR, alViewportEdgeIntersectCallback, &userData);

//		// OpenGL ES 2.0 doesn't have GL_LINE_SMOOTH
//		#ifdef GL_LINE_SMOOTH
//		if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
//			glDisable(GL_LINE_SMOOTH);
//		#endif

//		glDisable(GL_BLEND);

//		return;
	}
	/////////////////////////////////////////////////
	// Else draw small circles (declinations). (Technically, Equator is one, but ok...)
	else
	{
		// Draw the line
		const double lat=definingAngle*M_PI/180.0;
		SphericalCap declinationCap(Vec3d(0,0,1), std::sin(lat));
		const Vec3d rotCenter(0,0,declinationCap.d);

		Vec3d p1, p2;
		if (!SphericalCap::intersectionPoints(viewPortSphericalCap, declinationCap, p1, p2))
		{
			if ((viewPortSphericalCap.d<declinationCap.d && viewPortSphericalCap.contains(declinationCap.n))
					|| (viewPortSphericalCap.d<-declinationCap.d && viewPortSphericalCap.contains(-declinationCap.n)))
			{
				// The line is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
				Vec3d pt1;
				Vec3d pt2;
				Vec3d pt3;
				const double lon1=0.0;
				const double lon2=120.0*M_PI/180.0;
				const double lon3=240.0*M_PI/180.0;
				StelUtils::spheToRect(lon1, lat, pt1); pt1.normalize();
				StelUtils::spheToRect(lon2, lat, pt2); pt2.normalize();
				StelUtils::spheToRect(lon3, lat, pt3); pt3.normalize();

				sPainter.drawSmallCircleArc(pt1, pt2, rotCenter, alViewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(pt2, pt3, rotCenter, alViewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(pt3, pt1, rotCenter, alViewportEdgeIntersectCallback, &userData);
			}
		}
		else
		{
			// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
			Vec3d middlePoint = p1-rotCenter+p2-rotCenter;
			middlePoint.normalize();
			middlePoint*=(p1-rotCenter).length();
			middlePoint+=rotCenter;
			if (!viewPortSphericalCap.contains(middlePoint))
			{
				middlePoint-=rotCenter;
				middlePoint*=-1.;
				middlePoint+=rotCenter;
			}

			sPainter.drawSmallCircleArc(p1, middlePoint, rotCenter,alViewportEdgeIntersectCallback, &userData);
			sPainter.drawSmallCircleArc(p2, middlePoint, rotCenter, alViewportEdgeIntersectCallback, &userData);
		}
	}

	sPainter.setLineWidth(oldLineWidth); // restore
	sPainter.setLineSmooth(false);
	sPainter.setBlending(false);
}

void ArchaeoLine::setColor(const Vec3f& c)
{
	if (c!=color)
	{
		color = c;
		emit colorChanged(c);
	}
}
void ArchaeoLine::setDefiningAngle(const double angle)
{
	if (!qFuzzyCompare(angle, definingAngle))
	{
		definingAngle=angle;
		emit definingAngleChanged(angle);
	}
}
void ArchaeoLine::setLabelVisible(const bool b)
{
	if (b!=flagLabel)
	{
		flagLabel=b;
		emit flagLabelChanged(b);
	}
}
