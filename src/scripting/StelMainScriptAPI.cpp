/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
 * Copyright (C) 2007 Fabien Chereau
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

#include "StelMainScriptAPI.hpp"
#include "StelMainScriptAPIProxy.hpp"
#include "StelScriptMgr.hpp"
#include "StelLocaleMgr.hpp"

#include "ConstellationMgr.hpp"
#include "GridLinesMgr.hpp"
#include "LandscapeMgr.hpp"
#include "MeteorMgr.hpp"
#include "NebulaMgr.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StarMgr.hpp"
#include "StelApp.hpp"
#include "StelAudioMgr.hpp"
#include "StelVideoMgr.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocation.hpp"
#include "StelLocationMgr.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"

#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelProjector.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelUtils.hpp"
#include "StelGuiBase.hpp"
#include "MilkyWay.hpp"

#include <QAction>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QSet>
#include <QStringList>
#include <QTemporaryFile>

#include <cmath>

StelMainScriptAPI::StelMainScriptAPI(QObject *parent) : QObject(parent)
{
	if(StelSkyLayerMgr* smgr = GETSTELMODULE(StelSkyLayerMgr))
	{
		connect(this, SIGNAL(requestLoadSkyImage(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool)), smgr, SLOT(loadSkyImage(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool)));
		connect(this, SIGNAL(requestLoadSkyImageAltAz(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool)), smgr, SLOT(loadSkyImageAltAz(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool)));
		connect(this, SIGNAL(requestRemoveSkyImage(const QString&)), smgr, SLOT(removeSkyLayer(const QString&)));
	}

	connect(this, SIGNAL(requestLoadSound(const QString&, const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(loadSound(const QString&, const QString&)));
	connect(this, SIGNAL(requestPlaySound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(playSound(const QString&)));
	connect(this, SIGNAL(requestPauseSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(pauseSound(const QString&)));
	connect(this, SIGNAL(requestStopSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(stopSound(const QString&)));
	connect(this, SIGNAL(requestDropSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(dropSound(const QString&)));

	connect(this, SIGNAL(requestLoadVideo(const QString&, const QString&, float, float, bool, float)), StelApp::getInstance().getStelVideoMgr(), SLOT(loadVideo(const QString&, const QString&, float, float, bool, float)));
	connect(this, SIGNAL(requestPlayVideo(const QString&)), StelApp::getInstance().getStelVideoMgr(), SLOT(playVideo(const QString&)));
	connect(this, SIGNAL(requestPauseVideo(const QString&)), StelApp::getInstance().getStelVideoMgr(), SLOT(pauseVideo(const QString&)));
	connect(this, SIGNAL(requestStopVideo(const QString&)), StelApp::getInstance().getStelVideoMgr(), SLOT(stopVideo(const QString&)));
	connect(this, SIGNAL(requestDropVideo(const QString&)), StelApp::getInstance().getStelVideoMgr(), SLOT(dropVideo(const QString&)));
	connect(this, SIGNAL(requestSeekVideo(const QString&, qint64)), StelApp::getInstance().getStelVideoMgr(), SLOT(seekVideo(const QString&, qint64)));
	connect(this, SIGNAL(requestSetVideoXY(const QString&, float, float)), StelApp::getInstance().getStelVideoMgr(), SLOT(setVideoXY(const QString&, float, float)));
	connect(this, SIGNAL(requestSetVideoAlpha(const QString&, float)), StelApp::getInstance().getStelVideoMgr(), SLOT(setVideoAlpha(const QString&, float)));
	connect(this, SIGNAL(requestResizeVideo(const QString&, float, float)), StelApp::getInstance().getStelVideoMgr(), SLOT(resizeVideo(const QString&, float, float)));
	connect(this, SIGNAL(requestShowVideo(const QString&, bool)), StelApp::getInstance().getStelVideoMgr(), SLOT(showVideo(const QString&, bool)));

	connect(this, SIGNAL(requestExit()), this->parent(), SLOT(stopScript()));
	connect(this, SIGNAL(requestSetNightMode(bool)), &StelApp::getInstance(), SLOT(setVisionModeNight(bool)));
	connect(this, SIGNAL(requestSetProjectionMode(QString)), StelApp::getInstance().getCore(), SLOT(setCurrentProjectionTypeKey(QString)));
	connect(this, SIGNAL(requestSetSkyCulture(QString)), &StelApp::getInstance().getSkyCultureMgr(), SLOT(setCurrentSkyCultureID(QString)));
	connect(this, SIGNAL(requestSetDiskViewport(bool)), StelMainGraphicsView::getInstance().getMainScriptAPIProxy(), SLOT(setDiskViewport(bool)));	
	connect(this, SIGNAL(requestSetHomePosition()), StelApp::getInstance().getCore(), SLOT(returnToHome()));
}

StelMainScriptAPI::~StelMainScriptAPI()
{
}

//! Set the current date in Julian Day
//! @param JD the Julian Date
void StelMainScriptAPI::setJDay(double JD)
{
	StelApp::getInstance().getCore()->setJDay(JD);
}

//! Get the current date in Julian Day
//! @return the Julian Date
double StelMainScriptAPI::getJDay() const
{
	return StelApp::getInstance().getCore()->getJDay();
}

//! Set the current date in Modified Julian Day
//! @param MJD the Modified Julian Date
void StelMainScriptAPI::setMJDay(double MJD)
{
	StelApp::getInstance().getCore()->setMJDay(MJD);
}

//! Get the current date in Modified Julian Day
//! @return the Modified Julian Date
double StelMainScriptAPI::getMJDay() const
{
	return StelApp::getInstance().getCore()->getMJDay();
}

void StelMainScriptAPI::setDate(const QString& dt, const QString& spec)
{
	StelApp::getInstance().getCore()->setJDay(jdFromDateString(dt, spec));
}

QString StelMainScriptAPI::getDate(const QString& spec)
{
	if (spec=="utc")
		return StelUtils::julianDayToISO8601String(getJDay());
	else
		return StelUtils::julianDayToISO8601String(getJDay()+StelUtils::getGMTShiftFromQT(getJDay())/24);
}

QString StelMainScriptAPI::getDeltaT() const
{
	return StelUtils::hoursToHmsStr(StelUtils::getDeltaT(getJDay())/3600.);
}

//! Set time speed in JDay/sec
//! @param ts time speed in JDay/sec
void StelMainScriptAPI::setTimeRate(double ts)
{
	// 1 second = .00001157407407407407 JDay
	StelApp::getInstance().getCore()->setTimeRate(ts * 0.00001157407407407407 * StelMainGraphicsView::getInstance().getScriptMgr().getScriptRate());
}

//! Get time speed in JDay/sec
//! @return time speed in JDay/sec
double StelMainScriptAPI::getTimeRate() const
{
	return StelApp::getInstance().getCore()->getTimeRate() / (0.00001157407407407407 * StelMainGraphicsView::getInstance().getScriptMgr().getScriptRate());
}

bool StelMainScriptAPI::isRealTime()
{
	return StelApp::getInstance().getCore()->getIsTimeNow();
}

void StelMainScriptAPI::setRealTime()
{
	setTimeRate(1.0);
	StelApp::getInstance().getCore()->setTimeNow();
}

void StelMainScriptAPI::setObserverLocation(double longitude, double latitude, double altitude, double duration, const QString& name, const QString& planet)
{
	StelCore* core = StelApp::getInstance().getCore();
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	Q_ASSERT(ssmgr);

	StelLocation loc = core->getCurrentLocation();
	loc.longitude = longitude;
	loc.latitude = latitude;
	if (altitude > -1000)
		loc.altitude = altitude;
	if (ssmgr->searchByName(planet))
		loc.planetName = planet;
	loc.name = name;
	core->moveObserverTo(loc, duration);
}

void StelMainScriptAPI::setObserverLocation(const QString id, float duration)
{
	StelCore* core = StelApp::getInstance().getCore();
	bool ok;
	StelLocation loc = StelApp::getInstance().getLocationMgr().locationForSmallString(id, &ok);
	if (!ok)
		return;	// location find failed
	core->moveObserverTo(loc, duration);
}

QString StelMainScriptAPI::getObserverLocation()
{
	return StelApp::getInstance().getCore()->getCurrentLocation().getID();
}

QVariantMap StelMainScriptAPI::getObserverLocationInfo()
{
	StelCore* core = StelApp::getInstance().getCore();
	const PlanetP& planet = core->getCurrentPlanet();
	double siderealDay = planet->getSiderealDay();
	double siderealPeriod = planet->getSiderealPeriod();
	double solarDay;
	QString planetName = core->getCurrentLocation().planetName;
	if ((planetName == "Venus") || (planetName == "Uranus"))
		solarDay = StelUtils::calculateSolarDay(siderealPeriod, siderealDay, false);
	else
		solarDay = StelUtils::calculateSolarDay(siderealPeriod, siderealDay, true);

	QVariantMap map;
	map.insert("longitude", core->getCurrentLocation().longitude);
	map.insert("latitude", core->getCurrentLocation().latitude);
	map.insert("planet", planetName);
	map.insert("altitude", core->getCurrentLocation().altitude);
	map.insert("location", core->getCurrentLocation().getID());
	// extra data
	map.insert("sidereal-year", siderealPeriod);
	map.insert("sidereal-day", siderealDay*24.);
	map.insert("solar-day", solarDay*24.);

	return map;
}

void StelMainScriptAPI::screenshot(const QString& prefix, bool invert, const QString& dir)
{
	bool oldInvertSetting = StelMainGraphicsView::getInstance().getFlagInvertScreenShotColors();
	StelMainGraphicsView::getInstance().setFlagInvertScreenShotColors(invert);
	StelMainGraphicsView::getInstance().saveScreenShot(prefix, dir);
	StelMainGraphicsView::getInstance().setFlagInvertScreenShotColors(oldInvertSetting);
}

void StelMainScriptAPI::setGuiVisible(bool b)
{
	StelApp::getInstance().getGui()->setVisible(b);
}

void StelMainScriptAPI::setMinFps(float m)
{
	StelMainGraphicsView::getInstance().setMinFps(m);
}

float StelMainScriptAPI::getMinFps()
{
	return StelMainGraphicsView::getInstance().getMinFps();
}

void StelMainScriptAPI::setMaxFps(float m)
{
	StelMainGraphicsView::getInstance().setMaxFps(m);
}

float StelMainScriptAPI::getMaxFps()
{
	return StelMainGraphicsView::getInstance().getMaxFps();
}

QString StelMainScriptAPI::getMountMode()
{
	if (GETSTELMODULE(StelMovementMgr)->getMountMode() == StelMovementMgr::MountEquinoxEquatorial)
		return "equatorial";
	else
		return "azimuthal";
}

void StelMainScriptAPI::setMountMode(const QString& mode)
{
	if (mode=="equatorial")
		GETSTELMODULE(StelMovementMgr)->setMountMode(StelMovementMgr::MountEquinoxEquatorial);
	else if (mode=="azimuthal")
		GETSTELMODULE(StelMovementMgr)->setMountMode(StelMovementMgr::MountAltAzimuthal);
}

bool StelMainScriptAPI::getNightMode()
{
	return StelApp::getInstance().getVisionModeNight();
}

void StelMainScriptAPI::setNightMode(bool b)
{
	emit(requestSetNightMode(b));
}

QString StelMainScriptAPI::getProjectionMode()
{
	return StelApp::getInstance().getCore()->getCurrentProjectionTypeKey();
}

void StelMainScriptAPI::setProjectionMode(const QString& id)
{
	emit(requestSetProjectionMode(id));
}

QStringList StelMainScriptAPI::getAllSkyCultureIDs()
{
	return StelApp::getInstance().getSkyCultureMgr().getSkyCultureListIDs();
}

QString StelMainScriptAPI::getSkyCulture()
{
	return StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID();
}

void StelMainScriptAPI::setSkyCulture(const QString& id)
{
	emit(requestSetSkyCulture(id));
}

bool StelMainScriptAPI::getFlagGravityLabels()
{
	return StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000)->getFlagGravityLabels();
}

void StelMainScriptAPI::setFlagGravityLabels(bool b)
{
	StelApp::getInstance().getCore()->setFlagGravityLabels(b);
}

bool StelMainScriptAPI::getDiskViewport()
{
	return StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000)->getMaskType() == StelProjector::MaskDisk;
}

void StelMainScriptAPI::setDiskViewport(bool b)
{
	emit(requestSetDiskViewport(b));
}

void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
						 double ra0, double dec0,
						 double ra1, double dec1,
						 double ra2, double dec2,
						 double ra3, double dec3,
								 double minRes, double maxBright, bool visible)
{
	QString path = "scripts/" + filename;
	emit(requestLoadSkyImage(id, path, ra0, dec0, ra1, dec1, ra2, dec2, ra3, dec3, minRes, maxBright, visible));
}

void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
									 const QString& ra0, const QString& dec0,
									 const QString& ra1, const QString& dec1,
									 const QString& ra2, const QString& dec2,
									 const QString& ra3, const QString& dec3,
									 double minRes, double maxBright, bool visible)
{
	loadSkyImage(id, filename,
				 StelUtils::getDecAngle(ra0) *180./M_PI, StelUtils::getDecAngle(dec0)*180./M_PI,
				 StelUtils::getDecAngle(ra1) *180./M_PI, StelUtils::getDecAngle(dec1)*180./M_PI,
				 StelUtils::getDecAngle(ra2) *180./M_PI, StelUtils::getDecAngle(dec2)*180./M_PI,
				 StelUtils::getDecAngle(ra3) *180./M_PI, StelUtils::getDecAngle(dec3)*180./M_PI,
				 minRes, maxBright, visible);
}

void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
									 double ra, double dec, double angSize, double rotation,
									 double minRes, double maxBright, bool visible)
{
	Vec3f XYZ;
	static const float RADIUS_NEB = 1.;
	StelUtils::spheToRect(ra*M_PI/180., dec*M_PI/180., XYZ);
	XYZ*=RADIUS_NEB;
	float texSize = RADIUS_NEB * sin(angSize/2/60*M_PI/180);
	Mat4f matPrecomp = Mat4f::translation(XYZ) *
					   Mat4f::zrotation(ra*M_PI/180.) *
					   Mat4f::yrotation(-dec*M_PI/180.) *
					   Mat4f::xrotation(rotation*M_PI/180.);

	Vec3f corners[4];
		corners[0] = matPrecomp * Vec3f(0.f,-texSize,-texSize);
		corners[1] = matPrecomp * Vec3f(0.f,-texSize, texSize);
		corners[2] = matPrecomp * Vec3f(0.f, texSize,-texSize);
		corners[3] = matPrecomp * Vec3f(0.f, texSize, texSize);

	// convert back to ra/dec (radians)
	Vec3f cornersRaDec[4];
	for(int i=0; i<4; i++)
		StelUtils::rectToSphe(&cornersRaDec[i][0], &cornersRaDec[i][1], corners[i]);

	loadSkyImage(id, filename,
				 cornersRaDec[0][0]*180./M_PI, cornersRaDec[0][1]*180./M_PI,
				 cornersRaDec[1][0]*180./M_PI, cornersRaDec[1][1]*180./M_PI,
				 cornersRaDec[3][0]*180./M_PI, cornersRaDec[3][1]*180./M_PI,
				 cornersRaDec[2][0]*180./M_PI, cornersRaDec[2][1]*180./M_PI,
				 minRes, maxBright, visible);
}



void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
									 const QString& ra, const QString& dec, double angSize, double rotation,
									 double minRes, double maxBright, bool visible)
{
	loadSkyImage(id, filename, StelUtils::getDecAngle(ra)*180./M_PI,
				 StelUtils::getDecAngle(dec)*180./M_PI, angSize,
				 rotation, minRes, maxBright, visible);
}

