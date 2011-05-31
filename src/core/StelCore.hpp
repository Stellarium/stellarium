/*
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

#ifndef _STELCORE_HPP_
#define _STELCORE_HPP_

#include "StelProjector.hpp"
#include "StelProjectorType.hpp"
#include "StelLocation.hpp"
#include "StelSkyDrawer.hpp"
#include <QString>
#include <QStringList>
#include <QTime>

class StelToneReproducer;
class StelSkyDrawer;
class StelGeodesicGrid;
class StelMovementMgr;
class StelObserver;

//! @class StelCore
//! Main class for Stellarium core processing.
//! This class provides services like management of sky projections,
//! tone conversion, or reference frame conversion. It is used by the
//! various StelModules to update and display themself.
//! There is currently only one StelCore instance in Stellarium, but
//! in the future they may be more, allowing for example to display
//! several independent views of the sky at the same time.
//! @author Fabien Chereau
class StelCore : public QObject
{
	Q_OBJECT
	Q_ENUMS(ProjectionType)

public:
	//! @enum FrameType
	//! Supported reference frame types
	enum FrameType
	{
		FrameAltAz,                   //!< Altazimuthal reference frame centered on observer.
		FrameHeliocentricEcliptic,    //!< Ecliptic reference frame centered on the Sun
		FrameObservercentricEcliptic, //!< Ecliptic reference frame centered on the Observer
		FrameEquinoxEqu,              //!< Equatorial reference frame at the current equinox centered on the observer.
									  //! The north pole follows the precession of the planet on which the observer is located.
		FrameJ2000,                   //!< Equatorial reference frame at the J2000 equinox centered on the observer.
									  //! This is also the ICRS reference frame.
		FrameGalactic                 //! Galactic reference frame centered on observer.
	};

	//! Available projection types. A value of 1000 indicate the default projection
	enum ProjectionType
	{
		ProjectionPerspective,    //!< Perspective projection
		ProjectionEqualArea,      //!< Equal Area projection
		ProjectionStereographic,  //!< Stereograhic projection
		ProjectionFisheye,	      //!< Fisheye projection
		ProjectionHammer,         //!< Hammer-Aitoff projection
		ProjectionCylinder,	      //!< Cylinder projection
		ProjectionMercator,	      //!< Mercator projection
		ProjectionOrthographic	  //!< Orthographic projection
	};

	//! Available refraction mode.
	enum RefractionMode
	{
		RefractionAuto,			//!< Automatically decide to add refraction if atmosphere is activated
		RefractionOn,			//!< Always add refraction (i.e. apparent coordinates)
		RefractionOff			//!< Never add refraction (i.e. geometric coordinates)
	};

	StelCore();
	virtual ~StelCore();

	//! Init and load all main core components.
	void init();

	//! Update all the objects with respect to the time.
	//! @param deltaTime the time increment in sec.
	void update(double deltaTime);

	//! Handle the resizing of the window
	void windowHasBeenResized(float x, float y, float width, float height);

	//! Update core state before drawing modules.
	void preDraw();

	//! Update core state after drawing modules.
	void postDraw();

	//! Get a new instance of a simple 2d projection. This projection cannot be used to project or unproject but
	//! only for 2d painting
	StelProjectorP getProjection2d() const;

	//! Get a new instance of projector using a modelview transformation corresponding to the the given frame.
	//! If not specified the refraction effect is included if atmosphere is on.
	StelProjectorP getProjection(FrameType frameType, RefractionMode refractionMode=RefractionAuto) const;

	//! Get a new instance of projector using the given modelview transformatione.
	//! If not specified the projection used is the one currently used as default.
	StelProjectorP getProjection(StelProjector::ModelViewTranformP modelViewTransform, ProjectionType projType=(ProjectionType)1000) const;

	//! Get the current tone reproducer used in the core.
	StelToneReproducer* getToneReproducer() {return toneConverter;}
	//! Get the current tone reproducer used in the core.
	const StelToneReproducer* getToneReproducer() const {return toneConverter;}

	//! Get the current StelSkyDrawer used in the core.
	StelSkyDrawer* getSkyDrawer() {return skyDrawer;}
	//! Get the current StelSkyDrawer used in the core.
	const StelSkyDrawer* getSkyDrawer() const {return skyDrawer;}

	//! Get an instance of StelGeodesicGrid which is garanteed to allow for at least maxLevel levels
	const StelGeodesicGrid* getGeodesicGrid(int maxLevel) const;

	//! Get the instance of movement manager.
	StelMovementMgr* getMovementMgr() {return movementMgr;}
	//! Get the const instance of movement manager.
	const StelMovementMgr* getMovementMgr() const {return movementMgr;}

	//! Set the near and far clipping planes.
	void setClippingPlanes(double znear, double zfar) {currentProjectorParams.zNear=znear;currentProjectorParams.zFar=zfar;}
	//! Get the near and far clipping planes.
	void getClippingPlanes(double* zn, double* zf) const {*zn = currentProjectorParams.zNear; *zf = currentProjectorParams.zFar;}

	//! Get the translated projection name from its TypeKey for the current locale.
	QString projectionTypeKeyToNameI18n(const QString& key) const;

	//! Get the projection TypeKey from its translated name for the current locale.
	QString projectionNameI18nToTypeKey(const QString& nameI18n) const;

	//! Get the current set of parameters to use when creating a new StelProjector.
	StelProjector::StelProjectorParams getCurrentStelProjectorParams() const {return currentProjectorParams;}
	//! Set the set of parameters to use when creating a new StelProjector.
	void setCurrentStelProjectorParams(const StelProjector::StelProjectorParams& newParams) {currentProjectorParams=newParams;}

	//! Get vision direction
	void lookAtJ2000(const Vec3d& pos, const Vec3d& up);

	Vec3d altAzToEquinoxEqu(const Vec3d& v, RefractionMode refMode=RefractionAuto) const
	{
		if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
			return matAltAzToEquinoxEqu*v;
		Vec3d r(v);
		skyDrawer->getRefraction().backward(r);
		r.transfo4d(matAltAzToEquinoxEqu);
		return r;
	}
	Vec3d equinoxEquToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const
	{
		if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
			return matEquinoxEquToAltAz*v;
		Vec3d r(v);
		r.transfo4d(matEquinoxEquToAltAz);
		skyDrawer->getRefraction().forward(r);
		return r;
	}
	Vec3d altAzToJ2000(const Vec3d& v, RefractionMode refMode=RefractionAuto) const
	{
		if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
			return matEquinoxEquToJ2000*matAltAzToEquinoxEqu*v;
		Vec3d r(v);
		skyDrawer->getRefraction().backward(r);
		r.transfo4d(matEquinoxEquToJ2000*matAltAzToEquinoxEqu);
		return r;
	}
	Vec3d j2000ToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const
	{
		if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
			return matJ2000ToAltAz*v;
		Vec3d r(v);
		r.transfo4d(matJ2000ToAltAz);
		skyDrawer->getRefraction().forward(r);
		return r;
	}
	Vec3d galacticToJ2000(const Vec3d& v) const {return matGalacticToJ2000*v;}
	Vec3d equinoxEquToJ2000(const Vec3d& v) const {return matEquinoxEquToJ2000*v;}
	Vec3d j2000ToEquinoxEqu(const Vec3d& v) const {return matJ2000ToEquinoxEqu*v;}
	Vec3d j2000ToGalactic(const Vec3d& v) const {return matJ2000ToGalactic*v;}

	//! Transform vector from heliocentric ecliptic coordinate to altazimuthal
	Vec3d heliocentricEclipticToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const
	{
		if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
			return matHeliocentricEclipticToAltAz*v;
		Vec3d r(v);
		r.transfo4d(matHeliocentricEclipticToAltAz);
		skyDrawer->getRefraction().forward(r);
		return r;
	}

	//! Transform from heliocentric coordinate to equatorial at current equinox (for the planet where the observer stands)
	Vec3d heliocentricEclipticToEquinoxEqu(const Vec3d& v) const {return matHeliocentricEclipticToEquinoxEqu*v;}
	//! Transform vector from heliocentric coordinate to false equatorial : equatorial
	//! coordinate but centered on the observer position (usefull for objects close to earth)
	Vec3d heliocentricEclipticToEarthPosEquinoxEqu(const Vec3d& v) const {return matAltAzToEquinoxEqu*matHeliocentricEclipticToAltAz*v;}

	//! Get the modelview matrix for heliocentric ecliptic (Vsop87) drawing
	StelProjector::ModelViewTranformP getHeliocentricEclipticModelViewTransform(RefractionMode refMode=RefractionAuto) const
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
	StelProjector::ModelViewTranformP getObservercentricEclipticModelViewTransform(RefractionMode refMode=RefractionAuto) const
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
	StelProjector::ModelViewTranformP getEquinoxEquModelViewTransform(RefractionMode refMode=RefractionAuto) const
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
	StelProjector::ModelViewTranformP getAltAzModelViewTransform(RefractionMode refMode=RefractionAuto) const
	{
		if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
			return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView));
		Refraction* refr = new Refraction(skyDrawer->getRefraction());
		// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
		refr->setPostTransfoMat(matAltAzModelView);
		return StelProjector::ModelViewTranformP(refr);
	}

	//! Get the modelview matrix for observer-centric J2000 equatorial drawing
	StelProjector::ModelViewTranformP getJ2000ModelViewTransform(RefractionMode refMode=RefractionAuto) const
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
	StelProjector::ModelViewTranformP getGalacticModelViewTransform(RefractionMode refMode=RefractionAuto) const
	{
		if (refMode==RefractionOff || skyDrawer==false || (refMode==RefractionAuto && skyDrawer->getFlagHasAtmosphere()==false))
			return StelProjector::ModelViewTranformP(new StelProjector::Mat4dTransform(matAltAzModelView*matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matGalacticToJ2000));
		Refraction* refr = new Refraction(skyDrawer->getRefraction());
		// The pretransform matrix will convert from input coordinates to AltAz needed by the refraction function.
		refr->setPreTransfoMat(matEquinoxEquToAltAz*matJ2000ToEquinoxEqu*matGalacticToJ2000);
		refr->setPostTransfoMat(matAltAzModelView);
		return StelProjector::ModelViewTranformP(refr);
	}

	//! Rotation matrix from equatorial J2000 to ecliptic (Vsop87)
	static const Mat4d matJ2000ToVsop87;
	//! Rotation matrix from ecliptic (Vsop87) to equatorial J2000
	static const Mat4d matVsop87ToJ2000;
	//! Rotation matrix from J2000 to Galactic reference frame, using FITS convention.
	static const Mat4d matJ2000ToGalactic;
	//! Rotation matrix from J2000 to Galactic reference frame, using FITS convention.
	static const Mat4d matGalacticToJ2000;

	//! Return the observer heliocentric ecliptic position
	Vec3d getObserverHeliocentricEclipticPos() const;

	//! Get the informations on the current location
	const StelLocation& getCurrentLocation() const;

	//! Smoothly move the observer to the given location
	//! @param target the target location
	//! @param duration direction of view move duration in s
	//! @param durationIfPlanetChange direction of view + planet travel move duration in s.
	//! This is used only if the destination planet is different from the starting one.
	void moveObserverTo(const StelLocation& target, double duration=1., double durationIfPlanetChange=1.);

	// Conversion in standar Julian time format
	static const double JD_SECOND;
	static const double JD_MINUTE;
	static const double JD_HOUR;
	static const double JD_DAY;

	//! Get the sideral time shifted by the observer longitude
	//! @return the locale sideral time in radian
	double getLocalSideralTime() const;

	//! Get the duration of a sideral day for the current observer in day.
	double getLocalSideralDayLength() const;

	//! Return the startup mode, can be preset|Preset or anything else
	QString getStartupTimeMode() {return startupTimeMode;}
	void setStartupTimeMode(const QString& s);

public slots:
	//! Set the current ProjectionType to use
	void setCurrentProjectionType(ProjectionType type);
	ProjectionType getCurrentProjectionType() const {return currentProjectionType;}

	//! Get the current Mapping used by the Projection
	QString getCurrentProjectionTypeKey(void) const;
	//! Set the current ProjectionType to use from its key
	void setCurrentProjectionTypeKey(QString type);

	//! Get the list of all the available projections
	QStringList getAllProjectionTypeKeys() const;

	//! Set the mask type.
	void setMaskType(StelProjector::StelProjectorMaskType m) {currentProjectorParams.maskType = m; }

	//! Set the flag with decides whether to arrage labels so that
	//! they are aligned with the bottom of a 2d screen, or a 3d dome.
	void setFlagGravityLabels(bool gravity) { currentProjectorParams.gravityLabels = gravity; }
	//! Set the offset rotation angle in degree to apply to gravity text (only if gravityLabels is set to false).
	void setDefautAngleForGravityText(float a) { currentProjectorParams.defautAngleForGravityText = a; }
	//! Set the horizontal flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipHorz(bool flip) {currentProjectorParams.flipHorz = flip;}
	//! Set the vertical flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipVert(bool flip) {currentProjectorParams.flipVert = flip;}
	//! Get the state of the horizontal flip.
	//! @return True if flipped horizontally, else false.
	bool getFlipHorz(void) const {return currentProjectorParams.flipHorz;}
	//! Get the state of the vertical flip.
	//! @return True if flipped vertically, else false.
	bool getFlipVert(void) const {return currentProjectorParams.flipVert;}

	//! Get the location used by default at startup
	QString getDefaultLocationID() const {return defaultLocationID;}
	//! Set the location to use by default at startup
	void setDefaultLocationID(const QString& id);


	//! Set the current date in Julian Day
	void setJDay(double JD) {JDay=JD;}
	//! Get the current date in Julian Day
	double getJDay() const {return JDay;}

	//! Return the preset sky time in JD
	double getPresetSkyTime() const {return presetSkyTime;}
	//! Set the preset sky time from a JD
	void setPresetSkyTime(double d) {presetSkyTime=d;}

	//! Set time speed in JDay/sec
	void setTimeRate(double ts) {timeSpeed=ts; emit timeRateChanged(timeSpeed);}
	//! Get time speed in JDay/sec
	double getTimeRate() const {return timeSpeed;}

	//! Increase the time speed
	void increaseTimeSpeed();
	//! Decrease the time speed
	void decreaseTimeSpeed();
	//! Increase the time speed, but not as much as with increaseTimeSpeed()
	void increaseTimeSpeedLess();
	//! Decrease the time speed but not as much as with decreaseTimeSpeed()
	void decreaseTimeSpeedLess();

	//! Set time speed to 0, i.e. freeze the passage of simulation time
	void setZeroTimeSpeed() {setTimeRate(0);}
	//! Set real time speed, i.e. 1 sec/sec
	void setRealTimeSpeed() {setTimeRate(JD_SECOND);}
	//! Set real time speed or pause simulation if we are already in realtime speed.
	void toggleRealTimeSpeed() {(!getRealTimeSpeed()) ? setRealTimeSpeed() : setZeroTimeSpeed();}
	//! Get whether it is real time speed, i.e. 1 sec/sec
	bool getRealTimeSpeed() const {return (fabs(timeSpeed-JD_SECOND)<0.0000001);}

	//! Set stellarium time to current real world time
	void setTimeNow();
	//! Set the time to some value, leaving the day the same.
	void setTodayTime(const QTime& target);
	//! Get whether the current stellarium time is the real world time
	bool getIsTimeNow() const;

	//! get the initial "today time" from the config file
	QTime getInitTodayTime(void) {return initTodayTime;}
	//! set the initial "today time" from the config file
	void setInitTodayTime(const QTime& t) {initTodayTime=t;}
	//! Set the preset sky time from a QDateTime
	void setPresetSkyTime(QDateTime dt);

	//! Add one [Earth, solar] hour to the current simulation time.
	void addHour() {addSolarDays(0.04166666666666666667);}
	//! Add one [Earth, solar] day to the current simulation time.
	void addDay() {addSolarDays(1.0);}
	//! Add one [Earth, solar] week to the current simulation time.
	void addWeek() {addSolarDays(7.0);}

	//! Add one sidereal day to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void addSiderealDay() {addSiderealDays(1.0);}
	//! Add one sidereal week to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void addSiderealWeek() {addSiderealDays(7.0);}

	//! Subtract one [Earth, solar] hour to the current simulation time.
	void subtractHour() {addSolarDays(-0.04166666666666666667);}
	//! Subtract one [Earth, solar] day to the current simulation time.
	void subtractDay() {addSolarDays(-1.0);}
	//! Subtract one [Earth, solar] week to the current simulation time.
	void subtractWeek() {addSolarDays(-7.0);}

	//! Subtract one sidereal week to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void subtractSiderealDay() {addSiderealDays(-1.0);}
	//! Subtract one sidereal week to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void subtractSiderealWeek() {addSiderealDays(-7.0);}

	//! Add a number of Earth Solar days to the current simulation time
	//! @param d the decimal number of days to add (use negative values to subtract)
	void addSolarDays(double d);
	//! Add a number of sidereal days to the current simulation time,
	//! based on the observer body's rotational period.
	//! @param d the decimal number of sidereal days to add (use negative values to subtract)
	void addSiderealDays(double d);

	//! Move the observer to the selected object. This will only do something if
	//! the selected object is of the correct type - i.e. a planet.
	void moveObserverToSelected();

signals:
	//! This signal is emitted when the observer location has changed.
	void locationChanged(StelLocation);
	//! This signal is emitted when the time rate has changed
	void timeRateChanged(double rate);

private:
	StelToneReproducer* toneConverter;		// Tones conversion between stellarium world and display device
	StelSkyDrawer* skyDrawer;
	StelMovementMgr* movementMgr;		// Manage vision movements

	// Manage geodesic grid
	mutable StelGeodesicGrid* geodesicGrid;

	// The currently used projection type
	ProjectionType currentProjectionType;

	// Parameters to use when creating new instances of StelProjector
	StelProjector::StelProjectorParams currentProjectorParams;

	void updateTransformMatrices();
	void updateTime(double deltaTime);

	// Matrices used for every coordinate transfo
	Mat4d matHeliocentricEclipticToAltAz;	// Transform from heliocentric ecliptic (Vsop87) to observer-centric altazimuthal coordinate
	Mat4d matAltAzToHeliocentricEcliptic;	// Transform from observer-centric altazimuthal coordinate to heliocentric ecliptic (Vsop87)
	Mat4d matAltAzToEquinoxEqu;				// Transform from observer-centric altazimuthal coordinate to Earth Equatorial
	Mat4d matEquinoxEquToAltAz;				// Transform from observer-centric altazimuthal coordinate to Earth Equatorial
	Mat4d matHeliocentricEclipticToEquinoxEqu;// Transform from heliocentric ecliptic (Vsop87) to earth equatorial coordinate
	Mat4d matEquinoxEquToJ2000;
	Mat4d matJ2000ToEquinoxEqu;
	Mat4d matJ2000ToAltAz;

	Mat4d matAltAzModelView;				// Modelview matrix for observer-centric altazimuthal drawing
	Mat4d invertMatAltAzModelView;			// Inverted modelview matrix for observer-centric altazimuthal drawing

	// Position variables
	StelObserver* position;
	// The ID of the default startup location
	QString defaultLocationID;

	// Time variables
	double timeSpeed;                       // Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;                            // Curent time in Julian day
	double presetSkyTime;
	QTime initTodayTime;
	QString startupTimeMode;
};

#endif // _STELCORE_HPP_
