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
#include <QtNetwork>
#include <QSettings>
#include <QMouseEvent>
#include <cmath>

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
	info.contact = "http://homepage.univie.ac.at/Georg.Zotti";
	info.description = N_("A tool for archaeo-/ethnoastronomical alignment studies");
	info.version = ARCHAEOLINES_VERSION;
	return info;
}

ArchaeoLines::ArchaeoLines()
	: flagShowArchaeoLines(false)
	, withDecimalDegree(false)
	, flagUseDmsFormat(false)
	, flagShowEquinox(false)
	, flagShowSolstices(false)
	, flagShowCrossquarters(false)
	, flagShowMajorStandstills(false)
	, flagShowMinorStandstills(false)
	, flagShowZenithPassage(false)
	, flagShowNadirPassage(false)
	, flagShowSelectedObject(false)
	, flagShowCurrentSun(false)
	, flagShowCurrentMoon(false)
	, enumShowCurrentPlanet(ArchaeoLine::CurrentPlanetNone)
	, lastJD(0.0)
	, toolbarButton(NULL)
{
	setObjectName("ArchaeoLines");
	font.setPixelSize(16);
	core=StelApp::getInstance().getCore();
	Q_ASSERT(core);
	objMgr=GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(objMgr);

	// optimize readabiity so that each upper line of the lunistice doubles is labeled.
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
	currentSunLine     = new ArchaeoLine(ArchaeoLine::CurrentSun, 0.0);
	currentMoonLine    = new ArchaeoLine(ArchaeoLine::CurrentMoon, 0.0);
	currentPlanetLine  = new ArchaeoLine(ArchaeoLine::CurrentPlanetNone, 0.0);

	configDialog = new ArchaeoLinesDialog();
	conf = StelApp::getInstance().getSettings();
}

ArchaeoLines::~ArchaeoLines()
{
	delete equinoxLine; equinoxLine=NULL;
	delete northernSolsticeLine; northernSolsticeLine=NULL;
	delete southernSolsticeLine; southernSolsticeLine=NULL;
	delete northernCrossquarterLine; northernCrossquarterLine=NULL;
	delete southernCrossquarterLine; southernCrossquarterLine=NULL;
	delete northernMajorStandstillLine0; northernMajorStandstillLine0=NULL;
	delete northernMajorStandstillLine1; northernMajorStandstillLine1=NULL;
	delete northernMinorStandstillLine2; northernMinorStandstillLine2=NULL;
	delete northernMinorStandstillLine3; northernMinorStandstillLine3=NULL;
	delete southernMinorStandstillLine4; southernMinorStandstillLine4=NULL;
	delete southernMinorStandstillLine5; southernMinorStandstillLine5=NULL;
	delete southernMajorStandstillLine6; southernMajorStandstillLine6=NULL;
	delete southernMajorStandstillLine7; southernMajorStandstillLine7=NULL;
	delete zenithPassageLine;  zenithPassageLine=NULL;
	delete nadirPassageLine;   nadirPassageLine=NULL;
	delete selectedObjectLine; selectedObjectLine=NULL;
	delete currentSunLine;     currentSunLine=NULL;
	delete currentMoonLine;    currentMoonLine=NULL;
	delete currentPlanetLine;  currentPlanetLine=NULL;

	delete configDialog; configDialog=NULL;
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
	  return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.; // same as lines from core (hint AW)
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
	Q_ASSERT(currentSunLine);
	Q_ASSERT(currentMoonLine);
	Q_ASSERT(currentPlanetLine);

	if (!conf->childGroups().contains("ArchaeoLines"))
		restoreDefaultSettings();

	loadSettings();

	StelApp& app = StelApp::getInstance();

	// Create action for enable/disable & hook up signals	
	addAction("actionShow_Archaeo_Lines", N_("ArchaeoLines"), N_("ArchaeoLines"), "enabled", "Ctrl+U");

	// Add a toolbar button
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
		if (gui!=NULL)
		{
			toolbarButton = new StelButton(NULL,
						       QPixmap(":/archaeoLines/bt_archaeolines_on.png"),
						       QPixmap(":/archaeoLines/bt_archaeolines_off.png"),
						       QPixmap(":/graphicGui/glow32x32.png"),
						       "actionShow_Archaeo_Lines");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to create toolbar button for ArchaeoLines plugin: " << e.what();
	}
}

