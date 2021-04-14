/*
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2012 Matthew Gates
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

#include "StelCore.hpp"

#include "StelProjector.hpp"
#include "StelProjectorClasses.hpp"
#include "StelToneReproducer.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelMovementMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelPainter.hpp"
#include "StelLocationMgr.hpp"
#include "StelObserver.hpp"
#include "StelObjectMgr.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "LandscapeMgr.hpp"
#include "StelTranslator.hpp"
#include "StelActionMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelMainView.hpp"
#include "EphemWrapper.hpp"
#include "NomenclatureItem.hpp"
#include "precession.h"

#include <QSettings>
#include <QDebug>
#include <QMetaEnum>
#include <QTimeZone>
#include <QFile>
#include <QDir>

#include <iostream>
#include <fstream>

// Init static transfo matrices
// See vsop87.doc:
const Mat4d StelCore::matJ2000ToVsop87(Mat4d::xrotation(-23.4392803055555555556*M_PI_180) * Mat4d::zrotation(0.0000275*M_PI_180));
const Mat4d StelCore::matVsop87ToJ2000(matJ2000ToVsop87.transpose());
const Mat4d StelCore::matJ2000ToGalactic(-0.054875539726, 0.494109453312, -0.867666135858, 0, -0.873437108010, -0.444829589425, -0.198076386122, 0, -0.483834985808, 0.746982251810, 0.455983795705, 0, 0, 0, 0, 1);
const Mat4d StelCore::matGalacticToJ2000(matJ2000ToGalactic.transpose());
const Mat4d StelCore::matJ2000ToSupergalactic(0.37501548, -0.89832046, 0.22887497, 0, 0.34135896, -0.09572714, -0.93504565, 0, 0.86188018, 0.42878511, 0.27075058, 0, 0, 0, 0, 1);
const Mat4d StelCore::matSupergalacticToJ2000(matJ2000ToSupergalactic.transpose());
Mat4d StelCore::matJ2000ToJ1875; // gets to be initialized in constructor.

const double StelCore::JD_SECOND = 0.000011574074074074074074;	// 1/(24*60*60)=1/86400
const double StelCore::JD_MINUTE = 0.00069444444444444444444;	// 1/(24*60)   =1/1440
const double StelCore::JD_HOUR   = 0.041666666666666666666;	// 1/24
const double StelCore::JD_DAY    = 1.;
const double StelCore::ONE_OVER_JD_SECOND = 86400;		// 86400
const double StelCore::TZ_ERA_BEGINNING = 2395996.5;		// December 1, 1847

StelCore::StelCore()
	: skyDrawer(Q_NULLPTR)
	, movementMgr(Q_NULLPTR)
	, propMgr(Q_NULLPTR)
	, geodesicGrid(Q_NULLPTR)
	, currentProjectionType(ProjectionStereographic)
	, currentDeltaTAlgorithm(EspenakMeeus)
	, position(Q_NULLPTR)
	, flagUseNutation(true)
	, flagUseTopocentricCoordinates(true)	
	, timeSpeed(JD_SECOND)
	, JD(0.,0.)
	, presetSkyTime(0.)
	, milliSecondsOfLastJDUpdate(0)
	, jdOfLastJDUpdate(0.)
	, flagUseDST(true)
	, flagUseCTZ(false)
	, deltaTCustomNDot(-26.0)
	, deltaTCustomYear(1820.0)
	, deltaTnDot(-26.0)
	, deltaTdontUseMoon(false)
	, deltaTfunc(StelUtils::getDeltaTByEspenakMeeus)
	, deltaTstart(-1999)
	, deltaTfinish(3000)
	, de430Available(false)
	, de431Available(false)
	, de430Active(false)
	, de431Active(false)
{
	setObjectName("StelCore");
	registerMathMetaTypes();

	toneReproducer = new StelToneReproducer();
	milliSecondsOfLastJDUpdate = QDateTime::currentMSecsSinceEpoch();

	QSettings* conf = StelApp::getInstance().getSettings();
	// Create and initialize the default projector params
	QString tmpstr = conf->value("projection/viewport").toString();
	currentProjectorParams.maskType = StelProjector::stringToMaskType(tmpstr);
	const int viewport_width = conf->value("projection/viewport_width", currentProjectorParams.viewportXywh[2]).toInt();
	const int viewport_height = conf->value("projection/viewport_height", currentProjectorParams.viewportXywh[3]).toInt();
	const int viewport_x = conf->value("projection/viewport_x", 0).toInt();
	const int viewport_y = conf->value("projection/viewport_y", 0).toInt();
	currentProjectorParams.viewportXywh.set(viewport_x,viewport_y,viewport_width,viewport_height);

	const qreal viewportCenterX = conf->value("projection/viewport_center_x",0.5*viewport_width).toDouble();
	const qreal viewportCenterY = conf->value("projection/viewport_center_y",0.5*viewport_height).toDouble();
	currentProjectorParams.viewportCenter.set(viewportCenterX, viewportCenterY);
	const qreal viewportCenterOffsetX = conf->value("projection/viewport_center_offset_x",0.).toDouble();
	const qreal viewportCenterOffsetY = conf->value("projection/viewport_center_offset_y",0.).toDouble();
	currentProjectorParams.viewportCenterOffset.set(viewportCenterOffsetX, viewportCenterOffsetY);

	currentProjectorParams.viewportFovDiameter = conf->value("projection/viewport_fov_diameter", qMin(viewport_width,viewport_height)).toDouble();
	currentProjectorParams.flipHorz = conf->value("projection/flip_horz",false).toBool();
	currentProjectorParams.flipVert = conf->value("projection/flip_vert",false).toBool();

	currentProjectorParams.gravityLabels = conf->value("viewing/flag_gravity_labels").toBool();
	
	currentProjectorParams.devicePixelsPerPixel = StelApp::getInstance().getDevicePixelsPerPixel();

	flagUseNutation=conf->value("astro/flag_nutation", true).toBool();
	flagUseTopocentricCoordinates=conf->value("astro/flag_topocentric_coordinates", true).toBool();
	flagUseDST=conf->value("localization/flag_dst", true).toBool();

	// Initialize matJ2000ToJ1875 matrix
	double eps1875, chi1875, omega1875, psi1875;
	const double jdB1875 = StelUtils::getJDFromBesselianEpoch(1875.0);
	getPrecessionAnglesVondrak(jdB1875, &eps1875, &chi1875, &omega1875, &psi1875);
	matJ2000ToJ1875 = Mat4d::xrotation(84381.406*1./3600.*M_PI/180.) * Mat4d::zrotation(-psi1875) * Mat4d::xrotation(-omega1875) * Mat4d::zrotation(chi1875);
	matJ2000ToJ1875 = matJ2000ToJ1875.transpose();
}


StelCore::~StelCore()
{
	delete toneReproducer; toneReproducer=Q_NULLPTR;
	delete geodesicGrid; geodesicGrid=Q_NULLPTR;
	delete skyDrawer; skyDrawer=Q_NULLPTR;
	delete position; position=Q_NULLPTR;
}

/*************************************************************************
 Load core data and initialize with default values
*************************************************************************/
void StelCore::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();

	if (conf->childGroups().contains("location_run_once"))
		defaultLocationID = "stellarium_cli";
	else
		defaultLocationID = conf->value("init_location/location", "auto").toString();
	bool ok;
	StelLocationMgr* locationMgr = &StelApp::getInstance().getLocationMgr();
	StelLocation location=locationMgr->getLastResortLocation(); // first location: Paris. Required if no IP connection on first launch!
	if (defaultLocationID == "auto")
	{
		locationMgr->locationFromIP();
	}
	else if (defaultLocationID == "stellarium_cli")
	{
		location = locationMgr->locationFromCLI();
	}
	else if (defaultLocationID.startsWith("GPS", Qt::CaseInsensitive))
	{
		// The location is obtained already from init_location/last_resort_location
		location.name = conf->value("init_location/location").toString();
	}
	else
	{
		location = locationMgr->locationForString(defaultLocationID);
	}

	if (!location.isValid())
	{
		qWarning() << "Warning: location" << defaultLocationID << "is unknown.";
		location = locationMgr->getLastResortLocation();
	}
	position = new StelObserver(location);

	QString ctz = conf->value("localization/time_zone", "").toString();
	if (!ctz.isEmpty())
		setUseCustomTimeZone(true);
	else
		ctz = getCurrentLocation().ianaTimeZone;
	setCurrentTimeZone(ctz);

	// Delta-T stuff
	// Define default algorithm for time correction (Delta T)
	QString tmpDT = conf->value("navigation/time_correction_algorithm", "EspenakMeeus").toString();
	setCurrentDeltaTAlgorithmKey(tmpDT);

	// Define variables of custom equation for calculation of Delta T
	// Default: ndot = -26.0 "/cy/cy; year = 1820; DeltaT = -20 + 32*u^2, where u = (currentYear-1820)/100
	setDeltaTCustomYear(conf->value("custom_time_correction/year", 1820.0).toDouble());
	setDeltaTCustomNDot(conf->value("custom_time_correction/ndot", -26.0).toDouble());
	setDeltaTCustomEquationCoefficients(Vec3d(conf->value("custom_time_correction/coefficients", "-20,0,32").toString()));

	// Time stuff
	setTimeNow();

	// We want to be able to handle the old style preset time, recorded as a double
	// jday, or as a more human readable string...
	QString presetTimeStr = conf->value("navigation/preset_sky_time",2451545.).toString();
	presetSkyTime = presetTimeStr.toDouble(&ok);
	if (ok)
	{
		qDebug() << "navigation/preset_sky_time is a double - treating as jday:" << QString::number(presetSkyTime, 'f', 5);
	}
	else
	{
		qDebug() << "navigation/preset_sky_time was not a double, treating as string date:" << presetTimeStr;
		presetSkyTime = StelUtils::qDateTimeToJd(QDateTime::fromString(presetTimeStr));
	}
	setInitTodayTime(QTime::fromString(conf->value("navigation/today_time", "22:00").toString()));
	startupTimeMode = conf->value("navigation/startup_time_mode", "actual").toString().toLower();
	if (startupTimeMode=="preset")	
		setJD(presetSkyTime - static_cast<double>(getUTCOffset(presetSkyTime)) * JD_HOUR);
	else if (startupTimeMode=="today")
		setTodayTime(getInitTodayTime());

	// Compute transform matrices between coordinates systems
	updateTransformMatrices();

	movementMgr = new StelMovementMgr(this);
	movementMgr->init();
	currentProjectorParams.fov = static_cast<float>(movementMgr->getInitFov());
	StelApp::getInstance().getModuleMgr().registerModule(movementMgr);

	skyDrawer = new StelSkyDrawer(this);
	skyDrawer->init();

	propMgr = StelApp::getInstance().getStelPropertyManager();
	propMgr->registerObject(skyDrawer);
	propMgr->registerObject(this);

	setCurrentProjectionTypeKey(getDefaultProjectionTypeKey());
	updateMaximumFov();

	// activate DE430/431
	initEphemeridesFunctions();

	// Register all the core actions.
	QString timeGroup = N_("Date and Time");
	QString movementGroup = N_("Movement and Selection");
	QString displayGroup = N_("Display Options");
	StelActionMgr* actionsMgr = StelApp::getInstance().getStelActionManager();
	actionsMgr->addAction("actionIncrease_Time_Speed", timeGroup, N_("Increase time speed"), this, "increaseTimeSpeed()", "L");
	actionsMgr->addAction("actionDecrease_Time_Speed", timeGroup, N_("Decrease time speed"), this, "decreaseTimeSpeed()", "J");
	actionsMgr->addAction("actionIncrease_Time_Speed_Less", timeGroup, N_("Increase time speed (a little)"), this, "increaseTimeSpeedLess()", "Shift+L");
	actionsMgr->addAction("actionDecrease_Time_Speed_Less", timeGroup, N_("Decrease time speed (a little)"), this, "decreaseTimeSpeedLess()", "Shift+J");
	actionsMgr->addAction("actionSet_Real_Time_Speed", timeGroup, N_("Set normal time rate"), this, "toggleRealTimeSpeed()", "K");
	actionsMgr->addAction("actionSet_Time_Rate_Zero", timeGroup, N_("Set time rate to zero"), this, "setZeroTimeSpeed()", "7");
	actionsMgr->addAction("actionSet_Time_Reverse", timeGroup, N_("Set reverse time direction"), this, "revertTimeDirection()", "0");
	actionsMgr->addAction("actionReturn_To_Current_Time", timeGroup, N_("Set time to now"), this, "setTimeNow()", "8");
	actionsMgr->addAction("actionAdd_Solar_Minute", timeGroup, N_("Add 1 solar minute"), this, "addMinute()");
	actionsMgr->addAction("actionAdd_Solar_Hour", timeGroup, N_("Add 1 solar hour"), this, "addHour()", "Ctrl+=");
	actionsMgr->addAction("actionAdd_Solar_Day", timeGroup, N_("Add 1 solar day"), this, "addDay()", "=");
	actionsMgr->addAction("actionAdd_Solar_Week", timeGroup, N_("Add 7 solar days"), this, "addWeek()", "]");
	actionsMgr->addAction("actionSubtract_Solar_Minute", timeGroup, N_("Subtract 1 solar minute"), this, "subtractMinute()");
	actionsMgr->addAction("actionSubtract_Solar_Hour", timeGroup, N_("Subtract 1 solar hour"), this, "subtractHour()", "Ctrl+-");
	actionsMgr->addAction("actionSubtract_Solar_Day", timeGroup, N_("Subtract 1 solar day"), this, "subtractDay()", "-");
	actionsMgr->addAction("actionSubtract_Solar_Week", timeGroup, N_("Subtract 7 solar days"), this, "subtractWeek()", "[");
	actionsMgr->addAction("actionAdd_Sidereal_Day", timeGroup, N_("Add 1 sidereal day"), this, "addSiderealDay()", "Alt+=");
	actionsMgr->addAction("actionAdd_Sidereal_Week", timeGroup, N_("Add 7 sidereal days"), this, "addSiderealWeek()");
	actionsMgr->addAction("actionAdd_Sidereal_Year", timeGroup, N_("Add 1 sidereal year"), this, "addSiderealYear()", "Ctrl+Alt+Shift+]");
	actionsMgr->addAction("actionAdd_Sidereal_Century", timeGroup, N_("Add 100 sidereal years"), this, "addSiderealYears()");
	actionsMgr->addAction("actionAdd_Synodic_Month", timeGroup, N_("Add 1 synodic month"), this, "addSynodicMonth()");
	actionsMgr->addAction("actionAdd_Saros", timeGroup, N_("Add 1 saros"), this, "addSaros()");
	actionsMgr->addAction("actionAdd_Draconic_Month", timeGroup, N_("Add 1 draconic month"), this, "addDraconicMonth()");
	actionsMgr->addAction("actionAdd_Draconic_Year", timeGroup, N_("Add 1 draconic year"), this, "addDraconicYear()");
	actionsMgr->addAction("actionAdd_Anomalistic_Month", timeGroup, N_("Add 1 anomalistic month"), this, "addAnomalisticMonth()");
	actionsMgr->addAction("actionAdd_Anomalistic_Year", timeGroup, N_("Add 1 anomalistic year"), this, "addAnomalisticYear()");
	actionsMgr->addAction("actionAdd_Anomalistic_Century", timeGroup, N_("Add 100 anomalistic years"), this, "addAnomalisticYears()");
	actionsMgr->addAction("actionAdd_Mean_Tropical_Month", timeGroup, N_("Add 1 mean tropical month"), this, "addMeanTropicalMonth()");
	actionsMgr->addAction("actionAdd_Mean_Tropical_Year", timeGroup, N_("Add 1 mean tropical year"), this, "addMeanTropicalYear()");
	actionsMgr->addAction("actionAdd_Mean_Tropical_Century", timeGroup, N_("Add 100 mean tropical years"), this, "addMeanTropicalYears()");
	actionsMgr->addAction("actionAdd_Tropical_Year", timeGroup, N_("Add 1 tropical year"), this, "addTropicalYear()");
	actionsMgr->addAction("actionAdd_Julian_Year", timeGroup, N_("Add 1 Julian year"), this, "addJulianYear()");
	actionsMgr->addAction("actionAdd_Julian_Century", timeGroup, N_("Add 1 Julian century"), this, "addJulianYears()");
	actionsMgr->addAction("actionAdd_Gaussian_Year", timeGroup, N_("Add 1 Gaussian year"), this, "addGaussianYear()");
	actionsMgr->addAction("actionAdd_Calendric_Month", timeGroup, N_("Add 1 calendric month"), this, "addCalendricMonth()");
	actionsMgr->addAction("actionSubtract_Sidereal_Day", timeGroup, N_("Subtract 1 sidereal day"), this, "subtractSiderealDay()", "Alt+-");
	actionsMgr->addAction("actionSubtract_Sidereal_Week", timeGroup, N_("Subtract 7 sidereal days"), this, "subtractSiderealWeek()");
	actionsMgr->addAction("actionSubtract_Sidereal_Year", timeGroup, N_("Subtract 1 sidereal year"), this, "subtractSiderealYear()", "Ctrl+Alt+Shift+[");
	actionsMgr->addAction("actionSubtract_Sidereal_Century", timeGroup, N_("Subtract 100 sidereal years"), this, "subtractSiderealYears()");
	actionsMgr->addAction("actionSubtract_Synodic_Month", timeGroup, N_("Subtract 1 synodic month"), this, "subtractSynodicMonth()");
	actionsMgr->addAction("actionSubtract_Saros", timeGroup, N_("Subtract 1 saros"), this, "subtractSaros()");
	actionsMgr->addAction("actionSubtract_Draconic_Month", timeGroup, N_("Subtract 1 draconic month"), this, "subtractDraconicMonth()");
	actionsMgr->addAction("actionSubtract_Draconic_Year", timeGroup, N_("Subtract 1 draconic year"), this, "subtractDraconicYear()");
	actionsMgr->addAction("actionSubtract_Anomalistic_Month", timeGroup, N_("Subtract 1 anomalistic month"), this, "subtractAnomalisticMonth()");
	actionsMgr->addAction("actionSubtract_Anomalistic_Year", timeGroup, N_("Subtract 1 anomalistic year"), this, "subtractAnomalisticYear()");
	actionsMgr->addAction("actionSubtract_Anomalistic_Century", timeGroup, N_("Subtract 100 anomalistic years"), this, "subtractAnomalisticYears()");
	actionsMgr->addAction("actionSubtract_Mean_Tropical_Month", timeGroup, N_("Subtract 1 mean tropical month"), this, "subtractMeanTropicalMonth()");
	actionsMgr->addAction("actionSubtract_Mean_Tropical_Year", timeGroup, N_("Subtract 1 mean tropical year"), this, "subtractMeanTropicalYear()");
	actionsMgr->addAction("actionSubtract_Mean_Tropical_Century", timeGroup, N_("Subtract 100 mean tropical years"), this, "subtractMeanTropicalYears()");
	actionsMgr->addAction("actionSubtract_Tropical_Year", timeGroup, N_("Subtract 1 tropical year"), this, "subtractTropicalYear()");
	actionsMgr->addAction("actionSubtract_Julian_Year", timeGroup, N_("Subtract 1 Julian year"), this, "subtractJulianYear()");
	actionsMgr->addAction("actionSubtract_Julian_Century", timeGroup, N_("Subtract 1 Julian century"), this, "subtractJulianYears()");
	actionsMgr->addAction("actionSubtract_Gaussian_Year", timeGroup, N_("Subtract 1 Gaussian year"), this, "subtractGaussianYear()");
	actionsMgr->addAction("actionSubtract_Calendric_Month", timeGroup, N_("Subtract 1 calendric month"), this, "subtractCalendricMonth()");

	actionsMgr->addAction("actionSet_Home_Planet_To_Selected", movementGroup, N_("Set home planet to selected planet"), this, "moveObserverToSelected()", "Ctrl+G");
	actionsMgr->addAction("actionGo_Home_Global", movementGroup, N_("Go to home"), this, "returnToHome()", "Ctrl+H");

	actionsMgr->addAction("actionHorizontal_Flip", displayGroup, N_("Flip scene horizontally"), this, "flipHorz", "Ctrl+Shift+H", "", true);
	actionsMgr->addAction("actionVertical_Flip", displayGroup, N_("Flip scene vertically"), this, "flipVert", "Ctrl+Shift+V", "", true);
}

