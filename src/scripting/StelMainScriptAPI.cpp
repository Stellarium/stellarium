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
#include "AsterismMgr.hpp"
#include "GridLinesMgr.hpp"
#include "LandscapeMgr.hpp"
#include "SporadicMeteorMgr.hpp"
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
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelPropertyMgr.hpp"
#include "StelScriptMgr.hpp"

#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelProjector.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelUtils.hpp"
#include "StelGuiBase.hpp"
#include "MilkyWay.hpp"
#include "ZodiacalLight.hpp"
#include "ToastMgr.hpp"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QSet>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>
#include <QEventLoop>

#include <cmath>

StelMainScriptAPI::StelMainScriptAPI(QObject *parent) : QObject(parent)
{
	if(StelSkyLayerMgr* smgr = GETSTELMODULE(StelSkyLayerMgr))
	{
		connect(this, SIGNAL(requestLoadSkyImage(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool, StelCore::FrameType)),
			smgr, SLOT(         loadSkyImage(const QString&, const QString&, double, double, double, double, double, double, double, double, double, double, bool, StelCore::FrameType)));
		connect(this, SIGNAL(requestRemoveSkyImage(const QString&)), smgr, SLOT(removeSkyLayer(const QString&)));
	}

	connect(this, SIGNAL(requestLoadSound(const QString&, const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(loadSound(const QString&, const QString&)));
	connect(this, SIGNAL(requestPlaySound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(playSound(const QString&)));
	connect(this, SIGNAL(requestPauseSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(pauseSound(const QString&)));
	connect(this, SIGNAL(requestStopSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(stopSound(const QString&)));
	connect(this, SIGNAL(requestDropSound(const QString&)), StelApp::getInstance().getStelAudioMgr(), SLOT(dropSound(const QString&)));

	connect(this, SIGNAL(requestLoadVideo(const QString&, const QString&, float, float, bool, float)), StelApp::getInstance().getStelVideoMgr(), SLOT(loadVideo(const QString&, const QString&, float, float, bool, float)));
	connect(this, SIGNAL(requestPlayVideo(const QString&, const bool)), StelApp::getInstance().getStelVideoMgr(), SLOT(playVideo(const QString&, const bool)));
	connect(this, SIGNAL(requestPlayVideoPopout(QString,float,float,float,float,float,float,float,bool)), StelApp::getInstance().getStelVideoMgr(), SLOT(playVideoPopout(QString,float,float,float,float,float,float,float,bool)));
	connect(this, SIGNAL(requestPauseVideo(const QString&)), StelApp::getInstance().getStelVideoMgr(), SLOT(pauseVideo(const QString&)));
	connect(this, SIGNAL(requestStopVideo(const QString&)), StelApp::getInstance().getStelVideoMgr(), SLOT(stopVideo(const QString&)));
	connect(this, SIGNAL(requestDropVideo(const QString&)), StelApp::getInstance().getStelVideoMgr(), SLOT(dropVideo(const QString&)));
	connect(this, SIGNAL(requestSeekVideo(const QString&, qint64, bool)), StelApp::getInstance().getStelVideoMgr(), SLOT(seekVideo(const QString&, qint64, bool)));
	connect(this, SIGNAL(requestSetVideoXY(const QString&, float, float, bool)), StelApp::getInstance().getStelVideoMgr(), SLOT(setVideoXY(const QString&, float, float, bool)));
	connect(this, SIGNAL(requestSetVideoAlpha(const QString&, float)), StelApp::getInstance().getStelVideoMgr(), SLOT(setVideoAlpha(const QString&, float)));
	connect(this, SIGNAL(requestResizeVideo(const QString&, float, float)), StelApp::getInstance().getStelVideoMgr(), SLOT(resizeVideo(const QString&, float, float)));
	connect(this, SIGNAL(requestShowVideo(const QString&, bool)), StelApp::getInstance().getStelVideoMgr(), SLOT(showVideo(const QString&, bool)));

	connect(this, SIGNAL(requestExit()), this->parent(), SLOT(stopScript()));
	connect(this, SIGNAL(requestSetProjectionMode(QString)), StelApp::getInstance().getCore(), SLOT(setCurrentProjectionTypeKey(QString)));
	connect(this, SIGNAL(requestSetSkyCulture(QString)), &StelApp::getInstance().getSkyCultureMgr(), SLOT(setCurrentSkyCultureID(QString)));
	connect(this, SIGNAL(requestSetDiskViewport(bool)), StelApp::getInstance().getMainScriptAPIProxy(), SLOT(setDiskViewport(bool)));	
	connect(this, SIGNAL(requestSetHomePosition()), StelApp::getInstance().getCore(), SLOT(returnToHome()));
}

StelMainScriptAPI::~StelMainScriptAPI()
{
}

//! Set the current date in Julian Day
//! @param JD the Julian Date (UT)
void StelMainScriptAPI::setJDay(double JD)
{
	StelApp::getInstance().getCore()->setJD(JD);
}

//! Get the current date in Julian Day
//! @return the Julian Date (UT)
double StelMainScriptAPI::getJDay()
{
	return StelApp::getInstance().getCore()->getJD();
}

//! Set the current date in Modified Julian Day
//! @param MJD the Modified Julian Date
void StelMainScriptAPI::setMJDay(double MJD)
{
	StelApp::getInstance().getCore()->setMJDay(MJD);
}

//! Get the current date in Modified Julian Day
//! @return the Modified Julian Date
double StelMainScriptAPI::getMJDay()
{
	return StelApp::getInstance().getCore()->getMJDay();
}

void StelMainScriptAPI::setDate(const QString& dateStr, const QString& spec, const bool &dateIsDT)
{
	StelCore* core = StelApp::getInstance().getCore();
	double JD = jdFromDateString(dateStr, spec);
	if (dateIsDT)
		core->setJDE(JD);
	else
		core->setJD(JD);
}

QString StelMainScriptAPI::getDate(const QString& spec)
{
	if (spec=="utc")
		return StelUtils::julianDayToISO8601String(getJDay());
	else
		return StelUtils::julianDayToISO8601String(getJDay()+static_cast<double>(StelApp::getInstance().getCore()->getUTCOffset(getJDay()))/24);
}

QString StelMainScriptAPI::getDeltaT()
{
	return StelUtils::hoursToHmsStr(StelApp::getInstance().getCore()->getDeltaT()/3600.);
}

QString StelMainScriptAPI::getDeltaTAlgorithm()
{
	return StelApp::getInstance().getCore()->getCurrentDeltaTAlgorithmKey();
}

void StelMainScriptAPI::setDeltaTAlgorithm(QString algorithmName)
{
	StelApp::getInstance().getCore()->setCurrentDeltaTAlgorithmKey(algorithmName);
}

//! Set time speed in JDay/sec
//! @param ts time speed in JDay/sec
void StelMainScriptAPI::setTimeRate(double ts)
{
	// 1 second = .00001157407407407407 JDay
	StelApp::getInstance().getCore()->setTimeRate(ts * 0.00001157407407407407 * StelApp::getInstance().getScriptMgr().getScriptRate());
}

//! Get time speed in JDay/sec
//! @return time speed in JDay/sec
double StelMainScriptAPI::getTimeRate()
{
	return StelApp::getInstance().getCore()->getTimeRate() / (0.00001157407407407407 * StelApp::getInstance().getScriptMgr().getScriptRate());
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

bool StelMainScriptAPI::isPlanetocentricCalculations()
{
	return !(StelApp::getInstance().getCore()->getUseTopocentricCoordinates());
}

void StelMainScriptAPI::setPlanetocentricCalculations(bool f)
{
	StelApp::getInstance().getCore()->setUseTopocentricCoordinates(!f);
}

void StelMainScriptAPI::setObserverLocation(double longitude, double latitude, double altitude, double duration, const QString& name, const QString& planet)
{
	StelCore* core = StelApp::getInstance().getCore();
	StelObjectP ssObj = GETSTELMODULE(SolarSystem)->searchByName(planet);	
	StelLocation loc = core->getCurrentLocation();
	loc.longitude = static_cast<float>(longitude);
	loc.latitude = static_cast<float>(latitude);
	if (altitude > -1000)
		loc.altitude = qRound(altitude);
	if (!ssObj.isNull())
		loc.planetName = ssObj->getEnglishName();
	else
		return; // Avoid crash when planet is not defined or not exist

	QRegExp cico( "^\\s*([^,]+),\\s*(\\S.*)$" );
	if( cico.exactMatch( name ) )
	{
		loc.name = cico.cap(1);
		loc.country = cico.cap(2);
	}
	else
		loc.name = name;

	core->moveObserverTo(loc, duration, duration);
}

void StelMainScriptAPI::setObserverLocation(const QString &id, double duration)
{
	StelCore* core = StelApp::getInstance().getCore();
	StelLocation loc = StelApp::getInstance().getLocationMgr().locationForString(id);
	if (!loc.isValid())
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
	QString planetName = core->getCurrentLocation().planetName;
	QVariantMap map;
	map.insert("longitude", core->getCurrentLocation().longitude);
	map.insert("latitude", core->getCurrentLocation().latitude);
	map.insert("planet", planetName);
	map.insert("altitude", core->getCurrentLocation().altitude);
	map.insert("location", core->getCurrentLocation().getID());
	// extra data
	map.insert("sidereal-year", planet->getSiderealPeriod());
	map.insert("sidereal-day", planet->getSiderealDay()*24.);
	map.insert("solar-day", planet->getMeanSolarDay()*24.);
	unsigned int h, m;
	double s;
	StelUtils::radToHms(core->getLocalSiderealTime(), h, m, s);
	map.insert("local-sidereal-time", static_cast<double>(h) + static_cast<double>(m)/60. + s/3600);
	map.insert("local-sidereal-time-hms", StelUtils::radToHmsStr(core->getLocalSiderealTime()));
	map.insert("location-timezone", core->getCurrentLocation().ianaTimeZone);
	map.insert("timezone", core->getCurrentTimeZone());

	return map;
}

void StelMainScriptAPI::setTimezone(QString tz, int markAsCustom)
{
	StelCore* core = StelApp::getInstance().getCore();
	core->setCurrentTimeZone(tz);
	switch (markAsCustom){
		case 0: // and
		case 1:	core->setUseCustomTimeZone(static_cast<bool>(markAsCustom));
			break;
		default: break;
	}
}

QStringList StelMainScriptAPI::getAllTimezoneNames()
{
	return StelApp::getInstance().getLocationMgr().getAllTimezoneNames();
}

void StelMainScriptAPI::screenshot(const QString& prefix, bool invert, const QString& dir, const bool overwrite, const QString &format)
{
	bool oldInvertSetting = StelMainView::getInstance().getFlagInvertScreenShotColors();
	QString oldFormat=StelMainView::getInstance().getScreenshotFormat();
	StelMainView::getInstance().setFlagInvertScreenShotColors(invert);
	if ((format.length()>0) && (format.length()<=4))
		StelMainView::getInstance().setScreenshotFormat(format);
	StelMainView::getInstance().setFlagOverwriteScreenShots(overwrite);
	StelMainView::getInstance().saveScreenShot(prefix, dir, overwrite);
	StelMainView::getInstance().setFlagInvertScreenShotColors(oldInvertSetting);
	StelMainView::getInstance().setScreenshotFormat(oldFormat);
}

void StelMainScriptAPI::setGuiVisible(bool b)
{
	StelApp::getInstance().getGui()->setVisible(b);
}

void StelMainScriptAPI::setMinFps(float m)
{
	StelMainView::getInstance().setMinFps(m);
}

float StelMainScriptAPI::getMinFps()
{
	return StelMainView::getInstance().getMinFps();
}

void StelMainScriptAPI::setMaxFps(float m)
{
	StelMainView::getInstance().setMaxFps(m);
}

float StelMainScriptAPI::getMaxFps()
{
	return StelMainView::getInstance().getMaxFps();
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
	StelApp::getInstance().setVisionModeNight(b);
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
	GETSTELMODULE(StelObjectMgr)->unSelect(); // mistake-proofing!
	emit(requestSetSkyCulture(id));
}

QString StelMainScriptAPI::getSkyCultureName()
{
	return StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureEnglishName();
}

QString StelMainScriptAPI::getSkyCultureNameI18n()
{
	return StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureNameI18();
}

bool StelMainScriptAPI::getFlagGravityLabels()
{
	return StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000)->getFlagGravityLabels();
}

void StelMainScriptAPI::setFlagGravityLabels(bool b)
{
	StelApp::getInstance().getCore()->setFlagGravityLabels(b);
}

bool StelMainScriptAPI::getFlipHorz()
{
	return StelApp::getInstance().getCore()->getFlipHorz();
}

void StelMainScriptAPI::setFlipHorz(bool b)
{
	StelApp::getInstance().getCore()->setFlipHorz(b);
}

bool StelMainScriptAPI::getFlipVert()
{
	return StelApp::getInstance().getCore()->getFlipVert();
}

void StelMainScriptAPI::setFlipVert(bool b)
{
	StelApp::getInstance().getCore()->setFlipVert(b);
}

bool StelMainScriptAPI::getDiskViewport()
{
	return StelApp::getInstance().getCore()->getProjection(StelCore::FrameJ2000)->getMaskType() == StelProjector::MaskDisk;
}

void StelMainScriptAPI::setSphericMirror(bool b)
{
	StelCore* core = StelApp::getInstance().getCore();
	if (b)
	{
		core->setCurrentProjectionType(StelCore::ProjectionFisheye);
		StelApp::getInstance().setViewportEffect("sphericMirrorDistorter");
	}
	else
	{
		core->setCurrentProjectionTypeKey(core->getDefaultProjectionTypeKey());
		StelApp::getInstance().setViewportEffect("none");
	}
}

void StelMainScriptAPI::setDiskViewport(bool b)
{
	emit(requestSetDiskViewport(b));
}

void StelMainScriptAPI::setViewportStretch(const float stretch)
{
	StelApp::getInstance().getCore()->setViewportStretch(stretch);
}

void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
				     double lon0, double lat0,
				     double lon1, double lat1,
				     double lon2, double lat2,
				     double lon3, double lat3,
				     double minRes, double maxBright, bool visible, const QString &frame)
{
	QString path = "scripts/" + filename;
	StelCore::FrameType frameType=StelCore::FrameJ2000;
	if (frame=="EqDate")
		frameType=StelCore::FrameEquinoxEqu;
	else if (frame=="EclJ2000")
		frameType=StelCore::FrameObservercentricEclipticJ2000;
	else if (frame=="EclDate")
		frameType=StelCore::FrameObservercentricEclipticOfDate;
	else if (frame.startsWith("Gal"))
		frameType=StelCore::FrameGalactic;
	else if (frame.startsWith("SuperG"))
		frameType=StelCore::FrameSupergalactic;
	else if (frame=="AzAlt")
		frameType=StelCore::FrameAltAz;
	else if (frame!="EqJ2000")
	{
		qDebug() << "StelMainScriptAPI::loadSkyImage(): unknown frame type " << frame << " requested -- Using Equatorial J2000";
	}

	emit(requestLoadSkyImage(id, path, lon0, lat0, lon1, lat1, lon2, lat2, lon3, lat3, minRes, maxBright, visible, frameType));
}

// Convenience method:
void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
				     const QString& lon0, const QString& lat0,
				     const QString& lon1, const QString& lat1,
				     const QString& lon2, const QString& lat2,
				     const QString& lon3, const QString& lat3,
				     double minRes, double maxBright, bool visible, const QString& frame)
{
	loadSkyImage(id, filename,
		     StelUtils::getDecAngle(lon0) *M_180_PI, StelUtils::getDecAngle(lat0)*M_180_PI,
		     StelUtils::getDecAngle(lon1) *M_180_PI, StelUtils::getDecAngle(lat1)*M_180_PI,
		     StelUtils::getDecAngle(lon2) *M_180_PI, StelUtils::getDecAngle(lat2)*M_180_PI,
		     StelUtils::getDecAngle(lon3) *M_180_PI, StelUtils::getDecAngle(lat3)*M_180_PI,
		     minRes, maxBright, visible, frame);
}

// Convenience method: (Fixed 2017-03: rotation increased by 90deg, makes upright images now!)
void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
				     double lon, double lat, double angSize, double rotation,
				     double minRes, double maxBright, bool visible, const QString &frame)
{
	Vec3f XYZ;
	static const float RADIUS_NEB = 1.f;
	StelUtils::spheToRect(static_cast<float>(lon*M_PI_180), static_cast<float>(lat*M_PI_180), XYZ);
	XYZ*=RADIUS_NEB;
	float texSize = RADIUS_NEB * static_cast<float>(sin(angSize/2./60.*M_PI_180));
	Mat4f matPrecomp = Mat4f::translation(XYZ) *
			   Mat4f::zrotation(static_cast<float>(lon*M_PI_180)) *
			   Mat4f::yrotation(static_cast<float>(-lat*M_PI_180)) *
			   Mat4f::xrotation(static_cast<float>((rotation+90.0)*M_PI_180));

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
		     static_cast<double>(cornersRaDec[0][0])*(M_180_PI), static_cast<double>(cornersRaDec[0][1])*(M_180_PI),
		     static_cast<double>(cornersRaDec[1][0])*(M_180_PI), static_cast<double>(cornersRaDec[1][1])*(M_180_PI),
		     static_cast<double>(cornersRaDec[3][0])*(M_180_PI), static_cast<double>(cornersRaDec[3][1])*(M_180_PI),
		     static_cast<double>(cornersRaDec[2][0])*(M_180_PI), static_cast<double>(cornersRaDec[2][1])*(M_180_PI),
		     minRes, maxBright, visible, frame);
}

// Convenience method:
void StelMainScriptAPI::loadSkyImage(const QString& id, const QString& filename,
				     const QString& lon, const QString& lat,
				     double angSize, double rotation,
				     double minRes, double maxBright, bool visible, const QString &frame)
{
	loadSkyImage(id, filename, StelUtils::getDecAngle(lon)*M_180_PI,
		     StelUtils::getDecAngle(lat)*M_180_PI, angSize,
		     rotation, minRes, maxBright, visible, frame);
}

void StelMainScriptAPI::removeSkyImage(const QString& id)
{
	emit(requestRemoveSkyImage(id));
}

void StelMainScriptAPI::loadSound(const QString& filename, const QString& id)
{
	QString path = StelFileMgr::findFile("scripts/" + filename);
	if (path.isEmpty())
	{
		qWarning() << "cannot play sound" << QDir::toNativeSeparators(filename);
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

qint64 StelMainScriptAPI::getSoundPosition(const QString& id)
{
	return StelApp::getInstance().getStelAudioMgr()->position(id);
}

qint64 StelMainScriptAPI::getSoundDuration(const QString& id)
{
	return StelApp::getInstance().getStelAudioMgr()->duration(id);
}

void StelMainScriptAPI::loadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha)
{
	QString path = StelFileMgr::findFile("scripts/" + filename);
	if (path.isEmpty())
	{
		qWarning() << "cannot play video" << QDir::toNativeSeparators(filename);
		return;
	}

	emit(requestLoadVideo(path, id, x, y, show, alpha));
}

void StelMainScriptAPI::playVideo(const QString& id, bool keepVisibleAtEnd)
{
	emit(requestPlayVideo(id, keepVisibleAtEnd));
}

void StelMainScriptAPI::playVideoPopout(const QString& id, float fromX, float fromY, float atCenterX, float atCenterY, float finalSizeX, float finalSizeY, float popupDuration, bool frozenInTransition)
{
	emit(requestPlayVideoPopout(id, fromX, fromY, atCenterX, atCenterY, finalSizeX, finalSizeY, popupDuration, frozenInTransition));
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

void StelMainScriptAPI::seekVideo(const QString& id, qint64 ms, bool pause)
{
	emit(requestSeekVideo(id, ms, pause));
}

void StelMainScriptAPI::setVideoXY(const QString& id, float x, float y, bool relative)
{
	emit(requestSetVideoXY(id, x, y, relative));
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

qint64 StelMainScriptAPI::getVideoDuration(const QString& id)
{
	return StelApp::getInstance().getStelVideoMgr()->getVideoDuration(id);
}

qint64 StelMainScriptAPI::getVideoPosition(const QString& id)
{
	return StelApp::getInstance().getStelVideoMgr()->getVideoPosition(id);
}

int StelMainScriptAPI::getScreenWidth()
{
	return StelMainView::getInstance().size().width();
}

int StelMainScriptAPI::getScreenHeight()
{
	return StelMainView::getInstance().size().height();
}

double StelMainScriptAPI::getScriptRate()
{
        return StelApp::getInstance().getScriptMgr().getScriptRate();
}

void StelMainScriptAPI::setScriptRate(double r)
{
        return StelApp::getInstance().getScriptMgr().setScriptRate(r);
}

void StelMainScriptAPI::pauseScript()
{
	return StelApp::getInstance().getScriptMgr().pauseScript();
}

void StelMainScriptAPI::setSelectedObjectInfo(const QString& level)
{
	if (level == "AllInfo")
		StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::AllInfo));
	else if (level == "ShortInfo")
		StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelObject::ShortInfo));
	else if (level == "None")
		StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(Q_NULLPTR));
	else
		qWarning() << "setSelectedObjectInfo unknown level string \"" << level << "\"";
}