void ArchaeoLines::update(double deltaTime)
{
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
	double dec_equ, ra_equ;
	StelUtils::rectToSphe(&ra_equ,&dec_equ,planet->getEquinoxEquatorialPos(core));
	currentSunLine->setDeclination(dec_equ * 180.0/M_PI);
	planet=ssystem->getMoon();
	StelUtils::rectToSphe(&ra_equ,&dec_equ,planet->getEquinoxEquatorialPos(core));
	currentMoonLine->setDeclination(dec_equ * 180.0/M_PI);

	if (enumShowCurrentPlanet>ArchaeoLine::CurrentPlanetNone)
	{
		const char *planetStrings[]={"", "Mercury", "Venus", "Mars", "Jupiter", "Saturn"};
		QString currentPlanet(planetStrings[enumShowCurrentPlanet - ArchaeoLine::CurrentPlanetNone]);
		planet=ssystem->searchByEnglishName(currentPlanet);
		Q_ASSERT(planet);
		StelUtils::rectToSphe(&ra_equ,&dec_equ,planet->getEquinoxEquatorialPos(core));
		currentPlanetLine->setDeclination(dec_equ * 180.0/M_PI);
	}

	double newJD=core->getJDay();
	if (fabs(newJD-lastJD) > 10.0) // enough to compute this every 10 days?
	{
		eps= ssystem->getEarth()->getRotObliquity(core->getJDay()) *180.0/M_PI;
		static const double invSqrt2=1.0/std::sqrt(2.0);
		northernSolsticeLine->setDeclination( eps);
		southernSolsticeLine->setDeclination(-eps);
		northernCrossquarterLine->setDeclination( eps*invSqrt2);
		southernCrossquarterLine->setDeclination(-eps*invSqrt2);
		lastJD=newJD;
	}
	StelLocation loc=core->getCurrentLocation();

	// compute parallax correction with Meeus 40.6. First, find H from h=0, then add corrections.

	static const double b_over_a=0.99664719;
	double latRad=loc.latitude*M_PI/180.0;
	double u=std::atan(b_over_a*std::tan(latRad));
	double rhoSinPhiP=b_over_a*std::sin(u)+loc.altitude/6378140.0*std::sin(latRad);
	double rhoCosPhiP=         std::cos(u)+loc.altitude/6378140.0*std::cos(latRad);

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
	northernMajorStandstillLine0->setDeclination(lunarDEtopo[0] *180.0/M_PI);
	northernMajorStandstillLine1->setDeclination(lunarDEtopo[1] *180.0/M_PI);
	northernMinorStandstillLine2->setDeclination(lunarDEtopo[2] *180.0/M_PI);
	northernMinorStandstillLine3->setDeclination(lunarDEtopo[3] *180.0/M_PI);
	southernMinorStandstillLine4->setDeclination(lunarDEtopo[4] *180.0/M_PI);
	southernMinorStandstillLine5->setDeclination(lunarDEtopo[5] *180.0/M_PI);
	southernMajorStandstillLine6->setDeclination(lunarDEtopo[6] *180.0/M_PI);
	southernMajorStandstillLine7->setDeclination(lunarDEtopo[7] *180.0/M_PI);

	zenithPassageLine->setDeclination(loc.latitude);
	nadirPassageLine->setDeclination(-loc.latitude);

	// Selected object?
	if (objMgr->getWasSelected())
	{
		StelObjectP obj=objMgr->getSelectedObject().first();
		StelUtils::rectToSphe(&ra_equ,&dec_equ,obj->getEquinoxEquatorialPos(core));
		selectedObjectLine->setDeclination(dec_equ * 180.0/M_PI);
		selectedObjectLine->setLabel(obj->getNameI18n());

	}

	// Updates for line brightness
	lineFader.update((int)(deltaTime*1000));
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
	currentSunLine->update(deltaTime);
	currentMoonLine->update(deltaTime);
	currentPlanetLine->update(deltaTime);

	withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();;
}