QString StelCore::getDefaultProjectionTypeKey() const
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("projection/type", "ProjectionStereographic").toString();
}

// Get the shared instance of StelGeodesicGrid.
// The returned instance is garanteed to allow for at least maxLevel levels
const StelGeodesicGrid* StelCore::getGeodesicGrid(int maxLevel) const
{
	if (geodesicGrid==Q_NULLPTR)
	{
		geodesicGrid = new StelGeodesicGrid(maxLevel);
	}
	else if (maxLevel>geodesicGrid->getMaxLevel())
	{
		delete geodesicGrid;
		geodesicGrid = new StelGeodesicGrid(maxLevel);
	}
	return geodesicGrid;
}

StelProjectorP StelCore::getProjection2d() const
{
	StelProjectorP prj(new StelProjector2d());
	prj->init(currentProjectorParams);
	return prj;
}

StelProjectorP StelCore::getProjection(StelProjector::ModelViewTranformP modelViewTransform, ProjectionType projType) const
{
	if (projType==1000)
		projType = currentProjectionType;

	StelProjectorP prj;
	switch (projType)
	{
		case ProjectionPerspective:
			prj = StelProjectorP(new StelProjectorPerspective(modelViewTransform));
			break;
		case ProjectionEqualArea:
			prj = StelProjectorP(new StelProjectorEqualArea(modelViewTransform));
			break;
		case ProjectionStereographic:
			prj = StelProjectorP(new StelProjectorStereographic(modelViewTransform));
			break;
		case ProjectionFisheye:
			prj = StelProjectorP(new StelProjectorFisheye(modelViewTransform));
			break;
		case ProjectionHammer:
			prj = StelProjectorP(new StelProjectorHammer(modelViewTransform));
			break;
		case ProjectionCylinder:
			prj = StelProjectorP(new StelProjectorCylinder(modelViewTransform));
			break;
		case ProjectionMercator:
			prj = StelProjectorP(new StelProjectorMercator(modelViewTransform));
			break;
		case ProjectionOrthographic:
			prj = StelProjectorP(new StelProjectorOrthographic(modelViewTransform));
			break;
		case ProjectionSinusoidal:
			prj = StelProjectorP(new StelProjectorSinusoidal(modelViewTransform));
			break;
		case ProjectionMiller:
			prj = StelProjectorP(new StelProjectorMiller(modelViewTransform));
			break;
		default:
			qWarning() << "Unknown projection type: " << static_cast<int>(projType) << "using ProjectionStereographic instead";
			prj = StelProjectorP(new StelProjectorStereographic(modelViewTransform));
			Q_ASSERT(0);
	}
	prj->init(currentProjectorParams);
	return prj;
}

// Get an instance of projector using the current display parameters from Navigation, StelMovementMgr
StelProjectorP StelCore::getProjection(FrameType frameType, RefractionMode refractionMode) const
{
	switch (frameType)
	{
		case FrameAltAz:
			return getProjection(getAltAzModelViewTransform(refractionMode));
		case FrameHeliocentricEclipticJ2000:
			return getProjection(getHeliocentricEclipticModelViewTransform(refractionMode));
		case FrameObservercentricEclipticJ2000:
			return getProjection(getObservercentricEclipticJ2000ModelViewTransform(refractionMode));
		case FrameObservercentricEclipticOfDate:
			return getProjection(getObservercentricEclipticOfDateModelViewTransform(refractionMode));
		case FrameEquinoxEqu:
			return getProjection(getEquinoxEquModelViewTransform(refractionMode));
		case FrameJ2000:
			return getProjection(getJ2000ModelViewTransform(refractionMode));
		case FrameGalactic:
			return getProjection(getGalacticModelViewTransform(refractionMode));
		case FrameSupergalactic:
			return getProjection(getSupergalacticModelViewTransform(refractionMode));
		default:
			qDebug() << "Unknown reference frame type: " << static_cast<int>(frameType) << ".";
	}
	Q_ASSERT(0);
	return getProjection2d();
}

SphericalCap StelCore::getVisibleSkyArea() const
{
	const LandscapeMgr* landscapeMgr = GETSTELMODULE(LandscapeMgr);
	Vec3d up(0, 0, 1);
	up = altAzToJ2000(up, RefractionOff);
	
	// Limit star drawing to above landscape's minimal altitude (was const=-0.035, Bug lp:1469407)
	if (landscapeMgr->getIsLandscapeFullyVisible())
	{
		return SphericalCap(up, landscapeMgr->getLandscapeSinMinAltitudeLimit());
	}
	return SphericalCap(up, -1.);
}

// Handle the resizing of the window
void StelCore::windowHasBeenResized(qreal x, qreal y, qreal width, qreal height)
{
	// Maximize display when resized since it invalidates previous options anyway
	currentProjectorParams.viewportXywh.set(qRound(x), qRound(y), qRound(width), qRound(height));
	currentProjectorParams.viewportCenter.set(x+(0.5+currentProjectorParams.viewportCenterOffset.v[0])*width, y+(0.5+currentProjectorParams.viewportCenterOffset.v[1])*height);
	currentProjectorParams.viewportFovDiameter = qMin(width,height);
}

/*************************************************************************
 Update all the objects in function of the time
*************************************************************************/
void StelCore::update(double deltaTime)
{
	// Update the position of observation and time and recompute planet positions etc...
	updateTime(deltaTime);

	// Transform matrices between coordinates systems
	updateTransformMatrices();

	// Update direction of vision/Zoom level
	movementMgr->updateMotion(deltaTime);

	currentProjectorParams.fov = static_cast<float>(movementMgr->getCurrentFov());

	skyDrawer->update(deltaTime);
}


/*************************************************************************
 Execute all the pre-drawing functions
*************************************************************************/
void StelCore::preDraw()
{
	// Init openGL viewing with fov, screen size and clip planes
	currentProjectorParams.zNear = 0.000001;
	currentProjectorParams.zFar = 500.;

	// Clear the render buffer.
	// Here we can set a sky background color if really wanted (art
	// applications. Astronomical sky should be 0/0/0/0)
	Vec3f backColor = StelMainView::getInstance().getSkyBackgroundColor();
	QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
	gl->glClearColor(backColor[0], backColor[1], backColor[2], 0.f);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	skyDrawer->preDraw();
}


/*************************************************************************
 Update core state after drawing modules
*************************************************************************/
void StelCore::postDraw()
{
	StelPainter sPainter(getProjection(StelCore::FrameJ2000));
	sPainter.drawViewportShape();
}

void StelCore::updateMaximumFov()
{
	const float savedFov = currentProjectorParams.fov;
	currentProjectorParams.fov = 0.0001f;	// Avoid crash
	const float newMaxFov = getProjection(StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity())))->getMaxFov();
	movementMgr->setMaxFov(static_cast<double>(newMaxFov));
	currentProjectorParams.fov = qMin(newMaxFov, savedFov);
}

void StelCore::setCurrentProjectionType(ProjectionType type)
{
	if(type!=currentProjectionType)
	{
		currentProjectionType=type;
		updateMaximumFov();

		emit currentProjectionTypeChanged(type);
		emit currentProjectionTypeKeyChanged(getCurrentProjectionTypeKey());
		emit currentProjectionNameI18nChanged(getCurrentProjectionNameI18n());
	}
}

StelCore::ProjectionType StelCore::getCurrentProjectionType() const
{
	return currentProjectionType;
}