void StelMainScriptAPI::exit()
{
	emit(requestExit());
}

void StelMainScriptAPI::quitStellarium()
{
	StelApp::getInstance().quit(); // quit from planetarium
}

QStringList StelMainScriptAPI::getPropertyList()
{
	return StelApp::getInstance().getStelPropertyManager()->getPropertyList();
}

void StelMainScriptAPI::debug(const QString& s)
{
	qDebug() << "script: " << s;
	StelApp::getInstance().getScriptMgr().debug(s);
}

void StelMainScriptAPI::output(const QString &s)
{
	StelApp::getInstance().getScriptMgr().output(s);
}

//! print contents of a QVariantMap
//! @param map QVariantMap e.g. from getObjectInfo() or getLocationInfo()
QString StelMainScriptAPI::mapToString(const QVariantMap& map)
{
	QString res = QString("[\n");
	QList<QVariant::Type> simpleTypeList;
	simpleTypeList.push_back(QVariant::Bool);
	simpleTypeList.push_back(QVariant::Int);
	simpleTypeList.push_back(QVariant::UInt);
	simpleTypeList.push_back(QVariant::Double);

	for (auto i = map.constBegin(); i != map.constEnd(); ++i)
	{
		if (i.value().type()==QVariant::String)
		{
			res.append(QString("[ \"%1\" = \"%2\" ]\n").arg(i.key()).arg(i.value().toString()));
		}
		else if (simpleTypeList.contains(i.value().type()))
		{
			res.append(QString("[ \"%1\" = %2 ]\n").arg(i.key()).arg(i.value().toString()));
		}
		else
		{
			res.append(QString("[ \"%1\" = \"<%2>:%3\" ]\n").arg(i.key()).arg(i.value().typeName()).arg(i.value().toString()));
		}
	}
	res.append( QString("]\n"));
	return res;
}

