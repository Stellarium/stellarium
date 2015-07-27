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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelObserver.hpp"
#include "StelUtils.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelLocaleMgr.hpp"
#include "StelTranslator.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

#include "StelLocationMgr.hpp"
#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"

#include <QDebug>
#include <QSettings>
#include <QStringList>

class ArtificialPlanet : public Planet
{
public:
	ArtificialPlanet(const PlanetP& orig);
	void setDest(const PlanetP& dest);
	void computeAverage(double f1);
private:
	void setRot(const Vec3d &r);
	static Vec3d getRot(const Planet* p);
	PlanetP dest;
	const QString orig_name;
	const QString orig_name_i18n;
};

ArtificialPlanet::ArtificialPlanet(const PlanetP& orig) :
		Planet("", 0, 0, 0, Vec3f(0,0,0), 0, "", "", NULL, NULL, 0, false, true, false, true, ""), dest(0),
		orig_name(orig->getEnglishName()), orig_name_i18n(orig->getNameI18n())
{
	radius = 0;
	// set parent = sun:
	if (orig->getParent())
	{
		parent = orig->getParent();
		while (parent->getParent())
			parent = parent->getParent();
	}
	else
	{
		parent = orig; // sun
	}
	re = orig->getRotationElements();
	setRotEquatorialToVsop87(orig->getRotEquatorialToVsop87());
	setHeliocentricEclipticPos(orig->getHeliocentricEclipticPos());
}

void ArtificialPlanet::setDest(const PlanetP& dest)
{
	ArtificialPlanet::dest = dest;
	englishName = QString("%1->%2").arg(orig_name).arg(dest->getEnglishName());
	nameI18 = QString("%1->%2").arg(orig_name_i18n).arg(dest->getNameI18n());

	// rotation:
	const RotationElements &r(dest->getRotationElements());
	lastJD = StelApp::getInstance().getCore()->getJDay();

	re.offset = r.offset + fmod(re.offset - r.offset + 360.0*( (lastJD-re.epoch)/re.period - (lastJD-r.epoch)/r.period), 360.0);

	re.epoch = r.epoch;
	re.period = r.period;
	if (re.offset - r.offset < -180.f) re.offset += 360.f; else
	if (re.offset - r.offset >  180.f) re.offset -= 360.f;
}

