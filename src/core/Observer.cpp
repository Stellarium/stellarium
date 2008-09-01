/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#include "Observer.hpp"
#include "StelUtils.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "Translator.hpp"
  // unselecting selected planet after setHomePlanet:
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelCore.hpp" // getNavigator
#include "Navigator.hpp" // getJDay
  // setting the titlebar text:
#include "PlanetLocationMgr.hpp"
#include <cassert>
#include <QDebug>
#include <QSettings>
#include <QStringList>

class ArtificialPlanet : public Planet {
public:
  ArtificialPlanet(const Planet &orig);
  void setDest(const Planet &dest);
  void computeAverage(double f1);
private:
  void setRot(const Vec3d &r);
  static Vec3d GetRot(const Planet *p);
  const Planet *dest;
  const QString orig_name;
  const QString orig_name_i18n;
};

ArtificialPlanet::ArtificialPlanet(const Planet &orig) : 
		Planet(0, "", 0, 0, 0, 0, Vec3f(0,0,0), 0, "", "",
		posFuncType(), 0, false, true, false), dest(0),
		orig_name(orig.getEnglishName()), orig_name_i18n(orig.getNameI18n())
{
  radius = 0;
    // set parent = sun:
  if (orig.getParent()) {
    parent = orig.getParent();
    while (parent->getParent()) parent = parent->getParent();
  } else {
    parent = &orig; // sun
  }
  re = orig.getRotationElements();
  setRotEquatorialToVsop87(orig.getRotEquatorialToVsop87());
  setHeliocentricEclipticPos(orig.getHeliocentricEclipticPos());
}

void ArtificialPlanet::setDest(const Planet &dest)
{
	ArtificialPlanet::dest = &dest;
	englishName = QString("%1->%2").arg(orig_name).arg(dest.getEnglishName());
	nameI18 = QString("%1->%2").arg(orig_name_i18n).arg(dest.getNameI18n());
	
	// rotation:
	const RotationElements &r(dest.getRotationElements());
	lastJD = StelApp::getInstance().getCore()->getNavigation()->getJDay();
	
	re.offset = r.offset + fmod(re.offset - r.offset + 360.0*( (lastJD-re.epoch)/re.period - (lastJD-r.epoch)/r.period), 360.0);
	
	re.epoch = r.epoch;
	re.period = r.period;
	if (re.offset - r.offset < -180.f) re.offset += 360.f; else
	if (re.offset - r.offset >  180.f) re.offset -= 360.f;
}

void ArtificialPlanet::setRot(const Vec3d &r) {
  const double ca = cos(r[0]);
  const double sa = sin(r[0]);
  const double cd = cos(r[1]);
  const double sd = sin(r[1]);
  const double cp = cos(r[2]);
  const double sp = sin(r[2]);
  Mat4d m;
  m.r[ 0] = cd*cp;
  m.r[ 4] = sd;
  m.r[ 8] = cd*sp;
  m.r[12] = 0;
  m.r[ 1] = -ca*sd*cp -sa*sp;
  m.r[ 5] =  ca*cd;
  m.r[ 9] = -ca*sd*sp +sa*cp;
  m.r[13] = 0;
  m.r[ 2] =  sa*sd*cp -ca*sp;
  m.r[ 6] = -sa*cd;
  m.r[10] =  sa*sd*sp +ca*cp;
  m.r[14] = 0;
  m.r[ 3] = 0;
  m.r[ 7] = 0;
  m.r[11] = 0;
  m.r[15] = 1.0;
  setRotEquatorialToVsop87(m);
}

Vec3d ArtificialPlanet::GetRot(const Planet *p) {
  const Mat4d m(p->getRotEquatorialToVsop87());
  const double cos_r1 = sqrt(m.r[0]*m.r[0]+m.r[8]*m.r[8]);
  Vec3d r;
  r[1] = atan2(m.r[4],cos_r1);
    // not well defined if cos(r[1])==0:
  if (cos_r1 <= 0.0) {
    // if (m.r[4]>0.0) sin,cos(a-p)=m.r[ 9],m.r[10]
    // else sin,cos(a+p)=m.r[ 9],m.r[10]
    // so lets say p=0:
    r[2] = 0.0;
    r[0] = atan2(m.r[9],m.r[10]);
  } else {
    r[0] = atan2(-m.r[6],m.r[5]);
    r[2] = atan2( m.r[8],m.r[0]);
  }
  return r;
}

void ArtificialPlanet::computeAverage(double f1) {
  const double f2 = 1.0 - f1;
     // position:
  setHeliocentricEclipticPos(getHeliocentricEclipticPos()*f1
                      + dest->getHeliocentricEclipticPos()*f2);

    // 3 Euler angles:
  Vec3d a1(GetRot(this));
  const Vec3d a2(GetRot(dest));
  if (a1[0]-a2[0] >  M_PI) a1[0] -= 2.0*M_PI; else
  if (a1[0]-a2[0] < -M_PI) a1[0] += 2.0*M_PI;
  if (a1[2]-a2[2] >  M_PI) a1[2] -= 2.0*M_PI; else
  if (a1[2]-a2[2] < -M_PI) a1[2] += 2.0*M_PI;
  setRot(a1*f1 + a2*f2);

    // rotation offset:
  re.offset = f1*re.offset + f2*dest->getRotationElements().offset;
}




