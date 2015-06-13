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
#include "StelSkyDrawer.hpp"
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

#include <QSettings>
#include <QDebug>
#include <QMetaEnum>

// Init statics transfo matrices
// See vsop87.doc:
const Mat4d StelCore::matJ2000ToVsop87(Mat4d::xrotation(-23.4392803055555555556*(M_PI/180)) * Mat4d::zrotation(0.0000275*(M_PI/180)));
const Mat4d StelCore::matVsop87ToJ2000(matJ2000ToVsop87.transpose());
const Mat4d StelCore::matJ2000ToGalactic(-0.054875539726, 0.494109453312, -0.867666135858, 0, -0.873437108010, -0.444829589425, -0.198076386122, 0, -0.483834985808, 0.746982251810, 0.455983795705, 0, 0, 0, 0, 1);
const Mat4d StelCore::matGalacticToJ2000(matJ2000ToGalactic.transpose());

const double StelCore::JD_SECOND=0.000011574074074074074074;
const double StelCore::JD_MINUTE=0.00069444444444444444444;
const double StelCore::JD_HOUR  =0.041666666666666666666;
const double StelCore::JD_DAY   =1.;
const double StelCore::ONE_OVER_JD_SECOND = 24 * 60 * 60;


StelCore::StelCore()
	: skyDrawer(NULL)
	, movementMgr(NULL)
	, geodesicGrid(NULL)
	, currentProjectionType(ProjectionStereographic)
	, currentDeltaTAlgorithm(EspenakMeeus)
	, position(NULL)
	, timeSpeed(JD_SECOND)
	, JDay(0.)
	, presetSkyTime(0.)
	, secondsOfLastJDayUpdate(0.)
	, JDayOfLastJDayUpdate(0.)
	, deltaTCustomNDot(-26.0)
	, deltaTCustomYear(1820.0)
{
	toneConverter = new StelToneReproducer();

	QSettings* conf = StelApp::getInstance().getSettings();
	// Create and initialize the default projector params
	QString tmpstr = conf->value("projection/viewport").toString();
	currentProjectorParams.maskType = StelProjector::stringToMaskType(tmpstr);
	const int viewport_width = conf->value("projection/viewport_width", currentProjectorParams.viewportXywh[2]).toInt();
	const int viewport_height = conf->value("projection/viewport_height", currentProjectorParams.viewportXywh[3]).toInt();
	const int viewport_x = conf->value("projection/viewport_x", 0).toInt();
	const int viewport_y = conf->value("projection/viewport_y", 0).toInt();
	currentProjectorParams.viewportXywh.set(viewport_x,viewport_y,viewport_width,viewport_height);

	const float viewportCenterX = conf->value("projection/viewport_center_x",0.5f*viewport_width).toFloat();
	const float viewportCenterY = conf->value("projection/viewport_center_y",0.5f*viewport_height).toFloat();
	currentProjectorParams.viewportCenter.set(viewportCenterX, viewportCenterY);
	currentProjectorParams.viewportFovDiameter = conf->value("projection/viewport_fov_diameter", qMin(viewport_width,viewport_height)).toFloat();
	currentProjectorParams.flipHorz = conf->value("projection/flip_horz",false).toBool();
	currentProjectorParams.flipVert = conf->value("projection/flip_vert",false).toBool();

	currentProjectorParams.gravityLabels = conf->value("viewing/flag_gravity_labels").toBool();
	
	currentProjectorParams.devicePixelsPerPixel = StelApp::getInstance().getDevicePixelsPerPixel();
}


