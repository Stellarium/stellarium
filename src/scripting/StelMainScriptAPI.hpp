/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#ifndef STELMAINSCRIPTAPI_HPP
#define STELMAINSCRIPTAPI_HPP

#include <QObject>
#include <QVariant>
#include <QStringList>
#include "StelObject.hpp"
#include "StelCore.hpp"

class QScriptEngine;

//! Provide script API for Stellarium global functions.  Public slots in this class
//! may be used in Stellarium scripts, and are accessed as member function to the
//! "core" scripting object.  Module-specific functions, such as setting and clearing
//! of display flags (e.g. LandscapeMgr::setFlagAtmosphere) can be accessed directly
//! via the scripting object with the class name, e.g.  by using the scripting command:
//!  LandscapeMgr.setFlagAtmosphere(true);
class StelMainScriptAPI : public QObject
{
	Q_OBJECT
	Q_PROPERTY(double JDay READ getJDay WRITE setJDay)
	Q_PROPERTY(double timeSpeed READ getTimeRate WRITE setTimeRate)

public:
	StelMainScriptAPI(QObject *parent = Q_NULLPTR);
	~StelMainScriptAPI();

// These functions will be available in scripts
public slots:
	//! Set the current date as Julian Day number
	//! @param JD the Julian Day number
	static void setJDay(double JD);
	//! Get the current date as Julian Day number
	//! @return the Julian Day number
	static double getJDay();

	//! Set the current date as Modified Julian Day
	//! @param MJD the Modified Julian Day
	static void setMJDay(double MJD);
	//! Get the current date as Modified Julian Day
	//! @return the Modified Julian Day
	static double getMJDay();

	//! Sets Stellarium's date using an absolute or relative value.
	//! @param dateStr Defines the date or offset to use. Possible formats:
	//! - Date and time according to ISO 8601, without time zone, e.g., "2008-03-24T13:21:01"
	//! - Use "now" to set Stellarium's time to the system's (presumably) real time.
	//! - Specify an offset in the form <sign><value><unit> to set the date relative to its
	//!   current value. <unit> may be one of second, minute, hour, day, sol, week, month,
	//!   year, with a plural 's' optionally appended, e.g., "+3.25 days", " - 1 year".
	//! - Use "now" followed by an offset to set the date relative to the system's
	//!   current value, e.g., "now+1hour".
	//! - You may append "sidereal" to use units derived from the length of a sidereal day,
	//!   e.g., "now +2days sidereal".
	//! @note When sidereal time is used, the length of time for each unit is dependent on
	//! the current planet.  By contrast, when sidereal time is not specified (i.e., solar
	//! time is used) the value is conventional so that 1 day means 1 Earth Solar day.
	//! @param spec May be "local" or "utc". Only has an effect when
	//! an ISO format string is used. Defaults to "utc".
	//! @param dateIsDT set to \a true indicates that the given date is formulated in
	//! Dynamical Time, i.e. with DeltaT added.
	//! @note For fully compatible behaviour of this function with versions 0.11.4
	//! or earlier you should call \b core.setDeltaTAlgorithm("WithoutCorrection");
	//! before running \b core.setDate(); for disabling DeltaT correction.
	//! @note Starting with version 0.13.2 all relative dates are set without DeltaT correction.
	//! @note Starting with version 0.14.0 the Boolean argument has a different meaning and default!
	static void setDate(const QString& dateStr, const QString& spec="utc", const bool& dateIsDT=false);

	//! get the simulation date and time as a string in ISO format,
	//! e.g. "2008-03-24T13:21:01"
	//! @param spec if "utc", the returned string's timezone is UTC,
	//! else it is local time.
	//! @return the current simulation time.
	static QString getDate(const QString& spec="utc");

	//! get the DeltaT for the simulation date and time as a string
	//! in HMS format, e.g. "0h1m68.2s"
	//! @return the DeltaT for current simulation time.
	static QString getDeltaT();

	//! get the DeltaT equation name for the simulation date and time as a string
	//! @return name of the DeltaT equation
	static QString getDeltaTAlgorithm();

	//! set equation of the DeltaT for the simulation date and time
	//! @param algorithmName is name of equation, e.g. "WithoutCorrection" or "EspenakMeeus"
	//! @note list of possible names of equation for DeltaT:
	//! - WithoutCorrection
	//! - Schoch
	//! - Clemence
	//! - IAU
	//! - AstronomicalEphemeris
	//! - TuckermanGoldstine
	//! - MullerStephenson
	//! - Stephenson1978
	//! - SchmadelZech1979
	//! - MorrisonStephenson1982
	//! - StephensonMorrison1984
	//! - StephensonHoulden
	//! - Espenak
	//! - Borkowski
	//! - SchmadelZech1988
	//! - ChaprontTouze
	//! - StephensonMorrison1995
	//! - Stephenson1997
	//! - ChaprontMeeus
	//! - JPLHorizons
	//! - MeeusSimons
	//! - MontenbruckPfleger
	//! - ReingoldDershowitz
	//! - MorrisonStephenson2004
	//! - EspenakMeeus
	//! - Reijs
	//! - Banjevic
	//! - IslamSadiqQureshi
	//! - Henriksson2017
	//! - Custom
	static void setDeltaTAlgorithm(QString algorithmName);