Observer::Observer(const SolarSystem &ssystem) : ssystem(ssystem), planet(0), artificialPlanet(0)
{
	flagMoveTo = false;
}

Observer::~Observer()
{
	if (artificialPlanet)
		delete artificialPlanet;
}

Vec3d Observer::getCenterVsop87Pos(void) const
{
	return getHomePlanet()->getHeliocentricEclipticPos();
}

double Observer::getDistanceFromCenter(void) const
{
	return getHomePlanet()->getRadius() + (currentLocation.altitude/(1000*AU));
}

Mat4d Observer::getRotLocalToEquatorial(double jd) const
{
	double lat = currentLocation.latitude;
	// TODO: Figure out how to keep continuity in sky as reach poles
	// otherwise sky jumps in rotation when reach poles in equatorial mode
	// This is a kludge
	if( lat > 89.5 )  lat = 89.5;
	if( lat < -89.5 ) lat = -89.5;
	return Mat4d::zrotation((getHomePlanet()->getSiderealTime(jd)+currentLocation.longitude)*(M_PI/180.))
		* Mat4d::yrotation((90.-lat)*(M_PI/180.));
}

Mat4d Observer::getRotEquatorialToVsop87(void) const
{
	return getHomePlanet()->getRotEquatorialToVsop87();
}

// Get the sideral time shifted by the observer longitude
double Observer::getLocalSideralTime(double jd) const
{
	return (getHomePlanet()->getSiderealTime(jd)+currentLocation.longitude)*M_PI/180.;
}

void Observer::init()
{
	const QString locationName = StelApp::getInstance().getSettings()->value("init_location/location","Paris, Paris, France").toString();
	setPlanetLocation(StelApp::getInstance().getPlanetLocationMgr().locationForSmallString(locationName));
}

bool Observer::setHomePlanet(const QString &english_name)
{
	Planet *p = ssystem.searchByEnglishName(english_name);
	if (p==NULL)
	{
		qWarning() << "Can't set home planet to " + english_name + " because it is unknown";
  		return false;
	}
	setHomePlanet(p);
	return true;
}

void Observer::setHomePlanet(const Planet *p, float transitSeconds)
{
	assert(p); // Assertion enables to track bad calls. Please keep it this way.
	if (planet != p)
	{
		if (planet)
		{
			if (!artificialPlanet)
			{
				artificialPlanet = new ArtificialPlanet(*planet);
				currentLocation.name = "";
			}
			artificialPlanet->setDest(*p);
			timeToGo = (int)(1000.f * transitSeconds); // milliseconds
    	}
		planet = p; 
	}
}

const Planet* Observer::getHomePlanet(void) const
{
	return artificialPlanet ? artificialPlanet : planet;
}


// move gradually to a new observation location
void Observer::moveTo(const PlanetLocation& target, double duration)
{
	flagMoveTo = true;
	moveStartLocation = currentLocation;
	moveTargetLocation = target;
	moveToCoef = 1.0f/duration;
	moveToMult = 0;
}


// for moving observator position gradually
// TODO need to work on direction of motion...
void Observer::update(int deltaTime)
{
	if (artificialPlanet)
	{
		timeToGo -= deltaTime;
		if (timeToGo <= 0)
		{
			delete artificialPlanet;
			artificialPlanet = NULL;
			StelObjectMgr &objmgr(StelApp::getInstance().getStelObjectMgr());
			if (objmgr.getWasSelected() && objmgr.getSelectedObject()[0].get()==planet)
			{
				objmgr.unSelect();
			}
		}
		else
		{
			const double f1 = timeToGo/(double)(timeToGo + deltaTime);
			artificialPlanet->computeAverage(f1);
		}
	}

	if (flagMoveTo)
	{
		moveToMult += moveToCoef*deltaTime;
		if (moveToMult >= 1.f)
		{
			moveToMult = 1.f;
			flagMoveTo = false;
			currentLocation = moveTargetLocation;
		}
		currentLocation.latitude = moveStartLocation.latitude - moveToMult*(moveStartLocation.latitude-moveTargetLocation.latitude);
		currentLocation.longitude = moveStartLocation.longitude - moveToMult*(moveStartLocation.longitude-moveTargetLocation.longitude);
		currentLocation.altitude = int(moveStartLocation.altitude - moveToMult*(moveStartLocation.altitude-moveTargetLocation.altitude));
		currentLocation.name = moveStartLocation.name + " -> " + moveTargetLocation.name;
		currentLocation.planetName = "SpaceShip";
	}
}

//! Set the observer position to this planet location
void Observer::setPlanetLocation(const PlanetLocation& loc)
{
	currentLocation = loc;
	if (!setHomePlanet(loc.planetName))
		planet = ssystem.getEarth();
}
