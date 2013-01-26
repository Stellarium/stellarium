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
#include <QString>
#include <QDebug>
#include <QAction>
#include <QKeyEvent>
#include <QtNetwork>
#include <QKeyEvent>
#include <QMouseEvent>

#include "renderer/StelRenderer.hpp"
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
#include "StelCore.hpp"
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
	info.authors = "Ivan Marti-Vidal (Onsala Space Observatory)"; // non-translatable field
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
	MoonPerilune = 0.0024236308; // Smallest Earth-Moon distance (in AU). 
	nextFullMoon = 0.0;
	prevFullMoon = 0.0;
	RefracHoriz = 0.0;   // Geometric altitude at refraction-corrected horizon. 
	selName = "";



////////////////////////////
// Read configuration:

	QSettings* conf = StelApp::getInstance().getSettings();
	// Setup defaults if not present
	conf->beginGroup("Observability");
	if (!conf->contains("font_size"))
		conf->setValue("font_size", 15);

	if (!conf->contains("font_color"))
		conf->setValue("font_color", "0,0.5,1");

	if (!conf->contains("show_AcroCos"))
		conf->setValue("show_AcroCos", true);

	if (!conf->contains("show_Good_Nights"))
		conf->setValue("show_Good_Nights", true);

	if (!conf->contains("show_Best_Night"))
		conf->setValue("show_Best_Night", true);

	if (!conf->contains("show_Today"))
		conf->setValue("show_Today", true);

	if (!conf->contains("Sun_Altitude"))
		conf->setValue("Sun_Altitude", 12);

	if (!conf->contains("Horizon_Altitude"))
		conf->setValue("Horizon_Altitude", 0);

	if (!conf->contains("show_FullMoon"))
		conf->setValue("show_FullMoon", true);

//	if (!conf->contains("show_Crescent"))
//		conf->setValue("show_Crescent", true);

//	if (!conf->contains("show_SuperMoon"))
//		conf->setValue("show_SuperMoon", true);


	// Load settings from main config file
	fontSize = conf->value("font_size",15).toInt();
	iAltitude = conf->value("Sun_Altitude",12).toInt();
	iHorizAltitude = conf->value("Horizon_Altitude",0).toInt();
	AstroTwiAlti = -((double) iAltitude)/Rad2Deg ;
	HorizAlti = ((double) iHorizAltitude)/Rad2Deg ;
	font.setPixelSize(fontSize);
	QString fontColorStr = conf->value("font_color", "0,0.5,1").toString();
	fontColor = StelUtils::strToVec3f(fontColorStr);
	show_AcroCos = conf->value("show_AcroCos", true).toBool();
	show_Good_Nights = conf->value("show_Good_Nights", true).toBool();
	show_Best_Night = conf->value("show_Best_Night", true).toBool();
	show_Today = conf->value("show_Today", true).toBool();
	show_FullMoon = conf->value("show_FullMoon", true).toBool();