void ArtificialPlanet::setRot(const Vec3d &r)
{
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

Vec3d ArtificialPlanet::getRot(const Planet* p)
{
	const Mat4d m(p->getRotEquatorialToVsop87());
	const double cos_r1 = std::sqrt(m.r[0]*m.r[0]+m.r[8]*m.r[8]);
	Vec3d r;
	r[1] = atan2(m.r[4],cos_r1);
	// not well defined if cos(r[1])==0:
	if (cos_r1 <= 0.0)
	{
		// if (m.r[4]>0.0) sin,cos(a-p)=m.r[ 9],m.r[10]
		// else sin,cos(a+p)=m.r[ 9],m.r[10]
		// so lets say p=0:
		r[2] = 0.0;
		r[0] = atan2(m.r[9],m.r[10]);
	}
	else
	{
		r[0] = atan2(-m.r[6],m.r[5]);
		r[2] = atan2( m.r[8],m.r[0]);
	}
	return r;
}

void ArtificialPlanet::computeAverage(double f1)
{
	const double f2 = 1.0 - f1;
	// position
	setHeliocentricEclipticPos(getHeliocentricEclipticPos()*f1 + dest->getHeliocentricEclipticPos()*f2);

	// 3 Euler angles
	Vec3d a1(getRot(this));
	const Vec3d a2(getRot(dest.data()));
	if (a1[0]-a2[0] >  M_PI)
		a1[0] -= 2.0*M_PI;
	else
		if (a1[0]-a2[0] < -M_PI)
			a1[0] += 2.0*M_PI;
	if (a1[2]-a2[2] >  M_PI)
		a1[2] -= 2.0*M_PI;
	else
		if (a1[2]-a2[2] < -M_PI)
			a1[2] += 2.0*M_PI;
	setRot(a1*f1 + a2*f2);

	// rotation offset
	re.offset = f1*re.offset + f2*dest->getRotationElements().offset;
}




StelObserver::StelObserver(const StelLocation &loc) : currentLocation(loc)
{
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	planet = ssystem->searchByEnglishName(loc.planetName);
	if (planet==NULL)
	{
		qWarning() << "Can't create StelObserver on planet " + loc.planetName + " because it is unknown. Use Earth as default.";
		planet=ssystem->getEarth();
	}
}

StelObserver::~StelObserver()
{
}

const QSharedPointer<Planet> StelObserver::getHomePlanet(void) const
{
	Q_ASSERT(planet);
	return planet;
}

Vec3d StelObserver::getCenterVsop87Pos(void) const
{
	return getHomePlanet()->getHeliocentricEclipticPos();
}

// Used to approximate solution with assuming a spherical planet.
// Since V0.14, following Meeus, Astr. Alg. 2nd ed, Ch.11.
double StelObserver::getDistanceFromCenter(void) const
{
	if (getHomePlanet()->getRadius()==0.0) // the transitional ArtificialPlanet od SpaceShipObserver has this
		return currentLocation.altitude/(1000*AU);

	double a=getHomePlanet()->getRadius();
	double bByA = getHomePlanet()->getOneMinusOblateness(); // b/a;

	if (fabs(currentLocation.latitude)>=89.9) // avoid tan(90) issues.
		return a * bByA;

	double latRad=currentLocation.latitude*(M_PI/180.0);
	double u = atan( bByA * tan(latRad));
	// qDebug() << "getDistanceFromCenter: a=" << a*AU << "b/a=" << bByA << "b=" << bByA*a *AU  << "latRad=" << latRad << "u=" << u;
	Q_ASSERT(fabs(u)<= fabs(latRad));
	double altFix=(currentLocation.altitude/(1000.0*AU)) / a;

	double rhoSinPhiPrime= bByA * sin(u) + altFix*sin(latRad);
	double rhoCosPhiPrime= cos(u) + altFix*cos(latRad);

	double rho = sqrt(rhoSinPhiPrime*rhoSinPhiPrime+rhoCosPhiPrime*rhoCosPhiPrime);
	return rho*a;
}

Mat4d StelObserver::getRotAltAzToEquatorial(double jd) const
{
	double lat = currentLocation.latitude;
	// TODO: Figure out how to keep continuity in sky as we reach poles
	// otherwise sky jumps in rotation when reach poles in equatorial mode
	// This is a kludge
	// GZ: Actually, why would that be? Lat should be clamped elsewhere. Added tests.
	Q_ASSERT(lat <=  90.0);
	Q_ASSERT(lat >= -90.0);
	if( lat > 90.0 )  lat = 90.0;
	if( lat < -90.0 ) lat = -90.0;
	// Include a DeltaT correction. Sidereal time and longitude here are both in degrees, but DeltaT in seconds of time.
	// 360 degrees = 24hrs; 15 degrees = 1hr = 3600s; 1 degree = 240s
	// Apply DeltaT correction only for Earth
	// TODO: make code readable by calling siderealTime(JD_UT), this should not contain a deltaT in its algorithm.
	double deltaT = 0.;
	if (getHomePlanet()->getEnglishName()=="Earth")
		deltaT = StelApp::getInstance().getCore()->getDeltaT(jd)/240.;
	return Mat4d::zrotation((getHomePlanet()->getSiderealTime(jd)+currentLocation.longitude-deltaT)*M_PI/180.)
		* Mat4d::yrotation((90.-lat)*M_PI/180.);
}

Mat4d StelObserver::getRotEquatorialToVsop87(void) const
{
	return getHomePlanet()->getRotEquatorialToVsop87();
}

SpaceShipObserver::SpaceShipObserver(const StelLocation& startLoc, const StelLocation& target, double atransitSeconds) : StelObserver(startLoc),
		moveStartLocation(startLoc), moveTargetLocation(target), artificialPlanet(NULL), transitSeconds(atransitSeconds)
{
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	PlanetP targetPlanet = ssystem->searchByEnglishName(moveTargetLocation.planetName);
	if (moveStartLocation.planetName!=moveTargetLocation.planetName)
	{
		PlanetP startPlanet = ssystem->searchByEnglishName(moveStartLocation.planetName);
		if (startPlanet.isNull() || targetPlanet.isNull())
		{
			qWarning() << "Can't move from planet " + moveStartLocation.planetName + " to planet " + moveTargetLocation.planetName + " because it is unknown";
			timeToGo = -1.;	// Will abort properly the move
			if (targetPlanet==NULL)
			{
				// Stay at the same position as a failover
				moveTargetLocation = moveStartLocation;
			}
			return;
		}

		ArtificialPlanet* artPlanet = new ArtificialPlanet(startPlanet);
		artPlanet->setDest(targetPlanet);
		artificialPlanet = QSharedPointer<Planet>(artPlanet);
	}
	planet = targetPlanet;
	timeToGo = transitSeconds;
}

SpaceShipObserver::~SpaceShipObserver()
{
	artificialPlanet.clear();
	planet.clear();
}

void SpaceShipObserver::update(double deltaTime)
{
	timeToGo -= deltaTime;

	// If move is over
	if (timeToGo <= 0.)
	{
		timeToGo = 0.;
		currentLocation = moveTargetLocation;
		LandscapeMgr* ls = GETSTELMODULE(LandscapeMgr);
		SolarSystem* ss = GETSTELMODULE(SolarSystem);
		if (ls->getFlagLandscapeAutoSelection())
		{
			QString pType = ss->getPlanetType(currentLocation.planetName);
			// If we have a landscape for target planet then set it or check and use
			// landscape type of target planet, otherwise use default landscape
			if (ls->getAllLandscapeNames().indexOf(currentLocation.planetName)>0)
				ls->setCurrentLandscapeName(currentLocation.planetName);
			else if (ls->getAllLandscapeIDs().indexOf(pType)>0)
				ls->setCurrentLandscapeID(pType);
			else
				ls->setCurrentLandscapeID(ls->getDefaultLandscapeID());
		}
	}
	else
	{
		if (artificialPlanet)
		{
			// Update SpaceShip position
			static_cast<ArtificialPlanet*>(artificialPlanet.data())->computeAverage(timeToGo/(timeToGo + deltaTime));			
			currentLocation.planetName = "SpaceShip";
			const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
			currentLocation.name = trans.qtranslate(moveStartLocation.planetName) + " -> " + trans.qtranslate(moveTargetLocation.planetName);
		}
		else
		{
			currentLocation.name = moveStartLocation.name + " -> " + moveTargetLocation.name;
			currentLocation.planetName = moveTargetLocation.planetName;
		}

		// Move the lon/lat/alt on the planet
		const double moveToMult = 1.-(timeToGo/transitSeconds);
		currentLocation.latitude = moveStartLocation.latitude - moveToMult*(moveStartLocation.latitude-moveTargetLocation.latitude);
		currentLocation.longitude = moveStartLocation.longitude - moveToMult*(moveStartLocation.longitude-moveTargetLocation.longitude);
		currentLocation.altitude = int(moveStartLocation.altitude - moveToMult*(moveStartLocation.altitude-moveTargetLocation.altitude));		
	}
}

const QSharedPointer<Planet> SpaceShipObserver::getHomePlanet() const
{
	return (isObserverLifeOver() || artificialPlanet==NULL)  ? planet : artificialPlanet;
}