	//! Set time speed in JDay/sec
	//! @param ts the new rate of passage of time as a multiple of real time.
	//! For example if ts is 1, time will pass at the normal rate.  If ts == 10
	//! then simulation time will pass at 10 times the normal rate.
	//! If ts is negative, simulation time will go backwards.
	static void setTimeRate(double ts);
	//! Get simulation time rate.
	//! @return time speed as a multiple of real time.
	static double getTimeRate();

	//! Get the simulation time and rate state - is it "real time"
	//! @return true if the time rate is normal, and the simulation time
	//! is real time, else return false
	static bool isRealTime();

	//! Set the simulation time to the current system time, and the time rate to 1
	static void setRealTime();

	//! Get the type of calculations in the simulation - is it "planetocentric calculations" or not
	//! @return true if the calculations is planetocentric (geocentric on Earth), else return false
	static bool isPlanetocentricCalculations();

	//! Set the flag for use planetocentric calculations
	static void setPlanetocentricCalculations(bool f);

	//! Select an object by name
	//! @param name the name of the object to select (english)
	//! If the name is "", any currently selected objects will be
	//! de-selected.
	//! @param pointer whether or not to have the selection pointer enabled
	static void selectObjectByName(const QString& name, bool pointer=false);

	//! Select a constellation by name
	//! @param name the name of the constellation to select (english)
	static void selectConstellationByName(const QString& name);

	//! Fetch a map with data about an object's position, magnitude and so on
	//! @param name is the English name of the object for which data will be
	//! returned.
	//! @return a map of object data.  Keys:
	//! - above-horizon : true, if celestial body is above horizon
	//! - altitude : apparent altitude angle in decimal degrees
	//! - azimuth : apparent azimuth angle in decimal degrees
	//! - altitude-geometric : geometric altitude angle in decimal degrees
	//! - azimuth-geometric : geometric azimuth angle in decimal degrees
	//! - ra : right ascension angle (current date frame) in decimal degrees
	//! - dec : declination angle (current date frame) in decimal degrees
	//! - raJ2000 : right ascension angle (J2000 frame) in decimal degrees
	//! - decJ2000 : declination angle (J2000 frame) in decimal degrees
	//! - parallacticAngle : parallactic angle in decimal degrees (for non-star objects only)
	//! - hourAngle-dd : hour angle in decimal degrees
	//! - hourAngle-hms : hour angle in HMS format (formatted string)
	//! - iauConstellation : 3-letter abbreviation of IAU constellation (string)
	//! - meanSidTm : mean sidereal time, in decimal degrees (on Earth only!)
	//! - appSidTm : mean sidereal time, in decimal degrees (on Earth only!)
	//! - glong : galactic longitude in decimal degrees
	//! - glat : galactic latitude in decimal degrees
	//! - sglong : supergalactic longitude in decimal degrees
	//! - sglat : supergalactic latitude in decimal degrees
	//! - elong : ecliptic longitude in decimal degrees (on Earth only!)
	//! - elat : ecliptic latitude in decimal degrees (on Earth only!)
	//! - elongJ2000 : ecliptic longitude (Earth's J2000 frame) in decimal degrees
	//! - elatJ2000 : ecliptic latitude (Earth's J2000 frame) in decimal degrees
	//! - vmag : visual magnitude
	//! - vmage : visual magnitude (extincted)
	//! - size: angular size in radians
	//! - size-dd : angular size in decimal degrees
	//! - size-deg : angular size in decimal degrees (formatted string)
	//! - size-dms : angular size in DMS format
	//! - localized-name : localized name	
	//! The returned map can contain other information. For example, Solar System objects add:
	//! - distance : distance to object in AU (for Solar system objects only!)
	//! - phase : phase (illuminated fraction, 0..1) of object (for Solar system objects only!)
	//! - illumination : phase of object in percent (0..100) (for Solar system objects only!)
	//! - phase-angle : phase angle of object in radians (for Solar system objects only!)
	//! - phase-angle-dms : phase angle of object in DMS (for Solar system objects only!)
	//! - phase-angle-deg : phase angle of object in decimal degrees (for Solar system objects only!)
	//! - elongation : elongation of object in radians (for Solar system objects only!)
	//! - elongation-dms : elongation of object in DMS (for Solar system objects only!)
	//! - elongation-deg : elongation of object in decimal degrees (for Solar system objects only!)
	//! - velocity: the planet velocity around the parent planet in ecliptical coordinates in AU/d (for Solar system objects only!)
	//! - velocity-kms: the planet velocity around the parent planet in km/s (for Solar system objects only!)
	//! - heliocentric-velocity: the planet's heliocentric velocity in the solar system in ecliptical coordinates in AU/d (for Solar system objects only!)
	//! - heliocentric-velocity-kms: the planet heliocentric velocity in the solar system in km/s (for Solar system objects only!)
	//! - scale: scale factor for Solar system bodies (for Solar system objects only!)
	//! - eclipse-obscuration: value of obscuration for solar eclipse (for Sun only!)
	//! - eclipse-magnitude: value of magnitude for solar eclipse (for Sun only!)
	//! Other StelObject derivates, also those defined in plugins, may add more,
	//! these fields are documented in the respective classes, or simply try what you get:
	//! You can print a complete set of entries into output with the following commands:
	//! @code
	//! map=core.getSelectedObjectInfo();
	//! core.output(core.mapToString(map));
	//! @endcode
	static QVariantMap getObjectInfo(const QString& name);