//! Draw any parts on the screen which are for our module
void ArchaeoLines::draw(StelCore* core)
{
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
		selectedObjectLine->draw(core, lineFader.getInterstate());
	currentSunLine->draw(core, lineFader.getInterstate());
	currentMoonLine->draw(core, lineFader.getInterstate());
	if (enumShowCurrentPlanet>ArchaeoLine::CurrentPlanetNone)
		currentPlanetLine->draw(core, lineFader.getInterstate());
}


void ArchaeoLines::enableArchaeoLines(bool b)
{
	flagShowArchaeoLines = b;
	lineFader = b;
}


//void ArchaeoLines::useDmsFormat(bool b)
//{
//	flagUseDmsFormat=b;
//	conf->setValue("ArchaeoLines/angle_format_dms",       isDmsFormat());
//}

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
	//useDmsFormat(conf->value("ArchaeoLines/angle_format_dms", false).toBool());

	equinoxColor         = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_equinox",          "1.00,1.00,0.5").toString());
	equinoxLine->setColor(equinoxColor);
	solsticesColor       = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_solstices",        "1.00,1.00,0.25").toString());
	northernSolsticeLine->setColor(solsticesColor);
	southernSolsticeLine->setColor(solsticesColor);
	crossquartersColor   = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_crossquarters",    "1.00,0.75,0.25").toString());
	northernCrossquarterLine->setColor(crossquartersColor);
	southernCrossquarterLine->setColor(crossquartersColor);
	majorStandstillColor = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_major_standstill", "0.25,1.00,0.25").toString());
	northernMajorStandstillLine0->setColor(majorStandstillColor);
	northernMajorStandstillLine1->setColor(majorStandstillColor);
	southernMajorStandstillLine6->setColor(majorStandstillColor);
	southernMajorStandstillLine7->setColor(majorStandstillColor);
	minorStandstillColor = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_minor_standstill", "0.20,0.75,0.20").toString());
	northernMinorStandstillLine2->setColor(minorStandstillColor);
	northernMinorStandstillLine3->setColor(minorStandstillColor);
	southernMinorStandstillLine4->setColor(minorStandstillColor);
	southernMinorStandstillLine5->setColor(minorStandstillColor);
	zenithPassageColor   = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_zenith_passage",   "1.00,0.75,0.75").toString());
	zenithPassageLine->setColor(zenithPassageColor);
	nadirPassageColor    = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_nadir_passage",    "1.00,0.75,0.75").toString());
	nadirPassageLine->setColor(nadirPassageColor);
	selectedObjectColor    = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_selected_object",    "1.00,1.00,1.00").toString());
	selectedObjectLine->setColor(selectedObjectColor);
	currentSunColor    = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_current_sun",    "1.00,1.00,0.75").toString());
	currentSunLine->setColor(currentSunColor);
	currentMoonColor    = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_current_moon",    "0.50,1.00,0.50").toString());
	currentMoonLine->setColor(currentMoonColor);
	currentPlanetColor    = StelUtils::strToVec3f(conf->value("ArchaeoLines/color_current_planet",    "0.25,0.80,1.00").toString());
	currentPlanetLine->setColor(currentPlanetColor);

	// 5 solar limits
	showEquinox(conf->value("ArchaeoLines/show_equinox", true).toBool());
	showSolstices(conf->value("ArchaeoLines/show_solstices", true).toBool());
	showCrossquarters(conf->value("ArchaeoLines/show_crossquarters", true).toBool());
	// 4 lunar limits
	showMajorStandstills(conf->value("ArchaeoLines/show_major_standstills", true).toBool());
	showMinorStandstills(conf->value("ArchaeoLines/show_minor_standstills", true).toBool());
	// esp. Mesoamerica
	showZenithPassage(conf->value("ArchaeoLines/show_zenith_passage", true).toBool());
	showNadirPassage(conf->value("ArchaeoLines/show_nadir_passage",  true).toBool());
	// indicator for a line representing currently selected object
	showSelectedObject(conf->value("ArchaeoLines/show_selected_object", false).toBool());
	// indicators for current declinations (those move fast over days...)
	showCurrentSun(conf->value("ArchaeoLines/show_current_sun", true).toBool());
	showCurrentMoon(conf->value("ArchaeoLines/show_current_moon", true).toBool());
	showCurrentPlanet(conf->value("ArchaeoLines/show_current_planet", "none").toString());
}

