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
	isStar = true;

// Compute Sun coordinates through the year (max. error of +/-4 minutes, due to leap years):
	for (int i=0;i<365;i++) {
	 double fday = (double) i;
	 double RASun0 = 24./365.25*(fday-80.25);
	 double RASunTEq = -0.128*std::sin(0.2618*RASun0+1.307)+0.165*std::sin(0.5236+RASun0-0.08);
	 SunRA[i] = RASun0 + RASunTEq; // With simple model of Time Equation.
	 SunRA[i] += (SunRA[i]<0.0)?24.0:0.0;  // In hours.
	 SunDec[i] = 0.40893*std::sin(0.2618*SunRA[i]);  // In radians.
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
	bool isMoon, isSun, souChanged, locChanged;
	QString objnam;

	StelPainter painter(core->getProjection2d());
	painter.setColor(1,1,1,1);
	painter.setFont(font);

// Only execute plugin if we are on Earth.
	if (core->getCurrentLocation().planetName != "Earth") {return;};

// Get current location and local star's coordinates.
	double currlat = (core->getCurrentLocation().latitude)/Rad2Deg;
	Vec3d LocPos;

// Do we have changed our latitude?
	if (currlat == mylat) {
		locChanged = false;}
	else {
		locChanged = true;
// If so, update the vector of Sun's hour angle at twilight:
		mylat = currlat;
		for (int i=0;i<365;i++) {
			SunHTwi[i] = HourAngle(mylat,AstroTwiAlti,SunDec[i]);};
	}


// Get the selected object (if there is something selected):

	if (StelApp::getInstance().getStelObjectMgr().getWasSelected()) {

// Get the selected source and its name:
		StelObjectP selectedObject = StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]; 
		QString tempName = selectedObject->getEnglishName();

//		Check if the source belongs to the Solar System:
		Planet* pl = dynamic_cast<Planet*>(selectedObject.data());
		isStar = (pl)?false:true;

// Check if the source is Sun or Moon (i.e., it changes quite a bit during one day):
		isMoon = ("Moon" == tempName)?true:false;
		isSun = ("Sun" == tempName)?true:false;

// Check if the user has changed the source (or if the source is Sun/Moon). If so, update RA/Dec and check 
// if the (new) source belongs to the Solar System:
		if (tempName == selName && isMoon==false && isSun == false) {
			souChanged = false;} // Don't touch anything regarding RA/Dec (to save a bit of resources).
		else {
			selName = tempName; souChanged = true; // update current source (and re-compute RA/Dec)

//		Update RA/Dec
			EquPos = selectedObject->getEquinoxEquatorialPos(core);
			EquPos.normalize();
		};
		LocPos = core->equinoxEquToAltAz(EquPos);
	}
	else { // There is no source selected!

// If no source is selected, get the position vector of the screen center:
		selName = "";
		Vec3d currentPos = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
		EquPos = core->j2000ToEquinoxEqu(currentPos);
		LocPos = core->j2000ToAltAz(currentPos);}

// Convert EquPos to RA/Dec (only if it is necessary):
	if (souChanged) {
		selDec = std::asin(EquPos[2]);
		selRA = std::atan2(EquPos[1],EquPos[0])*Rad2Hr;
		toUnsignedRA(selRA);};

// Compute source's altitude (in radians):
	alti = std::asin(LocPos[2]);


// Compute rising/setting times from current time:
	double currH = HourAngle(mylat,alti,selDec);
	horizH = HourAngle(mylat,0.0,selDec);
	QString RS1, RS2, RS, Cul; // strings with Rise/Set/Culmination times 
	double Rise, Set;
	int d1,m1,s1,d2,m2,s2,dc,mc,sc;
	bool raised;

	if (isMoon==false) {  // Just a temporal if statement. Will implement the Moon soon.
	if (horizH>0.0) {

		if ( LocPos[1]>0.0 ) {
			if ( currH>horizH ) {
				Set = 24.-currH-horizH;
				Rise = currH-horizH;
				raised = false;}
			else {
				Rise = horizH-currH;
				Set = 2.*horizH-Rise;
				raised = true;}
		} 
		else {
			if ( currH>horizH ) {
				Set = currH-horizH;
				Rise = 24.-currH-horizH;
				raised = false;}
			else {
				Rise = horizH+currH;
				Set = horizH-currH;
				raised = true;}
		};

		double2hms(Set,d1,m1,s1);
		double2hms(Rise,d2,m2,s2);

		RS1 = (d1==0)?"":q_("%1h ").arg(d1); RS1 += (m1==0)?"":q_("%1m ").arg(m1); RS1 += q_("%1s ").arg(s1);
		RS2 = (d2==0)?"":q_("%1h ").arg(d2); RS2 += (m2==0)?"":q_("%1m ").arg(m2); RS2 += q_("%1s").arg(s2);

		if (raised) {RS = q_("Raised ")+RS2+q_(" ago. Will set in ")+RS1;} else {RS = q_("Has set ")+RS1+q_(" ago. Will raise in ")+RS2;};		
	}
	else {
		(alti>0.0)? RS = q_("Never sets."): RS = q_("Never rises.");
	}
	}