//! Set the current projection type to use
void StelCore::setCurrentProjectionTypeKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	ProjectionType newType = static_cast<ProjectionType>(en.keyToValue(key.toLatin1().data()));
	if (newType<0)
	{
		qWarning() << "Unknown projection type: " << key << "setting \"ProjectionStereographic\" instead";
		newType = ProjectionStereographic;
	}
	setCurrentProjectionType(newType);
}

//! Get the current Mapping used by the Projection
QString StelCore::getCurrentProjectionTypeKey(void) const
{
	return metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType")).key(currentProjectionType);
}

QString StelCore::getCurrentProjectionNameI18n() const
{
	return projectionTypeKeyToNameI18n(getCurrentProjectionTypeKey());
}

//! Get the list of all the available projections
QStringList StelCore::getAllProjectionTypeKeys() const
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	QStringList l;
	for (int i=0;i<en.keyCount();++i)
		l << en.key(i);
	return l;
}

void StelCore::setMaskType(StelProjector::StelProjectorMaskType m)
{
	currentProjectorParams.maskType = m;
}

void StelCore::setFlagGravityLabels(bool gravity)
{
	currentProjectorParams.gravityLabels = gravity;
	emit flagGravityLabelsChanged(gravity);
}

bool StelCore::getFlagGravityLabels() const
{
	return currentProjectorParams.gravityLabels;
}

void StelCore::setDefaultAngleForGravityText(float a)
{
	currentProjectorParams.defaultAngleForGravityText = a;
}

void StelCore::setFlipHorz(bool flip)
{
	if (currentProjectorParams.flipHorz != flip)
	{
		currentProjectorParams.flipHorz = flip;
		emit flipHorzChanged(flip);
	}
}

void StelCore::setFlipVert(bool flip)
{
	if (currentProjectorParams.flipVert != flip)
	{
		currentProjectorParams.flipVert = flip;
		emit flipVertChanged(flip);
	}
}

bool StelCore::getFlipHorz(void) const
{
	return currentProjectorParams.flipHorz;
}

bool StelCore::getFlipVert(void) const
{
	return currentProjectorParams.flipVert;
}

// Get current value for horizontal viewport offset [-50...50]
double StelCore::getViewportHorizontalOffset(void) const
{
	return (currentProjectorParams.viewportCenterOffset[0] * 100.0);
}
// Set horizontal viewport offset. Argument will be clamped to be inside [-50...50]
void StelCore::setViewportHorizontalOffset(double newOffsetPct)
{
	currentProjectorParams.viewportCenterOffset[0]=0.01* qBound(-50., newOffsetPct, 50.);
	currentProjectorParams.viewportCenter.set(currentProjectorParams.viewportXywh[0]+(0.5+currentProjectorParams.viewportCenterOffset.v[0])*currentProjectorParams.viewportXywh[2],
						currentProjectorParams.viewportXywh[1]+(0.5+currentProjectorParams.viewportCenterOffset.v[1])*currentProjectorParams.viewportXywh[3]);
}

// Get current value for vertical viewport offset [-50...50]
double StelCore::getViewportVerticalOffset(void) const
{
	return (currentProjectorParams.viewportCenterOffset[1] * 100.0);
}
// Set vertical viewport offset. Argument will be clamped to be inside [-50...50]
void StelCore::setViewportVerticalOffset(double newOffsetPct)
{
	currentProjectorParams.viewportCenterOffset[1]=0.01* qBound(-50., newOffsetPct, 50.);
	currentProjectorParams.viewportCenter.set(currentProjectorParams.viewportXywh[0]+(0.5+currentProjectorParams.viewportCenterOffset.v[0])*currentProjectorParams.viewportXywh[2],
						currentProjectorParams.viewportXywh[1]+(0.5+currentProjectorParams.viewportCenterOffset.v[1])*currentProjectorParams.viewportXywh[3]);
}

// Set both viewport offsets. Arguments will be clamped to be inside [-50...50]. I (GZ) hope this will avoid some of the shaking.
void StelCore::setViewportOffset(double newHorizontalOffsetPct, double newVerticalOffsetPct)
{
	currentProjectorParams.viewportCenterOffset[0]=0.01* qBound(-50., newHorizontalOffsetPct, 50.);
	currentProjectorParams.viewportCenterOffset[1]=0.01* qBound(-50., newVerticalOffsetPct,   50.);
	currentProjectorParams.viewportCenter.set(currentProjectorParams.viewportXywh[0]+(0.5+currentProjectorParams.viewportCenterOffset.v[0])*currentProjectorParams.viewportXywh[2],
						currentProjectorParams.viewportXywh[1]+(0.5+currentProjectorParams.viewportCenterOffset.v[1])*currentProjectorParams.viewportXywh[3]);
}

void StelCore::setViewportStretch(float stretch)
{
	currentProjectorParams.widthStretch=static_cast<qreal>(qMax(0.001f, stretch));
}

QString StelCore::getDefaultLocationID() const
{
	return defaultLocationID;
}

QString StelCore::projectionTypeKeyToNameI18n(const QString& key) const
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	QString s(getProjection(StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity())), static_cast<ProjectionType>(en.keyToValue(key.toLatin1())))->getNameI18());
	return s;
}

QString StelCore::projectionNameI18nToTypeKey(const QString& nameI18n) const
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	for (int i=0;i<en.keyCount();++i)
	{
		if (getProjection(StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity())), static_cast<ProjectionType>(i))->getNameI18()==nameI18n)
			return en.valueToKey(i);
	}
	// Unknown translated name
	Q_ASSERT(0);
	return en.valueToKey(ProjectionStereographic);
}

StelProjector::StelProjectorParams StelCore::getCurrentStelProjectorParams() const
{
	return currentProjectorParams;
}

void StelCore::setCurrentStelProjectorParams(const StelProjector::StelProjectorParams& newParams)
{
	currentProjectorParams=newParams;
}

void StelCore::lookAtJ2000(const Vec3d& pos, const Vec3d& aup)
{
	Vec3d f(j2000ToAltAz(pos, RefractionOff));
	Vec3d up(j2000ToAltAz(aup, RefractionOff));
	f.normalize();
	up.normalize();

	// Update the model view matrix
	Vec3d s(f^up);	// y vector
	s.normalize();
	Vec3d u(s^f);	// Up vector in AltAz coordinates
	u.normalize();
	matAltAzModelView.set(s[0],u[0],-f[0],0.,
			      s[1],u[1],-f[1],0.,
			      s[2],u[2],-f[2],0.,
			      0.,0.,0.,1.);
	invertMatAltAzModelView = matAltAzModelView.inverse();
}

void StelCore::setMatAltAzModelView(const Mat4d& mat)
{
	matAltAzModelView = mat;
	invertMatAltAzModelView = matAltAzModelView.inverse();
}

Vec3d StelCore::altAzToEquinoxEqu(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matAltAzToEquinoxEqu*v;
	Vec3d r(v);
	skyDrawer->getRefraction().backward(r);
	r.transfo4d(matAltAzToEquinoxEqu);
	return r;
}

Vec3d StelCore::equinoxEquToAltAz(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matEquinoxEquToAltAz*v;
	Vec3d r(v);
	r.transfo4d(matEquinoxEquToAltAz);
	skyDrawer->getRefraction().forward(r);
	return r;
}

Vec3d StelCore::altAzToJ2000(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matEquinoxEquDateToJ2000*matAltAzToEquinoxEqu*v;
	Vec3d r(v);
	skyDrawer->getRefraction().backward(r);
	r.transfo4d(matEquinoxEquDateToJ2000*matAltAzToEquinoxEqu);
	return r;
}

Vec3d StelCore::j2000ToAltAz(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matJ2000ToAltAz*v;
	Vec3d r(v);
	r.transfo4d(matJ2000ToAltAz);
	skyDrawer->getRefraction().forward(r);
	return r;
}

Vec3d StelCore::galacticToJ2000(const Vec3d& v) const
{
	return matGalacticToJ2000*v;
}

Vec3d StelCore::supergalacticToJ2000(const Vec3d& v) const
{
	return matSupergalacticToJ2000*v;
}

Vec3d StelCore::equinoxEquToJ2000(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matEquinoxEquDateToJ2000*v;
	Vec3d r(v);
	r.transfo4d(matEquinoxEquToAltAz);
	skyDrawer->getRefraction().backward(r);
	r.transfo4d(matAltAzToJ2000);
	return r;
}

Vec3d StelCore::j2000ToEquinoxEqu(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matJ2000ToEquinoxEqu*v;
	Vec3d r(v);
	r.transfo4d(matJ2000ToAltAz);
	skyDrawer->getRefraction().forward(r);
	r.transfo4d(matAltAzToEquinoxEqu);
	return r;
}

Vec3d StelCore::j2000ToJ1875(const Vec3d& v) const
{
	return matJ2000ToJ1875*v;
}

Vec3d StelCore::j2000ToGalactic(const Vec3d& v) const
{
	return matJ2000ToGalactic*v;
}

Vec3d StelCore::j2000ToSupergalactic(const Vec3d& v) const
{
	return matJ2000ToSupergalactic*v;
}

//! Transform vector from heliocentric ecliptic coordinate to altazimuthal
Vec3d StelCore::heliocentricEclipticToAltAz(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matHeliocentricEclipticJ2000ToAltAz*v;
	Vec3d r(v);
	r.transfo4d(matHeliocentricEclipticJ2000ToAltAz);
	skyDrawer->getRefraction().forward(r);
	return r;
}

//! Transform from heliocentric coordinate to equatorial at current equinox (for the planet where the observer stands)
Vec3d StelCore::heliocentricEclipticToEquinoxEqu(const Vec3d& v) const
{
	return matHeliocentricEclipticToEquinoxEqu*v;
}

/*
//! Transform vector from heliocentric coordinate to false equatorial : equatorial
//! coordinate but centered on the observer position (useful for objects close to earth)
//! Unused as of V0.13
Vec3d StelCore::heliocentricEclipticToEarthPosEquinoxEqu(const Vec3d& v) const
{
	return matAltAzToEquinoxEqu*matHeliocentricEclipticToAltAz*v;
}
*/

StelProjector::ModelViewTranformP StelCore::getHeliocentricEclipticModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matHeliocentricEclipticJ2000ToAltAz));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matHeliocentricEclipticJ2000ToAltAz);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric ecliptic J2000 (Vsop87A) drawing
StelProjector::ModelViewTranformP StelCore::getObservercentricEclipticJ2000ModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matJ2000ToAltAz*matVsop87ToJ2000));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matJ2000ToAltAz*matVsop87ToJ2000);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric ecliptic-of-date drawing
StelProjector::ModelViewTranformP StelCore::getObservercentricEclipticOfDateModelViewTransform(RefractionMode refMode) const
{
	double eps_A=getPrecessionAngleVondrakCurrentEpsilonA();
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matEquinoxEquToAltAz* Mat4d::xrotation(eps_A)));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matEquinoxEquToAltAz* Mat4d::xrotation(eps_A));
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric equatorial at equinox drawing
StelProjector::ModelViewTranformP StelCore::getEquinoxEquModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matEquinoxEquToAltAz));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matEquinoxEquToAltAz);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric altazimuthal drawing
StelProjector::ModelViewTranformP StelCore::getAltAzModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
	{
		// Catch problem with improperly initialized matAltAzModelView
		Q_ASSERT(matAltAzModelView[0]==matAltAzModelView[0]);
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView));
	}
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric J2000 equatorial drawing
StelProjector::ModelViewTranformP StelCore::getJ2000ModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matEquinoxEquToAltAz*matJ2000ToEquinoxEqu));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matEquinoxEquToAltAz*matJ2000ToEquinoxEqu);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric Galactic equatorial drawing
StelProjector::ModelViewTranformP StelCore::getGalacticModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matGalacticToJ2000));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matGalacticToJ2000);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric Supergalactic equatorial drawing
StelProjector::ModelViewTranformP StelCore::getSupergalacticModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==Q_NULLPTR || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matSupergalacticToJ2000));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matSupergalacticToJ2000);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

