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

#include <QString>
#include <QDebug>

#include "StelProjector.hpp"
#include "StarMgr.hpp"
#include "StelObject.hpp"
#include "StelObserver.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "ZoneArray.hpp"
#include "StelSkyDrawer.hpp"
#include "Observability.hpp"

#include "SolarSystem.hpp"
#include "Planet.hpp"

StelModule* ObservabilityStelPluginInterface::getStelModule() const
{
        return new Observability();
}

StelPluginInfo ObservabilityStelPluginInterface::getPluginInfo() const
{
        StelPluginInfo info;
        info.id = "Observability";
        info.displayedName = q_("Observability analysis");
        info.authors = "Ivan Marti-Vidal (Onsala Space Observatory)";
        info.contact = "i.martividal@gmail.com";
        info.description = q_("Determines source observability, based on source/observer coordinates and epoch of the year. It assumes that a source is observable if it is above horizon before/after the astronomical rise/set twilight (hence, and depending on the source magnitude, the real dates may differ by up to a couple of weeks). At the moment, it does not work for the Solar-System objects, and the rise/set/culmination times for the Moon are not given.");
        return info;
}

Q_EXPORT_PLUGIN2(Observability, ObservabilityStelPluginInterface)

Observability::Observability()
{
       setObjectName("Observability");
       font.setPixelSize(15);

	Rad2Deg = 180./3.1415927;
	Rad2Hr = 12./3.1415927;
	AstroTwiAlti = -12./Rad2Deg;
	selName = "";
	mylat = 1000.;
	myJD = 0.0;
	currYear = 0;
	isStar = true;
	isMoon = false;
	isSun = false;
	isScreen = true;

// Compute Sun coordinates through the year (max. error of +/-4 minutes, due to leap years):
	for (int i=0;i<365;i++) {
		SunRA[i] = 0.0; SunDec[i] = 0.0;
		ObjectRA[i] = 0.0; ObjectDec[i]=0.0;
		SunSidT[0][i]=0.0; SunSidT[1][i]=0.0;
		ObjectSidT[0][i]=0.0; ObjectSidT[1][i]=0.0;
		yearJD[i] = 0.0;
	};

// Day of year at the beginning of each month:
	int cums[13]={0,31,59,90,120,151,181,212,243,273,304,334,365};
	for (int i=0;i<13;i++) {
	 cumsum[i]=cums[i];
	};

	QString mons[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	for (int i=0;i<12;i++) {
	 months[i]=mons[i];
	};



}

Observability::~Observability()
{
}

double Observability::getCallOrder(StelModuleActionName actionName) const
{
       if (actionName==StelModule::ActionDraw)
               return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
       return 0;
}

void Observability::init()
{
       qDebug() << "init called for Observability";
}

/////////////////////////////////////////////
// MAIN CODE:
void Observability::draw(StelCore* core)
{

/////////////////////////////////////////////////////////////////
// PRELIMINARS:
	bool souChanged, locChanged, yearChanged, JDChanged;
	QString objnam;
	StelObjectP selectedObject;
	Planet* currPlanet;

// Only execute plugin if we are on Earth.
	if (core->getCurrentLocation().planetName != "Earth") {return;};

// Set the painter:
	StelPainter paintresult(core->getProjection2d());
	paintresult.setColor(1,1,1,1);
	paintresult.setFont(font);

// Get current date, location, and check if there is something selected.
	double currlat = (core->getCurrentLocation().latitude)/Rad2Deg;
	double currJD = core->getJDay();
	int auxm, auxd, auxy;
	StelUtils::getDateFromJulianDay(currJD,&auxy,&auxm,&auxd);
	bool isSource = StelApp::getInstance().getStelObjectMgr().getWasSelected();
//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// NOW WE CHECK THE CHANGED PARAMETERS W.R.T. THE PREVIOUS FRAME:

// Have we changed the Julian date?
	if (std::abs(currJD-myJD)>0.5) {
		JDChanged = true;
		myJD = currJD;}
	else {
		JDChanged = false;
	};

// If we have changed the year, we must recompute the Sun's position for each new day:
	if (auxy != currYear) {
		yearChanged = true;
		currYear = auxy;
		SunRADec(core);}
	else {
		yearChanged = false;
	};

// Have we changed the latitude?
	if (currlat == mylat) {
		locChanged = false;}
	else {
		locChanged = true;
		mylat = currlat;
	};

// If we have changed latitude (or year), we update the vector of Sun's hour 
// angles at twilight:
	if (locChanged || yearChanged) {SunHTwi();};

//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// NOW WE DEAL WITH THE SOURCE (OR SCREEN-CENTER) POSITION:
	Vec3d LocPos;
	if (isSource) { // There is something selected!

		if (isScreen) {souChanged=true;};

// Get the selected source and its name:
		selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]; 
		QString tempName = selectedObject->getEnglishName();

// Check if the source is Sun or Moon (i.e., it changes quite a bit during one day):
		isMoon = ("Moon" == tempName)?true:false;
		isSun = ("Sun" == tempName)?true:false;

// Check if the user has changed the source (or if the source is Sun/Moon). If so, update RA/Dec and check 
// if the (new) source belongs to the Solar System:
		if (tempName == selName && isMoon==false && isSun == false) {
			souChanged = false;} // Don't touch anything regarding RA/Dec (to save a bit of resources).
		else {
			selName = tempName; souChanged = true; // update current source (and re-compute RA/Dec)
			currPlanet = dynamic_cast<Planet*>(selectedObject.data());
			isStar = (currPlanet)?false:true;
//		Update RA/Dec
			EquPos = selectedObject->getEquinoxEquatorialPos(core);
			EquPos.normalize();
		};
		LocPos = core->equinoxEquToAltAz(EquPos);}
	else { // There is no source selected!

// If no source is selected, get the position vector of the screen center:
		selName = ""; isStar=true; isMoon = false; isSun = false; isScreen=true;
		Vec3d currentPos = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
		currentPos.normalize();
		EquPos = core->j2000ToEquinoxEqu(currentPos);
		LocPos = core->j2000ToAltAz(currentPos);
	}

// Convert EquPos to RA/Dec (only if it is necessary):
	if (souChanged) {
		selDec = std::asin(EquPos[2]);
		selRA = toUnsignedRA(std::atan2(EquPos[1],EquPos[0])*Rad2Hr);
	};

// Compute source's altitude (in radians):
	alti = std::asin(LocPos[2]);

/////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////
// NOW WE COMPUTE RISE/SET/TRANSIT TIMES FOR THE CURRENT DAY:
	double currH = HourAngle(mylat,alti,selDec);
	horizH = HourAngle(mylat,0.0,selDec);
	QString RS1, RS2, RS, Cul; // strings with Rise/Set/Culmination times 
	double Rise, Set; // Actual Rise/Set times
	int d1,m1,s1,d2,m2,s2,dc,mc,sc; // Integers for the time spans in hh:mm:ss.
	bool raised; // Is the source above the horizon?
	double culmAlt = std::abs(mylat-selDec); // 90.-altitude at transit.


	if (isMoon==false) {  // Just a temporal if statement. Will implement the Moon soon.

	if (horizH>0.0) { // The source is not circumpolar and can be seen from this latitude.

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

		double2hms(Set,d1,m1,s1);
		double2hms(Rise,d2,m2,s2);

//		Strings with time spans for rise/set/transit:
		RS1 = (d1==0)?"":q_("%1h ").arg(d1); RS1 += (m1==0)?"":q_("%1m ").arg(m1); RS1 += q_("%1s ").arg(s1);
		RS2 = (d2==0)?"":q_("%1h ").arg(d2); RS2 += (m2==0)?"":q_("%1m ").arg(m2); RS2 += q_("%1s").arg(s2);
		if (raised) 
		{
			RS = q_("Raised ")+RS2+q_(" ago. Will set in ")+RS1;} else {RS = q_("Has set ")+RS1+q_(" ago. Will raise in ")+RS2;};		
		}

	else { // The source is either circumpolar or never rises:
		(alti>0.0)? RS = q_("Never sets."): RS = q_("Never rises.");
	};

// 	Culmination:

	if (culmAlt<1.57079632675) { // Source can be observed.
		double altiAtCulmi = Rad2Deg*(1.57079632675-culmAlt);
		double2hms(currH,dc,mc,sc);

//		String with the time span for culmination:	
		Cul = (dc==0)?"":q_("%1h ").arg(dc); Cul += (mc==0)?"":q_("%1m ").arg(mc); Cul += q_("%1s").arg(sc); 
		if (LocPos[1]>0.0 && culmAlt<90.) { 
			Cul = q_("Will culminate in ")+Cul+q_(" (at %1 deg.)").arg(altiAtCulmi,0,'f',1);}
		else {
			Cul = q_("Culminated ")+Cul+q_(" ago (at %1 deg.)").arg(altiAtCulmi,0,'f',1);
		};
	};

	}; // This comes from the isMoon==false statement.
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// NOW WE ANALYZE THE SOURCE OBSERVABILITY FOR THE WHOLE YEAR:

// Compute yearly ephemeris (only if necessary):

	if (isMoon==true || isSun == true) {
		bestNight=""; ObsRange = "";}
	else {
		if (isStar==false && (souChanged || yearChanged)) { // Object moves.
			PlanetRADec(core,selName);}

		else { // Object is fixed on the sky.
			double auxH = HourAngle(mylat,0.0,selDec);
			double auxSidT1 = toUnsignedRA(selRA - auxH); 
			double auxSidT2 = toUnsignedRA(selRA + auxH); 
			for (int i=0;i<365;i++) {
				ObjectRA[i] = selRA;
				ObjectDec[i] = selDec;
				ObjectSidT[0][i] = auxSidT1;
				ObjectSidT[1][i] = auxSidT2;
			};
		};

// Determine source observability (only if something changed):
		if ((souChanged || locChanged || yearChanged)) {
			bestNight=""; ObsRange = "";}

			if (culmAlt>=1.57079632675) { // Source cannot be seen.
				ObsRange = "Source is not observable.";}

			else { // Source can be seen.

///////////////////////////
// - Part 1. Determine the best observing night (opposition to the Sun):
				int selday = 0;
				double deltaPhs = 24.0;
				double tempPhs;
	
				for (int i=0;i<365;i++) {
					tempPhs = toUnsignedRA(std::abs(ObjectSidT[0][i]-SunSidT[0][i]+12.));
					if (tempPhs<deltaPhs) {selday=i;deltaPhs=tempPhs;};
				};

				bestNight = q_("Best night: ") + CalenDate(selday);

////////////////////////////
// - Part 2. Determine range of good nights 
// (i.e., above horizon before/after astronomical twilight):

				if (horizH>0.0) { // Source is not circumpolar.
	
					int selday = 0; // Sunrise twilight before object rises.
					int selday2 = 0; // Sunset twilight before object sets. 
					bool bestBegun = false; // Are we inside a good time range?
					QString dateRange = "";

					for (int i=0;i<365;i++) {
						if (SunSidT[0][i]<ObjectSidT[0][i] && bestBegun == false) {
							selday = i;
							bestBegun = true;
						};

						if (SunSidT[1][i]<ObjectSidT[1][i] && bestBegun == true) {
							selday2 = i;
							bestBegun = false;
							if (selday2 > selday) {
								if (dateRange!="") { dateRange += " and ";};
								dateRange += " from "+CalenDate(selday)+" to "+CalenDate(selday2);
							};
						};
					};
					
					if (dateRange == "") { // All year is good.
						ObsRange = "Observable during the whole year.";}
					else {
						ObsRange = "Good observations"+dateRange;
					};
				}; // From :horizH>0.0"
			}; // From the "else" of "culmAlt>=..." 
		};// From  "souChanged || ..."
//	}; // From the "else" of "isMoon==true && ..."

// Print all results:

	paintresult.drawText(30, 60, ObsRange);
	paintresult.drawText(30, 80, bestNight);
	paintresult.drawText(30, 100, Cul);
	paintresult.drawText(30, 120, RS);

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
		{return -1.0;} // Source doesn't reach that altitude. 
	else 
	{return Rad2Hr*std::acos(Numer/Denom);}

}
////////////////////////////////////