void ArchaeoLines::showEquinox(bool b)
{
	flagShowEquinox=b;
	conf->setValue("ArchaeoLines/show_equinox",         isEquinoxDisplayed());
	equinoxLine->setDisplayed(b);
}
void ArchaeoLines::showSolstices(bool b)
{
	flagShowSolstices=b;
	conf->setValue("ArchaeoLines/show_solstices",         isSolsticesDisplayed());
	northernSolsticeLine->setDisplayed(b);
	southernSolsticeLine->setDisplayed(b);
}
void ArchaeoLines::showCrossquarters(bool b)
{
	flagShowCrossquarters=b;
	conf->setValue("ArchaeoLines/show_crossquarters",     isCrossquartersDisplayed());
	northernCrossquarterLine->setDisplayed(b);
	southernCrossquarterLine->setDisplayed(b);
}
void ArchaeoLines::showMajorStandstills(bool b)
{
	flagShowMajorStandstills=b;
	conf->setValue("ArchaeoLines/show_major_standstills", isMajorStandstillsDisplayed());
	northernMajorStandstillLine0->setDisplayed(b);
	northernMajorStandstillLine1->setDisplayed(b);
	southernMajorStandstillLine6->setDisplayed(b);
	southernMajorStandstillLine7->setDisplayed(b);
}
void ArchaeoLines::showMinorStandstills(bool b)
{
	flagShowMinorStandstills=b;
	conf->setValue("ArchaeoLines/show_minor_standstills", isMinorStandstillsDisplayed());
	northernMinorStandstillLine2->setDisplayed(b);
	northernMinorStandstillLine3->setDisplayed(b);
	southernMinorStandstillLine4->setDisplayed(b);
	southernMinorStandstillLine5->setDisplayed(b);
}
void ArchaeoLines::showZenithPassage(bool b)
{
	flagShowZenithPassage=b;
	conf->setValue("ArchaeoLines/show_zenith_passage",      isZenithPassageDisplayed());
	zenithPassageLine->setDisplayed(b);
}
void ArchaeoLines::showNadirPassage(bool b)
{
	flagShowNadirPassage=b;
	conf->setValue("ArchaeoLines/show_nadir_passage",       isNadirPassageDisplayed());
	nadirPassageLine->setDisplayed(b);
}
void ArchaeoLines::showSelectedObject(bool b)
{
	flagShowSelectedObject=b;
	conf->setValue("ArchaeoLines/show_selected_object",       isSelectedObjectDisplayed());
	selectedObjectLine->setDisplayed(b);
}
void ArchaeoLines::showCurrentSun(bool b)
{
	flagShowCurrentSun=b;
	conf->setValue("ArchaeoLines/show_current_sun",       isCurrentSunDisplayed());
	currentSunLine->setDisplayed(b);
}
void ArchaeoLines::showCurrentMoon(bool b)
{
	flagShowCurrentMoon=b;
	conf->setValue("ArchaeoLines/show_current_moon",       isCurrentMoonDisplayed());
	currentMoonLine->setDisplayed(b);
}

void ArchaeoLines::showCurrentPlanet(ArchaeoLine::Line l)
{
	enumShowCurrentPlanet=l;
	const char *planetStrings[]={"none", "Mercury", "Venus", "Mars", "Jupiter", "Saturn"};

	conf->setValue("ArchaeoLines/show_current_planet", planetStrings[l-ArchaeoLine::CurrentPlanetNone]);
	currentPlanetLine->setLineType(enumShowCurrentPlanet);
	currentPlanetLine->setDisplayed(enumShowCurrentPlanet != ArchaeoLine::CurrentPlanetNone);
}

void ArchaeoLines::showCurrentPlanet(QString planet)
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
}