// GZ: One of the most important functions, totally void of doc. :-(
// called in update() (for every frame)
void StelCore::updateTransformMatrices()
{
	matAltAzToEquinoxEqu = position->getRotAltAzToEquatorial(getJD(), getJDE());
	matEquinoxEquToAltAz = matAltAzToEquinoxEqu.transpose();

	// multiply static J2000 earth axis tilt (eclipticalJ2000<->equatorialJ2000)
	// in effect, this matrix transforms from VSOP87 ecliptical J2000 to planet-based equatorial coordinates.
	// For earth, matJ2000ToEquinoxEqu is the precession matrix.	
	matEquinoxEquDateToJ2000 = matVsop87ToJ2000 * position->getRotEquatorialToVsop87();
	matJ2000ToEquinoxEqu = matEquinoxEquDateToJ2000.transpose();
	matJ2000ToAltAz = matEquinoxEquToAltAz*matJ2000ToEquinoxEqu;
	matAltAzToJ2000 = matJ2000ToAltAz.transpose();

	matHeliocentricEclipticToEquinoxEqu = matJ2000ToEquinoxEqu * matVsop87ToJ2000 * Mat4d::translation(-position->getCenterVsop87Pos());

	// These two next have to take into account the position of the observer on the earth/planet of observation.	
	Mat4d matAltAzToVsop87 = matJ2000ToVsop87 * matEquinoxEquDateToJ2000 * matAltAzToEquinoxEqu;
	//Mat4d tmp1 = matJ2000ToVsop87 * matEquinoxEquDateToJ2000;

	// Before 0.14 getDistanceFromCenter assumed spherical planets. Now uses rectangular coordinates for observer!
	// In series 0.14 and 0.15, this was erroneous: offset by distance rho, but in the wrong direction.
	// Attempt to fix LP:1275092. This improves the situation, but is still not perfect.
	// Please keep the commented stuff until situation is really solved.
	if (flagUseTopocentricCoordinates)
	{
		const Vec4d offset=position->getTopographicOffsetFromCenter(); // [rho cosPhi', rho sinPhi', phi'_rad, rho]
		const double sigma=static_cast<double>(position->getCurrentLocation().latitude)*M_PI/180.0 - offset.v[2];
		const double rho=offset.v[3];

		matAltAzToHeliocentricEclipticJ2000 =  Mat4d::translation(position->getCenterVsop87Pos()) * matAltAzToVsop87 *
				Mat4d::translation(Vec3d(rho*sin(sigma), 0., rho*cos(sigma) ));

		matHeliocentricEclipticJ2000ToAltAz =
				Mat4d::translation(Vec3d(-rho*sin(sigma), 0., -rho*cos(sigma))) * matAltAzToVsop87.transpose() *
				Mat4d::translation(-position->getCenterVsop87Pos());

		// Here I tried to split tmp matrix. This does not work:
//		matAltAzToHeliocentricEclipticJ2000 =  Mat4d::translation(position->getCenterVsop87Pos()) * tmp1 *
//				Mat4d::translation(Vec3d(rho*sin(sigma), 0., rho*cos(sigma) )) * matAltAzToEquinoxEqu;

//		matHeliocentricEclipticJ2000ToAltAz =
//				matEquinoxEquToAltAz *
//				Mat4d::translation(Vec3d(-rho*sin(sigma), 0., -rho*cos(sigma))) * tmp1.transpose() *
//				Mat4d::translation(-position->getCenterVsop87Pos());


//		matAltAzToHeliocentricEclipticJ2000 =  Mat4d::translation(position->getCenterVsop87Pos()) * tmp *
//				Mat4d::translation(Vec3d(0.,0., position->getDistanceFromCenter()));

//		matHeliocentricEclipticJ2000ToAltAz =  Mat4d::translation(Vec3d(0.,0.,-position->getDistanceFromCenter())) * tmp.transpose() *
//				Mat4d::translation(-position->getCenterVsop87Pos());
	}
	else
	{
		matAltAzToHeliocentricEclipticJ2000 =  Mat4d::translation(position->getCenterVsop87Pos()) * matAltAzToVsop87;
		matHeliocentricEclipticJ2000ToAltAz =  matAltAzToVsop87.transpose() * Mat4d::translation(-position->getCenterVsop87Pos());
	}
}

// Return the observer heliocentric position
Vec3d StelCore::getObserverHeliocentricEclipticPos() const
{
	return Vec3d(matAltAzToHeliocentricEclipticJ2000[12], matAltAzToHeliocentricEclipticJ2000[13], matAltAzToHeliocentricEclipticJ2000[14]);
}

// Set the location to use by default at startup
void StelCore::setDefaultLocationID(const QString& id)
{
	StelLocation location = StelApp::getInstance().getLocationMgr().locationForString(id);
	if (!location.isValid())
	{
		qWarning() << "Trying to set an invalid location" << id;
		return;
	}
	defaultLocationID = id;
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("init_location/location", id);
}

void StelCore::returnToDefaultLocation()
{
	StelLocationMgr& locationMgr = StelApp::getInstance().getLocationMgr();
	StelLocation loc = locationMgr.locationForString(defaultLocationID);
	if (loc.isValid())
		moveObserverTo(loc, 0.);
}

void StelCore::returnToHome()
{
	// Using returnToDefaultLocation() and getCurrentLocation() introduce issue, because for flying
	// between planets using SpaceShip and second method give does not exist data
	StelLocationMgr& locationMgr = StelApp::getInstance().getLocationMgr();
	StelLocation loc;
	if (defaultLocationID == "auto")
	{
		locationMgr.locationFromIP();
		loc = locationMgr.getLastResortLocation();
	}
	else
		loc = locationMgr.locationForString(defaultLocationID);

	if (loc.isValid())
		moveObserverTo(loc, 0.);

	PlanetP p = GETSTELMODULE(SolarSystem)->searchByEnglishName(loc.planetName);
	QSettings* conf = StelApp::getInstance().getSettings();

	LandscapeMgr* landscapeMgr = GETSTELMODULE(LandscapeMgr);
	landscapeMgr->setCurrentLandscapeID(landscapeMgr->getDefaultLandscapeID());
	landscapeMgr->setFlagAtmosphere(p->hasAtmosphere() && conf->value("landscape/flag_atmosphere", true).toBool());
	landscapeMgr->setFlagFog(p->hasAtmosphere() && conf->value("landscape/flag_fog", true).toBool());
	landscapeMgr->setFlagLandscape(!p->getEnglishName().contains("observer", Qt::CaseInsensitive) && conf->value("landscape/flag_landscape", true).toBool());

	GETSTELMODULE(StelObjectMgr)->unSelect();

	StelMovementMgr* smmgr = getMovementMgr();
	smmgr->setViewDirectionJ2000(altAzToJ2000(smmgr->getInitViewingDirection(), StelCore::RefractionOff));
	smmgr->zoomTo(smmgr->getInitFov(), 1.);
}

double StelCore::getJDOfLastJDUpdate() const
{
	return jdOfLastJDUpdate;
}

void StelCore::setMilliSecondsOfLastJDUpdate(qint64 millis)
{
	milliSecondsOfLastJDUpdate = millis;
}

qint64 StelCore::getMilliSecondsOfLastJDUpdate() const
{
	return milliSecondsOfLastJDUpdate;
}

void StelCore::setJD(double newJD)
{
	JD.first=newJD;
	JD.second=computeDeltaT(newJD);	
	resetSync();
}

double StelCore::getJD() const
{
	return JD.first;
}

void StelCore::setJDE(double newJDE)
{
	// nitpickerish this is not exact, but as good as it gets...
	JD.second=computeDeltaT(newJDE);
	JD.first=newJDE-JD.second/86400.0;
	resetSync();
}

double StelCore::getJDE() const
{
	return JD.first+JD.second/86400.0;
}


void StelCore::setMJDay(double MJD)
{
	setJD(MJD+2400000.5);
}

double StelCore::getMJDay() const
{
	return JD.first-2400000.5;
}

double StelCore::getPresetSkyTime() const
{
	return presetSkyTime;
}

void StelCore::setPresetSkyTime(double d)
{
	presetSkyTime=d;
}

void StelCore::setTimeRate(double ts)
{
	timeSpeed=ts;
	resetSync();
	emit timeRateChanged(timeSpeed);
}

double StelCore::getTimeRate() const
{
	return timeSpeed;
}

void StelCore::revertTimeDirection(void)
{
	setTimeRate(-1*getTimeRate());
}

void StelCore::moveObserverToSelected()
{
	StelObjectMgr* objmgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(objmgr);
	if (objmgr->getWasSelected())
	{
		Planet* pl = dynamic_cast<Planet*>(objmgr->getSelectedObject()[0].data());
		if (pl)
		{
			// We need to move to the selected planet. Try to generate a location from the current one
			StelLocation loc = getCurrentLocation();
			if (loc.planetName != pl->getEnglishName())
			{
				loc.planetName = pl->getEnglishName();				
				loc.name = "landing site";
				loc.state = "";

				// Let's try guess name of location...
				LocationMap results = StelApp::getInstance().getLocationMgr().pickLocationsNearby(loc.planetName, loc.longitude, loc.latitude, 1.0f);
				if (results.size()>0)
					loc = results.value(results.firstKey()); // ...and use it!

				moveObserverTo(loc);
			}
		}
		else
		{
			NomenclatureItem* ni = dynamic_cast<NomenclatureItem*>(objmgr->getSelectedObject()[0].data());
			if (ni)
			{
				// We need to move to the nomenclature item's host planet.
				StelLocation loc; //  = getCurrentLocation();
				loc.planetName = ni->getPlanet()->getEnglishName();
				loc.name=ni->getEnglishName();
				loc.state = "";
				loc.longitude=ni->getLongitude();
				loc.latitude=ni->getLatitude();
				loc.bortleScaleIndex=1;

				moveObserverTo(loc);
				objmgr->unSelect(); // no use to keep it: Marker will flicker around the screen.
			}
		}
	}
	StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
	Q_ASSERT(mmgr);
	mmgr->setFlagTracking(false);
}

// Get the informations on the current location
const StelLocation& StelCore::getCurrentLocation() const
{
	return position->getCurrentLocation();
}

const QSharedPointer<Planet> StelCore::getCurrentPlanet() const
{
	return position->getHomePlanet();
}

const StelObserver *StelCore::getCurrentObserver() const
{
	return position;
}

void StelCore::setObserver(StelObserver *obs)
{
	delete position;
	position = obs;
	if (!getUseCustomTimeZone() && obs->getCurrentLocation().ianaTimeZone.length()>0)
		setCurrentTimeZone(obs->getCurrentLocation().ianaTimeZone);
}

// Smoothly move the observer to the given location
void StelCore::moveObserverTo(const StelLocation& target, double duration, double durationIfPlanetChange)
{
	double d = (getCurrentLocation().planetName==target.planetName) ? duration : durationIfPlanetChange;
	if (d>0.)
	{
		StelLocation curLoc = getCurrentLocation();
		if (position->isTraveling())
		{
			// Avoid using a temporary location name to create another temporary one (otherwise it looks like loc1 -> loc2 -> loc3 etc..)
			curLoc.name = ".";
		}
		SpaceShipObserver* newObs = new SpaceShipObserver(curLoc, target, d);		
		setObserver(newObs);
		newObs->update(0);
	}
	else
	{
		setObserver(new StelObserver(target));
	}
	emit targetLocationChanged(target);
	emit locationChanged(getCurrentLocation());
}

double StelCore::getUTCOffset(const double JD) const
{
	int year, month, day, hour, minute, second;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	StelUtils::getTimeFromJulianDay(JD, &hour, &minute, &second);
	// as analogous to second statement in getJDFromDate, nkerr
	if ( year <= 0 )
	{
		year = year - 1;
	}
	//getTime/DateFromJulianDay returns UTC time, not local time
	QDateTime universal(QDate(year, month, day), QTime(hour, minute, second), Qt::UTC);
	if (!universal.isValid())
	{
		//qWarning() << "JD " << QString("%1").arg(JD) << " out of bounds of QT help with GMT shift, using current datetime";
		// Assumes the GMT shift was always the same before year -4710
		// NOTE: QDateTime has no year 0, and therefore likely different leap year rules.
		// Under which circumstances do we get invalid universal?
		universal = QDateTime(QDate(-4710, month, day), QTime(hour, minute, second), Qt::UTC);
	}

#if defined(Q_OS_WIN)
	if (abs(year)<3)
	{
		// Mitigate a QTBUG on Windows (GH #594).
		// This bug causes offset to be MIN_INT in January to March, 1AD.
		// We assume a constant offset in this remote history,
		// so we construct yet another date to get a valid offset.
		// Application of the named time zones is inappropriate in any case.
		universal = QDateTime(QDate(3, month, day), QTime(hour, minute, second), Qt::UTC);
	}
#endif
	StelLocation loc = getCurrentLocation();
	QString tzName = getCurrentTimeZone();
	QTimeZone tz(tzName.toUtf8());
	if (!tz.isValid() && !QString("LMST LTST system_default").contains(tzName))
	{
		qWarning() << "Invalid timezone: " << tzName;
	}

	qint64 shiftInSeconds = 0;
	if (tzName=="system_default" || (loc.planetName=="Earth" && !tz.isValid() && !QString("LMST LTST").contains(tzName)))
	{
		QDateTime local = universal.toLocalTime();
		//Both timezones should be interpreted as UTC because secsTo() converts both
		//times to UTC if their zones have different daylight saving time rules.
		local.setTimeSpec(Qt::UTC);
		shiftInSeconds = universal.secsTo(local);
		if (abs(shiftInSeconds)>50000 || shiftInSeconds==INT_MIN)
		{
			qDebug() << "TZ system_default or invalid, At JD" << QString::number(JD, 'g', 11) << ", shift:" << shiftInSeconds;
		}
	}
	else
	{
		// The first adoption of a standard time was on December 1, 1847 in Great Britain
		if (tz.isValid() && loc.planetName=="Earth" && (JD>=StelCore::TZ_ERA_BEGINNING || getUseCustomTimeZone()))
		{
			if (getUseDST())
				shiftInSeconds = tz.offsetFromUtc(universal);
			else
				shiftInSeconds = tz.standardTimeOffset(universal);
			if (abs(shiftInSeconds)>500000 || shiftInSeconds==INT_MIN)
			{
				// Something very strange has happened. The Windows-only clause above already mitigated GH #594.
				// Trigger this with a named custom TZ like Europe/Stockholm.
				// Then try to wheel back some date in January-March from year 10 to 0. Instead of year 1, it jumps to 70,
				// an offset of INT_MIN
				qWarning() << "ERROR TRAPPED! --- Please submit a bug report with this logfile attached.";
				qWarning() << "TZ" << tz << "valid, but at JD" << QString::number(JD, 'g', 11) << ", shift:" << shiftInSeconds;
				qWarning() << "Universal reference date: " << universal.toString();
			}
		}
		else
		{
			shiftInSeconds = qRound((loc.longitude/15.f)*3600.f); // Local Mean Solar Time
		}
		if (tzName=="LTST")
			shiftInSeconds += getSolutionEquationOfTime(JD)*60;
	}
	#ifdef Q_OS_WIN
	// A dirty hack for report: https://github.com/Stellarium/stellarium/issues/686
	// TODO: switch to IANA TZ on all operating systems
	if (tzName=="Europe/Volgograd")
		shiftInSeconds = 4*3600; // UTC+04:00
	#endif

	// Extraterrestrial: Either use the configured Terrestrial timezone, or even a pseudo-LMST based on planet's rotation speed?
	if (loc.planetName!="Earth")
	{
		if (tz.isValid() && (JD>=StelCore::TZ_ERA_BEGINNING || getUseCustomTimeZone()))
		{
			if (getUseDST())
				shiftInSeconds = tz.offsetFromUtc(universal);
			else
				shiftInSeconds = tz.standardTimeOffset(universal);
			if (shiftInSeconds==INT_MIN) // triggered error
			{
				// Something very strange has happened. The Windows-only clause above already mitigated GH #594.
				// Trigger this with a named custom TZ like Europe/Stockholm.
				// Then try to wheel back some date in January-March from year 10 to 0. Instead of year 1, it jumps to 70,
				// an offset of INT_MIN
				qWarning() << "ERROR TRAPPED! --- Please submit a bug report with this logfile attached.";
				qWarning() << "TZ" << tz << "valid, but at JD" << QString::number(JD, 'g', 11) << ", shift:" << shiftInSeconds;
				qWarning() << "Universal reference date: " << universal.toString();
			}
		}
		else
		{
			// TODO: This should give "mean solar time" for any planet.
			// Combine rotation and orbit, or (for moons) rotation and orbit of parent planet.
			// LTST is even worse, needs equation of time for other planets.
			shiftInSeconds = 0; // For now, give UT
		}
	}

	return shiftInSeconds / 3600.0;
}