void StelMainScriptAPI::loadSkyImageAltAz(const QString& id, const QString& filename,
						 double alt0, double azi0,
						 double alt1, double azi1,
						 double alt2, double azi2,
						 double alt3, double azi3,
								 double minRes, double maxBright, bool visible)
{
	QString path = "scripts/" + filename;
	emit(requestLoadSkyImageAltAz(id, path, alt0, azi0, alt1, azi1, alt2, azi2, alt3, azi3, minRes, maxBright, visible));
}

void StelMainScriptAPI::loadSkyImageAltAz(const QString& id, const QString& filename,
									 double alt, double azi, double angSize, double rotation,
									 double minRes, double maxBright, bool visible)
{
	Vec3f XYZ;
	static const float RADIUS_NEB = 1.;

	StelUtils::spheToRect((180-azi)*M_PI/180., alt*M_PI/180., XYZ);
	XYZ*=RADIUS_NEB;
	float texSize = RADIUS_NEB * sin(angSize/2/60*M_PI/180);
	Mat4f matPrecomp = Mat4f::translation(XYZ) *
					   Mat4f::zrotation((180-azi)*M_PI/180.) *
					   Mat4f::yrotation(-alt*M_PI/180.) *
					   Mat4f::xrotation((rotation+90)*M_PI/180.);

	Vec3f corners[4];
		corners[0] = matPrecomp * Vec3f(0.f,-texSize,-texSize);
		corners[1] = matPrecomp * Vec3f(0.f,-texSize, texSize);
		corners[2] = matPrecomp * Vec3f(0.f, texSize,-texSize);
		corners[3] = matPrecomp * Vec3f(0.f, texSize, texSize);

	// convert back to alt/azi (radians)
	Vec3f cornersAltAz[4];
	for(int i=0; i<4; i++)
		StelUtils::rectToSphe(&cornersAltAz[i][0], &cornersAltAz[i][1], corners[i]);

	loadSkyImageAltAz(id, filename,
				 cornersAltAz[0][0]*180./M_PI, cornersAltAz[0][1]*180./M_PI,
				 cornersAltAz[1][0]*180./M_PI, cornersAltAz[1][1]*180./M_PI,
				 cornersAltAz[3][0]*180./M_PI, cornersAltAz[3][1]*180./M_PI,
				 cornersAltAz[2][0]*180./M_PI, cornersAltAz[2][1]*180./M_PI,
				 minRes, maxBright, visible);
}

