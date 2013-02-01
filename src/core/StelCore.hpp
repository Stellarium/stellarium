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
		FrameUninitialized,           //!< Reference frame is not set (FMajerech: Added to avoid condition on uninitialized value in StelSkyLayerMgr::draw())
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
	void init(class StelRenderer* renderer);

	//! Update all the objects with respect to the time.
	//! @param deltaTime the time increment in sec.
	void update(double deltaTime);

	//! Handle the resizing of the window
	void windowHasBeenResized(float x, float y, float width, float height);

	//! Update core state before drawing modules.
	void preDraw();

	//! Update core state after drawing modules.
	void postDraw(StelRenderer* renderer);

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
	StelToneReproducer* getToneReproducer();
	//! Get the current tone reproducer used in the core.
	const StelToneReproducer* getToneReproducer() const;

	//! Get the current StelSkyDrawer used in the core.
	StelSkyDrawer* getSkyDrawer();
	//! Get the current StelSkyDrawer used in the core.
	const StelSkyDrawer* getSkyDrawer() const;

	//! Get an instance of StelGeodesicGrid which is garanteed to allow for at least maxLevel levels
	const StelGeodesicGrid* getGeodesicGrid(int maxLevel) const;

	//! Get the instance of movement manager.
	StelMovementMgr* getMovementMgr();
	//! Get the const instance of movement manager.
	const StelMovementMgr* getMovementMgr() const;

	//! Set the near and far clipping planes.
	void setClippingPlanes(double znear, double zfar);
	//! Get the near and far clipping planes.
	void getClippingPlanes(double* zn, double* zf) const;

	//! Get the translated projection name from its TypeKey for the current locale.
	QString projectionTypeKeyToNameI18n(const QString& key) const;

	//! Get the projection TypeKey from its translated name for the current locale.
	QString projectionNameI18nToTypeKey(const QString& nameI18n) const;

	//! Get the current set of parameters to use when creating a new StelProjector.
	StelProjector::StelProjectorParams getCurrentStelProjectorParams() const;
	//! Set the set of parameters to use when creating a new StelProjector.
	void setCurrentStelProjectorParams(const StelProjector::StelProjectorParams& newParams);

	//! Set vision direction
	void lookAtJ2000(const Vec3d& pos, const Vec3d& up);

	Vec3d altAzToEquinoxEqu(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	Vec3d equinoxEquToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	Vec3d altAzToJ2000(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	Vec3d j2000ToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	Vec3d galacticToJ2000(const Vec3d& v) const;
	Vec3d equinoxEquToJ2000(const Vec3d& v) const;
	Vec3d j2000ToEquinoxEqu(const Vec3d& v) const;
	Vec3d j2000ToGalactic(const Vec3d& v) const;

	//! Transform vector from heliocentric ecliptic coordinate to altazimuthal
	Vec3d heliocentricEclipticToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;

	//! Transform from heliocentric coordinate to equatorial at current equinox (for the planet where the observer stands)
	Vec3d heliocentricEclipticToEquinoxEqu(const Vec3d& v) const;
	//! Transform vector from heliocentric coordinate to false equatorial : equatorial
	//! coordinate but centered on the observer position (usefull for objects close to earth)
	Vec3d heliocentricEclipticToEarthPosEquinoxEqu(const Vec3d& v) const;

	//! Get the modelview matrix for heliocentric ecliptic (Vsop87) drawing
	StelProjector::ModelViewTranformP getHeliocentricEclipticModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric ecliptic (Vsop87) drawing
	StelProjector::ModelViewTranformP getObservercentricEclipticModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric equatorial at equinox drawing
	StelProjector::ModelViewTranformP getEquinoxEquModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric altazimuthal drawing
	StelProjector::ModelViewTranformP getAltAzModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric J2000 equatorial drawing
	StelProjector::ModelViewTranformP getJ2000ModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric Galactic equatorial drawing
	StelProjector::ModelViewTranformP getGalacticModelViewTransform(RefractionMode refMode=RefractionAuto) const;

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

	const QSharedPointer<class Planet> getCurrentPlanet() const;

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
	QString getStartupTimeMode();
	void setStartupTimeMode(const QString& s);

public slots:
	//! Set the current ProjectionType to use
	void setCurrentProjectionType(ProjectionType type);
	ProjectionType getCurrentProjectionType() const;

	//! Get the current Mapping used by the Projection
	QString getCurrentProjectionTypeKey(void) const;
	//! Set the current ProjectionType to use from its key
	void setCurrentProjectionTypeKey(QString type);

	//! Get the list of all the available projections
	QStringList getAllProjectionTypeKeys() const;

	//! Set the mask type.
	void setMaskType(StelProjector::StelProjectorMaskType m);

	//! Set the flag with decides whether to arrage labels so that
	//! they are aligned with the bottom of a 2d screen, or a 3d dome.
	void setFlagGravityLabels(bool gravity);
	//! Set the offset rotation angle in degree to apply to gravity text (only if gravityLabels is set to false).
	void setDefautAngleForGravityText(float a);
	//! Set the horizontal flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipHorz(bool flip);
	//! Set the vertical flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	void setFlipVert(bool flip);
	//! Get the state of the horizontal flip.
	//! @return True if flipped horizontally, else false.
	bool getFlipHorz(void) const;
	//! Get the state of the vertical flip.
	//! @return True if flipped vertically, else false.
	bool getFlipVert(void) const;

	//! Get the location used by default at startup
	QString getDefaultLocationID() const;
	//! Set the location to use by default at startup
	void setDefaultLocationID(const QString& id);
	//! Return to the default location.
	void returnToDefaultLocation();
	//! Return to the default location and set default landscape with atmosphere and fog effects
	void returnToHome();

	//! Set the current date in Julian Day
	void setJDay(double JD);
	//! Get the current date in Julian Day
	double getJDay() const;

	//! Set the current date in Modified Julian Day
	void setMJDay(double MJD);
	//! Get the current date in Modified Julian Day
	double getMJDay() const;

	//! Return the preset sky time in JD
	double getPresetSkyTime() const;
	//! Set the preset sky time from a JD
	void setPresetSkyTime(double d);

	//! Set time speed in JDay/sec
	void setTimeRate(double ts);
	//! Get time speed in JDay/sec
	double getTimeRate() const;

	//! Increase the time speed
	void increaseTimeSpeed();
	//! Decrease the time speed
	void decreaseTimeSpeed();
	//! Increase the time speed, but not as much as with increaseTimeSpeed()
	void increaseTimeSpeedLess();
	//! Decrease the time speed but not as much as with decreaseTimeSpeed()
	void decreaseTimeSpeedLess();

	//! Set time speed to 0, i.e. freeze the passage of simulation time
	void setZeroTimeSpeed();
	//! Set real time speed, i.e. 1 sec/sec
	void setRealTimeSpeed();
	//! Set real time speed or pause simulation if we are already in realtime speed.
	void toggleRealTimeSpeed();
	//! Get whether it is real time speed, i.e. 1 sec/sec
	bool getRealTimeSpeed() const;

	//! Set stellarium time to current real world time
	void setTimeNow();
	//! Set the time to some value, leaving the day the same.
	void setTodayTime(const QTime& target);
	//! Get whether the current stellarium time is the real world time
	bool getIsTimeNow() const;

	//! get the initial "today time" from the config file
	QTime getInitTodayTime(void);
	//! set the initial "today time" from the config file
	void setInitTodayTime(const QTime& t);
	//! Set the preset sky time from a QDateTime
	void setPresetSkyTime(QDateTime dt);

	//! Add one [Earth, solar] hour to the current simulation time.
	void addHour();
	//! Add one [Earth, solar] day to the current simulation time.
	void addDay();
	//! Add one [Earth, solar] week to the current simulation time.
	void addWeek();

	//! Add one sidereal day to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void addSiderealDay();
	//! Add one sidereal week (7 sidereal days) to the simulation time. The length
	//! of time depends on the current planetary body on which the observer is located.
	void addSiderealWeek();
	//! Add one sidereal month (1/12 of sidereal year) to the simulation time. The length
	//! of time depends on the current planetary body on which the observer is located.
	//! Sidereal year connected to orbital period of planets.
	void addSiderealMonth();
	//! Add one sidereal year to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void addSiderealYear();
	//! Add one sidereal century (100 sidereal years) to the simulation time. The length
	//! of time depends on the current planetary body on which the observer is located.
	//! Sidereal year connected to orbital period of planets.
	void addSiderealCentury();

	//! Subtract one [Earth, solar] hour to the current simulation time.
	void subtractHour();
	//! Subtract one [Earth, solar] day to the current simulation time.
	void subtractDay();
	//! Subtract one [Earth, solar] week to the current simulation time.
	void subtractWeek();

	//! Subtract one sidereal day to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void subtractSiderealDay();
	//! Subtract one sidereal week (7 sidereal days) to the simulation time. The length
	//! of time depends on the current planetary body on which the observer is located.
	void subtractSiderealWeek();
	//! Subtract one sidereal month (1/12 of sidereal year) to the simulation time. The length
	//! of time depends on the current planetary body on which the observer is located.
	//! Sidereal year connected to orbital period of planets.
	void subtractSiderealMonth();
	//! Subtract one sidereal year to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void subtractSiderealYear();
	//! Subtract one sidereal century (100 sidereal years) to the simulation time. The length
	//! of time depends on the current planetary body on which the observer is located.
	//! Sidereal year connected to orbital period of planets.
	void subtractSiderealCentury();

	//! Add one synodic month to the simulation time.
	void addSynodicMonth();

	//! Add one draconic year to the simulation time.
	void addDraconicYear();
	//! Add one draconic month to the simulation time.
	void addDraconicMonth();

	//! Add one anomalistic month to the simulation time.
	void addAnomalisticMonth();

	//! Add one mean tropical month to the simulation time.
	void addTropicalMonth();
	//! Add one mean tropical year to the simulation time.
	void addTropicalYear();
	//! Add one mean tropical century to the simulation time.
	void addTropicalCentury();

	//! Subtract one synodic month to the simulation time.
	void subtractSynodicMonth();

	//! Subtract one draconic year to the simulation time.
	void subtractDraconicYear();
	//! Subtract one draconic month to the simulation time.
	void subtractDraconicMonth();

	//! Subtract one anomalistic month to the simulation time.
	void subtractAnomalisticMonth();

	//! Subtract one mean tropical month to the simulation time.
	void subtractTropicalMonth();
	//! Subtract one mean tropical year to the simulation time.
	void subtractTropicalYear();
	//! Subtract one mean tropical century to the simulation time.
	void subtractTropicalCentury();

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
	Mat4d matHeliocentricEclipticToAltAz;      // Transform from heliocentric ecliptic (Vsop87) to observer-centric altazimuthal coordinate
	Mat4d matAltAzToHeliocentricEcliptic;	   // Transform from observer-centric altazimuthal coordinate to heliocentric ecliptic (Vsop87)
	Mat4d matAltAzToEquinoxEqu;                // Transform from observer-centric altazimuthal coordinate to Earth Equatorial
	Mat4d matEquinoxEquToAltAz;                // Transform from observer-centric altazimuthal coordinate to Earth Equatorial
	Mat4d matHeliocentricEclipticToEquinoxEqu; // Transform from heliocentric ecliptic (Vsop87) to earth equatorial coordinate
	Mat4d matEquinoxEquToJ2000;
	Mat4d matJ2000ToEquinoxEqu;
	Mat4d matJ2000ToAltAz;

	Mat4d matAltAzModelView;           // Modelview matrix for observer-centric altazimuthal drawing
	Mat4d invertMatAltAzModelView;     // Inverted modelview matrix for observer-centric altazimuthal drawing

	// Position variables
	StelObserver* position;
	// The ID of the default startup location
	QString defaultLocationID;

	// Time variables
	double timeSpeed;                  // Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;                       // Curent time in Julian day
	double presetSkyTime;
	QTime initTodayTime;
	QString startupTimeMode;
};

#endif // _STELCORE_HPP_