void StelMainScriptAPI::resetOutput(void)
{
	StelApp::getInstance().getScriptMgr().resetOutput();
}

void StelMainScriptAPI::saveOutputAs(const QString &filename)
{
	StelApp::getInstance().getScriptMgr().saveOutputAs(filename);
}

double StelMainScriptAPI::jdFromDateString(const QString& dt, const QString& spec)
{
	QString tdt = dt.trimmed();
	StelCore *core = StelApp::getInstance().getCore();
	if (tdt == "now")
		return StelUtils::getJDFromSystem();
	
	bool ok;
	double jd;
	if (spec=="local")
		jd = StelApp::getInstance().getLocaleMgr().getJdFromISO8601TimeLocal(tdt, &ok);
	else
		jd = StelUtils::getJulianDayFromISO8601String(tdt, &ok);

	if (ok)
		return jd;

	QRegExp nowRe("(now)?"
		      "\\s*([-+])"
		      "\\s*(\\d+(?:\\.\\d+)?(?:[eE][-+]?\\d+)?)"
		      "\\s*(second|minute|hour|day|sol|week|month|year)s?"
		      "(?:\\s+(sidereal))?");
	if (nowRe.exactMatch(tdt))
	{
		double delta;
		double unit;
		double dayLength = 1.0;
		double yearLength = 365.242190419; // duration of Earth's mean tropical year
		double monthLength = 27.321582241; // duration of Earth's mean tropical month

		if (nowRe.cap(1)=="now")
			jd = StelUtils::getJDFromSystem();
		else
			jd = core->getJD();

		if (nowRe.cap(5) == "sidereal")
		{
			dayLength = core->getLocalSiderealDayLength();
			yearLength = core->getLocalSiderealYearLength();
			monthLength = 27.321661; // duration of Earth's sidereal month
		}

		QString unitString = nowRe.cap(4);
		if ( unitString == "second")
			unit = dayLength / (24*3600.);
		else if (unitString == "minute")
			unit = dayLength / (24*60.);
		else if (unitString == "hour")
			unit = dayLength / (24.);
		else if (unitString == "day")
			unit = dayLength;
		else if (unitString == "sol")
			unit = core->getCurrentPlanet()->getMeanSolarDay();
		else if (unitString == "week")
			unit = dayLength * 7.;
		else if (unitString == "month")
			unit = monthLength;
		else if (unitString == "year")
			unit = yearLength;
		else
		{
			qWarning() << "StelMainScriptAPI::setDate - unknown time unit:" << nowRe.cap(4);
			unit = 0;
		}

		delta = nowRe.cap(3).toDouble();

		if (nowRe.cap(2) == "+")
			jd += (unit * delta);
		else if (nowRe.cap(2) == "-")
			jd -= (unit * delta);
		return jd;
	}
	
	qWarning() << "StelMainScriptAPI::jdFromDateString error: date string" << dt << "not recognised, returning \"now\"";
	return StelUtils::getJDFromSystem();
}