//	show_Crescent = conf->value("show_Crescent", true).toBool();
//	show_SuperMoon = conf->value("show_SuperMoon", true).toBool();

	conf->endGroup();
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
	QString mons[12]={qc_("Jan", "short month name"), qc_("Feb", "short month name"), qc_("Mar", "short month name"), qc_("Apr", "short month name"), qc_("May", "short month name"), qc_("Jun", "short month name"), qc_("Jul", "short month name"), qc_("Aug", "short month name"), qc_("Sep", "short month name"), qc_("Oct", "short month name"), qc_("Nov", "short month name"), qc_("Dec", "short month name")};

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
void Observability::draw(StelCore* core, StelRenderer* renderer)
{

	if (!flagShowObservability) return; // Button is off.

/////////////////////////////////////////////////////////////////
// PRELIMINARS:
	bool locChanged, yearChanged;
	StelObjectP selectedObject;
	Planet* currPlanet;
	PlanetP Object, parent;

// Only execute plugin if we are on Earth.
	if (core->getCurrentLocation().planetName != "Earth") {return;};

// Set the painter:
	StelProjectorP projector = core->getProjection2d();
	renderer->setGlobalColor(fontColor[0],fontColor[1],fontColor[2],1);
	font.setPixelSize(fontSize);
	renderer->setFont(font);


// Get current date, location, and check if there is something selected.
	double currlat = (core->getCurrentLocation().latitude)/Rad2Deg;
	double currlon = (core->getCurrentLocation().longitude)/Rad2Deg;
	double currheight = (6371.+(core->getCurrentLocation().altitude)/1000.)/UA;
	double currJD = core->getJDay();
	double currJDint;
//	GMTShift = StelUtils::getGMTShiftFromQT(currJD)/24.0;
	GMTShift = StelApp::getInstance().getLocaleMgr().getGMTShift(currJD)/24.0;

//	qDebug() << QString("%1%2 ").arg(GMTShift);

	double currLocalT = 24.*modf(currJD + GMTShift,&currJDint);

	int auxm, auxd, auxy;
	StelUtils::getDateFromJulianDay(currJD,&auxy,&auxm,&auxd);
	bool isSource = StelApp::getInstance().getStelObjectMgr().getWasSelected();
	bool show_Year = show_Best_Night || show_Good_Nights || show_AcroCos; 

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



// Add refraction, if necessary:
	Vec3d TempRefr;	
	TempRefr[0] = std::cos(HorizAlti);  
	TempRefr[1] = 0.0; 
	TempRefr[2] = std::sin(HorizAlti);  
	Vec3d CorrRefr = core->altAzToEquinoxEqu(TempRefr,StelCore::RefractionAuto);
	TempRefr = core->equinoxEquToAltAz(CorrRefr,StelCore::RefractionOff);
	double RefracAlt = std::asin(TempRefr[2]);

	if (std::abs(RefracHoriz-RefracAlt)>2.91e-4)  // Diference larger than 1 arcminute.
	{   // Configuration for refraction changed notably:
		RefracHoriz = RefracAlt;
		configChanged = true;
		souChanged = true;
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

// If Moon is not selected (or was unselected), force re-compute of Full Moon next time it is selected:
		if (!isMoon) {prevFullMoon=0.0; nextFullMoon=0.0;};

//Update position:
		EquPos = selectedObject->getEquinoxEquatorialPos(core);
		EquPos.normalize();
		LocPos = core->equinoxEquToAltAz(EquPos,StelCore::RefractionOff);

// Check if the user has changed the source (or if the source is Sun/Moon). 
		if (tempName == selName) 
		{
			souChanged = false;}
		else 
		{ // Check also if the (new) source belongs to the Solar System:

			souChanged = true;
			selName = tempName;

			currPlanet = dynamic_cast<Planet*>(selectedObject.data());
			isStar = (currPlanet)?false:true;

			if (!isStar && !isMoon && !isSun)  // Object in the Solar System, but is not Sun nor Moon.
			{ 

				int gene = -1;

			// If object is a planet's moon, we get its parent planet:
				Object = GETSTELMODULE(SolarSystem)->searchByEnglishName(selName);

				parent = Object->getParent();

				if (parent) 
				{
					while (parent) {
					gene += 1;
					parent = parent->getParent();}
					};
					for (int g=0; g<gene;g++) {
					Object = Object->getParent();
				};
		
			// Now get a pointer to the planet's instance:
				myPlanet = Object.data();
			};

		};
	}
	else { // There is no source selected!

// If no source is selected, get the position vector of the screen center:
		selName = ""; isStar=true; isMoon = false; isSun = false; isScreen=true;
		Vec3d currentPos = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
		currentPos.normalize();
		EquPos = core->j2000ToEquinoxEqu(currentPos);
		LocPos = core->j2000ToAltAz(currentPos,StelCore::RefractionOff);
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
	horizH = HourAngle(mylat,RefracHoriz,selDec);
	QString RS1, RS2, Cul; // strings with Rise/Set/Culmination times
	double Rise, Set; // Actual Rise/Set times (in GMT).
	int d1,m1,s1,d2,m2,s2,dc,mc,sc; // Integers for the time spans in hh:mm:ss.
	bool solvedMoon = false; // Check if solutions were found for Sun, Moon, or planet.
	bool transit = false; // Is the source above the horizon? Did it culminate?

	int ephHour, ephMinute, ephSecond;  // Local time for selected ephemeris

	if (show_Today) {  // We show ephemeris for today (i.e., rise, set, and transit times).


		if (!isStar) 
		{
			int Kind = (isSun)?1:0;  // Set "Kind" according to the selected object.
			Kind += (isMoon)?2:0; Kind += (!isSun && !isMoon)?3:0;

			solvedMoon = SolarSystemSolve(core, Kind);  // False if fails; True otherwise.
			currH = std::abs(24.*(MoonCulm-myJD)/TFrac);
			transit = MoonCulm-myJD<0.0;
			if (solvedMoon) {  // If failed, Set and Rise will be dummy.
				Set = std::abs(24.*(MoonSet-myJD)/TFrac);
				Rise = std::abs(24.*(MoonRise-myJD)/TFrac);
			};
		} else if (horizH>0.0) { // The source is not circumpolar and can be seen from this latitude.

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

		if ((solvedMoon && MoonRise>0.0) || (!isSun && !isMoon && horizH>0.0))
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
				RS2 = q_("Rose at %1 (%2 ago)").arg(RiseTime).arg(RS2);
			}
			else 
			{
                                double2hms(toUnsignedRA(currLocalT-TFrac*Set+12.),ephHour,ephMinute,ephSecond);
				SetTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));

                                double2hms(toUnsignedRA(currLocalT+TFrac*Rise+12.),ephHour,ephMinute,ephSecond);
				RiseTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));

				RS1 = q_("Set at %1 (%2 ago)").arg(SetTime).arg(RS1);
				RS2 = q_("Rises at %1 (in %2)").arg(RiseTime).arg(RS2);
			};				
		}
		else { // The source is either circumpolar or never rises:
			(alti>RefracHoriz)? RS1 = q_("Circumpolar."): RS1 = q_("No rise.");
			RS2 = "";
		};