////////////////////////////////////
// Converts a float time/angle span (in hours/degrees) in the (integer) format hh/dd,mm,ss:
void Observability::double2hms(double hfloat, int &h1, int &h2, int &h3)
{
	double f1,f2,f3;

	double ffrac = std::modf(hfloat,&f1);
	double ffrac2 = std::modf(60.*ffrac,&f2);
	ffrac2 = std::modf(3600.*(ffrac-f2/60.),&f3);
	h1 = (int)f1 ; h2 = (int)std::abs(f2) ; h3 = (int)std::abs(f3) ;
} 
////////////////////////////////////

////////////////////////////////////
// Adds/subtracts 24hr to ensure a RA between 0 and 24hr:
double Observability::toUnsignedRA(double RA)
{
	double tempRA;
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
	return q_("%1 "+months[month-1]).arg(day);
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
	FILE *filePtr;
	filePtr = fopen("/home/marti/floatArray","w");
	fprintf(filePtr, "%.7g\t%.7g\n", yearJD[0], yearJD[364]);

	for (int i=0;i<365;i++) {
		myPlanet->computePosition(yearJD[i]);
		myPlanet->computeTransMatrix(yearJD[i]);
		TP = myPlanet->getHeliocentricEclipticPos() + EarthPos[i];
		LocTrans = (core->matVsop87ToJ2000)*(Mat4d::translation(EarthPos[i]));
		TP2 = core->j2000ToEquinoxEqu(LocTrans*TP);
		TP2.normalize();
		ObjectDec[i] = std::asin(TP2[2]);
		ObjectRA[i] = toUnsignedRA(std::atan2(TP2[1],TP2[0])*Rad2Hr);
		TempH = HourAngle(mylat,0.0,ObjectDec[i]);
		ObjectSidT[0][i] = toUnsignedRA(ObjectRA[i]-TempH);
		ObjectSidT[1][i] = toUnsignedRA(ObjectRA[i]+TempH);
      		fprintf(filePtr, "%.5g\t%.5g\t%.5g\n", ObjectRA[i], ObjectDec[i]*Rad2Deg,ObjectSidT[0][i]);
	}

	fclose(filePtr);

// Return the planet to its current time:
	myPlanet->computePosition(myJD);
	myPlanet->computeTransMatrix(myJD);


}

