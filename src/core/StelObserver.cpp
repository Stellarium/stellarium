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
#include "StelApp.hpp"
#include "StelCore.hpp"

#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"

#include <QDebug>
#include <QSettings>
#include <QStringList>

//! @class ArtificialPlanet Auxiliary construct used during transitions from one planet to another.
class ArtificialPlanet : public Planet
{
public:
	ArtificialPlanet(const PlanetP& orig);
	void setDest(const PlanetP& dest);
	//! computes position and orientation between itself and @variable dest.
	//! @param f1 mixing factor. [0...1]. 1 means dest.
	void computeAverage(double f1);
	//! This does nothing, but avoids a crash.
	virtual void computePosition(const double dateJDE, const Vec3d &aberrationPush) Q_DECL_OVERRIDE;
private:
	void setRot(const Vec3d &r);
	static Vec3d getRot(const Planet* p);
	PlanetP dest;
	const QString orig_name;
	const QString orig_name_i18n;
};

ArtificialPlanet::ArtificialPlanet(const PlanetP& orig) :
		Planet("art", 0, 0, Vec3f(0,0,0), 0, 0, "", "", "", "", Q_NULLPTR, Q_NULLPTR, Q_NULLPTR, false, true, false, false, "artificial"),
		dest(Q_NULLPTR), orig_name(orig->getEnglishName()), orig_name_i18n(orig->getNameI18n())
{
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
	englishName = QString("%1->%2").arg(orig_name, dest->getEnglishName());
	nameI18 = QString("%1->%2").arg(orig_name_i18n, dest->getNameI18n());

	// rotation:
	const RotationElements &r(dest->getRotationElements());
	lastJDE = StelApp::getInstance().getCore()->getJDE();

	re.offset = r.offset + fmod(re.offset - r.offset + 360.0*( (lastJDE-re.epoch)/re.period - (lastJDE-r.epoch)/r.period), 360.0);

	re.epoch = r.epoch;
	re.period = r.period;
	if (re.offset - r.offset < -180.) re.offset += 360.; else
	if (re.offset - r.offset >  180.) re.offset -= 360.;
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

void ArtificialPlanet::computePosition(const double dateJDE, const Vec3d &aberrationpush)
{
	Q_UNUSED(dateJDE)
	Q_UNUSED(aberrationpush)
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
	if (planet==Q_NULLPTR)
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
// Since V0.14, we follow Meeus, Astr. Alg. 2nd ed, Ch.11., but used offset rho in a wrong way. (offset angle phi in distance rho.)
double StelObserver::getDistanceFromCenter(void) const
{
	Vec4d tmp=getTopographicOffsetFromCenter();
	return tmp.v[3];
}

// Used to approximate solution with assuming a spherical planet.
// Since V0.14, following Meeus, Astr. Alg. 2nd ed, Ch.11.
// Since V0.16, we can produce the usual offset values plus geocentric latitude phi'.
// Since V0.19.1 we give rho*a as fourth return value, simplifying the previous method.
Vec4d StelObserver::getTopographicOffsetFromCenter(void) const
{
	if (getHomePlanet()->getEquatorialRadius()==0.0) // the transitional ArtificialPlanet or SpaceShipObserver have this
		return Vec4d(0.,0.,static_cast<double>(currentLocation.getLatitude())*(M_PI/180.0),currentLocation.altitude/(1000.0*AU));

	return getHomePlanet()->getRectangularCoordinates(static_cast<double>(currentLocation.getLongitude()),
							  static_cast<double>(currentLocation.getLatitude()),
							  currentLocation.altitude);
}

// For Earth we require JD, for other planets JDE to describe rotation!
Mat4d StelObserver::getRotAltAzToEquatorial(double JD, double JDE) const
{
	double lat = qBound(-90.0, static_cast<double>(currentLocation.getLatitude()), 90.0);
	// TODO: Figure out how to keep continuity in sky as we reach poles
	// otherwise sky jumps in rotation when reach poles in equatorial mode
	// This is a kludge
	return Mat4d::zrotation((getHomePlanet()->getSiderealTime(JD, JDE)+static_cast<double>(currentLocation.getLongitude()))*M_PI/180.)
		* Mat4d::yrotation((90.-lat)*M_PI/180.);
}

Mat4d StelObserver::getRotEquatorialToVsop87(void) const
{
	return getHomePlanet()->getRotEquatorialToVsop87();
}

// The transit can be cut short by feeding atimeToGo, a value smaller than the transition time
SpaceShipObserver::SpaceShipObserver(const StelLocation& startLoc, const StelLocation& target, double atransitSeconds, double atimeToGo) : StelObserver(startLoc),
		moveStartLocation(startLoc), moveTargetLocation(target), artificialPlanet(Q_NULLPTR), timeToGo(atimeToGo), transitSeconds(atransitSeconds)
{
	Q_ASSERT((atimeToGo<0) || (atimeToGo>=0 && atimeToGo<=atransitSeconds));
	if(timeToGo<0.0)
		timeToGo = transitSeconds;

	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	PlanetP targetPlanet = ssystem->searchByEnglishName(moveTargetLocation.planetName);
	if (moveStartLocation.planetName!=moveTargetLocation.planetName)
	{
		PlanetP startPlanet = ssystem->searchByEnglishName(moveStartLocation.planetName);
		if (startPlanet.isNull() || targetPlanet.isNull())
		{
			qWarning() << "Can't move from planet " + moveStartLocation.planetName + " to planet " + moveTargetLocation.planetName + " because it is unknown";
			timeToGo = -1.;	// Will abort properly the move
			if (targetPlanet==Q_NULLPTR)
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
	// avoid confusion with debug messages...
	currentLocation.region=QString();
	currentLocation.state=QString();
	currentLocation.isUserLocation=true;
	currentLocation.role='X';
	currentLocation.landscapeKey=moveTargetLocation.landscapeKey;
	currentLocation.ianaTimeZone=moveTargetLocation.ianaTimeZone;
}

SpaceShipObserver::~SpaceShipObserver()
{
	artificialPlanet.clear();
	planet.clear();
}

bool SpaceShipObserver::update(double deltaTime)
{
	if (timeToGo <= 0.) return false; // Already over.
	if (deltaTime==0.) return false;
	timeToGo -= deltaTime;
	SolarSystem* ss = GETSTELMODULE(SolarSystem);

	// If move is over
	if (timeToGo <= 0.)
	{
		timeToGo = 0.;
		currentLocation = moveTargetLocation;
	}
	else
	{
		currentLocation.name = ss->searchByEnglishName(moveStartLocation.planetName)->getNameI18n() + " -> " +
						      ss->searchByEnglishName(moveTargetLocation.planetName)->getNameI18n();
		if (artificialPlanet)
		{
			// Update SpaceShip position
			static_cast<ArtificialPlanet*>(artificialPlanet.data())->computeAverage(timeToGo/(timeToGo + deltaTime));			
			currentLocation.planetName = "SpaceShip";			
		}
		else
			currentLocation.planetName = moveTargetLocation.planetName;

		// Move the lon/lat/alt on the planet
		const float moveToMult = 1.f-static_cast<float>(timeToGo/transitSeconds);
		currentLocation.setLatitude( moveStartLocation.getLatitude() - moveToMult*(moveStartLocation.getLatitude()-moveTargetLocation.getLatitude()));
		currentLocation.setLongitude(moveStartLocation.getLongitude() - moveToMult*(moveStartLocation.getLongitude()-moveTargetLocation.getLongitude()));
		currentLocation.altitude = int(moveStartLocation.altitude - moveToMult*(moveStartLocation.altitude-moveTargetLocation.altitude));
	}
	return true;
}

const QSharedPointer<Planet> SpaceShipObserver::getHomePlanet() const
{
	return (isObserverLifeOver() || artificialPlanet==Q_NULLPTR)  ? planet : artificialPlanet;
}

