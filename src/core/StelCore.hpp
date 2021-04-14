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

#ifndef STELCORE_HPP
#define STELCORE_HPP

#include "StelProjector.hpp"
#include "StelProjectorType.hpp"
#include "StelLocation.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPropertyMgr.hpp"
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
	Q_ENUMS(FrameType)
	Q_ENUMS(ProjectionType)
	Q_ENUMS(RefractionMode)
	Q_ENUMS(DeltaTAlgorithm)
	Q_PROPERTY(bool flipHorz READ getFlipHorz WRITE setFlipHorz NOTIFY flipHorzChanged)
	Q_PROPERTY(bool flipVert READ getFlipVert WRITE setFlipVert NOTIFY flipVertChanged)
	Q_PROPERTY(bool flagUseNutation READ getUseNutation WRITE setUseNutation NOTIFY flagUseNutationChanged)
	Q_PROPERTY(bool flagUseTopocentricCoordinates READ getUseTopocentricCoordinates WRITE setUseTopocentricCoordinates NOTIFY flagUseTopocentricCoordinatesChanged)
	Q_PROPERTY(ProjectionType currentProjectionType READ getCurrentProjectionType WRITE setCurrentProjectionType NOTIFY currentProjectionTypeChanged)
	//! This is just another way to access the projection type, by string instead of enum
	Q_PROPERTY(QString currentProjectionTypeKey READ getCurrentProjectionTypeKey WRITE setCurrentProjectionTypeKey NOTIFY currentProjectionTypeKeyChanged STORED false)
	//! Read-only property returning the localized projection name
	Q_PROPERTY(QString currentProjectionNameI18n READ getCurrentProjectionNameI18n NOTIFY currentProjectionNameI18nChanged STORED false)
	Q_PROPERTY(bool flagGravityLabels READ getFlagGravityLabels WRITE setFlagGravityLabels NOTIFY flagGravityLabelsChanged)
	Q_PROPERTY(QString currentTimeZone READ getCurrentTimeZone WRITE setCurrentTimeZone NOTIFY currentTimeZoneChanged)
	Q_PROPERTY(bool flagUseCTZ READ getUseCustomTimeZone WRITE setUseCustomTimeZone NOTIFY useCustomTimeZoneChanged)
	Q_PROPERTY(bool flagUseDST READ getUseDST WRITE setUseDST NOTIFY flagUseDSTChanged)

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
								//!< The north pole follows the precession of the planet on which the observer is located.
								//!< On Earth, this may include nutation if so configured. Has been corrected for V0.14 to really properly reflect ecliptical motion and precession (Vondrak 2011 model) and nutation.
		FrameJ2000,				//!< Equatorial reference frame at the J2000 equinox centered on the observer. This is also the ICRS reference frame.
		FrameGalactic,				//!< Galactic reference frame centered on observer.
		FrameSupergalactic			//!< Supergalactic reference frame centered on observer.
	};

	//! @enum ProjectionType
	//! Available projection types. A value of 1000 indicates the default projection
	enum ProjectionType
	{
		ProjectionPerspective,		//!< Perspective projection
		ProjectionStereographic,	//!< Stereographic projection
		ProjectionFisheye,		//!< Fisheye projection
		ProjectionOrthographic,		//!< Orthographic projection
		ProjectionEqualArea,		//!< Equal Area projection
		ProjectionHammer,		//!< Hammer-Aitoff projection
		ProjectionSinusoidal,		//!< Sinusoidal projection
		ProjectionMercator,		//!< Mercator projection
		ProjectionMiller,		//!< Miller cylindrical projection
		ProjectionCylinder,		//!< Cylinder projection
	};

	//! @enum RefractionMode
	//! Available refraction mode.
	enum RefractionMode
	{
		RefractionAuto,		//!< Automatically decide to add refraction if atmosphere is activated
		RefractionOn,		//!< Always add refraction (i.e. apparent coordinates)
		RefractionOff		//!< Never add refraction (i.e. geometric coordinates)
	};

	//! @enum DeltaTAlgorithm
	//! Available DeltaT algorithms
	enum DeltaTAlgorithm
	{
		WithoutCorrection,			//!< Without correction, DeltaT is Zero. Like Stellarium versions before 0.12.
		Schoch,					//!< Schoch (1931) algorithm for DeltaT
		Clemence,					//!< Clemence (1948) algorithm for DeltaT
		IAU,						//!< IAU (1952) algorithm for DeltaT (based on observations by Spencer Jones (1939))
		AstronomicalEphemeris,		//!< Astronomical Ephemeris (1960) algorithm for DeltaT
		TuckermanGoldstine,		//!< Tuckerman (1962, 1964) & Goldstine (1973) algorithm for DeltaT
		MullerStephenson,			//!< Muller & Stephenson (1975) algorithm for DeltaT
		Stephenson1978,                     //!< Stephenson (1978) algorithm for DeltaT
		SchmadelZech1979,			//!< Schmadel & Zech (1979) algorithm for DeltaT
		MorrisonStephenson1982,	//!< Morrison & Stephenson (1982) algorithm for DeltaT (used by RedShift)
		StephensonMorrison1984,	//!< Stephenson & Morrison (1984) algorithm for DeltaT
		StephensonHoulden,			//!< Stephenson & Houlden (1986) algorithm for DeltaT
		Espenak,					//!< Espenak (1987, 1989) algorithm for DeltaT
		Borkowski,				//!< Borkowski (1988) algorithm for DeltaT
		SchmadelZech1988,			//!< Schmadel & Zech (1988) algorithm for DeltaT
		ChaprontTouze,			//!< Chapront-Touzé & Chapront (1991) algorithm for DeltaT
		StephensonMorrison1995,	//!< Stephenson & Morrison (1995) algorithm for DeltaT
		Stephenson1997,                     //!< Stephenson (1997) algorithm for DeltaT
		ChaprontMeeus,			//!< Chapront, Chapront-Touze & Francou (1997) & Meeus (1998) algorithm for DeltaT
		JPLHorizons,				//!< JPL Horizons algorithm for DeltaT
		MeeusSimons,				//!< Meeus & Simons (2000) algorithm for DeltaT
		MontenbruckPfleger,                 //!< Montenbruck & Pfleger (2000) algorithm for DeltaT
		ReingoldDershowitz,                 //!< Reingold & Dershowitz (2002, 2007) algorithm for DeltaT
		MorrisonStephenson2004,	//!< Morrison & Stephenson (2004, 2005) algorithm for DeltaT
		Reijs,					//!< Reijs (2006) algorithm for DeltaT
		EspenakMeeus,				//!< Espenak & Meeus (2006) algorithm for DeltaT (Recommended, default)
		EspenakMeeusZeroMoonAccel,	//!< Espenak & Meeus (2006) algorithm for DeltaT (but without additional Lunar acceleration. FOR TESTING ONLY, NONPUBLIC)
		Banjevic,					//!< Banjevic (2006) algorithm for DeltaT
		IslamSadiqQureshi,			//!< Islam, Sadiq & Qureshi (2008 + revisited 2013) algorithm for DeltaT (6 polynomials)
		KhalidSultanaZaidi,			//!< M. Khalid, Mariam Sultana and Faheem Zaidi polynomial approximation of time period 1620-2013 (2014)
		StephensonMorrisonHohenkerk2016,    //!< Stephenson, Morrison, Hohenkerk (2016) RSPA paper provides spline fit to observations for -720..2016 and else parabolic fit.
		Henriksson2017,			//!< Henriksson (2017) algorithm for DeltaT (The solution for Schoch formula for DeltaT (1931), but with ndot=-30.128"/cy^2)
		Custom					//!< User defined coefficients for quadratic equation for DeltaT
	};

	StelCore();
	virtual ~StelCore();

	//! Init and load all main core components.
	void init();

	//! Update all the objects with respect to the time.
	//! @param deltaTime the time increment in sec.
	void update(double deltaTime);

	//! Handle the resizing of the window
	void windowHasBeenResized(qreal x, qreal y, qreal width, qreal height);

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

	//! Get a new instance of projector using the given modelview transformation.
	//! If not specified the projection used is the one currently used as default.
	StelProjectorP getProjection(StelProjector::ModelViewTranformP modelViewTransform, ProjectionType projType=static_cast<ProjectionType>(1000)) const;

	//! Get the current tone reproducer used in the core.
	StelToneReproducer* getToneReproducer(){return toneReproducer;}
	//! Get the current tone reproducer used in the core.
	const StelToneReproducer* getToneReproducer() const{return toneReproducer;}

	//! Get the current StelSkyDrawer used in the core.
	StelSkyDrawer* getSkyDrawer(){return skyDrawer;}
	//! Get the current StelSkyDrawer used in the core.
	const StelSkyDrawer* getSkyDrawer() const{return skyDrawer;}

	//! Get an instance of StelGeodesicGrid which is garanteed to allow for at least maxLevel levels
	const StelGeodesicGrid* getGeodesicGrid(int maxLevel) const;

	//! Get the instance of movement manager.
	StelMovementMgr* getMovementMgr(){return movementMgr;}
	//! Get the const instance of movement manager.
	const StelMovementMgr* getMovementMgr() const{return movementMgr;}

	//! Set the near and far clipping planes.
	void setClippingPlanes(double znear, double zfar){
		currentProjectorParams.zNear=znear;currentProjectorParams.zFar=zfar;
	}
	//! Get the near and far clipping planes.
	void getClippingPlanes(double* zn, double* zf) const{
		*zn = currentProjectorParams.zNear;
		*zf = currentProjectorParams.zFar;
	}

	//! Get the translated projection name from its TypeKey for the current locale.
	QString projectionTypeKeyToNameI18n(const QString& key) const;

	//! Get the projection TypeKey from its translated name for the current locale.
	QString projectionNameI18nToTypeKey(const QString& nameI18n) const;

	//! Get the current set of parameters.
	StelProjector::StelProjectorParams getCurrentStelProjectorParams() const;
	//! Set the set of parameters to use when creating a new StelProjector.
	void setCurrentStelProjectorParams(const StelProjector::StelProjectorParams& newParams);

	//! Set vision direction
	void lookAtJ2000(const Vec3d& pos, const Vec3d& up);
	void setMatAltAzModelView(const Mat4d& mat);

	Vec3d altAzToEquinoxEqu(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	Vec3d equinoxEquToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	Vec3d altAzToJ2000(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	Vec3d j2000ToAltAz(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	void j2000ToAltAzInPlaceNoRefraction(Vec3f* v) const {v->transfo4d(matJ2000ToAltAz);}
	Vec3d galacticToJ2000(const Vec3d& v) const;
	Vec3d supergalacticToJ2000(const Vec3d& v) const;
	//! Transform position vector v from equatorial coordinates of date (which may also include atmospheric refraction) to those of J2000.
	//! Use refMode=StelCore::RefractionOff if you don't want any atmosphere correction.
	//! Use refMode=StelCore::RefractionOn to create observed (apparent) coordinates (which are subject to refraction).
	//! Use refMode=StelCore::RefractionAuto to correct coordinates for refraction only when atmosphere is active.
	Vec3d equinoxEquToJ2000(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	//! Use fixed matrix to allow fast transformation of positions related to the IAU constellation borders.
	Vec3d j2000ToJ1875(const Vec3d& v) const;
	//! Transform position vector v from equatorial coordinates J2000 to those of date (optionally corrected by refraction).
	//! Use refMode=StelCore::RefractionOff if you don't want any atmosphere correction.
	//! Use refMode=StelCore::RefractionOn to correct observed (apparent) coordinates (which are subject to refraction).
	//! Use refMode=StelCore::RefractionAuto to correct coordinates for refraction only when atmosphere is active.
	Vec3d j2000ToEquinoxEqu(const Vec3d& v, RefractionMode refMode=RefractionAuto) const;
	Vec3d j2000ToGalactic(const Vec3d& v) const;
	Vec3d j2000ToSupergalactic(const Vec3d& v) const;

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

	//! Get the modelview matrix for observer-centric Supergalactic equatorial drawing.
	StelProjector::ModelViewTranformP getSupergalacticModelViewTransform(RefractionMode refMode=RefractionAuto) const;

	//! Rotation matrix from equatorial J2000 to ecliptic (VSOP87A).
	static const Mat4d matJ2000ToVsop87;
	//! Rotation matrix from ecliptic (VSOP87A) to equatorial J2000.
	static const Mat4d matVsop87ToJ2000;
	//! Rotation matrix from J2000 to Galactic reference frame, using FITS convention.
	static const Mat4d matJ2000ToGalactic;
	//! Rotation matrix from Galactic to J2000 reference frame, using FITS convention.
	static const Mat4d matGalacticToJ2000;
	//! Rotation matrix from J2000 to Supergalactic reference frame.
	static const Mat4d matJ2000ToSupergalactic;
	//! Rotation matrix from Supergalactic to J2000 reference frame.
	static const Mat4d matSupergalacticToJ2000;

	//! Return the observer heliocentric ecliptic position (GZ: presumably J2000)
	Vec3d getObserverHeliocentricEclipticPos() const;

	//! Get the information on the current location
	const StelLocation& getCurrentLocation() const;
	//! Get the UTC offset on the current location (in hours)
	double getUTCOffset(const double JD) const;

	QString getCurrentTimeZone() const;
	void setCurrentTimeZone(const QString& tz);

	const QSharedPointer<class Planet> getCurrentPlanet() const;

	//! Unfortunately we also need this.
	//! Returns the current observer.
	//! Note that the observer object may be deleted at any time when control returns to StelCore.
	const StelObserver* getCurrentObserver() const;

	//! Replaces the current observer. StelCore assumes ownership of the observer.
	void setObserver(StelObserver* obs);

	SphericalCap getVisibleSkyArea() const;

	// Conversion in standard Julian time format
	static const double JD_SECOND;
	static const double JD_MINUTE;
	static const double JD_HOUR;
	static const double JD_DAY;
	static const double ONE_OVER_JD_SECOND;
	static const double TZ_ERA_BEGINNING;

	//! Get the sidereal time shifted by the observer longitude
	//! @return the local sidereal time in radian
	double getLocalSiderealTime() const;

	//! Get the duration of a sidereal day for the current observer in day.
	double getLocalSiderealDayLength() const;

	//! Get the duration of a sidereal year for the current observer in days.
	double getLocalSiderealYearLength() const;

	//! Return the startup mode, can be "actual" (i.e. take current time from system),
	//! "today" (take some time e.g. on the evening of today) or "preset" (completely preconfigured).
	QString getStartupTimeMode() const;
	void setStartupTimeMode(const QString& s);

	//! Get info about valid range for current algorithm for calculation of Delta-T
	//! @param JD the Julian Day number to test.
	//! @param marker receives a string: "*" if jDay is outside valid range, or "?" if range unknown, else an empty string.
	//! @return valid range as explanatory string.
	QString getCurrentDeltaTAlgorithmValidRangeDescription(const double JD, QString* marker) const;

	//! Checks for altitude of the Sun - is it night or day?
	//! @return true if sun higher than about -6 degrees, i.e. "day" includes civil twilight.
	//! @note Useful mostly for brightness-controlled GUI decisions like font colors.
	bool isBrightDaylight() const;

	//! Get value of the current Julian epoch (i.e. current year with decimal fraction, e.g. 2012.34567)
	double getCurrentEpoch() const;

	//! Get the default Mapping used by the Projection
	QString getDefaultProjectionTypeKey(void) const;

	Vec3d getMouseJ2000Pos(void) const;

public slots:
	//! Smoothly move the observer to the given location
	//! @param target the target location
	//! @param duration direction of view move duration in s
	//! @param durationIfPlanetChange direction of view + planet travel move duration in s.
	//! This is used only if the destination planet is different from the starting one.
	void moveObserverTo(const StelLocation& target, double duration=1., double durationIfPlanetChange=1.);

	//! Set the current ProjectionType to use
	void setCurrentProjectionType(ProjectionType type);
	ProjectionType getCurrentProjectionType() const;

	//! Get the current Mapping used by the Projection
	QString getCurrentProjectionTypeKey(void) const;
	//! Set the current ProjectionType to use from its key
	void setCurrentProjectionTypeKey(QString type);

	QString getCurrentProjectionNameI18n() const;

	//! Get the list of all the available projections
	QStringList getAllProjectionTypeKeys() const;

	//! Set the current algorithm and nDot used therein for time correction (DeltaT)
	void setCurrentDeltaTAlgorithm(DeltaTAlgorithm algorithm);
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
	//! return whether dome-aligned labels are in use
	bool getFlagGravityLabels() const;
	//! Set the offset rotation angle in degree to apply to gravity text (only if gravityLabels is set to false).
	void setDefaultAngleForGravityText(float a);
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

	// Vertical offset should even be available for animation, so at last with property mechanism.
	//! Get current value for horizontal viewport offset [-50...50]
	//! An offset of 50 percent means projective image center is on the right screen border
	double getViewportHorizontalOffset(void) const;
	//! Set horizontal viewport offset. Argument will be clamped to be inside [-50...50]
	//! An offset of 50 percent means projective image center is on the right screen border
	//! Animation is available via StelMovementMgr::moveViewport()
	void setViewportHorizontalOffset(double newOffsetPct);
	//! Get current value for vertical viewport offset [-50...50]
	//! An offset of 50 percent means projective image center is on the upper screen border
	double getViewportVerticalOffset(void) const;
	//! Set vertical viewport offset. Argument will be clamped to be inside [-50...50]
	//! An offset of 50 percent means projective image center is on the upper screen border
	//! Setting to a negative value will move the visible horizon down, this may be desired esp. in cylindrical projection.
	//! Animation is available via StelMovementMgr::moveViewport()
	void setViewportVerticalOffset(double newOffsetPct);
	// Set both viewport offsets. Arguments will be clamped to be inside [-50...50]. I (GZ) hope this will avoid some of the shaking.
	void setViewportOffset(double newHorizontalOffsetPct, double newVerticalOffsetPct);

	//! Can be used in specialized setups, intended e.g. for multi-projector installations with edge blending.
	//! @param stretch [default 1] enlarge to stretch image to non-square pixels. A minimum value of 0.001 is enforced.
	//! @note This only influences the projected content. Things like ScreenImages keep square pixels.
	void setViewportStretch(float stretch);

	//! Get the location used by default at startup
	QString getDefaultLocationID() const;
	//! Set the location to use by default at startup
	void setDefaultLocationID(const QString& id);
	//! Return to the default location.
	void returnToDefaultLocation();
	//! Return to the default location and set default landscape with atmosphere and fog effects
	void returnToHome();

	//! Returns the JD of the last time resetSync() was called
	double getJDOfLastJDUpdate() const;
	//! Sets the system date which corresponds to the jdOfLastJDUpdate.
	//! Usually, you do NOT want to call this method.
	//! This method is used by the RemoteSync plugin.
	void setMilliSecondsOfLastJDUpdate(qint64 millis);
	//! Returns the system date of the last time resetSync() was called
	qint64 getMilliSecondsOfLastJDUpdate() const;

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

	//! Get solution of equation of time
	//! Source: J. Meeus "Astronomical Algorithms" (2nd ed., with corrections as of August 10, 2009) p.183-187.
	//! @param JDE JD in Dynamical Time (previously called Ephemeris Time)
	//! @return time in minutes
	double getSolutionEquationOfTime(const double JDE) const;

	bool getUseDST() const;
	void setUseDST(const bool b);

	bool getUseCustomTimeZone(void) const;
	void setUseCustomTimeZone(const bool b);

	//! Set the current date in Modified Julian Day (UT).
	//! MJD is simply JD-2400000.5, getting rid of large numbers and starting days at midnight.
	//! It is mostly used in satellite contexts.
	void setMJDay(double MJD);
	//! Get the current date in Modified Julian Day (UT)
	double getMJDay() const;

	//! Compute DeltaT estimation for a given date.
	//! DeltaT is the accumulated effect of earth's rotation slowly getting slower, mostly caused by tidal braking by the Moon.
	//! For accurate positioning of objects in the sky, we must compute earth-based clock-dependent things like earth rotation, hour angles etc.
	//! using plain UT, but all orbital motions or rotation of the other planets must be computed in TT, which is a regular time frame.
	//! Also satellites are computed in the UT frame because (1) they are short-lived and (2) must follow paths over earth ground.
	//! (Note that we make no further difference between TT and DT, those might differ by milliseconds at best but are regarded equivalent for our purpose.)
	//! @param JD the date and time expressed as a Julian Day
	//! @return DeltaT in seconds
	//! @note Thanks to Rob van Gent who created a collection from many formulas for calculation of DeltaT: http://www.staff.science.uu.nl/~gent0113/deltat/deltat.htm
	//! @note Use this only if needed, prefer calling getDeltaT() for access to the current value.
	//! @note Up to V0.15.1, if the requested year was outside validity range, we returned zero or some useless value.
	//!       Starting with V0.15.2 the value from the edge of the defined range is returned instead if not explicitly zero is given in the source.
	//!       Limits can be queried with getCurrentDeltaTAlgorithmValidRangeDescription()

	double computeDeltaT(const double JD);
	//! Get current DeltaT.
	double getDeltaT() const;

	//! @return whether nutation is currently used.
	bool getUseNutation() const {return flagUseNutation;}
	//! Set whether you want computation and simulation of nutation (a slight wobble of Earth's axis, just a few arcseconds).
	void setUseNutation(bool use) { if (flagUseNutation != use) { flagUseNutation=use; emit flagUseNutationChanged(use); }}

	//! @return whether topocentric coordinates are currently used.
	bool getUseTopocentricCoordinates() const {return flagUseTopocentricCoordinates;}
	//! Set whether you want computation and simulation of nutation (a slight wobble of Earth's axis, just a few arcseconds).
	void setUseTopocentricCoordinates(bool use) { if (flagUseTopocentricCoordinates!= use) { flagUseTopocentricCoordinates=use; emit flagUseTopocentricCoordinatesChanged(use); }}

	//! Return the preset sky time in JD
	double getPresetSkyTime() const;
	//! Set the preset sky time from a JD
	void setPresetSkyTime(double d);

	//! Set time speed in JDay/sec
	void setTimeRate(double ts);
	//! Get time speed in JDay/sec
	double getTimeRate() const;

	void revertTimeDirection(void);

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
	QTime getInitTodayTime(void) const;
	//! set the initial "today time" from the config file
	void setInitTodayTime(const QTime& time);
	//! Set the preset sky time from a QDateTime
	void setPresetSkyTime(QDateTime dateTime);

	//! Add one [Earth, solar] minute to the current simulation time.
	void addMinute();
	//! Add one [Earth, solar] hour to the current simulation time.
	void addHour();
	//! Add one [Earth, solar] day to the current simulation time.
	void addDay();
	//! Add one [Earth, solar] week to the current simulation time.
	void addWeek();

	//! Add one sidereal day to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void addSiderealDay();
	//! Add seven sidereal days to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void addSiderealWeek();
	//! Add one sidereal year to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void addSiderealYear();
	//! Add n sidereal years to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void addSiderealYears(double n=100.);

	//! Subtract one [Earth, solar] minute to the current simulation time.
	void subtractMinute();
	//! Subtract one [Earth, solar] hour to the current simulation time.
	void subtractHour();
	//! Subtract one [Earth, solar] day to the current simulation time.
	void subtractDay();
	//! Subtract one [Earth, solar] week to the current simulation time.
	void subtractWeek();

	//! Subtract one sidereal day to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void subtractSiderealDay();
	//! Subtract seven sidereal days to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void subtractSiderealWeek();
	//! Subtract one sidereal year to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void subtractSiderealYear();
	//! Subtract n sidereal years to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located. Sidereal year
	//! connected to orbital period of planets.
	void subtractSiderealYears(double n=100.);

	//! Add one synodic month to the simulation time.
	void addSynodicMonth();

	//! Add one saros (223 synodic months) to the simulation time.
	void addSaros();

	//! Add one draconic year to the simulation time.
	void addDraconicYear();
	//! Add one draconic month to the simulation time.
	void addDraconicMonth();

	//! Add one anomalistic month to the simulation time.
	void addAnomalisticMonth();
	//! Add one anomalistic year to the simulation time.
	void addAnomalisticYear();
	//! Add n anomalistic years to the simulation time.
	void addAnomalisticYears(double n=100.);

	//! Add one mean tropical month to the simulation time.
	void addMeanTropicalMonth();
	//! Add one mean tropical year to the simulation time.
	void addMeanTropicalYear();
	//! Add n mean tropical years to the simulation time.
	void addMeanTropicalYears(double n=100.);
	//! Add one tropical year to the simulation time.
	void addTropicalYear();

	//! Add one calendric month to the simulation time.
	void addCalendricMonth();

	//! Add one Julian year to the simulation time.
	void addJulianYear();
	//! Add n Julian years to the simulation time.
	void addJulianYears(double n=100.);

	//! Add one Gaussian year to the simulation time. The Gaussian Year is 365.2568983 days, and is C.F.Gauss's value for the Sidereal Year.
	//! Note that 1 GaussY=2 &pi;/k where k is the Gaussian gravitational constant. A massless body orbits one solar mass in 1AU distance in a Gaussian Year.
	void addGaussianYear();

	//! Subtract one synodic month to the simulation time.
	void subtractSynodicMonth();

	//! Subtract one saros (223 synodic months) to the simulation time.
	void subtractSaros();

	//! Subtract one draconic year to the simulation time.
	void subtractDraconicYear();
	//! Subtract one draconic month to the simulation time.
	void subtractDraconicMonth();

	//! Subtract one anomalistic month to the simulation time.
	void subtractAnomalisticMonth();
	//! Subtract one anomalistic year to the simulation time.
	void subtractAnomalisticYear();
	//! Subtract n anomalistic years to the simulation time.
	void subtractAnomalisticYears(double n=100.);

	//! Subtract one mean tropical month to the simulation time.
	void subtractMeanTropicalMonth();
	//! Subtract one mean tropical year to the simulation time.
	void subtractMeanTropicalYear();
	//! Subtract n mean tropical years to the simulation time.
	void subtractMeanTropicalYears(double n=100.);
	//! Subtract one tropical year to the simulation time.
	void subtractTropicalYear();

	//! Subtract one calendric month to the simulation time.
	void subtractCalendricMonth();

	//! Subtract one Julian year to the simulation time.
	void subtractJulianYear();
	//! Subtract n Julian years to the simulation time.
	void subtractJulianYears(double n=100.);

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

	//! Set central year for custom equation for calculation of DeltaT
	//! @param y the year, e.g. 1820
	void setDeltaTCustomYear(double y) { deltaTCustomYear=y; }
	//! Set n-dot for custom equation for calculation of DeltaT
	//! @param v the n-dot value, e.g. -26.0
	void setDeltaTCustomNDot(double v) { deltaTCustomNDot=v; }
	//! Set coefficients for custom equation for calculation of DeltaT
	//! @param c the coefficients, e.g. -20,0,32
	void setDeltaTCustomEquationCoefficients(Vec3d c) { deltaTCustomEquationCoeff=c; }

	//! Get central year for custom equation for calculation of DeltaT
	double getDeltaTCustomYear() const { return deltaTCustomYear; }
	//! Get n-dot for custom equation for calculation of DeltaT
	double getDeltaTCustomNDot() const { return deltaTCustomNDot; }
	//! Get n-dot for current DeltaT algorithm
	double getDeltaTnDot() const { return deltaTnDot; }
	//! Get coefficients for custom equation for calculation of DeltaT
	Vec3d getDeltaTCustomEquationCoefficients() const { return deltaTCustomEquationCoeff; }

	//! initialize ephemerides calculation functions
	void initEphemeridesFunctions();

	bool de430IsAvailable();            //!< true if DE430 ephemeris file has been found
	bool de431IsAvailable();            //!< true if DE431 ephemeris file has been found
	bool de430IsActive();               //!< true if DE430 ephemeris is in use
	bool de431IsActive();               //!< true if DE431 ephemeris is in use
	void setDe430Active(bool status);   //!< switch DE430 use to @param status (if de430IsAvailable()). DE430 is only used if date is within range of DE430.
	void setDe431Active(bool status);   //!< switch DE431 use to @param status (if de431IsAvailable()). DE431 is only used if DE430 is not used and the date is within range of DE431.

	//! Return 3-letter abbreviation of IAU constellation name for position in equatorial coordinates on the current epoch.
	//! Follows 1987PASP...99..695R: Nancy Roman: Identification of a Constellation from a Position
	//! Data file from ADC catalog VI/42 with its amendment from 1999-12-30.
	//! @param positionEqJnow position vector in rectangular equatorial coordinates of current epoch&equinox.
	QString getIAUConstellation(const Vec3d positionEqJnow) const;

signals:
	//! This signal is emitted when the observer location has changed.
	void locationChanged(const StelLocation&);
	//! This signal is emitted whenever the targetted location changes
	void targetLocationChanged(const StelLocation&);
	//! This signal is emitted when the current timezone name is changed.
	void currentTimeZoneChanged(const QString& tz);
	//! This signal is emitted when custom timezone use is activated (true) or deactivated (false).
	void useCustomTimeZoneChanged(const bool b);
	//! This signal is emitted when daylight saving time is enabled or disabled.
	void flagUseDSTChanged(const bool b);
	//! This signal is emitted when the time rate has changed
	void timeRateChanged(double rate);
	//! This signal is emitted whenever the time is re-synced.
	//! This happens whenever the internal jDay is changed through setJDay/setMJDay and similar,
	//! and whenever the time rate changes.
	void timeSyncOccurred(double jDay);
	//! This signal is emitted when the date has changed.
	void dateChanged();
	//! This signal can be emitted when e.g. the date has changed in a way that planet trails or similar things should better be reset.
	//! TODO: Currently the signal is not used. Think of the proper way to apply it.
	void dateChangedForTrails();
	//! This signal is emitted when the date has changed for a month.
	void dateChangedForMonth();
	//! This signal is emitted when the date has changed by one year.
	void dateChangedByYear();
	//! This signal indicates a horizontal display flip
	void flipHorzChanged(bool b);
	//! This signal indicates a vertical display flip
	void flipVertChanged(bool b);
	//! This signal indicates a switch in use of nutation
	void flagUseNutationChanged(bool b);
	//! This signal indicates a switch in use of topocentric coordinates
	void flagUseTopocentricCoordinatesChanged(bool b);
	//! Emitted whenever the projection type changes
	void currentProjectionTypeChanged(StelCore::ProjectionType newType);
	//! Emitted whenever the projection type changes
	void currentProjectionTypeKeyChanged(const QString& newValue);
	//! Emitted whenever the projection type changes
	void currentProjectionNameI18nChanged(const QString& newValue);
	//! Emitted when gravity label use is changed
	void flagGravityLabelsChanged(bool gravity);
	//! Emitted when button "Save settings" is pushed
	void configurationDataSaved();
	void updateSearchLists();

private:
	StelToneReproducer* toneReproducer;		// Tones conversion between stellarium world and display device
	StelSkyDrawer* skyDrawer;
	StelMovementMgr* movementMgr;		// Manage vision movements
	StelPropertyMgr* propMgr;

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
	void updateMaximumFov();
	void resetSync();

	void registerMathMetaTypes();


	// Matrices used for every coordinate transfo
	Mat4d matHeliocentricEclipticJ2000ToAltAz; // Transform from heliocentric ecliptic Cartesian (VSOP87A) to topocentric (StelObserver) altazimuthal coordinate
	Mat4d matAltAzToHeliocentricEclipticJ2000; // Transform from topocentric (StelObserver) altazimuthal coordinate to heliocentric ecliptic Cartesian (VSOP87A)
	Mat4d matAltAzToEquinoxEqu;                // Transform from topocentric altazimuthal coordinate to Earth Equatorial
	Mat4d matEquinoxEquToAltAz;                // Transform from Earth Equatorial to topocentric (StelObserver) altazimuthal coordinate
	Mat4d matHeliocentricEclipticToEquinoxEqu; // Transform from heliocentric ecliptic Cartesian (VSOP87A) to earth equatorial coordinate
	Mat4d matEquinoxEquDateToJ2000;            // For Earth, this is almost the inverse precession matrix, =Rz(VSOPbias)Rx(eps0)Rz(-psiA)Rx(-omA)Rz(chiA)
	Mat4d matJ2000ToEquinoxEqu;                // precession matrix
	static Mat4d matJ2000ToJ1875;              // Precession matrix for IAU constellation lookup.

	Mat4d matJ2000ToAltAz;
	Mat4d matAltAzToJ2000;

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
	double timeSpeed;           // Positive : forward, Negative : Backward, 1 = 1sec/sec
	//double JDay;              // Current time in Julian day. IN V0.12 TO V0.14, this was JD in TT, and all places where UT was required had to subtract getDeltaT() explicitly.
	QPair<double,double> JD;    // From 0.14 on: JD.first=JD_UT, JD.second=DeltaT=TT-UT. To gain JD_TT, compute JDE=JD.first+JD.second or better just call getJDE()
				    // Use is best with calls getJD()/setJD() and getJDE()/setJDE() to explicitly state which flavour of JD you need.
	double presetSkyTime;
	QTime initTodayTime;
	QString startupTimeMode;
	qint64 milliSecondsOfLastJDUpdate;    // Time in milliseconds when the time rate or time last changed
	double jdOfLastJDUpdate;         // JD when the time rate or time last changed

	QString currentTimeZone;	
	bool flagUseDST;
	bool flagUseCTZ; // custom time zone

	// Variables for equations of DeltaT
	Vec3d deltaTCustomEquationCoeff;
	double deltaTCustomNDot;
	double deltaTCustomYear;
	double deltaTnDot; // The currently applied nDot correction. (different per algorithm, and displayed in status line.)
	bool  deltaTdontUseMoon; // true if the currenctly selected algorithm does not do a lunar correction (?????)
	double (*deltaTfunc)(const double JD); // This is a function pointer which must be set to a function which computes DeltaT(JD).
	int deltaTstart;   // begin year of validity range for the selected DeltaT algorithm. (SET INT_MIN to mark infinite)
	int deltaTfinish;  // end   year of validity range for the selected DeltaT algorithm. (Set INT_MAX to mark infinite)

	// Variables for DE430/431 ephem calculation
	bool de430Available; // ephem file found
	bool de431Available; // ephem file found
	bool de430Active;    // available and user-activated.
	bool de431Active;    // available and user-activated.
};

#endif // STELCORE_HPP