	//! Fetch a map with data about the latest selected object's position, magnitude and so on
	//! @return a map of object data.  See description for getObjectInfo(const QString& name);
	static QVariantMap getSelectedObjectInfo();

	//! Add some arbitrary string to the object information of the currently selected object.
	//! if @arg replace==true, replace this extra string, if false, add to it.
	//! Note that while most objects keep this information after unselection and reselection,
	//! stars will start with no extra information when they become selected again.
	static void addToSelectedObjectInfoString(const QString &str, bool replace=false);

	//! Clear the display options, setting a "standard" view.
	//! Preset states:
	//! - natural : azimuthal mount, atmosphere, landscape,
	//!   no lines, labels or markers
	//! - starchart : equatorial mount, constellation lines,
	//!   no landscape, atmosphere etc.  labels & markers on.	
	//! - deepspace : like starchart, but no planets, no eq.grid, no markers, no lines.
	//! - galactic  : like deepspace, but in galactic coordinate system.
	//! - supergalactic  : like deepspace, but in supergalactic coordinate system.
	//! @param state the name of a preset state.
	static void clear(const QString& state="natural");

	//! Get the current viewing direction altitude angle at center of view.
	//! @return the altitude angle in decimal degrees.
	//! 0 is horizon, zenith is 180, nadir = -180.
	static double getViewAltitudeAngle();

	//! Get the current viewing direction azimuth angle at center of view.
	//! @return the azimuth angle in decimal degrees as a compass bearing
	//! i.e.  0 is North, 90 is East, 180 is South, 270 is West.
	static double getViewAzimuthAngle();

	//! Get the current viewing direction Right Ascension at center of view.
	//! @return the Right Ascension angle in decimal degrees.
	//! The value returned falls in the range 0 .. 360
	static double getViewRaAngle();

	//! Get the current viewing direction Declination angle at center of view.
	//! @return the Declination angle in decimal degrees.
	//! The value returned falls in the range -180 .. 180
	static double getViewDecAngle();

	//! Get the current viewing direction Right Ascension in J2000 frame at center of view.
	//! @return the Right Ascension angle in J2000 frame in decimal degrees.
	static double getViewRaJ2000Angle();

	//! Get the current viewing direction Declination angle in J2000 frame at center of view.
	//! @return the Declination angle in J2000 frame in decimal degrees.
	static double getViewDecJ2000Angle();

	//! Move the current viewing direction to some object.
	//! @param name is the English name of the object
	//! @param duration the duration of the movement in seconds
	static void moveToObject(const QString& name, float duration=1.);

	//! Move the current viewing direction to selected object.
	//! @param duration the duration of the movement in seconds
	static void moveToSelectedObject(float duration=1.);

	//! move the current viewing direction to some specified altitude and azimuth.
	//! The move will run in AltAz coordinates. This will look different from moveToRaDec() when timelapse is fast.
	//! angles may be specified in a format recognised by StelUtils::getDecAngle()
	//! @param alt the altitude angle
	//! @param azi the azimuth angle
	//! @param duration the duration of the movement in seconds
	static void moveToAltAzi(const QString& alt, const QString& azi, float duration=1.);

	//! move the current viewing direction to some specified right ascension and declination.
	//! The move will run in equatorial coordinates. This will look different from moveToAltAzi() when timelapse is fast.
	//! angles may be specified in a format recognised by StelUtils::getDecAngle()
	//! @param ra the right ascension angle
	//! @param dec the declination angle
	//! @param duration the duration of the movement in seconds
	static void moveToRaDec(const QString& ra, const QString& dec, float duration=1.);

	//! move the current viewing direction to some specified right ascension and declination in the J2000 frame
	//! angles may be specified in a format recognised by StelUtils::getDecAngle()
	//! @param ra the right ascension angle
	//! @param dec the declination angle
	//! @param duration the duration of the movement in seconds
	static void moveToRaDecJ2000(const QString& ra, const QString& dec, float duration=1.);

	//! move the current viewing direction to some specified galactic coordinates
	//! angles may be specified in a format recognised by StelUtils::getDecAngle()
	//! @param lon the galactic longitude
	//! @param dec the galactic latitude
	//! @param duration the duration of the movement in seconds
	static void moveToGalLongLat(const QString& lon, const QString& lat, float duration);

	//! Set the observer location
	//! @param longitude the longitude in degrees. E is +ve.
	//!        values out of the range -180..180 mean that
	//!        the longitude will not be set
	//! @param latitude the latitude in degrees. N is +ve.
	//!        values out of the range -90..90 mean that
	//!        the latitude will not be set
	//! @param altitude the new altitude in meters.
	//!        values less than -1000 mean the altitude will not
	//!        be set.
	//! @param duration the time for the transition from the
	//!        old to the new location.
	//! @param name A name for the location (which will appear
	//!        in the status bar. Use "<city>, <country>" for
	//!        moving across a border.
	//! @param planet the English name of the new planet.
	//!        If the planet name is not known (e.g. ""), the
	//!        planet will not be set.
	static void setObserverLocation(double longitude, double latitude, double altitude, double duration=1., const QString& name="", const QString& planet="");