// Text for culmination:
	double culmAlt = std::abs(mylat-selDec);
	if (culmAlt<1.57079632675 and isMoon==false) {
		double altiAtCulmi = Rad2Deg*(1.57079632675-culmAlt);
		double2hms(currH,dc,mc,sc);	
		Cul = (dc==0)?"":q_("%1h ").arg(dc); Cul += (mc==0)?"":q_("%1m ").arg(mc); Cul += q_("%1s").arg(sc); 
		if (LocPos[1]>0.0 && culmAlt<90.) { 
			Cul = q_("Will culminate in ")+Cul+q_(" (at %1 deg.)").arg(altiAtCulmi,0,'f',1);}
		else {
			Cul = q_("Culminated ")+Cul+q_(" ago (at %1 deg.)").arg(altiAtCulmi,0,'f',1);
		};
	}

// Determine source observability (at the moment, this code is just for stars and deep sky):

	if (isStar==false) {
		bestNight="Object belongs to the Solar System";ObsRange="";}

	else if (souChanged || locChanged) {
		bestNight=""; ObsRange = "";

		if (culmAlt<1.57079632675) {

// Determine best observing night (opposition to the Sun):
			double BestSunRA = 12.+selRA; toUnsignedRA(BestSunRA);
			int selday = 0;
			double deltaRA = 12.0;
			double tempRA;

			for (int i=0;i<365;i++) {
				tempRA = std::abs(SunRA[i]-BestSunRA);
				if (tempRA<deltaRA) {selday=i+1;deltaRA=tempRA;};
			};

			bestNight = q_("Best night: ") + CalenDate(selday);

// Determine range of good nights (i.e., above horizon before/after astronomical twilight):
			if (horizH>0.0) {

//			First day: Rising twilight just when the source rises.
//			Last day: Setting twilight just when the source sets.
				double SidTime = selRA-horizH; toUnsignedRA(SidTime);
				double SidTime2 = selRA+horizH; toUnsignedRA(SidTime2);

				int selday = 0;
				int selday2 = 0;
				double deltaH = 12.0;
				double deltaH2 = 12.0;
				double tempH, tempH2;

				for (int i=0;i<365;i++) {
					tempH = SunRA[i]-SunHTwi[i]; toUnsignedRA(tempH);
					tempH = std::abs(SidTime - tempH); toUnsignedRA(tempH);
					tempH2 = SunRA[i]+SunHTwi[i]; toUnsignedRA(tempH2);
					tempH2 = std::abs(SidTime2 - tempH2); toUnsignedRA(tempH2);
					if (tempH<deltaH) {selday=i+1;deltaH=tempH;};
					if (tempH2<deltaH2) {selday2=i+1;deltaH2=tempH2;};
				};
				
//			Check if source rises before the twilight when it sets after the twilight. In such a case,
//			the source is observable during the whole year.
				double HStarTwi = std::abs(SunRA[selday2]-SunHTwi[selday2]-selRA)-horizH;
				if (HStarTwi<0.0 || HStarTwi>12.0) {
					ObsRange = q_("Observable during the whole year");}
				else {
					ObsRange=q_("Observable between (approx.) ")+CalenDate(selday)+q_(" and ")+CalenDate(selday2);
					if (selday2<selday) {ObsRange += q_(" (of next year)");};
				};}
			else {  // The source is circumpolar, so it is observable during the whole year.
				ObsRange = q_("Observable during the whole year");
			};

		};


	};

// Print all results:
	painter.drawText(30, 60, ObsRange);
	painter.drawText(30, 80, bestNight);
	painter.drawText(30, 100, Cul);
	painter.drawText(30, 120, RS);

}

// END OF MAIN CODE
///////////////////////////


//////////////////////////////
// AUXILIARY FUNCTIONS

////////////////////////////////////
// Returns the hour angle for a given altitude:
double Observability::HourAngle(double lat, double h, double Dec)
{
//	float latrad = lat/Rad2Deg; 
//	float hrad = h/Rad2Deg;
//	float Decrad = Dec/Rad2Deg;
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
void Observability::toUnsignedRA(double &RA)
{
	RA += (RA<0.0)?24.0:((RA>24.0)?-24.0:0.0);
}

///////////////////////////////////////////////
// Returns the day and month of year (to put it in format '25 Apr')
QString Observability::CalenDate(int day)
{
	bool nofound=true;
	int dom,mon;

	if (day == 0){
		return q_("None");}
	else {
		for (int m=0; m<12; m++){
			if (cumsum[m+1] >= day && nofound){
				mon = m;
				dom = day-cumsum[m];
				nofound=false;}
		};
		return q_("%1 "+months[mon]).arg(dom);
	};
}
//////////////////////////////////////////////