void StelMainScriptAPI::removeSkyImage(const QString& id)
{
	emit(requestRemoveSkyImage(id));
}

void StelMainScriptAPI::loadSound(const QString& filename, const QString& id)
{
	QString path;
	try
	{
		path = StelFileMgr::findFile("scripts/" + filename);
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "cannot play sound" << filename << ":" << e.what();
		return;
	}

	emit(requestLoadSound(path, id));
}

void StelMainScriptAPI::playSound(const QString& id)
{
	emit(requestPlaySound(id));
}

void StelMainScriptAPI::pauseSound(const QString& id)
{
	emit(requestPauseSound(id));
}

void StelMainScriptAPI::stopSound(const QString& id)
{
	emit(requestStopSound(id));
}

void StelMainScriptAPI::dropSound(const QString& id)
{
	emit(requestDropSound(id));
}

void StelMainScriptAPI::loadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha)
{
	QString path;
	try
	{
		path = StelFileMgr::findFile("scripts/" + filename);
	}
	catch(std::runtime_error& e)
	{
		qWarning() << "cannot play video" << filename << ":" << e.what();
		return;
	}

	emit(requestLoadVideo(path, id, x, y, show, alpha));
}

void StelMainScriptAPI::playVideo(const QString& id)
{
	emit(requestPlayVideo(id));
}

