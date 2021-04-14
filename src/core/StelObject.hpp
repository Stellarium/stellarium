/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef STELOBJECT_HPP
#define STELOBJECT_HPP

#include "VecMath.hpp"
#include "StelObjectType.hpp"
#include "StelRegionObject.hpp"

#include <QFlags>
#include <QString>

class StelCore;

//! The base abstract class for sky objects used in Stellarium like Stars, Planets, Constellations etc...
//! Normally you should use StelObjectP instead of StelObject* which have by default the same behaviour,
//! but which can be added reference counting if needed.
//! @sa StelObjectP
class StelObject : public StelRegionObject
{
	//Required for Q_FLAGS macro, this requires this header to be MOC'ed
	Q_GADGET
	Q_FLAGS(InfoStringGroupFlags InfoStringGroup)
public:
	//! Used as named bitfield flags as specifiers to
	//! filter results of getInfoString. The precise definition of these should
	//! be documented in the getInfoString documentation for the derived classes
	//! for all specifiers which are defined in that derivative.
	//! Use InfoStringGroup instead.
	enum InfoStringGroupFlags
	{
		None			= 0x00000000, //!< Show Nothing
		Name			= 0x00000001, //!< An object's name
		CatalogNumber		= 0x00000002, //!< Catalog numbers
		Magnitude		= 0x00000004, //!< Magnitude related data
		RaDecJ2000		= 0x00000008, //!< The equatorial position (J2000 ref)
		RaDecOfDate		= 0x00000010, //!< The equatorial position (of date)
		AltAzi			= 0x00000020, //!< The position (Altitude/Azimuth)
		Distance		= 0x00000040, //!< Info about an object's distance
		Elongation		= 0x00000080, //!< Info about elongation, phase angle etc. Most useful for Planets, but possible for all objects.
		Size			= 0x00000100, //!< Info about an object's size
		Velocity		= 0x00000200, //!< Info about object's velocity
		ProperMotion		= 0x00000400, //!< Annual proper motion (for stars) or hourly motion (for Planets)
		Extra			= 0x00000800, //!< Derived class-specific extra fields
		HourAngle		= 0x00001000, //!< The hour angle + DE (of date)
		AbsoluteMagnitude	= 0x00002000, //!< The absolute magnitude
		GalacticCoord		= 0x00004000, //!< The galactic position
		SupergalacticCoord	= 0x00008000, //!< The supergalactic position
		OtherCoord		= 0x00010000, //!< Unspecified additional coordinates. These can be "injected" into the extraInfoStrings by plugins.
		ObjectType		= 0x00020000, //!< The type of the object (star, planet, etc.)
		EclipticCoordJ2000	= 0x00040000, //!< The ecliptic position (J2000.0 ref) [+ XYZ of VSOP87A (used mainly for debugging, not public)]
		EclipticCoordOfDate	= 0x00080000, //!< The ecliptic position (of date)
		IAUConstellation        = 0x00100000, //!< Three-letter constellation code (And, Boo, Cas, ...)
		SiderealTime		= 0x00200000, //!< Mean and Apparent Sidereal Time
		RTSTime			= 0x00400000, //!< Time of rise, transit and set of celestial object
		Script                  = 0x00800000, //!< Should be used by Scripts only which can inject extraInfoStrings.
		DebugAid                = 0x01000000, //!< Should be used in DEBUG builds only, place messages into extraInfoStrings.
		NoFont			= 0x02000000,
		PlainText		= 0x04000000  //!< Strip HTML tags from output
	};
	Q_DECLARE_FLAGS(InfoStringGroup, InfoStringGroupFlags)