// called by the dialog UI, converts QColor (0..255) to Stellarium's Vec3f float color.
void ArchaeoLines::setLineColor(ArchaeoLine::Line whichLine, QColor color)
{
	switch (whichLine){
		case ArchaeoLine::Equinox:
			equinoxColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_equinox",        QString("%1,%2,%3").arg(equinoxColor.v[0]).arg(equinoxColor.v[1]).arg(equinoxColor.v[2]));
			equinoxLine->setColor(equinoxColor);
			break;
		case ArchaeoLine::Solstices:
			solsticesColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_solstices",        QString("%1,%2,%3").arg(solsticesColor.v[0]).arg(solsticesColor.v[1]).arg(solsticesColor.v[2]));
			northernSolsticeLine->setColor(solsticesColor);
			southernSolsticeLine->setColor(solsticesColor);
			break;
		case ArchaeoLine::Crossquarters:
			crossquartersColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_crossquarters",    QString("%1,%2,%3").arg(crossquartersColor.v[0]).arg(crossquartersColor.v[1]).arg(crossquartersColor.v[2]));
			northernCrossquarterLine->setColor(crossquartersColor);
			southernCrossquarterLine->setColor(crossquartersColor);
			break;
		case ArchaeoLine::MajorStandstill:
			majorStandstillColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_major_standstill", QString("%1,%2,%3").arg(majorStandstillColor.v[0]).arg(majorStandstillColor.v[1]).arg(majorStandstillColor.v[2]));
			northernMajorStandstillLine0->setColor(majorStandstillColor);
			southernMajorStandstillLine7->setColor(majorStandstillColor);
			northernMajorStandstillLine1->setColor(majorStandstillColor);
			southernMajorStandstillLine6->setColor(majorStandstillColor);
			break;
		case ArchaeoLine::MinorStandstill:
			minorStandstillColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_minor_standstill", QString("%1,%2,%3").arg(minorStandstillColor.v[0]).arg(minorStandstillColor.v[1]).arg(minorStandstillColor.v[2]));
			northernMinorStandstillLine2->setColor(minorStandstillColor);
			southernMinorStandstillLine4->setColor(minorStandstillColor);
			northernMinorStandstillLine3->setColor(minorStandstillColor);
			southernMinorStandstillLine5->setColor(minorStandstillColor);
			break;
		case ArchaeoLine::ZenithPassage:
			zenithPassageColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_zenith_passage",     QString("%1,%2,%3").arg(zenithPassageColor.v[0]).arg(zenithPassageColor.v[1]).arg(zenithPassageColor.v[2]));
			zenithPassageLine->setColor(zenithPassageColor);
			break;
		case ArchaeoLine::NadirPassage:
			nadirPassageColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_nadir_passage",      QString("%1,%2,%3").arg(nadirPassageColor.v[0]).arg(nadirPassageColor.v[1]).arg(nadirPassageColor.v[2]));
			nadirPassageLine->setColor(nadirPassageColor);
			break;
		case ArchaeoLine::SelectedObject:
			selectedObjectColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_selected_object",      QString("%1,%2,%3").arg(selectedObjectColor.v[0]).arg(selectedObjectColor.v[1]).arg(selectedObjectColor.v[2]));
			selectedObjectLine->setColor(selectedObjectColor);
			break;
		case ArchaeoLine::CurrentSun:
			currentSunColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_current_sun",      QString("%1,%2,%3").arg(currentSunColor.v[0]).arg(currentSunColor.v[1]).arg(currentSunColor.v[2]));
			currentSunLine->setColor(currentSunColor);
			break;
		case ArchaeoLine::CurrentMoon:
			currentMoonColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_current_moon",      QString("%1,%2,%3").arg(currentMoonColor.v[0]).arg(currentMoonColor.v[1]).arg(currentMoonColor.v[2]));
			currentMoonLine->setColor(currentMoonColor);
			break;
		case ArchaeoLine::CurrentPlanetNone:
		case ArchaeoLine::CurrentPlanetMercury:
		case ArchaeoLine::CurrentPlanetVenus:
		case ArchaeoLine::CurrentPlanetMars:
		case ArchaeoLine::CurrentPlanetJupiter:
		case ArchaeoLine::CurrentPlanetSaturn:
			currentPlanetColor.set(color.redF(), color.greenF(), color.blueF());
			conf->setValue("ArchaeoLines/color_current_planet",      QString("%1,%2,%3").arg(currentPlanetColor.v[0]).arg(currentPlanetColor.v[1]).arg(currentPlanetColor.v[2]));
			currentPlanetLine->setColor(currentPlanetColor);
			break;
		default:
			Q_ASSERT(0);
	}
}