void StelMainScriptAPI::pauseVideo(const QString& id)
{
	emit(requestPauseVideo(id));
}

void StelMainScriptAPI::stopVideo(const QString& id)
{
	emit(requestStopVideo(id));
}

void StelMainScriptAPI::dropVideo(const QString& id)
{
	emit(requestDropVideo(id));
}

void StelMainScriptAPI::seekVideo(const QString& id, qint64 ms)
{
	emit(requestSeekVideo(id, ms));
}

void StelMainScriptAPI::setVideoXY(const QString& id, float x, float y)
{
	emit(requestSetVideoXY(id, x, y));
}

void StelMainScriptAPI::setVideoAlpha(const QString& id, float alpha)
{
	emit(requestSetVideoAlpha(id, alpha));
}

void StelMainScriptAPI::resizeVideo(const QString& id, float w, float h)
{
	emit(requestResizeVideo(id, w, h));
}

void StelMainScriptAPI::showVideo(const QString& id, bool show)
{
	emit(requestShowVideo(id, show));
}

int StelMainScriptAPI::getScreenWidth()
{
	return StelMainGraphicsView::getInstance().size().width();
}

int StelMainScriptAPI::getScreenHeight()
{
	return StelMainGraphicsView::getInstance().size().height();
}

double StelMainScriptAPI::getScriptRate()
{
        return StelMainGraphicsView::getInstance().getScriptMgr().getScriptRate();
}