/////////////////////////////////////////////////
// Computes the Sun's RA and Dec (and the JD) for 
// each day of the current year.
void Observability::SunRADec(StelCore* core) 
{
	int day,month,year,DayOfYear;
	int day2,month2;
	Vec3d TP, TP2; 

	StelUtils::getDateFromJulianDay(myJD,&year,&month,&day);
	int tspan = day+cumsum[month-1]-3;

	StelUtils::getDateFromJulianDay(myJD - (double)tspan, &year,&month2,&day2);
	DayOfYear = cumsum[month-1]+day2+day-3;  // Deals well with leap years.

// This is the JD of the date 1 Jan in the current year:
	double Jan1stJD = myJD - (double)DayOfYear+1.0;

// Get pointer to the Earth:
	PlanetP Earth = GETSTELMODULE(SolarSystem)->getEarth();

// Get pointer to Planet's instance for the Earth:
	Planet* myEarth = Earth.data();

	FILE *filePtr;
	filePtr = fopen("/home/marti/SunArray","w");

// Compute Earth's position throughout the year:
	for (int i=0;i<365;i++) {
		yearJD[i] = Jan1stJD+(double)i;
		myEarth->computePosition(yearJD[i]);
		myEarth->computeTransMatrix(yearJD[i]);
		TP = myEarth->getHeliocentricEclipticPos();
		TP[0] = -TP[0]; TP[1] = -TP[1]; TP[2] = -TP[2];
		EarthPos[i] = TP;
		TP2 = core->j2000ToEquinoxEqu((core->matVsop87ToJ2000)*TP);
		TP2.normalize();
		SunDec[i] = std::asin(TP2[2]);
		SunRA[i] = toUnsignedRA(std::atan2(TP2[1],TP2[0])*Rad2Hr);
     		fprintf(filePtr, "%.5g\t%.5g\t%.7g\n", SunDec[i]*Rad2Deg,SunRA[i],yearJD[i]);
	};
	fclose(filePtr);

//Return the Earth to its current time:
	myEarth->computePosition(myJD);
	myEarth->computeTransMatrix(myJD);
}
///////////////////////////////////////////////////