void StelMainScriptAPI::wait(double t)
{
	StelScriptMgr* scriptMgr = &StelApp::getInstance().getScriptMgr();
	QEventLoop* loop = scriptMgr->getWaitEventLoop();
	QTimer::singleShot(qRound(1000*t), loop, SLOT(quit()));
	if( loop->exec() != 0 )
	{
		emit(requestExit()); // causes a call of stopScript
	}
}

void StelMainScriptAPI::waitFor(const QString& dt, const QString& spec)
{
	double deltaJD = jdFromDateString(dt, spec) - getJDay();
	double timeRate = getTimeRate();
	if (timeRate == 0.)
	{
		qDebug() << "waitFor() called with no time passing - would be infinite. Not waiting!";
		return;
	}
	int interval=qRound(1000*deltaJD*86400/timeRate);
	if (interval<=0)
	{
		qDebug() << "waitFor() called, but negative interval (time exceeded before starting timer). Not waiting!";
		return;
	}
	StelScriptMgr* scriptMgr = &StelApp::getInstance().getScriptMgr();
	QEventLoop* loop = scriptMgr->getWaitEventLoop();
	QTimer::singleShot(interval, loop, SLOT(quit()));
	if( loop->exec() != 0 )
	{
		emit(requestExit()); // causes a call of stopScript
	}
}