// 	Culmination:

	if (isStar)
	{
		culmAlt = std::abs(mylat-selDec); // 90.-altitude at transit.
		transit = LocPos[1]<0.0;
	};

	if (culmAlt<halfpi-RefracHoriz) { // Source can be observed.
		double altiAtCulmi = Rad2Deg*(halfpi-culmAlt-RefracHoriz);
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


	if (isSun) 
	{
		bestNight=""; ObsRange = "";
	}
	else if (!isMoon && show_Year) {

		if (isStar==false && (souChanged || yearChanged)) { // Object moves.
			PlanetRADec(core);} // Re-compute ephemeris.

		else { // Object is fixed on the sky.
			double auxH = HourAngle(mylat,RefracHoriz,selDec);
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

			if (culmAlt>=halfpi-RefracHoriz) { // Source cannot be seen.
				ObsRange = q_("Source is not observable.");
				AcroCos = q_("No Acronychal nor Cosmical rise/set.");
			}
			else { // Source can be seen.

///////////////////////////
// - Part 1. Determine the best observing night (i.e., opposition to the Sun):
				if (show_Best_Night) {
					int selday = 0;
					double deltaPhs = -1.0; // Initial dummy value
					double tempPhs; 	
					for (int i=0;i<nDays;i++) { // Maximize the Sun-object separation.
						tempPhs = Lambda(ObjectRA[i],ObjectDec[i],SunRA[i],SunDec[i]);
						if (tempPhs>deltaPhs) {selday=i;deltaPhs=tempPhs;};
					};

					if (selName=="Mercury" || selName=="Venus")
					{
						bestNight = q_("Greatest elongation: ");
					} else 
					{
						bestNight = q_("Largest Sun separation: ");
					};

					bestNight = bestNight + CalenDate(selday) + q_(" (at %1 deg.)").arg(deltaPhs*Rad2Deg,0,'f',1);
				};

///////////////////////////////
// - Part 2. Determine Acronychal and Cosmical rise and set:

				if (show_AcroCos) {
					int selRise, selSet, selRise2, selSet2; // days of year for Acronical and Cosmical rise/set.
					int Acro = CheckAcro(selRise,selSet,selRise2,selSet2);
					QString AcroRiseStr, AcroSetStr;
					QString CosmRiseStr, CosmSetStr;

					AcroRiseStr = (selRise>0)?CalenDate(selRise):q_("None");
					AcroSetStr = (selSet>0)?CalenDate(selSet):q_("None");

					CosmRiseStr = (selRise2>0)?CalenDate(selRise2):q_("None");
					CosmSetStr = (selSet2>0)?CalenDate(selSet2):q_("None");

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
								dateRange += QString("%1").arg(RangeCalenDates(selday, selday2));
							};
						};
					};

					if (bestBegun) { // There were good dates till the end of the year.
						 if (dateRange!="") { dateRange += ", ";};
						dateRange += RangeCalenDates(selday, 0);
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
						ObsRange = QString("%1 %2").arg(q_("Nights above horizon: ")).arg(dateRange);
					};

				}; // Comes from show_Good_Nights==True"
			}; // Comes from the "else" of "culmAlt>=..." 
		};// Comes from  "souChanged || ..."
	}; // Comes from the "else" with "!isMoon"