////////////////////////////////////////////
// Computes Sun's Sidereal Times at twilight and culmination:
void Observability::SunHTwi()
{
	double TempH;

	for (int i=0; i<365; i++) {
		TempH = HourAngle(mylat,AstroTwiAlti,SunDec[i]);
		SunSidT[0][i] = toUnsignedRA(SunRA[i]-TempH);
		SunSidT[1][i] = toUnsignedRA(SunRA[i]+TempH);
	};
}
////////////////////////////////////////////






//////////////////////////////////////////////
// Solves Moon's ephemeris time by bisection:
//double MoonBissect(double jd0, double jd1, int idx)
//{
//	double zTemp0, zTemp1, jdm;
//	for (int i=0,i<1000,i++) 
//	{
//		zTemp0 = Moon.computePosition(jd0)[idx];
//		zTemp1 = Moon.computePosition(jd1)[idx];
//		jdm = zTemp0*(jd1-jd0)/(zTemp1-zTemp0)+jd0;
//		zTempm = Moon.computePosition(jdm)[idx];
//		if (zTempm*zTemp0 < 0.0) {
//			zTemp1 = zTempm; jd1 = jdm;}
//		else {
//			zTemp0 = zTempm; jd0 = jdm;};
//		if (std::fabs(jd1-jd0) < 1./86400.) {i=1000};
//	}
//	return jdm;
//}