void StelMainScriptAPI::selectObjectByName(const QString& name, bool pointer)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	omgr->setFlagSelectedObjectPointer(pointer);
	if (name.isEmpty() || !omgr->findAndSelect(name))
	{
		omgr->unSelect();
	}
}

void StelMainScriptAPI::selectConstellationByName(const QString& name)
{
	StelObjectP constellation = Q_NULLPTR;
	if (!name.isEmpty())
		constellation = GETSTELMODULE(ConstellationMgr)->searchByName(name);

	if (!constellation.isNull())
		GETSTELMODULE(StelObjectMgr)->setSelectedObject(constellation);
}

QVariantMap StelMainScriptAPI::getObjectInfo(const QString& name)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	StelObjectP obj = omgr->searchByName(name);

	return StelObjectMgr::getObjectInfo(obj);
}

QVariantMap StelMainScriptAPI::getSelectedObjectInfo()
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	QVariantMap map;
	if (omgr->getSelectedObject().isEmpty())
	{
		debug("getObjectData WARNING - object not selected");
		map.insert("found", false);
		return map;
	}

	StelObjectP obj = omgr->getSelectedObject()[0];

	return StelObjectMgr::getObjectInfo(obj);
}

void StelMainScriptAPI::addToSelectedObjectInfoString(const QString &str, bool replace)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	if (omgr->getSelectedObject().isEmpty())
	{
		debug("addToSelectedObjectInfoString WARNING - no object selected");
		return;
	}

	StelObjectP obj = omgr->getSelectedObject()[0];
	if (obj)
	{
		if (replace)
			obj->setExtraInfoString(StelObject::Script, str);
		else
			obj->addToExtraInfoString(StelObject::Script, str);
	}
}