// Print all results:

	int spacing = (int) (1.3* ( (double) fontSize));  // between lines
	int spacing2 = 6*fontSize;  // between daily and yearly results
	int yLine = 8*fontSize+110;
	int xLine = 80;

	if (show_Today) 
	{
		renderer->drawText(TextParams(xLine, yLine,q_("TODAY:")));
		renderer->drawText(TextParams(xLine+fontSize, yLine-spacing, RS2));
		renderer->drawText(TextParams(xLine+fontSize, yLine-spacing*2, RS1));
		renderer->drawText(TextParams(xLine+fontSize, yLine-spacing*3, Cul));
		yLine -= spacing2;
	};
	
	if ((isMoon && show_FullMoon) || (!isSun && !isMoon && show_Year)) 
	{
		renderer->drawText(TextParams(xLine,yLine,q_("THIS YEAR:")));
		if (show_Best_Night || show_FullMoon)
		{
			yLine -= spacing;
			renderer->drawText(TextParams(xLine+fontSize, yLine, bestNight));
		};
		if (show_Good_Nights) 
		{
			yLine -= spacing;
			renderer->drawText(TextParams(xLine+fontSize, yLine, ObsRange));
		};
		if (show_AcroCos) 
		{
			yLine -= spacing;
			renderer->drawText(TextParams(xLine+fontSize, yLine, AcroCos));
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
	h1 = (int)f1 ; h2 = (int)std::abs(f2+0.0*ffrac2) ; h3 = (int)std::abs(f3);
} 
////////////////////////////////////


////////////////////////////////////
// Adds/subtracts 24hr to ensure a RA between 0 and 24hr:
double Observability::toUnsignedRA(double RA)
{
	double tempRA,tempmod;
	//FIXME: tempmod is unused variable; need fix
	if (RA<0.0) {tempmod = std::modf(-RA/24.,&tempRA); RA += 24.*(tempRA+1.0)+0.0*tempmod;};
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

///////////////////////////////////////////////
// Returns the day and month of year (to put it in format '25 Apr')
QString Observability::RangeCalenDates(int fDoY, int sDoY)
{
	int day1,month1,year1,day2,month2,year2;
	QString range;
	StelUtils::getDateFromJulianDay(yearJD[fDoY],&year1,&month1,&day1);
	StelUtils::getDateFromJulianDay(yearJD[sDoY],&year2,&month2,&day2);
	if (sDoY==0)
	{
		day2 = 31;
		month2 = 12;
	}
	if (fDoY==0)
	{
		day1 = 1;
		month1 = 1;
	}
	if (month1==month2)
	{
		range = QString("%1 - %2 %3").arg(day1).arg(day2).arg(months[month1-1]);
	}
	else
	{
		range = QString("%1 %2 - %3 %4").arg(day1).arg(months[month1-1]).arg(day2).arg(months[month2-1]);
	}

	return range;
}
//////////////////////////////////////////////


//////////////////////////////////////////////////
// Returns the RA and Dec of the selected planet 
//for each day of the current year:
void Observability::PlanetRADec(StelCore *core)
{
	double TempH;

// Compute planet's position for each day of the current year:

	for (int i=0;i<nDays;i++) {
		getPlanetCoords(core,yearJD[i],ObjectRA[i],ObjectDec[i],false);
		TempH = HourAngle(mylat,RefracHoriz,ObjectDec[i]);
		ObjectH0[i] = TempH;
		ObjectSidT[0][i] = toUnsignedRA(ObjectRA[i]-TempH);
		ObjectSidT[1][i] = toUnsignedRA(ObjectRA[i]+TempH);
	}

// Return the planet to its current time:
	getPlanetCoords(core,myJD,ObjectRA[0],ObjectDec[0],true);


}

/////////////////////////////////////////////////
// Computes the Sun's RA and Dec (and the JD) for 
// each day of the current year.
void Observability::SunRADec(StelCore* core) 
{
	int day,month,year,year2;

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
		Pos1 = myEarth->getHeliocentricEclipticPos();
		Pos2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-Pos1));
		EarthPos[i] = -Pos1;
		toRADec(Pos2,SunRA[i],SunDec[i]);
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
		TempH00 = HourAngle(mylat,RefracHoriz,SunDec[i]);
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
void Observability::getSunMoonCoords(StelCore *core, double JD, double &RASun, double &DecSun, double &RAMoon, double &DecMoon, double &EclLon, bool getBack) //, Vec3d &AltAzVector)
{

	if (getBack) // Return the Moon and Earth to their current position:
	{
		myEarth->computePosition(JD);
		myEarth->computeTransMatrix(JD);
		myMoon->computePosition(JD);
		myMoon->computeTransMatrix(JD);
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

                EclLon = Pos1[0]*Pos0[1] - Pos1[1]*Pos0[0];

		toRADec(Pos2,RAMoon,DecMoon);
	};
}
//////////////////////////////////////////////



//////////////////////////
// Get the Observer-to-Moon distance JD:
// getBack controls whether Earth and Moon must be returned to their original positions after computation.
void Observability::getMoonDistance(StelCore *core, double JD, double &Distance, bool getBack)
{

	if (getBack) // Return the Moon and Earth to their current position:
	{
		myEarth->computePosition(JD);
		myEarth->computeTransMatrix(JD);
		myMoon->computePosition(JD);
		myMoon->computeTransMatrix(JD);
	} 
	else
	{	// Compute coordinates:
		myEarth->computePosition(JD);
		myEarth->computeTransMatrix(JD);
		Pos0 = myEarth->getHeliocentricEclipticPos();
//		double currSidT;

// Sun coordinates:
//		Pos2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-Pos0));
//		toRADec(Pos2,RASun,DecSun);

// Moon coordinates:
//		currSidT = myEarth->getSiderealTime(JD)/Rad2Deg;
//		RotObserver = (Mat4d::zrotation(currSidT))*ObserverLoc;
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-Pos0));
		myMoon->computePosition(JD);
		myMoon->computeTransMatrix(JD);
		Pos1 = myMoon->getHeliocentricEclipticPos();
		Pos2 = (core->j2000ToEquinoxEqu(LocTrans*Pos1)); //-RotObserver;

		Distance = std::sqrt(Pos2*Pos2);

//		toRADec(Pos2,RAMoon,DecMoon);
	};
}
//////////////////////////////////////////////




