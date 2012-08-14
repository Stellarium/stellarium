/*
 * Copyright (C) 2012 Ivan Marti-Vidal
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

#include <QSettings>
#include <QPixmap>
#include <QTimer>
#include <QtOpenGL>
#include <QString>
#include <QDebug>
#include <QAction>
#include <QKeyEvent>
#include <QtNetwork>
#include <QKeyEvent>
#include <QMouseEvent>

#include "StelIniParser.hpp"
#include "StelProjector.hpp"
#include "StarMgr.hpp"
#include "StelObject.hpp"
#include "StelObserver.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelMovementMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelVertexArray.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "ZoneArray.hpp"
#include "StelSkyDrawer.hpp"
#include "Observability.hpp"
#include "ObservabilityDialog.hpp"

#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelFader.hpp"

StelModule* ObservabilityStelPluginInterface::getStelModule() const
{
        return new Observability();
}

StelPluginInfo ObservabilityStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Observability);

        StelPluginInfo info;
	info.id = "Observability";
        info.displayedName = N_("Observability analysis");
	info.authors = N_("Ivan Marti-Vidal (Onsala Space Observatory)");
	info.contact = "i.martividal@gmail.com";
        info.description = N_("Reports an analysis of source observability (rise, set, and transit times), as well as the epochs of year when the source is best observed. It assumes that a source is observable if it is above the horizon during a fraction of the night. The plugin also gives the day for largest separation from the Sun and the days of Acronychal and Cosmical rise/set.<br><br> An explanation of the quantities shown by this script is given in the 'About' tab of the configuration window");
        return info;
}

Q_EXPORT_PLUGIN2(Observability, ObservabilityStelPluginInterface)

Observability::Observability()
	: flagShowObservability(false), OnIcon(NULL), OffIcon(NULL), GlowIcon(NULL),toolbarButton(NULL)
{
	setObjectName("Observability");
	configDialog = new ObservabilityDialog();


	// Some useful constants:
	Rad2Deg = 180./3.1415927; // Convert degrees into radians
	Rad2Hr = 12./3.1415927;  // Convert hours into radians
	UA = 1.4958e+8;         // Astronomical Unit in Km.
	TFrac = 0.9972677595628414;  // Convert sidereal time into Solar time
	JDsec = 1./86400.;      // A second in days.
	halfpi = 1.57079632675; // pi/2
	MoonT = 29.530588; // Moon synodic period (used as first estimate of Full Moon).
	RefFullMoon = 2451564.696; // Reference Julian date of a Full Moon.
	nextFullMoon = 0.0;
	prevFullMoon = 0.0;
	selName = "";



////////////////////////////
// Read configuration:

	QSettings* conf = StelApp::getInstance().getSettings();
	// Setup defaults if not present
	if (!conf->contains("Observability/font_size"))
		conf->setValue("Observability/font_size", 15);

	if (!conf->contains("Observability/font_color"))
		conf->setValue("Observability/font_color", "0,0.5,1");

	if (!conf->contains("Observability/show_AcroCos"))
		conf->setValue("Observability/show_AcroCos", true);

	if (!conf->contains("Observability/show_Good_Nights"))
		conf->setValue("Observability/show_Good_Nights", true);

	if (!conf->contains("Observability/show_Best_Night"))
		conf->setValue("Observability/show_Best_Night", true);

	if (!conf->contains("Observability/show_Today"))
		conf->setValue("Observability/show_Today", true);

	if (!conf->contains("Observability/Sun_Altitude"))
		conf->setValue("Observability/Sun_Altitude", 12);

	if (!conf->contains("Observability/show_FullMoon"))
		conf->setValue("Observability/show_FullMoon", true);

//	if (!conf->contains("Observability/show_Crescent"))
//		conf->setValue("Observability/show_Crescent", true);

//	if (!conf->contains("Observability/show_SuperMoon"))
//		conf->setValue("Observability/show_SuperMoon", true);


	// Load settings from main config file
	fontSize = conf->value("Observability/font_size",15).toInt();
	iAltitude = conf->value("Observability/Sun_Altitude",12).toInt();
	AstroTwiAlti = -((double) iAltitude)/Rad2Deg ;
	font.setPixelSize(fontSize);
	QString fontColorStr = conf->value("Observability/font_color", "0,0.5,1").toString();
	fontColor = StelUtils::strToVec3f(fontColorStr);
	show_AcroCos = conf->value("Observability/show_AcroCos", true).toBool();
	show_Good_Nights = conf->value("Observability/show_Good_Nights", true).toBool();
	show_Best_Night = conf->value("Observability/show_Best_Night", true).toBool();
	show_Today = conf->value("Observability/show_Today", true).toBool();
	show_FullMoon = conf->value("Observability/show_FullMoon", true).toBool();
//	show_Crescent = conf->value("Observability/show_Crescent", true).toBool();
//	show_SuperMoon = conf->value("Observability/show_SuperMoon", true).toBool();

/////////////////////////////////



	// Dummy initial values for parameters and data vectors:
	mylat = 1000.; mylon = 1000.;
	myJD = 0.0;
	currYear = 0;
	isStar = true;
	isMoon = false;
	isSun = false;
	isScreen = true;
	raised=false;

	ObserverLoc[0]=0.0;ObserverLoc[1]=0.0;ObserverLoc[2]=0.0;

//Get pointer to the Earth:
	PlanetP Earth = GETSTELMODULE(SolarSystem)->getEarth();
	myEarth = Earth.data();

// Get pointer to the Moon/Sun:
	PlanetP Moon = GETSTELMODULE(SolarSystem)->getMoon();
	myMoon = Moon.data();


	for (int i=0;i<366;i++) {
		SunRA[i] = 0.0; SunDec[i] = 0.0;
		ObjectRA[i] = 0.0; ObjectDec[i]=0.0;
		SunSidT[0][i]=0.0; SunSidT[1][i]=0.0;
		ObjectSidT[0][i]=0.0; ObjectSidT[1][i]=0.0;
		ObjectH0[i] = 0.0;
		yearJD[i] = 0.0;
	};


	// Set names of the months:
	QString mons[12]={q_("January"), q_("February"), q_("March"), q_("April"), q_("May"), q_("June"), q_("July"), q_("August"), q_("September"), q_("October"), q_("November"), q_("December")};
	for (int i=0;i<12;i++) {
	 months[i]=mons[i];
	};

}

Observability::~Observability()
{
	if (GlowIcon!=NULL)
		delete GlowIcon;
	if (OnIcon!=NULL)
		delete OnIcon;
	if (OffIcon!=NULL)
		delete OffIcon;
	delete configDialog;
}

double Observability::getCallOrder(StelModuleActionName actionName) const
{
       if (actionName==StelModule::ActionDraw)
               return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
       return 0;
}

void Observability::init()
{
       qDebug() << "init called for Observability";

	try 
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		GlowIcon = new QPixmap(":/graphicGui/glow32x32.png");
		OnIcon = new QPixmap(":/observability/bt_observab_on.png");
		OffIcon = new QPixmap(":/observability/bt_observab_off.png");

		gui->getGuiAction("actionShow_Observability")->setChecked(flagShowObservability);
		toolbarButton = new StelButton(NULL, *OnIcon, *OffIcon, *GlowIcon, gui->getGuiAction("actionShow_Observability"));
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		connect(gui->getGuiAction("actionShow_Observability"), SIGNAL(toggled(bool)), this, SLOT(enableObservability(bool)));
		connect(gui->getGuiAction("actionShow_Observability_ConfigDialog"), SIGNAL(toggled(bool)), configDialog, SLOT(setVisible(bool)));
		connect(configDialog, SIGNAL(visibleChanged(bool)), gui->getGuiAction("actionShow_Observability_ConfigDialog"), SLOT(setChecked(bool)));
	}
	catch (std::exception &e)
	{
		qWarning() << "WARNING: unable create toolbar button for Observability plugin (or load gonfig GUI). " << e.what();
	};
}

/////////////////////////////////////////////
// MAIN CODE:
void Observability::draw(StelCore* core)
{

	if (!flagShowObservability) return; // Button is off.

/////////////////////////////////////////////////////////////////
// PRELIMINARS:
	bool souChanged, locChanged, yearChanged;
	StelObjectP selectedObject;
	Planet* currPlanet;


// Only execute plugin if we are on Earth.
	if (core->getCurrentLocation().planetName != "Earth") {return;};

// Set the painter:
	StelPainter paintresult(core->getProjection2d());
	paintresult.setColor(fontColor[0],fontColor[1],fontColor[2],1);
	font.setPixelSize(fontSize);
	paintresult.setFont(font);


// Get current date, location, and check if there is something selected.
	double currlat = (core->getCurrentLocation().latitude)/Rad2Deg;
	double currlon = (core->getCurrentLocation().longitude)/Rad2Deg;
	double currheight = (6371.+(core->getCurrentLocation().altitude)/1000.)/UA;
	double currJD = core->getJDay();
	double currJDint;
	double currLocalT = 24.*modf(currJD + StelApp::getInstance().getLocaleMgr().getGMTShift(currJD)/24.0,&currJDint);

	int auxm, auxd, auxy;
	StelUtils::getDateFromJulianDay(currJD,&auxy,&auxm,&auxd);
	bool isSource = StelApp::getInstance().getStelObjectMgr().getWasSelected();
	bool show_Year = show_Best_Night || show_Good_Nights || show_AcroCos; // || show_FullMoon;

//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// NOW WE CHECK THE CHANGED PARAMETERS W.R.T. THE PREVIOUS FRAME:

// Update JD.
	myJD = currJD;

// If we have changed the year, we must recompute the Sun's position for each new day:
	if (auxy != currYear) {
		yearChanged = true;
		currYear = auxy;
		SunRADec(core);}
	else {
		yearChanged = false;
	};

// Have we changed the latitude or longitude?
	if (currlat == mylat && currlon == mylon) {
		locChanged = false;}
	else {
		locChanged = true;
		mylat = currlat; mylon = currlon;
		double temp1 = currheight*std::cos(currlat);
		ObserverLoc[0] = temp1*std::cos(currlon);
		ObserverLoc[1] = temp1*std::sin(currlon);
		ObserverLoc[2] = currheight*std::sin(currlat);
	};

// If we have changed latitude (or year), we update the vector of Sun's hour 
// angles at twilight, and re-compute Sun/Moon ephemeris (if selected):
	if (locChanged || yearChanged || configChanged) 
	{
		SunHTwi();
		lastJDMoon = 0.0;
	};

//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// NOW WE DEAL WITH THE SOURCE (OR SCREEN-CENTER) POSITION:

	if (isScreen) souChanged=true; // Always re-compute everything for the screen center.

	if (isSource) { // There is something selected!

// Get the selected source and its name:
		selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]; 

// Don't do anything for satellites:
		if(selectedObject->getType()== "Satellite") return;

		QString tempName = selectedObject->getEnglishName();

// Check if the source is Sun or Moon (i.e., it changes quite a bit during one day):
		isMoon = ("Moon" == tempName)?true:false;
		isSun = ("Sun" == tempName)?true:false;

//Update position:
		EquPos = selectedObject->getEquinoxEquatorialPos(core);
		EquPos.normalize();
		LocPos = core->equinoxEquToAltAz(EquPos);

// Check if the user has changed the source (or if the source is Sun/Moon). 
		if (tempName == selName && isMoon==false && isSun == false) {
			souChanged = false;} // Don't retouch anything regarding RA/Dec (to save a bit of resources).
		else { // Check also if the (new) source belongs to the Solar System:
			currPlanet = dynamic_cast<Planet*>(selectedObject.data());
			isStar = (currPlanet)?false:true;
			souChanged = true;
			selName = tempName;
		};
	}
	else { // There is no source selected!

// If no source is selected, get the position vector of the screen center:
		selName = ""; isStar=true; isMoon = false; isSun = false; isScreen=true;
		Vec3d currentPos = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
		currentPos.normalize();
		EquPos = core->j2000ToEquinoxEqu(currentPos);
		LocPos = core->j2000ToAltAz(currentPos);
	}


// Convert EquPos to RA/Dec:
	toRADec(EquPos,selRA,selDec);

// Compute source's altitude (in radians):
	alti = std::asin(LocPos[2]);

// Force re-computation of ephemeris if the location changes or the user changes the configuration:
	if (locChanged || configChanged)
	{ 
		souChanged=true;
		configChanged=false;
	};

/////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////
// NOW WE COMPUTE RISE/SET/TRANSIT TIMES FOR THE CURRENT DAY:
	double currH = HourAngle(mylat,alti,selDec);
	horizH = HourAngle(mylat,0.0,selDec);
	QString RS1, RS2, Cul; // strings with Rise/Set/Culmination times
	double Rise, Set; // Actual Rise/Set times (in GMT).
	int d1,m1,s1,d2,m2,s2,dc,mc,sc; // Integers for the time spans in hh:mm:ss.
	bool solvedMoon = false; 
	bool transit; // Is the source above the horizon? Did it culminate?

	int ephHour, ephMinute, ephSecond;  // Local time for selected ephemeris

	if (show_Today) {  // We show ephemeris for today (i.e., rise, set, and transit times).

		if (isMoon || isSun) {
			solvedMoon = MoonSunSolve(core);  // False if fails; True otherwise.
			currH = std::abs(24.*(MoonCulm-myJD)/TFrac);
			transit = MoonCulm-myJD<0.0;
			if (solvedMoon) {  // If failed, Set and Rise will be dummy.
				Set = std::abs(24.*(MoonSet-myJD)/TFrac);
				Rise = std::abs(24.*(MoonRise-myJD)/TFrac);
			};
		} 
		else if (horizH>0.0) { // The source is not circumpolar and can be seen from this latitude.

			if ( LocPos[1]>0.0 ) {  // The source is at the eastern side...
				if ( currH>horizH ) {  // ... and below the horizon.
					Set = 24.-currH-horizH;
					Rise = currH-horizH;
					raised = false;}
				else { // ... and above the horizon.
					Rise = horizH-currH;
					Set = 2.*horizH-Rise;
					raised = true;};
			} 
			else { // The source is at the western side...
				if ( currH>horizH ) { // ... and below the horizon. 
					Set = currH-horizH;
					Rise = 24.-currH-horizH;
					raised = false;}
				else { // ... and above the horizon.
					Rise = horizH+currH;
					Set = horizH-currH;
					raised = true;};
			};
	
		};

		if ((solvedMoon && MoonRise>0.0) || (isMoon==false && isSun==false && horizH>0.0))
		{
			double2hms(TFrac*Set,d1,m1,s1);
			double2hms(TFrac*Rise,d2,m2,s2);

//		Strings with time spans for rise/set/transit:
			RS1 = (d1==0)?"":QString("%1%2 ").arg(d1).arg(q_("h")); RS1 += (m1==0)?"":QString("%1%2 ").arg(m1).arg(q_("m")); RS1 += QString("%1%2").arg(s1).arg(q_("s"));
			RS2 = (d2==0)?"":QString("%1%2 ").arg(d2).arg(q_("h")); RS2 += (m2==0)?"":QString("%1%2 ").arg(m2).arg(q_("m")); RS2 += QString("%1%2").arg(s2).arg(q_("s"));
			if (raised) 
			{
                                double2hms(toUnsignedRA(currLocalT+TFrac*Set+12.),ephHour,ephMinute,ephSecond);
				SetTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0')); // Local time for set.

                                double2hms(toUnsignedRA(currLocalT-TFrac*Rise+12.),ephHour,ephMinute,ephSecond); // Local time for rise.
				RiseTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));

				RS1 = q_("Sets at %1 (in %2)").arg(SetTime).arg(RS1);
				RS2 = q_("Raised at %1 (%2 ago)").arg(RiseTime).arg(RS2);
			}
			else 
			{
                                double2hms(toUnsignedRA(currLocalT-TFrac*Set+12.),ephHour,ephMinute,ephSecond);
				SetTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));

                                double2hms(toUnsignedRA(currLocalT+TFrac*Rise+12.),ephHour,ephMinute,ephSecond);
				RiseTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));

				RS1 = q_("Has set at %1 (%2 ago)").arg(SetTime).arg(RS1);
				RS2 = q_("Raises at %1 (in %2)").arg(RiseTime).arg(RS2);
			};				
		}
		else { // The source is either circumpolar or never rises:
			(alti>0.0)? RS1 = q_("Circumpolar."): RS1 = q_("No rise.");
			RS2 = "";
		};

// 	Culmination:

	transit = false;
	if (isSun==false && isMoon == false)
	{
		culmAlt = std::abs(mylat-selDec); // 90.-altitude at transit.
		transit = LocPos[1]<0.0;
	};

	if (culmAlt<halfpi) { // Source can be observed.
		double altiAtCulmi = Rad2Deg*(halfpi-culmAlt);
		double2hms(TFrac*currH,dc,mc,sc);

//		String with the time span for culmination:	
		Cul = (dc==0)?"":QString("%1%2 ").arg(dc).arg(q_("h")); Cul += (mc==0)?"":QString("%1%2 ").arg(mc).arg(q_("m")); Cul += QString("%1%2").arg(sc).arg(q_("s"));
		if (transit==false) { 

                        double2hms(toUnsignedRA(currLocalT+TFrac*currH+12.),ephHour,ephMinute,ephSecond); // Local time at transit.
			CulmTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
			Cul = q_("Culminates at %1 (in %2) at %3 deg.").arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);}
		else {
                        double2hms(toUnsignedRA(currLocalT-TFrac*currH+12.),ephHour,ephMinute,ephSecond);
			CulmTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
			Cul = q_("Culminated at %1 (%2 ago) at %3 deg.").arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
		};
	};

	}; // This comes from show_Today==True
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// NOW WE ANALYZE THE SOURCE OBSERVABILITY FOR THE WHOLE YEAR:

// Compute yearly ephemeris (only if necessary, and not for Sun nor Moon):

//	if (isMoon && (show_FullMoon || show_Crescent || show_SuperMoon)) 
//	{
//		computeMoonEphemeris();
//	};

	if (isSun) 
	{
		bestNight=""; ObsRange = "";
	}
	else if (!isMoon && show_Year) {

		if (isStar==false && (souChanged || yearChanged)) { // Object moves.
			PlanetRADec(core,selName);} // Re-compute ephemeris.

		else { // Object is fixed on the sky.
			double auxH = HourAngle(mylat,0.0,selDec);
			double auxSidT1 = toUnsignedRA(selRA - auxH); 
			double auxSidT2 = toUnsignedRA(selRA + auxH); 
			for (int i=0;i<nDays;i++) {
				ObjectH0[i] = auxH;
				ObjectRA[i] = selRA;
				ObjectDec[i] = selDec;
				ObjectSidT[0][i] = auxSidT1;
				ObjectSidT[1][i] = auxSidT2;
			};
		};

// Determine source observability (only if something changed):
		if ((souChanged || locChanged || yearChanged)) {
			bestNight=""; ObsRange = "";

			if (culmAlt>=halfpi) { // Source cannot be seen.
				ObsRange = q_("Source is not observable.");
				AcroCos = q_("No Acronychal nor Cosmical rise/set.");
			}
			else { // Source can be seen.

///////////////////////////
// - Part 1. Determine the best observing night (i.e., opposition to the Sun):
				if (show_Best_Night) {
					int selday = 0;
					double deltaPhs = -1.0; // Initial dummy value
					double tempPhs; //, tempSep;
	
					for (int i=0;i<nDays;i++) { // Maximize the Sun-object separation.
						tempPhs = Lambda(ObjectRA[i],ObjectDec[i],SunRA[i],SunDec[i]);
						if (tempPhs>deltaPhs) {selday=i;deltaPhs=tempPhs;};
					};
					bestNight = q_("Largest Sun separation: ");
					bestNight = bestNight + CalenDate(selday) + q_(" (at %1 deg.)").arg(deltaPhs*Rad2Deg,0,'f',1);
				};

///////////////////////////////
// - Part 2. Determine Acronychal and Cosmical rise and set:

				if (show_AcroCos) {
					int selRise, selSet, selRise2, selSet2; // days of year for Acronical and Cosmical rise/set.
					int Acro = CheckAcro(selRise,selSet,selRise2,selSet2);
					QString AcroRiseStr, AcroSetStr;
					QString CosmRiseStr, CosmSetStr;

					AcroRiseStr = (selRise>0)?CalenDate(selRise):q_("N/A");
					AcroSetStr = (selSet>0)?CalenDate(selSet):q_("N/A");

					CosmRiseStr = (selRise2>0)?CalenDate(selRise2):q_("N/A");
					CosmSetStr = (selSet2>0)?CalenDate(selSet2):q_("N/A");

					AcroCos = (Acro==3 || Acro==1)?QString("%1: %2/%3.").arg(q_("Acronychal rise/set")).arg(AcroRiseStr).arg(AcroSetStr):q_("No Acronychal rise/set.");
					AcroCos += (Acro==3 || Acro==2)?QString(" %1: %2/%3.").arg(q_("Cosmical rise/set")).arg(CosmRiseStr).arg(CosmSetStr):QString(" %1").arg(q_("No Cosmical rise/set."));

				};


////////////////////////////
// - Part 3. Determine range of good nights 
// (i.e., above horizon before/after twilight):

				if (show_Good_Nights) {
					int selday = 0;
					int selday2 = 0;
					bool bestBegun = false; // Are we inside a good time range?
					bool atLeastOne = false;
					QString dateRange = "";
					bool PoleNight, twiGood;

					for (int i=0;i<nDays;i++) {

						PoleNight = SunSidT[0][i]<0.0 && std::abs(SunDec[i]-mylat)>=halfpi; // Is it night during 24h?
						twiGood = (PoleNight && std::abs(ObjectDec[i]-mylat)<halfpi)?true:CheckRise(i);
						if (twiGood && bestBegun == false) {
							selday = i;
							bestBegun = true;
							atLeastOne = true;
						};

						if (!twiGood && bestBegun == true) {
							selday2 = i;
							bestBegun = false;
							if (selday2 > selday) {
								if (dateRange!="") { dateRange += ", ";};
								dateRange += q_("from %1 to %2").arg(CalenDate(selday)).arg(CalenDate(selday2));
							};
						};
					};

					if (bestBegun) { // There were good dates till the end of the year.
						 if (dateRange!="") { dateRange += ", ";};
						dateRange += CalenDate(selday)+q_(" to 31 Dec");
					};
					
					if (dateRange == "") 
					{ 
						if (atLeastOne) 
						{ // The whole year is good.
							ObsRange = q_("Observable during the whole year.");
						}
						else
						{
							ObsRange = q_("Not observable at dark night.");
						};
					}
					else
					{
						ObsRange = QString("%1 %2").arg(q_("Best observed")).arg(dateRange);
					};

//					if (selName == "Mercury") {ObsRange = "Observable in many dates of the year";}; // Special case of Mercury.

				}; // Comes from show_Good_Nights==True"
			}; // Comes from the "else" of "culmAlt>=..." 
		};// Comes from  "souChanged || ..."
	}; // Comes from the "else" with "!isMoon"

// Print all results:

	int yLine = 7*fontSize+80;
	int xLine = 50;

	if (show_Today) 
	{
		paintresult.drawText(xLine, yLine,q_(" TODAY:"));
		paintresult.drawText(xLine+fontSize, yLine-fontSize, RS2);
		paintresult.drawText(xLine+fontSize, yLine-fontSize*2, RS1);
		paintresult.drawText(xLine+fontSize, yLine-fontSize*3, Cul);
		yLine -= fontSize*5;
	};
		
	if ((isMoon && show_FullMoon) || (isSun == false && isMoon == false && show_Year)) 
	{
		paintresult.drawText(xLine,yLine," THIS YEAR:");
		if (show_Best_Night || show_FullMoon)
		{
			yLine -= fontSize;
			paintresult.drawText(xLine+fontSize, yLine, bestNight);
		};
		if (show_Good_Nights) 
		{
			yLine -= fontSize;
			paintresult.drawText(xLine+fontSize, yLine, ObsRange);
		};
		if (show_AcroCos) 
		{
			yLine -= fontSize;
			paintresult.drawText(xLine+fontSize, yLine, AcroCos);
		};

	};

}

// END OF MAIN CODE
///////////////////////////////////////////////////////


//////////////////////////////
// AUXILIARY FUNCTIONS

////////////////////////////////////
// Returns the hour angle for a given altitude:
double Observability::HourAngle(double lat, double h, double Dec)
{
	double Denom = std::cos(lat)*std::cos(Dec);
	double Numer = (std::sin(h)-std::sin(lat)*std::sin(Dec));

	if ( std::abs(Numer)>std::abs(Denom) ) 
		{return -0.5/86400.;} // Source doesn't reach that altitude. 
	else 
	{return Rad2Hr*std::acos(Numer/Denom);}

}
////////////////////////////////////


////////////////////////////////////
// Returns the angular separation between two points on the Sky:
// RA is given in hours and Dec in radians.
double Observability::Lambda(double RA1, double Dec1, double RA2, double Dec2)
{
	return std::acos(std::sin(Dec1)*std::sin(Dec2)+std::cos(Dec1)*std::cos(Dec2)*std::cos((RA1-RA2)/Rad2Hr));
}
////////////////////////////////////


////////////////////////////////////
// Returns the hour angle for a given a Sid. Time:
double Observability::HourAngle2(double RA, double ST)
{
	double Htemp = toUnsignedRA(RA-ST/15.);
	Htemp -= (Htemp>12.)?24.0:0.0;
	return Htemp;

}
////////////////////////////////////


////////////////////////////////////
// Converts a float time/angle span (in hours/degrees) in the (integer) format hh/dd,mm,ss:
void Observability::double2hms(double hfloat, int &h1, int &h2, int &h3)
{
	double f1,f2,f3;
	hfloat = std::abs(hfloat);
	double ffrac = std::modf(hfloat,&f1);
	double ffrac2 = std::modf(60.*ffrac,&f2);
	//FIXME: ffrac2 is unused variable; need fix
	ffrac2 = std::modf(3600.*(ffrac-f2/60.),&f3);
	h1 = (int)f1 ; h2 = (int)std::abs(f2) ; h3 = (int)std::abs(f3);
} 
////////////////////////////////////


////////////////////////////////////
// Adds/subtracts 24hr to ensure a RA between 0 and 24hr:
double Observability::toUnsignedRA(double RA)
{
	double tempRA,tempmod;
	//FIXME: tempmod is unused variable; need fix
	if (RA<0.0) {tempmod = std::modf(-RA/24.,&tempRA); RA += 24.*(tempRA+1.0);};
	double auxRA = 24.*std::modf(RA/24.,&tempRA);
	auxRA += (auxRA<0.0)?24.0:((auxRA>24.0)?-24.0:0.0);
	return auxRA;
}
////////////////////////////////////


///////////////////////////////////////////////
// Returns the day and month of year (to put it in format '25 Apr')
QString Observability::CalenDate(int selday)
{
	int day,month,year;
	StelUtils::getDateFromJulianDay(yearJD[selday],&year,&month,&day);
	return QString("%1 %2").arg(day).arg(months[month-1]);
}
//////////////////////////////////////////////


//////////////////////////////////////////////////
// Returns the RA and Dec of the selected planet 
//for each day of the current year:
void Observability::PlanetRADec(StelCore *core, QString Name)
{
	int gene = -1;
	double TempH;
	Vec3d TP, TP2;
	Mat4d LocTrans;

// If object is a Moon, we select its parent planet:
	PlanetP Object = GETSTELMODULE(SolarSystem)->searchByEnglishName(Name);
	PlanetP parent = Object->getParent();

	if (parent) {
		while (parent) {
			gene += 1;
			parent = parent->getParent();}
		};
	for (int g=0; g<gene;g++) {
		Object = Object->getParent();
	};
		
// Get a pointer to the planet's instance:
	Planet* myPlanet = Object.data();

// Compute planet's position for each day of the current year:

	for (int i=0;i<nDays;i++) {
		myPlanet->computePosition(yearJD[i]);
		myPlanet->computeTransMatrix(yearJD[i]);
		TP = myPlanet->getHeliocentricEclipticPos();
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(EarthPos[i]));
		TP2 = core->j2000ToEquinoxEqu(LocTrans*TP);
		toRADec(TP2,ObjectRA[i],ObjectDec[i]);
		TempH = HourAngle(mylat,0.0,ObjectDec[i]);
		ObjectH0[i] = TempH;
		ObjectSidT[0][i] = toUnsignedRA(ObjectRA[i]-TempH);
		ObjectSidT[1][i] = toUnsignedRA(ObjectRA[i]+TempH);
	}

// Return the planet to its current time:
	myPlanet->computePosition(myJD);
	myPlanet->computeTransMatrix(myJD);


}

/////////////////////////////////////////////////
// Computes the Sun's RA and Dec (and the JD) for 
// each day of the current year.
void Observability::SunRADec(StelCore* core) 
{
	int day,month,year,year2;
	Vec3d TP, TP2; 

// Get current date:
	StelUtils::getDateFromJulianDay(myJD,&year,&month,&day);

// Get JD for the Jan 1 of current year:
	StelUtils::getJDFromDate(&Jan1stJD,year,1,1,0,0,0);

// Check if we are on a leap year:
	StelUtils::getDateFromJulianDay(Jan1stJD+365.,&year2,&month,&day);
	nDays = (year==year2)?366:365;
	
// Compute Earth's position throughout the year:
	for (int i=0;i<nDays;i++) {
		yearJD[i] = Jan1stJD+(double)i;
		myEarth->computePosition(yearJD[i]);
		myEarth->computeTransMatrix(yearJD[i]);
		TP = myEarth->getHeliocentricEclipticPos();
		TP[0] = -TP[0]; TP[1] = -TP[1]; TP[2] = -TP[2];
		TP2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*TP);
		EarthPos[i] = TP;
		toRADec(TP2,SunRA[i],SunDec[i]);
	};

//Return the Earth to its current time:
	myEarth->computePosition(myJD);
	myEarth->computeTransMatrix(myJD);
}
///////////////////////////////////////////////////


////////////////////////////////////////////
// Computes Sun's Sidereal Times at twilight and culmination:
void Observability::SunHTwi()
{
	double TempH, TempH00;

	for (int i=0; i<nDays; i++) {
		TempH = HourAngle(mylat,AstroTwiAlti,SunDec[i]);
		TempH00 = HourAngle(mylat,0.0,SunDec[i]);
		if (TempH>0.0) {
			SunSidT[0][i] = toUnsignedRA(SunRA[i]-TempH*(1.00278));
			SunSidT[1][i] = toUnsignedRA(SunRA[i]+TempH*(1.00278));}
		else {
			SunSidT[0][i] = -1000.0;
			SunSidT[1][i] = -1000.0;};
		if (TempH00>0.0) {
			SunSidT[2][i] = toUnsignedRA(SunRA[i]+TempH00);
			SunSidT[3][i] = toUnsignedRA(SunRA[i]-TempH00);}
		else {
			SunSidT[2][i] = -1000.0;
			SunSidT[3][i] = -1000.0;};


	};
}
////////////////////////////////////////////


///////////////////////////////////////////
// Checks if a source can be observed with the Sun below the twilight altitude.
bool Observability::CheckRise(int i)
{

	// If Sun can't reach twilight elevation, return false:
	if (SunSidT[0][i]<0.0 || SunSidT[1][i]<0.0) { return false;};

	// Iterate over the whole year:
	int nBin = 1000;
	double auxSid1 = SunSidT[0][i];
	auxSid1 += (SunSidT[0][i] < SunSidT[1][i])?24.0:0.0;
	double deltaT = (auxSid1-SunSidT[1][i])/((double)nBin);

	double Hour; 
	for (int j=0;j<nBin;j++) {
		Hour = toUnsignedRA(SunSidT[1][i]+deltaT*(double)j - ObjectRA[i]);
		Hour -= (Hour>12.)?24.0:0.0;
		if (std::abs(Hour)<ObjectH0[i] || (ObjectH0[i] < 0.0 && alti>0.0)) {return true;};
	};	


	return false;
}
///////////////////////////////////////////


///////////////////////////////////////////
// Finds the dates of Acronichal (Rise, Set) and Cosmical (Rise2, Set2) dates.
// Returns 0 if no dates found, 1 if Acro exists, 2 if Cosm exists, and 3 if both are found.
int Observability::CheckAcro(int &Rise, int &Set, int &Rise2, int &Set2)
{

	Rise = -1;
	Set = -1;
	Rise2 = -1;
	Set2 = -1;

	double BestDiffRise = 12.0;
	double BestDiffSet = 12.0;
	double BestDiffRise2 = 12.0;
	double BestDiffSet2 = 12.0;

	double HourDiffRise, HourDiffSet, HourDiffRise2, HourDiffSet2;
	bool success = false;

 	for (int i=0;i<366;i++)
	{
		if (ObjectH0[i]>0.0 && SunSidT[2][i]>0.0 && SunSidT[3][i]>0.0) {
			success = true;
			HourDiffRise = toUnsignedRA(ObjectRA[i] - ObjectH0[i]);
			HourDiffRise2 = HourDiffRise-SunSidT[3][i];
			HourDiffRise -= SunSidT[2][i];

			HourDiffSet = toUnsignedRA(ObjectRA[i] + ObjectH0[i]);
			HourDiffSet2 = HourDiffSet - SunSidT[2][i];
			HourDiffSet -= SunSidT[3][i];

		// Acronychal Rise/Set:
			if (std::abs(HourDiffRise)<BestDiffRise) 
			{
				BestDiffRise = std::abs(HourDiffRise);
				Rise = i;
			};
			if (std::abs(HourDiffSet)<BestDiffSet) 
			{
				BestDiffSet = std::abs(HourDiffSet);
				Set = i;
			};

		// Cosmical Rise/Set:
			if (std::abs(HourDiffRise2)<BestDiffRise2) 
			{
				BestDiffRise2 = std::abs(HourDiffRise2);
				Rise2 = i;
			};
			if (std::abs(HourDiffSet2)<BestDiffSet2) 
			{
				BestDiffSet2 = std::abs(HourDiffSet2);
				Set2 = i;
			};


		};
	};

	Rise *= (BestDiffRise > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	Set *= (BestDiffSet > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	Rise2 *= (BestDiffRise2 > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	Set2 *= (BestDiffSet2 > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
//	qDebug() << q_("%1  %2").arg(Rise).arg(Rise2);
	int Result = (Rise>0 || Set>0)?1:0;
	Result += (Rise2>0 || Set2>0)?2:0;
	return (success)?Result:0;
}
///////////////////////////////////////////


////////////////////////////////////////////
// Convert an Equatorial Vec3d into RA and Dec:
void Observability::toRADec(Vec3d TempLoc, double &RA, double &Dec)
{
	TempLoc.normalize();
	Dec = std::asin(TempLoc[2]); // in radians
	RA = toUnsignedRA(std::atan2(TempLoc[1],TempLoc[0])*Rad2Hr); // in hours.
}
////////////////////////////////////////////



///////////////////////////
// Just return the sign of a double
double Observability::sign(double d)
{
	return (d<0.0)?-1.0:1.0;
}
//////////////////////////



//////////////////////////
// Get the coordinates of Sun or Moon for a given JD:
// getBack controls whether Earth and Moon must be returned to their original positions after computation.
void Observability::getSunMoonCoords(StelCore *core, double JD, double &RASun, double &DecSun, double &RAMoon, double &DecMoon, bool getBack) //, Vec3d &AltAzVector)
{

	if (getBack) // Return the Moon and Earth to their current position:
	{
		myEarth->computePosition(myJD);
		myEarth->computeTransMatrix(myJD);
		myMoon->computePosition(myJD);
		myMoon->computeTransMatrix(myJD);
	} 
	else
	{	// Compute coordinates:
		myEarth->computePosition(JD);
		myEarth->computeTransMatrix(JD);
		Pos0 = myEarth->getHeliocentricEclipticPos();
		double currSidT;

// Sun coordinates:
		Pos2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-Pos0));
		toRADec(Pos2,RASun,DecSun);

// Moon coordinates:
		currSidT = myEarth->getSiderealTime(JD)/Rad2Deg;
		RotObserver = (Mat4d::zrotation(currSidT))*ObserverLoc;
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-Pos0));
		myMoon->computePosition(JD);
		myMoon->computeTransMatrix(JD);
		Pos1 = myMoon->getHeliocentricEclipticPos();
		Pos2 = (core->j2000ToEquinoxEqu(LocTrans*Pos1))-RotObserver;

		toRADec(Pos2,RAMoon,DecMoon);
	};
}
//////////////////////////////////////////////



//////////////////////////////////////////////
// Solves Moon's or Sun's ephemeris by bissection. Returns JD:
bool Observability::MoonSunSolve(StelCore* core)
{

	int Niter = 100;
	int i;
	double Hhoriz, RA, Dec, RAS, DecS, TempH, jd1, tempEphH, currSidT;
	Vec3d Observer;

	Hhoriz = HourAngle(mylat,0.0,selDec);
	bool raises = Hhoriz > 0.0;


// Only recompute ephemeris from second to second (at least)
// or if the source has changed (i.e., Sun <-> Moon). This saves resources:
	if (std::abs(myJD-lastJDMoon)<JDsec && LastSun==isSun) return raises;

	LastSun = isSun;
	myEarth->computePosition(myJD);
	myEarth->computeTransMatrix(myJD);
	Pos0 = myEarth->getHeliocentricEclipticPos();

	if (isSun)
	{
		Pos2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-Pos0));}
	else
	{ 
		currSidT = myEarth->getSiderealTime(myJD)/Rad2Deg;
		RotObserver = (Mat4d::zrotation(currSidT))*ObserverLoc;
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-Pos0));
		myMoon->computePosition(myJD);
		myMoon->computeTransMatrix(myJD);
		Pos1 = myMoon->getHeliocentricEclipticPos();
		Pos2 = (core->j2000ToEquinoxEqu(LocTrans*Pos1))-RotObserver;
	};

	toRADec(Pos2,RA,Dec);
	Vec3d MoonAltAz = core->equinoxEquToAltAz(Pos2);
	raised = MoonAltAz[2] > 0.0;

// Initial guesses of rise/set/transit times.
// They are called 'Moon', but are also used for the Sun:

	double Hcurr = -HourAngle(mylat,alti,selDec)*sign(LocPos[1]);
	double SidT = toUnsignedRA(selRA + Hcurr);

	MoonCulm = -Hcurr; 
	MoonRise = (-Hhoriz-Hcurr);
	MoonSet = (Hhoriz-Hcurr);

	if (raises) {
		if (raised==false) {
			MoonRise += (MoonRise<0.0)?24.0:0.0;
			MoonSet -= (MoonSet>0.0)?24.0:0.0;
		};

// Rise time:
		tempEphH = MoonRise*TFrac;
		MoonRise = myJD + (MoonRise/24.);
		for (i=0; i<Niter; i++)
		{
	// Get modified coordinates:
			jd1 = MoonRise;
			getSunMoonCoords(core,jd1,RAS,DecS,RA,Dec,false);
			if (isSun) {RA = RAS; Dec = DecS;};

	// Current hour angle at mod. coordinates:
			Hcurr = toUnsignedRA(SidT-RA);
			Hcurr -= (raised)?0.0:24.;
			Hcurr -= (Hcurr>12.)?24.0:0.0;

	// H at horizon for mod. coordinates:
			Hhoriz = HourAngle(mylat,0.0,Dec);
	// Compute eph. times for mod. coordinates:
			TempH = (-Hhoriz-Hcurr)*TFrac;
			if (raised==false) TempH += (TempH<0.0)?24.0:0.0;
		// Check convergence:
			if (std::abs(TempH-tempEphH)<JDsec) break;
		// Update rise-time estimate:
			tempEphH = TempH;
			MoonRise = myJD + (tempEphH/24.);
		};

// Set time:  
		tempEphH = MoonSet;
		MoonSet = myJD + (MoonSet/24.);
		for (i=0; i<Niter; i++)
		{
	// Get modified coordinates:
			jd1 = MoonSet;
			getSunMoonCoords(core,jd1,RAS,DecS,RA,Dec,false);
			if (isSun) {RA = RAS; Dec = DecS;};

	// Current hour angle at mod. coordinates:
			Hcurr = toUnsignedRA(SidT-RA);
			Hcurr -= (raised)?24.:0.;
			Hcurr += (Hcurr<-12.)?24.0:0.0;
	// H at horizon for mod. coordinates:
			Hhoriz = HourAngle(mylat,0.0,Dec);
	// Compute eph. times for mod. coordinates:
			TempH = (Hhoriz-Hcurr)*TFrac;
			if (raised==false) TempH -= (TempH>0.0)?24.0:0.0;
	// Check convergence:
			if (std::abs(TempH-tempEphH)<JDsec) break;
	// Update set-time estimate:
			tempEphH = TempH;
			MoonSet = myJD + (tempEphH/24.);
		};
	} 
	else // Comes from if(raises)
	{
		MoonSet = -1.0; MoonRise = -1.0;
	};

// Culmination time:
	tempEphH = MoonCulm;
	MoonCulm = myJD + (MoonCulm/24.);

	for (i=0; i<Niter; i++)
	{
	// Get modified coordinates:
		jd1 = MoonCulm;
		getSunMoonCoords(core,jd1,RAS,DecS,RA,Dec,false);
		if (isSun) {RA = RAS; Dec = DecS;};


	// Current hour angle at mod. coordinates:
		Hcurr = toUnsignedRA(SidT-RA);
		Hcurr += (LocPos[1]<0.0)?24.0:-24.0;
		Hcurr -= (Hcurr>12.)?24.0:0.0;

	// Compute eph. times for mod. coordinates:
		TempH = -Hcurr*TFrac;
	// Check convergence:
		if (std::abs(TempH-tempEphH)<JDsec) break;
		tempEphH = TempH;
		MoonCulm = myJD + (tempEphH/24.);
		culmAlt = std::abs(mylat-Dec); // 90 - altitude at transit. 
	};


// Find out the days of Full Moon and Crescent at Sunset:
	if (isMoon && show_FullMoon) // || show_SuperMoon))
	{

	// Only extimate date of Full Moon if we have changed Lunar month:
		if (myJD > nextFullMoon || myJD < prevFullMoon)
		{


	// Estimate the closest Full Moon:
			double nT;
			double dT = std::modf((myJD-RefFullMoon)/MoonT,&nT);
			if (dT>0.5) {nT += 1.0;};
			double TempFullMoon = RefFullMoon + nT*MoonT;

	// Improve the estimate iteratively (Secant method over Lunar-phase vs. time):

			dT = 1./1440.; // Our time span for the finite-difference derivative estimate.
			//double Phase1, Phase2; // Variables for temporal use.
			double Deriv1, Deriv2; // Variables for temporal use.
			double Sec1, Sec2, Temp1, Temp2; // Variables for temporal use.

			for (int j=0; j<2; j++) 
			{ // Two steps: one for the previos Full Moon and the other for the next one.

				Sec1 = TempFullMoon - 0.01*MoonT; 
				Sec2 = TempFullMoon + 0.01*MoonT; 

				for (int i=0; i<100; i++) // A limit of 100 iterations.
				{
					getSunMoonCoords(core,Sec1+dT/2.,RAS,DecS,RA,Dec,false);
					Temp1 = Lambda(RA,Dec,RAS,DecS);
					getSunMoonCoords(core,Sec1-dT/2.,RAS,DecS,RA,Dec,false);
					Temp2 = Lambda(RA,Dec,RAS,DecS);

					Deriv1 = (Temp1-Temp2)/dT;
					//Phase1 = (Temp1+Temp2)/2.;

					getSunMoonCoords(core,Sec2+dT/2.,RAS,DecS,RA,Dec,false);
					Temp1 = Lambda(RA,Dec,RAS,DecS);
					getSunMoonCoords(core,Sec2-dT/2.,RAS,DecS,RA,Dec,false);
					Temp2 = Lambda(RA,Dec,RAS,DecS);

					Deriv2 = (Temp1-Temp2)/dT;
					//Phase2 = (Temp1+Temp2)/2.;

					Temp1 = Sec2 - Deriv2*(Sec2-Sec1)/(Deriv2-Deriv1);
					Sec1 = Sec2; Sec2 = Temp1;

					if (std::abs(Sec2-Sec1) < dT) {break;}  // Convergence.

				};

				if (TempFullMoon > myJD) 
				{
					nextFullMoon = TempFullMoon;
					TempFullMoon -= MoonT;
				} else
				{
					prevFullMoon = TempFullMoon;
					TempFullMoon += MoonT;
				};

			};

		};	

	// Update the string shown in the screen: 
		int fullDay, fullMonth,fullYear, fullHour, fullMinute, fullSecond;
		StelUtils::getDateFromJulianDay(prevFullMoon,&fullYear,&fullMonth,&fullDay);
		double dT;
		double nT = 24.*(std::modf(prevFullMoon,&dT));
		double2hms(nT,fullHour,fullMinute,fullSecond);
		bestNight = q_("Previous Full Moon: %1 %2 at %3:%4. ").arg(months[fullMonth-1]).arg(fullDay).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));
		StelUtils::getDateFromJulianDay(nextFullMoon,&fullYear,&fullMonth,&fullDay);
		nT = 24.*(std::modf(nextFullMoon,&dT));
		double2hms(nT,fullHour,fullMinute,fullSecond);
		bestNight += q_("Next Full Moon: %1 %2 at %3:%4. ").arg(months[fullMonth-1]).arg(fullDay).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));


	// Now, compute the days of all the Full Moons of the current year, and get the Earth/Moon distance:
//		double monthFrac;
//		int PrevMonths = (int) std::modf((nextFullMoon-Jan1stJD)/MoonT,&monthFrac); 
//		double BestDistance = 1.0; // initial dummy value for Sun-Moon distance;

//		for (int i=-PrevMonths; i<13 ; i++)
//		{
//			jd1 = nextFullMoon + MoonT*((double) i);
//			getSunMoonCoords(core,jd1,RAS,DecS,RA,Dec,false); 
//			COMPUTE THE DISTANCE VECTOR!
//			if (Dist < BestDistance)
//			{
//				BestDistance = Dist;
//				nextSuperMoon = jd1;
//			};
//		};
	} 
	else
	{
		bestNight = "";
	}; 

	ObsRange = ""; 
	AcroCos = "";


// Return the Moon and Earth to its current position:
	getSunMoonCoords(core,myJD,RAS,DecS,RA,Dec,true);
	lastJDMoon = myJD;

	return raises;
}




//////////////////////////////////
///  STUFF FOR THE GUI CONFIG

bool Observability::configureGui(bool show)
{
	if (show)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->getGuiAction("actionShow_Observability_ConfigDialog")->setChecked(true);
	}

	return true;
}

void Observability::restoreDefaults(void)
{
	restoreDefaultConfigIni();
	readSettingsFromConfig();
}

void Observability::restoreDefaultConfigIni(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	// delete all existing settings...
	conf->remove("");

	// Set defaults
	conf->setValue("Observability/font_size", 15);
	conf->setValue("Observability/Sun_Altitude", 12);
	conf->setValue("Observability/font_color", "0,0.5,1");
	conf->setValue("Observability/show_AcroCos", true);
	conf->setValue("Observability/show_Good_Nights", true);
	conf->setValue("Observability/show_Best_Night", true);
	conf->setValue("Observability/show_Today", true);
	conf->setValue("Observability/show_FullMoon", true);
//	conf->setValue("Observability/show_Crescent", true);
//	conf->setValue("Observability/show_SuperMoon", true);
}

void Observability::readSettingsFromConfig(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	// Load settings from main config file
	fontSize = conf->value("Observability/font_size",15).toInt();
	font.setPixelSize(fontSize);
	fontColor = StelUtils::strToVec3f(conf->value("Observability/font_color", "0,0.5,1").toString());
	show_AcroCos = conf->value("Observability/show_AcroCos", true).toBool();
	show_Good_Nights = conf->value("Observability/show_Good_Nights", true).toBool();
	show_Best_Night = conf->value("Observability/show_Best_Night", true).toBool();
	show_Today = conf->value("Observability/show_Today", true).toBool();
	show_FullMoon = conf->value("Observability/show_FullMoon", true).toBool();
//	show_Crescent = conf->value("Observability/show_Crescent", true).toBool();
//	show_SuperMoon = conf->value("Observability/show_SuperMoon", true).toBool();

	iAltitude = conf->value("Observability/Sun_Altitude", 12).toInt();
	AstroTwiAlti  = -((double)iAltitude)/Rad2Deg ;
	

}

void Observability::saveSettingsToConfig(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	QString fontColorStr = QString("%1,%2,%3").arg(fontColor[0],0,'f',2).arg(fontColor[1],0,'f',2).arg(fontColor[2],0,'f',2);
	// Set updated values
	conf->setValue("Observability/font_size", fontSize);
	conf->setValue("Observability/Sun_Altitude", iAltitude);
	conf->setValue("Observability/font_color", fontColorStr);
	conf->setValue("Observability/show_AcroCos", show_AcroCos);
	conf->setValue("Observability/show_Good_Nights", show_Good_Nights);
	conf->setValue("Observability/show_Best_Night", show_Best_Night);
	conf->setValue("Observability/show_Today", show_Today);
	conf->setValue("Observability/show_FullMoon", show_FullMoon);
//	conf->setValue("Observability/show_Crescent", show_Crescent);
//	conf->setValue("Observability/show_SuperMoon", show_SuperMoon);
}



void Observability::setShow(int output, bool setVal)
{
	switch(output)
	{
		case 1: {show_Today = setVal; break;}
		case 2: {show_AcroCos = setVal; break;}
		case 3: {show_Good_Nights = setVal; break;}
		case 4: {show_Best_Night = setVal; break;}
		case 5: {show_FullMoon = setVal; nextFullMoon=0.0; prevFullMoon=0.0; break;}
//		case 6: {show_Crescent = setVal; break;}
//		case 7: {show_SuperMoon = setVal; break;}
	};
	configChanged = true;
}

bool Observability::getShowFlags(int iFlag)
{
	switch (iFlag)
	{
		case 1: return show_Today;
		case 2: return show_AcroCos;
		case 3: return show_Good_Nights;
		case 4: return show_Best_Night;
		case 5: return show_FullMoon;
//		case 6: return show_Crescent;
//		case 7: return show_SuperMoon;
	};

	return false;
}

Vec3f Observability::getFontColor(void)
{
	return fontColor;
}

int Observability::getFontSize(void)
{
	return fontSize;
}

int Observability::getSunAltitude(void)
{
	return iAltitude;
}

void Observability::setFontColor(int color, int value)
{
	float fValue = (float)(value) / 100.; 
	fontColor[color] = fValue;
}

void Observability::setFontSize(int value)
{
	fontSize = value;
}

void Observability::setSunAltitude(int value)
{
	AstroTwiAlti  = -((double) value)/Rad2Deg ;
	iAltitude = value;
	configChanged = true;
}


///  END OF STUFF FOR THE GUI CONFIG.
///////////////////////////////




// Enable the Observability:
void Observability::enableObservability(bool b)
{
	flagShowObservability = b;
}