	//! Set the location by the name of the location.
	//! @param id the location ID as it would be found in the database
	//! of locations - do a search in the Location window to see what
	//! where is.  e.g. "York, UnitedKingdom".
	//! @param duration the number of seconds to take to move location.
	static void setObserverLocation(const QString& id, double duration=1.);

	//! Get the ID of the current observer location.
	static QString getObserverLocation();

	//! Get the info of the current observer location.
	//! @return a map of object data.  Keys:
	//! - altitude : altitude in meters
	//! - longitude : longitude in decimal degrees
	//! - latitude : latitude in decimal degrees
	//! - planet : name of planet
	//! - location : city and country
	//! - sidereal-year : duration of the sidereal year on the planet in Earth's days (since 0.12.0)
	//! - sidereal-day : duration of the sidereal day on the planet in Earth's hours (since 0.12.0)
	//! - solar-day : duration of the mean solar day on the planet in Earth's hours (since 0.12.0)
	//! - local-sidereal-time : local sidereal time on the planet in hours (since 0.13.3)
	//! - local-sidereal-time-hms : local sidereal time on the planet in hours in HMS format (since 0.13.3)
	//! - timezone : IANA timezone or "LMST" (Local Mean Solar Time) or "LTST" (Local True Solar Time) (since 0.18.3)
	//! - location-timezone : IANA timezone of current location (as stored in location database) (since 0.18.3)
	static QVariantMap getObserverLocationInfo();

	//! set timezone name. This only changes the currently used timezone (in StelCore).
	//! Location (database) timezone will not be touched.
	//! Valid values for tz can be found from the results of getAllTimezoneNames, including:
	//! "LMST" = Local Mean Solar Time
	//! "LTST" = Local True Solar Time
	//! @param markAsCustom set flag/property StelCore.flagUseCTZ. (since 0.19.0)
	//! Acceptable values: 0: set off; 1: set on. Other values: don't touch.
	//! This parameter is mostly to reflect the respective setting in the Location dialog.
	//!  - If tz is the default timezone for a site, markAsCustom should be 0.
	//!  - For a definitively custom zone, set 1.
	//!  - If unsure, set to -1.
	static void setTimezone(QString tz, int markAsCustom=-1);

	//! Return an array of all timezone names valid for setTimezone(tzName)
	static QStringList getAllTimezoneNames();

	//! Save a screenshot.
	//! @param prefix the prefix for the file name to use
	//! @param dir the path of the directory to save the screenshot in.  If
	//! none is specified, the default screenshot directory will be used.
	//! @param invert whether colors have to be inverted in the output image
	//! @param overwrite true to use exactly the prefix as filename (plus .png), and overwrite any existing file.
	//! @param format File format. One of png|bmp|jpg|jpeg|tif|tiff|webm|pbm|pgm|ppm|xbm|xpm|ico. Use current format if left empty or invalid.
	static void screenshot(const QString& prefix, bool invert=false, const QString& dir="", const bool overwrite=false, const QString& format="");

	//! Show or hide the GUI (toolbars).  Note this only applies to GUI plugins which
	//! provide the public slot "setGuiVisible(bool)".
	//! @param b if true, show the GUI, if false, hide the GUI.
	static void setGuiVisible(bool b);

	//! Set the minimum frames per second.  Usually this minimum will
	//! be switched to after there are no user events for some seconds
	//! to save power.  However, if can be useful to set this to a high
	//! value to improve playing smoothness in scripts.
	//! @param m the new minimum fps setting.
	static void setMinFps(float m);

	//! Get the current minimum frames per second.
	//! @return The current minimum frames per second setting.
	static float getMinFps();

	//! Set the maximum frames per second.
	//! @param m the new maximum fps setting.
	static void setMaxFps(float m);

	//! Get the current maximum frames per second.
	//! @return The current maximum frames per second setting.
	static float getMaxFps();

	//! Get the mount mode as a string
	//! @return "equatorial" or "azimuthal"
	static QString getMountMode();

	//! Set the mount mode
	//! @param mode should be "equatorial" or "azimuthal"
	static void setMountMode(const QString& mode);

	//! Get the current status of Night Mode
	//! @return true if night mode is currently set
	static bool getNightMode();

	//! Set the status of Night Mode
	//! @param b if true, set Night Mode, else set Normal Mode
	static void setNightMode(bool b);

	//! Get the current projection mode ID string
	//! @return the string which identifies the current projection mode.
	//! For a list of possibl results, see setProjectionMode();
	static QString getProjectionMode();

	//! Set the current projection mode
	//! @param id the name of the projection mode to use, e.g. "ProjectionPerspective" and so on.
	//! valid values of id are:
	//! - ProjectionPerspective
	//! - ProjectionEqualArea
	//! - ProjectionStereographic
	//! - ProjectionFisheye
	//! - ProjectionHammer
	//! - ProjectionCylinder
	//! - ProjectionMercator
	//! - ProjectionOrthographic
	//! - ProjectionSinusoidal
	//! - ProjectionMiller
	void setProjectionMode(const QString& id);

	//! Get the status of the disk viewport
	//! @return true if the disk view port is currently enabled
	static bool getDiskViewport();

	//! Set the disk viewport
	//! @param b if true, sets the disk viewport on, else sets it off
	void setDiskViewport(bool b);