//////////////////////////////////////////////
// Get the Coords of a planet:
void Observability::getPlanetCoords(StelCore *core, double JD, double &RA, double &Dec, bool getBack)
{

	if (getBack)
	{
	// Return the planet to its current time:
		myPlanet->computePosition(JD);
		myPlanet->computeTransMatrix(JD);
		myEarth->computePosition(JD);
		myEarth->computeTransMatrix(JD);
	} else
	{
	// Compute planet's position:
		myPlanet->computePosition(JD);
		myPlanet->computeTransMatrix(JD);
		Pos1 = myPlanet->getHeliocentricEclipticPos();
		myEarth->computePosition(JD);
		myEarth->computeTransMatrix(JD);
		Pos2 = myEarth->getHeliocentricEclipticPos();
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-Pos2));
		Pos2 = core->j2000ToEquinoxEqu(LocTrans*Pos1);
		toRADec(Pos2,RA,Dec);
	};

}
//////////////////////////////////////////////



//////////////////////////////////////////////
// Solves Moon's, Sun's, or Planet's ephemeris by bissection. Returns JD:
bool Observability::SolarSystemSolve(StelCore* core, int Kind)
{

	int Niter = 100;
	int i;
	double Hhoriz, RA, Dec, RAS, DecS, TempH, jd1, tempEphH, currSidT, EclLon;
	Vec3d Observer;

	Hhoriz = HourAngle(mylat,RefracHoriz,selDec);
	bool raises = Hhoriz > 0.0;


// Only recompute ephemeris from second to second (at least)
// or if the source has changed (i.e., Sun <-> Moon). This saves resources:
	if (std::abs(myJD-lastJDMoon)>JDsec || LastObject!=Kind || souChanged)
	{

//		qDebug() << q_("%1  %2   %3   %4").arg(Kind).arg(LastObject).arg(myJD,0,'f',5).arg(lastJDMoon,0,'f',5);

		LastObject = Kind;

		myEarth->computePosition(myJD);
		myEarth->computeTransMatrix(myJD);
		Pos0 = myEarth->getHeliocentricEclipticPos();

		if (Kind==1)
		{   // Sun position:
			Pos2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-Pos0));
		} else if (Kind==2)
		{   // Moon position:
			currSidT = myEarth->getSiderealTime(myJD)/Rad2Deg;
			RotObserver = (Mat4d::zrotation(currSidT))*ObserverLoc;
			LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-Pos0));
			myMoon->computePosition(myJD);
			myMoon->computeTransMatrix(myJD);
			Pos1 = myMoon->getHeliocentricEclipticPos();
			Pos2 = (core->j2000ToEquinoxEqu(LocTrans*Pos1))-RotObserver;
		} else 
		{   // Planet position:
       	        	myPlanet->computePosition(myJD);
       	        	myPlanet->computeTransMatrix(myJD);
       	        	Pos1 = myPlanet->getHeliocentricEclipticPos();
       		        LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(-Pos0));
	                Pos2 = core->j2000ToEquinoxEqu(LocTrans*Pos1);
		};

		toRADec(Pos2,RA,Dec);
		Vec3d MoonAltAz = core->equinoxEquToAltAz(Pos2,StelCore::RefractionOff);
		raised = MoonAltAz[2] > RefracHoriz;