StelCore::~StelCore()
{
	delete toneConverter; toneConverter=NULL;
	delete geodesicGrid; geodesicGrid=NULL;
	delete skyDrawer; skyDrawer=NULL;
	delete position; position=NULL;
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

	// Delta-T stuff
	// Define default algorithm for time correction (Delta T)
	QString tmpDT = conf->value("navigation/time_correction_algorithm", "EspenakMeeus").toString();
	setCurrentDeltaTAlgorithmKey(tmpDT);

	// Define variables of custom equation for calculation of Delta T
	// Default: ndot = -26.0 "/cy/cy; year = 1820; DeltaT = -20 + 32*u^2, where u = (currentYear-1820)/100
	setDeltaTCustomYear(conf->value("custom_time_correction/year", 1820.0).toFloat());
	setDeltaTCustomNDot(conf->value("custom_time_correction/ndot", -26.0).toFloat());
	setDeltaTCustomEquationCoefficients(StelUtils::strToVec3f(conf->value("custom_time_correction/coefficients", "-20,0,32").toString()));

	// Time stuff
	setTimeNow();

	// We want to be able to handle the old style preset time, recorded as a double
	// jday, or as a more human readable string...
	QString presetTimeStr = conf->value("navigation/preset_sky_time",2451545.).toString();
	presetSkyTime = presetTimeStr.toDouble(&ok);
	if (ok)
	{
		qDebug() << "navigation/preset_sky_time is a double - treating as jday:" << presetSkyTime;
	}
	else
	{
		qDebug() << "navigation/preset_sky_time was not a double, treating as string date:" << presetTimeStr;
		presetSkyTime = StelUtils::qDateTimeToJd(QDateTime::fromString(presetTimeStr));
	}
	setInitTodayTime(QTime::fromString(conf->value("navigation/today_time", "22:00").toString()));
	startupTimeMode = conf->value("navigation/startup_time_mode", "actual").toString().toLower();
	if (startupTimeMode=="preset")	
		setJDay(presetSkyTime - StelUtils::getGMTShiftFromQT(presetSkyTime) * JD_HOUR);
	else if (startupTimeMode=="today")
		setTodayTime(getInitTodayTime());

	// Compute transform matrices between coordinates systems
	updateTransformMatrices();

	movementMgr = new StelMovementMgr(this);
	movementMgr->init();
	currentProjectorParams.fov = movementMgr->getInitFov();
	StelApp::getInstance().getModuleMgr().registerModule(movementMgr);

	skyDrawer = new StelSkyDrawer(this);
	skyDrawer->init();

	QString tmpstr = conf->value("projection/type", "ProjectionStereographic").toString();
	setCurrentProjectionTypeKey(tmpstr);

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
	actionsMgr->addAction("actionReturn_To_Current_Time", timeGroup, N_("Set time to now"), this, "setTimeNow()", "8");
	actionsMgr->addAction("actionAdd_Solar_Hour", timeGroup, N_("Add 1 solar hour"), this, "addHour()", "Ctrl+=");
	actionsMgr->addAction("actionAdd_Solar_Day", timeGroup, N_("Add 1 solar day"), this, "addDay()", "=");
	actionsMgr->addAction("actionAdd_Solar_Week", timeGroup, N_("Add 1 solar week"), this, "addWeek()", "]");
	actionsMgr->addAction("actionSubtract_Solar_Hour", timeGroup, N_("Subtract 1 solar hour"), this, "subtractHour()", "Ctrl+-");
	actionsMgr->addAction("actionSubtract_Solar_Day", timeGroup, N_("Subtract 1 solar day"), this, "subtractDay()", "-");
	actionsMgr->addAction("actionSubtract_Solar_Week", timeGroup, N_("Subtract 1 solar week"), this, "subtractWeek()", "[");
	actionsMgr->addAction("actionAdd_Sidereal_Day", timeGroup, N_("Add 1 sidereal day"), this, "addSiderealDay()", "Alt+=");
	actionsMgr->addAction("actionAdd_Sidereal_Week", timeGroup, N_("Add 1 sidereal week"), this, "addSiderealWeek()", "Alt+]");
	actionsMgr->addAction("actionAdd_Sidereal_Month", timeGroup, N_("Add 1 sidereal month"), this, "addSiderealMonth()", "Alt+Shift+]");
	actionsMgr->addAction("actionAdd_Sidereal_Year", timeGroup, N_("Add 1 sidereal year"), this, "addSiderealYear()", "Ctrl+Alt+Shift+]");
	actionsMgr->addAction("actionAdd_Sidereal_Century", timeGroup, N_("Add 1 sidereal century"), this, "addSiderealCentury()");
	actionsMgr->addAction("actionAdd_Synodic_Month", timeGroup, N_("Add 1 synodic month"), this, "addSynodicMonth()");
	actionsMgr->addAction("actionAdd_Draconic_Month", timeGroup, N_("Add 1 draconic month"), this, "addDraconicMonth()");
	actionsMgr->addAction("actionAdd_Draconic_Year", timeGroup, N_("Add 1 draconic year"), this, "addDraconicYear()");
	actionsMgr->addAction("actionAdd_Anomalistic_Month", timeGroup, N_("Add 1 anomalistic month"), this, "addAnomalisticMonth()");
	actionsMgr->addAction("actionAdd_Tropical_Month", timeGroup, N_("Add 1 mean tropical month"), this, "addTropicalMonth()");
	actionsMgr->addAction("actionAdd_Tropical_Year", timeGroup, N_("Add 1 mean tropical year"), this, "addTropicalYear()");
	actionsMgr->addAction("actionAdd_Tropical_Century", timeGroup, N_("Add 1 mean tropical century"), this, "addTropicalCentury()");
	actionsMgr->addAction("actionSubtract_Sidereal_Day", timeGroup, N_("Subtract 1 sidereal day"), this, "subtractSiderealDay()", "Alt+-");
	actionsMgr->addAction("actionSubtract_Sidereal_Week", timeGroup, N_("Subtract 1 sidereal week"), this, "subtractSiderealWeek()", "Alt+[");
	actionsMgr->addAction("actionSubtract_Sidereal_Month", timeGroup, N_("Subtract 1 sidereal month"), this, "subtractSiderealMonth()", "Alt+Shift+[");
	actionsMgr->addAction("actionSubtract_Sidereal_Year", timeGroup, N_("Subtract 1 sidereal year"), this, "subtractSiderealYear()", "Ctrl+Alt+Shift+[");
	actionsMgr->addAction("actionSubtract_Sidereal_Century", timeGroup, N_("Subtract 1 sidereal century"), this, "subtractSiderealCentury()");
	actionsMgr->addAction("actionSubtract_Synodic_Month", timeGroup, N_("Subtract 1 synodic month"), this, "subtractSynodicMonth()");
	actionsMgr->addAction("actionSubtract_Draconic_Month", timeGroup, N_("Subtract 1 draconic month"), this, "subtractDraconicMonth()");
	actionsMgr->addAction("actionSubtract_Draconic_Year", timeGroup, N_("Subtract 1 draconic year"), this, "subtractDraconicYear()");
	actionsMgr->addAction("actionSubtract_Anomalistic_Month", timeGroup, N_("Subtract 1 anomalistic month"), this, "subtractAnomalisticMonth()");
	actionsMgr->addAction("actionSubtract_Tropical_Month", timeGroup, N_("Subtract 1 mean tropical month"), this, "subtractTropicalMonth()");
	actionsMgr->addAction("actionSubtract_Tropical_Year", timeGroup, N_("Subtract 1 mean tropical year"), this, "subtractTropicalYear()");
	actionsMgr->addAction("actionSubtract_Tropical_Century", timeGroup, N_("Subtract 1 mean tropical century"), this, "subtractTropicalCentury()");

	actionsMgr->addAction("actionSet_Home_Planet_To_Selected", movementGroup, N_("Set home planet to selected planet"), this, "moveObserverToSelected()", "Ctrl+G");
	actionsMgr->addAction("actionGo_Home_Global", movementGroup, N_("Go to home"), this, "returnToHome()", "Ctrl+H");
	actionsMgr->addAction("actionHorizontal_Flip", displayGroup, N_("Flip scene horizontally"), this, "flipHorz", "Ctrl+Shift+H", "", true);
	actionsMgr->addAction("actionVertical_Flip", displayGroup, N_("Flip scene vertically"), this, "flipVert", "Ctrl+Shift+V", "", true);
	
}


// Get the shared instance of StelGeodesicGrid.
// The returned instance is garanteed to allow for at least maxLevel levels
const StelGeodesicGrid* StelCore::getGeodesicGrid(int maxLevel) const
{
	if (geodesicGrid==NULL)
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
		default:
			qWarning() << "Unknown projection type: " << (int)(projType) << "using ProjectionStereographic instead";
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
		case FrameHeliocentricEcliptic:
			return getProjection(getHeliocentricEclipticModelViewTransform(refractionMode));
		case FrameObservercentricEcliptic:
			return getProjection(getObservercentricEclipticModelViewTransform(refractionMode));
		case FrameEquinoxEqu:
			return getProjection(getEquinoxEquModelViewTransform(refractionMode));
		case FrameJ2000:
			return getProjection(getJ2000ModelViewTransform(refractionMode));
		case FrameGalactic:
			 return getProjection(getGalacticModelViewTransform(refractionMode));
		default:
			qDebug() << "Unknown reference frame type: " << (int)frameType << ".";
	}
	Q_ASSERT(0);
	return getProjection2d();
}

StelToneReproducer* StelCore::getToneReproducer()
{
	return toneConverter;
}

const StelToneReproducer* StelCore::getToneReproducer() const
{
	return toneConverter;
}

StelSkyDrawer* StelCore::getSkyDrawer()
{
	return skyDrawer;
}

const StelSkyDrawer* StelCore::getSkyDrawer() const
{
	return skyDrawer;
}

StelMovementMgr* StelCore::getMovementMgr()
{
	return movementMgr;
}