	//! Set the viewport distortion effect
	//! @param b if true, sets the spherical mirror distortion effect for viewport on, else sets it off
	static void setSphericMirror(bool b);

	//! Set a lateral width distortion. Use this e.g. in startup.ssc.
	//! Implemented for 0.15 for a setup with 5 projectors with edge blending. The 9600x1200 get squeezed somewhat which looks a bit odd. Use this stretch to compensate.
	//! Experimental! To avoid overuse, there is currently no config.ini setting available.
	//! @note Currently only the projected content is affected. ScreenImage, ScreenLabel is not stretched.
	static void setViewportStretch(const float stretch);

	//! Get a list of Sky Culture IDs
	//! @return a list of valid sky culture IDs
	static QStringList getAllSkyCultureIDs();

	//! Find out the current sky culture
	//! @return the ID of the current sky culture (i.e. the name of the directory in
	//! which the curret sky cultures files are found, e.g. "western")
	static QString getSkyCulture();

	//! Set the current sky culture
	//! @param id the ID of the sky culture to set, e.g. western or inuit etc.
	void setSkyCulture(const QString& id);

	//! Find out the current sky culture and get it English name
	//! @return the English name of the current sky culture
	static QString getSkyCultureName();

	//! Find out the current sky culture and get it localized name
	//! @return the translated name of the current sky culture
	static QString getSkyCultureNameI18n();

	//! Get the current status of the gravity labels option
	//! @return true if gravity labels are enabled, else false
	static bool getFlagGravityLabels();

	//! Get the current status of the horizontal flip
	//! @return true if horizontal flip are enabled, else false
	static bool getFlipHorz();
	//! Set the horizontal flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	static void setFlipHorz(bool b);

	//! Get the current status of the vertical flip
	//! @return true if vertical flip are enabled, else false
	static bool getFlipVert();
	//! Set the vertical flip status.
	//! @param flip The new value (true = flipped, false = unflipped).
	static void setFlipVert(bool b);

	//! Turn on/off gravity labels
	//! @param b if true, turn on gravity labels, else turn them off
	static void setFlagGravityLabels(bool b);

	//! Load an image into the sky background at the given sky coordinates and be warped with the sky.
	//! The image is projected like a deep-sky object, with a notion for surface magnitude of the brightest parts.
	//! Transparent sections in the image are possibly rendered white, so make your image just RGB with black background.
	//! The black background covers the milky way, but is brightened by the Zodiacal light.
	//! @todo allow alpha in images?
	//! @param id a string ID to be used when referring to this
	//! image (e.g. when changing the displayed status or deleting it.
	//! @param filename the file name of the image.  If a relative
	//! path is specified, "scripts/" will be prefixed before the
	//! image is searched for using StelFileMgr.
	//! @param lon0 The right ascension/longitude/azimuth of the first corner of the image in degrees (bottom left)
	//! @param lat0 The declination/latitude/altitude of the first corner of the image in degrees (bottom left)
	//! @param lon1 The right ascension/longitude/azimuth of the second corner of the image in degrees (bottom right)
	//! @param lat1 The declination/latitude/altitude of the second corner of the image in degrees (bottom right)
	//! @param lon2 The right ascension/longitude/azimuth of the third corner of the image in degrees (top right)
	//! @param lat2 The declination/latitude/altitude of the third corner of the image in degrees (top right)
	//! @param lon3 The right ascension/longitude/azimuth of the fourth corner of the image in degrees (top left)
	//! @param lat3 The declination/latitude/altitude of the fourth corner of the image in degrees (top left)
	//! @param minRes The minimum resolution setting for the image
	//! @param maxBright The maximum brightness setting for the image
	//! @param visible The initial visibility of the image
	//! @param frame one of EqJ2000|EqDate|EclJ2000|EclDate|Gal(actic)|SuperG(alactic)|AzAlt.
	//! @note since 2017-03, you can select Frame.
	//! @note Images in AzAlt frame are not affected by atmosphere effects like refraction or extinction.
	void loadSkyImage(const QString& id, const QString& filename,
					  double lon0, double lat0,
					  double lon1, double lat1,
					  double lon2, double lat2,
					  double lon3, double lat3,
					  double minRes=2.5, double maxBright=14, bool visible=true, const QString &frame="EqJ2000");


	//! Convenience function which allows the user to provide longitudinal and latitudinal angles (RA/Dec or Long/Lat or Az/Alt)
	//! as strings (e.g. "12d 14m 8s" or "5h 26m 8s" - formats accepted by StelUtils::getDecAngle()).
	void loadSkyImage(const QString& id, const QString& filename,
					  const QString& lon0, const QString& lat0,
					  const QString& lon1, const QString& lat1,
					  const QString& lon2, const QString& lat2,
					  const QString& lon3, const QString& lat3,
					  double minRes=2.5, double maxBright=14, bool visible=true, const QString& frame="EqJ2000");