// Initial guesses of rise/set/transit times.
// They are called 'Moon', but are also used for the Sun or planet:

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
	
				if (Kind<3)
				{
					getSunMoonCoords(core,jd1,RAS,DecS,RA,Dec,EclLon,false);
				} else
				{
					getPlanetCoords(core,jd1,RA,Dec,false);
				};

				if (Kind==1) {RA = RAS; Dec = DecS;};

	// Current hour angle at mod. coordinates:
				Hcurr = toUnsignedRA(SidT-RA);
				Hcurr -= (raised)?0.0:24.;
				Hcurr -= (Hcurr>12.)?24.0:0.0;

	// H at horizon for mod. coordinates:
				Hhoriz = HourAngle(mylat,RefracHoriz,Dec);
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

                	        if (Kind<3)
				{
                                	getSunMoonCoords(core,jd1,RAS,DecS,RA,Dec,EclLon,false);
                        	} else
                        	{
                                	getPlanetCoords(core,jd1,RA,Dec,false);
                        	};

				if (Kind==1) {RA = RAS; Dec = DecS;};

	// Current hour angle at mod. coordinates:
				Hcurr = toUnsignedRA(SidT-RA);
				Hcurr -= (raised)?24.:0.;
				Hcurr += (Hcurr<-12.)?24.0:0.0;
	// H at horizon for mod. coordinates:
				Hhoriz = HourAngle(mylat,RefracHoriz,Dec);
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

                	        if (Kind<3)
                       		{
                                	getSunMoonCoords(core,jd1,RAS,DecS,RA,Dec,EclLon,false);
                        	} else
                        	{
                                	getPlanetCoords(core,jd1,RA,Dec,false);
                        	};


			if (Kind==1) {RA = RAS; Dec = DecS;};


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

//		qDebug() << q_("%1").arg(MoonCulm,0,'f',5);


	lastJDMoon = myJD;

	}; // Comes from if (std::abs(myJD-lastJDMoon)>JDsec || LastObject!=Kind)