const StelMovementMgr* StelCore::getMovementMgr() const
{
	return movementMgr;
}

SphericalCap StelCore::getVisibleSkyArea() const
{
	const LandscapeMgr* landscapeMgr = GETSTELMODULE(LandscapeMgr);
	Vec3d up(0, 0, 1);
	up = altAzToJ2000(up, RefractionOff);
	
	if (landscapeMgr->getIsLandscapeFullyVisible())
	{
		return SphericalCap(up, -0.035f);
	}
	return SphericalCap(up, -1.f);
}

void StelCore::setClippingPlanes(double znear, double zfar)
{
	currentProjectorParams.zNear=znear;currentProjectorParams.zFar=zfar;
}

void StelCore::getClippingPlanes(double* zn, double* zf) const
{
	*zn = currentProjectorParams.zNear;
	*zf = currentProjectorParams.zFar;
}

// Handle the resizing of the window
void StelCore::windowHasBeenResized(float x, float y, float width, float height)
{
	// Maximize display when resized since it invalidates previous options anyway
	currentProjectorParams.viewportXywh.set(x, y, width, height);
	currentProjectorParams.viewportCenter.set(x+0.5*width, y+0.5*height);
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

	currentProjectorParams.fov = movementMgr->getCurrentFov();

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

	skyDrawer->preDraw();

	// Clear areas not redrawn by main viewport (i.e. fisheye square viewport)
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
}


/*************************************************************************
 Update core state after drawing modules
*************************************************************************/
void StelCore::postDraw()
{
	StelPainter sPainter(getProjection(StelCore::FrameJ2000));
	sPainter.drawViewportShape();
}

void StelCore::setCurrentProjectionType(ProjectionType type)
{
	currentProjectionType=type;
	const double savedFov = currentProjectorParams.fov;
	currentProjectorParams.fov = 0.0001;	// Avoid crash
	double newMaxFov = getProjection(StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity())))->getMaxFov();
	movementMgr->setMaxFov(newMaxFov);
	currentProjectorParams.fov = qMin(newMaxFov, savedFov);
}

StelCore::ProjectionType StelCore::getCurrentProjectionType() const
{
	return currentProjectionType;
}

//! Set the current projection type to use
void StelCore::setCurrentProjectionTypeKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	ProjectionType newType = (ProjectionType)en.keyToValue(key.toLatin1().data());
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
}

void StelCore::setDefautAngleForGravityText(float a)
{
	currentProjectorParams.defautAngleForGravityText = a;
}

void StelCore::setFlipHorz(bool flip)
{
	currentProjectorParams.flipHorz = flip;
}

void StelCore::setFlipVert(bool flip)
{
	currentProjectorParams.flipVert = flip;
}

bool StelCore::getFlipHorz(void) const
{
	return currentProjectorParams.flipHorz;
}

bool StelCore::getFlipVert(void) const
{
	return currentProjectorParams.flipVert;
}

QString StelCore::getDefaultLocationID() const
{
	return defaultLocationID;
}

QString StelCore::projectionTypeKeyToNameI18n(const QString& key) const
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	QString s(getProjection(StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity())), (ProjectionType)en.keyToValue(key.toLatin1()))->getNameI18());
	return s;
}

QString StelCore::projectionNameI18nToTypeKey(const QString& nameI18n) const
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("ProjectionType"));
	for (int i=0;i<en.keyCount();++i)
	{
		if (getProjection(StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity())), (ProjectionType)i)->getNameI18()==nameI18n)
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

Vec3d StelCore::altAzToEquinoxEqu(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matAltAzToEquinoxEqu*v;
	Vec3d r(v);
	skyDrawer->getRefraction().backward(r);
	r.transfo4d(matAltAzToEquinoxEqu);
	return r;
}

Vec3d StelCore::equinoxEquToAltAz(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matEquinoxEquToAltAz*v;
	Vec3d r(v);
	r.transfo4d(matEquinoxEquToAltAz);
	skyDrawer->getRefraction().forward(r);
	return r;
}

Vec3d StelCore::altAzToJ2000(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matEquinoxEquToJ2000*matAltAzToEquinoxEqu*v;
	Vec3d r(v);
	skyDrawer->getRefraction().backward(r);
	r.transfo4d(matEquinoxEquToJ2000*matAltAzToEquinoxEqu);
	return r;
}

Vec3d StelCore::j2000ToAltAz(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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

Vec3d StelCore::equinoxEquToJ2000(const Vec3d& v) const
{
	return matEquinoxEquToJ2000*v;
}

Vec3d StelCore::j2000ToEquinoxEqu(const Vec3d& v) const
{
	return matJ2000ToEquinoxEqu*v;
}

Vec3d StelCore::j2000ToGalactic(const Vec3d& v) const
{
	return matJ2000ToGalactic*v;
}

//! Transform vector from heliocentric ecliptic coordinate to altazimuthal
Vec3d StelCore::heliocentricEclipticToAltAz(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matHeliocentricEclipticToAltAz*v;
	Vec3d r(v);
	r.transfo4d(matHeliocentricEclipticToAltAz);
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
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matHeliocentricEclipticToAltAz));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matHeliocentricEclipticToAltAz);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric ecliptic (Vsop87A) drawing
StelProjector::ModelViewTranformP StelCore::getObservercentricEclipticModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matJ2000ToAltAz*matVsop87ToJ2000));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matJ2000ToAltAz*matVsop87ToJ2000);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}


//! Get the modelview matrix for observer-centric equatorial at equinox drawing
StelProjector::ModelViewTranformP StelCore::getEquinoxEquModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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
	if (refMode==RefractionOff || skyDrawer==NULL || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matGalacticToJ2000));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matGalacticToJ2000);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