void StelMainScriptAPI::clear(const QString& state)
{
	static const QMap<QString, int>stateMap={
		{ "natural",   1},
		{ "starchart", 2},
		{ "deepspace", 3},
		{ "galactic",  4},
		{ "supergalactic", 5 }};
	const int stateInt = stateMap.value(state.toLower(), 0);
	if (stateInt == 0)
	{
		qWarning() << "WARNING clear(" << state << ") - state not known";
	}
	else
	{
		LandscapeMgr* lmgr = GETSTELMODULE(LandscapeMgr);
		SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);
		SporadicMeteorMgr* mmgr = GETSTELMODULE(SporadicMeteorMgr);
		StelSkyDrawer* skyd = StelApp::getInstance().getCore()->getSkyDrawer();
		ConstellationMgr* cmgr = GETSTELMODULE(ConstellationMgr);
		AsterismMgr* amgr = GETSTELMODULE(AsterismMgr);
		StarMgr* smgr = GETSTELMODULE(StarMgr);
		NebulaMgr* nmgr = GETSTELMODULE(NebulaMgr);
		GridLinesMgr* glmgr = GETSTELMODULE(GridLinesMgr);
		StelMovementMgr* movmgr = GETSTELMODULE(StelMovementMgr);
		ZodiacalLight* zl = GETSTELMODULE(ZodiacalLight);
		StelPropertyMgr* propMgr = StelApp::getInstance().getStelPropertyManager();

		// Hide artificial satellites through StelProperties to avoid crash if plugin was not loaded
		propMgr->setStelPropertyValue("Satellites.hintsVisible",   false);
		propMgr->setStelPropertyValue("Satellites.labelsVisible",  false);
		propMgr->setStelPropertyValue("Satellites.flagOrbitLines", false);

		// identical for all states
		glmgr->setFlagAllGrids(false);
		glmgr->setFlagAllLines(false);
		glmgr->setFlagAllPoints(false);
		ssmgr->setFlagHints(false);
		ssmgr->setFlagOrbits(false);
		ssmgr->setFlagMoonScale(false);
		ssmgr->setFlagMinorBodyScale(false);
		ssmgr->setFlagTrails(false);
		lmgr->setFlagCardinalsPoints(false);
		amgr->setFlagLines(false);
		amgr->setFlagLabels(false);
		amgr->setFlagRayHelpers(false);

		// applicable for most states
		skyd->setFlagTwinkle(false);
		skyd->setFlagLuminanceAdaptation(false);
		ssmgr->setFlagPlanets(false);
		mmgr->setZHR(0);
		cmgr->setFlagLines(false);
		cmgr->setFlagLabels(false);
		cmgr->setFlagBoundaries(false);
		cmgr->setFlagArt(false);
		smgr->setFlagLabels(false);
		ssmgr->setFlagLabels(false);
		lmgr->setFlagLandscape(false);
		lmgr->setFlagAtmosphere(false);
		lmgr->setFlagFog(false);
		nmgr->setFlagHints(false);
		zl->setFlagShow(false);

		switch (stateInt)
		{
			case 1: // natural
			{
				movmgr->setMountMode(StelMovementMgr::MountAltAzimuthal);
				skyd->setFlagTwinkle(true);
				skyd->setFlagLuminanceAdaptation(true);
				ssmgr->setFlagPlanets(true);
				mmgr->setZHR(10);
				lmgr->setFlagLandscape(true);
				lmgr->setFlagAtmosphere(true);
				lmgr->setFlagFog(true);
				zl->setFlagShow(true);
				break;
			}
			case 2: // starchart
			{
				movmgr->setMountMode(StelMovementMgr::MountEquinoxEquatorial);
				ssmgr->setFlagPlanets(true);
				cmgr->setFlagLines(true);
				cmgr->setFlagLabels(true);
				cmgr->setFlagBoundaries(true);
				smgr->setFlagLabels(true);
				ssmgr->setFlagLabels(true);
				nmgr->setFlagHints(true);
				glmgr->setFlagEquatorGrid(true);
				break;
			}
			case 3: // deepspace
				movmgr->setMountMode(StelMovementMgr::MountEquinoxEquatorial);
				break;
			case 4: // galactic
			{
				movmgr->setMountMode(StelMovementMgr::MountGalactic);
				glmgr->setFlagGalacticGrid(true);
				break;
			}
			case 5: // supergalactic
			{
				movmgr->setMountMode(StelMovementMgr::MountSupergalactic);
				glmgr->setFlagSupergalacticGrid(true);
				break;
			}
		}
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
	const Vec3d& current = StelApp::getInstance().getCore()->j2000ToEquinoxEqu(GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000(), StelCore::RefractionOff);
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	// returned RA angle is in range -PI .. PI, but we want 0 .. 360
	return std::fmod((ra*180/M_PI)+360., 360.); // convert to degrees from radians
}