void StelMainScriptAPI::setScriptRate(double r)
{
        return StelMainGraphicsView::getInstance().getScriptMgr().setScriptRate(r);
}

void StelMainScriptAPI::pauseScript()
{
	return StelMainGraphicsView::getInstance().getScriptMgr().pauseScript();
}

void StelMainScriptAPI::setSelectedObjectInfo(const QString& level)
{
	if (level == "AllInfo")
		StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::AllInfo));
	else if (level == "ShortInfo")
		StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::ShortInfo));
	else if (level == "None")
		StelApp::getInstance().getGui()->setInfoTextFilters((StelObject::InfoStringGroup)0);
	else
		qWarning() << "setSelectedObjectInfo unknown level string \"" << level << "\"";
}

void StelMainScriptAPI::exit()
{
	emit(requestExit());
}

void StelMainScriptAPI::quitStellarium()
{
	QCoreApplication::exit();
}

void StelMainScriptAPI::debug(const QString& s)
{
	qDebug() << "script: " << s;
	StelMainGraphicsView::getInstance().getScriptMgr().debug(s);
}

double StelMainScriptAPI::jdFromDateString(const QString& dt, const QString& spec)
{
	if (dt == "now")
		return StelUtils::getJDFromSystem();
	
	bool ok;
	double jd;
	if (spec=="local")
	{
		jd = StelApp::getInstance().getLocaleMgr().getJdFromISO8601TimeLocal(dt, &ok);
	}
	else
	{
		jd = StelUtils::getJulianDayFromISO8601String(dt, &ok);
	}
	if (ok)
		return jd;
	
	QRegExp nowRe("^(now)?(\\s*([+\\-])\\s*(\\d+(\\.\\d+)?)\\s*(second|seconds|minute|minutes|hour|hours|day|days|week|weeks))(\\s+(sidereal)?)?");
	if (nowRe.exactMatch(dt))
	{
		double delta;
		double unit;
		double dayLength = 1.0;

		if (nowRe.capturedTexts().at(1)=="now")
			jd = StelUtils::getJDFromSystem();
		else
			jd = StelApp::getInstance().getCore()->getJDay();

		if (nowRe.capturedTexts().at(8) == "sidereal")
			dayLength = StelApp::getInstance().getCore()->getLocalSideralDayLength();

		QString unitString = nowRe.capturedTexts().at(6);
		if (unitString == "seconds" || unitString == "second")
			unit = dayLength / (24*3600.);
		else if (unitString == "minutes" || unitString == "minute")
			unit = dayLength / (24*60.);
		else if (unitString == "hours" || unitString == "hour")
			unit = dayLength / (24.);
		else if (unitString == "days" || unitString == "day")
			unit = dayLength;
		else if (unitString == "weeks" || unitString == "week")
			unit = dayLength * 7.;
		else
		{
			qWarning() << "StelMainScriptAPI::setDate - unknown time unit:" << nowRe.capturedTexts().at(4);
			unit = 0;
		}

		delta = nowRe.capturedTexts().at(4).toDouble();

		if (nowRe.capturedTexts().at(3) == "+")
			jd += (unit * delta);
		else if (nowRe.capturedTexts().at(3) == "-")
			jd -= (unit * delta);
		return jd;
	}
	
	qWarning() << "StelMainScriptAPI::jdFromDateString error: date string" << dt << "not recognised, returning \"now\"";
	return StelUtils::getJDFromSystem();
}