// GZ: One of the most important functions, totally void of doc. :-(
// called in update() (for every frame)
void StelCore::updateTransformMatrices()
{
	matAltAzToEquinoxEqu = position->getRotAltAzToEquatorial(JDay);
	matEquinoxEquToAltAz = matAltAzToEquinoxEqu.transpose();

	// multiply static J2000 earth axis tilt (eclipticalJ2000<->equatorialJ2000)
	// in effect, this matrix transforms from VSOP87 ecliptical J2000 to planet-based equatorial coordinates.
	// For earth, this moves the equator relative to J2000 (precession of the equator).
	// TODO: fix Planet::getRotEquatorialToVsop87() so that matEquinoxEquToJ2000 performs Precession of the Equator properly.
	// TODO: rename matEquinoxEquToJ2000 to matEquinoxEquDateToJ2000
	matEquinoxEquToJ2000 = matVsop87ToJ2000 * position->getRotEquatorialToVsop87();
	matJ2000ToEquinoxEqu = matEquinoxEquToJ2000.transpose();
	matJ2000ToAltAz = matEquinoxEquToAltAz*matJ2000ToEquinoxEqu;

	matHeliocentricEclipticToEquinoxEqu = matJ2000ToEquinoxEqu * matVsop87ToJ2000 * Mat4d::translation(-position->getCenterVsop87Pos());

	// These two next have to take into account the position of the observer on the earth
	// GZ tmp could be called matAltAzToVsop87
	Mat4d tmp = matJ2000ToVsop87 * matEquinoxEquToJ2000 * matAltAzToEquinoxEqu;

	// TODO: rename to matAltAzToHeliocentricEcliptic2000
	// TODO: getDistanceFromCenter assumes spherical planets. Use rectangular coordinates for observer!
	matAltAzToHeliocentricEcliptic =  Mat4d::translation(position->getCenterVsop87Pos()) * tmp *
						  Mat4d::translation(Vec3d(0.,0., position->getDistanceFromCenter()));

	matHeliocentricEclipticToAltAz =  Mat4d::translation(Vec3d(0.,0.,-position->getDistanceFromCenter())) * tmp.transpose() *
						  Mat4d::translation(-position->getCenterVsop87Pos());
}

// Return the observer heliocentric position
Vec3d StelCore::getObserverHeliocentricEclipticPos() const
{
	return Vec3d(matAltAzToHeliocentricEcliptic[12], matAltAzToHeliocentricEcliptic[13], matAltAzToHeliocentricEcliptic[14]);
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

	GETSTELMODULE(StelObjectMgr)->unSelect();

	StelMovementMgr* smmgr = getMovementMgr();
	smmgr->setViewDirectionJ2000(altAzToJ2000(smmgr->getInitViewingDirection(), StelCore::RefractionOff));
	smmgr->zoomTo(smmgr->getInitFov(), 1.);
}

void StelCore::setJDay(double JD)
{
	JDay=JD;
	resetSync();
}

double StelCore::getJDay() const
{
	return JDay;
}

void StelCore::setMJDay(double MJD)
{
	JDay=MJD+2400000.5;
	resetSync();
}

double StelCore::getMJDay() const
{
	return JDay-2400000.5;
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
				loc.name = "-";
				loc.state = "";
				moveObserverTo(loc);

				LandscapeMgr* landscapeMgr = GETSTELMODULE(LandscapeMgr);
				if (pl->getEnglishName() == "Solar System Observer")
				{
					landscapeMgr->setFlagAtmosphere(false);
					landscapeMgr->setFlagFog(false);
					landscapeMgr->setFlagLandscape(false);
				}
				else
				{
					landscapeMgr->setFlagAtmosphere(pl->hasAtmosphere());
					landscapeMgr->setFlagFog(pl->hasAtmosphere());
					landscapeMgr->setFlagLandscape(true);
				}
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

// Smoothly move the observer to the given location
void StelCore::moveObserverTo(const StelLocation& target, double duration, double durationIfPlanetChange)
{
	emit(locationChanged(target));
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
		delete position;
		position = newObs;
		newObs->update(0);
	}
	else
	{
		delete position;
		position = new StelObserver(target);
	}
}


//! Set stellarium time to current real world time
void StelCore::setTimeNow()
{
	double JD = StelUtils::getJDFromSystem();
	// add Delta-T correction for actual time
	setJDay(JD+getDeltaT(JD)/86400);
}

