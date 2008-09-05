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

#include "StelApp.hpp"
#include "Navigator.hpp"
#include "StelUtils.hpp"
#include "SolarSystem.hpp"
#include "Observer.hpp"
#include "Planet.hpp"
#include "StelObjectMgr.hpp"
#include "StelCore.hpp"
#include "PlanetLocationMgr.hpp"

#include <QSettings>
#include <QStringList>
#include <QDateTime>
#include <QDebug>

////////////////////////////////////////////////////////////////////////////////
Navigator::Navigator() : timeSpeed(JD_SECOND), JDay(0.), position(NULL)
{
	localVision=Vec3d(1.,0.,0.);
	equVision=Vec3d(1.,0.,0.);
	J2000EquVision=Vec3d(1.,0.,0.);  // not correct yet...
	viewingMode = ViewHorizon;  // default
}

Navigator::~Navigator()
{}

const Planet *Navigator::getHomePlanet(void) const
{
	return position->getHomePlanet();
}

void Navigator::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	const QString locationName = StelApp::getInstance().getSettings()->value("init_location/location","Paris, Paris, France").toString();
	position = new Observer(StelApp::getInstance().getPlanetLocationMgr().locationForSmallString(locationName));
	
	setTimeNow();
	setLocalVision(Vec3f(1,1e-05,0.2));
	// Compute transform matrices between coordinates systems
	updateTransformMatrices();
	updateModelViewMat();
	QString tmpstr = conf->value("navigation/viewing_mode", "horizon").toString();
	if (tmpstr=="equator")
		setViewingMode(Navigator::ViewEquator);
	else
	{
		if (tmpstr=="horizon")
			setViewingMode(Navigator::ViewHorizon);
		else
		{
			qDebug() << "ERROR : Unknown viewing mode type : " << tmpstr;
			assert(0);
		}
	}
	
	initViewPos = StelUtils::strToVec3f(conf->value("navigation/init_view_pos").toString());
	setLocalVision(initViewPos);
	
	// Navigation section
	presetSkyTime 		= conf->value("navigation/preset_sky_time",2451545.).toDouble();
	startupTimeMode 	= conf->value("navigation/startup_time_mode", "actual").toString().toLower();
	if (startupTimeMode=="preset")
		setJDay(presetSkyTime - StelUtils::getGMTShiftFromQT(presetSkyTime) * JD_HOUR);
	else if (startupTimeMode=="today")
		setTodayTime(getInitTodayTime());
	else 
		setTimeNow();
}

//! Set stellarium time to current real world time
void Navigator::setTimeNow()
{
	setJDay(StelUtils::getJDFromSystem());
}

void Navigator::setTodayTime(const QTime& target)
{
	QDateTime dt = QDateTime::currentDateTime();
	if (target.isValid())
	{
		dt.setTime(target);
		// don't forget to adjust for timezone / daylight savings.
		setJDay(StelUtils::qDateTimeToJd(dt)-(StelUtils::getGMTShiftFromQT(StelUtils::getJDFromSystem()) * JD_HOUR));
	}
	else
	{
		qWarning() << "WARNING - time passed to Navigator::setTodayTime is not valid. The system time will be used." << target;
		setTimeNow();
	}
}

//! Get whether the current stellarium time is the real world time
bool Navigator::getIsTimeNow(void) const
{
	// cache last time to prevent to much slow system call
	static double lastJD = getJDay();
	static bool previousResult = (fabs(getJDay()-StelUtils::getJDFromSystem())<JD_SECOND);
	if (fabs(lastJD-getJDay())>JD_SECOND/4)
	{
		lastJD = getJDay();
		previousResult = (fabs(getJDay()-StelUtils::getJDFromSystem())<JD_SECOND);
	}
	return previousResult;
}

QTime Navigator::getInitTodayTime(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	return QTime::fromString(conf->value("navigation/today_time", "22:00").toString(), "hh:mm");
}

void Navigator::setInitTodayTime(const QTime& t)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	conf->setValue("navigation/today_time", t.toString("hh:mm"));
} 