double StelMainScriptAPI::getViewDecAngle()
{
	const Vec3d& current = StelApp::getInstance().getCore()->j2000ToEquinoxEqu(GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000(), StelCore::RefractionOff);
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

void StelMainScriptAPI::moveToObject(const QString& name, float duration)
{
	if (name.isEmpty())
		return;

	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	StelObjectP obj = omgr->searchByName(name);

	if (!obj.isNull())
		mvmgr->moveToObject(obj, duration);
}

void StelMainScriptAPI::moveToSelectedObject(float duration)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	if (omgr->getSelectedObject().isEmpty())
		return;

	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	mvmgr->moveToObject(omgr->getSelectedObject()[0], duration); // Object may be without English name
}

void StelMainScriptAPI::moveToAltAzi(const QString& alt, const QString& azi, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);
	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aim;
	double dAlt = StelUtils::getDecAngle(alt);	
	double dAzi = M_PI - StelUtils::getDecAngle(azi);

	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		dAzi -= M_PI;

	StelUtils::spheToRect(dAzi,dAlt,aim);

	// make up vector more stable:
	StelMovementMgr::MountMode mountMode=mvmgr->getMountMode();
	Vec3d aimUp;
	if ( (mountMode==StelMovementMgr::MountAltAzimuthal) && (fabs(dAlt)> (0.9*M_PI/2.0)) )
		aimUp=Vec3d(-cos(dAzi), -sin(dAzi), 0.) * (dAlt>0. ? 1. : -1.);
	else
		aimUp=Vec3d(0., 0., 1.);

	mvmgr->moveToAltAzi(aim, aimUp, duration);
}

void StelMainScriptAPI::moveToRaDec(const QString& ra, const QString& dec, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);
	StelCore* core = StelApp::getInstance().getCore();

	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aim;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);

	StelUtils::spheToRect(dRa,dDec,aim);
	// make up vector more stable:
	StelMovementMgr::MountMode mountMode=mvmgr->getMountMode();
	Vec3d aimUp;
	if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(dDec)> (0.9*M_PI/2.0)) )
		aimUp=core->equinoxEquToJ2000(Vec3d(-cos(dRa), -sin(dRa), 0.) * (dDec>0. ? 1. : -1. ), StelCore::RefractionOff);
	else
		aimUp=core->equinoxEquToJ2000(Vec3d(0., 0., 1.), StelCore::RefractionOff);

	mvmgr->moveToJ2000(StelApp::getInstance().getCore()->equinoxEquToJ2000(aim, StelCore::RefractionOff), aimUp, duration);
}