	//! Convenience function which allows loading of a (square) sky image based on a
	//! central coordinate, angular size and rotation. Note that the edges will not be aligned with edges at center plus/minus size!
	//! @param id a string ID to be used when referring to this
	//! image (e.g. when changing the displayed status or deleting it.
	//! @param filename the file name of the image.  If a relative
	//! path is specified, "scripts/" will be prefixed before the
	//! image is searched for using StelFileMgr.
	//! @param lon The right ascension/longitude/azimuth of the center of the image in frame degrees
	//! @param lat The declination/latitude/altitude of the center of the image in frame degrees
	//! @param angSize The angular size of the image in arc minutes
	//! @param rotation The clockwise rotation angle of the image in degrees. Use 0 for an image with top=north. (New from 2017 -- This used to be 90!)
	//! @param minRes The minimum resolution setting for the image. UNCLEAR, using 2.5 seems to work well.
	//! @param maxBright The maximum brightness setting for the image, Vmag/arcmin^2. Use this to blend the brightest possible pixels with DSO. mag 15 or brighter seems ok.
	//! @param visible The initial visibility of the image
	//! @param frame one of EqJ2000|EqDate|EclJ2000|EclDate|Gal(actic)|SuperG(alactic)|AzAlt.
	//! @note since 2017-03, you can select Frame.
	//! @note Images in AzAlt frame are not affected by atmosphere effects like refraction or extinction.
	void loadSkyImage(const QString& id, const QString& filename,
					  double lon, double lat, double angSize, double rotation,
					  double minRes=2.5, double maxBright=14, bool visible=true, const QString& frame="EqJ2000");

	//! Convenience function which allows loading of a (square) sky image based on a
	//! central coordinate, angular size and rotation.  Parameters are the same
	//! as the version of this function which takes double values for the
	//! lon and lat, except here text expressions of angles may be used.
	void loadSkyImage(const QString& id, const QString& filename,
					  const QString& lon, const QString& lat, double angSize, double rotation,
					  double minRes=2.5, double maxBright=14, bool visible=true, const QString& frame="EqJ2000");

	//! Remove a SkyImage.
	//! @param id the ID of the image to remove.
	void removeSkyImage(const QString& id);

	//! Get screen coordinates from some specified altitude and azimuth
	//! angles may be specified in a format recognised by StelUtils::getDecAngle()
	//! @param alt the altitude angle [degrees]
	//! @param azi the azimuth angle [degrees]
	//! @return a map of object data.  Keys:
	//! - x : x coordinate on the screen
	//! - y : y coordinate on the screen
	static QVariantMap getScreenXYFromAltAzi(const QString& alt, const QString& azi);

	//! Load a sound from a file.
	//! @param filename the name of the file to load.
	//! @param id the identifier which will be used to refer to the sound
	//! when calling playSound, pauseSound, stopSound and dropSound.
	void loadSound(const QString& filename, const QString& id);

	//! Play a sound which has previously been loaded with loadSound
	//! @param id the identifier used when loadSound was called
	void playSound(const QString& id);

	//! Pause a sound which is playing.  Subsequent playSound calls will
	//! resume playing from the position in the file when it was paused.
	//! @param id the identifier used when loadSound was called
	void pauseSound(const QString& id);

	//! Stop a sound from playing.  This resets the position in the
	//! sound to the start so that subsequent playSound calls will
	//! start from the beginning.
	//! @param id the identifier used when loadSound was called
	void stopSound(const QString& id);

	//! Drop a sound from memory.  You should do this before the end
	//! of your script.
	//! @param id the identifier used when loadSound was called
	void dropSound(const QString& id);

	//! Get position in a playing sound.
	//! @param id the identifier used when loadSound was called
	//! @return position [ms] during play or pause, 0 when stopped, -1 in case of error.
	static qint64 getSoundPosition(const QString& id);

	//! Get duration of a sound object (if possible).
	//! @param id the identifier used when loadSound was called
	//! @return duration[ms] if known, 0 if unknown (e.g. during load/before playing), -1 in case of error.
	static qint64 getSoundDuration(const QString& id);

	//! Load a video from a file.
	//! @param filename the name of the file to load, relative to the scripts directory.
	//! @param id the identifier which will be used to refer to the video
	//! when calling playVideo(), pauseVideo(), stopVideo(), dropVideo() etc.
	//! @param x  the x-coordinate (pixels from left) for the video frame.
	//! @param y  the y-coordinate (pixels from top) for the video frame.
	//! @param show  the visibility state for the video. (Optional since V0.15)
	//! You should load a video with show=true (or leave default), to start it immediately in native size.
	//! Else set show=false, and then call resizeVideo(), playVideo() or use playVideoPopout().
	//! @param alpha the initial alpha value of the video, defaults to 1.
	//! @bug With Qt5/V0.15+, alpha does not work properly (no semitransparency), only alpha=0 makes it invisible.
	//! @bug With Qt5/V0.15+, show=false causes an assert failure (crash) on Windows. The responsible assert is not fired on release builds.
	void loadVideo(const QString& filename, const QString& id, float x, float y, bool show=true, float alpha=1.0f);

	//! Play a video which has previously been loaded with loadVideo
	//! @param id the identifier used when loadVideo was called
	void playVideo(const QString& id, bool keepVisibleAtEnd=false);