void StelMainScriptAPI::selectObjectByName(const QString& name, bool pointer)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	omgr->setFlagSelectedObjectPointer(pointer);
	if (name=="")
		omgr->unSelect();
	else
		omgr->findAndSelect(name);
}

//DEPRECATED: Use getObjectInfo()
QVariantMap StelMainScriptAPI::getObjectPosition(const QString& name)
{
	return getObjectInfo(name);
}

QVariantMap StelMainScriptAPI::getObjectInfo(const QString& name)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	StelObjectP obj = omgr->searchByName(name);
	QVariantMap map;

	if (!obj)
	{
		debug("getObjectData WARNING - object not found: " + name);
		map.insert("found", false);
		return map;
	}
	else
	{
		map.insert("found", true);
	}

	
	Vec3d pos;
	double ra, dec, alt, azi, glong, glat;
	StelCore* core = StelApp::getInstance().getCore();

	// ra/dec
	pos = obj->getEquinoxEquatorialPos(core);
	StelUtils::rectToSphe(&ra, &dec, pos);
	map.insert("ra", ra*180./M_PI);
	map.insert("dec", dec*180./M_PI);

	// ra/dec in J2000
	pos = obj->getJ2000EquatorialPos(core);
	StelUtils::rectToSphe(&ra, &dec, pos);
	map.insert("raJ2000", ra*180./M_PI);
	map.insert("decJ2000", dec*180./M_PI);

	// apparent altitude/azimuth
	pos = obj->getAltAzPosApparent(core);
	StelUtils::rectToSphe(&azi, &alt, pos);
	map.insert("altitude", alt*180./M_PI);
	map.insert("azimuth", azi*180./M_PI);

	// geometric altitude/azimuth
	pos = obj->getAltAzPosGeometric(core);
	StelUtils::rectToSphe(&azi, &alt, pos);
	map.insert("altitude-geometric", alt*180./M_PI);
	map.insert("azimuth-geometric", azi*180./M_PI);

	// galactic long/lat in J2000
	pos = obj->getJ2000GalacticPos(core);
	StelUtils::rectToSphe(&glong, &glat, pos);
	map.insert("glong", alt*180./M_PI);
	map.insert("glat", azi*180./M_PI);

	// magnitude
	map.insert("vmag", obj->getVMagnitude(core, false));
	map.insert("vmage", obj->getVMagnitude(core, true));

	// angular size
	map.insert("size", obj->getAngularSize(core));

	// localized name
	map.insert("localized-name", obj->getNameI18n());

	return map;
}

