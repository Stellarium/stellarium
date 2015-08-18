/*
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2012 Matthew Gates
 * Copyright (C) 2015 Georg Zotti (Precession fixes)
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
#include <QPair>

class StelToneReproducer;
class StelSkyDrawer;
class StelGeodesicGrid;
class StelMovementMgr;
class StelObserver;

//! @class StelCore
//! Main class for Stellarium core processing.
//! This class provides services like management of sky projections,
//! tone conversion, or reference frame conversion. It is used by the
//! various StelModules to update and display themselves.
//! There is currently only one StelCore instance in Stellarium, but
//! in the future they may be more, allowing for example to display
//! several independent views of the sky at the same time.
//! @author Fabien Chereau, Matthew Gates, Georg Zotti
class StelCore : public QObject
{
	Q_OBJECT
	Q_ENUMS(ProjectionType)
	Q_ENUMS(DeltaTAlgorithm)
	Q_PROPERTY(bool flipHorz READ getFlipHorz WRITE setFlipHorz)
	Q_PROPERTY(bool flipVert READ getFlipVert WRITE setFlipVert)
	Q_PROPERTY(bool flagUseNutation READ getUseNutation WRITE setUseNutation)
	Q_PROPERTY(bool flagUseTopocentricCoordinates READ getUseTopocentricCoordinates WRITE setUseTopocentricCoordinates)
	Q_PROPERTY(double JD READ getJD WRITE setJD NOTIFY updated)

public:
	//! @enum FrameType
	//! Supported reference frame types
	enum FrameType
	{
		FrameUninitialized,			//!< Reference frame is not set (FMajerech: Added to avoid condition on uninitialized value in StelSkyLayerMgr::draw())
		FrameAltAz,				//!< Altazimuthal reference frame centered on observer.
		FrameHeliocentricEclipticJ2000,		//!< Fixed-ecliptic reference frame centered on the Sun. GZ: This is J2000 ecliptical / almost VSOP87.
		FrameObservercentricEclipticJ2000,	//!< Fixed-ecliptic reference frame centered on the Observer. GZ: was ObservercentricEcliptic, but renamed because it is Ecliptic of J2000!
		FrameObservercentricEclipticOfDate,	//!< Moving ecliptic reference frame centered on the Observer. GZ new for V0.14: Ecliptic of date, i.e. includes the precession of the ecliptic.
		FrameEquinoxEqu,			//!< Equatorial reference frame at the current equinox centered on the observer.
							//!< The north pole follows the precession of the planet on which the observer is located. On Earth, this may include nutation if so configured.
							//!< Has been corrected for V0.14 to really properly reflect ecliptical motion and precession (Vondrak 2011 model) and nutation.
		FrameJ2000,				//!< Equatorial reference frame at the J2000 equinox centered on the observer.
							//!< This is also the ICRS reference frame.
		FrameGalactic				//!< Galactic reference frame centered on observer.
	};

	//! @enum ProjectionType
	//! Available projection types. A value of 1000 indicate the default projection
	enum ProjectionType
	{
		ProjectionPerspective,		//!< Perspective projection
		ProjectionEqualArea,		//!< Equal Area projection
		ProjectionStereographic,	//!< Stereograhic projection
		ProjectionFisheye,		//!< Fisheye projection
		ProjectionHammer,		//!< Hammer-Aitoff projection
		ProjectionCylinder,		//!< Cylinder projection
		ProjectionMercator,		//!< Mercator projection
		ProjectionOrthographic,		//!< Orthographic projection
		ProjectionSinusoidal		//!< Sinusoidal projection
	};

	//! @enum RefractionMode
	//! Available refraction mode.
	enum RefractionMode
	{
		RefractionAuto,			//!< Automatically decide to add refraction if atmosphere is activated
		RefractionOn,			//!< Always add refraction (i.e. apparent coordinates)
		RefractionOff			//!< Never add refraction (i.e. geometric coordinates)
	};

	//! @enum DeltaTAlgorithm
	//! Available DeltaT algorithms
	enum DeltaTAlgorithm
	{
		WithoutCorrection,              //!< Without correction, DeltaT is Zero. Like Stellarium versions before 0.12.
		Schoch,                         //!< Schoch (1931) algorithm for DeltaT
		Clemence,                       //!< Clemence (1948) algorithm for DeltaT
		IAU,                            //!< IAU (1952) algorithm for DeltaT (based on observations by Spencer Jones (1939))
		AstronomicalEphemeris,          //!< Astronomical Ephemeris (1960) algorithm for DeltaT
		TuckermanGoldstine,             //!< Tuckerman (1962, 1964) & Goldstine (1973) algorithm for DeltaT
		MullerStephenson,               //!< Muller & Stephenson (1975) algorithm for DeltaT
		Stephenson1978,                 //!< Stephenson (1978) algorithm for DeltaT
		SchmadelZech1979,               //!< Schmadel & Zech (1979) algorithm for DeltaT
		MorrisonStephenson1982,         //!< Morrison & Stephenson (1982) algorithm for DeltaT (used by RedShift)
		StephensonMorrison1984,         //!< Stephenson & Morrison (1984) algorithm for DeltaT
		StephensonHoulden,              //!< Stephenson & Houlden (1986) algorithm for DeltaT
		Espenak,                        //!< Espenak (1987, 1989) algorithm for DeltaT
		Borkowski,                      //!< Borkowski (1988) algorithm for DeltaT
		SchmadelZech1988,               //!< Schmadel & Zech (1988) algorithm for DeltaT
		ChaprontTouze,                  //!< Chapront-Touzé & Chapront (1991) algorithm for DeltaT
		StephensonMorrison1995,         //!< Stephenson & Morrison (1995) algorithm for DeltaT
		Stephenson1997,                 //!< Stephenson (1997) algorithm for DeltaT		
		ChaprontMeeus,                  //!< Chapront, Chapront-Touze & Francou (1997) & Meeus (1998) algorithm for DeltaT
		JPLHorizons,                    //!< JPL Horizons algorithm for DeltaT
		MeeusSimons,                    //!< Meeus & Simons (2000) algorithm for DeltaT
		MontenbruckPfleger,             //!< Montenbruck & Pfleger (2000) algorithm for DeltaT
		ReingoldDershowitz,             //!< Reingold & Dershowitz (2002, 2007) algorithm for DeltaT
		MorrisonStephenson2004,         //!< Morrison & Stephenson (2004, 2005) algorithm for DeltaT
		Reijs,                          //!< Reijs (2006) algorithm for DeltaT
		EspenakMeeus,                   //!< Espenak & Meeus (2006) algorithm for DeltaT (Recommended, default)
		EspenakMeeusZeroMoonAccel,      //   Espenak & Meeus (2006) algorithm for DeltaT (but without additional Lunar acceleration. FOR TESTING ONLY, NONPUBLIC)
		Banjevic,			//!< Banjevic (2006) algorithm for DeltaT
		IslamSadiqQureshi,		//!< Islam, Sadiq & Qureshi (2008 + revisited 2013) algorithm for DeltaT (6 polynomials)
		KhalidSultanaZaidi,		//!< M. Khalid, Mariam Sultana and Faheem Zaidi polinomial approximation of time period 1620-2013 (2014)
		Custom                          //!< User defined coefficients for quadratic equation for DeltaT
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

	//! Get a new instance of projector using a modelview transformation corresponding to the given frame.
	//! If not specified the refraction effect is included if atmosphere is on.
	StelProjectorP getProjection(FrameType frameType, RefractionMode refractionMode=RefractionAuto) const;
	//! Added so that we can access the html summary from qml.
	Q_INVOKABLE QString getProjectionHtmlSummary() const { return getProjection(FrameJ2000)->getHtmlSummary(); }

	//! Get a new instance of projector using the given modelview transformation.
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
	Q_INVOKABLE QString projectionTypeKeyToNameI18n(const QString& key) const;

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
	void j2000ToAltAzInPlaceNoRefraction(Vec3f* v) const {v->transfo4d(matJ2000ToAltAz);}
	Vec3d galacticToJ2000(const Vec3d& v) const;
	Vec3d equinoxEquToJ2000(const Vec3d& v) const;
	Vec3d j2000ToEquinoxEqu(const Vec3d& v) const;
	Vec3d j2000ToGalactic(const Vec3d& v) const;

	//! Transform vector from heliocentric ecliptic coordinate to altazimuthal
	Vec3d heliocentricEclipticToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;

	//! Transform from heliocentric coordinate to equatorial at current equinox (for the planet where the observer stands)
	Vec3d heliocentricEclipticToEquinoxEqu(const Vec3d& v) const;
//	//! Transform vector from heliocentric coordinate to false equatorial : equatorial
//	//! coordinate but centered on the observer position (useful for objects close to earth)
//	//! Unused as of V0.13
//	Vec3d heliocentricEclipticToEarthPosEquinoxEqu(const Vec3d& v) const;

	//! Get the modelview matrix for heliocentric ecliptic (Vsop87) drawing.
	StelProjector::ModelViewTranformP getHeliocentricEclipticModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric ecliptic (Vsop87) drawing.
	StelProjector::ModelViewTranformP getObservercentricEclipticJ2000ModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric ecliptic of Date drawing.
	StelProjector::ModelViewTranformP getObservercentricEclipticOfDateModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric equatorial at equinox drawing.
	StelProjector::ModelViewTranformP getEquinoxEquModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric altazimuthal drawing.
	StelProjector::ModelViewTranformP getAltAzModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric J2000 equatorial drawing.
	StelProjector::ModelViewTranformP getJ2000ModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Get the modelview matrix for observer-centric Galactic equatorial drawing.
	StelProjector::ModelViewTranformP getGalacticModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Rotation matrix from equatorial J2000 to ecliptic (VSOP87A).
	static const Mat4d matJ2000ToVsop87;
	//! Rotation matrix from ecliptic (VSOP87A) to equatorial J2000.
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

	//! Unfortunately we also need this.
	const StelObserver* getCurrentObserver() const;


	SphericalCap getVisibleSkyArea() const;
	
	//! Smoothly move the observer to the given location
	//! @param target the target location
	//! @param duration direction of view move duration in s
	//! @param durationIfPlanetChange direction of view + planet travel move duration in s.
	//! This is used only if the destination planet is different from the starting one.
	void moveObserverTo(const StelLocation& target, double duration=1., double durationIfPlanetChange=1.);

	// Conversion in standard Julian time format
	static const double JD_SECOND;
	static const double JD_MINUTE;
	static const double JD_HOUR;
	static const double JD_DAY;
	static const double ONE_OVER_JD_SECOND;

	//! Get the sidereal time shifted by the observer longitude
	//! @return the local sidereal time in radian
	double getLocalSiderealTime() const;

	//! Get the duration of a sidereal day for the current observer in day.
	double getLocalSiderealDayLength() const;

	//! Get the duration of a sidereal year for the current observer in days.
	double getLocalSiderealYearLength() const;

	//! Return the startup mode, can be "actual" (i.e. take current time from system),
	//! "today" (take some time e.g. on the evening of today) or "preset" (completely preconfigured).
	QString getStartupTimeMode();
	void setStartupTimeMode(const QString& s);

	//! Get info about valid range for current algorithm for calculation of Delta-T
	//! @param JD the Julian Day number to test.
	//! @param marker receives a string: "*" if jDay is outside valid range, or "?" if range unknown, else an empty string.
	//! @return valid range as explanatory string.
	QString getCurrentDeltaTAlgorithmValidRangeDescription(const double JD, QString* marker) const;

	//! Checks for altitude of sun - is it night or day?
	//! @return true if sun higher than about -6 degrees, i.e. "day" includes civil twilight.
	//! @note Useful mostly for brightness-controlled GUI decisions like font colors.
	bool isBrightDaylight() const;

	//! Get value of the current Julian epoch (i.e. current year with decimal fraction, e.g. 2012.34567)
	double getCurrentEpoch() const;

	//! Get the default Mapping used by the Projection
	QString getDefaultProjectionTypeKey(void) const;

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

	//! Set the current algorithm for time correction (DeltaT)
	void setCurrentDeltaTAlgorithm(DeltaTAlgorithm algorithm) { currentDeltaTAlgorithm=algorithm; }
	//! Get the current algorithm for time correction (DeltaT)
	DeltaTAlgorithm getCurrentDeltaTAlgorithm() const { return currentDeltaTAlgorithm; }
	//! Get description of the current algorithm for time correction
	QString getCurrentDeltaTAlgorithmDescription(void) const;
	//! Get the current algorithm used by the DeltaT
	QString getCurrentDeltaTAlgorithmKey(void) const;
	//! Set the current algorithm to use from its key
	void setCurrentDeltaTAlgorithmKey(QString type);

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

	//! Set the current date in Julian Day (UT)
	void setJD(double newJD);
	//! Set the current date in Julian Day (TT).
	//! The name is derived from the classical name "Ephemeris Time", of which TT is the successor.
	//! It is still frequently used in the literature.
	void setJDE(double newJDE);
	//! Get the current date in Julian Day (UT).
	double getJD() const;
	//! Get the current date in Julian Day (TT).
	//! The name is derived from the classical name "Ephemeris Time", of which TT is the successor.
	//! It is still frequently used in the literature.
	double getJDE() const;

	//! Set the current date in Modified Julian Day (UT).
	//! MJD is simply JD-2400000.5, getting rid of large numbers and starting days at midnight.
	//! It is mostly used in satellite contexts.
	void setMJDay(double MJD);
	//! Get the current date in Modified Julian Day (UT)
	double getMJDay() const;

	//! Compute Delta-T estimation for a given date.
	//! DeltaT is the accumulated effect of earth's rotation slowly getting slower, mostly caused by tidal braking by the Moon.
	//! For accurate positioning of objects in the sky, we must compute earth-based clock-dependent things like earth rotation, hour angles etc.
	//! using plain UT, but all orbital motions or rotation of the other planets must be computed in TT, which is a regular time frame.
	//! Also satellites are computed in the UT frame because (1) they are short-lived and (2) must follow paths over earth ground.
	//! (Note that we make no further difference between TT and DT, those are regarded equivalent for our purpose.)
	//!
	//! @param JD the date and time expressed as a Julian Day
	//! @return DeltaT in seconds
	//! @note Thanks to Rob van Gent which create a collection from many formulas for calculation of Delta-T: http://www.staff.science.uu.nl/~gent0113/deltat/deltat.htm
	//! @note Use this only if needed, prefer calling getDeltaT() for access to the current value.
	double computeDeltaT(const double JD) const;
	//! Get current DeltaT.
	double getDeltaT() const;

	//! @return whether nutation is currently used.
	bool getUseNutation() const {return flagUseNutation;}
	//! Set whether you want computation and simulation of nutation (a slight wobble of Earth's axis, just a few arcseconds).
	void setUseNutation(bool useNutation) { flagUseNutation=useNutation;}

	//! @return whether topocentric coordinates are currently used.
	bool getUseTopocentricCoordinates() const {return flagUseTopocentricCoordinates;}
	//! Set whether you want computation and simulation of nutation (a slight wobble of Earth's axis, just a few arcseconds).
	void setUseTopocentricCoordinates(bool use) { flagUseTopocentricCoordinates=use;}

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
	void setInitTodayTime(const QTime& time);
	//! Set the preset sky time from a QDateTime
	void setPresetSkyTime(QDateTime dateTime);

	//! Add one [Earth, solar] hour to the current simulation time.
	void addHour();
	//! Add one [Earth, solar] day to the current simulation time.
	void addDay();
	//! Add one [Earth, solar] week to the current simulation time.
	void addWeek();

	//! Add one sidereal day to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void addSiderealDay();
	//! Add one sidereal year to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void addSiderealYear();
	//! Add n sidereal years to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void addSiderealYears(float n=100.f);

	//! Subtract one [Earth, solar] hour to the current simulation time.
	void subtractHour();
	//! Subtract one [Earth, solar] day to the current simulation time.
	void subtractDay();
	//! Subtract one [Earth, solar] week to the current simulation time.
	void subtractWeek();

	//! Subtract one sidereal day to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void subtractSiderealDay();
	//! Subtract one sidereal year to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void subtractSiderealYear();
	//! Subtract n sidereal years to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void subtractSiderealYears(float n=100.f);

	//! Add one synodic month to the simulation time.
	void addSynodicMonth();

	//! Add one draconic year to the simulation time.
	void addDraconicYear();
	//! Add one draconic month to the simulation time.
	void addDraconicMonth();

	//! Add one anomalistic month to the simulation time.
	void addAnomalisticMonth();
	//! Add one anomalistic year to the simulation time.
	void addAnomalisticYear();
	//! Add n anomalistic years to the simulation time.
	void addAnomalisticYears(float n=100.f);

	//! Add one mean tropical month to the simulation time.
	void addMeanTropicalMonth();
	//! Add one mean tropical year to the simulation time.
	void addMeanTropicalYear();
	//! Add n mean tropical years to the simulation time.
	void addMeanTropicalYears(float n=100.f);
	//! Add one tropical year to the simulation time.
	void addTropicalYear();

	//! Add one Julian year to the simulation time.
	void addJulianYear();
	//! Add n Julian years to the simulation time.
	void addJulianYears(float n=100.f);

	//! Add one Gaussian year to the simulation time.
	void addGaussianYear();

	//! Subtract one synodic month to the simulation time.
	void subtractSynodicMonth();

	//! Subtract one draconic year to the simulation time.
	void subtractDraconicYear();
	//! Subtract one draconic month to the simulation time.
	void subtractDraconicMonth();

	//! Subtract one anomalistic month to the simulation time.
	void subtractAnomalisticMonth();
	//! Subtract one anomalistic year to the simulation time.
	void subtractAnomalisticYear();
	//! Subtract n anomalistic years to the simulation time.
	void subtractAnomalisticYears(float n=100.f);

	//! Subtract one mean tropical month to the simulation time.
	void subtractMeanTropicalMonth();
	//! Subtract one mean tropical year to the simulation time.
	void subtractMeanTropicalYear();
	//! Subtract n mean tropical years to the simulation time.
	void subtractMeanTropicalYears(float n=100.f);
	//! Subtract one tropical year to the simulation time.
	void subtractTropicalYear();

	//! Subtract one Julian year to the simulation time.
	void subtractJulianYear();
	//! Subtract n Julian years to the simulation time.
	void subtractJulianYears(float n=100.f);

	//! Subtract one Gaussian year to the simulation time.
	void subtractGaussianYear();

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

	//! Set central year for custom equation for calculation of Delta-T
	//! @param y the year, e.g. 1820
	void setDeltaTCustomYear(float y) { deltaTCustomYear=y; }
	//! Set n-dot for custom equation for calculation of Delta-T
	//! @param y the n-dot value, e.g. -26.0
	void setDeltaTCustomNDot(float v) { deltaTCustomNDot=v; }
	//! Set coefficients for custom equation for calculation of Delta-T
	//! @param y the coefficients, e.g. -20,0,32
	void setDeltaTCustomEquationCoefficients(Vec3f c) { deltaTCustomEquationCoeff=c; }

	//! Get central year for custom equation for calculation of Delta-T
	float getDeltaTCustomYear() const { return deltaTCustomYear; }
	//! Get n-dot for custom equation for calculation of Delta-T
	float getDeltaTCustomNDot() const { return deltaTCustomNDot; }
	//! Get coefficients for custom equation for calculation of Delta-T
	Vec3f getDeltaTCustomEquationCoefficients() const { return deltaTCustomEquationCoeff; }

signals:
	//! This signal is emitted when the observer location has changed.
	void locationChanged(StelLocation);
	//! This signal is emitted when the time rate has changed
	void timeRateChanged(double rate);
	//! Emitted at every iteration
	void updated();

private:
	StelToneReproducer* toneConverter;		// Tones conversion between stellarium world and display device
	StelSkyDrawer* skyDrawer;
	StelMovementMgr* movementMgr;		// Manage vision movements

	// Manage geodesic grid
	mutable StelGeodesicGrid* geodesicGrid;

	// The currently used projection type
	ProjectionType currentProjectionType;

	// The currentrly used time correction (DeltaT)
	DeltaTAlgorithm currentDeltaTAlgorithm;

	// Parameters to use when creating new instances of StelProjector
	StelProjector::StelProjectorParams currentProjectorParams;

	void updateTransformMatrices();
	void updateTime(double deltaTime);
	void resetSync();



	// Matrices used for every coordinate transfo
	Mat4d matHeliocentricEclipticJ2000ToAltAz; // Transform from heliocentric ecliptic Cartesian (VSOP87A) to topocentric (StelObserver) altazimuthal coordinate
	Mat4d matAltAzToHeliocentricEclipticJ2000; // Transform from topocentric (StelObserver) altazimuthal coordinate to heliocentric ecliptic Cartesian (VSOP87A)
	Mat4d matAltAzToEquinoxEqu;                // Transform from topocentric altazimuthal coordinate to Earth Equatorial
	Mat4d matEquinoxEquToAltAz;                // Transform from Earth Equatorial to topocentric (StelObserver) altazimuthal coordinate
	Mat4d matHeliocentricEclipticToEquinoxEqu; // Transform from heliocentric ecliptic Cartesian (VSOP87A) to earth equatorial coordinate
	Mat4d matEquinoxEquToJ2000;                // For Earth, this is almost the inverse precession matrix, =Rz(VSOPbias)Rx(eps0)Rz(-psiA)Rx(-omA)Rz(chiA)
	Mat4d matJ2000ToEquinoxEqu;                // precession matrix

	Mat4d matJ2000ToAltAz;

	Mat4d matAltAzModelView;           // Modelview matrix for observer-centric altazimuthal drawing
	Mat4d invertMatAltAzModelView;     // Inverted modelview matrix for observer-centric altazimuthal drawing

	// Position variables
	StelObserver* position;
	// The ID of the default startup location
	QString defaultLocationID;

	// flag to indicate we want to use nutation (the small-scale wobble of earth's axis)
	bool flagUseNutation;
	// flag to indicate that we show topocentrically corrected coordinates. (Switching to false for planetocentric coordinates is new for 0.14)
	bool flagUseTopocentricCoordinates;

	// Time variables
	double timeSpeed;                  // Positive : forward, Negative : Backward, 1 = 1sec/sec
	//double JDay;                     // Current time in Julian day. IN V0.12 TO V0.14, this was JD in TT, and all places where UT was required had to subtract getDeltaT() explicitly.
	QPair<double,double> JD;           // From 0.14 on: JD.first=JD_UT, JD.second=DeltaT=TT-UT. To gain JD_TT, compute JDE=JD.first+JD.second or better just call getJDE()
					   // Use is best with calls getJD()/setJD() and getJDE()/setJDE() to explicitly state which flavour of JD you need.
	double presetSkyTime;
	QTime initTodayTime;
	QString startupTimeMode;
	double secondsOfLastJDUpdate;    // Time in seconds when the time rate or time last changed
	double jdOfLastJDUpdate;         // JD when the time rate or time last changed

	// Variables for custom equation of Delta-T
	Vec3f deltaTCustomEquationCoeff;
	float deltaTCustomNDot;
	float deltaTCustomYear;

};

#endif // _STELCORE_HPP_