	//! Play a video which has previously been loaded with loadVideo with a complex effect.
	//! The video appears out of fromX/fromY,
	//! grows within popupDuration to size finalSizeX/finalSizeY, and
	//! shrinks back towards fromX/fromY at the end during popdownDuration.
	//! @param id the identifier used when loadVideo was called
	//! @param fromX X position of starting point, counted from left of window. May be absolute (if >1) or relative (0<X<1)
	//! @param fromY Y position of starting point, counted from top of window. May be absolute (if >1) or relative (0<Y<1)
	//! @param atCenterX X position of center of final video frame, counted from left of window. May be absolute (if >1) or relative (0<X<1)
	//! @param atCenterY Y position of center of final video frame, counted from top of window. May be absolute (if >1) or relative (0<Y<1)
	//! @param finalSizeX X size of final video frame. May be absolute (if >1) or relative to window size (0<X<1). If -1, scale proportional from finalSizeY.
	//! @param finalSizeY Y size of final video frame. May be absolute (if >1) or relative to window size (0<Y<1). If -1, scale proportional from finalSizeX.
	//! @param popupDuration duration of growing start transition (seconds)
	//! @param frozenInTransition true if video should be paused during growing/shrinking transition.
	void playVideoPopout(const QString& id, float fromX, float fromY, float atCenterX, float atCenterY, float finalSizeX, float finalSizeY, float popupDuration, bool frozenInTransition);

	//! Pause a video which is playing.  Subsequent playVideo() calls will
	//! resume playing from the position in the file when it was paused.
	//! @param id the identifier used when loadVideo() was called
	void pauseVideo(const QString& id);

	//! Stop a video from playing.  This resets the position in the
	//! video to the start so that subsequent playVideo() calls will
	//! start from the beginning.
	//! @param id the identifier used when loadVideo() was called
	void stopVideo(const QString& id);

	//! Drop a video from memory.  You should do this before the end
	//! of your script.
	//! @param id the identifier used when loadVideo() was called
	void dropVideo(const QString& id);

	//! Seeks a video to the requested time and either start playing or freeze there.
	//! @param id the identifier used when loadVideo() was called
	//! @param ms the time in milliseconds from the start of the media.
	//! @param pause true if you want to pause at the requested position, keep it false to play from here.
	void seekVideo(const QString& id, qint64 ms, bool pause=false);

	//! Sets the position of the video widget.
	//! @param id the identifier used when loadVideo() was called
	//! @param x the new x-coordinate for the video. (if <1, relative to main view size)
	//! @param y the new y-coordinate for the video. (if <1, relative to main view size)
	//! @param relative true if you want to move in relative coordinates, not set absolutely
	void setVideoXY(const QString& id, float x, float y, bool relative=false);

	//! Set the alpha value of a video when visible.
	//! @param id the identifier used when loadVideo() was called
	//! @param alpha the new alpha value to set.
	//! @bug With Qt5/V0.13+, @param alpha does not work properly, only @param alpha=0 makes it invisible.
	void setVideoAlpha(const QString& id, float alpha);

	//! Resize the video widget to the specified width, height.
	//! @param id the identifier used when loadVideo() was called
	//! @param w the new width for the widget. (if <1, relative to main view size)
	//! @param h the new height for the widget. (if <1, relative to main view size)
	void resizeVideo(const QString& id, float w, float h);

	//! Set the visibility state of a video.
	//! @param id the identifier used when loadVideo() was called
	//! @param show the new visible state of the video.
	//! @note You must call this if you called loadVideo() with its @param show=false, else video will be played hidden.
	void showVideo(const QString& id, bool show=true);

	//! Get the duration of a loaded video, or -1
	//! @param id the identifier used when loadVideo() was called
	static qint64 getVideoDuration(const QString& id);

	//! Get the current position of a loaded video, or -1
	//! @param id the identifier used when loadVideo() was called
	static qint64 getVideoPosition(const QString& id);

	//! Get the screen width in pixels.
	//! @return The screen width (actually, width of Stellarium main view) in pixels
	static int getScreenWidth();
	//! Get the screen height (actually, height of Stellarium main view) in pixels.
	//! @return The screen height in pixels
	static int getScreenHeight();

	//! Get the script execution rate as a multiple of normal execution speed
	//! @return the current script execution rate.
	static double getScriptRate();
	//! Set the script execution rate as a multiple of normal execution speed
	//! @param r the multiple of the normal script execution speed, i.e.
	//! if 5 is passed the script will execute 5 times faster than it would
	//! if the script rate was 1.
	static void setScriptRate(double r);

	//! Pause the currently running script. Note that you may need to use 
	//! a key sequence like 'Ctrl-D,R' or the GUI to resume script execution.
	static void pauseScript();

	//! Set the amount of selected object information to display
	//! @param level can be "AllInfo", "ShortInfo", "None"
	static void setSelectedObjectInfo(const QString& level);

	//! Stop the script
	void exit();

	//! Close Stellarium
	void quitStellarium();

	//! Return a QStringlist of all available properties. Useful for script development...
	static QStringList getPropertyList();

	//! print a debugging message to the console
	//! @param s the message to be displayed on the console.
	static void debug(const QString& s);

	//! print an output message from script
	//! @param s the message to be displayed on the output file.
	static void output(const QString& s);

	//! print contents of a QVariantMap as []-delimited list of [ "key" = <value>] lists.
	//! @param map QVariantMap e.g. from getObjectInfo() or getLocationInfo()
	//! @note string values are surrounded with ", simple numeric types are printed as themselves.
	//! @note More complicated value types like lists are only indicated by their type name. You must extract those (and their contents) yourself.
	static QString mapToString(const QVariantMap &map);