	//! A pre-defined set of specifiers for the getInfoString flags argument to getInfoString
	static const InfoStringGroupFlags AllInfo = static_cast<InfoStringGroupFlags>(Name|CatalogNumber|Magnitude|RaDecJ2000|RaDecOfDate|AltAzi|
									   Distance|Elongation|Size|Velocity|ProperMotion|Extra|HourAngle|AbsoluteMagnitude|
									   GalacticCoord|SupergalacticCoord|OtherCoord|ObjectType|EclipticCoordJ2000|
									   EclipticCoordOfDate|IAUConstellation|SiderealTime|RTSTime);
	//! A pre-defined set of specifiers for the getInfoString flags argument to getInfoString
	static const InfoStringGroupFlags ShortInfo = static_cast<InfoStringGroupFlags>(Name|CatalogNumber|Magnitude|RaDecJ2000);

	virtual ~StelObject() {}

	//! Default implementation of the getRegion method.
	//! Return the spatial region of the object.
	virtual SphericalRegionP getRegion() const {return SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(Q_NULLPTR)));}

	//! Default implementation of the getPointInRegion method.
	//! Return the J2000 Equatorial Position of the object.
	virtual Vec3d getPointInRegion() const {return getJ2000EquatorialPos(Q_NULLPTR);}
	
	//! Write I18n information about the object in QString.
	//! @param core the StelCore object to use
	//! @param flags a set of InfoStringGroup flags which are used to
	//! filter the return value - including specified types of information
	//! and altering the output format.
	//! @return an HTML string containing information about the StelObject.
	virtual QString getInfoString(const StelCore *core, const InfoStringGroup& flags=StelObject::AllInfo) const = 0;

	//! Return a key/value map with data about an object's position, magnitude and so on. Useful in a context like scripting.
	//! Derived objects can add their own special information tags.
	//! @param core the current StelCore
	//! @return a map of object data.  Keys:
	//! - above-horizon : true, if celestial body is above horizon
	//! - altitude : apparent altitude angle in decimal degrees
	//! - azimuth : apparent azimuth angle in decimal degrees
	//! - altitude-geometric : geometric altitude angle in decimal degrees
	//! - azimuth-geometric : geometric azimuth angle in decimal degrees
	//! - airmass : number of airmasses the object's light had to pass through the atmosphere. For negative altitudes this number may be meaningless.
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
	//! - vmage : visual magnitude (after atmospheric extinction)
	//! - size: angular size in radians
	//! - size-dd : angular size in decimal degrees
	//! - size-deg : angular size in decimal degrees (formatted string)
	//! - size-dms : angular size in DMS format
	//! - rise : time of rise in HM format
	//! - rise-dhr : time of rise in decimal hours
	//! - transit : time of transit in HM format
	//! - transit-dhr : time of transit in decimal hours
	//! - set : time of set in HM format
	//! - set-dhr : time of set in decimal hours
	//! - name : english name of the object
	//! - localized-name : localized name	
	//! @note Coordinate values may need modulo operation to bring them into ranges [0..360].
	virtual QVariantMap getInfoMap(const StelCore *core) const;

	//! Return object's type. It should be the name of the class.
	virtual QString getType() const = 0;

	//! Returns a unique identifier for this object.
	//! The ID should be unique for all objects of the same type,
	//! but may freely conflict with IDs of other types, so getType() must also be tested.
	//!
	//! With this it should be possible to at least identify the same object
	//! in a different instance of Stellarium running the same version, but
	//! it would even be better if the ID provides some degree of forward-compatibility.
	//! For some object types (e.g. planets) this may simply return getEnglishName(),
	//! but better candidates may be official designations or at least (stable) internal IDs.
	//!
	//! An object may have multiple IDs (different catalog numbers, etc). StelObjectMgr::searchByID()
	//! should search through all ID variants, but this method only returns one of them.
	virtual QString getID() const = 0;

	//! Return object's name in english
	virtual QString getEnglishName() const = 0;

	//! Return translated object's name
	virtual QString getNameI18n() const = 0;

	//! Get observer-centered equatorial coordinates at equinox J2000
	virtual Vec3d getJ2000EquatorialPos(const StelCore* core) const = 0;

	//! Get observer-centered equatorial coordinate at the current equinox
	//! The frame has its Z axis at the planet's current rotation axis
	//! At time 2000-01-01 this frame is almost the same as J2000, but ONLY if the observer is on earth
	Vec3d getEquinoxEquatorialPos(const StelCore* core) const;
	//! Like getEquinoxEquatorialPos(core), but always adds refraction correction to the position.
	Vec3d getEquinoxEquatorialPosApparent(const StelCore* core) const;
	//! Like getEquinoxEquatorialPos(core), but adds refraction correction to the position if atmosphere is active.
	Vec3d getEquinoxEquatorialPosAuto(const StelCore* core) const;

	//! Get observer-centered galactic coordinates
	Vec3d getGalacticPos(const StelCore* core) const;

	//! Get observer-centered supergalactic coordinates
	Vec3d getSupergalacticPos(const StelCore* core) const;

	//! Get observer-centered hour angle + declination (at current equinox)
	//! It is the geometric position, i.e. without taking refraction effect into account.
	//! The frame has its Z axis at the planet's current rotation axis
	Vec3d getSiderealPosGeometric(const StelCore* core) const;

	//! Get observer-centered hour angle + declination (at current equinox)
	//! It is the apparent position, i.e. taking the refraction effect into account.
	//! The frame has its Z axis at the planet's current rotation axis
	Vec3d getSiderealPosApparent(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the geometric position, i.e. without taking refraction effect into account.
	//! The frame has its Z axis at the zenith
	Vec3d getAltAzPosGeometric(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the apparent position, i.e. taking the refraction effect into account.
	//! The frame has its Z axis at the zenith
	Vec3d getAltAzPosApparent(const StelCore* core) const;

	//! Get observer-centered alt/az position
	//! It is the automatic position, i.e. taking the refraction effect into account if atmosphere is on.
	//! The frame has its Z axis at the zenith
	Vec3d getAltAzPosAuto(const StelCore* core) const;

	//! Get parallactic angle, which is the deviation between zenith angle and north angle. [radians]
	float getParallacticAngle(const StelCore* core) const;

	//! Checking position an object above mathematical horizon for current location.
	//! @return true if object an above mathematical horizon
	bool isAboveHorizon(const StelCore* core) const;

	//! Checking position an object above real horizon for current location.
	//! @return true if object an above real horizon (uses test for landscapes)
	bool isAboveRealHorizon(const StelCore* core) const;

	//! Get today's time of rise, transit and set for celestial object for current location.
	//! @return Vec3f - time of rise, transit and set; decimal hours
	//! @note The value -1.f is used as undefined value
	Vec3f getRTSTime(StelCore *core) const;

	//! Return object's apparent V magnitude as seen from observer, without including extinction.
	virtual float getVMagnitude(const StelCore* core) const;
	
	//! Return object's apparent V magnitude as seen from observer including extinction.
	//! Extinction obviously only if atmosphere=on.
	float getVMagnitudeWithExtinction(const StelCore* core) const;

	//! Return a priority value which is used to discriminate objects by priority
	//! As for magnitudes, the lower is the higher priority
	virtual float getSelectPriority(const StelCore*) const;

	//! Get a color used to display info about the object
	virtual Vec3f getInfoColor() const {return Vec3f(1.f,1.f,1.f);}

	//! Return the best FOV in degree to use for a close view of the object
	virtual double getCloseViewFov(const StelCore*) const {return 10.;}

	//! Return the best FOV in degree to use for a global view of the object satellite system (if there are satellites)
	virtual double getSatellitesFov(const StelCore*) const {return -1.;}
	virtual double getParentSatellitesFov(const StelCore*) const {return -1.;}

	//! Return the angular radius of a circle containing the object as seen from the observer
	//! with the circle center assumed to be at getJ2000EquatorialPos().
	//! @return radius in degree. This value is the apparent angular size of the object, and is independent of the current FOV.
	virtual double getAngularSize(const StelCore* core) const = 0;

	//! Return airmass value for the object (for atmosphere-dependent calculations)
	//! @param core
	//! @return airmass value or -1.f if calculations are not applicable or meaningless
	virtual float getAirmass(const StelCore *core) const;

public slots:
	//! Allow additions to the Info String. Can be used by plugins to show extra info for the selected object, or for debugging.
	//! Hard-set this string group to a single str, or delete all messages when str==""
	//! @note This should be used with caution. Usually you want to use addToExtraInfoString().
	virtual void setExtraInfoString(const InfoStringGroup& flags, const QString &str);
	//! Add str to the extra string. This should be preferrable over hard setting.
	//! Can be used by plugins to show extra info for the selected object, or for debugging.
	//! The strings will be shown in the InfoString for the selected object, below the default fields per-flag.
	//! Additional coordinates not fitting into one of the predefined coordinate sets should be flagged with OtherCoords,
	//! and must be adapted to table or non-table layout as required.
	//! The line ending must be given explicitly, usually just end a line with "<br/>", except when it may end up in a Table or appended to a line.
	//! See getCommonInfoString() or the respective getInfoString() in the subclasses for details of use.
	virtual void addToExtraInfoString(const StelObject::InfoStringGroup& flags, const QString &str);
	//! Retrieve an (unsorted) QStringList of all extra info strings that match flags.
	//! Normally the order matches the order of addition, but this cannot be guaranteed.
	QStringList getExtraInfoStrings(const InfoStringGroup& flags) const;
	//! Remove the extraInfoStrings with the given flags.
	//! This is a finer-grained removal than just extraInfoStrings.remove(flags), as it allows a combination of flags.
	//! After display, InfoPanel::setTextFromObjects() auto-clears the strings of the selected object using the AllInfo constant.
	//! extraInfoStrings having been set with the DebugAid and Script flags have to be removed by separate calls of this method.
	//! Those which have been set by scripts have to persist at least as long as the selection remains active.
	//! The behaviour of DebugAid texts depends on the use case.
	void removeExtraInfoStrings(const InfoStringGroup& flags);

protected:
	//! Format the positional info string containing J2000/of date/altaz/hour angle positions and constellation, sidereal time, etc. for the object
	//! FIXME: We should split this and provide shorter virtual methods for various parts of the InfoString.
	//! The ExtraInfoStrings should be placed per flag, where they best fit.
	QString getCommonInfoString(const StelCore *core, const InfoStringGroup& flags) const;

	//! Format the magnitude info string for the object
	//! @param core
	//! @param flags
	//! @param decimals significant digits after the comma.
	virtual QString getMagnitudeInfoString(const StelCore *core, const InfoStringGroup& flags, const int decimals=1) const;

	//! Apply post processing on the info string.
	//! This also removes all extraInfoStrings possibly injected by modules (plugins) etc., except for Script and DebugAid types.
	void postProcessInfoString(QString& str, const InfoStringGroup& flags) const;
private:
	//! Compute time of rise, transit and set for celestial object for current location.
	//! @return Vec3f - time of rise, transit and set; decimal hours
	//! @note The value -1.f is used as undefined value
	Vec3f computeRTSTime(StelCore* core) const;

	//! Location for additional object info that can be set for special purposes (at least for debugging, but maybe others), even via scripting.
	//! Modules are allowed to add new strings to be displayed in the various getInfoString() methods of subclasses.
	//! This helps avoiding screen collisions if a plugin wants to display some additional object information.
	//! This string map gets cleared by InfoPanel::setTextFromObjects(), with the exception of strings with Script or DebugAid flags,
	//! which have been injected by scripts or for debugging (take care of those yourself!).
	QMultiMap<InfoStringGroup, QString> extraInfoStrings;

	static int stelObjectPMetaTypeID;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(StelObject::InfoStringGroup)

#endif // STELOBJECT_HPP