// Find out the days of Full Moon:
	if (Kind==2 && show_FullMoon) // || show_SuperMoon))
	{

	// Only estimate date of Full Moon if we have changed Lunar month:
		if (myJD > nextFullMoon || myJD < prevFullMoon)
		{


	// Estimate the nearest (in time) Full Moon:
			double nT;
			double dT = std::modf((myJD-RefFullMoon)/MoonT,&nT);
			if (dT>0.5) {nT += 1.0;};
			if (dT<-0.5) {nT -= 1.0;};

			double TempFullMoon = RefFullMoon + nT*MoonT;

	// Improve the estimate iteratively (Secant method over Lunar-phase vs. time):

			dT = 0.1/1440.; // 6 seconds. Our time span for the finite-difference derivative estimate.
//			double Deriv1, Deriv2; // Variables for temporal use.
			double Sec1, Sec2, Temp1, Temp2; // Variables for temporal use.
			double iniEst1, iniEst2;  // JD values that MUST include the solution within them.
			double Phase1;

			for (int j=0; j<2; j++) 
			{ // Two steps: one for the previos Full Moon and the other for the next one.

				iniEst1 =  TempFullMoon - 0.25*MoonT; 
				iniEst2 =  TempFullMoon + 0.25*MoonT; 

				Sec1 = iniEst1; // TempFullMoon - 0.05*MoonT; // Initial estimates of Full-Moon dates
				Sec2 = iniEst2; // TempFullMoon + 0.05*MoonT; 

				getSunMoonCoords(core,Sec1,RAS,DecS,RA,Dec,EclLon,false);
				Temp1 = EclLon; //Lambda(RA,Dec,RAS,DecS);
				getSunMoonCoords(core,Sec2,RAS,DecS,RA,Dec,EclLon,false);
				Temp2 = EclLon; //Lambda(RA,Dec,RAS,DecS);


				for (int i=0; i<100; i++) // A limit of 100 iterations.
				{
                                        Phase1 = (Sec2-Sec1)/(Temp1-Temp2)*Temp1+Sec1;
					getSunMoonCoords(core,Phase1,RAS,DecS,RA,Dec,EclLon,false);

                                        if (Temp1*EclLon < 0.0) 
					{
						Sec2 = Phase1;
						Temp2 = EclLon;
					} else {
						Sec1 = Phase1;
						Temp1 = EclLon;

					};

				//	qDebug() << QString("%1 %2 %3 %4 ").arg(Sec1).arg(Sec2).arg(Temp1).arg(Temp2);	


					if (std::abs(Sec2-Sec1) < 10.*dT)  // 1 minute accuracy; convergence.
					{
						TempFullMoon = (Sec1+Sec2)/2.;
				//		qDebug() << QString("%1%2 ").arg(TempFullMoon);	
						break;
					};
					
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


	// Update the string shown in the screen: 
			int fullDay, fullMonth,fullYear, fullHour, fullMinute, fullSecond;
			double LocalPrev = prevFullMoon+GMTShift+0.5;  // Shift to the local time. 
			double LocalNext = nextFullMoon+GMTShift+0.5;
			double intMoon;
			double LocalTMoon = 24.*modf(LocalPrev,&intMoon);
			StelUtils::getDateFromJulianDay(intMoon,&fullYear,&fullMonth,&fullDay);
			double2hms(toUnsignedRA(LocalTMoon),fullHour,fullMinute,fullSecond);
			bestNight = q_("Previous Full Moon: %1 %2 at %3:%4. ").arg(months[fullMonth-1]).arg(fullDay).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));

			LocalTMoon = 24.*modf(LocalNext,&intMoon);
			StelUtils::getDateFromJulianDay(intMoon,&fullYear,&fullMonth,&fullDay);
			double2hms(toUnsignedRA(LocalTMoon),fullHour,fullMinute,fullSecond);			
			bestNight += q_("Next Full Moon: %1 %2 at %3:%4. ").arg(months[fullMonth-1]).arg(fullDay).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));

			ObsRange = ""; 
			AcroCos = "";


	// Now, compute the days of all the Full Moons of the current year, and get the Earth/Moon distance:
//			double monthFrac, monthTemp, maxMoonDate;
//			monthFrac = std::modf((nextFullMoon-Jan1stJD)/MoonT,&monthTemp);
//			int PrevMonths = (int)(monthTemp+0.0*monthFrac); 
//			double BestDistance = 1.0; // initial dummy value for Sun-Moon distance;
//			double Distance; // temporal variable to save Earth-Moon distance at each month.

//			qDebug() << q_("%1 ").arg(PrevMonths);

