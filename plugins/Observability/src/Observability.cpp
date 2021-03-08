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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QSettings>
#include <QString>
#include <QTimer>

#include "Observability.hpp"
#include "ObservabilityDialog.hpp"

#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StarMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFader.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelObserver.hpp"
#include "StelProjector.hpp"
#include "StelSkyDrawer.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"
#include "ZoneArray.hpp"


StelModule* ObservabilityStelPluginInterface::getStelModule() const
{
	return new Observability();
}

StelPluginInfo ObservabilityStelPluginInterface::getPluginInfo() const
{
	Q_INIT_RESOURCE(Observability);

	StelPluginInfo info;
	info.id = "Observability";
	info.displayedName = N_("Observability Analysis");
	info.authors = "Ivan Marti-Vidal (Onsala Space Observatory)"; // non-translatable field
	info.contact = "i.martividal@gmail.com";
	info.description = N_("Displays an analysis of a selected object's observability (rise, set, and transit times) for the current date, as well as when it is observable through the year. An object is assumed to be observable if it is above the horizon during a fraction of the night. Also included are the dates of the largest separation from the Sun and acronychal and cosmical rising and setting. (Explanations are provided in the 'About' tab of the plugin's configuration window.)");
	info.version = OBSERVABILITY_PLUGIN_VERSION;
	info.license = OBSERVABILITY_PLUGIN_LICENSE;
	return info;
}

// Some useful constants:
const double Observability::Rad2Deg = M_180_PI;         // Convert degrees into radians
const double Observability::Rad2Hr = 12./M_PI;           // Convert hours into radians
const double Observability::UA =  AU;                    // 1.4958e+8;         // Astronomical Unit in Km. ==> HAS BEEN DEFINED IN StelUtils.hpp!
const double Observability::TFrac = 0.9972677595628414;  // Convert sidereal time into Solar time
const double Observability::halfpi = M_PI_2;         //  1.57079632675; // pi/2
const double Observability::MoonT = 29.530588;           // Moon synodic period (used as first estimate of Full Moon). ==> FIND MORE DEC. PLACES!
const double Observability::RefFullMoon = 2451564.696; // Reference Julian date of a Full Moon.
const double Observability::MoonPerilune = 0.0024236308; // Smallest Earth-Moon distance (in AU).


Observability::Observability()
	: configDialog(new ObservabilityDialog())
	, nextFullMoon(0.)
	, prevFullMoon(0.)
	, GMTShift(0.)
	, Jan1stJD(0.)
	, twilightAltRad(0.)
	, twilightAltDeg(0.)
	, refractedHorizonAlt(0.)
	, horizonAltitude(0.)
	, horizonAltDeg(0.)
	, selRA(0.)
	, selDec(0.)
	, mylat(1000.)
	, mylon(1000.)
	, alti(0.)
	, horizH(0.)
	, culmAlt(0.)
	, myJD(0., 0.)
	, MoonRise(0.)
	, MoonSet(0.)
	, MoonCulm(0.)	
	, lastJDMoon(0.)	
	, ObserverLoc(0.)
	, myPlanet(Q_NULLPTR)
	, curYear(0)
	, nDays(0)
	, dmyFormat(false)
	, isStar(true)
	, isMoon(false)
	, isSun(false)
	, isScreen(true)
	, hasRisen(false)
	, configChanged(false)
	, souChanged(false)
	, lastType(0)
	, show_AcroCos(false)
	, show_Good_Nights(false)
	, show_Best_Night(false)
	, show_Today(false)
	, show_FullMoon(false)
	, flagShowReport(false)
	, fontSize(14)	
	, button(Q_NULLPTR)
{
	setObjectName("Observability");

	//Get pointer to the Earth:
	PlanetP Earth = GETSTELMODULE(SolarSystem)->getEarth();
	myEarth = Earth.data();

	// Get pointer to the Moon/Sun:
	PlanetP Moon = GETSTELMODULE(SolarSystem)->getMoon();
	myMoon = Moon.data();

	for (int i=0;i<366;i++) {
		yearJD[i]=QPair<double, double>(0.0, 0.0);
	};
	memset(sunRA,      0,   366*sizeof(double));
	memset(sunDec,     0,   366*sizeof(double));
	memset(objectRA,   0,   366*sizeof(double));
	memset(objectDec,  0,   366*sizeof(double));
	memset(sunSidT,    0, 4*366*sizeof(double));
	memset(objectSidT, 0, 2*366*sizeof(double));
	memset(objectH0,   0,   366*sizeof(double));
}

Observability::~Observability()
{
	// Shouldn't this be in the deinit()? --BM
	if (configDialog != Q_NULLPTR)
		delete configDialog;
}

void Observability::updateMessageText()
{
	// TRANSLATORS: Short for "hours".
	msgH		= q_("h");
	// TRANSLATORS: Short for "minutes".
	msgM		= q_("m");
	// TRANSLATORS: Short for "seconds".
	msgS		= q_("s");
	msgSetsAt	= q_("Sets at %1 (in %2)");
	msgRoseAt	= q_("Rose at %1 (%2 ago)");
	msgSetAt	= q_("Set at %1 (%2 ago)");
	msgRisesAt	= q_("Rises at %1 (in %2)");
	msgCircumpolar	= q_("Circumpolar.");
	msgNoRise	= q_("No rise.");
	msgCulminatesAt	= q_("Culminates at %1 (in %2) at %3 deg.");
	msgCulminatedAt	= q_("Culminated at %1 (%2 ago) at %3 deg.");
	msgSrcNotObs	= q_("Source is not observable.");
	msgNoACRise	= q_("No Acronychal nor Cosmical rise/set.");
	msgGreatElong	= q_("Greatest elongation: %1 (at %2 deg.)");
	msgLargSSep	= q_("Largest Sun separation: %1 (at %2 deg.)");
	msgNone		= q_("None");
	// TRANSLATORS: The space at the end is significant - another sentence may follow.
	msgAcroRise	= q_("Acronycal rise/set: %1/%2. ");
	msgHeliRise	= q_("Heliacal rise/set: %1/%2. ");
	msgNoHeliRise	= q_("No Heliacal rise/set. ");
	// TRANSLATORS: The space at the end is significant - another sentence may follow.
	msgNoAcroRise	= q_("No Acronycal rise/set. ");
	msgCosmRise	= q_("Cosmical rise/set: %1/%2.");
	msgNoCosmRise	= q_("No Cosmical rise/set.");
	msgWholeYear	= q_("Observable during the whole year.");
	msgNotObs	= q_("Not observable at dark night.");
	msgAboveHoriz	= q_("Nights above horizon: %1");
	msgToday	= q_("TODAY:");
	msgThisYear	= q_("THIS YEAR:");
	// TRANSLATORS: The space at the end is significant - another sentence may follow.
	msgPrevFullMoon	= q_("Previous Full Moon: %1 %2 at %3:%4. ");
	msgNextFullMoon	= q_("Next Full Moon: %1 %2 at %3:%4. ");
}

double Observability::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LabelMgr")->getCallOrder(actionName)+110.;
	return 0;
}

void Observability::init()
{
	loadConfiguration();
	
	StelAction* actionShow = addAction("actionShow_Observability",
	                                   N_("Observability"),
	                                   N_("Observability"),
	                                   "flagShowReport");
	// actionShow->setChecked(flagShowReport); //Unnecessary?
	addAction("actionShow_Observability_ConfigDialog",
	          N_("Observability"),
	          N_("Observability configuration window"),
		  configDialog, "visible", ""); // Allow assign shortkey

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui!=Q_NULLPTR)
	{
		button = new StelButton(Q_NULLPTR,
					QPixmap(":/observability/bt_observab_on.png"),
					QPixmap(":/observability/bt_observab_off.png"),
					QPixmap(":/graphicGui/miscGlow32x32.png"),
					actionShow);
		gui->getButtonBar()->addButton(button, "065-pluginsGroup");
	}
	
	updateMessageText();
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(updateMessageText()));
	connect(StelApp::getInstance().getCore(), SIGNAL(configurationDataSaved()), this, SLOT(saveConfiguration()));
}