void StelMainScriptAPI::clear(const QString& state)
{
	LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
	SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
	MeteorMgr* mmgr = GETSTELMODULE(MeteorMgr);
	StelSkyDrawer* skyd = StelApp::getInstance().getCore()->getSkyDrawer();
	ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
	StarMgr* smgr = GETSTELMODULE(StarMgr);
	NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
	GridLinesMgr* glmgr = GETSTELMODULE(GridLinesMgr);
	StelMovementMgr* movmgr = GETSTELMODULE(StelMovementMgr);

	if (state.toLower() == "natural")
	{
		movmgr->setMountMode(StelMovementMgr::MountAltAzimuthal);
		skyd->setFlagTwinkle(true);
		skyd->setFlagLuminanceAdaptation(true);
		ssmgr->setFlagPlanets(true);
		ssmgr->setFlagHints(false);
		ssmgr->setFlagOrbits(false);
		ssmgr->setFlagMoonScale(false);
		ssmgr->setFlagTrails(false);
		mmgr->setZHR(10);
		glmgr->setFlagAzimuthalGrid(false);
		glmgr->setFlagGalacticGrid(false);
		glmgr->setFlagEquatorGrid(false);
		glmgr->setFlagEquatorLine(false);
		glmgr->setFlagEclipticLine(false);
		glmgr->setFlagMeridianLine(false);
		glmgr->setFlagHorizonLine(false);
		glmgr->setFlagGalacticPlaneLine(false);
		glmgr->setFlagEquatorJ2000Grid(false);
		lmgr->setFlagCardinalsPoints(false);
		cmgr->setFlagLines(false);
		cmgr->setFlagLabels(false);
		cmgr->setFlagBoundaries(false);
		cmgr->setFlagArt(false);
		smgr->setFlagLabels(false);
		ssmgr->setFlagLabels(false);
		nmgr->setFlagHints(false);
		lmgr->setFlagLandscape(true);
		lmgr->setFlagAtmosphere(true);
		lmgr->setFlagFog(true);
	}
	else if (state.toLower() == "starchart")
	{
		movmgr->setMountMode(StelMovementMgr::MountEquinoxEquatorial);
		skyd->setFlagTwinkle(false);
		skyd->setFlagLuminanceAdaptation(false);
		ssmgr->setFlagPlanets(true);
		ssmgr->setFlagHints(false);
		ssmgr->setFlagOrbits(false);
		ssmgr->setFlagMoonScale(false);
		ssmgr->setFlagTrails(false);
		mmgr->setZHR(0);
		glmgr->setFlagAzimuthalGrid(false);
		glmgr->setFlagGalacticGrid(false);
		glmgr->setFlagEquatorGrid(true);
		glmgr->setFlagEquatorLine(false);
		glmgr->setFlagEclipticLine(false);
		glmgr->setFlagMeridianLine(false);
		glmgr->setFlagHorizonLine(false);
		glmgr->setFlagGalacticPlaneLine(false);
		glmgr->setFlagEquatorJ2000Grid(false);
		lmgr->setFlagCardinalsPoints(false);
		cmgr->setFlagLines(true);
		cmgr->setFlagLabels(true);
		cmgr->setFlagBoundaries(true);
		cmgr->setFlagArt(false);
		smgr->setFlagLabels(true);
		ssmgr->setFlagLabels(true);
		nmgr->setFlagHints(true);
		lmgr->setFlagLandscape(false);
		lmgr->setFlagAtmosphere(false);
		lmgr->setFlagFog(false);
	}
	else if (state.toLower() == "deepspace")
	{
		movmgr->setMountMode(StelMovementMgr::MountEquinoxEquatorial);
		skyd->setFlagTwinkle(false);
		skyd->setFlagLuminanceAdaptation(false);
		ssmgr->setFlagPlanets(false);
		ssmgr->setFlagHints(false);
		ssmgr->setFlagOrbits(false);
		ssmgr->setFlagMoonScale(false);
		ssmgr->setFlagTrails(false);
		mmgr->setZHR(0);
		glmgr->setFlagAzimuthalGrid(false);
		glmgr->setFlagGalacticGrid(false);
		glmgr->setFlagEquatorGrid(false);
		glmgr->setFlagEquatorLine(false);
		glmgr->setFlagEclipticLine(false);
		glmgr->setFlagMeridianLine(false);
		glmgr->setFlagHorizonLine(false);
		glmgr->setFlagGalacticPlaneLine(false);
		glmgr->setFlagEquatorJ2000Grid(false);
		lmgr->setFlagCardinalsPoints(false);
		cmgr->setFlagLines(false);
		cmgr->setFlagLabels(false);
		cmgr->setFlagBoundaries(false);
		cmgr->setFlagArt(false);
		smgr->setFlagLabels(false);
		ssmgr->setFlagLabels(false);
		nmgr->setFlagHints(false);
		lmgr->setFlagLandscape(false);
		lmgr->setFlagAtmosphere(false);
		lmgr->setFlagFog(false);
	}
	else
	{
		qWarning() << "WARNING clear(" << state << ") - state not known";
	}
}