//			for (int i=-PrevMonths; i<13 ; i++)
//			{
//				jd1 = nextFullMoon + MoonT*((double) i);
//				getMoonDistance(core,jd1,Distance,false); 
//				if (Distance < BestDistance)
//				{  // Month with the largest Full Moon:
//					BestDistance = Distance;
//					maxMoonDate = jd1;
//				};
//			};
//			maxMoonDate += GMTShift+0.5;
//			StelUtils::getDateFromJulianDay(maxMoonDate,&fullYear,&fullMonth,&fullDay);
//			double MoonSize = MoonPerilune/BestDistance*100.;
//			ObsRange = q_("Greatest Full Moon: %1 "+months[fullMonth-1]+" (%2% of Moon at Perilune)").arg(fullDay).arg(MoonSize,0,'f',2);
		};
	} 
	else if (Kind <3)
	{
		bestNight = "";
		ObsRange = ""; 
		AcroCos = "";

	}; 


// Return the Moon and Earth to its current position:
	if (Kind<3)
	{
		getSunMoonCoords(core,myJD,RAS,DecS,RA,Dec,EclLon,true);
	} else
	{
                getPlanetCoords(core,myJD,RA,Dec,true);
	};


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

	conf->beginGroup("Observability");

	// delete all existing settings...
	conf->remove("");

	// Set defaults
	conf->setValue("font_size", 15);
	conf->setValue("Sun_Altitude", 12);
	conf->setValue("Horizon_Altitude", 0);
	conf->setValue("font_color", "0,0.5,1");
	conf->setValue("show_AcroCos", true);
	conf->setValue("show_Good_Nights", true);
	conf->setValue("show_Best_Night", true);
	conf->setValue("show_Today", true);
	conf->setValue("show_FullMoon", true);
//	conf->setValue("show_Crescent", true);
//	conf->setValue("show_SuperMoon", true);

	conf->endGroup();
}

void Observability::readSettingsFromConfig(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	conf->beginGroup("Observability");

	// Load settings from main config file
	fontSize = conf->value("font_size",15).toInt();
	font.setPixelSize(fontSize);
	fontColor = StelUtils::strToVec3f(conf->value("font_color", "0,0.5,1").toString());
	show_AcroCos = conf->value("show_AcroCos", true).toBool();
	show_Good_Nights = conf->value("show_Good_Nights", true).toBool();
	show_Best_Night = conf->value("show_Best_Night", true).toBool();
	show_Today = conf->value("show_Today", true).toBool();
	show_FullMoon = conf->value("show_FullMoon", true).toBool();
//	show_Crescent = conf->value("show_Crescent", true).toBool();
//	show_SuperMoon = conf->value("show_SuperMoon", true).toBool();

	iAltitude = conf->value("Sun_Altitude", 12).toInt();
	AstroTwiAlti  = -((double)iAltitude)/Rad2Deg ;

	iHorizAltitude = conf->value("Horizon_Altitude", 0).toInt();
	HorizAlti = ((double)iHorizAltitude)/Rad2Deg ;
	

	conf->endGroup();
}

void Observability::saveSettingsToConfig(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	QString fontColorStr = QString("%1,%2,%3").arg(fontColor[0],0,'f',2).arg(fontColor[1],0,'f',2).arg(fontColor[2],0,'f',2);
	// Set updated values
	conf->beginGroup("Observability");
	conf->setValue("font_size", fontSize);
	conf->setValue("Sun_Altitude", iAltitude);
	conf->setValue("Horizon_Altitude", iHorizAltitude);
	conf->setValue("font_color", fontColorStr);
	conf->setValue("show_AcroCos", show_AcroCos);
	conf->setValue("show_Good_Nights", show_Good_Nights);
	conf->setValue("show_Best_Night", show_Best_Night);
	conf->setValue("show_Today", show_Today);
	conf->setValue("show_FullMoon", show_FullMoon);
//	conf->setValue("show_Crescent", show_Crescent);
//	conf->setValue("show_SuperMoon", show_SuperMoon);
	conf->endGroup();
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

int Observability::getHorizAltitude(void)
{
	return iHorizAltitude;
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

void Observability::setHorizAltitude(int value)
{
	HorizAlti = ((double) value)/Rad2Deg ;
	iHorizAltitude = value;
	configChanged = true;
}


///  END OF STUFF FOR THE GUI CONFIG.
///////////////////////////////




// Enable the Observability:
void Observability::enableObservability(bool b)
{
	flagShowObservability = b;
}