// called by the dialog UI, converts Stellarium's Vec3f float color to QColor (0..255).
QColor ArchaeoLines::getLineColor(ArchaeoLine::Line whichLine)
{
	QColor color(0,0,0);
	Vec3f* vColor;
	switch (whichLine){
		case ArchaeoLine::Equinox:
			vColor=&equinoxColor;
			break;
		case ArchaeoLine::Solstices:
			vColor=&solsticesColor;
			break;
		case ArchaeoLine::Crossquarters:
			vColor=&crossquartersColor;
			break;
		case ArchaeoLine::MajorStandstill:
			vColor=&majorStandstillColor;
			break;
		case ArchaeoLine::MinorStandstill:
			vColor=&minorStandstillColor;
			break;
		case ArchaeoLine::ZenithPassage:
			vColor=&zenithPassageColor;
			break;
		case ArchaeoLine::NadirPassage:
			vColor=&nadirPassageColor;
			break;
		case ArchaeoLine::SelectedObject:
			vColor=&selectedObjectColor;
			break;
		case ArchaeoLine::CurrentSun:
			vColor=&currentSunColor;
			break;
		case ArchaeoLine::CurrentMoon:
			vColor=&currentMoonColor;
			break;
		case ArchaeoLine::CurrentPlanetNone:
		case ArchaeoLine::CurrentPlanetMercury:
		case ArchaeoLine::CurrentPlanetVenus:
		case ArchaeoLine::CurrentPlanetMars:
		case ArchaeoLine::CurrentPlanetJupiter:
		case ArchaeoLine::CurrentPlanetSaturn:
			vColor=&currentPlanetColor;
			break;
		default:
			vColor=&selectedObjectColor; // this is only to silence compiler warning about uninitialized variable vColor.
			Q_ASSERT(0);
	}
	color.setRgbF(vColor->v[0], vColor->v[1], vColor->v[2]);
	return color;
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

	double angleDeg = std::atan2(-direc[1], -direc[0])*180./M_PI;
	float xshift=6.f;
	if (angleDeg>90. || angleDeg<-90.)
	{
		angleDeg+=180.;
		xshift=-d->sPainter->getFontMetrics().width(text)-6.f;
	}

	d->sPainter->drawText(screenPos[0], screenPos[1], text, angleDeg, xshift, 3);
	//d->sPainter->setColor(tmpColor[0], tmpColor[1], tmpColor[2], tmpColor[3]); // RESTORE
}


ArchaeoLine::ArchaeoLine(ArchaeoLine::Line lineType, double declination) :
	lineType(lineType), declination(declination), color(0.f, 0.f, 1.f), frameType(StelCore::FrameEquinoxEqu), flagLabel(true)
{
	// Font size is 14
	font.setPixelSize(StelApp::getInstance().getBaseFontSize()+1);
	updateLabel();
	fader.setDuration(1000);
	// Initialize the message strings and make sure they are translated when the language changes.
	StelApp& app = StelApp::getInstance();
	//updateLabel(); // WHY AGAIN??
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateLabel()));

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
	sPainter.setColor(color[0], color[1], color[2], intensity*fader.getInterstate());
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	// OpenGL ES 2.0 doesn't have GL_LINE_SMOOTH
	#ifdef GL_LINE_SMOOTH
	if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
		glEnable(GL_LINE_SMOOTH);
	#endif

	//Vec4f textColor(color[0], color[1], color[2], intensity*fader.getInterstate());

	ALViewportEdgeIntersectCallbackData userData(&sPainter);
	sPainter.setFont(font);
	//userData.textColor = textColor;
	userData.text = (isLabelVisible() ? label : "");
	/////////////////////////////////////////////////
	// Draw the line
	const double lat=declination*M_PI/180.0;
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
			#ifdef GL_LINE_SMOOTH
			if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
				glDisable(GL_LINE_SMOOTH);
			#endif
			glDisable(GL_BLEND);
			return;
		}
		else
		{
			#ifdef GL_LINE_SMOOTH
			if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
				glDisable(GL_LINE_SMOOTH);
			#endif
			glDisable(GL_BLEND);
			return;
		}
	}
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

	// OpenGL ES 2.0 doesn't have GL_LINE_SMOOTH
	#ifdef GL_LINE_SMOOTH
	if (QOpenGLContext::currentContext()->format().renderableType()==QSurfaceFormat::OpenGL)
		glDisable(GL_LINE_SMOOTH);
	#endif

	glDisable(GL_BLEND);
}