QString StelCore::getCurrentTimeZone() const
{
	return currentTimeZone;
}

void StelCore::setCurrentTimeZone(const QString& tz)
{
	if (StelApp::getInstance().getLocationMgr().getAllTimezoneNames().contains(tz))
	{
		currentTimeZone = tz;
		emit(currentTimeZoneChanged(tz));
	}
	else
	{
		qWarning() << "StelCore: Invalid timezone name:" << tz << " -- not setting timezone.";
	}
}

bool StelCore::getUseDST() const
{
	return flagUseDST;
}

void StelCore::setUseDST(const bool b)
{
	flagUseDST = b;
	StelApp::getInstance().getSettings()->setValue("localization/flag_dst", b);
	emit flagUseDSTChanged(b);
}

bool StelCore::getUseCustomTimeZone() const
{
	return flagUseCTZ;
}

void StelCore::setUseCustomTimeZone(const bool b)
{
	flagUseCTZ = b;
	emit useCustomTimeZoneChanged(b);
}

double StelCore::getSolutionEquationOfTime(const double JDE) const
{
	double tau = (JDE - 2451545.0)/365250.0;
	double sunMeanLongitude = 280.4664567 + tau*(360007.6892779 + tau*(0.03032028 + tau*(1./49931. - tau*(1./15300. - tau/2000000.))));

	// reduce the angle
	sunMeanLongitude = std::fmod(sunMeanLongitude, 360.);
	// force it to be the positive remainder, so that 0 <= angle < 360
	sunMeanLongitude = std::fmod(sunMeanLongitude + 360., 360.);

	Vec3d pos = GETSTELMODULE(StelObjectMgr)->searchByName("Sun")->getEquinoxEquatorialPos(this);
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, pos);

	// covert radians to degrees and reduce the angle
	double alpha = std::fmod(ra*M_180_PI, 360.);
	// force it to be the positive remainder, so that 0 <= angle < 360
	alpha = std::fmod(alpha + 360., 360.);

	double deltaPsi, deltaEps;
	getNutationAngles(JDE, &deltaPsi, &deltaEps); // these are radians!
	//double equation = 4*(sunMeanLongitude - 0.0057183 - alpha + get_nutation_longitude(JDE)*cos(get_mean_ecliptical_obliquity(JDE)));
	double equation = 4*(sunMeanLongitude - 0.0057183 - alpha + deltaPsi*M_180_PI*cos(getPrecessionAngleVondrakEpsilon(JDE)));
	// The equation of time is always smaller 20 minutes in absolute value
	if (qAbs(equation)>20)
	{
		// If absolute value of the equation of time appears to be too large, add 24 hours (1440 minutes) to or subtract it from our result
		if (equation>0.)
			equation -= 1440.;
		else
			equation += 1440.;
	}

	return equation;
}

//! Set stellarium time to current real world time
void StelCore::setTimeNow()
{
	setJD(StelUtils::getJDFromSystem());
	// Force emit dateChanged
	emit dateChanged();
}

void StelCore::setTodayTime(const QTime& target)
{
	QDateTime dt = QDateTime::currentDateTime();
	if (target.isValid())
	{
		dt.setTime(target);
		// don't forget to adjust for timezone / daylight savings.
		double JD = StelUtils::qDateTimeToJd(dt)-(static_cast<double>(getUTCOffset(StelUtils::getJDFromSystem())) * JD_HOUR);
		setJD(JD);
	}
	else
	{
		qWarning() << "WARNING - time passed to StelCore::setTodayTime is not valid. The system time will be used." << target;
		setTimeNow();
	}
}

//! Get whether the current stellarium time is the real world time
bool StelCore::getIsTimeNow(void) const
{
	// cache last time to prevent to much slow system call
	static double lastJD = getJD();
	static bool previousResult = (fabs(getJD()-(StelUtils::getJDFromSystem()))<JD_SECOND);
	if (fabs(lastJD-getJD())>JD_SECOND/4)
	{
		lastJD = getJD();
		previousResult = (fabs(getJD()-(StelUtils::getJDFromSystem()))<JD_SECOND);
	}
	return previousResult;
}

QTime StelCore::getInitTodayTime(void) const
{
	return initTodayTime;
}

void StelCore::setInitTodayTime(const QTime& time)
{
	initTodayTime=time;
}

void StelCore::setPresetSkyTime(QDateTime dateTime)
{
	setPresetSkyTime(StelUtils::qDateTimeToJd(dateTime));
}

void StelCore::addMinute()
{
	addSolarDays(JD_MINUTE);
}

void StelCore::addHour()
{
	addSolarDays(JD_HOUR);
}

void StelCore::addDay()
{
	addSolarDays(1.0);
}

void StelCore::addWeek()
{
	addSolarDays(7.0);
}

void StelCore::addSiderealDay()
{
	addSiderealDays(1.0);
}

void StelCore::addSiderealWeek()
{
	addSiderealDays(7.0);
}

void StelCore::addSiderealYear()
{
	addSiderealYears(1.);
}

void StelCore::addSiderealYears(double n)
{
	double days = 365.256363004;
	const PlanetP& home = position->getHomePlanet();
	if (!home->getEnglishName().contains("Observer", Qt::CaseInsensitive) && (home->getSiderealPeriod()>0))
		days = home->getSiderealPeriod();

	addSolarDays(days*n);
}

void StelCore::addSynodicMonth()
{
	addSolarDays(29.530588853);
}

void StelCore::addSaros()
{
	// 223 synodic months
	addSolarDays(223*29.530588853);
}

void StelCore::addDraconicMonth()
{
	addSolarDays(27.212220817);
}

void StelCore::addMeanTropicalMonth()
{
	addSolarDays(27.321582241);
}

void StelCore::addCalendricMonth()
{
	double cjd = getJD();
	int year, month, day, hour, minute, second;
	StelUtils::getDateFromJulianDay(cjd, &year, &month, &day);
	StelUtils::getTimeFromJulianDay(cjd, &hour, &minute, &second);
	month++;
	if (month>12)
	{
		month = 1;
		year++;
	}
	StelUtils::getJDFromDate(&cjd, year, month, day, hour, minute, second);
	setJD(cjd);
}

void StelCore::addAnomalisticMonth()
{
	addSolarDays(27.554549878);
}

void StelCore::addAnomalisticYear()
{
	addAnomalisticYears(1.);
}

void StelCore::addAnomalisticYears(double n)
{
	addSolarDays(365.259636*n);
}

void StelCore::addDraconicYear()
{
	addSolarDays(346.620075883);
}

void StelCore::addMeanTropicalYear()
{
	addMeanTropicalYears(1.0);
}

void StelCore::addTropicalYear()
{
	// Source: J. Meeus. More Mathematical Astronomy Morsels. 2002, p. 358.
	// Meeus, J. & Savoie, D. The history of the tropical year. Journal of the British Astronomical Association, vol.102, no.1, p.40-42
	// http://articles.adsabs.harvard.edu//full/1992JBAA..102...40M
	const double T = (getJD()-2451545.0)/365250.0;
	addSolarDays(365.242189623 - T*(0.000061522 - T*(0.0000000609 + T*0.00000026525)));
}

void StelCore::addMeanTropicalYears(double n)
{	
	// Source: https://en.wikipedia.org/wiki/Tropical_year#Mean_tropical_year_current_value
	addSolarDays(365.2421897*n); // The mean tropical year on January 1, 2000
}

void StelCore::addJulianYear()
{
	addJulianYears(1.);
}

void StelCore::addGaussianYear()
{
	addSolarDays(365.2568983);
}

void StelCore::addJulianYears(double n)
{
	addSolarDays(365.25*n);
}

void StelCore::subtractMinute()
{
	addSolarDays(-JD_MINUTE);
}

void StelCore::subtractHour()
{
	addSolarDays(-JD_HOUR);
}

void StelCore::subtractDay()
{
	addSolarDays(-1.0);
}

void StelCore::subtractWeek()
{
	addSolarDays(-7.0);
}

void StelCore::subtractSiderealDay()
{
	addSiderealDays(-1.0);
}

void StelCore::subtractSiderealWeek()
{
	addSiderealDays(-7.0);
}

void StelCore::subtractSiderealYear()
{
	addSiderealYears(-1.);
}

void StelCore::subtractSiderealYears(double n)
{
	addSiderealYears(-n);
}

void StelCore::subtractSynodicMonth()
{
	addSolarDays(-29.530588853);
}

void StelCore::subtractSaros()
{
	// 223 synodic months
	addSolarDays(-223*29.530588853);
}

void StelCore::subtractDraconicMonth()
{
	addSolarDays(-27.212220817);
}

void StelCore::subtractMeanTropicalMonth()
{
	addSolarDays(-27.321582241);
}

void StelCore::subtractCalendricMonth()
{
	double cjd = getJD();
	int year, month, day, hour, minute, second;
	StelUtils::getDateFromJulianDay(cjd, &year, &month, &day);
	StelUtils::getTimeFromJulianDay(cjd, &hour, &minute, &second);
	month--;
	if (month<1)
	{
		month = 12;
		year--;
	}
	StelUtils::getJDFromDate(&cjd, year, month, day, hour, minute, second);
	setJD(cjd);
}

void StelCore::subtractAnomalisticMonth()
{
	addSolarDays(-27.554549878);
}

void StelCore::subtractAnomalisticYear()
{
	subtractAnomalisticYears(1.);
}

void StelCore::subtractAnomalisticYears(double n)
{
	addSolarDays(-365.259636*n);
}

void StelCore::subtractDraconicYear()
{
	addSolarDays(-346.620075883);
}

void StelCore::subtractTropicalYear()
{
	// Source: J. Meeus. More Mathematical Astronomy Morsels. 2002, p. 358.
	double T = (getJD()-2451545.0)/365250.0;
	addSolarDays(-(365.242189623 - T*(0.000061522 - T*(0.0000000609 + T*0.00000026525))));
}

void StelCore::subtractMeanTropicalYear()
{
	addMeanTropicalYears(-1.0);
}

void StelCore::subtractMeanTropicalYears(double n)
{
	addMeanTropicalYears(-1.0*n);
}

void StelCore::subtractJulianYear()
{
	addSolarDays(-365.25);
}

void StelCore::subtractGaussianYear()
{
	addSolarDays(-365.2568983);
}

void StelCore::subtractJulianYears(double n)
{
	addSolarDays(-365.25*n);
}

void StelCore::addSolarDays(double d)
{
	const PlanetP& home = position->getHomePlanet();
	if (!home->getEnglishName().contains("Observer", Qt::CaseInsensitive))
		d *= home->getMeanSolarDay();

	setJD(getJD() + d);

	if (qAbs(d)>0.99) // WTF: qAbs(d)>=1.0 not working here!
		emit dateChanged();
}

void StelCore::addSiderealDays(double d)
{
	const PlanetP& home = position->getHomePlanet();
	if (!home->getEnglishName().contains("Observer", Qt::CaseInsensitive))
		d *= home->getSiderealDay();

	setJD(getJD() + d);
}

// Get the sidereal time of the prime meridian (i.e. Rotation Angle) shifted by the observer longitude
double StelCore::getLocalSiderealTime() const
{
	// On Earth, this requires UT deliberately with all its faults, on other planets we use the more regular TT.
	return (position->getHomePlanet()->getSiderealTime(getJD(), getJDE())+static_cast<double>(position->getCurrentLocation().longitude))*M_PI/180.;
}

//! Get the duration of a sidereal day for the current observer in day.
double StelCore::getLocalSiderealDayLength() const
{
	return position->getHomePlanet()->getSiderealDay();
}

//! Get the duration of a sidereal year for the current observer in days.
double StelCore::getLocalSiderealYearLength() const
{
	return position->getHomePlanet()->getSiderealPeriod();
}

QString StelCore::getStartupTimeMode() const
{
	return startupTimeMode;
}

//! Increase the time speed
void StelCore::increaseTimeSpeed()
{
	double s = getTimeRate();
	if (s>=JD_SECOND) s*=10.;
	else if (s<-JD_SECOND) s/=10.;
	else if (s>=0.) s=JD_SECOND;
	else s=0.;
	setTimeRate(s);
}

//! Decrease the time speed
void StelCore::decreaseTimeSpeed()
{
	double s = getTimeRate();
	if (s>JD_SECOND) s/=10.;
	else if (s<=-JD_SECOND) s*=10.;
	else if (s<=0.) s=-JD_SECOND;
	else s=0.;
	setTimeRate(s);
}

void StelCore::increaseTimeSpeedLess()
{
	double s = getTimeRate();
	if (s>=JD_SECOND) s*=2.;
	else if (s<-JD_SECOND) s/=2.;
	else if (s>=0.) s=JD_SECOND;
	else s=0.;
	setTimeRate(s);
}

void StelCore::decreaseTimeSpeedLess()
{
	double s = getTimeRate();
	if (s>JD_SECOND) s/=2.;
	else if (s<=-JD_SECOND) s*=2.;
	else if (s<=0.) s=-JD_SECOND;
	else s=0.;
	setTimeRate(s);
}

void StelCore::setZeroTimeSpeed()
{
	setTimeRate(0);
}