/////////////////////////////////////////////
// MAIN CODE:
void Observability::draw(StelCore* core)
{
	if (!flagShowReport)
		return; // Button is off.
/////////////////////////////////////////////////////////////////
// PRELIMINARS:
	bool locChanged, yearChanged;
	StelObjectP selectedObject;
	PlanetP ssObject, parentPlanet;

// Only execute plugin if we are on Earth.
	if (core->getCurrentLocation().planetName != "Earth")
		return;

// Set the painter:
	StelPainter painter(core->getProjection2d());
	painter.setColor(fontColor[0],fontColor[1],fontColor[2],1.f);
	font.setPixelSize(fontSize);
	painter.setFont(font);

// Get current date, location, and check if there is something selected.
	double currlat = static_cast<double>(core->getCurrentLocation().latitude)/Rad2Deg;
	double currlon = static_cast<double>(core->getCurrentLocation().longitude)/Rad2Deg;
	double currheight = (6371.+(core->getCurrentLocation().altitude)/1000.)/UA;
	double currJD = core->getJD();
	double currJDint;
	GMTShift = core->getUTCOffset(currJD)/24.0;


	double currLocalT = 24.*modf(currJD + GMTShift,&currJDint);
	int auxm, auxd, auxy;
	StelUtils::getDateFromJulianDay(currJD, &auxy, &auxm, &auxd);
	bool isSource = StelApp::getInstance().getStelObjectMgr().getWasSelected();
	bool show_Year = show_Best_Night || show_Good_Nights || show_AcroCos; 

//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// NOW WE CHECK THE CHANGED PARAMETERS W.R.T. THE PREVIOUS FRAME:

// Update JD.
	myJD.first = currJD;
	myJD.second = currJD + core->computeDeltaT(currJD)/86400.;

// If the year changed, we must recompute the Sun's position for each new day:
	if (auxy != curYear)
	{
		yearChanged = true;
		curYear = auxy;
		updateSunData(core);
	}
	else
	{
		yearChanged = false;
	};

// Have we changed the latitude or longitude?
	if (currlat == mylat && currlon == mylon)
	{
		locChanged = false;
	}
	else
	{
		locChanged = true;
		mylat = currlat; mylon = currlon;
		double temp1 = currheight*std::cos(currlat);
		ObserverLoc[0] = temp1*std::cos(currlon);
		ObserverLoc[1] = temp1*std::sin(currlon);
		ObserverLoc[2] = currheight*std::sin(currlat);
	};



// Add refraction, if necessary:
	Vec3d TempRefr;	
	TempRefr[0] = std::cos(horizonAltitude);  
	TempRefr[1] = 0.0; 
	TempRefr[2] = std::sin(horizonAltitude);  
	Vec3d CorrRefr = core->altAzToEquinoxEqu(TempRefr,StelCore::RefractionAuto);
	TempRefr = core->equinoxEquToAltAz(CorrRefr,StelCore::RefractionOff);
	double RefracAlt = std::asin(TempRefr[2]);

	// If the diference is larger than 1 arcminute...
	if (qAbs(refractedHorizonAlt-RefracAlt)>2.91e-4)
	{
		//... configuration for refraction changed notably.
		refractedHorizonAlt = RefracAlt;
		configChanged = true;
		souChanged = true;
	};





// If we have changed latitude (or year), we update the vector of Sun's hour 
// angles at twilight, and re-compute Sun/Moon ephemeris (if selected):
	if (locChanged || yearChanged || configChanged) 
	{
		updateSunH();
		lastJDMoon = 0.0;
	};

//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// NOW WE DEAL WITH THE SOURCE (OR SCREEN-CENTER) POSITION:

	if (isScreen) // Always re-compute everything for the screen center.
		souChanged = true; 

	if (isSource) // There is something selected!
	{ 
// Get the selected source and its name:
		selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]; 

// Don't do anything for satellites:
		if(selectedObject->getType() == "Satellite")
			return;

		QString name = selectedObject->getEnglishName();
		isMoon = ("Moon" == name);
		isSun = ("Sun" == name);

// If Moon is not selected (or was unselected), force re-compute of Full Moon next time it is selected:
		if (!isMoon)
		{
			prevFullMoon = 0.0;
			nextFullMoon = 0.0;
		};

//Update position:
		EquPos = selectedObject->getEquinoxEquatorialPos(core);
		EquPos.normalize();
		LocPos = core->equinoxEquToAltAz(EquPos, StelCore::RefractionOff);

// Check if the user has changed the source (or if the source is Sun/Moon). 
		if (name == selName) 
		{
			souChanged = false;
		}
		else 
		{
			// Check also if the (new) source belongs to the Solar System:
			souChanged = true;
			selName = name;

			Planet* planet = dynamic_cast<Planet*>(selectedObject.data());
			isStar = (planet == Q_NULLPTR);

			if (!isStar && !isMoon && !isSun)  // Object in the Solar System, but is not Sun nor Moon.
			{ 
				int gene = -1;

			// If object is a planet's moon, we get its parent planet:
				ssObject = GETSTELMODULE(SolarSystem)->searchByEnglishName(selName);
				// TODO: Isn't it easier just to use the planet object we just cast? --BM

				parentPlanet = ssObject->getParent();
				if (parentPlanet) 
				{
					while (parentPlanet)
					{
						gene += 1;
						parentPlanet = parentPlanet->getParent();
					}
				}
				for (int g=0; g<gene; g++)
				{
					ssObject = ssObject->getParent();
				}
				
			// Now get a pointer to the planet's instance:
				myPlanet = ssObject.data();
			}
		}
	}
	else // There is no source selected!
	{
	// If no source is selected, get the position vector of the screen center:
		selName.clear();
		isStar = true;
		isMoon = false;
		isSun = false;
		isScreen = true;
		Vec3d currentPos = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
		currentPos.normalize();
		EquPos = core->j2000ToEquinoxEqu(currentPos, StelCore::RefractionOff);
		LocPos = core->j2000ToAltAz(currentPos, StelCore::RefractionOff);
	}


// Convert EquPos to RA/Dec:
	toRADec(EquPos, selRA, selDec);

// Compute source's altitude (in radians):
	alti = std::asin(LocPos[2]);


// Force re-computation of ephemeris if the location changes or the user changes the configuration:
	if (locChanged || configChanged || yearChanged)
	{ 
		souChanged=true;
		configChanged=false;
	};