void StelCore::setTodayTime(const QTime& target)
{
	QDateTime dt = QDateTime::currentDateTime();
	if (target.isValid())
	{
		dt.setTime(target);
		// don't forget to adjust for timezone / daylight savings.
		double JD = StelUtils::qDateTimeToJd(dt)-(StelUtils::getGMTShiftFromQT(StelUtils::getJDFromSystem()) * JD_HOUR);
		setJDay(JD + getDeltaT(JD)/86400);
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
	static double lastJD = getJDay();
	static bool previousResult = (fabs(getJDay()-(StelUtils::getJDFromSystem()+getDeltaT(lastJD)/86400))<JD_SECOND);
	if (fabs(lastJD-getJDay())>JD_SECOND/4)
	{
		lastJD = getJDay();
		previousResult = (fabs(getJDay()-(StelUtils::getJDFromSystem()+getDeltaT(lastJD)/86400))<JD_SECOND);
	}
	return previousResult;
}

QTime StelCore::getInitTodayTime(void)
{
	return initTodayTime;
}

void StelCore::setInitTodayTime(const QTime& t)
{
	initTodayTime=t;
}

void StelCore::setPresetSkyTime(QDateTime dt)
{
	setPresetSkyTime(StelUtils::qDateTimeToJd(dt));
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

void StelCore::addSiderealMonth()
{
	double days = 27.321661;
	const PlanetP& home = position->getHomePlanet();
	if ((home->getEnglishName() != "Solar System Observer") && (home->getSiderealPeriod()>0))
		days = home->getSiderealPeriod()/12;

	addSolarDays(days);
}

void StelCore::addSiderealYear()
{
	double days = 365.256363004;
	const PlanetP& home = position->getHomePlanet();
	if ((home->getEnglishName() != "Solar System Observer") && (home->getSiderealPeriod()>0))
		days = home->getSiderealPeriod();

	addSolarDays(days);
}

void StelCore::addSiderealCentury()
{
	double days = 36525.6363004;
	const PlanetP& home = position->getHomePlanet();
	if ((home->getEnglishName() != "Solar System Observer") && (home->getSiderealPeriod()>0))
		days = home->getSiderealPeriod()*100;

	addSolarDays(days);
}

void StelCore::addSynodicMonth()
{
	addSolarDays(29.530588853);
}

void StelCore::addDraconicMonth()
{
	addSolarDays(27.212220817);
}

void StelCore::addTropicalMonth()
{
	addSolarDays(27.321582241);
}

void StelCore::addAnomalisticMonth()
{
	addSolarDays(27.554549878);
}

void StelCore::addDraconicYear()
{
	addSolarDays(346.620075883);
}

void StelCore::addTropicalYear()
{
	addSolarDays(365.242190419);
}

void StelCore::addTropicalCentury()
{
	addSolarDays(36524.21897);
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

void StelCore::subtractSiderealMonth()
{
	double days = -27.321661;
	const PlanetP& home = position->getHomePlanet();
	if ((home->getEnglishName() != "Solar System Observer") && (home->getSiderealPeriod()>0))
		days = -1*(home->getSiderealPeriod()/12);

	addSolarDays(days);
}

void StelCore::subtractSiderealYear()
{
	double days = 365.256363004;
	const PlanetP& home = position->getHomePlanet();
	if ((home->getEnglishName() != "Solar System Observer") && (home->getSiderealPeriod()>0))
		days = home->getSiderealPeriod();

	addSolarDays(-days);
}

void StelCore::subtractSiderealCentury()
{
	double days = 36525.6363004;
	const PlanetP& home = position->getHomePlanet();
	if ((home->getEnglishName() != "Solar System Observer") && (home->getSiderealPeriod()>0))
		days = home->getSiderealPeriod()*100;

	addSolarDays(-days);
}

void StelCore::subtractSynodicMonth()
{
	addSolarDays(-29.530588853);
}

void StelCore::subtractDraconicMonth()
{
	addSolarDays(-27.212220817);
}

void StelCore::subtractTropicalMonth()
{
	addSolarDays(-27.321582241);
}

void StelCore::subtractAnomalisticMonth()
{
	addSolarDays(-27.554549878);
}

void StelCore::subtractDraconicYear()
{
	addSolarDays(-346.620075883);
}

void StelCore::subtractTropicalYear()
{
	addSolarDays(-365.242190419);
}

void StelCore::subtractTropicalCentury()
{
	addSolarDays(-36524.21897);
}

void StelCore::addSolarDays(double d)
{
	const PlanetP& home = position->getHomePlanet();
	if (home->getEnglishName() != "Solar System Observer")	
		d *= home->getMeanSolarDay();

	setJDay(getJDay() + d);
}

void StelCore::addSiderealDays(double d)
{
	const PlanetP& home = position->getHomePlanet();
	if (home->getEnglishName() != "Solar System Observer")
		d *= home->getSiderealDay();
	setJDay(getJDay() + d);
}

// Get the sidereal time shifted by the observer longitude
double StelCore::getLocalSiderealTime() const
{
	return (position->getHomePlanet()->getSiderealTime(JDay)+position->getCurrentLocation().longitude)*M_PI/180.;
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

QString StelCore::getStartupTimeMode()
{
	return startupTimeMode;
}

//! Increase the time speed
void StelCore::increaseTimeSpeed()
{
	double s = getTimeRate();
	if (s>=JD_SECOND) s*=10.;
	else if (s<-JD_SECOND) s/=10.;
	else if (s>=0. && s<JD_SECOND) s=JD_SECOND;
	else if (s>=-JD_SECOND && s<0.) s=0.;
	setTimeRate(s);
}

//! Decrease the time speed
void StelCore::decreaseTimeSpeed()
{
	double s = getTimeRate();
	if (s>JD_SECOND) s/=10.;
	else if (s<=-JD_SECOND) s*=10.;
	else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
	else if (s>0. && s<=JD_SECOND) s=0.;
	setTimeRate(s);
}

void StelCore::increaseTimeSpeedLess()
{
	double s = getTimeRate();
	if (s>=JD_SECOND) s*=2.;
	else if (s<-JD_SECOND) s/=2.;
	else if (s>=0. && s<JD_SECOND) s=JD_SECOND;
	else if (s>=-JD_SECOND && s<0.) s=0.;
	setTimeRate(s);
}

void StelCore::decreaseTimeSpeedLess()
{
	double s = getTimeRate();
	if (s>JD_SECOND) s/=2.;
	else if (s<=-JD_SECOND) s*=2.;
	else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
	else if (s>0. && s<=JD_SECOND) s=0.;
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
		// Get rid of the error from the 1 /
		JDay = JDayOfLastJDayUpdate + (StelApp::getTotalRunTime() - secondsOfLastJDayUpdate) / ONE_OVER_JD_SECOND;
	}
	else
	{
		JDay = JDayOfLastJDayUpdate + (StelApp::getTotalRunTime() - secondsOfLastJDayUpdate) * timeSpeed;
	}

	// Fix time limits to -100000 to +100000 to prevent bugs
	if (JDay>38245309.499988) JDay = 38245309.499988;
	if (JDay<-34803211.500012) JDay = -34803211.500012;

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
	position->update(deltaTime);

	// Position of sun and all the satellites (ie planets)
	SolarSystem* solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
	solsystem->computePositions(getJDay(), position->getHomePlanet()->getHeliocentricEclipticPos());
}

void StelCore::resetSync()
{
	JDayOfLastJDayUpdate = getJDay();
	secondsOfLastJDayUpdate = StelApp::getTotalRunTime();
}

void StelCore::setStartupTimeMode(const QString& s)
{
	startupTimeMode = s;
}

double StelCore::getDeltaT(double jDay) const
{
	double DeltaT = 0.;
	double ndot = 0.;
	bool dontUseMoon = false;
	switch (getCurrentDeltaTAlgorithm())
	{
		case WithoutCorrection:
			// Without correction, DeltaT is disabled
			DeltaT = 0.;
			dontUseMoon = true;
			break;
		case Schoch:
			// Schoch (1931) algorithm for DeltaT
			ndot = -29.68; // n.dot = -29.68"/cy/cy
			DeltaT = StelUtils::getDeltaTBySchoch(jDay);
			break;
		case Clemence:
			// Clemence (1948) algorithm for DeltaT
			ndot = -22.44; // n.dot = -22.44 "/cy/cy
			DeltaT = StelUtils::getDeltaTByClemence(jDay);
			break;
		case IAU:
			// IAU (1952) algorithm for DeltaT, based on observations by Spencer Jones (1939)
			ndot = -22.44; // n.dot = -22.44 "/cy/cy
			DeltaT = StelUtils::getDeltaTByIAU(jDay);
			break;
		case AstronomicalEphemeris:
			// Astronomical Ephemeris (1960) algorithm for DeltaT
			ndot = -22.44; // n.dot = -22.44 "/cy/cy
			DeltaT = StelUtils::getDeltaTByAstronomicalEphemeris(jDay);
			break;
		case TuckermanGoldstine:
			// Tuckerman (1962, 1964) & Goldstine (1973) algorithm for DeltaT
			//FIXME: n.dot
			ndot = -22.44; // n.dot = -22.44 "/cy/cy ???
			DeltaT = StelUtils::getDeltaTByTuckermanGoldstine(jDay);
			break;
		case MullerStephenson:
			// Muller & Stephenson (1975) algorithm for DeltaT
			ndot = -37.5; // n.dot = -37.5 "/cy/cy
			DeltaT = StelUtils::getDeltaTByMullerStephenson(jDay);
			break;
		case Stephenson1978:
			// Stephenson (1978) algorithm for DeltaT
			ndot = -30.0; // n.dot = -30.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByStephenson1978(jDay);
			break;
		case SchmadelZech1979:
			// Schmadel & Zech (1979) algorithm for DeltaT
			ndot = -23.8946; // n.dot = -23.8946 "/cy/cy
			DeltaT = StelUtils::getDeltaTBySchmadelZech1979(jDay);
			break;
		case MorrisonStephenson1982:
			// Morrison & Stephenson (1982) algorithm for DeltaT (used by RedShift)
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByMorrisonStephenson1982(jDay);
			break;
		case StephensonMorrison1984:
			// Stephenson & Morrison (1984) algorithm for DeltaT
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByStephensonMorrison1984(jDay);
			break;
		case StephensonHoulden:
			// Stephenson & Houlden (1986) algorithm for DeltaT
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByStephensonHoulden(jDay);
			break;
		case Espenak:
			// Espenak (1987, 1989) algorithm for DeltaT
			//FIXME: n.dot
			ndot = -23.8946; // n.dot = -23.8946 "/cy/cy ???
			DeltaT = StelUtils::getDeltaTByEspenak(jDay);
			break;
		case Borkowski:
			// Borkowski (1988) algorithm for DeltaT, relates to ELP2000-85!
			ndot = -23.895; // GZ: I see -23.895 in the paper, not -23.859; (?) // n.dot = -23.859 "/cy/cy
			DeltaT = StelUtils::getDeltaTByBorkowski(jDay);
			break;
		case SchmadelZech1988:
			// Schmadel & Zech (1988) algorithm for DeltaT
			//FIXME: n.dot
			ndot = -26.0; // n.dot = -26.0 "/cy/cy ???
			DeltaT = StelUtils::getDeltaTBySchmadelZech1988(jDay);
			break;
		case ChaprontTouze:
			// Chapront-Touzé & Chapront (1991) algorithm for DeltaT
			ndot = -23.8946; // n.dot = -23.8946 "/cy/cy
			DeltaT = StelUtils::getDeltaTByChaprontTouze(jDay);
			break;
		case StephensonMorrison1995:
			// Stephenson & Morrison (1995) algorithm for DeltaT
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByStephensonMorrison1995(jDay);
			break;
		case Stephenson1997:
			// Stephenson (1997) algorithm for DeltaT
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByStephenson1997(jDay);
			break;
		case ChaprontMeeus:
			// Chapront, Chapront-Touze & Francou (1997) & Meeus (1998) algorithm for DeltaT
			ndot = -25.7376; // n.dot = -25.7376 "/cy/cy
			DeltaT = StelUtils::getDeltaTByChaprontMeeus(jDay);
			break;
		case JPLHorizons:
			// JPL Horizons algorithm for DeltaT
			ndot = -25.7376; // n.dot = -25.7376 "/cy/cy
			DeltaT = StelUtils::getDeltaTByJPLHorizons(jDay);
			break;
		case MeeusSimons:
			// Meeus & Simons (2000) algorithm for DeltaT
			ndot = -25.7376; // n.dot = -25.7376 "/cy/cy
			DeltaT = StelUtils::getDeltaTByMeeusSimons(jDay);
			break;
		case ReingoldDershowitz:
			// Reingold & Dershowitz (2002, 2007) algorithm for DeltaT
			// FIXME: n.dot
			ndot = -26.0; // n.dot = -26.0 "/cy/cy ???
			DeltaT = StelUtils::getDeltaTByReingoldDershowitz(jDay);
			break;
		case MontenbruckPfleger:
			// Montenbruck & Pfleger (2000) algorithm for DeltaT
			// NOTE: book not contains n.dot value
			// FIXME: n.dot
			ndot = -26.0; // n.dot = -26.0 "/cy/cy ???
			DeltaT = StelUtils::getDeltaTByMontenbruckPfleger(jDay);
			break;
		case MorrisonStephenson2004:
			// Morrison & Stephenson (2004, 2005) algorithm for DeltaT
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByMorrisonStephenson2004(jDay);
			break;
		case Reijs:
			// Reijs (2006) algorithm for DeltaT
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByReijs(jDay);
			break;
		case EspenakMeeus:
			// Espenak & Meeus (2006) algorithm for DeltaT
			ndot = -25.858; // n.dot = -25.858 "/cy/cy
			DeltaT = StelUtils::getDeltaTByEspenakMeeus(jDay);
			break;
		case Banjevic:
			// Banjevic (2006) algorithm for DeltaT
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByBanjevic(jDay);
			break;
		case IslamSadiqQureshi:
			// Islam, Sadiq & Qureshi (2008 + revisited 2013) algorithm for DeltaT (6 polynomials)
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByIslamSadiqQureshi(jDay);
			dontUseMoon = true; // Seems this solutions doesn't use value of secular acceleration of the Moon
			break;
		case KhalidSultanaZaidi:
			// M. Khalid, Mariam Sultana and Faheem Zaidi polinomial approximation of time period 1620-2013 (2014)
			ndot = -26.0; // n.dot = -26.0 "/cy/cy
			DeltaT = StelUtils::getDeltaTByKhalidSultanaZaidi(jDay);
			dontUseMoon = true; // Seems this solutions doesn't use value of secular acceleration of the Moon
			break;
		case Custom:
			// User defined coefficients for quadratic equation for DeltaT
			ndot = getDeltaTCustomNDot(); // n.dot = custom value "/cy/cy
			int year, month, day;
			Vec3f coeff = getDeltaTCustomEquationCoefficients();
			StelUtils::getDateFromJulianDay(jDay, &year, &month, &day);			
			double u = (StelUtils::getDecYear(year,month,day)-getDeltaTCustomYear())/100;
			DeltaT = coeff[0] + u*(coeff[1] + u*coeff[2]);
			break;
	}

	if (!dontUseMoon)
		DeltaT += StelUtils::getMoonSecularAcceleration(jDay, ndot);

	return DeltaT;
}

//! Set the current algorithm for time correction to use
void StelCore::setCurrentDeltaTAlgorithmKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("DeltaTAlgorithm"));
	DeltaTAlgorithm algo = (DeltaTAlgorithm)en.keyToValue(key.toLatin1().data());
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
			description = q_("This historical formula was obtained by C. Schoch in 1931 and was used by G. Henriksson in his article <em>Einstein's Theory of Relativity Confirmed by Ancient Solar Eclipses</em> (%1). See for more info %2here%3.").arg("2009").arg("<a href='http://journalofcosmology.com/AncientAstronomy123.html'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case Clemence: // historical value.
			description = q_("This empirical equation was published by G. M. Clemence in the article <em>On the system of astronomical constants</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1948AJ.....53..169C'>1948</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case IAU: // historical value.
			description = q_("This formula is based on a study of post-1650 observations of the Sun, the Moon and the planets by Spencer Jones (%1) and used by Jean Meeus in his <em>Astronomical Formulae for Calculators</em>. It was also adopted in the PC program SunTracker Pro.").arg("<a href='http://adsabs.harvard.edu/abs/1939MNRAS..99..541S'>1939</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			// find year of publication of AFFC
			break;
		case AstronomicalEphemeris: // historical value.
			description = q_("This is a slightly modified version of the IAU (1952) formula which was adopted in the <em>Astronomical Ephemeris</em> and in the <em>Canon of Solar Eclipses</em> by Mucke & Meeus (1983).").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			// TODO: expand the sentence: ... adopted ... from 19xx-19yy?
			break;
		case TuckermanGoldstine: // historical value.
			description = q_("The tables of Tuckerman (1962, 1964) list the positions of the Sun, the Moon and the planets at 5- and 10-day intervals from 601 BCE to 1649 CE. The same relation was also implicitly adopted in the syzygy tables of Goldstine (1973).").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			// TODO: These tables are sometimes found cited, but I have no details. Maybe add "based on ... " ?
			break;
		case MullerStephenson:
			description = q_("This equation was published by P. M. Muller and F. R. Stephenson in the article <em>The accelerations of the earth and moon from early astronomical observations</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1975grhe.conf..459M'>1975</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case Stephenson1978:
			description = q_("This equation was published by F. R. Stephenson in the article <em>Pre-Telescopic Astronomical Observations</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1978tfer.conf....5S'>1978</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case SchmadelZech1979: // outdated data fit, historical value?
			description = q_("This 12th-order polynomial equation (outdated and superseded by Schmadel & Zech (1988)) was published by L. D. Schmadel and G. Zech in the article <em>Polynomial approximations for the correction delta T E.T.-U.T. in the period 1800-1975</em> (%1) as fit through data published by Brouwer (1952).").arg("<a href='http://adsabs.harvard.edu/abs/1979AcA....29..101S'>1979</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case MorrisonStephenson1982:
			description = q_("This algorithm was adopted in P. Bretagnon & L. Simon's <em>Planetary Programs and Tables from -4000 to +2800</em> (1986) and in the PC planetarium program RedShift.").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case StephensonMorrison1984: // PRIMARY SOURCE
			description = q_("This formula was published by F. R. Stephenson and L. V. Morrison in the article <em>Long-term changes in the rotation of the earth - 700 B.C. to A.D. 1980</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1984RSPTA.313...47S'>1984</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case StephensonHoulden:
			description = q_("This algorithm is used in the PC planetarium program Guide 7.").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case Espenak: // limited range, but wide availability?
			description = q_("This algorithm was given by F. Espenak in his <em>Fifty Year Canon of Solar Eclipses: 1986-2035</em> (1987) and in his <em>Fifty Year Canon of Lunar Eclipses: 1986-2035</em> (1989).").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case Borkowski: // Linked to ELP2000-85, so it's important...
			description = q_("This formula was obtained by K.M. Borkowski (%1) from an analysis of 31 solar eclipse records dating between 2137 BCE and 1715 CE.").arg("<a href='http://adsabs.harvard.edu/abs/1988A&A...205L...8B'>1988</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case SchmadelZech1988: // data fit through Stephenson&Morrison1984, which itself is important. Unclear who uses this version?
			description = q_("This 12th-order polynomial equation was published by L. D. Schmadel and G. Zech in the article <em>Empirical Transformations from U.T. to E.T. for the Period 1800-1988</em> (%1) as data fit through values given by Stephenson & Morrison (1984).").arg("<a href='http://adsabs.harvard.edu/abs/1988AN....309..219S'>1988</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case ChaprontTouze:
			description = q_("This formula was adopted by M. Chapront-Touze & J. Chapront in the shortened version of the ELP 2000-85 lunar theory in their <em>Lunar Tables and Programs from 4000 B.C. to A.D. 8000</em> (1991).").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case StephensonMorrison1995:
			description = q_("This equation was published by F. R. Stephenson and L. V. Morrison in the article <em>Long-Term Fluctuations in the Earth's Rotation: 700 BC to AD 1990</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/1995RSPTA.351..165S'>1995</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case Stephenson1997:
			description = q_("F. R. Stephenson published this formula in his book <em>Historical Eclipses and Earth's Rotation</em> (%1).").arg("<a href='http://ebooks.cambridge.org/ebook.jsf?bid=CBO9780511525186'>1997</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case ChaprontMeeus:
			description = q_("From J. Meeus, <em>Astronomical Algorithms</em> (2nd ed., 1998), and widely used. Table for 1620..2000, and includes a variant of Chapront, Chapront-Touze & Francou (1997) for dates outside 1620..2000.").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case JPLHorizons:
			description = q_("The JPL Solar System Dynamics Group of the NASA Jet Propulsion Laboratory use this formula in their interactive website %1JPL Horizons%2.").arg("<a href='http://ssd.jpl.nasa.gov/?horizons'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case MeeusSimons:
			description = q_("This polynome was published by J. Meeus and L. Simons in article <em>Polynomial approximations to Delta T, 1620-2000 AD</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/2000JBAA..110..323M'>2000</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case MontenbruckPfleger: // uninspired
			description = q_("The fourth edition of O. Montenbruck & T. Pfleger's <em>Astronomy on the Personal Computer</em> (2000) provides simple 3rd-order polynomial data fits for the recent past.").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case ReingoldDershowitz: //
			description = q_("E. M. Reingold & N. Dershowitz present this polynomial data fit in <em>Calendrical Calculations</em> (3rd ed. 2007) and in their <em>Calendrical Tabulations</em> (2002). It is based on Jean Meeus' <em>Astronomical Algorithms</em> (1991).").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case MorrisonStephenson2004: // PRIMARY SOURCE
			description = q_("This important solution was published by L. V. Morrison and F. R. Stephenson in article <em>Historical values of the Earth's clock error %1T and the calculation of eclipses</em> (%2) with addendum in (%3).").arg(QChar(0x0394)).arg("<a href='http://adsabs.harvard.edu/abs/2004JHA....35..327M'>2004</a>").arg("<a href='http://adsabs.harvard.edu/abs/2005JHA....36..339M'>2005</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case Reijs:
			description = q_("From the Length of Day (LOD; as determined by Stephenson & Morrison (%2)), Victor Reijs derived a %1T formula by using a Simplex optimisation with a cosine and square function. This is based on a possible periodicy described by Stephenson (%2). See for more info %3here%4.").arg(QChar(0x0394)).arg("<a href='http://adsabs.harvard.edu/abs/2004JHA....35..327M'>2004</a>").arg("<a href='http://www.iol.ie/~geniet/eng/DeltaTeval.htm'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case EspenakMeeus: // GENERAL SOLUTION
			description = q_("This solution by F. Espenak and J. Meeus, based on Morrison & Stephenson (2004) and a polynomial fit through tabulated values for 1600-2000, is used for the %1NASA Eclipse Web Site%2 and in their <em>Five Millennium Canon of Solar Eclipses: -1900 to +3000</em> (2006). This formula is also used in the solar, lunar and planetary ephemeris program SOLEX.").arg("<a href='http://eclipse.gsfc.nasa.gov/eclipse.html'>").arg("</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker)).append(" <em>").append(q_("Used by default.")).append("</em>");
			break;
		case Banjevic:
			description = q_("This solution by B. Banjevic, based on Stephenson & Morrison (1984), was published in article <em>Ancient eclipses and dating the fall of Babylon</em> (%1).").arg("<a href='http://adsabs.harvard.edu/abs/2006POBeo..80..251B'>2006</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case IslamSadiqQureshi:
			description = q_("This solution by S. Islam, M. Sadiq and M. S. Qureshi, based on Meeus & Simons (2000), was published in article <em>Error Minimization of Polynomial Approximation of DeltaT</em> (%1) and revisited by Sana Islam in 2013.").arg("<a href='http://www.ias.ac.in/jaa/dec2008/JAA610.pdf'>2008</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case KhalidSultanaZaidi:
			description = q_("This polynomial approximation with 0.6 seconds of accuracy by M. Khalid, Mariam Sultana and Faheem Zaidi was published in <em>Delta T: Polynomial Approximation of Time Period 1620-2013</em> (%1).").arg("<a href='http://dx.doi.org/10.1155/2014/480964'>2014</a>").append(getCurrentDeltaTAlgorithmValidRange(jd, &marker));
			break;
		case Custom:
			description = q_("This is a quadratic formula for calculation of %1T with coefficients defined by the user.").arg(QChar(0x0394));
			break;
		default:
			description = q_("Error");
	}
	return description;
}

QString StelCore::getCurrentDeltaTAlgorithmValidRange(double jDay, QString *marker) const
{
	QString validRange = "";
	QString validRangeAppendix = "";
	*marker = "";
	int year, month, day;
	int start = 0;
	int finish = 0;
	StelUtils::getDateFromJulianDay(jDay, &year, &month, &day);
	switch (getCurrentDeltaTAlgorithm())
	{
		case WithoutCorrection:
			// say nothing
			break;
		case Schoch:
			// Valid range unknown
			break;
		case Clemence:
			start	= 1681;
			finish	= 1900;
			break;
		case IAU:
			start	= 1681;
			finish	= 1900;
			break;
		case AstronomicalEphemeris:
			// GZ: What is the source of "1681..1900"? Expl.Suppl.AE 1961-p87 says "...over periods extending back to ancient times"
			// I changed to what I estimate.
			start	= -500; // 1681;
			finish	=  2000; // 1900;
			break;
		case TuckermanGoldstine:
			start	= -600;
			finish	= 1649;
			break;
		case MullerStephenson:
			start	= -1375;
			finish	= 1975;
			break;
		case Stephenson1978:
			// Valid range unknown
			break;
		case SchmadelZech1979:
			start	= 1800;
			finish	= 1975;
			validRangeAppendix = q_("with meaningless values outside this range");
			break;
		case MorrisonStephenson1982:
			// FIXME: This is correct valid range?
			start	= -4000;
			finish	= 2800;
			break;
		case StephensonMorrison1984:
			start	= -391;
			finish	= 1600;
			break;
		case StephensonHoulden:
			start	= -600;
			finish	= 1600;
			break;
		case Espenak:
			start	= 1950;
			finish	= 2100;
			break;
		case Borkowski:
			start	= -2136;
			finish	= 1715;
			break;
		case SchmadelZech1988:
			start	= 1800;
			finish	= 1988;
			validRangeAppendix = q_("with a mean error of less than one second, max. error 1.9s, and meaningless values outside this range");
			break;
		case ChaprontTouze:
			// FIXME: It's valid range?
			start	= -4000;
			finish	= 8000;
			break;
		case StephensonMorrison1995:
			start	= -700;
			finish	= 1600;
			break;
		case Stephenson1997:
			start	= -500;
			finish	= 1600;
			break;
		case ChaprontMeeus:
			start	= -400; // 1800; // not explicitly given, but guess based on his using ChaprontFrancou which is cited elsewhere in a similar term with -391.
			finish	=  2150; // 1997;
			break;
		case JPLHorizons:
			start	= -2999;
			finish	= 1620;
			validRangeAppendix = q_("with zero values outside this range");
			break;
		case MeeusSimons:
			start	= 1620;
			finish	= 2000;
			validRangeAppendix = q_("with zero values outside this range");
			break;
		case MontenbruckPfleger:
			start	= 1825;
			finish	= 2005;
			validRangeAppendix = q_("with a typical 1-second accuracy and zero values outside this range");
			break;
		case ReingoldDershowitz:
			// GZ: while not original work, it's based on Meeus and therefore the full implementation covers likewise approximately:
			start	= -400; //1620;
			finish	= 2100; //2019;
			break;
		case MorrisonStephenson2004:
			start	= -1000;
			finish	= 2000;
			break;
		case Reijs:
			start	= -1500; // -500; // GZ: It models long-term variability, so we should reflect this. Not sure on the begin, though.
			finish	= 1100; // not 1620; // GZ: Not applicable for telescopic era, and better not after 1100 (pers.comm.)
			break;
		case EspenakMeeus: // the default, range stated in the Canon, p. 14.
			start	= -1999;
			finish	= 3000;
			break;
		case Banjevic:
			start	= -2020;
			finish	= 1620;
			validRangeAppendix = q_("with zero values outside this range");
			break;
		case IslamSadiqQureshi:
			start	= 1620;
			finish	= 2007;
			validRangeAppendix = q_("with zero values outside this range");
			break;
		case KhalidSultanaZaidi:
			start	= 1620;
			finish	= 2013;
			validRangeAppendix = q_("with zero values outside this range");
			break;
		case Custom:
			// Valid range unknown
			break;
	}

	if (start!=finish)
	{
		if (validRangeAppendix!="")
			validRange = q_("Valid range of usage: between years %1 and %2, %3.").arg(start).arg(finish).arg(validRangeAppendix);
		else
			validRange = q_("Valid range of usage: between years %1 and %2.").arg(start).arg(finish);
		if (start > year || year > finish)
			*marker = "*";
	}
	else
		*marker = "?";

	return QString(" %1").arg(validRange);
}

bool StelCore::isDay() const
{
	const Vec3d& sunPos = GETSTELMODULE(SolarSystem)->getSun()->getAltAzPosGeometric(this);
	return sunPos[2] > -0.12; // Nautical twilight (0.12 > sin (6 deg),
}

double StelCore::getCurrentEpoch() const
{
	int year, month, day;
	StelUtils::getDateFromJulianDay(getJDay(), &year, &month, &day);
	QDate date = QDate::fromString(QString("%1.%2.%3").arg(year, 4, 10, QLatin1Char('0')).arg(month).arg(day), "yyyy.M.d");
	return double(year) + double(date.dayOfYear())/double(date.daysInYear());
}