void StelCore::setRealTimeSpeed()
{
	setTimeRate(JD_SECOND);
}

void StelCore::toggleRealTimeSpeed()
{
	(!getRealTimeSpeed()) ? setRealTimeSpeed() : setZeroTimeSpeed();
}

bool StelCore::getRealTimeSpeed() const
{
	return (fabs(timeSpeed-JD_SECOND)<0.0000001);
}

////////////////////////////////////////////////////////////////////////////////
// Increment time
void StelCore::updateTime(double deltaTime)
{
	if (getRealTimeSpeed())
	{
		JD.first = jdOfLastJDUpdate + (QDateTime::currentMSecsSinceEpoch() - milliSecondsOfLastJDUpdate) / 1000.0 * JD_SECOND;
	}
	else
	{
		JD.first = jdOfLastJDUpdate + (QDateTime::currentMSecsSinceEpoch() - milliSecondsOfLastJDUpdate) / 1000.0 * timeSpeed;
	}

	// Fix time limits to -100000 to +100000 to prevent bugs
	if (JD.first>38245309.499988) JD.first = 38245309.499988;
	if (JD.first<-34803211.500012) JD.first = -34803211.500012;
	JD.second=computeDeltaT(JD.first);

	if (position->isObserverLifeOver())
	{
		// Unselect if the new home planet is the previously selected object
		StelObjectMgr* objmgr = GETSTELMODULE(StelObjectMgr);
		Q_ASSERT(objmgr);
		if (objmgr->getWasSelected() && objmgr->getSelectedObject()[0].data()==position->getHomePlanet())
		{
			objmgr->unSelect();
		}
		StelObserver* newObs = position->getNextObserver();
		delete position;
		position = newObs;
	}
	if (position->update(deltaTime))
		emit(locationChanged(getCurrentLocation()));

	// Position of sun and all the satellites (ie planets)
	// GZ maybe setting this static can speedup a bit?
	static SolarSystem* solsystem = static_cast<SolarSystem*>(StelApp::getInstance().getModuleMgr().getModule("SolarSystem"));
	// Likely the most important location where we need JDE:
	solsystem->computePositions(getJDE(), position->getHomePlanet());
}

void StelCore::resetSync()
{
	jdOfLastJDUpdate = getJD();
	//use currentMsecsSinceEpoch directly instead of StelApp::getTotalRuntime,
	//because the StelApp::startMSecs gets subtracted anyways in update()
	//also changed to qint64 to increase precision
	milliSecondsOfLastJDUpdate = QDateTime::currentMSecsSinceEpoch();
	emit timeSyncOccurred(jdOfLastJDUpdate);
}

void StelCore::registerMathMetaTypes()
{
	//enables use of these types in QVariant, StelProperty, signals and slots
	qRegisterMetaType<Vec2d>();
	qRegisterMetaType<Vec2f>();
	qRegisterMetaType<Vec2i>();
	qRegisterMetaType<Vec3d>();
	qRegisterMetaType<Vec3f>();
	qRegisterMetaType<Vec3i>();
	qRegisterMetaType<Vec4d>();
	qRegisterMetaType<Vec4f>();
	qRegisterMetaType<Vec4i>();
	qRegisterMetaType<Mat4d>();
	qRegisterMetaType<Mat4f>();
	qRegisterMetaType<Mat3d>();
	qRegisterMetaType<Mat3f>();

	//registers the QDataStream operators, so that QVariants with these types can be saved
	qRegisterMetaTypeStreamOperators<Vec2d>();
	qRegisterMetaTypeStreamOperators<Vec2f>();
	qRegisterMetaTypeStreamOperators<Vec2i>();
	qRegisterMetaTypeStreamOperators<Vec3d>();
	qRegisterMetaTypeStreamOperators<Vec3f>();
	qRegisterMetaTypeStreamOperators<Vec3i>();
	qRegisterMetaTypeStreamOperators<Vec4d>();
	qRegisterMetaTypeStreamOperators<Vec4f>();
	qRegisterMetaTypeStreamOperators<Vec4i>();
	qRegisterMetaTypeStreamOperators<Mat4d>();
	qRegisterMetaTypeStreamOperators<Mat4f>();
	qRegisterMetaTypeStreamOperators<Mat3d>();
	qRegisterMetaTypeStreamOperators<Mat3f>();

	//for debugging QVariants with these types, it helps if we register the string converters
	QMetaType::registerConverter(&Vec2d::toString);
	QMetaType::registerConverter(&Vec2f::toString);
	QMetaType::registerConverter(&Vec2i::toString);
	QMetaType::registerConverter(&Vec3d::toString);
	QMetaType::registerConverter(&Vec3f::toString);
	QMetaType::registerConverter(&Vec3i::toString);
	QMetaType::registerConverter(&Vec4d::toString);
	QMetaType::registerConverter(&Vec4f::toString);
	QMetaType::registerConverter(&Vec4i::toString);
}

void StelCore::setStartupTimeMode(const QString& s)
{
	startupTimeMode = s;
}

// return precomputed DeltaT in seconds. Public.
double StelCore::getDeltaT() const
{
	return JD.second;
}


// compute and return DeltaT in seconds. Try not to call it directly, current DeltaT, JD, and JDE are available.
double StelCore::computeDeltaT(const double JD)
{
	double DeltaT = 0.;
	if (currentDeltaTAlgorithm==Custom)
	{
		// User defined coefficients for quadratic equation for DeltaT may change frequently.
		deltaTnDot = deltaTCustomNDot; // n.dot = custom value "/cy/cy
		int year, month, day;
		StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
		double u = (StelUtils::yearFraction(year,month,day)-getDeltaTCustomYear())/100.;
		DeltaT = deltaTCustomEquationCoeff[0] + u*(deltaTCustomEquationCoeff[1] + u*deltaTCustomEquationCoeff[2]);
	}

	else
	{
		Q_ASSERT(deltaTfunc);
		DeltaT=deltaTfunc(JD);
	}

	if (!deltaTdontUseMoon)
		DeltaT += StelUtils::getMoonSecularAcceleration(JD, deltaTnDot, ((de430Active&&EphemWrapper::jd_fits_de430(JD)) || (de431Active&&EphemWrapper::jd_fits_de431(JD))));

	return DeltaT;
}

// set a function pointer here. This should make the actual computation simpler by just calling the function.
void StelCore::setCurrentDeltaTAlgorithm(DeltaTAlgorithm algorithm)
{
	currentDeltaTAlgorithm=algorithm;
	deltaTdontUseMoon = false; // most algorithms will use it!
	switch (currentDeltaTAlgorithm)
	{
		case WithoutCorrection:
			// Without correction, DeltaT is disabled
			deltaTfunc = StelUtils::getDeltaTwithoutCorrection;
			deltaTnDot = -26.; // n.dot = -26.0"/cy/cy OR WHAT SHALL WE DO HERE?
			deltaTdontUseMoon = true;
			deltaTstart	= INT_MIN;
			deltaTfinish	= INT_MAX;
			break;
		case Schoch:
			// Schoch (1931) algorithm for DeltaT
			deltaTnDot = -29.68; // n.dot = -29.68"/cy/cy
			deltaTfunc = StelUtils::getDeltaTBySchoch;
			deltaTstart	= -300;
			deltaTfinish	= 1980;
			break;
		case Clemence:
			// Clemence (1948) algorithm for DeltaT
			deltaTnDot = -22.44; // n.dot = -22.44 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByClemence;
			deltaTstart	= 1681;
			deltaTfinish	= 1900;
			break;
		case IAU:
			// IAU (1952) algorithm for DeltaT, based on observations by Spencer Jones (1939)
			deltaTnDot = -22.44; // n.dot = -22.44 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByIAU;
			deltaTstart	= 1681;
			deltaTfinish	= 1936; // Details in http://adsabs.harvard.edu/abs/1939MNRAS..99..541S
			break;
		case AstronomicalEphemeris:
			// Astronomical Ephemeris (1960) algorithm for DeltaT
			deltaTnDot = -22.44; // n.dot = -22.44 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByAstronomicalEphemeris;
			// GZ: What is the source of "1681..1900"? Expl.Suppl.AE 1961-p87 says "...over periods extending back to ancient times"
			// I changed to what I estimate.
			deltaTstart	= -500; // 1681;
			deltaTfinish	=  2000; // 1900;
			break;
		case TuckermanGoldstine:
			// Tuckerman (1962, 1964) & Goldstine (1973) algorithm for DeltaT
			//FIXME: n.dot
			deltaTnDot = -22.44; // n.dot = -22.44 "/cy/cy ???
			deltaTfunc = StelUtils::getDeltaTByTuckermanGoldstine;
			deltaTstart	= -600;
			deltaTfinish	= 1649;
			break;
		case MullerStephenson:
			// Muller & Stephenson (1975) algorithm for DeltaT
			deltaTnDot = -37.5; // n.dot = -37.5 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByMullerStephenson;
			deltaTstart	= -1375;
			deltaTfinish	= 1975;
			break;
		case Stephenson1978:
			// Stephenson (1978) algorithm for DeltaT
			deltaTnDot = -30.0; // n.dot = -30.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByStephenson1978;
			deltaTstart	= INT_MIN; // Range unknown!
			deltaTfinish	= INT_MAX;
			break;
		case SchmadelZech1979:
			// Schmadel & Zech (1979) algorithm for DeltaT
			deltaTnDot = -23.8946; // n.dot = -23.8946 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTBySchmadelZech1979;
			deltaTstart	= 1800;
			deltaTfinish	= 1975;
			break;
		case MorrisonStephenson1982:
			// Morrison & Stephenson (1982) algorithm for DeltaT (used by RedShift)
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByMorrisonStephenson1982;
			// FIXME: Is it valid range?
			deltaTstart	= -4000;
			deltaTfinish	= 2800;
			break;
		case StephensonMorrison1984:
			// Stephenson & Morrison (1984) algorithm for DeltaT
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByStephensonMorrison1984;
			deltaTstart	= -391;
			deltaTfinish	= 1600;
			break;
		case StephensonHoulden:
			// Stephenson & Houlden (1986) algorithm for DeltaT. The limits are implicitly given by the tabulated values.
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByStephensonHoulden;
			deltaTstart	= -600;
			deltaTfinish	= 1650;
			break;
		case Espenak:
			// Espenak (1987, 1989) algorithm for DeltaT
			//FIXME: n.dot
			deltaTnDot = -23.8946; // n.dot = -23.8946 "/cy/cy ???
			deltaTfunc = StelUtils::getDeltaTByEspenak;
			deltaTstart	= 1950;
			deltaTfinish	= 2100;
			break;
		case Borkowski:
			// Borkowski (1988) algorithm for DeltaT, relates to ELP2000-85!
			deltaTnDot = -23.895; // GZ: I see -23.895 in the paper, not -23.859; (?) // n.dot = -23.859 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByBorkowski;
			deltaTstart	= -2136;
			deltaTfinish	= 1715;
			break;
		case SchmadelZech1988:
			// Schmadel & Zech (1988) algorithm for DeltaT
			//FIXME: n.dot
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy ???
			deltaTfunc = StelUtils::getDeltaTBySchmadelZech1988;
			deltaTstart	= 1800;
			deltaTfinish	= 1988;
			break;
		case ChaprontTouze:
			// Chapront-Touzé & Chapront (1991) algorithm for DeltaT
			deltaTnDot = -23.8946; // n.dot = -23.8946 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByChaprontTouze;
			// FIXME: Is it valid range?
			deltaTstart	= -4000;
			deltaTfinish	= 8000;
			break;
		case StephensonMorrison1995:
			// Stephenson & Morrison (1995) algorithm for DeltaT
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByStephensonMorrison1995;
			deltaTstart	= -700;
			deltaTfinish	= 1600;
			break;
		case Stephenson1997:
			// Stephenson (1997) algorithm for DeltaT
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByStephenson1997;
			deltaTstart	= -500;
			deltaTfinish	= 1600;
			break;
		case ChaprontMeeus:
			// Chapront, Chapront-Touze & Francou (1997) & Meeus (1998) algorithm for DeltaT
			deltaTnDot = -25.7376; // n.dot = -25.7376 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByChaprontMeeus;
			deltaTstart	= -400; // 1800; // not explicitly given, but guess based on his using ChaprontFrancou which is cited elsewhere in a similar term with -391.
			deltaTfinish	=  2150; // 1997;
			break;
		case JPLHorizons:
			// JPL Horizons algorithm for DeltaT
			deltaTnDot = -25.7376; // n.dot = -25.7376 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByJPLHorizons;
			deltaTstart	= -2999;
			deltaTfinish	= 1620;
			break;
		case MeeusSimons:
			// Meeus & Simons (2000) algorithm for DeltaT
			deltaTnDot = -25.7376; // n.dot = -25.7376 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByMeeusSimons;
			deltaTstart	= 1620;
			deltaTfinish	= 2000;
			break;
		case ReingoldDershowitz:
			// Reingold & Dershowitz (2002, 2007, 2018) algorithm for DeltaT
			// FIXME: n.dot
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy ???
			deltaTfunc = StelUtils::getDeltaTByReingoldDershowitz;
			// GZ: while not original work, it's based on Meeus and therefore the full implementation covers likewise approximately:
			// AW: limits from 4th edition:
			deltaTstart	= -500; //1620;
			deltaTfinish	= 2150; //2019;
			break;
		case MontenbruckPfleger:
			// Montenbruck & Pfleger (2000) algorithm for DeltaT
			// NOTE: book does not contain n.dot value
			// FIXME: n.dot
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy ???
			deltaTfunc = StelUtils::getDeltaTByMontenbruckPfleger;
			deltaTstart	= 1825;
			deltaTfinish	= 2005;
			break;
		case MorrisonStephenson2004:
			// Morrison & Stephenson (2004, 2005) algorithm for DeltaT
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByMorrisonStephenson2004;
			deltaTstart	= -1000;
			deltaTfinish	= 2000;
			break;
		case Reijs:
			// Reijs (2006) algorithm for DeltaT
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByReijs;
			deltaTstart	= -1500; // -500; // GZ: It models long-term variability, so we should reflect this. Not sure on the begin, though.
			deltaTfinish	= 1100; // not 1620; // GZ: Not applicable for telescopic era, and better not after 1100 (pers.comm.)
			break;
		case EspenakMeeus:
			// Espenak & Meeus (2006) algorithm for DeltaT
			deltaTnDot = -25.858; // n.dot = -25.858 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByEspenakMeeus;
			deltaTstart	= -1999;
			deltaTfinish	= 3000;
			break;
		case EspenakMeeusZeroMoonAccel:
			// This is a trying area. Something is wrong with DeltaT, maybe ndot is still not applied correctly.
			// Espenak & Meeus (2006) algorithm for DeltaT
			deltaTnDot = -25.858; // n.dot = -25.858 "/cy/cy
			deltaTdontUseMoon = true;
			deltaTfunc = StelUtils::getDeltaTByEspenakMeeus;
			deltaTstart	= -1999;
			deltaTfinish	= 3000;
			break;
		case Banjevic:
			// Banjevic (2006) algorithm for DeltaT
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByBanjevic;
			deltaTstart	= -2020;
			deltaTfinish	= 1620;
			break;
		case IslamSadiqQureshi:
			// Islam, Sadiq & Qureshi (2008 + revisited 2013) algorithm for DeltaT (6 polynomials)
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByIslamSadiqQureshi;
			deltaTdontUseMoon = true; // Seems this solutions doesn't use value of secular acceleration of the Moon
			deltaTstart	= 1620;
			deltaTfinish	= 2007;
			break;
		case KhalidSultanaZaidi:
			// M. Khalid, Mariam Sultana and Faheem Zaidi polinomial approximation of time period 1620-2013 (2014)
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc = StelUtils::getDeltaTByKhalidSultanaZaidi;
			deltaTdontUseMoon = true; // Seems this solutions doesn't use value of secular acceleration of the Moon
			deltaTstart	= 1620;
			deltaTfinish	= 2013;
			break;
		case StephensonMorrisonHohenkerk2016:
			deltaTnDot = -25.82; // n.dot = -25.82 "/cy/cy
			deltaTfunc=StelUtils::getDeltaTByStephensonMorrisonHohenkerk2016;
			deltaTstart	= -720;
			deltaTfinish	= 2015;
			break;
		case Henriksson2017:
			// Henriksson solution (2017) for Schoch formula for DeltaT (1931)
			// Source: The Acceleration of the Moon and the Universe – the Mass of the Graviton.
			// Henriksson, G.
			// Advances in Astrophysics, Vol. 2, No. 3, August 2017
			// https://doi.org/10.22606/adap.2017.23004
			deltaTnDot = -30.128; // n.dot = -30.128"/cy/cy
			deltaTfunc = StelUtils::getDeltaTBySchoch;
			deltaTstart	= -4000;
			deltaTfinish	= 2000;
			break;
		case Custom:
			// User defined coefficients for quadratic equation for DeltaT. These can change, and we don't use the function pointer here.
			deltaTnDot = deltaTCustomNDot; // n.dot = custom value "/cy/cy
			deltaTfunc=Q_NULLPTR;
			deltaTstart	= INT_MIN; // Range unknown!
			deltaTfinish	= INT_MAX;
			break;
		default:
			deltaTnDot = -26.0; // n.dot = -26.0 "/cy/cy
			deltaTfunc=Q_NULLPTR;
			deltaTstart	= INT_MIN; // Range unknown!
			deltaTfinish	= INT_MAX;
			qCritical() << "StelCore: unknown DeltaT algorithm selected (" << currentDeltaTAlgorithm << ")! (setting nDot=-26., but no function!!)";
	}
	Q_ASSERT((currentDeltaTAlgorithm==Custom) || (deltaTfunc!=Q_NULLPTR));
}