QDateTime Navigator::getInitDateTime(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	return StelUtils::jdToQDateTime(conf->value("navigation/preset_sky_time",2451545.).toDouble() - StelUtils::getGMTShiftFromQT(presetSkyTime) * JD_HOUR);
}

void Navigator::setInitDateTime(const QDateTime& dt)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	conf->setValue("navigation/preset_sky_time", StelUtils::qDateTimeToJd(dt));
}

void Navigator::addSolarDays(double d)
{
	setJDay(getJDay() + d);
}

void Navigator::addSiderealDays(double d)
{
	const Planet* home = position->getHomePlanet();
	if (home->getEnglishName() != "Solar System Observer")
		d *= home->getSiderealDay();
	setJDay(getJDay() + d);
}

void Navigator::moveObserverToSelected(void)
{
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
	{
		Planet* pl = dynamic_cast<Planet*>(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0].get());
		if (pl)
		{
			// We need to move to the selected planet. Try to generate a location from the current one
			PlanetLocation loc = getCurrentLocation();
			loc.planetName = pl->getEnglishName();
			loc.name = "-";
			loc.state = "";
			moveObserverTo(loc);
		}
	}
}

// Get the informations on the current location
const PlanetLocation& Navigator::getCurrentLocation() const
{
	return position->getCurrentLocation();
}

// Smoothly move the observer to the given location
void Navigator::moveObserverTo(const PlanetLocation& target, double duration, double durationIfPlanetChange)
{
	double d = (getCurrentLocation().planetName==target.planetName) ? duration : durationIfPlanetChange;
	if (d>0.)
	{
		SpaceShipObserver* newObs = new SpaceShipObserver(getCurrentLocation(), target, d);
		delete position;
		position = newObs;
	}
	else
	{
		delete position;
		position = new Observer(target);
	}
}

// Get the sideral time shifted by the observer longitude
double Navigator::getLocalSideralTime() const
{
	return (position->getHomePlanet()->getSiderealTime(JDay)+position->getCurrentLocation().longitude)*M_PI/180.;
}

void Navigator::setInitViewDirectionToCurrent(void)
{
	QString dirStr = QString("%1,%2,%3").arg(localVision[0]).arg(localVision[1]).arg(localVision[2]);
	StelApp::getInstance().getSettings()->setValue("navigation/init_view_pos", dirStr);
}

//! Increase the time speed
void Navigator::increaseTimeSpeed()
{
	double s = getTimeSpeed();
	if (s>=JD_SECOND) s*=10.;
	else if (s<-JD_SECOND) s/=10.;
	else if (s>=0. && s<JD_SECOND) s=JD_SECOND;
	else if (s>=-JD_SECOND && s<0.) s=0.;
	setTimeSpeed(s);
}

//! Decrease the time speed
void Navigator::decreaseTimeSpeed()
{
	double s = getTimeSpeed();
	if (s>JD_SECOND) s/=10.;
	else if (s<=-JD_SECOND) s*=10.;
	else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
	else if (s>0. && s<=JD_SECOND) s=0.;
	setTimeSpeed(s);
}
	
////////////////////////////////////////////////////////////////////////////////
void Navigator::setLocalVision(const Vec3d& _pos)
{
	localVision = _pos;
	equVision=localToEarthEqu(localVision);
	J2000EquVision = matEarthEquToJ2000*equVision;
}

////////////////////////////////////////////////////////////////////////////////
void Navigator::setEquVision(const Vec3d& _pos)
{
	equVision = _pos;
	J2000EquVision = matEarthEquToJ2000*equVision;
	localVision = earthEquToLocal(equVision);
}

////////////////////////////////////////////////////////////////////////////////
void Navigator::setJ2000EquVision(const Vec3d& _pos)
{
	J2000EquVision = _pos;
	equVision = matJ2000ToEarthEqu*J2000EquVision;
	localVision = earthEquToLocal(equVision);
}