/////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////
// NOW WE COMPUTE RISE/SET/TRANSIT TIMES FOR THE CURRENT DAY:
	double currH = calculateHourAngle(mylat,alti,selDec);
	horizH = calculateHourAngle(mylat,refractedHorizonAlt,selDec);
	QString RS1, RS2, Cul; // strings with Rise/Set/Culmination times
	double risingTime = 0, settingTime = 0; // Actual Rise/Set times (in GMT).
	int d1,m1,s1,d2,m2,s2,dc,mc,sc; // Integers for the time spans in hh:mm:ss.
	bool solvedMoon = false; // Check if solutions were found for Sun, Moon, or planet.
	bool transit = false; // Is the source above the horizon? Did it culminate?

	int ephHour, ephMinute, ephSecond;  // Local time for selected ephemeris

	if (show_Today)
	{
		// Today's ephemeris (rise, set, and transit times)
		if (!isStar) 
		{
			int type = (isSun) ? 1:0;
			type += (isMoon) ? 2:0;
			type += (!isSun && !isMoon) ? 3:0;
			
			// Returns false if the calculation fails...
			solvedMoon = calculateSolarSystemEvents(core, type);
			currH = qAbs(24.*(MoonCulm-myJD.first)/TFrac);
			transit = MoonCulm-myJD.first<0.0;
			if (solvedMoon)
			{  // If failed, Set and Rise will be dummy.
				settingTime = qAbs(24.*(MoonSet-myJD.first)/TFrac);
				risingTime = qAbs(24.*(MoonRise-myJD.first)/TFrac);
			}
		}
		else if (horizH>0.0)
		{
			// The source is not circumpolar and can be seen from this latitude.
			if ( LocPos[1]>0.0 ) // The source is at the eastern side...
			{
				if ( currH>horizH ) // ... and below the horizon.
				{
					settingTime = 24.-currH-horizH;
					risingTime = currH-horizH;
					hasRisen = false;
				}
				else  // ... and above the horizon.
				{
					risingTime = horizH-currH;
					settingTime = 2.*horizH-risingTime;
					hasRisen = true;
				}
			}
			else // The source is at the western side...
			{
				if ( currH>horizH ) // ... and below the horizon. 
				{
					settingTime = currH-horizH;
					risingTime = 24.-currH-horizH;
					hasRisen = false;
				}
				else // ... and above the horizon.
				{
					risingTime = horizH+currH;
					settingTime = horizH-currH;
					hasRisen = true;
				}
			}
		}
		
		if ((solvedMoon && MoonRise>0.0) || (!isSun && !isMoon && horizH>0.0))
		{
			double2hms(TFrac*settingTime, d1, m1, s1);
			double2hms(TFrac*risingTime, d2, m2, s2);
			
			//		Strings with time spans for rise/set/transit:
			RS1 = (d1==0)?"":QString("%1%2 ").arg(d1).arg(msgH);
			RS1 += (m1==0)?"":QString("%1%2 ").arg(m1).arg(msgM);
			RS1 += QString("%1%2").arg(s1).arg(msgS);
			RS2 = (d2==0)?"":QString("%1%2 ").arg(d2).arg(msgH);
			RS2 += (m2==0)?"":QString("%1%2 ").arg(m2).arg(msgM);
			RS2 += QString("%1%2").arg(s2).arg(msgS);
			if (hasRisen) 
			{
				double2hms(toUnsignedRA(currLocalT+TFrac*settingTime+12.),
				           ephHour, ephMinute, ephSecond);
				SetTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QChar('0')); // Local time for set.

				double2hms(toUnsignedRA(currLocalT-TFrac*risingTime+12.),
				           ephHour, ephMinute, ephSecond); // Local time for rise.
				RiseTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				
				//RS1 = q_("Sets at %1 (in %2)").arg(SetTime).arg(RS1);
				//RS2 = q_("Rose at %1 (%2 ago)").arg(RiseTime).arg(RS2);
				RS1 = msgSetsAt.arg(SetTime).arg(RS1);
				RS2 = msgRoseAt.arg(RiseTime).arg(RS2);
			}
			else 
			{
				double2hms(toUnsignedRA(currLocalT-TFrac*settingTime+12.),
				           ephHour, ephMinute, ephSecond);
				SetTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				
				double2hms(toUnsignedRA(currLocalT+TFrac*risingTime+12.),
				           ephHour, ephMinute, ephSecond);
				RiseTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				
				//RS1 = q_("Set at %1 (%2 ago)").arg(SetTime).arg(RS1);
				//RS2 = q_("Rises at %1 (in %2)").arg(RiseTime).arg(RS2);
				RS1 = msgSetAt.arg(SetTime).arg(RS1);
				RS2 = msgRisesAt.arg(RiseTime).arg(RS2);
			}
		}
		else // The source is either circumpolar or never rises:
		{
			(alti>refractedHorizonAlt)? RS1 = msgCircumpolar: RS1 = msgNoRise;
			RS2 = "";
		};
		
		// 	Culmination:
		
		if (isStar)
		{
			culmAlt = qAbs(mylat-selDec); // 90.-altitude at transit.
			transit = LocPos[1]<0.0;
		};
		
		if (culmAlt < (halfpi - refractedHorizonAlt)) // Source can be observed.
		{
//			THESE IS FREE OF ATMOSPHERE AND REFERRED TO TRUE HORIZON!
			double altiAtCulmi = (halfpi-culmAlt); //-refractedHorizonAlt);

// Add refraction, if necessary:
			Vec3d TempRefr;	
			TempRefr[0] = std::cos(altiAtCulmi);  
			TempRefr[1] = 0.0; 
			TempRefr[2] = std::sin(altiAtCulmi);  
			Vec3d CorrRefr = core->altAzToEquinoxEqu(TempRefr,StelCore::RefractionOff);
			TempRefr = core->equinoxEquToAltAz(CorrRefr,StelCore::RefractionAuto);
			altiAtCulmi = Rad2Deg*std::asin(TempRefr[2]);

			double2hms(TFrac*currH,dc,mc,sc);
			
			//String with the time span for culmination:
			Cul = (dc==0)?"":QString("%1%2 ").arg(dc).arg(msgH);
			Cul += (mc==0)?"":QString("%1%2 ").arg(mc).arg(msgM);
			Cul += QString("%1%2").arg(sc).arg(msgS);
			if (!transit)
			{ 
				double2hms(toUnsignedRA(currLocalT + TFrac*currH + 12.),
				           ephHour, ephMinute, ephSecond); // Local time at transit.
				CulmTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				//Cul = q_("Culminates at %1 (in %2) at %3 deg.").arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
				Cul = msgCulminatesAt.arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
			}
			else
			{
				double2hms(toUnsignedRA(currLocalT - TFrac*currH + 12.),
				           ephHour, ephMinute, ephSecond);
				CulmTime = QString("%1:%2").arg(ephHour).arg(ephMinute,2,10,QLatin1Char('0'));
				//Cul = q_("Culminated at %1 (%2 ago) at %3 deg.").arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
				Cul = msgCulminatedAt.arg(CulmTime).arg(Cul).arg(altiAtCulmi,0,'f',1);
			}
		}
	} // This comes from show_Today==True
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// NOW WE ANALYZE THE SOURCE OBSERVABILITY FOR THE WHOLE YEAR:

// Compute yearly ephemeris (only if necessary, and not for Sun nor Moon):


	if (isSun) 
	{
		lineBestNight.clear();
		lineObservableRange.clear();
	}
	else if (!isMoon && show_Year)
	{
		if (!isStar && (souChanged || yearChanged)) // Object moves.
			updatePlanetData(core); // Re-compute ephemeris.
		else
		{ // Object is fixed on the sky.
			double auxH = calculateHourAngle(mylat,refractedHorizonAlt,selDec);
			double auxSidT1 = toUnsignedRA(selRA - auxH); 
			double auxSidT2 = toUnsignedRA(selRA + auxH); 
			for (int i=0;i<nDays;i++) {
				objectH0[i] = auxH;
				objectRA[i] = selRA;
				objectDec[i] = selDec;
				objectSidT[0][i] = auxSidT1;
				objectSidT[1][i] = auxSidT2;
			};
		};

// Determine source observability (only if something changed):
		if ((souChanged || locChanged || yearChanged))
		{
			lineBestNight.clear();
			lineObservableRange.clear();

			// Check if the target cannot be seen.
			if (culmAlt >= (halfpi - refractedHorizonAlt))
			{
				//ObsRange = q_("Source is not observable.");
				//AcroCos = q_("No Acronychal nor Cosmical rise/set.");
				lineObservableRange = msgSrcNotObs;
				lineAcroCos = msgNoACRise;
				lineHeli = msgNoHeliRise;
			}
			else
			{ // Source can be seen.
///////////////////////////
// - Part 1. Determine the best observing night (i.e., opposition to the Sun):
				if (show_Best_Night)
				{
					int selday = 0;
					double deltaPhs = -1.0; // Initial dummy value
					double tempPhs; 	
					for (int i=0; i<nDays; i++) // Maximize the Sun-object separation.
					{
						tempPhs = Lambda(objectRA[i], objectDec[i],
						                 sunRA[i], sunDec[i]);
						if (tempPhs > deltaPhs)
						{
							selday = i;
							deltaPhs = tempPhs;
						}
					}

					if (selName=="Mercury" || selName=="Venus")
					{
						lineBestNight = msgGreatElong;
					}
					else 
					{
						lineBestNight = msgLargSSep;
					}
					
					lineBestNight = lineBestNight
					                .arg(formatAsDate(selday))
					                .arg(deltaPhs*Rad2Deg, 0, 'f', 1);
				}

///////////////////////////////
// - Part 2. Determine Acronychal and Cosmical rise and set:

				if (show_AcroCos)
				{
					int acroRise, acroSet, cosRise, cosSet, heliRise, heliSet;

					int result = calculateAcroCos(acroRise, acroSet,
					                              cosRise, cosSet);
					int resultHeli = calculateHeli(0,heliRise,heliSet);

					QString acroRiseStr, acroSetStr;
					QString cosRiseStr, cosSetStr;
					QString heliRiseStr, heliSetStr;
					// TODO: Possible error? Day 0 is 1 Jan. ==> IMV: Indeed! Corrected
					acroRiseStr = (acroRise>=0)?formatAsDate(acroRise):msgNone;
					acroSetStr = (acroSet>=0)?formatAsDate(acroSet):msgNone;
					cosRiseStr = (cosRise>0)?formatAsDate(cosRise):msgNone;
					cosSetStr = (cosSet>0)?formatAsDate(cosSet):msgNone;
					heliRiseStr = (heliRise>=0)?formatAsDate(heliRise):msgNone;
					heliSetStr = (heliSet>=0)?formatAsDate(heliSet):msgNone;


					if (result==3 || result==1)
						lineAcroCos =  msgAcroRise
						               .arg(acroRiseStr)
						               .arg(acroSetStr);
					else
						lineAcroCos =  msgNoAcroRise;
					
					if (result==3 || result==2)
						lineAcroCos += msgCosmRise
						               .arg(cosRiseStr)
						               .arg(cosSetStr);
					else
						lineAcroCos += msgNoCosmRise;

					if (resultHeli==1)
						lineHeli = msgHeliRise.arg(heliRiseStr).arg(heliSetStr);
					else
						lineHeli = msgNoHeliRise;
				}

////////////////////////////
// - Part 3. Determine range of good nights 
// (i.e., above horizon before/after twilight):
				if (show_Good_Nights)
				{
					int selday = 0;
					int selday2 = 0;
					bool bestBegun = false; // Are we inside a good time range?
					bool atLeastOne = false;
					QString dateRange;
					bool poleNight, twiGood;

					for (int i=0; i<nDays; i++)
					{
						poleNight = sunSidT[0][i]<0.0 && qAbs(sunDec[i]-mylat)>=halfpi; // Is it night during 24h?
						twiGood = (poleNight && qAbs(objectDec[i]-mylat)<halfpi)?true:CheckRise(i);
						
						if (twiGood && bestBegun == false)
						{
							selday = i;
							bestBegun = true;
							atLeastOne = true;
						};

						if (!twiGood && bestBegun == true)
						{
							selday2 = i;
							bestBegun = false;
							if (selday2 > selday)
							{
								// FIXME: This kind of concatenation is bad for i18n.
								if (!dateRange.isEmpty())
									dateRange += ", ";
								dateRange += QString("%1").arg(formatAsDateRange(selday, selday2));
							};
						};
					};

					// Check if there were good dates till the end of the year.
					if (bestBegun)
					{
						// FIXME: This kind of concatenation is bad for i18n.
						 if (!dateRange.isEmpty())
							 dateRange += ", ";
						dateRange += formatAsDateRange(selday, 0);
					};
					
					if (dateRange.isEmpty()) 
					{ 
						if (atLeastOne) 
						{
							//ObsRange = q_("Observable during the whole year.");
							lineObservableRange = msgWholeYear;
						}
						else
						{
							//ObsRange = q_("Not observable at dark night.");
							lineObservableRange = msgNotObs;
						};
					}
					else
					{
						// Nights when the target is above the horizon
						lineObservableRange = msgAboveHoriz.arg(dateRange);
					};
				}; // Comes from show_Good_Nights==True"
			}; // Comes from the "else" of "culmAlt>=..." 
		};// Comes from  "souChanged || ..."
	}; // Comes from the "else" with "!isMoon"

// Print all results:
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	float ppx = static_cast<float>(params.devicePixelsPerPixel);
	int lineSpacing = static_cast<int>(ppx * 1.3 * fontSize);  // between lines
	int groupSpacing = static_cast<int>(6*fontSize*ppx);  // between daily and yearly results
	int yLine = static_cast<int>(8*fontSize*ppx) + 110*ppx;
	int xLine = 80;

	if (show_Today) 
	{
		//renderer->drawText(TextParams(xLine, yLine,q_("TODAY:")));
		painter.drawText(xLine, yLine, msgToday);
		painter.drawText(xLine + fontSize, yLine - lineSpacing, RS2);
		painter.drawText(xLine + fontSize, yLine - lineSpacing*2, RS1);
		painter.drawText(xLine + fontSize, yLine - lineSpacing*3, Cul);
		yLine -= groupSpacing;
	}
	
	if ((isMoon && show_FullMoon) || (!isSun && !isMoon && show_Year)) 
	{
		painter.drawText(xLine, yLine, msgThisYear);
		if (show_Best_Night || show_FullMoon)
		{
			yLine -= lineSpacing;
			painter.drawText(xLine + fontSize, yLine, lineBestNight);
		}
		if (show_Good_Nights)
		{
			yLine -= lineSpacing;
			painter.drawText(xLine + fontSize, yLine, lineObservableRange);
		}
		if (show_AcroCos)
		{
			yLine -= lineSpacing;
			painter.drawText(xLine + fontSize, yLine, lineAcroCos);
			yLine -= lineSpacing;
			painter.drawText(xLine + fontSize, yLine, lineHeli);
		}
	}
}

// END OF MAIN CODE
///////////////////////////////////////////////////////


//////////////////////////////
// AUXILIARY FUNCTIONS

////////////////////////////////////
// Returns the hour angle for a given altitude:
double Observability::calculateHourAngle(double latitude,
                                         double elevation,
                                         double declination)
{
	double denom = std::cos(latitude)*std::cos(declination);
	double numer = (std::sin(elevation)-std::sin(latitude)*std::sin(declination));

	if ( qAbs(numer) > qAbs(denom) )
	{
		return -0.5/86400.; // Source doesn't reach that altitude.
	}
	else
	{
		return Rad2Hr * std::acos(numer/denom);
	}
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
	double result = toUnsignedRA(RA-ST/15.);
	result -= (result > 12.) ? 24.0 : 0.0;
	return result;
}
////////////////////////////////////

////////////////////////////////////
// Converts a float time/angle span (in hours/degrees) in the (integer) format hh/dd,mm,ss:
void Observability::double2hms(double hfloat, int &h1, int &h2, int &h3)
{
	h1 = static_cast<int>(hfloat);
	h2 = static_cast<int>((qAbs(hfloat)-qAbs(double(h1)))*60);
	h3 = static_cast<int>(((qAbs(hfloat)-qAbs(double(h1)))*60)-h2)*60;
} 
////////////////////////////////////

////////////////////////////////////
// Adds/subtracts 24hr to ensure a RA between 0 and 24hr:
double Observability::toUnsignedRA(double RA)
{
	double tempRA,tempmod;	
	if (RA<0.0)
	{
		tempmod = std::modf(-RA/24.,&tempRA);
		RA += 24.*(tempRA+1.0)+0.0*tempmod;
	};
	double auxRA = 24.*std::modf(RA/24.,&tempRA);
	auxRA += (auxRA<0.0)?24.0:((auxRA>24.0)?-24.0:0.0);
	return auxRA;
}
////////////////////////////////////


QString Observability::formatAsDate(int dayNumber)
{
	int day, month, year;
	StelUtils::getDateFromJulianDay(yearJD[dayNumber].first, &year, &month, &day);

	QString formatString = (getDateFormat()) ? "%1 %2" : "%2 %1";
	QString result = formatString.arg(day).arg(StelLocaleMgr::shortMonthName(month));
	return result;
}

