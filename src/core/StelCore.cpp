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
#include "StelLocationMgr.hpp"
#include "StelObserver.hpp"
#include "StelObjectMgr.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "renderer/GenericVertexTypes.hpp"
#include "renderer/StelRenderer.hpp"
#include "LandscapeMgr.hpp"

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


StelCore::StelCore() : movementMgr(NULL), geodesicGrid(NULL), currentProjectionType(ProjectionStereographic), position(NULL), timeSpeed(JD_SECOND), JDay(0.)
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
void StelCore::init(class StelRenderer* renderer)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	defaultLocationID = conf->value("init_location/location","error").toString();
	bool ok;
	StelLocation location = StelApp::getInstance().getLocationMgr().locationForString(defaultLocationID, &ok);
	if (!ok)
	{
		qWarning() << "Warning: location" << defaultLocationID << "is unknown.";
	}
	position = new StelObserver(location);

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

	skyDrawer = new StelSkyDrawer(this, renderer);
	skyDrawer->init();

	QString tmpstr = conf->value("projection/type", "ProjectionStereographic").toString();
	setCurrentProjectionTypeKey(tmpstr);
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
	currentProjectorParams.zNear = 0.000001;
	currentProjectorParams.zFar = 50.;
	skyDrawer->preDraw();
}

//! Fill with black around viewport disc shape.
static void drawViewportShape(StelRenderer* renderer, StelProjectorP projector)
{
	if (projector->getMaskType() != StelProjector::MaskDisk)
	{
		return;
	}

	renderer->setBlendMode(BlendMode_None);
	renderer->setGlobalColor(0.0f, 0.0f, 0.0f);

	const float innerRadius = 0.5 * projector->getViewportFovDiameter();
	const float outerRadius = projector->getViewportWidth() + projector->getViewportHeight();
	Q_ASSERT_X(innerRadius >= 0.0f && outerRadius > innerRadius,
	           Q_FUNC_INFO, "Inner radius must be at least zero and outer radius must be greater");

	const float sweepAngle = 360.0f;

	static const int resolution = 192;

	float sinCache[resolution];
	float cosCache[resolution];

	const float deltaRadius = outerRadius - innerRadius;
	const int slices = resolution - 1;
	// Cache is the vertex locations cache
	for (int s = 0; s <= slices; s++)
	{
		const float angle = (M_PI * sweepAngle) / 180.0f * s / slices;
		sinCache[s] = std::sin(angle);
		cosCache[s] = std::cos(angle);
	}
	sinCache[slices] = sinCache[0];
	cosCache[slices] = cosCache[0];

	const float radiusHigh = outerRadius - deltaRadius;

	StelVertexBuffer<VertexP2>* vertices = 
		renderer->createVertexBuffer<VertexP2>(PrimitiveType_TriangleStrip);

	const Vec2f center = projector->getViewportCenterAbsolute();
	for (int i = 0; i <= slices; i++)
	{
		vertices->addVertex(VertexP2(center[0] + outerRadius * sinCache[i],
		                             center[1] + outerRadius * cosCache[i]));
		vertices->addVertex(VertexP2(center[0] + radiusHigh * sinCache[i],
		                             center[1] + radiusHigh * cosCache[i]));
	}

	vertices->lock();
	renderer->setCulledFaces(CullFace_None);
	renderer->drawVertexBuffer(vertices);
	delete vertices;
}

/*************************************************************************
 Update core state after drawing modules
*************************************************************************/
void StelCore::postDraw(StelRenderer* renderer)
{
	drawViewportShape(renderer, getProjection(StelCore::FrameJ2000));
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
	ProjectionType newType = (ProjectionType)en.keyToValue(key.toAscii().data());
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
	QString s(getProjection(StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(Mat4d::identity())), (ProjectionType)en.keyToValue(key.toAscii()))->getNameI18());
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
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matAltAzToEquinoxEqu*v;
	Vec3d r(v);
	skyDrawer->getRefraction().backward(r);
	r.transfo4d(matAltAzToEquinoxEqu);
	return r;
}

Vec3d StelCore::equinoxEquToAltAz(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matEquinoxEquToAltAz*v;
	Vec3d r(v);
	r.transfo4d(matEquinoxEquToAltAz);
	skyDrawer->getRefraction().forward(r);
	return r;
}

Vec3d StelCore::altAzToJ2000(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return matEquinoxEquToJ2000*matAltAzToEquinoxEqu*v;
	Vec3d r(v);
	skyDrawer->getRefraction().backward(r);
	r.transfo4d(matEquinoxEquToJ2000*matAltAzToEquinoxEqu);
	return r;
}

Vec3d StelCore::j2000ToAltAz(const Vec3d& v, RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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

//! Transform vector from heliocentric coordinate to false equatorial : equatorial
//! coordinate but centered on the observer position (usefull for objects close to earth)
Vec3d StelCore::heliocentricEclipticToEarthPosEquinoxEqu(const Vec3d& v) const
{
	return matAltAzToEquinoxEqu*matHeliocentricEclipticToAltAz*v;
}

StelProjector::ModelViewTranformP StelCore::getHeliocentricEclipticModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matHeliocentricEclipticToAltAz));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matHeliocentricEclipticToAltAz);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