////////////////////////////////////////////////////////////////////////////////
// Increment time
void Navigator::updateTime(double deltaTime)
{
	JDay+=timeSpeed*deltaTime;

	// Fix time limits to -100000 to +100000 to prevent bugs
	if (JDay>38245309.499988) JDay = 38245309.499988;
	if (JDay<-34803211.500012) JDay = -34803211.500012;
	
	if (position->isObserverLifeOver())
	{
		// Unselect if the new home planet is the previously selected object
		StelObjectMgr &objmgr(StelApp::getInstance().getStelObjectMgr());
		if (objmgr.getWasSelected() && objmgr.getSelectedObject()[0].get()==position->getHomePlanet())
		{
			objmgr.unSelect();
		}
		Observer* newObs = position->getNextObserver();
		delete position;
		position = newObs;
	}
	position->update(deltaTime);
}

////////////////////////////////////////////////////////////////////////////////
// The non optimized (more clear version is available on the CVS : before date 25/07/2003)
// see vsop87.doc:

const Mat4d matJ2000ToVsop87(Mat4d::xrotation(-23.4392803055555555556*(M_PI/180)) * Mat4d::zrotation(0.0000275*(M_PI/180)));
const Mat4d matVsop87ToJ2000(matJ2000ToVsop87.transpose());

void Navigator::updateTransformMatrices(void)
{
	matLocalToEarthEqu = position->getRotLocalToEquatorial(JDay);
	matEarthEquToLocal = matLocalToEarthEqu.transpose();

	matEarthEquToJ2000 = matVsop87ToJ2000 * position->getRotEquatorialToVsop87();
	matJ2000ToEarthEqu = matEarthEquToJ2000.transpose();
	matJ2000ToLocal = matEarthEquToLocal*matJ2000ToEarthEqu;
	
	matHelioToEarthEqu = matJ2000ToEarthEqu * matVsop87ToJ2000 * Mat4d::translation(-position->getCenterVsop87Pos());

	// These two next have to take into account the position of the observer on the earth
	Mat4d tmp = matJ2000ToVsop87 * matEarthEquToJ2000 * matLocalToEarthEqu;

	matLocalToHelio =  Mat4d::translation(position->getCenterVsop87Pos()) * tmp *
	                      Mat4d::translation(Vec3d(0.,0., position->getDistanceFromCenter()));

	matHelioToLocal =  Mat4d::translation(Vec3d(0.,0.,-position->getDistanceFromCenter())) * tmp.transpose() *
	                      Mat4d::translation(-position->getCenterVsop87Pos());
}

void Navigator::setStartupTimeMode(const QString& s)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	startupTimeMode = s;
	conf->setValue("navigation/startup_time_mode", startupTimeMode);
}

////////////////////////////////////////////////////////////////////////////////
// Update the modelview matrices
void Navigator::updateModelViewMat(void)
{
	Vec3d f;

	if( viewingMode == ViewEquator)
	{
		// view will use equatorial coordinates, so that north is always up
		f = equVision;
	}
	else
	{
		// view will correct for horizon (always down)
		f = localVision;
	}

	f.normalize();
	Vec3d s(f[1],-f[0],0.);

	if( viewingMode == ViewEquator)
	{
		// convert everything back to local coord
		f = localVision;
		f.normalize();
		s = earthEquToLocal( s );
	}

	Vec3d u(s^f);
	s.normalize();
	u.normalize();

	matLocalToEye.set(s[0],u[0],-f[0],0.,
	                     s[1],u[1],-f[1],0.,
	                     s[2],u[2],-f[2],0.,
	                     0.,0.,0.,1.);

	matEarthEquToEye = matLocalToEye*matEarthEquToLocal;
	matHelioToEye = matLocalToEye*matHelioToLocal;
	matJ2000ToEye = matEarthEquToEye*matJ2000ToEarthEqu;
}


////////////////////////////////////////////////////////////////////////////////
// Return the observer heliocentric position
Vec3d Navigator::getObserverHelioPos(void) const
{
	static const Vec3d v(0.,0.,0.);
	return matLocalToHelio*v;
}


////////////////////////////////////////////////////////////////////////////////
// Set type of viewing mode (align with horizon or equatorial coordinates)
void Navigator::setViewingMode(ViewingModeType viewMode)
{
	viewingMode = viewMode;

	// TODO: include some nice smoothing function trigger here to rotate between
	// the two modes
}