	//! Reset (clear) output file
	static void resetOutput(void);

	//! Save output file to new file (in same directory as output.txt).
	//! This is required to allow reading with other program on Windows while output.txt is still open.
	static void saveOutputAs(const QString &filename);

	//! Get the current application language.
	//! @return two letter language code, e.g. "en", or "de" and so on.
	static QString getAppLanguage();

	//! Set the current application language.
	//! @param langCode two letter language code, e.g. "en", or "de".
	static void setAppLanguage(QString langCode);

	//! Get the current sky language.
	//! @return two letter language code, e.g. "en", or "de" and so on.
	static QString getSkyLanguage();

	//! Set the current sky language.
	//! @param langCode two letter language code, e.g. "en", or "de".
	static void setSkyLanguage(QString langCode);

	//! Translate the string.
	static QString translate(QString englishText);

	//! Go to defaults position and direction of view
	void goHome();

	//! Returns the currently set Bortle scale index, which is used to simulate light pollution.
	//! Wrapper for StelSkyDrawer::getBortleScaleIndex
	//! @see https://en.wikipedia.org/wiki/Bortle_scale
	//! @return the Bortle scale index in range [1,9]
	static int getBortleScaleIndex();

	//! Changes the Bortle scale index, which is used to simulate light pollution.
	//! Wrapper for StelSkyDrawer::setBortleScaleIndex
	//! Valid values are in the range [1,9]
	//! @see https://en.wikipedia.org/wiki/Bortle_scale
	//! @param index the new Bortle scale index, must be in range [1,9]
	static void setBortleScaleIndex(int index);

	//! Apply refraction with current atmospheric parameters to altitude.
	//! @param altitude: degrees
	//! @param apparent: true to remove refraction from an apparent (observed) altitude
	static double refraction(double altitude, bool apparent=false);

	//! For use in setDate and waitFor
	//! For parameter descriptions see setDate().
	//! @return Julian day.
	static double jdFromDateString(const QString& dt, const QString& spec);

	// Methods wait() and waitFor() were added for documentation.
	// Details: https://bugs.launchpad.net/stellarium/+bug/1402200
	// re-implemented for 0.15.1 to avoid a busy-loop.
	//! Pauses the script for \e t seconds
	//! @param t the number of seconds to wait
	void wait(double t);

	//! Waits until a specified simulation date/time. This function
	//! will take into account the rate (and direction) in which simulation
	//! time is passing. e.g. if a future date is specified and the
	//! time is moving backwards, the function will return immediately.
	//! If the time rate is 0, the function will not wait.  This is to
	//! prevent infinite wait time.
	//! @param dt the date string to use, format like "2012-06-06T4:44:00" or "-1428-03-04T22:23:45"
	//! @param spec "local" or "utc"
	void waitFor(const QString& dt, const QString& spec="utc");

	//! Retrieve value of environment variable @param name.
	//! On desktop Windows and Qt before 5.10, this call may result in data loss if the original
	//! string contains Unicode characters not representable in the ANSI encoding.
	static QString getEnv(const QString& var);

	//! return whether a particular module has been loaded. Mostly useful to check whether a module available as plugin is active.
	//! @param moduleID the QObject name of the module instance, by convention it is equal to the class name.
	static bool isModuleLoaded(const QString& moduleID);

	//! @return The name of platform where running Stellarium. Keys:
	//! - FreeBSD
	//! - OpenBSD
	//! - NetBSD
	//! - Linux
	//! - Windows
	//! - macOS
	//! - Unknown
	static QString getPlatformName(void);

	//! Get the current status of media playback support.
	//! @return The current status of media playback support.
	static bool isMediaPlaybackSupported(void);

signals:
	void requestLoadSkyImage(const QString& id, const QString& filename,
							 double c1, double c2,
							 double c3, double c4,
							 double c5, double c6,
							 double c7, double c8,
							 double minRes, double maxBright, bool visible, const StelCore::FrameType frameType);

	void requestRemoveSkyImage(const QString& id);

	void requestLoadSound(const QString& filename, const QString& id);
	void requestPlaySound(const QString& id);
	void requestPauseSound(const QString& id);
	void requestStopSound(const QString& id);
	void requestDropSound(const QString& id);
	void requestLoadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha);
	void requestPlayVideo(const QString& id, const bool keepVisibleAtEnd);
	void requestPlayVideoPopout(const QString& id, float fromX, float fromY, float atCenterX, float atCenterY, float finalSizeX, float finalSizeY, float popupDuration, bool frozenInTransition);
	void requestPauseVideo(const QString& id);
	void requestStopVideo(const QString& id);
	void requestDropVideo(const QString& id);
	void requestSeekVideo(const QString& id, qint64 ms, bool pause=false);
	void requestSetVideoXY(const QString& id, float x, float y, bool relative=false);
	void requestSetVideoAlpha(const QString& id, float alpha);
	void requestResizeVideo(const QString& id, float w, float h);
	void requestShowVideo(const QString& id, bool show);
	
	void requestSetProjectionMode(QString id);
	void requestSetSkyCulture(QString id);
	void requestSetDiskViewport(bool b);
	void requestExit();
	void requestSetHomePosition();
};

#endif // STELMAINSCRIPTAPI_HPP