///////////////////////////////////////////////
// Returns the day and month of year (to put it in format '25 Apr')
QString Observability::formatAsDateRange(int startDay, int endDay)
{
	int sDay, sMonth, sYear, eDay, eMonth, eYear;
	QString range;
	StelUtils::getDateFromJulianDay(yearJD[startDay].first, &sYear, &sMonth, &sDay);
	StelUtils::getDateFromJulianDay(yearJD[endDay].first, &eYear, &eMonth, &eDay);
	if (endDay == 0)
	{
		eDay = 31;
		eMonth = 12;
	}
	if (startDay == 0)
	{
		sDay = 1;
		sMonth = 1;
	}
	
	// If it's the same month, display "X-Y Month" or "Month X-Y"
	if (sMonth == eMonth)
	{
		QString formatString = (getDateFormat()) ? "%1 - %2 %3" : "%3 %1 - %2";
		range = formatString.arg(sDay).arg(eDay).arg(StelLocaleMgr::shortMonthName(sMonth));
	}
	else
	{
		QString formatString = (getDateFormat()) ? "%1 %2 - %3 %4" 
		                                         : "%2 %1 - %4 %3";
		range = formatString.arg(sDay)
				    .arg(StelLocaleMgr::shortMonthName(sMonth))
		                    .arg(eDay)
				    .arg(StelLocaleMgr::shortMonthName(sMonth));
	}

	return range;
}
//////////////////////////////////////////////

// Compute planet's position for each day of the current year:
void Observability::updatePlanetData(StelCore *core)
{
	double tempH;
	for (int i=0; i<nDays; i++)
	{
		getPlanetCoords(core, yearJD[i], objectRA[i], objectDec[i], false);
		tempH = calculateHourAngle(mylat, refractedHorizonAlt, objectDec[i]);
		objectH0[i] = tempH;
		objectSidT[0][i] = toUnsignedRA(objectRA[i]-tempH);
		objectSidT[1][i] = toUnsignedRA(objectRA[i]+tempH);
	}

// Return the planet to its current time:
	getPlanetCoords(core, myJD, objectRA[0], objectDec[0], true);
}

/////////////////////////////////////////////////
// Computes the Sun's RA and Dec (and the JD) for 
// each day of the current year.
void Observability::updateSunData(StelCore* core) 
{
	int day, month, year, sameYear;
// Get current date:
	StelUtils::getDateFromJulianDay(myJD.first,&year,&month,&day);

// Get JD for the Jan 1 of current year:
	StelUtils::getJDFromDate(&Jan1stJD,year,1,1,0,0,0);

// Check if we are on a leap year:
	StelUtils::getDateFromJulianDay(Jan1stJD+365., &sameYear, &month, &day);
	nDays = (year==sameYear)?366:365;
	
// Compute Earth's position throughout the year:
	Vec3d pos, sunPos;
	for (int i=0; i<nDays; i++)
	{
		yearJD[i].first = Jan1stJD + i;
		yearJD[i].second = yearJD[i].first+core->computeDeltaT(yearJD[i].first)/86400.0;
		myEarth->computePosition(yearJD[i].second);
		myEarth->computeTransMatrix(yearJD[i].first, yearJD[i].second);
		pos = myEarth->getHeliocentricEclipticPos();
		sunPos = core->j2000ToEquinoxEqu((StelCore::matVsop87ToJ2000)*(-pos), StelCore::RefractionOff);
		EarthPos[i] = -pos;
		toRADec(sunPos,sunRA[i],sunDec[i]);
	};

//Return the Earth to its current time:
	myEarth->computePosition(myJD.second);
	myEarth->computeTransMatrix(myJD.first, myJD.second);
}
///////////////////////////////////////////////////


////////////////////////////////////////////
// Computes Sun's Sidereal Times at twilight and culmination:
void Observability::updateSunH()
{
	double tempH, tempH00;

	for (int i=0; i<nDays; i++)
	{
		tempH = calculateHourAngle(mylat, twilightAltRad, sunDec[i]);
		tempH00 = calculateHourAngle(mylat, refractedHorizonAlt, sunDec[i]);
		if (tempH > 0.0)
		{
			sunSidT[0][i] = toUnsignedRA(sunRA[i]-tempH*(1.00278));
			sunSidT[1][i] = toUnsignedRA(sunRA[i]+tempH*(1.00278));
		}
		else
		{
			sunSidT[0][i] = -1000.0;
			sunSidT[1][i] = -1000.0;
		}
		
		if (tempH00>0.0)
		{
			sunSidT[2][i] = toUnsignedRA(sunRA[i]+tempH00);
			sunSidT[3][i] = toUnsignedRA(sunRA[i]-tempH00);
		}
		else
		{
			sunSidT[2][i] = -1000.0;
			sunSidT[3][i] = -1000.0;
		}
	}
}
////////////////////////////////////////////

///////////////////////////////////////////
// Checks if a source can be observed with the Sun below the twilight altitude.
bool Observability::CheckRise(int day)
{
	// If Sun can't reach twilight elevation, the target is not visible.
	if (sunSidT[0][day]<0.0 || sunSidT[1][day]<0.0)
		return false;

	// Iterate over the whole year:
	int nBin = 1000;
	double auxSid1 = sunSidT[0][day];
	auxSid1 += (sunSidT[0][day] < sunSidT[1][day]) ? 24.0 : 0.0;
	double deltaT = (auxSid1-sunSidT[1][day]) / static_cast<double>(nBin);

	double hour; 
	for (int j=0; j<nBin; j++)
	{
		hour = toUnsignedRA(sunSidT[1][day]+deltaT*static_cast<double>(j) - objectRA[day]);
		hour -= (hour>12.) ? 24.0 : 0.0;
		if (qAbs(hour)<objectH0[day] || (objectH0[day] < 0.0 && alti>0.0))
			return true;
	}

	return false;
}
///////////////////////////////////////////