double StelMainScriptAPI::getViewAltitudeAngle()
{
	const Vec3d& current = StelApp::getInstance().getCore()->j2000ToAltAz(GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000(), StelCore::RefractionOff);
	double alt, azi;
	StelUtils::rectToSphe(&azi, &alt, current);
	return alt*180/M_PI; // convert to degrees from radians
}

double StelMainScriptAPI::getViewAzimuthAngle()
{
	const Vec3d& current = StelApp::getInstance().getCore()->j2000ToAltAz(GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000(), StelCore::RefractionOff);
	double alt, azi;
	StelUtils::rectToSphe(&azi, &alt, current);
	// The returned azimuth angle is in radians and set up such that:
	// N=+/-PI; E=PI/2; S=0; W=-PI/2;
	// But we want compass bearings, i.e. N=0, E=90, S=180, W=270
	return std::fmod(((azi*180/M_PI)*-1)+180., 360.);
}

double StelMainScriptAPI::getViewRaAngle()
{
	const Vec3d& current = StelApp::getInstance().getCore()->j2000ToEquinoxEqu(GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000());
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	// returned RA angle is in range -PI .. PI, but we want 0 .. 360
	return std::fmod((ra*180/M_PI)+360., 360.); // convert to degrees from radians
}

double StelMainScriptAPI::getViewDecAngle()
{
	const Vec3d& current = StelApp::getInstance().getCore()->j2000ToEquinoxEqu(GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000());
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	return dec*180/M_PI; // convert to degrees from radians
}

double StelMainScriptAPI::getViewRaJ2000Angle()
{
	Vec3d current = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	// returned RA angle is in range -PI .. PI, but we want 0 .. 360
	return std::fmod((ra*180/M_PI)+360., 360.); // convert to degrees from radians
}

double StelMainScriptAPI::getViewDecJ2000Angle()
{
	Vec3d current = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	return dec*180/M_PI; // convert to degrees from radians
}

void StelMainScriptAPI::moveToAltAzi(const QString& alt, const QString& azi, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);

	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aim;
	double dAlt = StelUtils::getDecAngle(alt);
	double dAzi = M_PI - StelUtils::getDecAngle(azi);

	StelUtils::spheToRect(dAzi,dAlt,aim);
	mvmgr->moveToJ2000(StelApp::getInstance().getCore()->altAzToJ2000(aim, StelCore::RefractionOff), duration);
}

void StelMainScriptAPI::moveToRaDec(const QString& ra, const QString& dec, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);

	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aim;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);

	StelUtils::spheToRect(dRa,dDec,aim);
	mvmgr->moveToJ2000(StelApp::getInstance().getCore()->equinoxEquToJ2000(aim), duration);
}

void StelMainScriptAPI::moveToRaDecJ2000(const QString& ra, const QString& dec, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);

	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aimJ2000, aimEquofDate;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);

	StelUtils::spheToRect(dRa,dDec,aimJ2000);
	aimEquofDate = StelApp::getInstance().getCore()->j2000ToEquinoxEqu(aimJ2000);
	mvmgr->moveToJ2000(aimEquofDate, duration);
}

QString StelMainScriptAPI::getAppLanguage()
{
	return StelApp::getInstance().getLocaleMgr().getAppLanguage();
}

void StelMainScriptAPI::setAppLanguage(QString langCode)
{
	StelApp::getInstance().getLocaleMgr().setAppLanguage(langCode);
}

QString StelMainScriptAPI::getSkyLanguage()
{
	return StelApp::getInstance().getLocaleMgr().getSkyLanguage();
}

void StelMainScriptAPI::setSkyLanguage(QString langCode)
{
	StelApp::getInstance().getLocaleMgr().setSkyLanguage(langCode);
}

void StelMainScriptAPI::goHome()
{
	emit(requestSetHomePosition());
}

void StelMainScriptAPI::setMilkyWayVisible(bool b)
{
	GETSTELMODULE(MilkyWay)->setFlagShow(b);
}

void StelMainScriptAPI::setMilkyWayIntensity(double i)
{
	GETSTELMODULE(MilkyWay)->setIntensity(i);
}

double StelMainScriptAPI::getMilkyWayIntensity()
{
	return GETSTELMODULE(MilkyWay)->getIntensity();
}