//! Set the current algorithm for time correction to use
void StelCore::setCurrentDeltaTAlgorithmKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("DeltaTAlgorithm"));
	DeltaTAlgorithm algo = static_cast<DeltaTAlgorithm>(en.keyToValue(key.toLatin1().data()));
	if (algo<0)
	{
		qWarning() << "Unknown DeltaT algorithm: " << key << "setting \"WithoutCorrection\" instead";
		algo = WithoutCorrection;
	}
	setCurrentDeltaTAlgorithm(algo);
}

//! Get the current algorithm used by the DeltaT
QString StelCore::getCurrentDeltaTAlgorithmKey(void) const
{
	return metaObject()->enumerator(metaObject()->indexOfEnumerator("DeltaTAlgorithm")).key(currentDeltaTAlgorithm);
}

//! Get description of the current algorithm for time correction
QString StelCore::getCurrentDeltaTAlgorithmDescription(void) const
{
	// GZ remarked where more info would be desirable. Generally, a full citation and the math. term would be nice to have displayed if it's just one formula, and the range of valid dates.
	QString description;
	double jd = 0;
	QString marker;
	switch (getCurrentDeltaTAlgorithm())
	{
		case WithoutCorrection:
			description = q_("Correction is disabled. Use only if you know what you are doing!");
			break;
		case Schoch: // historical value.
			description = q_("This historical formula was obtained by C. Schoch in 1931 and was used by G. Henriksson in his article <em>Einstein's Theory of Relativity Confirmed by Ancient Solar Eclipses</em> (%1). See for more info %2here%3.").arg("2009").arg("<a href='http://journalofcosmology.com/AncientAstronomy123.html'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case Clemence: // historical value.
			description = q_("This empirical equation was published by G. M. Clemence in the article <em>On the system of astronomical constants</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1948AJ.....53..169C'>1948</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case IAU: // historical value.
			description = q_("This formula is based on a study of post-1650 observations of the Sun, the Moon and the planets by Spencer Jones (%1) and used by Jean Meeus in his <em>Astronomical Formulae for Calculators</em>. It was also adopted in the PC program SunTracker Pro.").arg("<a href='http://adsabs.harvard.edu/abs/1939MNRAS..99..541S'>1939</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			// find year of publication of AFFC
			break;
		case AstronomicalEphemeris: // historical value.
			description = q_("This is a slightly modified version of the IAU (1952) formula which was adopted in the <em>Astronomical Ephemeris</em> and in the <em>Canon of Solar Eclipses</em> by Mucke & Meeus (1983).").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			// TODO: expand the sentence: ... adopted ... from 19xx-19yy?
			break;
		case TuckermanGoldstine: // historical value.
			description = q_("The tables of Tuckerman (1962, 1964) list the positions of the Sun, the Moon and the planets at 5- and 10-day intervals from 601 BCE to 1649 CE. The same relation was also implicitly adopted in the syzygy tables of Goldstine (1973).").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			// TODO: These tables are sometimes found cited, but I have no details. Maybe add "based on ... " ?
			break;
		case MullerStephenson:
			description = q_("This equation was published by P. M. Muller and F. R. Stephenson in the article <em>The accelerations of the earth and moon from early astronomical observations</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1975grhe.conf..459M'>1975</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case Stephenson1978:
			description = q_("This equation was published by F. R. Stephenson in the article <em>Pre-Telescopic Astronomical Observations</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1978tfer.conf....5S'>1978</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case SchmadelZech1979: // outdated data fit, historical value?
			description = q_("This 12th-order polynomial equation (outdated and superseded by Schmadel & Zech (1988)) was published by L. D. Schmadel and G. Zech in the article <em>Polynomial approximations for the correction delta T E.T.-U.T. in the period 1800-1975</em> (%1) as fit through data published by Brouwer (1952).").arg("<a href='http://adsabs.harvard.edu/abs/1979AcA....29..101S'>1979</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case MorrisonStephenson1982:
			description = q_("This algorithm was adopted in P. Bretagnon & L. Simon's <em>Planetary Programs and Tables from -4000 to +2800</em> (1986) and in the PC planetarium program RedShift.").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case StephensonMorrison1984: // PRIMARY SOURCE
			description = q_("This formula was published by F. R. Stephenson and L. V. Morrison in the article <em>Long-term changes in the rotation of the earth - 700 B.C. to A.D. 1980</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1984RSPTA.313...47S'>1984</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case StephensonHoulden:
			description = q_("This algorithm is used in the PC planetarium program Guide 7.").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case Espenak: // limited range, but wide availability?
			description = q_("This algorithm was given by F. Espenak in his <em>Fifty Year Canon of Solar Eclipses: 1986-2035</em> (1987) and in his <em>Fifty Year Canon of Lunar Eclipses: 1986-2035</em> (1989).").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case Borkowski: // Linked to ELP2000-85, so it's important...
			description = q_("This formula was obtained by K.M. Borkowski (%1) from an analysis of 31 solar eclipse records dating between 2137 BCE and 1715 CE.").arg("<a href='http://adsabs.harvard.edu/abs/1988A&A...205L...8B'>1988</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case SchmadelZech1988: // data fit through Stephenson&Morrison1984, which itself is important. Unclear who uses this version?
			description = q_("This 12th-order polynomial equation was published by L. D. Schmadel and G. Zech in the article <em>Empirical Transformations from U.T. to E.T. for the Period 1800-1988</em> (%1) as data fit through values given by Stephenson & Morrison (1984).").arg("<a href='http://adsabs.harvard.edu/abs/1988AN....309..219S'>1988</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case ChaprontTouze:
			description = q_("This formula was adopted by M. Chapront-Touze & J. Chapront in the shortened version of the ELP 2000-85 lunar theory in their <em>Lunar Tables and Programs from 4000 B.C. to A.D. 8000</em> (1991).").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case StephensonMorrison1995:
			description = q_("This equation was published by F. R. Stephenson and L. V. Morrison in the article <em>Long-Term Fluctuations in the Earth's Rotation: 700 BC to AD 1990</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1995RSPTA.351..165S'>1995</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case Stephenson1997:
			description = q_("F. R. Stephenson published this formula in his book <em>Historical Eclipses and Earth's Rotation</em> (%1).").arg("<a href='http://ebooks.cambridge.org/ebook.jsf?bid=CBO9780511525186'>1997</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case ChaprontMeeus:
			description = q_("From J. Meeus, <em>Astronomical Algorithms</em> (2nd ed., 1998), and widely used. Table for 1620..2000, and includes a variant of Chapront, Chapront-Touze & Francou (1997) for dates outside 1620..2000.").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case JPLHorizons:
			description = q_("The JPL Solar System Dynamics Group of the NASA Jet Propulsion Laboratory use this formula in their interactive website %1JPL Horizons%2.").arg("<a href='http://ssd.jpl.nasa.gov/?horizons'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case MeeusSimons:
			description = q_("This polynome was published by J. Meeus and L. Simons in article <em>Polynomial approximations to Delta T, 1620-2000 AD</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/2000JBAA..110..323M'>2000</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case MontenbruckPfleger: // uninspired
			description = q_("The fourth edition of O. Montenbruck & T. Pfleger's <em>Astronomy on the Personal Computer</em> (2000) provides simple 3rd-order polynomial data fits for the recent past.").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case ReingoldDershowitz: //
			description = q_("E. M. Reingold & N. Dershowitz present this polynomial data fit in <em>Calendrical Calculations</em> (4th ed. 2018) and in their <em>Calendrical Tabulations</em> (2002). It is based on Jean Meeus' <em>Astronomical Algorithms</em> (1991).").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case MorrisonStephenson2004: // PRIMARY SOURCE
			description = q_("This important solution was published by L. V. Morrison and F. R. Stephenson in article <em>Historical values of the Earth's clock error %1T and the calculation of eclipses</em> (%2) with addendum in (%3).").arg(QChar(0x0394)).arg("<a href='http://adsabs.harvard.edu/abs/2004JHA....35..327M'>2004</a>").arg("<a href='http://adsabs.harvard.edu/abs/2005JHA....36..339M'>2005</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case Reijs:
			description = q_("From the Length of Day (LOD; as determined by Stephenson & Morrison (%2)), Victor Reijs derived a %1T formula by using a Simplex optimisation with a cosine and square function. This is based on a possible periodicy described by Stephenson (%2). See for more info %3here%4.").arg(QChar(0x0394)).arg("<a href='http://adsabs.harvard.edu/abs/2004JHA....35..327M'>2004</a>").arg("<a href='http://www.iol.ie/~geniet/eng/DeltaTeval.htm'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case EspenakMeeus: // GENERAL SOLUTION
			description = q_("This solution by F. Espenak and J. Meeus, based on Morrison & Stephenson (2004) and a polynomial fit through tabulated values for 1600-2000, is used for the %1NASA Eclipse Web Site%2 and in their <em>Five Millennium Canon of Solar Eclipses: -1900 to +3000</em> (2006). This formula is also used in the solar, lunar and planetary ephemeris program SOLEX.").arg("<a href='http://eclipse.gsfc.nasa.gov/eclipse.html'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker)).append(" <em>").append(q_("Used by default.")).append("</em>");
			break;
		case EspenakMeeusZeroMoonAccel: // PATCHED SOLUTION. Experimental, it may not make sense to keep it in V1.0.
			description = QString("%1 %2").arg(q_("PATCHED VERSION WITHOUT ADDITIONAL LUNAR ACCELERATION.")).arg(q_("This solution by F. Espenak and J. Meeus, based on Morrison & Stephenson (2004) and a polynomial fit through tabulated values for 1600-2000, is used for the %1NASA Eclipse Web Site%2 and in their <em>Five Millennium Canon of Solar Eclipses: -1900 to +3000</em> (2006). This formula is also used in the solar, lunar and planetary ephemeris program SOLEX.").arg("<a href='http://eclipse.gsfc.nasa.gov/eclipse.html'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker)).append(" <em>").append("</em>"));
			break;
		case Banjevic:
			description = q_("This solution by B. Banjevic, based on Stephenson & Morrison (1984), was published in article <em>Ancient eclipses and dating the fall of Babylon</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/2006POBeo..80..251B'>2006</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case IslamSadiqQureshi:
			description = q_("This solution by S. Islam, M. Sadiq and M. S. Qureshi, based on Meeus & Simons (2000), was published in article <em>Error Minimization of Polynomial Approximation of DeltaT</em> (%1) and revisited by Sana Islam in 2013.").arg("<a href='http://www.ias.ac.in/article/fulltext/joaa/029/03-04/0363-0366'>2008</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case KhalidSultanaZaidi:
			description = q_("This polynomial approximation with 0.6 seconds of accuracy by M. Khalid, Mariam Sultana and Faheem Zaidi was published in <em>Delta T: Polynomial Approximation of Time Period 1620-2013</em> (%1).").arg("<a href='https://doi.org/10.1155/2014/480964'>2014</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case StephensonMorrisonHohenkerk2016: // PRIMARY SOURCE, SEEMS VERY IMPORTANT
			description = q_("This solution by F. R. Stephenson, L. V. Morrison and C. Y. Hohenkerk (2016) was published in <em>Measurement of the Earth’s rotation: 720 BC to AD 2015</em> (%1). Outside of the named range (modelled with a spline fit) it provides values from an approximate parabola.").arg("<a href='https://doi.org/10.1098/rspa.2016.0404'>2016</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case Henriksson2017:
			description = q_("This solution by G. Henriksson was published in the article <em>The Acceleration of the Moon and the Universe - the Mass of the Graviton</em> (%1) and based on C. Schoch's formula (1931).").arg("<a href='https://doi.org/10.22606/adap.2017.23004'>2017</a>").append(getCurrentDeltaTAlgorithmValidRangeDescription(jd, &marker));
			break;
		case Custom:
			description = q_("This is a quadratic formula for calculation of %1T with coefficients defined by the user.").arg(QChar(0x0394));
			break;
		default:
			description = q_("Error");
	}

	// Put n-dot value info
	if (getCurrentDeltaTAlgorithm()!=WithoutCorrection)
	{
		QString fp = QString("%1\"/cy%2").arg(QString::number(getDeltaTnDot(), 'f', 4)).arg(QChar(0x00B2));
		QString sp = QString("n%1").arg(QChar(0x2032));
		description.append(" " + q_("The solution's value of %1 for %2 (secular acceleration of the Moon) requires an adaptation, see Guide for details.").arg(fp).arg(sp));
	}

	return description;
}

QString StelCore::getCurrentDeltaTAlgorithmValidRangeDescription(const double JD, QString *marker) const
{
	QString validRange = "";
	QString validRangeAppendix = "";
	*marker = "";
	int year, month, day;

	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	switch (currentDeltaTAlgorithm)
	{
		// These models provide extrapolated values outside their specified/recommended range.
		case WithoutCorrection:      // and
		case Schoch:                 // and
		case Clemence:               // and
		case IAU:                    // and
		case AstronomicalEphemeris:  // and
		case TuckermanGoldstine:     // and
		case StephensonHoulden:      // and
		case MullerStephenson:       // and
		case Stephenson1978:         // and
		case MorrisonStephenson1982: // and
		case StephensonMorrison1984: // and
		case Espenak:                // and
		case Borkowski:              // and
		case StephensonMorrison1995: // and
		case Stephenson1997:         // and
		case ChaprontMeeus:          // and
		case ReingoldDershowitz:     // and
		case MorrisonStephenson2004: // and
		case Reijs:                  // and
		case EspenakMeeus: // the default, range stated in the Canon, p. 14.  ... and
		case EspenakMeeusZeroMoonAccel: // and
		case StephensonMorrisonHohenkerk2016: // and
		case Henriksson2017:
			break;
		case JPLHorizons: // and
		case MeeusSimons:
			validRangeAppendix = q_("with zero values outside this range");
			break;
		case MontenbruckPfleger:
			validRangeAppendix = q_("with a typical 1-second accuracy and zero values outside this range");
			break;
		case SchmadelZech1988:
			validRangeAppendix = q_("with a mean error of less than one second, max. error 1.9s, and values for the limit years outside this range");
			break;
		case SchmadelZech1979:  // and
		case ChaprontTouze:     // and
		case Banjevic:          // and
		case IslamSadiqQureshi: // and
		case KhalidSultanaZaidi:
			validRangeAppendix = q_("with values for the limit years outside this range");
			break;
		case Custom:
			// Valid range unknown
			break;
	}

	if (deltaTstart > INT_MIN) // limits declared?
	{
		if (validRangeAppendix!="")
			validRange = q_("Valid range of usage: between years %1 and %2, %3.").arg(deltaTstart).arg(deltaTfinish).arg(validRangeAppendix);
		else
			validRange = q_("Valid range of usage: between years %1 and %2.").arg(deltaTstart).arg(deltaTfinish);
		if ((year < deltaTstart) || (deltaTfinish < year))
			*marker = "*"; // mark "outside designed range, possibly wrong"
	}
	else
		*marker = "?"; // mark "no range given"

	return QString(" %1").arg(validRange);
}

// return if sky plus atmosphere is bright enough from sunlight so that e.g. screen labels should be rendered dark.
// DONE: Simply ask the atmosphere for its surrounding brightness
// TODO2: This could be moved to the SkyDrawer or even some GUI class, as it is used to decide a GUI thing.
bool StelCore::isBrightDaylight() const
{
	if (propMgr->getStelPropertyValue("Oculars.enableOcular", true).toBool())
		return false;
	SolarSystem* ssys = GETSTELMODULE(SolarSystem);
	if (!ssys->getFlagPlanets())
		return false;
	if (!getSkyDrawer()->getFlagHasAtmosphere())
		return false;
	if (ssys->getEclipseFactor(this).first<=0.01) // Total solar eclipse
		return false;

	// immediately decide upon sky background brightness...
	return (GETSTELMODULE(LandscapeMgr)->getAtmosphereAverageLuminance() > static_cast<float>(getSkyDrawer()->getDaylightLabelThreshold()));
}

double StelCore::getCurrentEpoch() const
{
	return 2000.0 + (getJD() - 2451545.0)/365.25;
}

// DE430/DE431 handling.
// NOTE: When DE431 is not available, DE430 installed, but date outside, de430IsActive() may be true but computations still go back to VSOP!
// To test whether DE43x is really currently in use, test (EphemWrapper::use_de430(jd) || EphemWrapper::use_de431(jd))

bool StelCore::de430IsAvailable()
{
	return de430Available;
}

bool StelCore::de431IsAvailable()
{
	return de431Available;
}

bool StelCore::de430IsActive()
{
	return de430Active;
}

bool StelCore::de431IsActive()
{
	return de431Active;
}

void StelCore::setDe430Active(bool status)
{
	de430Active = de430Available && status;
}

void StelCore::setDe431Active(bool status)
{
	de431Active = de431Available && status;
}

void StelCore::initEphemeridesFunctions()
{
	QSettings* conf = StelApp::getInstance().getSettings();

	QString de430ConfigPath = conf->value("astro/de430_path").toString();
	QString de431ConfigPath = conf->value("astro/de431_path").toString();

	QString de430FilePath;
	QString de431FilePath;

	//<-- DE430 -->
	if(de430ConfigPath.remove(QChar('"')).isEmpty())
		de430FilePath = StelFileMgr::findFile("ephem/" + QString(DE430_FILENAME), StelFileMgr::File);
	else
		de430FilePath = StelFileMgr::findFile(de430ConfigPath, StelFileMgr::File);

	de430Available=!de430FilePath.isEmpty();
	if(de430Available)
	{
		qDebug() << "DE430 at: " << de430FilePath;
		EphemWrapper::init_de430(de430FilePath.toStdString().c_str());
	}
	setDe430Active(de430Available && conf->value("astro/flag_use_de430", false).toBool());

	//<-- DE431 -->
	if(de431ConfigPath.remove(QChar('"')).isEmpty())
		de431FilePath = StelFileMgr::findFile("ephem/" + QString(DE431_FILENAME), StelFileMgr::File);
	else
		de431FilePath = StelFileMgr::findFile(de431ConfigPath, StelFileMgr::File);

	de431Available=!de431FilePath.isEmpty();
	if(de431Available)
	{
		qDebug() << "DE431 at: " << de431FilePath;
		EphemWrapper::init_de431(de431FilePath.toStdString().c_str());
	}
	setDe431Active(de431Available && conf->value("astro/flag_use_de431", false).toBool());
}

// Methods for finding constellation from J2000 position.
typedef struct iau_constline{
	double RAlow;  // low value of 1875.0 right ascension segment, HH.dddd
	double RAhigh; // high value of 1875.0 right ascension segment, HH.dddd
	double decLow; // declination 1875.0 of southern border, DD.dddd
	QString constellation; // 3-letter code of constellation
} iau_constelspan;

static QVector<iau_constelspan> iau_constlineVec;
static bool iau_constlineVecInitialized=false;

// File iau_constellations_spans.dat is converted from file data.dat from ADC catalog VI/42.
// We converted back to HH:MM:SS format to avoid the inherent rounding errors present in that file (Bug LP:#1690615).
QString StelCore::getIAUConstellation(const Vec3d positionEqJnow) const
{
	// Precess positionJ2000 to 1875.0
	const Vec3d pos1875=j2000ToJ1875(equinoxEquToJ2000(positionEqJnow, RefractionOff));
	double RA1875;
	double dec1875;
	StelUtils::rectToSphe(&RA1875, &dec1875, pos1875);
	RA1875 *= 12./M_PI; // hours
	if (RA1875 <0.) RA1875+=24.;
	dec1875 *= M_180_PI; // degrees
	Q_ASSERT(RA1875>=0.0);
	Q_ASSERT(RA1875<=24.0);
	Q_ASSERT(dec1875<=90.0);
	Q_ASSERT(dec1875>=-90.0);

	// read file into structure.
	if (!iau_constlineVecInitialized)
	{
		//struct iau_constline line;
		QFile file(StelFileMgr::findFile("data/constellations_spans.dat"));

		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qWarning() << "IAU constellation line data file data/constellations_spans.dat not found.";
			return "err";
		}
		iau_constelspan span;
		QRegExp emptyLine("^\\s*$");
		QTextStream in(&file);
		while (!in.atEnd())
		{
			// Build list of entries. The checks can certainly become more robust. Actually the file must have 4-part lines.
			QString line = in.readLine();
			if (line.length()==0) continue;
			if (emptyLine.exactMatch((line))) continue;
			if (line.at(0)=='#') continue; // skip comment lines.
			//QStringList list = line.split(QRegExp("\\b\\s+\\b"));
			QStringList list = line.trimmed().split(QRegExp("\\s+"));
			if (list.count() != 4)
			{
				qWarning() << "IAU constellation file constellations_spans.dat has bad line:" << line << "with" << list.count() << "elements";
				continue;
			}
			//qDebug() << "Creating span for decl=" << list.at(2) << " from RA=" << list.at(0) << "to" << list.at(1) << ": " << list.at(3);
			QStringList numList=list.at(0).split(QRegExp(":"));
			span.RAlow= atof(numList.at(0).toLatin1()) + atof(numList.at(1).toLatin1())/60. + atof(numList.at(2).toLatin1())/3600.;
			numList=list.at(1).split(QRegExp(":"));
			span.RAhigh=atof(numList.at(0).toLatin1()) + atof(numList.at(1).toLatin1())/60. + atof(numList.at(2).toLatin1())/3600.;
			numList=list.at(2).split(QRegExp(":"));
			span.decLow=atof(numList.at(0).toLatin1());
			if (span.decLow<0.0)
				span.decLow -= atof(numList.at(1).toLatin1())/60.;
			else
				span.decLow += atof(numList.at(1).toLatin1())/60.;
			span.constellation=list.at(3);
			iau_constlineVec.append(span);
		}
		file.close();
		iau_constlineVecInitialized=true;
	}

	// iterate through vector, find entry where declination is lower.
	int entry=0;
	while (iau_constlineVec.at(entry).decLow > dec1875)
		entry++;
	while (entry<iau_constlineVec.size())
	{
		while (iau_constlineVec.at(entry).RAhigh <= RA1875)
			entry++;
		while (iau_constlineVec.at(entry).RAlow >= RA1875)
			entry++;
		if (iau_constlineVec.at(entry).RAhigh > RA1875)
			return iau_constlineVec.at(entry).constellation;
		else
			entry++;
	}
	qDebug() << "getIAUconstellation error: Cannot determine, algorithm failed.";
	return "(?)";
}

Vec3d StelCore::getMouseJ2000Pos() const
{
	const StelProjectorP prj = getProjection(StelCore::FrameJ2000, StelCore::RefractionAuto);
	float ppx = static_cast<float>(getCurrentStelProjectorParams().devicePixelsPerPixel);

	QPoint p = StelMainView::getInstance().getMousePos(); // get screen coordinates of mouse cursor
	Vec3d mousePosition;
	const float wh = prj->getViewportWidth()*0.5f; // get half of width of the screen
	const float hh = prj->getViewportHeight()*0.5f; // get half of height of the screen
	const float mx = p.x()*ppx-wh; // point 0 in center of the screen, axis X directed to right
	const float my = p.y()*ppx-hh; // point 0 in center of the screen, axis Y directed to bottom
	// calculate position of mouse cursor via position of center of the screen (and invert axis Y)
	// If coordinates are invalid, don't draw them.
	bool coordsValid = prj->unProject(static_cast<double>(prj->getViewportPosX()+wh+mx), static_cast<double>(prj->getViewportPosY()+hh+1-my), mousePosition);
	if (coordsValid)
	{ // Nick Fedoseev patch
		Vec3d win;
		prj->project(mousePosition,win);
		float dx = prj->getViewportPosX()+wh+mx - static_cast<float>(win.v[0]);
		float dy = prj->getViewportPosY()+hh+1-my - static_cast<float>(win.v[1]);
		prj->unProject(static_cast<double>(prj->getViewportPosX()+wh+mx+dx), static_cast<double>(prj->getViewportPosY()+hh+1-my+dy), mousePosition);
	}

	return mousePosition;
}