void StelMainScriptAPI::moveToRaDecJ2000(const QString& ra, const QString& dec, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);
	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aimJ2000;
	double dRa = StelUtils::getDecAngle(ra);
	double dDec = StelUtils::getDecAngle(dec);

	StelUtils::spheToRect(dRa,dDec,aimJ2000);	
	// make up vector more stable. Not sure if we have to set the up vector in this case though.
	StelMovementMgr::MountMode mountMode=mvmgr->getMountMode();
	Vec3d aimUp;
	if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(dDec)> (0.9*M_PI/2.0)) )
		aimUp=Vec3d(-cos(dRa), -sin(dRa), 0.) * (dDec>0. ? 1. : -1. );
	else
		aimUp=Vec3d(0., 0., 1.);

	mvmgr->moveToJ2000(aimJ2000, aimUp, duration);
}

void StelMainScriptAPI::moveToGalLongLat(const QString& lon, const QString& lat, float duration)
{
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mvmgr);
	GETSTELMODULE(StelObjectMgr)->unSelect();

	Vec3d aimJ2000;
	double dRa = StelUtils::getDecAngle(lon);
	double dDec = StelUtils::getDecAngle(lat);

	StelUtils::spheToRect(dRa,dDec,aimJ2000);
	aimJ2000=StelApp::getInstance().getCore()->galacticToJ2000(aimJ2000);
	// make up vector more stable. Not sure if we have to set the up vector in this case though.
	StelMovementMgr::MountMode mountMode=mvmgr->getMountMode();
	Vec3d aimUp;
	if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(dDec)> (0.9*M_PI/2.0)) )
		aimUp=Vec3d(-cos(dRa), -sin(dRa), 0.) * (dDec>0. ? 1. : -1. );
	else
		aimUp=Vec3d(0., 0., 1.);

	mvmgr->moveToJ2000(aimJ2000, aimUp, duration);
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

QString StelMainScriptAPI::translate(QString englishText)
{
	return StelApp::getInstance().getLocaleMgr().getScriptsTranslator().qtranslate(englishText);
}

void StelMainScriptAPI::goHome()
{
	emit(requestSetHomePosition());
}

int StelMainScriptAPI::getBortleScaleIndex()
{
	return StelApp::getInstance().getCore()->getSkyDrawer()->getBortleScaleIndex();
}

void StelMainScriptAPI::setBortleScaleIndex(int index)
{
	StelApp::getInstance().getCore()->getSkyDrawer()->setBortleScaleIndex(index);
}

double StelMainScriptAPI::refraction(double altitude, bool apparent)
{
	Vec3d pos(1., 0., 0.);
	// rotate to set altitude.
	pos=Mat4d::yrotation(-altitude*M_PI_180)*pos;

	const Refraction refraction=StelApp::getInstance().getCore()->getSkyDrawer()->getRefraction();
	if (apparent)
	{
		refraction.backward(pos);
	}
	else
	{
		refraction.forward(pos);
	}
	return asin(pos[2])*M_180_PI;
}

QVariantMap StelMainScriptAPI::getScreenXYFromAltAzi(const QString &alt, const QString &azi)
{
	Vec3d aim, v;
	double dAlt = StelUtils::getDecAngle(alt);
	double dAzi = M_PI - StelUtils::getDecAngle(azi);

	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		dAzi -= M_PI;

	StelUtils::spheToRect(dAzi,dAlt,aim);

	const StelProjectorP prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

	prj->project(aim, v);

	QVariantMap map;
	map.insert("x", qRound(v[0]));
	map.insert("y", prj->getViewportHeight()-qRound(v[1]));

	return map;
}

QString StelMainScriptAPI::getEnv(const QString &var)
{
#if QT_VERSION >= 0x050A00
	return qEnvironmentVariable(var.toLocal8Bit().constData());
#else
	return QString::fromLocal8Bit(qgetenv(var.toLocal8Bit().constData()));
#endif
}

// return whether a particular module has been loaded. Mostly useful to check whether a module available as plugin is active.
bool StelMainScriptAPI::isModuleLoaded(const QString &moduleID)
{
	StelModule *module= StelApp::getInstance().getModuleMgr().getModule (moduleID, true);
	return module != Q_NULLPTR;
}

// return the name of platform where running Stellarium
QString StelMainScriptAPI::getPlatformName(void)
{
	// Get info about operating system
	QString os = StelUtils::getOperatingSystemInfo();
	if (os.contains("FreeBSD", Qt::CaseInsensitive))
		os = "FreeBSD";
	else if (os.contains("NetBSD", Qt::CaseInsensitive))
		os = "NetBSD";
	else if (os.contains("OpenBSD", Qt::CaseInsensitive))
		os = "OpenBSD";
	else if (os.contains("linux", Qt::CaseInsensitive) || QSysInfo::kernelType().contains("linux", Qt::CaseInsensitive))
		os = "Linux";
	else if (os.contains("windows", Qt::CaseInsensitive) || os.contains("winrt", Qt::CaseInsensitive))
		os = "Windows";
	else if (os.contains("osx", Qt::CaseInsensitive) || os.contains("macos", Qt::CaseInsensitive))
		os = "macOS";
	else
		os = "Unknown";

	return os;
}

// Get the current status of media playback support
bool StelMainScriptAPI::isMediaPlaybackSupported(void)
{
	#ifdef ENABLE_MEDIA
	return true;
	#else
	return false;
	#endif
}