//! Get the modelview matrix for observer-centric ecliptic (Vsop87) drawing
StelProjector::ModelViewTranformP StelCore::getObservercentricEclipticModelViewTransform(RefractionMode refMode) const
{
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
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
	if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
		return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matGalacticToJ2000));
	Refraction* refr = new Refraction(skyDrawer->getRefraction());
	// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
	refr->setPreTransfoMat(matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matGalacticToJ2000);
	refr->setPostTransfoMat(matAltAzModelView);
	return StelProjector::ModelViewTranformP(refr);
}

void StelCore::updateTransformMatrices()
{
	matAltAzToEquinoxEqu = position->getRotAltAzToEquatorial(JDay);
	matEquinoxEquToAltAz = matAltAzToEquinoxEqu.transpose();

	matEquinoxEquToJ2000 = matVsop87ToJ2000 * position->getRotEquatorialToVsop87();
	matJ2000ToEquinoxEqu = matEquinoxEquToJ2000.transpose();
	matJ2000ToAltAz = matEquinoxEquToAltAz*matJ2000ToEquinoxEqu;

	matHeliocentricEclipticToEquinoxEqu = matJ2000ToEquinoxEqu * matVsop87ToJ2000 * Mat4d::translation(-position->getCenterVsop87Pos());

	// These two next have to take into account the position of the observer on the earth
	Mat4d tmp = matJ2000ToVsop87 * matEquinoxEquToJ2000 * matAltAzToEquinoxEqu;

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
	bool ok = false;
	StelApp::getInstance().getLocationMgr().locationForSmallString(id, &ok);
	if (!ok)
		return;
	defaultLocationID = id;
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("init_location/location", id);
}

void StelCore::returnToDefaultLocation()
{
	StelLocationMgr& locationMgr = StelApp::getInstance().getLocationMgr();
	bool ok = false;
	StelLocation loc = locationMgr.locationForString(defaultLocationID, &ok);
	if (ok)
		moveObserverTo(loc, 0.);
}

void StelCore::returnToHome()
{
	// Using returnToDefaultLocation() and getCurrentLocation() introduce issue, because for flying
	// between planets using SpaceShip and second method give does not exist data
	StelLocationMgr& locationMgr = StelApp::getInstance().getLocationMgr();
	bool ok = false;
	StelLocation loc = locationMgr.locationForString(defaultLocationID, &ok);
	if (ok)
		moveObserverTo(loc, 0.);

	PlanetP p = GETSTELMODULE(SolarSystem)->searchByEnglishName(loc.planetName);

	LandscapeMgr* landscapeMgr = GETSTELMODULE(LandscapeMgr);
	landscapeMgr->setCurrentLandscapeID(landscapeMgr->getDefaultLandscapeID());
	landscapeMgr->setFlagAtmosphere(p->hasAtmosphere());
	landscapeMgr->setFlagFog(p->hasAtmosphere());

	GETSTELMODULE(StelObjectMgr)->unSelect();

	StelMovementMgr* smmgr = getMovementMgr();
	smmgr->setViewDirectionJ2000(altAzToJ2000(smmgr->getInitViewingDirection(), StelCore::RefractionOff));
}

void StelCore::setJDay(double JD)
{
	JDay=JD;
}

double StelCore::getJDay() const
{
	return JDay;
}

void StelCore::setMJDay(double MJD)
{
	JDay=MJD+2400000.5;
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
	timeSpeed=ts; emit timeRateChanged(timeSpeed);
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
	setJDay(StelUtils::getJDFromSystem());
}

void StelCore::setTodayTime(const QTime& target)
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
		qWarning() << "WARNING - time passed to StelCore::setTodayTime is not valid. The system time will be used." << target;
		setTimeNow();
	}
}

//! Get whether the current stellarium time is the real world time
bool StelCore::getIsTimeNow(void) const
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
	addSolarDays(365.2421897);
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
	addSolarDays(-365.2421897);
}

void StelCore::subtractTropicalCentury()
{
	addSolarDays(-36524.21897);
}

void StelCore::addSolarDays(double d)
{
	const PlanetP& home = position->getHomePlanet();
	if (home->getEnglishName() != "Solar System Observer")
	{
		double sp = home->getSiderealPeriod();
		double dsol = home->getSiderealDay();
		bool fwdDir = true;

		if ((home->getEnglishName() == "Venus") || (home->getEnglishName() == "Uranus") || (home->getEnglishName() == "Pluto"))
			fwdDir = false;

		d *= StelUtils::calculateSolarDay(sp, dsol, fwdDir);
	}

	setJDay(getJDay() + d);
}

void StelCore::addSiderealDays(double d)
{
	const PlanetP& home = position->getHomePlanet();
	if (home->getEnglishName() != "Solar System Observer")
		d *= home->getSiderealDay();
	setJDay(getJDay() + d);
}

// Get the sideral time shifted by the observer longitude
double StelCore::getLocalSideralTime() const
{
	return (position->getHomePlanet()->getSiderealTime(JDay)+position->getCurrentLocation().longitude)*M_PI/180.;
}

//! Get the duration of a sideral day for the current observer in day.
double StelCore::getLocalSideralDayLength() const
{
	return position->getHomePlanet()->getSiderealDay();
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
	JDay+=timeSpeed*deltaTime;

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

void StelCore::setStartupTimeMode(const QString& s)
{
	startupTimeMode = s;
}