///////////////////////////////////////////
// Finds the dates of Acronichal (Rise, Set) and Cosmical (Rise2, Set2) dates.
int Observability::calculateHeli(int imethod, int &heliRise, int &heliSet)
{
	Q_UNUSED(imethod)

	heliRise = -1;
	heliSet = -1;

	double bestDiffHeliRise = 12.0;
	double bestDiffHeliSet = 12.0;

	double hourDiffHeliRise, hourDiffHeliSet;
	bool success = false;

	for (int i=0; i<366; i++)
	{
		if (objectH0[i]>0.0 && sunSidT[0][i]>0.0 && sunSidT[1][i]>0.0)
		{
			success = true;
			hourDiffHeliRise = toUnsignedRA(objectRA[i] - objectH0[i]);
		//	hourDiffCosRise = hourDiffAcroRise-sunSidT[0][i];
			hourDiffHeliRise -= sunSidT[0][i];
			
			hourDiffHeliSet = toUnsignedRA(objectRA[i] + objectH0[i]);
		//	hourCosDiffSet = hourDiffAcroSet - sunSidT[1][i];
			hourDiffHeliSet -= sunSidT[1][i];
			
			// Heliacal rise/set:
			if (qAbs(hourDiffHeliRise) < bestDiffHeliRise)
			{
				bestDiffHeliRise = qAbs(hourDiffHeliRise);
				heliRise = i;
			};
			if (qAbs(hourDiffHeliSet) < bestDiffHeliSet)
			{
				bestDiffHeliSet = qAbs(hourDiffHeliSet);
				heliSet = i;
			};
		};
	};

	heliRise *= (bestDiffHeliRise > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	heliSet *= (bestDiffHeliSet > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	int result = (heliRise>0 || heliSet>0) ? 1 : 0;
	return (success) ? result : 0;
};

///////////////////////////////////////////
// Finds the dates of Acronichal (Rise, Set) and Cosmical (Rise2, Set2) dates.
int Observability::calculateAcroCos(int &acroRise, int &acroSet,
                                    int &cosRise, int &cosSet)
{
	acroRise = -1;
	acroSet = -1;
	cosRise = -1;
	cosSet = -1;

	double bestDiffAcroRise = 12.0;
	double bestDiffAcroSet = 12.0;
	double bestDiffCosRise = 12.0;
	double bestDiffCosSet = 12.0;

	double hourDiffAcroRise, hourDiffAcroSet, hourDiffCosRise, hourCosDiffSet;
	bool success = false;

	for (int i=0; i<366; i++)
	{
		if (objectH0[i]>0.0 && sunSidT[2][i]>0.0 && sunSidT[3][i]>0.0)
		{
			success = true;
			hourDiffAcroRise = toUnsignedRA(objectRA[i] - objectH0[i]);
			hourDiffCosRise = hourDiffAcroRise-sunSidT[3][i];
			hourDiffAcroRise -= sunSidT[2][i];
			
			hourDiffAcroSet = toUnsignedRA(objectRA[i] + objectH0[i]);
			hourCosDiffSet = hourDiffAcroSet - sunSidT[2][i];
			hourDiffAcroSet -= sunSidT[3][i];
			
			// Acronychal rise/set:
			if (qAbs(hourDiffAcroRise) < bestDiffAcroRise)
			{
				bestDiffAcroRise = qAbs(hourDiffAcroRise);
				acroRise = i;
			};
			if (qAbs(hourDiffAcroSet) < bestDiffAcroSet)
			{
				bestDiffAcroSet = qAbs(hourDiffAcroSet);
				acroSet = i;
			};
			
			// Cosmical Rise/Set:
			if (qAbs(hourDiffCosRise) < bestDiffCosRise)
			{
				bestDiffCosRise = qAbs(hourDiffCosRise);
				cosRise = i;
			};
			if (qAbs(hourCosDiffSet) < bestDiffCosSet)
			{
				bestDiffCosSet = qAbs(hourCosDiffSet);
				cosSet = i;
			};
		};
	};

	acroRise *= (bestDiffAcroRise > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	acroSet *= (bestDiffAcroSet > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	cosRise *= (bestDiffCosRise > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	cosSet *= (bestDiffCosSet > 0.083)?-1:1; // Check that difference is lower than 5 minutes.
	int result = (acroRise>0 || acroSet>0) ? 1 : 0;
	result += (cosRise>0 || cosSet>0) ? 2 : 0;
	return (success) ? result : 0;
}
///////////////////////////////////////////


////////////////////////////////////////////
// Convert an Equatorial Vec3d into RA and Dec:
void Observability::toRADec(Vec3d vec3d, double& ra, double &dec)
{
	vec3d.normalize();
	dec = std::asin(vec3d[2]); // in radians
	ra = toUnsignedRA(std::atan2(vec3d[1],vec3d[0])*Rad2Hr); // in hours.
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
void Observability::getSunMoonCoords(StelCore *core, QPair<double, double> JD,
				     double &raSun, double &decSun,
				     double &raMoon, double &decMoon,
				     double &eclLon, bool getBack)
                                     //, Vec3d &AltAzVector)
{
	if (getBack) // Return the Moon and Earth to their current position:
	{
		myEarth->computePosition(JD.second);
		myEarth->computeTransMatrix(JD.first, JD.second);
		myMoon->computePosition(JD.second);
		myMoon->computeTransMatrix(JD.first, JD.second);
	} 
	else // Compute coordinates:
	{
		myEarth->computePosition(JD.second);
		myEarth->computeTransMatrix(JD.first, JD.second);
		Vec3d earthPos = myEarth->getHeliocentricEclipticPos();
		double curSidT;

// Sun coordinates:
		Vec3d sunPos = core->j2000ToEquinoxEqu((StelCore::matVsop87ToJ2000)*(-earthPos), StelCore::RefractionOff);
		toRADec(sunPos, raSun, decSun);

// Moon coordinates:
		curSidT = myEarth->getSiderealTime(JD.first, JD.second)/Rad2Deg;
		RotObserver = (Mat4d::zrotation(curSidT))*ObserverLoc;
		LocTrans = (StelCore::matVsop87ToJ2000)*(Mat4d::translation(-earthPos));
		myMoon->computePosition(JD.second);
		myMoon->computeTransMatrix(JD.first, JD.second);
		Vec3d moonPos = myMoon->getHeliocentricEclipticPos();
		sunPos = (core->j2000ToEquinoxEqu(LocTrans*moonPos, StelCore::RefractionOff))-RotObserver;
		
		eclLon = moonPos[0]*earthPos[1] - moonPos[1]*earthPos[0];

		toRADec(sunPos,raMoon,decMoon);
	};
}
//////////////////////////////////////////////

//////////////////////////
// Get the Observer-to-Moon distance JD:
// getBack controls whether Earth and Moon must be returned to their original positions after computation.
void Observability::getMoonDistance(StelCore *core, QPair<double, double> JD, double &distance, bool getBack)
{
	if (getBack) // Return the Moon and Earth to their current position:
	{
		myEarth->computePosition(JD.second);
		myEarth->computeTransMatrix(JD.first, JD.second);
		myMoon->computePosition(JD.second);
		myMoon->computeTransMatrix(JD.first, JD.second);
	} 
	else
	{	// Compute coordinates:
		myEarth->computePosition(JD.second);
		myEarth->computeTransMatrix(JD.first, JD.second);
		Vec3d earthPos = myEarth->getHeliocentricEclipticPos();
//		double curSidT;

// Sun coordinates:
//		Pos2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*(-Pos0));
//		toRADec(Pos2,RASun,DecSun);

// Moon coordinates:
//		curSidT = myEarth->getSiderealTime(JD)/Rad2Deg;
//		RotObserver = (Mat4d::zrotation(curSidT))*ObserverLoc;
		LocTrans = (StelCore::matVsop87ToJ2000)*(Mat4d::translation(-earthPos));
		myMoon->computePosition(JD.second);
		myMoon->computeTransMatrix(JD.first, JD.second);
		Pos1 = myMoon->getHeliocentricEclipticPos();
		Pos2 = core->j2000ToEquinoxEqu(LocTrans*Pos1, StelCore::RefractionOff); //-RotObserver;

		distance = std::sqrt(Pos2*Pos2);

//		toRADec(Pos2,RAMoon,DecMoon);
	};
}
//////////////////////////////////////////////

//////////////////////////////////////////////
// Get the Coords of a planet:
void Observability::getPlanetCoords(StelCore *core, QPair<double, double> JD, double &RA, double &Dec, bool getBack)
{
	if (getBack)
	{
	// Return the planet to its current time:
		myPlanet->computePosition(JD.second);
		myPlanet->computeTransMatrix(JD.first, JD.second);
		myEarth->computePosition(JD.second);
		myEarth->computeTransMatrix(JD.first, JD.second);
	} else
	{
	// Compute planet's position:
		myPlanet->computePosition(JD.second);
		myPlanet->computeTransMatrix(JD.first, JD.second);
		Pos1 = myPlanet->getHeliocentricEclipticPos();
		myEarth->computePosition(JD.second);
		myEarth->computeTransMatrix(JD.first, JD.second);
		Pos2 = myEarth->getHeliocentricEclipticPos();
		LocTrans = (StelCore::matVsop87ToJ2000)*(Mat4d::translation(-Pos2));
		Pos2 = core->j2000ToEquinoxEqu(LocTrans*Pos1, StelCore::RefractionOff);
		toRADec(Pos2,RA,Dec);
	};
}
//////////////////////////////////////////////

//////////////////////////////////////////////
// Solves Moon's, Sun's, or Planet's ephemeris by bissection.
bool Observability::calculateSolarSystemEvents(StelCore* core, int bodyType)
{
	const int NUM_ITER = 100;
	int i;
	double hHoriz, ra, dec, raSun = 0.0, decSun = 0.0, tempH, /* tempJd, */ tempEphH, curSidT, eclLon;
	QPair<double, double> tempJd;
	//Vec3d Observer;

	hHoriz = calculateHourAngle(mylat, refractedHorizonAlt, selDec);
	bool raises = hHoriz > 0.0;


// Only recompute ephemeris from second to second (at least)
// or if the source has changed (i.e., Sun <-> Moon). This saves resources:
	if (qAbs(myJD.first-lastJDMoon)>StelCore::JD_SECOND || lastType!=bodyType || souChanged)
	{
		lastType = bodyType;

		myEarth->computePosition(myJD.second);
		myEarth->computeTransMatrix(myJD.first, myJD.second);
		Vec3d earthPos = myEarth->getHeliocentricEclipticPos();

		if (bodyType == 1) // Sun position
		{
			Pos2 = core->j2000ToEquinoxEqu((StelCore::matVsop87ToJ2000)*(-earthPos), StelCore::RefractionOff);
		}
		else if (bodyType==2) // Moon position
		{
			curSidT = myEarth->getSiderealTime(myJD.first, myJD.second)/Rad2Deg;
			RotObserver = (Mat4d::zrotation(curSidT))*ObserverLoc;
			LocTrans = (StelCore::matVsop87ToJ2000)*(Mat4d::translation(-earthPos));
			myMoon->computePosition(myJD.second);
			myMoon->computeTransMatrix(myJD.first, myJD.second);
			Pos1 = myMoon->getHeliocentricEclipticPos();
			Pos2 = (core->j2000ToEquinoxEqu(LocTrans*Pos1, StelCore::RefractionOff))-RotObserver;
		}
		else // Planet position
		{
			myPlanet->computePosition(myJD.second);
			myPlanet->computeTransMatrix(myJD.first, myJD.second);
			Pos1 = myPlanet->getHeliocentricEclipticPos();
			LocTrans = (StelCore::matVsop87ToJ2000)*(Mat4d::translation(-earthPos));
			Pos2 = core->j2000ToEquinoxEqu(LocTrans*Pos1, StelCore::RefractionOff);
		};

		toRADec(Pos2,ra,dec);
		Vec3d moonAltAz = core->equinoxEquToAltAz(Pos2, StelCore::RefractionOff);
		hasRisen = moonAltAz[2] > refractedHorizonAlt;

// Initial guesses of rise/set/transit times.
// They are called 'Moon', but are also used for the Sun or planet:

		double Hcurr = -calculateHourAngle(mylat,alti,selDec)*sign(LocPos[1]);
		double SidT = toUnsignedRA(selRA + Hcurr);

		MoonCulm = -Hcurr; 
		MoonRise = (-hHoriz-Hcurr);
		MoonSet = (hHoriz-Hcurr);

		if (raises)
		{
			if (!hasRisen)
			{
				MoonRise += (MoonRise<0.0)?24.0:0.0;
				MoonSet -= (MoonSet>0.0)?24.0:0.0;
			}

// Rise time:
			tempEphH = MoonRise*TFrac;
			MoonRise = myJD.first + (MoonRise/24.);
			for (i=0; i<NUM_ITER; i++)
			{
	// Get modified coordinates:
				tempJd.first = MoonRise;
				tempJd.second=tempJd.first+core->computeDeltaT(tempJd.first)/86400.0;
	
				if (bodyType<3)
				{
					getSunMoonCoords(core, tempJd,
					                 raSun, decSun,
					                 ra, dec,
					                 eclLon, false);
				} else
				{
					getPlanetCoords(core, tempJd, ra, dec, false);
				};

				if (bodyType==1) {ra = raSun; dec = decSun;};

	// Current hour angle at mod. coordinates:
				Hcurr = toUnsignedRA(SidT-ra);
				Hcurr -= (hasRisen)?0.0:24.;
				Hcurr -= (Hcurr>12.)?24.0:0.0;

	// H at horizon for mod. coordinates:
				hHoriz = calculateHourAngle(mylat,refractedHorizonAlt,dec);
	// Compute eph. times for mod. coordinates:
				tempH = (-hHoriz-Hcurr)*TFrac;
				if (hasRisen==false) tempH += (tempH<0.0)?24.0:0.0;
			// Check convergence:
				if (qAbs(tempH-tempEphH)<StelCore::JD_SECOND) break;
			// Update rise-time estimate:
				tempEphH = tempH;
				MoonRise = myJD.first + (tempEphH/24.);
			};
// Set time:  
			tempEphH = MoonSet;
			MoonSet = myJD.first + (MoonSet/24.);
			for (i=0; i<NUM_ITER; i++)
			{
	// Get modified coordinates:
				tempJd.first = MoonSet;
				tempJd.second=tempJd.first+core->computeDeltaT(tempJd.first)/86400.0;

				
				if (bodyType < 3)
					getSunMoonCoords(core, tempJd,
					                 raSun, decSun,
					                 ra, dec,
					                 eclLon, false);
				else
					getPlanetCoords(core, tempJd, ra, dec, false);
				
				if (bodyType==1) {ra = raSun; dec = decSun;};
				
	// Current hour angle at mod. coordinates:
				Hcurr = toUnsignedRA(SidT-ra);
				Hcurr -= (hasRisen)?24.:0.;
				Hcurr += (Hcurr<-12.)?24.0:0.0;
	// H at horizon for mod. coordinates:
				hHoriz = calculateHourAngle(mylat, refractedHorizonAlt, dec);
	// Compute eph. times for mod. coordinates:
				tempH = (hHoriz-Hcurr)*TFrac;
				if (!hasRisen)
					tempH -= (tempH>0.0)?24.0:0.0;
		// Check convergence:
				if (qAbs(tempH-tempEphH)<StelCore::JD_SECOND)
					break;
		// Update set-time estimate:
				tempEphH = tempH;
				MoonSet = myJD.first + (tempEphH/24.);
			};
		} 
		else // Comes from if(raises)
		{
			MoonSet = -1.0;
			MoonRise = -1.0;
		};

// Culmination time:
		tempEphH = MoonCulm;
		MoonCulm = myJD.first + (MoonCulm/24.);

		for (i=0; i<NUM_ITER; i++)
		{
			// Get modified coordinates:
			tempJd.first = MoonCulm;
			tempJd.second=tempJd.first+core->computeDeltaT(tempJd.first)/86400.0;


			if (bodyType<3)
			{
				getSunMoonCoords(core,tempJd,raSun,decSun,ra,dec,eclLon,false);
			} else
			{
				getPlanetCoords(core,tempJd,ra,dec,false);
			};


			if (bodyType==1) {ra = raSun; dec = decSun;};


	// Current hour angle at mod. coordinates:
			Hcurr = toUnsignedRA(SidT-ra);
			Hcurr += (LocPos[1]<0.0)?24.0:-24.0;
			Hcurr -= (Hcurr>12.)?24.0:0.0;

	// Compute eph. times for mod. coordinates:
			tempH = -Hcurr*TFrac;
	// Check convergence:
			if (qAbs(tempH-tempEphH)<StelCore::JD_SECOND) break;
			tempEphH = tempH;
			MoonCulm = myJD.first + (tempEphH/24.);
			culmAlt = qAbs(mylat-dec); // 90 - altitude at transit.
		};
		lastJDMoon = myJD.first;
	}; // Comes from if (qAbs(myJD.first-lastJDMoon)>JDsec || LastObject!=Kind)

// Find out the days of Full Moon:
	if (bodyType==2 && show_FullMoon) // || show_SuperMoon))
	{
	// Only estimate date of Full Moon if we have changed Lunar month:
		if (myJD.first > nextFullMoon || myJD.first < prevFullMoon)
		{
	// Estimate the nearest (in time) Full Moon:
			double nT;
			double dT = std::modf((myJD.first-RefFullMoon)/MoonT,&nT);
			if (dT>0.5) {nT += 1.0;};
			if (dT<-0.5) {nT -= 1.0;};

			double TempFullMoon = RefFullMoon + nT*MoonT;

	// Improve the estimate iteratively (Secant method over Lunar-phase vs. time):

			dT = 0.1/1440.; // 6 seconds. Our time span for the finite-difference derivative estimate.
//			double Deriv1, Deriv2; // Variables for temporal use.
			QPair<double, double>Sec1, Sec2; // Variables for temporal use. Contrary to the other cases, we have Sec[12].second=DeltaT[days], not JDE. This avoids needless re-computing.
			double Temp1, Temp2; // Variables for temporal use.
			double iniEst1, iniEst2;  // JD values that MUST include the solution within them.
			double Phase1;

			for (int j=0; j<2; j++) 
			{
				// Two steps: one for the previos Full Moon and the other for the next one.
				iniEst1 =  TempFullMoon - 0.25*MoonT; 
				iniEst2 =  TempFullMoon + 0.25*MoonT; 

				Sec1.first = iniEst1; // TempFullMoon - 0.05*MoonT; // Initial estimates of Full-Moon dates
				Sec1.second= core->computeDeltaT(Sec1.first)/86400.0; // enough to compute this once.
				Sec2.first = iniEst2; // TempFullMoon + 0.05*MoonT;
				Sec2.second= core->computeDeltaT(Sec2.first)/86400.0; // enough to compute this once.

				// for the computation calls, we need temporary QPairs here!
				getSunMoonCoords(core,QPair<double, double>(Sec1.first, Sec1.first+Sec1.second),raSun,decSun,ra,dec,eclLon,false);
				Temp1 = eclLon; //Lambda(RA,Dec,RAS,DecS);
				getSunMoonCoords(core,QPair<double, double>(Sec2.first, Sec2.first+Sec2.second),raSun,decSun,ra,dec,eclLon,false);
				Temp2 = eclLon; //Lambda(RA,Dec,RAS,DecS);

				for (int i=0; i<100; i++) // A limit of 100 iterations.
				{
					Phase1 = (Sec2.first-Sec1.first)/(Temp1-Temp2)*Temp1+Sec1.first;
					// The ad-hoc pair needs a DeltaT, use the one of Sec1
					getSunMoonCoords(core,QPair<double, double>(Phase1, Phase1+Sec1.second),raSun,decSun,ra,dec,eclLon,false);
					
					if (Temp1*eclLon < 0.0) 
					{
						Sec2.first = Phase1;
						Temp2 = eclLon;
					} else {
						Sec1.first = Phase1;
						Temp1 = eclLon;
					};

					if (qAbs(Sec2.first-Sec1.first) < 10.*dT)  // 1 minute accuracy; convergence.
					{
						TempFullMoon = (Sec1.first+Sec2.first)/2.;
						break;
					};
				};

				if (TempFullMoon > myJD.first)
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
			StelUtils::getDateFromJulianDay(intMoon, &fullYear, &fullMonth, &fullDay);
			double2hms(toUnsignedRA(LocalTMoon),fullHour,fullMinute,fullSecond);
			if (getDateFormat())
				lineBestNight = msgPrevFullMoon.arg(fullDay).arg(StelLocaleMgr::shortMonthName(fullMonth)).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));
			else
				lineBestNight = msgPrevFullMoon.arg(StelLocaleMgr::shortMonthName(fullMonth)).arg(fullDay).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));

			LocalTMoon = 24.*modf(LocalNext,&intMoon);
			StelUtils::getDateFromJulianDay(intMoon,&fullYear,&fullMonth,&fullDay);
			double2hms(toUnsignedRA(LocalTMoon),fullHour,fullMinute,fullSecond);			
			if (getDateFormat())
				lineBestNight += msgNextFullMoon.arg(fullDay).arg(StelLocaleMgr::shortMonthName(fullMonth)).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));
			else
				lineBestNight += msgNextFullMoon.arg(StelLocaleMgr::shortMonthName(fullMonth)).arg(fullDay).arg(fullHour).arg(fullMinute,2,10,QLatin1Char('0'));

			lineObservableRange.clear(); 
			lineAcroCos.clear();

	// Now, compute the days of all the Full Moons of the current year, and get the Earth/Moon distance:
//			double monthFrac, monthTemp, maxMoonDate;
//			monthFrac = std::modf((nextFullMoon-Jan1stJD)/MoonT,&monthTemp);
//			int PrevMonths = static_cast<int>(monthTemp+0.0*monthFrac);
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
	else if (bodyType <3)
	{
		lineBestNight.clear();
		lineObservableRange.clear(); 
		lineAcroCos.clear();
	}; 

// Return the Moon and Earth to its current position:
	if (bodyType<3)
	{
		getSunMoonCoords(core, myJD, raSun, decSun, ra, dec, eclLon, true);
	}
	else
	{
		getPlanetCoords(core, myJD, ra, dec, true);
	};

	return raises;
}

//////////////////////////////////
///  STUFF FOR THE GUI CONFIG

bool Observability::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

void Observability::resetConfiguration()
{
	// Remove the plug-in's group from the configuration,
	// after that it will be loaded with default values.
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->remove("Observability");
	loadConfiguration();
}

void Observability::loadConfiguration()
{
	QSettings* conf = StelApp::getInstance().getSettings();

	conf->beginGroup("Observability");

	// Load settings from main config file
	fontSize = conf->value("font_size",15).toInt();
	font.setPixelSize(fontSize);
	fontColor = Vec3f(conf->value("font_color", "0,0.5,1").toString());
	show_AcroCos = conf->value("show_AcroCos", true).toBool();
	show_Good_Nights = conf->value("show_Good_Nights", true).toBool();
	show_Best_Night = conf->value("show_Best_Night", true).toBool();
	show_Today = conf->value("show_Today", true).toBool();
	show_FullMoon = conf->value("show_FullMoon", true).toBool();
//	show_Crescent = conf->value("show_Crescent", true).toBool();
//	show_SuperMoon = conf->value("show_SuperMoon", true).toBool();

	// For backwards compatibility, the value of this key is stored with
	// inverted sign.
	// TODO: Skip the sign inversion when the key is changed.
	int altitude = -(conf->value("Sun_Altitude", 12).toInt());
	setTwilightAltitude(altitude); 

	altitude = conf->value("Horizon_Altitude", 0).toInt();
	setHorizonAltitude(altitude);
	
	conf->endGroup();
	
	// Load date format from main settings.
	// TODO: Handle date format properly.
	if (conf->value("localization/date_display_format", "system_default").toString() == "ddmmyyyy")
		setDateFormat(true);
	else
		setDateFormat(false);
}

void Observability::saveConfiguration()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	QString fontColorStr = QString("%1,%2,%3").arg(fontColor[0],0,'f',2).arg(fontColor[1],0,'f',2).arg(fontColor[2],0,'f',2);
	// Set updated values
	conf->beginGroup("Observability");
	conf->setValue("font_size", fontSize);
	// For backwards compatibility, the value of this key is stored with
	// inverted sign.
	// TODO: Skip the sign inversion when the key is changed.
	conf->setValue("Sun_Altitude", -twilightAltDeg);
	conf->setValue("Horizon_Altitude", horizonAltDeg);
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

void Observability::enableTodayField(bool enabled)
{
	show_Today = enabled;
	configChanged = true;
}

void Observability::enableAcroCosField(bool enabled)
{
	show_AcroCos = enabled;
	configChanged = true;
}

void Observability::enableGoodNightsField(bool enabled)
{
	show_Good_Nights = enabled;
	configChanged = true;
}

void Observability::enableOppositionField(bool enabled)
{
	show_Best_Night = enabled;
	configChanged = true;
}

void Observability::enableFullMoonField(bool enabled)
{
	show_FullMoon = enabled;
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

int Observability::getTwilightAltitude()
{
	return twilightAltDeg;
}

int Observability::getHorizonAltitude()
{
	return horizonAltDeg;
}


void Observability::setFontColor(const Vec3f& color)
{
	fontColor = color; // Vector3::operator =() is overloaded. :)
}

void Observability::setFontSize(int size)
{
	fontSize = size;
}

void Observability::setTwilightAltitude(int altitude)
{
	twilightAltRad  = static_cast<double>(altitude)/Rad2Deg ;
	twilightAltDeg = altitude;
	configChanged = true;
}

void Observability::setHorizonAltitude(int altitude)
{
	horizonAltitude = static_cast<double>(altitude)/Rad2Deg ;
	horizonAltDeg = altitude;
	configChanged = true;
}


void Observability::showReport(bool b)
{
	if (b!=flagShowReport)
	{
		flagShowReport = b;
		emit flagReportVisibilityChanged(b);
	}
}

